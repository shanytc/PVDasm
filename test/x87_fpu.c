/*
 * x87_fpu.c - x87 FPU instruction stress test for PVDasm
 *
 * Tests: all FPU arithmetic, load/store, transcendental,
 *        comparison, conditional moves, state management.
 *
 * Compile: cl /nologo /Od /GS- /Fe:x87_fpu.exe x87_fpu.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

static double g_d1 = 3.14159265;
static double g_d2 = 2.71828182;
static float  g_f1 = 1.5f;
static float  g_f2 = 2.5f;
static long   g_i1 = 42;
static short  g_i2 = 7;

/* ---- Basic FPU load/store ---- */
__declspec(noinline) void test_fpu_load_store(void)
{
    __asm {
        ; FLD (load float/double/extended)
        fld dword ptr [g_f1]
        fld qword ptr [g_d1]
        fld st(0)
        fld st(1)
        fld st(7)

        ; FILD (load integer)
        fild word ptr [g_i2]
        fild dword ptr [g_i1]

        ; FST / FSTP (store)
        fst dword ptr [g_f2]
        fst qword ptr [g_d2]
        fstp dword ptr [g_f1]
        fstp qword ptr [g_d1]

        ; FIST / FISTP (store as integer)
        fild dword ptr [g_i1]
        fist word ptr [g_i2]
        fistp dword ptr [g_i1]

        ; FLD constants
        fldz            ; push +0.0
        fld1            ; push +1.0
        fldpi           ; push pi
        fldl2e          ; push log2(e)
        fldl2t          ; push log2(10)
        fldlg2          ; push log10(2)
        fldln2          ; push ln(2)

        ; Clean up stack
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
    }
}

/* ---- FPU arithmetic ---- */
__declspec(noinline) void test_fpu_arithmetic(void)
{
    __asm {
        fld qword ptr [g_d1]
        fld qword ptr [g_d2]

        ; FADD
        fadd st(0), st(1)
        fadd dword ptr [g_f1]
        fadd qword ptr [g_d1]
        faddp st(1), st(0)

        fld qword ptr [g_d1]
        fld qword ptr [g_d2]

        ; FSUB / FSUBR
        fsub st(0), st(1)
        fsubr st(0), st(1)
        fsub dword ptr [g_f1]
        fsubr qword ptr [g_d1]
        fsubp st(1), st(0)

        fld qword ptr [g_d1]
        fld qword ptr [g_d2]

        ; FMUL
        fmul st(0), st(1)
        fmul dword ptr [g_f1]
        fmul qword ptr [g_d1]
        fmulp st(1), st(0)

        fld qword ptr [g_d1]
        fld qword ptr [g_d2]

        ; FDIV / FDIVR
        fdiv st(0), st(1)
        fdivr st(0), st(1)
        fdiv dword ptr [g_f1]
        fdivr qword ptr [g_d1]
        fdivp st(1), st(0)

        ; FIADD / FISUB / FIMUL / FIDIV (integer)
        fld qword ptr [g_d1]
        fiadd word ptr [g_i2]
        fisub word ptr [g_i2]
        fimul dword ptr [g_i1]
        fidiv dword ptr [g_i1]

        ; FABS / FCHS / FSQRT / FRNDINT
        fabs
        fchs
        fabs
        fsqrt
        frndint

        ; FPREM / FPREM1
        fld qword ptr [g_d2]
        fprem
        fprem1

        ; FSCALE / FXTRACT
        fscale
        fxtract

        ; Clean up
        fstp st(0)
        fstp st(0)
        fstp st(0)
    }
}

/* ---- FPU transcendental ---- */
__declspec(noinline) void test_fpu_transcendental(void)
{
    __asm {
        fld qword ptr [g_d1]

        ; Trigonometric
        fsin
        fcos
        fsincos
        fptan
        fpatan

        ; Logarithmic / exponential
        fld1
        fld qword ptr [g_d1]
        fabs
        fyl2x           ; st(1) * log2(st(0))

        fld1
        fld qword ptr [g_d1]
        fabs
        fyl2xp1         ; st(1) * log2(st(0)+1)

        fldl2e
        f2xm1           ; 2^st(0) - 1

        ; Clean up
        fstp st(0)
        fstp st(0)
        fstp st(0)
        fstp st(0)
    }
}

/* ---- FPU comparison ---- */
__declspec(noinline) void test_fpu_compare(void)
{
    __asm {
        fld qword ptr [g_d1]
        fld qword ptr [g_d2]

        ; FCOM / FCOMP / FCOMPP
        fcom st(1)
        fcom dword ptr [g_f1]
        fcomp st(1)
        fld qword ptr [g_d2]
        fcompp

        ; FTST
        fld qword ptr [g_d1]
        ftst

        ; FXAM
        fxam

        ; FUCOM / FUCOMP / FUCOMPP
        fld qword ptr [g_d2]
        fucom st(1)
        fucomp st(1)
        fld qword ptr [g_d2]
        fucompp

        ; FCOMI / FCOMIP / FUCOMI / FUCOMIP (Pentium Pro+)
        fld qword ptr [g_d1]
        fld qword ptr [g_d2]
        fcomi st(0), st(1)
        fcomip st(0), st(1)
        fld qword ptr [g_d2]
        fucomi st(0), st(1)
        fucomip st(0), st(1)

        ; Clean up
        fstp st(0)
    }
}

/* ---- FPU conditional move / exchange / state ---- */
__declspec(noinline) void test_fpu_misc(void)
{
    __asm {
        fld qword ptr [g_d1]
        fld qword ptr [g_d2]

        ; FXCH
        fxch st(1)

        ; FCMOVcc (conditional move, Pentium Pro+)
        fcmovb st(0), st(1)
        fcmove st(0), st(1)
        fcmovbe st(0), st(1)
        fcmovu st(0), st(1)
        fcmovnb st(0), st(1)
        fcmovne st(0), st(1)
        fcmovnbe st(0), st(1)
        fcmovnu st(0), st(1)

        ; FFREE
        ffree st(7)

        ; FINCSTP / FDECSTP
        fincstp
        fdecstp

        ; FNOP
        fnop

        ; FWAIT / WAIT
        fwait

        ; FINIT / FNINIT
        fninit

        ; FSTCW / FLDCW / FSTSW
        sub esp, 4
        fstcw word ptr [esp]
        fldcw word ptr [esp]
        fstsw ax
        fstsw word ptr [esp]
        add esp, 4

        ; Clean up
        fninit
    }
}

int main(void)
{
    test_fpu_load_store();
    test_fpu_arithmetic();
    test_fpu_transcendental();
    test_fpu_compare();
    test_fpu_misc();
    return 0;
}
