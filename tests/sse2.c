/*
 * sse2.c - SSE2 instruction stress test for PVDasm
 *
 * Tests: double-precision packed/scalar ops, integer SIMD on XMM,
 *        data movement, conversions (F2 prefix variants).
 *
 * Compile: cl /nologo /Od /GS- /arch:SSE2 /Fe:sse2.exe sse2.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

static double __declspec(align(16)) g_pd1[2] = {1.1, 2.2};
static double __declspec(align(16)) g_pd2[2] = {3.3, 4.4};
static double g_sd1 = 1.5;
static int    __declspec(align(16)) g_dq1[4] = {1, 2, 3, 4};

/* ---- SSE2 double-precision data movement ---- */
__declspec(noinline) void test_sse2_move(void)
{
    __asm {
        ; MOVAPD / MOVUPD (66 prefix variants of MOVAPS/MOVUPS)
        _emit 0x66  ; MOVAPD xmm0, [g_pd1]
        movaps xmm0, [g_pd1]
        _emit 0x66  ; MOVAPD xmm1, [g_pd2]
        movaps xmm1, [g_pd2]

        ; MOVSD (scalar double, F2 prefix)
        ; F2 0F 10 /r = movsd xmm, xmm/m64
        _emit 0xF2
        _emit 0x0F
        _emit 0x10
        _emit 0x05  ; xmm0, [disp32]
        _emit 0x00  ; placeholder addr (will be relocated)
        _emit 0x00
        _emit 0x00
        _emit 0x00

        ; MOVSD xmm1, xmm0 (reg-reg)
        _emit 0xF2
        _emit 0x0F
        _emit 0x10
        _emit 0xC8  ; mod=11, xmm1, xmm0

        ; MOVSD [mem], xmm0 (store)
        ; F2 0F 11 /r
        _emit 0xF2
        _emit 0x0F
        _emit 0x11
        _emit 0x05  ; [disp32], xmm0
        _emit 0x00
        _emit 0x00
        _emit 0x00
        _emit 0x00

        ; MOVDQA / MOVDQU (66 0F 6F/7F, F3 0F 6F/7F)
        _emit 0x66
        _emit 0x0F
        _emit 0x6F
        _emit 0xC1  ; movdqa xmm0, xmm1

        _emit 0xF3
        _emit 0x0F
        _emit 0x6F
        _emit 0xC2  ; movdqu xmm0, xmm2

        ; MOVQ (66 0F D6)
        _emit 0x66
        _emit 0x0F
        _emit 0xD6
        _emit 0xC1  ; movq xmm1, xmm0

        ; MOVHPD / MOVLPD (66 prefix)
        _emit 0x66
        _emit 0x0F
        _emit 0x16  ; movhpd xmm0, [mem]
        _emit 0x05
        _emit 0x00
        _emit 0x00
        _emit 0x00
        _emit 0x00

        _emit 0x66
        _emit 0x0F
        _emit 0x12  ; movlpd xmm0, [mem]
        _emit 0x05
        _emit 0x00
        _emit 0x00
        _emit 0x00
        _emit 0x00
    }
}

/* ---- SSE2 double-precision arithmetic ---- */
__declspec(noinline) void test_sse2_arithmetic(void)
{
    __asm {
        ; Packed double (66 prefix): addpd, subpd, mulpd, divpd, sqrtpd, maxpd, minpd
        ; 66 0F 58 = addpd
        _emit 0x66
        _emit 0x0F
        _emit 0x58
        _emit 0xC1  ; addpd xmm0, xmm1

        ; 66 0F 5C = subpd
        _emit 0x66
        _emit 0x0F
        _emit 0x5C
        _emit 0xC1  ; subpd xmm0, xmm1

        ; 66 0F 59 = mulpd
        _emit 0x66
        _emit 0x0F
        _emit 0x59
        _emit 0xC1  ; mulpd xmm0, xmm1

        ; 66 0F 5E = divpd
        _emit 0x66
        _emit 0x0F
        _emit 0x5E
        _emit 0xC1  ; divpd xmm0, xmm1

        ; 66 0F 51 = sqrtpd
        _emit 0x66
        _emit 0x0F
        _emit 0x51
        _emit 0xC1  ; sqrtpd xmm0, xmm1

        ; Scalar double (F2 prefix): addsd, subsd, mulsd, divsd, sqrtsd, maxsd, minsd
        ; F2 0F 58 = addsd
        _emit 0xF2
        _emit 0x0F
        _emit 0x58
        _emit 0xC1  ; addsd xmm0, xmm1

        ; F2 0F 5C = subsd
        _emit 0xF2
        _emit 0x0F
        _emit 0x5C
        _emit 0xC1  ; subsd xmm0, xmm1

        ; F2 0F 59 = mulsd
        _emit 0xF2
        _emit 0x0F
        _emit 0x59
        _emit 0xC1  ; mulsd xmm0, xmm1

        ; F2 0F 5E = divsd
        _emit 0xF2
        _emit 0x0F
        _emit 0x5E
        _emit 0xC1  ; divsd xmm0, xmm1

        ; F2 0F 51 = sqrtsd
        _emit 0xF2
        _emit 0x0F
        _emit 0x51
        _emit 0xC1  ; sqrtsd xmm0, xmm1

        ; F2 0F 5F = maxsd
        _emit 0xF2
        _emit 0x0F
        _emit 0x5F
        _emit 0xC1  ; maxsd xmm0, xmm1

        ; F2 0F 5D = minsd
        _emit 0xF2
        _emit 0x0F
        _emit 0x5D
        _emit 0xC1  ; minsd xmm0, xmm1
    }
}

/* ---- SSE2 comparison (F2 prefix) ---- */
__declspec(noinline) void test_sse2_compare(void)
{
    __asm {
        ; CMPSD (F2 0F C2 /r imm8)
        ; cmpeqsd xmm0, xmm1 (imm=0)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x00  ; predicate 0: cmpeqsd

        ; cmpltsd xmm0, xmm1 (imm=1)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x01  ; predicate 1: cmpltsd

        ; cmplesd (imm=2)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x02

        ; cmpunordsd (imm=3)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x03

        ; cmpneqsd (imm=4)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x04

        ; cmpnltsd (imm=5)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x05

        ; cmpnlesd (imm=6)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x06

        ; cmpordsd (imm=7)
        _emit 0xF2
        _emit 0x0F
        _emit 0xC2
        _emit 0xC1
        _emit 0x07

        ; UCOMISD (66 0F 2E)
        _emit 0x66
        _emit 0x0F
        _emit 0x2E
        _emit 0xC1  ; ucomisd xmm0, xmm1

        ; COMISD (66 0F 2F)
        _emit 0x66
        _emit 0x0F
        _emit 0x2F
        _emit 0xC1  ; comisd xmm0, xmm1
    }
}

/* ---- SSE2 conversion ---- */
__declspec(noinline) void test_sse2_convert(void)
{
    __asm {
        ; CVTSI2SD (F2 0F 2A) - integer to scalar double
        _emit 0xF2
        _emit 0x0F
        _emit 0x2A
        _emit 0xC0  ; cvtsi2sd xmm0, eax

        ; CVTSD2SI (F2 0F 2D) - scalar double to integer
        _emit 0xF2
        _emit 0x0F
        _emit 0x2D
        _emit 0xC0  ; cvtsd2si eax, xmm0

        ; CVTTSD2SI (F2 0F 2C) - truncated scalar double to integer
        _emit 0xF2
        _emit 0x0F
        _emit 0x2C
        _emit 0xC0  ; cvttsd2si eax, xmm0

        ; CVTPS2PD (0F 5A) - packed single to packed double
        _emit 0x0F
        _emit 0x5A
        _emit 0xC1  ; cvtps2pd xmm0, xmm1

        ; CVTPD2PS (66 0F 5A) - packed double to packed single
        _emit 0x66
        _emit 0x0F
        _emit 0x5A
        _emit 0xC1  ; cvtpd2ps xmm0, xmm1

        ; CVTSS2SD (F3 0F 5A) - scalar single to scalar double
        _emit 0xF3
        _emit 0x0F
        _emit 0x5A
        _emit 0xC1  ; cvtss2sd xmm0, xmm1

        ; CVTSD2SS (F2 0F 5A) - scalar double to scalar single
        _emit 0xF2
        _emit 0x0F
        _emit 0x5A
        _emit 0xC1  ; cvtsd2ss xmm0, xmm1

        ; CVTDQ2PS (0F 5B) - packed int32 to packed single
        _emit 0x0F
        _emit 0x5B
        _emit 0xC1  ; cvtdq2ps xmm0, xmm1

        ; CVTTPS2DQ (F3 0F 5B) - truncated packed single to packed int32
        _emit 0xF3
        _emit 0x0F
        _emit 0x5B
        _emit 0xC1  ; cvttps2dq xmm0, xmm1

        ; CVTPD2DQ (F2 0F E6) - packed double to packed int32
        _emit 0xF2
        _emit 0x0F
        _emit 0xE6
        _emit 0xC1  ; cvtpd2dq xmm0, xmm1

        ; CVTDQ2PD (F3 0F E6) - packed int32 to packed double
        _emit 0xF3
        _emit 0x0F
        _emit 0xE6
        _emit 0xC1  ; cvtdq2pd xmm0, xmm1

        ; CVTTPD2DQ (66 0F E6) - truncated packed double to packed int32
        _emit 0x66
        _emit 0x0F
        _emit 0xE6
        _emit 0xC1  ; cvttpd2dq xmm0, xmm1
    }
}

/* ---- SSE2 integer on XMM ---- */
__declspec(noinline) void test_sse2_integer(void)
{
    __asm {
        ; 66-prefixed integer ops on XMM registers
        ; PADDB (66 0F FC)
        _emit 0x66
        _emit 0x0F
        _emit 0xFC
        _emit 0xC1  ; paddb xmm0, xmm1

        ; PADDW (66 0F FD)
        _emit 0x66
        _emit 0x0F
        _emit 0xFD
        _emit 0xC1  ; paddw xmm0, xmm1

        ; PADDD (66 0F FE)
        _emit 0x66
        _emit 0x0F
        _emit 0xFE
        _emit 0xC1  ; paddd xmm0, xmm1

        ; PADDQ (66 0F D4) - new in SSE2
        _emit 0x66
        _emit 0x0F
        _emit 0xD4
        _emit 0xC1  ; paddq xmm0, xmm1

        ; PSUBB (66 0F F8)
        _emit 0x66
        _emit 0x0F
        _emit 0xF8
        _emit 0xC1

        ; PSUBW (66 0F F9)
        _emit 0x66
        _emit 0x0F
        _emit 0xF9
        _emit 0xC1

        ; PSUBD (66 0F FA)
        _emit 0x66
        _emit 0x0F
        _emit 0xFA
        _emit 0xC1

        ; PAND (66 0F DB)
        _emit 0x66
        _emit 0x0F
        _emit 0xDB
        _emit 0xC1

        ; POR (66 0F EB)
        _emit 0x66
        _emit 0x0F
        _emit 0xEB
        _emit 0xC1

        ; PXOR (66 0F EF)
        _emit 0x66
        _emit 0x0F
        _emit 0xEF
        _emit 0xC1

        ; PSHUFD (66 0F 70) - new in SSE2
        _emit 0x66
        _emit 0x0F
        _emit 0x70
        _emit 0xC1
        _emit 0x1B  ; pshufd xmm0, xmm1, 1Bh

        ; PSHUFHW (F3 0F 70)
        _emit 0xF3
        _emit 0x0F
        _emit 0x70
        _emit 0xC1
        _emit 0xE4  ; pshufhw xmm0, xmm1, E4h

        ; PSHUFLW (F2 0F 70)
        _emit 0xF2
        _emit 0x0F
        _emit 0x70
        _emit 0xC1
        _emit 0xE4  ; pshuflw xmm0, xmm1, E4h

        ; MOVDQ2Q (F2 0F D6) - XMM low -> MMX
        _emit 0xF2
        _emit 0x0F
        _emit 0xD6
        _emit 0xC0  ; movdq2q mm0, xmm0

        ; MOVQ2DQ (F3 0F D6) - MMX -> XMM low
        _emit 0xF3
        _emit 0x0F
        _emit 0xD6
        _emit 0xC0  ; movq2dq xmm0, mm0

        emms
    }
}

/* ---- SSE2 misc ---- */
__declspec(noinline) void test_sse2_misc(void)
{
    __asm {
        ; LFENCE / MFENCE / PAUSE / CLFLUSH
        lfence
        mfence
        pause

        ; MOVNTI (0F C3)
        _emit 0x0F
        _emit 0xC3
        _emit 0x03  ; movnti [ebx], eax

        ; MOVNTPD (66 0F 2B)
        _emit 0x66
        _emit 0x0F
        _emit 0x2B
        _emit 0x03  ; movntpd [ebx], xmm0

        ; MOVNTDQ (66 0F E7)
        _emit 0x66
        _emit 0x0F
        _emit 0xE7
        _emit 0x03  ; movntdq [ebx], xmm0

        ; MASKMOVDQU (66 0F F7)
        ; Needs EDI as implicit dest - skip actual execution

        ; PMOVMSKB (66 0F D7) on XMM
        _emit 0x66
        _emit 0x0F
        _emit 0xD7
        _emit 0xC0  ; pmovmskb eax, xmm0

        ; MOVMSKPD (66 0F 50)
        _emit 0x66
        _emit 0x0F
        _emit 0x50
        _emit 0xC0  ; movmskpd eax, xmm0
    }
}

int main(void)
{
    test_sse2_move();
    test_sse2_arithmetic();
    test_sse2_compare();
    test_sse2_convert();
    test_sse2_integer();
    test_sse2_misc();
    return 0;
}
