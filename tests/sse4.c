/*
 * sse4.c - SSE4.1 + SSE4.2 instruction stress test for PVDasm
 *
 * Tests: SSE4.1 blend, round, pmovsxXX, pmovzxXX, ptest, pmulld,
 *        phminposuw, dpps, dppd, mpsadbw, insertps, extractps,
 *        pinsrb/d, pextrb/w/d.
 *        SSE4.2 pcmpgtq, pcmpestrm/i, pcmpistrm/i, crc32, popcnt.
 *
 * Compile: cl /nologo /Od /GS- /arch:SSE2 /Fe:sse4.exe sse4.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

/* ======== SSE4.1 (0F 38 xx with 66 prefix) ======== */

__declspec(noinline) void test_sse41_blend(void)
{
    __asm {
        ; PBLENDVB xmm0, xmm1 (66 0F 38 10 /r) - implicit xmm0 mask
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x10
        _emit 0xC1

        ; BLENDVPS xmm0, xmm1 (66 0F 38 14 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x14
        _emit 0xC1

        ; BLENDVPD xmm0, xmm1 (66 0F 38 15 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x15
        _emit 0xC1

        ; BLENDPS xmm0, xmm1, imm8 (66 0F 3A 0C /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x0C
        _emit 0xC1
        _emit 0x0A  ; blend mask

        ; BLENDPD xmm0, xmm1, imm8 (66 0F 3A 0D /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x0D
        _emit 0xC1
        _emit 0x01

        ; PBLENDW xmm0, xmm1, imm8 (66 0F 3A 0E /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x0E
        _emit 0xC1
        _emit 0xCC
    }
}

__declspec(noinline) void test_sse41_round(void)
{
    __asm {
        ; ROUNDPS xmm0, xmm1, imm8 (66 0F 3A 08 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x08
        _emit 0xC1
        _emit 0x00  ; round to nearest

        ; ROUNDPD xmm0, xmm1, imm8 (66 0F 3A 09 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x09
        _emit 0xC1
        _emit 0x01  ; round toward -inf

        ; ROUNDSS xmm0, xmm1, imm8 (66 0F 3A 0A /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x0A
        _emit 0xC1
        _emit 0x02  ; round toward +inf

        ; ROUNDSD xmm0, xmm1, imm8 (66 0F 3A 0B /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x0B
        _emit 0xC1
        _emit 0x03  ; truncate
    }
}

__declspec(noinline) void test_sse41_pmovsx(void)
{
    __asm {
        ; PMOVSXBW xmm0, xmm1 (66 0F 38 20 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x20
        _emit 0xC1

        ; PMOVSXBD xmm0, xmm1 (66 0F 38 21)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x21
        _emit 0xC1

        ; PMOVSXBQ xmm0, xmm1 (66 0F 38 22)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x22
        _emit 0xC1

        ; PMOVSXWD xmm0, xmm1 (66 0F 38 23)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x23
        _emit 0xC1

        ; PMOVSXWQ xmm0, xmm1 (66 0F 38 24)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x24
        _emit 0xC1

        ; PMOVSXDQ xmm0, xmm1 (66 0F 38 25)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x25
        _emit 0xC1
    }
}

__declspec(noinline) void test_sse41_pmovzx(void)
{
    __asm {
        ; PMOVZXBW xmm0, xmm1 (66 0F 38 30)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x30
        _emit 0xC1

        ; PMOVZXBD (66 0F 38 31)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x31
        _emit 0xC1

        ; PMOVZXBQ (66 0F 38 32)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x32
        _emit 0xC1

        ; PMOVZXWD (66 0F 38 33)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x33
        _emit 0xC1

        ; PMOVZXWQ (66 0F 38 34)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x34
        _emit 0xC1

        ; PMOVZXDQ (66 0F 38 35)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x35
        _emit 0xC1
    }
}

__declspec(noinline) void test_sse41_minmax_mul(void)
{
    __asm {
        ; PTEST xmm0, xmm1 (66 0F 38 17 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x17
        _emit 0xC1

        ; PMULDQ xmm0, xmm1 (66 0F 38 28)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x28
        _emit 0xC1

        ; PCMPEQQ xmm0, xmm1 (66 0F 38 29)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x29
        _emit 0xC1

        ; PACKUSDW xmm0, xmm1 (66 0F 38 2B)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x2B
        _emit 0xC1

        ; PMINSB (66 0F 38 38)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x38
        _emit 0xC1

        ; PMINSD (66 0F 38 39)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x39
        _emit 0xC1

        ; PMINUW (66 0F 38 3A)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x3A
        _emit 0xC1

        ; PMINUD (66 0F 38 3B)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x3B
        _emit 0xC1

        ; PMAXSB (66 0F 38 3C)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x3C
        _emit 0xC1

        ; PMAXSD (66 0F 38 3D)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x3D
        _emit 0xC1

        ; PMAXUW (66 0F 38 3E)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x3E
        _emit 0xC1

        ; PMAXUD (66 0F 38 3F)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x3F
        _emit 0xC1

        ; PMULLD xmm0, xmm1 (66 0F 38 40)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x40
        _emit 0xC1

        ; PHMINPOSUW xmm0, xmm1 (66 0F 38 41)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x41
        _emit 0xC1
    }
}

__declspec(noinline) void test_sse41_insert_extract(void)
{
    __asm {
        ; PINSRB xmm0, eax, 3 (66 0F 3A 20 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x20
        _emit 0xC0  ; xmm0, eax
        _emit 0x03

        ; INSERTPS xmm0, xmm1, imm8 (66 0F 3A 21 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x21
        _emit 0xC1
        _emit 0x10

        ; PINSRD xmm0, eax, 2 (66 0F 3A 22 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x22
        _emit 0xC0
        _emit 0x02

        ; PEXTRB eax, xmm0, 5 (66 0F 3A 14 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x14
        _emit 0xC0
        _emit 0x05

        ; PEXTRW eax, xmm0, 3 (66 0F 3A 15 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x15
        _emit 0xC0
        _emit 0x03

        ; PEXTRD eax, xmm0, 1 (66 0F 3A 16 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x16
        _emit 0xC0
        _emit 0x01

        ; EXTRACTPS eax, xmm0, 2 (66 0F 3A 17 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x17
        _emit 0xC0
        _emit 0x02
    }
}

__declspec(noinline) void test_sse41_dp_mpsadbw(void)
{
    __asm {
        ; DPPS xmm0, xmm1, imm8 (66 0F 3A 40 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x40
        _emit 0xC1
        _emit 0xFF

        ; DPPD xmm0, xmm1, imm8 (66 0F 3A 41 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x41
        _emit 0xC1
        _emit 0x33

        ; MPSADBW xmm0, xmm1, imm8 (66 0F 3A 42 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x42
        _emit 0xC1
        _emit 0x04
    }
}

/* ======== SSE4.2 ======== */

__declspec(noinline) void test_sse42_string(void)
{
    __asm {
        ; PCMPGTQ xmm0, xmm1 (66 0F 38 37 /r)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x37
        _emit 0xC1

        ; PCMPESTRM xmm0, xmm1, imm8 (66 0F 3A 60 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x60
        _emit 0xC1
        _emit 0x00

        ; PCMPESTRI xmm0, xmm1, imm8 (66 0F 3A 61 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x61
        _emit 0xC1
        _emit 0x08

        ; PCMPISTRM xmm0, xmm1, imm8 (66 0F 3A 62 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x62
        _emit 0xC1
        _emit 0x40

        ; PCMPISTRI xmm0, xmm1, imm8 (66 0F 3A 63 /r imm8)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x63
        _emit 0xC1
        _emit 0x0C
    }
}

__declspec(noinline) void test_sse42_crc_popcnt(void)
{
    __asm {
        ; CRC32 eax, bl (F2 0F 38 F0 /r)
        _emit 0xF2
        _emit 0x0F
        _emit 0x38
        _emit 0xF0
        _emit 0xC3  ; eax, bl

        ; CRC32 eax, ebx (F2 0F 38 F1 /r)
        _emit 0xF2
        _emit 0x0F
        _emit 0x38
        _emit 0xF1
        _emit 0xC3  ; eax, ebx

        ; CRC32 eax, bx (66 F2 0F 38 F1 /r)
        _emit 0x66
        _emit 0xF2
        _emit 0x0F
        _emit 0x38
        _emit 0xF1
        _emit 0xC3  ; eax, bx

        ; POPCNT eax, ebx (F3 0F B8 /r)
        _emit 0xF3
        _emit 0x0F
        _emit 0xB8
        _emit 0xC3  ; eax, ebx

        ; POPCNT ecx, edx
        _emit 0xF3
        _emit 0x0F
        _emit 0xB8
        _emit 0xCA  ; ecx, edx
    }
}

int main(void)
{
    test_sse41_blend();
    test_sse41_round();
    test_sse41_pmovsx();
    test_sse41_pmovzx();
    test_sse41_minmax_mul();
    test_sse41_insert_extract();
    test_sse41_dp_mpsadbw();
    test_sse42_string();
    test_sse42_crc_popcnt();
    return 0;
}
