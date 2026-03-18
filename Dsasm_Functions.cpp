
/*
     8888888b.                  888     888 d8b
     888   Y88b                 888     888 Y8P
     888    888                 888     888
     888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888
     8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888
     888        888     888  888  Y88o88P   888 88888888 888  888  888
     888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P
     888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"


             PE Editor & Disassembler & File Identifier
             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 Written by Shany Golan.
 In January, 2003.
 I have investigated P.E / Opcodes file(s) format as thoroughly as possible,
 But I cannot claim that I am an expert yet, so some of its information  
 May give you wrong results.

 Language used: Visual C++ 6.0
 Date of creation: July 06, 2002

 Date of first release: unknown ??, 2003

 You can contact me: e-mail address: shanytc@yahoo.com

 Copyright (C) 2011. By Shany Golan.

 Permission is granted to make and distribute verbatim copies of this
 Program provided the copyright notice and this permission notice are
 Preserved on all copies.


 File: Disasm_Functions.cpp (main)
   This program was written by Shany Golan, Student at :
			Ruppin, department of computer science and engineering University.
*/


// ================================================================
// =========================  INCLUDES  ===========================
// ================================================================

#include "Disasm.h"
#include "Functions.h"

// ================================================================
// =====================  EXTERNAL VARIABLES  =====================
// ================================================================

#ifndef _EXTERNAL_
#define _EXTERNAL_

extern	bool		JumpApi;
extern	bool		CallAddrApi;
extern	DWORD_PTR	Address;
extern	bool		PushString;
extern	DISASM_OPTIONS	disop;

#endif

// ================================================================
// ==========================  DEFINES  ===========================
// ================================================================

#define USE_MMX

#ifdef USE_MMX
	#ifndef _M_X64
		#define StringLen(str) xlstrlen(str) // new lstrlen with mmx function
	#else
		#define StringLen(str) lstrlen(str) // old c library strlen
	#endif
#else
	#define StringLen(str) lstrlen(str) // old c library strlen
#endif

// ================================================================
// =====================  CONST VARIABLES  ========================
// ================================================================

// x86/x86-64 Registers
const char *regs[6][16] = {
	// REG8 (no REX) — indices 0-7 only valid
	{ "al","cl","dl","bl","ah","ch","dh","bh","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b" },
	// REG16
	{ "ax","cx","dx","bx","sp","bp","si","di","r8w","r9w","r10w","r11w","r12w","r13w","r14w","r15w" },
	// REG32
	{ "eax","ecx","edx","ebx","esp","ebp","esi","edi","r8d","r9d","r10d","r11d","r12d","r13d","r14d","r15d" },
	// FPU (unused placeholder, index 3)
	{ "","","","","","","","","","","","","","","","" },
	// REG64 (index 4)
	{ "rax","rcx","rdx","rbx","rsp","rbp","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15" },
	// REG8X (with REX — SPL/BPL/SIL/DIL replace AH-BH, index 5)
	{ "al","cl","dl","bl","spl","bpl","sil","dil","r8b","r9b","r10b","r11b","r12b","r13b","r14b","r15b" }
};

// x86 Data Size
const char *regSize[10]			= { "Qword","Dword","Word","Byte","Fword","TByte","(28)Byte","(108)Byte","DQword", "(512)Byte" };	// Registers Size of addressing

// x86 Segments
const char *segs[8]				= { "ES","CS","SS","DS","FS","GS","SEG?","SEG?" };	// Segments

// x86 SIB
const char *Scale[5]			= { "-","+","*2+","*4+","*8+" };	// Scale in SIB

// 16Bit Addressing
const char *addr16[8]			= { "BX+SI","BX+DI","BP+SI","BP+DI","SI","DI","BX","BP" };	// 16bit addressing

// x86 Instructions
const char *Instructions			[8] = { "add" , "or"  , "adc" ,		"sbb" ,	"and" , "sub"	, "xor" , "cmp"  }; // Basic      Repetive Assembly
const char *ArtimaticInstructions	[8] = { "rol" , "ror" , "rcl" ,		"rcr" ,	"shl" , "shr"	, "sal" , "sar"  }; // Bitwise    Repetive Assembly
const char *InstructionsSet2		[8] = { "test", "test", "not" ,		"neg" ,	"mul" , "imul"	, "div" , "idiv" }; // Arithmatic Repetive Assembly (test is Twice -> long repetive set)
const char *InstructionsSet3		[8] = { "inc" , "dec" , "???" ,		"???" , "???" , "???"    , "???" , "???"  }; // Arithmatic Repetive Assebly (Opcode 0xFE)
const char *InstructionsSet4		[8] = { "inc" , "dec" , "call","call far", "jmp" , "jmp far", "push", "???"  }; // Arithmatic Repetive Assebly (Opcode 0xFE)

// FPU instructions
const char *FpuRegs							[8] = { "st(0)", "st(1)", "st(2)", "st(3)" , "st(4)" , "st(5)" , "st(6)" , "st(7)"  }; // FPU Registers
const char *FpuInstructions					[8] = { "fadd" , "fmul" , "fcom" , "fcomp" , "fsub"  , "fsubr" , "fdiv"  , "fdivr"  }; // Unsigned fpu instructions
const char *FpuInstructionsSigned			[8] = { "fiadd", "fimul", "ficom", "ficomp", "fisub" , "fisubr", "fidiv" , "fidivr" }; // Signed fpu instructions
const char *FpuInstructionsSet2				[8] = { "fld"  , "???"  , "fst"  , "fstp"  , "fldenv", "fldcw" , "fstenv", "fstcw"  }; // set2 of Unsigned fpu instructions
const char *FpuInstructionsSet2Signed		[8] = { "fild" , "fisttp", "fist" , "fistp" , "???"   , "fld"   , "???"   , "fstp"   }; // set2 of Signed fpu instructions
const char *FpuInstructionsSet3				[8] = { "fld"  , "fisttp", "fst"  , "fstp"  , "frstor", "???"   , "fsave" , "fstsw"  }; // set3 of Unsigned fpu instructions
const char *FpuInstructionsSet2Signed_EX	[8] = { "fild" , "fisttp", "fist" , "fistp" , "fbld"  , "fild"  , "fbstp" , "fistp"  }; // set2 of Signed fpu instructions With Extended 2 instructions

// MMX, 3DNow! Registers
const char *Regs3DNow	[8]  = { "mm0"	, "mm1"		, "mm2"		, "mm3"		, "mm4"		, "mm5"		, "mm6"		, "mm7"		}; // 3DNow! Registers
const char *MMXRegs		[8]  = { "xmm0"	, "xmm1"	, "xmm2"	, "xmm3"	, "xmm4"	, "xmm5"	, "xmm6"	, "xmm7"	}; // MMX Registers

// Extended SSE/AVX/AVX-512 Registers
const char *XMMRegs[16] = { "xmm0","xmm1","xmm2","xmm3","xmm4","xmm5","xmm6","xmm7","xmm8","xmm9","xmm10","xmm11","xmm12","xmm13","xmm14","xmm15" };
const char *YMMRegs[16] = { "ymm0","ymm1","ymm2","ymm3","ymm4","ymm5","ymm6","ymm7","ymm8","ymm9","ymm10","ymm11","ymm12","ymm13","ymm14","ymm15" };
const char *ZMMRegs[32] = { "zmm0","zmm1","zmm2","zmm3","zmm4","zmm5","zmm6","zmm7","zmm8","zmm9","zmm10","zmm11","zmm12","zmm13","zmm14","zmm15",
                            "zmm16","zmm17","zmm18","zmm19","zmm20","zmm21","zmm22","zmm23","zmm24","zmm25","zmm26","zmm27","zmm28","zmm29","zmm30","zmm31" };
const char *KRegs[8]    = { "k0","k1","k2","k3","k4","k5","k6","k7" };

// MMX, 3DNow! (+extended), SSE , SSE2 Instructions
const char *NewSet		[8]  = { "sldt"		, "str"		, "lldt"	, "ltr"		, "verr"	, "verw"	, "???"		, "???"    }; // New Set1
const char *NewSet2		[8]  = { "sgdt"		, "sidt"	, "lgdt"	, "lidt"	, "smsw"	, "???"		, "lmsw"	, "invlpg" }; // New Set2
const char *NewSet3		[8]  = { "prefetchnta", "prefetcht0", "prefetcht1"	, "prefetcht2", "???"       , "???"     , "???"     , "???"    }; // New Set3
const char *NewSet4		[8]  = { "movaps"	, "movaps"		, "cvtpi2ps"	, "???"       , "cvttps2pi" , "cvtps2pi", "ucomiss" , "comiss" }; // New Set4
const char *NewSet5		[16] = { "cmovo"	, "cmovno"		, "cmovb"		, "cmovnb"    , "cmove"     , "cmovne"  , "cmovbe"  , "cmova"   , "cmovs"    , "cmovns"   , "cmovpe"     , "cmovpo"  , "cmovl"   , "cmovge" , "cmovle", "cmovg" }; // New Set5
const char *NewSet6		[16] = { "???"		, "sqrtps"		, "rsqrtps"		, "rcpps"     , "andps"     , "andnps"  , "orps"    , "xorps"   , "addps"    , "mulps"    , "???"        , "???"     , "subps"   , "minps"  , "divps" , "maxps" }; // New Set6
const char *NewSet6Ex	[16] = { "???"		, "sqrtss"		, "rsqrtss"		, "rcpss"     , "andps"     , "andnps"  , "orps"    , "xorps"   , "addss"    , "mulss"    , "???"        , "???"     , "subss"   , "minss"  , "divss" , "maxss" }; // New Set6 Extended (Prefix 0xF3)
const char *NewSet6_F2	[16] = { "???"		, "sqrtsd"		, "???"			, "???"       , "???"       , "???"     , "???"     , "???"     , "addsd"    , "mulsd"    , "???"        , "???"     , "subsd"   , "minsd"  , "divsd" , "maxsd" }; // New Set6 Extended (Prefix 0xF2)
const char *NewSet6_66	[16] = { "???"		, "sqrtpd"		, "???"			, "???"       , "andpd"     , "andnpd"  , "orpd"    , "xorpd"   , "addpd"    , "mulpd"    , "???"        , "???"     , "subpd"   , "minpd"  , "divpd" , "maxpd" }; // New Set6 Extended (Prefix 0x66)
const char *NewSet7		[16] = { "punpcklbw", "punpcklwd"	, "punpckldq"	, "packsswb"  , "pcmpgtb"   , "pcmpgtw" , "pcmpgtd" , "packuswb", "punpckhbw", "punpckhwd", "punpckhdq"  , "packssdw", "???"     , "???"    , "movd"  , "movq"  }; // New Set7
const char *NewSet8		[8]  = { "pshufw"	, "???"			, "???"			, "???"       , "pcmpeqb"   , "pcmpeqw" , "pcmpeqd" , "emms" };                                                                                                    // New Set8
const char *NewSet9		[16] = { "seto"		, "setno"		, "setb"		, "setnb"     , "sete"      , "setne"   , "setbe"   , "seta"    , "sets"     , "setns"    , "setpe"      , "setpo"   , "setl"    , "setge"  , "setle" , "setg"  }; // New Set9
const char *NewSet10	[16] = { "push fs"	, "pop fs"		, "cpuid"		, "bt"        , "shld"      , "shld"    , "???"     , "???"     , "push gs"  , "pop gs"   , "rsm"        , "bts"     , "shrd"    , "shrd"   , "fxsave", "imul"  }; // New Set10
const char *NewSet10Ex	[8]  = { "fxsave"	, "fxrstor"		, "ldmxcsr"		, "stmxcsr"   , "xsave"     , "xrstor"  , "xsaveopt", "clflush" };                                                                                              // New Set10 Extended (Opcode 0xAE)
const char *NewSet11	[16] = { "cmpxchg"	, "cmpxchg"		, "lss"			, "btr"       , "lfs"       , "lgs"     , "movzx"   , "movzx"   , "???"      , "???"      , "???"        , "btc"     , "bsf"     , "bsr"    , "movsx" , "movsx" }; // New Set11
const char *NewSet12	[8]  = { "cmpeqps"	, "cmpltps"		, "cmpleps"		, "cmpunordps", "cmpneqps"  , "cmpnltps", "cmpnleps", "cmpordps" };                                                                                                // New Set12
const char *NewSet12Ex	[8]  = { "cmpeqss"	, "cmpltss"		, "cmpless"		, "cmpunordss", "cmpneqss"  , "cmpnltss", "cmpnless", "cmpordss" };                                                                                                // New Set12 Extended (Prefix 0xF3)
const char *NewSet12_F2	[8]  = { "cmpeqsd"	, "cmpltsd"		, "cmplesd"		, "cmpunordsd", "cmpneqsd"  , "cmpnltsd", "cmpnlesd", "cmpordsd" };                                                                                                // New Set12 Extended (Prefix 0xF2)
const char *NewSet12_66	[8]  = { "cmpeqpd"	, "cmpltpd"		, "cmplepd"		, "cmpunordpd", "cmpneqpd"  , "cmpnltpd", "cmpnlepd", "cmpordpd" };                                                                                                // New Set12 Extended (Prefix 0x66)
const char *NewSet13	[16] = { "???"		, "psrlw"		, "psrld"		, "psrlq"     , "paddq"     , "pmullw"  , "???"     , "pmovmskb", "psubusb"  , "psubusw"  , "pminub"     , "pand"    , "paddusb" , "paddusw", "pmaxub", "pandn" }; // New Set13
const char *NewSet14	[16] = { "pavgb"	, "psraw"		, "psrad"		, "pavgw"     , "pmulhuw"   , "pmulhw"  , "???"     , "movntq"  , "psubsb"   , "psubsw"   , "pminsw"     , "por"     , "paddsb"  , "paddsw" , "pmaxsw", "pxor"  }; // New Set14
const char *NewSet15	[16] = { "???"		, "psllw"		, "pslld"		, "psllq"     , "???"       , "pmaddwd" , "psadbw"  , "maskmovq", "psubb"    , "psubw"    , "psubd"      , "psubq"   , "paddb"   , "paddw"  , "paddd" , "???"   }; // New Set15
const char *NewSet16	[8]  = { "???"       , "???"        , "movdq2q"     , "movq2dq"   , "???"       , "???"     , "movq"    , "???" }; // Used at: (0x66/0x73/0x72)0Fxx ; note the prefix.

// Debug/Control/Test Registers
const char *DebugRegs  [8]  = { "dr0"	, "dr1"	, "dr2"	, "dr3"	, "dr4"	, "dr5"	, "dr6"	, "dr7"	}; // Debug Registers
const char *ControlRegs[8]  = { "cr0"	, "cr1"	, "cr2"	, "cr3"	, "cr4"	, "cr5"	, "cr6"	, "cr7"	}; // Control Registers
//const char *TestRegs [8]  = { "tr0"	, "tr1"	, "tr2"	, "tr3"	, "tr4"	, "tr5"	, "tr6"	, "tr7"	}; // Test Registers


// ================================================================
// =====================  DECODING FUNCTIONS  =====================
// ================================================================

void Mod_11_RM(BYTE d, BYTE w,char **Opcode,DISASSEMBLY **Disasm,char instruction[],bool PrefixReg,BYTE Op,DWORD_PTR **index)
{
	/* 
		Function Mod_11_RM Checks whatever we have
		Both bit d (direction) and bit w (full/partial size).

		There are 4 states:
		00 - d=0 / w=0 ; direction -> (ie: DH->DL),		partial size (AL,DH,BL..)
		01 - d=0 / w=1 ; direction -> (ie: EDX->EAX),	partial size (EAX,EBP,EDI..)
		10 - d=1 / w=0 ; direction <- (ie: DH<-DL),		partial size (AL,DH,BL..)
		11 - d=1 / w=1 ; direction <- (ie: EDX<-EAX),	partial size (EAX,EBP,EDI..)

		Also deals with harder opcodes which have diffrent
		Addresing type.
	*/

    DWORD_PTR dwMem=0,dwOp=0;
	DWORD_PTR RM,IndexAdd=1,m_OpcodeSize=2,Pos; // Register(s) Pointer
	WORD wMem=0,wOp=0;
    BYTE reg1=0,reg2=0,m_Opcode=0,REG;
    BYTE FOpcode;
	char assembly[50]="",temp[128]="",m_Bytes[128]="";

    Pos=(*(*index)); // Current Position
    m_Opcode = (BYTE)(*(*Opcode+Pos+1)); // Decode registers from second byte
    
    // Strip Used Instructions / Used Segment
    REG=(BYTE)(*(*Opcode+Pos+1));
    REG>>=3;
	REG&=0x07;

	// Check Opcode range
	if((Op>=0x80 && Op<=0x83) || Op==0xC7 || Op==0x69){
		switch(Op){ // Find Current Opcode
			// Different Opcodes has different Modes.
			case 0x80: case 0x82: case 0x83:{ // 1 byte
				RM=REG8;
				if(Op==0x83 && PrefixReg==0){ // full size reg
					RM=REG32;
					if((*Disasm)->Mode64 && (*Disasm)->RexW)
						RM=REG64;
				}

				if(PrefixReg==1){
					RM=REG16;
				}

				reg1=(m_Opcode&7); // Get Destination Register
				if((*Disasm)->Mode64 && (*Disasm)->RexB)
					reg1 |= 0x08;
				SwapWord((BYTE*)(*Opcode+Pos+1),&wOp,&wMem);

				FOpcode=wOp&0x00FF;
				if(FOpcode>0x7F){ // check for signed numbers!!
					FOpcode = 0x100-FOpcode; // -XX
					wsprintf(temp,"%s%02Xh",Scale[0],FOpcode); // '-' arithmetic
				}
				else{
					wsprintf(temp,"%02Xh",FOpcode);
				}
				// Read Opcodes: Opcode - imm8
				wsprintf(m_Bytes,"%02X%04X",Op,wOp);
				m_OpcodeSize=3;
				(*(*index))+=2; // Prepare to read next Instruction
			}
			break;

			case 0x81: case 0xC7: case 0x69:{ // 2 (WORD)/4 (DWORD) bytes
				// 0x66 is being Used
				if(PrefixReg==1){ // READ WORD
					RM=REG16;
					reg1=(m_Opcode&0x07); // Get Destination Register
					if((*Disasm)->Mode64 && (*Disasm)->RexB)
						reg1 |= 0x08;
					SwapWord((BYTE*)(*Opcode+Pos+2),&wOp,&wMem);
					SwapDword((BYTE*)(*Opcode+Pos),&dwOp,&dwMem);
					// Read imm16
					wsprintf(temp,"%04Xh",wMem);
					// Read Opcodes: Opcode - imm16
					wsprintf(m_Bytes,"%08X",dwOp);
					m_OpcodeSize=4; // Instruction Size
					(*(*index))+=3;
				}
				else{ //no reg prefix
					RM=REG32;
					if((*Disasm)->Mode64 && (*Disasm)->RexW)
						RM=REG64;
					reg1=(m_Opcode&0x07); // Get Destination Register
					if((*Disasm)->Mode64 && (*Disasm)->RexB)
						reg1 |= 0x08;
					SwapDword((BYTE*)(*Opcode+Pos+2),&dwOp,&dwMem);
					SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
					// Read Dword Data number (imm32)
					if((dwMem&0x80000000)==0x80000000) // check for leading 0
					wsprintf(temp,"0%08Xh",dwMem);
					else
					wsprintf(temp,"%08Xh",dwMem);
					// Read Opcodes: Opcode - imm32
					wsprintf(m_Bytes,"%04X %08X",wOp,dwOp);
					m_OpcodeSize=6; // Instruction Size
					(*(*index))+=5;
				}
			}
			break;
		}

		if(Op==0xC7){
			/*
			Instruction rule: Mem,Imm ->  1100011woo000mmm,imm
			Code Block: 1100011
			w = Reg Size
			oo - Mod
			000 - Must be!
			mmm - Reg/Mem
			imm - Immidiant (����)
			*/

			if(((m_Opcode&0x38)>>3)==7){ // XBEGIN rel16/rel32 (C7 F8)
				wsprintf(assembly,"xbegin %s",temp);
			}
			else if(((m_Opcode&0x38)>>3)!=0){ // check 000
			  lstrcat((*Disasm)->Remarks,"Invalid Instruction");
			  wsprintf(assembly,"%s %s, %s","mov",regs[RM][reg1],temp);
			}
			else{
			  wsprintf(assembly,"%s %s, %s","mov",regs[RM][reg1],temp);
			}
        }
        else{
			// Build assembly
			if(Op==0x69){
				reg2=((m_Opcode&0x038)>>3);
				wsprintf(assembly,"imul %s, %s, %s",regs[RM][reg2],regs[RM][reg1],temp);
			}
			else{
				wsprintf(assembly,"%s %s, %s",Instructions[REG],regs[RM][reg1],temp);
			}
		}
        
		lstrcat((*Disasm)->Assembly,assembly);
		(*Disasm)->OpcodeSize=m_OpcodeSize;
		lstrcat((*Disasm)->Opcode,m_Bytes);
		return; // RET
    }
	else{ // Check Other Set of Opcodes
		// Special Types usnig Segments
		if(Op==0x8C || Op==0x8E){
			RM=REG16;
			reg1=(m_Opcode&0x07);
			SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
			wsprintf(m_Bytes,"%04X",wOp);
            
			if(REG<=5){ // SEG IS KNOWN
				if(d==0){ // (->) Direction
					wsprintf(assembly,"%s %s, %s",instruction,regs[RM][reg1],segs[REG]);
				}
				else{ // (<-) Direction
					wsprintf(assembly,"%s %s, %s",instruction,segs[REG],regs[RM][reg1]);
				}
			}
			else{ // UNKNOWN SEG (NOT IN RANGE 0-5)
				if(d==0){ // (->) Direction
					wsprintf(assembly,"%s %s, SEG ??",instruction,regs[RM][reg1]);
				}
				else{ //(<-) Direction
					wsprintf(assembly,"%s SEG ??,%s",instruction,regs[RM][reg1]);
				}
				// Put warning
				lstrcat((*Disasm)->Remarks,";Unknown Segment Used,");
			}

			// Add data to the Struct
			(*Disasm)->OpcodeSize=2; // Instruction Size
			lstrcat((*Disasm)->Assembly,assembly);
			lstrcat((*Disasm)->Opcode,m_Bytes); 

			// Segment Modification Opcode ( MOV <SEG>, <REG>)
			if(Op==0x8E){
				lstrcat((*Disasm)->Remarks,";Segment Is Being Modified!");
			}

			(*(*index))++;
			return;
        }

		if(Op==0xC6){
			RM=REG8;
			if(m_Opcode==0xF8){ // XABORT imm8 (C6 F8 ib)
				BYTE imm = (BYTE)(*(*Opcode+Pos+2));
				wsprintf(m_Bytes,"C6 F8 %02X",imm);
				wsprintf(assembly,"xabort %02Xh",imm);
				m_OpcodeSize=3;
				(*(*index))+=2;
				lstrcat((*Disasm)->Assembly,assembly);
				(*Disasm)->OpcodeSize=m_OpcodeSize;
				lstrcat((*Disasm)->Opcode,m_Bytes);
				return;
			}
			if(m_Opcode>=0xC0 && m_Opcode<=0xC7){
				reg1=(m_Opcode&0x07); // Get Destination Register
				SwapWord((BYTE*)(*Opcode+Pos+1),&wOp,&wMem);
				// Read imm16
				wsprintf(temp,"%02X",*((BYTE*)(*Opcode+Pos+2)));
				wsprintf(m_Bytes,"C6 %04X",wOp);
				// Read Opcodes: Opcode - imm16
				m_OpcodeSize=3; // Instruction Size
				(*(*index))+=2;
				wsprintf(assembly,"%s %s, %s","mov",regs[RM][reg1],temp);
			}
			else{
				SwapWord((BYTE*)(*Opcode+Pos+1),&wOp,&wMem);
				wsprintf(m_Bytes,"C6 %04X",wOp);
				m_OpcodeSize=3;
				(*(*index))+=2;
				lstrcpy(assembly,"???");
			}
			lstrcat((*Disasm)->Assembly,assembly);
			(*Disasm)->OpcodeSize=m_OpcodeSize;
			lstrcat((*Disasm)->Opcode,m_Bytes);
			return;
		}

		// Mixed Instructions
		if(Op==0xC0 || Op==0xC1){
			// Check register Size
			if(w==0){
				RM=REG8;
			}
			else{
				if(PrefixReg==1){
					RM=REG16;
				}
				else{
					RM=REG32;
				}
			}

			reg1=(m_Opcode&7); // Get Destination Register

			// 64-bit mode: apply REX extensions for C0/C1
			if((*Disasm)->Mode64){
				if((*Disasm)->RexW && w==1){
					RM=REG64;
				}
				if((*Disasm)->RexB){
					reg1 |= 0x08;
				}
				if((*Disasm)->RexPrefix && w==0){
					RM=REG8X;
				}
			}

			SwapWord((BYTE*)(*Opcode+Pos+1),&wOp,&wMem);
			wsprintf(temp,"%02Xh",wOp&0x00FF);
			// Read Opcodes: Opcode - imm8
			wsprintf(m_Bytes,"%02X%04X",Op,wOp);
			m_OpcodeSize=3;
			(*(*index))+=2; // Prepare to read next Instruction
			// Build assembly
			wsprintf(assembly,"%s %s, %s",ArtimaticInstructions[REG],regs[RM][reg1],temp);
			lstrcat((*Disasm)->Assembly,assembly);
			(*Disasm)->OpcodeSize=m_OpcodeSize;
			lstrcat((*Disasm)->Opcode,m_Bytes);
			return; // exit the function
		}
        
		// XCHG Register
		if(Op>=0x91 && Op<=0x97){
			m_Opcode=(*(*Opcode+Pos));	// 1 byte Opcode
			m_Opcode+=0x30;				// Add 0x30 in order to get values of EAX-EDI (trick)
			IndexAdd=0;					// Don't Add to the index counter.
			m_OpcodeSize=1;				// 1 byte opcode          
		}

		// (->) / reg8
		if(d==0 && w==0){
			RM=REG8;
			reg1=(m_Opcode&0x07);
			reg2=(m_Opcode&0x38)>>3;
		}

		// (->) / reg32
		if(d==0 && w==1){    
			RM=REG32;
			if(PrefixReg==1){
			RM=REG16; // (->) / reg16 (RegPerfix is being used)
			}

			reg1=(m_Opcode&0x07);
			reg2=(m_Opcode&0x38)>>3;
		}
        
		// (<-) / reg8
		if(d==1 && w==0){    
			RM=REG8;
			reg2=(m_Opcode&0x07);
			reg1=(m_Opcode&0x38)>>3;
		}
        
		// (<-) / reg32
		if(d==1 && w==1){
			RM=REG32;
			if(PrefixReg==1){
				RM=REG16; // (<-) / reg16
			}

			reg2=(m_Opcode&0x07);
			reg1=(m_Opcode&0x38)>>3;
		}

		// 64-bit mode: apply REX extensions
		if((*Disasm)->Mode64){
			// REX.W promotes operand size to 64-bit
			if((*Disasm)->RexW && w==1){
				RM=REG64;
			}
			// REX.R extends reg field (reg1 in d=1, reg2 in d=0)
			if((*Disasm)->RexR){
				if(d==1) reg1 |= 0x08;
				else     reg2 |= 0x08;
			}
			// REX.B extends r/m field (reg2 in d=1, reg1 in d=0)
			if((*Disasm)->RexB){
				if(d==1) reg2 |= 0x08;
				else     reg1 |= 0x08;
			}
			// REX prefix present + w=0: use REG8X (SPL/BPL/SIL/DIL)
			if((*Disasm)->RexPrefix && w==0){
				RM=REG8X;
			}
		}

		// Check Opcode Size (XCHG changes it)
		if(m_OpcodeSize==1){
			wsprintf(temp,"%02X",Op);
		}
		else{ // Default
			SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
			wsprintf(temp,"%04X",wOp);
		}       

		switch(Op){
			case 0x6B:{ // IMUL REG,REG,IIM
				SwapWord((BYTE*)(*Opcode+Pos+1),&wOp,&wMem);
				FOpcode=wOp&0x00FF;

				if(FOpcode>0x7F){ // check for signed numbers!!
					FOpcode = 0x100-FOpcode; // -XX (Signed)
					wsprintf(temp,"%s",Scale[0]); // '-' arithmetic (Signed)
				}
				else{
					strcpy_s(temp,"");
				}

				m_OpcodeSize=3;
				(*(*index))++;
				wsprintf(assembly,"imul %s,%s,%s%02Xh",regs[RM][reg1],regs[RM][reg2],temp,FOpcode);
				wsprintf(temp,"%02X%04X",Op,wOp);
			}
			break;
           
			case 0x8F:{ // POP REG
				if((BYTE)(*(*Opcode+Pos+1))>=0xC8){ // above bytes has !=000 there for invalid
					lstrcat((*Disasm)->Remarks,";Invalid Instruction");
				}
				wsprintf(assembly,"%s %s",instruction,regs[RM][reg2]);
			}
			break;

			case 0xD0: case 0xD1:{
				wsprintf(assembly,"%s %s, 1",ArtimaticInstructions[REG],regs[RM][reg1]);
			}
			break;

			case 0xD2: case 0xD3:{
				wsprintf(assembly,"%s %s, cl",ArtimaticInstructions[REG],regs[RM][reg2]);
			}
			break;

			case 0xD8:{ // FPU Instruction
				if(REG==3){ // fcomp uses 1 operand
					wsprintf(assembly,"%s %s",FpuInstructions[REG],FpuRegs[reg1]);
				}
				else{ // st(0) is the dest
					wsprintf(assembly,"%s st,%s",FpuInstructions[REG],FpuRegs[reg1]);
				}
			}
			break;
         
			case 0xD9:{ // FPU Instructions
				// 2 byte FPU Instructions
				switch((BYTE)(*(*Opcode+Pos+1)))
				{
					case 0xC8:case 0xC9:case 0xCA:case 0xCB:
					case 0xCC:case 0xCD:case 0xCE:case 0xCF:
					{
						wsprintf(assembly,"fxch %s",FpuRegs[reg1]);
					}
					break;
                
					case 0xD1:case 0xD2:case 0xD3:case 0xD4:
					case 0xD5:case 0xD6:case 0xD7:
					{
					   wsprintf(assembly,"fst %s",FpuRegs[reg1]);
					}
					break;

					case 0xD8:case 0xD9:case 0xDA:case 0xDB:
					case 0xDC:case 0xDD:case 0xDE:case 0xDF:
					{
					wsprintf(assembly,"fstp %s",FpuRegs[reg1]);
					}
					break;

					case 0xE2:case 0xE3:case 0xE6:case 0xE7:
					{
					   wsprintf(assembly,"fldenv %s",FpuRegs[reg1]);
					}
					break;

					case 0xEF:{
					   wsprintf(assembly,"fldcw %s",FpuRegs[reg1]);
					}
					break;

					case 0xC0:case 0xC1:case 0xC2:case 0xC3:case 0xC4:
					case 0xC5:case 0xC6:case 0xC7:
					{
						wsprintf(assembly,"fld %s",FpuRegs[reg1]);
					}
					break;

					case 0xD0: strcpy_s(assembly,"fnop");		break;
					case 0xE0: strcpy_s(assembly,"fchs");		break;
					case 0xE1: strcpy_s(assembly,"fabs");		break;
					case 0xE4: strcpy_s(assembly,"ftst");		break;
					case 0xE5: strcpy_s(assembly,"fxam");		break;
					case 0xE8: strcpy_s(assembly,"fld1");	break;
					case 0xE9: strcpy_s(assembly,"fldl2t");	break;
					case 0xEA: strcpy_s(assembly,"fldl2e");	break;
					case 0xEB: strcpy_s(assembly,"fldpi");	break;
					case 0xEC: strcpy_s(assembly,"fldlg2");	break;
					case 0xED: strcpy_s(assembly,"fldln2");	break;
					case 0xEE: strcpy_s(assembly,"fldz");		break;
					case 0xF0: strcpy_s(assembly,"f2xm1");	break;
					case 0xF1: strcpy_s(assembly,"fyl2x");	break;
					case 0xF2: strcpy_s(assembly,"fptan");	break;
					case 0xF3: strcpy_s(assembly,"fpatan");	break;
					case 0xF4: strcpy_s(assembly,"fxtract");	break;
					case 0xF5: strcpy_s(assembly,"fprem1");	break;
					case 0xF6: strcpy_s(assembly,"fdecstp");	break;
					case 0xF7: strcpy_s(assembly,"fincstp");	break;
					case 0xF8: strcpy_s(assembly,"fprem");	break;
					case 0xF9: strcpy_s(assembly,"fyl2xp1"); break;
					case 0xFA: strcpy_s(assembly,"fsqrt");	break;
					case 0xFB: strcpy_s(assembly,"fsincos");	break;
					case 0xFC: strcpy_s(assembly,"frndint");	break;
					case 0xFD: strcpy_s(assembly,"fscale");	break;
					case 0xFE: strcpy_s(assembly,"fsin");		break;
					case 0xFF: strcpy_s(assembly,"fcos");		break;                                
				}                 
			}
			break;
         
			case 0xDA:{ // FPU Instructions
				switch((BYTE)(*(*Opcode+Pos+1))){
					case 0xC0:case 0xC1:case 0xC2:case 0xC3: // FCMOVB
					case 0xC4:case 0xC5:case 0xC6:case 0xC7:
					{
						wsprintf(assembly,"fcmovb st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xC8:case 0xC9:case 0xCA:case 0xCB: // FCMOVE
					case 0xCC:case 0xCD:case 0xCE:case 0xCF:
					{
						wsprintf(assembly,"fcmove st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xD0:case 0xD1:case 0xD2:case 0xD3: // FCMOVBE
					case 0xD4:case 0xD5:case 0xD6:case 0xD7:
					{
						wsprintf(assembly,"fcmovbe st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xD8:case 0xD9:case 0xDA:case 0xDB: // FCMOVU
					case 0xDC:case 0xDD:case 0xDE:case 0xDF:
					{
						wsprintf(assembly,"fcmovu st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xE9: // FUCOMPP
					{
						strcpy_s(assembly,"fucompp");
					}
					break;

					// Default Signed FPU Instructions
					default: wsprintf(assembly,"%s %s",FpuInstructionsSigned[REG],FpuRegs[reg2]); break;
				}
			}
			break;

			case 0xDB:{ // FPU Instruction
				switch((BYTE)(*(*Opcode+Pos+1))){
					case 0xC0:case 0xC1:case 0xC2:case 0xC3: // FCMOVNB
					case 0xC4:case 0xC5:case 0xC6:case 0xC7: // FCMOVNB
					{
						wsprintf(assembly,"fcmovnb st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xC8:case 0xC9:case 0xCA:case 0xCB: // FCMOVNE
					case 0xCC:case 0xCD:case 0xCE:case 0xCF: // FCMOVNE
					{
						wsprintf(assembly,"fcmovne st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xD0:case 0xD1:case 0xD2:case 0xD3: // FCMOVNBE
					case 0xD4:case 0xD5:case 0xD6:case 0xD7: // FCMOVNBE
					{
						wsprintf(assembly,"fcmovnbe st,%s",FpuRegs[reg2]);
					}
					break;
                 
					case 0xD8:case 0xD9:case 0xDA:case 0xDB: // FCMOVNU
					case 0xDC:case 0xDD:case 0xDE:case 0xDF: // FCMOVNU
					{
						wsprintf(assembly,"fcmovnu st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xE0: strcpy_s(assembly,"feni");  break;
					case 0xE1: strcpy_s(assembly,"fdisi"); break;
					case 0xE2: strcpy_s(assembly,"fnclex"); break;
					case 0xE3: strcpy_s(assembly,"fninit"); break;

					case 0xE4: case 0xE5: case 0xE6: case 0xE7: // (Invalid) Reserved instructions..???
					{
						lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						strcpy_s(assembly,"???");
					}
					break;

					case 0xE8:case 0xE9:case 0xEA:case 0xEB: // 
					case 0xEC:case 0xED:case 0xEE:case 0xEF: // 
					{
						wsprintf(assembly,"fucomi st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xF0:case 0xF1:case 0xF2:case 0xF3: // 
					case 0xF4:case 0xF5:case 0xF6:case 0xF7: // 
					{
						wsprintf(assembly,"fcomi st,%s",FpuRegs[reg2]);
					}
					break;

					default: wsprintf(assembly,"fstp %s",FpuRegs[reg2]); break;
				}
			}
			break;
         
			case 0xDC:{ // FPU Instruction
				if(REG==3){ // fcomp uses 1 operand
					wsprintf(assembly,"%s %s",FpuInstructions[REG],FpuRegs[reg1]);
				}
				else{ // st(0) is the src
					switch(REG){ // fdiv<->fdivr / fsub <-> fsubr (changed positions)
						case 4:REG++;break;
						case 5:REG--;break;
						case 6:REG++;break;
						case 7:REG--;break;
					}
					wsprintf(assembly,"%s %s,st",FpuInstructions[REG],FpuRegs[reg1]);
				}
			}
			break;

			case 0xDD:{ // FPU Instruction
				switch((BYTE)(*(*Opcode+Pos+1))){
					case 0xC0:case 0xC1:case 0xC2:case 0xC3: 
					case 0xC4:case 0xC5:case 0xC6:case 0xC7: 
					{
						wsprintf(assembly,"ffree %s",FpuRegs[reg1]);
					}
					break;

					case 0xC8:case 0xC9:case 0xCA:case 0xCB: 
					case 0xCC:case 0xCD:case 0xCE:case 0xCF: 
					{
						lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						strcpy_s(assembly,"???");
					}
					break;
                 
					case 0xD0:case 0xD1:case 0xD2:case 0xD3:
					case 0xD4:case 0xD5:case 0xD6:case 0xD7:
					case 0xD8:case 0xD9:case 0xDA:case 0xDB:
					case 0xDC:case 0xDD:case 0xDE:case 0xDF:
					{
						wsprintf(assembly,"%s %s",FpuInstructionsSet2[REG],FpuRegs[reg1]);
					}
					break;

					case 0xE0:case 0xE1:case 0xE2:case 0xE3: 
					case 0xE4:case 0xE5:case 0xE6:case 0xE7: 
					{
						wsprintf(assembly,"fucom %s",FpuRegs[reg1]);
					}
					break;

					case 0xE8:case 0xE9:case 0xEA:case 0xEB:
					case 0xEC:case 0xED:case 0xEE:case 0xEF:
					{
						wsprintf(assembly,"fucomp %s",FpuRegs[reg1]);
					}
					break;

					case 0xF0:case 0xF1:case 0xF2:case 0xF3:
					case 0xF4:case 0xF5:case 0xF6:case 0xF7:
					case 0xF8:case 0xF9:case 0xFA:case 0xFB:
					case 0xFC:case 0xFD:case 0xFE:case 0xFF:
					{
						wsprintf(assembly,"%s %s",FpuInstructionsSet3[REG],FpuRegs[reg1]);
					}
					break;
				}
			}
			break;

			case 0xDE:{ // FPU Instruction
				switch((BYTE)(*(*Opcode+Pos+1))){
					case 0xC0:case 0xC1:case 0xC2:case 0xC3:
					case 0xC4:case 0xC5:case 0xC6:case 0xC7:
					{
						wsprintf(assembly,"faddp %s,st",FpuRegs[reg2]);
					}
					break;

					case 0xC8:case 0xC9:case 0xCA:case 0xCB:
					case 0xCC:case 0xCD:case 0xCE:case 0xCF:
					{
						wsprintf(assembly,"fmulp %s,st",FpuRegs[reg2]);
					}
					break;

					case 0xD0:case 0xD1:case 0xD2:case 0xD3:
					case 0xD4:case 0xD5:case 0xD6:case 0xD7:
					{
						wsprintf(assembly,"ficom %s",FpuRegs[reg2]);
					}
					break;

					case 0xD9: // FCOMPP
					{
						strcpy_s(assembly,"fcompp");
					}
					break;

					case 0xD8:case 0xDA:case 0xDB:
					case 0xDC:case 0xDD:case 0xDE:case 0xDF:
					{
						wsprintf(assembly,"ficomp %s",FpuRegs[reg2]);
					}
					break;

					case 0xE0:case 0xE1:case 0xE2:case 0xE3:
					case 0xE4:case 0xE5:case 0xE6:case 0xE7:
					{
						wsprintf(assembly,"fsubrp %s,st",FpuRegs[reg2]);
					}
					break;

					case 0xE8:case 0xE9:case 0xEA:case 0xEB:
					case 0xEC:case 0xED:case 0xEE:case 0xEF:
					{
						wsprintf(assembly,"fsubp %s,st",FpuRegs[reg2]);
					}
					break;

					case 0xF0:case 0xF1:case 0xF2:case 0xF3:
					case 0xF4:case 0xF5:case 0xF6:case 0xF7:
					{
						wsprintf(assembly,"fdivrp %s,st",FpuRegs[reg2]);
					}
					break;

					case 0xF8:case 0xF9:case 0xFA:case 0xFB:
					case 0xFC:case 0xFD:case 0xFE:case 0xFF:
					{
						wsprintf(assembly,"fdivp %s,st",FpuRegs[reg2]);
					}
					break;
				}
			}
			break;

			case 0xDF: // FPU Instruction
			{
				switch((BYTE)(*(*Opcode+Pos+1))){
					case 0xC0:case 0xC1:case 0xC2:case 0xC3:
					case 0xC4:case 0xC5:case 0xC6:case 0xC7:
					{
						wsprintf(assembly,"ffreep %s",FpuRegs[reg2]);
					}
					break;
                     
					case 0xC8:case 0xC9:case 0xCA:case 0xCB:
					case 0xCC:case 0xCD:case 0xCE:case 0xCF:
					{
						lstrcat((*Disasm)->Remarks,"Invalid Instruction");
						strcpy_s(assembly,"???");
					}
					break;

					case 0xD0:case 0xD1:case 0xD2:case 0xD3:
					case 0xD4:case 0xD5:case 0xD6:case 0xD7:
					{
						wsprintf(assembly,"fist %s",FpuRegs[reg2]);
					}
					break;

					case 0xD8:case 0xD9:case 0xDA:case 0xDB:
					case 0xDC:case 0xDD:case 0xDE:case 0xDF:
					{
						wsprintf(assembly,"fistp %s",FpuRegs[reg2]);
					}
					break;

					case 0xE0:
					{
						strcpy_s(assembly,"fnstsw ax");
					}
					break;

					case 0xE1:case 0xE2:case 0xE3:
					case 0xE4:case 0xE5:case 0xE6:case 0xE7:
					{
						wsprintf(assembly,"fbld %s",FpuRegs[reg2]);
					}
					break;

					case 0xE9:case 0xE8:case 0xEA:case 0xEB:
					case 0xEC:case 0xED:case 0xEE:case 0xEF:
					{
						wsprintf(assembly,"fucomip st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xF0:case 0xF1:case 0xF2:case 0xF3:
					case 0xF4:case 0xF5:case 0xF6:case 0xF7:
					{
						wsprintf(assembly,"fcomip st,%s",FpuRegs[reg2]);
					}
					break;

					case 0xF8:case 0xF9:case 0xFA:case 0xFB:
					case 0xFC:case 0xFD:case 0xFE:case 0xFF:
					{
						wsprintf(assembly,"fistp %s",FpuRegs[reg2]);
					}
					break;
				}
			}
			break;

			case 0xF6:{
				if(reg1==0 || reg1==1){
					SwapWord((BYTE*)(*Opcode+Pos+1),&wOp,&wMem);
					wsprintf(assembly,"%s %s,%02Xh",InstructionsSet2[REG],regs[RM][reg2],wOp&0x00FF);
					(*(*index))++;
					m_OpcodeSize++;
					wsprintf(m_Bytes,"%02X",wOp&0x00FF);
					lstrcat(temp,m_Bytes);
				}
				else{
					wsprintf(assembly,"%s %s",InstructionsSet2[REG],regs[RM][reg2]);
				}
			}
			break;

			case 0xF7:{
				if(reg1==0 || reg1==1){
					if(!PrefixReg){ // no 0x66 prefix used (read DWORD)
						SwapDword((BYTE*)(*Opcode+Pos+2),&dwOp,&dwMem);
						wsprintf(assembly,"%s %s,%08Xh",InstructionsSet2[REG],regs[RM][reg2],dwMem);
						wsprintf(m_Bytes," %08X",dwOp);
						(*(*index))+=4;
						m_OpcodeSize+=4;
					}
					else{ // prefix 0x66 is being used (read WORD)
						SwapWord((BYTE*)(*Opcode+Pos+2),&wOp,&wMem);
						wsprintf(assembly,"%s %s,%04Xh",InstructionsSet2[REG],regs[RM][reg2],wMem);
						wsprintf(m_Bytes," %04X",wOp);
						(*(*index))+=2;
						m_OpcodeSize+=2;
					}
					lstrcat(temp,m_Bytes);
				}
				else{
					wsprintf(assembly,"%s %s",InstructionsSet2[REG],regs[RM][reg2]);
				}
			}
			break;

			case 0xFE:{ // MIX Instructions (INC,DEC,INVALID,INVALID...)
				wsprintf(assembly,"%s %s",InstructionsSet3[REG],regs[RM][reg2]);
				if(REG>1){
					lstrcat((*Disasm)->Remarks,";Illegal Instruction");
				}
			}
			break;

			case 0xFF:{
				DWORD_PTR ff_RM = RM;
				// In 64-bit mode, CALL/JMP/PUSH always use 64-bit registers
				if((*Disasm)->Mode64 && (REG==2 || REG==4 || REG==6)){
					ff_RM = REG64;
				}
				wsprintf(assembly,"%s %s",InstructionsSet4[REG],regs[ff_RM][reg2]);
				if(REG==7){
					lstrcat((*Disasm)->Remarks,";Illegal Instruction");
				}
			}
			break;

			case 0x8D:{
				wsprintf(assembly,"%s %s, %s",instruction,regs[RM][reg2],regs[RM][reg1]);
				lstrcat((*Disasm)->Remarks,"Illegal Instruction");
			}
			break;

			// Default General Instructions
			default: wsprintf(assembly,"%s %s, %s",instruction,regs[RM][reg1],regs[RM][reg2]); break;
		}

		lstrcat((*Disasm)->Assembly,assembly);
		(*Disasm)->OpcodeSize=m_OpcodeSize;
		lstrcat((*Disasm)->Opcode,temp);
		(*(*index))+=IndexAdd;
		// strcpy(menemonic,assembly);
	}
	return; // RET
}

void Mod_RM_SIB(
		  DISASSEMBLY **Disasm,
		  char **Opcode, DWORD_PTR pos, 
		  bool AddrPrefix,
		  int SEG,
		  DWORD_PTR **index,
		  BYTE Bit_d, 
		  BYTE Bit_w, 
		  char *instruction,
		  BYTE Op,
		  bool PrefixReg,
		  bool PrefixSeg,
		  bool PrefixAddr
		 )
{
	/*
		This Function will resolve BigSet menemonics: 
		ADC, ADD, AND, CMP, MOV, OR, SBB, SUB, XOR,ARPL, BOUND..
		We analyze the opcode using ;
		BitD, BitW,SIB ( SS III BBB : Sacle-Index-Base)
		MOD/RM
	*/

	// Set Defaults
	DWORD_PTR dwOp,dwMem;
    int RM=REG8,SCALE=0,SIB,ADDRM=REG32;
    WORD wOp,wMem;
    char RSize[10]="byte",Aritmathic[5]="+",tempAritmathic[5]="+";
	BYTE reg1=0,reg2=0,REG=0,Extension=0,FOpcode=0;
    char menemonic[128]="",tempMeme[128]="",Addr[15]="",temp[128]="";
	char instr[50]="";
	bool bound=0,UsesFPU=0;

    // Get used Register
	// Get target register, example:
	// 1. add byte ptr [ecx], -> al <-
	// 2. add -> al <- ,byte ptr [ecx]
    REG=(BYTE)(*(*Opcode+pos+1));
	REG>>=3;
	REG&=0x07;

    //Displacement MOD (none|BYTE/WORD|DWORD)
	Extension=(BYTE)(*(*Opcode+pos+1))>>6;
	/*
		There are 3 types of Displacement to RegMem
		00 -> [00] 000 000 ; no byte extention ([RegMem])
		40->  [01] 000 000 ; 1 byte extenton ([RegMem+XX])
		80 -> [10] 000 000 ; 4 bytes extention ([RegMem+XXXXXXXX])
	*/

    //===================//
    // Bitwise OverRides //
    //===================//
	// Arpl, Bound, Test, Xchg menemonics are special cases! when alone.
	// so we need to set specific static bits for d/w
    // We specify Size of Data corresponding to each mnemonic.

	switch((BYTE)(*(*Opcode+pos))){
		case 0x20:			{	PrefixReg=0; }	break; // Force Byte Size Regardless Operands.
		case 0x39: case 0x3B:	{	strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);													}	break; // DWORD
		case 0x63:			{
			if((*Disasm)->Mode64){
				Bit_d=1; Bit_w=1; // MOVSXD: register is destination
			} else {
				Bit_d=0; Bit_w=1; // ARPL: memory/reg is destination
			}
			strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); // DWORD source
		}	break;
		case 0x62:			{	RM=REG32; bound=1; Bit_d=1; Bit_w=0; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]);				}	break; // QWORD
		case 0x69:			{	Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);									}	break; // DWORD
		case 0x6B:			{	Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);									}	break; // DWORD
		case 0x84: case 0x86: {	Bit_d=0; Bit_w=0; }	break; // BYTE
		case 0x85: case 0x87: {	Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);									}	break; // DWORD
		case 0x80: case 0x82: case 0xC6: case 0xF6:{ Bit_d=0;Bit_w=0; strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]);				}	break;
		case 0x81: case 0x83: case 0xC7: case 0xF7: case 0x89:{ Bit_d=0;Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);	}	break;	
		case 0x8B: strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);																		break; // DWORD
		case 0x8C: case 0x8E: { strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]);														}	break; // WORD
		case 0x8D: case 0x8F: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);									}	break; // POP/LEA
		case 0xC0:			{ Bit_d=1; Bit_w=0;																}	break; // BYTE
		case 0xC1:			{ Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);									}	break; // MIX
		case 0xC4: case 0xC5: { RM=REG32; Bit_d=1; Bit_w=0; strcpy_s(RSize,StringLen(regSize[4])+1,regSize[4]);							}	break; // LES/LDS
		case 0xD0: case 0xD2: { Bit_d=0; Bit_w=0; strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]);									}	break; // MIX
		case 0xD1: case 0xD3: { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);									}	break; // MIXED
		case 0xD8:			{ UsesFPU=1; Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);						}	break; // FPU
		case 0xD9:{
			UsesFPU=1; Bit_d=0; Bit_w=0; 
				switch(REG){
					case 0: case 2: case 3:strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);	break; // DWORD (REAL4)
					case 4: case 6: strcpy_s(RSize,StringLen(regSize[6])+1,regSize[6]);			break; // 28Bytes                                       
					case 5: case 7: strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]);			break; // WORD (REAL2)
			}
		}
		break; // FPU

		case 0xDA: { UsesFPU=1; Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);											}	break; // FPU
		case 0xDB: { UsesFPU=1; Bit_d=0; Bit_w=0; if(REG<4) strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); else strcpy_s(RSize,StringLen(regSize[5])+1,regSize[5]);}	break; // FPU
		case 0xDC: { UsesFPU=1; Bit_d=0; Bit_w=0; strcpy_s(RSize,regSize[0]); }	break; // FPU
		case 0xDD: {
				UsesFPU=1; Bit_d=0; Bit_w=0;
				switch(REG){
					case 0: case 1: case 2: case 3: strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]);	break; // QWORD
					case 4: case 5: case 6: strcpy_s(RSize,StringLen(regSize[7])+1,regSize[7]); break; // (108)Byte
					case 7: strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]); break; // WORD
				}
		}
		break; // FPU

		case 0xDE: { UsesFPU=1; Bit_d=0; Bit_w=0; strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]); }		break; // WORD
		case 0xDF: {
				UsesFPU=1; Bit_d=0; Bit_w=0;
				switch(REG){
					case 0: case 1: case 2: case 3: strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]);	break; // WORD
					case 4: case 6: strcpy_s(RSize,StringLen(regSize[5])+1,regSize[5]);					break; // TByte
					case 5: case 7: strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]);					break; // QWord
				}
		}
		break;
		case 0xFE: { Bit_d=0; Bit_w=0; strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]); }	break; // BYTE
		case 0xFF: { 
			Bit_d=0; Bit_w=0; 
			if(REG==3 || REG==5){ // FAR JMP/CALL
				strcpy_s(RSize,StringLen(regSize[4])+1,regSize[4]); // FWORD
			}
			else{
				strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);
			}
		}
		break; // DWORD
	}

	// check for bit register size : 16bit/32bit
	if(Bit_w==1){
	   RM=REG32; // 32bit registers set
	   //if(bound==0 && Op==0x62)// Special Case
	   strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); // Dword ptr
	}

	// check for prefix 0x66 OverRide (change default size)
	if(PrefixReg==1){
		if(!UsesFPU){ // FPU DataSize doesn't Change, others are, on prefix 0x66.
			if(lstrcmp(RSize,"Byte")!=0){ // doesn't affect byte mode
				RM=REG16; // 16bit registers
				strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]); // word ptr
				if(Op==0x62 || Op==0xC4 || Op==0xC5){ // Special Case
					strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); // word ptr
				}
			}
		}
	}

	// 64-bit mode: REX.W promotes operand size to 64-bit
	if((*Disasm)->Mode64 && (*Disasm)->RexW && !UsesFPU){
		if(lstrcmp(RSize,"Byte")!=0){
			RM=REG64;
			strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); // Qword ptr
		}
	}

	// Special case: MOVSXD — source memory operand is always 32-bit
	if((*Disasm)->Mode64 && (BYTE)(*(*Opcode+pos))==0x63){
		strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); // Dword ptr (source is always 32-bit)
	}

	// 64-bit mode: CALL/JMP/PUSH indirect default to 64-bit operand size
	if((*Disasm)->Mode64 && (BYTE)(*(*Opcode+pos))==0xFF){
		if(REG==2 || REG==4 || REG==6){ // CALL, JMP, PUSH
			RM=REG64;
			strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); // Qword ptr
		}
	}

	// 64-bit mode: REX.R extends REG field
	if((*Disasm)->Mode64 && (*Disasm)->RexR){
		REG |= 0x08;
	}

	// 64-bit mode: default address size is 64-bit (unless 0x67 prefix)
	if((*Disasm)->Mode64){
		if(!PrefixAddr)
			ADDRM=REG64;
		else
			ADDRM=REG32; // 0x67 in 64-bit: use 32-bit addressing
	}

	// REX prefix present + byte mode: use REG8X (SPL/BPL/SIL/DIL)
	if((*Disasm)->Mode64 && (*Disasm)->RexPrefix && Bit_w==0){
		RM=REG8X;
	}

	// SCALE INDEX BASE :
	SIB=(BYTE)(*(*Opcode+pos+1))&0x07; // Get SIB extension
	/*
		Example:
		--------

		format of sib is:
		ss iii bbb.
		where ss is 2 upper bits for scale
		and they represent power (exponent) of 2 for
		scale index multiplier.
		iii is 3 middle bits for index.
		bbb is 3 low bits for base.

		*SIB == 4
		*NO SIB != 4

		0x04 -> 00 000 [100] <- SIB
		0x0C -> 00 001 [100] <- SIB
		0x64 -> 01 100 [100] <- SIB
		0x60 -> 01 100 [000] <- NO SIB
		0xB5 -> 10 110 [101] <- NO SIB
		0x76 -> 01 110 [110] <- NO SIB

		Extract SS II BB information (3rd byte)
		=======================================
		0x81,0xAC,0x20

		0x20 =  00 100 000

		Scale: 00 = *1 (not shown)
		100 - ESP = not Shown, Cannot be an Index register
		000 - EAX = shown

		if MOD 10/01 is being used, get displacement data after 
		the SIB.
	*/

	// =================================================//
	//				AddrPrefix is being used!			//
	// =================================================//

	if(PrefixAddr==1 && !(*Disasm)->Mode64){ // Prefix 0x67 in 32-bit mode: Change to 16-bit addressing
		FOpcode=((BYTE)(*(*Opcode+pos+1))&0x0F); // Get addressing Mode (8 types of mode)
		reg1=((BYTE)(*(*Opcode+pos+1))&0x38)>>3;

		// Check if we decode POP instruction, which had few valid instruction.
		if(Op==0x8F && reg1!=0){
			lstrcat((*Disasm)->Remarks,"Invalid Instruction");
		}
        
        // Choose Mode + Segment
		switch(FOpcode){
			case 0x00: case 0x08: wsprintf(Addr,"%s",addr16[0]); /*SEG=SEG_DS;*/	break; // Mode 0:[BX+SI]
			case 0x01: case 0x09: wsprintf(Addr,"%s",addr16[1]); /*SEG=SEG_DS;*/	break; // Mode 1:[BX+DI]
			case 0x02: case 0x0A: wsprintf(Addr,"%s",addr16[2]); SEG=SEG_SS;		break; // Mode 2:[BP+SI]
			case 0x03: case 0x0B: wsprintf(Addr,"%s",addr16[3]); SEG=SEG_SS;		break; // Mode 3:[BP+DI]
			case 0x04: case 0x0C: wsprintf(Addr,"%s",addr16[4]); /*SEG=SEG_DS;*/	break; // Mode 4:[SI]
			case 0x05: case 0x0D: wsprintf(Addr,"%s",addr16[5]); /*SEG=SEG_DS;*/	break; // Mode 5:[DI]
			case 0x06: case 0x0E:{ // Mode 6: [BP+XX/XXXX] | [XX]
				if(Extension==0){ // 0x00-0x3F only! has special [XXXX]
					/*SEG=SEG_DS;*/
					SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
					wsprintf(Addr,"%04X",wMem);
					(*(*index))+=2; // read 2 bytes
				}
				else{ // 0x50-0xBF has [BP+]
					SEG=SEG_SS; // SS Segment
					wsprintf(Addr,"%s",addr16[7]);
				}
			}
			break;
			case 0x07: case 0x0F: wsprintf(Addr,"%s",addr16[6]); /*SEG=SEG_DS;*/ break; // Mode 7: [BX]
		}

		// Choose used extension 
		// And Decode properly the menemonic
		switch(Extension){
			case 0:{ // No extension of bytes to RegMem (except mode 6)
				wsprintf(tempMeme,"%s ptr %s:[%s]",RSize,segs[SEG],Addr);
				SwapDword((BYTE*)(*Opcode+pos),&dwOp,&dwMem);
				SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
                
				if(((wOp&0x00FF)&0x0F)==0x06 || ((wOp&0x00FF)&0x0F)==0x0E){ // 0x00-0x3F with mode 6 only!
					wsprintf(menemonic,"%08X",dwOp);
					(*Disasm)->OpcodeSize=4;
					lstrcat((*Disasm)->Opcode,menemonic);
				}
				else{ // other modes
					wsprintf(menemonic,"%04X",wOp);
					(*Disasm)->OpcodeSize=2;
					lstrcat((*Disasm)->Opcode,menemonic);
				}
			}
			break;

			case 1:{ // 1 Byte Extension to regMem
				SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
				FOpcode=wOp&0x00FF;

				if(FOpcode>0x7F){ // check for signed numbers
					wsprintf(Aritmathic,"%s",Scale[0]); // '-' Signed Numbers
					FOpcode = 0x100-FOpcode; // -XX
				}
				wsprintf(menemonic,"%02X%04X",Op,wOp);
				lstrcat((*Disasm)->Opcode,menemonic);
				wsprintf(tempMeme,"%s ptr %s:[%s%s%02Xh]",RSize,segs[SEG],Addr,Aritmathic,FOpcode);
				++(*(*index)); // 1 byte read
				(*Disasm)->OpcodeSize=3;
			}
			break;
			
			case 2:{ // 2 Bytes Extension to RegMem
				SwapDword((BYTE*)(*Opcode+pos),&dwOp,&dwMem);
				SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
				wsprintf(menemonic,"%08X",dwOp);
				(*Disasm)->OpcodeSize=4;
				lstrcat((*Disasm)->Opcode,menemonic);
				wsprintf(tempMeme,"%s ptr %s:[%s%s%04Xh]",RSize,segs[SEG],Addr,Aritmathic,wMem);
				(*(*index))+=2; // we read 2 bytes
			}
			break;
		}

		// Switch Direction Mode.
		// And Build Mnemonic from that direction
		switch(Bit_d){
			case 0:{ // (->)
				// Check for More Mnemonics add ons
				switch(Op){ // Check for all Cases
					case 0x6B:{
						// We check Extension because there is a diff
						// Reading position of bytes depend on the extension
						// 1 = read Byte, 3rd position
						// 2 = read Dword, 6th position
						
						switch(Extension){
							case 1:{ // read 1 byte at 3rd position
								SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
								FOpcode=wOp&0x00FF;
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							case 2:{ //read byte at 7th position (dword read before)
								SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
								FOpcode=wOp&0x00FF;
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							default:{ // Extension==0
								SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
								FOpcode=wOp&0x00FF;
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
						}
                        
						if(FOpcode>0x7F){ // check for signed numbers!!
							FOpcode = 0x100-FOpcode; // -XX (Signed)
							wsprintf(Aritmathic,"%s",Scale[0]); // '-' arithmetic (Signed)
						}
						else{
							strcpy_s(Aritmathic,"");
						}

						strcpy_s(instruction,StringLen("imul")+1,"imul");
						wsprintf(temp,"%s %s,%s,%s%02Xh",instruction,regs[RM][reg2],tempMeme,Aritmathic,FOpcode);
						(*(*index))++;
						(*Disasm)->OpcodeSize++;
					}
					break;

                    case 0x81: case 0xC7: case 0x69:{
						// Get Extensions!
						switch(Extension){
							case 0:{
								if(PrefixReg==0){   
									SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
									wsprintf(temp," %08X",dwOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%08Xh",dwMem);
								}
								else{
									SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
									wsprintf(temp," %04X",wOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%04Xh",wMem);
								}
							}
							break;

							case 1:{
								if(PrefixReg==0){   
									SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
									wsprintf(temp," %08X",dwOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%08Xh",dwMem);
								}
								else{
									SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
									wsprintf(temp," %04X",wOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%04Xh",wMem);
								}
							}
							break;

							case 2:{
								if(PrefixReg==0){
									SwapDword((BYTE*)(*Opcode+pos+4),&dwOp,&dwMem);
									wsprintf(temp," %08X",dwOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%08Xh",dwMem);
								}
								else{
									SwapWord((BYTE*)(*Opcode+pos+4),&wOp,&wMem);
									wsprintf(temp," %04X",wOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%04Xh",wMem);
								}
							}
							break;
						}
                        
						if(Op==0xC7){
							/* 
								Instruction rule: Mem,Imm ->  1100011woo000mmm,imm
								Code Block: 1100011
								w = Reg Size
								oo - Mod
								000 - Must be!
								mmm - Reg/Mem
								imm - Immidiant (����)
							*/
                            
							if(reg1!=0){
								lstrcat((*Disasm)->Remarks,";Invalid Instruction");
							}

							wsprintf(instruction,"%s","mov");
						}
						else{
							if (Op==0x69){ // IMUL REG,MEM,IIM32
								wsprintf(instruction,"imul %s,",regs[RM][reg1]);
							}
							else{
								wsprintf(instruction,"%s",Instructions[REG]);
							}
						}
						wsprintf(menemonic,"%s %s,%s",instruction,tempMeme,temp);
						strcpy_s(temp,menemonic);
						(*(*index))+=4;
						(*Disasm)->OpcodeSize+=4;
					}
					break;

					case 0x80:case 0x82: case 0x83: case 0xC6:{
						// We check Extension because there is a diff
						// Reading position of bytes depend on the extension
						// 1 = read byte, 3rd position
						// 2 = read dword, 6th position
						
						switch(Extension){
							case 1:{ // read 1 byte at 3rd position
								SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
								FOpcode=wOp&0x00FF;
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							case 2:{ //read byte at 7th position (dword read before)
								SwapWord((BYTE*)(*Opcode+pos+4),&wOp,&wMem);
                                FOpcode=wOp&0x00FF;
                                wsprintf(temp,"%02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							default:{ // Extension==0
								SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
								FOpcode=wOp&0x00FF;
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
						}

						strcpy_s(Aritmathic,"");
						
						if(Op==0x82 || Op==0x83){
							if(FOpcode>0x7F){ // check for signed numbers
								wsprintf(Aritmathic,"%s",Scale[0]); // '-' Signed Numbers
								FOpcode = 0x100-FOpcode; // -XX (Negative the Number)
							}
						}
						// Check Opcode
						if(Op==0xC6){
							/* 
								Instruction rule: Mem,Imm ->  1100011woo000mmm,imm
								Code Block: 1100011
								w = Reg Size
								oo - Mod
								000 - Must be!
								mmm - Reg/Mem
								imm - Immidiant (����)
							*/
							// Check valid Opcode, must have 000 bit
							if(reg1!=0){
								lstrcat( (*Disasm)->Remarks,";Invalid Instruction!");
							}
							wsprintf(instruction,"%s","mov"); // Instruction
						}
						else{
							wsprintf(instruction,"%s",Instructions[REG]);
						}

						wsprintf(temp,"%s %s,%s%02Xh",instruction,tempMeme,Aritmathic,FOpcode);
						(*(*index))++;
						(*Disasm)->OpcodeSize++;
					}
					break;
					
					case 0x8C:{ // Segments in Source Register
						wsprintf(temp,"%s %s,%s",instruction,tempMeme,segs[REG]);
					}
					break;

					case 0xD0: case 0xD1:{
						wsprintf(temp,"%s %s,1",ArtimaticInstructions[REG],tempMeme);
					}
					break;

					case 0xD2: case 0xD3:{
						wsprintf(temp,"%s %s,cl",ArtimaticInstructions[REG],tempMeme);
					}
					break;

					case 0xD8: case 0xDC:{ // Unsigned FPU Instructions (unsigned)
						wsprintf(temp,"%s %s",FpuInstructions[REG],tempMeme);
					}
					break;

					case 0xD9:{ // FPU Instructions Set2 (UnSigned)
						if(REG==0 && reg1!=0){ // (11011001oo[000]mmm) must have 00 else invalid! fld instruction only
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						else{
							if(REG==1){ // no such fpu instruction!
								lstrcat((*Disasm)->Remarks,";Invalid Instruction");
							}
						}
						wsprintf(temp,"%s %s",FpuInstructionsSet2[REG],tempMeme);
					}
					break;

					case 0xDA: case 0xDE:{ // FPU Instructions (Signed)
						wsprintf(temp,"%s %s",FpuInstructionsSigned[REG],tempMeme);
					}
					break;

					case 0xDB:{ // FPU Instructions Set2 (Signed)
						if(REG==1 || REG==4 || REG==6){ // No such fpu instructions!
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						wsprintf(temp,"%s %s",FpuInstructionsSet2Signed[REG],tempMeme);
					}
					break;

					case 0xDD:{ // FPU Instructions Set2 (Signed)
						if(REG==1 ||  REG==5){ // no such fpu instruction!
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						wsprintf(temp,"%s %s",FpuInstructionsSet3[REG],tempMeme);
					}
					break;

					case 0xDF:{ // Extended FPU Instructions Set2 (Signed)
						if(REG==1){ // no such fpu instruction!
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						wsprintf(temp,"%s %s",FpuInstructionsSet2Signed_EX[REG],tempMeme);
					}
					break;

					case 0xF6:{
						// We check Extension because there is a diff
						// Reading position of bytes depend on the extension
						// 1 = read byte, 3rd position
						// 2 = read dword, 6th position
						switch(Extension){
							case 1:{
								// read 1 byte at 3rd position
								SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
								FOpcode=wOp&0x00FF;
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							case 2:{
								//read byte at 7th position (dword read before)
								SwapWord((BYTE*)(*Opcode+pos+4),&wOp,&wMem);
								FOpcode=wOp&0x00FF;
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							default:{
								// Extension==0
                                SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
                                FOpcode=wOp&0x00FF;
                                wsprintf(temp,"%02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,temp);
							}
						}

						strcpy_s(Aritmathic,"");
  				        wsprintf(instruction,"%s",InstructionsSet2[REG]);

						if(reg1==0 || reg1==1){
							wsprintf(temp,"%s %s,%s%02Xh",instruction,tempMeme,Aritmathic,FOpcode);
							(*(*index))++;
							(*Disasm)->OpcodeSize++;
						}
						else{
							wsprintf(temp,"%s %s",instruction,tempMeme);
						}
					}
					break;

					case 0xF7:{
						// get instruction
						wsprintf(instruction,"%s",InstructionsSet2[REG]);

						// Get Extensions!
						if(reg1==0 || reg1==1){
							switch(Extension){
								case 0:{
									if(PrefixReg==0){   
										SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
								break;

								case 1:{
									if(PrefixReg==0){
										SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
								break;

								case 2:{
									if(PrefixReg==0){   
										SwapDword((BYTE*)(*Opcode+pos+4),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+4),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
								break;
							}

							wsprintf(menemonic,"%s %s,%s",instruction,tempMeme,temp);
							(*(*index))+=4;
							(*Disasm)->OpcodeSize+=4;
						}
						else{
							wsprintf(menemonic,"%s %s",instruction,tempMeme);
						}
						strcpy_s(temp,menemonic);
				}
				break;

				case 0xFE:{ // MIX Instructions (INC,DEC,INVALID,INVALID,INVALID...)
					wsprintf(temp,"%s %s",InstructionsSet3[REG],tempMeme);
					if(REG>1){ // Invalid instructions
						lstrcat((*Disasm)->Remarks,";Invalid Instruction");
					}
				}
				break;

				case 0xFF:{ // MIX Instructions (INC,DEC,CALL,PUSH,JMP,FAR JMP,FAR CALL,INVALID)
					wsprintf(temp,"%s %s",InstructionsSet4[REG],tempMeme);
					switch(REG){
						case 3:{  // FAR CALL
							lstrcat((*Disasm)->Remarks,";Far Call");
						}
						break;

						case 5:{ // FAR JUMP
							lstrcat((*Disasm)->Remarks,";Far Jump");
						}
						break;

						case 7:{ // Invalid instructions
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						break;
					}
				}
				break;

				default:{
					wsprintf(temp,"%s %s,%s",instruction,tempMeme,regs[RM][REG]);
				}
				break;
			}
				
			lstrcat((*Disasm)->Assembly,temp);
			/*
				wsprintf(menemonic,"%s %s,%s",instruction,tempMeme,regs[RM][REG]);
				lstrcat((*Disasm)->Assembly,menemonic);
			*/
		}
		break;

		case 1:{ // (<-) Direction (Bit_D)
			// Check Used Opcode Set
			switch(Op){
				case 0x8E:{ // Segments in Destination Register
					wsprintf(menemonic,"%s %s,%s",instruction,segs[REG],tempMeme);
				}
				break;

				// Mixed Bit Rotation Instructions (rol/ror/shl..)
				case 0xC0: case 0xC1:{
					// Check Extension
					switch(Extension){
						case 0:{ // No Extension
							SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
							FOpcode=wOp&0x00FF;
							wsprintf(menemonic,"%s %s,%02Xh",ArtimaticInstructions[REG],tempMeme,FOpcode);
							wsprintf(tempMeme," %02X",FOpcode);
							lstrcat((*Disasm)->Opcode,tempMeme);
							(*(*index))++;
							(*Disasm)->OpcodeSize++;
						}
						break;

						case 1:{ // 1 byte Extension (Displacement)
							SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
							FOpcode=wOp&0x00FF;
							wsprintf(menemonic,"%s %s,%02Xh",ArtimaticInstructions[REG],tempMeme,FOpcode);
							wsprintf(tempMeme," %02X",FOpcode);
							lstrcat((*Disasm)->Opcode,tempMeme);
							(*(*index))++;
							(*Disasm)->OpcodeSize++;
						}
						break;

						case 2:{ // 2 Bytes Extension (Displacement)
							SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
							FOpcode=wOp&0x00FF;
							wsprintf(menemonic,"%s %s,%02Xh",ArtimaticInstructions[REG],tempMeme,FOpcode);
							wsprintf(tempMeme," %02X",FOpcode);
							lstrcat((*Disasm)->Opcode,tempMeme);
							(*(*index))++;
							(*Disasm)->OpcodeSize++;
						}
						break;
                    }
				}
				break;

				// POP DWORD PTR[REG/MEM/DISP]
				case 0x8F:{
					wsprintf(menemonic,"%s %s",instruction,tempMeme);
				}
				break;

				case 0xC4:{ // LES
					strcpy_s(instruction,StringLen("les")+1,"les");
					wsprintf(menemonic,"%s %s,%s",instruction,regs[RM][REG],tempMeme);
				}
				break;

				case 0xC5:{ // LDS
					strcpy_s(instruction,StringLen("lds")+1,"lds");
					wsprintf(menemonic,"%s %s,%s",instruction,regs[RM][REG],tempMeme);
				}
				break;

				// Default Decode, using regular registers
				default:{
					wsprintf(menemonic,"%s %s,%s",instruction,regs[RM][REG],tempMeme);
				}
				break;
			}

			strcpy_s(tempMeme,StringLen(menemonic)+1,menemonic);
			lstrcat((*Disasm)->Assembly,tempMeme);
		}
		break;
		}

		++(*(*index)); // add 1 byte to index
		// no need to continue!! exit the function and proeed with decoding next bytes.
		return;
	}

	//===================================================//
	//============== NO SIB Being used! =================//
	//===================================================//

	if(SIB!=SIB_EX){ // NO SIB extension (i.e: 0x0001 = add byte ptr [ecx], al)
		reg1=((BYTE)(*(*Opcode+pos+1))&0x07); // Get the register (we have only one)
		reg2=(((BYTE)(*(*Opcode+pos+1))&0x38)>>3);

		// 64-bit mode: apply REX.B to r/m field
		if((*Disasm)->Mode64 && (*Disasm)->RexB){
			reg1 |= 0x08;
		}

		// Check for valid/invalid pop instruction,
		// pop insteruction must have reg bit 000
		if(Op==0x8F && reg2!=0){
			lstrcat((*Disasm)->Remarks,"; Invalid Instruction");
		}

		switch(Extension){ // Check what extension we have (None/Byte/Dword)
			case 0:{ // no extention to regMem
				if((reg1&0x07)==REG_EBP){ // mod=00 rm=5: [disp32] or [RIP+disp32]
                    SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
					SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
					Address=dwMem;
                    wsprintf(menemonic,"%04X%08X",wOp,dwOp);
					lstrcat((*Disasm)->Opcode,menemonic);
					if((*Disasm)->Mode64){
						// 64-bit mode: RIP-relative addressing
						wsprintf(instr,"%08Xh",dwMem);
						wsprintf(menemonic,"%s ptr [RIP+%s]",RSize,instr);
					}else{
						wsprintf(instr,"%08Xh",dwMem);
						wsprintf(menemonic,"%s ptr %s:[%s]",RSize,segs[SEG],instr);
					}
					(*Disasm)->OpcodeSize=6;
					(*(*index))+=5;
				}
				else{
                    SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
					wsprintf(menemonic,"%04X",wOp);
					lstrcat((*Disasm)->Opcode,menemonic);
					wsprintf(menemonic,"%s ptr %s:[%s]",RSize,segs[SEG],regs[ADDRM][reg1]);
                    ++(*(*index)); // only 1 byte read
                    (*Disasm)->OpcodeSize=2; // total used opcodes
				}
			}
			break;
			
			case 1:{ // 1 btye extention to regMem
                SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
				wsprintf(menemonic,"%02X%04X",Op,wOp);
				lstrcat((*Disasm)->Opcode,menemonic);
                FOpcode=wOp&0xFF; // get lower part of word.

				if(FOpcode>0x7F){ // check for signed numbers
					wsprintf(Aritmathic,"%s",Scale[0]); // '-' aritmathic
					FOpcode = 0x100-FOpcode; // -XX
				}
				
				if(reg1==REG_EBP && PrefixSeg==0){
					SEG=SEG_SS;
				}
				
                wsprintf(menemonic,"%s ptr %s:[%s%s%02Xh]",RSize,segs[SEG],regs[ADDRM][reg1],Aritmathic,FOpcode);
                (*(*index))+=2; // x + 1 byte(s) read
                (*Disasm)->OpcodeSize=3; // total used opcodes
			}
			break;
			
			case 2:{ // 4 btye extention to regMem
				// if ebp and there is no prefix 0x67, use SS segment
				if(reg1==REG_EBP && PrefixSeg==0){
					SEG=SEG_SS;
				}

                SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
				SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
                Address=dwMem;
                wsprintf(menemonic,"%04X %08X",wOp,dwOp);
				lstrcat((*Disasm)->Opcode,menemonic);
				wsprintf(instr,"%08Xh",dwMem);
				wsprintf(menemonic,"%s ptr %s:[%s+%s]",RSize,segs[SEG],regs[ADDRM][reg1],instr);
				(*(*index))+=5; // x + 1 + 4 byte(s) read
                (*Disasm)->OpcodeSize=6; // total used opcodes
			}
			break;
			//case 02:break;
		}
		// check direction of menemonic
		switch(Bit_d){
			case 0:{ // (->) Direction
				// Check for More Menemonics Addons
				switch(Op){ // Check for all Cases Availble
					case 0x6B:{
                        // We check Extension because there is a diff
						// Reading position of bytes depend on the extension
						// 1 = read byte, 3rd position
						// 2 = read dword, 6th position

						switch(Extension){
							case 1:{
								// read 1 byte at 3rd position
								FOpcode=(BYTE)(*(*Opcode+pos+3));
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							case 2:{
								// read byte at 7th position (dword read before)
								FOpcode=(BYTE)(*(*Opcode+pos+6));
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							default:{
								FOpcode=(BYTE)(*(*Opcode+pos+2));
                                wsprintf(temp,"%02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,temp);
							}
						}
                        
                        if(FOpcode>0x7F){ // check for signed numbers!!
                            FOpcode = 0x100-FOpcode; // -XX
                            wsprintf(Aritmathic,"%s",Scale[0]); // '-' aritmathic (Signed)
                        }
						else{
						   strcpy_s(Aritmathic,"");
						}

                        strcpy_s(instruction,StringLen("imul")+1,"imul");
						wsprintf(tempMeme,"%s %s,%s,%s%02Xh",instruction,regs[RM][reg2],menemonic,Aritmathic,FOpcode);
						(*(*index))++;
						(*Disasm)->OpcodeSize++;
                    }
					break;

					case 0x81: case 0xC7: case 0x69:{ // Opcode 0x81/0xC7/0x69
						switch(Extension){  // Decode  byte extensions!
							case 1:{ // 1 byte extersion
								if(PrefixReg==0){
									SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
									wsprintf(temp," %08X",dwOp);
									Address=dwMem;
									PushString=TRUE;
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%08Xh",dwMem);
								}
								else{
									SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
									wsprintf(temp," %04X",wOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%04Xh",wMem);
								}
							}
							break;

							case 2:{ // 4 bytes Extensions
								if(PrefixReg==0){
                                    SwapDword((BYTE*)(*Opcode+pos+6),&dwOp,&dwMem);
									Address=dwMem;
									PushString=TRUE;
									wsprintf(temp," %08X",dwOp);
                                    lstrcat((*Disasm)->Opcode,temp);
                                    wsprintf(temp,"%08Xh",dwMem);
                                }
								else{ //0x66 prefix
									SwapWord((BYTE*)(*Opcode+pos+6),&wOp,&wMem);
									wsprintf(temp," %04X",wOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%04Xh",wMem);
                                }
							}
							break;

							default:{ // No Extension!
								if(PrefixReg==0){   
									if(reg1==REG_EBP){
										SwapDword((BYTE*)(*Opcode+pos+6),&dwOp,&dwMem);
										Address=dwMem;
										PushString=TRUE;
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp," %08Xh",dwMem);
									}
								}
								else{ // 0x66 prefix
									if(reg1==REG_EBP){
										SwapWord((BYTE*)(*Opcode+pos+6),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
									else{   
										SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
							}
						}

                        if(Op==0xC7){
							/* 
								Instruction rule: Mem,Imm ->  1100011woo000mmm,imm
								Code Block: 1100011
								w = Reg Size
								oo - Mod
								000 - Must be!
								mmm - Reg/Mem
								imm - Immidiant (����)
							*/
							if(reg2!=0){
								lstrcat((*Disasm)->Remarks,";Invalid Instruction");
							}

							wsprintf(instruction,"%s","mov");
						}
						else{
							if(Op==0x69){ // IMUL REG,MEM,IIM
								wsprintf(instruction,"imul %s,",regs[RM][reg2]);
							}
							else{
								wsprintf(instruction,"%s",Instructions[REG]);
							}
						}
                        
						wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,temp);
						if(PrefixReg==0){                        
							(*(*index))+=4;
							(*Disasm)->OpcodeSize+=4;
						}
						else{
							(*(*index))+=2;
							(*Disasm)->OpcodeSize+=2;
						}
					}
					break;

					case 0x80:case 0x82: case 0x83: case 0xC6:{
						// We check Extension because there is a diff
						// Reading position of bytes depend on the extension
						// 1 = read byte, 3rd position
						// 2 = read dword, 6th position

						switch(Extension){
							case 1:{ // read 1 byte at 3rd position
								FOpcode=(BYTE)(*(*Opcode+pos+3));
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							case 2:{ // read byte at 7th position (dword read before)
								FOpcode=(BYTE)(*(*Opcode+pos+6));
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							default:{
								if(reg1==REG_EBP){
									FOpcode=(BYTE)(*(*Opcode+pos+6));
								}
								else{
									FOpcode=(BYTE)(*(*Opcode+pos+2));
								}

								wsprintf(temp," %02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
						}

						strcpy_s(Aritmathic,"");
						
						// Opcodes with signed number
						if(Op==0x82 || Op==0x83){
							if(FOpcode>0x7F){ // check for signed numbers
								wsprintf(Aritmathic,"%s",Scale[0]); // '-' aritmathic
								FOpcode = 0x100-FOpcode; // -XX (Negative the Number)
							} 
						}

						// C6 Code Block Opcodes is Mov!
						if(Op==0xC6){
							/* 
							Instruction rule: Mem,Imm ->  1100011woo000mmm,imm
							Code Block: 1100011
							w = Reg Size
							oo - Mod
							000 - Must be!
							mmm - Reg/Mem
							imm - Immidiant (����)
							*/
							if(reg2!=0){
								lstrcat((*Disasm)->Remarks,";Invalid Instruction");
							}

							wsprintf(instruction,"%s","mov");  
                        }
						else{ // Others Opcode we decode from the instruction tables
							wsprintf(instruction,"%s",Instructions[REG]);
						}

						wsprintf(tempMeme,"%s %s,%s%02Xh",instruction,menemonic,Aritmathic,FOpcode);
						(*(*index))++;
						(*Disasm)->OpcodeSize++;
					}
					break;

					case 0x8C:{ // Segments in Source Register
						if(REG>5){ // invalid segment
							lstrcat((*Disasm)->Remarks,";Invalid Segment Usage");
						}
						wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,segs[REG]);
					}
					break;

					case 0xD0: case 0xD1:{
						strcpy_s(instruction,StringLen(ArtimaticInstructions[REG])+1,ArtimaticInstructions[REG]);
						wsprintf(tempMeme,"%s %s,1",instruction,menemonic);
					}
					break;

					case 0xD2: case 0xD3:{
						strcpy_s(instruction,StringLen(ArtimaticInstructions[REG])+1,ArtimaticInstructions[REG]);
						wsprintf(tempMeme,"%s %s,cl",instruction,menemonic);
					}
					break;

					case 0xD8: case 0xDC:{ // FPU Instruction (unsigned instructions)
						wsprintf(tempMeme,"%s %s",FpuInstructions[REG],menemonic);
					}
					break;

					case 0xD9:{ // FPU Instructions Set2 (UnSigned)
						if(REG==0 && reg2!=0){ // (11011001oo[000]mmm) must have 00 else invalid fld instruction only
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						else{
							if(REG==1){ // no such fpu instruction!
								lstrcat((*Disasm)->Remarks,";Invalid Instruction");
							}
						}
						wsprintf(tempMeme,"%s %s",FpuInstructionsSet2[REG],menemonic);
					}
					break;

					case 0xDA: case 0xDE:{ // FPU Instructions (Signed)
						wsprintf(tempMeme,"%s %s",FpuInstructionsSigned[REG],menemonic);
					}
					break;

					case 0xDB:{ // FPU Instructions Set2 (Signed)
						if(REG==1 || REG==4 || REG==6){ // no such fpu instruction!
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						wsprintf(tempMeme,"%s %s",FpuInstructionsSet2Signed[REG],menemonic);
					}
					break;

					case 0xDD:{ // FPU Instructions Set3
						if(REG==1 ||  REG==5){ // no such fpu instruction!
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						wsprintf(tempMeme,"%s %s",FpuInstructionsSet3[REG],menemonic);
					}
					break;

					case 0xDF:{ // Extended FPU Instructions Set2 (Signed)
						if(REG==1){ // no such fpu instruction!
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						wsprintf(tempMeme,"%s %s",FpuInstructionsSet2Signed_EX[REG],menemonic);
					}
					break;

                    case 0xF6:{ // MIXED Instructions (MUL,DIV,NOT...)
						// We check Extension because there is a diff
						// Reading position of bytes depend on the extension
						// 1 = read byte, 3rd position
						// 2 = read dword, 6th position

						switch(Extension){
							case 1:{ // read 1 byte at 3rd position
								if(reg2==0 || reg2==1){ // TEST Only
								   FOpcode=(BYTE)(*(*Opcode+pos+3));
								   wsprintf(temp,"%02X",FOpcode);
								   lstrcat((*Disasm)->Opcode,temp);
								}
							}
							break;

							case 2:{ // read byte at 7th position (dword read before)
								if(reg2==0 || reg2==1){ // TEST Only
									FOpcode=(BYTE)(*(*Opcode+pos+6));
									wsprintf(temp,"%02X",FOpcode);
									lstrcat((*Disasm)->Opcode,temp);
								}
							}
							break;

							default:{
								if(reg2==0 || reg2==1){ // TEST Only
									FOpcode=(BYTE)(*(*Opcode+pos+2));
									wsprintf(temp,"%02X",FOpcode);
									lstrcat((*Disasm)->Opcode,temp);
								}
							}
						}
                            
						strcpy_s(Aritmathic,"");
						wsprintf(instruction,"%s",InstructionsSet2[REG]);   
						if(reg2==0 || reg2==1){ // TEST instruction
							wsprintf(tempMeme,"%s %s,%s%02Xh",instruction,menemonic,Aritmathic,FOpcode);
							(*(*index))++;
							(*Disasm)->OpcodeSize++;
						}
						else{ // NOT/NEG/MUL/IMUL/DIV/IDIV instruction must not have operands
							wsprintf(tempMeme,"%s %s",instruction,menemonic);
						}
					}
					break;

					case 0xF7:{
						wsprintf(instruction,"%s",InstructionsSet2[REG]); // Get Instruction
                        
						// Get Extensions!//
						if(reg2==0 || reg2==1){ // TEST Instruction
							switch(Extension){
								case 1:{ // 1 byte extersion
									if(PrefixReg==0){   
										SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
								break;

								case 2:{ // 4 bytes Extensions
									if(PrefixReg==0){   
										SwapDword((BYTE*)(*Opcode+pos+6),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+6),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
								break;

								default:{ // No Extension!  (check ebp)
									if(PrefixReg==0){
										if(reg1==REG_EBP){
											SwapDword((BYTE*)(*Opcode+pos+6),&dwOp,&dwMem);
											wsprintf(temp," %08X",dwOp);
											lstrcat((*Disasm)->Opcode,temp);
											wsprintf(temp,"%08Xh",dwMem);
										}
										else{
											SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
											wsprintf(temp," %08X",dwOp);
											lstrcat((*Disasm)->Opcode,temp);
											wsprintf(temp,"%08Xh",dwMem);
										}
									}
									else{
										if(reg1==REG_EBP){
											SwapWord((BYTE*)(*Opcode+pos+6),&wOp,&wMem);
											wsprintf(temp," %04X",wOp);
											lstrcat((*Disasm)->Opcode,temp);
											wsprintf(temp,"%04Xh",wMem);
										}
										else{
											SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
											wsprintf(temp," %04X",wOp);
											lstrcat((*Disasm)->Opcode,temp);
											wsprintf(temp,"%04Xh",wMem);
										}
									}
								}
							}

							wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,temp);
							if(PrefixReg==0){
								(*(*index))+=4;
								(*Disasm)->OpcodeSize+=4;
							}
							else{
								(*(*index))+=2;
								(*Disasm)->OpcodeSize+=2;
							}
                        }
						else{
							wsprintf(tempMeme,"%s %s",instruction,menemonic);
						}
                    }
                    break;

					case 0xFE:{ // MIX Instructions (INC,DEC,INVALID,INVALID,INVALID...)
						wsprintf(tempMeme,"%s %s",InstructionsSet3[REG],menemonic);
						if(REG>1){ // Invalid instructions
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
					}
					break;

					case 0xFF:{ // MIX Instructions (INC,DEC,CALL,PUSH,JMP,FAR JMP,FAR CALL,INVALID)
						wsprintf(tempMeme,"%s %s",InstructionsSet4[REG],menemonic);
                        
						if(reg1==REG_EBP && REG==4){
							JumpApi=TRUE;
						}
						else{
							if(reg1==REG_EBP && REG==2){
								CallAddrApi=TRUE;
							}
						}

						if(REG==3){ // FAR CALL
							lstrcat((*Disasm)->Remarks,";Far Call");
							break;
						}

						if(REG==5){ // FAR JUMP
							lstrcat((*Disasm)->Remarks,";Far Jump");
							break;
						}

						if(REG==7){ // Invalid instructions
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
					}
					break;

					// Decode non imm8/16/32 source opcodes
					// i.e: mov dword ptr[eax],eax
					default:{
						wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,regs[RM][REG]);
					}
					break;
				}

				lstrcat((*Disasm)->Assembly,tempMeme);
			}
			break;
			
			case 1:{ // (<-) Direction of decoding
				switch(Op){
					case 0x8E:{ // Segments in Destination Register
						wsprintf(tempMeme,"%s %s,%s",instruction,segs[REG],menemonic);
					}
					break;

					case 0x8F:{ // POP DWORD PTR[REG/MEM/DISP]
						wsprintf(tempMeme,"%s %s",instruction,menemonic);
					}
					break;

					// Mixed Bit Rotation Instructions (rol/ror/shl..)
					case 0xC0:case 0xC1:{
						switch(Extension) {
							case 0:{
								if(reg1==REG_EBP){
									FOpcode=(BYTE)(*(*Opcode+pos+6));
								}
								else{
									FOpcode=(BYTE)(*(*Opcode+pos+2));
								}
							}
							break;
							case 1: FOpcode=(BYTE)(*(*Opcode+pos+3)); break;
							case 2: FOpcode=(BYTE)(*(*Opcode+pos+6)); break;
						}
						wsprintf(tempMeme,"%s %s,%02Xh",ArtimaticInstructions[REG],menemonic,FOpcode);
						wsprintf(menemonic," %02X",FOpcode);
						lstrcat((*Disasm)->Opcode,menemonic);
						(*(*index))++;
						(*Disasm)->OpcodeSize++;
					}
					break;
                    
					case 0xC4:{
						strcpy_s(instruction,StringLen("les")+1,"les");
						wsprintf(tempMeme,"%s %s,%s",instruction,regs[RM][REG],menemonic);
					}
					break;

					case 0xC5:{
						strcpy_s(instruction,StringLen("lds")+1,"lds");
						wsprintf(tempMeme,"%s %s,%s",instruction,regs[RM][REG],menemonic);
					}
					break;

					// Default Decode
					default:{
						PushString=TRUE;   // e.g:LEA EDI,DWORD PTR [00401234] -> "LALALA"
						wsprintf(tempMeme,"%s %s,%s",instruction,regs[RM][REG],menemonic);
					}
					break;
				}
				lstrcat((*Disasm)->Assembly,tempMeme);
			}
			break;
		}
		return;
	}

    //////////////////////////////////////////////////////    
    //				SIB is being used!					//
    //////////////////////////////////////////////////////
    
	else if(SIB==SIB_EX){ // Found SIB, lets strip the extensions
		/* 
		   Example menemonic for SIB: 
		   Opcodes:		000401
		   Menemonic:	add byte ptr [eax+ecx], al
		   Binary:		0000 0000 0000 0100 0000 0001
        */
		reg1=((BYTE)(*(*Opcode+pos+2))&0x38)>>3;  // Register A
		reg2=((BYTE)(*(*Opcode+pos+2))&0x07);     // Register B
		SCALE=((BYTE)(*(*Opcode+pos+2))&0xC0)>>6; // Scale size (0,2,4,8)

		/* 
			Check for valid/invalid pop instruction,
			pop insteruction must have reg bit 000
			pop code/ModRM:

			Code Block: 1000 1111
			Mod/RM: oo000mmm 
			oo - Mod
			000 - Must be 0
			mmm - <reg>
		*/

		if(Op==0x8F){
			if( (((BYTE)(*(*Opcode+pos+1))&0x38)>>3)!=0 ){ // check 000
				lstrcat((*Disasm)->Remarks,";Invalid Instruction");
			}
		}

		// Scale look up.
		switch(SCALE){
			case 0: wsprintf(Aritmathic,"%s",Scale[1]); break; // +
			case 1: wsprintf(Aritmathic,"%s",Scale[2]); break; // *2+
			case 2: wsprintf(Aritmathic,"%s",Scale[3]); break; // *4+
			case 3: wsprintf(Aritmathic,"%s",Scale[4]); break; // *8+
		}

		switch(Extension){ // +/+00/+00000000
			case 00:{ // No extension of bytes
				if(reg1==REG_ESP && reg2!=REG_EBP){
					if(reg2==REG_ESP) SEG=SEG_SS; // IF ESP is being used, User SS Segment Overridr
					SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
					wsprintf(menemonic,"%02X%04X",Op,wOp);
					lstrcat((*Disasm)->Opcode,menemonic);
					wsprintf(menemonic,"%s ptr %s:[%s]",RSize,segs[SEG],regs[ADDRM][reg2]);
					(*(*index))+=2; //2 byte read				
					(*Disasm)->OpcodeSize=3; // total used opcodes
				}
				else if(reg2!=REG_EBP){ // No EBP in RegMem
					if(reg2==REG_ESP){
						SEG=SEG_SS; // IF ESP is being used, User SS Segment Override
					}
					SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
					wsprintf(menemonic,"%02X%04X",Op,wOp);
					lstrcat((*Disasm)->Opcode,menemonic);
					wsprintf(menemonic,"%s ptr %s:[%s%s%s]",RSize,segs[SEG],regs[ADDRM][reg1],Aritmathic,regs[ADDRM][reg2]);
					(*(*index))+=2; //2 byte read
					(*Disasm)->OpcodeSize=3; // total used opcodes
				}
				else if(reg2==REG_EBP){ // Replace EBP with Dword Number
					// get 4 bytes extensions for memReg addon
					// insted of Normal Registers

					// Format Opcodes (HEX)
					SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
					SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
					wsprintf(menemonic,"%02X %04X %08X",Op,wOp,dwOp);
					lstrcat((*Disasm)->Opcode,menemonic);
					// Format menemonic

					// Check If if ESP is being Used.
					if(reg1==REG_ESP){ // Must Not Be ESP (Index)
						wsprintf(temp,"");
						strcpy_s(Aritmathic,"");
					}
					else{
						wsprintf(temp,regs[ADDRM][reg1]);
					}

					wsprintf(menemonic,"%s ptr %s:[%s%s%08Xh]",
						RSize,		// size of regmem
						segs[SEG],	// segment
						temp,		// reg
						Aritmathic,	// +,-,*2,*4,*8
						dwMem);		// extensions

					Extension=2; // OverRide Extension (?????), Check toDo.txt
					(*(*index))+=6; //6 byte read
					(*Disasm)->OpcodeSize=7; // total used opcodes
				}
			}
			break;
            
			case 01:{ // 1 byte extension
				FOpcode=(BYTE)(*(*Opcode+pos+3));
				if(FOpcode>0x7F){ // check for signed numbers!!
					wsprintf(tempAritmathic,"%s",Scale[0]); // '-' aritmathic
					FOpcode = 0x100-FOpcode; // -XX
				}

				if(/*reg2==REG_EBP ||*/ reg1==REG_ESP){ // no ESP in [Mem]
					SEG=SEG_SS;
					// added REG+Arithmatic [21.3.2004]
					//wsprintf(tempMeme,"%s ptr %s:[%s%s%s%s%02Xh]",RSize,segs[SEG],regs[ADDRM][reg2],tempAritmathic,regs[ADDRM][reg1],tempAritmathic,FOpcode);
					wsprintf(tempMeme,"%s ptr %s:[%s%s%02Xh]",RSize,segs[SEG],regs[ADDRM][reg1],tempAritmathic,FOpcode);
				}
				else{
					wsprintf(tempMeme,"%s ptr %s:[%s%s%s%s%02Xh]",RSize,segs[SEG],regs[ADDRM][reg1],Aritmathic,regs[ADDRM][reg2],tempAritmathic,FOpcode);
				}

				(*(*index))+=3; // x + 3 byte(s) read
				SwapDword((BYTE*)(*Opcode+pos),&dwOp,&dwMem);
				wsprintf(menemonic,"%08X",dwOp);
				lstrcat((*Disasm)->Opcode,menemonic);
				(*Disasm)->OpcodeSize=4; // total used opcodes
				strcpy_s(menemonic,StringLen(tempMeme)+1,tempMeme);
			}
			break;

			case 02:{ // Dword extension
				SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
				SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
				
				// Mnemonic decode
				if(reg1!=REG_ESP){
					if(reg2==REG_EBP || reg2==REG_ESP){
						SEG=SEG_SS;
					}

					wsprintf(tempMeme,"%s ptr %s:[%s%s%s%s%08Xh]",
						RSize,  // size of register
						segs[SEG], // segment
						regs[ADDRM][reg1],
						Aritmathic,
						regs[ADDRM][reg2],
						tempAritmathic,
						dwMem);	
				}
				else{// ESP Must not be as Index, Code = 100b
					if(reg2==REG_ESP){
						SEG=SEG_SS;
					}

					wsprintf(tempMeme,"%s ptr %s:[%s%s%08Xh]",
						RSize,  // size of register
						segs[SEG], // segment
						regs[ADDRM][reg2],
						tempAritmathic,
						dwMem);
				}
				// Format Opcode
				wsprintf(menemonic,"%02X %04X %08X",Op,wOp,dwOp);
				lstrcat((*Disasm)->Opcode,menemonic);
				(*(*index))+=6; // x + 3 byte(s) read	
				(*Disasm)->OpcodeSize=7; // total used opcodes
				strcpy_s(menemonic,StringLen(tempMeme)+1,tempMeme);
			}
			break;
		}

		// Finish up the opcode with position of target register
		switch(Bit_d){
			case 0:{ // (->) Direction
				/*
				wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,regs[RM][REG]);
				lstrcat((*Disasm)->Assembly,tempMeme);
				*/
				// Check for More Menemonics Addons
				switch(Op){ // Check for all Cases
                    case 0x6B:{
                        // We check Extension because there is a diff
                        // Reading position of bytes depend on the extension
                        // 1 = read byte, 3rd position
                        // 2 = read dword, 6th position
                        
						switch(Extension){
							case 1:{ // read 1 byte at 3rd position
								FOpcode=(BYTE)(*(*Opcode+pos+3));
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							case 2:{ // read byte at 7th position (dword read before)
								FOpcode=(BYTE)(*(*Opcode+pos+7));
                                wsprintf(temp,"%02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							default:{
								FOpcode=(BYTE)(*(*Opcode+pos+2));
                                wsprintf(temp,"%02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,temp);
							}
						}
                        
                        if(FOpcode>0x7F){ // check for signed numbers!!
                            FOpcode = 0x100-FOpcode; // -XX
                            wsprintf(Aritmathic,"%s",Scale[0]); // '-' aritmathic (Signed)
                        }
						else{
                            strcpy_s(Aritmathic,"");
						}
                        
						BYTE Level=(BYTE)(*(*Opcode+pos+1));
						if( (Level>=0x00 && Level<=0x07) || (Level>=0x40 && Level<=0x47) || (Level>=0x80 && Level<=0x87) ){
							reg2=0;
						}
						
						if( (Level>=0x08 && Level<=0x0F) || (Level>=0x48 && Level<=0x4F) || (Level>=0x88 && Level<=0x8F) ){
							reg2=1;
						}
						
						if( (Level>=0x10 && Level<=0x17) || (Level>=0x50 && Level<=0x57) || (Level>=0x90 && Level<=0x97) ){
							reg2=2;
						}
						
						if( (Level>=0x18 && Level<=0x1F) || (Level>=0x58 && Level<=0x5F) || (Level>=0x98 && Level<=0x9F) ){
							reg2=3;
						}
						
						if( (Level>=0x20 && Level<=0x27) || (Level>=0x60 && Level<=0x67) || (Level>=0xA0 && Level<=0xA7) ){
							reg2=4;
						}
						
						if( (Level>=0x28 && Level<=0x2F) || (Level>=0x68 && Level<=0x6F) || (Level>=0xA8 && Level<=0xAF) ){
							reg2=5;
						}
						
						if( (Level>=0x30 && Level<=0x37) || (Level>=0x70 && Level<=0x77) || (Level>=0xB0 && Level<=0xB7) ){
							reg2=6;
						}
						
						if( (Level>=0x38 && Level<=0x3F) || (Level>=0x78 && Level<=0x7F) || (Level>=0xB8 && Level<=0xBF) ){
							reg2=7;
						}

                        strcpy_s(instruction,StringLen("imul")+1,"imul");
                        wsprintf(tempMeme,"%s %s,%s,%s%02Xh",instruction,regs[RM][reg2],menemonic,Aritmathic,FOpcode);
                        (*(*index))++;
						(*Disasm)->OpcodeSize++;
                    }
                    break;

                    case 0x81: case 0xC7: case 0x69:{ // Opcodes 0x81/0xC7/0x69
                        // Get Extensions!

						switch(Extension){
							case 1:{ // 1 byte extersion
								if(PrefixReg==0){ // No Reg Prefix
								   SwapDword((BYTE*)(*Opcode+pos+4),&dwOp,&dwMem);
								   Address=dwMem;
								   PushString=TRUE;
								   wsprintf(temp," %08X",dwOp);
								   lstrcat((*Disasm)->Opcode,temp);
								   wsprintf(temp,"%08Xh",dwMem);
								}
								else{
								   SwapWord((BYTE*)(*Opcode+pos+4),&wOp,&wMem);
								   wsprintf(temp," %04X",wOp);
								   lstrcat((*Disasm)->Opcode,temp);
								   wsprintf(temp,"%04Xh",wMem);
								}
							}
							break;

							case 2:{ // 4 bytes Extensions
								if(PrefixReg==0){ // No Reg Prefix
									SwapDword((BYTE*)(*Opcode+pos+7),&dwOp,&dwMem);
									Address=dwMem;
									PushString=TRUE;
									wsprintf(temp," %08X",dwOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%08Xh",dwMem);
								}
								else{
									SwapWord((BYTE*)(*Opcode+pos+7),&wOp,&wMem);
									wsprintf(temp," %04X",wOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%04Xh",wMem);
								}
							}
							break;

							default:{
								if(PrefixReg==0){ // No Reg Prefix
									SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
									Address=dwMem;
									PushString=TRUE;
									wsprintf(temp," %08X",dwOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%08Xh",dwMem);
								}
								else{
									SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
									wsprintf(temp," %04X",wOp);
									lstrcat((*Disasm)->Opcode,temp);
									wsprintf(temp,"%04Xh",wMem);
								}
							}
						}
                        
                        if(Op==0xC7){
                            /* 
                                Instruction rule: Mem,Imm ->  1100011woo000mmm,imm
                                Code Block: 1100011
                                w = Reg Size
                                oo - Mod
                                000 - Must be!
                                mmm - Reg/Mem
                                imm - Immidiant (����)
                            */

                            reg1=((BYTE)(*(*Opcode+pos+1))&0x38)>>3; // Check for valid opcode, result must be 0

							if(reg1!=0){
                                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
							}
                            
                            wsprintf(instruction,"%s","mov");
                        }
                        else{
                            if (Op==0x69){ // IMUL REG,MEM,IIM
                                reg1=((BYTE)(*(*Opcode+pos+1))&0x38)>>3; // get register
                                wsprintf(instruction,"imul %s,",regs[RM][reg1]);
                            }
							else{
                                wsprintf(instruction,"%s",Instructions[REG]);
							}
                        }

                        wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,temp);
                        if(PrefixReg==0){ // No regPrefix
                            (*(*index))+=4;
                            (*Disasm)->OpcodeSize+=4;
                        }
                        else{
                            (*(*index))+=2;
                            (*Disasm)->OpcodeSize+=2;
                        }
                    }
					break;

                    case 0x80:case 0x82: case 0x83: case 0xC6:{
						
						switch(Extension){
							case 1:{ // read 1 byte at 3rd position
								FOpcode=(BYTE)(*(*Opcode+pos+4));
								wsprintf(temp,"%02X",FOpcode);
								lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							case 2:{ // read byte at 7th position (dword read before)
								FOpcode=(BYTE)(*(*Opcode+pos+7));
                                wsprintf(temp,"%02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,temp);
							}
							break;

							default:{
								FOpcode=(BYTE)(*(*Opcode+pos+3));
                                wsprintf(temp,"%02X",FOpcode);
							    lstrcat((*Disasm)->Opcode,temp);
							}
						}
                        
						strcpy_s(Aritmathic,"");
						
						if(Op==0x82 || Op==0x83){
							if(FOpcode>0x7F){ // check for signed numbers
							 wsprintf(Aritmathic,"%s",Scale[0]); // '-' aritmathic
							 FOpcode = 0x100-FOpcode; // -XX (Negative the Number)
							}
						}
						
                        // Code Block of C6 is Mov instruction
                        if(Op==0xC6){
                            /* 
                                Instruction rule: Mem,Imm ->  1100011woo000mmm,imm
                                Code Block: 1100011
                                w = Reg Size
                                oo - Mod
                                000 - Must be!
                                mmm - Reg/Mem
                                imm - Immidiant (����)
                            */

                            // Check for valid intruction, reg1 must be 000 to be valid
                            reg1=((BYTE)(*(*Opcode+pos+1))&0x38)>>3; 
                            
							if(reg1!=0){
                                lstrcat((*Disasm)->Remarks,";Invalid Instruction!");
							}

                            wsprintf(instruction,"%s","mov");  
                        }
						else{ // Decode from instruction table
                            wsprintf(instruction,"%s",Instructions[REG]);
						}

						wsprintf(tempMeme,"%s %s,%s%02Xh",instruction,menemonic,Aritmathic,FOpcode);
						
						(*(*index))++;
						(*Disasm)->OpcodeSize++;
					}
					break;

                    case 0x8C:{ // Segments in Source register
                        wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,segs[REG]);
                    }
                    break;

                    case 0xD0: case 0xD1:{
                        strcpy_s(instruction,StringLen(ArtimaticInstructions[REG])+1,ArtimaticInstructions[REG]);
                        wsprintf(tempMeme,"%s %s,1",instruction,menemonic);
                    }
                    break;

                    case 0xD2: case 0xD3:{
                        strcpy_s(instruction,StringLen(ArtimaticInstructions[REG])+1,ArtimaticInstructions[REG]);
                        wsprintf(tempMeme,"%s %s,cl",instruction,menemonic);
                    }
                    break;

                    case 0xD8: case 0xDC:{ // FPU Instructions (UnSigned)
                      wsprintf(tempMeme,"%s %s",FpuInstructions[REG],menemonic);
                    }
                    break;

                    case 0xD9:{ // FPU Instructions Set2 (UnSigned)
                       wsprintf(tempMeme,"%s %s",FpuInstructionsSet2[REG],menemonic);
                    }
                    break;

                    case 0xDA: case 0xDE:{ // FPU Instructions (Signed)
                       wsprintf(tempMeme,"%s %s",FpuInstructionsSigned[REG],menemonic);
                    }
                    break;

                    case 0xDB:{ // FPU Instructions Set2 (Signed)
						if(REG==1 || REG==4 || REG==6){ // No such fpu instructions!
                            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
                        wsprintf(tempMeme,"%s %s",FpuInstructionsSet2Signed[REG],menemonic);
                    }
                    break;

                    case 0xDD:{ // FPU Instructions Set3
						if(REG==1 ||  REG==5){ // no such fpu instruction!
                          lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
                       wsprintf(tempMeme,"%s %s",FpuInstructionsSet3[REG],menemonic);
                    }
                    break;

                    case 0xDF:{ // Extended FPU Instructions Set2 (Signed)
						if(REG==1){ // no such fpu instruction!
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
						wsprintf(tempMeme,"%s %s",FpuInstructionsSet2Signed_EX[REG],menemonic);
                    }
                    break;

                    case 0xF6:{
                        // strip Instruction Bits (1111011woo[000]mmm)
                        reg1=((BYTE)(*(*Opcode+pos+1))&0x38)>>3;

						switch(Extension){
							case 1:{ // read 1 byte at 3rd position
								if(reg1==0 || reg1==1){ // check bites: TEST 
									FOpcode=(BYTE)(*(*Opcode+pos+3));
									wsprintf(temp,"%02X",FOpcode);
									lstrcat((*Disasm)->Opcode,temp);
								}
							}
							break;

							case 2:{ // check bites: TEST
								if(reg1==0 || reg1==1){
                                    // read byte at 7th position (dword read before)
                                    FOpcode=(BYTE)(*(*Opcode+pos+7));
                                    wsprintf(temp,"%02X",FOpcode);
                                    lstrcat((*Disasm)->Opcode,temp);
                                }
							}
							break;

							default:{
								if(reg1==0 || reg1==1){ // check bites: TEST 
                                    FOpcode=(BYTE)(*(*Opcode+pos+2));
                                    wsprintf(temp,"%02X",FOpcode);
                                    lstrcat((*Disasm)->Opcode,temp);
                                }
							}
						}
                        
						strcpy_s(Aritmathic,"");
                        wsprintf(instruction,"%s",InstructionsSet2[REG]);

                        if(reg1==0 || reg1==1){ // TEST
						    wsprintf(tempMeme,"%s %s,%s%02Xh",instruction,menemonic,Aritmathic,FOpcode);
                            (*(*index))++;
						    (*Disasm)->OpcodeSize++;
                        }
						else{ // NOT/NEG/MUL/IMUL/DIV/IDIV instruction must not have operands
                            wsprintf(tempMeme,"%s %s",instruction,menemonic);
						}
                    }
                    break;

                    case 0xF7:{
                        // Get Instruction
                        wsprintf(instruction,"%s",InstructionsSet2[REG]);
                        reg1=((BYTE)(*(*Opcode+pos+1))&0x38)>>3;

                        //================//
                        // Get Extensions!//
                        //================//
                        
                        if(reg1==0 || reg1==1){ // TEST Instruction
                            
							switch(Extension){
								case 1:{ // 1 byte extersion
									if(PrefixReg==0){  // no 0x66 prefix
										SwapDword((BYTE*)(*Opcode+pos+4),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+4),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
								break;

								case 2:{ // 4 bytes Extensions
									if(PrefixReg==0){   
										SwapDword((BYTE*)(*Opcode+pos+7),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+7),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
								break;

								default:{ // No Extension!
									if(PrefixReg==0){   
										SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
										wsprintf(temp," %08X",dwOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%08Xh",dwMem);
									}
									else{
										SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
										wsprintf(temp," %04X",wOp);
										lstrcat((*Disasm)->Opcode,temp);
										wsprintf(temp,"%04Xh",wMem);
									}
								}
							}
                            
                            wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,temp);
                            if(PrefixReg==0){ // No Reg prefix
                                (*(*index))+=4;
                                (*Disasm)->OpcodeSize+=4;
                            }
                            else{
                                (*(*index))+=2;
                                (*Disasm)->OpcodeSize+=2;
                            }
                        }
						else{
                            wsprintf(tempMeme,"%s %s",instruction,menemonic);
						}
                    }
                    break;
                    
                    case 0xFE:{ // MIX Instructions (INC,DEC,INVALID,INVALID,INVALID...)
                       wsprintf(tempMeme,"%s %s",InstructionsSet3[REG],menemonic);
                            
					   if(REG>1){ // Invalid instructions
                           lstrcat((*Disasm)->Remarks,";Invalid Instruction");
					   }
                    }
                    break;

                    case 0xFF:{ // MIX Instructions (INC,DEC,CALL,PUSH,JMP,FAR JMP,FAR CALL,INVALID)
                       wsprintf(tempMeme,"%s %s",InstructionsSet4[REG],menemonic);
					   switch(REG){
							case 3:{ // FAR CALL
								lstrcat((*Disasm)->Remarks,";Far Call");
								 break;
							}
							break;

							case 5:{ // FAR JMP
								lstrcat((*Disasm)->Remarks,";Far Jump");
								break;
							}
							break;

							case 7: {lstrcat((*Disasm)->Remarks,";Invalid Instruction"); } break; // Invalid instructions
					   }  
                    }
                    break;

					default:{
					   wsprintf(tempMeme,"%s %s,%s",instruction,menemonic,regs[RM][REG]);
					}
					break;
				}

				lstrcat((*Disasm)->Assembly,tempMeme);
			}
			break;

			case 1:{ // (<-) Direction
                switch(Op){
                    case 0x8E:{ // Segments in Destination Register
                        wsprintf(tempMeme,"%s %s,%s",instruction,segs[REG],menemonic);
                    }
                	break;

                    // POP DWORD PTR[REG/MEM/DISP]
                    case 0x8F:{
                        wsprintf(tempMeme,"%s %s",instruction,menemonic);
                    }
                    break;
                    
                    // Mixed Bit Rotation Instructions (rol/ror/shl..)
                    case 0xC0:case 0xC1:{
                        switch(Extension){
                            case 0:{
                                FOpcode=(BYTE)(*(*Opcode+pos+3));
                                wsprintf(tempMeme,"%s %s,%02X",ArtimaticInstructions[REG],menemonic,FOpcode);
                                wsprintf(menemonic," %02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,menemonic);
                                (*(*index))++;
                                (*Disasm)->OpcodeSize++;
                            }
                            break;

                            case 1:{
                                FOpcode=(BYTE)(*(*Opcode+pos+4));
                                wsprintf(tempMeme,"%s %s,%02Xh",ArtimaticInstructions[REG],menemonic,FOpcode);
                                wsprintf(menemonic," %02X",(BYTE)(*(*Opcode+pos+4)));
                                lstrcat((*Disasm)->Opcode,menemonic);
                                (*(*index))++;
                                (*Disasm)->OpcodeSize++;
                            }
                            break;

                            case 2:{
                                FOpcode=(BYTE)(*(*Opcode+pos+7));
                                wsprintf(tempMeme,"%s %s,%02Xh",ArtimaticInstructions[REG],menemonic,FOpcode);
                                wsprintf(menemonic," %02X",FOpcode);
                                lstrcat((*Disasm)->Opcode,menemonic);
                                (*(*index))++;
                                (*Disasm)->OpcodeSize++;
                            }
                            break;
                        }
                    }
                    break;
                    
                    case 0xC4:{
                       strcpy_s(instruction,StringLen("les")+1,"les");
                       wsprintf(tempMeme,"%s %s,%s",instruction,regs[RM][REG],menemonic);
                    }
                    break;

                    case 0xC5:{
                       strcpy_s(instruction,StringLen("lds")+1,"lds");
                       wsprintf(tempMeme,"%s %s,%s",instruction,regs[RM][REG],menemonic);
                    }
                    break;

                    default:{
                        wsprintf(tempMeme,"%s %s,%s",instruction,regs[RM][REG],menemonic);
                    }
                    break;
                }
				lstrcat((*Disasm)->Assembly,tempMeme);
			}
			break;
		}		
	}	
}

void GetInstruction(BYTE Opcode,char *menemonic)
{
	// Function GetInstance gets 2 parameters:
	// Opcode - byte to get the instruction for
	// Menemonic - pointer, we put menemonic in here
	
	// This function check which instruction belongs
	// To what opcode(s).
	// There are standard 9 groups of instruction with 4 diff
	// Codes (check bit d/w)
	
	// �������� ���� ���� ����� �����
	// ��� ����.
	// ���� 9 ������ ����� ��  ����� ����� �� �����
	
	switch (Opcode){
        // Opcodes for Menemonics
		case 0x04: case 0x05: case 0x00: case 0x01: case 0x02: case 0x03: strcpy_s(menemonic,StringLen("add")+1,"add");break; // ADD
		case 0x0C: case 0x0D: case 0x08: case 0x09: case 0x0A: case 0x0B: strcpy_s(menemonic,StringLen("or")+1,"or"); break; // OR
		case 0x14: case 0x15: case 0x10: case 0x11: case 0x12: case 0x13: strcpy_s(menemonic,StringLen("adc")+1,"adc");break; // ADC
		case 0x1C: case 0x1D: case 0x18: case 0x19: case 0x1A: case 0x1B: strcpy_s(menemonic,StringLen("sbb")+1,"sbb");break; // SBB
		case 0x24: case 0x25: case 0x20: case 0x21: case 0x22: case 0x23: strcpy_s(menemonic,StringLen("and")+1,"and");break; // AND
		case 0x2C: case 0x2D: case 0x28: case 0x29: case 0x2A: case 0x2B: strcpy_s(menemonic,StringLen("sub")+1,"sub");break; // SUB
		case 0x34: case 0x35: case 0x30: case 0x31: case 0x32: case 0x33: strcpy_s(menemonic,StringLen("xor")+1,"xor");break; // XOR
		case 0x3C: case 0x3D: case 0x38: case 0x39: case 0x3A: case 0x3B: strcpy_s(menemonic,StringLen("cmp")+1,"cmp");break; // CMP
        case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8E: strcpy_s(menemonic,StringLen("mov")+1,"mov");break; // MOV
		case 0x62: strcpy_s(menemonic,StringLen("bound")+1,"bound"); break; // BOUND
		case 0x63: strcpy_s(menemonic,StringLen("arpl")+1,"arpl"); break; // ARPL
		case 0xA8: case 0xA9: strcpy_s(menemonic,StringLen("test")+1,"test");	break; // TEST
		case 0xE4: case 0xE5: strcpy_s(menemonic,StringLen("in")+1,"in");		break; // IN
		case 0xE6: case 0xE7: strcpy_s(menemonic,StringLen("out")+1,"out");		break; // OUT
		case 0x84: case 0x85: strcpy_s(menemonic,StringLen("test")+1,"test");	break; // TEST
		case 0x86: case 0x87: strcpy_s(menemonic,StringLen("xchg")+1,"xchg");	break; // XCHG
        case 0x8D: strcpy_s(menemonic,StringLen("lea")+1,"lea"); break; // LEA
        case 0x8F: strcpy_s(menemonic,StringLen("pop")+1,"pop"); break; // POP
        case 0xC4: strcpy_s(menemonic,StringLen("les")+1,"les"); break; // LES
        case 0xC5: strcpy_s(menemonic,StringLen("lds")+1,"lds"); break; // LDS
	}
}

void GetJumpInstruction(BYTE Opcode,char *menemonic)
{
	// Function returns the name of the menemonic,
	// Associated with an opcode
	switch (Opcode){
        case 0x70: strcpy_s(menemonic,StringLen("jo")+1,"jo");		break;
        case 0x71: strcpy_s(menemonic,StringLen("jno")+1,"jno");	break;
        case 0x72: strcpy_s(menemonic,StringLen("jb")+1,"jb");		break;
        case 0x73: strcpy_s(menemonic,StringLen("jnb")+1,"jnb");	break;
        case 0x74: strcpy_s(menemonic,StringLen("jz")+1,"jz");		break;
        case 0x75: strcpy_s(menemonic,StringLen("jnz")+1,"jnz");	break;
        case 0x76: strcpy_s(menemonic,StringLen("jbe")+1,"jbe");	break;
        case 0x77: strcpy_s(menemonic,StringLen("ja")+1,"ja");		break;
        case 0x78: strcpy_s(menemonic,StringLen("js")+1,"js");		break;
        case 0x79: strcpy_s(menemonic,StringLen("jns")+1,"jns");	break;
        case 0x7A: strcpy_s(menemonic,StringLen("jp")+1,"jp");		break;
        case 0x7B: strcpy_s(menemonic,StringLen("jnp")+1,"jnp");	break;
        case 0x7C: strcpy_s(menemonic,StringLen("jl")+1,"jl");		break;
        case 0x7D: strcpy_s(menemonic,StringLen("jge")+1,"jge");	break;
        case 0x7E: strcpy_s(menemonic,StringLen("jle")+1,"jle");	break;
        case 0x7F: strcpy_s(menemonic,StringLen("jg")+1,"jg");		break;
        case 0xE0: strcpy_s(menemonic,StringLen("loopne")+1,"loopne");	break;
        case 0xE1: strcpy_s(menemonic,StringLen("loope")+1,"loope");	break;
        case 0xE2: strcpy_s(menemonic,StringLen("loop")+1,"loop");		break;
        case 0xE3: strcpy_s(menemonic,StringLen("jecxz")+1,"jecxz");	break;
        case 0xEB: strcpy_s(menemonic,StringLen("jmp")+1,"jmp");		break;
	}
}

int GetNewInstruction(BYTE Op,char *ASM,bool RegPrefix,char *Opcode, DWORD_PTR Index)
{
    // return values:
    // Found = 0 -> big set instruction
    // Found = 1 -> 1 byte Instruction
    // Found = 2 -> Jump Instruction
    
    int Found=1,RM=REG32;
    char Inst[50]="";
    char *JumpTable[16]={
        "jo","jno","jb","jnb","jz","jnz",
        "jbe","ja","js","jns","jpe",
        "jpo","jl","jge","jle","jg"
    };

    switch(Op){
        // 1 BYTE INSTRUCTIONS
        case 0x05: strcpy_s(Inst,StringLen("SysCall")+1,"SysCall");	break;
        case 0x06: strcpy_s(Inst,StringLen("clts")+1,"clts");		break;
        case 0x07: strcpy_s(Inst,StringLen("sysret")+1,"sysret");	break;
        case 0x08: strcpy_s(Inst,StringLen("invd")+1,"invd");		break;
        case 0x09: strcpy_s(Inst,StringLen("wbinvd")+1,"wbinvd");	break;
        case 0x0B: strcpy_s(Inst,StringLen("ud2")+1,"ud2");			break;
        case 0x0E: strcpy_s(Inst,StringLen("femms")+1,"femms");		break;
        case 0x30: strcpy_s(Inst,StringLen("wrmsr")+1,"wrmsr");		break;
        case 0x31: strcpy_s(Inst,StringLen("rdtsc")+1,"rdtsc");		break;
        case 0x32: strcpy_s(Inst,StringLen("rdmsr")+1,"rdmsr");		break;
        case 0x33: strcpy_s(Inst,StringLen("rdpmc")+1,"rdpmc");		break;
        case 0x34: strcpy_s(Inst,StringLen("sysenter")+1,"sysenter");	break;
        case 0x35: strcpy_s(Inst,StringLen("sysexit")+1,"sysexit");	break;
        case 0x77: strcpy_s(Inst,StringLen("emms")+1,"emms");		break;
        case 0xA0: strcpy_s(Inst,StringLen("push fs")+1,"push fs");	break;
        case 0xA1: strcpy_s(Inst,StringLen("pop fs")+1,"pop fs");	break;
        case 0xA2: strcpy_s(Inst,StringLen("cpuid")+1,"cpuid");		break;
        case 0xA8: strcpy_s(Inst,StringLen("push gs")+1,"push gs");	break;
        case 0xA9: strcpy_s(Inst,StringLen("pop gs")+1,"pop gs");	break;
        case 0xAA: strcpy_s(Inst,StringLen("rsm")+1,"rsm");			break;
        
        // BSWAP <REG>
        case 0xC8: case 0xC9: case 0xCA: case 0xCB: 
        case 0xCC: case 0xCD: case 0xCE: case 0xCF:{
			if(RegPrefix){ // check prefix
                RM=REG16;
			}
            wsprintf(Inst,"bswap %s",regs[RM][Op&7]);
        }
        break;

        // Invalid instructions, but have a valid 0xC0.
        case 0x20: case 0x21:case 0x22:
        case 0x23: case 0x50: case 0x71:
        case 0x72: case 0x73:{
            strcpy_s(Inst,"???");
            Found=3;
        }
        break;

        // 3-byte opcode escape: 0F 38 xx
        case 0x38:{
            strcpy_s(Inst,"");
            Found=7; // 3-byte opcode map 0F 38
        }
        break;

        // 3-byte opcode escape: 0F 3A xx
        case 0x3A:{
            strcpy_s(Inst,"");
            Found=8; // 3-byte opcode map 0F 3A
        }
        break;

        // SSE3: haddpd/haddps (0F 7C), hsubpd/hsubps (0F 7D)
        case 0x7C: case 0x7D:{
            strcpy_s(Inst,"");
            Found=0; // decode via Mod_RM_SIB_EX / Mod_11_RM_EX
        }
        break;

        // SSE3: addsubpd/addsubps (0F D0)
        case 0xD0:{
            strcpy_s(Inst,"");
            Found=0;
        }
        break;

        // SSE3: lddqu (0F F0)
        case 0xF0:{
            strcpy_s(Inst,"");
            Found=0;
        }
        break;

        // SSE2/3 conversion instructions (0F 5A, 0F 5B, 0F E6)
        case 0x5A: case 0x5B: case 0xE6:{
            strcpy_s(Inst,"");
            Found=0;
        }
        break;

        // SSE4.2: popcnt (F3 0F B8)
        case 0xB8:{
            strcpy_s(Inst,"");
            Found=0;
        }
        break;

        // 3DNow! (0F 0F ModRM suffix)
        case 0x0F:{
            strcpy_s(Inst,"");
            Found=0;
        }
        break;

        // CET ENDBR / NOP hint (0F 1E xx)
        case 0x1E:{
            strcpy_s(Inst,"nop");
            Found=3;
        }
        break;

        // Invalid Instructions!!
        case 0x19: case 0x1A: case 0x1B:
        case 0x1C: case 0x1D: case 0x04:
        case 0x0A: case 0x0C: case 0x24:
        case 0x36: case 0x37: case 0x25:
        case 0x39: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
        case 0x26: case 0x27:
        case 0x7A: case 0x7B:
        case 0xA6: case 0xA7:
        case 0xB9:
        case 0xF4: case 0xFF:{
            strcpy_s(Inst,"???");
        }
        break;

		case 0xD6:{
			// MOVQ (0x66 first opcode before 0x0F)
			// MOVQ2DQ (0xF3 first opcode before 0x0F)
			// MOVDQ2Q (0xF2 first opcode before 0x0F)
			strcpy_s(Inst,"");
			Found=0;
		}
		break;

        // JUMPS [JXX]
        case 0x80:case 0x81:case 0x82:case 0x83:
        case 0x84:case 0x85:case 0x86:case 0x87:
        case 0x88:case 0x89:case 0x8A:case 0x8B:
        case 0x8C:case 0x8D:case 0x8E:case 0x8F:
        {
            wsprintf(Inst,"%s ",JumpTable[Op&0x0F]);
            Found=2;
        }
        break;

		// 0F C7 XX [XX- has valid 0x08-0x0F]
		case 0xC7:{
			strcpy_s(Inst,"");
			Found=0;
		}
		break;
        
		default:{
			Found=0;
		}
		break;
    }

    strcpy_s(ASM,StringLen(Inst)+1,Inst);
    return Found;
}

//==================================================================================//
//						Decode MMX / 3DNow! / SSE / SSE2 Functions					//
//==================================================================================//

void Mod_11_RM_EX(BYTE d, BYTE w,char **Opcode,DISASSEMBLY **Disasm,bool PrefixReg,BYTE Op,DWORD_PTR **index,BYTE RepPrefix)
{
   /* 
       Function Mod_11_RM Checks whatever we have
	   Both bit d (direction) and bit w (full/partial size).
	 
       There are 4 states:
	   00 - d=0 / w=0 ; direction -> (ie: DH->DL),   partial size (AL,DH,BL..)
	   01 - d=0 / w=1 ; direction -> (ie: EDX->EAX), partial size (EAX,EBP,EDI..)
	   10 - d=1 / w=0 ; direction <- (ie: DH<-DL),   partial size (AL,DH,BL..)
	   11 - d=1 / w=1 ; direction <- (ie: EDX<-EAX), partial size (EAX,EBP,EDI..)
	
       Also deals with harder opcodes which have diffrent
       Addresing type.
    */
    
	DWORD_PTR RM,IndexAdd=1,m_OpcodeSize=2,Pos; // Register(s) Pointer
    WORD wOp,wMem;
	BYTE reg1=0,reg2=0,m_Opcode=0,REG;
	char assembly[50]="",temp[128]="",m_Bytes[128]="";
    
    Pos=(*(*index)); // Current Position
    m_Opcode = (BYTE)(*(*Opcode+Pos+1));// Decode registers from second byte
    
    // Strip Used Instructions / Used Segment
    REG=m_Opcode; 
    REG>>=3;
	REG&=0x07;

    // (->) / reg8
    if(d==0 && w==0){    
        RM=REG8;
        reg1=(m_Opcode&0x07);
        reg2=(m_Opcode&0x38)>>3;
    }
    
    // (->) / reg32
    if(d==0 && w==1){    
        RM=REG32;
		if(PrefixReg==1){
            RM=REG16; // (->) / reg16 (RegPerfix is being used)
		}
        
        reg1=(m_Opcode&0x07);
        reg2=(m_Opcode&0x38)>>3;
    }
    
    // (<-) / reg8
    if(d==1 && w==0){    
        RM=REG8;
        reg2=(m_Opcode&0x07);
        reg1=(m_Opcode&0x38)>>3;
    }
    
    // (<-) / reg32
    if(d==1 && w==1){
        RM=REG32;
		if(PrefixReg==1){
            RM=REG16; // (<-) / reg16
		}

        reg2=(m_Opcode&0x07);
        reg1=(m_Opcode&0x38)>>3;
    }

    // 64-bit mode: apply REX extensions
    if((*Disasm)->Mode64){
        if((*Disasm)->RexW && w==1){
            RM=REG64;
        }
        if((*Disasm)->RexR){
            if(d==1) reg1 |= 0x08;
            else     reg2 |= 0x08;
        }
        if((*Disasm)->RexB){
            if(d==1) reg2 |= 0x08;
            else     reg1 |= 0x08;
        }
        if((*Disasm)->RexPrefix && w==0){
            RM=REG8X;
        }
    }

    switch(Op){
        case 0x00:{
            RM=REG16; // FORCE 16BIT
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(assembly,"%s %s",NewSet[REG],regs[RM][reg2]);
            wsprintf(temp,"%04X",wOp);
			if(REG>5){
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
			}
        }
        break;

        case 0x01:{
            // SSE3: MONITOR (0F 01 C8), MWAIT (0F 01 C9)
            if(m_Opcode==0xC8){
                wsprintf(assembly,"monitor");
                SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
                wsprintf(temp,"%04X",wOp);
                break;
            }
            if(m_Opcode==0xC9){
                wsprintf(assembly,"mwait");
                SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
                wsprintf(temp,"%04X",wOp);
                break;
            }
            // VMX instructions (0F 01 C1-C4)
            if(m_Opcode==0xC1){ wsprintf(assembly,"vmcall");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xC2){ wsprintf(assembly,"vmlaunch"); SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xC3){ wsprintf(assembly,"vmresume"); SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xC4){ wsprintf(assembly,"vmxoff");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            // XSAVE: XGETBV/XSETBV (0F 01 D0/D1)
            if(m_Opcode==0xD0){ wsprintf(assembly,"xgetbv");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xD1){ wsprintf(assembly,"xsetbv");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            // TSX: XEND/XTEST (0F 01 D5/D6)
            if(m_Opcode==0xD5){ wsprintf(assembly,"xend");     SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xD6){ wsprintf(assembly,"xtest");    SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            // SVM instructions (0F 01 D8-DF)
            if(m_Opcode==0xD8){ wsprintf(assembly,"vmrun");    SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xD9){ wsprintf(assembly,"vmmcall");  SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xDA){ wsprintf(assembly,"vmload");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xDB){ wsprintf(assembly,"vmsave");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xDC){ wsprintf(assembly,"stgi");     SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xDD){ wsprintf(assembly,"clgi");     SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xDE){ wsprintf(assembly,"skinit");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            if(m_Opcode==0xDF){ wsprintf(assembly,"invlpga");  SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            // SWAPGS (0F 01 F8) - 64-bit only
            if(m_Opcode==0xF8){ wsprintf(assembly,"swapgs");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }
            // RDTSCP (0F 01 F9)
            if(m_Opcode==0xF9){ wsprintf(assembly,"rdtscp");   SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem); wsprintf(temp,"%04X",wOp); break; }

            RM=REG32; // DEFAULT 32Bit
			if(REG>=4 && REG<=6){ // USES 32bit
                RM=REG16;
			}

			if(REG==7){ // USES 8BIT
                RM=REG8;
			}

            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(assembly,"%s %s",NewSet2[REG],regs[RM][reg2]);
            wsprintf(temp,"%04X",wOp);
			if(REG==5){
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
			}
        }
        break;

        case 0x1E:{ // CET: ENDBR64 (0F 1E FA), ENDBR32 (0F 1E FB), RDSSPD/Q (F3 0F 1E /1)
            if(RepPrefix==0xF3 && REG==1){ // RDSSPD/RDSSPQ (F3 0F 1E /1)
                wsprintf(assembly,"%s %s",
                    ((*Disasm)->Mode64 && (*Disasm)->RexW)?"rdsspq":"rdsspd",
                    regs[RM][reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF3 && m_Opcode==0xFB){
                wsprintf(assembly,"endbr32");
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF3 && m_Opcode==0xFA){
                wsprintf(assembly,"endbr64");
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                wsprintf(assembly,"nop %s",regs[RM][reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x1F:{
            wsprintf(assembly,"nop %s",regs[RM][reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x02:{ // LAR
            wsprintf(assembly,"lar %s, %s",regs[RM][reg1],regs[RM][reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x03:{ // LSL
          wsprintf(assembly,"lsl %s, %s",regs[RM][reg1],regs[RM][reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x10:{ // MOVUPS / MOVSS / MOVSD / MOVUPD
           if(RepPrefix==0xF3){
               wsprintf(assembly,"movss %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
               strcpy_s((*Disasm)->Assembly,"");
               m_OpcodeSize++;
           }
           else if(RepPrefix==0xF2){
               wsprintf(assembly,"movsd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
               strcpy_s((*Disasm)->Assembly,"");
               m_OpcodeSize++;
           }
		   else if(PrefixReg){
               wsprintf(assembly,"movupd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
		   }
		   else{
               wsprintf(assembly,"movups %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
		   }

           SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
           wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x11:{ // MOVUPS / MOVSS / MOVSD / MOVUPD
           if(RepPrefix==0xF3){
                wsprintf(assembly,"movss %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
           }
           else if(RepPrefix==0xF2){
                wsprintf(assembly,"movsd %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
           }
		   else if(PrefixReg){
               wsprintf(assembly,"movupd %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
		   }
		   else{
               wsprintf(assembly,"movups %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
		   }

           SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
           wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x12:{ // MOVHLPS / MOVDDUP (F2) / MOVSLDUP (F3)
            if(RepPrefix==0xF2){
                wsprintf(assembly,"movddup %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF3){
                wsprintf(assembly,"movsldup %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                wsprintf(assembly,"movhlps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x13:{ // MOVLPS / MOVLPD (66)
            if(PrefixReg)
                wsprintf(assembly,"movlpd %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
            else
                wsprintf(assembly,"movlps %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x14:{ // UNPCKLPS / UNPCKLPD (66)
           if(PrefixReg)
               wsprintf(assembly,"unpcklpd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
           else
               wsprintf(assembly,"unpcklps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
           SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
           wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x15:{ // UNPCKHPS / UNPCKHPD (66)
          if(PrefixReg)
              wsprintf(assembly,"unpckhpd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
          else
              wsprintf(assembly,"unpckhps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x16:{ // MOVLHPS / MOVSHDUP (F3)
          if(RepPrefix==0xF3){
              wsprintf(assembly,"movshdup %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
              strcpy_s((*Disasm)->Assembly,"");
              m_OpcodeSize++;
          }
          else{
              wsprintf(assembly,"movlhps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
          }
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x17:{ // MOVHPS
           wsprintf(assembly,"movhps %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
           SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
           wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x18:{
           wsprintf(assembly,"%s, %s",NewSet3[REG],regs[RM][reg2]);
		   if(REG>3){
               lstrcat((*Disasm)->Remarks,";Invalid Instruction");
		   }
           SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
           wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x28:{ // MOVAPS / MOVAPD (66)
           if(PrefixReg)
               wsprintf(assembly,"movapd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
           else
               wsprintf(assembly,"movaps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
           SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
           wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x29:{ // MOVAPS / MOVAPD (66)
          if(PrefixReg)
              wsprintf(assembly,"movapd %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
          else
              wsprintf(assembly,"movaps %s, %s",MMXRegs[reg2],MMXRegs[reg1]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x2A:{ // CVTPI2PS / CVTSI2SS (F3) / CVTSI2SD (F2)
          if(RepPrefix==0xF3){
              wsprintf(assembly,"cvtsi2ss %s, %s",MMXRegs[reg1],regs[RM][reg2]);
              strcpy_s((*Disasm)->Assembly,"");
              m_OpcodeSize++;
          }
          else if(RepPrefix==0xF2){
              wsprintf(assembly,"cvtsi2sd %s, %s",MMXRegs[reg1],regs[RM][reg2]);
              strcpy_s((*Disasm)->Assembly,"");
              m_OpcodeSize++;
          }
		  else{
              wsprintf(assembly,"%s %s, %s",NewSet4[(Op&0x0F)-0x08],MMXRegs[reg1],Regs3DNow[reg2]);
		  }
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x2C: case 0x2D:{ // CVTTPS2PI, CVTPS2PI / CVTTSS2SI (F3) / CVTTSD2SI (F2)
            if(RepPrefix==0xF3){
				if(Op==0x2C){
                    strcpy_s(temp,"cvttss2si");
				}
				else{
                    strcpy_s(temp,"cvtss2si");
				}
                wsprintf(assembly,"%s %s, %s",temp,regs[RM][reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF2){
				if(Op==0x2C){
                    strcpy_s(temp,"cvttsd2si");
				}
				else{
                    strcpy_s(temp,"cvtsd2si");
				}
                wsprintf(assembly,"%s %s, %s",temp,regs[RM][reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
			else{
                wsprintf(assembly,"%s %s, %s",NewSet4[(Op&0x0F)-0x08],Regs3DNow[reg1],MMXRegs[reg2]);
			}

            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x2E: case 0x2F:{ // UCOMISS/COMISS / UCOMISD/COMISD (66)
            if(PrefixReg)
                wsprintf(assembly,"%s %s, %s",(Op==0x2E)?"ucomisd":"comisd",MMXRegs[reg1],MMXRegs[reg2]);
            else
                wsprintf(assembly,"%s %s, %s",NewSet4[(Op&0x0F)-0x08],MMXRegs[reg1],MMXRegs[reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x47:
        case 0x48:case 0x49:case 0x4A:case 0x4B:case 0x4C:case 0x4D:case 0x4E:case 0x4F:
        {
          wsprintf(assembly,"%s %s,%s",NewSet5[Op&0x0F],regs[RM][reg1],regs[RM][reg2]); 
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:
        case 0x58:case 0x59:case 0x5C:case 0x5D:case 0x5E:case 0x5F:
        {
            if(RepPrefix==0xF3){ // F3 prefix: scalar single
                wsprintf(assembly,"%s %s,%s",NewSet6Ex[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF2){ // F2 prefix: scalar double
                wsprintf(assembly,"%s %s,%s",NewSet6_F2[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
			else if(PrefixReg){ // 66 prefix: packed double
                wsprintf(assembly,"%s %s,%s",NewSet6_66[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
			}
			else{
                wsprintf(assembly,"%s %s,%s",NewSet6[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
			}

            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break; // MIX

        case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:
        case 0x68:case 0x69:case 0x6A:case 0x6B:case 0x6F:
        {
          if(Op==0x6F && RepPrefix==0xF3){
              wsprintf(assembly,"movdqu %s,%s",MMXRegs[reg1],MMXRegs[reg2]);
              strcpy_s((*Disasm)->Assembly,"");
              m_OpcodeSize++;
          }
          else if(Op==0x6F && PrefixReg){
              wsprintf(assembly,"movdqa %s,%s",MMXRegs[reg1],MMXRegs[reg2]);
          }
          else if(PrefixReg){
              wsprintf(assembly,"%s %s,%s",NewSet7[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
          }
          else{
              wsprintf(assembly,"%s %s,%s",NewSet7[Op&0x0F],Regs3DNow[reg1],Regs3DNow[reg2]);
          }
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x6E:{ // MOVD mm/xmm, r/m32
          RM=REG32;
          if(PrefixReg)
              wsprintf(assembly,"%s %s,%s",NewSet7[Op&0x0F],MMXRegs[reg1],regs[RM][reg2]);
          else
              wsprintf(assembly,"%s %s,%s",NewSet7[Op&0x0F],Regs3DNow[reg1],regs[RM][reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x6C:{ // PUNPCKLQDQ (66 only)
          if(PrefixReg)
              wsprintf(assembly,"punpcklqdq %s,%s",MMXRegs[reg1],MMXRegs[reg2]);
          else{
              strcpy_s(assembly,"???");
              lstrcat((*Disasm)->Remarks,";Invalid Instruction");
          }
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x6D:{ // PUNPCKHQDQ (66 only)
          if(PrefixReg)
              wsprintf(assembly,"punpckhqdq %s,%s",MMXRegs[reg1],MMXRegs[reg2]);
          else{
              strcpy_s(assembly,"???");
              lstrcat((*Disasm)->Remarks,";Invalid Instruction");
          }
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x70:{
            if(RepPrefix==0xF3){
                wsprintf(assembly,"pshufhw %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF2){
                wsprintf(assembly,"pshuflw %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(PrefixReg){
                wsprintf(assembly,"pshufd %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
            }
            else{
                wsprintf(assembly,"%s %s,%s,%02Xh",NewSet8[Op&0x0F],Regs3DNow[reg1],Regs3DNow[reg2],(BYTE)(*(*Opcode+Pos+2)));
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0x74:case 0x75:case 0x76:{ // PCMPEQx
          if(PrefixReg)
              wsprintf(assembly,"%s %s,%s",NewSet8[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
          else
              wsprintf(assembly,"%s %s,%s",NewSet8[Op&0x0F],Regs3DNow[reg1],Regs3DNow[reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x7E:{
          if(RepPrefix==0xF3){
              wsprintf(assembly,"movq %s,%s",MMXRegs[reg1],MMXRegs[reg2]);
              strcpy_s((*Disasm)->Assembly,"");
              m_OpcodeSize++;
          }
          else if(PrefixReg){
              RM=REG32;
              wsprintf(assembly,"%s %s,%s",NewSet7[Op&0x0F],regs[RM][reg2],MMXRegs[reg1]);
          }
          else{
              wsprintf(assembly,"%s %s,%s",NewSet7[Op&0x0F],regs[RM][reg2],Regs3DNow[reg1]);
          }
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x7F:{
          if(RepPrefix==0xF3){
              wsprintf(assembly,"movdqu %s,%s",MMXRegs[reg2],MMXRegs[reg1]);
              strcpy_s((*Disasm)->Assembly,"");
              m_OpcodeSize++;
          }
          else if(PrefixReg){
              wsprintf(assembly,"movdqa %s,%s",MMXRegs[reg2],MMXRegs[reg1]);
          }
          else{
              wsprintf(assembly,"%s %s,%s",NewSet7[Op&0x0F],Regs3DNow[reg2],Regs3DNow[reg1]);
          }
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x90:case 0x91:case 0x92:case 0x93:case 0x94:case 0x95:case 0x96:case 0x97:
        case 0x98:case 0x99:case 0x9A:case 0x9B:case 0x9C:case 0x9D:case 0x9E:case 0x9F:
        {
          RM=REG8; // FORCE 8BIT
          wsprintf(assembly,"%s %s",NewSet9[Op&0x0F],regs[RM][reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break; // MIX

        case 0xA3: case 0xAB:{
          wsprintf(assembly,"%s %s,%s",NewSet10[Op&0x0F],regs[RM][reg2],regs[RM][reg1]); 
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xA4: case 0xAC:{
            wsprintf(assembly,"%s %s,%s,%02Xh",NewSet10[Op&0x0F],regs[RM][reg2],regs[RM][reg1],(BYTE)(*(*Opcode+Pos+2))); 
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0xA5: case 0xAD:{
          wsprintf(assembly,"%s %s,%s,cl",NewSet10[Op&0x0F],regs[RM][reg2],regs[RM][reg1]); 
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xAE:{
            wsprintf(temp,"%02X%02X",(BYTE)(*(*Opcode+Pos)),(BYTE)(*(*Opcode+Pos+1)));
            if(RepPrefix==0xF3 && REG==5){ // INCSSPD/INCSSPQ (F3 0F AE /5)
                wsprintf(assembly,"%s %s",
                    ((*Disasm)->Mode64 && (*Disasm)->RexW)?"incsspq":"incsspd",
                    regs[RM][reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(REG>3){ // Check for Invalid
                m_Opcode=(BYTE)(*(*Opcode+Pos+1));
                switch(m_Opcode){ // Lone Instructions
                    case 0xE8: strcpy_s(assembly,"lfence"); break;
                    case 0xF0: strcpy_s(assembly,"mfence"); break;
                    case 0xF8: strcpy_s(assembly,"sfence"); break;
                    default: lstrcat((*Disasm)->Remarks,";Invalid Instruction"); break;
                }
            }
			else{
                wsprintf(assembly,"%s %s",NewSet10Ex[REG],regs[RM][reg2]);
			}
        }
        break;

        case 0xAF:{
          wsprintf(assembly,"%s %s,%s",NewSet10[Op&0x0F],regs[RM][reg1],regs[RM][reg2]); 
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xB0: case 0xB1: case 0xB3: case 0xBB:{
			if((Op&0x0F)==0){
				RM=REG8;
			}
			wsprintf(assembly,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][reg2],regs[RM][reg1]); 
			SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
			wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xBC:{ // BSF or TZCNT (F3 0F BC)
            if(RepPrefix==0xF3){
                wsprintf(assembly,"tzcnt %s,%s",regs[RM][reg1],regs[RM][reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                wsprintf(assembly,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][reg1],regs[RM][reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xBD:{ // BSR or LZCNT (F3 0F BD)
            if(RepPrefix==0xF3){
                wsprintf(assembly,"lzcnt %s,%s",regs[RM][reg1],regs[RM][reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                wsprintf(assembly,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][reg1],regs[RM][reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xB2:case 0xB4:case 0xB5:
        case 0xB6:case 0xB7:
        case 0xBE:case 0xBF:
        {
          BYTE reg=Op&0x0F;
          int RM2=REG32; // default

		  if(reg==0x06 || reg==0x0E){
              RM2=REG8;
		  }

		  if(reg==0x07 || reg==0x0F){
              RM2=REG16;
		  }

          wsprintf(assembly,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][reg1],regs[RM2][reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xC0: case 0xC1:{
			if(Op==0xC0){
				RM=REG8;
			}
			wsprintf(assembly,"xadd %s,%s",regs[RM][reg2],regs[RM][reg1]); 
			SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
			wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xC2:{
            if((BYTE)(*(*Opcode+Pos+2))<8){ // Instructions here
                if(RepPrefix==0xF3){
                    wsprintf(assembly,"%s %s,%s",NewSet12Ex[(BYTE)(*(*Opcode+Pos+2))],MMXRegs[reg1],MMXRegs[reg2]);
                    strcpy_s((*Disasm)->Assembly,"");
                    m_OpcodeSize++;
                }
                else if(RepPrefix==0xF2){
                    wsprintf(assembly,"%s %s,%s",NewSet12_F2[(BYTE)(*(*Opcode+Pos+2))],MMXRegs[reg1],MMXRegs[reg2]);
                    strcpy_s((*Disasm)->Assembly,"");
                    m_OpcodeSize++;
                }
				else if(PrefixReg){
                    wsprintf(assembly,"%s %s,%s",NewSet12_66[(BYTE)(*(*Opcode+Pos+2))],MMXRegs[reg1],MMXRegs[reg2]);
				}
				else{
                    wsprintf(assembly,"%s %s,%s",NewSet12[(BYTE)(*(*Opcode+Pos+2))],MMXRegs[reg1],MMXRegs[reg2]);
				}
            }
            else{
                if(RepPrefix==0xF3){
                    wsprintf(assembly,"cmpss %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
                    strcpy_s((*Disasm)->Assembly,"");
                    m_OpcodeSize++;
                }
                else if(RepPrefix==0xF2){
                    wsprintf(assembly,"cmpsd %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
                    strcpy_s((*Disasm)->Assembly,"");
                    m_OpcodeSize++;
                }
				else if(PrefixReg){
                    wsprintf(assembly,"cmppd %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
				}
				else{
                    wsprintf(assembly,"cmpps %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
				}
            }
            
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0xC4:{
            RM=REG32;
            if(PrefixReg)
                wsprintf(assembly,"pinsrw %s,%s,%02Xh",MMXRegs[reg1],regs[RM][reg2],(BYTE)(*(*Opcode+Pos+2)));
            else
                wsprintf(assembly,"pinsrw %s,%s,%02Xh",Regs3DNow[reg1],regs[RM][reg2],(BYTE)(*(*Opcode+Pos+2)));
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0xC5:{
            RM=REG32;
            if(PrefixReg)
                wsprintf(assembly,"pextrw %s,%s,%02Xh",regs[RM][reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
            else
                wsprintf(assembly,"pextrw %s,%s,%02Xh",regs[RM][reg1],Regs3DNow[reg2],(BYTE)(*(*Opcode+Pos+2)));
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0xC6:{
            if(PrefixReg)
                wsprintf(assembly,"shufpd %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
            else
                wsprintf(assembly,"shufps %s,%s,%02Xh",MMXRegs[reg1],MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0xC7:{ // RDRAND (REG==6) / RDSEED (REG==7) - register form
            if(REG==6){
                wsprintf(assembly,"rdrand %s",regs[RM][reg2]);
            }
            else if(REG==7){
                wsprintf(assembly,"rdseed %s",regs[RM][reg2]);
            }
            else{
                strcpy_s(assembly,"???");
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x78:{ // VMREAD r/m64, r64
            wsprintf(assembly,"vmread %s,%s",regs[RM][reg2],regs[RM][reg1]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x79:{ // VMWRITE r64, r/m64
            wsprintf(assembly,"vmwrite %s,%s",regs[RM][reg1],regs[RM][reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xD7:{
            RM=REG32;
            if(PrefixReg)
                wsprintf(assembly,"%s %s,%s",NewSet13[Op&0x0F],regs[RM][reg1],MMXRegs[reg2]);
            else
                wsprintf(assembly,"%s %s,%s",NewSet13[Op&0x0F],regs[RM][reg1],Regs3DNow[reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xD1:case 0xD2:case 0xD3:case 0xD4:case 0xD5:case 0xD8:case 0xDF:
        case 0xD9:case 0xDA:case 0xDB:case 0xDC:case 0xDD:case 0xDE:
        {
            if(PrefixReg)
                wsprintf(assembly,"%s %s,%s",NewSet13[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
            else
                wsprintf(assembly,"%s %s,%s",NewSet13[Op&0x0F],Regs3DNow[reg1],Regs3DNow[reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xE0:case 0xE1:case 0xE2:case 0xE3:
        case 0xE4:case 0xE5:case 0xE8:case 0xE9:
        case 0xEA:case 0xEB:case 0xEC:case 0xED:
        case 0xEE:case 0xEF:
        {
            if(PrefixReg)
                wsprintf(assembly,"%s %s,%s",NewSet14[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
            else
                wsprintf(assembly,"%s %s,%s",NewSet14[Op&0x0F],Regs3DNow[reg1],Regs3DNow[reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xE7:{
            if(PrefixReg)
                wsprintf(assembly,"movntdq %s,%s",regs[RM][reg2],MMXRegs[reg1]);
            else
                wsprintf(assembly,"%s %s,%s",NewSet14[Op&0x0F],regs[RM][reg2],Regs3DNow[reg1]);
            wsprintf(temp,"%02X%02X",(BYTE)(*(*Opcode+Pos)),(BYTE)(*(*Opcode+Pos+1)));
        }
        break;

        case 0xF1:case 0xF2:case 0xF3:case 0xF5:case 0xF6:
        case 0xF7:case 0xF8:case 0xF9:case 0xFA:case 0xFB:case 0xFC:
        case 0xFD:case 0xFE:
        {
            if(PrefixReg){
                if(Op==0xF7)
                    wsprintf(assembly,"maskmovdqu %s,%s",MMXRegs[reg1],MMXRegs[reg2]);
                else
                    wsprintf(assembly,"%s %s,%s",NewSet15[Op&0x0F],MMXRegs[reg1],MMXRegs[reg2]);
            }
            else{
                wsprintf(assembly,"%s %s,%s",NewSet15[Op&0x0F],Regs3DNow[reg1],Regs3DNow[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x0D: {
            BYTE NextByte = (BYTE)(*(*Opcode+Pos+1));
            
			if((NextByte&0x0F)<=7){
                strcpy_s(temp,"prefetch");
			}
			else{
                strcpy_s(temp,"prefetchw");
			}

			if((NextByte)<=0xCF){
                wsprintf(assembly,"%s %s",temp,regs[RM][reg2]);
			}
            else{
                strcpy_s(assembly,"???");
                lstrcat((*Disasm)->Remarks,"Invalid Instruction");
            }

            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x20:{
            wsprintf(assembly,"mov %s,%s",regs[RM][reg2],ControlRegs[reg1]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x21:{
          wsprintf(assembly,"mov %s,%s",regs[RM][reg2],DebugRegs[reg1]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x22:{
          wsprintf(assembly,"mov %s,%s",ControlRegs[reg1],regs[RM][reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x23:{
          wsprintf(assembly,"mov %s,%s",DebugRegs[reg1],regs[RM][reg2]);
          SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
          wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x50:{
            RM=REG32;
            if(PrefixReg)
                wsprintf(assembly,"movmskpd %s,%s",regs[RM][reg1],MMXRegs[reg2]);
            else
                wsprintf(assembly,"movmskps %s,%s",regs[RM][reg1],MMXRegs[reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x71:{
            BYTE NextByte = (BYTE)(*(*Opcode+Pos+1));

            if(
                (NextByte>=0xD0 && NextByte<=0xD7) ||
                (NextByte>=0xE0 && NextByte<=0xE7) ||
                (NextByte>=0xF0 && NextByte<=0xF7)
              )
            {
				if(NextByte>=0xD0 && NextByte<=0xD7){
                   strcpy_s(temp,"psrlw");
				}
				else{
					if (NextByte>=0xE0 && NextByte<=0xE7){
                        strcpy_s(temp,"psraw");
					}
					else{
                        strcpy_s(temp,"psllw");
					}
				}

                if(PrefixReg)
                    wsprintf(assembly,"%s %s,%02Xh",temp,MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
                else
                    wsprintf(assembly,"%s %s,%02Xh",temp,Regs3DNow[reg2],(BYTE)(*(*Opcode+Pos+2)));
            }
            else{
                strcpy_s(assembly,"???");
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            }

            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0x72:{
            BYTE NextByte = (BYTE)(*(*Opcode+Pos+1));

            if(
                (NextByte>=0xD0 && NextByte<=0xD7) ||
                (NextByte>=0xE0 && NextByte<=0xE7) ||
                (NextByte>=0xF0 && NextByte<=0xF7)
                )
            {
				if(NextByte>=0xD0 && NextByte<=0xD7){
                    strcpy_s(temp,"psrld");
				}
				else{
					if(NextByte>=0xE0 && NextByte<=0xE7){
						strcpy_s(temp,"psrad");
					}
					else{
						strcpy_s(temp,"pslld");
					}
				}

                if(PrefixReg)
                    wsprintf(assembly,"%s %s,%02Xh",temp,MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
                else
                    wsprintf(assembly,"%s %s,%02Xh",temp,Regs3DNow[reg2],(BYTE)(*(*Opcode+Pos+2)));
            }
            else{
                strcpy_s(assembly,"???");
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            }

            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0x73:{
            BYTE NextByte = (BYTE)(*(*Opcode+Pos+1));

            if(NextByte>=0xD0 && NextByte<=0xD7){
                strcpy_s(temp,"psrlq");
            }
            else if(NextByte>=0xF0 && NextByte<=0xF7){
                strcpy_s(temp,"psllq");
            }
            else if(PrefixReg && NextByte>=0xD8 && NextByte<=0xDF){
                strcpy_s(temp,"psrldq");
            }
            else if(PrefixReg && NextByte>=0xF8 && NextByte<=0xFF){
                strcpy_s(temp,"pslldq");
            }
            else{
                strcpy_s(temp,"???");
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            }

            if(PrefixReg)
                wsprintf(assembly,"%s %s,%02Xh",temp,MMXRegs[reg2],(BYTE)(*(*Opcode+Pos+2)));
            else
                wsprintf(assembly,"%s %s,%02Xh",temp,Regs3DNow[reg2],(BYTE)(*(*Opcode+Pos+2)));
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0xBA:{
            BYTE NextByte = (BYTE)(*(*Opcode+Pos+1));

            if(NextByte>=0xE0 && NextByte<=0xFF){
				if(NextByte>=0xE0 && NextByte<=0xE7){
                    strcpy_s(temp,"bt");
				}
                else if (NextByte>=0xE8 && NextByte<=0xEF)
                    strcpy_s(temp,"bts");
                else if (NextByte>=0xF0 && NextByte<=0xF7)
                        strcpy_s(temp,"btr");
                    else
                        strcpy_s(temp,"btc");

                wsprintf(assembly,"%s %s,%02Xh",temp,regs[RM][reg2],(BYTE)(*(*Opcode+Pos+2)));
            }
            else{
                strcpy_s(assembly,"???");
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            }

            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,(BYTE)(*(*Opcode+Pos+2)));
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

        case 0xC3:{ // MOVNTI is memory-only; reg-reg is invalid
            strcpy_s(assembly,"???");
            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xD6:{ // MOVQ (66) / MOVQ2DQ (F3) / MOVDQ2Q (F2)
            if(RepPrefix==0xF3){
                wsprintf(assembly,"movq2dq %s,%s",MMXRegs[reg1],Regs3DNow[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF2){
                wsprintf(assembly,"movdq2q %s,%s",Regs3DNow[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(PrefixReg){
                wsprintf(assembly,"movq %s,%s",MMXRegs[reg2],MMXRegs[reg1]);
            }
            else{
                strcpy_s(assembly,"???");
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        // ==================== SSE3 Instructions ====================

        case 0x7C:{ // HADDPD (66), HADDPS (F2)
            if(RepPrefix==0xF2){
                wsprintf(assembly,"haddps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                // 66 prefix handled by Disasm.cpp (assembly already has prefix)
                wsprintf(assembly,"haddpd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x7D:{ // HSUBPD (66), HSUBPS (F2)
            if(RepPrefix==0xF2){
                wsprintf(assembly,"hsubps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                wsprintf(assembly,"hsubpd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xD0:{ // ADDSUBPD (66), ADDSUBPS (F2)
            if(RepPrefix==0xF2){
                wsprintf(assembly,"addsubps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                wsprintf(assembly,"addsubpd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xF0:{ // LDDQU (F2)
            if(RepPrefix==0xF2){
                wsprintf(assembly,"lddqu %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                strcpy_s(assembly,"???");
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x5A:{ // CVTPS2PD / CVTPD2PS (66) / CVTSS2SD (F3) / CVTSD2SS (F2)
            if(RepPrefix==0xF3){
                wsprintf(assembly,"cvtss2sd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF2){
                wsprintf(assembly,"cvtsd2ss %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(PrefixReg){
                wsprintf(assembly,"cvtpd2ps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            else{
                wsprintf(assembly,"cvtps2pd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x5B:{ // CVTDQ2PS / CVTPS2DQ (66) / CVTTPS2DQ (F3)
            if(RepPrefix==0xF3){
                wsprintf(assembly,"cvttps2dq %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(PrefixReg){
                wsprintf(assembly,"cvtps2dq %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            else{
                wsprintf(assembly,"cvtdq2ps %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xE6:{ // CVTPD2DQ (F2) / CVTDQ2PD (F3) / CVTTPD2DQ (66)
            if(RepPrefix==0xF2){
                wsprintf(assembly,"cvtpd2dq %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else if(RepPrefix==0xF3){
                wsprintf(assembly,"cvtdq2pd %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                wsprintf(assembly,"cvttpd2dq %s, %s",MMXRegs[reg1],MMXRegs[reg2]);
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0xB8:{ // POPCNT (F3 0F B8)
            if(RepPrefix==0xF3){
                wsprintf(assembly,"popcnt %s, %s",regs[RM][reg1],regs[RM][reg2]);
                strcpy_s((*Disasm)->Assembly,"");
                m_OpcodeSize++;
            }
            else{
                strcpy_s(assembly,"???");
                lstrcat((*Disasm)->Remarks,";Invalid Instruction");
            }
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X",wOp);
        }
        break;

        case 0x0F:{ // 3DNow! (0F 0F ModRM suffix)
            BYTE suffix = (BYTE)(*(*Opcode+Pos+2));
            const char *mnem3d = "???";
            switch(suffix){
                case 0x0D: mnem3d = "pi2fd";    break;
                case 0x1D: mnem3d = "pf2id";    break;
                case 0x90: mnem3d = "pfcmpge";  break;
                case 0x94: mnem3d = "pfmin";     break;
                case 0x96: mnem3d = "pfrcp";     break;
                case 0x97: mnem3d = "pfrsqrt";   break;
                case 0x9A: mnem3d = "pfsub";     break;
                case 0x9E: mnem3d = "pfadd";     break;
                case 0xA0: mnem3d = "pfcmpgt";   break;
                case 0xA4: mnem3d = "pfmax";     break;
                case 0xA6: mnem3d = "pfrcpit1";  break;
                case 0xA7: mnem3d = "pfrsqit1";  break;
                case 0xAA: mnem3d = "pfsubr";    break;
                case 0xAE: mnem3d = "pfacc";     break;
                case 0xB0: mnem3d = "pfcmpeq";   break;
                case 0xB4: mnem3d = "pfmul";     break;
                case 0xB6: mnem3d = "pfrcpit2";  break;
                case 0xB7: mnem3d = "pmulhrw";   break;
                case 0xBB: mnem3d = "pswapd";    break;
                case 0xBF: mnem3d = "pavgusb";   break;
            }
            wsprintf(assembly,"%s %s,%s",mnem3d,Regs3DNow[reg1],Regs3DNow[reg2]);
            SwapWord((BYTE*)(*Opcode+Pos),&wOp,&wMem);
            wsprintf(temp,"%04X %02X",wOp,suffix);
            (*(*index))++;
            m_OpcodeSize++;
        }
        break;

    }
    
    lstrcat((*Disasm)->Assembly,assembly);
    (*Disasm)->OpcodeSize=m_OpcodeSize;
    lstrcat((*Disasm)->Opcode,temp);
    (*(*index))++;
}


void Mod_RM_SIB_EX(
                    DISASSEMBLY **Disasm,
                    char **Opcode, DWORD_PTR pos,
                    bool AddrPrefix,
                    int SEG,
                    DWORD_PTR **index,
                    BYTE Op,
                    bool PrefixReg,
                    bool PrefixSeg,
                    bool PrefixAddr,
                    BYTE Bit_d,
                    BYTE Bit_w,
                    BYTE RepPrefix
                )
{
	/*
		This Function will resolve BigSet menemonics: 
		Of MMX,3DNow! and New Set Instructions.
	*/

	// Set Defaults
	DWORD_PTR dwOp,dwMem;
    int RM=REG8,SCALE=0,SIB,ADDRM=REG32;
    WORD wOp,wMem;
    char RSize[10]="byte",Aritmathic[5]="+",tempAritmathic[5]="+";
	BYTE reg1=0,reg2=0,REG=0,Extension=0,FOpcode=0;
    char menemonic[128]="",tempMeme[128]="",Addr[15]="",temp[128]="";
	char instr[50]="";

    // Get used Register
	// Get target register, example:
	// 1. add byte ptr [ecx], -> al <-
	// 2. add -> al <- ,byte ptr [ecx]
    REG=(BYTE)(*(*Opcode+pos+1)); 
	REG>>=3;
	REG&=0x07;

    //Displacement MOD (none|BYTE/WORD|DWORD)
	Extension=(BYTE)(*(*Opcode+pos+1))>>6;

    switch((BYTE)(*(*Opcode+pos))){
      case 0x00:{Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]); } break; // WORD
      case 0x01:{
                 Bit_d=0; Bit_w=1;
                 switch(REG){
                    case 0: case 1: case 2: case 3: strcpy_s(RSize,StringLen(regSize[4])+1,regSize[4]);	break; // FWORD
                    case 4: case 5: case 6: strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]);			break; // WORD  
                    case 7:strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]);							break; // BYTE
                 }
                }
                break;
      case 0x02: case 0x03:{ Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }		break; // DWORD
      case 0x0D: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); }				break; // DWORD
      case 0x1F: { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }				break; // DWORD  (NOP)
	  case 0x10: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); }				break; // DQWORD
      case 0x11: { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); }				break; // DQWORD
      case 0x12: case 0x16: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); }		break; // QWORD
      case 0x13: case 0x17: { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); }		break; // QWORD
      case 0x14: case 0x15: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); }		break; // DQWORD
      case 0x18: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); }				break; // QWORD
      case 0x28: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); }				break; // DQWORD
      case 0x29: { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); }				break; // DQWORD
      case 0x2B: { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); }				break; // DQWORD (MOVNTPS)
      case 0x2A: case 0x2C: case 0x2D:case 0x2E: case 0x2F:                  
      {
          Bit_d=1; Bit_w=1;
          switch((Op&0x0F)-0x08){
            case 2: strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]);			break; // QWORD
            case 4: case 5: strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]);	break; // DQWORD
            case 6: case 7: strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);	break; // DWORD
          }
      }
      break;
      case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x47:
      case 0x48:case 0x49:case 0x4A:case 0x4B:case 0x4C:case 0x4D:case 0x4E:case 0x4F:
          { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); } // DWORD
      break;

      case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:
      case 0x58:case 0x59:case 0x5C:case 0x5D:case 0x5E:case 0x5F:
          { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); } // DQWORD
      break;
      case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:
      case 0x68:case 0x69:case 0x6A:case 0x6B:case 0x6C:case 0x6D:case 0x6E:
          { Bit_d=1; Bit_w=1; if((Op&0x0F)==0x0E)strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]);else strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); }  // DWORD/QWORD
      break;
      case 0x6F:{  // MOVQ (MMX) / MOVDQA (66) / MOVDQU (F3) — 128-bit with prefix
          Bit_d=1; Bit_w=1;
          if(PrefixReg || RepPrefix==0xF3)
              strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]);  // DQword (128-bit)
          else
              strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]);  // Qword (64-bit MMX)
      }
      break;
      case 0x70:case 0x74:case 0x75:case 0x76: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); } break; // QWORD
      case 0x7E: { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }  break;   // DWORD
      case 0x7F:{  // MOVQ (MMX) / MOVDQA (66) / MOVDQU (F3) — 128-bit with prefix
          Bit_d=0; Bit_w=1;
          if(PrefixReg || RepPrefix==0xF3)
              strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]);  // DQword (128-bit)
          else
              strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]);  // Qword (64-bit MMX)
      }
      break;
      case 0x90:case 0x91:case 0x92:case 0x93:case 0x94:case 0x95:case 0x96:case 0x97:
      case 0x98:case 0x99:case 0x9A:case 0x9B:case 0x9C:case 0x9D:case 0x9E:case 0x9F:
          { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]); }  // BYTE
      break;
      case 0xA3:case 0xA4:case 0xA5:case 0xAB:case 0xAC:case 0xAD:{ Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); } break;//DWORD
      case 0xAE:{ 
          Bit_d=1; Bit_w=1;
		  if(REG<2){
			strcpy_s(RSize,StringLen(regSize[9])+1,regSize[9]); // (512)Byte
		  }
		  else{
			strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); // DWORD
		  }
      } 
      break; //512Byte / DWORD  (FXSAVE)
      case 0xAF:			{ Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }				break; // DWORD  (IMUL)
      case 0x78:			{ Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }				break; // DWORD  (VMREAD)
      case 0x79:			{ Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }				break; // DWORD  (VMWRITE)
      case 0xB0:			{ Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]); }				break; // BYTE   (CMPXCHG)
      case 0xB1: case 0xB3: case 0xBA: case 0xBB:{ Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }	break; // DWORD  (CMPXCHG/BT/BTC/BTR)
      case 0xB2: case 0xB4: case 0xB5:{ Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[4])+1,regSize[4]); }	break; // FWORD  (LSS/LFS/LGS)
      case 0xB6: case 0xBE: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]); }				break; // BYTE   (MOVSX/MOVZX)
      case 0xB7: case 0xBF: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]); }				break; // WORD   (MOVSX/MOVZX)
      case 0xBC: case 0xBD: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }				break; // DWORD  (BSF/BSR)
      case 0xC0:			{ Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[3])+1,regSize[3]); }				break; // BYTE   (XADD)
      case 0xC1:			{ Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }				break; // DWORD  (XADD)
      case 0xC2: case 0xC6: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); }				break; // DQWORD (MIX)
      case 0xC3:			{ Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); }				break; // DWORD  (MOVNTI)
      case 0xC4:			{ Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]); }				break; // WORD   (MIX)
      case 0xC5: case 0xC7: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); }				break; // QWORD  (MIX)
      case 0xD1:case 0xD2:case 0xD3:case 0xD4:case 0xD5:case 0xD7:
      case 0xD8:case 0xD9:case 0xDA:case 0xDB:case 0xDC:
      case 0xDD:case 0xDE:case 0xDF:case 0xE0:case 0xE1:
      case 0xE2:case 0xE3:case 0xE4:case 0xE5:case 0xE8:
      case 0xE9:case 0xEA:case 0xEB:case 0xEC:case 0xED:
      case 0xEE:case 0xEF:
      case 0xF1:case 0xF2:case 0xF3:case 0xF5:case 0xF6:
      case 0xF7:case 0xF8:case 0xF9:case 0xFA:case 0xFB:case 0xFC:
      case 0xFD:case 0xFE:{
		  if((BYTE)(*(*Opcode))==0x66 || (BYTE)(*(*Opcode))==0xf3 || (BYTE)(*(*Opcode))==0xf2){
			  Bit_w=1; /* used prefixes has their own direction: [66|F2|F3]0FD6xxxx */
		  }else{
			  Bit_d=1; Bit_w=1;
		  }
          strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); // QWORD
      }
      break;

      case 0xD6:            { Bit_d=0; Bit_w=1; strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); } break; // QWORD  (MOVQ)
      case 0xE7: Bit_d=0; Bit_w=1; if(PrefixReg) strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); else strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); break; // DQWORD(movntdq) / QWORD(movntq)

      // SSE3 opcodes
      case 0x7C: case 0x7D: { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); } break; // DQWORD (haddpd/ps, hsubpd/ps)
      case 0xD0:            { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); } break; // DQWORD (addsubpd/ps)
      case 0xF0:            { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); } break; // DQWORD (lddqu)
      case 0x5A:            { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); } break; // DQWORD (cvt variants)
      case 0x5B:            { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); } break; // DQWORD (cvt variants)
      case 0xE6:            { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[8])+1,regSize[8]); } break; // DQWORD (cvt variants)
      case 0xB8:            { Bit_d=1; Bit_w=1; strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); } break; // DWORD  (popcnt)
    }

    	// check for bit register size : 16bit/32bit
	if(Bit_w==1){
	   RM=REG32; // 32bit registers set
	}
	
    if(PrefixReg==1){ // Change 32bit Data Size to 16Bit
        // All Opcodes with DWORD Data Size (excludes SSE2 opcodes where 66 is mandatory prefix)
        BYTE DOpcodes[31]={
                    0x02,0x03,
                    0xA3,0xA4,0xA5,0xAB,0xAC,0xAD,
                    0xAF,0xB1,0xB3,0xBB,0xBC,0xBD,
                    0xC1,0x40,0x41,0x42,0x43,0x44,
                    0x45,0x46,0x47,0x48,0x49,0x4A,
                    0x4B,0x4C,0x4D,0x4E,0x4F
        };

        for(int i=0;i<31;i++)
            if(Op==DOpcodes[i]){
                RM=REG16; // 16Bit registers
                strcpy_s(RSize,StringLen(regSize[2])+1,regSize[2]); // word ptr
                break;
            }
    }
    
    if(RepPrefix!=0 && Op!=0x6F && Op!=0x7F){
       strcpy_s(RSize,StringLen(regSize[1])+1,regSize[1]); // DWORD
    }

    // 64-bit mode: REX.W promotes operand size, REX.R extends REG
    if((*Disasm)->Mode64){
        if((*Disasm)->RexW){
            RM=REG64;
            strcpy_s(RSize,StringLen(regSize[0])+1,regSize[0]); // Qword ptr
        }
        if((*Disasm)->RexR) REG |= 0x08;
        if(!AddrPrefix)
            ADDRM=REG64;
        else
            ADDRM=REG32;
    }

    // SCALE INDEX BASE
	SIB=(BYTE)(*(*Opcode+pos+1))&0x07; // Get SIB extension

    // =================================================//
    //				AddrPrefix is being used!			//
    // =================================================//

    if(PrefixAddr==1 && !(*Disasm)->Mode64){ // Prefix 0x67 in 32-bit mode: 16-bit addressing
        FOpcode=((BYTE)(*(*Opcode+pos+1))&0x0F); // Get addressing Mode (8 types of mode)
        reg1=((BYTE)(*(*Opcode+pos+1))&0x38)>>3;
        
        // Choose Mode + Segment
        switch(FOpcode){
            case 0x00: case 0x08: wsprintf(Addr,"%s",addr16[0]); /*SEG=SEG_DS;*/	break;	// Mode 0:[BX+SI]
            case 0x01: case 0x09: wsprintf(Addr,"%s",addr16[1]); /*SEG=SEG_DS;*/	break;	// Mode 1:[BX+DI]
            case 0x02: case 0x0A: wsprintf(Addr,"%s",addr16[2]); SEG=SEG_SS;		break;	// Mode 2:[BP+SI]
            case 0x03: case 0x0B: wsprintf(Addr,"%s",addr16[3]); SEG=SEG_SS;		break;	// Mode 3:[BP+DI]
            case 0x04: case 0x0C: wsprintf(Addr,"%s",addr16[4]); /*SEG=SEG_DS;*/	break;	// Mode 4:[SI]
            case 0x05: case 0x0D: wsprintf(Addr,"%s",addr16[5]); /*SEG=SEG_DS;*/	break;	// Mode 5:[DI]
            case 0x06: case 0x0E: // Mode 6: [BP+XX/XXXX] | [XX]
            {
                if(Extension==0){ // 0x00-0x3F only! has special [XXXX]
                    /*SEG=SEG_DS;*/
                    SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
                    wsprintf(Addr,"%04X",wMem);
                    (*(*index))+=2; // read 2 bytes
                }
                else{ // 0x50-0xBF has [BP+]
                    SEG=SEG_SS; // SS Segment
                    wsprintf(Addr,"%s",addr16[7]);
                }
            }
            break;
            
            case 0x07: case 0x0F: wsprintf(Addr,"%s",addr16[6]); /*SEG=SEG_DS;*/ break; // Mode 7: [BX]
        }
        
        // Choose used extension 
        // And Decode properly the menemonic
        switch(Extension){
            case 0:{ // No extension of bytes to RegMem (except mode 6)
                wsprintf(tempMeme,"%s ptr %s:[%s]",RSize,segs[SEG],Addr);
                SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
                SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
                
                if(((wOp&0x00FF)&0x0F)==0x06 || ((wOp&0x00FF)&0x0F)==0x0E ){ // 0x00-0x3F with mode 6 only!
                    wsprintf(menemonic,"%08X",dwOp);
                    (*Disasm)->OpcodeSize=4;
                    lstrcat((*Disasm)->Opcode,menemonic);
                    FOpcode=(BYTE)(*(*Opcode+pos+4));
                }
                else{ // other modes
                    wsprintf(menemonic,"%04X",wOp);
                    (*Disasm)->OpcodeSize=2;
                    lstrcat((*Disasm)->Opcode,menemonic);
                    FOpcode=(BYTE)(*(*Opcode+pos+2));
                }
            }
            break;
            
            case 1:{ // 1 Byte Extension to regMem
                SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
                FOpcode=wOp&0x00FF;
                
                if(FOpcode>0x7F){ // check for signed numbers
                    wsprintf(Aritmathic,"%s",Scale[0]); // '-' Signed Numbers
                    FOpcode = 0x100-FOpcode; // -XX
                }
                wsprintf(menemonic,"%02X%04X",Op,wOp);
                lstrcat((*Disasm)->Opcode,menemonic);
                wsprintf(tempMeme,"%s ptr %s:[%s%s%02Xh]",RSize,segs[SEG],Addr,Aritmathic,FOpcode);
                ++(*(*index)); // 1 byte read
                (*Disasm)->OpcodeSize=3;
                FOpcode=(BYTE)(*(*Opcode+pos+3));
            }
            break;
            
            case 2:{ // 2 Bytes Extension to RegMem
                SwapDword((BYTE*)(*Opcode+pos),&dwOp,&dwMem);
                SwapWord((BYTE*)(*Opcode+pos+2),&wOp,&wMem);
                wsprintf(menemonic,"%08X",dwOp);
                (*Disasm)->OpcodeSize=4;
                lstrcat((*Disasm)->Opcode,menemonic);
                wsprintf(tempMeme,"%s ptr %s:[%s%s%04Xh]",RSize,segs[SEG],Addr,Aritmathic,wMem);
                (*(*index))+=2; // we read 2 bytes
                FOpcode=(BYTE)(*(*Opcode+pos+4));
            }
            break;
		}
        
        switch(Bit_d){
            case 0:{ // direction (->)
                switch(Op){
                    case 0x00:{
                        wsprintf(temp,"%s %s",NewSet[REG],tempMeme);

                        if(REG>5) // Invalid operation
                            lstrcat((*Disasm)->Remarks,";Invalid instruction");
                    }
                    break;

                    case 0x01:{
                        wsprintf(temp,"%s %s",NewSet2[REG],tempMeme);

						if(REG==5){ // Invalid operation
                            lstrcat((*Disasm)->Remarks,";Invalid instruction");
						}
                    }
                    break;

                    case 0x1F:{
                        wsprintf(temp,"nop %s",tempMeme);
                    }
                    break;

                    case 0x11:{
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"movss %s,%s",tempMeme,MMXRegs[REG]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(temp,"movsd %s,%s",tempMeme,MMXRegs[REG]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
						else if(PrefixReg){
                            wsprintf(temp,"movupd %s,%s",tempMeme,MMXRegs[REG]);
						}
						else{
                            wsprintf(temp,"movups %s,%s",tempMeme,MMXRegs[REG]);
						}
                    }
                    break; // MOVUPS/MOVSS/MOVSD/MOVUPD
                    case 0x13:{
                        if(PrefixReg)
                            wsprintf(temp,"movlpd %s,%s",tempMeme,MMXRegs[REG]);
                        else
                            wsprintf(temp,"movlps %s,%s",tempMeme,MMXRegs[REG]);
                    }
                    break; // MOVLPS/MOVLPD
                    case 0x17: wsprintf(temp,"movhps %s,%s",tempMeme,MMXRegs[REG]); break; // MOVHPS
                    case 0x29:{
                        if(PrefixReg)
                            wsprintf(temp,"movapd %s,%s",tempMeme,MMXRegs[REG]);
                        else
                            wsprintf(temp,"movaps %s,%s",tempMeme,MMXRegs[REG]);
                    }
                    break; // MOVAPS/MOVAPD
                    case 0x2B:{
                        if(PrefixReg)
                            wsprintf(temp,"movntpd %s,%s",tempMeme,MMXRegs[REG]);
                        else
                            wsprintf(temp,"movntps %s,%s",tempMeme,MMXRegs[REG]);
                    }
                    break; // MOVNTPS/MOVNTPD
                    case 0x7E:{
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"movq %s,%s",MMXRegs[REG],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(temp,"%s %s,%s",NewSet7[Op&0x0F],tempMeme,MMXRegs[REG]);
                        else
                            wsprintf(temp,"%s %s,%s",NewSet7[Op&0x0F],tempMeme,Regs3DNow[REG]);
                    }
                    break; // MOVD/MOVQ
                    case 0x7F:{
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"movdqu %s,%s",tempMeme,MMXRegs[REG]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(temp,"movdqa %s,%s",tempMeme,MMXRegs[REG]);
                        else
                            wsprintf(temp,"%s %s,%s",NewSet7[Op&0x0F],tempMeme,Regs3DNow[REG]);
                    }
                    break; // MOVQ/MOVDQA/MOVDQU
                    case 0xC3: wsprintf(temp,"movnti %s,%s",tempMeme,regs[RM][REG]); break; // MOVNTI
                    case 0xD6:{
                        if(PrefixReg)
                            wsprintf(temp,"movq %s,%s",tempMeme,MMXRegs[REG]);
                        else{
                            wsprintf(temp,"??? %s,%s",tempMeme,Regs3DNow[REG]);
                        }
                    }
                    break; // MOVQ (66 0F D6)
                    case 0xE7:{
                        if(PrefixReg)
                            wsprintf(temp,"movntdq %s,%s",tempMeme,MMXRegs[REG]);
                        else
                            wsprintf(temp,"%s %s,%s",NewSet14[Op&0x07],tempMeme,Regs3DNow[REG]);
                    }
                    break;
                    case 0xA3: case 0xAB:{
                      wsprintf(temp,"%s %s,%s",NewSet10[Op&0x0F],tempMeme,regs[RM][REG]);
                    }
                    break;

                    case 0xA4:case 0xAC:{
                       wsprintf(temp,"%s %s,%s,%02Xh",NewSet10[Op&0x0F],tempMeme,regs[RM][REG],FOpcode);
                       wsprintf(menemonic," %02X",FOpcode);
                       lstrcat((*Disasm)->Opcode,menemonic);
                       (*Disasm)->OpcodeSize++;
                       (*(*index))++;
                    }
                    break;

                    case 0xA5: case 0xAD:{
                      wsprintf(temp,"%s %s,%s,cl",NewSet10[Op&0x0F],tempMeme,regs[RM][REG]);
                    }
                    break;

                    case 0xB0: case 0xB1: case 0xB3: case 0xBB:{
						if((Op&0x0F)==0x00){
							RM=REG8;
						}

                      wsprintf(temp,"%s %s,%s",NewSet11[Op&0x0F],tempMeme,regs[RM][REG]);
                    }
                    break;

                    case 0xBA:{
                        const char *btOps[] = {"???","???","???","???","bt","bts","btr","btc"};
                        BYTE regField = REG & 0x07;
                        if(regField < 4){
                            wsprintf(temp,"??? %s",tempMeme);
                            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
                        } else {
                            wsprintf(temp,"%s %s,%02Xh",btOps[regField],tempMeme,FOpcode);
                        }
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0x78: wsprintf(temp,"vmread %s,%s",tempMeme,regs[RM][REG]); break; // VMREAD

                    case 0xC0:{
                        RM=REG8;
                        wsprintf(temp,"xadd %s,%s",tempMeme,regs[RM][REG]);
                    }
                    break; // XADD

                    case 0xC1:wsprintf(temp,"xadd %s,%s",tempMeme,regs[RM][REG]); break;

                }
                lstrcat((*Disasm)->Assembly,temp);
            }
            break;

            case 1:{ // direction (<-)
                switch(Op){
                    case 0x02: wsprintf(temp,"lar %s,%s",regs[RM][REG],tempMeme);     break; // LAR
                    case 0x03: wsprintf(temp,"lsl %s,%s",regs[RM][REG],tempMeme);     break; // LAR
                    case 0x10:{
                      if(RepPrefix==0xF3){
                        wsprintf(temp,"movss %s,%s",MMXRegs[REG],tempMeme);
                        strcpy_s((*Disasm)->Assembly,"");
                        (*Disasm)->OpcodeSize++;
                      }
                      else if(RepPrefix==0xF2){
                        wsprintf(temp,"movsd %s,%s",MMXRegs[REG],tempMeme);
                        strcpy_s((*Disasm)->Assembly,"");
                        (*Disasm)->OpcodeSize++;
                      }
					  else if(PrefixReg){
						wsprintf(temp,"movupd %s,%s",MMXRegs[REG],tempMeme);
					  }
					  else{
						wsprintf(temp,"movups %s,%s",MMXRegs[REG],tempMeme);
					  }
                    }
                    break; // MOVUPS/MOVSS/MOVSD/MOVUPD

                    case 0x12:{  // MOVLPS / MOVLPD (66) / MOVDDUP (F2) / MOVSLDUP (F3)
                        if(RepPrefix==0xF2){
                            wsprintf(temp,"movddup %s,%s",MMXRegs[REG],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF3){
                            wsprintf(temp,"movsldup %s,%s",MMXRegs[REG],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(temp,"movlpd %s,%s",MMXRegs[REG],tempMeme);
                        else{
                            wsprintf(temp,"movlps %s,%s",MMXRegs[REG],tempMeme);
                        }
                    }
                    break; // MOVLPS/MOVLPD
                    case 0x14:{
                        if(PrefixReg) wsprintf(temp,"unpcklpd %s,%s",MMXRegs[REG],tempMeme);
                        else wsprintf(temp,"unpcklps %s,%s",MMXRegs[REG],tempMeme);
                    }
                    break; // UNPCKLPS/UNPCKLPD
                    case 0x15:{
                        if(PrefixReg) wsprintf(temp,"unpckhpd %s,%s",MMXRegs[REG],tempMeme);
                        else wsprintf(temp,"unpckhps %s,%s",MMXRegs[REG],tempMeme);
                    }
                    break; // UNPCKHPS/UNPCKHPD
                    case 0x16:{ // MOVHPS / MOVHPD (66) / MOVSHDUP (F3)
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"movshdup %s,%s",MMXRegs[REG],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(temp,"movhpd %s,%s",MMXRegs[REG],tempMeme);
                        else{
                            wsprintf(temp,"movhps %s,%s",MMXRegs[REG],tempMeme);
                        }
                    }
                    break; // MOVHPS/MOVHPD

                    case 0x18:{
                      wsprintf(temp,"%s,%s",NewSet3[REG],tempMeme);
                      if(REG>3)
                         lstrcat((*Disasm)->Remarks,";Invalid instruction");
                    } 
                    break; // MIX
                    
                    case 0x28:{
                        if(PrefixReg) wsprintf(temp,"movapd %s,%s",MMXRegs[REG],tempMeme);
                        else wsprintf(temp,"movaps %s,%s",MMXRegs[REG],tempMeme);
                    }
                    break; // MOVAPS/MOVAPD

                    case 0x2A: case 0x2C: case 0x2D:case 0x2E: case 0x2F:{
                      BYTE R=((Op&0x0F)-0x08);
					  if(R==4 || R==5){
                          wsprintf(instr,"%s",Regs3DNow[REG]); // 3DNow! Regs
					  }
					  else{
                          wsprintf(instr,"%s",MMXRegs[REG]); // MMX Regs
					  }
                        
                      if(RepPrefix!=0){
                          if(Op==0x2A || Op==0x2C || Op==0x2D){
                              if(RepPrefix==0xF3){
                                  switch(Op){
                                    case 0x2A:strcpy_s(menemonic,"cvtsi2ss");  break;
                                    case 0x2C:{
                                      strcpy_s(menemonic,"cvttss2si");
                                      wsprintf(instr,"%s",regs[RM][REG]);
                                    }
                                    break;
                                    case 0x2D:{
                                       strcpy_s(menemonic,"cvtss2si");
                                       wsprintf(instr,"%s",regs[RM][REG]);
                                    }
                                    break;
                                  }
                              }
                              else{ // F2 prefix
                                  switch(Op){
                                    case 0x2A:strcpy_s(menemonic,"cvtsi2sd");  break;
                                    case 0x2C:{
                                      strcpy_s(menemonic,"cvttsd2si");
                                      wsprintf(instr,"%s",regs[RM][REG]);
                                    }
                                    break;
                                    case 0x2D:{
                                       strcpy_s(menemonic,"cvtsd2si");
                                       wsprintf(instr,"%s",regs[RM][REG]);
                                    }
                                    break;
                                  }
                              }
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                              wsprintf(temp,"%s %s,%s",menemonic,instr,tempMeme);
                          }
                      }
					  else{
                        wsprintf(temp,"%s %s,%s",NewSet4[(Op&0x0F)-0x08],instr,tempMeme);
					  }
                    }
                    break; // MIX
                    
                    case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x47:
                    case 0x48:case 0x49:case 0x4A:case 0x4B:case 0x4C:case 0x4D:case 0x4E:case 0x4F:
                    {
                       wsprintf(temp,"%s %s,%s",NewSet5[Op&0x0F],regs[RM][REG],tempMeme); 
                    }
                    break; // MIX

                    case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:
                    case 0x58:case 0x59:case 0x5C:case 0x5D:case 0x5E:case 0x5F:
                    {
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"%s %s,%s",NewSet6Ex[Op&0x0F],MMXRegs[reg2],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(temp,"%s %s,%s",NewSet6_F2[Op&0x0F],MMXRegs[reg2],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
						else if(PrefixReg){
                            wsprintf(temp,"%s %s,%s",NewSet6_66[Op&0x0F],MMXRegs[REG],tempMeme);
						}
						else{
                            wsprintf(temp,"%s %s,%s",NewSet6[Op&0x0F],MMXRegs[REG],tempMeme);
						}
                    }
                    break; // MIX

                    case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:
                    case 0x68:case 0x69:case 0x6A:case 0x6B:case 0x6E:case 0x6F:
                    {
                        if(Op==0x6F && RepPrefix==0xF3){
                            wsprintf(temp,"movdqu %s,%s",MMXRegs[REG],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(Op==0x6F && PrefixReg){
                            wsprintf(temp,"movdqa %s,%s",MMXRegs[REG],tempMeme);
                        }
                        else if(PrefixReg){
                            wsprintf(temp,"%s %s,%s",NewSet7[Op&0x0F],MMXRegs[REG],tempMeme);
                        }
                        else{
                            wsprintf(temp,"%s %s,%s",NewSet7[Op&0x0F],Regs3DNow[REG],tempMeme);
                        }
                    }
                    break;

                    case 0x70:{
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"pshufhw %s,%s,%02X",MMXRegs[REG],tempMeme,FOpcode);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(temp,"pshuflw %s,%s,%02X",MMXRegs[REG],tempMeme,FOpcode);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg){
                            wsprintf(temp,"pshufd %s,%s,%02X",MMXRegs[REG],tempMeme,FOpcode);
                        }
                        else{
                            wsprintf(temp,"%s %s,%s,%02X",NewSet8[Op&0x0F],Regs3DNow[reg2],tempMeme,FOpcode);
                        }
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0x74:case 0x75:case 0x76:{
                        if(PrefixReg)
                            wsprintf(temp,"%s %s,%s",NewSet8[Op&0x0F],MMXRegs[REG],tempMeme);
                        else
                            wsprintf(temp,"%s %s,%s",NewSet8[Op&0x0F],Regs3DNow[REG],tempMeme);
                    }
                    break;

                    case 0x90:case 0x91:case 0x92:case 0x93:case 0x94:case 0x95:case 0x96:case 0x97:
                    case 0x98:case 0x99:case 0x9A:case 0x9B:case 0x9C:case 0x9D:case 0x9E:case 0x9F:
                    {
						wsprintf(temp,"%s %s",NewSet9[Op&0x0F],tempMeme);
                    }
                    break; // MIX

                    case 0xAE:{ // FXSAVE/XSAVE/etc
                        wsprintf(temp,"%s %s",NewSet10Ex[REG],tempMeme);
                    }
                    break;
                    case 0xAF:wsprintf(temp,"%s %s,%s",NewSet10[Op&0x0F],regs[RM][REG],tempMeme);break;
                    
                    case 0xBC:{ // BSF or TZCNT (F3 0F BC)
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"tzcnt %s,%s",regs[RM][REG],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(temp,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][REG],tempMeme);
                        }
                    }
                    break;
                    case 0xBD:{ // BSR or LZCNT (F3 0F BD)
                        if(RepPrefix==0xF3){
                            wsprintf(temp,"lzcnt %s,%s",regs[RM][REG],tempMeme);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(temp,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][REG],tempMeme);
                        }
                    }
                    break;
                    case 0xB2:case 0xB4:case 0xB5:
                    case 0xB6:case 0xB7:
                    case 0xBE:case 0xBF:
                    {
                      wsprintf(temp,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][REG],tempMeme);
                    }
                    break;

                    case 0xC2:{
                      if(FOpcode<8){
                          if(RepPrefix==0xF3){
                              wsprintf(temp,"%s %s,%s",NewSet12Ex[FOpcode],MMXRegs[REG],tempMeme);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
                          else if(RepPrefix==0xF2){
                              wsprintf(temp,"%s %s,%s",NewSet12_F2[FOpcode],MMXRegs[REG],tempMeme);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
						  else if(PrefixReg){
                              wsprintf(temp,"%s %s,%s",NewSet12_66[FOpcode],MMXRegs[REG],tempMeme);
						  }
						  else{
                              wsprintf(temp,"%s %s,%s",NewSet12[FOpcode],MMXRegs[REG],tempMeme);
						  }
                      }
                      else{
                          if(RepPrefix==0xF3){
                              wsprintf(temp,"cmpss %s,%s,%02X",MMXRegs[REG],tempMeme,FOpcode);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
                          else if(RepPrefix==0xF2){
                              wsprintf(temp,"cmpsd %s,%s,%02X",MMXRegs[REG],tempMeme,FOpcode);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
						  else if(PrefixReg){
                              wsprintf(temp,"cmppd %s,%s,%02X",MMXRegs[REG],tempMeme,FOpcode);
						  }
						  else{
                              wsprintf(temp,"cmpps %s,%s,%02X",MMXRegs[REG],tempMeme,FOpcode);
						  }
                      }

                      wsprintf(menemonic," %02X",FOpcode);
                      lstrcat((*Disasm)->Opcode,menemonic);
                      (*Disasm)->OpcodeSize++;
                      (*(*index))++;
                    }
                    break;

                    case 0xC4:{
                        if(PrefixReg)
                            wsprintf(temp,"pinsrw %s,%s,%02Xh",MMXRegs[REG],tempMeme,FOpcode);
                        else
                            wsprintf(temp,"pinsrw %s,%s,%02Xh",Regs3DNow[REG],tempMeme,FOpcode);
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0xC6:{
                        if(PrefixReg)
                            wsprintf(temp,"shufpd %s,%s,%02Xh",MMXRegs[REG],tempMeme,FOpcode);
                        else
                            wsprintf(temp,"shufps %s,%s,%02Xh",MMXRegs[REG],tempMeme,FOpcode);
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0xD7:{
                        RM=REG32;
                        wsprintf(temp,"%s %s,%s",NewSet13[Op&0x0F],regs[RM][REG],tempMeme);
                    }
                    break;
                    case 0xD1:case 0xD2:case 0xD3:case 0xD4:case 0xD5:case 0xD8:case 0xDF:
                    case 0xD9:case 0xDA:case 0xDB:case 0xDC:case 0xDD:case 0xDE:
                    {
                        if(PrefixReg)
                            wsprintf(temp,"%s %s,%s",NewSet13[Op&0x0F],MMXRegs[REG],tempMeme);
                        else
                            wsprintf(temp,"%s %s,%s",NewSet13[Op&0x0F],Regs3DNow[REG],tempMeme);
                    }
                    break;

                    case 0xE0:case 0xE1:case 0xE2:case 0xE3:
                    case 0xE4:case 0xE5:case 0xE8:case 0xE9:
                    case 0xEA:case 0xEB:case 0xEC:case 0xED:
                    case 0xEE:case 0xEF:{
                        if(PrefixReg)
                            wsprintf(temp,"%s %s,%s",NewSet14[Op&0x0F],MMXRegs[REG],tempMeme);
                        else
                            wsprintf(temp,"%s %s,%s",NewSet14[Op&0x0F],Regs3DNow[REG],tempMeme);
                    }
                    break;

                    case 0x79: wsprintf(temp,"vmwrite %s,%s",regs[RM][REG],tempMeme); break; // VMWRITE

                    case 0xC7:{ // CMPXCHG8B/16B / VMPTRLD / VMCLEAR / VMXON / VMPTRST
                        BYTE regField = REG & 0x07;
                        if(regField==1){
                            if((*Disasm)->Mode64 && (*Disasm)->RexW)
                                wsprintf(temp,"cmpxchg16b %s",tempMeme);
                            else
                                wsprintf(temp,"cmpxchg8b %s",tempMeme);
                        }
                        else if(regField==6){
                            if(RepPrefix==0xF3){
                                wsprintf(temp,"vmxon %s",tempMeme);
                                strcpy_s((*Disasm)->Assembly,"");
                                (*Disasm)->OpcodeSize++;
                            }
                            else if(PrefixReg){
                                wsprintf(temp,"vmclear %s",tempMeme);
                            }
                            else{
                                wsprintf(temp,"vmptrld %s",tempMeme);
                            }
                        }
                        else if(regField==7){
                            wsprintf(temp,"vmptrst %s",tempMeme);
                        }
                        else{
                            wsprintf(temp,"??? %s",tempMeme);
                            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
                        }
                    }
                    break;

                    case 0xF1:case 0xF2:case 0xF3:case 0xF5:case 0xF6:
                    case 0xF7:case 0xF8:case 0xF9:case 0xFA:case 0xFB:case 0xFC:
                    case 0xFD:case 0xFE:{
                        if(PrefixReg){
                            if(Op==0xF7)
                                wsprintf(temp,"maskmovdqu %s,%s",MMXRegs[REG],tempMeme);
                            else
                                wsprintf(temp,"%s %s,%s",NewSet15[Op&0x0F],MMXRegs[REG],tempMeme);
                        }
                        else{
                            wsprintf(temp,"%s %s,%s",NewSet15[Op&0x0F],Regs3DNow[REG],tempMeme);
                        }
                    }
                    break;
                }

                lstrcat((*Disasm)->Assembly,temp);
            }
            break;
        }

        ++(*(*index)); // add 1 byte to index
		return; // no need to continue!! exit the function and proeed with decoding next bytes.
    }
    
    //==================================================//
    //				NO SIB Being used!					//
    //==================================================//
    
    if(SIB!=SIB_EX){ // NO SIB extension (i.e: 0x0001 = add byte ptr [ecx], al)
        reg1=((BYTE)(*(*Opcode+pos+1))&0x07); // get register (we have only one)
        reg2=(((BYTE)(*(*Opcode+pos+1))&0x38)>>3);

        // 64-bit mode: apply REX.B to r/m field, REX.R to reg field
        if((*Disasm)->Mode64){
            if((*Disasm)->RexB) reg1 |= 0x08;
            if((*Disasm)->RexR) reg2 |= 0x08;
        }

        switch(Extension){ // Check what extension we have (None/Byte/Dword)
            case 00:{ // no extension to regMem
                if((reg1&0x07)==REG_EBP){ // mod=00 rm=5: [disp32] or [RIP+disp32]
                    SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
                    SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
                    wsprintf(menemonic,"%04X%08X",wOp,dwOp);
                    lstrcat((*Disasm)->Opcode,menemonic);
                    if((*Disasm)->Mode64){
                        wsprintf(instr,"%08Xh",dwMem);
                        wsprintf(menemonic,"%s ptr [RIP+%s]",RSize,instr);
                    }else{
                        wsprintf(instr,"%08Xh",dwMem);
                        wsprintf(menemonic,"%s ptr %s:[%s]",RSize,segs[SEG],instr);
                    }
                    (*Disasm)->OpcodeSize=6;
                    (*(*index))+=5;
                    FOpcode=(BYTE)(*(*Opcode+pos+6));
                }
                else{
                    SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
                    wsprintf(menemonic,"%04X",wOp);
                    lstrcat((*Disasm)->Opcode,menemonic);
                    wsprintf(menemonic,"%s ptr %s:[%s]",RSize,segs[SEG],regs[ADDRM][reg1]);
                    ++(*(*index)); // only 1 byte read
                    (*Disasm)->OpcodeSize=2; // total used opcodes
                    FOpcode=(BYTE)(*(*Opcode+pos+2));
                }
            }
            break;
            
            case 01:{ // 1 byte extension to regMem
                FOpcode=(BYTE)(*(*Opcode+pos+2));
                SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
                wsprintf(menemonic,"%04X%02X",wOp,FOpcode);
                lstrcat((*Disasm)->Opcode,menemonic);
                
                if(FOpcode>0x7F){ // check for signed numbers
                    wsprintf(Aritmathic,"%s",Scale[0]); // '-' arithmetic
                    FOpcode = 0x100-FOpcode; // -XX
                }
                
				if(reg1==REG_EBP && PrefixSeg==0){
                    SEG=SEG_SS;
				}
                
                wsprintf(menemonic,"%s ptr %s:[%s%s%02Xh]",RSize,segs[SEG],regs[ADDRM][reg1],Aritmathic,FOpcode);
                (*(*index))+=2; // x + 1 byte(s) read
                (*Disasm)->OpcodeSize=3; // total used opcodes
                FOpcode=(BYTE)(*(*Opcode+pos+3));
            }
            break;
            
            case 02:{ // 4 byte extension to regMem
                // if ebp and there is no prefix 0x67, use SS segment
				if(reg1==REG_EBP && PrefixSeg==0){
                    SEG=SEG_SS;
				}
                
                SwapDword((BYTE*)(*Opcode+pos+2),&dwOp,&dwMem);
                SwapWord((BYTE*)(*Opcode+pos),&wOp,&wMem);
                wsprintf(menemonic,"%04X %08X",wOp,dwOp);
                lstrcat((*Disasm)->Opcode,menemonic);
                wsprintf(instr,"%08Xh",dwMem);
                wsprintf(menemonic,"%s ptr %s:[%s+%s]",RSize,segs[SEG],regs[ADDRM][reg1],instr);
                (*(*index))+=5; // x + 1 + 4 byte(s) read
                (*Disasm)->OpcodeSize=6; // total used opcodes
                FOpcode=(BYTE)(*(*Opcode+pos+6));
            }
            break;
        }
        
        switch(Bit_d){            
            case 0:{ // direction (->)
                switch(Op){ // Check for all Cases Available
                    case 0x00:{ // MIX Instructions
                        wsprintf(tempMeme,"%s %s",NewSet[REG],menemonic);
                        
						if(REG>5){ // Invalid operation
                            lstrcat((*Disasm)->Remarks,";Invalid instruction");
						}
                    }
                    break;

                    case 0x01:{ // MIX Instructions
                        wsprintf(tempMeme,"%s %s",NewSet2[REG],menemonic);

						if(REG==5){ // Invalid operation
                            lstrcat((*Disasm)->Remarks,";Invalid instruction");
						}
                    }
                    break;

                    case 0x1F:{
                        wsprintf(tempMeme,"nop %s",menemonic);
                    }
                    break;

                    case 0x11:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movss %s,%s",menemonic,MMXRegs[reg2]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"movsd %s,%s",menemonic,MMXRegs[reg2]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
						else if(PrefixReg){
                            wsprintf(tempMeme,"movupd %s,%s",menemonic,MMXRegs[reg2]);
						}
						else{
                            wsprintf(tempMeme,"movups %s,%s",menemonic,MMXRegs[reg2]);
						}
                    }
                    break; // MOVUPS/MOVSS/MOVSD/MOVUPD
                    case 0x13:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"movlpd %s,%s",menemonic,MMXRegs[reg2]);
                        else
                            wsprintf(tempMeme,"movlps %s,%s",menemonic,MMXRegs[reg2]);
                    }
                    break; // MOVLPS/MOVLPD
                    case 0x17: wsprintf(tempMeme,"movhps %s,%s",menemonic,MMXRegs[reg2]); break; // MOVHPS
                    case 0x29:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"movapd %s,%s",menemonic,MMXRegs[reg2]);
                        else
                            wsprintf(tempMeme,"movaps %s,%s",menemonic,MMXRegs[reg2]);
                    }
                    break; // MOVAPS/MOVAPD
                    case 0x2B:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"movntpd %s,%s",menemonic,MMXRegs[reg2]);
                        else
                            wsprintf(tempMeme,"movntps %s,%s",menemonic,MMXRegs[reg2]);
                    }
                    break; // MOVNTPS/MOVNTPD
                    case 0x7E:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movq %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],menemonic,MMXRegs[reg2]);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],menemonic,Regs3DNow[reg2]);
                    }
                    break; // MOVD/MOVQ
                    case 0x7F:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movdqu %s,%s",menemonic,MMXRegs[reg2]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(tempMeme,"movdqa %s,%s",menemonic,MMXRegs[reg2]);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],menemonic,Regs3DNow[reg2]);
                    }
                    break; // MOVQ/MOVDQA/MOVDQU
                    case 0xC3: wsprintf(tempMeme,"movnti %s,%s",menemonic,regs[RM][reg2]); break; // MOVNTI
					case 0xD6:{
						if(PrefixReg)
							wsprintf(tempMeme,"movq %s,%s",menemonic,MMXRegs[reg2]);
						else
							wsprintf(tempMeme,"??? %s,%s",menemonic,Regs3DNow[reg2]);
					}
					break; // MOVQ (66 0F D6)
					case 0xE7:{
						if(PrefixReg)
							wsprintf(tempMeme,"movntdq %s,%s",menemonic,MMXRegs[reg2]);
						else
							wsprintf(tempMeme,"%s %s,%s",NewSet14[Op&0x07],menemonic,Regs3DNow[reg2]);
					}
					break;
                    case 0xA3: case 0xAB:{
                        wsprintf(tempMeme,"%s %s,%s",NewSet10[Op&0x0F],menemonic,regs[RM][reg2]);
                    }
                    break;
                    case 0xA4:case 0xAC:{
                       wsprintf(tempMeme,"%s %s,%s,%02Xh",NewSet10[Op&0x0F],menemonic,regs[RM][reg2],FOpcode);
                       wsprintf(menemonic," %02X",FOpcode);
                       lstrcat((*Disasm)->Opcode,menemonic);
                       (*Disasm)->OpcodeSize++;
                       (*(*index))++;
                    }
                    break;
                    case 0xA5: case 0xAD:wsprintf(tempMeme,"%s %s,%s,cl",NewSet10[Op&0x0F],menemonic,regs[RM][reg2]); break;
                    
                    case 0xB0: case 0xB1:  case 0xB3: case 0xBB:{
						if((Op&0x0F)==0x00){
                            RM=REG8;
						}

                        wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],menemonic,regs[RM][reg2]);
                    }
                    break;

                    case 0xBA:{
                        const char *btOps[] = {"???","???","???","???","bt","bts","btr","btc"};
                        BYTE regField = REG & 0x07;
                        if(regField < 4){
                            wsprintf(tempMeme,"??? %s",menemonic);
                            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
                        } else {
                            wsprintf(tempMeme,"%s %s,%02Xh",btOps[regField],menemonic,FOpcode);
                        }
                        wsprintf(temp," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,temp);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0x78: wsprintf(tempMeme,"vmread %s,%s",menemonic,regs[RM][reg2]); break; // VMREAD

                    case 0xC0: RM=REG8; wsprintf(tempMeme,"xadd %s,%s",menemonic,regs[RM][reg2]);break;
                    case 0xC1: wsprintf(tempMeme,"xadd %s,%s",menemonic,regs[RM][reg2]); break;
                }

                lstrcat((*Disasm)->Assembly,tempMeme); // copy the decoded assembly
            }
            break;

            case 1:{ // direction (<-)
                switch(Op){
                    case 0x02: wsprintf(tempMeme,"lar %s,%s",regs[RM][reg2],menemonic);	break; // LAR
                    case 0x03: wsprintf(tempMeme,"lsl %s,%s",regs[RM][reg2],menemonic);	break; // LSL
					case 0x0D:{
						FOpcode=(BYTE)(*(*Opcode+pos+1));
						if(
							(FOpcode>=0x00 &&FOpcode<=0x0F) ||
							(FOpcode>=0x40 &&FOpcode<=0x4F) ||
							(FOpcode>=0x80 &&FOpcode<=0x8F)
							)
						{
							if((FOpcode&0x0F)<=0x07){
								wsprintf(tempMeme,"prefetch %s",menemonic);
							}
							else{
								wsprintf(tempMeme,"prefetchW %s",menemonic);
							}
						}
						else{
							strcpy_s(tempMeme,"???");
						}
					}
					break;
					
					case 0x10:{
						if(RepPrefix==0xF3){
							strcpy_s((*Disasm)->Assembly,"");
							(*Disasm)->OpcodeSize++;
							wsprintf(tempMeme,"movss %s,%s",MMXRegs[reg2],menemonic);
						}
						else if(RepPrefix==0xF2){
							strcpy_s((*Disasm)->Assembly,"");
							(*Disasm)->OpcodeSize++;
							wsprintf(tempMeme,"movsd %s,%s",MMXRegs[reg2],menemonic);
						}
						else if(PrefixReg){
							wsprintf(tempMeme,"movupd %s,%s",MMXRegs[reg2],menemonic);
						}
						else{
							wsprintf(tempMeme,"movups %s,%s",MMXRegs[reg2],menemonic);
						}
                    }
                    break; // MOVUPS/MOVSS/MOVSD/MOVUPD
                    case 0x14:{
                        if(PrefixReg) wsprintf(tempMeme,"unpcklpd %s,%s",MMXRegs[reg2],menemonic);
                        else wsprintf(tempMeme,"unpcklps %s,%s",MMXRegs[reg2],menemonic);
                    }
                    break; // UNPCKLPS/UNPCKLPD
                    case 0x15:{
                        if(PrefixReg) wsprintf(tempMeme,"unpckhpd %s,%s",MMXRegs[reg2],menemonic);
                        else wsprintf(tempMeme,"unpckhps %s,%s",MMXRegs[reg2],menemonic);
                    }
                    break; // UNPCKHPS/UNPCKHPD
                    
                    case 0x18:{
                        wsprintf(tempMeme,"%s %s",NewSet3[REG],menemonic);
						if(REG>3){
                            lstrcat((*Disasm)->Remarks,";Invalid instruction");
						}
                    } 
                    break;
                    
                    case 0x28:{
                        if(PrefixReg) wsprintf(tempMeme,"movapd %s,%s",MMXRegs[reg2],menemonic);
                        else wsprintf(tempMeme,"movaps %s,%s",MMXRegs[reg2],menemonic);
                    }
                    break; // MOVAPS/MOVAPD

                    case 0x2A: case 0x2C: case 0x2D:case 0x2E: case 0x2F:{
                       BYTE R=((Op&0x0F)-0x08);

					   if(R==4 || R==5){
                           wsprintf(temp,"%s",Regs3DNow[reg2]); // 3DNow! Regs
					   }
					   else{
                           wsprintf(temp,"%s",MMXRegs[reg2]); // MMX Regs
					   }

                       if(PrefixReg && (Op==0x2E || Op==0x2F)){
                           wsprintf(tempMeme,"%s %s,%s",(Op==0x2E)?"ucomisd":"comisd",MMXRegs[reg2],menemonic);
                       }
                       else if(RepPrefix!=0){
                           char instruction[20];
                           if(Op==0x2A || Op==0x2C || Op==0x2D){
                               if(RepPrefix==0xF3){
                                   switch(Op){
                                        case 0x2A:strcpy_s(instruction,"cvtsi2ss"); break;
                                        case 0x2C:{
                                            wsprintf(temp,"%s",regs[RM][reg2]);
                                            strcpy_s(instruction,"cvttss2si");
                                        }
                                        break;
                                        case 0x2D:{
                                            wsprintf(temp,"%s",regs[RM][reg2]);
                                            strcpy_s(instruction,"cvtss2si");
                                        }
                                        break;
                                   }
                               }
                               else{ // F2 prefix
                                   switch(Op){
                                        case 0x2A:strcpy_s(instruction,"cvtsi2sd"); break;
                                        case 0x2C:{
                                            wsprintf(temp,"%s",regs[RM][reg2]);
                                            strcpy_s(instruction,"cvttsd2si");
                                        }
                                        break;
                                        case 0x2D:{
                                            wsprintf(temp,"%s",regs[RM][reg2]);
                                            strcpy_s(instruction,"cvtsd2si");
                                        }
                                        break;
                                   }
                               }
                               strcpy_s((*Disasm)->Assembly,"");
                               (*Disasm)->OpcodeSize++;
                               wsprintf(tempMeme,"%s %s,%s",instruction,temp,menemonic);
                           }
                      }
					  else{
						  wsprintf(tempMeme,"%s %s,%s",NewSet4[(Op&0x0F)-0x08],temp,menemonic);
						}
                    }
                    break; // MIX

                    case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x47:
                    case 0x48:case 0x49:case 0x4A:case 0x4B:case 0x4C:case 0x4D:case 0x4E:case 0x4F:
                    {
                        wsprintf(tempMeme,"%s %s,%s",NewSet5[Op&0x0F],regs[RM][reg2],menemonic); 
                    }
                    break;

                    case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:
                    case 0x58:case 0x59:case 0x5C:case 0x5D:case 0x5E:case 0x5F:
                    {
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"%s %s,%s",NewSet6Ex[Op&0x0F],MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"%s %s,%s",NewSet6_F2[Op&0x0F],MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
						else if(PrefixReg){
                            wsprintf(tempMeme,"%s %s,%s",NewSet6_66[Op&0x0F],MMXRegs[reg2],menemonic);
						}
						else{
                            wsprintf(tempMeme,"%s %s,%s",NewSet6[Op&0x0F],MMXRegs[reg2],menemonic);
						}
                    }
                    break;

                    case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:
                    case 0x68:case 0x69:case 0x6A:case 0x6B:case 0x6E:case 0x6F:
                    {
                        if(Op==0x6F && RepPrefix==0xF3){
                            wsprintf(tempMeme,"movdqu %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(Op==0x6F && PrefixReg){
                            wsprintf(tempMeme,"movdqa %s,%s",MMXRegs[reg2],menemonic);
                        }
                        else if(PrefixReg){
                            if(Op==0x6E)
                                wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],MMXRegs[reg2],menemonic);
                            else
                                wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],MMXRegs[reg2],menemonic);
                        }
                        else{
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],Regs3DNow[reg2],menemonic);
                        }
                    }
                    break; // MIX

                    case 0x6C:{ // PUNPCKLQDQ (66 only)
                        if(PrefixReg)
                            wsprintf(tempMeme,"punpcklqdq %s,%s",MMXRegs[reg2],menemonic);
                        else
                            wsprintf(tempMeme,"???");
                    }
                    break;

                    case 0x6D:{ // PUNPCKHQDQ (66 only)
                        if(PrefixReg)
                            wsprintf(tempMeme,"punpckhqdq %s,%s",MMXRegs[reg2],menemonic);
                        else
                            wsprintf(tempMeme,"???");
                    }
                    break;

                    case 0x70:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"pshufhw %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"pshuflw %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg){
                            wsprintf(tempMeme,"pshufd %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                        }
                        else{
                            wsprintf(tempMeme,"%s %s,%s,%02Xh",NewSet8[Op&0x0F],Regs3DNow[reg2],menemonic,FOpcode);
                        }
                       wsprintf(menemonic," %02X",FOpcode);
                       lstrcat((*Disasm)->Opcode,menemonic);
                       (*Disasm)->OpcodeSize++;
                       (*(*index))++;
                    }
                    break;

                    case 0x74:case 0x75:case 0x76:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet8[Op&0x0F],MMXRegs[reg2],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet8[Op&0x0F],Regs3DNow[reg2],menemonic);
                    }
                    break;

                    case 0x90:case 0x91:case 0x92:case 0x93:case 0x94:case 0x95:case 0x96:case 0x97:
                    case 0x98:case 0x99:case 0x9A:case 0x9B:case 0x9C:case 0x9D:case 0x9E:case 0x9F:
                    {
                        wsprintf(tempMeme,"%s %s",NewSet9[Op&0x0F],menemonic);
                    }
                    break; // MIX

                    case 0xAE:{
                        wsprintf(tempMeme,"%s %s",NewSet10Ex[REG],menemonic);
                    }
                    break;
                    case 0xAF:wsprintf(tempMeme,"%s %s,%s",NewSet10[Op&0x0F],regs[RM][reg2],menemonic);break;

                    case 0xBC:{ // BSF or TZCNT (F3 0F BC)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"tzcnt %s,%s",regs[RM][reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][reg2],menemonic);
                        }
                    }
                    break;
                    case 0xBD:{ // BSR or LZCNT (F3 0F BD)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"lzcnt %s,%s",regs[RM][reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][reg2],menemonic);
                        }
                    }
                    break;
                    case 0xB2:case 0xB4:case 0xB5:
                    case 0xB6:case 0xB7:
                    case 0xBE:case 0xBF:
                    {
                        wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][reg2],menemonic);
                    }
                    break;

                    case 0xC2:{
                       if(FOpcode<8){ // Instructions here
                           if(RepPrefix==0xF3){
                               wsprintf(tempMeme,"%s %s,%s",NewSet12Ex[FOpcode],MMXRegs[reg2],menemonic);
                               strcpy_s((*Disasm)->Assembly,"");
                               (*Disasm)->OpcodeSize++;
                           }
                           else if(RepPrefix==0xF2){
                               wsprintf(tempMeme,"%s %s,%s",NewSet12_F2[FOpcode],MMXRegs[reg2],menemonic);
                               strcpy_s((*Disasm)->Assembly,"");
                               (*Disasm)->OpcodeSize++;
                           }
						   else if(PrefixReg){
                               wsprintf(tempMeme,"%s %s,%s",NewSet12_66[FOpcode],MMXRegs[reg2],menemonic);
						   }
						   else{
                               wsprintf(tempMeme,"%s %s,%s",NewSet12[FOpcode],MMXRegs[reg2],menemonic);
						   }
                       }
                       else{
                           if(RepPrefix==0xF3){
                               wsprintf(tempMeme,"cmpss %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                               strcpy_s((*Disasm)->Assembly,"");
                               (*Disasm)->OpcodeSize++;
                           }
                           else if(RepPrefix==0xF2){
                               wsprintf(tempMeme,"cmpsd %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                               strcpy_s((*Disasm)->Assembly,"");
                               (*Disasm)->OpcodeSize++;
                           }
						   else if(PrefixReg){
                               wsprintf(tempMeme,"cmppd %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
						   }
						   else{
                               wsprintf(tempMeme,"cmpps %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
						   }
                       }

                       wsprintf(menemonic," %02X",FOpcode);
                       lstrcat((*Disasm)->Opcode,menemonic);
                       (*Disasm)->OpcodeSize++;
                       (*(*index))++;
                    }
                    break;

                    case 0xC4:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"pinsrw %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                        else
                            wsprintf(tempMeme,"pinsrw %s,%s,%02Xh",Regs3DNow[reg2],menemonic,FOpcode);
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0xC6:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"shufpd %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                        else
                            wsprintf(tempMeme,"shufps %s,%s,%02Xh",MMXRegs[reg2],menemonic,FOpcode);
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

					case 0x79: wsprintf(tempMeme,"vmwrite %s,%s",regs[RM][reg2],menemonic); break; // VMWRITE

					case 0xC7:{ // CMPXCHG8B/16B / VMPTRLD / VMCLEAR / VMXON / VMPTRST
						BYTE regField = REG & 0x07;
						if(regField==1){
							if((*Disasm)->Mode64 && (*Disasm)->RexW)
								wsprintf(tempMeme,"cmpxchg16b %s",menemonic);
							else
								wsprintf(tempMeme,"cmpxchg8b %s",menemonic);
						}
						else if(regField==6){
							if(RepPrefix==0xF3){
								wsprintf(tempMeme,"vmxon %s",menemonic);
								strcpy_s((*Disasm)->Assembly,"");
								(*Disasm)->OpcodeSize++;
							}
							else if(PrefixReg){
								wsprintf(tempMeme,"vmclear %s",menemonic);
							}
							else{
								wsprintf(tempMeme,"vmptrld %s",menemonic);
							}
						}
						else if(regField==7){
							wsprintf(tempMeme,"vmptrst %s",menemonic);
						}
						else{
							wsprintf(tempMeme,"??? %s",menemonic);
							lstrcat((*Disasm)->Remarks,";Invalid Instruction");
						}
					}
					break;

					case 0xD6:{
						if(PrefixReg)
							wsprintf(tempMeme,"movq %s,%s",menemonic,MMXRegs[reg2]);
						else
							wsprintf(tempMeme,"??? %s,%s",menemonic,Regs3DNow[reg2]);
					}
					break;

                    case 0xD7:{
                        RM=REG32;
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],regs[RM][reg2],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],regs[RM][reg2],menemonic);
                    }
                    break;

                    case 0xD1:case 0xD2:case 0xD3:case 0xD4:case 0xD5:case 0xD8:case 0xDF:
                    case 0xD9:case 0xDA:case 0xDB:case 0xDC:case 0xDD:case 0xDE:
                    {
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],MMXRegs[reg2],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],Regs3DNow[reg2],menemonic);
                    }
                    break;

                    case 0xE0:case 0xE1:case 0xE2:case 0xE3:
                    case 0xE4:case 0xE5:case 0xE8:case 0xE9:
                    case 0xEA:case 0xEB:case 0xEC:case 0xED:
                    case 0xEE:case 0xEF:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet14[Op&0x0F],MMXRegs[reg2],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet14[Op&0x0F],Regs3DNow[reg2],menemonic);
                    }
                    break;

                    case 0xF1:case 0xF2:case 0xF3:case 0xF5:case 0xF6:
                    case 0xF7:case 0xF8:case 0xF9:case 0xFA:case 0xFB:case 0xFC:
                    case 0xFD:case 0xFE:{
                        if(PrefixReg){
                            if(Op==0xF7)
                                wsprintf(tempMeme,"maskmovdqu %s,%s",MMXRegs[reg2],menemonic);
                            else
                                wsprintf(tempMeme,"%s %s,%s",NewSet15[Op&0x0F],MMXRegs[reg2],menemonic);
                        }
                        else{
                            wsprintf(tempMeme,"%s %s,%s",NewSet15[Op&0x0F],Regs3DNow[reg2],menemonic);
                        }
                    }
                    break;

                    // ==================== SSE3 memory-src forms ====================
                    case 0x12:{ // MOVLPS / MOVLPD (66) / MOVDDUP (F2) / MOVSLDUP (F3)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"movddup %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movsldup %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(tempMeme,"movlpd %s,%s",MMXRegs[reg2],menemonic);
                        else{
                            wsprintf(tempMeme,"movlps %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0x16:{ // MOVHPS / MOVHPD (66) / MOVSHDUP (F3)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movshdup %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(tempMeme,"movhpd %s,%s",MMXRegs[reg2],menemonic);
                        else{
                            wsprintf(tempMeme,"movhps %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0x7C:{ // HADDPD (66) / HADDPS (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"haddps %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"haddpd %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0x7D:{ // HSUBPD (66) / HSUBPS (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"hsubps %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"hsubpd %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0xD0:{ // ADDSUBPD (66) / ADDSUBPS (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"addsubps %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"addsubpd %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0xF0:{ // LDDQU (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"lddqu %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"??? %s",menemonic);
                        }
                    }
                    break;

                    case 0x5A:{ // CVTPS2PD / CVTSS2SD (F3) / CVTSD2SS (F2)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"cvtss2sd %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"cvtsd2ss %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"cvtps2pd %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0x5B:{ // CVTDQ2PS / CVTTPS2DQ (F3)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"cvttps2dq %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"cvtdq2ps %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0xE6:{ // CVTPD2DQ (F2) / CVTDQ2PD (F3) / CVTTPD2DQ (66)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"cvtpd2dq %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"cvtdq2pd %s,%s",MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"cvttpd2dq %s,%s",MMXRegs[reg2],menemonic);
                        }
                    }
                    break;

                    case 0xB8:{ // POPCNT (F3 0F B8)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"popcnt %s,%s",regs[RM][reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else{
                            wsprintf(tempMeme,"??? %s",menemonic);
                        }
                    }
                    break;

                }
                lstrcat((*Disasm)->Assembly,tempMeme);
            }
            break;
        }
        return; // safe exit
    }
    // =================================================//
    //					SIB is being used!				//
    // =================================================//
	else if(SIB==SIB_EX){ // Found SIB, lets strip the extensions
		/* 
		   Example menemonic for SIB: 
		   Opcodes:		000401  
		   Mnemonic:	add byte ptr [eax+ecx], al
		   Binary:		0000 0000 0000 0100 0000 0001
        */
		reg1=((BYTE)(*(*Opcode+pos+2))&0x38)>>3;	// Register A
		reg2=((BYTE)(*(*Opcode+pos+2))&0x07);		// Register B
		SCALE=((BYTE)(*(*Opcode+pos+2))&0xC0)>>6;	// Scale size (0,2,4,8)

		// Scale look up
		switch(SCALE){
			case 0:wsprintf(Aritmathic,"%s",Scale[1]);break; // +
			case 1:wsprintf(Aritmathic,"%s",Scale[2]);break; // *2+
			case 2:wsprintf(Aritmathic,"%s",Scale[3]);break; // *4+
			case 3:wsprintf(Aritmathic,"%s",Scale[4]);break; // *8+
		}

		switch(Extension){ // +/+00/+00000000
			case 00:{ // No extension of bytes
                SwapWord((BYTE*)(*Opcode+pos+1),&wOp,&wMem);
                SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);

                if(reg1==REG_ESP && reg2!=REG_EBP){
                    if(reg2==REG_ESP) SEG=SEG_SS; // IF ESP is being used, User SS Segment Override
                    wsprintf(menemonic,"%02X%04X",Op,wOp);
                    lstrcat((*Disasm)->Opcode,menemonic);
                    wsprintf(menemonic,"%s ptr %s:[%s]",RSize,segs[SEG],regs[ADDRM][reg2]);
                    (*(*index))+=2; //2 byte read				
                    (*Disasm)->OpcodeSize=3; // total used opcodes
                    FOpcode=(BYTE)(*(*Opcode+pos+3));
                }
                else if(reg2!=REG_EBP){ // No EBP in RegMem
					if(reg2==REG_ESP){
						SEG=SEG_SS; // IF ESP is being used, User SS Segment Override
					}
                    wsprintf(menemonic,"%02X%04X",Op,wOp);
                    lstrcat((*Disasm)->Opcode,menemonic);
                    wsprintf(menemonic,"%s ptr %s:[%s%s%s]",RSize,segs[SEG],regs[ADDRM][reg1],Aritmathic,regs[ADDRM][reg2]);
                    (*(*index))+=2; //2 byte read
                    (*Disasm)->OpcodeSize=3; // total used opcodes
                    FOpcode=(BYTE)(*(*Opcode+pos+3));
                }
                else if(reg2==REG_EBP){ // Replace EBP with Dword Number
                    // get 4 bytes extensions for memReg add on
                    // instead of Normal Registers
                    
                    // Format Opcodes (HEX)
                    wsprintf(menemonic,"%02X%04X %08X",Op,wOp,dwOp);
                    lstrcat((*Disasm)->Opcode,menemonic);
                    // Format menemonic
                    
                    // Check If if ESP is being Used.
                    if(reg1==REG_ESP){ // Must Not Be ESP (Index)
                        strcpy_s(temp,"");
                        strcpy_s(Aritmathic,"");
                    }
					else{
                        wsprintf(temp,"%s",regs[ADDRM][reg1]);
					}
                    
                    wsprintf(menemonic,"%s ptr %s:[%s%s%08Xh]",
                        RSize,		// size of regmem
                        segs[SEG],	// segment
                        temp,		// reg
                        Aritmathic,	//+,-,*2,*4,*8
                        dwMem);		// extensions
                    
                    Extension=2; // OverRide Extension (?????), Check toDo.txt
                    (*(*index))+=6; //6 byte read
                    (*Disasm)->OpcodeSize=7; // total used opcodes
                    FOpcode=(BYTE)(*(*Opcode+pos+6));
                }
            }
            break;

			case 01:{ // 1 byte extension
				FOpcode=(BYTE)(*(*Opcode+pos+3));
				if(FOpcode>0x7F){ // check for signed numbers!!
					wsprintf(tempAritmathic,"%s",Scale[0]); // '-' aritmathic
					FOpcode = 0x100-FOpcode; // -XX
				}
				
				if(reg2==REG_EBP || reg1==REG_ESP){ // no ESP in [Mem]
					SEG=SEG_SS;
                    wsprintf(tempMeme,"%s ptr %s:[%s%s%02X]",RSize,segs[SEG],regs[ADDRM][reg2],tempAritmathic,FOpcode);
                }
				else{
				    wsprintf(tempMeme,"%s ptr %s:[%s%s%s%s%02Xh]",RSize,segs[SEG],regs[ADDRM][reg1],Aritmathic,regs[ADDRM][reg2],tempAritmathic,FOpcode);
				}

				(*(*index))+=3; // x + 3 byte(s) read
                SwapDword((BYTE*)(*Opcode+pos),&dwOp,&dwMem);
                wsprintf(menemonic,"%08X",dwOp);
				lstrcat((*Disasm)->Opcode,menemonic);
				(*Disasm)->OpcodeSize=4; // total used opcodes
                strcpy_s(menemonic,StringLen(tempMeme)+1,tempMeme);
                FOpcode=(BYTE)(*(*Opcode+pos+4));
			}
			break;

			case 02:{ // Dword extension
                // Mnemonic decode
                SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
                SwapWord((BYTE*)(*Opcode+pos+3),&wOp,&wMem);
                if(reg1!=REG_ESP){
					if(reg2==REG_EBP){
                        SEG=SEG_SS;
					}
                    
                    wsprintf(tempMeme,"%s ptr %s:[%s%s%s%s%08Xh]",
                        RSize,		// size of register
                        segs[SEG],	// segment
                        regs[ADDRM][reg1],
                        Aritmathic,
                        regs[ADDRM][reg2],
                        tempAritmathic,
                        dwMem);	
                    
                }
				else{// ESP Must not be as Index, Code = 100b
                    wsprintf(tempMeme,"%s ptr %s:[%s%s%08Xh]",
                        RSize,		// size of register
                        segs[SEG],	// segment
                        regs[ADDRM][reg2],
                        tempAritmathic,
                        dwMem);
                    
                }
                
                // Format Opcode
                wsprintf(menemonic,"%02X%04X %08X",Op,wOp,dwOp);
                lstrcat((*Disasm)->Opcode,menemonic);
                (*(*index))+=6;			// x + 3 byte(s) read	
                (*Disasm)->OpcodeSize=7;// total used opcodes
                strcpy_s(menemonic,StringLen(tempMeme)+1,tempMeme);
                FOpcode=(BYTE)(*(*Opcode+pos+7));
			}
			break;
		}
        switch(Bit_d){
            case 0:{ // direction (->)
                switch(Op){ // Check for all Cases Available
                    case 0x00:{
                        wsprintf(tempMeme,"%s %s",NewSet[REG],menemonic);

						if(REG>5){ // Invalid operation
                            lstrcat((*Disasm)->Remarks,";Invalid instruction");
						}
                    }
                    break;

                    case 0x01:{
                        wsprintf(tempMeme,"%s %s",NewSet2[REG],menemonic); // removed a ,%s

						if(REG==5){ // Invalid operation
                            lstrcat((*Disasm)->Remarks,";Invalid instruction");
						}
                    }
                    break;

                    case 0x1F:{
                        wsprintf(tempMeme,"nop %s",menemonic);
                    }
                    break;

                    case 0x11:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movss %s,%s",menemonic,MMXRegs[REG]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"movsd %s,%s",menemonic,MMXRegs[REG]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
						else if(PrefixReg){
                            wsprintf(tempMeme,"movupd %s,%s",menemonic,MMXRegs[REG]);
						}
						else{
                            wsprintf(tempMeme,"movups %s,%s",menemonic,MMXRegs[REG]);
						}
                    }
                    break; // MOVUPS/MOVSS/MOVSD/MOVUPD
                    case 0x13:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"movlpd %s,%s",menemonic,MMXRegs[REG]);
                        else
                            wsprintf(tempMeme,"movlps %s,%s",menemonic,MMXRegs[REG]);
                    }
                    break; // MOVLPS/MOVLPD
                    case 0x17: wsprintf(tempMeme,"movhps %s,%s",menemonic,MMXRegs[REG]); break; // MOVHPS
                    case 0x29:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"movapd %s,%s",menemonic,MMXRegs[REG]);
                        else
                            wsprintf(tempMeme,"movaps %s,%s",menemonic,MMXRegs[REG]);
                    }
                    break; // MOVAPS/MOVAPD
                    case 0x2B:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"movntpd %s,%s",menemonic,MMXRegs[REG]);
                        else
                            wsprintf(tempMeme,"movntps %s,%s",menemonic,MMXRegs[REG]);
                    }
                    break; // MOVNTPS/MOVNTPD
                    case 0x7E:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movq %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],menemonic,MMXRegs[REG]);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],menemonic,Regs3DNow[REG]);
                    }
                    break; // MOVD/MOVQ
                    case 0x7F:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movdqu %s,%s",menemonic,MMXRegs[REG]);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg)
                            wsprintf(tempMeme,"movdqa %s,%s",menemonic,MMXRegs[REG]);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],menemonic,Regs3DNow[REG]);
                    }
                    break; // MOVQ/MOVDQA/MOVDQU
                    case 0xC3: wsprintf(tempMeme,"movnti %s,%s",menemonic,regs[RM][REG]); break; // MOVNTI
					case 0xD6:{
						if(PrefixReg)
							wsprintf(tempMeme,"movq %s,%s",menemonic,MMXRegs[REG]);
						else
							wsprintf(tempMeme,"??? %s,%s",menemonic,Regs3DNow[REG]);
					}
					break;
					case 0xE7:{
						if(PrefixReg)
							wsprintf(tempMeme,"movntdq %s,%s",menemonic,MMXRegs[REG]);
						else
							wsprintf(tempMeme,"%s %s,%s",NewSet14[Op&0x07],menemonic,Regs3DNow[REG]);
					}
					break;
                    case 0xA3: case 0xAB:{
                      wsprintf(tempMeme,"%s %s,%s",NewSet10[Op&0x0F],menemonic,regs[RM][REG]);
                    }
                    break;
                    case 0xA4:case 0xAC:{
                      wsprintf(tempMeme,"%s %s,%s,%02Xh",NewSet10[Op&0x0F],menemonic,regs[RM][REG],FOpcode);
                      wsprintf(menemonic," %02X",FOpcode);
                      lstrcat((*Disasm)->Opcode,menemonic);
                      (*Disasm)->OpcodeSize++;
                      (*(*index))++;
                    }
                    break;
                    case 0xA5: case 0xAD:{
                      wsprintf(tempMeme,"%s %s,%s,cl",NewSet10[Op&0x0F],menemonic,regs[RM][REG]);
                    }
                    break;

                    case 0xB0: case 0xB1: case 0xB3: case 0xBB:{
						if((Op&0x0F)==0x00){
							RM=REG8;
						}

						wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],menemonic,regs[RM][REG]);
                    }
                    break;

                    case 0xBA:{
                        const char *btOps[] = {"???","???","???","???","bt","bts","btr","btc"};
                        BYTE regField = REG & 0x07;
                        if(regField < 4){
                            wsprintf(tempMeme,"??? %s",menemonic);
                            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
                        } else {
                            wsprintf(tempMeme,"%s %s,%02Xh",btOps[regField],menemonic,FOpcode);
                        }
                        wsprintf(temp," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,temp);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0x78: wsprintf(tempMeme,"vmread %s,%s",menemonic,regs[RM][REG]); break; // VMREAD

                    case 0xC0: { RM=REG8; wsprintf(tempMeme,"xadd %s,%s",menemonic,regs[RM][REG]); }		break; // XADD
                    case 0xC1: wsprintf(tempMeme,"xadd %s,%s",menemonic,regs[RM][REG]);						break; // XADD
                }
                lstrcat((*Disasm)->Assembly,tempMeme);
            }
            break;

            case 1:{ // direction (<-)
                switch(Op){ // Decode Instructions
                    case 0x02: wsprintf(tempMeme,"lar %s,%s",regs[RM][REG],menemonic);	break; // LAR
                    case 0x03: wsprintf(tempMeme,"lsl %s,%s",regs[RM][REG],menemonic);	break; // LSL
					case 0x10:{
						if(RepPrefix==0xF3){
							strcpy_s((*Disasm)->Assembly,"");
							(*Disasm)->OpcodeSize++;
							wsprintf(tempMeme,"movss %s,%s",MMXRegs[REG],menemonic);
						}
						else if(RepPrefix==0xF2){
							strcpy_s((*Disasm)->Assembly,"");
							(*Disasm)->OpcodeSize++;
							wsprintf(tempMeme,"movsd %s,%s",MMXRegs[REG],menemonic);
						}
						else if(PrefixReg){
							wsprintf(tempMeme,"movupd %s,%s",MMXRegs[REG],menemonic);
						}
						else{
							wsprintf(tempMeme,"movups %s,%s",MMXRegs[REG],menemonic);
						}
                    }
                    break; // MOVUPS/MOVSS/MOVSD/MOVUPD
                    case 0x14:{
                        if(PrefixReg) wsprintf(tempMeme,"unpcklpd %s,%s",MMXRegs[REG],menemonic);
                        else wsprintf(tempMeme,"unpcklps %s,%s",MMXRegs[REG],menemonic);
                    }
                    break; // UNPCKLPS/UNPCKLPD
                    case 0x15:{
                        if(PrefixReg) wsprintf(tempMeme,"unpckhpd %s,%s",MMXRegs[REG],menemonic);
                        else wsprintf(tempMeme,"unpckhps %s,%s",MMXRegs[REG],menemonic);
                    }
                    break; // UNPCKHPS/UNPCKHPD
                    case 0x18:{
                      wsprintf(tempMeme,"%s,%s",NewSet3[REG],menemonic);
                      if(REG>3)// Invalid Instructions
                          lstrcat((*Disasm)->Remarks,";Invalid instruction");
                    }
                    break;
                    case 0x28:{
                        if(PrefixReg) wsprintf(tempMeme,"movapd %s,%s",MMXRegs[REG],menemonic);
                        else wsprintf(tempMeme,"movaps %s,%s",MMXRegs[REG],menemonic);
                    }
                    break; // MOVAPS/MOVAPD

                    case 0x2A: case 0x2C: case 0x2D:case 0x2E: case 0x2F:{
                       BYTE R=((Op&0x0F)-0x08);
					   if(R==4 || R==5){
                            wsprintf(temp,"%s",Regs3DNow[REG]); // 3DNow! Regs
					   }
					   else{
                            wsprintf(temp,"%s",MMXRegs[REG]); // MMX Regs
					   }

                       if(PrefixReg && (Op==0x2E || Op==0x2F)){
                           wsprintf(tempMeme,"%s %s,%s",(Op==0x2E)?"ucomisd":"comisd",MMXRegs[REG],menemonic);
                       }
                       else if(RepPrefix!=0){
                           char instruction[20];
                           if(Op==0x2A || Op==0x2C || Op==0x2D){
                               if(RepPrefix==0xF3){
                                   switch(Op){
                                       case 0x2A:strcpy_s(instruction,"cvtsi2ss"); break;
                                       case 0x2C:{
                                           wsprintf(temp,"%s",regs[RM][REG]);
                                           strcpy_s(instruction,"cvttss2si");
                                       }
                                       break;
                                       case 0x2D:{
                                           wsprintf(temp,"%s",regs[RM][REG]);
                                           strcpy_s(instruction,"cvtss2si");
                                       }
                                       break;
                                   }
                               }
                               else{ // F2 prefix
                                   switch(Op){
                                       case 0x2A:strcpy_s(instruction,"cvtsi2sd"); break;
                                       case 0x2C:{
                                           wsprintf(temp,"%s",regs[RM][REG]);
                                           strcpy_s(instruction,"cvttsd2si");
                                       }
                                       break;
                                       case 0x2D:{
                                           wsprintf(temp,"%s",regs[RM][REG]);
                                           strcpy_s(instruction,"cvtsd2si");
                                       }
                                       break;
                                   }
                               }
                               strcpy_s((*Disasm)->Assembly,"");
                               (*Disasm)->OpcodeSize++;
                               wsprintf(tempMeme,"%s %s,%s",instruction,temp,menemonic);
                           }
                      }
					 else{
						wsprintf(tempMeme,"%s %s,%s",NewSet4[(Op&0x0F)-0x08],temp,menemonic); 
					 }
                    }
                    break; // MIX
                    case 0x40:case 0x41:case 0x42:case 0x43:case 0x44:case 0x45:case 0x46:case 0x47:
                    case 0x48:case 0x49:case 0x4A:case 0x4B:case 0x4C:case 0x4D:case 0x4E:case 0x4F:
                    {
                      wsprintf(tempMeme,"%s %s,%s",NewSet5[Op&0x0F],regs[RM][REG],menemonic); 
                    }
                    break;

                    case 0x51:case 0x52:case 0x53:case 0x54:case 0x55:case 0x56:case 0x57:
                    case 0x58:case 0x59:case 0x5C:case 0x5D:case 0x5E:case 0x5F:
                    {
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"%s %s,%s",NewSet6Ex[Op&0x0F],MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"%s %s,%s",NewSet6_F2[Op&0x0F],MMXRegs[reg2],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
						else if(PrefixReg){
                            wsprintf(tempMeme,"%s %s,%s",NewSet6_66[Op&0x0F],MMXRegs[REG],menemonic);
						}
						else{
                            wsprintf(tempMeme,"%s %s,%s",NewSet6[Op&0x0F],MMXRegs[REG],menemonic);
						}
                    }
                    break;

                    case 0x60:case 0x61:case 0x62:case 0x63:case 0x64:case 0x65:case 0x66:case 0x67:
                    case 0x68:case 0x69:case 0x6A:case 0x6B:case 0x6E:case 0x6F:
                    {
                        if(Op==0x6F && RepPrefix==0xF3){
                            wsprintf(tempMeme,"movdqu %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(Op==0x6F && PrefixReg){
                            wsprintf(tempMeme,"movdqa %s,%s",MMXRegs[REG],menemonic);
                        }
                        else if(PrefixReg){
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],MMXRegs[REG],menemonic);
                        }
                        else{
                            wsprintf(tempMeme,"%s %s,%s",NewSet7[Op&0x0F],Regs3DNow[REG],menemonic);
                        }
                    }
                    break;

                    case 0x6C:{ // PUNPCKLQDQ (66 only)
                        if(PrefixReg)
                            wsprintf(tempMeme,"punpcklqdq %s,%s",MMXRegs[REG],menemonic);
                        else
                            wsprintf(tempMeme,"???");
                    }
                    break;

                    case 0x6D:{ // PUNPCKHQDQ (66 only)
                        if(PrefixReg)
                            wsprintf(tempMeme,"punpckhqdq %s,%s",MMXRegs[REG],menemonic);
                        else
                            wsprintf(tempMeme,"???");
                    }
                    break;

                    case 0x70:{
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"pshufhw %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"pshuflw %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        }
                        else if(PrefixReg){
                            wsprintf(tempMeme,"pshufd %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                        }
                        else{
                            wsprintf(tempMeme,"%s %s,%s,%02Xh",NewSet8[Op&0x0F],Regs3DNow[REG],menemonic,FOpcode);
                        }
                      wsprintf(menemonic," %02X",FOpcode);
                      lstrcat((*Disasm)->Opcode,menemonic);
                      (*Disasm)->OpcodeSize++;
                      (*(*index))++;
                    }
                    break;

                    case 0x74:case 0x75:case 0x76:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet8[Op&0x0F],MMXRegs[REG],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet8[Op&0x0F],Regs3DNow[REG],menemonic);
                    }
                    break;

                    case 0x90:case 0x91:case 0x92:case 0x93:case 0x94:case 0x95:case 0x96:case 0x97:
                    case 0x98:case 0x99:case 0x9A:case 0x9B:case 0x9C:case 0x9D:case 0x9E:case 0x9F:
                    {
                      wsprintf(tempMeme,"%s %s",NewSet9[Op&0x0F],menemonic);
                    }
                    break; // MIX

                    case 0xAE:{
                        wsprintf(tempMeme,"%s %s",NewSet10Ex[REG],menemonic);
                    }
                    break;
                    case 0xAF:wsprintf(tempMeme,"%s %s,%s",NewSet10[Op&0x0F],regs[RM][REG],menemonic);break;

                    case 0xBC:{ // BSF or TZCNT (F3 0F BC)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"tzcnt %s,%s",regs[RM][REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][REG],menemonic);
                        }
                    }
                    break;
                    case 0xBD:{ // BSR or LZCNT (F3 0F BD)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"lzcnt %s,%s",regs[RM][REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,"");
                            (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][REG],menemonic);
                        }
                    }
                    break;
                    case 0xB2:case 0xB4:case 0xB5:
                    case 0xB6:case 0xB7:
                    case 0xBE:case 0xBF:
                    {
                     wsprintf(tempMeme,"%s %s,%s",NewSet11[Op&0x0F],regs[RM][REG],menemonic);
                    }
                    break;

                    case 0xC2:{
                      if(FOpcode<8){
                          if(RepPrefix==0xF3){
                              wsprintf(tempMeme,"%s %s,%s",NewSet12Ex[FOpcode],MMXRegs[REG],menemonic);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
                          else if(RepPrefix==0xF2){
                              wsprintf(tempMeme,"%s %s,%s",NewSet12_F2[FOpcode],MMXRegs[REG],menemonic);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
						  else if(PrefixReg){
                              wsprintf(tempMeme,"%s %s,%s",NewSet12_66[FOpcode],MMXRegs[REG],menemonic);
						  }
						  else{
                              wsprintf(tempMeme,"%s %s,%s",NewSet12[FOpcode],MMXRegs[REG],menemonic);
						  }
                      }
                      else{
                          if(RepPrefix==0xF3){
                              wsprintf(tempMeme,"cmpss %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
                          else if(RepPrefix==0xF2){
                              wsprintf(tempMeme,"cmpsd %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                              strcpy_s((*Disasm)->Assembly,"");
                              (*Disasm)->OpcodeSize++;
                          }
						  else if(PrefixReg){
                              wsprintf(tempMeme,"cmppd %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
						  }
						  else{
                              wsprintf(tempMeme,"cmpps %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
						  }
                      }
                      wsprintf(menemonic," %02X",FOpcode);
                      lstrcat((*Disasm)->Opcode,menemonic);
                      (*Disasm)->OpcodeSize++;
                      (*(*index))++;
                    }
                    break;

                    case 0xC4:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"pinsrw %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                        else
                            wsprintf(tempMeme,"pinsrw %s,%s,%02Xh",Regs3DNow[REG],menemonic,FOpcode);
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0x79: wsprintf(tempMeme,"vmwrite %s,%s",regs[RM][REG],menemonic); break; // VMWRITE

                    case 0xC6:{
                        if(PrefixReg)
                            wsprintf(tempMeme,"shufpd %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                        else
                            wsprintf(tempMeme,"shufps %s,%s,%02Xh",MMXRegs[REG],menemonic,FOpcode);
                        wsprintf(menemonic," %02X",FOpcode);
                        lstrcat((*Disasm)->Opcode,menemonic);
                        (*Disasm)->OpcodeSize++;
                        (*(*index))++;
                    }
                    break;

                    case 0xC7:{ // CMPXCHG8B/16B / VMPTRLD / VMCLEAR / VMXON / VMPTRST
                        BYTE regField = REG & 0x07;
                        if(regField==1){
                            if((*Disasm)->Mode64 && (*Disasm)->RexW)
                                wsprintf(tempMeme,"cmpxchg16b %s",menemonic);
                            else
                                wsprintf(tempMeme,"cmpxchg8b %s",menemonic);
                        }
                        else if(regField==6){
                            if(RepPrefix==0xF3){
                                wsprintf(tempMeme,"vmxon %s",menemonic);
                                strcpy_s((*Disasm)->Assembly,"");
                                (*Disasm)->OpcodeSize++;
                            }
                            else if(PrefixReg){
                                wsprintf(tempMeme,"vmclear %s",menemonic);
                            }
                            else{
                                wsprintf(tempMeme,"vmptrld %s",menemonic);
                            }
                        }
                        else if(regField==7){
                            wsprintf(tempMeme,"vmptrst %s",menemonic);
                        }
                        else{
                            wsprintf(tempMeme,"??? %s",menemonic);
                            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
                        }
                    }
                    break;

					case 0xD6:{
						if(PrefixReg)
							wsprintf(tempMeme,"movq %s,%s",menemonic,MMXRegs[REG]);
						else
							wsprintf(tempMeme,"??? %s,%s",menemonic,Regs3DNow[REG]);
					}
					break;

                    case 0xD7:{
                        RM=REG32;
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],regs[RM][REG],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],regs[RM][REG],menemonic);
                    }
                    break;

                    case 0xD1:case 0xD2:case 0xD3:case 0xD4:case 0xD5:case 0xD8:case 0xDF:
                    case 0xD9:case 0xDA:case 0xDB:case 0xDC:case 0xDD:case 0xDE:
                    {
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],MMXRegs[REG],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet13[Op&0x0F],Regs3DNow[REG],menemonic);
                    }
                    break;

                    case 0xE0:case 0xE1:case 0xE2:case 0xE3:
                    case 0xE4:case 0xE5:case 0xE8:case 0xE9:
                    case 0xEA:case 0xEB:case 0xEC:case 0xED:
                    case 0xEE:case 0xEF:
                    {
                        if(PrefixReg)
                            wsprintf(tempMeme,"%s %s,%s",NewSet14[Op&0x0F],MMXRegs[REG],menemonic);
                        else
                            wsprintf(tempMeme,"%s %s,%s",NewSet14[Op&0x0F],Regs3DNow[REG],menemonic);
                    }
                    break;

                    case 0xF1:case 0xF2:case 0xF3:case 0xF5:case 0xF6:
                    case 0xF7:case 0xF8:case 0xF9:case 0xFA:case 0xFB:case 0xFC:
                    case 0xFD:case 0xFE:{
                        if(PrefixReg){
                            if(Op==0xF7)
                                wsprintf(tempMeme,"maskmovdqu %s,%s",MMXRegs[REG],menemonic);
                            else
                                wsprintf(tempMeme,"%s %s,%s",NewSet15[Op&0x0F],MMXRegs[REG],menemonic);
                        }
                        else{
                            wsprintf(tempMeme,"%s %s,%s",NewSet15[Op&0x0F],Regs3DNow[REG],menemonic);
                        }
                    }
					break;

                    // ==================== SSE3 memory-src forms (SIB) ====================
                    case 0x12:{ // MOVLPS / MOVLPD (66) / MOVDDUP (F2) / MOVSLDUP (F3)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"movddup %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movsldup %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else if(PrefixReg){
                            wsprintf(tempMeme,"movlpd %s,%s",MMXRegs[REG],menemonic);
                        } else {
                            wsprintf(tempMeme,"movlps %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0x16:{ // MOVHPS / MOVHPD (66) / MOVSHDUP (F3)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"movshdup %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else if(PrefixReg){
                            wsprintf(tempMeme,"movhpd %s,%s",MMXRegs[REG],menemonic);
                        } else {
                            wsprintf(tempMeme,"movhps %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0x7C:{ // HADDPD (66) / HADDPS (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"haddps %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"haddpd %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0x7D:{ // HSUBPD (66) / HSUBPS (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"hsubps %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"hsubpd %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0xD0:{ // ADDSUBPD (66) / ADDSUBPS (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"addsubps %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"addsubpd %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0xF0:{ // LDDQU (F2)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"lddqu %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else { wsprintf(tempMeme,"??? %s",menemonic); }
                    }
                    break;
                    case 0x5A:{ // CVTPS2PD / CVTSS2SD (F3) / CVTSD2SS (F2)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"cvtss2sd %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"cvtsd2ss %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"cvtps2pd %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0x5B:{ // CVTDQ2PS / CVTTPS2DQ (F3)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"cvttps2dq %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"cvtdq2ps %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0xE6:{ // CVTPD2DQ (F2) / CVTDQ2PD (F3) / CVTTPD2DQ (66)
                        if(RepPrefix==0xF2){
                            wsprintf(tempMeme,"cvtpd2dq %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"cvtdq2pd %s,%s",MMXRegs[REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else {
                            wsprintf(tempMeme,"cvttpd2dq %s,%s",MMXRegs[REG],menemonic);
                        }
                    }
                    break;
                    case 0xB8:{ // POPCNT (F3)
                        if(RepPrefix==0xF3){
                            wsprintf(tempMeme,"popcnt %s,%s",regs[RM][REG],menemonic);
                            strcpy_s((*Disasm)->Assembly,""); (*Disasm)->OpcodeSize++;
                        } else { wsprintf(tempMeme,"??? %s",menemonic); }
                    }
                    break;

                }
                lstrcat((*Disasm)->Assembly,tempMeme);
            }
            break;
        }
    }
// end
}

//==================================================================================//
//          Decode 3-byte opcodes: 0F 38 xx (SSSE3 / SSE4.1 / SSE4.2)              //
//==================================================================================//

void Decode3ByteOpcode_0F38(
    DISASSEMBLY **Disasm,
    char **Opcode, DWORD_PTR pos,
    bool AddrPrefix, int SEG, DWORD_PTR **index,
    bool PrefixReg, bool PrefixSeg, bool PrefixAddr,
    BYTE RepPrefix)
{
    BYTE ThirdByte = (BYTE)(*(*Opcode+pos+1)); // The 3rd opcode byte (after 0F 38)
    BYTE ModRM = (BYTE)(*(*Opcode+pos+2));     // ModR/M byte
    BYTE MOD = (ModRM >> 6) & 0x03;
    BYTE REG = (ModRM >> 3) & 0x07;
    BYTE RM  = ModRM & 0x07;

    char assembly[256]="", temp[128]="", menemonic[128]="";
    WORD wOp, wMem;
    DWORD_PTR m_OpcodeSize = 2; // 38 + ModRM (ThirdByte added below; caller adds 0F)

    // Check if 66 prefix is present (mandatory prefix for most SSE4 instructions)
    bool Has66 = PrefixReg;

    const char *mnem = NULL;

    // SSSE3 instructions (0F 38 00-0B, 1C-1E) - work with both MMX (no prefix) and XMM (66 prefix)
    switch(ThirdByte){
        // SSSE3
        case 0x00: mnem = "pshufb";    break;
        case 0x01: mnem = "phaddw";    break;
        case 0x02: mnem = "phaddd";    break;
        case 0x03: mnem = "phaddsw";   break;
        case 0x04: mnem = "pmaddubsw"; break;
        case 0x05: mnem = "phsubw";    break;
        case 0x06: mnem = "phsubd";    break;
        case 0x07: mnem = "phsubsw";   break;
        case 0x08: mnem = "psignb";    break;
        case 0x09: mnem = "psignw";    break;
        case 0x0A: mnem = "psignd";    break;
        case 0x0B: mnem = "pmulhrsw";  break;
        case 0x1C: mnem = "pabsb";     break;
        case 0x1D: mnem = "pabsw";     break;
        case 0x1E: mnem = "pabsd";     break;

        // SSE4.1 (require 66 prefix)
        case 0x10: mnem = "pblendvb";  break;
        case 0x14: mnem = "blendvps";  break;
        case 0x15: mnem = "blendvpd";  break;
        case 0x17: mnem = "ptest";     break;
        case 0x20: mnem = "pmovsxbw";  break;
        case 0x21: mnem = "pmovsxbd";  break;
        case 0x22: mnem = "pmovsxbq";  break;
        case 0x23: mnem = "pmovsxwd";  break;
        case 0x24: mnem = "pmovsxwq";  break;
        case 0x25: mnem = "pmovsxdq";  break;
        case 0x28: mnem = "pmuldq";    break;
        case 0x29: mnem = "pcmpeqq";   break;
        case 0x2A: mnem = "movntdqa";  break;
        case 0x2B: mnem = "packusdw";  break;
        case 0x30: mnem = "pmovzxbw";  break;
        case 0x31: mnem = "pmovzxbd";  break;
        case 0x32: mnem = "pmovzxbq";  break;
        case 0x33: mnem = "pmovzxwd";  break;
        case 0x34: mnem = "pmovzxwq";  break;
        case 0x35: mnem = "pmovzxdq";  break;
        case 0x38: mnem = "pminsb";    break;
        case 0x39: mnem = "pminsd";    break;
        case 0x3A: mnem = "pminuw";    break;
        case 0x3B: mnem = "pminud";    break;
        case 0x3C: mnem = "pmaxsb";    break;
        case 0x3D: mnem = "pmaxsd";    break;
        case 0x3E: mnem = "pmaxuw";    break;
        case 0x3F: mnem = "pmaxud";    break;
        case 0x40: mnem = "pmulld";    break;
        case 0x41: mnem = "phminposuw"; break;

        // SSE4.2
        case 0x37: mnem = "pcmpgtq";   break;

        // SHA extensions
        case 0xC8: mnem = "sha1nexte";   break;
        case 0xC9: mnem = "sha1msg1";    break;
        case 0xCA: mnem = "sha1msg2";    break;
        case 0xCB: mnem = "sha256rnds2"; break;
        case 0xCC: mnem = "sha256msg1";  break;
        case 0xCD: mnem = "sha256msg2";  break;

        // GFNI (66 0F 38 CF)
        case 0xCF: if(Has66) mnem = "gf2p8mulb"; break;

        // AES-NI (require 66 prefix)
        case 0xDB: mnem = "aesimc";      break;
        case 0xDC: mnem = "aesenc";      break;
        case 0xDD: mnem = "aesenclast";  break;
        case 0xDE: mnem = "aesdec";      break;
        case 0xDF: mnem = "aesdeclast";  break;

        // MOVBE / CRC32 (F2 0F 38 F0/F1)
        case 0xF0: case 0xF1:{
            if(RepPrefix==0xF2){
                mnem = "crc32";
            } else {
                mnem = "movbe";
            }
            break;
        }

        // ADCX (66 0F 38 F6) / ADOX (F3 0F 38 F6)
        case 0xF6:{
            if(Has66) mnem = "adcx";
            else if(RepPrefix==0xF3) mnem = "adox";
            break;
        }

        // VMX: INVEPT (66 0F 38 80) / INVVPID (66 0F 38 81)
        case 0x80:{
            if(Has66) mnem = "invept";
            break;
        }
        case 0x81:{
            if(Has66) mnem = "invvpid";
            break;
        }

        default: mnem = NULL; break;
    }

    if(mnem == NULL){
        // Unknown 3-byte opcode
        wsprintf(assembly,"???");
        wsprintf(temp,"%02X %02X",ThirdByte,ModRM);
        lstrcat((*Disasm)->Opcode,temp);
        lstrcat((*Disasm)->Assembly,assembly);
        (*Disasm)->OpcodeSize = m_OpcodeSize;
        (*(*index))+=2;
        return;
    }

    // Format opcode bytes
    wsprintf(temp,"%02X",ThirdByte);
    lstrcat((*Disasm)->Opcode,temp);
    m_OpcodeSize++; // for the 3rd byte

    // Determine register set
    // With 66 prefix: XMM registers (SSSE3 XMM form / SSE4.1+)
    // Without 66: MM registers (SSSE3 MMX form)
    const char **regSet;
    if(Has66){
        regSet = MMXRegs; // XMM registers
    } else {
        regSet = Regs3DNow; // MM registers (SSSE3 MMX form)
    }

    // SHA extensions always use XMM registers regardless of 66 prefix
    if(ThirdByte >= 0xC8 && ThirdByte <= 0xCD){
        regSet = MMXRegs;
    }

    // Check if this instruction uses GPR operands
    bool usesGPR38 = false;
    if((ThirdByte == 0xF0 || ThirdByte == 0xF1) && RepPrefix!=0xF2) usesGPR38 = true; // MOVBE
    if(ThirdByte == 0xF6) usesGPR38 = true; // ADCX/ADOX
    if(ThirdByte == 0x80 || ThirdByte == 0x81) usesGPR38 = true; // INVEPT/INVVPID

    if(MOD == 0x03){ // Register-register form
        // CRC32 special: GPR operands
        if(ThirdByte == 0xF0 && RepPrefix==0xF2){
            wsprintf(assembly,"%s %s, %s",mnem,regs[REG32][REG],regs[REG8][RM]);
            strcpy_s((*Disasm)->Assembly,"");
            m_OpcodeSize++;
        }
        else if(ThirdByte == 0xF1 && RepPrefix==0xF2){
            wsprintf(assembly,"%s %s, %s",mnem,regs[REG32][REG],regs[Has66?REG16:REG32][RM]);
            strcpy_s((*Disasm)->Assembly,"");
            m_OpcodeSize++;
        }
        else if(ThirdByte == 0xF0 && RepPrefix!=0xF2){ // MOVBE r, r/m (load)
            wsprintf(assembly,"%s %s, %s",mnem,regs[REG32][REG],regs[REG32][RM]);
        }
        else if(ThirdByte == 0xF1 && RepPrefix!=0xF2){ // MOVBE r/m, r (store)
            wsprintf(assembly,"%s %s, %s",mnem,regs[REG32][RM],regs[REG32][REG]);
        }
        else if(ThirdByte == 0xF6){ // ADCX/ADOX r, r/m
            wsprintf(assembly,"%s %s, %s",mnem,regs[REG32][REG],regs[REG32][RM]);
            if(Has66) strcpy_s((*Disasm)->Assembly,"");
            else if(RepPrefix==0xF3){ strcpy_s((*Disasm)->Assembly,""); m_OpcodeSize++; }
        }
        else if(ThirdByte == 0x80 || ThirdByte == 0x81){ // INVEPT/INVVPID - memory only, reg form invalid
            wsprintf(assembly,"???");
            lstrcat((*Disasm)->Remarks,";Invalid Instruction");
        }
        else{
            // Implicit xmm0 for blendv / sha256rnds2 instructions
            if(ThirdByte == 0x10 || ThirdByte == 0x14 || ThirdByte == 0x15 || ThirdByte == 0xCB){
                wsprintf(assembly,"%s %s, %s, xmm0",mnem,regSet[REG],regSet[RM]);
            } else {
                wsprintf(assembly,"%s %s, %s",mnem,regSet[REG],regSet[RM]);
            }
        }

        wsprintf(temp,"%02X",ModRM);
        lstrcat((*Disasm)->Opcode,temp);
        (*(*index))+=2;
    }
    else{ // Memory form - decode ModRM memory operand
        DWORD_PTR dwOp=0, dwMem=0;
        char memStr[128]="";
        char opcExtra[64]="";
        int segOvr = SEG;

        // Segment override for EBP base with MOD=01/02
        if(RM==REG_EBP && MOD!=0 && !PrefixSeg)
            segOvr = SEG_SS;

        // Decode memory operand into memStr, format extra opcode bytes into opcExtra
        if(MOD == 0x00){
            if(RM == 0x05){ // [disp32]
                SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
                wsprintf(memStr,"%s:[%08Xh]",segs[segOvr],dwMem);
                wsprintf(opcExtra,"%08X",dwOp);
            }
            else if(RM == 0x04){ // SIB
                BYTE sib = (BYTE)(*(*Opcode+pos+3));
                BYTE sibIdx   = (sib>>3)&0x07;
                BYTE sibBase  = sib&0x07;
                BYTE sibScale = (sib>>6)&0x03;
                const char *scStr = Scale[sibScale+1]; // "+","*2+","*4+","*8+"

                if(sibBase == REG_EBP){ // base=5 + MOD=00 -> disp32
                    SwapDword((BYTE*)(*Opcode+pos+4),&dwOp,&dwMem);
                    wsprintf(opcExtra,"%02X%08X",sib,dwOp);
                    if(sibIdx == REG_ESP) // no index
                        wsprintf(memStr,"%s:[%08Xh]",segs[segOvr],dwMem);
                    else
                        wsprintf(memStr,"%s:[%s%s%08Xh]",segs[segOvr],regs[REG32][sibIdx],scStr,dwMem);
                }
                else{
                    if(sibBase==REG_ESP && !PrefixSeg) segOvr=SEG_SS;
                    wsprintf(opcExtra,"%02X",sib);
                    if(sibIdx == REG_ESP) // no index
                        wsprintf(memStr,"%s:[%s]",segs[segOvr],regs[REG32][sibBase]);
                    else
                        wsprintf(memStr,"%s:[%s%s%s]",segs[segOvr],regs[REG32][sibIdx],scStr,regs[REG32][sibBase]);
                }
            }
            else{ // [reg]
                wsprintf(memStr,"%s:[%s]",segs[segOvr],regs[REG32][RM]);
            }
        }
        else if(MOD == 0x01){ // [reg+disp8]
            if(RM == 0x04){ // SIB + disp8
                BYTE sib = (BYTE)(*(*Opcode+pos+3));
                BYTE sibIdx   = (sib>>3)&0x07;
                BYTE sibBase  = sib&0x07;
                BYTE sibScale = (sib>>6)&0x03;
                BYTE disp8    = (BYTE)(*(*Opcode+pos+4));
                const char *scStr = Scale[sibScale+1];
                char dsign[4]="+"; BYTE dval=disp8;
                if(disp8>0x7F){ strcpy_s(dsign,"-"); dval=(BYTE)(0x100-disp8); }
                if((sibBase==REG_EBP||sibBase==REG_ESP) && !PrefixSeg) segOvr=SEG_SS;
                wsprintf(opcExtra,"%02X%02X",sib,disp8);
                if(sibIdx == REG_ESP)
                    wsprintf(memStr,"%s:[%s%s%02Xh]",segs[segOvr],regs[REG32][sibBase],dsign,dval);
                else
                    wsprintf(memStr,"%s:[%s%s%s%s%02Xh]",segs[segOvr],regs[REG32][sibIdx],scStr,regs[REG32][sibBase],dsign,dval);
            }
            else{ // [reg+disp8]
                BYTE disp8 = (BYTE)(*(*Opcode+pos+3));
                char dsign[4]="+"; BYTE dval=disp8;
                if(disp8>0x7F){ strcpy_s(dsign,"-"); dval=(BYTE)(0x100-disp8); }
                wsprintf(memStr,"%s:[%s%s%02Xh]",segs[segOvr],regs[REG32][RM],dsign,dval);
                wsprintf(opcExtra,"%02X",disp8);
            }
        }
        else if(MOD == 0x02){ // [reg+disp32]
            if(RM == 0x04){ // SIB + disp32
                BYTE sib = (BYTE)(*(*Opcode+pos+3));
                BYTE sibIdx   = (sib>>3)&0x07;
                BYTE sibBase  = sib&0x07;
                BYTE sibScale = (sib>>6)&0x03;
                const char *scStr = Scale[sibScale+1];
                SwapDword((BYTE*)(*Opcode+pos+4),&dwOp,&dwMem);
                if((sibBase==REG_EBP||sibBase==REG_ESP) && !PrefixSeg) segOvr=SEG_SS;
                char tmp2[16]; wsprintf(tmp2,"%08X",dwOp);
                wsprintf(opcExtra,"%02X",sib);
                lstrcat(opcExtra,tmp2);
                if(sibIdx == REG_ESP)
                    wsprintf(memStr,"%s:[%s+%08Xh]",segs[segOvr],regs[REG32][sibBase],dwMem);
                else
                    wsprintf(memStr,"%s:[%s%s%s+%08Xh]",segs[segOvr],regs[REG32][sibIdx],scStr,regs[REG32][sibBase],dwMem);
            }
            else{ // [reg+disp32]
                SwapDword((BYTE*)(*Opcode+pos+3),&dwOp,&dwMem);
                wsprintf(memStr,"%s:[%s+%08Xh]",segs[segOvr],regs[REG32][RM],dwMem);
                wsprintf(opcExtra,"%08X",dwOp);
            }
        }

        // Build instruction assembly using decoded memStr
        if(ThirdByte == 0xF0 && RepPrefix==0xF2){
            wsprintf(assembly,"%s %s, byte ptr %s",mnem,regs[REG32][REG],memStr);
            strcpy_s((*Disasm)->Assembly,"");
            m_OpcodeSize++;
        }
        else if(ThirdByte == 0xF1 && RepPrefix==0xF2){
            wsprintf(assembly,"%s %s, %s ptr %s",mnem,regs[REG32][REG],Has66?"word":"dword",memStr);
            strcpy_s((*Disasm)->Assembly,"");
            m_OpcodeSize++;
        }
        else if(ThirdByte == 0x80 || ThirdByte == 0x81){ // INVEPT/INVVPID: r32, m128
            wsprintf(assembly,"%s %s, %s",mnem,regs[REG32][REG],memStr);
            strcpy_s((*Disasm)->Assembly,""); // clear 66 prefix
        }
        else if(usesGPR38){ // MOVBE, ADCX, ADOX - GPR operands
            if(ThirdByte == 0xF1) // MOVBE store: m, r
                wsprintf(assembly,"%s dword ptr %s, %s",mnem,memStr,regs[REG32][REG]);
            else // MOVBE load / ADCX / ADOX: r, m
                wsprintf(assembly,"%s %s, dword ptr %s",mnem,regs[REG32][REG],memStr);
            if(Has66 || RepPrefix==0xF3) strcpy_s((*Disasm)->Assembly,"");
            if(RepPrefix==0xF3) m_OpcodeSize++;
        }
        else if(ThirdByte == 0x10 || ThirdByte == 0x14 || ThirdByte == 0x15 || ThirdByte == 0xCB){
            wsprintf(assembly,"%s %s, %s, xmm0",mnem,regSet[REG],memStr);
        }
        else{
            wsprintf(assembly,"%s %s, %s",mnem,regSet[REG],memStr);
        }

        // Append ModRM byte + SIB/displacement opcode bytes
        wsprintf(temp,"%02X",ModRM);
        lstrcat((*Disasm)->Opcode,temp);
        if(opcExtra[0]) lstrcat((*Disasm)->Opcode,opcExtra);

        // Advance index and opcode size for displacement bytes
        if(MOD == 0x00){
            if(RM == 0x05){ // disp32
                (*(*index))+=4;
                m_OpcodeSize+=4;
            }
            else if(RM == 0x04){ // SIB
                (*(*index))++;
                m_OpcodeSize++;
                BYTE sib = (BYTE)(*(*Opcode+pos+3));
                if((sib & 0x07) == 0x05){
                    (*(*index))+=4;
                    m_OpcodeSize+=4;
                }
            }
        }
        else if(MOD == 0x01){ // disp8
            (*(*index))++;
            m_OpcodeSize++;
            if(RM == 0x04){ (*(*index))++; m_OpcodeSize++; } // SIB
        }
        else if(MOD == 0x02){ // disp32
            (*(*index))+=4;
            m_OpcodeSize+=4;
            if(RM == 0x04){ (*(*index))++; m_OpcodeSize++; } // SIB
        }
        (*(*index))+=2;
    }

    if(Has66 && RepPrefix==0){
        strcpy_s((*Disasm)->Assembly,""); // clear 66 prefix mnemonic
    }
    if(RepPrefix==0xF2){
        strcpy_s((*Disasm)->Assembly,""); // clear rep prefix mnemonic
    }
    if(RepPrefix==0xF3){
        strcpy_s((*Disasm)->Assembly,""); // clear rep prefix mnemonic
    }

    lstrcat((*Disasm)->Assembly,assembly);
    (*Disasm)->OpcodeSize = m_OpcodeSize;
}


//==================================================================================//
//          Decode 3-byte opcodes: 0F 3A xx (SSSE3 / SSE4.1)                        //
//==================================================================================//

void Decode3ByteOpcode_0F3A(
    DISASSEMBLY **Disasm,
    char **Opcode, DWORD_PTR pos,
    bool AddrPrefix, int SEG, DWORD_PTR **index,
    bool PrefixReg, bool PrefixSeg, bool PrefixAddr,
    BYTE RepPrefix)
{
    BYTE ThirdByte = (BYTE)(*(*Opcode+pos+1)); // The 3rd opcode byte (after 0F 3A)
    BYTE ModRM = (BYTE)(*(*Opcode+pos+2));     // ModR/M byte
    BYTE MOD = (ModRM >> 6) & 0x03;
    BYTE REG = (ModRM >> 3) & 0x07;
    BYTE RM  = ModRM & 0x07;
    BYTE Imm8 = 0;

    char assembly[256]="", temp[128]="";
    WORD wOp, wMem;
    DWORD_PTR m_OpcodeSize = 3; // 3A + ModRM + imm8 (ThirdByte added below; caller adds 0F)

    bool Has66 = PrefixReg;

    const char *mnem = NULL;
    bool usesGPR = false; // Some instructions use GPR as dest/src

    switch(ThirdByte){
        // SSSE3
        case 0x0F: mnem = "palignr";   break;

        // SSE4.1
        case 0x08: mnem = "roundps";   break;
        case 0x09: mnem = "roundpd";   break;
        case 0x0A: mnem = "roundss";   break;
        case 0x0B: mnem = "roundsd";   break;
        case 0x0C: mnem = "blendps";   break;
        case 0x0D: mnem = "blendpd";   break;
        case 0x0E: mnem = "pblendw";   break;
        case 0x14: mnem = "pextrb";  usesGPR=true; break;
        case 0x15: mnem = "pextrw";  usesGPR=true; break;
        case 0x16: mnem = "pextrd";  usesGPR=true; break;
        case 0x17: mnem = "extractps"; usesGPR=true; break;
        case 0x20: mnem = "pinsrb";  usesGPR=true; break;
        case 0x21: mnem = "insertps";  break;
        case 0x22: mnem = "pinsrd";  usesGPR=true; break;
        case 0x40: mnem = "dpps";      break;
        case 0x41: mnem = "dppd";      break;
        case 0x42: mnem = "mpsadbw";   break;

        // PCLMULQDQ (66 0F 3A 44)
        case 0x44: mnem = "pclmulqdq"; break;

        // SSE4.2
        case 0x60: mnem = "pcmpestrm"; break;
        case 0x61: mnem = "pcmpestri"; break;
        case 0x62: mnem = "pcmpistrm"; break;
        case 0x63: mnem = "pcmpistri"; break;

        // AESKEYGENASSIST (66 0F 3A DF)
        case 0xDF: mnem = "aeskeygenassist"; break;

        // GFNI (66 0F 3A CE/CF)
        case 0xCE: if(Has66) mnem = "gf2p8affineqb"; break;
        case 0xCF: if(Has66) mnem = "gf2p8affineinvqb"; break;

        default: mnem = NULL; break;
    }

    if(mnem == NULL){
        wsprintf(assembly,"???");
        wsprintf(temp,"%02X %02X",ThirdByte,ModRM);
        lstrcat((*Disasm)->Opcode,temp);
        lstrcat((*Disasm)->Assembly,assembly);
        (*Disasm)->OpcodeSize = m_OpcodeSize;
        (*(*index))+=3; // ThirdByte + ModRM + Imm8
        return;
    }

    // Format opcode bytes
    wsprintf(temp,"%02X",ThirdByte);
    lstrcat((*Disasm)->Opcode,temp);
    m_OpcodeSize++; // for the 3rd byte

    const char **regSet;
    if(Has66 || !Has66){ // Most 0F3A instructions are XMM-only with 66 prefix
        regSet = MMXRegs;
    }
    // SSSE3 palignr without 66 uses MM regs
    if(ThirdByte == 0x0F && !Has66){
        regSet = Regs3DNow;
    }

    if(MOD == 0x03){ // Register-register form
        if(ThirdByte == 0x0F && !Has66){
            // PALIGNR mm, mm, imm8 (SSSE3 MMX form)
            Imm8 = (BYTE)(*(*Opcode+pos+3));
            wsprintf(assembly,"%s %s, %s, %02Xh",mnem,Regs3DNow[REG],Regs3DNow[RM],Imm8);
        }
        else if(usesGPR){
            Imm8 = (BYTE)(*(*Opcode+pos+3));
            // pextrb/w/d: GPR dest, XMM src
            // pinsrb/d: XMM dest, GPR src
            if(ThirdByte >= 0x14 && ThirdByte <= 0x17){
                wsprintf(assembly,"%s %s, %s, %02Xh",mnem,regs[REG32][RM],MMXRegs[REG],Imm8);
            } else {
                wsprintf(assembly,"%s %s, %s, %02Xh",mnem,MMXRegs[REG],regs[REG32][RM],Imm8);
            }
        }
        else{
            Imm8 = (BYTE)(*(*Opcode+pos+3));
            wsprintf(assembly,"%s %s, %s, %02Xh",mnem,MMXRegs[REG],MMXRegs[RM],Imm8);
        }

        wsprintf(temp,"%02X %02X",ModRM,Imm8);
        lstrcat((*Disasm)->Opcode,temp);
        (*(*index))+=3; // ModRM + Imm8 + ThirdByte
    }
    else{ // Memory form (simplified)
        Imm8 = (BYTE)(*(*Opcode+pos+3)); // approximate - may need displacement offset

        if(usesGPR && ThirdByte >= 0x14 && ThirdByte <= 0x17){
            wsprintf(assembly,"%s %s, %s, %02Xh",mnem,regs[REG32][RM],MMXRegs[REG],Imm8);
        } else {
            wsprintf(assembly,"%s %s, %s, %02Xh",mnem,MMXRegs[REG],MMXRegs[RM],Imm8);
        }

        wsprintf(temp,"%02X %02X",ModRM,Imm8);
        lstrcat((*Disasm)->Opcode,temp);

        // Handle displacement
        if(MOD == 0x00){
            if(RM == 0x05){ (*(*index))+=4; m_OpcodeSize+=4; }
            else if(RM == 0x04){ (*(*index))++; m_OpcodeSize++;
                BYTE sib = (BYTE)(*(*Opcode+pos+3));
                if((sib & 0x07) == 0x05){ (*(*index))+=4; m_OpcodeSize+=4; }
            }
        }
        else if(MOD == 0x01){ (*(*index))++; m_OpcodeSize++;
            if(RM == 0x04){ (*(*index))++; m_OpcodeSize++; }
        }
        else if(MOD == 0x02){ (*(*index))+=4; m_OpcodeSize+=4;
            if(RM == 0x04){ (*(*index))++; m_OpcodeSize++; }
        }
        (*(*index))+=3;
    }

    if(Has66){
        strcpy_s((*Disasm)->Assembly,"");
    }

    lstrcat((*Disasm)->Assembly,assembly);
    (*Disasm)->OpcodeSize = m_OpcodeSize;
}


//==================================================================================//
//                  Decode VEX prefix + AVX/AVX2 instructions                        //
//==================================================================================//

void DecodeVEX(
    DISASSEMBLY **Disasm,
    char **Opcode, DWORD_PTR pos,
    bool AddrPrefix, int SEG, DWORD_PTR **index,
    bool PrefixReg, bool PrefixSeg, bool PrefixAddr)
{
    BYTE VexByte1 = (BYTE)(*(*Opcode+pos));   // 0xC4 or 0xC5
    BYTE P1 = (BYTE)(*(*Opcode+pos+1));

    char assembly[256]="", temp[128]="";
    DWORD_PTR m_OpcodeSize = 0;

    // Decoded VEX fields
    BYTE R=1, X=1, B=1, W=0;
    BYTE vvvv=0, L=0, pp=0, mmmmm=1;

    if(VexByte1 == 0xC5){ // 2-byte VEX
        R     = (P1 >> 7) & 1;
        vvvv  = (~P1 >> 3) & 0x0F;
        L     = (P1 >> 2) & 1;
        pp    = P1 & 0x03;
        mmmmm = 1; // implied 0F map

        wsprintf(temp,"%02X%02X",VexByte1,P1);
        lstrcat((*Disasm)->Opcode,temp);
        m_OpcodeSize = 2;
    }
    else{ // 3-byte VEX (0xC4)
        BYTE P2 = (BYTE)(*(*Opcode+pos+2));
        R     = (P1 >> 7) & 1;
        X     = (P1 >> 6) & 1;
        B     = (P1 >> 5) & 1;
        mmmmm = P1 & 0x1F;
        W     = (P2 >> 7) & 1;
        vvvv  = (~P2 >> 3) & 0x0F;
        L     = (P2 >> 2) & 1;
        pp    = P2 & 0x03;

        wsprintf(temp,"%02X%02X%02X",VexByte1,P1,P2);
        lstrcat((*Disasm)->Opcode,temp);
        m_OpcodeSize = 3;
    }

    // Get the actual opcode byte
    BYTE OpByte = (BYTE)(*(*Opcode+pos+m_OpcodeSize));
    BYTE ModRM  = (BYTE)(*(*Opcode+pos+m_OpcodeSize+1));
    BYTE MOD = (ModRM >> 6) & 0x03;
    BYTE REG = ((ModRM >> 3) & 0x07) | ((R^1) << 3); // R inverted
    BYTE RM  = (ModRM & 0x07) | ((B^1) << 3);

    wsprintf(temp," %02X%02X",OpByte,ModRM);
    lstrcat((*Disasm)->Opcode,temp);
    m_OpcodeSize += 2; // opcode + ModRM

    // Select register set based on L
    const char **dstRegs = L ? YMMRegs : XMMRegs;
    const char **srcRegs = L ? YMMRegs : XMMRegs;

    // Resolve mnemonic from opcode map
    // pp: 0=none, 1=66, 2=F3, 3=F2
    const char *baseMnem = NULL;

    // Map of common AVX instructions (mmmmm=1 = 0F map)
    if(mmmmm == 1){
        switch(OpByte){
            // Data movement
            case 0x10: baseMnem = (pp==2)?"movss":(pp==3)?"movsd":(pp==1)?"movupd":"movups"; break;
            case 0x11: baseMnem = (pp==2)?"movss":(pp==3)?"movsd":(pp==1)?"movupd":"movups"; break;
            case 0x12: baseMnem = (pp==3)?"movddup":(pp==2)?"movsldup":(pp==1)?"movlpd":"movlps"; break;
            case 0x13: baseMnem = (pp==1)?"movlpd":"movlps"; break;
            case 0x14: baseMnem = (pp==1)?"unpcklpd":"unpcklps"; break;
            case 0x15: baseMnem = (pp==1)?"unpckhpd":"unpckhps"; break;
            // AVX-512 opmask register instructions (VEX-encoded)
            case 0x41: baseMnem = W?(pp==1?"kandd":"kandq"):(pp==1?"kandb":"kandw"); break;
            case 0x42: baseMnem = W?(pp==1?"kandnd":"kandnq"):(pp==1?"kandnb":"kandnw"); break;
            case 0x44: baseMnem = W?(pp==1?"knotd":"knotq"):(pp==1?"knotb":"knotw"); break;
            case 0x45: baseMnem = W?(pp==1?"kord":"korq"):(pp==1?"korb":"korw"); break;
            case 0x46: baseMnem = W?(pp==1?"kxnord":"kxnorq"):(pp==1?"kxnorb":"kxnorw"); break;
            case 0x47: baseMnem = W?(pp==1?"kxord":"kxorq"):(pp==1?"kxorb":"kxorw"); break;
            case 0x4A: baseMnem = W?(pp==1?"kaddd":"kaddq"):(pp==1?"kaddb":"kaddw"); break;
            case 0x4B: baseMnem = (pp==1)?"kunpckbw":"kunpckwd"; break;
            case 0x16: baseMnem = (pp==2)?"movshdup":(pp==1)?"movhpd":"movlhps"; break;
            case 0x17: baseMnem = (pp==1)?"movhpd":"movhps"; break;
            case 0x28: baseMnem = (pp==1)?"movapd":"movaps"; break;
            case 0x29: baseMnem = (pp==1)?"movapd":"movaps"; break;
            case 0x2A: baseMnem = (pp==2)?"cvtsi2ss":(pp==3)?"cvtsi2sd":"cvtpi2ps"; break;
            case 0x2B: baseMnem = (pp==1)?"movntpd":"movntps"; break;
            case 0x2C: baseMnem = (pp==2)?"cvttss2si":(pp==3)?"cvttsd2si":"cvttps2pi"; break;
            case 0x2D: baseMnem = (pp==2)?"cvtss2si":(pp==3)?"cvtsd2si":"cvtps2pi"; break;
            case 0x2E: baseMnem = (pp==1)?"ucomisd":"ucomiss"; break;
            case 0x2F: baseMnem = (pp==1)?"comisd":"comiss"; break;
            case 0x50: baseMnem = (pp==1)?"movmskpd":"movmskps"; break;
            case 0x51: baseMnem = (pp==2)?"sqrtss":(pp==3)?"sqrtsd":(pp==1)?"sqrtpd":"sqrtps"; break;
            case 0x52: baseMnem = (pp==2)?"rsqrtss":"rsqrtps"; break;
            case 0x53: baseMnem = (pp==2)?"rcpss":"rcpps"; break;
            case 0x54: baseMnem = (pp==1)?"andpd":"andps"; break;
            case 0x55: baseMnem = (pp==1)?"andnpd":"andnps"; break;
            case 0x56: baseMnem = (pp==1)?"orpd":"orps"; break;
            case 0x57: baseMnem = (pp==1)?"xorpd":"xorps"; break;
            case 0x58: baseMnem = (pp==2)?"addss":(pp==3)?"addsd":(pp==1)?"addpd":"addps"; break;
            case 0x59: baseMnem = (pp==2)?"mulss":(pp==3)?"mulsd":(pp==1)?"mulpd":"mulps"; break;
            case 0x5A: baseMnem = (pp==2)?"cvtss2sd":(pp==3)?"cvtsd2ss":(pp==1)?"cvtpd2ps":"cvtps2pd"; break;
            case 0x5B: baseMnem = (pp==2)?"cvttps2dq":(pp==1)?"cvtps2dq":"cvtdq2ps"; break;
            case 0x5C: baseMnem = (pp==2)?"subss":(pp==3)?"subsd":(pp==1)?"subpd":"subps"; break;
            case 0x5D: baseMnem = (pp==2)?"minss":(pp==3)?"minsd":(pp==1)?"minpd":"minps"; break;
            case 0x5E: baseMnem = (pp==2)?"divss":(pp==3)?"divsd":(pp==1)?"divpd":"divps"; break;
            case 0x5F: baseMnem = (pp==2)?"maxss":(pp==3)?"maxsd":(pp==1)?"maxpd":"maxps"; break;
            case 0x60: baseMnem = "punpcklbw"; break;
            case 0x61: baseMnem = "punpcklwd"; break;
            case 0x62: baseMnem = "punpckldq"; break;
            case 0x63: baseMnem = "packsswb"; break;
            case 0x64: baseMnem = "pcmpgtb"; break;
            case 0x65: baseMnem = "pcmpgtw"; break;
            case 0x66: baseMnem = "pcmpgtd"; break;
            case 0x67: baseMnem = "packuswb"; break;
            case 0x68: baseMnem = "punpckhbw"; break;
            case 0x69: baseMnem = "punpckhwd"; break;
            case 0x6A: baseMnem = "punpckhdq"; break;
            case 0x6B: baseMnem = "packssdw"; break;
            case 0x6C: baseMnem = "punpcklqdq"; break;
            case 0x6D: baseMnem = "punpckhqdq"; break;
            case 0x6E: baseMnem = "movd"; break;
            case 0x6F: baseMnem = (pp==2)?"movdqu":(pp==1)?"movdqa":"movq"; break;
            case 0x70: baseMnem = (pp==2)?"pshufhw":(pp==3)?"pshuflw":(pp==1)?"pshufd":"pshufw"; break;
            case 0x74: baseMnem = "pcmpeqb"; break;
            case 0x75: baseMnem = "pcmpeqw"; break;
            case 0x76: baseMnem = "pcmpeqd"; break;
            case 0x77: baseMnem = "zeroupper"; break; // special: no operands when L=0
            case 0x7C: baseMnem = (pp==3)?"haddps":"haddpd"; break;
            case 0x7D: baseMnem = (pp==3)?"hsubps":"hsubpd"; break;
            case 0x7E: baseMnem = (pp==2)?"movq":(pp==1)?"movd":"movd"; break;
            case 0x7F: baseMnem = (pp==2)?"movdqu":(pp==1)?"movdqa":"movq"; break;
            case 0x90: baseMnem = W?(pp==1?"kmovd":"kmovq"):(pp==1?"kmovb":"kmovw"); break;
            case 0x91: baseMnem = W?(pp==1?"kmovd":"kmovq"):(pp==1?"kmovb":"kmovw"); break;
            case 0x92: baseMnem = W?(pp==3?"kmovd":"kmovq"):(pp==1?"kmovb":"kmovw"); break;
            case 0x93: baseMnem = W?(pp==3?"kmovd":"kmovq"):(pp==1?"kmovb":"kmovw"); break;
            case 0x98: baseMnem = W?(pp==1?"kortestd":"kortestq"):(pp==1?"kortestb":"kortestw"); break;
            case 0x99: baseMnem = W?(pp==1?"ktestd":"ktestq"):(pp==1?"ktestb":"ktestw"); break;
            case 0xAE: baseMnem = "ldmxcsr"; break; // special
            case 0xC2: baseMnem = (pp==2)?"cmpss":(pp==3)?"cmpsd":(pp==1)?"cmppd":"cmpps"; break;
            case 0xC6: baseMnem = (pp==1)?"shufpd":"shufps"; break;
            case 0xD0: baseMnem = (pp==3)?"addsubps":"addsubpd"; break;
            case 0xD1: baseMnem = "psrlw"; break;
            case 0xD2: baseMnem = "psrld"; break;
            case 0xD3: baseMnem = "psrlq"; break;
            case 0xD4: baseMnem = "paddq"; break;
            case 0xD5: baseMnem = "pmullw"; break;
            case 0xD6: baseMnem = "movq"; break;
            case 0xD7: baseMnem = "pmovmskb"; break;
            case 0xD8: baseMnem = "psubusb"; break;
            case 0xD9: baseMnem = "psubusw"; break;
            case 0xDA: baseMnem = "pminub"; break;
            case 0xDB: baseMnem = "pand"; break;
            case 0xDC: baseMnem = "paddusb"; break;
            case 0xDD: baseMnem = "paddusw"; break;
            case 0xDE: baseMnem = "pmaxub"; break;
            case 0xDF: baseMnem = "pandn"; break;
            case 0xE0: baseMnem = "pavgb"; break;
            case 0xE1: baseMnem = "psraw"; break;
            case 0xE2: baseMnem = "psrad"; break;
            case 0xE3: baseMnem = "pavgw"; break;
            case 0xE4: baseMnem = "pmulhuw"; break;
            case 0xE5: baseMnem = "pmulhw"; break;
            case 0xE6: baseMnem = (pp==3)?"cvtpd2dq":(pp==2)?"cvtdq2pd":"cvttpd2dq"; break;
            case 0xE7: baseMnem = "movntdq"; break;
            case 0xE8: baseMnem = "psubsb"; break;
            case 0xE9: baseMnem = "psubsw"; break;
            case 0xEA: baseMnem = "pminsw"; break;
            case 0xEB: baseMnem = "por"; break;
            case 0xEC: baseMnem = "paddsb"; break;
            case 0xED: baseMnem = "paddsw"; break;
            case 0xEE: baseMnem = "pmaxsw"; break;
            case 0xEF: baseMnem = "pxor"; break;
            case 0xF0: baseMnem = "lddqu"; break;
            case 0xF1: baseMnem = "psllw"; break;
            case 0xF2: baseMnem = "pslld"; break;
            case 0xF3: baseMnem = "psllq"; break;
            case 0xF4: baseMnem = "pmuludq"; break;
            case 0xF5: baseMnem = "pmaddwd"; break;
            case 0xF6: baseMnem = "psadbw"; break;
            case 0xF7: baseMnem = "maskmovdqu"; break;
            case 0xF8: baseMnem = "psubb"; break;
            case 0xF9: baseMnem = "psubw"; break;
            case 0xFA: baseMnem = "psubd"; break;
            case 0xFB: baseMnem = "psubq"; break;
            case 0xFC: baseMnem = "paddb"; break;
            case 0xFD: baseMnem = "paddw"; break;
            case 0xFE: baseMnem = "paddd"; break;
            default: baseMnem = NULL; break;
        }
    }
    else if(mmmmm == 2){ // 0F 38 map
        switch(OpByte){
            case 0x00: baseMnem = "pshufb"; break;
            case 0x01: baseMnem = "phaddw"; break;
            case 0x02: baseMnem = "phaddd"; break;
            case 0x03: baseMnem = "phaddsw"; break;
            case 0x04: baseMnem = "pmaddubsw"; break;
            case 0x05: baseMnem = "phsubw"; break;
            case 0x06: baseMnem = "phsubd"; break;
            case 0x07: baseMnem = "phsubsw"; break;
            case 0x08: baseMnem = "psignb"; break;
            case 0x09: baseMnem = "psignw"; break;
            case 0x0A: baseMnem = "psignd"; break;
            case 0x0B: baseMnem = "pmulhrsw"; break;
            case 0x0C: baseMnem = "permilps"; break;
            case 0x0D: baseMnem = "permilpd"; break;
            case 0x0E: baseMnem = "testps"; break;
            case 0x0F: baseMnem = "testpd"; break;
            case 0x17: baseMnem = "ptest"; break;
            case 0x18: baseMnem = "broadcastss"; break;
            case 0x19: baseMnem = "broadcastsd"; break;
            case 0x1A: baseMnem = "broadcastf128"; break;
            case 0x1C: baseMnem = "pabsb"; break;
            case 0x1D: baseMnem = "pabsw"; break;
            case 0x1E: baseMnem = "pabsd"; break;
            case 0x20: baseMnem = "pmovsxbw"; break;
            case 0x21: baseMnem = "pmovsxbd"; break;
            case 0x22: baseMnem = "pmovsxbq"; break;
            case 0x23: baseMnem = "pmovsxwd"; break;
            case 0x24: baseMnem = "pmovsxwq"; break;
            case 0x25: baseMnem = "pmovsxdq"; break;
            case 0x28: baseMnem = "pmuldq"; break;
            case 0x29: baseMnem = "pcmpeqq"; break;
            case 0x2A: baseMnem = "movntdqa"; break;
            case 0x2B: baseMnem = "packusdw"; break;
            case 0x2C: baseMnem = "maskmovps"; break;
            case 0x2D: baseMnem = "maskmovpd"; break;
            case 0x2E: baseMnem = "maskmovps"; break;
            case 0x2F: baseMnem = "maskmovpd"; break;
            case 0x30: baseMnem = "pmovzxbw"; break;
            case 0x31: baseMnem = "pmovzxbd"; break;
            case 0x32: baseMnem = "pmovzxbq"; break;
            case 0x33: baseMnem = "pmovzxwd"; break;
            case 0x34: baseMnem = "pmovzxwq"; break;
            case 0x35: baseMnem = "pmovzxdq"; break;
            case 0x36: baseMnem = "permd"; break;
            case 0x37: baseMnem = "pcmpgtq"; break;
            case 0x38: baseMnem = "pminsb"; break;
            case 0x39: baseMnem = "pminsd"; break;
            case 0x3A: baseMnem = "pminuw"; break;
            case 0x3B: baseMnem = "pminud"; break;
            case 0x3C: baseMnem = "pmaxsb"; break;
            case 0x3D: baseMnem = "pmaxsd"; break;
            case 0x3E: baseMnem = "pmaxuw"; break;
            case 0x3F: baseMnem = "pmaxud"; break;
            case 0x40: baseMnem = "pmulld"; break;
            case 0x41: baseMnem = "phminposuw"; break;
            case 0x45: baseMnem = W ? "psrlvq" : "psrlvd"; break;
            case 0x46: baseMnem = "psravd"; break;
            case 0x47: baseMnem = W ? "psllvq" : "psllvd"; break;
            case 0x58: baseMnem = "pbroadcastd"; break;
            case 0x59: baseMnem = "pbroadcastq"; break;
            case 0x5A: baseMnem = "broadcasti128"; break;
            case 0x78: baseMnem = "pbroadcastb"; break;
            case 0x79: baseMnem = "pbroadcastw"; break;
            case 0x8C: baseMnem = "pmaskmovd"; break;
            case 0x8E: baseMnem = "pmaskmovd"; break;
            case 0x90: baseMnem = W ? "pgatherdq" : "pgatherdd"; break;
            case 0x91: baseMnem = W ? "pgatherqq" : "pgatherqd"; break;
            case 0x92: baseMnem = W ? "gatherdpd" : "gatherdps"; break;
            case 0x93: baseMnem = W ? "gatherqpd" : "gatherqps"; break;
            // FMA3 (0F38 map)
            case 0x96: baseMnem = W ? "fmaddsub132pd" : "fmaddsub132ps"; break;
            case 0x97: baseMnem = W ? "fmsubadd132pd" : "fmsubadd132ps"; break;
            case 0x98: baseMnem = W ? "fmadd132pd" : "fmadd132ps"; break;
            case 0x99: baseMnem = W ? "fmadd132sd" : "fmadd132ss"; break;
            case 0x9A: baseMnem = W ? "fmsub132pd" : "fmsub132ps"; break;
            case 0x9B: baseMnem = W ? "fmsub132sd" : "fmsub132ss"; break;
            case 0x9C: baseMnem = W ? "fnmadd132pd" : "fnmadd132ps"; break;
            case 0x9D: baseMnem = W ? "fnmadd132sd" : "fnmadd132ss"; break;
            case 0x9E: baseMnem = W ? "fnmsub132pd" : "fnmsub132ps"; break;
            case 0x9F: baseMnem = W ? "fnmsub132sd" : "fnmsub132ss"; break;
            case 0xA6: baseMnem = W ? "fmaddsub213pd" : "fmaddsub213ps"; break;
            case 0xA7: baseMnem = W ? "fmsubadd213pd" : "fmsubadd213ps"; break;
            case 0xA8: baseMnem = W ? "fmadd213pd" : "fmadd213ps"; break;
            case 0xA9: baseMnem = W ? "fmadd213sd" : "fmadd213ss"; break;
            case 0xAA: baseMnem = W ? "fmsub213pd" : "fmsub213ps"; break;
            case 0xAB: baseMnem = W ? "fmsub213sd" : "fmsub213ss"; break;
            case 0xAC: baseMnem = W ? "fnmadd213pd" : "fnmadd213ps"; break;
            case 0xAD: baseMnem = W ? "fnmadd213sd" : "fnmadd213ss"; break;
            case 0xAE: baseMnem = W ? "fnmsub213pd" : "fnmsub213ps"; break;
            case 0xAF: baseMnem = W ? "fnmsub213sd" : "fnmsub213ss"; break;
            case 0xB6: baseMnem = W ? "fmaddsub231pd" : "fmaddsub231ps"; break;
            case 0xB7: baseMnem = W ? "fmsubadd231pd" : "fmsubadd231ps"; break;
            case 0xB8: baseMnem = W ? "fmadd231pd" : "fmadd231ps"; break;
            case 0xB9: baseMnem = W ? "fmadd231sd" : "fmadd231ss"; break;
            case 0xBA: baseMnem = W ? "fmsub231pd" : "fmsub231ps"; break;
            case 0xBB: baseMnem = W ? "fmsub231sd" : "fmsub231ss"; break;
            case 0xBC: baseMnem = W ? "fnmadd231pd" : "fnmadd231ps"; break;
            case 0xBD: baseMnem = W ? "fnmadd231sd" : "fnmadd231ss"; break;
            case 0xBE: baseMnem = W ? "fnmsub231pd" : "fnmsub231ps"; break;
            case 0xBF: baseMnem = W ? "fnmsub231sd" : "fnmsub231ss"; break;
            // F16C: vcvtph2ps (VEX.256/128.66.0F38.W0 13)
            case 0x13: if(pp==1) baseMnem = "cvtph2ps"; break;
            // BMI1: ANDN (VEX.NDS.LZ.0F38.W0/W1 F2)
            case 0xF2: if(pp==0) baseMnem = "andn"; break;
            // BMI1: BLSI/BLSMSK/BLSR (VEX.NDD.LZ.0F38.W0/W1 F3 /reg)
            case 0xF3:{
                if(pp==0){
                    BYTE regField = (ModRM >> 3) & 0x07;
                    if(regField==1) baseMnem = "blsr";
                    else if(regField==2) baseMnem = "blsmsk";
                    else if(regField==3) baseMnem = "blsi";
                }
                break;
            }
            // BMI2: BZHI (pp=0), PDEP (pp=3/F2), PEXT (pp=2/F3) — opcode F5
            case 0xF5:{
                if(pp==0) baseMnem = "bzhi";
                else if(pp==3) baseMnem = "pdep";
                else if(pp==2) baseMnem = "pext";
                break;
            }
            // BMI2: MULX (F2), opcode F6
            case 0xF6: if(pp==3) baseMnem = "mulx"; break;
            // BMI1: BEXTR (pp=0), BMI2: SARX (pp=2/F3), SHLX (pp=1/66), SHRX (pp=3/F2) — opcode F7
            case 0xF7:{
                if(pp==0) baseMnem = "bextr";
                else if(pp==2) baseMnem = "sarx";
                else if(pp==1) baseMnem = "shlx";
                else if(pp==3) baseMnem = "shrx";
                break;
            }
            // GFNI (VEX.66.0F38 CF)
            case 0xCF: if(pp==1) baseMnem = "gf2p8mulb"; break;
            // VAES (VEX.66.0F38 DC-DF)
            case 0xDC: if(pp==1) baseMnem = "aesenc"; break;
            case 0xDD: if(pp==1) baseMnem = "aesenclast"; break;
            case 0xDE: if(pp==1) baseMnem = "aesdec"; break;
            case 0xDF: if(pp==1) baseMnem = "aesdeclast"; break;
            default: baseMnem = NULL; break;
        }
    }
    else if(mmmmm == 3){ // 0F 3A map
        switch(OpByte){
            case 0x00: baseMnem = "permq"; break;
            case 0x01: baseMnem = "permpd"; break;
            case 0x02: baseMnem = "pblendd"; break;
            case 0x04: baseMnem = "permilps"; break;
            case 0x05: baseMnem = "permilpd"; break;
            case 0x06: baseMnem = "perm2f128"; break;
            case 0x08: baseMnem = "roundps"; break;
            case 0x09: baseMnem = "roundpd"; break;
            case 0x0A: baseMnem = "roundss"; break;
            case 0x0B: baseMnem = "roundsd"; break;
            case 0x0C: baseMnem = "blendps"; break;
            case 0x0D: baseMnem = "blendpd"; break;
            case 0x0E: baseMnem = "pblendw"; break;
            case 0x0F: baseMnem = "palignr"; break;
            case 0x14: baseMnem = "pextrb"; break;
            case 0x15: baseMnem = "pextrw"; break;
            case 0x16: baseMnem = "pextrd"; break;
            case 0x17: baseMnem = "extractps"; break;
            case 0x18: baseMnem = "insertf128"; break;
            case 0x19: baseMnem = "extractf128"; break;
            case 0x20: baseMnem = "pinsrb"; break;
            case 0x21: baseMnem = "insertps"; break;
            case 0x22: baseMnem = "pinsrd"; break;
            // AVX-512 opmask shift instructions
            case 0x30: baseMnem = W?"kshiftrq":"kshiftrw"; break;
            case 0x31: baseMnem = W?"kshiftrd":"kshiftrb"; break;
            case 0x32: baseMnem = W?"kshiftlq":"kshiftlw"; break;
            case 0x33: baseMnem = W?"kshiftld":"kshiftlb"; break;
            case 0x38: baseMnem = "inserti128"; break;
            case 0x39: baseMnem = "extracti128"; break;
            case 0x40: baseMnem = "dpps"; break;
            case 0x41: baseMnem = "dppd"; break;
            case 0x42: baseMnem = "mpsadbw"; break;
            case 0x44: baseMnem = "pclmulqdq"; break;
            case 0x46: baseMnem = "perm2i128"; break;
            case 0x4A: baseMnem = "blendvps"; break;
            case 0x4B: baseMnem = "blendvpd"; break;
            case 0x4C: baseMnem = "pblendvb"; break;
            case 0x60: baseMnem = "pcmpestrm"; break;
            case 0x61: baseMnem = "pcmpestri"; break;
            case 0x62: baseMnem = "pcmpistrm"; break;
            case 0x63: baseMnem = "pcmpistri"; break;
            // F16C: vcvtps2ph (VEX.256/128.66.0F3A.W0 1D)
            case 0x1D: if(pp==1) baseMnem = "cvtps2ph"; break;
            // BMI2: RORX (VEX.LZ.F2.0F3A.W0/W1 F0)
            case 0xF0: if(pp==3) baseMnem = "rorx"; break;
            // GFNI (VEX.66.0F3A CE/CF)
            case 0xCE: if(pp==1) baseMnem = "gf2p8affineqb"; break;
            case 0xCF: if(pp==1) baseMnem = "gf2p8affineinvqb"; break;
            default: baseMnem = NULL; break;
        }
    }

    if(baseMnem == NULL){
        // Account for immediate byte on map 0F3A even for unknown opcodes
        if(mmmmm == 3){
            wsprintf(temp," %02X",(BYTE)(*(*Opcode+pos+m_OpcodeSize)));
            lstrcat((*Disasm)->Opcode,temp);
            m_OpcodeSize++;
        }
        wsprintf(assembly,"??? (VEX)");
        lstrcat((*Disasm)->Assembly,assembly);
        (*Disasm)->OpcodeSize = m_OpcodeSize;
        **index = pos + m_OpcodeSize - 1; // main loop does Index++
        return;
    }

    // Clear any prefixes from assembly
    strcpy_s((*Disasm)->Assembly,"");

    // Build the AVX mnemonic with "v" prefix (K-register ops already have 'k' prefix)
    // BMI1/BMI2 instructions don't get "v" prefix
    char avxMnem[64];
    bool isBMI = false;
    if(mmmmm == 2 && (OpByte >= 0xF2 && OpByte <= 0xF7)){
        // BMI1/BMI2 instructions use GPR, no "v" prefix
        isBMI = true;
    }
    // RORX in map 3
    if(mmmmm == 3 && OpByte == 0xF0 && pp == 3){
        isBMI = true;
    }

    if(baseMnem[0] == 'k')
        wsprintf(avxMnem,"%s",baseMnem);
    else if(isBMI)
        wsprintf(avxMnem,"%s",baseMnem);
    else
        wsprintf(avxMnem,"v%s",baseMnem);

    // For BMI instructions, use GPR register set
    int gprSize = W ? REG64 : REG32;
    if(isBMI){
        dstRegs = NULL; // signal to use regs[][] below
        srcRegs = NULL;
    }

    // Format operands - 3-operand form: vop dest, vvvv, src
    // Some instructions don't use vvvv (moves, converts, etc.)
    bool needsVVVV = true;
    // Instructions that don't use vvvv source
    if(mmmmm == 1){
        switch(OpByte){
            case 0x10: case 0x11: case 0x12: case 0x13:
            case 0x16: case 0x17: case 0x28: case 0x29:
            case 0x2B: case 0x2E: case 0x2F: case 0x50:
            case 0x6E: case 0x6F: case 0x70: case 0x77:
            case 0x7E: case 0x7F: case 0xAE:
            case 0xD6: case 0xD7: case 0xE7: case 0xF0:
            case 0x2A: case 0x2C: case 0x2D:
            case 0x5A: case 0x5B: case 0xE6:
                needsVVVV = false;
                break;
        }
    }

    if(isBMI){
        // BMI instructions: use GPR registers
        // Most BMI: dest=REG, src1=vvvv, src2=RM (3-operand)
        // BLSI/BLSMSK/BLSR: dest=vvvv, src=RM (2-operand, vvvv is dest)
        // BEXTR/BZHI: dest=REG, src1=RM, src2=vvvv (different order)
        if(mmmmm==2 && OpByte==0xF3 && pp==0){
            // BLSI/BLSMSK/BLSR: vvvv = dest, RM = src
            if(MOD == 0x03)
                wsprintf(assembly,"%s %s, %s",avxMnem,regs[gprSize][vvvv&0x0F],regs[gprSize][RM&0x0F]);
            else
                wsprintf(assembly,"%s %s, [...]",avxMnem,regs[gprSize][vvvv&0x0F]);
        }
        else if(mmmmm==2 && (OpByte==0xF7 && pp==0)){ // BEXTR: dest=REG, src1=RM, src2=vvvv
            if(MOD == 0x03)
                wsprintf(assembly,"%s %s, %s, %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][RM&0x0F],regs[gprSize][vvvv&0x0F]);
            else
                wsprintf(assembly,"%s %s, [...], %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][vvvv&0x0F]);
        }
        else if(mmmmm==2 && OpByte==0xF5 && pp==0){ // BZHI: dest=REG, src1=RM, src2=vvvv
            if(MOD == 0x03)
                wsprintf(assembly,"%s %s, %s, %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][RM&0x0F],regs[gprSize][vvvv&0x0F]);
            else
                wsprintf(assembly,"%s %s, [...], %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][vvvv&0x0F]);
        }
        else if(mmmmm==2 && (OpByte==0xF7 && pp!=0)){ // SARX/SHLX/SHRX: dest=REG, src1=RM, src2=vvvv
            if(MOD == 0x03)
                wsprintf(assembly,"%s %s, %s, %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][RM&0x0F],regs[gprSize][vvvv&0x0F]);
            else
                wsprintf(assembly,"%s %s, [...], %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][vvvv&0x0F]);
        }
        else if(mmmmm==3 && OpByte==0xF0 && pp==3){ // RORX: dest=REG, src=RM (imm8 handled later)
            if(MOD == 0x03)
                wsprintf(assembly,"%s %s, %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][RM&0x0F]);
            else
                wsprintf(assembly,"%s %s, [...]",avxMnem,regs[gprSize][REG&0x0F]);
            needsVVVV = false;
        }
        else{
            // Default BMI: dest=REG, src1=vvvv, src2=RM (ANDN, PDEP, PEXT, MULX)
            if(MOD == 0x03)
                wsprintf(assembly,"%s %s, %s, %s",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][vvvv&0x0F],regs[gprSize][RM&0x0F]);
            else
                wsprintf(assembly,"%s %s, %s, [...]",avxMnem,regs[gprSize][REG&0x0F],regs[gprSize][vvvv&0x0F]);
        }

        // Handle memory displacement for BMI
        if(MOD != 0x03){
            if(MOD == 0x00){
                if((RM&0x07) == 0x05){ m_OpcodeSize+=4; }
                else if((RM&0x07) == 0x04){ m_OpcodeSize++; }
            }
            else if(MOD == 0x01){ m_OpcodeSize++;
                if((RM&0x07) == 0x04){ m_OpcodeSize++; }
            }
            else if(MOD == 0x02){ m_OpcodeSize+=4;
                if((RM&0x07) == 0x04){ m_OpcodeSize++; }
            }
        }
    }
    else if(MOD == 0x03){ // reg,reg form
        if(needsVVVV && vvvv < 16){
            wsprintf(assembly,"%s %s, %s, %s",avxMnem,dstRegs[REG&0x0F],dstRegs[vvvv],srcRegs[RM&0x0F]);
        } else {
            wsprintf(assembly,"%s %s, %s",avxMnem,dstRegs[REG&0x0F],srcRegs[RM&0x0F]);
        }
    }
    else{ // memory form (simplified)
        if(needsVVVV && vvvv < 16){
            wsprintf(assembly,"%s %s, %s, %s",avxMnem,dstRegs[REG&0x0F],dstRegs[vvvv],srcRegs[RM&0x07]);
        } else {
            wsprintf(assembly,"%s %s, %s",avxMnem,dstRegs[REG&0x0F],srcRegs[RM&0x07]);
        }

        // Handle displacement
        if(MOD == 0x00){
            if((RM&0x07) == 0x05){ m_OpcodeSize+=4; }
            else if((RM&0x07) == 0x04){ m_OpcodeSize++; }
        }
        else if(MOD == 0x01){ m_OpcodeSize++;
            if((RM&0x07) == 0x04){ m_OpcodeSize++; }
        }
        else if(MOD == 0x02){ m_OpcodeSize+=4;
            if((RM&0x07) == 0x04){ m_OpcodeSize++; }
        }
    }

    // Handle immediate byte for certain instructions
    if(mmmmm == 3 || OpByte == 0x70 || OpByte == 0xC2 || OpByte == 0xC6){
        BYTE imm = (BYTE)(*(*Opcode+pos+m_OpcodeSize));
        char immStr[16];
        wsprintf(immStr,", %02Xh",imm);
        lstrcat(assembly,immStr);
        wsprintf(temp," %02X",imm);
        lstrcat((*Disasm)->Opcode,temp);
        m_OpcodeSize++;
    }

    **index = pos + m_OpcodeSize - 1; // main loop does Index++
    lstrcat((*Disasm)->Assembly,assembly);
    (*Disasm)->OpcodeSize = m_OpcodeSize;
    (*Disasm)->IsVEX = 1;
    (*Disasm)->VexVVVV = vvvv;
    (*Disasm)->VexL = L;
    (*Disasm)->VexW = W;
    (*Disasm)->VexPP = pp;
    (*Disasm)->VexMMMM = mmmmm;
}


//==================================================================================//
//                  Decode EVEX prefix + AVX-512F instructions                       //
//==================================================================================//

void DecodeEVEX(
    DISASSEMBLY **Disasm,
    char **Opcode, DWORD_PTR pos,
    bool AddrPrefix, int SEG, DWORD_PTR **index,
    bool PrefixReg, bool PrefixSeg, bool PrefixAddr)
{
    // EVEX is 4 bytes: 62 P0 P1 P2
    BYTE P0 = (BYTE)(*(*Opcode+pos+1));
    BYTE P1 = (BYTE)(*(*Opcode+pos+2));
    BYTE P2 = (BYTE)(*(*Opcode+pos+3));

    char assembly[256]="", temp[128]="";
    DWORD_PTR m_OpcodeSize = 4; // EVEX prefix

    // Decode EVEX fields
    BYTE R    = (P0 >> 7) & 1;      // inverted
    BYTE X    = (P0 >> 6) & 1;      // inverted
    BYTE B    = (P0 >> 5) & 1;      // inverted
    BYTE Rp   = (P0 >> 4) & 1;      // R' inverted
    BYTE mm   = P0 & 0x03;          // opcode map (1=0F, 2=0F38, 3=0F3A)

    BYTE W    = (P1 >> 7) & 1;
    BYTE vvvv = (~P1 >> 3) & 0x0F;  // inverted
    BYTE pp   = P1 & 0x03;          // mandatory prefix

    BYTE z    = (P2 >> 7) & 1;      // zeroing-masking
    BYTE LL   = (P2 >> 5) & 0x03;   // vector length: 0=128, 1=256, 2=512
    BYTE b_bit= (P2 >> 4) & 1;      // broadcast/RC/SAE
    BYTE Vp   = (~P2 >> 3) & 1;     // V' (5th bit of vvvv) inverted
    BYTE aaa  = P2 & 0x07;          // opmask register

    wsprintf(temp,"%02X%02X%02X%02X",0x62,P0,P1,P2);
    lstrcat((*Disasm)->Opcode,temp);

    // Get opcode byte and ModRM
    BYTE OpByte = (BYTE)(*(*Opcode+pos+4));
    BYTE ModRM  = (BYTE)(*(*Opcode+pos+5));
    BYTE MOD = (ModRM >> 6) & 0x03;
    BYTE REG_field = ((ModRM >> 3) & 0x07) | ((R^1) << 3) | ((Rp^1) << 4);
    BYTE RM_field  = (ModRM & 0x07) | ((B^1) << 3) | ((X^1) << 4);
    BYTE VVVV_full = vvvv | (Vp << 4);

    wsprintf(temp," %02X%02X",OpByte,ModRM);
    lstrcat((*Disasm)->Opcode,temp);
    m_OpcodeSize += 2;

    // Select register set based on LL
    const char **dstRegs;
    switch(LL){
        case 0: dstRegs = XMMRegs; break;
        case 1: dstRegs = YMMRegs; break;
        case 2: default: dstRegs = ZMMRegs; break;
    }

    // Resolve mnemonic (AVX-512F subset)
    const char *baseMnem = NULL;
    if(mm == 1){ // 0F map
        switch(OpByte){
            case 0x10: baseMnem = (pp==2)?"movss":(pp==3)?"movsd":(pp==1)?"movupd":"movups"; break;
            case 0x11: baseMnem = (pp==2)?"movss":(pp==3)?"movsd":(pp==1)?"movupd":"movups"; break;
            case 0x14: baseMnem = (pp==1)?"unpcklpd":"unpcklps"; break;
            case 0x15: baseMnem = (pp==1)?"unpckhpd":"unpckhps"; break;
            case 0x28: baseMnem = (pp==1)?"movapd":"movaps"; break;
            case 0x29: baseMnem = (pp==1)?"movapd":"movaps"; break;
            case 0x51: baseMnem = (pp==2)?"sqrtss":(pp==3)?"sqrtsd":(pp==1)?"sqrtpd":"sqrtps"; break;
            case 0x54: baseMnem = (pp==1)?"andpd":"andps"; break;
            case 0x55: baseMnem = (pp==1)?"andnpd":"andnps"; break;
            case 0x56: baseMnem = (pp==1)?"orpd":"orps"; break;
            case 0x57: baseMnem = (pp==1)?"xorpd":"xorps"; break;
            case 0x58: baseMnem = (pp==2)?"addss":(pp==3)?"addsd":(pp==1)?"addpd":"addps"; break;
            case 0x59: baseMnem = (pp==2)?"mulss":(pp==3)?"mulsd":(pp==1)?"mulpd":"mulps"; break;
            case 0x5A: baseMnem = (pp==2)?"cvtss2sd":(pp==3)?"cvtsd2ss":(pp==1)?"cvtpd2ps":"cvtps2pd"; break;
            case 0x5B: baseMnem = (pp==2)?"cvttps2dq":(pp==1)?"cvtps2dq":W?"cvtqq2ps":"cvtdq2ps"; break;
            case 0x5C: baseMnem = (pp==2)?"subss":(pp==3)?"subsd":(pp==1)?"subpd":"subps"; break;
            case 0x5D: baseMnem = (pp==2)?"minss":(pp==3)?"minsd":(pp==1)?"minpd":"minps"; break;
            case 0x5E: baseMnem = (pp==2)?"divss":(pp==3)?"divsd":(pp==1)?"divpd":"divps"; break;
            case 0x5F: baseMnem = (pp==2)?"maxss":(pp==3)?"maxsd":(pp==1)?"maxpd":"maxps"; break;
            case 0x66: baseMnem = "pcmpgtd"; break;
            case 0x6F: baseMnem = W?(pp==2?"movdqu64":(pp==1?"movdqa64":"movdqu64")):(pp==2?"movdqu32":(pp==1?"movdqa32":"movdqu32")); break;
            case 0x76: baseMnem = "pcmpeqd"; break;
            case 0x7F: baseMnem = W?(pp==2?"movdqu64":(pp==1?"movdqa64":"movdqu64")):(pp==2?"movdqu32":(pp==1?"movdqa32":"movdqu32")); break;
            case 0xC2: baseMnem = (pp==2)?"cmpss":(pp==3)?"cmpsd":(pp==1)?"cmppd":"cmpps"; break;
            case 0xC6: baseMnem = (pp==1)?"shufpd":"shufps"; break;
            // AVX-512 BW: integer byte/word ops
            case 0x60: baseMnem = "punpcklbw"; break;
            case 0x61: baseMnem = "punpcklwd"; break;
            case 0x62: baseMnem = "punpckldq"; break;
            case 0x63: baseMnem = "packsswb"; break;
            case 0x64: baseMnem = "pcmpgtb"; break;
            case 0x65: baseMnem = "pcmpgtw"; break;
            case 0x67: baseMnem = "packuswb"; break;
            case 0x68: baseMnem = "punpckhbw"; break;
            case 0x69: baseMnem = "punpckhwd"; break;
            case 0x6A: baseMnem = "punpckhdq"; break;
            case 0x6B: baseMnem = "packssdw"; break;
            case 0x6C: baseMnem = "punpcklqdq"; break;
            case 0x6D: baseMnem = "punpckhqdq"; break;
            // AVX-512 DQ: movd/movq
            case 0x6E: baseMnem = W?"movq":"movd"; break;
            case 0x70: baseMnem = (pp==1)?"pshufd":(pp==2)?"pshufhw":(pp==3)?"pshuflw":NULL; break;
            case 0x74: baseMnem = "pcmpeqb"; break;
            case 0x75: baseMnem = "pcmpeqw"; break;
            // AVX-512 DQ: conversion instructions
            case 0x78: baseMnem = (pp==1)?W?"cvttpd2udq":"cvttps2udq" : (pp==2)?W?"cvttsd2usi":"cvttss2usi" : NULL; break;
            case 0x79: baseMnem = (pp==1)?W?"cvtpd2udq":"cvtps2udq" : (pp==2)?W?"cvtsd2usi":"cvtss2usi" : NULL; break;
            case 0x7A: baseMnem = (pp==1)?W?"cvttpd2qq":"cvttps2qq" : (pp==2)?"cvtudq2pd" : (pp==3)?"cvtudq2ps" : NULL; break;
            case 0x7B: baseMnem = (pp==1)?W?"cvtpd2qq":"cvtps2qq" : (pp==2)?W?"cvtusi2sd":"cvtusi2ss" : (pp==3)?"cvtpd2dq" : NULL; break;
            case 0x7E: baseMnem = W?"movq":"movd"; break;
            case 0xD1: baseMnem = "psrlw"; break;
            case 0xD2: baseMnem = "psrld"; break;
            case 0xD3: baseMnem = "psrlq"; break;
            case 0xD4: baseMnem = "paddq"; break;
            case 0xD5: baseMnem = "pmullw"; break;
            case 0xD8: baseMnem = "psubusb"; break;
            case 0xD9: baseMnem = "psubusw"; break;
            case 0xDA: baseMnem = "pminub"; break;
            case 0xDB: baseMnem = W?"pandq":"pandd"; break;
            case 0xDC: baseMnem = "paddusb"; break;
            case 0xDD: baseMnem = "paddusw"; break;
            case 0xDE: baseMnem = "pmaxub"; break;
            case 0xDF: baseMnem = W?"pandnq":"pandnd"; break;
            case 0xE0: baseMnem = "pavgb"; break;
            case 0xE1: baseMnem = "psraw"; break;
            case 0xE2: baseMnem = "psrad"; break;
            case 0xE3: baseMnem = "pavgw"; break;
            case 0xE4: baseMnem = "pmulhuw"; break;
            case 0xE5: baseMnem = "pmulhw"; break;
            case 0xE6: baseMnem = (pp==1)?"cvttpd2dq":(pp==2)?(W?"cvtqq2pd":"cvtdq2pd"):(pp==3)?"cvtpd2dq":NULL; break;
            case 0xE8: baseMnem = "psubsb"; break;
            case 0xE9: baseMnem = "psubsw"; break;
            case 0xEA: baseMnem = "pminsw"; break;
            case 0xEB: baseMnem = W?"porq":"pord"; break;
            case 0xEC: baseMnem = "paddsb"; break;
            case 0xED: baseMnem = "paddsw"; break;
            case 0xEE: baseMnem = "pmaxsw"; break;
            case 0xEF: baseMnem = W?"pxorq":"pxord"; break;
            case 0xF1: baseMnem = "psllw"; break;
            case 0xF2: baseMnem = "pslld"; break;
            case 0xF3: baseMnem = "psllq"; break;
            case 0xF4: baseMnem = "pmuludq"; break;
            case 0xF5: baseMnem = "pmaddwd"; break;
            case 0xF6: baseMnem = "psadbw"; break;
            case 0xF8: baseMnem = "psubb"; break;
            case 0xF9: baseMnem = "psubw"; break;
            case 0xFA: baseMnem = "psubd"; break;
            case 0xFB: baseMnem = "psubq"; break;
            case 0xFC: baseMnem = "paddb"; break;
            case 0xFD: baseMnem = "paddw"; break;
            case 0xFE: baseMnem = "paddd"; break;
            default: baseMnem = NULL; break;
        }
    }
    else if(mm == 2){ // 0F 38 map
        switch(OpByte){
            case 0x04: baseMnem = "pmaddubsw"; break;
            case 0x0B: baseMnem = "pmulhrsw"; break;
            case 0x0C: baseMnem = "permilps"; break;
            case 0x0D: baseMnem = "permilpd"; break;
            case 0x10: baseMnem = (pp==2)?"pmovuswb":"psrlvw"; break;
            case 0x11: baseMnem = (pp==2)?"pmovusdb":"psravw"; break;
            case 0x12: baseMnem = (pp==2)?"pmovusqb":"psllvw"; break;
            case 0x13: if(pp==2) baseMnem = "pmovusdw"; break;
            case 0x14: baseMnem = (pp==2)?"pmovusqw":"prorvd"; break;
            case 0x15: baseMnem = (pp==2)?"pmovusqd":"prolvd"; break;
            case 0x18: baseMnem = "broadcastss"; break;
            case 0x19: baseMnem = "broadcastsd"; break;
            case 0x1A: baseMnem = W?"broadcasti64x4":"broadcasti32x4"; break;
            case 0x1E: baseMnem = "pabsd"; break;
            case 0x1F: baseMnem = "pabsq"; break;
            case 0x20: if(pp==2) baseMnem = "pmovswb"; break;
            case 0x21: if(pp==2) baseMnem = "pmovsdb"; break;
            case 0x22: if(pp==2) baseMnem = "pmovsqb"; break;
            case 0x23: if(pp==2) baseMnem = "pmovsdw"; break;
            case 0x24: if(pp==2) baseMnem = "pmovsqw"; break;
            case 0x25: if(pp==2) baseMnem = "pmovsqd"; break;
            case 0x27: baseMnem = "ptestnmd"; break;
            case 0x28: baseMnem = (pp==2)?(W?"pmovm2w":"pmovm2b"):"pmuldq"; break;
            case 0x29: baseMnem = (pp==2)?(W?"pmovw2m":"pmovb2m"):"pcmpeqq"; break;
            case 0x2A: baseMnem = "movntdqa"; break;
            case 0x30: if(pp==2) baseMnem = "pmovwb"; break;
            case 0x31: if(pp==2) baseMnem = "pmovdb"; break;
            case 0x32: if(pp==2) baseMnem = "pmovqb"; break;
            case 0x33: if(pp==2) baseMnem = "pmovdw"; break;
            case 0x34: if(pp==2) baseMnem = "pmovqw"; break;
            case 0x35: if(pp==2) baseMnem = "pmovqd"; break;
            case 0x36: baseMnem = W?"permq":"permd"; break;
            case 0x37: baseMnem = "pcmpgtq"; break;
            case 0x38: baseMnem = (pp==2)?(W?"pmovm2q":"pmovm2d"):"pminsb"; break;
            case 0x39: baseMnem = (pp==2)?(W?"pmovq2m":"pmovd2m"):"pminsd"; break;
            case 0x3A: baseMnem = "pminuw"; break;
            case 0x3B: baseMnem = "pminud"; break;
            case 0x3C: baseMnem = "pmaxsb"; break;
            case 0x3D: baseMnem = "pmaxsd"; break;
            case 0x3E: baseMnem = "pmaxuw"; break;
            case 0x3F: baseMnem = "pmaxud"; break;
            case 0x40: baseMnem = W?"pmullq":"pmulld"; break; // AVX-512 DQ: W-dependent
            case 0x45: baseMnem = W?"psrlvq":"psrlvd"; break;
            case 0x46: baseMnem = W?"psravq":"psravd"; break;
            case 0x47: baseMnem = W?"psllvq":"psllvd"; break;
            case 0x4C: baseMnem = W?"rcp14pd":"rcp14ps"; break;
            case 0x4E: baseMnem = W?"rsqrt14pd":"rsqrt14ps"; break;
            case 0x58: baseMnem = "pbroadcastd"; break;
            case 0x59: baseMnem = "pbroadcastq"; break;
            case 0x78: baseMnem = "pbroadcastb"; break;
            case 0x79: baseMnem = "pbroadcastw"; break;
            case 0x88: baseMnem = "expandps"; break;
            case 0x89: baseMnem = "expandpd"; break;
            case 0x8A: baseMnem = "compressps"; break;
            case 0x8B: baseMnem = "compresspd"; break;
            case 0x90: baseMnem = "pgatherdd"; break;
            case 0x91: baseMnem = "pgatherqd"; break;
            case 0x92: baseMnem = "gatherdps"; break;
            case 0x93: baseMnem = "gatherqps"; break;
            case 0xA0: baseMnem = "pscatterdd"; break;
            case 0xA1: baseMnem = "pscatterqd"; break;
            case 0xA2: baseMnem = "scatterdps"; break;
            case 0xA3: baseMnem = "scatterqps"; break;
            // AVX-512 VBMI
            case 0x75: baseMnem = W?"permi2w":"permi2b"; break;
            case 0x7D: baseMnem = W?"permt2w":"permt2b"; break;
            case 0x83: baseMnem = "pmultishiftqb"; break;
            case 0x8D: baseMnem = W?"permw":"permb"; break;
            // AVX-512 IFMA
            case 0xB4: baseMnem = "pmadd52luq"; break;
            case 0xB5: baseMnem = "pmadd52huq"; break;
            case 0xC8: baseMnem = "exp2ps"; break;
            case 0xCA: baseMnem = "rcp28ps"; break;
            case 0xCC: baseMnem = "rsqrt28ps"; break;
            // GFNI (EVEX.66.0F38 CF)
            case 0xCF: baseMnem = "gf2p8mulb"; break;
            // VAES (EVEX.66.0F38 DC-DF)
            case 0xDC: baseMnem = "aesenc"; break;
            case 0xDD: baseMnem = "aesenclast"; break;
            case 0xDE: baseMnem = "aesdec"; break;
            case 0xDF: baseMnem = "aesdeclast"; break;
            default: baseMnem = NULL; break;
        }
    }
    else if(mm == 3){ // 0F 3A map
        switch(OpByte){
            case 0x00: baseMnem = "permq"; break;
            case 0x01: baseMnem = "permpd"; break;
            case 0x03: baseMnem = "alignd"; break;
            case 0x08: baseMnem = "roundps"; break;
            case 0x09: baseMnem = "roundpd"; break;
            case 0x0A: baseMnem = "roundss"; break;
            case 0x0B: baseMnem = "roundsd"; break;
            case 0x18: baseMnem = W?"insertf64x2":"insertf32x4"; break;  // AVX-512 DQ: W-dependent
            case 0x19: baseMnem = W?"extractf64x2":"extractf32x4"; break; // AVX-512 DQ: W-dependent
            case 0x1A: baseMnem = "insertf64x4"; break;
            case 0x1B: baseMnem = "extractf64x4"; break;
            case 0x1E: baseMnem = "pcmpud"; break;
            case 0x1F: baseMnem = "pcmpd"; break;
            case 0x25: baseMnem = "ternlogd"; break;
            case 0x26: baseMnem = "getmantps"; break;
            case 0x27: baseMnem = "getmantss"; break;
            case 0x38: baseMnem = W?"inserti64x2":"inserti32x4"; break;  // AVX-512 DQ: W-dependent
            case 0x39: baseMnem = W?"extracti64x2":"extracti32x4"; break; // AVX-512 DQ: W-dependent
            case 0x3A: baseMnem = "inserti64x4"; break;
            case 0x3B: baseMnem = "extracti64x4"; break;
            case 0x42: baseMnem = "dbpsadbw"; break;
            case 0x43: baseMnem = "shuffi32x4"; break;
            // VPCLMULQDQ (EVEX.66.0F3A 44)
            case 0x44: baseMnem = "pclmulqdq"; break;
            // AVX-512 DQ: VRANGE
            case 0x50: baseMnem = W?"rangepd":"rangeps"; break;
            case 0x51: baseMnem = W?"rangesd":"rangess"; break;
            case 0x54: baseMnem = "fixupimmps"; break;
            case 0x55: baseMnem = "fixupimmss"; break;
            // AVX-512 DQ: VFPCLASS
            case 0x66: baseMnem = W?"fpclasspd":"fpclassps"; break;
            case 0x67: baseMnem = W?"fpclasssd":"fpclassss"; break;
            // GFNI (EVEX.66.0F3A CE/CF)
            case 0xCE: baseMnem = "gf2p8affineqb"; break;
            case 0xCF: baseMnem = "gf2p8affineinvqb"; break;
            default: baseMnem = NULL; break;
        }
    }

    if(baseMnem == NULL){
        // Account for immediate byte on map 0F3A even for unknown opcodes
        if(mm == 3){
            wsprintf(temp," %02X",(BYTE)(*(*Opcode+pos+m_OpcodeSize)));
            lstrcat((*Disasm)->Opcode,temp);
            m_OpcodeSize++;
        }
        wsprintf(assembly,"??? (EVEX)");
        lstrcat((*Disasm)->Assembly,assembly);
        (*Disasm)->OpcodeSize = m_OpcodeSize;
        **index = pos + m_OpcodeSize - 1; // main loop does Index++
        return;
    }

    // Clear prefixes
    strcpy_s((*Disasm)->Assembly,"");

    // Build the AVX-512 mnemonic with "v" prefix
    char avxMnem[64];
    wsprintf(avxMnem,"v%s",baseMnem);

    // Format operands with opmask and zeroing decorators
    char destStr[64], srcStr[64], vvvvStr[64], maskStr[32]="";

    // Destination register
    wsprintf(destStr,"%s",dstRegs[REG_field & (LL==2 ? 31 : 15)]);

    // Opmask decorator
    if(aaa != 0){
        wsprintf(maskStr," {%s}",KRegs[aaa]);
        if(z){
            lstrcat(maskStr,"{z}");
        }
    }

    // VVVV register
    wsprintf(vvvvStr,"%s",dstRegs[VVVV_full & (LL==2 ? 31 : 15)]);

    // Source register or memory
    if(MOD == 0x03){
        wsprintf(srcStr,"%s",dstRegs[RM_field & (LL==2 ? 31 : 15)]);
    } else {
        wsprintf(srcStr,"%s",dstRegs[RM_field & 0x07]); // simplified for memory
    }

    // Compare instructions write to mask register (k0-k7), not vector register
    if(mm == 1 && (OpByte == 0x64 || OpByte == 0x65 || OpByte == 0x66 ||
                   OpByte == 0x74 || OpByte == 0x75 || OpByte == 0x76)){
        wsprintf(destStr,"%s",KRegs[REG_field & 7]);
    }

    // Shift-by-xmm instructions: source (shift count) is always XMM regardless of LL
    if(mm == 1 && (OpByte == 0xD1 || OpByte == 0xD2 || OpByte == 0xD3 ||
                   OpByte == 0xE1 || OpByte == 0xE2 ||
                   OpByte == 0xF1 || OpByte == 0xF2 || OpByte == 0xF3)){
        if(MOD == 0x03){
            wsprintf(srcStr,"%s",XMMRegs[RM_field & 15]);
        }
    }

    // AVX-512 BW/DQ: Truncation instructions (map 2, pp=2, opcodes 0x10-0x15, 0x20-0x25, 0x30-0x35)
    // Source is full-size REG, dest is reduced-size RM
    BOOL isTruncation = FALSE;
    if(mm == 2 && pp == 2 &&
       ((OpByte >= 0x10 && OpByte <= 0x15) ||
        (OpByte >= 0x20 && OpByte <= 0x25) ||
        (OpByte >= 0x30 && OpByte <= 0x35))){
        isTruncation = TRUE;
        wsprintf(srcStr,"%s",dstRegs[REG_field & (LL==2 ? 31 : 15)]);
        int lo = OpByte & 0x0F;
        const char **rr;
        if(lo == 0 || lo == 3 || lo == 5)
            rr = (LL==2)?YMMRegs:XMMRegs;  // half-size
        else
            rr = XMMRegs;  // quarter or eighth
        if(MOD == 0x03)
            wsprintf(destStr,"%s",rr[RM_field & 15]);
    }

    // VPMOVM2B/W/D/Q: dest=vector(REG), src=mask(RM)
    if(mm == 2 && pp == 2 && (OpByte == 0x28 || OpByte == 0x38)){
        wsprintf(srcStr,"%s",KRegs[RM_field & 7]);
    }

    // VPMOVB2M/W2M/D2M/Q2M: dest=mask(REG), src=vector(RM)
    if(mm == 2 && pp == 2 && (OpByte == 0x29 || OpByte == 0x39)){
        wsprintf(destStr,"%s",KRegs[REG_field & 7]);
    }

    // VFPCLASS: k dest register
    if(mm == 3 && (OpByte == 0x66 || OpByte == 0x67)){
        wsprintf(destStr,"%s",KRegs[REG_field & 7]);
    }

    // Broadcast decorator
    char bcastStr[16]="";
    if(b_bit && MOD != 0x03){
        if(W){
            switch(LL){
                case 0: strcpy_s(bcastStr,"{1to2}"); break;
                case 1: strcpy_s(bcastStr,"{1to4}"); break;
                case 2: strcpy_s(bcastStr,"{1to8}"); break;
            }
        } else {
            switch(LL){
                case 0: strcpy_s(bcastStr,"{1to4}"); break;
                case 1: strcpy_s(bcastStr,"{1to8}"); break;
                case 2: strcpy_s(bcastStr,"{1to16}"); break;
            }
        }
    }

    // Some instructions use only 2 operands (dest, src) — no VVVV register
    BOOL twoOperand = FALSE;
    if(mm == 1 && (OpByte == 0x70 ||  // VPSHUFD/VPSHUFHW/VPSHUFLW
                   OpByte == 0x6E || OpByte == 0x7E)){  // VMOVD/VMOVQ
        twoOperand = TRUE;
    }
    if(isTruncation) twoOperand = TRUE;
    // VPMOVM2B/W/D/Q and VPMOVB2M/W2M/D2M/Q2M
    if(mm == 2 && pp == 2 && (OpByte == 0x28 || OpByte == 0x29 || OpByte == 0x38 || OpByte == 0x39))
        twoOperand = TRUE;
    // VFPCLASS (k dest + imm8, 2-operand)
    if(mm == 3 && (OpByte == 0x66 || OpByte == 0x67))
        twoOperand = TRUE;

    // Build full assembly string
    if(twoOperand){
        wsprintf(assembly,"%s %s%s, %s%s",avxMnem,destStr,maskStr,srcStr,bcastStr);
    } else {
        wsprintf(assembly,"%s %s%s, %s, %s%s",avxMnem,destStr,maskStr,vvvvStr,srcStr,bcastStr);
    }

    // Handle displacement
    if(MOD == 0x00){
        if((RM_field&0x07) == 0x05){ m_OpcodeSize+=4; }
        else if((RM_field&0x07) == 0x04){ m_OpcodeSize++; }
    }
    else if(MOD == 0x01){ m_OpcodeSize++;
        if((RM_field&0x07) == 0x04){ m_OpcodeSize++; }
    }
    else if(MOD == 0x02){ m_OpcodeSize+=4;
        if((RM_field&0x07) == 0x04){ m_OpcodeSize++; }
    }

    // Handle immediate byte for 0F3A map and specific 0F map instructions
    if(mm == 3 || OpByte == 0xC2 || OpByte == 0xC6 || OpByte == 0x70){
        BYTE imm = (BYTE)(*(*Opcode+pos+m_OpcodeSize));
        char immStr[16];
        wsprintf(immStr,", %02Xh",imm);
        lstrcat(assembly,immStr);
        wsprintf(temp," %02X",imm);
        lstrcat((*Disasm)->Opcode,temp);
        m_OpcodeSize++;
    }

    **index = pos + m_OpcodeSize - 1; // main loop does Index++
    lstrcat((*Disasm)->Assembly,assembly);
    (*Disasm)->OpcodeSize = m_OpcodeSize;
    (*Disasm)->IsEVEX = 1;
    (*Disasm)->EvexAAA = aaa;
    (*Disasm)->EvexZ = z;
    (*Disasm)->EvexB = b_bit;
    (*Disasm)->EvexLL = LL;
    (*Disasm)->VexVVVV = VVVV_full;
    (*Disasm)->VexW = W;
    (*Disasm)->VexPP = pp;
    (*Disasm)->VexMMMM = mm;
}