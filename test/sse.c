/*
 * sse.c - SSE (Streaming SIMD Extensions) instruction stress test for PVDasm
 *
 * Tests: all SSE packed/scalar single-precision float ops,
 *        data movement, shuffles, comparisons, conversions.
 *
 * Compile: cl /nologo /Od /GS- /arch:SSE /Fe:sse.exe sse.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

static float __declspec(align(16)) g_ps1[4] = {1.0f, 2.0f, 3.0f, 4.0f};
static float __declspec(align(16)) g_ps2[4] = {5.0f, 6.0f, 7.0f, 8.0f};
static float g_ss1 = 1.5f;

/* ---- SSE data movement ---- */
__declspec(noinline) void test_sse_move(void)
{
    __asm {
        ; MOVAPS (aligned packed single)
        movaps xmm0, [g_ps1]
        movaps xmm1, [g_ps2]
        movaps xmm2, xmm0
        movaps [g_ps2], xmm1

        ; MOVUPS (unaligned packed single)
        movups xmm3, [g_ps1]
        movups xmm4, [g_ps2]
        movups [g_ps1], xmm3

        ; MOVSS (scalar single)
        movss xmm5, [g_ss1]
        movss xmm6, xmm5
        movss [g_ss1], xmm5

        ; MOVLPS / MOVHPS (low/high packed single)
        movlps xmm0, [g_ps1]
        movhps xmm0, [g_ps2]
        movlps [g_ps1], xmm1
        movhps [g_ps2], xmm1

        ; MOVLHPS / MOVHLPS (between registers)
        movlhps xmm0, xmm1
        movhlps xmm2, xmm3

        ; MOVMSKPS (extract sign bits)
        movmskps eax, xmm0
    }
}

/* ---- SSE arithmetic ---- */
__declspec(noinline) void test_sse_arithmetic(void)
{
    __asm {
        movaps xmm0, [g_ps1]
        movaps xmm1, [g_ps2]
        movss  xmm2, [g_ss1]

        ; Packed single
        addps xmm0, xmm1
        addps xmm0, [g_ps2]
        subps xmm0, xmm1
        mulps xmm0, xmm1
        divps xmm0, xmm1
        sqrtps xmm3, xmm0
        rcpps xmm4, xmm0
        rsqrtps xmm5, xmm0
        maxps xmm0, xmm1
        minps xmm0, xmm1

        ; Scalar single (F3 prefix)
        addss xmm2, xmm0
        addss xmm2, [g_ss1]
        subss xmm2, xmm0
        mulss xmm2, xmm0
        divss xmm2, xmm0
        sqrtss xmm3, xmm2
        rcpss xmm4, xmm2
        rsqrtss xmm5, xmm2
        maxss xmm2, xmm0
        minss xmm2, xmm0
    }
}

/* ---- SSE logical ---- */
__declspec(noinline) void test_sse_logical(void)
{
    __asm {
        movaps xmm0, [g_ps1]
        movaps xmm1, [g_ps2]

        ; Bitwise logical
        andps xmm0, xmm1
        andnps xmm0, xmm1
        orps xmm0, xmm1
        xorps xmm0, xmm1

        ; Memory operand
        andps xmm2, [g_ps1]
        orps xmm3, [g_ps2]
    }
}

/* ---- SSE comparison ---- */
__declspec(noinline) void test_sse_compare(void)
{
    __asm {
        movaps xmm0, [g_ps1]
        movaps xmm1, [g_ps2]

        ; CMPPS (packed compare with immediate predicate)
        cmpeqps xmm0, xmm1          ; predicate 0
        cmpltps xmm0, xmm1          ; predicate 1
        cmpleps xmm0, xmm1          ; predicate 2
        cmpunordps xmm0, xmm1       ; predicate 3
        cmpneqps xmm0, xmm1         ; predicate 4
        cmpnltps xmm0, xmm1         ; predicate 5
        cmpnleps xmm0, xmm1         ; predicate 6
        cmpordps xmm0, xmm1         ; predicate 7

        ; CMPSS (scalar compare with immediate predicate)
        cmpeqss xmm0, xmm1          ; predicate 0
        cmpltss xmm0, xmm1          ; predicate 1
        cmpless xmm0, xmm1          ; predicate 2
        cmpunordss xmm0, xmm1       ; predicate 3
        cmpneqss xmm0, xmm1         ; predicate 4
        cmpnltss xmm0, xmm1         ; predicate 5
        cmpnless xmm0, xmm1         ; predicate 6
        cmpordss xmm0, xmm1         ; predicate 7

        ; UCOMISS / COMISS
        ucomiss xmm0, xmm1
        comiss xmm0, xmm1
        ucomiss xmm0, [g_ss1]
    }
}

/* ---- SSE shuffle / unpack ---- */
__declspec(noinline) void test_sse_shuffle(void)
{
    __asm {
        movaps xmm0, [g_ps1]
        movaps xmm1, [g_ps2]

        ; SHUFPS (shuffle with immediate)
        shufps xmm0, xmm1, 00h
        shufps xmm0, xmm1, 1Bh
        shufps xmm0, xmm1, 0FFh
        shufps xmm0, [g_ps2], 93h

        ; UNPCKLPS / UNPCKHPS
        unpcklps xmm0, xmm1
        unpckhps xmm0, xmm1
        unpcklps xmm2, [g_ps1]
        unpckhps xmm3, [g_ps2]
    }
}

/* ---- SSE conversion ---- */
__declspec(noinline) void test_sse_convert(void)
{
    __asm {
        movaps xmm0, [g_ps1]

        ; CVTPI2PS (MMX int -> packed float)
        movd mm0, eax
        cvtpi2ps xmm0, mm0

        ; CVTPS2PI (packed float -> MMX int)
        cvtps2pi mm0, xmm0

        ; CVTTPS2PI (truncated)
        cvttps2pi mm0, xmm0

        ; CVTSI2SS (scalar int -> scalar float)
        cvtsi2ss xmm0, eax
        cvtsi2ss xmm0, [g_ss1]

        ; CVTSS2SI (scalar float -> scalar int)
        cvtss2si eax, xmm0

        ; CVTTSS2SI (truncated)
        cvttss2si eax, xmm0

        emms
    }
}

/* ---- SSE state / prefetch / misc ---- */
__declspec(noinline) void test_sse_misc(void)
{
    __asm {
        ; LDMXCSR / STMXCSR
        sub esp, 4
        stmxcsr [esp]
        ldmxcsr [esp]
        add esp, 4

        ; PREFETCH
        prefetchnta [g_ps1]
        prefetcht0  [g_ps1]
        prefetcht1  [g_ps1]
        prefetcht2  [g_ps1]

        ; SFENCE
        sfence

        ; MOVNTPS (non-temporal store)
        movaps xmm0, [g_ps1]
        movntps [g_ps2], xmm0

        ; FXSAVE / FXRSTOR (if we had a buffer, but let's just test encoding)
        ; These need 512-byte aligned buffer - skip actual execution
    }
}

int main(void)
{
    test_sse_move();
    test_sse_arithmetic();
    test_sse_logical();
    test_sse_compare();
    test_sse_shuffle();
    test_sse_convert();
    test_sse_misc();
    return 0;
}
