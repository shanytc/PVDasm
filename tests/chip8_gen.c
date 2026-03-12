/*
 * chip8_gen.c - Generates Chip8/SCHIP test ROM for PVDasm
 *
 * Writes chip8.bin containing all standard Chip8 and SCHIP
 * instruction opcodes. Load the resulting chip8.bin into
 * PVDasm (as Chip8 binary) to test Chip8 disassembly.
 *
 * Chip8 instructions are 2 bytes, big-endian.
 * Programs normally load at address 0x200.
 *
 * Compile: cl /nologo /Fe:chip8_gen.exe chip8_gen.c /link /SUBSYSTEM:CONSOLE
 * Run:     chip8_gen.exe   (produces chip8.bin)
 */
#include <stdio.h>
#include <string.h>

static unsigned char rom[4096];
static int pos = 0;

static void emit16(unsigned short op)
{
    rom[pos++] = (unsigned char)(op >> 8);
    rom[pos++] = (unsigned char)(op & 0xFF);
}

int main(void)
{
    FILE *fp;

    memset(rom, 0, sizeof(rom));

    /* ======== Standard Chip8 Opcodes ======== */

    /* 00E0: CLS - clear the display */
    emit16(0x00E0);

    /* 00EE: RET - return from subroutine */
    emit16(0x00EE);

    /* 0NNN: SYS addr - call RCA 1802 (usually ignored) */
    emit16(0x0234);

    /* 1NNN: JP addr - jump to address */
    emit16(0x1400);
    emit16(0x1ABC);

    /* 2NNN: CALL addr - call subroutine */
    emit16(0x2400);
    emit16(0x2DEF);

    /* 3XNN: SE Vx, byte - skip if Vx == NN */
    emit16(0x3042);
    emit16(0x35FF);
    emit16(0x3A00);

    /* 4XNN: SNE Vx, byte - skip if Vx != NN */
    emit16(0x4110);
    emit16(0x4B80);

    /* 5XY0: SE Vx, Vy - skip if Vx == Vy */
    emit16(0x5010);
    emit16(0x5AB0);

    /* 6XNN: LD Vx, byte - load immediate */
    emit16(0x60FF);
    emit16(0x6100);
    emit16(0x652A);
    emit16(0x6F42);

    /* 7XNN: ADD Vx, byte - add immediate (no carry flag) */
    emit16(0x7001);
    emit16(0x7310);
    emit16(0x7AFF);

    /* 8XY0: LD Vx, Vy - register copy */
    emit16(0x8010);
    emit16(0x8A50);

    /* 8XY1: OR Vx, Vy */
    emit16(0x8011);
    emit16(0x8231);

    /* 8XY2: AND Vx, Vy */
    emit16(0x8012);
    emit16(0x8342);

    /* 8XY3: XOR Vx, Vy */
    emit16(0x8013);

    /* 8XY4: ADD Vx, Vy (VF = carry) */
    emit16(0x8014);
    emit16(0x8564);

    /* 8XY5: SUB Vx, Vy (VF = NOT borrow) */
    emit16(0x8015);

    /* 8XY6: SHR Vx {, Vy} (VF = LSB before shift) */
    emit16(0x8016);
    emit16(0x8506);

    /* 8XY7: SUBN Vx, Vy (Vx = Vy - Vx, VF = NOT borrow) */
    emit16(0x8017);

    /* 8XYE: SHL Vx {, Vy} (VF = MSB before shift) */
    emit16(0x801E);
    emit16(0x850E);

    /* 9XY0: SNE Vx, Vy - skip if Vx != Vy */
    emit16(0x9010);
    emit16(0x9AB0);

    /* ANNN: LD I, addr - set index register */
    emit16(0xA300);
    emit16(0xAFFF);

    /* BNNN: JP V0, addr - jump to V0 + NNN */
    emit16(0xB400);
    emit16(0xB200);

    /* CXNN: RND Vx, byte - Vx = random AND NN */
    emit16(0xC0FF);
    emit16(0xC50F);

    /* DXYN: DRW Vx, Vy, N - draw N-byte sprite at (Vx,Vy) */
    emit16(0xD015);     /* V0, V1, 5 rows */
    emit16(0xD238);     /* V2, V3, 8 rows */
    emit16(0xDABF);     /* VA, VB, 15 rows */
    emit16(0xD451);     /* V4, V5, 1 row */

    /* EX9E: SKP Vx - skip if key Vx is pressed */
    emit16(0xE09E);
    emit16(0xE59E);

    /* EXA1: SKNP Vx - skip if key Vx is NOT pressed */
    emit16(0xE0A1);
    emit16(0xEBA1);

    /* FX07: LD Vx, DT - read delay timer */
    emit16(0xF007);
    emit16(0xFA07);

    /* FX0A: LD Vx, K - wait for key press */
    emit16(0xF00A);
    emit16(0xF50A);

    /* FX15: LD DT, Vx - set delay timer */
    emit16(0xF015);
    emit16(0xF315);

    /* FX18: LD ST, Vx - set sound timer */
    emit16(0xF018);
    emit16(0xF718);

    /* FX1E: ADD I, Vx - I += Vx */
    emit16(0xF01E);
    emit16(0xFA1E);

    /* FX29: LD F, Vx - I = font sprite addr for digit Vx */
    emit16(0xF029);
    emit16(0xF929);

    /* FX33: LD B, Vx - store BCD of Vx at I, I+1, I+2 */
    emit16(0xF033);
    emit16(0xF533);

    /* FX55: LD [I], Vx - store V0..Vx at memory[I..] */
    emit16(0xF055);     /* store V0 only */
    emit16(0xF555);     /* store V0-V5 */
    emit16(0xFF55);     /* store V0-VF (all) */

    /* FX65: LD Vx, [I] - load V0..Vx from memory[I..] */
    emit16(0xF065);     /* load V0 only */
    emit16(0xF565);     /* load V0-V5 */
    emit16(0xFF65);     /* load V0-VF (all) */

    /* ======== SCHIP (Super Chip-48) Extensions ======== */

    /* 00CN: SCD N - scroll display down N lines */
    emit16(0x00C1);
    emit16(0x00C4);
    emit16(0x00C8);
    emit16(0x00CF);

    /* 00FB: SCR - scroll display right 4 pixels */
    emit16(0x00FB);

    /* 00FC: SCL - scroll display left 4 pixels */
    emit16(0x00FC);

    /* 00FD: EXIT - exit interpreter */
    emit16(0x00FD);

    /* 00FE: LOW - disable extended screen mode */
    emit16(0x00FE);

    /* 00FF: HIGH - enable extended screen mode (128x64) */
    emit16(0x00FF);

    /* DXY0: DRW Vx, Vy, 0 - draw 16x16 sprite (SCHIP) */
    emit16(0xD010);     /* V0, V1, 16x16 */
    emit16(0xD230);     /* V2, V3, 16x16 */

    /* FX30: LD HF, Vx - I = high-res font addr for digit Vx */
    emit16(0xF030);
    emit16(0xF930);

    /* FX75: LD R, Vx - store V0..Vx in RPL user flags (X<=7) */
    emit16(0xF075);
    emit16(0xF775);

    /* FX85: LD Vx, R - load V0..Vx from RPL user flags (X<=7) */
    emit16(0xF085);
    emit16(0xF785);

    /* --- Write ROM file --- */
    fp = fopen("chip8.bin", "wb");
    if (!fp) {
        printf("Error: cannot create chip8.bin\n");
        return 1;
    }
    fwrite(rom, 1, pos, fp);
    fclose(fp);

    printf("chip8.bin written (%d bytes, %d instructions)\n", pos, pos / 2);
    return 0;
}
