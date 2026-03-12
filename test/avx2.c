/*
 * avx2.c - AVX2 instruction stress test for PVDasm
 *
 * Tests: 256-bit integer SIMD (VEX.256.66.0F/0F38/0F3A),
 *        VPBROADCAST, VPERMD/Q, VPERM2I128, VGATHER,
 *        shift variable, FMA3.
 *
 * Compile: cl /nologo /Od /GS- /Fe:avx2.exe avx2.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

/* ---- AVX2 256-bit integer arithmetic ---- */
__declspec(noinline) void test_avx2_int_arith(void)
{
    __asm {
        ; VPADDB ymm0, ymm1, ymm2 (VEX.256.66.0F FC /r)
        ; C5 F5 FC C2 (pp=01(66), L=1, vvvv=1110(ymm1))
        _emit 0xC5
        _emit 0xF5
        _emit 0xFC
        _emit 0xC2

        ; VPADDW ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xFD
        _emit 0xC2

        ; VPADDD ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xFE
        _emit 0xC2

        ; VPADDQ ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xD4
        _emit 0xC2

        ; VPSUBB ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xF8
        _emit 0xC2

        ; VPSUBW ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xF9
        _emit 0xC2

        ; VPSUBD ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xFA
        _emit 0xC2

        ; VPMULLD ymm0, ymm1, ymm2 (VEX.256.66.0F38 40 /r)
        _emit 0xC4
        _emit 0xE2  ; mmmmm=00010 (0F38)
        _emit 0x75  ; W=0,vvvv=1110,L=1,pp=01
        _emit 0x40
        _emit 0xC2

        ; VPMULLW ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xD5
        _emit 0xC2

        ; VPAND ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xDB
        _emit 0xC2

        ; VPOR ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xEB
        _emit 0xC2

        ; VPXOR ymm0, ymm1, ymm2
        _emit 0xC5
        _emit 0xF5
        _emit 0xEF
        _emit 0xC2
    }
}

/* ---- AVX2 broadcast ---- */
__declspec(noinline) void test_avx2_broadcast(void)
{
    __asm {
        ; VPBROADCASTD ymm0, xmm1 (VEX.256.66.0F38 58 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x7D  ; W=0,vvvv=1111,L=1,pp=01
        _emit 0x58
        _emit 0xC1

        ; VPBROADCASTQ ymm0, xmm1 (VEX.256.66.0F38 59 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x7D
        _emit 0x59
        _emit 0xC1

        ; VPBROADCASTB ymm0, xmm1 (VEX.256.66.0F38 78 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x7D
        _emit 0x78
        _emit 0xC1

        ; VPBROADCASTW ymm0, xmm1 (VEX.256.66.0F38 79 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x7D
        _emit 0x79
        _emit 0xC1

        ; VBROADCASTSS ymm0, xmm1 (VEX.256.66.0F38 18 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x7D
        _emit 0x18
        _emit 0xC1

        ; VBROADCASTSD ymm0, xmm1 (VEX.256.66.0F38 19 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x7D
        _emit 0x19
        _emit 0xC1
    }
}

/* ---- AVX2 permute ---- */
__declspec(noinline) void test_avx2_permute(void)
{
    __asm {
        ; VPERMD ymm0, ymm1, ymm2 (VEX.256.66.0F38 36 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x75  ; vvvv=ymm1
        _emit 0x36
        _emit 0xC2

        ; VPERMQ ymm0, ymm1, imm8 (VEX.256.66.0F3A 00 /r imm8, W=1)
        _emit 0xC4
        _emit 0xE3  ; mmmmm=00011 (0F3A)
        _emit 0xFD  ; W=1,vvvv=1111,L=1,pp=01
        _emit 0x00
        _emit 0xC1
        _emit 0xD8  ; imm8

        ; VPERMPD ymm0, ymm1, imm8 (VEX.256.66.0F3A 01 /r imm8, W=1)
        _emit 0xC4
        _emit 0xE3
        _emit 0xFD
        _emit 0x01
        _emit 0xC1
        _emit 0xD8

        ; VPERM2I128 ymm0, ymm1, ymm2, imm8 (VEX.256.66.0F3A 46 /r imm8)
        _emit 0xC4
        _emit 0xE3
        _emit 0x75
        _emit 0x46
        _emit 0xC2
        _emit 0x31

        ; VPBLENDD ymm0, ymm1, ymm2, imm8 (VEX.256.66.0F3A 02 /r imm8)
        _emit 0xC4
        _emit 0xE3
        _emit 0x75
        _emit 0x02
        _emit 0xC2
        _emit 0xCC
    }
}

/* ---- AVX2 variable shift ---- */
__declspec(noinline) void test_avx2_varshift(void)
{
    __asm {
        ; VPSLLVD ymm0, ymm1, ymm2 (VEX.256.66.0F38 47 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x75
        _emit 0x47
        _emit 0xC2

        ; VPSRLVD ymm0, ymm1, ymm2 (VEX.256.66.0F38 45 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x75
        _emit 0x45
        _emit 0xC2

        ; VPSRAVD ymm0, ymm1, ymm2 (VEX.256.66.0F38 46 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x75
        _emit 0x46
        _emit 0xC2

        ; VPSLLVQ ymm0, ymm1, ymm2 (VEX.256.66.0F38.W1 47 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0xF5  ; W=1
        _emit 0x47
        _emit 0xC2

        ; VPSRLVQ ymm0, ymm1, ymm2 (VEX.256.66.0F38.W1 45 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0xF5
        _emit 0x45
        _emit 0xC2
    }
}

/* ---- AVX2 gather ---- */
__declspec(noinline) void test_avx2_gather(void)
{
    __asm {
        ; VGATHERDPS xmm0, [eax+xmm1*4], xmm2 (VEX.128.66.0F38 92 /r)
        ; ModR/M for VSIB: mod=00, reg=xmm0(000), rm=100(SIB)
        ; SIB: scale=10(x4), index=001(xmm1), base=000(eax)
        _emit 0xC4
        _emit 0xE2
        _emit 0x69  ; W=0,vvvv=1101(xmm2),L=0,pp=01
        _emit 0x92
        _emit 0x04  ; ModR/M: mod=00,reg=000,rm=100
        _emit 0x88  ; SIB: scale=10,index=001,base=000

        ; VGATHERDPD xmm0, [eax+xmm1*8], xmm2 (VEX.128.66.0F38.W1 92 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0xE9  ; W=1,vvvv=1101(xmm2),L=0,pp=01
        _emit 0x92
        _emit 0x04
        _emit 0xC8  ; SIB: scale=11(x8),index=001,base=000

        ; VPGATHERDD xmm0, [eax+xmm1*4], xmm2 (VEX.128.66.0F38 90 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x69
        _emit 0x90
        _emit 0x04
        _emit 0x88
    }
}

/* ---- FMA3 (VEX.128/256.66.0F38 xx) ---- */
__declspec(noinline) void test_fma3(void)
{
    __asm {
        ; VFMADD132PS xmm0, xmm1, xmm2 (VEX.128.66.0F38 98 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0x98
        _emit 0xC2

        ; VFMADD213PS xmm0, xmm1, xmm2 (VEX.128.66.0F38 A8 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0xA8
        _emit 0xC2

        ; VFMADD231PS xmm0, xmm1, xmm2 (VEX.128.66.0F38 B8 /r)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0xB8
        _emit 0xC2

        ; VFMADD132PS ymm0, ymm1, ymm2 (VEX.256)
        _emit 0xC4
        _emit 0xE2
        _emit 0x75
        _emit 0x98
        _emit 0xC2

        ; VFMADD132PD xmm0, xmm1, xmm2 (W=1)
        _emit 0xC4
        _emit 0xE2
        _emit 0xF1  ; W=1
        _emit 0x98
        _emit 0xC2

        ; VFNMADD132PS xmm0, xmm1, xmm2 (VEX.128.66.0F38 9C)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0x9C
        _emit 0xC2

        ; VFMSUB132PS xmm0, xmm1, xmm2 (VEX.128.66.0F38 9A)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0x9A
        _emit 0xC2
    }
}

int main(void)
{
    test_avx2_int_arith();
    test_avx2_broadcast();
    test_avx2_permute();
    test_avx2_varshift();
    test_avx2_gather();
    test_fma3();
    return 0;
}
