/*
 * x86.c - Basic 32-bit x86 instruction stress test for PVDasm
 *
 * Tests: general-purpose instructions, control flow, string ops,
 *        addressing modes, prefixes, segment overrides, all ModR/M
 *        and SIB combinations.
 *
 * Compile: cl /nologo /Od /GS- /Fe:x86.exe x86.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>

#pragma optimize("", off)

/* ---- Data transfer ---- */
__declspec(noinline) void test_data_transfer(void)
{
    __asm {
        ; MOV variants
        mov eax, ebx
        mov ecx, edx
        mov esi, edi
        mov esp, ebp
        mov al, bl
        mov ch, dh
        mov ax, bx
        mov eax, 12345678h
        mov ebx, 0DEADBEEFh
        mov al, 42h
        mov cx, 1234h

        ; MOV memory addressing modes
        mov eax, [ebx]
        mov eax, [ebx+04h]
        mov eax, [ebx+12345678h]
        mov eax, [ebx+ecx*1]
        mov eax, [ebx+ecx*2]
        mov eax, [ebx+ecx*4]
        mov eax, [ebx+ecx*8]
        mov eax, [ebx+ecx*4+10h]
        mov eax, [esp]
        mov eax, [esp+04h]
        mov eax, [ebp]
        mov eax, [ebp+04h]
        mov [ebx], eax
        mov [ebx+ecx*4+10h], edx

        ; XCHG
        xchg eax, ebx
        xchg ecx, edx
        xchg al, bl

        ; LEA
        lea eax, [ebx+ecx*4+10h]
        lea edx, [esi+edi*8]
        lea eax, [ebp+0]

        ; MOVZX / MOVSX
        movzx eax, bl
        movzx eax, bx
        movsx ecx, dl
        movsx ecx, dx

        ; CBW/CWDE/CDQ
        cbw
        cwde
        cdq

        ; PUSH / POP all regs
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax
        push 12345678h
        push 42h

        ; PUSHAD / POPAD / PUSHFD / POPFD
        pushad
        popad
        pushfd
        popfd
    }
}

/* ---- Arithmetic ---- */
__declspec(noinline) void test_arithmetic(void)
{
    __asm {
        ; ADD
        add eax, ebx
        add eax, 10h
        add al, 5
        add [ebx], eax
        add eax, [ecx+edx*4]

        ; SUB
        sub eax, ebx
        sub eax, 10h
        sub al, 5

        ; ADC / SBB
        adc eax, ebx
        sbb eax, ebx
        adc eax, 100h
        sbb ecx, 200h

        ; INC / DEC
        inc eax
        inc ebx
        inc ecx
        inc edx
        dec esi
        dec edi
        dec ebp

        ; NEG / NOT
        neg eax
        not ebx

        ; MUL / IMUL / DIV / IDIV
        mul ebx
        imul ecx
        imul eax, ebx
        imul eax, ebx, 10h
        imul eax, 42h

        ; CMP
        cmp eax, ebx
        cmp eax, 0
        cmp al, 0FFh

        ; TEST
        test eax, ebx
        test eax, 0FFFFFFFFh
        test al, 01h
    }
}

/* ---- Bitwise / Shift ---- */
__declspec(noinline) void test_bitwise(void)
{
    __asm {
        ; AND / OR / XOR
        and eax, ebx
        or ecx, edx
        xor esi, edi
        and eax, 0FF00FF00h
        or eax, 000FF00FFh
        xor eax, eax

        ; SHL / SHR / SAR / ROL / ROR
        shl eax, 1
        shl eax, cl
        shl eax, 5
        shr ebx, 1
        shr ebx, cl
        sar ecx, 1
        sar ecx, cl
        rol edx, 1
        rol edx, cl
        ror esi, 1
        ror esi, cl
        rcl eax, 1
        rcr ebx, 1

        ; SHLD / SHRD (double-precision shift)
        shld eax, ebx, 4
        shld eax, ebx, cl
        shrd ecx, edx, 8
        shrd ecx, edx, cl

        ; BSF / BSR
        bsf eax, ebx
        bsr ecx, edx

        ; BT / BTS / BTR / BTC
        bt eax, 5
        bts ebx, 10
        btr ecx, 15
        btc edx, 20
        bt eax, ebx
        bts ecx, edx

        ; BSWAP
        bswap eax
        bswap ebx
        bswap ecx
        bswap edx
    }
}

/* ---- Control flow ---- */
__declspec(noinline) void test_control_flow(void)
{
    __asm {
        ; Conditional jumps (short)
        je lbl1
        jne lbl1
        jb lbl1
        jnb lbl1
        jbe lbl1
        ja lbl1
        jl lbl1
        jge lbl1
        jle lbl1
        jg lbl1
        js lbl1
        jns lbl1
        jp lbl1
        jnp lbl1
        jo lbl1
        jno lbl1

    lbl1:
        ; LOOP variants
        loop lbl1
        loope lbl1
        loopne lbl1

        ; CALL / RET
        call lbl2
        jmp lbl3
    lbl2:
        nop
        ret
    lbl3:

        ; SETcc
        sete al
        setne bl
        setb cl
        setnb dl
        setl al
        setge bl
        setle cl
        setg dl
        seto al
        setno bl
        sets cl
        setns dl
        setp al
        setnp bl
        setbe cl
        seta dl

        ; CMOVcc
        cmove eax, ebx
        cmovne ecx, edx
        cmovb eax, ebx
        cmovnb ecx, edx
        cmovl eax, ebx
        cmovge ecx, edx
        cmovle eax, ebx
        cmovg ecx, edx
        cmovo eax, ebx
        cmovs eax, ebx
    }
}

/* ---- String operations ---- */
__declspec(noinline) void test_string_ops(void)
{
    __asm {
        ; Basic string ops
        movsb
        movsw
        movsd
        stosb
        stosw
        stosd
        lodsb
        lodsw
        lodsd
        cmpsb
        cmpsw
        cmpsd
        scasb
        scasw
        scasd

        ; REP prefixed
        rep movsb
        rep movsd
        rep stosb
        rep stosd
        repe cmpsb
        repe cmpsd
        repne scasb
        repne scasd
    }
}

/* ---- Miscellaneous ---- */
__declspec(noinline) void test_misc(void)
{
    __asm {
        ; NOP (various sizes)
        nop

        ; CLC / STC / CLD / STD / CLI / STI / CMC
        clc
        stc
        cld
        std
        cmc

        ; CPUID
        cpuid

        ; RDTSC
        rdtsc

        ; XLAT
        xlatb

        ; ENTER / LEAVE
        enter 16, 0
        leave

        ; INT
        int 3

        ; Segment prefixes
        mov eax, fs:[0]

        ; Address size prefix (16-bit addressing in 32-bit mode)
        _emit 0x67  ; addr prefix
        _emit 0x8B  ; mov eax, [bx+si]
        _emit 0x00

        ; Operand size prefix (16-bit operand in 32-bit mode)
        _emit 0x66
        _emit 0x89  ; mov [ebx], ax
        _emit 0x03

        ; LOCK prefix
        lock inc dword ptr [eax]
        lock xadd [ebx], ecx
        lock cmpxchg [edx], eax

        ; CMPXCHG8B
        cmpxchg8b qword ptr [esi]

        ; XADD
        xadd [eax], ebx
    }
}

/* ---- FPU SSE3: FISTTP ---- */
__declspec(noinline) void test_fisttp(void)
{
    __asm {
        ; FISTTP m32 (DB /1 with memory operand, e.g. [eax])
        _emit 0xDB
        _emit 0x08  ; ModRM: mod=00 reg=1 rm=000 => fisttp dword ptr [eax]

        ; FISTTP m64 (DD /1 with memory operand)
        _emit 0xDD
        _emit 0x08  ; ModRM: mod=00 reg=1 rm=000 => fisttp qword ptr [eax]

        ; FISTTP m16 (DF /1 with memory operand)
        _emit 0xDF
        _emit 0x08  ; ModRM: mod=00 reg=1 rm=000 => fisttp word ptr [eax]
    }
}

/* ---- XSAVE / XRSTOR / XSAVEOPT ---- */
__declspec(noinline) void test_xsave(void)
{
    __asm {
        ; XSAVE [eax] (0F AE /4)
        _emit 0x0F
        _emit 0xAE
        _emit 0x20  ; ModRM: mod=00 reg=4 rm=000

        ; XRSTOR [eax] (0F AE /5)
        _emit 0x0F
        _emit 0xAE
        _emit 0x28  ; ModRM: mod=00 reg=5 rm=000

        ; XSAVEOPT [eax] (0F AE /6)
        _emit 0x0F
        _emit 0xAE
        _emit 0x30  ; ModRM: mod=00 reg=6 rm=000
    }
}

/* ---- 0F 01 instructions: VMX, SVM, XGETBV, RDTSCP ---- */
__declspec(noinline) void test_0f01_instructions(void)
{
    __asm {
        ; VMCALL (0F 01 C1)
        _emit 0x0F
        _emit 0x01
        _emit 0xC1

        ; VMLAUNCH (0F 01 C2)
        _emit 0x0F
        _emit 0x01
        _emit 0xC2

        ; VMRESUME (0F 01 C3)
        _emit 0x0F
        _emit 0x01
        _emit 0xC3

        ; VMXOFF (0F 01 C4)
        _emit 0x0F
        _emit 0x01
        _emit 0xC4

        ; XGETBV (0F 01 D0)
        _emit 0x0F
        _emit 0x01
        _emit 0xD0

        ; XSETBV (0F 01 D1)
        _emit 0x0F
        _emit 0x01
        _emit 0xD1

        ; XEND (0F 01 D5)
        _emit 0x0F
        _emit 0x01
        _emit 0xD5

        ; XTEST (0F 01 D6)
        _emit 0x0F
        _emit 0x01
        _emit 0xD6

        ; VMRUN (0F 01 D8)
        _emit 0x0F
        _emit 0x01
        _emit 0xD8

        ; VMMCALL (0F 01 D9)
        _emit 0x0F
        _emit 0x01
        _emit 0xD9

        ; VMLOAD (0F 01 DA)
        _emit 0x0F
        _emit 0x01
        _emit 0xDA

        ; VMSAVE (0F 01 DB)
        _emit 0x0F
        _emit 0x01
        _emit 0xDB

        ; STGI (0F 01 DC)
        _emit 0x0F
        _emit 0x01
        _emit 0xDC

        ; CLGI (0F 01 DD)
        _emit 0x0F
        _emit 0x01
        _emit 0xDD

        ; SKINIT (0F 01 DE)
        _emit 0x0F
        _emit 0x01
        _emit 0xDE

        ; INVLPGA (0F 01 DF)
        _emit 0x0F
        _emit 0x01
        _emit 0xDF

        ; RDTSCP (0F 01 F9)
        _emit 0x0F
        _emit 0x01
        _emit 0xF9
    }
}

/* ---- LZCNT / TZCNT ---- */
__declspec(noinline) void test_lzcnt_tzcnt(void)
{
    __asm {
        ; LZCNT eax, ebx (F3 0F BD C3)
        _emit 0xF3
        _emit 0x0F
        _emit 0xBD
        _emit 0xC3  ; ModRM: mod=11 reg=0(eax) rm=3(ebx)

        ; TZCNT eax, ebx (F3 0F BC C3)
        _emit 0xF3
        _emit 0x0F
        _emit 0xBC
        _emit 0xC3  ; ModRM: mod=11 reg=0(eax) rm=3(ebx)

        ; LZCNT ecx, [eax] (F3 0F BD 08)
        _emit 0xF3
        _emit 0x0F
        _emit 0xBD
        _emit 0x08  ; ModRM: mod=00 reg=1(ecx) rm=0([eax])

        ; TZCNT edx, [eax] (F3 0F BC 10)
        _emit 0xF3
        _emit 0x0F
        _emit 0xBC
        _emit 0x10  ; ModRM: mod=00 reg=2(edx) rm=0([eax])
    }
}

/* ---- RDRAND / RDSEED ---- */
__declspec(noinline) void test_rdrand_rdseed(void)
{
    __asm {
        ; RDRAND eax (0F C7 F0) - ModRM: mod=11 reg=6 rm=0
        _emit 0x0F
        _emit 0xC7
        _emit 0xF0

        ; RDRAND ecx (0F C7 F1) - ModRM: mod=11 reg=6 rm=1
        _emit 0x0F
        _emit 0xC7
        _emit 0xF1

        ; RDSEED eax (0F C7 F8) - ModRM: mod=11 reg=7 rm=0
        _emit 0x0F
        _emit 0xC7
        _emit 0xF8

        ; RDSEED ebx (0F C7 FB) - ModRM: mod=11 reg=7 rm=3
        _emit 0x0F
        _emit 0xC7
        _emit 0xFB
    }
}

/* ---- AES-NI instructions ---- */
__declspec(noinline) void test_aesni(void)
{
    __asm {
        ; AESENC xmm0, xmm1 (66 0F 38 DC C1)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xDC
        _emit 0xC1  ; ModRM: mod=11 reg=0 rm=1

        ; AESENCLAST xmm0, xmm1 (66 0F 38 DD C1)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xDD
        _emit 0xC1

        ; AESDEC xmm0, xmm1 (66 0F 38 DE C1)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xDE
        _emit 0xC1

        ; AESDECLAST xmm0, xmm1 (66 0F 38 DF C1)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xDF
        _emit 0xC1

        ; AESIMC xmm0, xmm1 (66 0F 38 DB C1)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xDB
        _emit 0xC1

        ; AESKEYGENASSIST xmm0, xmm1, 01h (66 0F 3A DF C1 01)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0xDF
        _emit 0xC1
        _emit 0x01

        ; PCLMULQDQ xmm0, xmm1, 00h (66 0F 3A 44 C1 00)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0x44
        _emit 0xC1
        _emit 0x00
    }
}

/* ---- SHA instructions ---- */
__declspec(noinline) void test_sha(void)
{
    __asm {
        ; SHA1NEXTE xmm0, xmm1 (0F 38 C8 C1)
        _emit 0x0F
        _emit 0x38
        _emit 0xC8
        _emit 0xC1

        ; SHA1MSG1 xmm0, xmm1 (0F 38 C9 C1)
        _emit 0x0F
        _emit 0x38
        _emit 0xC9
        _emit 0xC1

        ; SHA1MSG2 xmm0, xmm1 (0F 38 CA C1)
        _emit 0x0F
        _emit 0x38
        _emit 0xCA
        _emit 0xC1

        ; SHA256RNDS2 xmm0, xmm1 (0F 38 CB C1)
        _emit 0x0F
        _emit 0x38
        _emit 0xCB
        _emit 0xC1

        ; SHA256MSG1 xmm0, xmm1 (0F 38 CC C1)
        _emit 0x0F
        _emit 0x38
        _emit 0xCC
        _emit 0xC1

        ; SHA256MSG2 xmm0, xmm1 (0F 38 CD C1)
        _emit 0x0F
        _emit 0x38
        _emit 0xCD
        _emit 0xC1
    }
}

/* ---- MOVBE ---- */
__declspec(noinline) void test_movbe(void)
{
    __asm {
        ; MOVBE eax, [ecx] (0F 38 F0 01) - load
        _emit 0x0F
        _emit 0x38
        _emit 0xF0
        _emit 0x01  ; ModRM: mod=00 reg=0 rm=1

        ; MOVBE [ecx], eax (0F 38 F1 01) - store
        _emit 0x0F
        _emit 0x38
        _emit 0xF1
        _emit 0x01  ; ModRM: mod=00 reg=0 rm=1
    }
}

/* ---- ADCX / ADOX ---- */
__declspec(noinline) void test_adcx_adox(void)
{
    __asm {
        ; ADCX eax, ebx (66 0F 38 F6 C3)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xF6
        _emit 0xC3  ; ModRM: mod=11 reg=0 rm=3

        ; ADOX eax, ebx (F3 0F 38 F6 C3)
        _emit 0xF3
        _emit 0x0F
        _emit 0x38
        _emit 0xF6
        _emit 0xC3
    }
}

/* ---- ENDBR32 / ENDBR64 ---- */
__declspec(noinline) void test_endbr(void)
{
    __asm {
        ; ENDBR32 (F3 0F 1E FB)
        _emit 0xF3
        _emit 0x0F
        _emit 0x1E
        _emit 0xFB

        ; ENDBR64 (F3 0F 1E FA)
        _emit 0xF3
        _emit 0x0F
        _emit 0x1E
        _emit 0xFA
    }
}

/* ---- VEX-encoded: BMI1/BMI2 ---- */
__declspec(noinline) void test_bmi(void)
{
    __asm {
        ; ANDN eax, ebx, ecx: VEX.NDS.LZ.0F38.W0 F2 /r
        ; VEX.3byte: C4 E2 60 F2 C1
        ; P1=E2 (R=1,X=1,B=1,mmmmm=00010), P2=60 (W=0,vvvv=~3=0100,L=0,pp=00)
        ; but vvvv is inverted: vvvv=ebx(3) => ~3=0xC => P2 bits: W=0, vvvv=1100, L=0, pp=00 => 0x60
        _emit 0xC4
        _emit 0xE2  ; R=1 X=1 B=1 mmmmm=00010
        _emit 0x60  ; W=0 vvvv=1100(ebx) L=0 pp=00
        _emit 0xF2  ; opcode
        _emit 0xC1  ; ModRM: mod=11 reg=0(eax) rm=1(ecx)

        ; BLSI eax, ecx: VEX.NDD.LZ.0F38.W0 F3 /3
        ; C4 E2 78 F3 D9 (vvvv=eax=0 => ~0=0xF => P2=0x78)
        _emit 0xC4
        _emit 0xE2
        _emit 0x78  ; W=0 vvvv=0000(eax) L=0 pp=00
        _emit 0xF3
        _emit 0xD9  ; ModRM: mod=11 reg=3(/3=blsi) rm=1(ecx)

        ; BLSMSK eax, ecx: VEX.NDD.LZ.0F38.W0 F3 /2
        _emit 0xC4
        _emit 0xE2
        _emit 0x78
        _emit 0xF3
        _emit 0xD1  ; ModRM: mod=11 reg=2(/2=blsmsk) rm=1(ecx)

        ; BLSR eax, ecx: VEX.NDD.LZ.0F38.W0 F3 /1
        _emit 0xC4
        _emit 0xE2
        _emit 0x78
        _emit 0xF3
        _emit 0xC9  ; ModRM: mod=11 reg=1(/1=blsr) rm=1(ecx)

        ; BEXTR eax, ecx, ebx: VEX.NDS.LZ.0F38.W0 F7 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x60  ; vvvv=ebx
        _emit 0xF7
        _emit 0xC1  ; ModRM: mod=11 reg=0(eax) rm=1(ecx)

        ; BZHI eax, ecx, ebx: VEX.NDS.LZ.0F38.W0 F5 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x60  ; vvvv=ebx
        _emit 0xF5
        _emit 0xC1  ; ModRM: mod=11 reg=0(eax) rm=1(ecx)

        ; SARX eax, ecx, ebx: VEX.NDS.LZ.F3.0F38.W0 F7 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x62  ; W=0 vvvv=ebx L=0 pp=10(F3)
        _emit 0xF7
        _emit 0xC1

        ; SHLX eax, ecx, ebx: VEX.NDS.LZ.66.0F38.W0 F7 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x61  ; W=0 vvvv=ebx L=0 pp=01(66)
        _emit 0xF7
        _emit 0xC1

        ; SHRX eax, ecx, ebx: VEX.NDS.LZ.F2.0F38.W0 F7 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x63  ; W=0 vvvv=ebx L=0 pp=11(F2)
        _emit 0xF7
        _emit 0xC1

        ; PDEP eax, ebx, ecx: VEX.NDS.LZ.F2.0F38.W0 F5 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x63  ; W=0 vvvv=ebx L=0 pp=11(F2)
        _emit 0xF5
        _emit 0xC1

        ; PEXT eax, ebx, ecx: VEX.NDS.LZ.F3.0F38.W0 F5 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x62  ; W=0 vvvv=ebx L=0 pp=10(F3)
        _emit 0xF5
        _emit 0xC1

        ; MULX edx, eax, ecx: VEX.NDD.LZ.F2.0F38.W0 F6 /r
        _emit 0xC4
        _emit 0xE2
        _emit 0x7B  ; W=0 vvvv=eax(0=>~0=F) => 0x7B = W=0,vvvv=1111,L=0,pp=11
        _emit 0xF6
        _emit 0xD1  ; ModRM: mod=11 reg=2(edx) rm=1(ecx)

        ; RORX eax, ecx, 5: VEX.LZ.F2.0F3A.W0 F0 /r ib
        _emit 0xC4
        _emit 0xE3  ; R=1 X=1 B=1 mmmmm=00011
        _emit 0x7B  ; W=0 vvvv=1111 L=0 pp=11(F2)
        _emit 0xF0
        _emit 0xC1  ; ModRM: mod=11 reg=0(eax) rm=1(ecx)
        _emit 0x05  ; imm8 = 5
    }
}

int main(void)
{
    test_data_transfer();
    test_arithmetic();
    test_bitwise();
    test_control_flow();
    test_string_ops();
    test_misc();
    test_fisttp();
    test_xsave();
    test_0f01_instructions();
    test_lzcnt_tzcnt();
    test_rdrand_rdseed();
    test_aesni();
    test_sha();
    test_movbe();
    test_adcx_adox();
    test_endbr();
    test_bmi();
    return 0;
}
