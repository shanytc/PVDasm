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

/* ---- VMX extended instructions ---- */
__declspec(noinline) void test_vmx_extended(void)
{
    __asm {
        ; VMREAD eax, ecx (0F 78 C1) - register form
        _emit 0x0F
        _emit 0x78
        _emit 0xC1  ; ModRM: mod=11 reg=0(eax) rm=1(ecx)

        ; VMREAD edx, ebx (0F 78 DA) - register form
        _emit 0x0F
        _emit 0x78
        _emit 0xDA  ; ModRM: mod=11 reg=3(ebx) rm=2(edx)

        ; VMREAD [eax], ecx (0F 78 08) - memory form
        _emit 0x0F
        _emit 0x78
        _emit 0x08  ; ModRM: mod=00 reg=1(ecx) rm=0([eax])

        ; VMREAD [ecx+10h], edx (0F 78 51 10) - memory+disp8
        _emit 0x0F
        _emit 0x78
        _emit 0x51  ; ModRM: mod=01 reg=2(edx) rm=1([ecx+disp8])
        _emit 0x10

        ; VMWRITE ecx, eax (0F 79 C8) - register form
        _emit 0x0F
        _emit 0x79
        _emit 0xC8  ; ModRM: mod=11 reg=1(ecx) rm=0(eax)

        ; VMWRITE ebx, edx (0F 79 D3) - register form
        _emit 0x0F
        _emit 0x79
        _emit 0xD3  ; ModRM: mod=11 reg=2(edx) rm=3(ebx)

        ; VMWRITE eax, [ecx] (0F 79 01) - memory form
        _emit 0x0F
        _emit 0x79
        _emit 0x01  ; ModRM: mod=00 reg=0(eax) rm=1([ecx])

        ; VMPTRLD [eax] (NP 0F C7 /6 mem) = 0F C7 30
        _emit 0x0F
        _emit 0xC7
        _emit 0x30  ; ModRM: mod=00 reg=6 rm=0([eax])

        ; VMPTRLD [ecx+08h] (0F C7 71 08) - memory+disp8
        _emit 0x0F
        _emit 0xC7
        _emit 0x71  ; ModRM: mod=01 reg=6 rm=1([ecx+disp8])
        _emit 0x08

        ; VMPTRST [eax] (NP 0F C7 /7 mem) = 0F C7 38
        _emit 0x0F
        _emit 0xC7
        _emit 0x38  ; ModRM: mod=00 reg=7 rm=0([eax])

        ; VMCLEAR [eax] (66 0F C7 /6 mem) = 66 0F C7 30
        _emit 0x66
        _emit 0x0F
        _emit 0xC7
        _emit 0x30  ; ModRM: mod=00 reg=6 rm=0([eax])

        ; VMXON [eax] (F3 0F C7 /6 mem) = F3 0F C7 30
        _emit 0xF3
        _emit 0x0F
        _emit 0xC7
        _emit 0x30  ; ModRM: mod=00 reg=6 rm=0([eax])

        ; INVEPT eax, [ecx] (66 0F 38 80 01)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x80
        _emit 0x01  ; ModRM: mod=00 reg=0(eax) rm=1([ecx])

        ; INVEPT edx, [ebx] (66 0F 38 80 13)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x80
        _emit 0x13  ; ModRM: mod=00 reg=2(edx) rm=3([ebx])

        ; INVVPID eax, [ecx] (66 0F 38 81 01)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x81
        _emit 0x01  ; ModRM: mod=00 reg=0(eax) rm=1([ecx])

        ; INVVPID edx, [ebx+10h] (66 0F 38 81 53 10)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0x81
        _emit 0x53  ; ModRM: mod=01 reg=2(edx) rm=3([ebx+disp8])
        _emit 0x10
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

/* ---- TSX Extended: XBEGIN / XABORT ---- */
__declspec(noinline) void test_tsx_extended(void)
{
    __asm {
        ; XABORT 42h (C6 F8 42)
        _emit 0xC6
        _emit 0xF8
        _emit 0x42

        ; XABORT 00h (C6 F8 00)
        _emit 0xC6
        _emit 0xF8
        _emit 0x00

        ; XBEGIN rel32=0 (C7 F8 00000000) - jump to next instruction
        _emit 0xC7
        _emit 0xF8
        _emit 0x00
        _emit 0x00
        _emit 0x00
        _emit 0x00

        ; XBEGIN rel32=0x10 (C7 F8 10000000)
        _emit 0xC7
        _emit 0xF8
        _emit 0x10
        _emit 0x00
        _emit 0x00
        _emit 0x00

        ; XBEGIN rel16=0 with 66 prefix (66 C7 F8 0000)
        _emit 0x66
        _emit 0xC7
        _emit 0xF8
        _emit 0x00
        _emit 0x00
    }
}

/* ---- CET Extended: RDSSPD / INCSSPD ---- */
__declspec(noinline) void test_cet_extended(void)
{
    __asm {
        ; RDSSPD eax (F3 0F 1E C8) - ModRM: mod=11 reg=1 rm=0(eax)
        _emit 0xF3
        _emit 0x0F
        _emit 0x1E
        _emit 0xC8

        ; RDSSPD ecx (F3 0F 1E C9) - ModRM: mod=11 reg=1 rm=1(ecx)
        _emit 0xF3
        _emit 0x0F
        _emit 0x1E
        _emit 0xC9

        ; RDSSPD edx (F3 0F 1E CA) - ModRM: mod=11 reg=1 rm=2(edx)
        _emit 0xF3
        _emit 0x0F
        _emit 0x1E
        _emit 0xCA

        ; INCSSPD eax (F3 0F AE E8) - ModRM: mod=11 reg=5 rm=0(eax)
        _emit 0xF3
        _emit 0x0F
        _emit 0xAE
        _emit 0xE8

        ; INCSSPD ecx (F3 0F AE E9) - ModRM: mod=11 reg=5 rm=1(ecx)
        _emit 0xF3
        _emit 0x0F
        _emit 0xAE
        _emit 0xE9
    }
}

/* ---- GFNI Legacy (66 0F 38/3A) ---- */
__declspec(noinline) void test_gfni_legacy(void)
{
    __asm {
        ; GF2P8MULB xmm0, xmm1 (66 0F 38 CF C1)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xCF
        _emit 0xC1  ; ModRM: mod=11 reg=0 rm=1

        ; GF2P8MULB xmm2, xmm3 (66 0F 38 CF D3)
        _emit 0x66
        _emit 0x0F
        _emit 0x38
        _emit 0xCF
        _emit 0xD3  ; ModRM: mod=11 reg=2 rm=3

        ; GF2P8AFFINEQB xmm0, xmm1, 00h (66 0F 3A CE C1 00)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0xCE
        _emit 0xC1
        _emit 0x00

        ; GF2P8AFFINEQB xmm0, xmm1, 0Fh (66 0F 3A CE C1 0F)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0xCE
        _emit 0xC1
        _emit 0x0F

        ; GF2P8AFFINEINVQB xmm0, xmm1, 01h (66 0F 3A CF C1 01)
        _emit 0x66
        _emit 0x0F
        _emit 0x3A
        _emit 0xCF
        _emit 0xC1
        _emit 0x01
    }
}

/* ---- GFNI VEX + VAES VEX ---- */
__declspec(noinline) void test_gfni_vaes_vex(void)
{
    __asm {
        ; === GFNI VEX ===

        ; VGF2P8MULB xmm0, xmm1, xmm2 (VEX.128.66.0F38.W0 CF)
        ; C4 E2 71 CF C2
        _emit 0xC4
        _emit 0xE2  ; R=1 X=1 B=1 mmmmm=00010
        _emit 0x71  ; W=0 vvvv=~1=1110 L=0 pp=01(66)
        _emit 0xCF
        _emit 0xC2  ; ModRM: mod=11 reg=0(xmm0) rm=2(xmm2)

        ; VGF2P8AFFINEQB xmm0, xmm1, xmm2, 00h (VEX.128.66.0F3A.W0 CE)
        ; C4 E3 71 CE C2 00
        _emit 0xC4
        _emit 0xE3  ; mmmmm=00011
        _emit 0x71
        _emit 0xCE
        _emit 0xC2
        _emit 0x00

        ; VGF2P8AFFINEINVQB xmm0, xmm1, xmm2, 01h (VEX.128.66.0F3A.W0 CF)
        ; C4 E3 71 CF C2 01
        _emit 0xC4
        _emit 0xE3
        _emit 0x71
        _emit 0xCF
        _emit 0xC2
        _emit 0x01

        ; === VAES VEX ===

        ; VAESENC xmm0, xmm1, xmm2 (VEX.128.66.0F38.WIG DC)
        ; C4 E2 71 DC C2
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0xDC
        _emit 0xC2

        ; VAESENCLAST xmm0, xmm1, xmm2 (VEX.128.66.0F38.WIG DD)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0xDD
        _emit 0xC2

        ; VAESDEC xmm0, xmm1, xmm2 (VEX.128.66.0F38.WIG DE)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0xDE
        _emit 0xC2

        ; VAESDECLAST xmm0, xmm1, xmm2 (VEX.128.66.0F38.WIG DF)
        _emit 0xC4
        _emit 0xE2
        _emit 0x71
        _emit 0xDF
        _emit 0xC2
    }
}

/* ---- EVEX: GFNI, VAES, VPCLMULQDQ ---- */
__declspec(noinline) void test_evex_gfni_vaes_vpclmul(void)
{
    /*
     * EVEX prefix: 62 P0 P1 P2 OpByte ModRM [imm8]
     * P0: R(7) X(6) B(5) R'(4) 00(3:2) mm(1:0)  -- all R/X/B/R' inverted, =1 means no extension
     *     mm=01(0F), 10(0F38), 11(0F3A)
     * P1: W(7) ~vvvv(6:3) 1(2) pp(1:0)  -- vvvv inverted; pp: 01=66,10=F3,11=F2
     * P2: z(7) L'L(6:5) b(4) ~V'(3) aaa(2:0)  -- V' inverted
     *
     * Common values used below:
     *   P0=F2 (mm=10, 0F38), P0=F3 (mm=11, 0F3A), P0=F1 (mm=01, 0F)
     *   P1=75 (W=0,vvvv=1,pp=01), P1=F5 (W=1,vvvv=1,pp=01)
     *   P1=7D (W=0,vvvv=0,pp=01)
     *   P2=08 (128-bit,no mask), P2=48 (512-bit,no mask)
     *   ModRM C2 = mod=11 reg=0 rm=2
     */
    __asm {
        ; === GFNI EVEX (map 2 = 0F38) ===

        ; VGF2P8MULB xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W0 CF)
        ; 62 F2 75 08 CF C2
        _emit 0x62
        _emit 0xF2  ; P0: mm=10
        _emit 0x75  ; P1: W=0 vvvv=1 pp=01
        _emit 0x08  ; P2: LL=00(128) no mask
        _emit 0xCF  ; opcode
        _emit 0xC2  ; ModRM

        ; === GFNI EVEX (map 3 = 0F3A) ===

        ; VGF2P8AFFINEQB xmm0, xmm1, xmm2, 00h (EVEX.128.66.0F3A.W0 CE)
        ; 62 F3 75 08 CE C2 00
        _emit 0x62
        _emit 0xF3  ; P0: mm=11
        _emit 0x75
        _emit 0x08
        _emit 0xCE
        _emit 0xC2
        _emit 0x00  ; imm8

        ; VGF2P8AFFINEINVQB xmm0, xmm1, xmm2, 01h (EVEX.128.66.0F3A.W0 CF)
        ; 62 F3 75 08 CF C2 01
        _emit 0x62
        _emit 0xF3
        _emit 0x75
        _emit 0x08
        _emit 0xCF
        _emit 0xC2
        _emit 0x01

        ; === VAES EVEX (map 2 = 0F38) ===

        ; VAESENC xmm0, xmm1, xmm2 (EVEX.128.66.0F38.WIG DC)
        ; 62 F2 75 08 DC C2
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x08
        _emit 0xDC
        _emit 0xC2

        ; VAESENCLAST xmm0, xmm1, xmm2 (EVEX.128.66.0F38.WIG DD)
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x08
        _emit 0xDD
        _emit 0xC2

        ; VAESDEC xmm0, xmm1, xmm2 (EVEX.128.66.0F38.WIG DE)
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x08
        _emit 0xDE
        _emit 0xC2

        ; VAESDECLAST xmm0, xmm1, xmm2 (EVEX.128.66.0F38.WIG DF)
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x08
        _emit 0xDF
        _emit 0xC2

        ; === VPCLMULQDQ EVEX (map 3 = 0F3A) ===

        ; VPCLMULQDQ xmm0, xmm1, xmm2, 00h (EVEX.128.66.0F3A.W0 44)
        ; 62 F3 75 08 44 C2 00
        _emit 0x62
        _emit 0xF3
        _emit 0x75
        _emit 0x08
        _emit 0x44
        _emit 0xC2
        _emit 0x00

        ; VPCLMULQDQ xmm0, xmm1, xmm2, 11h (high x high)
        ; 62 F3 75 08 44 C2 11
        _emit 0x62
        _emit 0xF3
        _emit 0x75
        _emit 0x08
        _emit 0x44
        _emit 0xC2
        _emit 0x11
    }
}

/* ---- EVEX: AVX-512 IFMA + VBMI ---- */
__declspec(noinline) void test_evex_ifma_vbmi(void)
{
    __asm {
        ; === AVX-512 IFMA (EVEX map 2 = 0F38, W=1) ===

        ; VPMADD52LUQ xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W1 B4)
        ; 62 F2 F5 08 B4 C2
        _emit 0x62
        _emit 0xF2  ; P0: mm=10
        _emit 0xF5  ; P1: W=1 vvvv=1 pp=01
        _emit 0x08  ; P2: LL=00(128)
        _emit 0xB4
        _emit 0xC2

        ; VPMADD52HUQ xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W1 B5)
        ; 62 F2 F5 08 B5 C2
        _emit 0x62
        _emit 0xF2
        _emit 0xF5
        _emit 0x08
        _emit 0xB5
        _emit 0xC2

        ; === AVX-512 VBMI (EVEX map 2 = 0F38) ===

        ; VPERMI2B xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W0 75)
        ; 62 F2 75 08 75 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x75  ; W=0
        _emit 0x08
        _emit 0x75
        _emit 0xC2

        ; VPERMI2W xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W1 75)
        ; 62 F2 F5 08 75 C2
        _emit 0x62
        _emit 0xF2
        _emit 0xF5  ; W=1
        _emit 0x08
        _emit 0x75
        _emit 0xC2

        ; VPERMT2B xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W0 7D)
        ; 62 F2 75 08 7D C2
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x08
        _emit 0x7D
        _emit 0xC2

        ; VPERMT2W xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W1 7D)
        ; 62 F2 F5 08 7D C2
        _emit 0x62
        _emit 0xF2
        _emit 0xF5
        _emit 0x08
        _emit 0x7D
        _emit 0xC2

        ; VPMULTISHIFTQB xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W0 83)
        ; 62 F2 75 08 83 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x08
        _emit 0x83
        _emit 0xC2

        ; VPERMB xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W0 8D)
        ; 62 F2 75 08 8D C2
        _emit 0x62
        _emit 0xF2
        _emit 0x75
        _emit 0x08
        _emit 0x8D
        _emit 0xC2

        ; VPERMW xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W1 8D)
        ; 62 F2 F5 08 8D C2
        _emit 0x62
        _emit 0xF2
        _emit 0xF5
        _emit 0x08
        _emit 0x8D
        _emit 0xC2
    }
}

/* ---- EVEX: AVX-512 DQ ---- */
__declspec(noinline) void test_evex_avx512dq(void)
{
    __asm {
        ; === VPMULLQ (map 2, W=1, opcode 40) ===

        ; VPMULLQ xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W1 40)
        ; 62 F2 F5 08 40 C2
        _emit 0x62
        _emit 0xF2  ; mm=10
        _emit 0xF5  ; W=1 vvvv=1 pp=01
        _emit 0x08  ; LL=00
        _emit 0x40
        _emit 0xC2

        ; VPMULLD xmm0, xmm1, xmm2 (EVEX.128.66.0F38.W0 40) - for comparison
        ; 62 F2 75 08 40 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x75  ; W=0
        _emit 0x08
        _emit 0x40
        _emit 0xC2

        ; === W-dependent insert/extract (map 3) ===

        ; VINSERTF64X2 (W=1): EVEX.128.66.0F3A.W1 18
        ; 62 F3 F5 08 18 C2 00
        _emit 0x62
        _emit 0xF3  ; mm=11
        _emit 0xF5  ; W=1
        _emit 0x08
        _emit 0x18
        _emit 0xC2
        _emit 0x00

        ; VINSERTF32X4 (W=0): EVEX.128.66.0F3A.W0 18
        ; 62 F3 75 08 18 C2 00
        _emit 0x62
        _emit 0xF3
        _emit 0x75  ; W=0
        _emit 0x08
        _emit 0x18
        _emit 0xC2
        _emit 0x00

        ; VEXTRACTF64X2 (W=1): EVEX.128.66.0F3A.W1 19
        ; 62 F3 F5 08 19 C2 00
        _emit 0x62
        _emit 0xF3
        _emit 0xF5
        _emit 0x08
        _emit 0x19
        _emit 0xC2
        _emit 0x00

        ; VINSERTI64X2 (W=1): EVEX.128.66.0F3A.W1 38
        ; 62 F3 F5 08 38 C2 00
        _emit 0x62
        _emit 0xF3
        _emit 0xF5
        _emit 0x08
        _emit 0x38
        _emit 0xC2
        _emit 0x00

        ; VEXTRACTI64X2 (W=1): EVEX.128.66.0F3A.W1 39
        ; 62 F3 F5 08 39 C2 00
        _emit 0x62
        _emit 0xF3
        _emit 0xF5
        _emit 0x08
        _emit 0x39
        _emit 0xC2
        _emit 0x00

        ; === DQ Conversion instructions (map 1 = 0F) ===

        ; VMOVD xmm0, xmm2 (EVEX.128.66.0F.W0 6E)
        ; 62 F1 7D 08 6E C2
        _emit 0x62
        _emit 0xF1  ; mm=01
        _emit 0x7D  ; W=0 vvvv=0 pp=01
        _emit 0x08
        _emit 0x6E
        _emit 0xC2

        ; VMOVQ xmm0, xmm2 (EVEX.128.66.0F.W1 6E)
        ; 62 F1 FD 08 6E C2
        _emit 0x62
        _emit 0xF1
        _emit 0xFD  ; W=1 vvvv=0 pp=01
        _emit 0x08
        _emit 0x6E
        _emit 0xC2

        ; VCVTTPS2UDQ zmm0, zmm2 (EVEX.512.66.0F.W0 78)
        ; 62 F1 7D 48 78 C2
        _emit 0x62
        _emit 0xF1
        _emit 0x7D  ; W=0 vvvv=0 pp=01
        _emit 0x48  ; LL=10(512)
        _emit 0x78
        _emit 0xC2

        ; VCVTTPD2UDQ zmm0, zmm2 (EVEX.512.66.0F.W1 78)
        ; 62 F1 FD 48 78 C2
        _emit 0x62
        _emit 0xF1
        _emit 0xFD  ; W=1
        _emit 0x48
        _emit 0x78
        _emit 0xC2

        ; VCVTPS2UDQ zmm0, zmm2 (EVEX.512.66.0F.W0 79)
        ; 62 F1 7D 48 79 C2
        _emit 0x62
        _emit 0xF1
        _emit 0x7D
        _emit 0x48
        _emit 0x79
        _emit 0xC2

        ; VCVTTPD2QQ zmm0, zmm2 (EVEX.512.66.0F.W1 7A)
        ; 62 F1 FD 48 7A C2
        _emit 0x62
        _emit 0xF1
        _emit 0xFD
        _emit 0x48
        _emit 0x7A
        _emit 0xC2

        ; VCVTTPS2QQ zmm0, zmm2 (EVEX.512.66.0F.W0 7A)
        ; 62 F1 7D 48 7A C2
        _emit 0x62
        _emit 0xF1
        _emit 0x7D
        _emit 0x48
        _emit 0x7A
        _emit 0xC2

        ; VCVTPD2QQ zmm0, zmm2 (EVEX.512.66.0F.W1 7B)
        ; 62 F1 FD 48 7B C2
        _emit 0x62
        _emit 0xF1
        _emit 0xFD
        _emit 0x48
        _emit 0x7B
        _emit 0xC2

        ; VCVTPS2QQ zmm0, zmm2 (EVEX.512.66.0F.W0 7B)
        ; 62 F1 7D 48 7B C2
        _emit 0x62
        _emit 0xF1
        _emit 0x7D
        _emit 0x48
        _emit 0x7B
        _emit 0xC2

        ; VCVTTPD2DQ zmm0, zmm2 (EVEX.512.66.0F.W0 E6, pp=01)
        ; 62 F1 7D 48 E6 C2
        _emit 0x62
        _emit 0xF1
        _emit 0x7D
        _emit 0x48
        _emit 0xE6
        _emit 0xC2

        ; VCVTDQ2PD zmm0, zmm2 (EVEX.512.F3.0F.W0 E6, pp=10)
        ; 62 F1 7E 48 E6 C2
        _emit 0x62
        _emit 0xF1
        _emit 0x7E  ; W=0 vvvv=0 pp=10(F3)
        _emit 0x48
        _emit 0xE6
        _emit 0xC2

        ; VMOVD xmm0, xmm2 (EVEX.128.66.0F.W0 7E) - store form
        ; 62 F1 7D 08 7E C2
        _emit 0x62
        _emit 0xF1
        _emit 0x7D
        _emit 0x08
        _emit 0x7E
        _emit 0xC2
    }
}

/* ---- EVEX: AVX-512 DQ/BW new instructions ---- */
__declspec(noinline) void test_evex_avx512dq_bw_new(void)
{
    __asm {
        ; === VCVTQQ2PS (EVEX.NP.0F.W1 5B, pp=0) zmm ===
        ; 62 F1 FC 48 5B C2  (mm=01, W=1, pp=00, LL=10)
        _emit 0x62
        _emit 0xF1  ; mm=01
        _emit 0xFC  ; W=1 vvvv=0 pp=00
        _emit 0x48  ; LL=10(512)
        _emit 0x5B
        _emit 0xC2

        ; === VCVTQQ2PD (EVEX.F3.0F.W1 E6, pp=2) zmm ===
        ; 62 F1 FE 48 E6 C2  (mm=01, W=1, pp=10)
        _emit 0x62
        _emit 0xF1
        _emit 0xFE  ; W=1 vvvv=0 pp=10(F3)
        _emit 0x48
        _emit 0xE6
        _emit 0xC2

        ; === Truncation: VPMOVUSWB (EVEX.F3.0F38 10, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 10 C2  (mm=10, W=0, pp=10, LL=10)
        _emit 0x62
        _emit 0xF2  ; mm=10
        _emit 0x7E  ; W=0 vvvv=0 pp=10(F3)
        _emit 0x48
        _emit 0x10
        _emit 0xC2

        ; === VPMOVUSDB (EVEX.F3.0F38 11, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 11 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x11
        _emit 0xC2

        ; === VPMOVUSQB (EVEX.F3.0F38 12, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 12 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x12
        _emit 0xC2

        ; === VPMOVUSDW (EVEX.F3.0F38 13, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 13 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x13
        _emit 0xC2

        ; === VPMOVUSQW (EVEX.F3.0F38 14, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 14 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x14
        _emit 0xC2

        ; === VPMOVUSQD (EVEX.F3.0F38 15, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 15 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x15
        _emit 0xC2

        ; === VPMOVSWB (EVEX.F3.0F38 20, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 20 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x20
        _emit 0xC2

        ; === VPMOVSDB (EVEX.F3.0F38 21, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 21 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x21
        _emit 0xC2

        ; === VPMOVSQB (EVEX.F3.0F38 22, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 22 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x22
        _emit 0xC2

        ; === VPMOVSDW (EVEX.F3.0F38 23, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 23 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x23
        _emit 0xC2

        ; === VPMOVSQW (EVEX.F3.0F38 24, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 24 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x24
        _emit 0xC2

        ; === VPMOVSQD (EVEX.F3.0F38 25, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 25 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x25
        _emit 0xC2

        ; === VPMOVWB (EVEX.F3.0F38 30, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 30 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x30
        _emit 0xC2

        ; === VPMOVDB (EVEX.F3.0F38 31, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 31 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x31
        _emit 0xC2

        ; === VPMOVQB (EVEX.F3.0F38 32, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 32 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x32
        _emit 0xC2

        ; === VPMOVDW (EVEX.F3.0F38 33, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 33 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x33
        _emit 0xC2

        ; === VPMOVQW (EVEX.F3.0F38 34, pp=2) zmm->xmm ===
        ; 62 F2 7E 48 34 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x34
        _emit 0xC2

        ; === VPMOVQD (EVEX.F3.0F38 35, pp=2) zmm->ymm ===
        ; 62 F2 7E 48 35 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x48
        _emit 0x35
        _emit 0xC2

        ; === VPMOVM2B (EVEX.F3.0F38.W0 28, pp=2) zmm, k ===
        ; 62 F2 7E 08 28 C2  (LL=00, W=0)
        _emit 0x62
        _emit 0xF2
        _emit 0x7E  ; W=0 vvvv=0 pp=10(F3)
        _emit 0x08  ; LL=00(128)
        _emit 0x28
        _emit 0xC2  ; REG=0(xmm0), RM=2(k2)

        ; === VPMOVM2W (EVEX.F3.0F38.W1 28, pp=2) zmm, k ===
        ; 62 F2 FE 08 28 C2
        _emit 0x62
        _emit 0xF2
        _emit 0xFE  ; W=1 vvvv=0 pp=10(F3)
        _emit 0x08
        _emit 0x28
        _emit 0xC2

        ; === VPMOVB2M (EVEX.F3.0F38.W0 29, pp=2) k, xmm ===
        ; 62 F2 7E 08 29 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x08
        _emit 0x29
        _emit 0xC2

        ; === VPMOVW2M (EVEX.F3.0F38.W1 29, pp=2) k, xmm ===
        ; 62 F2 FE 08 29 C2
        _emit 0x62
        _emit 0xF2
        _emit 0xFE
        _emit 0x08
        _emit 0x29
        _emit 0xC2

        ; === VPMOVM2D (EVEX.F3.0F38.W0 38, pp=2) zmm, k ===
        ; 62 F2 7E 08 38 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x08
        _emit 0x38
        _emit 0xC2

        ; === VPMOVM2Q (EVEX.F3.0F38.W1 38, pp=2) zmm, k ===
        ; 62 F2 FE 08 38 C2
        _emit 0x62
        _emit 0xF2
        _emit 0xFE
        _emit 0x08
        _emit 0x38
        _emit 0xC2

        ; === VPMOVD2M (EVEX.F3.0F38.W0 39, pp=2) k, xmm ===
        ; 62 F2 7E 08 39 C2
        _emit 0x62
        _emit 0xF2
        _emit 0x7E
        _emit 0x08
        _emit 0x39
        _emit 0xC2

        ; === VPMOVQ2M (EVEX.F3.0F38.W1 39, pp=2) k, xmm ===
        ; 62 F2 FE 08 39 C2
        _emit 0x62
        _emit 0xF2
        _emit 0xFE
        _emit 0x08
        _emit 0x39
        _emit 0xC2

        ; === VRANGEPS (EVEX.66.0F3A.W0 50) zmm, zmm, zmm, imm8 ===
        ; 62 F3 75 48 50 C2 01
        _emit 0x62
        _emit 0xF3  ; mm=11
        _emit 0x75  ; W=0 vvvv=1 pp=01(66)
        _emit 0x48
        _emit 0x50
        _emit 0xC2
        _emit 0x01

        ; === VRANGEPD (EVEX.66.0F3A.W1 50) zmm, zmm, zmm, imm8 ===
        ; 62 F3 F5 48 50 C2 02
        _emit 0x62
        _emit 0xF3
        _emit 0xF5  ; W=1
        _emit 0x48
        _emit 0x50
        _emit 0xC2
        _emit 0x02

        ; === VRANGESS (EVEX.66.0F3A.W0 51) xmm, xmm, xmm, imm8 ===
        ; 62 F3 75 08 51 C2 03
        _emit 0x62
        _emit 0xF3
        _emit 0x75
        _emit 0x08
        _emit 0x51
        _emit 0xC2
        _emit 0x03

        ; === VRANGESD (EVEX.66.0F3A.W1 51) xmm, xmm, xmm, imm8 ===
        ; 62 F3 F5 08 51 C2 04
        _emit 0x62
        _emit 0xF3
        _emit 0xF5
        _emit 0x08
        _emit 0x51
        _emit 0xC2
        _emit 0x04

        ; === VFPCLASSPS (EVEX.66.0F3A.W0 66) k, zmm, imm8 ===
        ; 62 F3 7D 48 66 C2 05
        _emit 0x62
        _emit 0xF3
        _emit 0x7D  ; W=0 vvvv=0 pp=01(66)
        _emit 0x48
        _emit 0x66
        _emit 0xC2
        _emit 0x05

        ; === VFPCLASSPD (EVEX.66.0F3A.W1 66) k, zmm, imm8 ===
        ; 62 F3 FD 48 66 C2 06
        _emit 0x62
        _emit 0xF3
        _emit 0xFD  ; W=1 vvvv=0 pp=01(66)
        _emit 0x48
        _emit 0x66
        _emit 0xC2
        _emit 0x06

        ; === VFPCLASSSS (EVEX.66.0F3A.W0 67) k, xmm, imm8 ===
        ; 62 F3 7D 08 67 C2 07
        _emit 0x62
        _emit 0xF3
        _emit 0x7D
        _emit 0x08
        _emit 0x67
        _emit 0xC2
        _emit 0x07

        ; === VFPCLASSSD (EVEX.66.0F3A.W1 67) k, xmm, imm8 ===
        ; 62 F3 FD 08 67 C2 08
        _emit 0x62
        _emit 0xF3
        _emit 0xFD
        _emit 0x08
        _emit 0x67
        _emit 0xC2
        _emit 0x08
    }
}

/* ---- EVEX: AVX-512 BW (integer byte/word ops) ---- */
__declspec(noinline) void test_evex_avx512bw(void)
{
    /*
     * All use EVEX.512.66.0F (P0=F1, P1=75[W=0]/F5[W=1] with vvvv=1, P2=48)
     * ModRM C2 = mod=11 reg=0 rm=2
     */
    __asm {
        ; VPUNPCKLBW zmm0, zmm1, zmm2 (60)
        ; 62 F1 75 48 60 C2
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x60
        _emit 0xC2

        ; VPUNPCKLWD zmm0, zmm1, zmm2 (61)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x61
        _emit 0xC2

        ; VPUNPCKLDQ zmm0, zmm1, zmm2 (62)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x62
        _emit 0xC2

        ; VPACKSSWB zmm0, zmm1, zmm2 (63)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x63
        _emit 0xC2

        ; VPCMPGTB zmm0, zmm1, zmm2 (64)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x64
        _emit 0xC2

        ; VPCMPGTW zmm0, zmm1, zmm2 (65)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x65
        _emit 0xC2

        ; VPACKUSWB zmm0, zmm1, zmm2 (67)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x67
        _emit 0xC2

        ; VPUNPCKHBW zmm0, zmm1, zmm2 (68)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x68
        _emit 0xC2

        ; VPUNPCKHWD zmm0, zmm1, zmm2 (69)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x69
        _emit 0xC2

        ; VPUNPCKHDQ zmm0, zmm1, zmm2 (6A)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x6A
        _emit 0xC2

        ; VPACKSSDW zmm0, zmm1, zmm2 (6B)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x6B
        _emit 0xC2

        ; VPUNPCKLQDQ zmm0, zmm1, zmm2 (6C)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x6C
        _emit 0xC2

        ; VPUNPCKHQDQ zmm0, zmm1, zmm2 (6D)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x6D
        _emit 0xC2

        ; VPSHUFD zmm0, zmm2, 01h (EVEX.512.66.0F 70, pp=01)
        ; 62 F1 7D 48 70 C2 01
        _emit 0x62
        _emit 0xF1
        _emit 0x7D  ; W=0 vvvv=0 pp=01(66)
        _emit 0x48
        _emit 0x70
        _emit 0xC2
        _emit 0x01

        ; VPSHUFHW zmm0, zmm2, 01h (EVEX.512.F3.0F 70, pp=10)
        ; 62 F1 7E 48 70 C2 01
        _emit 0x62
        _emit 0xF1
        _emit 0x7E  ; W=0 vvvv=0 pp=10(F3)
        _emit 0x48
        _emit 0x70
        _emit 0xC2
        _emit 0x01

        ; VPSHUFLW zmm0, zmm2, 01h (EVEX.512.F2.0F 70, pp=11)
        ; 62 F1 7F 48 70 C2 01
        _emit 0x62
        _emit 0xF1
        _emit 0x7F  ; W=0 vvvv=0 pp=11(F2)
        _emit 0x48
        _emit 0x70
        _emit 0xC2
        _emit 0x01

        ; VPCMPEQB zmm0, zmm1, zmm2 (74)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x74
        _emit 0xC2

        ; VPCMPEQW zmm0, zmm1, zmm2 (75)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0x75
        _emit 0xC2

        ; VPSRLW zmm0, zmm1, zmm2 (D1)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xD1
        _emit 0xC2

        ; VPSRLD zmm0, zmm1, zmm2 (D2)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xD2
        _emit 0xC2

        ; VPSRLQ zmm0, zmm1, zmm2 (D3)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xD3
        _emit 0xC2

        ; VPMULLW zmm0, zmm1, zmm2 (D5)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xD5
        _emit 0xC2

        ; VPSUBUSB zmm0, zmm1, zmm2 (D8)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xD8
        _emit 0xC2

        ; VPMINUB zmm0, zmm1, zmm2 (DA)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xDA
        _emit 0xC2

        ; VPADDUSB zmm0, zmm1, zmm2 (DC)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xDC
        _emit 0xC2

        ; VPMAXUB zmm0, zmm1, zmm2 (DE)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xDE
        _emit 0xC2

        ; VPAVGB zmm0, zmm1, zmm2 (E0)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xE0
        _emit 0xC2

        ; VPSRAW zmm0, zmm1, zmm2 (E1)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xE1
        _emit 0xC2

        ; VPMULHUW zmm0, zmm1, zmm2 (E4)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xE4
        _emit 0xC2

        ; VPMULHW zmm0, zmm1, zmm2 (E5)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xE5
        _emit 0xC2

        ; VPSUBSB zmm0, zmm1, zmm2 (E8)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xE8
        _emit 0xC2

        ; VPMINSW zmm0, zmm1, zmm2 (EA)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xEA
        _emit 0xC2

        ; VPADDSB zmm0, zmm1, zmm2 (EC)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xEC
        _emit 0xC2

        ; VPMAXSW zmm0, zmm1, zmm2 (EE)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xEE
        _emit 0xC2

        ; VPSLLW zmm0, zmm1, zmm2 (F1)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xF1
        _emit 0xC2

        ; VPSLLD zmm0, zmm1, zmm2 (F2)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xF2
        _emit 0xC2

        ; VPSLLQ zmm0, zmm1, zmm2 (F3)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xF3
        _emit 0xC2

        ; VPMADDWD zmm0, zmm1, zmm2 (F5)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xF5
        _emit 0xC2

        ; VPSADBW zmm0, zmm1, zmm2 (F6)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xF6
        _emit 0xC2

        ; VPSUBB zmm0, zmm1, zmm2 (F8)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xF8
        _emit 0xC2

        ; VPSUBW zmm0, zmm1, zmm2 (F9)
        _emit 0x62
        _emit 0xF1
        _emit 0x75
        _emit 0x48
        _emit 0xF9
        _emit 0xC2
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
    test_vmx_extended();
    test_tsx_extended();
    test_cet_extended();
    test_gfni_legacy();
    test_gfni_vaes_vex();
    test_evex_gfni_vaes_vpclmul();
    test_evex_ifma_vbmi();
    test_evex_avx512dq();
    test_evex_avx512dq_bw_new();
    test_evex_avx512bw();
    return 0;
}
