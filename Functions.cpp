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

 File: Functions.cpp (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

//======================================================================================
//====================================  INCLUDES =======================================
//======================================================================================

#include <windows.h>
#include "Functions.h"
#include "resource\resource.h"
#include <commctrl.h>
#include <fstream>
#include <uxtheme.h>
#pragma comment(lib, "uxtheme.lib")

#ifndef __EXTERN_DATA__
#define __EXTERN_DATA__

#include "CDisasmData.h"
#include "Disasm.h"
// STL
#include <vector>
#include <map>

using namespace std; // Use Microsoft STL Templates

//======================================================================================
//=================================  TYPE DEFINES ======================================
//======================================================================================

typedef vector<CDisasmData>				DisasmDataArray;
typedef vector<CODE_BRANCH>				CodeBranch;
typedef vector<BRANCH_DATA>				BranchData;  
typedef vector<int>						DisasmImportsArray,IntArray;
typedef multimap<const int, int>		XRef,MapTree;
typedef pair<const int const, int>		XrefAdd;
typedef XRef::iterator					ItrXref;
typedef vector<FUNCTION_INFORMATION>	FunctionInfo;
typedef MapTree::iterator				TreeMapItr;

//======================================================================================
//============================ EXTERNAL VARIABLES ======================================
//======================================================================================

extern DisasmDataArray		DisasmDataLines;
extern CDisasmColor			DisasmColors;
extern CodeBranch			DisasmCodeFlow;
extern BranchData			BranchTrace;
extern DisasmImportsArray	ImportsLines;
extern DisasmImportsArray	StrRefLines;
extern XRef					XrefData;
extern DWORD_PTR			iSelected;
extern DWORD_PTR			iSelectItem,hFileSize;
extern HWND					Main_hWnd;
extern HWND					hWndTB;
extern int					DisasmIndex;
extern bool					DCodeFlow;
extern bool					FilesInMemory,DisassemblerReady,LoadedPe;
extern bool					LoadedPe64;
extern HMODULE				hRadHex;
extern char					szFileName[MAX_PATH];
extern HANDLE				hFile,hFileMap,hDisasmThread;
extern char					*OrignalData;
extern IMAGE_DOS_HEADER		*doshdr;
extern IMAGE_NT_HEADERS		*nt_hdr;
extern IMAGE_SECTION_HEADER	*SectionHeader;
extern FunctionInfo			fFunctionInfo;
extern MapTree				DataAddersses;
#endif

//////////////////////////////////////////////////////////////////////////
//							DEFINES										//
//////////////////////////////////////////////////////////////////////////

#define USE_MMX
#define AddNew(key,data) XrefData.insert(XrefAdd(key,data));

#ifdef USE_MMX
	#ifndef _M_X64
		#define StringLen(str) xlstrlen(str) // new lstrlen with mmx function
	#else
		#define StringLen(str) lstrlen(str) // old c library strlen
	#endif
#else
    #define StringLen(str) lstrlen(str) // old c library strlen
#endif

// ============================================================================================
// ======================== Convert Relative Virtual Address to Raw Offset ============================
// ============================================================================================
PIMAGE_SECTION_HEADER RVAToOffset(IMAGE_NT_HEADERS *nt,IMAGE_SECTION_HEADER *sec,DWORD_PTR RVA,bool *Err)
{
   /*************************************************************************** 

   Function: 
   ---------
   "RVAToOffset"

   Parameters:
   -----------
   1. Pointer to a NT_HEADER struct
   2. Pointer to an SECTION_HEADER struct
   3. DWORD RVA
	   
   Return Values:
   --------------
   1. The function returns an Pointer to
   an SECTION_HEADER struct filled with the
   Section we found to be valid.
   else the function will return 0 if failed.

   Usage:
   ------
   This Functions gets an RVA ,
   And convert it to OFFSET.
   This means that we search for an Section
   At the EXE (PE Header) that the (RVA >= && RVA<)
   Is between 2 sections.
   If found, we reeturn the offset of the Section!

   The function returns a DWORD in type of PIMAGE_SECTION_HEADER

   פונקציה זו מוצאת באיזה איזור בראש הקובץ
   RVA נמצא.
   החיפוש נעשה ע"י בדיקה אם ה אר-וי-איי נמצא באותו
   התחום של האיזור אותו אנו סורקים.
   התחום תריך להיות בין שני איזורים
   הפונקציה תחזיר מצביע לתחילת האיזור כאשר נמצא.

   ***************************************************************************/

	// Get first Section (address to first address)
	DWORD NumOfSections=nt->FileHeader.NumberOfSections,i=0;
	sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));

	// Process and scan all sections
	for(;i<NumOfSections;i++){
       //if(sec->VirtualAddress!=0) 
		// check if the RVA is located at the section we search in
		if ( ((DWORD)RVA >= sec->VirtualAddress) && 
             ((DWORD)RVA < (sec->VirtualAddress + sec->Misc.VirtualSize)))
        {
			*Err=false;
			return sec; // return the section where imports at                
        }
		// read/scan next section
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(sec)+sizeof(IMAGE_SECTION_HEADER));
	}
	// no section found, return 0
	
    *Err=true;
	return (IMAGE_SECTION_HEADER *)((UCHAR *)(sec)-sizeof(IMAGE_SECTION_HEADER)); // returns last section on error.
}

// ============================================================================================
// ======================== Convert Relative Virtual Address to Raw Offset ============================
// ============================================================================================
PIMAGE_SECTION_HEADER RVAToOffset64(IMAGE_NT_HEADERS64 *nt,IMAGE_SECTION_HEADER *sec,DWORD_PTR RVA,bool *Err)
{
   /*************************************************************************** 

   Function: 
   ---------
   "RVAToOffset"

   Parameters:
   -----------
   1. Pointer to a NT_HEADER struct
   2. Pointer to an SECTION_HEADER struct
   3. DWORD RVA
	   
   Return Values:
   --------------
   1. The function returns an Pointer to
   an SECTION_HEADER struct filled with the
   Section we found to be valid.
   else the function will return 0 if failed.

   Usage:
   ------
   This Functions gets an RVA ,
   And covnert it to OFFSET.
   This means that we search for an Section
   At the EXE (PE Header) that the (RVA >= && RVA<)
   Is between 2 sections.
   If found, we reeturn the offset of the Section!

   The function returns a DWORD in type of PIMAGE_SECTION_HEADER

   פונקציה זו מוצאת באיזה איזור בראש הקובץ
   RVA נמצא.
   החיפוש נעשה ע"י בדיקה אם ה אר-וי-איי נמצא באותו
   התחום של האיזור אותו אנו סורקים.
   התחום תריך להיות בין שני איזורים
   הפונקציה תחזיר מצביע לתחילת האיזור כאשר נמצא.

   ***************************************************************************/

	// Get first Section (address to first address)
	DWORD NumOfSections=nt->FileHeader.NumberOfSections,i=0;
	sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));

	// Process and scan all sections
	for(;i<NumOfSections;i++)
	{
       //if(sec->VirtualAddress!=0) 
		// check if the RVA is located at the section we search in
		if ( (RVA >= sec->VirtualAddress) && 
             (RVA < (sec->VirtualAddress + sec->Misc.VirtualSize)))
        {
			*Err=false;
			return sec; // return the section where imports at                
        }
		// read/scan next section
		sec = (IMAGE_SECTION_HEADER *)((UCHAR *)(sec)+sizeof(IMAGE_SECTION_HEADER));
	}
	// no section found, return 0
	
    *Err=true;
	return (IMAGE_SECTION_HEADER *)((UCHAR *)(sec)-sizeof(IMAGE_SECTION_HEADER)); // returns last section on error.
}


// ========================================
// ====== Convert Hex String to __int64 =====
// ========================================
__int64 StringToQword(char *Text)
{
	__int64 qdword;
	sscanf_s(( LPCTSTR )Text, "%I64X", &qdword );
	return qdword;
}

// ========================================
// ====== Convert Hex String to DWORD =====
// ========================================
DWORD StringToDword(char *Text)
{
	/***************************************************************************

	Function Name:
	-------------
	"StringToDword"

	Parameters: 
	-----------
	1. Pointer to a char/string

	Return values: 
	--------------
	1. DWORD number

	Usage:
	------
	this function will convert,
	an Hexadecimal String into a,
	DWORD hex number using assembly inline directive.

	פונקציה זו תמיר טקסט המכיל
	אותיות הקסדצימליות למספר הקסדצימלי אמיתי
	!מסוג מילה כפולה

    ***************************************************************************/
	
	DWORD DwordNum=0;
	#ifndef _M_X64
		// 32 Bit
		#ifdef USE_MMX    
	    
		_asm align 8
		__int64 mxc_30		= 0x3030303030303030;
		__int64 mxc_09		= 0x0909090909090909;
		__int64 mxc_07		= 0x0707070707070707;
		__int64 mxc_0F00	= 0x0f000f000f000f00;
		__int64 mxc_000F	= 0x000f000f000f000f;

		_asm{
				push esi
				push eax
				mov esi,Text
				movq mm0,QWORD PTR [esi]
				psubusb  mm0,mxc_30
				movq mm1,mm0
				pcmpgtb mm1,mxc_09
				pand mm1,mxc_07
				psubusb mm0,mm1
				movq mm1,mm0
				pand mm0,mxc_0F00
				pand mm1,mxc_000F
				psrlq mm0,8
				packuswb mm1,mm1
				packuswb mm0,mm0
				psllq mm1,4
				por mm0,mm1
				movd eax,mm0
				EMMS
				bswap eax
				mov DwordNum,eax
				pop eax
				pop esi
		 }

		#else // backward compability - slower algorithm

		char table[79]={
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
		};

		_asm{
				PUSH ECX			//
				PUSH EAX			// Save registers
				PUSH EDI			//
				PUSH ESI			//
				PUSH EDI
				PUSH EBX
				PUSH EDX
				XOR EAX,EAX			// Clear Register
				MOV EDI,2*4H		// Times to decode: 8 - dword
				MOV ESI,Text		// ESI Points to the string
				LEA EBX,[table]

		 _start:
				MOVZX ECX,[ESI] // CL has the ascii value
				MOVZX EDX,[EBX+ECX]
				ADD EAX,EDX		// EAX+=ECX
				CMP EDI,1		// First time?
				JZ _out			// yes, do not shift.
				LEA EAX,[EAX*8]	//
				SHL EAX,1
				//ADD EAX,EAX	// SHL EAX,04h	
		_out:
				ADD ESI,1		// Next char
				SUB EDI,1		// index++
				JNZ _start		// finished ?
				MOV DwordNum,EAX // yes, return the value
				POP EDX
				POP EBX
				POP EDI
				POP ESI
				POP EDI
				POP EAX
				POP ECX
		 }

		#endif
	#else
		// 64 Bit
		char table[79]={
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
		};
		register DWORD length, i, value, shift;
		length=8;
		shift = (length - 1) * 4;
		for (i = value = 0; i <= length; i++, shift -= 4){
			value += table[(DWORD)Text[i] & 127] << shift;
		}
		DwordNum=value;
	#endif
	return DwordNum;
}

// =======================================
// ====== Convert Hex String to WORD =====
// =======================================
WORD StringToWord(char *Text)
{
	/***************************************************************************

	Fcuntion: 
	---------
	"StringToWord"

	Parameters: 
	-----------
	1. Pointer to a char/string
		
	Return values: 
	--------------
	1. WORD number

	Usage:
	------
	this function will convert and return
	an Hexadecimel String into a real
	WORD hex number using assembly directive.

	פונקציה זו תמיר טקסט המכיל
    אותיות הקסדצימליות למספר הקסדצימלי אמיתי
    !מסוג מילה

    ***************************************************************************/
	WORD WordNum=0;

	#ifndef _M_X64
		
		#ifdef USE_MMX

		_asm align 4
		DWORD mxc_30	= 0x30303030;
		DWORD mxc_09	= 0x09090909;
		DWORD mxc_07	= 0x07070707;
		DWORD mxc_0F00	= 0x0f000f00;
		DWORD mxc_000F	= 0x000f000f;

		_asm{
				push esi
				push eax
				mov esi,Text
				movq mm0,DWORD PTR [esi]
				psubusb  mm0,mxc_30
				movq mm1,mm0 
				pcmpgtb mm1,mxc_09 
				pand mm1,mxc_07
				psubusb mm0,mm1
				movq mm1,mm0
				pand mm0,mxc_0F00
				pand mm1,mxc_000F
				psrlq mm0,8
				packuswb mm1,mm1
				packuswb mm0,mm0
				psllq mm1,4
				por mm0,mm1
				movd eax,mm0
				EMMS
				bswap eax
				shr eax,16
				mov WordNum,ax
				pop eax
				pop esi
		 }

		#else // backward compability - slower algorithm
		
		char table[79]={
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x00,0x00,0x00,0x00,0x00,
		 0x00,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
		};	

		_asm{
				PUSH ECX		//
				PUSH EAX		// Save registers
				PUSH EDI		//
				PUSH ESI		//
				PUSH EDI
				PUSH EBX
				PUSH EDX
				XOR EAX,EAX		// Clear Register
				MOV EDI,4H		// Times to decode: 4 - dword
				MOV ESI,Text	// ESI Points to the string
				LEA EBX,[table]

		 _start:
				MOVZX ECX,[ESI] // CL has the ascii value
				MOVZX EDX,[EBX+ECX]
				ADD EAX,EDX		// EAX+=ECX
				CMP EDI,1		// First time?
				JZ _out			// yes, do not shift.
				LEA EAX,[EAX*8]	//
				SHL EAX,1
				//ADD EAX,EAX	// SHL EAX,04h	
		_out:
				ADD ESI,1		// Next char
				SUB EDI,1		// index++
				JNZ _start		// finished ?
				MOV WordNum,AX // yes, return the value
				POP EDX
				POP EBX
				POP EDI
				POP ESI
				POP EDI
				POP EAX
				POP ECX
		 }
		#endif	
	#else
		// 64 Bit
		char table[79] = {
		  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x00,0x00,0x00,0x00,0x00,0x00,
		  0x00,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
		};
		register DWORD length, i, value, shift;
		length = 4;
		shift = (length - 1) * 4;
		for (i = value = 0; i < length; i++, shift -= 4){
			value += table[(DWORD)Text[i] & 127] << shift;
		}
		WordNum = (WORD)value;
	#endif
	return WordNum;
}

//===========================
//========= XLSTRLEN ========
//===========================
int xlstrlen(const char* string)
{
	DWORD len=0;
	#ifndef _M_X64
		// 32 Bit
		const unsigned __int64 STRINGTBL[8] = {0, 0xff,
			0xffff, 0xffffff, 0xffffffff, 0xffffffffff,
			0xffffffffffff, 0xffffffffffffff};

		_asm{
			push eax
			push ecx
			push edx
			mov eax,string
			pxor	mm1, mm1
			mov		ecx, eax
			mov		edx, eax
			and		ecx, -8
			and		eax, 7
			movq	mm0, [ecx]
			por	mm0, [STRINGTBL+eax*8]
		MAIN:
			add			ecx, 8
			pcmpeqb		mm0, mm1
			packsswb	mm0, mm0
			movd		eax, mm0
			movq		mm0, [ecx]
			test		eax, eax
			jz			MAIN
			EMMS
			bsf			eax, eax
			shr			eax, 2
			lea			ecx, [ecx+eax-8]
			sub			ecx, edx
			mov len,ecx
			pop edx
			pop ecx
			pop eax
		}
	#else
		// 64 bit
		len=lstrlen(string);
	#endif
	return len;
}

// =======================================
// ============= OutDebug ================
// =======================================
void OutDebug(HWND hWnd,char* debug)
{
   /*************************************************************************** 

   Function:
   ---------
   "OutDebug"
	   
   Parameters: 
   -----------
   1. HWND hWnd - handler to a window
   2. pointer to a char/string

   Return Values:
   --------------
   None

   Usage:
   ------
   Print program's info on the
   list control or any text dialog 
   specified by hWnd!
      
   פונקציה שירות זו תדפיס לדיאלוג את כל
   הודעות הטקסט המטפנות מפונקציות שירות אחרות

   ***************************************************************************/

	// Get handler to the output list window
   HWND   hList=GetDlgItem(hWnd,IDC_LIST);   
   LVITEM LvItem; // Item struct

   ZeroMemory(&LvItem,0); 
   LvItem.mask=LVIF_TEXT;	// סגנון טקסט
   LvItem.cchTextMax = 256; // גודל בבתים המקסימלי של הטקסט
   LvItem.iItem=500000000;	// מתחיל מ 
   LvItem.iSubItem=0;		// אין טקסט תת-משני
   LvItem.pszText=debug;	// טקסט להצגה
   SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem); // שליחת ההודעה לאובייקט
   SendMessage(hList,LVM_SCROLL ,0,(LPARAM)8); // שליחת ההודעה לאובייקט

}

// ===================================
// === Set Colors for an List view ===
// ===================================
LONG SetColor(long handle)
{
	/***************************************************************************

	Function:
	---------
	"SetColor"

	Parameters:
	-----------
	1. return value (optional)
		
	Return Values:
	--------------
	1. Return the Color of user selected
	Color from a color dialog
		
	Usage:
	------
	This function will return an
	RGB (red/green/blue) LONG number
	Selected by the user with the
	Color's dialog!

	פונקציה זו תחזיר את מספר
	הצבע הנבחר בתיבת בחירת הצבעים
	ע"י המשתמש

    ***************************************************************************/
	// Color dialog struct
	static CHOOSECOLOR	cc;
	static COLORREF		crCustColors[16];
	
	// Init the struct
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize	= sizeof (cc);
	cc.hwndOwner	= NULL;
	cc.hInstance	= NULL;
	
	if(handle!=0)
		cc.rgbResult	= RGB(0x0, 0x0, 0x0);
	else
		cc.rgbResult	= RGB(0x80, 0x80, 0x80);
	
	cc.lpCustColors		= (LPDWORD) crCustColors;
	cc.Flags			= CC_RGBINIT | CC_FULLOPEN;
	cc.lCustData		= 0;
	cc.lpfnHook			= NULL;
	cc.lpTemplateName	= NULL;

	// show color picker dialog

	if(ChooseColor (&cc)==0)
        return -1;

	// return selected color.
	return cc.rgbResult;
}

// ===================================
// ========= Strip EXE's Path ========
// ===================================
void GetExeName(char *FileName)
{
	/***************************************************************************

	Function:
	---------

	Parameters:
	-----------
	1. Pointer to a full path of file string

	Return Values:
	--------------
	None

	Usage:
	------
	This function will delete
    the full path of the parameter
    called FileName.
	and will return on the file's name
		
	פונקציה זו תמחק את המסלול
	הנוכחי שהקובץ נמצא
	ותחזיר רק את שם הקובץ!

    Example:
	Before: FileName "c:\my documents\Example.exe"
	After:  FileName "Example.exe"
    ***************************************************************************/

	int  len=0,i=0;
	char temp[MAX_PATH]="";

	// Get the string's length
	len=StringLen(FileName)-1; 

	// We search from the end, untill '\'
	while(FileName[len]!='\\')
	{
		len--; // keep searching
	}

	len++;

	// Lets copy only the executable file name
	while(FileName[len]!='\0')
	{
		temp[i]=FileName[len];
		len++;
		i++;
	}
	temp[i]=NULL;

	// copy temp to the parameter
	lstrcpy(FileName,temp);
}

// ===============================================================
// ======== Mask Edit Control to accept only user's input ========
// ===============================================================
BOOL MaskEditControl(HWND hwndEdit, char szMask[], DWORD nOptions)
{
	/***************************************************************************

	Function:
	---------
	"MaskEditControl"

	Parameters:
	-----------
	1. HWND hwndEdit - Handler to the window to mask
	2. char szMask[] - pointer/array of chars to use only!
	3. DWORD nOptions - Options to mask
		
	Return Values:
	--------------
	Success/no Success

	Usage:
	------
	this function will mask an edit control.
	meaning it will change/subClassed the edit control's
	attributes into user defined text!

   פונקציה זו תשנה את אובייקט הטקסט
	!כך שתשתמש בתווים המוגדרים ע"י המשתמש בלבד

    ***************************************************************************/
    
	int		i;
    WNDPROC	oldproc;
    BOOL*	mask;
	MASK_EDIT_CONTROL *userData = new MASK_EDIT_CONTROL();
    // Don't make a new mask if there is already one available
	// Get Window's styles
    oldproc = (WNDPROC)GetWindowLongPtr(hwndEdit, GWL_WNDPROC);

    // build the lookup table. The method varies depending
    // on whether we want to allow or disallow the specified szMask characters
    if(nOptions == TRUE){
        for(i = 0; i < 255; i++)
			userData->mask[i] = FALSE;

        for(i = 0; szMask[i] != '\0'; i++)
			userData->mask[szMask[i]] = TRUE;
    }
    else{
        for(i = 0; i < 255; i++)
			userData->mask[i] = TRUE;

        for(i = 0; szMask[i] != '\0'; i++)
			userData->mask[szMask[i]] = FALSE;
    }

    // Don't update the user data if it is already in place
    if(oldproc != MaskedEditProc){
		userData->window = GetWindowLongPtr(hwndEdit, GWL_WNDPROC);
		// Set Styles
        SetWindowLongPtr(hwndEdit, GWL_WNDPROC, (LONG_PTR)MaskedEditProc);
        SetWindowLongPtr(hwndEdit, GWL_USERDATA, (LONG_PTR)userData);
    }

    return TRUE;
}


// ================================================================================
// ============================ MaskedEditProc ====================================
// ================================================================================
LRESULT CALLBACK MaskedEditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/***************************************************************************

	Function:
	---------
    "MaskedEditProc"

	Parameters:
	-----------
	1. HWND hwnd - handler to the window to change
	2. UINT uMsg - Window Messages
	3/4 - L/W-PARAM - window parameters

	Return Values:
	--------------
	Result of callback function

	Usage:
	------
	This is a call back function!
	it will subClass the edit control (handler)
	by trapping the WM_CHAR message and checking
	each time which character is entered!
	looking in the mask[] array we look if the user
	has enter the good chars or not

	זוהי פונקציה חזרה
    מטרתה היא לבדוק אם התווים שהוכנסו הם חוקיים
	או לא.
	mask הבדיקה נעשית ע"י חיפוש במערך
		
    ***************************************************************************/
	MASK_EDIT_CONTROL *userData;
	// get Style (retrieves information about the specified window)
	userData = (MASK_EDIT_CONTROL*) GetWindowLongPtr(hwnd, GWL_USERDATA);
	WNDPROC window = (WNDPROC)userData->window;

    switch(uMsg){
		// WM_NCDESTROY is the LAST message that a window will receive - 
		// Therefore we must finally remove the old wndproc here
		case WM_NCDESTROY:{
			delete userData;
		}
		break;

		// Sub Class the WM_CHAR Message
		case WM_CHAR:{
			if(userData->mask[wParam & 0xff] == TRUE)
				break;
			else
				return 0; // Ignore entered keys not valid
		}
    }

	// Return the message information to the specified window
    return CallWindowProc(window, hwnd, uMsg, wParam, lParam);
}

// ===============================================
// ======= Set specified font to a control =======
// ===============================================
void SetFont(HWND hWnd)
{
	/***************************************************************************
		
    Function:
	---------
	"SetFont"

	Parameters:
	-----------
	1. HWND hWnd - Window Handler

	Return Values:
	--------------
	None

	Usage:
	------
	This function will open the
	Font dialog properties/selector
	and will allow to change the font as seen
	on a specific dialog pointed by hWnd (window handler).

	פונקציה זו פותחת את דיאלוג הפונטים
	בו אנו נוכל לשנות את הפונט המוצג על גבי
	hWnd אובייקט המוצבע ע"י 

    ***************************************************************************/

	// Font struct
	LOGFONT LogFont; 
	// create a logical font 
	HFONT hFont= CreateFontIndirect(&LogFont);
	// structure contains information that the ChooseFont function
	CHOOSEFONT cf = {sizeof(CHOOSEFONT)};
	// he COLORREF value is a 32-bit value used to specify an RGB color. 
	COLORREF g_rgbText = RGB(0, 0, 0);
	cf.Flags = CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS;
	cf.hwndOwner = hWnd;
	cf.lpLogFont = &LogFont;
	cf.rgbColors = g_rgbText;
	// Font Properties:
	LogFont.lfHeight=14;
	LogFont.lfWidth=8;
	LogFont.lfEscapement=0;		// Making Font 
	LogFont.lfOrientation=0;
	LogFont.lfWeight=300;		// Font weight
	LogFont.lfItalic=0;			// No Italic
	LogFont.lfUnderline=0;		// No Underline
	LogFont.lfStrikeOut=0;		// No strike
	LogFont.lfCharSet=1;
	// Styles
	LogFont.lfOutPrecision=OUT_TT_PRECIS;
	LogFont.lfClipPrecision=CLIP_DEFAULT_PRECIS;
	LogFont.lfQuality=DRAFT_QUALITY;
	LogFont.lfPitchAndFamily=FF_MODERN;
	
	// Function creates a Font common dialog box 
	if(ChooseFont(&cf) /* Font Dialog Window*/)
	{
		// GetFont
		HFONT hf = CreateFontIndirect(&LogFont);
		if(hf)// Return value!=0 (Success font selected)
		{
			hFont = hf;
			// set new font on the dialog (hWnd)
			SendMessage(hWnd,WM_SETFONT,(WPARAM)hFont, TRUE);
		}
	}
}

// =====================================================
// ========= Intialize The DASM List ===================
// =====================================================
void InitDisasmWindow(HWND hWnd)
{
	/***************************************************************************
		
    Function:
	---------
	"InitDisasmWindow"

	Parameters:
	-----------
	1. HWND hWnd - window Handler

	Return Values:
	--------------
	None

	Usage:
	------
	This function intialize the disassembly
	ListView parameters and columns
	as well as the extended styles!

	פונקציה זו יוצרת ומשנה את
	אובייקט הרשימה שבו האסמבלי יופיע

    ***************************************************************************/
	
	// Column Struct
	LVCOLUMN LvCol; 
	
	INITCOMMONCONTROLSEX InitCtrlEx;

	InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCtrlEx.dwICC  = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&InitCtrlEx);

	// Set ListView Styles
	SendMessage(GetDlgItem(hWnd,IDC_DISASM),LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER/*|LVS_EX_GRIDLINES|LVS_EX_FLATSB*/);
	
	// Reset Struct members to Zero
	ZeroMemory(&LvCol,0);
	LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;	// Type of mask
	LvCol.pszText="Address";	// First Header Text
	LvCol.cx=0x40;																	// width of column
	SendMessage(GetDlgItem(hWnd,IDC_DISASM),LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);		// Insert/Show the coloum
	LvCol.pszText="Opcodes";														// First Header Text
	LvCol.cx=0x78;																	// width of column
	SendMessage(GetDlgItem(hWnd,IDC_DISASM),LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);		// Insert/Show the coloum
	LvCol.pszText="Disassembly";													// First Header Text
	LvCol.cx=0x100;																	// width of column
	SendMessage(GetDlgItem(hWnd,IDC_DISASM),LVM_INSERTCOLUMN,2,(LPARAM)&LvCol);		// Insert/Show the coloum
	LvCol.pszText="Comments";														// First Header Text
	LvCol.cx=0x100;																	// width of column
	SendMessage(GetDlgItem(hWnd,IDC_DISASM),LVM_INSERTCOLUMN,3,(LPARAM)&LvCol);		// Insert/Show the coloum	
	LvCol.pszText="References";
	LvCol.cx=0x120;
	SendMessage(GetDlgItem(hWnd,IDC_DISASM),LVM_INSERTCOLUMN,4,(LPARAM)&LvCol);		// Insert/Show the coloum	

    ListView_SetBkColor(GetDlgItem(hWnd,IDC_DISASM),DisasmColors.GetBackGroundColor());
    // First Time intialize, hide window
	ShowWindow(GetDlgItem(hWnd,IDC_DISASM),SW_HIDE);
}

// =====================================================
// ========= Creating Main Tool Bar ====================
// =====================================================
HWND CreateToolBar(HWND hWnd, HINSTANCE hInst)
{
	/***************************************************************************
	
    Function:
	---------
	"CreateToolBar"

	Parameters:
	-----------
	1. HWND hWnd - Window handler to create toolbar on
	2. HINSTANCE hInst - Our exe handler
	
	Return Values:
	--------------
	Function returns Window Handler of the tool-bar

	Usage:
	------
	This function will create a tool bar.
	and will set up the necessary buttons,
	pictures on buttons, as well as the
	styles and position !

    ***************************************************************************/

	HIMAGELIST	hImglBtn=NULL, hImglBtnHot=NULL;	// Toolbar images handlers
	TBBUTTON	tbb[25];		// Button struct for tool bar
	HBITMAP		hBtn;			// bitmaps handle
	HWND		hWndTB=NULL;	// Toolbar window handler	
	DWORD_PTR error;
	INITCOMMONCONTROLSEX InitCtrlEx;

	InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCtrlEx.dwICC  = ICC_LISTVIEW_CLASSES  |  //  list view
		ICC_PROGRESS_CLASS    |  //  progress bar
		ICC_TREEVIEW_CLASSES  |  //  tree view & tooltip
		ICC_BAR_CLASSES       |  //  toolbar, status bar, trackbar & tooltip
		ICC_UPDOWN_CLASS      |  //  up-down
		ICC_USEREX_CLASSES    |  //  extended combo box
		ICC_TAB_CLASSES       |  //  tab & tooltip
		ICC_COOL_CLASSES      |  //  rebar
		ICC_DATE_CLASSES      |  //  date & time picker
		0;
	InitCommonControlsEx(&InitCtrlEx);
	
	DWORD dwMainToolbarStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | TBSTYLE_TOOLTIPS |  TBSTYLE_FLAT | TBSTYLE_AUTOSIZE;
	
	// Begin ToolBar window creation
	hWndTB = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, dwMainToolbarStyle, 0, 0, 0, 0, hWnd, (HMENU)ID_TOOLBAR, GetModuleHandle(NULL), NULL);
	// Backwards compatibility with previous windows OS versions
	SendMessage(hWndTB, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);

	error = GetLastError();
	// Create image List
	hImglBtn = ImageList_Create(25, 22, ILC_COLOR32 | ILC_MASK , 19, 1);
	error = GetLastError();
	hBtn = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_TOOLBAR), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	error = GetLastError();
	// use transparency
	error=ImageList_AddMasked(hImglBtn, hBtn, RGB(255, 0, 255));
	error = GetLastError();
	DeleteObject(hBtn); // Kill image object
	error = GetLastError();

	// Create image List Hot Bar
	hImglBtnHot = ImageList_Create(25, 22, ILC_COLOR32 | ILC_MASK , 19, 1);
	hBtn = (HBITMAP)LoadImage(hInst, MAKEINTRESOURCE(IDB_TOOLBARHOT), IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR);
	// Use transparency
	ImageList_AddMasked(hImglBtnHot, hBtn, RGB(255, 0, 255));
	DeleteObject(hBtn); // Kill image object
	
	// Attach the image lists to the window handler
	SendMessage(hWndTB, TB_SETIMAGELIST, (WPARAM)0, (LPARAM)hImglBtn);
	SendMessage(hWndTB, TB_SETHOTIMAGELIST, (WPARAM)0, (LPARAM)hImglBtnHot);
	
	// button 1 properties
	tbb[0].iBitmap = 0;		// Array of picture to show
	tbb[0].idCommand = 0;
	tbb[0].fsState = TBSTATE_ENABLED;	// button status
	tbb[0].fsStyle = TBSTYLE_BUTTON;	// button style
	tbb[0].dwData = 0;
	tbb[0].iString = 0;
	tbb[0].idCommand=IDC_LOAD_FILE;		// button identifier

	// button 2 properties
	tbb[1].iBitmap = 1;
	tbb[1].idCommand = 0;
	tbb[1].fsState = TBSTATE_ENABLED;
	tbb[1].fsStyle = TBSTYLE_BUTTON;
	tbb[1].dwData = 0;
	tbb[1].iString = 0;
	tbb[1].idCommand=IDC_SAVE_DISASM;
	
	// separator properties
	tbb[2].iBitmap = 0;
	tbb[2].idCommand = 0;
	tbb[2].fsState = TBSTATE_ENABLED;
	tbb[2].fsStyle = TBSTYLE_SEP;
	tbb[2].dwData = 0;
	tbb[2].iString = 0;
	
	// button 4 properties
	tbb[3].iBitmap = 2;
	tbb[3].idCommand = 0;
	tbb[3].fsState = TBSTATE_ENABLED;
	tbb[3].fsStyle = TBSTYLE_BUTTON;
	tbb[3].dwData = 0;
	tbb[3].iString = 0;
	tbb[3].idCommand=IDC_PE_EDITOR;
	
	// button 5 properties
	tbb[4].iBitmap = 3;
	tbb[4].idCommand = 0;
	tbb[4].fsState = TBSTATE_ENABLED;
	tbb[4].fsStyle = TBSTYLE_BUTTON;
	tbb[4].dwData = 0;
	tbb[4].iString = 0;
	tbb[4].idCommand=ID_PROCESS_SHOW;
	
	// separator properties
	tbb[5].iBitmap = 0;
	tbb[5].idCommand = 0;
	tbb[5].fsState = TBSTATE_ENABLED;
	tbb[5].fsStyle = TBSTYLE_SEP;
	tbb[5].dwData = 0;
	tbb[5].iString = 0;

	// button 7 properties
	tbb[6].iBitmap = 4;
	tbb[6].idCommand = 0;
	tbb[6].fsState = TBSTATE_ENABLED;
	tbb[6].fsStyle = TBSTYLE_BUTTON;
	tbb[6].dwData = 0;
	tbb[6].iString = 0;
	tbb[6].idCommand=ID_EP_SHOW;

	// button 8 properties
	tbb[7].iBitmap = 5;
	tbb[7].idCommand = 0;
	tbb[7].fsState = TBSTATE_ENABLED;
	tbb[7].fsStyle = TBSTYLE_BUTTON;
	tbb[7].dwData = 0;
	tbb[7].iString = 0;
	tbb[7].idCommand=ID_CODE_SHOW;

	// button 9 properties
	tbb[8].iBitmap = 6;
	tbb[8].idCommand = 0;
	tbb[8].fsState = TBSTATE_ENABLED;
	tbb[8].fsStyle = TBSTYLE_BUTTON;
	tbb[8].dwData = 0;
	tbb[8].iString = 0;
	tbb[8].idCommand=ID_ADDR_SHOW;

	// separator properties
	tbb[9].iBitmap = 0;
	tbb[9].idCommand = 0;
	tbb[9].fsState = TBSTATE_ENABLED;
	tbb[9].fsStyle = TBSTYLE_SEP;
	tbb[9].dwData = 0;
	tbb[9].iString = 0;

    // button 11 properties
	tbb[10].iBitmap = 7;
	tbb[10].idCommand = 0;
	tbb[10].fsState = TBSTATE_ENABLED;
	tbb[10].fsStyle = TBSTYLE_BUTTON;
	tbb[10].dwData = 0;
	tbb[10].iString = 0;
	tbb[10].idCommand=ID_SHOW_IMPORTS;

    // button 12 properties
	tbb[11].iBitmap = 8;
	tbb[11].idCommand = 0;
	tbb[11].fsState = TBSTATE_ENABLED;
	tbb[11].fsStyle = TBSTYLE_BUTTON;
	tbb[11].dwData = 0;
	tbb[11].iString = 0;
	tbb[11].idCommand=ID_SHOW_XREF;

    // separator properties
	tbb[12].iBitmap = 0;
	tbb[12].idCommand = 0;
	tbb[12].fsState = TBSTATE_ENABLED;
	tbb[12].fsStyle = TBSTYLE_SEP;
	tbb[12].dwData = 0;
	tbb[12].iString = 0;

    // button 13 properties
	tbb[13].iBitmap = 9;
	tbb[13].idCommand = 0;
	tbb[13].fsState = TBSTATE_ENABLED;
	tbb[13].fsStyle = TBSTYLE_BUTTON;
	tbb[13].dwData = 0;
	tbb[13].iString = 0;
	tbb[13].idCommand=ID_EXEC_JUMP;

    // button 12 properties
	tbb[14].iBitmap = 10;
	tbb[14].idCommand = 0;
	tbb[14].fsState = TBSTATE_ENABLED;
	tbb[14].fsStyle = TBSTYLE_BUTTON;
	tbb[14].dwData = 0;
	tbb[14].iString = 0;
	tbb[14].idCommand=ID_RET_JUMP;
    
    // button 12 properties
	tbb[15].iBitmap = 11;
	tbb[15].idCommand = 0;
	tbb[15].fsState = TBSTATE_ENABLED;
	tbb[15].fsStyle = TBSTYLE_BUTTON;
	tbb[15].dwData = 0;
	tbb[15].iString = 0;
	tbb[15].idCommand=ID_EXEC_CALL;
    
    // button 13 properties
	tbb[16].iBitmap = 12;
	tbb[16].idCommand = 0;
	tbb[16].fsState = TBSTATE_ENABLED;
	tbb[16].fsStyle = TBSTYLE_BUTTON;
	tbb[16].dwData = 0;
	tbb[16].iString = 0;
	tbb[16].idCommand=ID_RET_CALL;

    // button 14 properties
	tbb[17].iBitmap = 13;
	tbb[17].idCommand = 0;
	tbb[17].fsState = TBSTATE_ENABLED;
	tbb[17].fsStyle = TBSTYLE_BUTTON;
	tbb[17].dwData = 0;
	tbb[17].iString = 0;
	tbb[17].idCommand=ID_XREF;

    // separator properties
	tbb[18].iBitmap = 0;
	tbb[18].idCommand = 0;
	tbb[18].fsState = TBSTATE_ENABLED;
	tbb[18].fsStyle = TBSTYLE_SEP;
	tbb[18].dwData = 0;
	tbb[18].iString = 0;

     // button 15 properties
	tbb[19].iBitmap = 14;
	tbb[19].idCommand = 0;
	tbb[19].fsState = TBSTATE_ENABLED;
	tbb[19].fsStyle = TBSTYLE_CHECK;
	tbb[19].dwData = 0;
	tbb[19].iString = 0;
	tbb[19].idCommand=ID_DOCK_DBG;

    // separator properties
	tbb[20].iBitmap = 0;
	tbb[20].idCommand = 0;
	tbb[20].fsState = TBSTATE_ENABLED;
	tbb[20].fsStyle = TBSTYLE_SEP;
	tbb[20].dwData = 0;
	tbb[20].iString = 0;	

     // button 16 properties
	tbb[21].iBitmap = 15;
	tbb[21].idCommand = 0;
	tbb[21].fsState = TBSTATE_ENABLED;
	tbb[21].fsStyle = TBSTYLE_BUTTON;
	tbb[21].dwData = 0;
	tbb[21].iString = 0;
	tbb[21].idCommand=ID_HEX;

    // button 17 properties
	tbb[22].iBitmap = 16;
	tbb[22].idCommand = 0;
	tbb[22].fsState = TBSTATE_ENABLED;
	tbb[22].fsStyle = TBSTYLE_BUTTON;
	tbb[22].dwData = 0;
	tbb[22].iString = 0;
	tbb[22].idCommand=ID_SEARCH;

    // button 18 properties
	tbb[23].iBitmap = 17;
	tbb[23].idCommand = 0;
	tbb[23].fsState = TBSTATE_ENABLED;
	tbb[23].fsStyle = TBSTYLE_BUTTON;
	tbb[23].dwData = 0;
	tbb[23].iString = 0;
	tbb[23].idCommand=ID_PATCH;

	// button 19 properties 
	tbb[24].iBitmap = 18;
	tbb[24].idCommand = 0;
	tbb[24].fsState = TBSTATE_ENABLED;
	tbb[24].fsStyle = TBSTYLE_BUTTON;
	tbb[24].dwData = 0;
	tbb[24].iString = 0;
	tbb[24].idCommand=ID_SCRIPT;
    
    // Add the buttons
	SendMessage(hWndTB, TB_ADDBUTTONS, (WPARAM)25, (LPARAM)(LPTBBUTTON)&tbb);	// add buttons
	
	// Auto Disable buttons on startup
	SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_PE_EDITOR,		(LPARAM)FALSE);	// Disable Buttons
	SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_SAVE_DISASM,	(LPARAM)FALSE);	// at first run
	
	SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EP_SHOW,	(LPARAM)FALSE);
	SendMessage(hWndTB, TB_ENABLEBUTTON, ID_CODE_SHOW,	(LPARAM)FALSE);
	SendMessage(hWndTB, TB_ENABLEBUTTON, ID_ADDR_SHOW,	(LPARAM)FALSE);

    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SHOW_IMPORTS,	(LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SHOW_XREF,		(LPARAM)FALSE);

    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_JUMP, (LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,  (LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_CALL, (LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,  (LPARAM)FALSE);

    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_XREF,	(LPARAM)FALSE);
    SendMessage(hWndTB, TB_CHECKBUTTON,  ID_DOCK_DBG,	(LPARAM)TRUE);

    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SEARCH,	(LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_PATCH,	(LPARAM)FALSE);

    // Disable Hex Button if Invalid Handler
	if(hRadHex==NULL){
		SendMessage(hWndTB, TB_ENABLEBUTTON, ID_HEX,  (LPARAM)FALSE);
	}

	// Kill image object
	DeleteObject(hBtn);

	// Return tool bar window handler
	return hWndTB;
}

// =========================
// ========= Clear =========
// =========================
void Clear(HWND hList)
{
   /***************************************************************************
	
    Function:
	---------
	"Clear"

	Parameters:
	-----------
	1. HWND hList - Window handler to clean,
	(mostly list views to be used)

	Return Values:
	--------------
	None

	Usage:
	------
	This Procedure is mostly used to clear a list view's
	items, when we want to reset the content.
	note; this function is slow when list view has allot
	of items, there for we need to create a thread for this!

   ***************************************************************************/
   
   // Clear the ListView with LVM_DELETEALLITEMS message. 
   SendMessage(hList,LVM_DELETEALLITEMS,0,0);
}


// =====================================================
// ================= SearchItemText ====================
// =====================================================
DWORD_PTR SearchItemText(HWND hList,char *SearchStr)
{    
	/***************************************************************************
	
    Function: SearchItemText
	---------

	Parameters: (HWND hList,char *SearchStr)
	-----------
	1. HWND hList		- Window handler to search in (ListViews).
	2. char *SearchStr	- String to find.

	Return Values: DWORD , Index of string
	--------------

	Usage:
	------
	The Function Returns the Index of the specified Search String
	If no Search found, the Function will return -1.

   ***************************************************************************/

    DWORD_PTR Index=0,iItems=0;
    bool  flag=false;

    iItems=SendMessage(hList,LVM_GETITEMCOUNT,0,0); // Number of Items in the window

    for(;Index<iItems;Index++){
        if(lstrcmp(DisasmDataLines[Index].GetAddress(),SearchStr)==0){
            flag=true;
            break;
        }
    }

    if(flag) return Index; else return -1;
}

// =====================================================
// ===================== SelectItem ====================
// =====================================================
void SelectItem(HWND hList,DWORD_PTR Index)
{
	/***************************************************************************
		
    Function: SelectItem
	---------

	Parameters: (HWND hList,DWORD Index)
	-----------
	1. HWND hList  - Window handler to search in (ListViews)
	2. DWORD Index - Item's Index

	Return Values: None
	--------------		

	Usage:
	------
	The Procedure Selects & Focus an Item as specified by Index Parameter.

   ***************************************************************************/
   
   ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);					// Deselect all items
   SendMessage(hList,LVM_ENSUREVISIBLE ,(WPARAM)Index,FALSE);			// If item is far, scroll to it
   ListView_SetItemState(hList,Index,LVIS_SELECTED ,LVIS_SELECTED);		// Select item   
   ListView_SetItemState(hList,Index,LVIS_FOCUSED ,LVIS_FOCUSED);		// Select item   
}

// =====================================================
// =================== GetLastItem =====================
// =====================================================
DWORD_PTR GetLastItem(HWND hList)
{
   	/***************************************************************************
	
    Function: GetLastItem
	---------

	Parameters: (HWND hList)
	-----------
	1. HWND hList  - Window handler to search in (ListViews)

	Return Values: DWORD
	--------------		

	Usage:
	------
	The Function Returns the Index of the last Item in the List.

   ***************************************************************************/

   DWORD_PTR Item;
   Item = SendMessage(hList,LVM_GETITEMCOUNT ,0,0);
   return Item;
}

//////////////////////////////////////////////////////////////////////////
//							SelectLastItem                              //
//////////////////////////////////////////////////////////////////////////
void SelectLastItem(HWND hList)
{
   	/***************************************************************************
	
    Function: SelectLastItem
	---------

	Parameters: (HWND hList)
	-----------
	1. HWND hList  - Window handler to search in (ListViews)		

	Return Values: None
	--------------		

	Usage:
	------
	The Procedure Selects & Focus the Last Item on the List.
    
    ****************************************************************************/

    SelectItem(hList,GetLastItem(hList)-1);
    SetFocus(hList);
}


//////////////////////////////////////////////////////////////////////////
//						SelectAllItems									//
//////////////////////////////////////////////////////////////////////////

void SelectAllItems(HWND hList){
    ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);		// Deselect all
    SendMessage(hList,LVM_ENSUREVISIBLE ,(WPARAM)-1,FALSE);	// Send to the Listview
    ListView_SetItemState(hList,-1,LVIS_SELECTED ,LVIS_SELECTED); 
}

//////////////////////////////////////////////////////////////////////////
//						ChangeDisasmScheme								//
//////////////////////////////////////////////////////////////////////////

void ChangeDisasmScheme(DWORD_PTR iIndex)
{
   	/*************************************************************************
	
    Function: ChangeDisasmScheme
	---------

	Parameters: (int iIndex)
	-----------
	1. int iIndex  - Color Schem Number

	Return Values: None
	--------------		

	Usage:
	------
	The Procedure Change The ListView's Color Scheme As Specified at iIndex

    *************************************************************************/
   
    switch(iIndex) 
    {
      case SOFTICE: SoftIce_Schem();	break;
      case OLLYDBG: OllyDbg_Schem();	break;
      case IDA:		IDA_Schem();		break;
      case W32DASM: W32dasm_Schem();	break;
      case CUSTOM:	Custom_Schem();	break;
    }
}

//////////////////////////////////////////////////////////////////////////
//						SoftICE Schem									//
//////////////////////////////////////////////////////////////////////////

void SoftIce_Schem(){
    DisasmColors.SetAddressTextColor(RGB(192,192,192));
    DisasmColors.SetAddressBackGroundTextColor(RGB(0,0,0));

    DisasmColors.SetOpcodesTextColor(RGB(192,192,192));
    DisasmColors.SetOpcodesBackGroundTextColor(RGB(0,0,0));

    DisasmColors.SetAssemblyTextColor(RGB(192,192,192));
    DisasmColors.SetAssemblyBackGroundTextColor(RGB(0,0,0));

    DisasmColors.SetCommentsTextColor(RGB(192,192,192));
    DisasmColors.SetCommentsBackGroundTextColor(RGB(0,0,0));

    DisasmColors.SetResolvedApiTextColor(RGB(192,192,192));
    DisasmColors.SetCallAddressTextColor(RGB(192,192,192));
    DisasmColors.SetJumpAddressTextColor(RGB(192,192,192));
    DisasmColors.SetBackGroundColor(RGB(0,0,0));
}

//////////////////////////////////////////////////////////////////////////
//							OllyDBG Schem								//
//////////////////////////////////////////////////////////////////////////

void OllyDbg_Schem(){
    DisasmColors.SetAddressTextColor(RGB(128,128,128));
    DisasmColors.SetAddressBackGroundTextColor(RGB(255,251,240));
    
    DisasmColors.SetOpcodesTextColor(RGB(0,0,0));
    DisasmColors.SetOpcodesBackGroundTextColor(RGB(255,251,240));
    
    DisasmColors.SetAssemblyTextColor(RGB(128,128,128));
    DisasmColors.SetAssemblyBackGroundTextColor(RGB(255,251,240));
    
    DisasmColors.SetCommentsTextColor(RGB(0,0,0));
    DisasmColors.SetCommentsBackGroundTextColor(RGB(255,251,240));

    DisasmColors.SetResolvedApiTextColor(RGB(0,0,0));
    DisasmColors.SetCallAddressTextColor(RGB(0,0,0));
    DisasmColors.SetJumpAddressTextColor(RGB(0,0,0));
    DisasmColors.SetBackGroundColor(RGB(255,251,240));
}

//////////////////////////////////////////////////////////////////////////
//							IDA Schem									//
//////////////////////////////////////////////////////////////////////////

void IDA_Schem(){
    DisasmColors.SetAddressTextColor(RGB(0,0,0));
    DisasmColors.SetAddressBackGroundTextColor(RGB(255,255,255));
    
    DisasmColors.SetOpcodesTextColor(RGB(0,0,255));
    DisasmColors.SetOpcodesBackGroundTextColor(RGB(255,255,255));
    
    DisasmColors.SetAssemblyTextColor(RGB(0,0,128));
    DisasmColors.SetAssemblyBackGroundTextColor(RGB(255,255,255));
    
    DisasmColors.SetCommentsTextColor(RGB(0,0,255));
    DisasmColors.SetCommentsBackGroundTextColor(RGB(255,255,255));
    
    DisasmColors.SetResolvedApiTextColor(RGB(0,0,255));
    DisasmColors.SetCallAddressTextColor(RGB(0,0,255));
    DisasmColors.SetJumpAddressTextColor(RGB(0,0,255));
    DisasmColors.SetBackGroundColor(RGB(255,255,255));
}

//////////////////////////////////////////////////////////////////////////
//							W32dasm Schem								//
//////////////////////////////////////////////////////////////////////////

void W32dasm_Schem(){
    DisasmColors.SetAddressTextColor(RGB(0,0,0));
    DisasmColors.SetAddressBackGroundTextColor(RGB(192,192,192));
    
    DisasmColors.SetOpcodesTextColor(RGB(0,0,0));
    DisasmColors.SetOpcodesBackGroundTextColor(RGB(192,192,192));
    
    DisasmColors.SetAssemblyTextColor(RGB(0,0,0));
    DisasmColors.SetAssemblyBackGroundTextColor(RGB(192,192,192));
    
    DisasmColors.SetCommentsTextColor(RGB(0,128,0));
    DisasmColors.SetCommentsBackGroundTextColor(RGB(192,192,192));
    
    DisasmColors.SetResolvedApiTextColor(RGB(0,0,255));
    DisasmColors.SetCallAddressTextColor(RGB(0,0,255));
    DisasmColors.SetJumpAddressTextColor(RGB(0,0,255));
    DisasmColors.SetBackGroundColor(RGB(192,192,192));
}

//////////////////////////////////////////////////////////////////////////
//							Custom Schem								//
//////////////////////////////////////////////////////////////////////////

void Custom_Schem(){
	DisasmColors.SetAddressTextColor(DisasmColors.GetAddressTextColor());
    DisasmColors.SetAddressBackGroundTextColor(DisasmColors.GetAddressBackGroundTextColor());
    
    DisasmColors.SetOpcodesTextColor(DisasmColors.GetOpcodesTextColor());
    DisasmColors.SetOpcodesBackGroundTextColor(DisasmColors.GetOpcodesBackGroundTextColor());
    
    DisasmColors.SetAssemblyTextColor(DisasmColors.GetAssemblyTextColor());
    DisasmColors.SetAssemblyBackGroundTextColor(DisasmColors.GetAssemblyBackGroundTextColor());
    
    DisasmColors.SetCommentsTextColor(DisasmColors.GetCommentsTextColor());
    DisasmColors.SetCommentsBackGroundTextColor(DisasmColors.GetCommentsBackGroundTextColor());
    
    DisasmColors.SetResolvedApiTextColor(DisasmColors.GetResolvedApiTextColor());
    DisasmColors.SetCallAddressTextColor(DisasmColors.GetCallAddressTextColor());
    DisasmColors.SetJumpAddressTextColor(DisasmColors.GetJumpAddressTextColor());

    DisasmColors.SetBackGroundColor(DisasmColors.GetBackGroundColor());
}
//////////////////////////////////////////////////////////////////////////
//                      Dark Mode Settings                              //
//////////////////////////////////////////////////////////////////////////

bool g_DarkMode = false;
bool g_CodeMapVisible = true;
HBRUSH g_hDarkBrush = NULL;
COLORREF g_DarkBkColor = RGB(30, 30, 30);
COLORREF g_DarkTextColor = RGB(200, 200, 200);
COLORREF g_LightBkColor = RGB(255, 251, 240);
COLORREF g_LightTextColor = RGB(0, 0, 0);

// Windows 10/11 dark mode API support for dark scrollbars
typedef BOOL (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, BOOL allow);
typedef BOOL (WINAPI *fnSetPreferredAppMode)(int appMode);
static fnAllowDarkModeForWindow AllowDarkModeForWindow = NULL;
static fnSetPreferredAppMode SetPreferredAppMode = NULL;
static bool g_DarkModeAPIsLoaded = false;

void InitDarkModeAPIs() {
    if (g_DarkModeAPIsLoaded) return;
    g_DarkModeAPIsLoaded = true;

    HMODULE hUxTheme = LoadLibraryA("uxtheme.dll");
    if (hUxTheme) {
        // Ordinal 133 = AllowDarkModeForWindow
        AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(133));
        // Ordinal 135 = SetPreferredAppMode (Windows 10 1903+)
        SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxTheme, MAKEINTRESOURCEA(135));

        // Enable dark mode for the app (1 = AllowDark)
        if (SetPreferredAppMode) {
            SetPreferredAppMode(1);
        }
    }
}

void ApplyDarkModeToControl(HWND hWnd, bool dark) {
    if (!hWnd) return;
    InitDarkModeAPIs();

    if (AllowDarkModeForWindow) {
        AllowDarkModeForWindow(hWnd, dark);
    }
    SetWindowTheme(hWnd, dark ? L"DarkMode_Explorer" : L"Explorer", NULL);

    // For ListView, also apply to header
    HWND hHeader = ListView_GetHeader(hWnd);
    if (hHeader) {
        if (AllowDarkModeForWindow) {
            AllowDarkModeForWindow(hHeader, dark);
        }
        SetWindowTheme(hHeader, dark ? L"DarkMode_ItemsView" : L"ItemsView", NULL);
    }
}

void LoadSettings() {
    char szPath[MAX_PATH];
    GetModuleFileNameA(NULL, szPath, MAX_PATH);
    char* lastSlash = strrchr(szPath, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';
    strcat_s(szPath, "PVDasm.ini");
    g_DarkMode = GetPrivateProfileIntA("Settings", "DarkMode", 0, szPath) != 0;
    g_CodeMapVisible = GetPrivateProfileIntA("Settings", "CodeMap", 1, szPath) != 0;
}

void SaveSettings() {
    char szPath[MAX_PATH];
    GetModuleFileNameA(NULL, szPath, MAX_PATH);
    char* lastSlash = strrchr(szPath, '\\');
    if (lastSlash) *(lastSlash + 1) = '\0';
    strcat_s(szPath, "PVDasm.ini");
    WritePrivateProfileStringA("Settings", "DarkMode", g_DarkMode ? "1" : "0", szPath);
    WritePrivateProfileStringA("Settings", "CodeMap", g_CodeMapVisible ? "1" : "0", szPath);
}

void ApplyDarkMode(bool dark) {
    // Delete old brush if exists
    if (g_hDarkBrush) {
        DeleteObject(g_hDarkBrush);
        g_hDarkBrush = NULL;
    }

    if (dark) {
        SoftIce_Schem();
        g_hDarkBrush = CreateSolidBrush(g_DarkBkColor);
    } else {
        // Light mode: default colors
        DisasmColors.SetAddressTextColor(RGB(0,0,0));
        DisasmColors.SetAddressBackGroundTextColor(RGB(255,251,240));
        DisasmColors.SetOpcodesTextColor(RGB(0,0,0));
        DisasmColors.SetOpcodesBackGroundTextColor(RGB(255,251,240));
        DisasmColors.SetAssemblyTextColor(RGB(0,0,0));
        DisasmColors.SetAssemblyBackGroundTextColor(RGB(255,251,240));
        DisasmColors.SetCommentsTextColor(RGB(0,0,0));
        DisasmColors.SetCommentsBackGroundTextColor(RGB(255,251,240));
        DisasmColors.SetResolvedApiTextColor(RGB(0,0,0));
        DisasmColors.SetCallAddressTextColor(RGB(0,0,0));
        DisasmColors.SetJumpAddressTextColor(RGB(0,0,0));
        DisasmColors.SetBackGroundColor(RGB(255,251,240));
        g_hDarkBrush = CreateSolidBrush(g_LightBkColor);
    }

    // Update main disassembly ListView
    HWND hList = GetDlgItem(Main_hWnd, IDC_DISASM);
    if (hList) {
        ListView_SetBkColor(hList, DisasmColors.GetBackGroundColor());
        ListView_SetTextBkColor(hList, DisasmColors.GetBackGroundColor());
        ApplyDarkModeToControl(hList, dark);  // Dark scrollbars
    }

    // Update message ListView (IDC_LIST)
    HWND hMsgList = GetDlgItem(Main_hWnd, IDC_LIST);
    if (hMsgList) {
        ListView_SetBkColor(hMsgList, dark ? g_DarkBkColor : g_LightBkColor);
        ListView_SetTextBkColor(hMsgList, dark ? g_DarkBkColor : g_LightBkColor);
        ListView_SetTextColor(hMsgList, dark ? g_DarkTextColor : g_LightTextColor);
        ApplyDarkModeToControl(hMsgList, dark);  // Dark scrollbars
    }

    // Update progress bar
    HWND hProgress = GetDlgItem(Main_hWnd, IDC_DISASM_PROGRESS);
    if (hProgress) {
        SendMessage(hProgress, PBM_SETBKCOLOR, 0, (LPARAM)(dark ? g_DarkBkColor : g_LightBkColor));
        SendMessage(hProgress, PBM_SETBARCOLOR, 0, (LPARAM)(dark ? RGB(0, 120, 215) : RGB(0, 120, 215)));
    }

    // Apply dark mode to main window for dark title bar (Windows 10/11)
    if (Main_hWnd) {
        ApplyDarkModeToControl(Main_hWnd, dark);
        // Invalidate all children to trigger repaint with new colors
        RedrawWindow(Main_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
    }
}

void ToggleDarkMode(HWND hWnd) {
    g_DarkMode = !g_DarkMode;
    ApplyDarkMode(g_DarkMode);
    CheckMenuItem(GetMenu(hWnd), IDC_DARK_MODE, g_DarkMode ? MF_CHECKED : MF_UNCHECKED);
    SaveSettings();
}
// =====================================================
// ==================== SwapWord =======================
// =====================================================
void SwapWord(BYTE *MemPtr,WORD *Original,WORD* Mirrored){

  /**********************************************************

  Name: SwapWord
  ---------------

  Parameters: (BYTE*,WORD*,WORD*)
  -----------

  Return Values: Void
  --------------

  Usage:
  ------
  The Procedure retrieve a dword and cut it to a word 
  from a memory pointer.
  it swap the word so the return values at the pointers
  are mirrored word and normal word

  ***********************************************************/
	#ifndef _M_X64
		// 32 Bit
		DWORD OriginalWord;
		DWORD MirroredWord;
	    
		_asm {
				push eax
				push edi
				xor eax,eax
				xor edi,edi
				mov edi,MemPtr
				mov ax,word ptr[edi]
				and eax,0FFFFh
				mov MirroredWord,eax	// 0x1312
				bswap eax
				shr eax,16
				mov OriginalWord,eax	// 0x1213
				pop edi
				pop eax
		}
	    
		*Original = OriginalWord;
		*Mirrored = MirroredWord;
	#else
		// 64 Bit
		DWORD OriginalDword=0;
		DWORD MirroredDword=0;

		OriginalDword = ((*MemPtr)<<8) | (*(MemPtr+1));
		MirroredDword = ((*(MemPtr+1))<<8) | (*MemPtr);

		*Original = (WORD)OriginalDword;
		*Mirrored = (WORD)MirroredDword;
	#endif
}


// =====================================================
// ==================== SwapDword ======================
// =====================================================
void SwapDword(BYTE *MemPtr,DWORD_PTR *Original,DWORD_PTR* Mirrored){
    /**********************************************************

     Name: SwapDword
     ---------------
  
     Parameters: (BYTE*,DWORD*,DWORD*)
     -----------
    
     Return Values: Void
     --------------
      
     Usage:
     ------
     The Procedure retrieve a dword from a memory pointer.
     It swap the dword so the return values at the pointers,
     Are mirrored dword and normal dword

    ***********************************************************/
	#ifndef _M_X64
		// 32 Bit
		DWORD OriginalDword=0;
		DWORD MirroredDword=0;
	    
		_asm {
				push edi
				push eax
				mov edi,MemPtr
				mov eax,dword ptr[edi]
				mov MirroredDword,eax	// 0x15141312
				bswap eax
				mov OriginalDword,eax	// 0x12131415
				pop eax
				pop edi
		}

		*Original = OriginalDword;
		*Mirrored = MirroredDword;
	#else
		// 64 Bit
		DWORD_PTR OriginalDword=0;
		DWORD_PTR MirroredDword=0;
		
		OriginalDword = ((*MemPtr)<<24) | ((*(MemPtr+1))<<16) | ((*(MemPtr+2))<<8) | (*(MemPtr+3));
		MirroredDword = ((*(MemPtr+3))<<24) | ((*(MemPtr+2))<<16) | ((*(MemPtr+1))<<8) | (*MemPtr);

		*Original = OriginalDword;
		*Mirrored = MirroredDword;
	#endif
}

// =====================================================
// ==================== HitColumn ======================
// =====================================================
LONG HitColumn(HWND hWnd,DWORD dimx){
  /**********************************************************

  Name: HitColumn
  -----

  Parameters: (HWND, DWORD)
  ----------

  Return Values: LONG
  -------------

  Usage:
  ------
  The function returns the column index that
  the Mouse curser is currently pointing at,
  (if inside the listView)

  ***********************************************************/

    DWORD i=0,sumx=0,x;
    // While we scan all columns
    while (TRUE){
        x=ListView_GetColumnWidth(hWnd,i);
        sumx+=x;
        if(dimx<sumx){
			// return Column Index
            return i;
        }
        i++;
    }

	return -1;
}

LONG HitColumnEx(HWND hWnd,DWORD dimx,DWORD Columns){

  /**********************************************************

  Name: HitColumn
  -----

  Parameters: (HWND, DWORD,DWORD)
  ----------

  Return Values: LONG
  -------------

  Usage:
  ------
  The function returns the column index that
  the Mouse curser is currently pointing at,
  (if inside the listView)

  ***********************************************************/

    DWORD i=0,sumx=0,x;
	if(Columns==0)
		return -1;

    // While we scan all columns
    while (Columns>0){
        x=ListView_GetColumnWidth(hWnd,i);
        sumx+=x;
        if(dimx<sumx){
			// return Column Index
            return i;
        }
        i++;
		Columns--;
    }

	return -1;
}

DWORD HitRow(HWND hWnd,LVHITTESTINFO *HitInfo)
{
    return ListView_HitTest(hWnd,HitInfo);
}

//////////////////////////////////////////////////////////////////////////
//						GetBranchDestination							//
//////////////////////////////////////////////////////////////////////////

DWORD_PTR GetBranchDestination(HWND hWnd)
{
    unsigned int uiIndex;
    HMENU hMenu=GetMenu(Main_hWnd);

    // Get Current Selected Item
    iSelected=SendMessage(hWnd,LVM_GETNEXTITEM,-1,LVNI_FOCUSED|LVNI_SELECTED);
    
	// Item is Not valid
	if(iSelected==-1){
        return -1;
	}

    try{
		// Scan all CodeFlow List
        for(uiIndex=0; uiIndex<DisasmCodeFlow.size();uiIndex++ ){ 
			// Check if selected address is located in the list.
            if( iSelected == DisasmCodeFlow[uiIndex].Current_Index ){
                // Get the Branch Destination
                iSelectItem = DisasmCodeFlow[uiIndex].Branch_Destination;
                // We Can trace the jump/call 
                DCodeFlow = TRUE;
                
                if(DisasmCodeFlow[uiIndex].BranchFlow.Jump==TRUE){
                    EnableMenuItem(hMenu,ID_GOTO_JUMP,MF_ENABLED);
                    EnableMenuItem(hMenu,ID_GOTO_EXECUTECALL,MF_GRAYED);
                    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_JUMP, (LPARAM)TRUE);
                    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_CALL, (LPARAM)FALSE);
                }
                else if(DisasmCodeFlow[uiIndex].BranchFlow.Call==TRUE){
                    EnableMenuItem(hMenu,ID_GOTO_JUMP,MF_GRAYED);
                    EnableMenuItem(hMenu,ID_GOTO_EXECUTECALL,MF_ENABLED);
                    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_JUMP, (LPARAM)FALSE);
                    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_CALL, (LPARAM)TRUE);
                }

                DisasmIndex=uiIndex;
                return iSelectItem;
            }
        } 
    }
    catch(...){ }
    // We can't navigate threw the code
    DCodeFlow = FALSE;
    EnableMenuItem(hMenu,ID_GOTO_JUMP,MF_GRAYED);
    EnableMenuItem(hMenu,ID_GOTO_EXECUTECALL,MF_GRAYED);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_JUMP, (LPARAM)FALSE);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_CALL, (LPARAM)FALSE);
    return -1;
}

DWORD_PTR GetBranchDestByIndex(DWORD_PTR Index)
{
    try{
		// Scan all CodeFlow List
        for(int uiIndex=0; uiIndex<DisasmCodeFlow.size();uiIndex++ ){ 
			// Check if selected address is located in the list.
            if( Index == DisasmCodeFlow[uiIndex].Current_Index ){
                // Get the Branch Destination
                Index = DisasmCodeFlow[uiIndex].Branch_Destination;
                return Index;
            }
        } 
    }
    catch(...){ }

    return -1;
}

//////////////////////////////////////////////////////////////////////////
//							GotoBranch									//
//////////////////////////////////////////////////////////////////////////

void GotoBranch()
{
    BRANCH_DATA BData;
    HWND Disasm_LV=GetDlgItem(Main_hWnd,IDC_DISASM);
    HMENU hMenu=GetMenu(Main_hWnd);
    DWORD_PTR Index;
    char Text[128];
    
    // Address of destination
    if(LoadedPe64)
        wsprintf(Text,"%08X%08X",(DWORD)((DWORD_PTR)iSelectItem>>32),(DWORD)iSelectItem);
    else
        wsprintf(Text,"%08X",iSelectItem);

    // Index of of the destinaton address
    Index = SearchItemText(Disasm_LV,Text); // search item
    
    // if found, go to it
    if(Index!=-1){ // found index  
        try{
            // Insert BackTrace Index.
            BData.ItemIndex=(DWORD)iSelected;
            BData.Call = DisasmCodeFlow[DisasmIndex].BranchFlow.Call;
            BData.Jump = DisasmCodeFlow[DisasmIndex].BranchFlow.Jump;
			// Insert new BackTrace
            BranchTrace.insert(BranchTrace.end(), BData);
            
            // Show Information
            if(LoadedPe64)
                wsprintf(Text,"Tracing Branch From %s -> %08X%08X",DisasmDataLines[iSelected].GetAddress(),(DWORD)((DWORD_PTR)iSelectItem>>32),(DWORD)iSelectItem);
            else
                wsprintf(Text,"Tracing Branch From %s -> %08X",DisasmDataLines[iSelected].GetAddress(),iSelectItem);
            OutDebug(Main_hWnd,Text);
            SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST)); 
            SetFocus(Disasm_LV);
            // Selects the New Destination
            SelectItem(Disasm_LV,Index); // select and scrolls to the item

            // Check what to enable.
            if(BranchTrace.back().Jump==TRUE){
                EnableMenuItem(hMenu,ID_GOTO_RETURN_JUMP,MF_ENABLED);
                EnableMenuItem(hMenu,ID_GOTO_RETURN_CALL,MF_GRAYED);
                SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,  (LPARAM)TRUE);
                SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,  (LPARAM)FALSE);
            }
            else if(BranchTrace.back().Call==TRUE){
                EnableMenuItem(hMenu,ID_GOTO_RETURN_JUMP,MF_GRAYED);
                EnableMenuItem(hMenu,ID_GOTO_RETURN_CALL,MF_ENABLED);
                SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,  (LPARAM)FALSE);
                SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,  (LPARAM)TRUE);
            }   
            
               // See if Next Instruction is a Branch Instruction
               GetBranchDestination(Disasm_LV);
        }
        catch(...){
            wsprintf(Text,"Faild on Tracing Branch");
            OutDebug(Main_hWnd,Text);
            SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST));
            SetFocus(Disasm_LV);
        }
    }
    else{
        if(LoadedPe64)
            wsprintf(Text,"Tracing Branch From %s -> %08X%08X : Invalid Destination",DisasmDataLines[iSelected].GetAddress(),(DWORD)((DWORD_PTR)iSelectItem>>32),(DWORD)iSelectItem);
        else
            wsprintf(Text,"Tracing Branch From %s -> %08X : Invalid Destination",DisasmDataLines[iSelected].GetAddress(),iSelectItem);
        OutDebug(Main_hWnd,Text);
        SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST));
        SetFocus(Disasm_LV);
    }
    
    // Can't Trace Next Time
    DCodeFlow = FALSE;
}

//////////////////////////////////////////////////////////////////////////
//							BackTrace									//
//////////////////////////////////////////////////////////////////////////

//
// Tracing Back user's Branch Tracing
//
void BackTrace()
{
   DWORD dwIndex;
   HWND Disasm_LV=GetDlgItem(Main_hWnd,IDC_DISASM);
   HMENU hMenu=GetMenu(Main_hWnd);
   char Text[40];

   // Check if the List is not Empty
   if(BranchTrace.empty()!=TRUE){
       try{
		   // Get Last Item on the vector
           dwIndex = BranchTrace.back().ItemIndex;
		   // Remove Last Item
           BranchTrace.pop_back();
           
           wsprintf(Text,"Tracing Back To -> %s",DisasmDataLines[dwIndex].GetAddress());
           OutDebug(Main_hWnd,Text);
           SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST)); 
           SetFocus(Disasm_LV);
           
		   // Select the item we just poped
           SelectItem(Disasm_LV,dwIndex); // select and scrolls to the item
           GetBranchDestination(Disasm_LV);
           
		   // Set Visual Interface when back trace ahs complete
           if(BranchTrace.empty()==TRUE){
               EnableMenuItem(hMenu,ID_GOTO_RETURN_JUMP,MF_GRAYED);
               EnableMenuItem(hMenu,ID_GOTO_RETURN_CALL,MF_GRAYED);
               SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,	(LPARAM)FALSE);
               SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,	(LPARAM)FALSE);
               return;
           }
           
           if(BranchTrace.back().Jump==TRUE){
               EnableMenuItem(hMenu,ID_GOTO_RETURN_JUMP,MF_ENABLED);
               EnableMenuItem(hMenu,ID_GOTO_RETURN_CALL,MF_GRAYED);
               SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,	(LPARAM)TRUE);
               SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,	(LPARAM)FALSE);
           }
           else if(BranchTrace.back().Call==TRUE){
               EnableMenuItem(hMenu,ID_GOTO_RETURN_JUMP,MF_GRAYED);
               EnableMenuItem(hMenu,ID_GOTO_RETURN_CALL,MF_ENABLED);
               SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,	(LPARAM)FALSE);
               SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,	(LPARAM)TRUE);
           } 
       }
       catch(...){ }
   }
   else{
       EnableMenuItem(hMenu,ID_GOTO_RETURN_JUMP,MF_GRAYED);
       EnableMenuItem(hMenu,ID_GOTO_RETURN_CALL,MF_GRAYED);
   }
}

//////////////////////////////////////////////////////////////////////////
//							GetXReferences								//
//////////////////////////////////////////////////////////////////////////

BOOL GetXReferences(HWND hWnd)
{
	DWORD Addr;
	DWORD_PTR Index;
    ItrXref it; // Iterator (Pointer) 
    HMENU hMenu=GetMenu(Main_hWnd);

	Index=SendMessage(hWnd,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
	if(Index==-1){
        return FALSE;
	}

    try{
		// Get current selected Address
        Addr=StringToDword(DisasmDataLines[Index].GetAddress());
        
		// Check if the address is located in the references
        it = XrefData.find(Addr);
		// Check if found
        if(it!=XrefData.end()){
			// Enable Button/menu
            iSelected = Addr;
            EnableMenuItem(hMenu,IDC_XREF_MENU,MF_ENABLED);
            SendMessage(hWndTB, TB_ENABLEBUTTON, ID_XREF,  (LPARAM)TRUE);
            return TRUE;
        }

    }
    catch(...){ }

    EnableMenuItem(hMenu,IDC_XREF_MENU,MF_GRAYED);
    SendMessage(hWndTB, TB_ENABLEBUTTON, ID_XREF,  (LPARAM)FALSE);
	return FALSE;

}

//////////////////////////////////////////////////////////////////////////
//							UnDockDebug									//
//////////////////////////////////////////////////////////////////////////

//
// This function removes the debug window from
// The main Dialog -> Undock it.
//
void UnDockDebug(HWND DestHWND, HWND hWndSrc,HWND hWnd)
{
    RECT rc,rcMain;
    int Height,Width;
    // Undock the Debug Window
    // Get Rectangle Information of the main gui window
    // And the Disasm's rectangle.
    GetWindowRect(DestHWND,&rc); // Get Rect of Window  
    GetClientRect(hWnd,&rcMain);
    // Calculate Width / Height
    Height = rc.bottom - rc.top;
    Width =  rc.right - rc.left;
    // Put new Size and Positino
    MoveWindow(DestHWND,rcMain.left+6,rcMain.top+36,Width,Height+90,TRUE);
    // Hide the Debug Window
    ShowWindow(hWndSrc,SW_HIDE);
    UpdateWindow(hWnd);
    CheckMenuItem(GetMenu(hWnd),IDC_DOCK_DEBUG,MF_UNCHECKED);
    SendMessage(hWndTB, TB_CHECKBUTTON, ID_DOCK_DBG,	(LPARAM)FALSE);
}

//////////////////////////////////////////////////////////////////////////
//							DockDebug									//
//////////////////////////////////////////////////////////////////////////

//
// This function Docks the hidden Debug Window
// Back to the main Dialog Window.
//
void DockDebug(HWND DestHWND, HWND hWndSrc,HWND hWnd)
{
    RECT rc,rcMain;
    int Height,Width;

    // Dock the Debug Window
    // Get Rectangle Information of the main gui window
    // And the Disasm's rectangle.
    GetWindowRect(DestHWND,&rc); // Get Rect of Window
    GetClientRect(hWnd,&rcMain);
    
    // Calculate Width / Height
    Height = rc.bottom - rc.top;
    Width =  rc.right - rc.left;
    // Put new Size and Position
    MoveWindow(DestHWND,rcMain.left+6,rcMain.top+36,Width,Height-90,TRUE);
    // Show Debug Window
    ShowWindow(hWndSrc,SW_SHOW);
    UpdateWindow(hWnd);
    CheckMenuItem(GetMenu(hWnd),IDC_DOCK_DEBUG,MF_CHECKED);
    SendMessage(hWndTB, TB_CHECKBUTTON, ID_DOCK_DBG,  (LPARAM)TRUE);

}

//////////////////////////////////////////////////////////////////////////
//								SearchText                              //
//////////////////////////////////////////////////////////////////////////

DWORD_PTR SearchText(DWORD_PTR dwPos,char* cpText,HWND hListView,bool bMatchCase,bool WholeWord)
{
    DWORD_PTR dwIndex=0,iItems;
    bool  found=false;

    if(dwPos==-1)	{ dwIndex=0;			} else { dwIndex=dwPos; }
    if(!bMatchCase)	{ CharUpper(cpText);	}
  
	// Number of Items in the window
    iItems=SendMessage(hListView,LVM_GETITEMCOUNT,0,0); 
    
	// No Items, Can't search
    if(iItems==0) return -1;

	// Scan and Find accourding to the Fucntion's parameters.
    for(;dwIndex<iItems;dwIndex++){
        if(WholeWord==FALSE){
			// Search string in string
            if(strstr(DisasmDataLines[dwIndex].GetAddress(),cpText)!=NULL)	{found=TRUE; break;}
            if(strstr(DisasmDataLines[dwIndex].GetCode(),cpText)!=NULL)		{found=TRUE; break;}
            if(strstr(DisasmDataLines[dwIndex].GetMnemonic(),cpText)!=NULL)	{found=TRUE; break;}
            if(strstr(DisasmDataLines[dwIndex].GetComments(),cpText)!=NULL)	{found=TRUE; break;}
        }
        else{
            if(WholeWord==TRUE){
				// Compare strings
                if(lstrcmp(DisasmDataLines[dwIndex].GetAddress(),cpText)==0)	{found=TRUE; break;}
                if(lstrcmp(DisasmDataLines[dwIndex].GetCode(),cpText)==0)		{found=TRUE; break;}
                if(lstrcmp(DisasmDataLines[dwIndex].GetMnemonic(),cpText)==0)	{found=TRUE; break;}
                if(lstrcmp(DisasmDataLines[dwIndex].GetComments(),cpText)==0)	{found=TRUE; break;}
            }
        }
    }

	if(found==TRUE){
        return dwIndex;
	}
    return -1;
}

//////////////////////////////////////////////////////////////////////////
//						ExtractFilePath									//
//////////////////////////////////////////////////////////////////////////
char* ExtractFilePath(char *cpFilePath)
{
    INT i=0,Length = StringLen(cpFilePath);
    cpFilePath+=Length;
    
	// Find the '\' character
    while(*cpFilePath!='\\'){
		i++;
		cpFilePath--;
    }

    cpFilePath++;
	// Put NULL after '\'
    *cpFilePath='\0';
    cpFilePath-=(Length-i+1);

	// Return the Pointer.
    return cpFilePath;
}

//////////////////////////////////////////////////////////////////////////
//					Copy Text To Clipboard								//
//////////////////////////////////////////////////////////////////////////
void CopyToClipboard(char *stringbuffer, HWND window)
{
    //try to open clipboard first
    if (OpenClipboard(window)){
        //clear clipboard
        EmptyClipboard();
        
        HGLOBAL clipbuffer;
        char * buffer;
        
        //alloc enough mem for the string;
        //must be GMEM_DDESHARE to work with the clipboard
        clipbuffer = GlobalAlloc(GHND, StringLen(stringbuffer));
        buffer = (char*)GlobalLock(clipbuffer);
        strcpy_s(buffer,StringLen(LPCSTR(stringbuffer))+1 ,LPCSTR(stringbuffer));
        GlobalUnlock(clipbuffer);
        
        //fill the clipboard with data
        //CF_TEXT indicates that the buffer is a text-string
        SetClipboardData(CF_TEXT,clipbuffer);
        //close clipboard as we don't need it anymore
        CloseClipboard(); 
    }
}

//////////////////////////////////////////////////////////////////////////
//					CopyDisasmToClipboard								//
//////////////////////////////////////////////////////////////////////////

//
// This fucntion copy the selected lines at,
// Disasse,bly window to a ClipBoard.
// - Thread
//
void CopyDisasmToClipboard()
{   
	char *Buffer=NULL,*NewBuff=NULL;
	HGLOBAL clipbuffer=NULL;

	try{
		if(OpenClipboard(NULL)){
			EmptyClipboard();

			Buffer=CopyFromDisasm();

			clipbuffer = GlobalAlloc(GMEM_DDESHARE, StringLen(Buffer)+1);
			NewBuff = (char*)GlobalLock(clipbuffer);
			strcpy_s(NewBuff, StringLen(LPCSTR(Buffer))+1,LPCSTR(Buffer));
			GlobalUnlock(clipbuffer);
			SetClipboardData(CF_TEXT,clipbuffer);

			//m_Clipboard = SetClipboardData(CF_TEXT,Buffer);

			OutDebug(Main_hWnd,"Buffer Copied to Clipboard.");
			Sleep(55);
			SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST));

			// Close the Clipboard
			CloseClipboard();

			if(clipbuffer){
				GlobalFree(clipbuffer);
				clipbuffer=NULL;
				NewBuff=NULL;
			}

			if(Buffer){
				delete Buffer;
				Buffer=NULL;
			}
		}
		else{
			OutDebug(Main_hWnd,"Clipbload cannot opened!");
		}
	}
	catch(...){
		OutDebug(Main_hWnd,"Error copying to clipbload!");
	}

    // Close Thread Handle and Kill The Thread
    CloseHandle(hDisasmThread);
    TerminateThread(hDisasmThread,0);
    ExitThread(0);
}

//////////////////////////////////////////////////////////////////////////
//						CopyDisasmToFile								//
//////////////////////////////////////////////////////////////////////////

//
// This fucntion copy the selected lines at,
// Disassembly window to a file.
// - Thread
//
void CopyDisasmToFile()
{
    OPENFILENAME ofn; // File selecting struct
    char FileName[MAX_PATH]="",*Buffer;
	
    HWND hList = GetDlgItem(Main_hWnd,IDC_DISASM);
    HANDLE hFile=NULL;
    DWORD Size=NULL;
	
    ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of Struct
    ofn.hwndOwner = Main_hWnd;
    ofn.lpstrFilter = "Text files (*.txt)\0*.txt\0ASM Files (*.asm)\0*.asm\0";
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "txt";
    
	// Save File dialog
    if(GetSaveFileName(&ofn)){
		// Create the file for WRITE mode.
        hFile = CreateFile(FileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		// Check if file is created
        if(hFile!=INVALID_HANDLE_VALUE){
			// Write to the File with the entire copied text
			OutDebug(Main_hWnd,"Copying to file...");
			Buffer=CopyFromDisasm();
			WriteFile(hFile,Buffer,StringLen(Buffer),&Size,NULL);
            CloseHandle(hFile);
			if(Buffer){
				delete Buffer;
				Buffer=NULL;
			}
        }
    }

    OutDebug(Main_hWnd,"Buffer Copied to File.");
    Sleep(55);
    SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST));	
    // Close Thread Handle and Kill The Thread
    CloseHandle(hDisasmThread);
    TerminateThread(hDisasmThread,0);
    ExitThread(0);
}

//////////////////////////////////////////////////////////////////////////
//						CopyFromDisasm									//
//////////////////////////////////////////////////////////////////////////

//
// This fucntion generate the dynamic string
// Which holds all the text selected by the user,
// At the Disassembly Window.
//
char* CopyFromDisasm()
{
	char *Address=NULL,*Opcode=NULL,*Assembly=NULL,*Comment=NULL,*Buffer=NULL,*TempBuffer=NULL,*CopyBuff=NULL;
	DWORD_PTR iSize=0,iSelect=-1,Padding=0,TotalSize=0;
	HWND hList = GetDlgItem(Main_hWnd,IDC_DISASM);
	HANDLE m_Clipboard=NULL;
	char* NewBuff=NULL;

	Buffer = new char[TAB_SIZE];
	ZeroMemory(Buffer,TAB_SIZE);
	lstrcpy(Buffer,"\0");
	
	// While we copy all lines
    while(TRUE){
		// Get selected index
        iSelect=SendMessage(hList,LVM_GETNEXTITEM,iSelect,LVNI_SELECTED);
        if(iSelect==-1){
            break; // exit loop when we finished processing the lines
		}

		iSize = StringLen(DisasmDataLines[iSelect].GetAddress());
		Address = new char[iSize+1+(TAB_SIZE)];
		ZeroMemory(Address,iSize+1+(TAB_SIZE));
		wsprintf(Address,"%s\t",DisasmDataLines[iSelect].GetAddress());

		iSize = StringLen(DisasmDataLines[iSelect].GetCode());
		switch(iSize){
			case 1: case 2: case 12: Padding=2; break;
			case 4: case 11:		 Padding=3; break;
			case 6:					 Padding=1; break;
			default:				 Padding=4; break;
		}
		
		Opcode = new char[iSize+1+Padding];
		ZeroMemory(Opcode,iSize+1+Padding);

		switch(iSize){
			case 1:	wsprintf(Opcode,"%s\t\t",	DisasmDataLines[iSelect].GetCode()); break;
			case 2:	wsprintf(Opcode,"%s\t\t",	DisasmDataLines[iSelect].GetCode()); break;
			case 4:	wsprintf(Opcode,"%s\t  ",	DisasmDataLines[iSelect].GetCode()); break;
			case 6:	wsprintf(Opcode,"%s\t",		DisasmDataLines[iSelect].GetCode()); break;
			case 11:wsprintf(Opcode,"%s   ",	DisasmDataLines[iSelect].GetCode()); break;
			case 12:wsprintf(Opcode,"%s  ",		DisasmDataLines[iSelect].GetCode()); break;
			default:wsprintf(Opcode,"%s    ",	DisasmDataLines[iSelect].GetCode()); break;
        }
		
		iSize = StringLen(DisasmDataLines[iSelect].GetMnemonic());
		Assembly = new char[iSize+1+(TAB_SIZE)];
		ZeroMemory(Assembly,iSize+1+(TAB_SIZE));
		wsprintf(Assembly,"%s\t",DisasmDataLines[iSelect].GetMnemonic());
		
		iSize = StringLen(DisasmDataLines[iSelect].GetComments());
		Comment = new char[iSize+1+(TAB_SIZE)];
		ZeroMemory(Comment,iSize+1+(TAB_SIZE));
		wsprintf(Comment,"%s\t",DisasmDataLines[iSelect].GetComments());

		iSize = StringLen(Address) + StringLen(Opcode) + StringLen(Assembly) + StringLen(Comment);
		TempBuffer = new char[iSize+(TAB_SIZE*2)+1];
		ZeroMemory(TempBuffer,iSize+(TAB_SIZE*2)+1);
		wsprintf(TempBuffer,"%s%s%s%s\r\n",Address,Opcode,Assembly,Comment);
		
		/*
		for(int i=0;i<iSize+(TAB_SIZE*2)+1;i++)
        {
            if(TempBuffer[i]==NULL){
				TempBuffer[i]=' ';
			}
        }
		
		TempBuffer[iSize+(TAB_SIZE*2)]=NULL;
		*/

		CopyBuff = new char[TotalSize+1];
		ZeroMemory(CopyBuff,TotalSize+1);
		strcpy_s(CopyBuff,StringLen(Buffer)+1,Buffer);
		if(Buffer){
			delete Buffer;
			Buffer=NULL;
		}

		TotalSize+=StringLen(TempBuffer)+1;
		Buffer = new char[TotalSize];
		ZeroMemory(Buffer,TotalSize);
		strcpy_s(Buffer,StringLen(CopyBuff)+1,CopyBuff);
		lstrcat(Buffer,TempBuffer);

		if(CopyBuff){
			delete CopyBuff;
			CopyBuff=NULL;
		}
		/*
		AllocBuffer2 = GlobalAlloc(GHND, TotalSize+1);
		CopyBuff = (char*)GlobalLock(AllocBuffer2);
		strcpy(CopyBuff,Buffer);
		GlobalUnlock(AllocBuffer2);

		GlobalFree(AllocBuffer);
		Buffer=NULL;
		
		TotalSize+=StringLen(TempBuffer)+1;
		AllocBuffer = GlobalAlloc(GHND, TotalSize);
		Buffer = (char*)GlobalLock(AllocBuffer);
		strcpy(Buffer,CopyBuff);
		lstrcat(Buffer,TempBuffer);
		GlobalUnlock(AllocBuffer);
		*/

		// Free Memory
		if(Address){
			delete Address;
			Address=NULL;
		}

		if(Opcode){
			delete Opcode;
			Opcode=NULL;
		}

		if(Assembly){
			delete Assembly;
			Assembly=NULL;
		}

		if(Comment){
			delete Comment;
			Comment=NULL;
		}

		if(TempBuffer){
			delete TempBuffer;
			TempBuffer=NULL;
		}
	}

	OutDebug(Main_hWnd,"Buffering Complete!");
	
	return Buffer;

	/*
    char Text[256],Temp1[256],Temp[256];
    char* Buffer,*tBuff;
    //HGLOBAL OldClip,NewClip;
    LVITEM LvItem;
    DWORD iSelect,iSize=1;
    HWND hList = GetDlgItem(Main_hWnd,IDC_DISASM);

	// Prepare the buffer, make it 1 byte only.
    Buffer = new char[iSize];
    strcpy(Buffer,"\0");
    
	// Intialize the Listview ITEM struct
	memset(&LvItem,0,sizeof(LvItem));
    LvItem.mask=LVIF_TEXT;
    LvItem.pszText=Text;
    LvItem.cchTextMax=256;
    
    OutDebug(Main_hWnd,"Buffering...");
    
    iSelect=-1; // Set as no lines selected

	// While we copy all lines
    while(TRUE)
    {
		// Get selected index
        iSelect=SendMessage(hList,LVM_GETNEXTITEM,iSelect,LVNI_SELECTED);
        if(iSelect==-1)
            break; // exit loop when we finished processing the lines
        
        LvItem.iItem=iSelect;
        ZeroMemory(Temp1,sizeof(Temp1)); // intialize the temp buffer

        // Format String
        for(int j=0;j<4;j++) // loop all columns
        {
			// current column
            LvItem.iSubItem=j;

			// Get the text
            SendMessage(hList,LVM_GETITEMTEXT, iSelect, (LPARAM)&LvItem);

			// Format the String (Opcode), depend on the lenght.
            if(j==1)
            {
                switch(StringLen(Text))
                {
					case 1:wsprintf(Temp,"%s\t\t",Text); break;
					case 2:wsprintf(Temp,"%s\t\t",Text); break;
					case 4:wsprintf(Temp,"%s\t  ",Text); break;
					case 6:wsprintf(Temp,"%s\t",Text);   break;
					case 11:wsprintf(Temp,"%s   ",Text); break;
					case 12:wsprintf(Temp,"%s  ",Text);  break;
					default:wsprintf(Temp,"%s    ",Text);
                }
            }
            else wsprintf(Temp,"%s    ",Text);

            lstrcat(Temp1,Temp); // Build a temp file
        }
        
        lstrcat(Temp1,"\r\n"); // add CR,LF
        // Dymaic String Growing...
        tBuff = new char[iSize];
		// Copy old Buffer to new Buffer
        strcpy(tBuff,Buffer);
		// Size is growing with new string formated
        iSize=StringLen(tBuff)+1;
        iSize+=StringLen(Temp1)+1;

		// Delete old buffer
        delete[]Buffer;
		// Realocate it with new Size
        Buffer = new char[iSize];

		// Intialize the Buffer
        ZeroMemory(Buffer,iSize);
		// get Lenght
        int Size=StringLen(Temp1);
        
        // Remove Hidden NULLs to avoid Bad length.
        for(j=0;j<Size;j++)
        {
            if(Temp1[j]==NULL)
				Temp1[j]=' ';
        }
        Temp1[Size]=NULL;
        
		// Copy old Buffer
        lstrcpy(Buffer,tBuff);
        lstrcat(Buffer,Temp1);
        Buffer[iSize-1]=NULL; // Put NULL in the end.
		// Delete Buffer
        delete[]tBuff;
    } 

	OutDebug(Main_hWnd,"Buffering Complete!");
	// Return the Pointer.
    return Buffer;
	*/
}

//////////////////////////////////////////////////////////////////////////
//						GetSelectedLines								//
//////////////////////////////////////////////////////////////////////////
void GetSelectedLines(DWORD_PTR* Start,DWORD_PTR* End)
{
	DWORD_PTR iSelect=-1;
    HWND hList = GetDlgItem(Main_hWnd,IDC_DISASM);

	iSelect=SendMessage(hList,LVM_GETNEXTITEM,iSelect,LVNI_SELECTED);
    if(iSelect==-1){
		*Start=-1;
		*End=-1;
        return; // exit loop when we finished processing the lines
	}

	*Start=iSelect;
	while(TRUE){
		iSelect=SendMessage(hList,LVM_GETNEXTITEM,iSelect,LVNI_SELECTED);
		if(iSelect==-1){
            break; // exit loop when we finished processing the lines
		}
		*End=iSelect;
	}
}

//////////////////////////////////////////////////////////////////////////
//							ExportMapFile								//
//////////////////////////////////////////////////////////////////////////

void ExportMapFile()
{
	OPENFILENAME ofn;
	HANDLE hFile;
	TreeMapItr itr;
	DWORD WritenBytes;
	char MapFile[256]="",Temp[128];

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of Struct
    ofn.hwndOwner = Main_hWnd;
    ofn.lpstrFilter = "Map Files (*.map)\0*.map\0";
	ofn.lpstrFile = MapFile;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "map";

	if(GetSaveFileName(&ofn)){
		hFile = CreateFile(MapFile,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if(hFile==INVALID_HANDLE_VALUE){
			MessageBox(Main_hWnd,"Can't Create Source File!","Error",MB_OK);
			return;
		}

		// Functions
		WriteFile(hFile,"[FUNCTIONS]\r\n",StringLen("[FUNCTIONS]\r\n"),&WritenBytes,NULL);
		for(int i=0;i<fFunctionInfo.size();i++){
			wsprintf(Temp,"%08X:%08X:%s\r\n",fFunctionInfo[i].FunctionStart,fFunctionInfo[i].FunctionEnd,fFunctionInfo[i].FunctionName);
			WriteFile(hFile,Temp,StringLen(Temp),&WritenBytes,NULL);
		}
		wsprintf(Temp,"Total=%d\r\n",fFunctionInfo.size());
		WriteFile(hFile,Temp,StringLen(Temp),&WritenBytes,NULL);

		// Data
		WriteFile(hFile,"[DATA]\r\n",StringLen("[DATA]\r\n"),&WritenBytes,NULL);
		
		itr=DataAddersses.begin();
		for(;itr!=DataAddersses.end();itr++){
			wsprintf(Temp,"%08X:%08X\r\n",(*itr).first,(*itr).second);
			WriteFile(hFile,Temp,StringLen(Temp),&WritenBytes,NULL);
		}
		wsprintf(Temp,"Total=%d\r\n",DataAddersses.size());
		WriteFile(hFile,Temp,StringLen(Temp),&WritenBytes,NULL);

		CloseHandle(hFile);
	}
}

//////////////////////////////////////////////////////////////////////////
//							ProduceDisasm								//
//////////////////////////////////////////////////////////////////////////
void ProduceDisasm()
{
	OPENFILENAME ofn;
	HANDLE hFile;
	DWORD WritenBytes;
	char prjFileName[256]="";
	char Data[1024];

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of Struct
    ofn.hwndOwner = Main_hWnd;
    ofn.lpstrFilter = "ASM Files (*.asm)\0*.asm\0";
	ofn.lpstrFile = prjFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "asm";

	if(GetSaveFileName(&ofn)){
		hFile = CreateFile(prjFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if(hFile==INVALID_HANDLE_VALUE)
		{
			MessageBox(Main_hWnd,"Can't Create Source File!","Error",MB_OK);
			return;
		}

		// Write all disassembly lines directly from data vector
		for(DWORD_PTR i=0;i<DisasmDataLines.size();i++){
			wsprintf(Data,"%s  %s  %s  %s\r\n",
				DisasmDataLines[i].GetAddress(),
				DisasmDataLines[i].GetCode(),
				DisasmDataLines[i].GetMnemonic(),
				DisasmDataLines[i].GetComments());
			WriteFile(hFile,Data,StringLen(Data),&WritenBytes,NULL);
		}

		OutDebug(Main_hWnd,"Disassembly View Copied to file.");
		CloseHandle(hFile);
	}

	TerminateThread(hDisasmThread,0);
	ExitThread(0);
}

//////////////////////////////////////////////////////////////////////////
//							Define Data Area							//
//////////////////////////////////////////////////////////////////////////
BOOL DefineAddressesAsData(DWORD_PTR dwStart, DWORD_PTR dwEnd, BOOL mDisplayError)
{
	char Message[128];
	TreeMapItr itr=DataAddersses.find((DWORD)dwStart);
	if(itr==DataAddersses.end()){
	   // Key is not found in the list, add new
	   AddNewDataDefine((DWORD)dwStart,(DWORD)dwEnd);
	   if(mDisplayError){
		   if(LoadedPe64)
			   wsprintf(Message,"Addresses %08X%08X-%08X%08X Transformed to Data",(DWORD)(dwStart>>32),(DWORD)dwStart,(DWORD)(dwEnd>>32),(DWORD)dwEnd);
		   else
			   wsprintf(Message,"Addresses %08X-%08X Transformed to Data",dwStart,dwEnd);
		   OutDebug(Main_hWnd,Message);
	   }
	   SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST));
	   return TRUE;
	}
	
	if(mDisplayError){OutDebug(Main_hWnd,"Start Address Already Defined.");}
	return FALSE;
}

//////////////////////////////////////////////////////////////////////////
//						Define Function Area							//
//////////////////////////////////////////////////////////////////////////
BOOL DefineAddressesAsFunction(DWORD_PTR dwStart, DWORD_PTR dwEnd, char* FName, BOOL mDisplayError)
{
	FUNCTION_INFORMATION fFunc;
	bool Found=FALSE;
	char Message[128];
	
	if((dwStart!=-1 && dwEnd!=-1) && (dwEnd>dwStart)){
		if(mDisplayError){ // also checks if this is TRUE than we come from PVScript Engine
			dwStart = StringToDword(DisasmDataLines[dwStart].GetAddress());
			dwEnd   = StringToDword(DisasmDataLines[dwEnd].GetAddress());
		}
		
		for(int i=0;i<fFunctionInfo.size();i++){
			if(fFunctionInfo[i].FunctionStart==dwStart){
				Found=TRUE;
				break;
			}
		}

		if(Found==FALSE){
			fFunc.FunctionStart=(DWORD)dwStart;
			fFunc.FunctionEnd=(DWORD)dwEnd;
			strcpy_s(fFunc.FunctionName,StringLen(FName)+1,FName);
			AddNewFunction(fFunc);
			if(mDisplayError){
				if(LoadedPe64)
					wsprintf(Message,"Range %08X%08X:%08X%08X -> Function",(DWORD)(dwStart>>32),(DWORD)dwStart,(DWORD)(dwEnd>>32),(DWORD)dwEnd);
				else
					wsprintf(Message,"Address Range %08X:%08X Transformed to Function EntryPoint",dwStart,dwEnd);
				OutDebug(Main_hWnd,Message);
				SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST));
			}
			return TRUE;
		}
		
		OutDebug(Main_hWnd,"Start Address Already Defined as EntryPoint.");
	}

	return FALSE;
}