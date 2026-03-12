/*
 * mmx.c - MMX / 3DNow! instruction stress test for PVDasm
 *
 * Tests: all MMX pack/unpack, arithmetic, shift, comparison,
 *        data movement, and EMMS. 3DNow! included via _emit.
 *
 * Compile: cl /nologo /Od /GS- /Fe:mmx.exe mmx.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

static __int64 g_mm1 = 0x0102030405060708LL;
static __int64 g_mm2 = 0x0807060504030201LL;

/* ---- MMX data movement ---- */
__declspec(noinline) void test_mmx_move(void)
{
    __asm {
        ; MOVD (32-bit <-> MMX)
        movd mm0, eax
        movd mm1, [g_mm1]
        movd eax, mm0
        movd [g_mm1], mm1

        ; MOVQ (64-bit <-> MMX)
        movq mm0, [g_mm1]
        movq mm1, [g_mm2]
        movq mm2, mm0
        movq [g_mm2], mm3

        ; MOVQ between all registers
        movq mm0, mm1
        movq mm2, mm3
        movq mm4, mm5
        movq mm6, mm7
    }
}

/* ---- MMX pack/unpack ---- */
__declspec(noinline) void test_mmx_pack(void)
{
    __asm {
        movq mm0, [g_mm1]
        movq mm1, [g_mm2]

        ; PUNPCKL (interleave low)
        punpcklbw mm0, mm1
        punpcklwd mm2, mm3
        punpckldq mm4, mm5

        ; PUNPCKH (interleave high)
        punpckhbw mm0, mm1
        punpckhwd mm2, mm3
        punpckhdq mm4, mm5

        ; PACKSSWB / PACKSSDW / PACKUSWB
        packsswb mm0, mm1
        packssdw mm2, mm3
        packuswb mm4, mm5

        emms
    }
}

/* ---- MMX arithmetic ---- */
__declspec(noinline) void test_mmx_arithmetic(void)
{
    __asm {
        movq mm0, [g_mm1]
        movq mm1, [g_mm2]

        ; ADD (wrap)
        paddb mm0, mm1
        paddw mm2, mm3
        paddd mm4, mm5

        ; ADD (saturate)
        paddsb mm0, mm1
        paddsw mm2, mm3
        paddusb mm4, mm5
        paddusw mm6, mm7

        ; SUB (wrap)
        psubb mm0, mm1
        psubw mm2, mm3
        psubd mm4, mm5

        ; SUB (saturate)
        psubsb mm0, mm1
        psubsw mm2, mm3
        psubusb mm4, mm5
        psubusw mm6, mm7

        ; MULTIPLY
        pmullw mm0, mm1
        pmulhw mm2, mm3
        pmaddwd mm4, mm5

        ; SSE integer extensions to MMX
        pmulhuw mm0, mm1
        pminsw mm2, mm3
        pmaxsw mm4, mm5
        pminub mm6, mm7
        pmaxub mm0, mm1
        pavgb mm2, mm3
        pavgw mm4, mm5
        psadbw mm6, mm7

        emms
    }
}

/* ---- MMX comparison ---- */
__declspec(noinline) void test_mmx_compare(void)
{
    __asm {
        movq mm0, [g_mm1]
        movq mm1, [g_mm2]

        ; Compare equal
        pcmpeqb mm0, mm1
        pcmpeqw mm2, mm3
        pcmpeqd mm4, mm5

        ; Compare greater than
        pcmpgtb mm0, mm1
        pcmpgtw mm2, mm3
        pcmpgtd mm4, mm5

        emms
    }
}

/* ---- MMX shift ---- */
__declspec(noinline) void test_mmx_shift(void)
{
    __asm {
        movq mm0, [g_mm1]

        ; Logical shift left
        psllw mm0, 4
        pslld mm1, 8
        psllq mm2, 16

        ; Logical shift right
        psrlw mm3, 4
        psrld mm4, 8
        psrlq mm5, 16

        ; Arithmetic shift right
        psraw mm6, 4
        psrad mm7, 8

        ; Shift by MMX register
        psllw mm0, mm1
        pslld mm2, mm3
        psrlw mm4, mm5
        psrld mm6, mm7

        emms
    }
}

/* ---- MMX logical / misc ---- */
__declspec(noinline) void test_mmx_logical(void)
{
    __asm {
        movq mm0, [g_mm1]
        movq mm1, [g_mm2]

        ; Logical
        pand mm0, mm1
        pandn mm2, mm3
        por mm4, mm5
        pxor mm6, mm7

        ; PSHUFW (SSE extension)
        pshufw mm0, mm1, 1Bh

        ; PMOVMSKB
        pmovmskb eax, mm0

        ; MOVNTQ (non-temporal store)
        movntq [g_mm2], mm0

        ; MASKMOVQ
        ; maskmovq mm0, mm1  ; needs EDI pointing to valid memory

        ; PINSRW / PEXTRW (SSE integer extensions)
        pinsrw mm0, eax, 2
        pextrw eax, mm0, 3

        emms
    }
}

/* ---- 3DNow! (AMD) via raw bytes ---- */
__declspec(noinline) void test_3dnow(void)
{
    __asm {
        movq mm0, [g_mm1]
        movq mm1, [g_mm2]

        ; 3DNow! uses 0F 0F /r <imm8> encoding
        ; PFADD mm0, mm1 = 0F 0F C1 9E
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1  ; mm0, mm1
        _emit 0x9E  ; PFADD

        ; PFSUB mm0, mm1 = 0F 0F C1 9A
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0x9A  ; PFSUB

        ; PFMUL mm0, mm1 = 0F 0F C1 B4
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0xB4  ; PFMUL

        ; PFCMPEQ mm0, mm1 = 0F 0F C1 B0
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0xB0  ; PFCMPEQ

        ; PFCMPGE mm0, mm1 = 0F 0F C1 90
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0x90  ; PFCMPGE

        ; PFCMPGT mm0, mm1 = 0F 0F C1 A0
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0xA0  ; PFCMPGT

        ; PFMIN mm0, mm1 = 0F 0F C1 94
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0x94  ; PFMIN

        ; PFMAX mm0, mm1 = 0F 0F C1 A4
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0xA4  ; PFMAX

        ; PFRCP mm0, mm1 = 0F 0F C1 96
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0x96  ; PFRCP

        ; PFRSQRT mm0, mm1 = 0F 0F C1 97
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0x97  ; PFRSQRT

        ; PI2FD mm0, mm1 = 0F 0F C1 0D
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0x0D  ; PI2FD

        ; PF2ID mm0, mm1 = 0F 0F C1 1D
        _emit 0x0F
        _emit 0x0F
        _emit 0xC1
        _emit 0x1D  ; PF2ID

        ; FEMMS (3DNow! version of EMMS)
        _emit 0x0F
        _emit 0x0E

        emms
    }
}

int main(void)
{
    test_mmx_move();
    test_mmx_pack();
    test_mmx_arithmetic();
    test_mmx_compare();
    test_mmx_shift();
    test_mmx_logical();
    test_3dnow();
    return 0;
}
