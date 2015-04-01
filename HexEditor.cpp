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


 File: AnchorResizing.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/

#include "HexEditor.h"
#include "Functions.h"
#include "Resize\AnchorResizing.h"

extern HINSTANCE  hInst;

// ================================================================
// ====================== GLOBAL VARIABLES ========================
// ================================================================

HMODULE  hRadHex;
//HACCEL   hHexAccel;
//MSG      Msg;
char     FileName[MAX_PATH]="";

//////////////////////////////////////////////////////////////////////////
//							DEFINES										//
//////////////////////////////////////////////////////////////////////////
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


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

BOOL CALLBACK HexEditorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: 
		{               
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_RAHEXEDIT), GWL_USERDATA,ANCHOR_RIGHT  | ANCHOR_BOTTOM);
            /*
            hHexAccel = LoadAccelerators (hInst, MAKEINTRESOURCE(IDR_ACCELERATOR2));

            //////////////////////////////////////////////
            //				Message Pump				//
            //////////////////////////////////////////////
                       
            // Get the waiting Message
            while(GetMessage(&Msg, NULL, 0, 0)){
                // Translate Accelerator Keys
                if (!TranslateAccelerator(hWnd, hHexAccel, &Msg)){
                    TranslateMessage (&Msg);
                }
                
                // Dispatch/Process The Waiting Message
                DispatchMessage (&Msg);
            }
            */
		}
		break;

        // auto reintialize controls
        case WM_SHOWWINDOW:
		{
			InitializeResizeControls(hWnd);
		}
		break;

		// resizing the window auto resize the specified
		// controls on the window
		case WM_SIZE:
		{
			ResizeControls(hWnd);
		}
		break;

		case WM_LBUTTONDOWN: 
		{
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;

		case WM_PAINT: // constantly painting the window
		{
			return 0;
		}
		break;
		
        case WM_CLOSE: // We colsing the Dialog
        {
          EndDialog(hWnd,0);
        }
	    break;

		case WM_COMMAND: // Controling the Buttons
		{
			switch (LOWORD(wParam)) // what we pressed on?
			{
                case IDM_OPEN_FILE:
                {
                    LoadFileToRAHexEd(hWnd);
                }
                break;
                
                case IDM_SAVE_FILE:
                {
                    SaveFileFromRAHexEd(hWnd);
                }
                break;

                case IDM_SAVE_FILE_AS:
                {
                    SaveFileAsFromRaHexEd(hWnd);
                }
                break;

                case IDM_EXIT:
                {
                    HWND RadHexhWnd=GetDlgItem(hWnd,IDC_RAHEXEDIT);

                    if(SendMessage(RadHexhWnd,EM_GETMODIFY,0,0)==TRUE)
                    {
                        if(MessageBox(hWnd,"There Has Been Changes, Do You Want To Save Them?","Save Changes",MB_OKCANCEL|MB_ICONQUESTION)==IDOK)
                        {
                            SaveFileFromRAHexEd(hWnd);
                        }
                    }
                    
                    SendMessage(hWnd,WM_CLOSE,0,0);
                }
                break;

                case IDM_ADDRESS:
                {
                    Hide_UnHide_RadHexAddress(hWnd);
                }
                break;

                case IDM_ASCII:
                {
                    Hide_UnHide_RadHexAscii(hWnd);
                }
                break;

                case IDM_UPPERCASE:
                {
                    UpperLower_caseRadHex(hWnd);
                }
                break;

                case IDM_COPY:
                {
                    SendMessage(GetDlgItem(hWnd,IDC_RAHEXEDIT),WM_COPY,0,0);
                }
                break;

                case IDM_CUT:
                {
                    SendMessage(GetDlgItem(hWnd,IDC_RAHEXEDIT),WM_CUT,0,0);
                }
                break;

                case IDM_PASTE:
                {
                    SendMessage(GetDlgItem(hWnd,IDC_RAHEXEDIT),WM_PASTE,0,0);
                }
                break;

                case IDM_UNDO:
                {
                    SendMessage(GetDlgItem(hWnd,IDC_RAHEXEDIT),EM_UNDO,0,0);
                }
                break;

                case IDM_REDO:
                {
                    SendMessage(GetDlgItem(hWnd,IDC_RAHEXEDIT),EM_REDO,0,0);
                }
                break;

                case IDM_DELETE:
                {
                    SendMessage(GetDlgItem(hWnd,IDC_RAHEXEDIT),WM_CLEAR,0,0);
                }
                break;
			}
			break;
		}
		break;
	}
	return 0;
}

void LoadFileToRAHexEd(HWND hWnd)
{
    // File Select dialog struct
	OPENFILENAME ofn;
    EDITSTREAM editstream;
    CHARRANGE chrg;
    HANDLE hFile;
    HWND RadHexhWnd=GetDlgItem(hWnd,IDC_RAHEXEDIT);
    HMENU Menu = GetMenu(hWnd);
    char Temp[20]="",Text[256]="";

	// Intialize struct
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
    ZeroMemory(&editstream, sizeof(EDITSTREAM));
    ZeroMemory(&chrg, sizeof(CHARRANGE));

	ofn.lStructSize = sizeof(OPENFILENAME); // SEE NOTE BELOW
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "Executable files (*.exe, *.dll)\0*.exe;*.dll\0ROM Files (*.gb,*.gbc)\0*.gb;*.gbc\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = FileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "exe";
    
    if(GetOpenFileName(&ofn)!=NULL)
    {
        hFile=CreateFile(FileName,
			GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );
    }
    else return;

    if(hFile==INVALID_HANDLE_VALUE)
		return;
    
    strcpy_s(Temp,StringLen(FileName)+1,FileName);
    GetExeName(Temp);
    wsprintf(Text," HexEditor - [%s]",Temp);
    SetWindowText(hWnd,Text);

    editstream.dwCookie = (DWORD)hFile;
    editstream.pfnCallback=(EDITSTREAMCALLBACK)StreamInProc;

    SendMessage(RadHexhWnd,EM_STREAMIN,(WPARAM)SF_TEXT,(LPARAM)&editstream);
    CloseHandle(hFile);
    SendMessage(RadHexhWnd,EM_SETMODIFY,FALSE,0);
    chrg.cpMin=0;
    chrg.cpMax=0;
    SendMessage(RadHexhWnd,EM_EXSETSEL,0,(LPARAM)&chrg);

    EnableMenuItem(Menu,IDM_SAVE_FILE,MF_ENABLED);
    EnableMenuItem(Menu,IDM_SAVE_FILE_AS,MF_ENABLED);
}

void SaveFileFromRAHexEd(HWND hWnd)
{
    HWND RadHexhWnd=GetDlgItem(hWnd,IDC_RAHEXEDIT);

    // content of edit control has been modified?
    if(SendMessage(RadHexhWnd,EM_GETMODIFY,0,0)==TRUE)
    {
        SaveFile(RadHexhWnd,FileName);
    }
    else MessageBox(hWnd,"Nothing To Save, No Changes!","Opps",MB_OK);
}


void SaveFileAsFromRaHexEd(HWND hWnd)
{
    HWND RadHexhWnd=GetDlgItem(hWnd,IDC_RAHEXEDIT);
    OPENFILENAME ofn;
    char szFileNameSave[MAX_PATH]="";

    ZeroMemory(&ofn, sizeof(OPENFILENAME)); // Init the struct to Zero
    ofn.lStructSize = sizeof(OPENFILENAME); // SEE NOTE BELOW
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "Executable files (*.exe, *.dll)\0*.exe;*.dll\0ROM Files (*.gb,*.gbc)\0*.gb;*.gbc\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileNameSave;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
	ofn.lpstrDefExt = "exe";

    // content of edit control has been modified?
    if(SendMessage(RadHexhWnd,EM_GETMODIFY,0,0)==TRUE)
    {
        if(GetSaveFileName(&ofn)!=NULL)
        {
            SaveFile(RadHexhWnd,szFileNameSave);
        }
        else return;
    }
    else MessageBox(hWnd,"Nothing To Save, No Changes!","Opps",MB_OK);
}

DWORD CALLBACK StreamInProc(DWORD dwCookie, LPBYTE lpbBuff, LONG cb, LONG FAR *pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;

    if (!ReadFile(hFile,
            (LPVOID)lpbBuff, cb, (LPDWORD)pcb, NULL))
        return((DWORD)-1);
    return(0);
}

DWORD CALLBACK StreamOutProc(DWORD dwCookie, LPBYTE lpbBuff, LONG cb, LONG FAR *pcb)
{
    HANDLE hFile = (HANDLE)dwCookie;

    if (!WriteFile(hFile,
            (LPVOID)lpbBuff, cb, (LPDWORD)pcb, NULL))
        return((DWORD)-1);
    return(0);
}

void SaveFile(HWND RadWin,char *FileName)
{
    EDITSTREAM editstream;
    HANDLE hFile;

    // Init the struct to Zero
    ZeroMemory(&editstream, sizeof(EDITSTREAM));

    hFile=CreateFile(FileName,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
    
    if(hFile==INVALID_HANDLE_VALUE)
        return;
    
    editstream.dwCookie = (DWORD)hFile;
    editstream.pfnCallback=(EDITSTREAMCALLBACK)StreamOutProc;
    SendMessage(RadWin,EM_STREAMOUT,(WPARAM)SF_TEXT,(LPARAM)&editstream);
    CloseHandle(hFile);
    SendMessage(RadWin,EM_SETMODIFY,FALSE,0);
}

void Hide_UnHide_RadHexAddress(HWND hWnd)
{
    HWND RadHWnd = GetDlgItem(hWnd,IDC_RAHEXEDIT);
    HMENU Menu = GetMenu(hWnd);
    DWORD Style=0;

    Style = GetWindowLongPtr(RadHWnd,GWL_STYLE);

    if((GetMenuState(Menu,IDM_ADDRESS,MF_BYCOMMAND)==MF_CHECKED))
    {
        // Set Style Off
        Style |= STYLE_NOADDRESS;
        CheckMenuItem(Menu,IDM_ADDRESS,MF_UNCHECKED);
    }
    else{
        // Set Style On
        Style &= (-1 ^ STYLE_NOADDRESS);
        CheckMenuItem(Menu,IDM_ADDRESS,MF_CHECKED);
    }

    SetWindowLongPtr(RadHWnd,GWL_STYLE,Style);
    SendMessage(RadHWnd,HEM_REPAINT,0,0);
}

void Hide_UnHide_RadHexAscii(HWND hWnd)
{
    HWND RadHWnd = GetDlgItem(hWnd,IDC_RAHEXEDIT);
    HMENU Menu = GetMenu(hWnd);
    DWORD Style=0;

    Style = GetWindowLongPtr(RadHWnd,GWL_STYLE);

    if((GetMenuState(Menu,IDM_ASCII,MF_BYCOMMAND)==MF_CHECKED))
    {
        // Set Style Off
        Style |= STYLE_NOASCII;
        CheckMenuItem(Menu,IDM_ASCII,MF_UNCHECKED);
    }
    else{
        // Set Style On
        Style &= (-1 ^ STYLE_NOASCII);
        CheckMenuItem(Menu,IDM_ASCII,MF_CHECKED);
    }

    SetWindowLongPtr(RadHWnd,GWL_STYLE,Style);
    SendMessage(RadHWnd,HEM_REPAINT,0,0);
}

void UpperLower_caseRadHex(HWND hWnd)
{
    HWND RadHWnd = GetDlgItem(hWnd,IDC_RAHEXEDIT);
    HMENU Menu = GetMenu(hWnd);
    DWORD Style=0;

    Style = GetWindowLongPtr(RadHWnd,GWL_STYLE);

    if((GetMenuState(Menu,IDM_UPPERCASE,MF_BYCOMMAND)==MF_CHECKED))
    {
        // Set Style Off
        Style |= STYLE_NOUPPERCASE;
        CheckMenuItem(Menu,IDM_UPPERCASE,MF_UNCHECKED);
    }
    else{
        // Set Style On
        Style &= (-1 ^ STYLE_NOUPPERCASE);
        CheckMenuItem(Menu,IDM_UPPERCASE,MF_CHECKED);
    }

    SetWindowLongPtr(RadHWnd,GWL_STYLE,Style);
    SendMessage(RadHWnd,HEM_REPAINT,0,0);
}