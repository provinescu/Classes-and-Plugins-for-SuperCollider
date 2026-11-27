//
//  HPkohonen.cpp
//  
//
//  Created by h p on 07.04.16.
//
//

#include "SC_PlugIn.h"
#include <limits>
#include <string.h>
#include <cmath>
#include <cstdlib>
#include <ctime>

#define PI 3.14159265358979323846

static InterfaceTable *ft;

//////////////////////////////////////////////////////////////

struct HPkohonen3 : public Unit
{
    float outKohonen;
    float delai_freq;
    float matriceNode [128];
    float vecteurNode [128];
    float weightNode [128] [3];
};

struct HPkohonen1 : public Unit
{
    float outKohonen;
    float delai_freq;
    float matriceNode [513];
    float vecteurNode [513];
    float weightNode [513];
};

////////////////////////////////////////////////////////////

extern "C"
{
    void load(InterfaceTable *inTable);
    
    void HPkohonen3_next_a(HPkohonen3 *unit, int inNumSamples);
    void HPkohonen3_next_k(HPkohonen3 *unit, int inNumSamples);
    void HPkohonen3_Ctor(HPkohonen3* unit);
    void HPkohonen1_next_a(HPkohonen1 *unit, int inNumSamples);
    void HPkohonen1_next_k(HPkohonen1 *unit, int inNumSamples);
    void HPkohonen1_Ctor(HPkohonen1* unit);
};

/////////////////////////////////////////////////////////////////

void HPkohonen3_Ctor(HPkohonen3* unit)
{
   if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPkohonen3_next_a);
    } else
        {
            SETCALC(HPkohonen3_next_k);
        }
    
    unit->delai_freq = 0.f;
    unit->outKohonen = 0.f;
    
    RGen & rgen = *unit->mParent->mRGen; // pour Hasard
    
    //Init hasard
    srand((unsigned)time(0));
    float number;
    
    // Init Kohonen
    for(int t = 0; t <= 127; ++t)
    {
        unit->matriceNode [t] = t; // position dans la matrice
        unit->vecteurNode [t] = sqrt(pow(t, 2));// vecteur position
        
        for(int i = 0; i < 3; ++i)
        {
            number = rgen.frand();
            unit->weightNode[t] [i] = number;
        }
    }
    
    OUT0(0) = 0.f;
}

void HPkohonen1_Ctor(HPkohonen1* unit)
{
    if (INRATE(0) == calc_FullRate)
    {
        SETCALC(HPkohonen1_next_a);
    } else
    {
        SETCALC(HPkohonen1_next_k);
    }
    
    unit->delai_freq = 0.f;
    unit->outKohonen = 0.f;
    
    RGen & rgen = *unit->mParent->mRGen; // pour Hasard
    
    //Init hasard
    srand((unsigned)time(0));
    float number;
    
    // Init Kohonen
    for(int t = 0; t <= 512; ++t)
    {
        unit->matriceNode [t] = t; // position dans la matrice
        unit->vecteurNode [t] = sqrt(pow(t, 2));// vecteur position
        number = rgen.frand();
        unit->weightNode[t] = number;
    }
    
    OUT0(0) = 0.f;
}

/////////////////////////////////////////////////////////////

void HPkohonen3_next_k(HPkohonen3 *unit, int inNumSamples)
{
    float *out = OUT(0);

    float rate = IN0(0);
    float *data1 = IN(1);
    float *data2 = IN(2);
    float *data3 = IN(3);
    float learningRate = IN0(4);
    float distanceNeighbourhood = IN0(5);
    float gate = IN0(6);
    float delai_freq = unit->delai_freq;
    float outKohonen = unit->outKohonen;
    
    // set FHZ de calcul de l'algo
    float samplesPerCycle;
    if(rate < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(rate, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {

        if(delai_freq >= samplesPerCycle)

        {
            delai_freq -= samplesPerCycle;
            
            // Kohonen Algorithme
            
            float distance = 999999;
            float arrayDistance [128];
            float deltaRate = 1;
            float d = 0;
            float indexNodeGagnant = 0;
            float distanceNodeGagnant = 0;
            float distanceVoisin = 0;
            
            for(int i=0; i<=127; ++i)
            {
                d = 0;
                d = pow(d + ((data1[z] + 1.0 / 2.0) - unit->weightNode [i] [0]), 2);
                d = pow(d + ((data2[z] + 1.0 / 2.0) - unit->weightNode [i] [1]), 2);
                d = pow(d + ((data3[z] + 1.0 / 2.0) - unit->weightNode [i] [2]), 2);
                d = sqrt(d);
                arrayDistance [i] = d;
                if(d < distance) {distance = d; indexNodeGagnant = i; distanceNodeGagnant = unit->vecteurNode [i];};
            }

        if(gate == 1)
        {
            for(int i=0; i<=127; ++i)
            {
                distanceVoisin = fabs(distance - unit->vecteurNode [i]);
                if(distanceVoisin <= distanceNeighbourhood)
                {
                    deltaRate = exp( -1 * (pow(distanceVoisin, 2) / pow(distanceNeighbourhood, 2)));
                    unit->weightNode [i] [0] = unit->weightNode [i] [0] + (deltaRate * learningRate * ((data1[z] + 1.0 / 2.0) - unit->weightNode [i] [0]));
                    unit->weightNode [i] [1] = unit->weightNode [i] [1] + (deltaRate * learningRate * ((data2[z] + 1.0 / 2.0) - unit->weightNode [i] [1]));
                    unit->weightNode [i] [2] = unit->weightNode [i] [2] + (deltaRate * learningRate * ((data3[z] + 1.0 / 2.0) - unit->weightNode [i] [2]));
                }
            }
        }
            else
            {
                for(int i=0; i<=127; ++i)
                {
                    //indexNodeGagnant -> winner
                }
            }
            outKohonen = indexNodeGagnant / 127;// Sortie reseau
        }

       delai_freq++;
        
        out[z] = outKohonen * 2.0 - 1.0;// sortie control * 2.0 - 1.0

    }

    unit->delai_freq = delai_freq;
    unit->outKohonen = outKohonen;
}

void HPkohonen3_next_a(HPkohonen3 *unit, int inNumSamples)
{
    float *out = OUT(0);
    
    float rate = IN0(0);
    float *data1 = IN(1);
    float *data2 = IN(2);
    float *data3 = IN(3);
    float learningRate = IN0(4);
    float distanceNeighbourhood = IN0(5);
    float gate = IN0(6);
    float delai_freq = unit->delai_freq;
    float outKohonen = unit->outKohonen;
    
    // set FHZ de calcul de l'algo
    float samplesPerCycle;
    if(rate < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(rate, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        
        if(delai_freq >= samplesPerCycle)
            
        {
            delai_freq -= samplesPerCycle;
            
            // Kohonen Algorithme
            
            float distance = 999999;
            float arrayDistance [128];
            float deltaRate = 1;
            float d = 0;
            float indexNodeGagnant = 0;
            float distanceNodeGagnant = 0;
            float distanceVoisin = 0;
            
            for(int i=0; i<=127; ++i)
            {
                d = 0;
                d = pow(d + ((data1[z] + 1.0 / 2.0) - unit->weightNode [i] [0]), 2);
                d = pow(d + ((data2[z] + 1.0 / 2.0) - unit->weightNode [i] [1]), 2);
                d = pow(d + ((data3[z] + 1.0 / 2.0) - unit->weightNode [i] [2]), 2);
                d = sqrt(d);
                arrayDistance [i] = d;
                if(d < distance) {distance = d; indexNodeGagnant = i; distanceNodeGagnant = unit->vecteurNode [i];};
            }
            
            if(gate == 1)
            {
                for(int i=0; i<=127; ++i)
                {
                    distanceVoisin = fabs(distance - unit->vecteurNode [i]);
                    if(distanceVoisin <= distanceNeighbourhood)
                    {
                        deltaRate = exp( -1 * (pow(distanceVoisin, 2) / pow(distanceNeighbourhood, 2)));
                        unit->weightNode [i] [0] = unit->weightNode [i] [0] + (deltaRate * learningRate * ((data1[z] + 1.0 / 2.0) - unit->weightNode [i] [0]));
                        unit->weightNode [i] [1] = unit->weightNode [i] [1] + (deltaRate * learningRate * ((data2[z] + 1.0 / 2.0) - unit->weightNode [i] [1]));
                        unit->weightNode [i] [2] = unit->weightNode [i] [2] + (deltaRate * learningRate * ((data3[z] + 1.0 / 2.0) - unit->weightNode [i] [2]));
                    }
                }
            }
            else
            {
                for(int i=0; i<=127; ++i)
                {
                    //indexNodeGagnant -> winner
                }
            }
            outKohonen = indexNodeGagnant / 127;// Sortie reseau
        }
        
        delai_freq++;
        
        out[z] = outKohonen * 2.0 - 1.0;// sortie control * 2.0 - 1.0
        
    }
    
    unit->delai_freq = delai_freq;
    unit->outKohonen = outKohonen;
}

void HPkohonen1_next_k(HPkohonen1 *unit, int inNumSamples)
{
    float *out = OUT(0);
    
    float rate = IN0(0);
    float *data1 = IN(1);
    float learningRate = IN0(2);
    float distanceNeighbourhood = IN0(3);
    float gate = IN0(4);
    float delai_freq = unit->delai_freq;
    float outKohonen = unit->outKohonen;
    
    // set FHZ de calcul de l'algo
    float samplesPerCycle;
    if(rate < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(rate, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        
        if(delai_freq >= samplesPerCycle)
            
        {
            delai_freq -= samplesPerCycle;
            
            // Kohonen Algorithme
            
            float distance = 999999;
            float arrayDistance [513];
            float deltaRate = 1;
            float d = 0;
            float indexNodeGagnant = 0;
            float distanceNodeGagnant = 0;
            float distanceVoisin = 0;
            
            for(int i=0; i<=512; ++i)
            {
                d = 0;
                d = pow(d + ((data1[z] + 1.0 / 2.0) - unit->weightNode [i]), 2);
                d = sqrt(d);
                arrayDistance [i] = d;
                if(d < distance) {distance = d; indexNodeGagnant = i; distanceNodeGagnant = unit->vecteurNode [i];};
            }
            
            if(gate == 1)
            {
                for(int i=0; i<=512; ++i)
                {
                    distanceVoisin = fabs(distance - unit->vecteurNode [i]);
                    if(distanceVoisin <= distanceNeighbourhood)
                    {
                        deltaRate = exp( -1 * (pow(distanceVoisin, 2) / pow(distanceNeighbourhood, 2)));
                        unit->weightNode [i] = unit->weightNode [i] + (deltaRate * learningRate * ((data1[z] + 1.0 / 2.0) - unit->weightNode [i]));
                    }
                }
            }
            else
            {
                for(int i=0; i<=512; ++i)
                {
                    //indexNodeGagnant -> winner
                }
            }
            outKohonen = indexNodeGagnant / 512;// Sortie reseau
        }
        
        delai_freq++;
        
        out[z] = outKohonen * 2.0 - 1.0;// sortie control * 2.0 - 1.0
        
    }
    
    unit->delai_freq = delai_freq;
    unit->outKohonen = outKohonen;
}

void HPkohonen1_next_a(HPkohonen1 *unit, int inNumSamples)
{
    float *out = OUT(0);
    
    float rate = IN0(0);
    float *data1 = IN(1);
    float learningRate = IN0(2);
    float distanceNeighbourhood = IN0(3);
    float gate = IN0(4);
    float delai_freq = unit->delai_freq;
    float outKohonen = unit->outKohonen;
    
    // set FHZ de calcul de l'algo
    float samplesPerCycle;
    if(rate < unit->mRate->mSampleRate)
        samplesPerCycle = unit->mRate->mSampleRate / sc_max(rate, 0.001f);
    else samplesPerCycle = 1.f;
    
    for (int z=0; z<inNumSamples; ++z)
    {
        
        if(delai_freq >= samplesPerCycle)
            
        {
            delai_freq -= samplesPerCycle;
            
            // Kohonen Algorithme
            
            float distance = 999999;
            float arrayDistance [513];
            float deltaRate = 1;
            float d = 0;
            float indexNodeGagnant = 0;
            float distanceNodeGagnant = 0;
            float distanceVoisin = 0;
            
            for(int i=0; i<=512; ++i)
            {
                d = 0;
                d = pow(d + ((data1[z] + 1.0 / 2.0) - unit->weightNode [i]), 2);
                d = sqrt(d);
                arrayDistance [i] = d;
                if(d < distance) {distance = d; indexNodeGagnant = i; distanceNodeGagnant = unit->vecteurNode [i];};
            }
            
            if(gate == 1)
            {
                for(int i=0; i<=512; ++i)
                {
                    distanceVoisin = fabs(distance - unit->vecteurNode [i]);
                    if(distanceVoisin <= distanceNeighbourhood)
                    {
                        deltaRate = exp( -1 * (pow(distanceVoisin, 2) / pow(distanceNeighbourhood, 2)));
                        unit->weightNode [i] = unit->weightNode [i] + (deltaRate * learningRate * ((data1[z] + 1.0 / 2.0) - unit->weightNode [i]));
                    }
                }
            }
            else
            {
                for(int i=0; i<=512; ++i)
                {
                    //indexNodeGagnant -> winner
                }
            }
            outKohonen = indexNodeGagnant / 512;// Sortie reseau
        }
        
        delai_freq++;
        
        out[z] = outKohonen * 2.0 - 1.0;// sortie control * 2.0 - 1.0
        
    }
    
    unit->delai_freq = delai_freq;
    unit->outKohonen = outKohonen;
}

//////////////////////////////////////////////////////////////

PluginLoad(HPUGens)
{
    ft = inTable;
    
    DefineSimpleUnit(HPkohonen3);
    DefineSimpleUnit(HPkohonen1);
}

//////////////////////////////////////////////////////////////

