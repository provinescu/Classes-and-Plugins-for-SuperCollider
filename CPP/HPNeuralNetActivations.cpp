#include "SC_PlugIn.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
static InterfaceTable* ft;
constexpr int ML=8, MI=32, MO=32, MN=128;
enum Activation { Sigmoid=0, Tanh=1, ReLU=2, LeakyReLU=3, Linear=4, Softsign=5 };
struct HPNeuralNetActivations : Unit {
 int ni,no,nh,L,sz[ML+2],off[ML+2],act[ML+1],ib,tb,cb,nn,nw,np;
 float *w,*b,*a,*d,oldTrain,oldReset,oldLoad,oldSave; uint32 rng;
};
static uint32 xr(uint32&s){s^=s<<13;s^=s>>17;s^=s<<5;return s;}
static float rw(uint32&s){return(float(xr(s)&0xffffff)/16777216.f-.5f)*.5f;}
static float activate(float x,int t,float temp){temp=std::max(std::fabs(temp),1e-4f);x/=temp;switch(t){case Tanh:return std::tanh(x);case ReLU:return std::max(0.f,x);case LeakyReLU:return x>=0?x:.01f*x;case Linear:return x;case Softsign:return x/(1+std::fabs(x));default:x=std::max(-60.f,std::min(60.f,x));return 1/(1+std::exp(-x));}}
static float deriv(float y,int t,float temp){temp=std::max(std::fabs(temp),1e-4f);switch(t){case Tanh:return(1-y*y)/temp;case ReLU:return y>0?1/temp:0;case LeakyReLU:return y>=0?1/temp:.01f/temp;case Linear:return 1/temp;case Softsign:{float q=1-std::fabs(y);return(q*q)/temp;}default:return y*(1-y)/temp;}}
static int ws(HPNeuralNetActivations*u,int l){int n=0;for(int q=1;q<l;q++)n+=u->sz[q-1]*u->sz[q];return n;}
static void initw(HPNeuralNetActivations*u,uint32 seed){u->rng=seed?seed:1;for(int i=0;i<u->nw;i++)u->w[i]=rw(u->rng);for(int i=0;i<u->nn-u->ni;i++)u->b[i]=rw(u->rng);}
static SndBuf* gb(HPNeuralNetActivations*u,int n){if(n<0||n>=int(u->mWorld->mNumSndBufs))return nullptr;SndBuf*x=u->mWorld->mSndBufs+n;return x->data&&x->channels==1&&x->frames>=u->np?x:nullptr;}
static void savebuf(HPNeuralNetActivations*u,int n){SndBuf*x=gb(u,n);if(!x)return;int z=0;for(int i=0;i<u->nw;i++)x->data[z++]=u->w[i];for(int i=0;i<u->nn-u->ni;i++)x->data[z++]=u->b[i];}
static void loadbuf(HPNeuralNetActivations*u,int n){SndBuf*x=gb(u,n);if(!x)return;int z=0;for(int i=0;i<u->nw;i++)u->w[i]=x->data[z++];for(int i=0;i<u->nn-u->ni;i++)u->b[i]=x->data[z++];}
static float val(Unit* unit,int i,int n){return INRATE(i)==calc_FullRate?IN(i)[n]:IN0(i);}
static void clear(HPNeuralNetActivations* unit,int n){ClearUnitOutputs(unit,n);}
static void next(HPNeuralNetActivations* unit,int N){auto*u=unit;int c=u->cb;float lr=std::max(0.f,std::min(1.f,IN0(c))),temp=IN0(c+1),tr=IN0(c+2),rr=IN0(c+3),ld=IN0(c+6),sv=IN0(c+7);int bn=int(IN0(c+5));if(rr>0&&u->oldReset<=0)initw(u,uint32(std::max(1.f,IN0(c+4))));u->oldReset=rr;if(ld>0&&u->oldLoad<=0)loadbuf(u,bn);u->oldLoad=ld;if(sv>0&&u->oldSave<=0)savebuf(u,bn);u->oldSave=sv;
 for(int n=0;n<N;n++){for(int i=0;i<u->ni;i++)u->a[i]=val(unit,u->ib+i,n);int bo=0;for(int l=1;l<u->L;l++){int p=u->sz[l-1],q=u->sz[l],W=ws(u,l),po=u->off[l-1],co=u->off[l];for(int j=0;j<q;j++){float s=u->b[bo+j];for(int i=0;i<p;i++)s+=u->a[po+i]*u->w[W+i*q+j];u->a[co+j]=activate(s,u->act[l-1],temp);}bo+=q;}int oo=u->off[u->L-1];for(int k=0;k<u->no;k++)OUT(k)[n]=u->a[oo+k];}
 if(tr>0&&u->oldTrain<=0&&lr>0){int last=u->L-1,oo=u->off[last];for(int k=0;k<u->no;k++){float y=u->a[oo+k];u->d[oo+k]=(IN0(u->tb+k)-y)*deriv(y,u->act[last-1],temp);}for(int l=last-1;l>=1;l--){int q=u->sz[l],r=u->sz[l+1],co=u->off[l],ro=u->off[l+1],W=ws(u,l+1);for(int i=0;i<q;i++){float s=0;for(int j=0;j<r;j++)s+=u->d[ro+j]*u->w[W+i*r+j];u->d[co+i]=s*deriv(u->a[co+i],u->act[l-1],temp);}}int bo=0;for(int l=1;l<u->L;l++){int p=u->sz[l-1],q=u->sz[l],po=u->off[l-1],co=u->off[l],W=ws(u,l);for(int i=0;i<p;i++)for(int j=0;j<q;j++)u->w[W+i*q+j]+=lr*u->a[po+i]*u->d[co+j];for(int j=0;j<q;j++)u->b[bo+j]+=lr*u->d[co+j];bo+=q;}}u->oldTrain=tr;}
static bool allocmem(HPNeuralNetActivations*u){u->w=(float*)RTAlloc(u->mWorld,u->nw*sizeof(float));u->b=(float*)RTAlloc(u->mWorld,(u->nn-u->ni)*sizeof(float));u->a=(float*)RTAlloc(u->mWorld,u->nn*sizeof(float));u->d=(float*)RTAlloc(u->mWorld,u->nn*sizeof(float));return u->w&&u->b&&u->a&&u->d;}
static void ctor(HPNeuralNetActivations* unit){auto*u=unit;u->w=u->b=u->a=u->d=nullptr;u->ni=int(IN0(0));u->no=int(IN0(1));u->nh=int(IN0(2));u->sz[0]=u->ni;for(int i=0;i<ML;i++)u->sz[i+1]=int(IN0(3+i));u->sz[u->nh+1]=u->no;for(int i=0;i<ML+1;i++)u->act[i]=std::max(0,std::min(5,int(IN0(3+ML+i))));u->L=u->nh+2;bool ok=u->ni>0&&u->ni<=MI&&u->no>0&&u->no<=MO&&u->nh>0&&u->nh<=ML&&u->mNumOutputs==u->no;for(int i=1;i<=u->nh;i++)ok=ok&&u->sz[i]>0&&u->sz[i]<=MN;u->off[0]=0;u->nn=u->ni;u->nw=0;for(int l=1;l<u->L;l++){u->off[l]=u->nn;u->nn+=u->sz[l];u->nw+=u->sz[l-1]*u->sz[l];}u->np=u->nw+u->nn-u->ni;u->ib=3+ML+(ML+1);u->tb=u->ib+u->ni;u->cb=u->tb+u->no;if(!ok||!allocmem(u)){SETCALC(clear);ClearUnitOutputs(unit,1);return;}u->oldTrain=u->oldReset=u->oldLoad=u->oldSave=0;initw(u,uint32(std::max(1.f,IN0(u->cb+4))));SETCALC(next);ClearUnitOutputs(unit,1);}
static void dtor(HPNeuralNetActivations*u){RTFree(u->mWorld,u->w);RTFree(u->mWorld,u->b);RTFree(u->mWorld,u->a);RTFree(u->mWorld,u->d);}
PluginLoad(HPNeuralNetActivations){ft=inTable;(*ft->fDefineUnit)("HPNeuralNetActivations",sizeof(HPNeuralNetActivations),(UnitCtorFunc)&ctor,(UnitDtorFunc)&dtor,0);}
