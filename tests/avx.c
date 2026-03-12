/*
 * avx.c - AVX instruction stress test for PVDasm
 *
 * Tests: VEX-encoded 128/256-bit AVX instructions.
 *        2-byte VEX (C5), 3-byte VEX (C4), 3-operand forms,
 *        all opcode maps (0F, 0F38, 0F3A).
 *
 * All instructions encoded as raw bytes via _emit since MSVC
 * 32-bit inline asm does not support VEX-encoded instructions.
 *
 * Compile: cl /nologo /Od /GS- /Fe:avx.exe avx.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

/*
 * VEX prefix encoding reminder:
 *   2-byte: C5 [RvvvvLpp]
 *   3-byte: C4 [RXBmmmmm] [WvvvvLpp]
 *
 * R=1 (not inverted) for reg<8, vvvv=1111 (xmm0), L=0 (128-bit), pp=00 (no prefix)
 * For 32-bit mode, R/X/B are always 1 (inverted=0, no extension needed).
 *
 * Common 2-byte VEX patterns:
 *   C5 F8 = VEX.128.0F (no prefix, L=0)
 *   C5 FC = VEX.256.0F (no prefix, L=1)
 *   C5 F9 = VEX.128.66.0F (pp=01)
 *   C5 FD = VEX.256.66.0F (pp=01, L=1)
 *   C5 FA = VEX.128.F3.0F (pp=10)
 *   C5 FB = VEX.128.F2.0F (pp=11)
 *
 * vvvv field (2nd source): inverted, so vvvv=1111=xmm0, 1110=xmm1, etc.
 *   C5 F0 = VEX.128.0F, vvvv=xmm1 (1110)
 *   C5 E8 = VEX.128.0F, vvvv=xmm2 (1101)
 */

/* ---- AVX 128-bit data movement ---- */
__declspec(noinline) void test_avx_move128(void)
{
    __asm {
        ; VMOVAPS xmm0, xmm1 (VEX.128.0F 28 /r)
        ; C5 F8 28 C1
        _emit 0xC5
        _emit 0xF8
        _emit 0x28
        _emit 0xC1

        ; VMOVAPS xmm2, xmm3
        _emit 0xC5
        _emit 0xF8
        _emit 0x28
        _emit 0xD3

        ; VMOVUPS xmm0, xmm1 (VEX.128.0F 10 /r)
        _emit 0xC5
        _emit 0xF8
        _emit 0x10
        _emit 0xC1

        ; VMOVSS xmm0, xmm1, xmm2 (VEX.128.F3.0F 10 /r, 3-operand reg form)
        ; C5 F2 10 C2 => vvvv=xmm1 (F2 = pp=10, vvvv=1110)
        _emit 0xC5
        _emit 0xF2
        _emit 0x10
        _emit 0xC2

        ; VMOVSD xmm0, xmm1, xmm2 (VEX.128.F2.0F 10 /r)
        ; C5 F3 10 C2 => pp=11 (F2), vvvv=1110 (xmm1)
        _emit 0xC5
        _emit 0xF3
        _emit 0x10
        _emit 0xC2

        ; VMOVAPD xmm0, xmm1 (VEX.128.66.0F 28 /r)
        _emit 0xC5
        _emit 0xF9
        _emit 0x28
        _emit 0xC1

        ; VMOVDQA xmm0, xmm1 (VEX.128.66.0F 6F /r)
        _emit 0xC5
        _emit 0xF9
        _emit 0x6F
        _emit 0xC1

        ; VMOVDQU xmm0, xmm1 (VEX.128.F3.0F 6F /r)
        _emit 0xC5
        _emit 0xFA
        _emit 0x6F
        _emit 0xC1

        ; VMOVLHPS xmm0, xmm1, xmm2 (VEX.128.0F 16 /r)
        _emit 0xC5
        _emit 0xF0  ; vvvv=xmm1
        _emit 0x16
        _emit 0xC2

        ; VMOVHLPS xmm0, xmm1, xmm2 (VEX.128.0F 12 /r)
        _emit 0xC5
        _emit 0xF0
        _emit 0x12
        _emit 0xC2
    }
}

/* ---- AVX 256-bit data movement ---- */
__declspec(noinline) void test_avx_move256(void)
{
    __asm {
        ; VMOVAPS ymm0, ymm1 (VEX.256.0F 28 /r)
        ; C5 FC 28 C1
        _emit 0xC5
        _emit 0xFC
        _emit 0x28
        _emit 0xC1

        ; VMOVUPS ymm0, ymm1 (VEX.256.0F 10 /r)
        _emit 0xC5
        _emit 0xFC
        _emit 0x10
        _emit 0xC1

        ; VMOVAPD ymm0, ymm1 (VEX.256.66.0F 28 /r)
        _emit 0xC5
        _emit 0xFD
        _emit 0x28
        _emit 0xC1

        ; VMOVDQA ymm0, ymm1 (VEX.256.66.0F 6F /r)
        _emit 0xC5
        _emit 0xFD
        _emit 0x6F
        _emit 0xC1

        ; VMOVDQU ymm0, ymm1 (VEX.256.F3.0F 6F /r)
        _emit 0xC5
        _emit 0xFE
        _emit 0x6F
        _emit 0xC1
    }
}

/* ---- AVX arithmetic (3-operand) ---- */
__declspec(noinline) void test_avx_arithmetic(void)
{
    __asm {
        ; VADDPS xmm0, xmm1, xmm2 (VEX.128.0F 58 /r)
        ; C5 F0 58 C2 (vvvv=1110=xmm1)
        _emit 0xC5
        _emit 0xF0
        _emit 0x58
        _emit 0xC2

        ; VADDPS ymm0, ymm1, ymm2 (VEX.256.0F 58 /r)
        ; C5 F4 58 C2 (L=1, vvvv=1110)
        _emit 0xC5
        _emit 0xF4
        _emit 0x58
        _emit 0xC2

        ; VSUBPS xmm0, xmm1, xmm2
        _emit 0xC5
        _emit 0xF0
        _emit 0x5C
        _emit 0xC2

        ; VMULPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x59
        _emit 0xC2

        ; VDIVPS xmm0, xmm1, xmm2
        _emit 0xC5
        _emit 0xF0
        _emit 0x5E
        _emit 0xC2

        ; VSQRTPS xmm0, xmm1 (VEX.128.0F 51 /r, 2-operand)
        _emit 0xC5
        _emit 0xF8
        _emit 0x51
        _emit 0xC1

        ; VSQRTPS ymm0, ymm1
        _emit 0xC5
        _emit 0xFC
        _emit 0x51
        _emit 0xC1

        ; VADDSS xmm0, xmm1, xmm2 (VEX.128.F3.0F 58 /r)
        _emit 0xC5
        _emit 0xF2  ; pp=10 (F3), vvvv=xmm1
        _emit 0x58
        _emit 0xC2

        ; VADDSD xmm0, xmm1, xmm2 (VEX.128.F2.0F 58 /r)
        _emit 0xC5
        _emit 0xF3  ; pp=11 (F2), vvvv=xmm1
        _emit 0x58
        _emit 0xC2

        ; VADDPD ymm0, ymm1, ymm2 (VEX.256.66.0F 58 /r)
        _emit 0xC5
        _emit 0xF5  ; pp=01 (66), L=1, vvvv=xmm1
        _emit 0x58
        _emit 0xC2

        ; VMAXPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x5F
        _emit 0xC2

        ; VMINPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x5D
        _emit 0xC2
    }
}

/* ---- AVX logical ---- */
__declspec(noinline) void test_avx_logical(void)
{
    __asm {
        ; VANDPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x54
        _emit 0xC2

        ; VANDNPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x55
        _emit 0xC2

        ; VORPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x56
        _emit 0xC2

        ; VXORPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x57
        _emit 0xC2

        ; VXORPS xmm0, xmm0, xmm0 (zero register idiom)
        _emit 0xC5
        _emit 0xF8
        _emit 0x57
        _emit 0xC0
    }
}

/* ---- AVX shuffle / unpack ---- */
__declspec(noinline) void test_avx_shuffle(void)
{
    __asm {
        ; VSHUFPS ymm0, ymm1, ymm2, imm8 (VEX.256.0F C6 /r imm8)
        _emit 0xC5
        _emit 0xF4
        _emit 0xC6
        _emit 0xC2
        _emit 0x1B  ; imm8

        ; VUNPCKLPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x14
        _emit 0xC2

        ; VUNPCKHPS ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF4
        _emit 0x15
        _emit 0xC2

        ; VPERMILPS xmm0, xmm1, xmm2 (VEX.128.66.0F38 0C /r)
        ; 3-byte VEX: C4 E2 71 0C C2
        _emit 0xC4
        _emit 0xE2  ; R=1,X=1,B=1,mmmmm=00010 (0F38)
        _emit 0x71  ; W=0,vvvv=1110(xmm1),L=0,pp=01(66)
        _emit 0x0C
        _emit 0xC2

        ; VPERM2F128 ymm0, ymm1, ymm2, imm8 (VEX.256.66.0F3A 06 /r imm8)
        _emit 0xC4
        _emit 0xE3  ; R=1,X=1,B=1,mmmmm=00011 (0F3A)
        _emit 0x75  ; W=0,vvvv=1110,L=1,pp=01
        _emit 0x06
        _emit 0xC2
        _emit 0x31  ; imm8
    }
}

/* ---- AVX comparison ---- */
__declspec(noinline) void test_avx_compare(void)
{
    __asm {
        ; VCMPPS xmm0, xmm1, xmm2, imm8 (VEX.128.0F C2 /r imm8)
        _emit 0xC5
        _emit 0xF0
        _emit 0xC2
        _emit 0xC2
        _emit 0x00  ; EQ

        _emit 0xC5
        _emit 0xF0
        _emit 0xC2
        _emit 0xC2
        _emit 0x01  ; LT

        ; VCMPPS ymm0, ymm1, ymm2, imm8 (VEX.256.0F C2)
        _emit 0xC5
        _emit 0xF4
        _emit 0xC2
        _emit 0xC2
        _emit 0x0E  ; GT_OS (AVX extended predicate)

        ; VUCOMISS xmm0, xmm1 (VEX.128.0F 2E /r)
        _emit 0xC5
        _emit 0xF8
        _emit 0x2E
        _emit 0xC1

        ; VUCOMISD xmm0, xmm1 (VEX.128.66.0F 2E /r)
        _emit 0xC5
        _emit 0xF9
        _emit 0x2E
        _emit 0xC1
    }
}

/* ---- AVX conversion ---- */
__declspec(noinline) void test_avx_convert(void)
{
    __asm {
        ; VCVTPS2PD ymm0, xmm1 (VEX.256.0F 5A /r)
        _emit 0xC5
        _emit 0xFC
        _emit 0x5A
        _emit 0xC1

        ; VCVTPD2PS xmm0, ymm1 (VEX.256.66.0F 5A /r)
        _emit 0xC5
        _emit 0xFD
        _emit 0x5A
        _emit 0xC1

        ; VCVTSI2SS xmm0, xmm1, eax (VEX.128.F3.0F 2A /r)
        _emit 0xC5
        _emit 0xF2
        _emit 0x2A
        _emit 0xC0

        ; VCVTSI2SD xmm0, xmm1, eax (VEX.128.F2.0F 2A /r)
        _emit 0xC5
        _emit 0xF3
        _emit 0x2A
        _emit 0xC0

        ; VCVTTSS2SI eax, xmm1 (VEX.128.F3.0F 2C /r)
        _emit 0xC5
        _emit 0xFA
        _emit 0x2C
        _emit 0xC1

        ; VCVTTSD2SI eax, xmm1 (VEX.128.F2.0F 2C /r)
        _emit 0xC5
        _emit 0xFB
        _emit 0x2C
        _emit 0xC1
    }
}

/* ---- AVX misc: VZEROALL, VZEROUPPER, VBROADCASTSS ---- */
__declspec(noinline) void test_avx_misc(void)
{
    __asm {
        ; VZEROUPPER (VEX.128.0F 77)
        _emit 0xC5
        _emit 0xF8
        _emit 0x77

        ; VZEROALL (VEX.256.0F 77)
        _emit 0xC5
        _emit 0xFC
        _emit 0x77

        ; VTESTPS xmm0, xmm1 (VEX.128.66.0F38 0E /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x79  ; W=0,vvvv=1111,L=0,pp=01
        _emit 0x0E
        _emit 0xC1

        ; VPTEST xmm0, xmm1 (VEX.128.66.0F38 17 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x79
        _emit 0x17
        _emit 0xC1

        ; VMOVMSKPS eax, ymm0 (VEX.256.0F 50 /r)
        _emit 0xC5
        _emit 0xFC
        _emit 0x50
        _emit 0xC0
    }
}

int main(void)
{
    test_avx_move128();
    test_avx_move256();
    test_avx_arithmetic();
    test_avx_logical();
    test_avx_shuffle();
    test_avx_compare();
    test_avx_convert();
    test_avx_misc();
    return 0;
}
