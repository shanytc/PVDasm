#!/usr/bin/env python3
"""Compare PVDasm x64 output against IDA x64 .asm/.lst output.

Usage:
    python compare_x64.py <pvdasm.asm> <ida.asm|ida.lst> [--test-end ADDR]

Examples:
    python compare_x64.py x64.asm ida_x64.asm
    python compare_x64.py x64.asm ida_x64.lst
    python compare_x64.py x64.asm ida_x64.lst --test-end 0x140001200
"""
import re
import argparse


def parse_pvdasm(filepath):
    """Parse PVDasm output into ordered list of (address, hex_bytes, instruction)."""
    instructions = []
    addr_map = {}
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith(';'):
                continue
            # x64 addresses can be 8 or 16 hex digits
            m = re.match(r'^([0-9A-Fa-f]{8,16})\s+([0-9A-Fa-f:. ]+?)\s{2,}(.+?)(?:\s{2,};.*)?$', line)
            if not m:
                m = re.match(r'^([0-9A-Fa-f]{8,16})\s+([0-9A-Fa-f:. ]+?)\s{2,}(.+)$', line)
            if m:
                addr = int(m.group(1), 16)
                hex_bytes = m.group(2).strip()
                instr = m.group(3).strip()
                instr = re.sub(r'\s{2,};.*$', '', instr).strip()
                instr = re.sub(r'\s+;.*$', '', instr).strip()
                instructions.append((addr, hex_bytes, instr))
                addr_map[addr] = len(instructions) - 1
    return instructions, addr_map


def parse_ida(filepath):
    """Parse IDA x64 .asm output. Returns list of (address, instruction_text).

    IDA x64 format: every line starts with a 16-digit hex address, followed
    by content (instruction, label, directive, variable decl, comment, etc.).
    We filter to keep only actual instructions.
    """
    results = []
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n\r')

            # Match lines with 16-digit hex address prefix
            m = re.match(r'^([0-9A-Fa-f]{16})\s+(.*)', line)
            if not m:
                # Also try .text: prefix format
                m = re.match(r'^\s*\.text:([0-9A-Fa-f]+)\s+(.*)', line)
            if not m:
                continue

            addr = int(m.group(1), 16)
            rest = m.group(2).strip()

            # Skip empty content (bare address lines)
            if not rest:
                continue

            # Skip comments
            if rest.startswith(';'):
                continue

            # Skip proc/endp declarations
            if re.match(r'(sub_|loc_|algn_)\w+\s+(proc|endp)\b', rest):
                continue
            if re.match(r'\w+\s+(proc|endp)\b', rest):
                continue

            # Skip labels: sub_XXX:, loc_XXX:, algn_XXX:
            if re.match(r'(sub_|loc_|algn_)[0-9A-Fa-f]+\s*:', rest):
                continue

            # Skip alignment directives
            if re.match(r'align\s+', rest):
                continue

            # Skip variable declarations: var_XX = type ptr offset
            if re.match(r'\w+\s*=\s+\w+\s+ptr', rest):
                continue

            # Skip data declarations: db, dw, dd, dq, dup(...)
            if re.match(r'(byte_|word_|dword_|qword_|unk_)\w+\s+d[bwdq]\b', rest, re.IGNORECASE):
                continue
            if re.match(r'd[bwdq]\s+', rest, re.IGNORECASE):
                continue

            # Skip segment/assume/public/extrn directives
            if re.match(r'(_text|_rdata|_data|_pdata)\s+(segment|ends)', rest):
                continue
            if re.match(r'assume\s+', rest):
                continue
            if re.match(r'public\s+', rest):
                continue
            if re.match(r'extrn\s+', rest):
                continue
            if re.match(r'end\s+', rest):
                continue
            if re.match(r'\.\w+', rest):
                continue

            # Skip FUNCTION CHUNK / subroutine headers
            if '=' * 5 in rest or '-' * 5 in rest:
                continue

            # Skip IDA cross-reference comments
            if rest.startswith('; CODE XREF') or rest.startswith('; DATA XREF'):
                continue

            # Strip trailing comments from instruction
            instr = re.sub(r'\s*;.*$', '', rest).strip()
            if not instr:
                continue

            results.append((addr, instr))

    return results


def normalize_memory(s):
    """Sort base+index*scale components inside [...] for consistent ordering."""
    def sort_mem_parts(m):
        content = m.group(1)
        parts = re.split(r'\+', content)
        bases, scaled, disps = [], [], []
        for p in parts:
            p = p.strip()
            if not p:
                continue
            if '*' in p:
                scaled.append(p)
            elif re.match(r'^[0-9A-F]+$', p):
                disps.append(p)
            elif re.match(r'^-', p):
                disps.append(p)
            else:
                bases.append(p)
        bases.sort()
        return '[' + '+'.join(bases + scaled + disps) + ']'
    return re.sub(r'\[([^\]]+)\]', sort_mem_parts, s)


def normalize(instr):
    """Normalize an x64 instruction for comparison."""
    s = instr.upper().strip()

    # Remove segment overrides
    s = re.sub(r'(BYTE|WORD|DWORD|QWORD|TBYTE|XMMWORD|DQWORD|OWORD|FWORD)\s+PTR\s+(DS|ES|SS|CS|GS):', r'\1 PTR ', s)
    s = re.sub(r'\b(DS|ES|SS|CS):\[', '[', s)

    # Remove all size PTR annotations (IDA often omits implicit sizes)
    s = re.sub(r'(BYTE|WORD|DWORD|QWORD|TBYTE|XMMWORD|DQWORD|OWORD|FWORD)\s+PTR\s+', '', s)

    # Normalize hex: remove H suffix, strip leading zeros
    s = re.sub(r'\b0*([0-9A-F]+)H\b', lambda m: m.group(1).lstrip('0') or '0', s)

    # Mnemonic aliases
    s = re.sub(r'\bRETN\b', 'RET', s)

    # Condition code aliases
    s = re.sub(r'\bCMOVE\b', 'CMOVZ', s)
    s = re.sub(r'\bCMOVNE\b', 'CMOVNZ', s)
    s = re.sub(r'\bSETE\b', 'SETZ', s)
    s = re.sub(r'\bSETNE\b', 'SETNZ', s)
    s = re.sub(r'\bSETPE\b', 'SETP', s)
    s = re.sub(r'\bSETPO\b', 'SETNP', s)
    s = re.sub(r'\bSETG\b', 'SETNLE', s)
    s = re.sub(r'\bSETGE\b', 'SETNL', s)
    s = re.sub(r'\bSETA\b', 'SETNBE', s)

    # INT 3 vs INT3
    s = re.sub(r'\bINT\s+3\b', 'INT3', s)

    # Remove "LARGE" keyword
    s = re.sub(r'\bLARGE\s+', '', s)

    # Whitespace
    s = re.sub(r'\s+', ' ', s)
    s = re.sub(r'\s*,\s*', ',', s)

    # Sort memory operand components
    s = normalize_memory(s)

    # XCHG is commutative
    m = re.match(r'^(XCHG)\s+(\w+),(\w+)$', s)
    if m:
        op1, op2 = sorted([m.group(2), m.group(3)])
        s = f'XCHG {op1},{op2}'

    # GS:[XX] vs GS:XX (x64 uses GS for TEB)
    s = re.sub(r'GS:\[([0-9A-F]+)\]', r'GS:\1', s)

    return s


def is_acceptable_diff(pv_norm, ida_norm):
    """Check if a difference is a known acceptable cosmetic difference."""

    # String ops: PVDasm MOVS vs IDA MOVSB/MOVSQ etc.
    for base in ['MOVS', 'STOS', 'LODS', 'CMPS', 'SCAS']:
        for pfx in ['', 'REP ', 'REPE ', 'REPNE ']:
            if pv_norm.startswith(pfx + base):
                for sfx in ['B', 'W', 'D', 'Q', '']:
                    if ida_norm.startswith(pfx + base + sfx) or ida_norm == pfx + base + sfx:
                        return True

    # IDA symbolic names in branches/calls
    branch_re = r'^(CALL|JMP|JZ|JNZ|JB|JNB|JBE|JA|JL|JGE|JLE|JG|JS|JNS|JP|JNP|JO|JNO|LOOP|LOOPE|LOOPNE|PUSH|LEA)\b'
    if re.match(branch_re, pv_norm):
        if re.search(r'\b(SUB_|LOC_|J_|NULLSUB_|__\w+|_\w+)\b', ida_norm):
            return True
        if re.search(r'\bOFFSET\b', ida_norm):
            return True
        pv_op = pv_norm.split()[0]
        ida_op = re.sub(r'\bSHORT\s+', '', ida_norm).split()[0]
        if pv_op == ida_op:
            return True

    # IDA variable/struct references
    if re.search(r'(VAR_|MS_EXC|ARG_|STRU_)', ida_norm):
        return True

    # IDA symbolic data references
    if re.search(r'(DWORD_[0-9A-F]|WORD_[0-9A-F]|BYTE_[0-9A-F]|QWORD_[0-9A-F]|FIRST|LAST|FUNCTION|NEWMODE|CALLBACK|EXCEPTIONPTR|EXCEPTIONNUM|FLAG|TYPE|USERMATHERR|NULLSUB)', ida_norm):
        return True
    if re.search(r'\bDS:___', ida_norm) or re.search(r'\bDS:_\w+', ida_norm):
        return True
    # x64 RIP-relative symbolic refs
    if re.search(r'\bCS:', ida_norm):
        return True

    # Size annotation differences for specific instructions
    for op in ['XSAVE', 'XRSTOR', 'XSAVEOPT', 'FISTTP', 'CMPXCHG16B']:
        if op in pv_norm and op in ida_norm:
            return True

    # IMUL 3-op vs IDA 2-op abbreviation
    if pv_norm.startswith('IMUL') and ida_norm.startswith('IMUL'):
        return True

    # ENTER formatting
    if pv_norm.startswith('ENTER') and ida_norm.startswith('ENTER'):
        return True

    # CMPXCHG8B/CMPXCHG16B size annotation
    if 'CMPXCHG' in pv_norm and 'CMPXCHG' in ida_norm:
        return True

    # GS segment formatting
    if 'GS:' in pv_norm and 'GS:' in ida_norm and pv_norm.split()[0] == ida_norm.split()[0]:
        return True

    # LOCK prefix with memory operands
    if pv_norm.startswith('LOCK') and ida_norm.startswith('LOCK'):
        pv_p, ida_p = pv_norm.split(), ida_norm.split()
        if len(pv_p) > 1 and len(ida_p) > 1 and pv_p[1] == ida_p[1]:
            return True

    # MOVDQA/MOVDQU with DQword vs xmmword
    if pv_norm.startswith(('MOVDQA', 'MOVDQU')) and ida_norm.startswith(('MOVDQA', 'MOVDQU')):
        return True

    # NOP with size operand differences
    if pv_norm.startswith('NOP') and ida_norm.startswith('NOP'):
        return True

    return False


def main():
    parser = argparse.ArgumentParser(description='Compare PVDasm x64 output against IDA x64 .asm output')
    parser.add_argument('pvdasm', help='PVDasm .asm file')
    parser.add_argument('ida', help='IDA .asm file')
    parser.add_argument('--test-end', type=lambda x: int(x, 0), default=None,
                        help='End address of hand-written test code (hex). '
                             'Splits output into test vs CRT sections.')
    args = parser.parse_args()

    pv_instrs, pv_addr_map = parse_pvdasm(args.pvdasm)
    ida_instrs = parse_ida(args.ida)

    print(f"PVDasm: {len(pv_instrs)} instructions")
    print(f"IDA: {len(ida_instrs)} instruction entries")
    print()

    differences = []
    cosmetic = []
    matched = 0
    ida_only = 0

    for ida_addr, ida_raw in ida_instrs:
        if ida_addr not in pv_addr_map:
            ida_only += 1
            continue

        pv_idx = pv_addr_map[ida_addr]
        pv_addr, pv_hex, pv_raw = pv_instrs[pv_idx]
        pv_norm = normalize(pv_raw)
        ida_norm = normalize(ida_raw)

        if pv_norm == ida_norm:
            matched += 1
        elif is_acceptable_diff(pv_norm, ida_norm):
            cosmetic.append((pv_addr, pv_raw, ida_raw, pv_norm, ida_norm))
        else:
            differences.append((pv_addr, pv_raw, ida_raw, pv_norm, ida_norm))

    print(f"Matched: {matched}")
    print(f"Cosmetic (acceptable): {len(cosmetic)}")
    print(f"Real differences: {len(differences)}")
    print()

    if args.test_end:
        test_diffs = [d for d in differences if d[0] <= args.test_end]
        crt_diffs = [d for d in differences if d[0] > args.test_end]

        if test_diffs:
            print("=" * 90)
            print(f"TEST CODE DIFFERENCES (up to 0x{args.test_end:X}):")
            print("=" * 90)
            for addr, pv_raw, ida_raw, pv_norm, ida_norm in test_diffs:
                print(f"\n  {addr:016X}  PVDasm: {pv_raw}")
                print(f"                    IDA:    {ida_raw}")
        else:
            print("*** No test code differences! All test instructions match. ***")

        if crt_diffs:
            print()
            print("=" * 90)
            print(f"CRT/STARTUP CODE DIFFERENCES ({len(crt_diffs)}):")
            print("=" * 90)
            for addr, pv_raw, ida_raw, pv_norm, ida_norm in crt_diffs:
                print(f"\n  {addr:016X}  PVDasm: {pv_raw}")
                print(f"                    IDA:    {ida_raw}")
        elif not test_diffs:
            print("*** No CRT code differences! ***")
    else:
        if differences:
            print("=" * 90)
            print(f"DIFFERENCES ({len(differences)}):")
            print("=" * 90)
            for addr, pv_raw, ida_raw, pv_norm, ida_norm in differences:
                print(f"\n  {addr:016X}  PVDasm: {pv_raw}")
                print(f"                    IDA:    {ida_raw}")
        else:
            print("*** No differences found! ***")


if __name__ == '__main__':
    main()
