/*
 *  HPUGens.cpp
 *  PluginsHP
 *
 *  Created by H P on 08.06.08
 *
 */

#ifdef NOVA_SIMD
#include "simd_memory.hpp"
#endif

#include "SC_PlugIn.h"
#include <cstdio>
#include <algorithm>
#include <cmath>


using namespace std; // for math functions

static InterfaceTable *ft;


// Corrected two-point discontinuity smoother.
// Keeps the original musical behaviour: when the jump exceeds threshold,
// move from the previous output toward the current sample by sensitivity.
// Unlike the former implementation, sensitivity is independent of playback rate.
static inline double hpSmoothDiscontinuity2(
    double previous,
    double current,
    double threshold,
    double sensitivity)
{
    if (!std::isfinite(previous)) previous = 0.0;
    if (!std::isfinite(current)) current = previous;

    threshold = std::max(0.0, threshold);
    sensitivity = std::max(0.0, std::min(1.0, sensitivity));

    const double difference = current - previous;

    // A threshold <= 0 disables the correction, preserving the existing API.
    if (threshold <= 0.0 || std::fabs(difference) < threshold)
        return current;

    return previous + difference * sensitivity;
}

struct HPplayBuf : public Unit
{
	double m_phase;
	float m_prevtrig;
	float m_fbufnum;
    float m_failedBufNum;
	double prevData[16];
	SndBuf *m_buf;
};

const int kMaxGrains = 64;

struct Grain
{
	double phase, rate;
	double b1, y1, y2; // envelope
	float pan1, pan2;
	int counter;
	int bufnum;
	int chan;
	int interp;
};

struct HPtGrains : public Unit
{
	float mPrevTrig;
	int mNumActive;
	Grain mGrains[kMaxGrains];
	double prevDatas[kMaxGrains];
	double prevData;
};

struct HPbufRd : public Unit
{
	float m_fbufnum;
    float m_failedBufNum;
	double prevData[16];
	SndBuf *m_buf;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{
	void load(InterfaceTable *inTable);
	
	void HPplayBuf_next_aa(HPplayBuf *unit, int inNumSamples);
	void HPplayBuf_next_ak(HPplayBuf *unit, int inNumSamples);
	void HPplayBuf_next_ka(HPplayBuf *unit, int inNumSamples);
	void HPplayBuf_next_kk(HPplayBuf *unit, int inNumSamples);
	void HPplayBuf_Ctor(HPplayBuf* unit);
	
	void HPtGrains_next(HPtGrains *unit, int inNumSamples);
	void HPtGrains_Ctor(HPtGrains* unit);
	
	void HPbufRd_Ctor(HPbufRd *unit);
	void HPbufRd_next_4(HPbufRd *unit, int inNumSamples);
	void HPbufRd_next_2(HPbufRd *unit, int inNumSamples);
	void HPbufRd_next_1(HPbufRd *unit, int inNumSamples);
}


//////////////////////////////////////////////////////////////////////////////////////////////////

inline double sc_loop(Unit *unit, double in, double hi, int loop)
{
    // avoid the divide if possible
    if (in >= hi) {
        if (!loop) {
            unit->mDone = true;
            return hi;
        }
        in -= hi;
        if (in < hi) return in;
    } else if (in < 0.) {
        if (!loop) {
            unit->mDone = true;
            return 0.;
        }
        in += hi;
        if (in >= 0.) return in;
    } else return in;
    
    return in - hi * floor(in/hi);
}

#define CHECK_BUF \
if (!bufData) { \
unit->mDone = true; \
ClearUnitOutputs(unit, inNumSamples); \
return; \
}

#define SETUP_IN(offset) \
uint32 numInputs = unit->mNumInputs - (uint32)offset; \
if (numInputs != bufChannels) { \
if(unit->mWorld->mVerbosity > -1 && !unit->mDone){ \
Print("buffer-writing UGen channel mismatch: numInputs %i, yet buffer has %i channels\n", numInputs, bufChannels); \
} \
unit->mDone = true; \
ClearUnitOutputs(unit, inNumSamples); \
return; \
} \
if(!unit->mIn){ \
unit->mIn = (float**)RTAlloc(unit->mWorld, numInputs * sizeof(float*)); \
if (unit->mIn == NULL) { \
unit->mDone = true; \
ClearUnitOutputs(unit, inNumSamples); \
return; \
} \
} \
float **in = unit->mIn; \
for (uint32 i=0; i<numInputs; ++i) { \
in[i] = ZIN(i+offset); \
}

#define TAKEDOWN_IN \
if(unit->mIn){ \
RTFree(unit->mWorld, unit->mIn); \
}

#define LOOP_INNER_BODY_1(SAMPLE_INDEX) \
OUT(channel)[SAMPLE_INDEX] = table1[index]; \
\
OUT(channel)[SAMPLE_INDEX] = (float)hpSmoothDiscontinuity2( \
    unit->prevData[index], OUT(channel)[SAMPLE_INDEX], newSeuil, newSensibilite); \
unit->prevData[index] = OUT(channel)[SAMPLE_INDEX]; \

#define LOOP_INNER_BODY_2(SAMPLE_INDEX) \
float b = table1[index]; \
float c = table2[index]; \
OUT(channel)[SAMPLE_INDEX] = b + fracphase * (c - b); \
\
OUT(channel)[SAMPLE_INDEX] = (float)hpSmoothDiscontinuity2( \
    unit->prevData[index], OUT(channel)[SAMPLE_INDEX], newSeuil, newSensibilite); \
unit->prevData[index] = OUT(channel)[SAMPLE_INDEX]; \

#define LOOP_INNER_BODY_4(SAMPLE_INDEX) \
float a = table0[index]; \
float b = table1[index]; \
float c = table2[index]; \
float d = table3[index]; \
OUT(channel)[SAMPLE_INDEX] = cubicinterp(fracphase, a, b, c, d); \
\
OUT(channel)[SAMPLE_INDEX] = (float)hpSmoothDiscontinuity2( \
    unit->prevData[index], OUT(channel)[SAMPLE_INDEX], newSeuil, newSensibilite); \
unit->prevData[index] = OUT(channel)[SAMPLE_INDEX]; \

#define LOOP_BODY_4(SAMPLE_INDEX) \
phase = sc_loop((Unit*)unit, phase, loopMax, loop); \
int32 iphase = (int32)phase; \
const float* table1 = bufData + iphase * bufChannels; \
const float* table0 = table1 - bufChannels; \
const float* table2 = table1 + bufChannels; \
const float* table3 = table2 + bufChannels; \
if (iphase == 0) { \
if (loop) { \
table0 += bufSamples; \
} else { \
table0 += bufChannels; \
} \
} else if (iphase >= guardFrame) { \
if (iphase == guardFrame) { \
if (loop) { \
table3 -= bufSamples; \
} else { \
table3 -= bufChannels; \
} \
} else { \
if (loop) { \
table2 -= bufSamples; \
table3 -= bufSamples; \
} else { \
table2 -= bufChannels; \
table3 -= 2 * bufChannels; \
} \
} \
} \
int32 index = 0; \
float fracphase = phase - (double)iphase; \
if(numOutputs == bufChannels) { \
for (uint32 channel=0; channel<numOutputs; ++channel) { \
LOOP_INNER_BODY_4(SAMPLE_INDEX) \
index++; \
} \
} else if (numOutputs < bufChannels) { \
for (uint32 channel=0; channel<numOutputs; ++channel) { \
LOOP_INNER_BODY_4(SAMPLE_INDEX) \
index++; \
} \
index += (bufChannels - numOutputs); \
} else { \
for (uint32 channel=0; channel<bufChannels; ++channel) { \
LOOP_INNER_BODY_4(SAMPLE_INDEX) \
index++; \
} \
for (uint32 channel=bufChannels; channel<numOutputs; ++channel) { \
OUT(channel)[SAMPLE_INDEX] = 0.f; \
index++; \
} \
} \


#define LOOP_BODY_2(SAMPLE_INDEX) \
phase = sc_loop((Unit*)unit, phase, loopMax, loop); \
int32 iphase = (int32)phase; \
const float* table1 = bufData + iphase * bufChannels; \
const float* table2 = table1 + bufChannels; \
if (iphase > guardFrame) { \
if (loop) { \
table2 -= bufSamples; \
} else { \
table2 -= bufChannels; \
} \
} \
int32 index = 0; \
float fracphase = phase - (double)iphase; \
if(numOutputs == bufChannels) { \
for (uint32 channel=0; channel<numOutputs; ++channel) { \
LOOP_INNER_BODY_2(SAMPLE_INDEX) \
index++; \
} \
} else if (numOutputs < bufChannels) { \
for (uint32 channel=0; channel<numOutputs; ++channel) { \
LOOP_INNER_BODY_2(SAMPLE_INDEX) \
index++; \
} \
index += (bufChannels - numOutputs); \
} else { \
for (uint32 channel=0; channel<bufChannels; ++channel) { \
LOOP_INNER_BODY_2(SAMPLE_INDEX) \
index++; \
} \
for (uint32 channel=bufChannels; channel<numOutputs; ++channel) { \
OUT(channel)[SAMPLE_INDEX] = 0.f; \
index++; \
} \
} \


#define LOOP_BODY_1(SAMPLE_INDEX) \
phase = sc_loop((Unit*)unit, phase, loopMax, loop); \
int32 iphase = (int32)phase; \
const float* table1 = bufData + iphase * bufChannels; \
int32 index = 0; \
if(numOutputs == bufChannels) { \
for (uint32 channel=0; channel<numOutputs; ++channel) { \
LOOP_INNER_BODY_1(SAMPLE_INDEX) \
index++; \
} \
} else if (numOutputs < bufChannels) { \
for (uint32 channel=0; channel<numOutputs; ++channel) { \
LOOP_INNER_BODY_1(SAMPLE_INDEX) \
index++; \
} \
index += (bufChannels - numOutputs); \
} else { \
for (uint32 channel=0; channel<bufChannels; ++channel) { \
LOOP_INNER_BODY_1(SAMPLE_INDEX) \
index++; \
} \
for (uint32 channel=bufChannels; channel<numOutputs; ++channel) { \
OUT(channel)[SAMPLE_INDEX] = 0.f; \
index++; \
} \
} \



#define CHECK_BUFFER_DATA \
if (!bufData) { \
if(unit->mWorld->mVerbosity > -1 && !unit->mDone && (unit->m_failedBufNum != fbufnum)) { \
Print("Buffer UGen: no buffer data\n"); \
unit->m_failedBufNum = fbufnum; \
} \
ClearUnitOutputs(unit, inNumSamples); \
return; \
} else { \
if (bufChannels != numOutputs) { \
if(unit->mWorld->mVerbosity > -1 && !unit->mDone && (unit->m_failedBufNum != fbufnum)) { \
Print("Buffer UGen channel mismatch: expected %i, yet buffer has %i channels\n", \
numOutputs, bufChannels); \
unit->m_failedBufNum = fbufnum; \
} \
} \
} \


void HPplayBuf_Ctor(HPplayBuf *unit)
{
	if (INRATE(1) == calc_FullRate) {
		if (INRATE(2) == calc_FullRate) {
			SETCALC(HPplayBuf_next_aa);
		} else {
			SETCALC(HPplayBuf_next_ak);
		}
	} else {
		if (INRATE(2) == calc_FullRate) {
			SETCALC(HPplayBuf_next_ka);
		} else {
			SETCALC(HPplayBuf_next_kk);
		}
	}
	
	unit->m_fbufnum = -1e9f;
	unit->m_prevtrig = 0.;
	unit->m_phase = ZIN0(3);
	for(uint32 i=0; i<16; ++i)
	{
		unit->prevData[i] = 0.0;
	}
	
	ClearUnitOutputs(unit, 1);
}

void HPplayBuf_next_aa(HPplayBuf *unit, int inNumSamples)
{
	float *ratein  = ZIN(1);
	float *trigin  = ZIN(2);
	int32 loop     = (int32)ZIN0(4);
	double seuil    = ZIN0(5);
	double sensibilite = ZIN0(6);
	
	
	float fbufnum  = ZIN0(0);
	if (fbufnum != unit->m_fbufnum) {
		uint32 bufnum = (int)fbufnum;
		World *world = unit->mWorld;
		if (bufnum >= world->mNumSndBufs) bufnum = 0;
		unit->m_fbufnum = fbufnum;
		unit->m_buf = world->mSndBufs + bufnum;
	}
    const SndBuf *buf = unit->m_buf;
    ACQUIRE_SNDBUF_SHARED(buf);
    const float *bufData __attribute__((__unused__)) = buf->data;
    uint32 bufChannels __attribute__((__unused__)) = buf->channels;
    uint32 bufSamples __attribute__((__unused__)) = buf->samples;
    uint32 bufFrames = buf->frames;
    int mask __attribute__((__unused__)) = buf->mask;
    int guardFrame __attribute__((__unused__)) = bufFrames - 2;
    
    int numOutputs = unit->mNumOutputs;
    
    CHECK_BUFFER_DATA;
    
    double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
    double phase = unit->m_phase;
    float prevtrig = unit->m_prevtrig;
	
	for (int i=0; i<inNumSamples; ++i) {
		
		double newRate = fabs(ZXP(ratein));
		if (newRate > 8.0) newRate = 8.0;
		double newSeuil = seuil * newRate;
		double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
		if (newSeuil > 2.0) newSeuil = 2.0;
		
		
		
		float trig = ZXP(trigin);
		if (trig > 0.f && prevtrig <= 0.f) {
			unit->mDone = false;
			phase = ZIN0(3);
		}
		prevtrig = trig;
		
		LOOP_BODY_4(i)
		
		phase += ZXP(ratein);
	}
    RELEASE_SNDBUF_SHARED(buf);
    
    if(unit->mDone)
        DoneAction((int)ZIN0(5), unit);
    unit->m_phase = phase;
    unit->m_prevtrig = prevtrig;
}

void HPplayBuf_next_ak(HPplayBuf *unit, int inNumSamples)
{
	float *ratein  = ZIN(1);
	float trig     = ZIN0(2);
	int32 loop     = (int32)ZIN0(4);
	double seuil    = ZIN0(5);
	double sensibilite = ZIN0(6);
	
	
    float fbufnum  = ZIN0(0);
    if (fbufnum != unit->m_fbufnum) {
        uint32 bufnum = (int)fbufnum;
        World *world = unit->mWorld;
        if (bufnum >= world->mNumSndBufs) bufnum = 0;
        unit->m_fbufnum = fbufnum;
        unit->m_buf = world->mSndBufs + bufnum;
    }
    const SndBuf *buf = unit->m_buf;
    ACQUIRE_SNDBUF_SHARED(buf);
    const float *bufData __attribute__((__unused__)) = buf->data;
    uint32 bufChannels __attribute__((__unused__)) = buf->channels;
    uint32 bufSamples __attribute__((__unused__)) = buf->samples;
    uint32 bufFrames = buf->frames;
    int mask __attribute__((__unused__)) = buf->mask;
    int guardFrame __attribute__((__unused__)) = bufFrames - 2;
    
    int numOutputs = unit->mNumOutputs;
    
    CHECK_BUFFER_DATA
    
    double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
    double phase = unit->m_phase;
    if(phase == -1.) phase = bufFrames;
    if (trig > 0.f && unit->m_prevtrig <= 0.f) {
        unit->mDone = false;
        phase = ZIN0(3);
    }
    unit->m_prevtrig = trig;
	for (int i=0; i<inNumSamples; ++i) {
		
		double newRate = fabs(ZXP(ratein));
		if (newRate > 8.0) newRate = 8.0;
		double newSeuil = seuil * newRate;
		double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
		if (newSeuil > 2.0) newSeuil = 2.0;
		
		
		
		LOOP_BODY_4(i)
		
		phase += ZXP(ratein);
	}
    RELEASE_SNDBUF_SHARED(buf);
    if(unit->mDone)
        DoneAction((int)ZIN0(5), unit);
    unit->m_phase = phase;
}

void HPplayBuf_next_kk(HPplayBuf *unit, int inNumSamples)
{
	float rate     = ZIN0(1);
	float trig     = ZIN0(2);
	int32 loop     = (int32)ZIN0(4);
	double seuil    = ZIN0(5);
	double sensibilite = ZIN0(6);
	
	
    GET_BUF_SHARED
    int numOutputs = unit->mNumOutputs;
    
    CHECK_BUFFER_DATA
    
    double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
    double phase = unit->m_phase;
    if (trig > 0.f && unit->m_prevtrig <= 0.f) {
        unit->mDone = false;
        phase = ZIN0(3);
    }
    unit->m_prevtrig = trig;
    
	double newRate = fabs(rate);
	double newSeuil = seuil * newRate;
	double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
	if (newSeuil > 2.0) newSeuil = 2.0;
	
	
	
	for (int i=0; i<inNumSamples; ++i) {
		
		LOOP_BODY_4(i)
		
		phase += rate;
	}
    if(unit->mDone)
        DoneAction((int)ZIN0(5), unit);
    unit->m_phase = phase;
}

void HPplayBuf_next_ka(HPplayBuf *unit, int inNumSamples)
{
	float rate     = ZIN0(1);
	float *trigin  = ZIN(2);
	int32 loop     = (int32)ZIN0(4);
	double seuil    = ZIN0(5);
	double sensibilite = ZIN0(6);
	
	
    GET_BUF_SHARED
    int numOutputs = unit->mNumOutputs;
    
    CHECK_BUFFER_DATA
    
    double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
    double phase = unit->m_phase;
    float prevtrig = unit->m_prevtrig;
	
	float newRate = fabs(rate);
	float newSeuil = seuil * newRate;
	double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
	if (newSeuil > 2.0) newSeuil = 2.0;
	
	
	
	
	for (int i=0; i<inNumSamples; ++i) {
		float trig = ZXP(trigin);
		if (trig > 0.f && prevtrig <= 0.f) {
			unit->mDone = false;
			if (INRATE(3) == calc_FullRate) phase = IN(3)[i];
			else phase = ZIN0(3);
		}
		prevtrig = trig;
		
		LOOP_BODY_4(i)
		
		phase += rate;
	}
    if(unit->mDone)
        DoneAction((int)ZIN0(5), unit);
    unit->m_phase = phase;
    unit->m_prevtrig = prevtrig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void HPbufRd_Ctor(HPbufRd *unit)
{	
	int interp = (int)ZIN0(6);
	switch (interp) {
		case 1 : SETCALC(HPbufRd_next_1); break;
		case 2 : SETCALC(HPbufRd_next_2); break;
		default : SETCALC(HPbufRd_next_4); break;
	}
	
	unit->m_fbufnum = -1e9f;
    unit->m_failedBufNum = -1e9f;
	for(uint32 i=0; i<16; ++i)
	{
		unit->prevData[i] = 0.0;
	}
	
		HPbufRd_next_1(unit, 1);
}

void HPbufRd_next_4(HPbufRd *unit, int inNumSamples)
{
	float *phasein = ZIN(1);
	float *ratein  = ZIN(2);
	int32 loop     = (int32)ZIN0(2);
	double seuil    = ZIN0(4);
	double sensibilite = ZIN0(5);
	
    GET_BUF_SHARED
    uint32 numOutputs = unit->mNumOutputs;
    
    CHECK_BUFFER_DATA
    
    double loopMax = (double)(loop ? bufFrames : bufFrames - 1);
    
    double newRate = fabs(ZXP(ratein));
	if (newRate > 8.0) newRate = 8.0;
	double newSeuil = seuil * newRate;
	double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
	if (newSeuil > 2.0) newSeuil = 2.0;
	
	
	
	
	for (int i=0; i<inNumSamples; ++i) {
		double phase = ZXP(phasein);
		
		LOOP_BODY_4(i)
		
	}
}

void HPbufRd_next_2(HPbufRd *unit, int inNumSamples)
{
	float *phasein = ZIN(1);
	float *ratein  = ZIN(2);
	int32 loop     = (int32)ZIN0(2);
	double seuil    = ZIN0(4);
	double sensibilite = ZIN0(5);
	
    GET_BUF_SHARED
    uint32 numOutputs = unit->mNumOutputs;
    
    CHECK_BUFFER_DATA
    
    double loopMax = (double)(loop ? bufFrames : bufFrames - 1);

	double newRate = fabs(ZXP(ratein));
	if (newRate > 8.0) newRate = 8.0;
	double newSeuil = seuil * newRate;
	double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
	if (newSeuil > 2.0) newSeuil = 2.0;
	
	
	
	
	for (int i=0; i<inNumSamples; ++i) {
		double phase = ZXP(phasein);
		
		LOOP_BODY_2(i)
		
	}
}

void HPbufRd_next_1(HPbufRd *unit, int inNumSamples)
{
	float *phasein = ZIN(1);
	float *ratein  = ZIN(2);
	int32 loop     = (int32)ZIN0(2);
	double seuil    = ZIN0(4);
	double sensibilite = ZIN0(5);
	
    GET_BUF_SHARED
    uint32 numOutputs = unit->mNumOutputs;
    
    CHECK_BUFFER_DATA
    
    double loopMax = (double)(loop ? bufFrames : bufFrames - 1);

    double newRate = fabs(ZXP(ratein));
	if (newRate > 8.0) newRate = 8.0;
	double newSeuil = seuil * newRate;
	double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
	if (newSeuil > 2.0) newSeuil = 2.0;
	
	
	
	
	for (int i=0; i<inNumSamples; ++i) {
		double phase = ZXP(phasein);
		
		LOOP_BODY_1(i)
		
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

#define GRAIN_BUF \
const SndBuf *buf = NULL;                                           \
if (bufnum >= world->mNumSndBufs) {                                 \
int localBufNum = bufnum - numBufs;                             \
Graph *parent = unit->mParent;                                  \
if(localBufNum <= parent->localBufNum) {                        \
buf = parent->mLocalSndBufs + localBufNum;                  \
} else {                                                        \
continue;                                                   \
}                                                               \
} else {                                                            \
buf = bufs + bufnum;                                            \
}                                                                   \
LOCK_SNDBUF_SHARED(buf);                                            \
const float *bufData __attribute__((__unused__)) = buf->data;       \
uint32 bufChannels __attribute__((__unused__)) = buf->channels;     \
uint32 bufSamples __attribute__((__unused__)) = buf->samples;       \
uint32 bufFrames = buf->frames;                                     \
int guardFrame __attribute__((__unused__)) = bufFrames - 2;         \

inline float IN_AT(Unit* unit, int index, int offset)
{
    if (INRATE(index) == calc_FullRate) return IN(index)[offset];
    if (INRATE(index) == calc_DemandRate) return DEMANDINPUT_A(index, offset + 1);
    return ZIN0(index);
}

inline double sc_gloop(double in, double hi)
{
    // avoid the divide if possible
    if (in >= hi) {
        in -= hi;
        if (in < hi) return in;
    } else if (in < 0.) {
        in += hi;
        if (in >= 0.) return in;
    } else return in;
    
    return in - hi * floor(in/hi);
}

#define GRAIN_LOOP_BODY_4 \
float amp = y1 * y1; \
phase = sc_gloop(phase, loopMax); \
int32 iphase = (int32)phase; \
const float* table1 = bufData + iphase; \
const float* table0 = table1 - 1; \
const float* table2 = table1 + 1; \
const float* table3 = table1 + 2; \
if (iphase == 0) { \
table0 += bufSamples; \
} else if (iphase >= guardFrame) { \
if (iphase == guardFrame) { \
table3 -= bufSamples; \
} else { \
table2 -= bufSamples; \
table3 -= bufSamples; \
} \
} \
float fracphase = phase - (double)iphase; \
float a = table0[0]; \
float b = table1[0]; \
float c = table2[0]; \
float d = table3[0]; \
float outval = amp * cubicinterp(fracphase, a, b, c, d); \
outval = (float)hpSmoothDiscontinuity2( \
    unit->prevDatas[numGrain], outval, newSeuil, newSensibilite); \
unit->prevDatas[numGrain] = outval; \
\
ZXP(out1) += outval * pan1; \
ZXP(out2) += outval * pan2; \
double y0 = b1 * y1 - y2; \
y2 = y1; \
y1 = y0; \


#define GRAIN_LOOP_BODY_2 \
float amp = y1 * y1; \
phase = sc_gloop(phase, loopMax); \
int32 iphase = (int32)phase; \
const float* table1 = bufData + iphase; \
const float* table2 = table1 + 1; \
if (iphase > guardFrame) { \
table2 -= bufSamples; \
} \
float fracphase = phase - (double)iphase; \
float b = table1[0]; \
float c = table2[0]; \
float outval = amp * (b + fracphase * (c - b)); \
outval = (float)hpSmoothDiscontinuity2( \
    unit->prevDatas[numGrain], outval, newSeuil, newSensibilite); \
unit->prevDatas[numGrain] = outval; \
\
ZXP(out1) += outval * pan1; \
ZXP(out2) += outval * pan2; \
double y0 = b1 * y1 - y2; \
y2 = y1; \
y1 = y0; \


#define GRAIN_LOOP_BODY_1 \
float amp = y1 * y1; \
phase = sc_gloop(phase, loopMax); \
int32 iphase = (int32)phase; \
float outval = amp * bufData[iphase]; \
outval = (float)hpSmoothDiscontinuity2( \
    unit->prevDatas[numGrain], outval, newSeuil, newSensibilite); \
unit->prevDatas[numGrain] = outval; \
\
ZXP(out1) += outval * pan1; \
ZXP(out2) += outval * pan2; \
double y0 = b1 * y1 - y2; \
y2 = y1; \
y1 = y0; \

void HPtGrains_next(HPtGrains *unit, int inNumSamples)
{
	float *trigin = IN(0);
	float prevtrig = unit->mPrevTrig;
	double seuil = ZIN0(7);
	double sensibilite = ZIN0(8);
	
	
    uint32 numOutputs = unit->mNumOutputs;
    ClearUnitOutputs(unit, inNumSamples);
    float *out[16];
    for (uint32 i=0; i<numOutputs; ++i) out[i] = ZOUT(i);
    
    World *world = unit->mWorld;
    SndBuf *bufs = world->mSndBufs;
    uint32 numBufs = world->mNumSndBufs;
	
	for (int i=0; i < unit->mNumActive; ) {
		Grain *grain = unit->mGrains + i;
        int numGrain= i;
		uint32 bufnum = grain->bufnum;
		
		GRAIN_BUF
		
		if (bufChannels != 1) {
			++i;
			continue;
		}
		
		double loopMax = (double)bufFrames;
		
		float pan1 = grain->pan1;
		float pan2 = grain->pan2;
		double rate = grain->rate;
		double phase = grain->phase;
		double b1 = grain->b1;
		double y1 = grain->y1;
		double y2 = grain->y2;
		
        double newRate = fabs(rate);
        double newSeuil = seuil * newRate;
        double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
        if (newSeuil > 2.0) newSeuil = 2.0;
        
        
		uint32 chan1 = grain->chan;
		uint32 chan2 = chan1 + 1;
		if (chan2 >= numOutputs) chan2 = 0;
		
		float *out1 = out[chan1];
		float *out2 = out[chan2];
		//printf("B chan %d %d  %08X %08X", chan1, chan2, out1, out2);
		
		int nsmps = sc_min(grain->counter, inNumSamples);
		if (grain->interp >= 4) {
			for (int j=0; j<nsmps; ++j) {
				GRAIN_LOOP_BODY_4;
				phase += rate;
			}
		} else if (grain->interp >= 2) {
			for (int j=0; j<nsmps; ++j) {
				GRAIN_LOOP_BODY_2;
				phase += rate;
			}
		} else {
			for (int j=0; j<nsmps; ++j) {
				GRAIN_LOOP_BODY_1;
				phase += rate;
			}
		}
		
		grain->phase = phase;
		grain->y1 = y1;
		grain->y2 = y2;
		
		grain->counter -= nsmps;
		if (grain->counter <= 0) {
			// remove grain
			int removedIndex = (int)(grain - unit->mGrains);
            int lastIndex = --unit->mNumActive;
            if (removedIndex != lastIndex) {
                *grain = unit->mGrains[lastIndex];
                unit->prevDatas[removedIndex] = unit->prevDatas[lastIndex];
            }
            unit->prevDatas[lastIndex] = 0.0;
		} else ++i;
	}
	
	int trigSamples = INRATE(0) == calc_FullRate ? inNumSamples : 1;
	
	for (int i=0; i<trigSamples; ++i) {
		float trig = trigin[i];
		
		if (trig > 0.f && prevtrig <= 0.f) {
			// start a grain
			if (unit->mNumActive+1 >= kMaxGrains) break;
			uint32 bufnum = (uint32)IN_AT(unit, 1, i);
			if (bufnum >= numBufs) continue;
			GRAIN_BUF
			
			if (bufChannels != 1) continue;
			
			float bufSampleRate = buf->samplerate;
			float bufRateScale = bufSampleRate * SAMPLEDUR;
			double loopMax = (double)bufFrames;
			
			Grain *grain = unit->mGrains + unit->mNumActive++;
			int numGrain = unit->mNumActive - 1;
			unit->prevDatas[numGrain] = 0.0;
			grain->bufnum = bufnum;
			
			double counter = floor(IN_AT(unit, 4, i) * SAMPLERATE);
			counter = sc_max(4., counter);
			grain->counter = (int)counter;
			
			double rate = grain->rate = IN_AT(unit, 2, i) * bufRateScale;
			double newRate = fabs(rate);
			double newSeuil = seuil * newRate;
			double newSensibilite = std::max(0.0, std::min(1.0, sensibilite));
			if (newSeuil > 2.0) newSeuil = 2.0;
			
			
			
			double centerPhase = IN_AT(unit, 3, i) * bufSampleRate;
			double phase = centerPhase - 0.5 * counter * rate;
			
			float pan = IN_AT(unit, 5, i);
			float amp = IN_AT(unit, 6, i);
			grain->interp = (int)IN_AT(unit, 9, i);
			
			float panangle;
			if (numOutputs > 2) {				
				pan = sc_wrap(pan * 0.5f, 0.f, 1.f);
				float cpan = numOutputs * pan + 0.5;
				float ipan = floor(cpan);
				float panfrac = cpan - ipan;
				panangle = panfrac * pi2;
				grain->chan = (int)ipan;
				if (grain->chan >= (int)numOutputs) grain->chan -= numOutputs;
			} else {
				grain->chan = 0;
				pan = sc_wrap(pan * 0.5f + 0.5f, 0.f, 1.f);
				panangle = pan * pi2;
			}
			float pan1 = grain->pan1 = amp * cos(panangle);
			float pan2 = grain->pan2 = amp * sin(panangle);
			double w = pi / counter;
			double b1 = grain->b1 = 2. * cos(w);
			double y1 = sin(w);
			double y2 = 0.;
			
			uint32 chan1 = grain->chan;
			uint32 chan2 = chan1 + 1;
			if (chan2 >= numOutputs) chan2 = 0;
			
			float *out1 = out[chan1] + i;
			float *out2 = out[chan2] + i;
			
			int nsmps = sc_min(grain->counter, inNumSamples - i);
			if (grain->interp >= 4) {
				for (int j=0; j<nsmps; ++j) {
					GRAIN_LOOP_BODY_4;
					phase += rate;
				}
			} else if (grain->interp >= 2) {
				for (int j=0; j<nsmps; ++j) {
					GRAIN_LOOP_BODY_2;
					phase += rate;
				}
			} else {
				for (int j=0; j<nsmps; ++j) {
					GRAIN_LOOP_BODY_1;
					phase += rate;
				}
			}
			
			grain->phase = phase;
			grain->y1 = y1;
			grain->y2 = y2;
			
			grain->counter -= nsmps;
			if (grain->counter <= 0) {
				// remove grain
				int removedIndex = (int)(grain - unit->mGrains);
            int lastIndex = --unit->mNumActive;
            if (removedIndex != lastIndex) {
                *grain = unit->mGrains[lastIndex];
                unit->prevDatas[removedIndex] = unit->prevDatas[lastIndex];
            }
            unit->prevDatas[lastIndex] = 0.0;
			}
		}
		prevtrig = trig;
	}	
	
	unit->mPrevTrig = prevtrig;
}

void HPtGrains_Ctor(HPtGrains *unit)
{	
	SETCALC(HPtGrains_next);
	
	unit->mNumActive = 0;
	unit->mPrevTrig = 0.;
	unit->prevData = 0.0;
	for(uint32 i=0; i<kMaxGrains; ++i)
	{
		unit->prevDatas[i] = 0.0;
	}
	
	ClearUnitOutputs(unit, 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

PluginLoad(HPdelayUGens)
{
    ft = inTable;
    
#define DefineInfoUnit(name) \
(*ft->fDefineUnit)(#name, sizeof(Unit), (UnitCtorFunc)&name##_Ctor, 0, 0);
    
#define DefineBufInfoUnit(name) \
(*ft->fDefineUnit)(#name, sizeof(BufInfoUnit), (UnitCtorFunc)&name##_Ctor, 0, 0);
    
    DefineSimpleCantAliasUnit(HPplayBuf);
    DefineSimpleUnit(HPbufRd);
    DefineSimpleCantAliasUnit(HPtGrains);

}
