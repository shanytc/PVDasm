/*
 * avx512.c - AVX-512F instruction stress test for PVDasm
 *
 * Tests: EVEX-encoded 512-bit instructions (zmm registers),
 *        opmask decorators {k1}-{k7}, zero-masking {z},
 *        broadcast {1toN}, opmask register ops (KANDW, etc.).
 *
 * All instructions encoded via _emit (raw EVEX/VEX bytes) since
 * MSVC inline asm doesn't support AVX-512 mnemonics.
 *
 * EVEX prefix format (4 bytes):
 *   Byte 0: 0x62
 *   Byte 1 (P0): R  X  B  R' 0  0  m  m    (mm = opcode map)
 *   Byte 2 (P1): W  v  v  v  v  1  p  p    (vvvv inverted, pp = prefix)
 *   Byte 3 (P2): z  L' L  b  V' a  a  a    (aaa = opmask, z = zeroing)
 *
 * Compile: cl /nologo /Od /GS- /Fe:avx512.exe avx512.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

/* ---- AVX-512F basic float arithmetic ---- */
__declspec(noinline) void test_avx512_arith(void)
{
    __asm {
        ; VADDPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 58 /r)
        ; P0: R=1,X=1,B=1,R'=1,00,mm=01 -> 0xF1
        ; P1: W=0,vvvv=1110,1,pp=00 -> 0x74
        ; P2: z=0,L'L=10,b=0,V'=1,aaa=000 -> 0x48
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x58
        _emit 0xC2

        ; VSUBPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 5C /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x5C
        _emit 0xC2

        ; VMULPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 59 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x59
        _emit 0xC2

        ; VDIVPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 5E /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x5E
        _emit 0xC2

        ; VMAXPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 5F /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x5F
        _emit 0xC2

        ; VMINPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 5D /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x5D
        _emit 0xC2
    }
}

/* ---- AVX-512F double-precision arithmetic ---- */
__declspec(noinline) void test_avx512_arith_pd(void)
{
    __asm {
        ; VADDPD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W1 58 /r)
        ; P1: W=1,vvvv=1110,1,pp=01 -> 0xF5
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0x58
        _emit 0xC2

        ; VSUBPD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W1 5C /r)
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0x5C
        _emit 0xC2

        ; VMULPD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W1 59 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0x59
        _emit 0xC2

        ; VDIVPD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W1 5E /r)
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0x5E
        _emit 0xC2
    }
}

/* ---- AVX-512F with opmask decorators ---- */
__declspec(noinline) void test_avx512_opmask(void)
{
    __asm {
        ; VADDPS zmm0 {k1}, zmm1, zmm2 (aaa=001)
        ; P2: z=0,L'L=10,b=0,V'=1,aaa=001 -> 0x49
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x49
        _emit 0x58
        _emit 0xC2

        ; VADDPS zmm0 {k3}, zmm1, zmm2 (aaa=011)
        ; P2: 0x4B
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x4B
        _emit 0x58
        _emit 0xC2

        ; VADDPS zmm0 {k7}, zmm1, zmm2 (aaa=111)
        ; P2: 0x4F
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x4F
        _emit 0x58
        _emit 0xC2

        ; VADDPS zmm0 {k1}{z}, zmm1, zmm2 (z=1, aaa=001)
        ; P2: 1,10,0,1,001 -> 0xC9
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0xC9
        _emit 0x58
        _emit 0xC2

        ; VSUBPS zmm0 {k2}{z}, zmm1, zmm2 (z=1, aaa=010)
        ; P2: 1,10,0,1,010 -> 0xCA
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0xCA
        _emit 0x5C
        _emit 0xC2
    }
}

/* ---- AVX-512F data movement ---- */
__declspec(noinline) void test_avx512_mov(void)
{
    __asm {
        ; VMOVAPS zmm0, zmm1 (EVEX.512.0F.W0 28 /r)
        ; P1: W=0,vvvv=1111,1,pp=00 -> 0x7C
        _emit 0x62
        _emit 0xF1
        _emit 0x7C
        _emit 0x48
        _emit 0x28
        _emit 0xC1

        ; VMOVUPS zmm0, zmm1 (EVEX.512.0F.W0 10 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x7C
        _emit 0x48
        _emit 0x10
        _emit 0xC1

        ; VMOVAPD zmm0, zmm1 (EVEX.512.66.0F.W1 28 /r)
        ; P1: W=1,vvvv=1111,1,pp=01 -> 0xFD
        _emit 0x62
        _emit 0xF1
        _emit 0xFD
        _emit 0x48
        _emit 0x28
        _emit 0xC1

        ; VMOVDQA32 zmm0, zmm1 (EVEX.512.66.0F.W0 6F /r)
        ; P1: W=0,vvvv=1111,1,pp=01 -> 0x7D
        _emit 0x62
        _emit 0xF1
        _emit 0x7D
        _emit 0x48
        _emit 0x6F
        _emit 0xC1

        ; VMOVDQA64 zmm0, zmm1 (EVEX.512.66.0F.W1 6F /r)
        _emit 0x62
        _emit 0xF1
        _emit 0xFD
        _emit 0x48
        _emit 0x6F
        _emit 0xC1

        ; VMOVDQU32 zmm0, zmm1 (EVEX.512.F3.0F.W0 6F /r)
        ; P1: W=0,vvvv=1111,1,pp=10 -> 0x7E
        _emit 0x62
        _emit 0xF1
        _emit 0x7E
        _emit 0x48
        _emit 0x6F
        _emit 0xC1

        ; VMOVDQU64 zmm0, zmm1 (EVEX.512.F3.0F.W1 6F /r)
        ; P1: W=1,vvvv=1111,1,pp=10 -> 0xFE
        _emit 0x62
        _emit 0xF1
        _emit 0xFE
        _emit 0x48
        _emit 0x6F
        _emit 0xC1
    }
}

/* ---- AVX-512F integer arithmetic ---- */
__declspec(noinline) void test_avx512_int(void)
{
    __asm {
        ; VPADDD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W0 FE /r)
        ; P1: W=0,vvvv=1110,1,pp=01 -> 0x75
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xFE
        _emit 0xC2

        ; VPSUBD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W0 FA /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xFA
        _emit 0xC2

        ; VPADDQ zmm0, zmm1, zmm2 (EVEX.512.66.0F.W1 D4 /r)
        ; P1: W=1,vvvv=1110,1,pp=01 -> 0xF5
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0xD4
        _emit 0xC2

        ; VPSUBQ zmm0, zmm1, zmm2 (EVEX.512.66.0F.W1 FB /r)
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0xFB
        _emit 0xC2

        ; VPMULUDQ zmm0, zmm1, zmm2 (EVEX.512.66.0F.W1 F4 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0xF4
        _emit 0xC2

        ; VPANDD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W0 DB /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xDB
        _emit 0xC2

        ; VPORD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W0 EB /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xEB
        _emit 0xC2

        ; VPXORD zmm0, zmm1, zmm2 (EVEX.512.66.0F.W0 EF /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xEF
        _emit 0xC2
    }
}

/* ---- AVX-512F broadcast ---- */
__declspec(noinline) void test_avx512_broadcast(void)
{
    __asm {
        ; VBROADCASTSS zmm0, xmm1 (EVEX.512.66.0F38.W0 18 /r)
        ; P0: R=1,X=1,B=1,R'=1,00,mm=10 -> 0xF2
        ; P1: W=0,vvvv=1111,1,pp=01 -> 0x7D
        _emit 0x62
        _emit 0xF2
        _emit 0x7D
        _emit 0x48
        _emit 0x18
        _emit 0xC1

        ; VBROADCASTSD zmm0, xmm1 (EVEX.512.66.0F38.W1 19 /r)
        ; P1: W=1,vvvv=1111,1,pp=01 -> 0xFD
        _emit 0x62
        _emit 0xF2
        _emit 0xFD
        _emit 0x48
        _emit 0x19
        _emit 0xC1

        ; VPBROADCASTD zmm0, xmm1 (EVEX.512.66.0F38.W0 58 /r)
        _emit 0x62
        _emit 0xF2
        _emit 0x7D
        _emit 0x48
        _emit 0x58
        _emit 0xC1

        ; VPBROADCASTQ zmm0, xmm1 (EVEX.512.66.0F38.W1 59 /r)
        _emit 0x62
        _emit 0xF2
        _emit 0xFD
        _emit 0x48
        _emit 0x59
        _emit 0xC1
    }
}

/* ---- AVX-512F compare (result to opmask register) ---- */
__declspec(noinline) void test_avx512_cmp(void)
{
    __asm {
        ; VCMPPS k1, zmm1, zmm2, 0 (EQ) (EVEX.512.0F.W0 C2 /r imm8)
        ; P1: W=0,vvvv=1110,1,pp=00 -> 0x74
        ; ModR/M: mod=11,reg=001(k1),rm=010(zmm2) -> 0xCA
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0xC2
        _emit 0xCA
        _emit 0x00

        ; VCMPPS k2, zmm1, zmm2, 1 (LT)
        ; ModR/M: mod=11,reg=010(k2),rm=010 -> 0xD2
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0xC2
        _emit 0xD2
        _emit 0x01

        ; VCMPPS k3, zmm1, zmm2, 4 (NEQ)
        ; ModR/M: mod=11,reg=011(k3),rm=010 -> 0xDA
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0xC2
        _emit 0xDA
        _emit 0x04

        ; VCMPPD k1, zmm1, zmm2, 2 (LE) (EVEX.512.66.0F.W1 C2 /r imm8)
        ; P1: W=1,vvvv=1110,1,pp=01 -> 0xF5
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0xC2
        _emit 0xCA
        _emit 0x02

        ; VPCMPEQD k1, zmm1, zmm2 (EVEX.512.66.0F.W0 76 /r)
        ; P1: W=0,vvvv=1110,1,pp=01 -> 0x75
        ; ModR/M: 11,001,010 -> 0xCA
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x76
        _emit 0xCA

        ; VPCMPGTD k1, zmm1, zmm2 (EVEX.512.66.0F.W0 66 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x66
        _emit 0xCA
    }
}

/* ---- AVX-512F logical ---- */
__declspec(noinline) void test_avx512_logical(void)
{
    __asm {
        ; VANDPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 54 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x54
        _emit 0xC2

        ; VANDNPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 55 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x55
        _emit 0xC2

        ; VORPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 56 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x56
        _emit 0xC2

        ; VXORPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 57 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x57
        _emit 0xC2
    }
}

/* ---- AVX-512F shuffle/permute ---- */
__declspec(noinline) void test_avx512_shuffle(void)
{
    __asm {
        ; VSHUFPS zmm0, zmm1, zmm2, 0x44 (EVEX.512.0F.W0 C6 /r imm8)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0xC6
        _emit 0xC2
        _emit 0x44

        ; VSHUFPD zmm0, zmm1, zmm2, 0x55 (EVEX.512.66.0F.W1 C6 /r imm8)
        ; P1: W=1,vvvv=1110,1,pp=01 -> 0xF5
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x48
        _emit 0xC6
        _emit 0xC2
        _emit 0x55

        ; VPERMILPS zmm0, zmm1, zmm2 (EVEX.512.66.0F38.W0 0C /r)
        ; P0: mm=10 -> 0xF2
        ; P1: W=0,vvvv=1110,1,pp=01 -> 0x75
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x48
        _emit 0x0C
        _emit 0xC2

        ; VPERMD zmm0, zmm1, zmm2 (EVEX.512.66.0F38.W0 36 /r)
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x48
        _emit 0x36
        _emit 0xC2

        ; VPERMQ zmm0, zmm1, zmm2 (EVEX.512.66.0F38.W1 36 /r)
        ; P1: W=1,vvvv=1110,1,pp=01 -> 0xF5
        _emit 0x62
        _emit 0xF2
        _emit 0xF5
        _emit 0x48
        _emit 0x36
        _emit 0xC2

        ; VUNPCKLPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 14 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x14
        _emit 0xC2

        ; VUNPCKHPS zmm0, zmm1, zmm2 (EVEX.512.0F.W0 15 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x48
        _emit 0x15
        _emit 0xC2
    }
}

/* ---- AVX-512F conversion / math ---- */
__declspec(noinline) void test_avx512_cvt(void)
{
    __asm {
        ; VCVTPS2PD zmm0, ymm1 (EVEX.512.0F.W0 5A /r)
        ; P1: W=0,vvvv=1111,1,pp=00 -> 0x7C
        _emit 0x62
        _emit 0xF1
        _emit 0x7C
        _emit 0x48
        _emit 0x5A
        _emit 0xC1

        ; VCVTPD2PS ymm0, zmm1 (EVEX.512.66.0F.W1 5A /r)
        ; P1: W=1,vvvv=1111,1,pp=01 -> 0xFD
        _emit 0x62
        _emit 0xF1
        _emit 0xFD
        _emit 0x48
        _emit 0x5A
        _emit 0xC1

        ; VCVTDQ2PS zmm0, zmm1 (EVEX.512.0F.W0 5B /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x7C
        _emit 0x48
        _emit 0x5B
        _emit 0xC1

        ; VCVTPS2DQ zmm0, zmm1 (EVEX.512.66.0F.W0 5B /r)
        ; P1: W=0,vvvv=1111,1,pp=01 -> 0x7D
        _emit 0x62
        _emit 0xF1
        _emit 0x7D
        _emit 0x48
        _emit 0x5B
        _emit 0xC1

        ; VSQRTPS zmm0, zmm1 (EVEX.512.0F.W0 51 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0x7C
        _emit 0x48
        _emit 0x51
        _emit 0xC1

        ; VSQRTPD zmm0, zmm1 (EVEX.512.66.0F.W1 51 /r)
        _emit 0x62
        _emit 0xF1
        _emit 0xFD
        _emit 0x48
        _emit 0x51
        _emit 0xC1

        ; VRCP14PS zmm0, zmm1 (EVEX.512.66.0F38.W0 4C /r)
        ; P0: mm=10 -> 0xF2
        ; P1: W=0,vvvv=1111,1,pp=01 -> 0x7D
        _emit 0x62
        _emit 0xF2
        _emit 0x7D
        _emit 0x48
        _emit 0x4C
        _emit 0xC1

        ; VRSQRT14PS zmm0, zmm1 (EVEX.512.66.0F38.W0 4E /r)
        _emit 0x62
        _emit 0xF2
        _emit 0x7D
        _emit 0x48
        _emit 0x4E
        _emit 0xC1
    }
}

/* ---- AVX-512F opmask register instructions ---- */
__declspec(noinline) void test_avx512_kmask(void)
{
    __asm {
        ; Opmask instructions use VEX encoding (not EVEX).

        ; KANDW k1, k2, k3 (VEX.NDS.256.0F.W0 41 /r)
        ; 2-byte VEX: C5 [R,vvvv,L,pp]
        ; R=1, vvvv=~k2=~010=1101, L=1, pp=00
        ; C5 [1,1101,1,00] = C5 EC
        ; ModR/M: mod=11, reg=001(k1), rm=011(k3) -> 0xCB
        _emit 0xC5
        _emit 0xEC
        _emit 0x41
        _emit 0xCB

        ; KORW k1, k2, k3 (VEX.NDS.256.0F.W0 45 /r)
        _emit 0xC5
        _emit 0xEC
        _emit 0x45
        _emit 0xCB

        ; KXORW k1, k2, k3 (VEX.NDS.256.0F.W0 47 /r)
        _emit 0xC5
        _emit 0xEC
        _emit 0x47
        _emit 0xCB

        ; KXNORW k1, k2, k3 (VEX.NDS.256.0F.W0 46 /r)
        _emit 0xC5
        _emit 0xEC
        _emit 0x46
        _emit 0xCB

        ; KANDNW k1, k2, k3 (VEX.NDS.256.0F.W0 42 /r)
        _emit 0xC5
        _emit 0xEC
        _emit 0x42
        _emit 0xCB

        ; KNOTW k1, k2 (VEX.0F.W0 44 /r)
        ; vvvv=1111 (unused), L=0, pp=00
        ; C5 [1,1111,0,00] = C5 F8
        ; ModR/M: mod=11, reg=001(k1), rm=010(k2) -> 0xCA
        _emit 0xC5
        _emit 0xF8
        _emit 0x44
        _emit 0xCA

        ; KMOVW k1, k2 (VEX.L0.0F.W0 90 /r)
        _emit 0xC5
        _emit 0xF8
        _emit 0x90
        _emit 0xCA

        ; KSHIFTLW k1, k2, 4 (VEX.L0.66.0F3A.W1 32 /r imm8)
        ; 3-byte VEX: C4 [R,X,B,mmmmm] [W,vvvv,L,pp]
        ; C4 [1,1,1,00011] [1,1111,0,01]
        ; C4 E3 F9 32 CA 04
        _emit 0xC4
        _emit 0xE3
        _emit 0xF9
        _emit 0x32
        _emit 0xCA
        _emit 0x04

        ; KSHIFTRW k1, k2, 4 (VEX.L0.66.0F3A.W1 30 /r imm8)
        _emit 0xC4
        _emit 0xE3
        _emit 0xF9
        _emit 0x30
        _emit 0xCA
        _emit 0x04

        ; KUNPCKBW k1, k2, k3 (VEX.NDS.256.66.0F.W0 4B /r)
        ; C5 [1,1101,1,01] = C5 ED
        _emit 0xC5
        _emit 0xED
        _emit 0x4B
        _emit 0xCB

        ; KORTESTW k1, k2 (VEX.L0.0F.W0 98 /r)
        _emit 0xC5
        _emit 0xF8
        _emit 0x98
        _emit 0xCA

        ; KTESTW k1, k2 (VEX.L0.0F.W0 99 /r)
        _emit 0xC5
        _emit 0xF8
        _emit 0x99
        _emit 0xCA
    }
}

/* ---- AVX-512F with broadcast from memory ---- */
__declspec(noinline) void test_avx512_bcast_mem(void)
{
    __asm {
        ; VADDPS zmm0, zmm1, [eax]{1to16} (EVEX.512.0F.W0 58 /r, b=1)
        ; P2: z=0,L'L=10,b=1,V'=1,aaa=000 -> 0x58
        ; ModR/M: mod=00,reg=000(zmm0),rm=000(eax) -> 0x00
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x58
        _emit 0x58
        _emit 0x00

        ; VADDPD zmm0, zmm1, [ecx]{1to8} (EVEX.512.66.0F.W1 58 /r, b=1)
        ; P1: W=1 -> 0xF5
        ; P2: z=0,L'L=10,b=1,V'=1,aaa=000 -> 0x58
        ; ModR/M: mod=00,reg=000,rm=001(ecx) -> 0x01
        _emit 0x62
        _emit 0xF1
        _emit 0xF5
        _emit 0x58
        _emit 0x58
        _emit 0x01

        ; VPADDD zmm0, zmm1, [edx]{1to16} (EVEX.512.66.0F.W0 FE /r, b=1)
        ; P1: 0x75,  P2: 0x58
        ; ModR/M: mod=00,reg=000,rm=010(edx) -> 0x02
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x58
        _emit 0xFE
        _emit 0x02

        ; VMULPS zmm0 {k1}, zmm1, [ebx]{1to16} (b=1, aaa=001)
        ; P2: z=0,L'L=10,b=1,V'=1,aaa=001 -> 0x59
        ; ModR/M: mod=00,reg=000,rm=011(ebx) -> 0x03
        _emit 0x62
        _emit 0xF1
        _emit 0x74
        _emit 0x59
        _emit 0x59
        _emit 0x03
    }
}

int main(void)
{
    test_avx512_arith();
    test_avx512_arith_pd();
    test_avx512_opmask();
    test_avx512_mov();
    test_avx512_int();
    test_avx512_broadcast();
    test_avx512_cmp();
    test_avx512_logical();
    test_avx512_shuffle();
    test_avx512_cvt();
    test_avx512_kmask();
    test_avx512_bcast_mem();
    return 0;
}
