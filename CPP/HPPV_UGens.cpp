/*
	SuperCollider real time audio synthesis system
    Copyright (c) 2002 James McCartney. All rights reserved.
	http://www.audiosynth.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/


#include "FFT_UGens.h"

#define TWOPI 6.28318530717952646f

InterfaceTable *ft;

struct PV_OutOfPlace : Unit
{
	int m_numbins;
	float *m_tempbuf;
};

/// Provinescu Plugin

struct PV_HPecartType : PV_OutOfPlace
{
};

struct PV_HPinverse : PV_OutOfPlace
{
};

struct PV_HPfiltre : PV_OutOfPlace
{
};

struct PV_HPshiftDown : PV_OutOfPlace
{
};

//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{
    // Provinescu Plugin
    
    void PV_HPshiftDown_Ctor(PV_HPshiftDown *unit);
    void PV_HPshiftDown_Dtor(PV_HPshiftDown *unit);
    void PV_HPshiftDown_next(PV_HPshiftDown *unit, int inNumSamples);
    
    void PV_HPecartType_Ctor(PV_HPecartType *unit);
    void PV_HPecartType_Dtor(PV_HPecartType *unit);
    void PV_HPecartType_next(PV_HPecartType *unit, int inNumSamples);
    
    void PV_HPinverse_Ctor(PV_HPinverse *unit);
    void PV_HPinverse_Dtor(PV_HPinverse *unit);
    void PV_HPinverse_next(PV_HPinverse *unit, int inNumSamples);
    
    void PV_HPfiltre_Ctor(PV_HPfiltre *unit);
    void PV_HPfiltre_Dtor(PV_HPfiltre *unit);
    void PV_HPfiltre_next(PV_HPfiltre *unit, int inNumSamples);
    
/* spectral feature extractors? :
		bin freq
		bin magnitude
		bin phase
		bin laden ;-}
		average magnitude over a range of bins
		max magnitude over a range of bins
		max magnitude bin freq


*/

}

//////////////////////////////////////////////////////////////////////////////////////////////////


//SCPolarBuf* ToPolarApx(SndBuf *buf);
/*
SCPolarBuf* ToPolarApx(SndBuf *buf)
{
	if (buf->coord == coord_Complex) {
		SCComplexBuf* p = (SCComplexBuf*)buf->data;
		int numbins = buf->samples - 2 >> 1;
		for (int i=0; i<numbins; ++i) {
			p->bin[i].ToPolarApxInPlace();
		}
		buf->coord = coord_Polar;
	}
	return (SCPolarBuf*)buf->data;
}
*/
//SCComplexBuf* ToComplexApx(SndBuf *buf);
/*
SCComplexBuf* ToComplexApx(SndBuf *buf)
{
	if (buf->coord == coord_Polar) {
		SCPolarBuf* p = (SCPolarBuf*)buf->data;
		int numbins = buf->samples - 2 >> 1;
		for (int i=0; i<numbins; ++i) {
			p->bin[i].ToComplexApxInPlace();
		}
		buf->coord = coord_Complex;
	}
	return (SCComplexBuf*)buf->data;
}
*/

/////////////////////////////////////////////////////////////////////////////////////////////

// Provinescu Plugin

void PV_HPshiftDown_next(PV_HPshiftDown *unit, int inNumSamples)
{
    PV_GET_BUF
    MAKE_TEMP_BUF
    
    SCPolarBuf *p = ToPolarApx(buf);
    SCPolarBuf *q = (SCPolarBuf*)unit->m_tempbuf;
    
    int shift = ZIN0(1);
    
    if(shift < 0) shift = 0;
    if(shift >= numbins/2) shift = numbins / 2;
    
    q->dc = p->dc;
    q->nyq = p->nyq;
    for (int i=0, j=shift; i<numbins; ++i, j=(j + 1)%(numbins))
    {
        q->bin[i] = p->bin[j];
    }
    
    // copy tampon + OUT
    memcpy(p->bin, q->bin, numbins * sizeof(SCComplex));
}

void PV_HPshiftDown_Ctor(PV_HPshiftDown *unit)
{
    SETCALC(PV_HPshiftDown_next);
    ZOUT0(0) = ZIN0(0);
    unit->m_tempbuf = 0;
}

void PV_HPshiftDown_Dtor(PV_HPshiftDown *unit)
{
    RTFree(unit->mWorld, unit->m_tempbuf);
}
void PV_HPecartType_next(PV_HPecartType *unit, int inNumSamples)
{
    PV_GET_BUF
    MAKE_TEMP_BUF
    
    SCPolarBuf *p = ToPolarApx(buf);
    SCPolarBuf *q = (SCPolarBuf*)unit->m_tempbuf;
    
    float mulEcartType = ZIN0(1);
    if(mulEcartType < 0.0) mulEcartType = mulEcartType * (-1.0);
    
    // Calcul de la moyenne + init OUT
    float sumMag = 0.0;
    float sumPha = 0.0;
    for (int j=0; j<numbins/2; j++)
    {
        sumMag += p->bin[j].mag;
        sumPha += p->bin[j].phase;
    }
    // Moyenne
    float moyenneMag = sumMag / (numbins/2);
    float moyennePha = sumPha / (numbins/2);
    // Calcul de la variance
    sumMag = 0.0;
    sumPha = 0.0;
    for (int j=0; j<numbins/2; j++)
    {
        sumMag += pow((p->bin[j].mag - moyenneMag), 2.0);
        sumPha += pow((p->bin[j].phase - moyennePha), 2.0);
    }
    // Variance
    float varianceMag = sumMag / (numbins/2);
    float variancePha = sumPha / (numbins/2);
    // Ecart-type
    float ecartTypeMag = sqrt(varianceMag) * mulEcartType;
    float ecartTypePha = sqrt(variancePha) * mulEcartType;
    
    q->dc = p->dc;
    q->nyq = p->nyq;
    for (int i=1; i<numbins - 1; i++)
    {
        if(p->bin[i].mag > ecartTypeMag)
        {
            q->bin[i].mag = ecartTypeMag;
        }
        else
        {
            q->bin[i].mag = 0.0;
        }
        if(p->bin[i].phase > ecartTypePha)
        {
            q->bin[i].phase = ecartTypePha;
        }
        else
        {
            q->bin[i].phase = 0.0;
        }
    }
    // copy tampon + OUT
    memcpy(p->bin, q->bin, numbins * sizeof(SCComplex));
}

void PV_HPecartType_Ctor(PV_HPecartType *unit)
{
    SETCALC(PV_HPecartType_next);
    ZOUT0(0) = ZIN0(0);
    unit->m_tempbuf = 0;
}

void PV_HPecartType_Dtor(PV_HPecartType *unit)
{
    RTFree(unit->mWorld, unit->m_tempbuf);
}

void PV_HPinverse_next(PV_HPinverse *unit, int inNumSamples)
{
    PV_GET_BUF
    MAKE_TEMP_BUF
    
    SCPolarBuf *p = ToPolarApx(buf);
    SCPolarBuf *q = (SCPolarBuf*)unit->m_tempbuf;
    
    int index = ZIN0(1);
    if(index < 0) index = 1;
    if(index > numbins/2) index = numbins/2;
    
    q->dc = p->dc;
    q->nyq = p->nyq;
    for(int i=0, j=numbins - 1; i<numbins; ++i, --j)
    {
        q->bin[i].mag = p->bin[j].mag;
        q->bin[i].phase = p->bin[j].phase;
    }
    
    // copy tampon + OUT
    memcpy(p->bin, q->bin, numbins * sizeof(SCComplex));
}

void PV_HPinverse_Ctor(PV_HPinverse *unit)
{
    SETCALC(PV_HPinverse_next);
    
    ZOUT0(0) = ZIN0(0);
    unit->m_tempbuf = 0;
}

void PV_HPinverse_Dtor(PV_HPinverse *unit)
{
    RTFree(unit->mWorld, unit->m_tempbuf);
}

void PV_HPfiltre_next(PV_HPfiltre *unit, int inNumSamples)
{
    PV_GET_BUF
    MAKE_TEMP_BUF
    
    SCPolarBuf *p = ToPolarApx(buf);
    SCPolarBuf *q = (SCPolarBuf*)unit->m_tempbuf;
    
    int index = ZIN0(1);
    int size = ZIN0(2);
    
    if(index < 0) index = 0;
    if(index > numbins) index = numbins;
    if(size < 0) size =0;
    if(size > numbins) size = numbins;
    
    // Set zero OUT
    for(int i=0; i<numbins; ++i)
    {
        q->bin[i].mag = 0.0;
    }
    
    q->dc = p->dc;
    q->nyq = p->nyq;
    // Apply filtre
    int pos = 0;
    while (pos < size)
    {
        q->bin[(index+pos)%(numbins)].mag = p->bin[(index+pos)%(numbins)].mag;
        q->bin[(index+pos)%(numbins)].phase = p->bin[(index+pos)%(numbins)].phase;
        pos = pos + 1;
    }
    
    // copy tampon + OUT
    memcpy(p->bin, q->bin, numbins * sizeof(SCComplex));
}

void PV_HPfiltre_Ctor(PV_HPfiltre *unit)
{
    SETCALC(PV_HPfiltre_next);
    
    ZOUT0(0) = ZIN0(0);
    unit->m_tempbuf = 0;
}

void PV_HPfiltre_Dtor(PV_HPfiltre *unit)
{
    RTFree(unit->mWorld, unit->m_tempbuf);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

#define DefinePVUnit(name) \
	(*ft->fDefineUnit)(#name, sizeof(PV_Unit), (UnitCtorFunc)&name##_Ctor, 0, 0);


void initPV(InterfaceTable *inTable)
{  
    // provinescu Plugin
    
    DefineDtorUnit(PV_HPshiftDown);
    DefineDtorUnit(PV_HPecartType);
    DefineDtorUnit(PV_HPinverse);
    DefineDtorUnit(PV_HPfiltre);
    
}


PluginLoad(HPPV_UGens)
{
    ft = inTable;
    initPV(inTable);
}
