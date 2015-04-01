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

 Copyright (C) 2011. By Shany Golan..

 Permission is granted to make and distribute verbatim copies of this
 Program provided the copyright notice and this permission notice are
 Preserved on all copies.


 File: Patcher.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/
   
// ================================================================
// ========================== INCLUDES ============================
// ================================================================

#include "Patcher.h"

using namespace std;  // Use Microsoft's STL Template Implemation

// ================================================================
// =======================  TYPE DEFINES  =========================
// ================================================================

typedef vector<CDisasmData>	DisasmDataArray;

// ================================================================
// =====================  EXTERNAL VARIABLES  =====================
// ================================================================

extern						WNDPROC OldWndProc;
extern HWND					Main_hWnd;
extern IMAGE_SECTION_HEADER	*SectionHeader;
extern IMAGE_NT_HEADERS		*nt_hdr;
extern DisasmDataArray		DisasmDataLines;
extern char					*OrignalData;
extern char					szFileName[MAX_PATH];
extern bool					LoadedPe,LoadedPe64,LoadedNe;

// ================================================================
// =====================  GLOBAL VARIABLES  =======================
// ================================================================
DWORD_PTR Size;
DWORD_PTR EipAddress;
bool Patched;
char *CodePointer;

//////////////////////////////////////////////////////////////////////////
//							DEFINES										//
//////////////////////////////////////////////////////////////////////////
#define USE_MMX

#ifdef USE_MMX
	#define StringLen(str) xlstrlen(str) // new lstrlen with mmx function
#else
    #define StringLen(str) lstrlen(str) // old c library strlen
#endif

//======================================================================================
//===============================  PatcherProc =========================================
//======================================================================================

BOOL CALLBACK PatcherProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: 
		{
            HWND hDisasm = GetDlgItem(Main_hWnd,IDC_DISASM);
            DWORD_PTR item=-1,Offset=0,lenght;
            DWORD_PTR i=0,j=0;
            char FormatHexData[128]="",HexData[128]="",Text[10];
            Patched=FALSE;

            try{
                CodePointer=OrignalData; // Point to the Beginning of the File 
                
                // get Item
                item=SendMessage(hDisasm,LVM_GETNEXTITEM,(WPARAM)-1,(LPARAM)LVNI_FOCUSED);
                if(item==-1)
                {
                    MessageBox(Main_hWnd,"Can't Patch Selected Line!","Notice",MB_OK|MB_ICONINFORMATION);
                    SendMessage(hWnd,WM_CLOSE,0,0);
                    return FALSE;
                }

                // Get Current Address
                lstrcpy(Text,DisasmDataLines[item].GetAddress());
                
                // Blank Address? (e.g: Entry Point Line/Subroutine)
                if(StringLen(Text)==0)
                {
                    MessageBox(Main_hWnd,"Can't Patch Selected Line!","Notice",MB_OK|MB_ICONINFORMATION);
                    SendMessage(hWnd,WM_CLOSE,0,0);
                    return FALSE;
                }
                
                // Put New Caption String
                wsprintf(HexData," Patching Address: %s",Text);
                SetWindowText(hWnd,HexData);
                HexData[0]='\0';
                
                EipAddress=Offset=StringToDword(Text); // Convert to DWORD
                // Goto Actual Offset in the EXE to read from.
                Offset -= nt_hdr->OptionalHeader.ImageBase;
                Offset = (Offset - SectionHeader->VirtualAddress)+SectionHeader->PointerToRawData;
                CodePointer+=Offset; // Point to the Selected Instruction Code Offset.
                Size = DisasmDataLines[item].GetSize(); // Number of bytes used by the instruction            
                Size += DisasmDataLines[item].GetPrefixSize();
                
                //Format bytes to String
                for(i=0;i<Size;i++,CodePointer++)
                {
                    wsprintf(Text,"%02X",(BYTE)*CodePointer);
                    lstrcat(HexData,Text);
                }
                
                CodePointer-=Size; // Back to the Instruction Code Offset.
                
                // Add Spaces (XX XX XX XX formating)
				lenght=StringLen(HexData);
                for(j=0,i=0;j<lenght;j+=2,i+=3)
                {
                    FormatHexData[i]=HexData[j];
                    FormatHexData[i+1]=HexData[j+1];
                    FormatHexData[i+2]=' ';
                }
                
                FormatHexData[i-1]='\0';
                
                // SubClass the EditControl
                OldWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_HEX),GWL_WNDPROC,(LONG_PTR)PatcherSubClass);
                
                // Put Formated Text
                SetDlgItemText(hWnd,IDC_HEX,FormatHexData);
                // Limit the Text
                SendMessage(GetDlgItem(hWnd,IDC_HEX),EM_SETLIMITTEXT,(WPARAM)(lenght+(lenght/2))-1,0);
                SendMessage(GetDlgItem(hWnd,IDC_HEX),EM_SETSEL,(WPARAM)0,(LPARAM)1);
                SetFocus(GetDlgItem(hWnd,IDC_PREVIEW));
                HideCaret(GetDlgItem(hWnd,IDC_PREVIEW)); // Remove | (caret) from EditBox
                SetFocus(GetDlgItem(hWnd,IDC_HEX));
                HideCaret(GetDlgItem(hWnd,IDC_HEX));     // Remove | (caret) from EditBox
            }
            catch(...)
            {
            }
		}
		break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
              case IDC_PATCH:
              {
                  DISASSEMBLY Disasm;
                  DWORD_PTR k=0,x=0;
                  DWORD_PTR Index=0,lenght;
                  byte table[]={0,1,2,3,4,5,6,7,8,9,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F}; 
                  char Temp[50]="",Text[50]="",*DisasmPtr;
                  
                  try{
                      
                      SendMessage(GetDlgItem(hWnd,IDC_HEX),WM_GETTEXT,(WPARAM)256,(LPARAM)Temp);
                      
                      // Remove Spaces
					  lenght=StringLen(Temp);
                      for(;k<lenght;k++,x++)
                      {
                          if(Temp[k]==' ')
                              k++;
                          
                          Text[x]=Temp[k];
                      }
                      
                      // Allocate Size bytes
                      byte *op=new byte[Size];
                      
                      // Format New Changes
					  lenght=StringLen(Text);
                      for(k=0,x=0;k<lenght;k+=2,x++)
                      {
                          if(Text[k]>=0x30 && Text[k]<=0x39)
                              op[x]=(table[Text[k]-0x30])<<4;
                          
                          if(Text[k]>='A' && Text[k]<='F')
                              op[x] = (table[Text[k]-0x37])<<4;
                          
                          if(Text[k+1]>=0x30 && Text[k+1]<=0x39)
                              op[x]|=(table[Text[k+1]-0x30]);
                          
                          
                          if(Text[k+1]>='A' && Text[k+1]<='F')
                              op[x] |= table[Text[k+1]-0x37];
                      }
                      
                      // Copy New Changes
                      for(k=0;k<Size;k++,CodePointer++)
                          *CodePointer=op[k];
                      
                      //Back to the Old Pointer
                      CodePointer-=Size;
                      
                      // Create On the fly Disassembly
                      DisasmPtr = (char*)op;
                      FlushDecoded(&Disasm);
                      k=0;
                      Disasm.Address=(DWORD)EipAddress;
                      Decode(&Disasm,DisasmPtr,&Index);
                      CharUpper(Disasm.Assembly);
                      SendMessage(GetDlgItem(hWnd,IDC_PREVIEW),WM_SETTEXT,0,(LPARAM)Disasm.Assembly);
                      
                      Patched=TRUE; // File has Been Patched
                      delete[] op;  // Free Allocated Memory
                  }
                  catch(...)
                  {
                  }
              }
              break;

              case IDC_BACKUP:
              {
                  char szNewFileName[MAX_PATH];

                  lstrcpy(szNewFileName,szFileName);
                  lstrcat(szNewFileName,".Bak");
                  if(CopyFile(szFileName,szNewFileName,TRUE)!=0)
                      MessageBox(hWnd,"BackUp File Created!","Patcher",MB_OK);
              }
              break;

              case IDC_PATCHER_EXIT:
              {
                  SendMessage(hWnd,WM_CLOSE,0,0);
              }
              break;
            }
        }
        break;

		case WM_PAINT:
		{
             return FALSE;
		}
		break;
		
        case WM_CLOSE: // We colsing the Dialog
        {
          if(Patched==TRUE)
          {
              EndDialog(hWnd,0);
              if(MessageBox(NULL,"Do you want to ReDisassemble to View Changes?","Notice",MB_YESNO)==IDYES)
              {
                  Patched=FALSE;
                  Clear(GetDlgItem(hWnd,IDC_LIST));
                  IntializeDisasm(LoadedPe,LoadedPe64,LoadedNe,Main_hWnd,OrignalData,szFileName);
                  DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DISASM), hWnd, (DLGPROC)DisasmDlgProc);
                  break;
              }
          }   

          EndDialog(hWnd,0);
        }
	    break;
	}
	return 0;
}


//======================================================================================
//=================================  PatcherSubClass ===================================
//======================================================================================

LRESULT CALLBACK PatcherSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
	    case WM_GETDLGCODE:
		{
			return DLGC_WANTALLKEYS;
		}
		break;

        case WM_INITDIALOG:
        {
            SetFocus(hWnd);
            HideCaret(hWnd);
        }
        break;

        case WM_SETCURSOR:
        {
            SetClassLongPtr(hWnd,GCL_HCURSOR, (LONG)LoadCursor(NULL,IDC_ARROW));                       
        }
        break; 

        case WM_MOUSEMOVE:
        {
            if(wParam & MK_LBUTTON)
                return false;
        }
        break;

        case WM_CHAR:
        {
           if(GoodChars(wParam)==FALSE) return false;
        }
        break;

        
        case WM_CONTEXTMENU:
        {
            return false;
        }
        break;

        case WM_KEYDOWN:
        {
            switch(wParam)
            {
                case VK_RIGHT: case VK_DOWN:
                case VK_UP: case VK_LEFT:
                {
                    if(GetKeyState(VK_SHIFT) & 0x8000) return false;
                }
                break;

                case VK_DELETE:
                {
                    return false;
                }
                break;
            }
            break;
        }
        break;

        case WM_KEYUP:
        {
            switch(wParam)
            {
                case VK_HOME:
                {
                    SendMessage(hWnd,EM_SETSEL,(WPARAM)0,(LPARAM)1);
                }
                break;

                case VK_RIGHT:  case VK_DOWN:
                {
                    DWORD_PTR res=0;
                    res=SendMessage(hWnd,EM_GETSEL,(WPARAM)0,(LPARAM)0);
                    res&=0x0FFFF;  // Get Starting Point 
                    if(res%3!=0)
                     res--;
                    SendMessage(hWnd,EM_SETSEL,(WPARAM)res,(LPARAM)res+1);
                }
                break;

                case VK_UP: case VK_LEFT:case VK_END:
                {
                    DWORD_PTR res=0;
                    res=SendMessage(hWnd,EM_GETSEL,(WPARAM)0,(LPARAM)0);
                    res&=0x0FFFF;  // Get Starting Point
                    
                    if((res-1)==-1)
                    {
                        SendMessage(hWnd,EM_SETSEL,(WPARAM)0,(LPARAM)1);
                        break;
                    } 
                    
                    if(res%3==0)
                     res--;

                    SendMessage(hWnd,EM_SETSEL,(WPARAM)res,(LPARAM)res-1);
                }
                break;

                default:
                {
                    DWORD_PTR res=0,pos;
                    res=SendMessage(hWnd,EM_GETSEL,(WPARAM)0,(LPARAM)0);
                    res&=0x0FFFF;  // Get Starting Point 
                    res++;

                    if(res%3!=0)
                        res--;
                        
                    SendMessage(hWnd,EM_SETSEL,(WPARAM)res,(LPARAM)res+1);
                    pos=SendMessage(hWnd,EM_LINELENGTH,0,(LPARAM)-1);
                    if(res>pos)
                        SendMessage(hWnd,EM_SETSEL,(WPARAM)pos-1,(LPARAM)pos);
                }
                break;
            }
            break;
        }
        break;

        case WM_UNDO:
        case WM_PASTE:
        {
            return false;
        }
        break;
            
        case WM_LBUTTONUP:
        {
            DWORD_PTR res=0;
            SetFocus(hWnd);
            HideCaret(hWnd);
            res=SendMessage(hWnd,EM_GETSEL,(WPARAM)0,(LPARAM)0);
            res&=0x0FFFF;  // Get Starting Point 
            if(res%3!=0)
                res--;
            SendMessage(hWnd,EM_SETSEL,(WPARAM)res,(LPARAM)res+1);
        }
        break;
    }   

    return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}


//======================================================================================
//================================  GoodChars ==========================================
//======================================================================================

BOOL GoodChars(DWORD_PTR wParam)
{
    switch(wParam)
    {
      case 0x61: case 0x62:
      case 0x63: case 0x64:
      case 0x65: case 0x66:
      case 0x30: case 0x31:
      case 0x32: case 0x33:
      case 0x34: case 0x35:
      case 0x36: case 0x37:
      case 0x38: case 0x39:
      case 0x41: case 0x42:
      case 0x43: case 0x44:
      case 0x45: case 0x46:
      {
        return true;
      }
      break;

      default: return false; break;
    }

    return false;
}