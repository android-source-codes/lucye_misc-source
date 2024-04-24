#include "ResampleClass.h"



ResampleClass:: ResampleClass()
{
    //init members from resamplesub.c
    _firstChunk = 0;
    _Xoff = 0;
    _XoffSizeByte = 0;
    _offsetSizeX2inBytes = 0;
    _Time = 0;
    _writtenSamples = 0;
    _inputSamples = 0;
    _highQuality = 0;
    _nChannel = 0;
    _resampleFactor = 0.0;

    _creepNo = 0;
    _creepNoBytes = 0;

    _X1 = NULL;
    _X2 = NULL;
    _Y1 = NULL;
    _Y2 = NULL;
    _X1_Remain = NULL;
    _X2_Remain = NULL;

}

ResampleClass:: ~ResampleClass()
{

}


 __inline HWORD ResampleClass:: WordToHword(WORD v, int scl)
{
    HWORD out;
    WORD llsb = (1<<(scl-1));
    v += llsb;          /* round */
    v >>= scl;
    if (v>MAX_HWORD) {
        v = MAX_HWORD;
    } else if (v < MIN_HWORD) {
        v = MIN_HWORD;
    }
    out = (HWORD) v;
    return out;
}

/* Sampling rate up-conversion only subroutine;
 * Slightly faster than down-conversion; */
int ResampleClass:: SrcUp(HWORD X[], HWORD Y[], double factor, UWORD *Time,
                 UHWORD Nx, UHWORD Nwing, UHWORD LpScl,
                 HWORD Imp[], HWORD ImpD[], BOOL Interp)
{
    HWORD *Xp, *Ystart;
    WORD v;

    double dt;                  /* Step through input signal */
    UWORD dtb;                  /* Fixed-point version of Dt */
    UWORD endTime;              /* When Time reaches EndTime, return to user */

    dt = 1.0/factor;            /* Output sampling period */
    dtb = dt*(1<<Np) + 0.5;     /* Fixed-point representation */

    Ystart = Y;
    endTime = *Time + (1<<Np)*(WORD)Nx;
    while (*Time < endTime)
    {
        Xp = &X[*Time>>Np];      /* Ptr to current input sample */
        /* Perform left-wing inner product */
        v = FilterUp(Imp, ImpD, Nwing, Interp, Xp, (HWORD)(*Time&Pmask),-1);
        /* Perform right-wing inner product */
        v += FilterUp(Imp, ImpD, Nwing, Interp, Xp+1,
              /* previous (triggers warning): (HWORD)((-*Time)&Pmask),1); */
                      (HWORD)((((*Time)^Pmask)+1)&Pmask),1);
        v >>= Nhg;              /* Make guard bits */
        v *= LpScl;             /* Normalize for unity filter gain */
        *Y++ = WordToHword(v,NLpScl);   /* strip guard bits, deposit output */
        *Time += dtb;           /* Move to next sample by time increment */
    }
    return (Y - Ystart);        /* Return the number of output samples */
}


/* Sampling rate conversion subroutine */
int ResampleClass:: SrcUD(HWORD X[], HWORD Y[], double factor, UWORD *Time,
                 UHWORD Nx, UHWORD Nwing, UHWORD LpScl,
                 HWORD Imp[], HWORD ImpD[], BOOL Interp)
{
    HWORD *Xp, *Ystart;
    WORD v;

    double dh;                  /* Step through filter impulse response */
    double dt;                  /* Step through input signal */
    UWORD endTime;              /* When Time reaches EndTime, return to user */
    UWORD dhb, dtb;             /* Fixed-point versions of Dh,Dt */

    dt = 1.0/factor;            /* Output sampling period */
    dtb = dt*(1<<Np) + 0.5;     /* Fixed-point representation */

    dh = MIN(Npc, factor*Npc);  /* Filter sampling period */
    dhb = dh*(1<<Na) + 0.5;     /* Fixed-point representation */

    Ystart = Y;
    endTime = *Time + (1<<Np)*(WORD)Nx;
    while (*Time < endTime)
    {
        Xp = &X[*Time>>Np];     /* Ptr to current input sample */
        v = FilterUD(Imp, ImpD, Nwing, Interp, Xp, (HWORD)(*Time&Pmask),
                     -1, dhb);  /* Perform left-wing inner product */
        v += FilterUD(Imp, ImpD, Nwing, Interp, Xp+1,
              /* previous (triggers warning): (HWORD)((-*Time)&Pmask), */
                      (HWORD)((((*Time)^Pmask)+1)&Pmask),
                      1, dhb);  /* Perform right-wing inner product */
        v >>= Nhg;              /* Make guard bits */
        v *= LpScl;             /* Normalize for unity filter gain */
        *Y++ = WordToHword(v,NLpScl);   /* strip guard bits, deposit output */
        *Time += dtb;           /* Move to next sample by time increment */
    }

    return (Y - Ystart);        /* Return the number of output samples */
}



/*
_Xoff = offset sample number
_Time = (Xoff<<Np)   //DATA start point (after offset1).
_XoffSizeByte = Xoff *2;
_offsetSizeX2inBytes = Xoff *4


//FIRST
|Offset1| DATA  |Offset2|Offset3|
Input Data = Data + Offset2 + Offset3
NX = DATA + Offset 2 = processing data, Offset1 is zero padded.
RemainBuffer Offset2, Offset3

//Process1
|Offset2|Offset3| DATA  |Offset4|Offset5|
Input Data = Data + Offset4 + Offset5
NX = Offset3+DATA+Offset4
RemainBuffer Offset4, Offset5

//Process2
|Offset4|Offset5| DATA  |Offset6|Offset7|
Input Data = Data + Offset6 + Offset7
NX = Offset5+DATA+Offset6
RemainBuffer Offset6, Offset7

//LAST
|Offset6|Offset7| DATA  |Offset6|
Input Data = Data
NX = Offset7 + DATA. Offset 6 is zero padded
*/

int ResampleClass:: resampleWithFilter_mem(  /* number of output samples returned */
    double factor,              /* factor = outSampleRate/inSampleRate */
    short * inbuffer,                   /* input and output file descriptors */
    short *outputBuffer,
    int inCount,                /* number of input samples to convert */
    int outCount,               /* number of output samples to compute */
    int nChans,                 /* number of sound channels (1 or 2) */
    BOOL interpFilt,            /* TRUE means interpolate filter coeffs */
    HWORD Imp[], HWORD ImpD[],
    UHWORD LpScl, UHWORD Nmult, UHWORD Nwing,BOOL lastChunk)
{
    int Nout;

    short* X1= _X1;
    short* Y1= _Y1;                 /* I/O buffers */
    UWORD Time;                     //= (24<<Np);       //start time. offset
    UHWORD Nx;
    int Ncreep = 0;

    Time = _Time;
    Nx = inCount;                   //NX means resample sample Numbers

    if (factor < 1)
      LpScl = LpScl*factor + 0.5;   //need only one time

    if(_firstChunk)
        {
            memcpy((char *)X1+_XoffSizeByte,inbuffer,inCount*2);                                //MAKE FIRST CHUNK
            Nx = inCount - _Xoff;
            _firstChunk = FALSE;
        }else{
            if(lastChunk)
            {
                Nx = inCount;
                Nx = Nx + _Xoff;
                memset(X1,0x00,sizeof(short)*IBUFFSIZE +_offsetSizeX2inBytes);              //zero padd Offset6
            }else{
                Nx = inCount -_creepNo;
            }
                memcpy ((char *)X1,_X1_Remain,_offsetSizeX2inBytes-_creepNoBytes);          //copy offset2, offset3
                memcpy((char *)X1+_offsetSizeX2inBytes-_creepNoBytes,inbuffer,inCount*2);   //copy data + offset4 + offset5
        }

    if(factor>=1.0)
        Nout=SrcUp(X1,Y1,factor,&Time,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);
    else
        Nout=SrcUD(X1,Y1,factor,&Time,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);

    Time -= (Nx<<Np);       /* Move converter Nx samples back in time */
    _Time = Time ;
    Ncreep = (Time>>Np) - _Xoff; /* Calc time accumulation in Time */
        if (Ncreep) {
            _Time -= (Ncreep<<Np);    /* Remove time accumulation */
            _creepNo= Ncreep;
            _creepNoBytes = _creepNo <<1;
        }else{
            _creepNo = 0;
            _creepNoBytes = 0;
        }

    _writtenSamples = _writtenSamples + Nout;
    _inputSamples = _inputSamples + inCount ;

    if(lastChunk)
    {
        int expectedSamples = _inputSamples * (float)factor;
        if(expectedSamples < _writtenSamples)
        {
            Nout = Nout - (_writtenSamples - expectedSamples);      //detach extra samples.
        }
    }

    memcpy ((char *)_X1_Remain,(char *)inbuffer+inCount*2 -_offsetSizeX2inBytes + _creepNoBytes,_offsetSizeX2inBytes-_creepNoBytes); //make Remain Buffer offset 2,3   4,5   6,7///
    memcpy(outputBuffer,Y1,sizeof(short)*Nout);

    return Nout;
}


int ResampleClass:: resampleWithFilter_mem2Ch(  /* number of output samples returned */
    double factor,              /* factor = outSampleRate/inSampleRate */
    short * inbuffer,
    short *outputBuffer,
    int inCount,                /* number of input samples to convert */
    int outCount,               /* number of output samples to compute */
    int nChans,                 /* number of sound channels (1 or 2) */
    BOOL interpFilt,            /* TRUE means interpolate filter coeffs */
    HWORD Imp[], HWORD ImpD[],
    UHWORD LpScl, UHWORD Nmult, UHWORD Nwing,BOOL lastChunk)
{

    int Nout;

    short* X1= _X1;
    short* Y1= _Y1;
    short* X2 = _X2;
    short* Y2 = _Y2;
    UWORD Time, Time2;
    UHWORD Nx;
    int Ncreep = 0;
    short* inbufL;
    short* inbufR ;
    int itr = 0;

    inbufL = (short*)malloc(inCount*sizeof(short));
    if(inbufL == NULL){return -90;}
    inbufR = (short*)malloc(inCount*sizeof(short));
    if(inbufR == NULL){free(inbufL);return -91;}

    for(itr = 0;itr < inCount;itr++)
    {
        inbufL[itr] = inbuffer[itr *2] ;
        inbufR[itr] = inbuffer[itr *2+1] ;
    }

    Time = _Time;
    Time2 = _Time;
    Nx = inCount;               //NX means resample sample Numbers

    if (factor < 1)
      LpScl = LpScl*factor + 0.5;           //need only one time

    if(_firstChunk)
        {
            memcpy((char *)X1+_XoffSizeByte,inbufL,inCount*2);                              //MAKE FIRST CHUNK
            memcpy((char *)X2+_XoffSizeByte,inbufR,inCount*2);                              //MAKE FIRST CHUNK
            Nx = inCount - _Xoff;
            _firstChunk = FALSE;
        }else{
            if(lastChunk)
            {
                Nx = inCount;
                Nx = Nx + _Xoff;
                memset(X1,0x00,sizeof(short)*IBUFFSIZE +_offsetSizeX2inBytes);      //zero padd Offset6
                memset(X2,0x00,sizeof(short)*IBUFFSIZE +_offsetSizeX2inBytes);      //zero padd Offset6
            }else{
                Nx = inCount -_creepNo;
            }
                memcpy ((char *)X1,_X1_Remain,_offsetSizeX2inBytes-_creepNoBytes); //copy offset2, offset3
                memcpy ((char *)X2,_X2_Remain,_offsetSizeX2inBytes-_creepNoBytes); //copy offset2, offset3
                memcpy((char *)X1+_offsetSizeX2inBytes-_creepNoBytes,inbufL,inCount*2);  //copy data + offset4 + offset5
                memcpy((char *)X2+_offsetSizeX2inBytes-_creepNoBytes,inbufR,inCount*2);  //copy data + offset4 + offset5
        }

    if(factor>=1.0){
        Nout=SrcUp(X1,Y1,factor,&Time,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);
        Nout=SrcUp(X2,Y2,factor,&Time2,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);
        }
    else{
        Nout=SrcUD(X1,Y1,factor,&Time,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);
        Nout=SrcUD(X2,Y2,factor,&Time2,Nx,Nwing,LpScl,Imp,ImpD,interpFilt);
        }

    Time -= (Nx<<Np);       /* Move converter Nx samples back in time */
    _Time = Time ;
    Ncreep = (Time>>Np) - _Xoff; /* Calc time accumulation in Time */
        if (Ncreep) {
            _Time -= (Ncreep<<Np);    /* Remove time accumulation */
            _creepNo= Ncreep;
            _creepNoBytes = _creepNo <<1;
        }else{
            _creepNo = 0;
            _creepNoBytes = 0;
        }

    _writtenSamples = _writtenSamples + Nout;
    _inputSamples = _inputSamples + inCount ;

    if(lastChunk)
    {
        int expectedSamples = _inputSamples * (float)factor;
        if(expectedSamples < _writtenSamples)
        {
            Nout = Nout - (_writtenSamples - expectedSamples);      //detach extra samples.
        }
    }

    memcpy ((char *)_X1_Remain,(char *)inbufL+inCount*2 -_offsetSizeX2inBytes + _creepNoBytes,_offsetSizeX2inBytes-_creepNoBytes); //make Remain Buffer offset 2,3   4,5   6,7///
    memcpy ((char *)_X2_Remain,(char *)inbufR+inCount*2 -_offsetSizeX2inBytes + _creepNoBytes,_offsetSizeX2inBytes-_creepNoBytes); //make Remain Buffer offset 2,3   4,5   6,7///

    for(itr = 0;itr < Nout;itr++)
    {
        outputBuffer[2*itr] = Y1[itr] ;
        outputBuffer[2*itr+1] = Y2[itr];
    }
    free(inbufL);
    free(inbufR);

    return Nout;

}


int ResampleClass:: resample_m(
                                    short* inputbuffer,
                                    short* outputbuffer,
                                    int inCount,                /* number of input samples to convert */
                                    BOOL lastChunk)
{
    return resample_mem(_resampleFactor, inputbuffer, outputbuffer, inCount,
                      inCount*_resampleFactor , _nChannel ,FALSE, 0, _highQuality, NULL,lastChunk);
}

int ResampleClass:: resample_mem(                   /* number of output samples returned */
    double factor,              /* factor = Sndout/Sndin */
    short*    inputbuffer,
    short* outputbuffer,
    int inCount,                /* number of input samples to convert */
    int outCount,               /* number of output samples to compute */
    int nChans,                 /* number of sound channels (1 or 2) */
    BOOL interpFilt,            /* TRUE means interpolate filter coeffs */
    int fastMode,               /* 0 = highest quality, slowest speed */
    BOOL largeFilter,           /* TRUE means use 65-tap FIR filter */
    char *filterFile,
    BOOL lastChunk)           /* NULL for internal filter, else filename */
{
    UHWORD LpScl;               /* Unity-gain scale factor */
    UHWORD Nwing;               /* Filter table size */
    UHWORD Nmult;               /* Filter length for up-conversions */
    HWORD *Imp=0;               /* Filter coefficients */
    HWORD *ImpD=0;              /* ImpD[n] = Imp[n+1]-Imp[n] */


if (largeFilter) {
        Nmult = LARGE_FILTER_NMULT;
        Imp = LARGE_FILTER_IMP;         /* Impulse response */
        ImpD = LARGE_FILTER_IMPD;       /* Impulse response deltas */
        LpScl = LARGE_FILTER_SCALE;     /* Unity-gain scale factor */
        Nwing = LARGE_FILTER_NWING;     /* Filter table length */
    } else {
        Nmult = SMALL_FILTER_NMULT;
        Imp = SMALL_FILTER_IMP;         /* Impulse response */
        ImpD = SMALL_FILTER_IMPD;       /* Impulse response deltas */
        LpScl = SMALL_FILTER_SCALE;     /* Unity-gain scale factor */
        Nwing = SMALL_FILTER_NWING;     /* Filter table length */
    }

        LpScl *= 0.95;
    if(nChans==2)return resampleWithFilter_mem2Ch(factor,inputbuffer,outputbuffer,inCount,outCount,nChans,
                                   interpFilt, Imp, ImpD, LpScl, Nmult, Nwing,lastChunk);
    else return resampleWithFilter_mem(factor,inputbuffer,outputbuffer,inCount,outCount,nChans,
                                   interpFilt, Imp, ImpD, LpScl, Nmult, Nwing,lastChunk);

}

int ResampleClass:: getResampleXoffVal()
{
    return _Xoff;
}

int ResampleClass:: getMinResampleInputSampleNo()
{
    return _Xoff * 2;       //_Xoff * 2 is limitation.
}

int ResampleClass:: getMaxResampleInputBufferSize()
{
    return sizeof(short)*IBUFFSIZE ;
}

/*
return val >= 0  Success
return val < 0  Fail
*/
int ResampleClass:: initResample(double factor, int nChans, int qualityHigh)
{
    int obuffSize = (int)(((double)IBUFFSIZE)*factor+2.0 ); //ORG code has 2bytes margine.

    _highQuality = qualityHigh;
    _resampleFactor = factor;
    _nChannel = nChans;


    if(qualityHigh)
        _Xoff = ((LARGE_FILTER_NMULT+1)/2.0) * MAX(1.0,1.0/factor) + 10;
    else _Xoff = ((SMALL_FILTER_NMULT+1)/2.0) * MAX(1.0,1.0/factor) + 10;


    /* Calc reach of LP filter wing & give some creeping room */
    _Time = (_Xoff<<Np);
    _XoffSizeByte = _Xoff *2;
    _offsetSizeX2inBytes = _Xoff *4;

    _X1 = (short*)malloc(sizeof(short)*IBUFFSIZE +_offsetSizeX2inBytes);     //MAX InputData Size = sizeof(short)*IBUFFSIZE
    if(_X1 == NULL)return -1;
    memset(_X1,0x00,sizeof(short)*IBUFFSIZE+_offsetSizeX2inBytes);

    _X1_Remain = (short*)malloc(_offsetSizeX2inBytes);
    if(_X1_Remain == NULL){free(_X1);return -5;}
    memset(_X1_Remain,0x00,_offsetSizeX2inBytes);

    _Y1 = (short*)malloc(sizeof(short)*obuffSize);
    if(_Y1 == NULL){free(_X1);free(_X1_Remain);return -2;}
    memset(_Y1,0x00,sizeof(short)*obuffSize);

    if(nChans == 2)
    {
        _X2 = (short*)malloc(sizeof(short)*IBUFFSIZE+_offsetSizeX2inBytes);
        if(_X2 == NULL){free(_X1);free(_X1_Remain);free(_Y1);return -3;}
        memset(_X2,0x00,sizeof(short)*IBUFFSIZE+_offsetSizeX2inBytes);

        _X2_Remain = (short*)malloc(_offsetSizeX2inBytes);
        if(_X2_Remain == NULL){free(_X1);free(_X1_Remain);free(_X2);free(_Y1);return -6;}
        memset(_X2_Remain,0x00,_offsetSizeX2inBytes);

        _Y2 = (short*)malloc(sizeof(short)*obuffSize);
        if(_Y2 == NULL){free(_X1);free(_X1_Remain);free(_Y1);free(_X2);free(_X2_Remain);return -4;}
        memset(_Y2,0x00,sizeof(short)*obuffSize);
    }else
    {
        _X2 = NULL;
        _X2_Remain = NULL;
        _Y2 = NULL;
    }

    _creepNo = 0;
    _creepNoBytes = 0;
    _firstChunk = TRUE;
    _writtenSamples = 0;
    _inputSamples = 0;

    return 1;
}


int ResampleClass:: deInitResample()
{
    if(_X1 != NULL)free(_X1);
    if(_X1_Remain != NULL)free(_X1_Remain);
    if(_Y1 != NULL)free(_Y1);
    if(_X2 != NULL)free(_X2);
    if(_X2_Remain != NULL)free(_X2_Remain);
    if(_Y2 != NULL)free(_Y2);

    _creepNo = 0;
    _creepNoBytes = 0;
    _firstChunk = TRUE;
    _writtenSamples = 0;
    _inputSamples = 0;
    _highQuality = 0;
    _resampleFactor = 0.0;

    return 1;
}

WORD ResampleClass:: FilterUp(HWORD Imp[], HWORD ImpD[],
             UHWORD Nwing, BOOL Interp,
             HWORD *Xp, HWORD Ph, HWORD Inc)
{
    HWORD *Hp, *Hdp = NULL, *End;
    HWORD a = 0;
    WORD v, t;

    v=0;
    Hp = &Imp[Ph>>Na];
    End = &Imp[Nwing];
    if (Interp) {
    Hdp = &ImpD[Ph>>Na];
    a = Ph & Amask;
    }
    if (Inc == 1)       /* If doing right wing...              */
    {               /* ...drop extra coeff, so when Ph is  */
    End--;          /*    0.5, we don't do too many mult's */
    if (Ph == 0)        /* If the phase is zero...           */
    {           /* ...then we've already skipped the */
        Hp += Npc;      /*    first sample, so we must also  */
        Hdp += Npc;     /*    skip ahead in Imp[] and ImpD[] */
    }
    }
    if (Interp)
      while (Hp < End) {
      t = *Hp;      /* Get filter coeff */
      t += (((WORD)*Hdp)*a)>>Na; /* t is now interp'd filter coeff */
      Hdp += Npc;       /* Filter coeff differences step */
      t *= *Xp;     /* Mult coeff by input sample */
      if (t & (1<<(Nhxn-1)))  /* Round, if needed */
        t += (1<<(Nhxn-1));
      t >>= Nhxn;       /* Leave some guard bits, but come back some */
      v += t;           /* The filter output */
      Hp += Npc;        /* Filter coeff step */
      Xp += Inc;        /* Input signal step. NO CHECK ON BOUNDS */
      }
    else
      while (Hp < End) {
      t = *Hp;      /* Get filter coeff */
      t *= *Xp;     /* Mult coeff by input sample */
      if (t & (1<<(Nhxn-1)))  /* Round, if needed */
        t += (1<<(Nhxn-1));
      t >>= Nhxn;       /* Leave some guard bits, but come back some */
      v += t;           /* The filter output */
      Hp += Npc;        /* Filter coeff step */
      Xp += Inc;        /* Input signal step. NO CHECK ON BOUNDS */
      }
    return(v);
}

//OOOO
WORD ResampleClass:: FilterUD( HWORD Imp[], HWORD ImpD[],
             UHWORD Nwing, BOOL Interp,
             HWORD *Xp, HWORD Ph, HWORD Inc, UHWORD dhb)
{
    HWORD a;
    HWORD *Hp, *Hdp, *End;
    WORD v, t;
    UWORD Ho;

    v=0;
    Ho = (Ph*(UWORD)dhb)>>Np;
    End = &Imp[Nwing];
    if (Inc == 1)       /* If doing right wing...              */
    {               /* ...drop extra coeff, so when Ph is  */
    End--;          /*    0.5, we don't do too many mult's */
    if (Ph == 0)        /* If the phase is zero...           */
      Ho += dhb;        /* ...then we've already skipped the */
    }               /*    first sample, so we must also  */
                /*    skip ahead in Imp[] and ImpD[] */
    if (Interp)
      while ((Hp = &Imp[Ho>>Na]) < End) {
      t = *Hp;      /* Get IR sample */
      Hdp = &ImpD[Ho>>Na];  /* get interp (lower Na) bits from diff table*/
      a = Ho & Amask;   /* a is logically between 0 and 1 */
      t += (((WORD)*Hdp)*a)>>Na; /* t is now interp'd filter coeff */
      t *= *Xp;     /* Mult coeff by input sample */
      if (t & 1<<(Nhxn-1))  /* Round, if needed */
        t += 1<<(Nhxn-1);
      t >>= Nhxn;       /* Leave some guard bits, but come back some */
      v += t;           /* The filter output */
      Ho += dhb;        /* IR step */
      Xp += Inc;        /* Input signal step. NO CHECK ON BOUNDS */
      }
    else
      while ((Hp = &Imp[Ho>>Na]) < End) {
      t = *Hp;      /* Get IR sample */
      t *= *Xp;     /* Mult coeff by input sample */
      if (t & 1<<(Nhxn-1))  /* Round, if needed */
        t += 1<<(Nhxn-1);
      t >>= Nhxn;       /* Leave some guard bits, but come back some */
      v += t;           /* The filter output */
      Ho += dhb;        /* IR step */
      Xp += Inc;        /* Input signal step. NO CHECK ON BOUNDS */
      }
    return(v);
}



//ResampleClass * createResampler()
ResampleInterface * createResampler()
{
    return new ResampleClass();
}

//void  destroyResampler(ResampleClass * rsmpl)
void  destroyResampler(ResampleInterface * rsmpl)
{
    delete rsmpl;
}


