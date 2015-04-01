/*
     8888888b.                  888     888 d8b
     888   Y88b                 888     888 Y8P
     888    888                 888     888
     888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888
     8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888
     888        888     888  888  Y88o88P   888 88888888 888  888  888
     888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P
     888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"


             PE Editor & Dissasembler & File Identifier
             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   
 Written by Shany Golan.
 In januar, 2003.
 I have investigated P.E. file format as thoroughly as possible,
 But I cannot claim that I am an expert yet, so some of its information  
 May give you wrong results.

 Language used: Visual C++ 6.0
 Date of creation: July 06, 2002

 Date of first release: unknown ??, 2003

 You can contact me: e-mail address: Shanytc@yahoo.com

 Copyright (C) 2011. By Shany Golan.

 Permission is granted to make and distribute verbatim copies of this
 Program provided the copyright notice and this permission notice are
 Preserved on all copies.


 File: CDisasmData.h (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#ifndef _C_DISASM_DATA_H_
#define _C_DISASM_DATA_H_

// ================================================================
// =========================  INCLUDE  ============================
// ================================================================
#include <string> // changed from <string.h>
#include <WINDOWS.H>

// ================================================================
// ===========================  CLASS  ============================
// ================================================================

class CDisasmData {

	private:        
        DWORD m_InstructionSize;
        DWORD m_PrefixSize;
		char* m_pcAddress;
		char* m_pcCode;
		char* m_pcMnemonic;
		char* m_pcComments;  
		char* m_pcReference;

	public:
		
		CDisasmData  (										); // Default constructor       
        CDisasmData  ( const CDisasmData &obj				); // Copy Constructor		
		CDisasmData  ( char* pcAddress, char* pcCode, 
			           char* pcMnemonic, char* pcComments, 
					   DWORD dwSize,DWORD dwPrefSize,char* pcRef ); // Advanced constructor                       	
		~CDisasmData (                                    ); // Destructor
		
		// Set Methods
		void  SetAddress   ( char* pcAddress	); // Set the address of the disassembled line        
        void  SetCode      ( char* pcCode		); // Set the Opcode of the disassembled line        
        void  SetMnemonic  ( char* pcMnemonic	); // Set the Assembly of the disassembled line        
        void  SetComments  ( char* pcComment	); // Set the Comment of the disassembled line
		void  SetReference ( char* pcRef		); // Set the Reference for the disassembled line
		void  SetSize      ( DWORD dwSize		); // Set the Instruction Size
        void  SetPrefixSize( DWORD dwPrefSize	); // Set the Prefixes Size        

		// Get Methods
		char* GetAddress   (					); // Get the address of the disassembled line        
        char* GetCode      (					); // Get the Opcode of the disassembled line        
        char* GetMnemonic  (					); // Get the Assembly of the disassembled line        
        char* GetComments  (					); // Get the Comment of the disassembled line
        char* GetReference (					); // Get The reference of the disassembled line
		DWORD GetSize      (					); // Get the Instruction Size
        DWORD GetPrefixSize(					); // Get the Prefixes Size

};


// ================================================================
// ===========================  CLASS  ============================
// ================================================================

class CDisasmColor{
    
private:    
    // Address Colors
    COLORREF m_AddressTextColor;
    COLORREF m_AddressBgTextColor;
    // Opcode Colors
    COLORREF m_OpcodesTextColor;
    COLORREF m_OpcodesBgTextColor;
    // Assembly Colors
    COLORREF m_AssemblyTextColor;
    COLORREF m_AssemblyBgTextColor;
    // Comments Colors
    COLORREF m_CommentsTextColor;
    COLORREF m_CommentsBgTextColor;
    // Other Colors
    COLORREF m_BackGround;
    COLORREF m_CallApiTextColor;
    COLORREF m_CallTextColor;
    COLORREF m_JumpTextColor;
    
public:
    // Default Constructor
    CDisasmColor();
    // Overloaded Constructor
    CDisasmColor(
                 COLORREF Address,COLORREF AddressBg,
                 COLORREF Opcode,COLORREF OpcodeBg,
                 COLORREF Asm,COLORREF AsmBg,
                 COLORREF Comment,COLORREF CommentBg,
                 COLORREF ApiColor,COLORREF CallColor,
                 COLORREF JumpColor,COLORREF BackGround
                );
    
    ~CDisasmColor			(								); // Destructor
    CDisasmColor			( const CDisasmColor& CopyObj	); // Copy Constructor
    CDisasmColor& operator=	( const CDisasmColor& ColorObj	); // Operator (Copy Constructor)
    
    void		SetAddressTextColor				( COLORREF AddrText		);
    void		SetAddressBackGroundTextColor	( COLORREF AddrBgText	);
    void		SetOpcodesTextColor				( COLORREF OpText		);
    void		SetOpcodesBackGroundTextColor	( COLORREF OpBgText		);
    void		SetAssemblyTextColor			( COLORREF AsmText		);
    void		SetAssemblyBackGroundTextColor	( COLORREF AsmBgText	);
    void		SetCommentsTextColor			( COLORREF CommentText	);
    void		SetCommentsBackGroundTextColor	( COLORREF CommentBgText);
    void		SetResolvedApiTextColor			( COLORREF ApiColor		);
    void		SetCallAddressTextColor			( COLORREF CallColor	);
    void		SetJumpAddressTextColor			( COLORREF JumpColor	);   
    void		SetBackGroundColor				( COLORREF Background	);
    COLORREF	GetAddressTextColor				(						) const;
    COLORREF	GetAddressBackGroundTextColor	(						) const;    
    COLORREF	GetOpcodesTextColor				(						) const;
    COLORREF	GetOpcodesBackGroundTextColor	(						) const;    
    COLORREF	GetAssemblyTextColor			(						) const;
    COLORREF	GetAssemblyBackGroundTextColor	(						) const;    
    COLORREF	GetCommentsTextColor			(						) const;
    COLORREF	GetCommentsBackGroundTextColor	(						) const;    
    COLORREF	GetResolvedApiTextColor			(						) const;
    COLORREF	GetCallAddressTextColor			(						) const;
    COLORREF	GetJumpAddressTextColor			(						) const;
    COLORREF	GetBackGroundColor				(						) const;
};

#endif
