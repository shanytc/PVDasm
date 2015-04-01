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

 File: CDisasmColor.cpp (main)
   This program was written by Shany Golan, Student at :
				Ruppin, department of computer science and engineering University.
*/

#include "CDisasmData.h"

CDisasmColor::CDisasmColor()
{
	// Set Default Colors
	m_AddressTextColor=RGB(0,0,0);
	m_AddressBgTextColor=RGB(255,251,240);
	// Opcode Colors
	m_OpcodesTextColor=RGB(0,0,0);
	m_OpcodesBgTextColor=RGB(255,251,240);
	// Assembly Colors
	m_AssemblyTextColor=RGB(0,0,0);
	m_AssemblyBgTextColor=RGB(255,251,240);
	// Comments Colors
	m_CommentsTextColor=RGB(0,0,0);
	m_CommentsBgTextColor=RGB(255,251,240);
	// Other Colors
	m_BackGround = RGB(255,251,240);
	m_CallApiTextColor=RGB(0,0,0);
	m_CallTextColor=RGB(0,0,0);
	m_JumpTextColor=RGB(0,0,0);
}

// Overloaded Constructor
CDisasmColor::CDisasmColor(
				COLORREF m_Address,COLORREF m_AddressBg,
				COLORREF m_Opcode,COLORREF m_OpcodeBg,
				COLORREF m_Asm,COLORREF m_AsmBg,
				COLORREF m_Comment,COLORREF m_CommentBg,
				COLORREF m_ApiColor,COLORREF m_CallColor,
				COLORREF m_JumpColor,COLORREF BackGround
			)
{
	// Address Colors
	m_AddressTextColor=m_Address;
	m_AddressBgTextColor=m_AddressBg;
	// Opcode Colors
	m_OpcodesTextColor=m_Opcode;
	m_OpcodesBgTextColor=m_OpcodeBg;
	// Assembly Colors
	m_AssemblyTextColor=m_Asm;
	m_AssemblyBgTextColor=m_AsmBg;
	// Comments Colors
	m_CommentsTextColor=m_Comment;
	m_CommentsBgTextColor=m_CommentBg;
	// Other Colors
	m_BackGround = BackGround;
	m_CallApiTextColor=m_ApiColor;
	m_CallTextColor=m_CallColor;
	m_JumpTextColor=m_JumpColor;    
}

// Destructor
CDisasmColor::~CDisasmColor(){
}

// Copy Constructor
CDisasmColor::CDisasmColor(const CDisasmColor& CopyObj)
{
	// Set Default Colors
	this->m_AddressTextColor=CopyObj.m_AddressTextColor;
	this->m_AddressBgTextColor=CopyObj.m_AddressBgTextColor;
	// Opcode Colors
	this->m_OpcodesTextColor=CopyObj.m_OpcodesTextColor;
	this->m_OpcodesBgTextColor=CopyObj.m_OpcodesBgTextColor;
	// Assembly Colors
	this->m_AssemblyTextColor=CopyObj.m_AssemblyTextColor;
	this->m_AssemblyBgTextColor=CopyObj.m_AssemblyBgTextColor;
	// Comments Colors
	this->m_CommentsTextColor=CopyObj.m_CommentsTextColor;
	this->m_CommentsBgTextColor=CopyObj.m_CommentsBgTextColor;
	// Other Colors
	this->m_CallApiTextColor=CopyObj.m_CallApiTextColor;
	this->m_CallTextColor=CopyObj.m_CallTextColor;
	this->m_JumpTextColor=CopyObj.m_JumpTextColor;
	this->m_BackGround = CopyObj.m_BackGround;
}

CDisasmColor& CDisasmColor::operator = (const CDisasmColor& ColorObj)
{	
	// Set Default Colors
	m_AddressTextColor=ColorObj.m_AddressTextColor;
	m_AddressBgTextColor=ColorObj.m_AddressBgTextColor;
	// Opcode Colors
	m_OpcodesTextColor=ColorObj.m_OpcodesTextColor;
	m_OpcodesBgTextColor=ColorObj.m_OpcodesBgTextColor;
	// Assembly Colors
	m_AssemblyTextColor=ColorObj.m_AssemblyTextColor;
	m_AssemblyBgTextColor=ColorObj.m_AssemblyBgTextColor;
	// Comments Colors
	m_CommentsTextColor=ColorObj.m_CommentsTextColor;
	m_CommentsBgTextColor=ColorObj.m_CommentsBgTextColor;
	// Other Colors
	m_CallApiTextColor=ColorObj.m_CallApiTextColor;
	m_CallTextColor=ColorObj.m_CallTextColor;
	m_JumpTextColor=ColorObj.m_JumpTextColor;
	m_BackGround=ColorObj.m_BackGround;

	return *this;
}

void CDisasmColor::SetAddressTextColor(COLORREF AddrTextColor)
{
	m_AddressTextColor=AddrTextColor;  // Set Color for Address
}

void CDisasmColor::SetAddressBackGroundTextColor(COLORREF AddrBgTextColor)
{
    m_AddressBgTextColor=AddrBgTextColor;
}

void CDisasmColor::SetOpcodesTextColor(COLORREF OpcodeTextColor)
{
	m_OpcodesTextColor = OpcodeTextColor;
}

void CDisasmColor::SetOpcodesBackGroundTextColor(COLORREF OpcodeBgTextColor)
{
	m_OpcodesBgTextColor = OpcodeBgTextColor;
}

void CDisasmColor::SetAssemblyTextColor(COLORREF AssemblyTextColor)
{
	m_AssemblyTextColor = AssemblyTextColor;
}

void CDisasmColor::SetAssemblyBackGroundTextColor(COLORREF AssemblyBgTextColor)
{
	m_AssemblyBgTextColor = AssemblyBgTextColor;
}

void CDisasmColor::SetCommentsTextColor(COLORREF CommentTextColor)
{
	m_CommentsTextColor = CommentTextColor;
}

void CDisasmColor::SetCommentsBackGroundTextColor(COLORREF CommentBgTextColor)
{
	m_CommentsBgTextColor = CommentBgTextColor;
}

void CDisasmColor::SetResolvedApiTextColor(COLORREF ApiColorText)
{
	m_CallApiTextColor = ApiColorText;
}

void CDisasmColor::SetCallAddressTextColor(COLORREF CallColorText)
{
	m_CallTextColor = CallColorText;
}

void CDisasmColor::SetJumpAddressTextColor(COLORREF JumpColorText)
{
	m_JumpTextColor = JumpColorText;
}

void CDisasmColor::SetBackGroundColor(COLORREF Background)
{
	m_BackGround = Background;
}

COLORREF CDisasmColor::GetAddressTextColor() const
{
	return m_AddressTextColor;
}

COLORREF CDisasmColor::GetAddressBackGroundTextColor() const
{
	return m_AddressBgTextColor;
}

COLORREF CDisasmColor::GetOpcodesTextColor() const
{
	return m_OpcodesTextColor;
}

COLORREF CDisasmColor::GetOpcodesBackGroundTextColor() const
{
	return m_OpcodesBgTextColor;
}

COLORREF CDisasmColor::GetAssemblyTextColor() const
{
	return m_AssemblyTextColor;
}

COLORREF CDisasmColor::GetAssemblyBackGroundTextColor() const
{
	return m_AssemblyBgTextColor;
}

COLORREF CDisasmColor::GetCommentsTextColor() const
{
	return m_CommentsTextColor;
}

COLORREF CDisasmColor::GetCommentsBackGroundTextColor() const
{
	return m_CommentsBgTextColor;
}

COLORREF CDisasmColor::GetResolvedApiTextColor() const
{
	return m_CallApiTextColor;
}

COLORREF CDisasmColor::GetCallAddressTextColor() const
{
	return m_CallTextColor;
}

COLORREF CDisasmColor::GetJumpAddressTextColor() const
{
	return m_JumpTextColor;
}

COLORREF CDisasmColor::GetBackGroundColor() const
{
	return m_BackGround;
}