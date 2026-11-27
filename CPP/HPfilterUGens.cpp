/*
 *  HPUGens.cpp
 *  PluginsHP
 *
 *  Created by H P on 08.06.08
 *
 */

#include "SC_PlugIn.h"
#include <cstdio>

#define PI 3.1415926535898f

const int kMAXMEDIANSIZE = 32;

static InterfaceTable *ft;

struct DelayUnit : public Unit
{
	float *m_dlybuf;
	
	float m_dsamp, m_fdelaylen;
	float m_delaytime, m_maxdelaytime;
	long m_iwrphase, m_idelaylen, m_mask;
	long m_numoutput;
};

struct HPdelayN : public DelayUnit
{
};

struct FeedbackDelay : public DelayUnit
{
	float m_feedbk, m_decaytime;
};

struct HPcombC : public FeedbackDelay
{
};

//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{
	void load(InterfaceTable *inTable);

	void DelayUnit_Reset(DelayUnit *unit);
	void DelayUnit_Dtor(DelayUnit *unit);
	void DelayUnit_AllocDelayLine(DelayUnit *unit);
	void FeedbackDelay_Reset(FeedbackDelay *unit);

	void HPdelayN_Ctor(HPdelayN *unit);
	void HPdelayN_next(HPdelayN *unit, int inNumSamples);
	void HPdelayN_next_z(HPdelayN *unit, int inNumSamples);

	void HPcombC_Ctor(HPcombC *unit);
	void HPcombC_next(HPcombC *unit, int inNumSamples);
	void HPcombC_next_z(HPcombC *unit, int inNumSamples);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DelayUnit_AllocDelayLine(DelayUnit *unit)
{
	long delaybufsize = (long)ceil(unit->m_maxdelaytime * SAMPLERATE + 1.f);
	delaybufsize = delaybufsize + BUFLENGTH;
	delaybufsize = NEXTPOWEROFTWO(delaybufsize);  // round up to next power of two
	unit->m_fdelaylen = unit->m_idelaylen = delaybufsize;
	
	RTFree(unit->mWorld, unit->m_dlybuf);
	unit->m_dlybuf = (float*)RTAlloc(unit->mWorld, delaybufsize * sizeof(float));
	unit->m_mask = delaybufsize - 1;
}

float CalcDelay(DelayUnit *unit, float delaytime);
float CalcDelay(DelayUnit *unit, float delaytime)
{
	float next_dsamp = delaytime * SAMPLERATE;
	return sc_clip(next_dsamp, 1.f, unit->m_fdelaylen);
}

void DelayUnit_Reset(DelayUnit *unit)
{
	unit->m_maxdelaytime = ZIN0(1);
	unit->m_delaytime = ZIN0(2);
	unit->m_dlybuf = 0;
	
	DelayUnit_AllocDelayLine(unit);
	
	unit->m_dsamp = CalcDelay(unit, unit->m_delaytime);
	
	unit->m_numoutput = 0;
	unit->m_iwrphase = 0;
}


void DelayUnit_Dtor(DelayUnit *unit)
{
	RTFree(unit->mWorld, unit->m_dlybuf);
}

///////////////////////////////////////////////////////////////////////////////////////////

void HPdelayN_Ctor(HPdelayN *unit)
{
	DelayUnit_Reset(unit);
	SETCALC(HPdelayN_next_z);
	ZOUT0(0) = 0.f;
}

void HPdelayN_next(HPdelayN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	
	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;
	
	//Print("DelayN_next %08X %g %g  %d %d\n", unit, delaytime, dsamp, mask, iwrphase);
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyrd   = dlybuf1 + (irdphase & mask);
		float* dlywr   = dlybuf1 + (iwrphase & mask);
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		long remain = inNumSamples;
		while (remain) {
			long rdspace = dlyN - dlyrd;
			long wrspace = dlyN - dlywr;
			long nsmps = sc_min(rdspace, wrspace);
			nsmps = sc_min(remain, nsmps);
			remain -= nsmps;
			LOOP(nsmps,
				 ZXP(dlywr) = ZXP(in);
				 ZXP(out) = ZXP(dlyrd);
				 );
			if (dlyrd == dlyN) dlyrd = dlybuf1;
			if (dlywr == dlyN) dlywr = dlybuf1;
		}
		iwrphase += inNumSamples;
	} else {
		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
		
		LOOP1(inNumSamples,
			  dlybuf[iwrphase & mask] = ZXP(in);
			  dsamp += dsamp_slope;
			  ++iwrphase;
			  long irdphase = iwrphase - (long)dsamp;
			  ZXP(out) = dlybuf[irdphase & mask];
			  );
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}
	
	unit->m_iwrphase = iwrphase;
	
}


void HPdelayN_next_z(HPdelayN *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	
	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	long mask = unit->m_mask;
	
	if (delaytime == unit->m_delaytime) {
		long irdphase = iwrphase - (long)dsamp;
		float* dlybuf1 = dlybuf - ZOFF;
		float* dlyN    = dlybuf1 + unit->m_idelaylen;
		long remain = inNumSamples;
		while (remain) {
			float* dlywr = dlybuf1 + (iwrphase & mask);
			float* dlyrd = dlybuf1 + (irdphase & mask);
			long rdspace = dlyN - dlyrd;
			long wrspace = dlyN - dlywr;
			long nsmps = sc_min(rdspace, wrspace);
			nsmps = sc_min(remain, nsmps);
			remain -= nsmps;
			if (irdphase < 0) {
				LOOP(nsmps,
					 ZXP(dlywr) = ZXP(in);
					 ZXP(out) = 0.f;
					 );
			} else {
				LOOP(nsmps,
					 ZXP(dlywr) = ZXP(in);
					 ZXP(out) = ZXP(dlyrd);
					 );
			}
			iwrphase += nsmps;
			irdphase += nsmps;
		}
	} else {
		
		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
		
		LOOP1(inNumSamples,
			  dsamp += dsamp_slope;
			  long irdphase = iwrphase - (long)dsamp;
			  
			  dlybuf[iwrphase & mask] = ZXP(in);
			  if (irdphase < 0) {
				  ZXP(out) = 0.f;
			  } else {
				  ZXP(out) = dlybuf[irdphase & mask];
			  }
			  iwrphase++;
			  );
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
	}
	
	unit->m_iwrphase = iwrphase;
	
	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(HPdelayN_next);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void FeedbackDelay_Reset(FeedbackDelay *unit)
{
	unit->m_decaytime = ZIN0(3);
	
	DelayUnit_Reset(unit);
	
	unit->m_feedbk = sc_CalcFeedback(unit->m_delaytime, unit->m_decaytime);
}

////////////////////////////////////////////////////////////////////////////////////////////

void HPcombC_Ctor(HPcombC *unit)
{
	FeedbackDelay_Reset(unit);
	SETCALC(HPcombC_next_z);
	ZOUT0(0) = 0.f;
}

void HPcombC_next(HPcombC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);
	
	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	
	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP1(inNumSamples,
			  long irdphase1 = iwrphase - idsamp;
			  long irdphase2 = irdphase1 - 1;
			  long irdphase3 = irdphase1 - 2;
			  long irdphase0 = irdphase1 + 1;
			  float d0 = dlybuf[irdphase0 & mask];
			  float d1 = dlybuf[irdphase1 & mask];
			  float d2 = dlybuf[irdphase2 & mask];
			  float d3 = dlybuf[irdphase3 & mask];
			  float value = cubicinterp(frac, d0, d1, d2, d3);
			  dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
			  ZXP(out) = value;
			  iwrphase++;
			  );
	} else {
		
		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
		
		float next_feedbk = sc_CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
		
		LOOP1(inNumSamples,
			  dsamp += dsamp_slope;
			  long idsamp = (long)dsamp;
			  float frac = dsamp - idsamp;
			  long irdphase1 = iwrphase - idsamp;
			  long irdphase2 = irdphase1 - 1;
			  long irdphase3 = irdphase1 - 2;
			  long irdphase0 = irdphase1 + 1;
			  float d0 = dlybuf[irdphase0 & mask];
			  float d1 = dlybuf[irdphase1 & mask];
			  float d2 = dlybuf[irdphase2 & mask];
			  float d3 = dlybuf[irdphase3 & mask];
			  float value = cubicinterp(frac, d0, d1, d2, d3);
			  dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
			  ZXP(out) = value;
			  feedbk += feedbk_slope;
			  iwrphase++;
			  );
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}
	
	unit->m_iwrphase = iwrphase;
	
}


void HPcombC_next_z(HPcombC *unit, int inNumSamples)
{
	float *out = ZOUT(0);
	float *in = ZIN(0);
	float delaytime = ZIN0(2);
	float decaytime = ZIN0(3);
	
	float *dlybuf = unit->m_dlybuf;
	long iwrphase = unit->m_iwrphase;
	float dsamp = unit->m_dsamp;
	float feedbk = unit->m_feedbk;
	long mask = unit->m_mask;
	float d0, d1, d2, d3;
	
	if (delaytime == unit->m_delaytime && decaytime == unit->m_decaytime) {
		long idsamp = (long)dsamp;
		float frac = dsamp - idsamp;
		LOOP1(inNumSamples,
			  long irdphase1 = iwrphase - idsamp;
			  long irdphase2 = irdphase1 - 1;
			  long irdphase3 = irdphase1 - 2;
			  long irdphase0 = irdphase1 + 1;
			  
			  if (irdphase0 < 0) {
				  dlybuf[iwrphase & mask] = ZXP(in);
				  ZXP(out) = 0.f;
			  } else {
				  if (irdphase1 < 0) {
					  d1 = d2 = d3 = 0.f;
					  d0 = dlybuf[irdphase0 & mask];
				  } else if (irdphase2 < 0) {
					  d1 = d2 = d3 = 0.f;
					  d0 = dlybuf[irdphase0 & mask];
					  d1 = dlybuf[irdphase1 & mask];
				  } else if (irdphase3 < 0) {
					  d3 = 0.f;
					  d0 = dlybuf[irdphase0 & mask];
					  d1 = dlybuf[irdphase1 & mask];
					  d2 = dlybuf[irdphase2 & mask];
				  } else {
					  d0 = dlybuf[irdphase0 & mask];
					  d1 = dlybuf[irdphase1 & mask];
					  d2 = dlybuf[irdphase2 & mask];
					  d3 = dlybuf[irdphase3 & mask];
				  }
				  float value = cubicinterp(frac, d0, d1, d2, d3);
				  dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
				  ZXP(out) = value;
			  }
			  iwrphase++;
			  );
	} else {
		
		float next_dsamp = CalcDelay(unit, delaytime);
		float dsamp_slope = CALCSLOPE(next_dsamp, dsamp);
		
		float next_feedbk = sc_CalcFeedback(delaytime, decaytime);
		float feedbk_slope = CALCSLOPE(next_feedbk, feedbk);
		
		LOOP1(inNumSamples,
			  dsamp += dsamp_slope;
			  long idsamp = (long)dsamp;
			  float frac = dsamp - idsamp;
			  long irdphase1 = iwrphase - idsamp;
			  long irdphase2 = irdphase1 - 1;
			  long irdphase3 = irdphase1 - 2;
			  long irdphase0 = irdphase1 + 1;
			  
			  if (irdphase0 < 0) {
				  dlybuf[iwrphase & mask] = ZXP(in);
				  ZXP(out) = 0.f;
			  } else {
				  if (irdphase1 < 0) {
					  d1 = d2 = d3 = 0.f;
					  d0 = dlybuf[irdphase0 & mask];
				  } else if (irdphase2 < 0) {
					  d1 = d2 = d3 = 0.f;
					  d0 = dlybuf[irdphase0 & mask];
					  d1 = dlybuf[irdphase1 & mask];
				  } else if (irdphase3 < 0) {
					  d3 = 0.f;
					  d0 = dlybuf[irdphase0 & mask];
					  d1 = dlybuf[irdphase1 & mask];
					  d2 = dlybuf[irdphase2 & mask];
				  } else {
					  d0 = dlybuf[irdphase0 & mask];
					  d1 = dlybuf[irdphase1 & mask];
					  d2 = dlybuf[irdphase2 & mask];
					  d3 = dlybuf[irdphase3 & mask];
				  }
				  float value = cubicinterp(frac, d0, d1, d2, d3);
				  dlybuf[iwrphase & mask] = ZXP(in) + feedbk * value;
				  ZXP(out) = value;
			  }
			  feedbk += feedbk_slope;
			  iwrphase++;
			  );
		unit->m_feedbk = feedbk;
		unit->m_dsamp = dsamp;
		unit->m_delaytime = delaytime;
		unit->m_decaytime = decaytime;
	}
	
	unit->m_iwrphase = iwrphase;
	
	unit->m_numoutput += inNumSamples;
	if (unit->m_numoutput >= unit->m_idelaylen) {
		SETCALC(HPcombC_next);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////

PluginLoad(HPfilterUGens)
{
	ft = inTable;

#define DefineDelayUnit(name) \
(*ft->fDefineUnit)(#name, sizeof(name), (UnitCtorFunc)&name##_Ctor, \
(UnitDtorFunc)&DelayUnit_Dtor, 0);

	DefineDelayUnit(HPdelayN);
	DefineDelayUnit(HPcombC);

}
