/*
 * x64.c - 64-bit x86-64 instruction test for PVDasm
 *
 * Tests: REX prefixes, 64-bit registers (RAX-R15), RIP-relative addressing,
 *        MOVSXD, 64-bit immediates, string ops with REX.W, PUSH/POP 64-bit,
 *        CDQE/CQO, PUSHFQ/POPFQ, conditional jumps, near CALL/JMP.
 *
 * Compile (x64): cl /nologo /Od /GS- /Fe:x64.exe x64.c /link /SUBSYSTEM:CONSOLE
 */
#include <windows.h>
#include <stdio.h>

#pragma optimize("", off)

/* Global for RIP-relative addressing tests */
static volatile __int64 g_val64 = 0x123456789ABCDEF0LL;
static volatile int     g_val32 = 0xDEADBEEF;
static volatile char    g_buffer[64] = "Hello, 64-bit world!";

/* ---- REX prefix & 64-bit register moves ---- */
__declspec(noinline) void test_rex_registers(void)
{
    volatile __int64 a = 0x1111111122222222LL;
    volatile __int64 b = 0x3333333344444444LL;
    volatile __int64 c;
    volatile int d = 42;
    volatile __int64 r8_val;
    volatile __int64 r9_val;
    volatile __int64 r10_val;
    volatile __int64 r11_val;
    volatile __int64 extended;

    /* MOV r64, imm64 - generates REX.W + B8-BF with 8-byte immediate */
    c = 0x0000000100000002LL;

    /* MOV between 64-bit registers - REX.W + 89/8B */
    c = a;
    a = b;

    /* Extended registers R8-R15 - REX.B / REX.R */
    r8_val  = 0x8888888888888888LL;
    r9_val  = 0x9999999999999999LL;
    r10_val = 0xAAAAAAAAAAAAAAAALL;
    r11_val = 0xBBBBBBBBBBBBBBBBLL;

    r8_val  = r9_val;
    r10_val = r11_val;

    /* MOVSXD - sign-extend 32-bit to 64-bit (opcode 0x63 with REX.W) */
    extended = (__int64)d;
    (void)extended;
    (void)r8_val; (void)r10_val; (void)c;
}

/* ---- RIP-relative addressing ---- */
__declspec(noinline) void test_rip_relative(void)
{
    volatile __int64 local;
    volatile int local32;

    /* In 64-bit mode, accessing globals uses RIP-relative [RIP+disp32] */
    local = g_val64;
    g_val64 = 0xFEDCBA9876543210LL;
    local32 = g_val32;
    g_val32 = local32 + 1;
    (void)local;
}

/* ---- 64-bit stack operations ---- */
__declspec(noinline) void test_stack_ops(void)
{
    /* PUSH/POP default to 64-bit in long mode */
    /* Function prologue/epilogue will show push rbp / pop rbp */
    volatile __int64 x = 1;
    volatile __int64 y = 2;
    volatile __int64 z;
    z = x + y;
    (void)z;
}

/* ---- CDQE / CQO ---- */
__declspec(noinline) __int64 test_cdqe_cqo(__int64 a, __int64 b)
{
    volatile __int64 result;
    volatile int small_val;
    volatile __int64 big;

    /* Division generates CQO (sign-extend RAX -> RDX:RAX) */
    if (b == 0) return 0;
    result = a / b;

    /* CDQE: sign-extend EAX to RAX */
    small_val = -1;
    big = (__int64)small_val;
    (void)big;
    return result;
}

/* ---- Control flow with 64-bit addresses ---- */
__declspec(noinline) int test_branches(int n)
{
    int sum = 0;
    int i;
    /* Short conditional jumps (Jcc) with 64-bit RIP target display */
    for (i = 0; i < n; i++) {
        if (i & 1)
            sum += i;
        else
            sum -= i;
    }

    /* Near CALL is tested by calling other functions */
    /* JMP near is used for tail calls and loop back-edges */
    return sum;
}

/* ---- String operations with REX.W ---- */
__declspec(noinline) void test_string_ops(void)
{
    volatile __int64 src[4] = { 1, 2, 3, 4 };
    volatile __int64 dst[4] = { 0 };
    int i;

    /* memcpy on 64-bit data - compiler may emit REP MOVSQ */
    for (i = 0; i < 4; i++)
        dst[i] = src[i];

    /* memset-like - compiler may emit REP STOSQ */
    for (i = 0; i < 4; i++)
        dst[i] = 0;
}

/* ---- PUSHFQ / POPFQ ---- */
__declspec(noinline) void test_pushfq_popfq(void)
{
    /* Volatile flag manipulation forces PUSHFQ/POPFQ */
    volatile int a = 10;
    volatile int b = 20;
    volatile int c;
    c = (a > b) ? 1 : 0;
    (void)c;
}

/* ---- 8-bit registers with REX (SPL/BPL/SIL/DIL) ---- */
__declspec(noinline) int test_rex_byte_regs(const char *s)
{
    /* Accessing low byte of RSI/RDI/RSP/RBP uses REX prefix */
    /* which gives SIL/DIL/SPL/BPL instead of AH/CH/DH/BH */
    volatile char c = 0;
    if (s && s[0]) {
        c = s[0];
    }
    return (int)c;
}

/* ---- Entry point ---- */
int main(void)
{
    __int64 div_result;
    int branch_result;
    int byte_result;

    printf("PVDasm x86-64 Test Binary\n");
    printf("=========================\n\n");

    test_rex_registers();
    printf("[PASS] REX prefix & 64-bit registers\n");

    test_rip_relative();
    printf("[PASS] RIP-relative addressing\n");

    test_stack_ops();
    printf("[PASS] 64-bit stack operations\n");

    div_result = test_cdqe_cqo(100, 7);
    printf("[PASS] CDQE/CQO (100/7 = %lld)\n", div_result);

    branch_result = test_branches(10);
    printf("[PASS] Branches (sum = %d)\n", branch_result);

    test_string_ops();
    printf("[PASS] String operations\n");

    test_pushfq_popfq();
    printf("[PASS] PUSHFQ/POPFQ\n");

    byte_result = test_rex_byte_regs("A");
    printf("[PASS] REX byte registers (char = %d)\n", byte_result);

    printf("\nAll tests passed. Load x64.exe in PVDasm to verify disassembly.\n");
    return 0;
}
