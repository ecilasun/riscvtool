#include "core.h"
#include "uart.h"

#include <stdio.h>
#include <math.h>

/*uint32_t asuint(const float F)
{
    float K = F;
    uint32_t *Kasu32 = (uint32_t*)(&K);
    return *Kasu32;
}*/

/*float asfloat(const uint32_t I)
{
    uint32_t K = I;
    float *Kasf = (float*)(&K);
    return *Kasf;
}*/

float __attribute((naked)) test_div()
{
    asm volatile (
        "la a0, test_div_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fdiv.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_div_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_mul()
{
    asm volatile (
        "la a0, test_mul_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fmul.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_mul_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_add()
{
    asm volatile (
        "la a0, test_add_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fadd.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_add_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_sub()
{
    asm volatile (
        "la a0, test_sub_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsub.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_sub_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_min()
{
    asm volatile (
        "la a0, test_min_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fmin.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_min_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_max()
{
    asm volatile (
        "la a0, test_max_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fmax.s fa0, fs0, fs1 ;"
        "ret ;"
        "test_max_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_sqrt()
{
    asm volatile (
        "la a0, test_sqrt_data ;"
        "flw fs1, 4(a0) ;"
        "fsqrt.s fa0, fs1 ;"
        "ret ;"
        "test_sqrt_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_madd()
{
    asm volatile (
        "la a0, test_madd_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fmadd.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_madd_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

float __attribute((naked)) test_msub()
{
    asm volatile (
        "la a0, test_msub_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fmsub.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_msub_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

float __attribute((naked)) test_nmsub()
{
    asm volatile (
        "la a0, test_nmsub_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fnmsub.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_nmsub_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

float __attribute((naked)) test_nmadd()
{
    asm volatile (
        "la a0, test_nmadd_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flw fs2, 8(a0) ;"
        "fnmadd.s fa0, fs0,fs1,fs2 ;"
        "ret ;"
        "test_nmadd_data: ;"
        ".float 1.1324145 ;"
        ".float 553.538131 ;"
        ".float 33.784341 ;"
        :  : :
    );
}

int __attribute((naked)) test_fcvtws()
{
    asm volatile (
        "la a0, test_cvtws_data ;"
        "flw fs0, 0(a0) ;"
        "fcvt.w.s a0, fs0, rtz ;"
        "ret ;"
        "test_cvtws_data: ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fcvtsw()
{
    asm volatile (
        "la a0, test_cvtsw_data ;"
        "lw a1, 0(a0) ;"
        "fcvt.s.w fa0, a1 ;"
        "ret ;"
        "test_cvtsw_data: ;"
        ".int 554 ;"
        :  : :
    );
}

float __attribute((naked)) test_fsgnj()
{
    asm volatile (
        "la a0, test_sgnj_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsgnj.s fa0,fs0,fs1 ;"
        "ret ;"
        "test_sgnj_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fsgnjn()
{
    asm volatile (
        "la a0, test_sgnjn_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsgnjn.s fa0,fs0,fs1 ;"
        "ret ;"
        "test_sgnjn_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fsgnjx()
{
    asm volatile (
        "la a0, test_sgnjx_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fsgnjx.s fa0,fs0,fs1 ;"
        "ret ;"
        "test_sgnjx_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_flt()
{
    asm volatile (
        "la a0, test_lt_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "flt.s a0,fs0,fs1 ;"
        "ret ;"
        "test_lt_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_fle()
{
    asm volatile (
        "la a0, test_le_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "fle.s a0,fs0,fs1 ;"
        "ret ;"
        "test_le_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_feq()
{
    asm volatile (
        "la a0, test_eq_data ;"
        "flw fs0, 0(a0) ;"
        "flw fs1, 4(a0) ;"
        "feq.s a0,fs0,fs1 ;"
        "ret ;"
        "test_eq_data: ;"
        ".float -111.5555 ;"
        ".float 553.538131 ;"
        :  : :
    );
}

int __attribute((naked)) test_fmvxw()
{
    asm volatile (
        "la a0, test_mvxw_data ;"
        "flw fs0, 0(a0) ;"
        "fmv.x.w a0,fs0 ;"
        "ret ;"
        "test_mvxw_data: ;"
        ".float 553.538131 ;"
        :  : :
    );
}

float __attribute((naked)) test_fmvwx()
{
    asm volatile (
        "la a0, test_mvwx_data ;"
        "lw a1, 0(a0) ;"
        "fmv.w.x fa0,a1 ;"
        "ret ;"
        "test_mvwx_data: ;"
        ".int 0x440a6271 ;" // 553.53814697265625
        :  : :
    );
}

int main()
{
    setbuf(stdout, NULL);

    const float a = 1.3124145f;
    const float b = 553.538131f;
    const float c = 33.784341f;
    const float d = -111.5555f;
    float r = 0.f;
    int t = 554;
    int s = 0x440a6271;
    int ri = 0;

    printf("Floating point unit instruction set test\n");
    printf("Single precision\n\n");

    r = test_div();
    printf("fdiv: %f / %f = %f\n", a, b, r);

    r = test_mul();
    printf("fmul: %f * %f = %f\n", a, b, r);

    r = test_add();
    printf("fadd: %f + %f = %f\n", a, b, r);

    r = test_sub();
    printf("fsub: %f - %f = %f\n", a, b, r);

    r = test_min();
    printf("fmin: min(%f,%f) = %f\n", a, b, r);

    r = test_max();
    printf("fmax: max(%f,%f) = %f\n", a, b, r);

    r = test_sqrt();
    printf("fsqrt: sqrt(%f) = %f\n", b, r);

    r = test_madd();
    printf("fmadd: %f*%f+%f = %f\n", a, b, c, r);

    r = test_msub();
    printf("fmsub: %f*%f-%f = %f\n", a, b, c, r);

    r = test_nmsub();
    printf("fnmsub: -%f*%f+%f = %f\n", a, b, c, r);

    r = test_nmadd();
    printf("fnmadd: -%f*%f-%f = %f\n", a, b, c, r);

    ri = test_fcvtws();
    printf("fcvt.w.s: %f = %d\n", b, ri);

    r = test_fcvtsw();
    printf("fcvt.s.w: %d = %f\n", t, r);

    r = test_fsgnj();
    printf("fsgnj.s: sgnj(%f,%f) = %f\n", d, b, r);

    r = test_fsgnjn();
    printf("fsgnjn.s: sgnj(%f,%f) = %f\n", d, b, r);

    r = test_fsgnjx();
    printf("fsgnjx.s: sgnj(%f,%f) = %f\n", d, b, r);

    ri = test_flt();
    printf("flt.s: %f < %f ? = %d\n", d, b, ri);

    ri = test_fle();
    printf("fle.s: %f <= %f ? = %d\n", d, b, ri);

    ri = test_feq();
    printf("feq.s: %f == %f ? = %d\n", d, b, ri);

    ri = test_fmvxw();
    printf("fmv.x.w: %f -> %.8X\n", b, ri);

    r = test_fmvwx();
    printf("fmv.w.x: %.8X -> %f\n", s, r);

    printf("All tests done\n");

    return 0;
}
