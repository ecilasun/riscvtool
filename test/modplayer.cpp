#include "nekoichi.h"
#include "SDCARD.h"
#include "FAT.h"

// Original post on pouet.net by https://www.pouet.net/user.php?who=182

#include "math.h"

#define sMin(_X,_Y) ((_X)<(_Y)) ? (_X):(_Y)
#define sMax(_X,_Y) ((_X)>(_Y)) ? (_X):(_Y)
#define sClamp(_X,_A,_B) sMin(sMax(_X,_A),_B)

const int PAULARATE=3740000; // approx. pal timing
const int OUTRATE=48000;     // approx. pal timing
const int OUTFPS=50;         // approx. pal timing

//------------------------------------------------------------------------------ 


const float sFPi=4*atanf(1);
union sIntFlt { unsigned int U32; float F32; };

template<typename T> T sSqr(T v) { return v*v; }
template<typename T> T sLerp(T a, T b, float f) { return a+f*(b-a); }
template<typename T> T sAbs(T x) { return abs(x); }
inline float sFSqrt(float x) { return sqrtf(x); }
inline float sFSin(float x) { return sinf(x); }
inline float sFCos(float x) { return cosf(x); }
inline float sFSinc(float x) { return x?sFSin(x)/x:1; }
inline float sFHamming(float x) { return (x>-1 && x<1)?sSqr(sFCos(x*sFPi/2)):0;}
inline float sFPow(float b, float e) { return powf(b,e); }
inline void sSetMem(void *dest, unsigned char v, int size) { __builtin_memset(dest,v,size); }
inline void sCopyMem(void *dest, const void *src, int size) { __builtin_memcpy(dest,src,size); }
inline void sZeroMem(void *dest, int size) { sSetMem(dest,0,size); } 


//------------------------------------------------------------------------------ 

class Paula
{
public:

  static const int FIR_WIDTH=512;
  float FIRMem[2*FIR_WIDTH+1];

  struct Voice
  {
  private:
    int Pos;
    int PWMCnt, DivCnt;
    sIntFlt Cur;

  public:
    char* Sample;
    int SampleLen;
    int LoopLen;
    int Period; // 124 .. 65535
    int Volume; // 0 .. 64

    Voice() : Period(65535), Volume(0), Sample(0), Pos(0), PWMCnt(0), DivCnt(0), LoopLen(1) { Cur.F32=0; }
    void Render(float *buffer, int samples)
    {
      if (!Sample || !Volume) return;

      unsigned char *smp=(unsigned char*)Sample;
      for (int i=0; i<samples; i++)
      {
        if (!DivCnt)
        {
          // todo: use a fake d/a table for this
          Cur.U32=((smp[Pos]^0x80)<<15)|0x40000000;
          Cur.F32-=3.0f;
          if (++Pos==SampleLen) Pos-=LoopLen;
          DivCnt=Period;
        }
        if (PWMCnt<Volume) buffer[i]+=Cur.F32;
        PWMCnt=(PWMCnt+1)&0x3f;
        DivCnt--;
      }
    }

    void Trigger(char *smp,int sl, int ll, int offs=0)
    {
      Sample=smp;
      SampleLen=sl;
      LoopLen=ll;
      Pos=sMin(offs,SampleLen-1);
    }
  };

  Voice V[4];

  // rendering in paula freq
  static const int RBSIZE = 4096;
  float RingBuf[2*RBSIZE];
  int WritePos;
  int ReadPos;
  float ReadFrac;
  //float FltFreq;
  //float FltBuf;

  void CalcFrag(float *out, int samples)
  {
    sZeroMem(out,sizeof(float)*samples);
    sZeroMem(out+RBSIZE,sizeof(float)*samples);
    for (int i=0; i<4; i++)
    {
      if (i==1 || i==2)
        V[i].Render(out+RBSIZE,samples);
      else
        V[i].Render(out,samples);
    }
  }

  void Calc() // todo: stereo
  {
    int RealReadPos=ReadPos-FIR_WIDTH-1;
    int samples=(RealReadPos-WritePos)&(RBSIZE-1);

    int todo=sMin(samples,RBSIZE-WritePos);
    CalcFrag(RingBuf+WritePos,todo);
    if (todo<samples)
    {
      WritePos=0;
      todo=samples-todo;
      CalcFrag(RingBuf,todo);
    }
    WritePos+=todo;
  };

  float MasterVolume;
  float MasterSeparation;

  // rendering in output freq
  void Render(float *outbuf, int samples)
  {
    const float step=float(PAULARATE)/float(OUTRATE);
    const float pan=0.5f+0.5f*MasterSeparation;
    const float vm0=MasterVolume*sFSqrt(pan);
    const float vm1=MasterVolume*sFSqrt(1-pan);

    for (int s=0; s<samples; s++)
    {
      int ReadEnd=ReadPos+FIR_WIDTH+1;
      if (WritePos<ReadPos) ReadEnd-=RBSIZE;
      if (ReadEnd>WritePos) Calc();
      float outl0=0, outl1=0;
      float outr0=0, outr1=0;

      // this needs optimization. SSE would come to mind.
      int offs=(ReadPos-FIR_WIDTH-1)&(RBSIZE-1);
      float vl=RingBuf[offs];
      float vr=RingBuf[offs+RBSIZE];
      for (int i=1; i<2*FIR_WIDTH-1; i++)
      {
        float w=FIRMem[i];
        outl0+=vl*w;
        outr0+=vr*w;
        offs=(offs+1)&(RBSIZE-1);
        vl=RingBuf[offs];
        vr=RingBuf[offs+RBSIZE];
        outl1+=vl*w;
        outr1+=vr*w;
      }
      float outl=sLerp(outl0,outl1,ReadFrac);
      float outr=sLerp(outr0,outr1,ReadFrac);
      *outbuf++=vm0*outl+vm1*outr;
      *outbuf++=vm1*outl+vm0*outr;

      ReadFrac+=step;
      int rfi=int(ReadFrac);
      ReadPos=(ReadPos+rfi)&(RBSIZE-1);
      ReadFrac-=rfi;
    }
  }

  Paula()
  {
    // make FIR table
    float *FIRTable=FIRMem+FIR_WIDTH;
    float yscale=float(OUTRATE)/float(PAULARATE);
    float xscale=sFPi*yscale;
    for (int i=-FIR_WIDTH; i<=FIR_WIDTH; i++)
      FIRTable[i]=yscale*sFSinc(float(i)*xscale)*sFHamming(float(i)/float(FIR_WIDTH-1));

    sZeroMem(RingBuf,sizeof(RingBuf));
    ReadPos=0;
    ReadFrac=0;
    WritePos=FIR_WIDTH;

    MasterVolume=0.66f;
    MasterSeparation=0.5f;
    //FltBuf=0;
  }
};

//------------------------------------------------------------------------------ 

class ModPlayer
{

  Paula *P;

  static inline void SwapEndian(unsigned short &v) { v=((v&0xff)<<8)|(v>>8); }

  static int BasePTable[5*12+1];
  static int PTable[16][60];
  static int VibTable[3][15][64];

  struct Sample
  {
    char Name[22];
    unsigned short Length;
    char  Finetune;
    unsigned char  Volume;
    unsigned short LoopStart;
    unsigned short LoopLen;

    void Prepare()
    {
      SwapEndian(Length);
      SwapEndian(LoopStart);
      SwapEndian(LoopLen);
      Finetune&=0x0f;
      if (Finetune>=8) Finetune-=16;
    }
  };

  struct Pattern
  {
    struct Event
    {
      int Sample;
      int Note;
      int FX;
      int FXParm;
    } Events[64][4];

    Pattern() { sZeroMem(this,sizeof(Pattern)); }

    void Load(unsigned char *ptr)
    {
      for (int row=0; row<64; row++) for (int ch=0; ch<4; ch++)
      {
        Event &e=Events[row][ch];
        e.Sample = (ptr[0]&0xf0)|(ptr[2]>>4);
        e.FX     = ptr[2]&0x0f;
        e.FXParm = ptr[3];

        e.Note=0;        
        int period = (int(ptr[0]&0x0f)<<8)|ptr[1];
        int bestd = sAbs(period-BasePTable[0]);
        if (period) for (int i=1; i<=60; i++)
        {
          int d=sAbs(period-BasePTable[i]);
          if (d<bestd)
          {
            bestd=d;
            e.Note=i;
          }
        }

        ptr+=4;
      }
    }
  };

  Sample *Samples;
  char    *SData[32];
  int   SampleCount;
  int   ChannelCount;

  unsigned char    PatternList[128];
  int   PositionCount;
  int   PatternCount;

  Pattern Patterns[128];

  struct Chan
  {
    int Note;
    int Period;
    int Sample;
    int FineTune;
    int Volume;
    int FXBuf[16];
    int FXBuf14[16];
    int LoopStart;
    int LoopCount;
    int RetrigCount;
    int VibWave;
    int VibRetr;
    int VibPos;
    int TremWave;
    int TremRetr;
    int TremPos;

    Chan() { sZeroMem(this,sizeof(Chan)); }
    
    int GetPeriod(int offs=0, int fineoffs=0) 
    { 
      int ft=FineTune+fineoffs;
      while (ft>7) { offs++; ft-=16; }
      while (ft<-8) { offs--; ft+=16; }
      return Note?(PTable[ft&0x0f][sClamp(Note+offs-1,0,59)]):0; 
    }
    void SetPeriod(int offs=0, int fineoffs=0) { if (Note) Period=GetPeriod(offs,fineoffs); }

  } Chans[4];

  int Speed;
  int TickRate;
  int TRCounter;

  int CurTick;
  int CurRow;
  int CurPos;
  int Delay;

  void CalcTickRate(int bpm)
  {
    TickRate=(125*OUTRATE)/(bpm*OUTFPS);
  }

  void TrigNote(int ch, const Pattern::Event &e)
  {
    Chan &c=Chans[ch];
    Paula::Voice &v=P->V[ch];
    const Sample &s=Samples[c.Sample];
    int offset=0;

    if (e.FX==9) offset=c.FXBuf[9]<<8;
    if (e.FX!=3 && e.FX!=5)
    {
      c.SetPeriod();
      if (s.LoopLen>1)
        v.Trigger(SData[c.Sample],2*(s.LoopStart+s.LoopLen),2*s.LoopLen,offset);
      else
        v.Trigger(SData[c.Sample],v.SampleLen=2*s.Length,1,offset);
      if (!c.VibRetr) c.VibPos=0;
      if (!c.TremRetr) c.TremPos=0;
    }
    
  }

  void Reset()
  {
    CalcTickRate(125);
    Speed=6;
    TRCounter=0;
    CurTick=0;
    CurRow=0;
    CurPos=0;
    Delay=0;
  }


  void Tick()
  {
    const Pattern &p=Patterns[PatternList[CurPos]];
    const Pattern::Event *re=p.Events[CurRow];
    for (int ch=0; ch<4; ch++)
    {
      const Pattern::Event &e=re[ch];
      Paula::Voice &v=P->V[ch];
      Chan &c=Chans[ch];
      const int fxpl=e.FXParm&0x0f;
      int TremVol=0;
      if (!CurTick)
      {
        if (e.Sample)
        {
          c.Sample=e.Sample;
          c.FineTune=Samples[c.Sample].Finetune;
          c.Volume=Samples[c.Sample].Volume;
        }

        if (e.FXParm)
          c.FXBuf[e.FX]=e.FXParm;
        
        if (e.Note && (e.FX!=14 || ((e.FXParm>>4)!=13)))
        {
          c.Note=e.Note;
          TrigNote(ch,e);
        }

        switch (e.FX)
        {
        case 4: // vibrato
          if (c.FXBuf[4]&0x0f) c.SetPeriod(0,VibTable[c.VibWave][(c.FXBuf[4]&0x0f)-1][c.VibPos]);
          break;
        case 7: // tremolo
          if (c.FXBuf[7]&0x0f) TremVol=VibTable[c.TremWave][(c.FXBuf[7]&0x0f)-1][c.TremPos];
          break;
        case 12: // set vol
          c.Volume=sClamp(e.FXParm,0,64);
          break;
        case 14: // special
          if (fxpl) c.FXBuf14[e.FXParm>>4]=fxpl;
          switch (e.FXParm>>4)
          {
          case 0: // set filter
            break;
          case 1: // fineslide up
            c.Period=sMax(113,c.Period-c.FXBuf14[1]);
            break;
          case 2: // slide down
            c.Period=sMin(856,c.Period+c.FXBuf14[2]);
            break;
          case 3: // set glissando sucks!
            break;
          case 4: // set vib waveform
            c.VibWave=fxpl&3;
            if (c.VibWave==3) c.VibWave=0;
            c.VibRetr=fxpl&4;
            break;
          case 5: // set finetune
            c.FineTune=fxpl;
            if (c.FineTune>=8) c.FineTune-=16;
            break;
          case 7:  // set tremolo 
            c.TremWave=fxpl&3;
            if (c.TremWave==3) c.TremWave=0;
            c.TremRetr=fxpl&4;
            break;
          case 9: // retrigger
            if (c.FXBuf14[9] && !e.Note)
              TrigNote(ch,e);
            c.RetrigCount=0;
            break;
          case 10: // fine volslide up
            c.Volume=sMin(c.Volume+c.FXBuf14[10],64);
            break;
          case 11: // fine volslide down;
            c.VibRetr=sMax(c.Volume-c.FXBuf14[11],0);
            break;
          case 14: // delay pattern
            Delay=c.FXBuf14[14];
            break;
          case 15: // invert loop (WTF)
            break;            
          }
          break;
        case 15: // set speed
          if (e.FXParm)
            if (e.FXParm<=32)
              Speed=e.FXParm;
            else
              CalcTickRate(e.FXParm);
          break;
        }


      }
      else
      {
        switch (e.FX)
        {
        case 0: // arpeggio
          if (e.FXParm)
          {
            int no=0;
            switch (CurTick%3)
            {
            case 1: no=e.FXParm>>4; break;
            case 2: no=e.FXParm&0x0f; break;
            }
            c.SetPeriod(no);
          }
          break;
        case 1: // slide up
          c.Period=sMax(113,c.Period-c.FXBuf[1]);
          break;
        case 2: // slide down
          c.Period=sMin(856,c.Period+c.FXBuf[2]);
          break;
        case 5: // slide plus volslide
          if (c.FXBuf[5]&0xf0)
            c.Volume=sMin(c.Volume+(c.FXBuf[5]>>4),0x40);
          else
            c.Volume=sMax(c.Volume-(c.FXBuf[5]&0x0f),0);
          // no break!
        case 3: // slide to note
          {
            int np=c.GetPeriod();
            if (c.Period>np)
              c.Period=sMax(c.Period-c.FXBuf[3],np);
            else if (c.Period<np)
              c.Period=sMin(c.Period+c.FXBuf[3],np);
          }
          break;
        case 6: // vibrato plus volslide
          if (c.FXBuf[6]&0xf0)
            c.Volume=sMin(c.Volume+(c.FXBuf[6]>>4),0x40);
          else
            c.Volume=sMax(c.Volume-(c.FXBuf[6]&0x0f),0);
          // no break!
        case 4: // vibrato ???
          if (c.FXBuf[4]&0x0f) c.SetPeriod(0,VibTable[c.VibWave][(c.FXBuf[4]&0x0f)-1][c.VibPos]);
          c.VibPos=(c.VibPos+(c.FXBuf[4]>>4))&0x3f;
          break;
        case 7: // tremolo ???
          if (c.FXBuf[7]&0x0f) TremVol=VibTable[c.TremWave][(c.FXBuf[7]&0x0f)-1][c.TremPos];
          c.TremPos=(c.TremPos+(c.FXBuf[7]>>4))&0x3f;
          break;
        case 10: // volslide
          if (c.FXBuf[10]&0xf0)
            c.Volume=sMin(c.Volume+(c.FXBuf[10]>>4),0x40);
          else
            c.Volume=sMax(c.Volume-(c.FXBuf[10]&0x0f),0);
          break;
        case 11: // pos jump
          if (CurTick==Speed-1)
          {
            CurRow=-1;
            CurPos=e.FXParm;
          }
          break;
        case 13: // pattern break
          if (CurTick==Speed-1)
          {
            CurPos++;
            CurRow=(10*(e.FXParm>>4)+(e.FXParm&0x0f))-1;
          }
          break;
        case 14: // special
          switch (e.FXParm>>4)
          {
          case 6: // loop pattern
            if (!fxpl) // loop start
              c.LoopStart=CurRow;
            else
              if (c.LoopCount<fxpl)
              {
                CurRow=c.LoopStart-1;
                c.LoopCount++;
              }
              else
                c.LoopCount=0;
            break;
          case 9: // retrigger
            if (++c.RetrigCount == c.FXBuf14[9])
            {
              c.RetrigCount=0;
              TrigNote(ch,e);
            }
            break;
          case 12: // cut
            if (CurTick==c.FXBuf14[12])
              c.Volume=0;
            break;
          case 13: // delay
            if (CurTick==c.FXBuf14[13])
              TrigNote(ch,e);
            break;
          }
          break;       
        }
      }

      v.Volume=sClamp(c.Volume+TremVol,0,64);
      v.Period=c.Period;
    }

    CurTick++;
    if (CurTick>=Speed*(Delay+1))
    {
      CurTick=0;
      CurRow++;
      Delay=0;
    }
    if (CurRow>=64)
    {
      CurRow=0;
      CurPos++;
    }
    if (CurPos>=PositionCount)
      CurPos=0;
  };

public:

  char Name[21];

  ModPlayer(Paula *p, unsigned char *moddata) : P(p)
  {
    // calc ptable
    for (int ft=0; ft<16; ft++)
    {
      int rft= -((ft>=8)?ft-16:ft);
      float fac=sFPow(2.0f,float(rft)/(12.0f*16.0f));
      for (int i=0; i<60; i++)
        PTable[ft][i]=int(float(BasePTable[i])*fac+0.5f);
    }

    // calc vibtable
    for (int ampl=0; ampl<15; ampl++)
    {
      float scale=ampl+1.5f;
      float shift=0;
      for (int x=0; x<64; x++)
      {
        VibTable[0][ampl][x]=int(scale*sFSin(x*sFPi/32.0f)+shift);
        VibTable[1][ampl][x]=int(scale*((63-x)/31.5f-1.0f)+shift);
        VibTable[2][ampl][x]=int(scale*((x<32)?1:-1)+shift);
      }
    }

    // "load" the mod
    __builtin_memcpy(Name,moddata,20); Name[20]=0; moddata+=20;

    SampleCount=16;
    ChannelCount=4;
    Samples=(Sample*)(moddata-sizeof(Sample)); moddata+=15*sizeof(Sample);
    unsigned int &tag=*(unsigned int*)(moddata+130+16*sizeof(Sample));
    switch (tag)
    {
    case '.K.M': case '4LTF': case '!K!M':
      SampleCount=32;
      break;
    }
    if (SampleCount>16)
      moddata+=(SampleCount-16)*sizeof(Sample);
    for (int i=1; i<SampleCount; i++) Samples[i].Prepare();

    PositionCount=*moddata; moddata+=2; // + skip unused byte
    __builtin_memcpy(PatternList,moddata,128); moddata+=128;
    if (SampleCount>15) moddata+=4; // skip tag

    PatternCount=0;
    for (int i=0; i<128; i++)
      PatternCount=sClamp(PatternCount,PatternList[i]+1,128);

    for (int i=0; i<PatternCount; i++)
    {
      Patterns[i].Load(moddata);
      moddata+=1024;
    }

    sZeroMem(SData,sizeof(SData));
    for (int i=1; i<SampleCount; i++)
    {
      SData[i]=(char*)moddata;
      moddata+=2*Samples[i].Length;
    }

    Reset();
  }

  unsigned int Render(float *buf, unsigned int len)
  {
    while (len)
    {
      int todo=sMin(len,TRCounter);
      if (todo)
      {
        P->Render(buf,todo);
        buf+=2*todo;
        len-=todo;
        TRCounter-=todo;
      }
      else
      {
        Tick();
        TRCounter=TickRate;
      }
    }
    return 1;
  }
  
  /*static unsigned int __stdcall RenderProxy(void *parm, float *buf, unsigned int len)
  { 
    return ((ModPlayer*)parm)->Render(buf,len); 
  }*/

};

int ModPlayer::BasePTable[61]=
{
  0, 1712,1616,1525,1440,1357,1281,1209,1141,1077,1017, 961, 907,
   856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
   428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
   214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
   107, 101,  95,  90,  85,  80,  76,  71,  67,  64,  60,  57,
};

int ModPlayer::PTable[16][60];
int ModPlayer::VibTable[3][15][64];

//------------------------------------------------------------------------------ 

FATFS Fs;
int main(int argc, char* argv[])
{
    pf_mount(&Fs);
    FRESULT fr = pf_open("SONG.MOD");
    if (fr == FR_OK)
    {
        int size=Fs.fsize;
        unsigned char *mod = new unsigned char[size];
        WORD *bytesread=0;
        pf_read(mod, size, bytesread);
        //pf_close();

        Paula P;
        ModPlayer player(&P, mod);

        while (1)
        {
            float tmpbuf[64];
            player.Render(tmpbuf, 64);

            for (int i=0;i<64;++i)
                *IO_AudioOutput = (uint32_t)(65536.f*tmpbuf[i]);
        }

        delete[] mod;
    }
  
    return 0;
}
