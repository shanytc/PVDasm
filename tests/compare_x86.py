#!/usr/bin/env python3
"""Compare PVDasm x86 output against IDA x86 .asm/.lst output.

Usage:
    python compare_x86.py <pvdasm.asm> <ida.asm|ida.lst> [--test-end ADDR]

Examples:
    python compare_x86.py x86.asm ida_x86.asm
    python compare_x86.py x86.asm ida_x86.lst
    python compare_x86.py x86.asm ida_x86.lst --test-end 0x40142C
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
            m = re.match(r'^([0-9A-Fa-f]{8})\s+([0-9A-Fa-f:. ]+?)\s{2,}(.+?)(?:\s{2,};.*)?$', line)
            if not m:
                m = re.match(r'^([0-9A-Fa-f]{8})\s+([0-9A-Fa-f:. ]+?)\s{2,}(.+)$', line)
            if m:
                addr = int(m.group(1), 16)
                hex_bytes = m.group(2).strip()
                instr = m.group(3).strip()
                instr = re.sub(r'\s{2,};.*$', '', instr).strip()
                instr = re.sub(r'\s+;.*$', '', instr).strip()
                instructions.append((addr, hex_bytes, instr))
                addr_map[addr] = len(instructions) - 1
    return instructions, addr_map


def detect_lst_format(filepath):
    """Detect if the file is IDA .lst format (lines start with .text:XXXXXXXX)."""
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            if re.match(r'^\s*\.text:[0-9A-Fa-f]+\s', line):
                return True
            # Check enough lines to be sure
            if line.strip() and not line.strip().startswith(';') and not line.strip().startswith('.text'):
                return False
    return False


def parse_ida_lst(filepath):
    """Parse IDA .lst output. Returns list of (address, instruction_text).

    IDA .lst format has .text:XXXXXXXX prefix on every line, giving precise
    address for each instruction.
    """
    results = []
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n\r')

            # Match .text:XXXXXXXX prefix
            m = re.match(r'^\s*\.text:([0-9A-Fa-f]+)\s+(.*)', line)
            if not m:
                continue

            addr = int(m.group(1), 16)
            rest = m.group(2).strip()

            # Skip empty content
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

            # Skip labels
            if re.match(r'(sub_|loc_|algn_)[0-9A-Fa-f]+\s*:', rest):
                continue

            # Skip alignment directives
            if re.match(r'align\s+', rest):
                continue

            # Skip variable declarations
            if re.match(r'\w+\s*=\s+\w+\s+ptr', rest):
                continue

            # Skip data declarations
            if re.match(r'(byte_|word_|dword_|qword_|unk_)\w+\s+d[bwdq]\b', rest, re.IGNORECASE):
                continue
            if re.match(r'd[bwdq]\s+', rest, re.IGNORECASE):
                continue

            # Skip segment/assume/public/extrn/end directives
            if re.match(r'(_text|_rdata|_data)\s+(segment|ends)', rest):
                continue
            if re.match(r'(assume|public|extrn|end)\s+', rest):
                continue
            if re.match(r'\.\w+', rest):
                continue

            # Skip function chunk markers
            if '=' * 5 in rest or '-' * 5 in rest:
                continue

            # Strip trailing comments from instruction
            instr = re.sub(r'\s*;.*$', '', rest).strip()
            if not instr:
                continue

            results.append((addr, instr))

    return results


def parse_ida(filepath):
    """Parse IDA .asm output. Returns list of (address_or_None, instruction_text).

    IDA x86 .asm output uses labels (sub_XXXXXX, loc_XXXXXX) instead of inline
    addresses. These labels serve as sync points for matching against PVDasm.

    Also supports .lst format (auto-detected) where every line has an address.
    """
    # Auto-detect .lst format
    if detect_lst_format(filepath):
        return parse_ida_lst(filepath)

    results = []
    current_addr = None
    with open(filepath, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.rstrip('\n\r')
            m = re.match(r'^\s*sub_([0-9A-Fa-f]+)\s+proc\b', line)
            if m:
                current_addr = int(m.group(1), 16)
                continue
            m = re.match(r'^\s*(loc|sub)_([0-9A-Fa-f]+)\s*:', line)
            if m:
                current_addr = int(m.group(2), 16)
                continue
            stripped = line.strip()
            if not stripped or stripped.startswith(';'):
                continue
            if re.match(r'sub_[0-9A-Fa-f]+\s+endp', stripped):
                continue
            if re.match(r'align\s+', stripped):
                continue
            if re.match(r'\w+=\s+\w+\s+ptr', stripped):
                continue
            if re.match(r'\.\w+', stripped):
                continue
            if re.match(r'_text\s+(segment|ends)', stripped):
                continue
            if re.match(r'assume\s+', stripped):
                continue
            if re.match(r'end\s+', stripped):
                continue
            if re.match(r'public\s+', stripped):
                continue
            if re.match(r'extrn\s+', stripped):
                continue
            if re.match(r'd[dbwq]\s+', stripped, re.IGNORECASE):
                continue
            if 'FUNCTION CHUNK' in stripped or 'START OF FUNCTION' in stripped or 'END OF FUNCTION' in stripped:
                continue
            if stripped.startswith('; __') or re.match(r'; starts at', stripped):
                continue
            instr = re.sub(r'\s*;.*$', '', stripped).strip()
            if not instr:
                continue
            results.append((current_addr, instr))
            current_addr = None
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
    """Normalize an x86 instruction for comparison."""
    s = instr.upper().strip()

    # Remove segment overrides
    s = re.sub(r'(BYTE|WORD|DWORD|QWORD|TBYTE|XMMWORD|DQWORD|OWORD|FWORD)\s+PTR\s+(DS|ES|SS|CS):', r'\1 PTR ', s)
    s = re.sub(r'\b(DS|ES|SS|CS):\[', '[', s)

    # Remove all size PTR annotations (IDA often omits implicit sizes)
    s = re.sub(r'(BYTE|WORD|DWORD|QWORD|TBYTE|XMMWORD|DQWORD|OWORD|FWORD)\s+PTR\s+', '', s)

    # Normalize hex: remove H suffix, strip leading zeros
    s = re.sub(r'\b0*([0-9A-F]+)H\b', lambda m: m.group(1).lstrip('0') or '0', s)

    # Mnemonic aliases
    s = re.sub(r'\bRETN\b', 'RET', s)
    s = re.sub(r'\bPUSHA\b', 'PUSHAD', s)
    s = re.sub(r'\bPOPA\b', 'POPAD', s)
    s = re.sub(r'\bPUSHF\b', 'PUSHFD', s)
    s = re.sub(r'\bPOPF\b', 'POPFD', s)

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

    # FS:[0] vs FS:0
    s = re.sub(r'FS:\[([0-9A-F]+)\]', r'FS:\1', s)

    return s


def is_acceptable_diff(pv_norm, ida_norm):
    """Check if a difference is a known acceptable cosmetic difference."""

    # String ops: PVDasm MOVS vs IDA MOVSB/MOVSD etc.
    for base in ['MOVS', 'STOS', 'LODS', 'CMPS', 'SCAS']:
        for pfx in ['', 'REP ', 'REPE ', 'REPNE ']:
            if pv_norm.startswith(pfx + base):
                for sfx in ['B', 'W', 'D', '']:
                    if ida_norm.startswith(pfx + base + sfx) or ida_norm == pfx + base + sfx:
                        return True

    # IDA symbolic names in branches/calls
    branch_re = r'^(CALL|JMP|JZ|JNZ|JB|JNB|JBE|JA|JL|JGE|JLE|JG|JS|JNS|JP|JNP|JO|JNO|LOOP|LOOPE|LOOPNE|PUSH)\b'
    if re.match(branch_re, pv_norm):
        if re.search(r'\b(SUB_|LOC_|J_|NULLSUB_|__SEH_|__P_|_\w+)\b', ida_norm):
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

    # Size annotation differences for specific instructions
    for op in ['XSAVE', 'XRSTOR', 'XSAVEOPT', 'FISTTP']:
        if op in pv_norm and op in ida_norm:
            return True

    # IMUL 3-op vs IDA 2-op abbreviation
    if pv_norm.startswith('IMUL') and ida_norm.startswith('IMUL'):
        return True

    # ENTER formatting
    if pv_norm.startswith('ENTER') and ida_norm.startswith('ENTER'):
        return True

    # CMPXCHG8B size annotation
    if 'CMPXCHG8B' in pv_norm and 'CMPXCHG8B' in ida_norm:
        return True

    # FS segment formatting
    if 'FS:' in pv_norm and 'FS:' in ida_norm and pv_norm.split()[0] == ida_norm.split()[0]:
        return True

    # LOCK prefix with memory operands
    if pv_norm.startswith('LOCK') and ida_norm.startswith('LOCK'):
        pv_p, ida_p = pv_norm.split(), ida_norm.split()
        if len(pv_p) > 1 and len(ida_p) > 1 and pv_p[1] == ida_p[1]:
            return True

    # XBEGIN target formatting: PVDasm shows raw displacement, IDA shows resolved target
    if pv_norm.startswith('XBEGIN') and ida_norm.startswith('XBEGIN'):
        return True

    # IDA symbolic data references: offset unk_XXXXX, ___security_cookie, Table._end, etc.
    if re.search(r'\bOFFSET\b', ida_norm):
        return True
    if re.search(r'___\w+', ida_norm):
        return True
    if re.search(r'TABLE\.\w+', ida_norm):
        return True
    if re.search(r'UNK_[0-9A-F]', ida_norm):
        return True

    # IDA named variables in memory operands: [EBP+Code], [EBP+SystemTimeAsFileTime.dwLowDateTime], etc.
    # Note: normalize_memory() sorts components, so [EBP+CODE] becomes [CODE+EBP]
    if re.search(r'[A-Z_]{2,}', ida_norm) and re.search(r'\[.*EBP.*\]', ida_norm) and re.search(r'\[.*EBP.*\]', pv_norm):
        if pv_norm.split()[0] == ida_norm.split()[0]:
            return True

    # IDA stack variable references with compound names: [ESP+32Ch+Size]
    if re.search(r'\[ESP\+\w+\+\w+\]', ida_norm) and re.search(r'\[ESP\]', pv_norm):
        if pv_norm.split()[0] == ida_norm.split()[0]:
            return True

    # Signed negative vs unsigned hex: -01H vs 0FFFFFFFFH, -02H vs 0FFFFFFFEH
    if pv_norm.split()[0] == ida_norm.split()[0]:
        pv_clean = re.sub(r'-([0-9A-F]+)\b', lambda m: hex((-int(m.group(1),16)) & 0xFFFFFFFF)[2:].upper(), pv_norm)
        if pv_clean == ida_norm:
            return True

    # XCHG operand order (commutative) with memory operand
    if pv_norm.startswith('XCHG') and ida_norm.startswith('XCHG'):
        return True

    # IDA DS:ADDR vs PVDasm [ADDR] with size PTR (both normalized away size PTR)
    if re.search(r'\bDS:[0-9A-F]+H?\b', ida_norm) and pv_norm.split()[0] == ida_norm.split()[0]:
        return True

    # LEA with IDA named variable
    if pv_norm.startswith('LEA') and ida_norm.startswith('LEA'):
        if re.search(r'\[EBP\+\w{3,}\]', ida_norm):
            return True

    return False


def main():
    parser = argparse.ArgumentParser(description='Compare PVDasm x86 output against IDA x86 .asm output')
    parser.add_argument('pvdasm', help='PVDasm .asm file')
    parser.add_argument('ida', help='IDA .asm file')
    parser.add_argument('--test-end', type=lambda x: int(x, 0), default=None,
                        help='End address of hand-written test code (hex). '
                             'Splits output into test vs CRT sections.')
    args = parser.parse_args()

    pv_instrs, pv_addr_map = parse_pvdasm(args.pvdasm)
    ida_instrs = parse_ida(args.ida)

    # Check if all IDA entries have addresses (lst format = address-based matching)
    all_have_addr = all(a is not None for a, _ in ida_instrs)
    addr_anchors = sum(1 for a, _ in ida_instrs if a is not None)

    print(f"PVDasm: {len(pv_instrs)} instructions")
    if all_have_addr:
        print(f"IDA: {len(ida_instrs)} entries (all with addresses, .lst format)")
    else:
        print(f"IDA: {len(ida_instrs)} entries ({addr_anchors} with address anchors)")
    print()

    differences = []
    cosmetic = []
    matched = 0
    ida_only = 0

    if all_have_addr:
        # Address-based matching (more precise, used for .lst format)
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
    else:
        # Sequential matching with label sync points (used for .asm format)
        pv_idx = 0
        for ida_entry_addr, ida_raw in ida_instrs:
            if ida_entry_addr is not None:
                if ida_entry_addr in pv_addr_map:
                    pv_idx = pv_addr_map[ida_entry_addr]
                else:
                    continue
            if pv_idx >= len(pv_instrs):
                break

            pv_addr, pv_hex, pv_raw = pv_instrs[pv_idx]
            pv_norm = normalize(pv_raw)
            ida_norm = normalize(ida_raw)

            if pv_norm == ida_norm:
                matched += 1
            elif is_acceptable_diff(pv_norm, ida_norm):
                cosmetic.append((pv_addr, pv_raw, ida_raw, pv_norm, ida_norm))
            else:
                differences.append((pv_addr, pv_raw, ida_raw, pv_norm, ida_norm))

            pv_idx += 1

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
                print(f"\n  {addr:08X}  PVDasm: {pv_raw}")
                print(f"            IDA:    {ida_raw}")
        else:
            print("*** No test code differences! All test instructions match. ***")

        if crt_diffs:
            print()
            print("=" * 90)
            print(f"CRT/STARTUP CODE DIFFERENCES ({len(crt_diffs)}):")
            print("=" * 90)
            for addr, pv_raw, ida_raw, pv_norm, ida_norm in crt_diffs:
                print(f"\n  {addr:08X}  PVDasm: {pv_raw}")
                print(f"            IDA:    {ida_raw}")
        elif not test_diffs:
            print("*** No CRT code differences! ***")
    else:
        if differences:
            print("=" * 90)
            print(f"DIFFERENCES ({len(differences)}):")
            print("=" * 90)
            for addr, pv_raw, ida_raw, pv_norm, ida_norm in differences:
                print(f"\n  {addr:08X}  PVDasm: {pv_raw}")
                print(f"            IDA:    {ida_raw}")
        else:
            print("*** No differences found! ***")


if __name__ == '__main__':
    main()
