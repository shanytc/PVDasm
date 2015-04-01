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


 File: CDisasmData.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/

//////////////////////////////////////////////////////////////////////////
//							INCLUDES									//
//////////////////////////////////////////////////////////////////////////
#include "CDisasmData.h"
#include "functions.h"

//////////////////////////////////////////////////////////////////////////
//							FUNCTIONS									//
//////////////////////////////////////////////////////////////////////////

// Default constructor
CDisasmData::CDisasmData()
{
	m_pcAddress=NULL;
	m_pcCode=NULL;
	m_pcMnemonic=NULL;
	m_pcComments=NULL;
	m_pcReference=NULL;
	m_InstructionSize=0;
	m_PrefixSize=0;
}

//Copy Constructor
CDisasmData::CDisasmData(const CDisasmData &obj)
{
	this->m_pcAddress=_strdup(obj.m_pcAddress);
	this->m_pcCode=_strdup(obj.m_pcCode);
	this->m_pcMnemonic=_strdup(obj.m_pcMnemonic);
	this->m_pcComments=_strdup(obj.m_pcComments);
	this->m_pcReference=_strdup(obj.m_pcReference);
	this->m_InstructionSize=obj.m_InstructionSize;
	this->m_PrefixSize = obj.m_PrefixSize;
}

// Advanced constructor
CDisasmData::CDisasmData(char* pcAddress, char* pcCode, char* pcMnemonic, char* pcComments, DWORD dwSize,DWORD dwPrefSize,char* pcRef)
{
	m_pcAddress = _strdup(pcAddress);
	m_pcCode = _strdup(pcCode);
	m_pcMnemonic = _strdup(pcMnemonic);
	m_pcComments = _strdup(pcComments);
	m_pcReference = _strdup(pcRef);
	m_InstructionSize = dwSize;
	m_PrefixSize = dwPrefSize;
}

// Destructor
CDisasmData::~CDisasmData()
{
	if(m_pcAddress!=NULL)	delete[] m_pcAddress;
	if(m_pcCode!=NULL)		delete[] m_pcCode;
	if(m_pcMnemonic!=NULL)	delete[] m_pcMnemonic;
	if(m_pcComments!=NULL)	delete[] m_pcComments;
	if(m_pcReference!=NULL)	delete[] m_pcReference;
}

// Set the address of the disassembled line
void CDisasmData::SetAddress(char* pcAddress)
{
	if(m_pcAddress!=NULL)
		delete m_pcAddress;

	m_pcAddress = _strdup(pcAddress);
}

void CDisasmData::SetCode(char* pcCode)
{
	if(m_pcCode!=NULL)
		delete m_pcCode;

	m_pcCode =_strdup(pcCode);
}

void CDisasmData::SetComments(char* pcComment)
{
	if(m_pcComments!=NULL)
		delete m_pcComments;

	m_pcComments = _strdup(pcComment);
}

void CDisasmData::SetMnemonic(char* pcMnemonic)
{
	if(m_pcMnemonic!=NULL)
		delete m_pcMnemonic;

	m_pcMnemonic = _strdup(pcMnemonic);
}

void CDisasmData::SetReference(char* pcRef)
{
	if(m_pcReference!=NULL)
		delete m_pcReference;

	m_pcReference = _strdup(pcRef);
}

void CDisasmData::SetSize(DWORD dwSize)
{
	m_InstructionSize = dwSize;
}

void CDisasmData::SetPrefixSize(DWORD dwPrefSize)
{
	m_PrefixSize = dwPrefSize;
}

// Get the address of the disassembled line
char* CDisasmData::GetAddress()
{
	return m_pcAddress;
}

char* CDisasmData::GetCode()
{
	return m_pcCode;
}

char* CDisasmData::GetComments()
{
	return m_pcComments;
}

char* CDisasmData::GetMnemonic()
{
	return m_pcMnemonic;
}

DWORD CDisasmData::GetSize()
{
	return m_InstructionSize;
}

DWORD CDisasmData::GetPrefixSize()
{
	return m_PrefixSize;
}

char* CDisasmData::GetReference()
{
	return m_pcReference;
}