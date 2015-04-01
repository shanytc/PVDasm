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


 File: Sections.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/

#include <windows.h>
#include "resource\resource.h"
#include <stdio.h>
#include <stdlib.h>
#include <commctrl.h>
#include "PE_Sections.h"
#include "Functions.h"

// ================================================================
// ==================== EXTERNAL VARIABLES ========================
// ================================================================
extern bool	LoadedPe;
extern bool LoadedPe64;

//==========================================================================
//=========================== GLOBAL VARIABLES =============================
//==========================================================================

HWND					hList=NULL;
IMAGE_DOS_HEADER		*Dos_Header;
IMAGE_NT_HEADERS		*Nt_Header;
IMAGE_NT_HEADERS64		*Nt_Header64;
IMAGE_SECTION_HEADER	*sec_header;
struct PeSection		*Head=NULL;
HINSTANCE				hInsts;
HBITMAP					hMenuItemBitmap;
DWORD_PTR					iSelect;
bool					flag;
char					*MemPointer="";
char					*MemPointerCopy="";

//////////////////////////////////////////////////////////////////////////
//							DEFINES										//
//////////////////////////////////////////////////////////////////////////
#define USE_MMX

#ifdef USE_MMX
	#define StringLen(str) xlstrlen(str) // new lstrlen with mmx function
#else
    #define StringLen(str) lstrlen(str) // old c library strlen
#endif

//==========================================================================
//============================ FUNCTIONS ===================================
//==========================================================================

void Set_PE_Info(char *Data,HINSTANCE hInstance)
{
  /*
    Function:
	---------
	"Set_PE_Info"

	Parameters:
	-----------
	1. char *Data - pointer to memory mapped file
	2. HINSTANCE hInstance - file's handler

	Return Values:
	--------------
	None

	Usage:
	------
    פרמטרים: קבלת הליך הטיפול בדיאלוג שבו אנו נמצאים
	כתוצאה מזו אנו נוכל לטפל באובייקטים הנמצאים בו
	
	This Function read the PE header from memory address
	Of the mapped file.
	And will create a linked list chain, which
	Will be filled with the sections information from the PE.
	   
    פונקציה זו קוראת את הקובץ שממופה בזיכרון ע"י 
	שימוש במצביע על תחילת המיפוי, פונקציה זו תדפיס ותמלא
	את הרשימה המקושרת בנתונים הקשורים לראש הקובץ
  */

	// Get file map pointers
	MemPointer=Data;
	MemPointerCopy=Data; // make a backup!
	hInsts=hInstance;    // our exe instance
	DWORD i,NumberOfSections;
	// Head = NULL, this specify the link is empty and ready to be filled
	//  בראש מציינת כי הרשימה היא ריקה NULL השמת 
	Head=NULL;
	Dos_Header=(IMAGE_DOS_HEADER*)MemPointer;
	
	// Check 32/64Bit version of the PE
	if(LoadedPe==TRUE){
		Nt_Header=(IMAGE_NT_HEADERS*)(Dos_Header->e_lfanew+MemPointer);
		NumberOfSections=Nt_Header->FileHeader.NumberOfSections;
	}
	else if(LoadedPe64==TRUE)
	{
		Nt_Header64=(IMAGE_NT_HEADERS64*)(Dos_Header->e_lfanew+MemPointer);
		NumberOfSections=Nt_Header64->FileHeader.NumberOfSections;
	}
	
	/* 
		Create dynamic Linked List using,
		Number of setions from the PE header,
		the created list will hold each section information.
	
		יצירת רשימה מקושרת דינאמית, הרשימה תווצר ע"י
		כמות החלקים שהקובץ בנוי מהם
	*/
	
	for(i=0;i<NumberOfSections;i++)
	{  
		// Create the list, 
		// Number of nodes = number of sections
		AddNodeToBeginning(&Head); 
	}

	/*
	=========================================
	  Filling linked lists with sections info
	  מילוי הרשימה בנתונים מתמונת הקובץ 
	/=========================================*/
	
	/* 
		Read the first Section using,
	    the pointer to the mapped File
	
	    ע"י שימוש במצביע לקובץ ממופה בזיכרון אנו יכולים
	    לקרוא את מבנה את הקובץ
	
	    sec_hdr = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(nt_hdr->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
	
	    Pointer to the list (first Node)
	    מצביע לאיבר הראשון שברשימה
	*/

	// Get address of first section
	if(LoadedPe==TRUE){
		sec_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(Nt_Header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
	}
	else if(LoadedPe64==TRUE)
	{
		sec_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(Nt_Header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
	}
	
	
	struct PeSection *Ptr=Head; // Head=NULL
	
	// Read File PE Section header and save it into the Linked List
    for(i=0;i<NumberOfSections;i++)
	{
		wsprintf(Ptr->Name,"%s",sec_header->Name);
		Ptr->Characteristics		= sec_header->Characteristics;
		Ptr->NumberOfLinenumbers	= sec_header->NumberOfLinenumbers;
		Ptr->NumberOfRelocations	= sec_header->NumberOfRelocations;
		Ptr->PointerToLinenumbers	= sec_header->PointerToLinenumbers;
		Ptr->PointerToRawData		= sec_header->PointerToRawData;
		Ptr->SizeOfRawData			= sec_header->SizeOfRawData;
		Ptr->VirtualAddress			= sec_header->VirtualAddress;
		Ptr->PhysicalAddress		= sec_header->Misc.PhysicalAddress;
		Ptr->VirtualSize			= sec_header->Misc.VirtualSize;
		Ptr->PointerToRelocations	= sec_header->PointerToRelocations;

		// Read Next Section from Pe
		// אנו מקדמים את המצביע של מבנה התמונה של הקובץ
		// לתמונה הבאה
		sec_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(sec_header)+sizeof(IMAGE_SECTION_HEADER));
		Ptr=Ptr->Next; // Move to the Next List
	}
}


//==============================================================
//====================== SECTION EDITOR ========================
//==============================================================
// Section editor main Dialog
BOOL CALLBACK PE_Sections(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{ 	 
	    case WM_INITDIALOG:
		{
			LVCOLUMN LvCol;
			// get list view handle
			hList=GetDlgItem(hWnd,IDC_SECTIONS_VIEW);

			// Set ListView Ex-Styles
			SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_FLATSB|LVS_EX_ONECLICKACTIVATE);

			// Intilize ListView
			memset(&LvCol,0,sizeof(LvCol)); // set 0
			LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;	// Type of mask
			LvCol.cx=0x45;									// width between each coloum
			LvCol.pszText="Sections";						// First Header
			SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);	// Insert/Show the coloum
			LvCol.cx=0x47;
			LvCol.pszText="Virtual Size";							// Next coloum
			SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
			LvCol.cx=0x59;
			LvCol.pszText="Virtual Address";
			SendMessage(hList,LVM_INSERTCOLUMN,2,(LPARAM)&LvCol);
			LvCol.cx=0x55;
			LvCol.pszText="Raw Size";
			SendMessage(hList,LVM_INSERTCOLUMN,3,(LPARAM)&LvCol);
			LvCol.cx=0x55;
			LvCol.pszText="Raw Offset";
			SendMessage(hList,LVM_INSERTCOLUMN,4,(LPARAM)&LvCol);
			LvCol.cx=0x59;
			LvCol.pszText="Charactaristics";
			SendMessage(hList,LVM_INSERTCOLUMN,5,(LPARAM)&LvCol);
			
			// Show Section's information onto the ListView
			Show_Sections(Head);
			return true;
		}
		break;
		
		case WM_NOTIFY:
		{
			switch(LOWORD(wParam)) // hit control
			{
				case IDC_SECTIONS_VIEW:
				{
					// Duble clicking a section to edit
					if(((LPNMHDR)lParam)->code == NM_DBLCLK)
					{
						iSelect=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED|LVNI_SELECTED); // return item selected
						if(iSelect!=-1)
						{
							// Show Section editor
						    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_EDIT_SEC_HEADER), hWnd, (DLGPROC)Section_Header_Editor);					
						}
					}

					// Right click on ListView
					if(((LPNMHDR)lParam)->code == NM_RCLICK)
					{
							//char Section_Name[10];
							LVITEM LvItem;

							memset(&LvItem,0,sizeof(LvItem));
							LvItem.mask=LVIF_TEXT;
							LvItem.iSubItem=0;
							//LvItem.pszText=Section_Name;
							//LvItem.cchTextMax=256;
							//LvItem.iItem=iSelect;
							iSelect=SendMessage(hList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED|LVNI_SELECTED); // return item selected
							
							if(iSelect==-1) // no items selected
							{
								break;
							}
							
							flag=true; // set flag so that we know there was a hit
							
							if(flag==true && iSelect!=-1)
							{
								flag=false;
								// On right click, show menu with options!
								HMENU hMenu = LoadMenu (NULL, MAKEINTRESOURCE (IDR_SECTION));
								HMENU hPopupMenu = GetSubMenu (hMenu, 0);
								POINT pt;
								// Load bitmap for option 1
								hMenuItemBitmap = LoadBitmap(hInsts, MAKEINTRESOURCE(IDB_EDIT_PE)); 
								SetMenuItemBitmaps(hPopupMenu, 0, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap); 
								// Load bitmap for option 2
								hMenuItemBitmap = LoadBitmap(hInsts, MAKEINTRESOURCE(IDB_DUMP_PE)); 
								SetMenuItemBitmaps(hPopupMenu, 1, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap); 
								// Load bitmap for option 3
								hMenuItemBitmap = LoadBitmap(hInsts, MAKEINTRESOURCE(IDB_TABLE_PE)); 
								SetMenuItemBitmaps(hPopupMenu, 3, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap); 								
								// set Default option (1) - bold text
								SetMenuDefaultItem (hPopupMenu, 0, TRUE);
								// get cursor x,y
								GetCursorPos (&pt);
								// "hide" background
								SetForegroundWindow (hWnd);
								// show the menu using cursor points
								TrackPopupMenu (hPopupMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
								SetForegroundWindow (hWnd);
								// kill menus after loaded
								DestroyMenu (hPopupMenu);
								DestroyMenu (hMenu);
							}
					}
				}
				break;
			}		
		}
		break;

	    case WM_LBUTTONDOWN:
		{
			// moving the dialog with left button
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
		
	    case WM_PAINT:
		{
			// we are not painting anything
			return false;
		}
		break;
		
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) 
		{ 	 
			// Click on sub Menu for edit a section
			case ID_EDIT_SECTION:
			{
				DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_EDIT_SEC_HEADER), hWnd, (DLGPROC)Section_Header_Editor);
			}
			break;

			case ID_SAVE_SECTION:
			{
				// Saving the section option from the sub menu
				DumpSection(hWnd);
			}
			break;

			case ID_SEC_TABLE:
			{
				// Viewing the PE section's whole table
				DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_SECTIONS_TABLE), hWnd, (DLGPROC)Sections_Table);
			}
			break;

			case IDC_PE_SECTION_CANCEL:
			{
				// Deleting Object of bitmap
				DeleteObject(hMenuItemBitmap);
				// close main editor dialog
				EndDialog(hWnd,0);
			}
			break;
		}
		break;
	
		case WM_CLOSE:
		{
			// Free List from memory after closing the PE editor
			struct PeSection *Ptr=Head;
			FreeList(Ptr);
			// Free bitmap object
			DeleteObject(hMenuItemBitmap);
			// end the dialog
			EndDialog(hWnd,0);
		}
		break;
	}
	break;

	}
	return 0;
}


//==============================================================
//====================== SECTION HEADER EDITOR =================
//==============================================================
// We edit a section in here!
BOOL CALLBACK Section_Header_Editor(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{
		// This Window Message is the heart of the dialog //
		//================================================//
	    case WM_INITDIALOG: 
		{
			struct PeSection *ListPtr=Head;
			DWORD i;
			char Temp[256]="";

			// move to the selected section in our list
			for(i=0;i<iSelect;i++)
				ListPtr=ListPtr->Next;

			// show section information from the list
			wsprintf(Temp,"Modifying Section: %s",ListPtr->Name);
			SetWindowText(hWnd,Temp);
			SetDlgItemText(hWnd,IDC_EDIT_SEC_NAME,ListPtr->Name);
			wsprintf(Temp,"%08X",ListPtr->VirtualAddress);
			SetDlgItemText(hWnd,IDC_EDIT_SEC_RVA,Temp);
			wsprintf(Temp,"%08X",ListPtr->VirtualSize);
			SetDlgItemText(hWnd,IDC_EDIT_SEC_VSIZE,Temp);
			wsprintf(Temp,"%08X",ListPtr->PointerToRawData);
			SetDlgItemText(hWnd,IDC_EDIT_SEC_RAW,Temp);
			wsprintf(Temp,"%08X",ListPtr->SizeOfRawData);
			SetDlgItemText(hWnd,IDC_EDIT_SEC_RSIZE,Temp);
			wsprintf(Temp,"%08X",ListPtr->Characteristics);
			SetDlgItemText(hWnd,IDC_EDIT_SEC_CHAR,Temp);

			// subcluss the edit boxes to get only 0-f chars!
			MaskEditControl(GetDlgItem(hWnd,IDC_EDIT_SEC_RVA),  "0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_EDIT_SEC_NAME), "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_EDIT_SEC_VSIZE),"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_EDIT_SEC_RAW),  "0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_EDIT_SEC_RSIZE),"0123456789abcdefABCDEF\b",TRUE);
			MaskEditControl(GetDlgItem(hWnd,IDC_EDIT_SEC_CHAR), "0123456789abcdefABCDEF\b",TRUE);
			
			// limit the text on the edit boxes.
			SendDlgItemMessage(hWnd,IDC_EDIT_SEC_NAME,EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_EDIT_SEC_RVA,EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_EDIT_SEC_VSIZE,EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_EDIT_SEC_RAW,EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_EDIT_SEC_RSIZE,EM_SETLIMITTEXT,	(WPARAM)8,0);
			SendDlgItemMessage(hWnd,IDC_EDIT_SEC_CHAR,EM_SETLIMITTEXT,	(WPARAM)8,0);
			
			return true;
		}
		break;
		
	    case WM_LBUTTONDOWN: 
		{
			// moving dialog with left button
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
		
	    case WM_PAINT: 
		{
			return false;
		}
		break;
		
	    case WM_COMMAND: 
		{
			switch (LOWORD(wParam)) 
			{
				case IDC_EDIT_SEC_EXIT:
				{
					// close section header editor
					EndDialog(hWnd,0); 
				}
				break;

				case IDC_EDIT_SEC_SAVE:
				{
					// saving data from editboxes to the pe header
					struct PeSection *SecPtr=Head;
					Save_Sections(hWnd);
					// update the header
					Update_Pe();
					// show new information
					SendMessage(hList,LVM_DELETEALLITEMS ,0,0);
					// show the new information
					Show_Sections(SecPtr);
					// auto close the section header dialog
					EndDialog(hWnd,0);
				}
				break;
			}
			break;
			
		    case WM_CLOSE: 
			{
				//close header dialog
				EndDialog(hWnd,0);
			}
			break;
		}
		break;
	}
	return 0;
}

//========================================================
//==================== SECTION'S TABLE ===================
//========================================================
// show Section's structure in a tree control
BOOL CALLBACK Sections_Table(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{
		case WM_INITDIALOG: 
		{
		  // Show all section's information
		  // on the tree view
		  HIMAGELIST hImageList;
		  HTREEITEM Parent,Before;
		  TV_INSERTSTRUCT tvinsert;
		  HBITMAP hBitMap; 
		  struct PeSection *SecPtr=Head;
		  char Temp[10]="";

		  // Reset control
		  InitCommonControls();
		  // create pictures on the tree control
		  hImageList=ImageList_Create(16,16,ILC_COLOR16,2,10);
		  hBitMap=LoadBitmap(hInsts,MAKEINTRESOURCE(IDB_PE_TREE));
		  ImageList_Add(hImageList,hBitMap,NULL);
		  DeleteObject(hBitMap);
		  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_SETIMAGELIST,0,(LPARAM)hImageList);

		  // scan our linked list and put information
		  // onto the tree control.
		  while(SecPtr!=NULL)
		  {
			  tvinsert.hParent=NULL;			// top most level no need handle
			  tvinsert.hInsertAfter=TVI_ROOT;	// work as root level
			  tvinsert.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE;
			  tvinsert.item.pszText=SecPtr->Name;
			  tvinsert.item.iImage=0;
			  tvinsert.item.iSelectedImage=1;
			  Before=Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
			  //=============================
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Characteristics";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->Characteristics);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      //=============================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Number Of Line Numbers";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%04X",SecPtr->NumberOfLinenumbers);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      //=============================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Number Of Relocations";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%04X",SecPtr->NumberOfRelocations);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      //=====================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Physical Address (Virtual Size)";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->PhysicalAddress);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      //=====================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Pointer To Line Numberss";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->PointerToLinenumbers);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      //=====================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Pointer To Raw Data (Raw Offset)";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->PointerToRawData);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
              //=====================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Pointer To Relocations";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->PointerToRelocations);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
              //=====================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Size Of Raw Data (Raw Size)";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->SizeOfRawData);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
              //=====================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Virtual Address";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->VirtualAddress);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
              //=====================
			  tvinsert.hParent=Before;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText="Virtual Size";
			  Parent=(HTREEITEM)SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
		      
			  wsprintf(Temp,"%08X",SecPtr->VirtualSize);
			  tvinsert.hParent=Parent;
			  tvinsert.hInsertAfter=TVI_LAST;
			  tvinsert.item.pszText=Temp;
			  SendDlgItemMessage(hWnd,IDC_SECTIONS_TREE,TVM_INSERTITEM,0,(LPARAM)&tvinsert);
             
			  SecPtr=SecPtr->Next;
		  }
		  UpdateWindow(hWnd);
		}
		break;
		
		case WM_LBUTTONDOWN:
		{
			ReleaseCapture();
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0);
		}
		break;
		
		case WM_PAINT:
		{
			return false;
		}
		break;
		
		case WM_COMMAND:
		{	
			case IDC_SEC_TABLE_EXIT:
			{
				EndDialog(hWnd,0); 
			}
			break;

			case WM_CLOSE:
			{
				EndDialog(hWnd,0); 
			}
			break;
		}
		break;
	}
	return 0;
}
//=================================================================
//=======================FUNCTIONS=================================
//=================================================================

// ========================================
// ====== Add a new Block to the list =====
// ========================================
int AddNodeToBeginning(struct PeSection **headPtr)
{
	/*
	    Function:
		---------
		"AddNodeToBeginning"

		Parameters:
		-----------
		1. struct PeSection **headPtr - struct to modify

		Return Values:
		--------------
		result: TRUE/FALSE of function

		Usage:
		------
		This function adds a new node 
		To the linked list.
		The new struct node is not intialized!

		הפונקציה מוסיפה איבר חדש לרשימה המקושרת

	*/

	// make a temp block
	struct PeSection *temp;
	
	// Allocate memory for the temp block
	temp=(struct PeSection*)malloc(sizeof(struct PeSection));
	
	// Check if the allocation is not succesful
	if(temp==NULL) 
	{
		return false;
	}

	// Update the List's head
	temp->Next=*headPtr;
	*headPtr=temp;
	
	// Succesfuly added a node
	return true;
}

// =======================================
// =========== Show PE Info ==============
// =======================================
void SelListMake(struct PeSection *MemPtr)
{
	
	/* 
		Function:
		---------
		"SelListMake"

		Parameters:
		-----------
		1. struct PeSection **MemPtr - struct to read from

		Return Values:
		--------------
		None

		Usage:
		------
	   This function will print info of PE
	   From the PeSection struct onto the
	   List Control.
	*/

	char Text[255];
	LVITEM LvItem;
	memset(&LvItem,0,sizeof(LvItem));
	LvItem.mask=LVIF_TEXT;
	LvItem.iItem=0;
	LvItem.iSubItem=0;
	
	// put the info
	LvItem.pszText=MemPtr->Name;
	SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	LvItem.iSubItem=1;
	LvItem.pszText=MemPtr->Name;
	SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);
	//=================================================
	LvItem.iSubItem=1;
	sprintf_s(Text,"%08X",MemPtr->VirtualSize);
	LvItem.pszText=Text;
	SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	LvItem.iSubItem=1;
	LvItem.pszText=Text;
	SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);
	//===============================================
	LvItem.iSubItem=2;
	sprintf_s(Text,"%08X",MemPtr->VirtualAddress);
	LvItem.pszText=Text;
	SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
	LvItem.iSubItem=2;
	LvItem.pszText=Text;
	SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);
	//===============================================
	LvItem.iSubItem=3;
	sprintf_s(Text,"%08X",MemPtr->SizeOfRawData);
	LvItem.pszText=Text;
	SendMessage(hList,LVM_INSERTITEM,3,(LPARAM)&LvItem);
	LvItem.iSubItem=3;
	LvItem.pszText=Text;
	SendMessage(hList,LVM_SETITEM,3,(LPARAM)&LvItem);
	//===============================================
	LvItem.iSubItem=4;
	sprintf_s(Text,"%08X",MemPtr->PointerToRawData);
	LvItem.pszText=Text;
	SendMessage(hList,LVM_INSERTITEM,4,(LPARAM)&LvItem);
	LvItem.iSubItem=4;
	LvItem.pszText=Text;
	SendMessage(hList,LVM_SETITEM,4,(LPARAM)&LvItem);
	//===============================================
	LvItem.iSubItem=5;
	sprintf_s(Text,"%08X",MemPtr->Characteristics);
	LvItem.pszText=Text;
	SendMessage(hList,LVM_INSERTITEM,5,(LPARAM)&LvItem);
	LvItem.iSubItem=5;
	LvItem.pszText=Text;
	SendMessage(hList,LVM_SETITEM,5,(LPARAM)&LvItem);
	//=================================================
}


// ========================================
// === Free the linked List from Memory ===
// ========================================
void FreeList(struct PeSection *Ptr)
{
	/* 
		Function:
		---------
		"FreeList"

		Parameters:
		-----------
		1. struct PeSection *Ptr - struct to free

		Return Values:
		--------------
		None

		Usage:
		------
	    Recursive function to free a Linked List
	    From memory.
	
	    פונקציה לשיחרור רשימה מקושרת מהזיכרון
    */
	
	// Recurse, check if we are at the 
	// end of the list
	if(Ptr!=NULL)
	{
		FreeList(Ptr->Next);
		free(Ptr);
	}
}

// ========================================
// ========= Print Section Info ===========
// ========================================
void Show_Sections(struct PeSection *HeadPtr)
{
	/* 
		Function:
		---------
		"Show_Sections"

		Parameters:
		-----------
		1. struct PeSection *HeadPtr - struct to show from

		Return Values:
		--------------
		None

		Usage:
		------
	    This function will show the section
	    Information from our linked list contaning
	    All header information.

	    הפונקציה סורקת את הרשימה המקושרת
	    ומציגה את כל האינפורמציה ע"ג
	    אובייקט הרשימה.

	*/

	// Temp Pointer
	struct PeSection *Ptr=HeadPtr;
    
	// scan the list
	if(Ptr!=NULL)
	{
		// Show the section info
		Show_Sections(Ptr->Next);
		SelListMake(Ptr);
	}
}

// ================================================
// === Save the new edited Pe info into the List ==
// ================================================
void Save_Sections(HWND hWnd)
{
	/*
		Function:
		---------
		"Show_Sections"

		Parameters:
		-----------
		1. HWND hWnd - Widnow Handler

		Return Values:
		--------------
		None

		Usage:
		------
		This function updates the linked list
		With new information from the edit-boxes.

        הפונקציה מעדכנת את הרשימה המקושרת,
		את אותו איבר אותו אנו עידכנו!
    */

	
	struct PeSection *SecPtr=Head;
	DWORD SaveTemp,i;
	char Temp[256]="";
	
	// Save only the selected Section into the list
	for(i=0;i<iSelect;i++)
		SecPtr=SecPtr->Next;

	// Get and save
	GetDlgItemText(hWnd,IDC_EDIT_SEC_NAME,Temp,8);
	wsprintf(SecPtr->Name,"%s",Temp);
	
	GetDlgItemText(hWnd,IDC_EDIT_SEC_RVA,Temp,9);
	SaveTemp=StringToDword(Temp);
	SecPtr->VirtualAddress=SaveTemp;

	GetDlgItemText(hWnd,IDC_EDIT_SEC_VSIZE,Temp,9);
	SaveTemp=StringToDword(Temp);
	SecPtr->VirtualSize=SaveTemp;

	GetDlgItemText(hWnd,IDC_EDIT_SEC_RAW,Temp,9);
	SaveTemp=StringToDword(Temp);
	SecPtr->PointerToRawData=SaveTemp;

	GetDlgItemText(hWnd,IDC_EDIT_SEC_RSIZE,Temp,9);
	SaveTemp=StringToDword(Temp);
	SecPtr->SizeOfRawData=SaveTemp;

	GetDlgItemText(hWnd,IDC_EDIT_SEC_CHAR,Temp,9);
	SaveTemp=StringToDword(Temp);
	SecPtr->Characteristics=SaveTemp;
}

// =========================================================
// ======= Update the PE Header from the linked List =======
// =========================================================
void Update_Pe()
{
	/*
		Function:
		---------
		"Update_Pe"

		Parameters:
		-----------
		None

		Return Values:
		--------------
		None

		Usage:
		------
		This function updates the PE header
		Of the memory Mapped file from our Linked List!

		הפונקציה מעדכת את הקובץ ששוכן בזיכרון
		ע"י העתקת המידע מהרשימה המקושרת
    */
	
	struct PeSection *Ptr=Head; 
	DWORD i,len;

	// Pointer to the Section Header
	if(LoadedPe==TRUE){
		sec_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(Nt_Header->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER));
	}
	else if(LoadedPe64==TRUE)
	{
		sec_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(&(Nt_Header64->OptionalHeader))+sizeof(IMAGE_OPTIONAL_HEADER64));
	}
	
	// Start browsing the List, and copy data!
    while(Ptr!=NULL)
	{
		// save Section's name
		len=StringLen(Ptr->Name);
		for(i=0;i<len;i++)
		  sec_header->Name[i]=Ptr->Name[i];

		// if ramaining place, add ' ' (0x20) to it (Padding).
		for(;i<8;i++)
			sec_header->Name[i]=0x00;

		// Copy new Data from the list to the memory pointer of the file [PE]
		sec_header->Characteristics      = Ptr->Characteristics;
		sec_header->NumberOfLinenumbers  = Ptr->NumberOfLinenumbers;
		sec_header->NumberOfRelocations  = Ptr->NumberOfRelocations;
		sec_header->PointerToLinenumbers = Ptr->PointerToLinenumbers;
		sec_header->PointerToRawData     = Ptr->PointerToRawData;
		sec_header->SizeOfRawData        = Ptr->SizeOfRawData;
		sec_header->VirtualAddress       = Ptr->VirtualAddress;
		sec_header->Misc.PhysicalAddress = Ptr->PhysicalAddress;
		sec_header->Misc.VirtualSize     = Ptr->VirtualSize;
		
		// Read Next Section of PE
		sec_header = (IMAGE_SECTION_HEADER *)((UCHAR *)(sec_header)+sizeof(IMAGE_SECTION_HEADER));
		
		// Move to the Next Item in the List
		Ptr=Ptr->Next; 
	}
}

// =======================================
// ========= Dump Section To HDD =========
// =======================================
void DumpSection(HWND hWnd)
{
	/*
		Function:
		---------
		"DumpSection"

		Parameters:
		-----------
		1. HWND hWnd - Window Handler

		Return Values:
		--------------
		None

		Usage:
		------
		This function creates a new file, and calls
		DumpSectionBytes() to dump the selected section's
		Data from memory to a file in our hard drive.

		DumpSectionBytes הפונקציה יוצרת קובץ חדש וקוראת לפונקציה 
		בשביל העתקת תכולת האיזור המסומן מהזיכרון לקובץ בכונן הקשיח
    */
	
	
    // File Open Dialog Struct
	OPENFILENAME ofn;
	// File Handler
	HANDLE hFile;
	char pszFileName[MAX_PATH]="";
	
	// Intialize Struct
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); // SEE NOTE BELOW
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
	ofn.lpstrFile = pszFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = "";

	// Open Save Dialog
	if(GetSaveFileName(&ofn))
	{
		// Create a new file
		hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		
		// If handler is good, Dump the section
		if(hFile != INVALID_HANDLE_VALUE)
			DumpSectionBytes(hFile);

	}
	else return;
}

// ========================================
// ======== Section Dump ==================
// ========================================
void DumpSectionBytes(HANDLE hFile)
{
	/*
		Function:
		---------
		"DumpSectionBytes"

		Parameters:
		-----------
		1. HANDLE hFile - File to save to

		Return Values:
		--------------
		None

		Usage:
		------
		The function calculate the selected section's Size
		And write all the information from the list to a file.

		הפונקציה מחשבת את גודל האיזור אותו אנו רוצים להעתיק
		ומעתיקה לקובץ את כל תכולת האיזור שרשום ברשימה המקושרת
    */

	struct PeSection *SPtr=Head;
	DWORD dataWritten,i;

	// Get the memory pointer of Originaly Loaded File
	// New List Pointer
	char *SectionPointer=MemPointer;

	// Get the section selected from the linked list
	for(i=0;i<iSelect;i++)
		 SPtr=SPtr->Next;

	// Calculate Size of Selected Section
	SectionPointer+=SPtr->PointerToRawData;
	
	// Begine Exception handling rutnine
	try{
		// try write mem to hdd
		if(WriteFile(hFile,SectionPointer,SPtr->SizeOfRawData,&dataWritten,NULL)==0)
			throw 1;
	}
	catch(int)
	{
		// error accourd!! close the file at any cause.
		// and exit imidiatly.
		CloseHandle(hFile);
		return;
	}
	// Finished Dumping
	CloseHandle(hFile);
}
