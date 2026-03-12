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

int main(void)
{
    test_data_transfer();
    test_arithmetic();
    test_bitwise();
    test_control_flow();
    test_string_ops();
    test_misc();
    return 0;
}
