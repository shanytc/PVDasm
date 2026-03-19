# PVDasm CHIP-8 Opcode Coverage

This document lists all CHIP-8 and Super CHIP-8 (SCHIP) opcodes supported by PVDasm. Every entry below has been verified against the source code in `Chip8.cpp`.

All CHIP-8 opcodes are 2 bytes (big-endian). Code execution begins at address `0x200`. Variables are named `Vx`/`Vy` where `x` and `y` are the corresponding nibbles of the opcode.

---

## Standard CHIP-8

### 0x0NNN — System / Flow Control

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x00E0 | `cls` | Clear the display |
| 0x00EE | `rts` | Return from subroutine |
| 0x00CN | `scdown N` | Scroll display down by N pixels *(SCHIP)* |
| 0x0NNN | `sys NNN` | Call machine code routine at NNN (N != 0) |

### 0x1NNN — Jump

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x1NNN | `jmp NNN` | Jump to address NNN |

### 0x2NNN — Call

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x2NNN | `jsr NNN` | Call subroutine at NNN |

### 0x3XNN — Skip If Equal (immediate)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x3XNN | `skeq Vx, NN` | Skip next instruction if Vx == NN |

### 0x4XNN — Skip If Not Equal (immediate)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x4XNN | `skne Vx, NN` | Skip next instruction if Vx != NN |

### 0x5XY0 — Skip If Equal (register)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x5XY0 | `skeq Vx, Vy` | Skip next instruction if Vx == Vy |

### 0x6XNN — Load Immediate

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x6XNN | `mov Vx, NN` | Set Vx = NN |

### 0x7XNN — Add Immediate

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x7XNN | `add Vx, NN` | Set Vx = Vx + NN (no carry flag) |

### 0x8XYn — Arithmetic / Logic

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x8XY0 | `mov Vx, Vy` | Set Vx = Vy |
| 0x8XY1 | `or Vx, Vy` | Set Vx = Vx OR Vy |
| 0x8XY2 | `and Vx, Vy` | Set Vx = Vx AND Vy |
| 0x8XY3 | `xor Vx, Vy` | Set Vx = Vx XOR Vy |
| 0x8XY4 | `add Vx, Vy` | Set Vx = Vx + Vy; VF = carry |
| 0x8XY5 | `sub Vx, Vy` | Set Vx = Vx - Vy; VF = NOT borrow |
| 0x8XY6 | `shr Vx` | Set Vx = Vx >> 1; VF = LSB before shift |
| 0x8XY7 | `rsb Vx, Vy` | Set Vx = Vy - Vx; VF = NOT borrow |
| 0x8XYE | `shl Vx` | Set Vx = Vx << 1; VF = MSB before shift |

### 0x9XY0 — Skip If Not Equal (register)

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x9XY0 | `skne Vx, Vy` | Skip next instruction if Vx != Vy |

### 0xANNN — Set Index Register

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0xANNN | `mvi NNN` | Set I = NNN |

### 0xBNNN — Jump With Offset

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0xBNNN | `jmi NNN` | Jump to address NNN + V0 |

### 0xCXNN — Random

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0xCXNN | `rand Vx, NN` | Set Vx = random byte AND NN |

### 0xDXYN — Draw Sprite

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0xDXYN | `sprite X, Y, N` | Draw N-byte sprite at (Vx, Vy); VF = collision |
| 0xDXY0 | `xsprite X, Y` | Draw 16x16 sprite at (Vx, Vy) *(SCHIP)* |

### 0xEX__ — Key Input

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0xEX9E | `skpr X` | Skip next instruction if key Vx is pressed |
| 0xEXA1 | `skup X` | Skip next instruction if key Vx is NOT pressed |

### 0xFX__ — Timers, Memory, BCD

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0xFX07 | `gdelay Vx` | Set Vx = delay timer value |
| 0xFX0A | `key Vx` | Wait for key press, store key in Vx |
| 0xFX15 | `sdelay Vx` | Set delay timer = Vx |
| 0xFX18 | `ssound Vx` | Set sound timer = Vx |
| 0xFX1E | `adi Vx` | Set I = I + Vx |
| 0xFX29 | `font Vx` | Set I = location of font sprite for digit Vx |
| 0xFX33 | `bcd Vx` | Store BCD representation of Vx at I, I+1, I+2 |
| 0xFX55 | `str V0-Vx` | Store registers V0..Vx in memory starting at I |
| 0xFX65 | `ldr V0-Vx` | Load registers V0..Vx from memory starting at I |

---

## Super CHIP-8 (SCHIP) Extensions

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x00CN | `scdown N` | Scroll display down by N pixels |
| 0x00FB | `scright` | Scroll display right by 4 pixels |
| 0x00FC | `scleft` | Scroll display left by 4 pixels |
| 0x00FD | `exit` | Exit the interpreter |
| 0x00FE | `low` | Disable high-resolution mode (64x32) |
| 0x00FF | `high` | Enable high-resolution mode (128x64) |
| 0xDXY0 | `xsprite X, Y` | Draw 16x16 sprite at (Vx, Vy) |
| 0xFX30 | `xfont Vx` | Set I = location of 10-byte font sprite for digit Vx |
| 0xFX75 | `xstr V0-Vx` | Store V0..Vx in RPL user flags (x <= 7) |
| 0xFX85 | `xldr V0-Vx` | Load V0..Vx from RPL user flags (x <= 7) |

---

## Summary

- **34 standard CHIP-8 opcodes** — full coverage
- **10 SCHIP extension opcodes** — full coverage
- All opcodes decoded; unrecognized bit patterns display as `???`
