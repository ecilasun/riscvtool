#include "nekoichi.h"
#include <math.h>

int sinewave[1024] = {
    0x4000,0x4065,0x40c9,0x412e,0x4192,0x41f7,0x425b,0x42c0,
    0x4324,0x4388,0x43ed,0x4451,0x44b5,0x451a,0x457e,0x45e2,
    0x4646,0x46aa,0x470e,0x4772,0x47d6,0x4839,0x489d,0x4901,
    0x4964,0x49c7,0x4a2b,0x4a8e,0x4af1,0x4b54,0x4bb7,0x4c1a,
    0x4c7c,0x4cdf,0x4d41,0x4da4,0x4e06,0x4e68,0x4eca,0x4f2b,
    0x4f8d,0x4fee,0x5050,0x50b1,0x5112,0x5173,0x51d3,0x5234,
    0x5294,0x52f4,0x5354,0x53b4,0x5413,0x5473,0x54d2,0x5531,
    0x5590,0x55ee,0x564c,0x56ab,0x5709,0x5766,0x57c4,0x5821,
    0x587e,0x58db,0x5937,0x5993,0x59ef,0x5a4b,0x5aa7,0x5b02,
    0x5b5d,0x5bb8,0x5c12,0x5c6c,0x5cc6,0x5d20,0x5d79,0x5dd3,
    0x5e2b,0x5e84,0x5edc,0x5f34,0x5f8c,0x5fe3,0x603a,0x6091,
    0x60e7,0x613d,0x6193,0x61e8,0x623d,0x6292,0x62e7,0x633b,
    0x638e,0x63e2,0x6435,0x6488,0x64da,0x652c,0x657e,0x65cf,
    0x6620,0x6671,0x66c1,0x6711,0x6760,0x67af,0x67fe,0x684c,
    0x689a,0x68e7,0x6935,0x6981,0x69ce,0x6a1a,0x6a65,0x6ab0,
    0x6afb,0x6b45,0x6b8f,0x6bd8,0x6c21,0x6c6a,0x6cb2,0x6cfa,
    0x6d41,0x6d88,0x6dcf,0x6e15,0x6e5a,0x6e9f,0x6ee4,0x6f28,
    0x6f6c,0x6faf,0x6ff2,0x7034,0x7076,0x70b8,0x70f9,0x7139,
    0x7179,0x71b9,0x71f8,0x7236,0x7274,0x72b2,0x72ef,0x732c,
    0x7368,0x73a3,0x73df,0x7419,0x7453,0x748d,0x74c6,0x74ff,
    0x7537,0x756e,0x75a5,0x75dc,0x7612,0x7648,0x767d,0x76b1,
    0x76e5,0x7718,0x774b,0x777e,0x77b0,0x77e1,0x7812,0x7842,
    0x7871,0x78a1,0x78cf,0x78fd,0x792b,0x7958,0x7984,0x79b0,
    0x79db,0x7a06,0x7a30,0x7a59,0x7a82,0x7aab,0x7ad3,0x7afa,
    0x7b21,0x7b47,0x7b6d,0x7b92,0x7bb6,0x7bda,0x7bfd,0x7c20,
    0x7c42,0x7c64,0x7c85,0x7ca5,0x7cc5,0x7ce4,0x7d03,0x7d21,
    0x7d3f,0x7d5b,0x7d78,0x7d93,0x7daf,0x7dc9,0x7de3,0x7dfc,
    0x7e15,0x7e2d,0x7e45,0x7e5c,0x7e72,0x7e88,0x7e9d,0x7eb1,
    0x7ec5,0x7ed8,0x7eeb,0x7efd,0x7f0f,0x7f20,0x7f30,0x7f40,
    0x7f4f,0x7f5d,0x7f6b,0x7f78,0x7f85,0x7f91,0x7f9c,0x7fa7,
    0x7fb1,0x7fbb,0x7fc4,0x7fcc,0x7fd4,0x7fdb,0x7fe1,0x7fe7,
    0x7fec,0x7ff1,0x7ff5,0x7ff8,0x7ffb,0x7ffd,0x7fff,0x8000,
    0x8000,0x8000,0x7fff,0x7ffd,0x7ffb,0x7ff8,0x7ff5,0x7ff1,
    0x7fec,0x7fe7,0x7fe1,0x7fdb,0x7fd4,0x7fcc,0x7fc4,0x7fbb,
    0x7fb1,0x7fa7,0x7f9c,0x7f91,0x7f85,0x7f78,0x7f6b,0x7f5d,
    0x7f4f,0x7f40,0x7f30,0x7f20,0x7f0f,0x7efd,0x7eeb,0x7ed8,
    0x7ec5,0x7eb1,0x7e9d,0x7e88,0x7e72,0x7e5c,0x7e45,0x7e2d,
    0x7e15,0x7dfc,0x7de3,0x7dc9,0x7daf,0x7d93,0x7d78,0x7d5b,
    0x7d3f,0x7d21,0x7d03,0x7ce4,0x7cc5,0x7ca5,0x7c85,0x7c64,
    0x7c42,0x7c20,0x7bfd,0x7bda,0x7bb6,0x7b92,0x7b6d,0x7b47,
    0x7b21,0x7afa,0x7ad3,0x7aab,0x7a82,0x7a59,0x7a30,0x7a06,
    0x79db,0x79b0,0x7984,0x7958,0x792b,0x78fd,0x78cf,0x78a1,
    0x7871,0x7842,0x7812,0x77e1,0x77b0,0x777e,0x774b,0x7718,
    0x76e5,0x76b1,0x767d,0x7648,0x7612,0x75dc,0x75a5,0x756e,
    0x7537,0x74ff,0x74c6,0x748d,0x7453,0x7419,0x73df,0x73a3,
    0x7368,0x732c,0x72ef,0x72b2,0x7274,0x7236,0x71f8,0x71b9,
    0x7179,0x7139,0x70f9,0x70b8,0x7076,0x7034,0x6ff2,0x6faf,
    0x6f6c,0x6f28,0x6ee4,0x6e9f,0x6e5a,0x6e15,0x6dcf,0x6d88,
    0x6d41,0x6cfa,0x6cb2,0x6c6a,0x6c21,0x6bd8,0x6b8f,0x6b45,
    0x6afb,0x6ab0,0x6a65,0x6a1a,0x69ce,0x6981,0x6935,0x68e7,
    0x689a,0x684c,0x67fe,0x67af,0x6760,0x6711,0x66c1,0x6671,
    0x6620,0x65cf,0x657e,0x652c,0x64da,0x6488,0x6435,0x63e2,
    0x638e,0x633b,0x62e7,0x6292,0x623d,0x61e8,0x6193,0x613d,
    0x60e7,0x6091,0x603a,0x5fe3,0x5f8c,0x5f34,0x5edc,0x5e84,
    0x5e2b,0x5dd3,0x5d79,0x5d20,0x5cc6,0x5c6c,0x5c12,0x5bb8,
    0x5b5d,0x5b02,0x5aa7,0x5a4b,0x59ef,0x5993,0x5937,0x58db,
    0x587e,0x5821,0x57c4,0x5766,0x5709,0x56ab,0x564c,0x55ee,
    0x5590,0x5531,0x54d2,0x5473,0x5413,0x53b4,0x5354,0x52f4,
    0x5294,0x5234,0x51d3,0x5173,0x5112,0x50b1,0x5050,0x4fee,
    0x4f8d,0x4f2b,0x4eca,0x4e68,0x4e06,0x4da4,0x4d41,0x4cdf,
    0x4c7c,0x4c1a,0x4bb7,0x4b54,0x4af1,0x4a8e,0x4a2b,0x49c7,
    0x4964,0x4901,0x489d,0x4839,0x47d6,0x4772,0x470e,0x46aa,
    0x4646,0x45e2,0x457e,0x451a,0x44b5,0x4451,0x43ed,0x4388,
    0x4324,0x42c0,0x425b,0x41f7,0x4192,0x412e,0x40c9,0x4065,
    0x4000,0x3f9b,0x3f37,0x3ed2,0x3e6e,0x3e09,0x3da5,0x3d40,
    0x3cdc,0x3c78,0x3c13,0x3baf,0x3b4b,0x3ae6,0x3a82,0x3a1e,
    0x39ba,0x3956,0x38f2,0x388e,0x382a,0x37c7,0x3763,0x36ff,
    0x369c,0x3639,0x35d5,0x3572,0x350f,0x34ac,0x3449,0x33e6,
    0x3384,0x3321,0x32bf,0x325c,0x31fa,0x3198,0x3136,0x30d5,
    0x3073,0x3012,0x2fb0,0x2f4f,0x2eee,0x2e8d,0x2e2d,0x2dcc,
    0x2d6c,0x2d0c,0x2cac,0x2c4c,0x2bed,0x2b8d,0x2b2e,0x2acf,
    0x2a70,0x2a12,0x29b4,0x2955,0x28f7,0x289a,0x283c,0x27df,
    0x2782,0x2725,0x26c9,0x266d,0x2611,0x25b5,0x2559,0x24fe,
    0x24a3,0x2448,0x23ee,0x2394,0x233a,0x22e0,0x2287,0x222d,
    0x21d5,0x217c,0x2124,0x20cc,0x2074,0x201d,0x1fc6,0x1f6f,
    0x1f19,0x1ec3,0x1e6d,0x1e18,0x1dc3,0x1d6e,0x1d19,0x1cc5,
    0x1c72,0x1c1e,0x1bcb,0x1b78,0x1b26,0x1ad4,0x1a82,0x1a31,
    0x19e0,0x198f,0x193f,0x18ef,0x18a0,0x1851,0x1802,0x17b4,
    0x1766,0x1719,0x16cb,0x167f,0x1632,0x15e6,0x159b,0x1550,
    0x1505,0x14bb,0x1471,0x1428,0x13df,0x1396,0x134e,0x1306,
    0x12bf,0x1278,0x1231,0x11eb,0x11a6,0x1161,0x111c,0x10d8,
    0x1094,0x1051,0x100e,0xfcc,0xf8a,0xf48,0xf07,0xec7,
    0xe87,0xe47,0xe08,0xdca,0xd8c,0xd4e,0xd11,0xcd4,
    0xc98,0xc5d,0xc21,0xbe7,0xbad,0xb73,0xb3a,0xb01,
    0xac9,0xa92,0xa5b,0xa24,0x9ee,0x9b8,0x983,0x94f,
    0x91b,0x8e8,0x8b5,0x882,0x850,0x81f,0x7ee,0x7be,
    0x78f,0x75f,0x731,0x703,0x6d5,0x6a8,0x67c,0x650,
    0x625,0x5fa,0x5d0,0x5a7,0x57e,0x555,0x52d,0x506,
    0x4df,0x4b9,0x493,0x46e,0x44a,0x426,0x403,0x3e0,
    0x3be,0x39c,0x37b,0x35b,0x33b,0x31c,0x2fd,0x2df,
    0x2c1,0x2a5,0x288,0x26d,0x251,0x237,0x21d,0x204,
    0x1eb,0x1d3,0x1bb,0x1a4,0x18e,0x178,0x163,0x14f,
    0x13b,0x128,0x115,0x103,0xf1,0xe0,0xd0,0xc0,
    0xb1,0xa3,0x95,0x88,0x7b,0x6f,0x64,0x59,
    0x4f,0x45,0x3c,0x34,0x2c,0x25,0x1f,0x19,
    0x14,0xf,0xb,0x8,0x5,0x3,0x1,0x0,
    0x0,0x0,0x1,0x3,0x5,0x8,0xb,0xf,
    0x14,0x19,0x1f,0x25,0x2c,0x34,0x3c,0x45,
    0x4f,0x59,0x64,0x6f,0x7b,0x88,0x95,0xa3,
    0xb1,0xc0,0xd0,0xe0,0xf1,0x103,0x115,0x128,
    0x13b,0x14f,0x163,0x178,0x18e,0x1a4,0x1bb,0x1d3,
    0x1eb,0x204,0x21d,0x237,0x251,0x26d,0x288,0x2a5,
    0x2c1,0x2df,0x2fd,0x31c,0x33b,0x35b,0x37b,0x39c,
    0x3be,0x3e0,0x403,0x426,0x44a,0x46e,0x493,0x4b9,
    0x4df,0x506,0x52d,0x555,0x57e,0x5a7,0x5d0,0x5fa,
    0x625,0x650,0x67c,0x6a8,0x6d5,0x703,0x731,0x75f,
    0x78f,0x7be,0x7ee,0x81f,0x850,0x882,0x8b5,0x8e8,
    0x91b,0x94f,0x983,0x9b8,0x9ee,0xa24,0xa5b,0xa92,
    0xac9,0xb01,0xb3a,0xb73,0xbad,0xbe7,0xc21,0xc5d,
    0xc98,0xcd4,0xd11,0xd4e,0xd8c,0xdca,0xe08,0xe47,
    0xe87,0xec7,0xf07,0xf48,0xf8a,0xfcc,0x100e,0x1051,
    0x1094,0x10d8,0x111c,0x1161,0x11a6,0x11eb,0x1231,0x1278,
    0x12bf,0x1306,0x134e,0x1396,0x13df,0x1428,0x1471,0x14bb,
    0x1505,0x1550,0x159b,0x15e6,0x1632,0x167f,0x16cb,0x1719,
    0x1766,0x17b4,0x1802,0x1851,0x18a0,0x18ef,0x193f,0x198f,
    0x19e0,0x1a31,0x1a82,0x1ad4,0x1b26,0x1b78,0x1bcb,0x1c1e,
    0x1c72,0x1cc5,0x1d19,0x1d6e,0x1dc3,0x1e18,0x1e6d,0x1ec3,
    0x1f19,0x1f6f,0x1fc6,0x201d,0x2074,0x20cc,0x2124,0x217c,
    0x21d5,0x222d,0x2287,0x22e0,0x233a,0x2394,0x23ee,0x2448,
    0x24a3,0x24fe,0x2559,0x25b5,0x2611,0x266d,0x26c9,0x2725,
    0x2782,0x27df,0x283c,0x289a,0x28f7,0x2955,0x29b4,0x2a12,
    0x2a70,0x2acf,0x2b2e,0x2b8d,0x2bed,0x2c4c,0x2cac,0x2d0c,
    0x2d6c,0x2dcc,0x2e2d,0x2e8d,0x2eee,0x2f4f,0x2fb0,0x3012,
    0x3073,0x30d5,0x3136,0x3198,0x31fa,0x325c,0x32bf,0x3321,
    0x3384,0x33e6,0x3449,0x34ac,0x350f,0x3572,0x35d5,0x3639,
    0x369c,0x36ff,0x3763,0x37c7,0x382a,0x388e,0x38f2,0x3956,
    0x39ba,0x3a1e,0x3a82,0x3ae6,0x3b4b,0x3baf,0x3c13,0x3c78,
    0x3cdc,0x3d40,0x3da5,0x3e09,0x3e6e,0x3ed2,0x3f37,0x3f9b
};

int ssin(int s)
{
    return(sinewave[(s%1024)]-16384);
}

int main()
{
    int divider0=0;
    int divider1=0;
    int k=0;
    int l=0;

    while(1)
    {
        int B = 1 + (ssin(k)&0x0000000F);

        int R = B*ssin(k+l);
        int L = B*ssin(k+l);

        // TODO: Access APU instead of the audio port
        //*IO_AudioOutput = ((R&0xFFFF)<<16) | (L&0xFFFF);

        // Direct output mode
        *IO_APUFIFO = ((R&0xFFFF)<<16) | 0x0000; // WriteRightDirect
        *IO_APUFIFO = ((L&0xFFFF)<<16) | 0x0010; // WriteLeftDirect

        divider0++;
        if (divider0>64)
        {
            divider0=0;
            k+=1;
        }

        divider1++;
        if (divider1>16)
        {
            divider1=0;
            l+=1;
        }
    }

    return 0;
}