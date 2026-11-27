/*
 *  HPFFT_UGens.cpp
 *  Plugins HP
 *
 *  Created by H P on 01.03.08.
 *
 */

#include "FFT_UGens.h"

InterfaceTable *ft;

// Fonction pour hasard
int hasard(int min, int max);

struct PV_OutOfPlace : Unit
{
	int m_numbins;
	float *m_tempbuf;
};

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

/*struct PV_HPdifference : PV_Unit
{
};

struct PV_HPrandom : PV_Unit
{
};

struct PV_HPneurones: PV_Unit
{
	float neuactiva [16];
	float seuils [16];
	float neuerreur [16];
	float poids [16][16];
	int neurones;
};*/

//////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
{	
	/*void PV_HPdifference_Ctor(PV_Unit *unit);
	void PV_HPdifference_next(PV_Unit *unit, int inNumSamples);

	void PV_HPrandom_Ctor(PV_Unit *unit);
	void PV_HPrandom_next(PV_Unit *unit, int inNumSamples);

	void PV_HPneurones_Ctor(PV_Unit *unit);
	void PV_HPneurones_next(PV_Unit *unit, int inNumSamples);*/

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

/*void PV_HPdifference_next(PV_Unit *unit, int inNumSamples)
{
	PV_GET_BUF2
	
	SCPolarBuf *p = ToPolarApx(buf1);
	SCPolarBuf *q = ToPolarApx(buf2);

	q->dc = p->dc;
	q->nyq = p->nyq;
	for (int i=0; i<numbins; ++i)
		{
		p->bin[i].mag -= q->bin[i].mag;
		p->bin[i].phase -= q->bin[i].phase;
		}
}

void PV_HPdifference_Ctor(PV_Unit *unit)
{
	SETCALC(PV_HPdifference_next);
	ZOUT0(0) = ZIN0(0);
}

void PV_HPrandom_next(PV_Unit *unit, int inNumSamples)
{
	PV_GET_BUF2
	
	SCPolarBuf *p = ToPolarApx(buf1);
	SCPolarBuf *q = ToPolarApx(buf2);

	q->dc = p->dc;
	q->nyq = p->nyq;
	for (int i=0; i<numbins; ++i)
		{
		p->bin[i].mag = hasard(p->bin[i].mag * 1000000, q->bin[i].mag * 1000000);
		p->bin[i].mag = p->bin[i].mag / 1000000;
		p->bin[i].phase = hasard(p->bin[i].phase * 1000000, q->bin[i].phase * 1000000);
		p->bin[i].phase = p->bin[i].phase / 1000000;
		}
}

void PV_HPrandom_Ctor(PV_Unit *unit)
{
	SETCALC(PV_HPrandom_next);

	srand((unsigned)time(0));

	ZOUT0(0) = ZIN0(0);
}*/

/*void PV_HPneurones_next(PV_HPneurones *unit, int inNumSamples)
 {
 PV_GET_BUF2
 
 SCPolarBuf *p = ToPolarApx(buf1);
 SCPolarBuf *q = ToPolarApx(buf2);
 
 int neurones = unit->neurones;//IN0(2)
 float apprentissage = IN0(3);
 float temperature = IN0(4);
 float mode = IN0(5);
 int neuin1 = 0;
 int neuin2 = 0;
 int neucouche1 = 1;
 int neucouche2 = neurones - 2;
 int neuout1 = neurones - 1;
 int neuout2 = neurones - 1;
 float activationneurones = 0.0;
 float activationerreurs = 0.0;
 float maxMag = 0.0;
 
 for (int z=0; z<numbins/2; ++z)
 {
 //Algo Neurones	sortie [-1, 1]
 for (int i=neuin1; i<=neurones; ++i)
 {
 unit->neuactiva[i] = 0.0; unit->neuerreur[i] = 0.0;
 }
 for(int j=neucouche1 ; j<=neucouche2; ++j) 
 {
 activationneurones=0.0;
 for(int i=neuin1; i<=neuin2; ++i)
 {
 activationneurones=activationneurones+(log(p->bin[z].mag) * unit->poids[i][j]);
 }
 unit->neuactiva[j]=tanh((activationneurones-unit->seuils[j])/temperature);
 }
 for(int j=neuout1 ; j <=neuout2; ++j)
 {
 activationneurones=0.0;
 for(int i=neucouche1; i<=neucouche2; ++i) 
 {
 activationneurones=activationneurones+(unit->neuactiva[i]*unit->poids[i][j]);
 }
 unit->neuactiva[j] = tanh((activationneurones-unit->seuils[j])/temperature);
 }
 if(mode > 0.0) 
 {
 for(int i=neuout1; i<=neuout2; ++i)
 {
 unit->neuerreur[i]=(q->bin[z].mag - unit->neuactiva[i])*(1.0-(unit->neuactiva[i]*unit->neuactiva[i]));
 unit->seuils[i] = unit->seuils[i] - (unit->neuerreur[i]*apprentissage);
 }
 for(int i=neucouche1 ; i<=neucouche2; ++i)
 {
 activationerreurs=0.0;
 for(int j=neuout1;  j<=neuout2; ++j)
 {
 activationerreurs=activationerreurs+(unit->neuerreur[j]*unit->poids[i][j]);
 };
 unit->neuerreur[i] = activationerreurs*(1.0-(unit->neuactiva[i]*unit->neuactiva[i]));
 };
 for(int i=neuin1; i<=neuin2; ++i)
 {
 activationerreurs=0.0;
 for(int j=neucouche1 ; j<=neucouche2; ++j)
 {
 activationerreurs=activationerreurs+(unit->neuerreur[j]*unit->poids[i][j]);
 };
 unit->neuerreur[i] = activationerreurs*(1.0-(unit->neuactiva[i]*unit->neuactiva[i]));
 }
 for(int i=neucouche1 ; i<=neucouche2; ++i)
 {
 unit->seuils[i] = unit->seuils[i]-(unit->neuerreur[i]*apprentissage);
 for(int j=neuout1 ; j<=neuout2; ++j)
 {
 unit->poids[i][j] = unit->poids[i][j]-((-1.0)*apprentissage*unit->neuerreur[j]*unit->neuactiva[i]);
 }
 };
 for(int i=neuin1; i<=neuin2; ++i)
 {
 unit->seuils[i] = unit->seuils[i]-(unit->neuerreur[i]*apprentissage);
 for(int j=neucouche1 ; j<=neucouche2; ++j)
 {
 unit->poids[i][j] = unit->poids[i][j]-((-1.0)*apprentissage*unit->neuerreur[j]*unit->neuactiva[i]);
 };
 };	
 };
 
 q->dc = p->dc;
 q->nyq = p->nyq;
 // Sortie  neural network
 for(int i=neuout1; i<=neuout2; ++i)
 {
 p->bin[z].mag = unit->neuactiva[i];
 p->bin[z].phase = q->bin[z].phase;
 }
 }
 }
 
 void PV_HPneurones_Ctor(PV_Unit *unit)
 {
 SETCALC(PV_HPneurones_next);
 
 int neurones = unit->neurones = IN0(2);
 
 if (neurones > 16) 
 {
 neurones = unit->neurones = 16;
 }
 else {
 if(neurones < 1)
 {
 neurones = unit->neurones = 1;
 }
 }
 
 // Generation aleatoire
 srand((unsigned)time(0)); 
 float number;
 for(int a = 0; a < neurones; ++a)
 {
 number = hasard (0, 1000000);
 number = number / 1000000;
 unit->neuactiva[a] = 0.0;
 unit->neuerreur[a] = 0.0;
 unit->seuils[a] = number;
 for(int b = 0; b < neurones; ++b)
 {
 number = hasard (0, 1000000);
 number = number / 1000000 - 0.5;
 unit->poids[a][b] = number;
 }
 }
 
 ZOUT0(0) = ZIN0(0);
 }*/

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
	double tamponMag = 0.0;
	double tamponPhase = 0.0;
	
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

	/*srand((unsigned)time(0)); 
	double number;
	number = hasard (0, 1000000000);*/

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
		q->bin[i] = 0.0;
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

void init_SCComplex(InterfaceTable *inTable);

#define DefinePVUnit(name) \
(*ft->fDefineUnit)(#name, sizeof(PV_Unit), (UnitCtorFunc)&name##_Ctor, 0, 0);


void initPV(InterfaceTable *inTable)
{	
	/*DefinePVUnit(PV_HPdifference);
	DefinePVUnit(PV_HPrandom);
	DefinePVUnit(PV_HPneurones);*/
	
	DefineDtorUnit(PV_HPshiftDown);
	DefineDtorUnit(PV_HPecartType);
	DefineDtorUnit(PV_HPinverse);
	DefineDtorUnit(PV_HPfiltre);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

//Fonction hasard
int hasard (int min, int max)
	{
	return (int) (min + ((float) rand() / RAND_MAX * (max - min + 1)));
	}

PluginLoad(HPFFT_UGens)
{
 ft = inTable;
 initPV(inTable);
}
