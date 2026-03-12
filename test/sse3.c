/*
 * sse3.c - SSE3 + SSSE3 instruction stress test for PVDasm
 *
 * Tests: SSE3 horizontal add/sub, movddup, movsldup, movshdup,
 *        addsubps/pd, lddqu, monitor, mwait.
 *        SSSE3 pshufb, phaddw/d, pabsb/w/d, palignr, etc.
 *
 * All instructions encoded via _emit (raw bytes) since MSVC inline
 * asm doesn't support these mnemonics natively.
 *
 * Compile: cl /nologo /Od /GS- /arch:SSE2 /Fe:sse3.exe sse3.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

/* ======== SSE3 Instructions ======== */

/* ---- SSE3 horizontal add/sub ---- */
__declspec(noinline) void test_sse3_hadd_hsub(void)
{
    __asm {
        ; HADDPS xmm0, xmm1 (F2 0F 7C /r)
        _emit 0xF2
        _emit 0x0F
        _emit 0x7C
        _emit 0xC1

        ; HADDPD xmm0, xmm1 (66 0F 7C /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x7C
        _emit 0xC1

        ; HSUBPS xmm0, xmm1 (F2 0F 7D /r)
        _emit 0xF2
        _emit 0x0F
        _emit 0x7D
        _emit 0xC1

        ; HSUBPD xmm0, xmm1 (66 0F 7D /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x7D
        _emit 0xC1

        ; ADDSUBPS xmm0, xmm1 (F2 0F D0 /r)
        _emit 0xF2
        _emit 0x0F
        _emit 0xD0
        _emit 0xC1

        ; ADDSUBPD xmm0, xmm1 (66 0F D0 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0xD0
        _emit 0xC1

        ; HADDPS xmm2, xmm3
        _emit 0xF2
        _emit 0x0F
        _emit 0x7C
        _emit 0xD3

        ; HSUBPS xmm4, xmm5
        _emit 0xF2
        _emit 0x0F
        _emit 0x7D
        _emit 0xE5
    }
}

/* ---- SSE3 data movement ---- */
__declspec(noinline) void test_sse3_move(void)
{
    __asm {
        ; MOVDDUP xmm0, xmm1 (F2 0F 12 /r)
        _emit 0xF2
        _emit 0x0F
        _emit 0x12
        _emit 0xC1

        ; MOVSLDUP xmm0, xmm1 (F3 0F 12 /r)
        _emit 0xF3
        _emit 0x0F
        _emit 0x12
        _emit 0xC1

        ; MOVSHDUP xmm0, xmm1 (F3 0F 16 /r)
        _emit 0xF3
        _emit 0x0F
        _emit 0x16
        _emit 0xC1

        ; MOVDDUP xmm2, xmm3
        _emit 0xF2
        _emit 0x0F
        _emit 0x12
        _emit 0xD3

        ; MOVSLDUP xmm4, xmm5
        _emit 0xF3
        _emit 0x0F
        _emit 0x12
        _emit 0xE5

        ; MOVSHDUP xmm6, xmm7
        _emit 0xF3
        _emit 0x0F
        _emit 0x16
        _emit 0xF7

        ; LDDQU xmm0, [ebx] (F2 0F F0 /r)
        _emit 0xF2
        _emit 0x0F
        _emit 0xF0
        _emit 0x03  ; mod=00, xmm0, [ebx]
    }
}

/* ---- SSE3 monitor/mwait ---- */
__declspec(noinline) void test_sse3_monitor(void)
{
    __asm {
        ; MONITOR (0F 01 C8) - just test encoding, don't actually execute
        ; MWAIT   (0F 01 C9)
        ; We'll jump over them to avoid actual execution
        jmp skip_monitor
        _emit 0x0F
        _emit 0x01
        _emit 0xC8  ; monitor

        _emit 0x0F
        _emit 0x01
        _emit 0xC9  ; mwait
    skip_monitor:
        nop
    }
}

/* ======== SSSE3 Instructions (3-byte: 0F 38 xx) ======== */

/* ---- SSSE3 shuffle / horizontal ---- */
__declspec(noinline) void test_ssse3_shuffle(void)
{
    __asm {
        ; PSHUFB xmm0, xmm1 (66 0F 38 00 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x00
        _emit 0xC1

        ; PHADDW xmm0, xmm1 (66 0F 38 01 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x01
        _emit 0xC1

        ; PHADDD xmm0, xmm1 (66 0F 38 02 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x02
        _emit 0xC1

        ; PHADDSW xmm0, xmm1 (66 0F 38 03 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x03
        _emit 0xC1

        ; PMADDUBSW xmm0, xmm1 (66 0F 38 04 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x04
        _emit 0xC1

        ; PHSUBW xmm0, xmm1 (66 0F 38 05 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x05
        _emit 0xC1

        ; PHSUBD xmm0, xmm1 (66 0F 38 06 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x06
        _emit 0xC1

        ; PHSUBSW xmm0, xmm1 (66 0F 38 07 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x07
        _emit 0xC1

        ; PMULHRSW xmm0, xmm1 (66 0F 38 0B /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x0B
        _emit 0xC1
    }
}

/* ---- SSSE3 sign / abs ---- */
__declspec(noinline) void test_ssse3_sign_abs(void)
{
    __asm {
        ; PSIGNB xmm0, xmm1 (66 0F 38 08 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x08
        _emit 0xC1

        ; PSIGNW xmm0, xmm1 (66 0F 38 09 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x09
        _emit 0xC1

        ; PSIGND xmm0, xmm1 (66 0F 38 0A /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x0A
        _emit 0xC1

        ; PABSB xmm0, xmm1 (66 0F 38 1C /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x1C
        _emit 0xC1

        ; PABSW xmm0, xmm1 (66 0F 38 1D /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x1D
        _emit 0xC1

        ; PABSD xmm0, xmm1 (66 0F 38 1E /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x1E
        _emit 0xC1
    }
}

/* ---- SSSE3 MMX register forms (no 66 prefix) ---- */
__declspec(noinline) void test_ssse3_mmx(void)
{
    __asm {
        ; PSHUFB mm0, mm1 (0F 38 00 /r)
        _emit 0x0F
        _emit 0x38
        _emit 0x00
        _emit 0xC1  ; mm0, mm1

        ; PHADDW mm0, mm1 (0F 38 01 /r)
        _emit 0x0F
        _emit 0x38
        _emit 0x01
        _emit 0xC1

        ; PABSB mm0, mm1 (0F 38 1C /r)
        _emit 0x0F
        _emit 0x38
        _emit 0x1C
        _emit 0xC1

        ; PABSW mm0, mm1 (0F 38 1D /r)
        _emit 0x0F
        _emit 0x38
        _emit 0x1D
        _emit 0xC1

        ; PABSD mm0, mm1 (0F 38 1E /r)
        _emit 0x0F
        _emit 0x38
        _emit 0x1E
        _emit 0xC1

        emms
    }
}

/* ---- SSSE3 palignr (0F 3A 0F) ---- */
__declspec(noinline) void test_ssse3_palignr(void)
{
    __asm {
        ; PALIGNR xmm0, xmm1, 4 (66 0F 3A 0F /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x0F
        _emit 0xC1
        _emit 0x04  ; imm8 = 4

        ; PALIGNR xmm2, xmm3, 8
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x0F
        _emit 0xD3
        _emit 0x08  ; imm8 = 8

        ; PALIGNR mm0, mm1, 4 (MMX form: 0F 3A 0F /r imm8)
        _emit 0x0F
        _emit 0x3A
        _emit 0x0F
        _emit 0xC1
        _emit 0x04

        emms
    }
}

int main(void)
{
    test_sse3_hadd_hsub();
    test_sse3_move();
    test_sse3_monitor();
    test_ssse3_shuffle();
    test_ssse3_sign_abs();
    test_ssse3_mmx();
    test_ssse3_palignr();
    return 0;
}
