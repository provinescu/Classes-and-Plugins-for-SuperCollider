/*
 *  HPUGens.cpp
 *  PluginsHP
 *
 *  Created by H P on 01.11.07.
 *
 */

#include "SC_PlugIn.h"
#include <limits>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string.h>

#define PI 3.14159265358979323846

// pour FFT Algorithme
#define swap(a,b) norm=(a); (a)=(b); (b)=norm

static InterfaceTable *ft;

// Fonction pour hasard
int hasard(int min, int max);

// FFT Algorithme
void fft(float *reel, float *imag, int log2n, int sign);

//////////////////////////////////////////////////////////////

struct HPchaos : public Unit
{
    float data;
    float delai_freq;
};


struct HPneurones : public Unit
{
    float delai_freq;
    float freq;
    double neuactiva [65];
    double seuils [65];
    double neuerreur [65];
    double poids [65][65];
    double neural_out;
    int neurones;
};

struct HPgenetiques : public Unit
{
    float delai_freq;
    float freq;
    double genetic_out;
    double population [65];
    int individus;
    float mutation;
    float croisement;
    float mode;
};

struct HPneurones3D : public Unit
{
    float delai_freq;
    float freq;
    double neuactiva [65];
    double seuils [65];
    double neuerreur [65];
    double poids [65][65];
    double neural_out [3];
    int neurones;
};

struct HPgenetiques3D : public Unit
{
    float delai_freq;
    float freq;
    double genetic_out[3];
    double population [65][3];// 65 individus avec genomes de 3 datas
    int individus;
    float mutation;
    float croisement;
    float mode;
};

struct HPprobability1 : public Unit
{
    float delai_freq;
    float freq;
    double probability_out;
    int tailleBuffer;
    int compteurBuffer;
    float buffer [65];// buffer maximum data pour compute algo
};

struct HPprobability2 : public Unit
{
    float delai_freq;
    float freq;
    double probability_out;
    double lastProbability_out;
    int tailleBuffer;
    int compteurBuffer;
    float buffer [65];// buffer maximum data pour compute algo
    float bufferInterval [512];// buffer intervals 0.01
    float bufferDirection [3];// direction - 0 +
};

struct HPfractal3D : public Unit
{
    float freq;
    float delai_freq;
    double inF;
    double inA;
    double inD;
    double transformations [100] [12];
};

struct HPfftShift : public Unit
{
    float reel[64], imag[64], imag1[64];
};

struct HPfftDiff : public Unit
{
    float reel[64], imag[64], imag1[64], imag2[64];
};

struct HPfftRand : public Unit
{
    float reel[64], imag[64], imag1[64], imag2[64];
};

struct HPfftLog : public Unit
{
    float reel[64], imag[64], imag1[64], imag2[64];
};
////////////////////////////////////////////////////////////

extern "C"
{
    void load(InterfaceTable *inTable);
    
    void HPchaos_next_a(HPchaos *unit, int inNumSamples);
    void HPchaos_next_k(HPchaos *unit, int inNumSamples);
    void HPchaos_Ctor(HPchaos* unit);
    
    void HPneurones_next_a(HPneurones *unit, int inNumSamples);
    void HPneurones_next_k(HPneurones *unit, int inNumSamples);
    void HPneurones_Ctor(HPneurones* unit);
    
    void HPgenetiques_next_a(HPgenetiques *unit, int inNumSamples);
    void HPgenetiques_next_k(HPgenetiques *unit, int inNumSamples);
    void HPgenetiques_Ctor(HPgenetiques* unit);
    
    void HPneurones3D_next_k(HPneurones3D *unit, int inNumSamples);
    void HPneurones3D_Ctor(HPneurones3D* unit);
    
    void HPgenetiques3D_next_k(HPgenetiques3D *unit, int inNumSamples);
    void HPgenetiques3D_Ctor(HPgenetiques3D* unit);
    
    void HPprobability1_next_a(HPprobability1 *unit, int inNumSamples);
    void HPprobability1_next_k(HPprobability1 *unit, int inNumSamples);
    void HPprobability1_Ctor(HPprobability1* unit);
    
    void HPprobability2_next_a(HPprobability2 *unit, int inNumSamples);
    void HPprobability2_next_k(HPprobability2 *unit, int inNumSamples);
    void HPprobability2_Ctor(HPprobability2* unit);
    
    void HPfractal3D_next_a(HPfractal3D *unit, int inNumSamples);
    void HPfractal3D_next_k(HPfractal3D *unit, int inNumSamples);
    void HPfractal3D_Ctor(HPfractal3D* unit);
    
    void HPfftShift_next(HPfftShift *unit, int inNumSamples);
    void HPfftShift_Ctor(HPfftShift* unit);
    
    void HPfftDiff_next(HPfftDiff *unit, int inNumSamples);
    void HPfftDiff_Ctor(HPfftDiff* unit);
    
    void HPfftRand_next(HPfftRand *unit, int inNumSamples);
    void HPfftRand_Ctor(HPfftRand* unit);
    
    void HPfftLog_next(HPfftLog *unit, int inNumSamples);
    void HPfftLog_Ctor(HPfftLog* unit);
};

/////////////////////////////////////////////////////////////////

void HPchaos_Ctor(HPchaos* unit)
{
    if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPchaos_next_a);
    } else
    {
        SETCALC(HPchaos_next_k);
    }
    
    unit->data = IN0(2);
    unit->delai_freq = 0.f;
    
    OUT0(0) = 0.f;
}

void HPneurones_Ctor(HPneurones* unit)
{
    if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPneurones_next_a);
    } else
    {
        SETCALC(HPneurones_next_k);
    }
    
    unit->freq = IN0(0);
    unit->delai_freq = 0.f;
    int neurones = unit->neurones = IN0(6);
    unit->neural_out = 0.0;
    
    if (neurones > 64)
    {
        neurones = unit->neurones = 64;
    }
    else {
        if(neurones < 3)
        {
            neurones = unit->neurones = 3;
        }
    }
    
    // Generation aleatoire
//    srand((unsigned)time(0));
//    double number;
    RGen & rgen = *unit->mParent->mRGen; // pour Hasard
    
    for(int a = 0; a < neurones; ++a)
    {
        float number = rgen.frand();
        unit->neuactiva[a] = 0.0;
        unit->neuerreur[a] = 0.0;
        unit->seuils[a] = number;
        for(int b = 0; b < neurones; ++b)
        {
            number = rgen.frand() - 0.5;
            unit->poids[a][b] = number;
        }
    }
				
    OUT0(0) = 0.f;
}

void HPgenetiques_Ctor(HPgenetiques* unit)
{
    if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPgenetiques_next_a);
    } else
    {
        SETCALC(HPgenetiques_next_k);
    }
    
    unit->freq = IN0(0);
    int individus = unit->individus = IN0(4);
    unit->genetic_out = 0.f;
    unit->delai_freq = 0.f;
    
    if (individus > 64)
    {
        individus = unit->individus = 64;
    }
    else {
        if(individus < 2)
        {
            individus = unit->individus = 2;
        }
    }
				
    // Generation population aleatoire
//    srand((unsigned)time(0));
//    double number;
    RGen & rgen = *unit->mParent->mRGen; // pour Hasard
    
    for(int i = 0; i < individus; ++i)
    {
       float  number = rgen.frand() * 2.0 - 1.0;
        unit->population[i] = number;
    }
    
    OUT0(0) = 0.f;
}

void HPneurones3D_Ctor(HPneurones3D* unit)
{
    SETCALC(HPneurones3D_next_k);
    
    unit->freq = IN0(0);
    unit->delai_freq = 0.f;
    int neurones = unit->neurones = IN0(10);
    
    if (neurones > 64)
    {
        neurones = unit->neurones = 64;
    }
    else {
        if(neurones < 9)
        {
            neurones = unit->neurones = 9;
        }
    }
    
    // Initialisation des sorties Fhz Amp Dur
    for(int a = 0; a <= 2; ++a)
    {
        unit->neural_out[a] = 0.0;
    }
    // Generation aleatoire
    srand((unsigned)time(0));
    double number;
    for(int a = 0; a < neurones; ++a)
    {
        number = hasard (0, 1000000000);
        number = number / 1000000000;
        unit->neuactiva[a] = 0.0;
        unit->neuerreur[a] = 0.0;
        unit->seuils[a] = number;
        for(int b = 0; b < neurones; ++b)
        {
            number = hasard (0, 1000000000);
            number = number / 1000000000 - 0.5;
            unit->poids[a][b] = number;
        }
    }
				
    OUT0(0) = 0.f;
    OUT0(1) = 0.f;
    OUT0(2) = 0.f;
}

void HPgenetiques3D_Ctor(HPgenetiques3D* unit)
{
    SETCALC(HPgenetiques3D_next_k);
    
    unit->freq = IN0(0);
    int individus = unit->individus = IN0(6);
    unit->delai_freq = 0.f;
    
    if (individus > 64)
    {
        individus = unit->individus = 64;
    }
    else {
        if(individus < 3)
        {
            individus = unit->individus = 3;
        }
    }
    
    // Initialisation des sorties Fhz Amp Dur
    for(int a = 0; a <= 2; ++a)
    {
        unit->genetic_out[a] = 0.0;//Init datas out Fhz Amp Dur
    }
    // Generation population aleatoire
    srand((unsigned)time(0));
    double number;
    for(int i = 0; i < individus; ++i)
    {
        for(int ii = 0; ii <= 2; ++ii)
        {
            number = hasard (0, 1000000000);
            number = number / 1000000000 * 2.0 - 1.0;;
            unit->population[i][ii] = number;//Init 3 datas Fhz Amp Dur
        }
    }
    
    OUT0(0) = 0.f;
    OUT0(1) = 0.f;
    OUT0(2) = 0.f;
}

void HPprobability1_Ctor(HPprobability1* unit)
{
    if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPprobability1_next_a);
    } else
    {
        SETCALC(HPprobability1_next_k);
    }
    
    unit->freq = IN0(0);
    unit->tailleBuffer = IN0(2);// size bufferdatas
    unit->probability_out = 0.f;
    unit->delai_freq = 0.f;
    unit->compteurBuffer = 0;
    
    if (unit->tailleBuffer > 64)
    {
        unit->tailleBuffer = 64;
    }
    else {
        if(unit->tailleBuffer < 1)
        {
            unit->tailleBuffer = 1;
        }
    }
    // Init buffer
    for(int i = 0; i < 64; ++i)
    {
        unit->buffer[i] = 0.f;
    }
    
    OUT0(0) = 0.f;
}

void HPprobability2_Ctor(HPprobability2* unit)
{
    if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPprobability2_next_a);
    } else
    {
        SETCALC(HPprobability2_next_k);
    }
    
    unit->freq = IN0(0);
    unit->tailleBuffer = IN0(2);// size bufferdatas
    unit->probability_out = 0.f;
    unit->lastProbability_out = 0.f;
    unit->delai_freq = 0.f;
    unit->compteurBuffer = 0;
    
    if (unit->tailleBuffer > 64)
    {
        unit->tailleBuffer = 64;
    }
    else {
        if(unit->tailleBuffer < 1)
        {
            unit->tailleBuffer = 1;
        }
    }
    
    //Init hasard
    srand((unsigned)time(0));
    double number;
    number = hasard (0, 1000000000);
    
    // Init buffers
    for(int i = 0; i < 64; ++i)
    {
        unit->buffer[i] = 0.f;
    }
    for(int i = 0; i < 512; ++i)
    {
        unit->bufferInterval[i] = 0.f;
    }
    for(int i = 0; i < 3; ++i)
    {
        unit->bufferDirection[i] = 0.f;
    }
    
    OUT0(0) = 0.f;
}

void HPfractal3D_Ctor(HPfractal3D* unit)
{
    if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPfractal3D_next_a);
    } else
    {
        SETCALC(HPfractal3D_next_k);
    }
    
    unit->freq = IN0(0);
    unit->inF = IN0(1);
    unit->inA = IN0(2);
    unit->inD = IN0(3);
    unit->delai_freq = 0.f;
    
    //Init hasard
    srand((unsigned)time(0));
    double number;
    number = hasard (0, 1000000000);
    
    // Init transformations pour fractals
    for(int t = 0; t < 100; ++t)
    {
        for(int i = 0; i < 12; ++i)
        {
            number = hasard (0, 1000000000);
            number = number / 1000000000 - 0.5;
            unit->transformations [t] [i] = number;
        }
    }
    
    OUT0(0) = 0.f;
    OUT0(1) = 0.f;
    OUT0(2) = 0.f;
}

void HPfftShift_Ctor(HPfftShift* unit)
{
    SETCALC(HPfftShift_next);
    
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;
    }
    
    OUT0(0) = 0.f;
}

void HPfftDiff_Ctor(HPfftDiff* unit)
{
    SETCALC(HPfftDiff_next);
    
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;
        unit->imag2[i] = 0.0;
    }
    
    OUT0(0) = 0.f;
}

void HPfftRand_Ctor(HPfftRand* unit)
{
    SETCALC(HPfftRand_next);
    
    //Init hasard
    srand((unsigned)time(0));
    double number;
    number = hasard (0, 1000000000);
    
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;
        unit->imag2[i] = 0.0;
    }
    
    OUT0(0) = 0.f;
}

void HPfftLog_Ctor(HPfftLog* unit)
{
    SETCALC(HPfftLog_next);
    
    //Init hasard
    srand((unsigned)time(0));
    double number;
    number = hasard (0, 1000000000);
    
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;
    }
    
    OUT0(0) = 0.f;
}

/////////////////////////////////////////////////////////////

void HPchaos_next_a(HPchaos *unit, int inNumSamples)
{
    float *out = OUT(0);
    float freq = IN0(0);
    float chaos = IN0(1);
    float data = unit->data;
    float delai_freq = unit->delai_freq;
    
    if(chaos > 4.0) chaos = 4.0;
    if(chaos < 0.0) chaos = 0.01;
    
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int i=0; i<inNumSamples; ++i)
    {
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            data = chaos * data * (1.0 - data);
        }
        delai_freq++;
        out[i] = data * 2.0 - 1.0; // between [-1, 1] for audio out
    }
    unit->delai_freq = delai_freq;
    unit->data = data;

}

void HPchaos_next_k(HPchaos *unit, int inNumSamples)
{
    float *out = OUT(0);
    float freq = IN0(0);
    float chaos = IN0(1);
    float data = unit->data;
    float delai_freq = unit->delai_freq;
    
    if(chaos > 4.0) chaos = 4.0;
    if(chaos < 0.0) chaos = 0.01;
    
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int i=0; i<inNumSamples; ++i)
    {
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            data = chaos * data * (1.0 - data);
        }
        delai_freq++;
        out[i] = data * 2.0 - 1.0; // between [-1, 1] for audio out
    }
    unit->delai_freq = delai_freq;
    unit->data = data;
    
}

/////////////////////////////////////////////////////////////

void HPneurones_next_a(HPneurones *unit, int inNumSamples)
{
    float *out = OUT(0);
    
    // init parametres neurones
    float fhztime = IN0(0);
    float *datain = IN(1);
    float *dataout = IN(2);
    float mode = IN0(3);
    float apprentissage = IN0(4);
    float temperature = IN0(5);
    int neurones = unit->neurones;//IN0(6)
    int neuin1 = 0;
    int neuin2 = 0;
    int neucouche1 = 1;
    int neucouche2 = neurones - 2;
    int neuout1 = neurones - 1;
    int neuout2 = neurones - 1;
    double activationneurones = 0.0;
    double activationerreurs = 0.0;
    float delai_freq = unit->delai_freq;
    double neural_out = unit->neural_out;
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(fhztime < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(fhztime, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float o = neural_out;
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
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
                    activationneurones=activationneurones+(datain[z] * unit->poids[i][j]);
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
                    unit->neuerreur[i]=(dataout[z] - unit->neuactiva[i])*(1.0-(unit->neuactiva[i]*unit->neuactiva[i]));
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
            
            // Sortie  neural network
            for(int i=neuout1; i<=neuout2; ++i)
            {
                neural_out = unit->neuactiva[i];
            }
        }
        delai_freq++;
        out[z] = o;// sortie neurones [-1,1]
    }
    unit->delai_freq = delai_freq;
    unit->neural_out = neural_out;
}

void HPneurones_next_k(HPneurones *unit, int inNumSamples)
{
    float *out = OUT(0);
    
    // init parametres neurones
    float fhztime = IN0(0);
    float *datain = IN(1);
    float *dataout = IN(2);
    float mode = IN0(3);
    float apprentissage = IN0(4);
    float temperature = IN0(5);
    int neurones = unit->neurones;//IN0(6)
    int neuin1 = 0;
    int neuin2 = 0;
    int neucouche1 = 1;
    int neucouche2 = neurones - 2;
    int neuout1 = neurones - 1;
    int neuout2 = neurones - 1;
    double activationneurones = 0.0;
    double activationerreurs = 0.0;
    float delai_freq = unit->delai_freq;
    double neural_out = unit->neural_out;
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(fhztime < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(fhztime, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float o = neural_out;
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
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
                    activationneurones=activationneurones+(datain[z] * unit->poids[i][j]);
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
                    unit->neuerreur[i]=(dataout[z] - unit->neuactiva[i])*(1.0-(unit->neuactiva[i]*unit->neuactiva[i]));
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
            
            // Sortie  neural network
            for(int i=neuout1; i<=neuout2; ++i)
            {
                neural_out = unit->neuactiva[i];
            }
        }
        delai_freq++;
        out[z] = o;// sortie neurones [-1,1]
    }
    unit->delai_freq = delai_freq;
    unit->neural_out = neural_out;
}

//////////////////////////////////////////////////////////////

void HPgenetiques_next_a(HPgenetiques *unit, int inNumSamples)
{
    float *out = OUT(0);
    
    // init parametres genetiques
    float freq = IN0(0);
    float *datain = IN(1);
    float croisement = unit->croisement = IN0(2);
    float mutation = unit->mutation = IN0(3);
    int individus = unit->individus;//IN0(4)
    float mode = IN0(5);
    float delai_freq = unit->delai_freq;
    double genetic_out = unit->genetic_out;
    int gagnant;
    int perdant;
    double minimum;
    double maximum;
    double difference;
    double differenceall;
    double number;
    double parent1;
    double parent2;
    double enfant1;
    double enfant2;
    double data1;
    double data2;
    double autre;
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float o = genetic_out;
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            
            //Algo genetiques
            // Compute difference entre datain et population
            minimum = 2.0;
            maximum = 0.0;
            differenceall = 0.0;
            for (int i=0; i<individus; ++i)
            {
                difference = datain[z] - unit->population[i];
                differenceall = differenceall + difference;
                difference = fabs(difference);
                if (difference < minimum)
                {
                    gagnant = i;
                    minimum = difference;// set gagnant et minimum
                }
                if (difference > maximum)
                {
                    perdant = i;
                    maximum = difference;// set perdant et maximum
                }
            }
            differenceall = differenceall / individus;// moyenne
            //Croisement entre datain et gagnant
            parent1 = datain[z];
            parent2 = unit->population[gagnant];
            enfant1 = 0.0;
            enfant2 = 0.0;
            //Croisement double = 9 decimales
            for (int i=0;i<9;++i) {
                parent1 = parent1 * 10;// pour prendre l'unite suivante
                parent2 = parent2 * 10;
                data1 = ceil(parent1);// partie entiere seulement
                data2 = ceil(parent2);
                parent1 = parent1 - data1;//update  parent1
                parent2 = parent2 - data2;
                //Croisement
                number = hasard (0, 1000000000);
                number = number / 1000000000;
                if (number <= croisement)
                {
                    enfant1 = enfant1 * 10 + data2;
                    enfant2 = enfant2 * 10 + data1;
                } else
                {
                    enfant1 = enfant1 * 10 + data1;
                    enfant2 = enfant2 * 10 + data2;
                }
                //
            }
            enfant1 = enfant1 / 1000000000;
            enfant2 = enfant2 / 1000000000;
            //Mutation des enfants
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            if( number <= mutation)
            {
                number = hasard (0, differenceall * 1000000000);
                number = number / 1000000000;
                enfant1 = enfant1 + number;
                if (enfant1 > 1.0) enfant1 = enfant1 - 2.0;
                else
                    if (enfant1 < -1.0) enfant1 = enfant1 + 2.0;
                number = hasard (0, differenceall * 1000000000);
                number = number / 1000000000;
                enfant2 = enfant2 + number;
                if (enfant2 > 1.0) enfant2 = enfant2 - 2.0;
                else
                    if (enfant2 < -1.0) enfant2 = enfant2 + 2.0;
            }
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            if( number <= 0.5)
            {
                genetic_out = enfant1;
                autre = parent2;
            } else
            {
                genetic_out = enfant2;
                autre = parent1;
            }
            //Update population
            if (mode > 0.0)
            {
                unit->population[perdant] = genetic_out;
                int number = hasard (0, 63);
                unit->population[number] = autre;
            }
        }
        delai_freq++;
        out[z] = o;// sortie genetic [-1,1]
    }
    
    unit->delai_freq = delai_freq;
    unit->genetic_out = genetic_out;
}

void HPgenetiques_next_k(HPgenetiques *unit, int inNumSamples)
{
    float *out = OUT(0);
    
    // init parametres genetiques
    float freq = IN0(0);
    float *datain = IN(1);
    float croisement = unit->croisement = IN0(2);
    float mutation = unit->mutation = IN0(3);
    int individus = unit->individus;//IN0(4)
    float mode = IN0(5);
    float delai_freq = unit->delai_freq;
    double genetic_out = unit->genetic_out;
    int gagnant;
    int perdant;
    double minimum;
    double maximum;
    double difference;
    double differenceall;
    double number;
    double parent1;
    double parent2;
    double enfant1;
    double enfant2;
    double data1;
    double data2;
    double autre;
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float o = genetic_out;
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            
            //Algo genetiques
            // Compute difference entre datain et population
            minimum = 2.0;
            maximum = 0.0;
            differenceall = 0.0;
            for (int i=0; i<individus; ++i)
            {
                difference = datain[z] - unit->population[i];
                differenceall = differenceall + difference;
                difference = fabs(difference);
                if (difference < minimum)
                {
                    gagnant = i;
                    minimum = difference;// set gagnant et minimum
                }
                if (difference > maximum)
                {
                    perdant = i;
                    maximum = difference;// set perdant et maximum
                }
            }
            differenceall = differenceall / individus;// moyenne
            //Croisement entre datain et gagnant
            parent1 = datain[z];
            parent2 = unit->population[gagnant];
            enfant1 = 0.0;
            enfant2 = 0.0;
            //Croisement double = 9 decimales
            for (int i=0;i<9;++i) {
                parent1 = parent1 * 10;// pour prendre l'unite suivante
                parent2 = parent2 * 10;
                data1 = ceil(parent1);// partie entiere seulement
                data2 = ceil(parent2);
                parent1 = parent1 - data1;//update  parent1
                parent2 = parent2 - data2;
                //Croisement
                number = hasard (0, 1000000000);
                number = number / 1000000000;
                if (number <= croisement)
                {
                    enfant1 = enfant1 * 10 + data2;
                    enfant2 = enfant2 * 10 + data1;
                } else
                {
                    enfant1 = enfant1 * 10 + data1;
                    enfant2 = enfant2 * 10 + data2;
                }
                //
            }
            enfant1 = enfant1 / 1000000000;
            enfant2 = enfant2 / 1000000000;
            //Mutation des enfants
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            if( number <= mutation)
            {
                number = hasard (0, differenceall * 1000000000);
                number = number / 1000000000;
                enfant1 = enfant1 + number;
                if (enfant1 > 1.0) enfant1 = enfant1 - 2.0;
                else
                    if (enfant1 < -1.0) enfant1 = enfant1 + 2.0;
                number = hasard (0, differenceall * 1000000000);
                number = number / 1000000000;
                enfant2 = enfant2 + number;
                if (enfant2 > 1.0) enfant2 = enfant2 - 2.0;
                else
                    if (enfant2 < -1.0) enfant2 = enfant2 + 2.0;
            }
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            if( number <= 0.5)
            {
                genetic_out = enfant1;
                autre = parent2;
            } else
            {
                genetic_out = enfant2;
                autre = parent1;
            }
            //Update population
            if (mode > 0.0)
            {
                unit->population[perdant] = genetic_out;
                int number = hasard (0, 63);
                unit->population[number] = autre;
            }
        }
        delai_freq++;
        out[z] = o;// sortie genetic [-1,1]
    }
    
    unit->delai_freq = delai_freq;
    unit->genetic_out = genetic_out;
}

void HPneurones3D_next_k(HPneurones3D *unit, int inNumSamples)
{
    float *outF = OUT(0);
    float *outA = OUT(1);
    float *outD = OUT(2);
    
    // init parametres neurones
    float freq = IN0(0);
    float *datain [3] = {IN(1), IN(2), IN(3)};// {Fhz, Amp, Dur}
    float *dataout [3] = {IN(4), IN(5), IN(6)};// {Fhz, Amp, Dur}
    float mode = IN0(7);
    float apprentissage = IN0(8);
    float temperature = IN0(9);
    int neurones = unit->neurones;//IN0(10)
    int neuin1 = 0;
    int neuin2 = 2;
    int neucouche1 = 3;
    int neucouche2 = neurones - 4;
    int neuout1 = neurones - 3;
    int neuout2 = neurones - 1;
    double activationneurones = 0.0;
    double activationerreurs = 0.0;
    float delai_freq = unit->delai_freq;
    double neural_out[3];
    
    for(int a = 0; a <= 2; ++a)
    {
        neural_out[a] = unit->neural_out[a];
    }
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float of = unit->neural_out[0];
        float oa = unit->neural_out[1];
        float od = unit->neural_out[2];
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            
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
                    activationneurones=activationneurones+(datain[i][z] * unit->poids[i][j]);
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
                    unit->neuerreur[i]=(dataout[i - neuout1][z] - unit->neuactiva[i])*(1.0-(unit->neuactiva[i]*unit->neuactiva[i]));
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
                } ;
            };
            
            // Sortie  neural network
            for(int i=neuout1; i<=neuout2; ++i)
            {
                neural_out[i - neuout1] = unit->neuactiva[i];
            }
        }
        delai_freq++;
        outF[z] = of;// sortie neurones Fhz [-1,1]
        outA[z] = oa;// sortie neurones Amp [-1,1]
        outD[z] = od;// sortie neurones Dur [-1,1]
    }
    
    unit->delai_freq = delai_freq;
    // Stockage Fhz Amp Dur
    for(int a = 0; a <= 2; ++a)
    {
        unit->neural_out[a] = neural_out[a];
    }
}

void HPgenetiques3D_next_k(HPgenetiques3D *unit, int inNumSamples)
{
    float *outF = OUT(0);
    float *outA = OUT(1);
    float *outD = OUT(2);
    
    // init parametres genetiques
    float freq = IN0(0);
    float *datain [3] = {IN(1), IN(2), IN(3)};// {Fhz, Amp, Dur}
    float croisement = unit->croisement = IN0(4);
    float mutation = unit->mutation = IN0(5);
    int individus = unit->individus;//IN0(6)
    float mode = IN0(7);
    float delai_freq = unit->delai_freq;
    int gagnant;
    int perdant;
    double number;
    double parent1[3];
    double parent2[3];
    double enfant1[3];
    double enfant2[3];
    double genetic_out[3];
    
    for (int a = 0; a <= 2; ++a)
    {
        genetic_out[a] = unit->genetic_out[a];// Get Fhz Amp Dur
    }
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float of = genetic_out[0];
        float oa = genetic_out[1];
        float od = genetic_out[2];
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            
            //Algo genetiques
            // Compute difference entre datain et population
            double minimum = 2.0;
            double maximum = 0.0;
            double difference = 0.0;
            double differenceall = 0.0;
            for (int i=0; i<individus; ++i)
            {
                difference = 0.0;
                for (int ii=0; ii<=2; ++ii)
                {
                    difference = difference + pow((datain[ii][z] - unit->population[i][ii]), 2.0);
                }
                difference = sqrt(difference);
                differenceall = differenceall + difference;
                if(difference <= minimum)
                {
                    gagnant = i;//set gagnant
                    minimum = difference;//set minimum
                }
                if(difference > maximum)
                {
                    perdant = i;//set perdant
                    maximum = difference;//set maximum
                }
            }
            differenceall = differenceall / individus;//Moyenne
            //Croisement et mutation entre datain et gagnant
            for (int p=0; p<=2; ++p)
            {
                parent1[p] = datain[p][z];
                parent2[p] = unit->population[gagnant][p];
                enfant1[p] = 0.0;
                enfant2[p] = 0.0;
                float signe = 1.0;
                //Croisement et mutation
                number = hasard (0, 1000000000);
                number = number / 1000000000;
                if (number <= croisement)
                {
                    number = hasard (0, 1000000000);
                    number = number / 1000000000;
                    if (number <= mutation)
                    {
                        number = hasard (0, differenceall * 1000000000);
                        number = number / 1000000000;
                        signe = hasard (0, 2);
                        signe = signe - 1.0;
                        if(signe == 0.0) signe = 1.0;
                        enfant1[p] = parent2[p] + (number * signe);
                        if (enfant1[p] > 1.0) enfant1[p] = enfant1[p] - 2.0;
                        else
                            if (enfant1[p] < -1.0) enfant1[p] = enfant1[p] + 2.0;
                        number = hasard (0, differenceall * 1000000000);
                        number = number / 1000000000;
                        signe = hasard (0, 2);
                        signe = signe - 1.0;
                        if(signe == 0.0) signe = 1.0;
                        enfant2[p] = parent1[p] + (number * signe);
                        if (enfant2[p] > 1.0) enfant2[p] = enfant2[p] - 2.0;
                        else
                            if (enfant2[p] < -1.0) enfant2[p] = enfant2[p] + 2.0;
                    } else
                    {
                        enfant1[p] = parent2[p];
                        enfant2[p] = parent1[p];
                    }
                } else
                {
                    number = hasard (0, 1000000000);
                    number = number / 1000000000;
                    if (number <= mutation)
                    {
                        number = hasard (0, differenceall * 1000000000);
                        number = number / 1000000000;
                        signe = hasard (0, 2);
                        signe = signe - 1.0;
                        if(signe == 0.0) signe = 1.0;
                        enfant1[p] = parent1[p] + (number * signe);
                        if (enfant1[p] > 1.0) enfant1[p] = enfant1[p] - 2.0;
                        else
                            if (enfant1[p] < -1.0) enfant1[p] = enfant1[p] + 2.0;
                        number = hasard (0, differenceall * 1000000000);
                        number = number / 1000000000;
                        signe = hasard (0, 2);
                        signe = signe - 1.0;
                        if(signe == 0.0) signe = 1.0;
                        enfant2[p] = parent2[p] + (number * signe);
                        if (enfant2[p] > 1.0) enfant2[p] = enfant2[p] - 2.0;
                        else
                            if (enfant2[p] < -1.0) enfant2[p] = enfant2[p] + 2.0;
                    } else
                    {
                        enfant1[p] = parent1[p];
                        enfant2[p] = parent2[p];
                    }
                }
            }
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            if (number < 0.5)
            {
                for (int p=0 ; p < 2 ; ++p)
                {
                    genetic_out[p] = enfant1[p];
                }
            } else
            {
                for (int p=0 ; p < 2 ; ++p)
                {
                    genetic_out[p] = enfant2[p];
                }
            }
            //Update population
            if (mode > 0.0)
            {
                for(int p=0; p<=2; ++p)
                {
                    unit->population[perdant][p] = genetic_out[p];
                }
            }
        }
        delai_freq++;
        outF[z] = of;// sortie genetic Fhz [-1,1]
        outA[z] = oa;// sortie genetic Amp [-1,1]
        outD[z] = od;// sortie genetic Dur [-1,1]
    }
    
    unit->delai_freq = delai_freq;
    // Stockage Fhz Amp Dur
    for(int a = 0; a <= 2; ++a)
    {
        unit->genetic_out[a] = genetic_out[a];
    }
}

void HPprobability1_next_k(HPprobability1 *unit, int inNumSamples)
{
    float *out = OUT(0);
    // init parametres
    float freq = IN0(0);
    float *datain = IN(1);
    int tailleBuffer = unit->tailleBuffer = IN0(2);
    float delai_freq = unit->delai_freq;
    float probability_out = unit->probability_out;
    int compteurBuffer = unit->compteurBuffer;
    float moyenne = 0.f;
    float variance = 0.f;
    float somme = 0.0;
    float ecartType = 0.f;
    
    if (tailleBuffer > 64)
    {
        tailleBuffer = 64;
    }
    else {
        if(tailleBuffer < 1)
        {
            tailleBuffer = 1;
        }
    }
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int i=0; i<inNumSamples; ++i)
    {
        float o = probability_out;
        if (delai_freq >= samplesPerCycle)
        {
            // Update buffer avec datain
            unit->buffer[compteurBuffer] = datain[i];
            compteurBuffer = compteurBuffer + 1;
            if (compteurBuffer >= tailleBuffer)
            {
                compteurBuffer = 0;
            }
            
            // Calcule de la moyenne
            moyenne = 0.f;
            for (int z=0; z<tailleBuffer; ++z)
            {
                moyenne = moyenne + unit->buffer[z];
            }
            moyenne = moyenne / tailleBuffer;
            // calcul de la variance
            somme = 0.0;
            for (int z=0; z<tailleBuffer; ++z)
            {
                somme = somme + pow((unit->buffer[z] - moyenne), 2.0);
            }
            variance = somme / tailleBuffer;
            // Calcule ecartType
            ecartType = sqrt(variance);
            if (ecartType != 0.0)
            {
                probability_out = (probability_out - moyenne) / ecartType;
            } else
            {
                probability_out = 0.f;
            }
            if (probability_out > 1.0)
            {
                probability_out = probability_out - floor(probability_out);
            }
            else
            {if (probability_out < -1.0)
                probability_out = probability_out - ceil(probability_out);
            }
            delai_freq -= samplesPerCycle;
        }
        delai_freq++;
        out[i] = o; // between [-1, 1] for audio out
    }
    unit->delai_freq = delai_freq;
    unit->probability_out = probability_out;
    unit->compteurBuffer = compteurBuffer;
    unit->tailleBuffer = tailleBuffer;
}

void HPprobability1_next_a(HPprobability1 *unit, int inNumSamples)
{
    float *out = OUT(0);
    // init parametres
    float freq = IN0(0);
    float *datain = IN(1);
    int tailleBuffer = unit->tailleBuffer = IN0(2);
    float delai_freq = unit->delai_freq;
    float probability_out = unit->probability_out;
    int compteurBuffer = unit->compteurBuffer;
    float moyenne = 0.f;
    float variance = 0.f;
    float somme = 0.0;
    float ecartType = 0.f;
    
    if (tailleBuffer > 64)
    {
        tailleBuffer = 64;
    }
    else {
        if(tailleBuffer < 1)
        {
            tailleBuffer = 1;
        }
    }
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int i=0; i<inNumSamples; ++i)
    {
        float o = probability_out;
        if (delai_freq >= samplesPerCycle)
        {
            // Update buffer avec datain
            unit->buffer[compteurBuffer] = datain[i];
            compteurBuffer = compteurBuffer + 1;
            if (compteurBuffer >= tailleBuffer)
            {
                compteurBuffer = 0;
            }
            
            // Calcule de la moyenne
            moyenne = 0.f;
            for (int z=0; z<tailleBuffer; ++z)
            {
                moyenne = moyenne + unit->buffer[z];
            }
            moyenne = moyenne / tailleBuffer;
            // calcule de la variance
            somme = 0.0;
            for (int z=0; z<tailleBuffer; ++z)
            {
                somme = somme + pow((unit->buffer[z] - moyenne), 2.0);
            }
            variance = somme / tailleBuffer;
            // Calcule ecartType
            ecartType = sqrt(variance);
            if (ecartType != 0.0)
            {
                probability_out = (probability_out - moyenne) / ecartType;
            } else
            {
                probability_out = 0.f;
            }
            if (probability_out > 1.0)
            {
                probability_out = probability_out - floor(probability_out);
            }
            else
            {if (probability_out < -1.0)
                probability_out = probability_out - ceil(probability_out);
            }
            delai_freq -= samplesPerCycle;
        }
        delai_freq++;
        out[i] = o; // between [-1, 1] for audio out
    }
    unit->delai_freq = delai_freq;
    unit->probability_out = probability_out;
    unit->compteurBuffer = compteurBuffer;
    unit->tailleBuffer = tailleBuffer;
}

void HPprobability2_next_k(HPprobability2 *unit, int inNumSamples)
{
    float *out = OUT(0);
    // init parametres
    float freq = IN0(0);
    float *datain = IN(1);
    int tailleBuffer = unit->tailleBuffer = IN0(2);
    float delai_freq = unit->delai_freq;
    float probability_out = unit->probability_out;
    float lastProbability_out = unit->lastProbability_out;
    int compteurBuffer = unit->compteurBuffer;
    float difference = 0.0;
    float somme = 0.f;
    float number;
    int indexInterval;
    int indexDirection;
    float newInterval = 0.0;
    float newDirection = 0.0;
    bool condition = true;
    int position;
    
    if (tailleBuffer > 64)
    {
        tailleBuffer = 64;
    }
    else {
        if(tailleBuffer < 1)
        {
            tailleBuffer = 1;
        }
    }
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float o = probability_out;
        if (delai_freq >= samplesPerCycle)
        {
            //Init buffer interval et direction
            for(int i = 0; i < 512; ++i)
            {
                unit->bufferInterval[i] = 0.f;
            }
            for(int i = 0; i < 3; ++i)
            {
                unit->bufferDirection[i] = 0.f;
            }
            // Update buffer avec datain
            unit->buffer[compteurBuffer] = datain[z];
            compteurBuffer = compteurBuffer + 1;
            if (compteurBuffer >= tailleBuffer)
            {
                compteurBuffer = 0;
            }
            //Update buffer intervals et direction
            for (int i=1; i<tailleBuffer; ++i)
            {
                difference = unit->buffer[i - 1] - unit->buffer[i];// pour signe
                if(difference == 0.0)
                {
                    unit->bufferDirection[1] = unit->bufferDirection[1] + 1.0;
                }
                else
                {
                    if(difference < 0.0)
                    {
                        unit->bufferDirection[0] = unit->bufferDirection[0] + 1.0;
                    }
                    else
                    {
                        unit->bufferDirection[2] = unit->bufferDirection[2] + 1.0;
                    }
                }
                difference = fabs(unit->buffer[i - 1]) - fabs(unit->buffer[i]);//Good value
                position = ceil(fabs(difference * 512.0));
                unit->bufferInterval[position] = unit->bufferInterval[position] + 1.0;
            }
            //Normalise buffer intervals et direction
            for (int i=0; i<512; ++i)
            {
                somme = somme + unit->bufferInterval[i];
            }
            for (int i=0; i<512; ++i)
            {
                unit->bufferInterval[i] = unit->bufferInterval[i] / somme;
            }
            somme = 0.f;
            for (int i=0; i<3; ++i)
            {
                somme = somme + unit->bufferDirection[i];
            }
            for (int i=0; i<3; ++i)
            {
                unit->bufferDirection[i] = unit->bufferDirection[i] / somme;
            }
            // New interval
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            somme = 0.0;
            condition = true;
            indexInterval = 0.0;
            while (condition == true)
            {
                somme = somme + unit->bufferInterval[indexInterval];
                if (somme >= number)
                {
                    condition = false;
                }
                else
                {
                    indexInterval = indexInterval + 1;
                }
                if (indexInterval >= 512)
                {
                    condition = false;
                }
            }
            newInterval = indexInterval / 512.0;
            // New direction
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            somme = 0.0;
            bool condition = true;
            indexDirection = 0.0;
            while (condition == true)
            {
                somme = somme + unit->bufferDirection[indexDirection];
                if (somme >= number)
                {
                    condition = false;
                }
                else
                {
                    indexDirection = indexDirection + 1;
                }
                if (indexDirection >= 3)
                {
                    condition = false;
                }
            }
            newDirection = indexDirection - 1.0;
            //New data
            probability_out = (lastProbability_out + (newInterval * newDirection)) * newDirection;
            if (probability_out > 1.0)
            {
                probability_out = probability_out - floor(probability_out);
            }
            else
            {if (probability_out < -1.0)
                probability_out = probability_out - ceil(probability_out);
            }
            delai_freq -= samplesPerCycle;
        }
        delai_freq++;
        out[z] = o; // between [-1, 1] for audio out
    }
    unit->delai_freq = delai_freq;
    unit->probability_out = probability_out;
    unit->lastProbability_out = probability_out;
    unit->compteurBuffer = compteurBuffer;
    unit->tailleBuffer = tailleBuffer;
}

void HPprobability2_next_a(HPprobability2 *unit, int inNumSamples)
{
    float *out = OUT(0);
    // init parametres
    float freq = IN0(0);
    float *datain = IN(1);
    int tailleBuffer = unit->tailleBuffer = IN0(2);
    float delai_freq = unit->delai_freq;
    float probability_out = unit->probability_out;
    float lastProbability_out = unit->lastProbability_out;
    int compteurBuffer = unit->compteurBuffer;
    float difference = 0.0;
    float somme = 0.f;
    float number;
    int indexInterval;
    int indexDirection;
    float newInterval = 0.0;
    float newDirection = 0.0;
    bool condition = true;
    int position;
    
    if (tailleBuffer > 64)
    {
        tailleBuffer = 64;
    }
    else {
        if(tailleBuffer < 1)
        {
            tailleBuffer = 1;
        }
    }
    
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        float o = probability_out;
        if (delai_freq >= samplesPerCycle)
        {
            //Init buffer interval et direction
            for(int i = 0; i < 512; ++i)
            {
                unit->bufferInterval[i] = 0.f;
            }
            for(int i = 0; i < 3; ++i)
            {
                unit->bufferDirection[i] = 0.f;
            }
            // Update buffer avec datain
            unit->buffer[compteurBuffer] = datain[z];
            compteurBuffer = compteurBuffer + 1;
            if (compteurBuffer >= tailleBuffer)
            {
                compteurBuffer = 0;
            }
            //Update buffer intervals et direction
            for (int i=1; i<tailleBuffer; ++i)
            {
                difference = unit->buffer[i - 1] - unit->buffer[i];// pour signe
                if(difference == 0.0)
                {
                    unit->bufferDirection[1] = unit->bufferDirection[1] + 1.0;
                }
                else
                {
                    if(difference < 0.0)
                    {
                        unit->bufferDirection[0] = unit->bufferDirection[0] + 1.0;
                    }
                    else
                    {
                        unit->bufferDirection[2] = unit->bufferDirection[2] + 1.0;
                    }
                }
                difference = fabs(unit->buffer[i - 1]) - fabs(unit->buffer[i]);//Good value
                position = ceil(fabs(difference * 512.0));
                unit->bufferInterval[position] = unit->bufferInterval[position] + 1.0;
            }
            //Normalise buffer intervals et direction
            for (int i=0; i<512; ++i)
            {
                somme = somme + unit->bufferInterval[i];
            }
            for (int i=0; i<512; ++i)
            {
                unit->bufferInterval[i] = unit->bufferInterval[i] / somme;
            }
            somme = 0.f;
            for (int i=0; i<3; ++i)
            {
                somme = somme + unit->bufferDirection[i];
            }
            for (int i=0; i<3; ++i)
            {
                unit->bufferDirection[i] = unit->bufferDirection[i] / somme;
            }
            // New interval
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            somme = 0.0;
            condition = true;
            indexInterval = 0.0;
            while (condition == true)
            {
                somme = somme + unit->bufferInterval[indexInterval];
                if (somme >= number)
                {
                    condition = false;
                }
                else
                {
                    indexInterval = indexInterval + 1;
                }
                if (indexInterval >= 512)
                {
                    condition = false;
                }
            }
            newInterval = indexInterval / 512.0;
            // New direction
            number = hasard (0, 1000000000);
            number = number / 1000000000;
            somme = 0.0;
            bool condition = true;
            indexDirection = 0.0;
            while (condition == true)
            {
                somme = somme + unit->bufferDirection[indexDirection];
                if (somme >= number)
                {
                    condition = false;
                }
                else
                {
                    indexDirection = indexDirection + 1;
                }
                if (indexDirection >= 3)
                {
                    condition = false;
                }				
            }
            newDirection = indexDirection - 1.0;
            //New data
            probability_out = (lastProbability_out + (newInterval * newDirection)) * newDirection;
            if (probability_out > 1.0)
            {
                probability_out = probability_out - floor(probability_out);
            }
            else
            {if (probability_out < -1.0)
                probability_out = probability_out - ceil(probability_out);
            }
            delai_freq -= samplesPerCycle;
        }
        delai_freq++;
        out[z] = o; // between [-1, 1] for audio out
    }
    unit->delai_freq = delai_freq;
    unit->probability_out = probability_out;
    unit->lastProbability_out = probability_out;
    unit->compteurBuffer = compteurBuffer;
    unit->tailleBuffer = tailleBuffer;
}

void HPfractal3D_next_k(HPfractal3D *unit, int inNumSamples)
{
    float *outF = OUT(0);
    float *outA = OUT(1);
    float *outD = OUT(2);
    
    // init parametres fractal
    float freq = IN0(0);
    double inF = unit->inF;
    double inA = unit->inA;
    double inD = unit->inD;
    int nombreTransformations = IN0(4);
    float mode = IN0(5);
    float delai_freq = unit->delai_freq;
    double transformation [12];
    double number;
    
    if (nombreTransformations > 100) 
    {
        nombreTransformations = 100;
    }
    else {
        if(nombreTransformations < 1)
        {
            nombreTransformations = 1;
        }
    }
    
    // Init new fractals
    if(mode > 0.0)
    {
        for(int t = 0; t < 100; ++t)
        {
            for(int i = 0; i < 12; ++i)
            {
                number = hasard (0, 1000000000);
                number = number / 1000000000 - 0.5;
                unit->transformations [t] [i] = number;
            }
        }
    }
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        double oF = inF;
        double oA = inA;
        double oD = inD;
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            //Choix de la transformation
            int number;
            number = hasard (0, nombreTransformations - 1);
            for(int i = 0; i < 12; ++i)
            {
                transformation[i] = unit->transformations [number] [i];
            };
            
            //Algo fractal
            double f = (transformation[0] * inF) + (transformation[1] * inA) + (transformation[2] * inD) + transformation[3] ;
            double a = (transformation[4] * inF) + (transformation[5] * inA) + (transformation[6] * inD) + transformation[7] ;
            double d = (transformation[8] * inF) + (transformation[9] * inA) + (transformation[10] * inD) + transformation[11] ;
            inF = f;
            inA = a;
            inD = d; 
        }
        delai_freq++;
        outF[z] = oF;// sortie fractal Fhz [-1,1]
        outA[z] = oA;// sortie fractal Amp [-1,1]
        outD[z] = oD;// sortie fractal Dur [-1,1]
    }
    
    unit->delai_freq = delai_freq;
    unit->inF = inF;
    unit->inA = inA;
    unit->inD = inD;
}

void HPfractal3D_next_a(HPfractal3D *unit, int inNumSamples)
{
    float *outF = OUT(0);
    float *outA = OUT(1);
    float *outD = OUT(2);
    
    // init parametres fractal
    float freq = IN0(0);
    double inF = unit->inF;
    double inA = unit->inA;
    double inD = unit->inD;
    int nombreTransformations = IN0(4);
    float mode = IN0(5);
    float delai_freq = unit->delai_freq;
    double transformation [12];
    double number;
    
    if (nombreTransformations > 100) 
    {
        nombreTransformations = 100;
    }
    else {
        if(nombreTransformations < 1)
        {
            nombreTransformations = 1;
        }
    }
    
    // Init new fractals
    if(mode > 0.0)
    {
        for(int t = 0; t < 100; ++t)
        {
            for(int i = 0; i < 12; ++i)
            {
                number = hasard (0, 1000000000);
                number = number / 1000000000 - 0.5;
                unit->transformations [t] [i] = number;
            }
        }
    }
    // set FHZ de calcule de l'algo
    float samplesPerCycle;
    if(freq < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(freq, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        double oF = inF;
        double oA = inA;
        double oD = inD;
        if(delai_freq >= samplesPerCycle)
        {
            delai_freq -= samplesPerCycle;
            //Choix de la transformation
            int number;
            number = hasard (0, nombreTransformations - 1);
            for(int i = 0; i < 12; ++i)
            {
                transformation[i] = unit->transformations [number] [i];
            };
            
            //Algo fractal
            double f = (transformation[0] * inF) + (transformation[1] * inA) + (transformation[2] * inD) + transformation[3] ;
            double a = (transformation[4] * inF) + (transformation[5] * inA) + (transformation[6] * inD) + transformation[7] ;
            double d = (transformation[8] * inF) + (transformation[9] * inA) + (transformation[10] * inD) + transformation[11] ;
            inF = f;
            inA = a;
            inD = d; 
        }
        delai_freq++;
        outF[z] = oF;// sortie fractal Fhz [-1,1]
        outA[z] = oA;// sortie fractal Amp [-1,1]
        outD[z] = oD;// sortie fractal Dur [-1,1]
    }
    
    unit->delai_freq = delai_freq;
    unit->inF = inF;
    unit->inA = inA;
    unit->inD = inD;
}

void HPfftShift_next(HPfftShift *unit, int inNumSamples)
{
    float *out = OUT(0);
    float *reel1 = IN(0);
    int index = IN0(1);
    
    if(index < 0) index = 0;
    if(index >= 64) index = 63;
    
    fft(reel1, unit->imag1, 6, +1);// Calcul de la fft in1
    
    // Transforme spectre
    for (int i=0, j=index; i<64; ++i, j=(j + 1)%64)
    {
        unit->reel[i] = reel1[j];
        unit->imag[i] = unit->imag1[j];
    }
    
    fft(unit->reel, unit->imag, 6, -1);// Calcul de la fft inverse
    
    
    // Sortie play FFT
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;// Init imag pour new calcul
        out[i] = unit->reel[i];
    }
}

void HPfftDiff_next(HPfftDiff *unit, int inNumSamples)
{
    float *out = OUT(0);
    float *reel1 = IN(0);
    float *reel2 = IN(1);
    float vecteur, vecteur1, vecteur2, angle, angle1, angle2;
    
    fft(reel1, unit->imag1, 6, +1);// Calcul de la fft in1
    fft(reel2, unit->imag2, 6, +1);// Calcul de la fft in2
    
    // Transforme spectre
    for (int i=0; i<64; ++i)
    {
        vecteur1 = sqrt((reel1[i]*reel1[i]) + (unit->imag1[i]*unit->imag1[i]));
        vecteur2 = sqrt((reel2[i]*reel2[i]) + (unit->imag2[i]*unit->imag2[i]));
        
        angle1 = atan(unit->imag1[i] / reel1[i]);
        angle2 = atan(unit->imag2[i] / reel2[i]);
        
        vecteur = vecteur1 - vecteur2;
        angle = angle1 - angle2;
        
        unit->reel[i] = vecteur * cos(angle);
        unit->imag[i] = vecteur * sin(angle);
    }
    
    fft(unit->reel, unit->imag, 6, -1);// Calcul de la fft inverse
    
    
    // Sortie play FFT
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;// Init imag pour new calcul
        unit->imag2[i] = 0.0;
        out[i] = unit->reel[i];
    }
}

void HPfftRand_next(HPfftRand *unit, int inNumSamples)
{
    float *out = OUT(0);
    float *reel1 = IN(0);
    float *reel2 = IN(1);
    float vecteur, vecteur1, vecteur2, angle, angle1, angle2;
    
    fft(reel1, unit->imag1, 6, +1);// Calcul de la fft in1
    fft(reel2, unit->imag2, 6, +1);// Calcul de la fft in2
    
    // Transforme spectre
    for (int i=0; i<64; ++i)
    {
        vecteur1 = sqrt((reel1[i]*reel1[i]) + (unit->imag1[i]*unit->imag1[i]));
        vecteur2 = sqrt((reel2[i]*reel2[i]) + (unit->imag2[i]*unit->imag2[i]));
        
        angle1 = atan(unit->imag1[i] / reel1[i]);
        angle2 = atan(unit->imag2[i] / reel2[i]);
        
        vecteur = hasard(vecteur1 * 1000000, vecteur2 * 1000000);
        vecteur = vecteur / 1000000;
        angle = hasard(angle1 * 1000000, angle2 * 1000000);
        angle = angle / 1000000;
        
        unit->reel[i] = vecteur * cos(angle);
        unit->imag[i] = vecteur * sin(angle);
    }
    
    fft(unit->reel, unit->imag, 6, -1);// Calcul de la fft inverse
    
    
    // Sortie play FFT
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;// Init imag pour new calcul
        unit->imag2[i] = 0.0;
        out[i] = unit->reel[i];
    }
}

void HPfftLog_next(HPfftLog *unit, int inNumSamples)
{
    float *out = OUT(0);
    float *reel1 = IN(0);
    
    fft(reel1, unit->imag1, 6, +1);// Calcul de la fft in1
    
    // Transforme spectre
    for (int i=0; i<64; ++i)
    {
        unit->reel[i] = log(reel1[i]);
        unit->imag[i] = log(unit->imag1[i]);
    }
    
    fft(unit->reel, unit->imag, 6, -1);// Calcul de la fft inverse
    
    
    // Sortie play FFT
    for (int i=0; i<64; ++i)
    {
        unit->imag1[i] = 0.0;// Init imag pour new calcul
        out[i] = unit->reel[i];
    }
}

//////////////////////////////////////////////////////////////

PluginLoad(HPUGens)
{
    ft = inTable;
    
    DefineSimpleUnit(HPchaos);
    DefineSimpleUnit(HPneurones);
    DefineSimpleUnit(HPgenetiques);
    DefineSimpleUnit(HPneurones3D);
    DefineSimpleUnit(HPgenetiques3D);
    DefineSimpleUnit(HPprobability1);
    DefineSimpleUnit(HPprobability2);
    DefineSimpleUnit(HPfractal3D);
    DefineSimpleUnit(HPfftShift);
    DefineSimpleUnit(HPfftDiff);
    DefineSimpleUnit(HPfftRand);
    DefineSimpleUnit(HPfftLog);
}

//////////////////////////////////////////////////////////////

//Fonction hasard
int hasard (int min, int max)
{
    return (int) (min + ((float) rand() / RAND_MAX * (max - min + 1)));
}

// FFT Algorithme

//reel[] et imag[i] sont la liste des réelles et des imaginaires
// sign = 1 donne la transformée de Fourier
// sign = -1 donne la transformée de Fourier inverse

void fft(float *reel, float *imag, int log2n, int sign) {
    
    int n, m, m2, i, j, k, l;
    float c1, c2, norm, norm2, cphi, sphi;
    
    n = 1<<log2n;
    
    /* Inversement des bits */
    for(i=0; i<n; i++) {
        
        for(j=log2n-1, m=0, k=i; j>=0; j--, k>>=1) m += (k&1)<<j;
        
        if(m>i) {
            swap(reel[i],reel[m]);
            swap(imag[i],imag[m]);
        }
    }
    
    /* normalisation de la transformée de Fourier */
    norm = 1.0/sqrt((float)n);
    for(i=0; i<n ;i++) {
        reel[i] *= norm;
        imag[i] *= norm;
    }
    
    /* calcul de la FFT */
    for(j=0; j < log2n; j++) {
        m = 1<<j; m2 = 2*m;
        c1 = 1.0;
        c2 = 0.0;
        cphi = cos(sign*2.0*M_PI/((float)m2));
        sphi = sin(sign*2.0*M_PI/((float)m2));
        for(k=0; k<m; k++) {
            for(i=k; i<n; i+=m2) {
                l = i + m;
                norm = c1*reel[l] - c2*imag[l];
                norm2 = c1*imag[l] + c2*reel[l];
                reel[l] = reel[i] - norm;
                imag[l] = imag[i] - norm2;
                reel[i] += norm;
                imag[i] += norm2;
            }
            norm = c1*cphi - c2*sphi; // Calcul de exp(2 pi k/m) avec
            norm2 = c1*sphi + c2*cphi; // le théorème d'addition
            c1 = norm; c2 = norm2;
        }
    }
    //return 0.0;
}

