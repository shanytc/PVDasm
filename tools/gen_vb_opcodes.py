#!/usr/bin/env python3
"""
gen_vb_opcodes.py
Reads vb_opcodes.json (located one directory up from this script's location)
and generates VBOpcodeTable.h — a static C++ opcode lookup table used by
VBPCode.cpp to decode Visual Basic 5 and Visual Basic 6 PCODE instructions.

Usage:
    python3 tools/gen_vb_opcodes.py

Output:
    VBOpcodeTable.h  (written to the parent directory, i.e., the project root)

Opcode structure in the JSON:
    {
        "#": "sequential index",
        "Opcode": "hex value (2 chars = base, 4 chars = extended with prefix)",
        "Name": "instruction mnemonic",
        "Length": "total byte length (opcode byte(s) + arguments)",
        "ArgStr": "argument type codes or null",
        "SrcStr": "source representation or null",
        "Comments": "notes or null"
    }

Extended opcodes use two-byte encoding:
    Prefix 0xFB -> Lead0 table
    Prefix 0xFC -> Lead1 table
    Prefix 0xFD -> Lead2 table
    Prefix 0xFE -> Lead3 table
    Prefix 0xFF -> Lead4 table
"""

import json
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(SCRIPT_DIR)
JSON_PATH = os.path.join(PROJECT_DIR, "vb_opcodes.json")
OUTPUT_PATH = os.path.join(PROJECT_DIR, "VBOpcodeTable.h")

PREFIX_MAP = {
    "FB": "VBLead0Opcodes",  # Lead0
    "FC": "VBLead1Opcodes",  # Lead1
    "FD": "VBLead2Opcodes",  # Lead2
    "FE": "VBLead3Opcodes",  # Lead3
    "FF": "VBLead4Opcodes",  # Lead4
}

UNKNOWN_ENTRY = '{0, "???", NULL, NULL}'


def escape_c_string(s):
    """Escape a string for use in a C string literal."""
    if s is None:
        return "NULL"
    s = s.replace("\\", "\\\\")
    s = s.replace('"', '\\"')
    s = s.replace("\n", "\\n")
    s = s.replace("\r", "\\r")
    return f'"{s}"'


def parse_length(length_str):
    """Parse the Length field; return 0 for unknown/variable (-1 or non-numeric)."""
    try:
        v = int(length_str)
        return max(0, v)  # negative → 0 (unknown/variable)
    except (ValueError, TypeError):
        return 0


def build_tables(opcodes):
    """Build base table and 5 extended prefix tables from opcode list."""
    # Initialize tables: 256 slots each, filled with unknown sentinel
    base = [None] * 256
    lead0 = [None] * 256  # FB
    lead1 = [None] * 256  # FC
    lead2 = [None] * 256  # FD
    lead3 = [None] * 256  # FE
    lead4 = [None] * 256  # FF

    prefix_tables = {
        "FB": lead0,
        "FC": lead1,
        "FD": lead2,
        "FE": lead3,
        "FF": lead4,
    }

    for entry in opcodes:
        opcode_hex = entry["Opcode"].upper()
        name = entry.get("Name", "???")
        length = parse_length(entry.get("Length", "0"))
        arg_str = entry.get("ArgStr")
        src_str = entry.get("SrcStr")

        if len(opcode_hex) == 2:
            # Base opcode (single byte)
            idx = int(opcode_hex, 16)
            if 0 <= idx <= 255:
                base[idx] = (length, name, arg_str, src_str)
        elif len(opcode_hex) == 4:
            # Extended two-byte opcode: prefix + second byte
            prefix = opcode_hex[:2]
            second_byte_idx = int(opcode_hex[2:], 16)
            if prefix in prefix_tables and 0 <= second_byte_idx <= 255:
                prefix_tables[prefix][second_byte_idx] = (length, name, arg_str, src_str)

    return base, lead0, lead1, lead2, lead3, lead4


def format_entry(entry):
    """Format a single VB_OPCODE_ENTRY initializer."""
    if entry is None:
        return UNKNOWN_ENTRY
    length, name, arg_str, src_str = entry
    return f'{{{length}, {escape_c_string(name)}, {escape_c_string(arg_str)}, {escape_c_string(src_str)}}}'


def write_table(f, table_name, table, comment):
    """Write a C++ static array of VB_OPCODE_ENTRY entries."""
    f.write(f"// {comment}\n")
    f.write(f"static const VB_OPCODE_ENTRY {table_name}[256] = {{\n")
    for i, entry in enumerate(table):
        entry_str = format_entry(entry)
        comma = "," if i < 255 else ""
        f.write(f"    /* 0x{i:02X} */ {entry_str}{comma}\n")
    f.write("};\n\n")


def main():
    if not os.path.exists(JSON_PATH):
        print(f"ERROR: Cannot find {JSON_PATH}", file=sys.stderr)
        sys.exit(1)

    with open(JSON_PATH, "r", encoding="utf-8") as f:
        opcodes = json.load(f)

    print(f"Loaded {len(opcodes)} opcodes from {JSON_PATH}")

    base, lead0, lead1, lead2, lead3, lead4 = build_tables(opcodes)

    base_defined = sum(1 for e in base if e is not None)
    lead0_defined = sum(1 for e in lead0 if e is not None)
    lead1_defined = sum(1 for e in lead1 if e is not None)
    lead2_defined = sum(1 for e in lead2 if e is not None)
    lead3_defined = sum(1 for e in lead3 if e is not None)
    lead4_defined = sum(1 for e in lead4 if e is not None)
    print(f"Base:  {base_defined}/256 defined")
    print(f"Lead0 (FB): {lead0_defined}/256 defined")
    print(f"Lead1 (FC): {lead1_defined}/256 defined")
    print(f"Lead2 (FD): {lead2_defined}/256 defined")
    print(f"Lead3 (FE): {lead3_defined}/256 defined")
    print(f"Lead4 (FF): {lead4_defined}/256 defined")

    with open(OUTPUT_PATH, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("/*\n")
        f.write(" * VBOpcodeTable.h\n")
        f.write(" * AUTO-GENERATED by tools/gen_vb_opcodes.py\n")
        f.write(" * DO NOT EDIT MANUALLY — edit vb_opcodes.json and re-run the generator.\n")
        f.write(" *\n")
        f.write(" * Contains lookup tables for Visual Basic 5 and Visual Basic 6 PCODE\n")
        f.write(" * instruction decoding used by VBPCode.cpp.\n")
        f.write(" *\n")
        f.write(f" * Source: vb_opcodes.json ({len(opcodes)} opcodes total)\n")
        f.write(f" * Base opcodes:       {base_defined}/256 defined\n")
        f.write(f" * Lead0 (FB prefix):  {lead0_defined}/256 defined\n")
        f.write(f" * Lead1 (FC prefix):  {lead1_defined}/256 defined\n")
        f.write(f" * Lead2 (FD prefix):  {lead2_defined}/256 defined\n")
        f.write(f" * Lead3 (FE prefix):  {lead3_defined}/256 defined\n")
        f.write(f" * Lead4 (FF prefix):  {lead4_defined}/256 defined\n")
        f.write(" */\n\n")

        f.write("#ifndef _VB_OPCODE_TABLE_H_\n")
        f.write("#define _VB_OPCODE_TABLE_H_\n\n")

        f.write("// VB PCODE opcode table entry\n")
        f.write("struct VB_OPCODE_ENTRY {\n")
        f.write("    unsigned char  length;      // Total instruction size (opcode + args); 0 = unknown/variable\n")
        f.write("    const char*    name;         // Instruction mnemonic (e.g. \"AddI2\", \"ExitProc\")\n")
        f.write("    const char*    argStr;       // Argument type codes (NULL if no arguments)\n")
        f.write("    const char*    srcStr;       // Source representation hint (NULL if unavailable)\n")
        f.write("};\n\n")

        write_table(f, "VBBaseOpcodes", base,
                    "Base single-byte opcodes (0x00-0xFF). Extended prefixes 0xFB-0xFF redirect to Lead tables.")
        write_table(f, "VBLead0Opcodes", lead0,
                    "Lead0: two-byte opcodes with prefix 0xFB. Indexed by second byte.")
        write_table(f, "VBLead1Opcodes", lead1,
                    "Lead1: two-byte opcodes with prefix 0xFC. Indexed by second byte.")
        write_table(f, "VBLead2Opcodes", lead2,
                    "Lead2: two-byte opcodes with prefix 0xFD. Indexed by second byte.")
        write_table(f, "VBLead3Opcodes", lead3,
                    "Lead3: two-byte opcodes with prefix 0xFE. Indexed by second byte.")
        write_table(f, "VBLead4Opcodes", lead4,
                    "Lead4: two-byte opcodes with prefix 0xFF. Indexed by second byte.")

        f.write("#endif // _VB_OPCODE_TABLE_H_\n")

    print(f"Generated: {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
