// Latest error on win64 -- CCCCC on disassembled code

/*
	8888888b.                  888     888 d8b
	888   Y88b                 888     888 Y8P
	888    888                 888     888
	888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888
	8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888
	888        888     888  888  Y88o88P   888 88888888 888  888  888
	888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P
	888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"


				PE Editor & 32Bit Disassembler & File Identifier
				~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

 Written by Shany Golan.
 In January, 2003.
 I have investigated P.E. file format as thoroughly as possible,
 But I cannot claim that I am an expert yet, so some of its information
 May give you wrong results.

 Language used: Visual C++ 6.0
 Date of creation: July 06, 2002
 Date of first release: unknown ??, 2003
 You can contact me: e-mail address: shanytc@gmail.com

 Copyright (C) 2011. By Shany Golan.

 Permission is granted to make and distribute verbatim copies of this
 Program provided the copyright notice and this permission notice are
 Preserved on all copies.

 File: FileMap.cpp (main)
   This program was written by Shany Golan, Student at :
			Ruppin, department of computer science and engineering University.
*/

// ================================================================
// ========================== INCLUDES ============================
// ================================================================

#include "MappedFile.h"
#include "VBPCode.h"
// ================================================================
// =====================  EXTERNAL VARIABLES  =====================
// ================================================================

#ifndef _EXTERNALS_
#define _EXTERNALS_

extern HANDLE				hDisasmThread;   // Thread Handle (incase we want to suspend/stop)
extern IMAGE_SECTION_HEADER *SectionHeader;
extern bool					DisasmIsWorking;
extern HMODULE				hRadHex;
extern DllArray				hDllArray;
extern MapTree				DataAddersses;
extern IntArray				BranchTargets;
extern bool					LoadFirstPass;
extern HBITMAP				hMenuItemBitmap;
extern DWORD				DisasmThreadId;
extern DataMapTree			DataSection;
extern CodeSectionArray		CodeSections;
extern WizardList			WizardCodeList;
extern DataMapTree			DataSection;
extern CodeSectionArray		CodeSections;
extern bool                 g_DarkMode;
extern HBRUSH               g_hDarkBrush;
extern COLORREF             g_DarkBkColor;
extern COLORREF             g_DarkTextColor;
extern COLORREF             g_LightBkColor;
extern COLORREF             g_LightTextColor;
extern DISASM_OPTIONS       disop;

#endif

// ================================================================
// =====================  GLOBAL VARIABLES  =======================
// ================================================================
DisasmDataArray		DisasmDataLines;			// STL Data Container for Disassembly Information
DisasmImportsArray	ImportsLines;				// STL Data Container for Imports Index.
DisasmImportsArray	StrRefLines;				// STL Data Container for String references Index.
FunctionInfo		fFunctionInfo;				// STL Data Container for Function Information used by a plugin
ExportInfoArray		fExportInfo;					// STL Data Container for Export Information
CodeBranch			DisasmCodeFlow;				// STL Data Container for Jumps / Calls in the program.
BranchData			BranchTrace;				// STL Data Container for The Branch Tracing.
XRef				XrefData;					// STL Data Container for Xreferences
CDisasmColor		DisasmColors;				// Disassembler Colors Struct
WNDPROC				OldWndProc;					// Original Window Proc Handler (import editBox)
WNDPROC				LVOldWndProc;				// Window Proc Handler to the Disassembly Listview
WNDPROC				MsgListOldWndProc;			// Window Proc Handler to the Message Listview
WNDPROC				LVXRefOldWndProc;			// Window Proc Handler to the XRef Listview
WNDPROC				ToolTipWndProc;				// Window Proc Handler for the ToolTip listview
HINSTANCE			hInst;						// Main function handler
HDC					dcSkin;						// Window Device Context Handler for the About Window
DWORD_PTR			dwThreadId;					// thread ID 
DWORD_PTR			hFileSize;					// Size of the loaded file
DWORD_PTR			iSelected,iSelectItem;		// Selected item's index in Disassembler list view
HWND				hWndTB,hWin,Main_hWnd,Imp_hWnd,About_hWnd,ToolTip_hWnd;	// Window's Dialog Handlers
HWND				XRefWnd=NULL;				// Handler to the Xref Window - used in ListviewXRefSubclass proc.
LVCOLUMN			LvCol;						// ListView column struct
IMAGE_DOS_HEADER	*doshdr;					// dos header struct
IMAGE_NT_HEADERS	*nt_hdr;					// NT 32Bit header struct
IMAGE_NT_HEADERS64	*nt_hdr64;					// NT 64Bit header struct
IMAGE_OS2_HEADER	*ne_hdr;					// WIN3x NE header struct
HACCEL				hAccel;						// Accelerators handle (shurcut keys)
MSG					msg;						// Message Loop handler
HANDLE				hThread=0;					// hThread handler
HANDLE				hFile;						// Handler to the loaded File
HANDLE				hFileMap;					// Handler to the maped file object
struct				Section *Head=NULL;			// Section's Linked List Pointer
int					ErrorMsg=0;					// Error notifier
int					NumberOfBytesDumped=0;		// Absulote
int					DisasmIndex;				// Index to the current disasm item (when tracing calls/jumps) 
bool				linked=false;				// Absulote 
bool				FilesInMemory=false;		// TRUE: File is memory, FALSE: no file loaded
bool				LoadedPe=false,LoadedPe64=false,LoadedNe=false;// TRUE: 32Bit(PE)/62Bit(PE+)/16Bit(NE) Exe is loaded, FALSE: Other format loaded. 
bool				LoadedNonePe=false;			// TRUE: None 32Bit File is Loaded, FALSE: 32Bit File Loaded
bool				DisassemblerReady=false;	// finished disasm the file true/false
bool				DCodeFlow;					// Read to jump to address from code flow
bool				bToolTip=false;				// Process New Colors when viewing toolTip
bool				bSplitterDragging=false;	// Splitter drag state
int					nSplitterDragY=0;			// Y position when drag started
BYTE				Opcode;						// Absulote 
char				*OrignalData;				// Copy of pointer to memory map data
char				*Data;						// Pointer to the maped file in memory
char				szFileName[MAX_PATH] = "";	// File's Path
char				Buffer[128],Buffer1[128],Buffer2[128],Buffer3[128],Buffer4[256];	// Displaying text on ListView
char				TempAddress[20];			// used when we want to display the address in the tooltip

// ================================================================
// ================ DISASSEMBLER MAIM GUI =========================
// ================================================================
// Forward declaration for header custom draw
LRESULT ProcessHeaderCustomDraw(LPARAM lParam);
LRESULT CALLBACK MessageListSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ================================================================
// ======================  CODE MAP BAR  ==========================
// ================================================================
#define CODE_MAP_BAR_HEIGHT  14  // The color-coded navigation strip
#define CODE_MAP_LEGEND_HEIGHT 16 // Legend text row below the strip
#define CODE_MAP_HEIGHT (CODE_MAP_BAR_HEIGHT + CODE_MAP_LEGEND_HEIGHT) // Total height

// Code map type classification
#define CMAP_CODE      0
#define CMAP_FUNCTION  1
#define CMAP_IMPORT    2
#define CMAP_DATA      3

// Code map colors (light mode)
#define CODEMAP_COLOR_CODE       RGB(255, 255, 192)  // Light yellow - plain code
#define CODEMAP_COLOR_FUNCTION   RGB(0, 0, 180)      // Dark blue - function body
#define CODEMAP_COLOR_IMPORT     RGB(0, 180, 255)    // Cyan - import call
#define CODEMAP_COLOR_DATA       RGB(128, 128, 128)  // Gray - data
#define CODEMAP_COLOR_BG         RGB(220, 220, 220)  // Light gray - background

// Code map colors (dark mode)
#define CODEMAP_COLOR_CODE_DK    RGB(180, 180, 120)
#define CODEMAP_COLOR_FUNCTION_DK RGB(0, 0, 140)
#define CODEMAP_COLOR_IMPORT_DK  RGB(0, 140, 200)
#define CODEMAP_COLOR_DATA_DK    RGB(100, 100, 100)
#define CODEMAP_COLOR_BG_DK      RGB(50, 50, 50)

static std::vector<BYTE> g_CodeMapTypes;  // One byte per DisasmDataLines entry
static bool g_CodeMapClassRegistered = false;

// Forward declarations
LRESULT CALLBACK CodeMapWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void BuildCodeMapData();
void RepositionDisasmForCodeMap(HWND hWnd, BOOL show);

// Helper: get the Y pixel position where the disassembly listview starts
static int GetDisasmTopY(HWND hWnd)
{
    HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
    if (!hDisasm) return 22; // fallback
    RECT rc;
    GetWindowRect(hDisasm, &rc);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rc, 2);
    return rc.top;
}

// ================================================================
// =================  CODE MAP IMPLEMENTATION  ====================
// ================================================================

void BuildCodeMapData()
{
    g_CodeMapTypes.clear();
    if (DisasmDataLines.size() == 0) return;

    size_t count = DisasmDataLines.size();
    g_CodeMapTypes.resize(count, CMAP_CODE);

    // Build a sorted set of import line indices for fast lookup
    std::vector<bool> isImport(count, false);
    for (size_t i = 0; i < ImportsLines.size(); i++) {
        int idx = ImportsLines[i];
        if (idx >= 0 && idx < (int)count)
            isImport[idx] = true;
    }

    // Build a sorted set of data addresses for fast lookup
    // DataAddersses is a multimap<const int, int> keyed by address
    std::vector<bool> isData(count, false);
    if (DataAddersses.size() > 0) {
        for (size_t i = 0; i < count; i++) {
            char* addrStr = DisasmDataLines[i].GetAddress();
            if (addrStr && addrStr[0]) {
                DWORD_PTR addr = (DWORD_PTR)strtoul(addrStr, NULL, 16);
                if (DataAddersses.find((int)addr) != DataAddersses.end())
                    isData[i] = true;
            }
        }
    }

    // Classify each entry
    for (size_t i = 0; i < count; i++) {
        if (isImport[i]) {
            g_CodeMapTypes[i] = CMAP_IMPORT;
        } else if (isData[i]) {
            g_CodeMapTypes[i] = CMAP_DATA;
        } else {
            // Check if inside a known function
            char* addrStr = DisasmDataLines[i].GetAddress();
            if (addrStr && addrStr[0]) {
                DWORD_PTR addr = (DWORD_PTR)strtoul(addrStr, NULL, 16);
                for (size_t j = 0; j < fFunctionInfo.size(); j++) {
                    if (addr >= fFunctionInfo[j].FunctionStart && addr <= fFunctionInfo[j].FunctionEnd) {
                        g_CodeMapTypes[i] = CMAP_FUNCTION;
                        break;
                    }
                }
            }
        }
    }
}

static COLORREF GetCodeMapColor(BYTE type)
{
    if (g_DarkMode) {
        switch (type) {
            case CMAP_FUNCTION: return CODEMAP_COLOR_FUNCTION_DK;
            case CMAP_IMPORT:   return CODEMAP_COLOR_IMPORT_DK;
            case CMAP_DATA:     return CODEMAP_COLOR_DATA_DK;
            default:            return CODEMAP_COLOR_CODE_DK;
        }
    } else {
        switch (type) {
            case CMAP_FUNCTION: return CODEMAP_COLOR_FUNCTION;
            case CMAP_IMPORT:   return CODEMAP_COLOR_IMPORT;
            case CMAP_DATA:     return CODEMAP_COLOR_DATA;
            default:            return CODEMAP_COLOR_CODE;
        }
    }
}

// Draw a small colored square followed by label text; returns the X position after it
static int DrawLegendItem(HDC hdc, int x, int y, int height, COLORREF color, const char* label)
{
    int swatchSize = height - 2;
    int swatchY = y + 1;

    // Draw color swatch
    RECT swatchRc = { x, swatchY, x + swatchSize, swatchY + swatchSize };
    HBRUSH hBr = CreateSolidBrush(color);
    FillRect(hdc, &swatchRc, hBr);
    DeleteObject(hBr);
    // Swatch border
    HPEN hPen = CreatePen(PS_SOLID, 1, g_DarkMode ? RGB(120, 120, 120) : RGB(80, 80, 80));
    HPEN hOld = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, x, swatchY, x + swatchSize, swatchY + swatchSize);
    SelectObject(hdc, hOld);
    SelectObject(hdc, hOldBr);
    DeleteObject(hPen);

    // Draw label text
    RECT textRc = { x + swatchSize + 2, y, x + swatchSize + 200, y + height };
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, g_DarkMode ? RGB(180, 180, 180) : RGB(0, 0, 0));
    DrawTextA(hdc, label, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    SIZE sz;
    GetTextExtentPoint32A(hdc, label, lstrlenA(label), &sz);
    return x + swatchSize + 2 + sz.cx + 8;  // 8px gap between items
}

LRESULT CALLBACK CodeMapWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1;  // We paint the entire surface in WM_PAINT

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            int totalWidth = rc.right - rc.left;
            int totalHeight = rc.bottom - rc.top;

            // Fill entire background
            COLORREF bgColor = g_DarkMode ? CODEMAP_COLOR_BG_DK : CODEMAP_COLOR_BG;
            HBRUSH hBgBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &rc, hBgBrush);
            DeleteObject(hBgBrush);

            // --- Draw the color bar in the top portion ---
            int barHeight = CODE_MAP_BAR_HEIGHT;
            size_t count = DisasmDataLines.size();
            if (count > 0 && g_CodeMapTypes.size() == count && totalWidth > 0) {
                COLORREF prevColor = 0;
                int runStart = 0;

                for (int x = 0; x <= totalWidth; x++) {
                    COLORREF color = 0;
                    if (x < totalWidth) {
                        size_t startIdx = (size_t)((double)x / totalWidth * count);
                        size_t endIdx   = (size_t)((double)(x + 1) / totalWidth * count);
                        if (endIdx > count) endIdx = count;
                        if (startIdx >= count) startIdx = count - 1;
                        if (endIdx <= startIdx) endIdx = startIdx + 1;

                        BYTE bestType = CMAP_CODE;
                        for (size_t i = startIdx; i < endIdx; i++) {
                            BYTE t = g_CodeMapTypes[i];
                            if (t == CMAP_IMPORT) { bestType = CMAP_IMPORT; break; }
                            if (t == CMAP_FUNCTION && bestType < CMAP_FUNCTION) bestType = CMAP_FUNCTION;
                            if (t == CMAP_DATA && bestType < CMAP_DATA) bestType = CMAP_DATA;
                        }
                        color = GetCodeMapColor(bestType);
                    }

                    if (x == 0) {
                        prevColor = color;
                        runStart = 0;
                    } else if (color != prevColor || x == totalWidth) {
                        RECT fillRc = { runStart, 0, x, barHeight };
                        HBRUSH hBr = CreateSolidBrush(prevColor);
                        FillRect(hdc, &fillRc, hBr);
                        DeleteObject(hBr);
                        prevColor = color;
                        runStart = x;
                    }
                }

                // Draw current view position indicator
                HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
                if (hDisasm) {
                    int topIndex = (int)SendMessage(hDisasm, LVM_GETTOPINDEX, 0, 0);
                    int visibleCount = (int)SendMessage(hDisasm, LVM_GETCOUNTPERPAGE, 0, 0);

                    int x1 = (int)((double)topIndex / count * totalWidth);
                    int x2 = (int)((double)(topIndex + visibleCount) / count * totalWidth);
                    if (x2 <= x1) x2 = x1 + 2;
                    if (x2 > totalWidth) x2 = totalWidth;

                    HPEN hIndicatorPen = CreatePen(PS_SOLID, 1, g_DarkMode ? RGB(255, 255, 255) : RGB(0, 0, 0));
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hIndicatorPen);
                    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    Rectangle(hdc, x1, 0, x2, barHeight);
                    SelectObject(hdc, hOldPen);
                    SelectObject(hdc, hOldBrush);
                    DeleteObject(hIndicatorPen);

                    // Draw small arrow indicator at the current cursor position
                    int cursorIdx = (int)SendMessage(hDisasm, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                    if (cursorIdx >= 0 && cursorIdx < (int)count) {
                        int cx = (int)((double)cursorIdx / count * totalWidth);
                        // Small downward-pointing triangle (5px wide, 4px tall)
                        POINT arrow[3] = {
                            { cx - 4, 0 },
                            { cx + 4, 0 },
                            { cx, 5 }
                        };
                        HBRUSH hArrowBrush = CreateSolidBrush(RGB(255, 255, 0));
                        HPEN hArrowPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                        HPEN hOldPen2 = (HPEN)SelectObject(hdc, hArrowPen);
                        HBRUSH hOldBr2 = (HBRUSH)SelectObject(hdc, hArrowBrush);
                        Polygon(hdc, arrow, 3);
                        SelectObject(hdc, hOldPen2);
                        SelectObject(hdc, hOldBr2);
                        DeleteObject(hArrowPen);
                        DeleteObject(hArrowBrush);
                    }
                }
            }

            // --- Draw legend row below the bar ---
            int legendY = barHeight;
            HFONT hSmallFont = CreateFontA(CODE_MAP_LEGEND_HEIGHT - 1, 0, 0, 0, FW_NORMAL,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "MS Shell Dlg");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hSmallFont);

            int xPos = 4;
            xPos = DrawLegendItem(hdc, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                g_DarkMode ? CODEMAP_COLOR_FUNCTION_DK : CODEMAP_COLOR_FUNCTION, "Function");
            xPos = DrawLegendItem(hdc, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                g_DarkMode ? CODEMAP_COLOR_IMPORT_DK : CODEMAP_COLOR_IMPORT, "Import");
            xPos = DrawLegendItem(hdc, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                g_DarkMode ? CODEMAP_COLOR_DATA_DK : CODEMAP_COLOR_DATA, "Data");
            xPos = DrawLegendItem(hdc, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                g_DarkMode ? CODEMAP_COLOR_CODE_DK : CODEMAP_COLOR_CODE, "Code");

            SelectObject(hdc, hOldFont);
            DeleteObject(hSmallFont);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetCapture(hWnd);
            // Fall through to mouse move handling
        }
        case WM_MOUSEMOVE: {
            if (uMsg == WM_MOUSEMOVE && !(wParam & MK_LBUTTON))
                break;

            RECT rc;
            GetClientRect(hWnd, &rc);
            int barWidth = rc.right - rc.left;
            int mouseX = (short)LOWORD(lParam);

            size_t count = DisasmDataLines.size();
            if (count > 0 && barWidth > 0) {
                if (mouseX < 0) mouseX = 0;
                if (mouseX >= barWidth) mouseX = barWidth - 1;
                DWORD_PTR index = (DWORD_PTR)((double)mouseX / barWidth * count);
                if (index >= count) index = count - 1;

                HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
                if (hDisasm) {
                    SelectItem(hDisasm, index);
                    ListView_EnsureVisible(hDisasm, (int)index, FALSE);
                }
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            ReleaseCapture();
            return 0;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void RefreshCodeMapBar()
{
    if (!g_CodeMapVisible || !Main_hWnd) return;
    BuildCodeMapData();
    HWND hBar = GetDlgItem(Main_hWnd, IDC_CODE_MAP_BAR);
    if (!hBar) return;

    // Auto-show the bar if it isn't visible yet (first disassembly after launch/close)
    if (!IsWindowVisible(hBar)) {
        RepositionDisasmForCodeMap(Main_hWnd, TRUE);
        ShowWindow(hBar, SW_SHOW);
    }
    InvalidateRect(hBar, NULL, TRUE);
}

void RepositionDisasmForCodeMap(HWND hWnd, BOOL show)
{
    HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
    HWND hBar = GetDlgItem(hWnd, IDC_CODE_MAP_BAR);
    if (!hDisasm) return;

    RECT rc;
    GetWindowRect(hDisasm, &rc);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rc, 2);

    if (show) {
        // Place code map at the disasm listview's current top
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        if (hBar) MoveWindow(hBar, 0, rc.top, rcClient.right, CODE_MAP_HEIGHT, TRUE);
        // Push listview down
        MoveWindow(hDisasm, rc.left, rc.top + CODE_MAP_HEIGHT,
                   rc.right - rc.left, (rc.bottom - rc.top) - CODE_MAP_HEIGHT, TRUE);
    } else {
        // Pull listview back up
        MoveWindow(hDisasm, rc.left, rc.top - CODE_MAP_HEIGHT,
                   rc.right - rc.left, (rc.bottom - rc.top) + CODE_MAP_HEIGHT, TRUE);
    }

    // Re-initialize resize anchors so future WM_SIZE works correctly
    InitializeResizeControls(hWnd);
}

// Stretch last column of ListView to fill remaining width (fixes white areas in dark mode)
void StretchLastColumn(HWND hListView)
{
    if (!hListView) return;

    RECT rc;
    GetClientRect(hListView, &rc);
    int listWidth = rc.right - rc.left;

    // Get header to count columns
    HWND hHeader = ListView_GetHeader(hListView);
    if (!hHeader) return;

    int columnCount = Header_GetItemCount(hHeader);
    if (columnCount <= 0) return;

    // Calculate sum of all columns except the last one
    int usedWidth = 0;
    for (int i = 0; i < columnCount - 1; i++) {
        usedWidth += ListView_GetColumnWidth(hListView, i);
    }

    // Only subtract scrollbar width if vertical scrollbar is actually visible
    int scrollbarWidth = 0;
    SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL };
    if (GetScrollInfo(hListView, SB_VERT, &si)) {
        // Scrollbar is visible if max > page (more items than can fit)
        if (si.nMax >= (int)si.nPage && si.nPage > 0) {
            scrollbarWidth = GetSystemMetrics(SM_CXVSCROLL);
        }
    }

    // Set last column to fill remaining space
    int lastColWidth = listWidth - usedWidth - scrollbarWidth;
    if (lastColWidth > 50) {  // Minimum width
        ListView_SetColumnWidth(hListView, columnCount - 1, lastColWidth);
    }
}

BOOL CALLBACK DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Check What Event we are going to process
	switch (message){ // what are we doing ?
		//////////////////////////////////////////////////////////////////////////
		//						PLUGIN SDK MESSAGES								//
		//////////////////////////////////////////////////////////////////////////
		//
		// This Message Returns an Disassembled Vector
		// Pointed by wParam, to the Struct Pointed by lParam.
		//
		case PI_GETASM:{
			DISASSEMBLY *DisasmData = (DISASSEMBLY*)lParam; // Create Pointer
			DWORD_PTR Index=0;
			try{
				FlushDecoded(DisasmData);
				DisasmData->Address = nt_hdr->OptionalHeader.ImageBase;
				DisasmData->Address += SectionHeader->VirtualAddress;
				Decode(DisasmData,(char*)wParam,&Index);
			}
			catch(...){}
		}
		break;

		//
		// This messages clears the Disassembly struct.
		// Pointed by lParam.
		//
		case PI_FLUSHDISASM:{
			DISASSEMBLY *DisasmData = (DISASSEMBLY*)lParam;
			try{
				FlushDecoded(DisasmData);
			}
			catch(...){}
		}
		break;

		//
		// This Message returns the Start & End of the
		// Current Selected line at the disassembler Window.
		//
		case PI_GETFUNCTIONEPFROMINDEX:{
			DWORD_PTR* EndAddr   =(DWORD_PTR*)lParam;
			DWORD_PTR* StartAddr =(DWORD_PTR*)wParam;
			
			try{
				// Check for Valid Index.
				if(*StartAddr==-1){
					break;
				}

				// Get Current Address
				*StartAddr=StringToDword(DisasmDataLines[*StartAddr].GetAddress());

				// Search for the Function's Boundries.
				for(int i=0;i<fFunctionInfo.size();i++){
					if( (*StartAddr>=fFunctionInfo[i].FunctionStart) && (*StartAddr<=fFunctionInfo[i].FunctionEnd) ){
						*StartAddr = fFunctionInfo[i].FunctionStart; // Return the Start Address.
						*EndAddr = fFunctionInfo[i].FunctionEnd;  // Return the End Address.
						break;
					}
				}
			}
			catch(...){ }
		}
		break;
		
		//
		// This Message Returns the disassembled information
		// for Index Specified at wParam.
		//
		case PI_GETASMFROMINDEX:{
			// wParam = Index data to retrieve disasm data
			// (DISASSEMBLY*)lParam = Pointer to the Disasm Struct to get the data
			DISASSEMBLY *DisasmData = (DISASSEMBLY*)lParam;
			// Intialize struct
            try{
				FlushDecoded(DisasmData);
				DisasmData->Address=StringToDword(DisasmDataLines[wParam].GetAddress());
				DisasmData->OpcodeSize=DisasmDataLines[wParam].GetSize();
				wsprintf(DisasmData->Assembly,"%s",DisasmDataLines[wParam].GetMnemonic());
				wsprintf(DisasmData->Opcode,"%s",DisasmDataLines[wParam].GetCode());
				wsprintf(DisasmData->Remarks,"%s",DisasmDataLines[wParam].GetComments());
				for(DWORD_PTR index=0;index<DisasmCodeFlow.size();index++){
					if(wParam==DisasmCodeFlow[index].Current_Index){
						DisasmData->CodeFlow.Call=DisasmCodeFlow[index].BranchFlow.Call;
						DisasmData->CodeFlow.Jump=DisasmCodeFlow[index].BranchFlow.Jump;
						DisasmData->CodeFlow.Ret=DisasmCodeFlow[index].BranchFlow.Ret;
						DisasmData->CodeFlow.BranchSize = DisasmCodeFlow[index].BranchFlow.BranchSize;
						break;
					}
				}
			}
			catch(...){ }
		}
		break;

        // This Message Returns the EntryPoint of the EXE
        case PI_GETENTRYPOINT:{
          DWORD_PTR *EntryPoint=(DWORD_PTR*)wParam;
          *EntryPoint=nt_hdr->OptionalHeader.AddressOfEntryPoint+nt_hdr->OptionalHeader.ImageBase;
        }
        break;

        // Put text on the debug output window from the plugin
        case PI_PRINTDBGTEXT:{
          OutDebug(hWnd,(char*)wParam);
        }
        break;

        // Get Byte From Address
        case PI_GETBYTEFROMADDRESS:{   
            DWORD_PTR Address=(DWORD_PTR)wParam;
            BYTE*ptr_data=(BYTE*)lParam;
            bool error;
            IMAGE_SECTION_HEADER* secheader=0;
            Address-=nt_hdr->OptionalHeader.ImageBase;
            secheader=RVAToOffset(nt_hdr,secheader,Address,&error);            
            Address-=secheader->VirtualAddress;
            DWORD_PTR offset=secheader->PointerToRawData+Address;
            *ptr_data=*(OrignalData+offset);
        }
        break;

        // Return Number of analyzed String References
        case PI_GETNUMOFSTRINGREF:{
          DWORD_PTR *NumOfStrRef=(DWORD_PTR*)wParam;
          *NumOfStrRef = StrRefLines.size();
        }
        break;

        // Convert RVA to Offset
        case PI_RVATOFFSET:{
            DWORD_PTR Address=(DWORD_PTR)wParam;
            DWORD_PTR* offset=(DWORD_PTR*)lParam;
            bool error;
            IMAGE_SECTION_HEADER* secheader=0;
            Address-=nt_hdr->OptionalHeader.ImageBase;
            secheader=RVAToOffset(nt_hdr,secheader,Address,&error);
            BYTE*ptr_data=(BYTE*)lParam;
            Address-=secheader->VirtualAddress;
            *offset=secheader->PointerToRawData+Address;
        }
        break;

        // Get String reference (from index)
        case PI_GETSTRINGREFERENCE:{
            DWORD_PTR Index = (DWORD_PTR)wParam;
            CHAR* StrRef = (CHAR*)lParam;
            char *ptr,temp[128],Api[128];

            ItrImports itr=StrRefLines.begin();
            for(DWORD_PTR i=0;i<Index;i++){
                itr++;
            }

            strcpy_s(temp,StringLen(DisasmDataLines[(*itr)].GetComments())+1,DisasmDataLines[(*itr)].GetComments());
            ptr=temp;
			while(*ptr!=':'){
                ptr++;
			}
            ptr+=2;
            strcpy_s(Api,StringLen(ptr)+1,ptr);
            int len=StringLen(Api);
			if(Api[len-1]==']'){
                Api[len-1]=NULL;
			}

            strcpy_s((char*)StrRef,StringLen(Api)+1,Api);
        }
        break;

        // Replace Current Comment
        case PI_SETCOMMENT:{
           char* cComment = (char*)lParam;
           DWORD Index = (DWORD)wParam;

           DisasmDataLines[Index].SetComments(cComment);
           RedrawWindow(GetDlgItem(hWnd,IDC_DISASM),NULL,NULL,TRUE);
        }
        break;

        //
        // Add Comment to Existing Comment.
        //
        case PI_ADDCOMMENT:{
            char* cComment = (char*)lParam;
            DWORD Index = (DWORD)wParam;

            char *temp=new char[StringLen(cComment)+StringLen(DisasmDataLines[Index].GetComments())+2];
            if(StringLen(temp)>255){
                delete temp;
                break;
            }
            wsprintf(temp,"%s %s",DisasmDataLines[Index].GetComments(),cComment);
            DisasmDataLines[Index].SetComments(temp);
            RedrawWindow(GetDlgItem(hWnd,IDC_DISASM),NULL,NULL,TRUE);
            delete temp;
        }
        break;

		case PI_ADDFUNCTIONNAME:{
			FUNCTION_INFORMATION *FuncInfo = (FUNCTION_INFORMATION*)lParam;
			FUNCTION_INFORMATION iFuncInfo;

			if(FuncInfo==NULL){
				break;
			}
			
			iFuncInfo.FunctionEnd=FuncInfo->FunctionEnd;
			iFuncInfo.FunctionStart=FuncInfo->FunctionStart;
			lstrcpy(iFuncInfo.FunctionName,FuncInfo->FunctionName);
			fFunctionInfo.insert(fFunctionInfo.end(),iFuncInfo);
		}
		break;

		case PI_GETFUNCTIONNAME:{
			DWORD	SearchAddress=(DWORD)lParam;
			char*	FunctionName=(char*)wParam;
		 
			try{
				for(int i=0;i<fFunctionInfo.size();i++){
					if(SearchAddress>=fFunctionInfo[i].FunctionStart && SearchAddress<=fFunctionInfo[i].FunctionEnd){
						strcpy_s(FunctionName,StringLen(fFunctionInfo[i].FunctionName)+1,fFunctionInfo[i].FunctionName);
						break;
					}
				}
			}
			catch(...){}
		}
		break;

		case PI_GETCODESEGMENTSTARTEND:{
			DWORD*	CodeSegmentStart=(DWORD*)wParam;
			DWORD*	CodeSegmentEnd=(DWORD*)lParam;
			*CodeSegmentStart=SectionHeader->VirtualAddress+nt_hdr->OptionalHeader.ImageBase;
			*CodeSegmentEnd=SectionHeader->VirtualAddress+SectionHeader->SizeOfRawData+nt_hdr->OptionalHeader.ImageBase;
		}
		break;

		// Get total of exports
		case PI_GET_NUMBER_OF_EXPORTS:{
			DWORD*	TotalImports=(DWORD*)lParam;
			*TotalImports = GetNumberOfExports(OrignalData);
		}
		break;

		// Get the export Name by Index
		case PI_GET_EXPORT_NAME:{
			DWORD	ExportIndex=(DWORD)wParam;
			char*	ExportName=(char*)lParam;
			GetExportName(OrignalData,ExportIndex,ExportName);
		}
		break;

		// Get Export's Index in the Disassembler List
		case PI_GET_EXPORT_DASM_INDEX:{
			DWORD	ExportIndex=(DWORD)wParam;
			DWORD*	ExportDasmIndex=(DWORD*)lParam;
			*ExportDasmIndex = GetExportDisasmIndex(OrignalData, ExportIndex);
		}
		break;

		// Get Ordinal By Index
		case PI_GET_EXPORT_ORDINAL:{
			DWORD	ExportIndex=(DWORD)wParam;
			DWORD*	ExportOrdinal=(DWORD*)lParam;
			*ExportOrdinal = GetExportOrdinal(OrignalData, ExportIndex);
		}
		break;

		//////////////////////////////////////////////////////////////////////
        //				End of Plugin Handler Routines						//
		//////////////////////////////////////////////////////////////////////


		// Dark mode: paint dialog and control backgrounds
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:{
			if (g_hDarkBrush) {
				HDC hdcStatic = (HDC)wParam;
				if (g_DarkMode) {
					SetTextColor(hdcStatic, g_DarkTextColor);
					SetBkColor(hdcStatic, g_DarkBkColor);
				} else {
					SetTextColor(hdcStatic, g_LightTextColor);
					SetBkColor(hdcStatic, g_LightBkColor);
				}
				return (INT_PTR)g_hDarkBrush;
			}
		}
		break;

		// auto reinitialize controls
		case WM_SHOWWINDOW:{
			InitializeResizeControls(hWnd);
		}
		break;	

		// resizing the window auto resize the specified
		// controls on the window
		case WM_SIZE:{
			ResizeControls(hWnd);
			// Resize Code Map bar to match window width and repaint
			if (g_CodeMapVisible) {
				HWND hBar = GetDlgItem(hWnd, IDC_CODE_MAP_BAR);
				if (hBar && IsWindowVisible(hBar)) {
					RECT rcBar;
					GetWindowRect(hBar, &rcBar);
					MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rcBar, 2);
					RECT rcClient;
					GetClientRect(hWnd, &rcClient);
					MoveWindow(hBar, 0, rcBar.top, rcClient.right, CODE_MAP_HEIGHT, TRUE);
					InvalidateRect(hBar, NULL, FALSE);
				}
			}
			// Stretch last column to fill width (fixes white areas in dark mode)
			StretchLastColumn(GetDlgItem(hWnd, IDC_DISASM));
			StretchLastColumn(GetDlgItem(hWnd, IDC_LIST));
		}
		break;

		// Show resize cursor when hovering over splitter
		case WM_SETCURSOR:{
			POINT pt;
			RECT splitterRect;
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt);
			GetWindowRect(GetDlgItem(hWnd, IDC_SPLITTER), &splitterRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&splitterRect, 2);
			// Expand hit area slightly for easier grabbing
			splitterRect.top -= 2;
			splitterRect.bottom += 2;
			if (PtInRect(&splitterRect, pt)) {
				SetCursor(LoadCursor(NULL, IDC_SIZENS));
				return TRUE;
			}
		}
		break;

		case WM_MOUSEMOVE:{
			if (bSplitterDragging) {
				int newY = (short)HIWORD(lParam);
				int deltaY = newY - nSplitterDragY;
				if (deltaY != 0) {
					RECT disasmRect, splitterRect, listRect;
					HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
					HWND hSplitter = GetDlgItem(hWnd, IDC_SPLITTER);
					HWND hList = GetDlgItem(hWnd, IDC_LIST);

					GetWindowRect(hDisasm, &disasmRect);
					GetWindowRect(hSplitter, &splitterRect);
					GetWindowRect(hList, &listRect);
					MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&disasmRect, 2);
					MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&splitterRect, 2);
					MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&listRect, 2);

					// Calculate new heights with limits
					int newDisasmHeight = (disasmRect.bottom - disasmRect.top) + deltaY;
					int newListHeight = (listRect.bottom - listRect.top) - deltaY;
					int minHeight = 30;

					if (newDisasmHeight >= minHeight && newListHeight >= minHeight) {
						// Use deferred window positioning for smoother updates
						HDWP hdwp = BeginDeferWindowPos(3);
						if (hdwp) {
							// Resize disassembly view
							hdwp = DeferWindowPos(hdwp, hDisasm, NULL, 0, 0,
								disasmRect.right - disasmRect.left,
								newDisasmHeight,
								SWP_NOMOVE | SWP_NOZORDER);

							// Move splitter
							hdwp = DeferWindowPos(hdwp, hSplitter, NULL,
								splitterRect.left,
								splitterRect.top + deltaY,
								0, 0, SWP_NOSIZE | SWP_NOZORDER);

							// Move and resize history list
							hdwp = DeferWindowPos(hdwp, hList, NULL,
								listRect.left,
								listRect.top + deltaY,
								listRect.right - listRect.left,
								newListHeight,
								SWP_NOZORDER);

							EndDeferWindowPos(hdwp);
						}
						nSplitterDragY = newY;
					}
				}
			}
		}
		break;

		case WM_LBUTTONUP:{
			if (bSplitterDragging) {
				bSplitterDragging = false;
				ReleaseCapture();
				// Update column widths after drag completes
				StretchLastColumn(GetDlgItem(hWnd, IDC_DISASM));
				StretchLastColumn(GetDlgItem(hWnd, IDC_LIST));
			}
		}
		break;

        // User Defined Message (Sent from the Disassembler Core)
        case WM_USER+101:{
            if(LoadedPe64)
                wsprintf(Buffer,"Disassembling Address: %08X%08X",(DWORD)((DWORD_PTR)lParam>>32),(DWORD)lParam);
            else
                wsprintf(Buffer,"Disassembling Address: %08X",lParam);
            SetDlgItemText(hWnd,IDC_MESSAGE1,Buffer);
        }
        break;

        // Process First Load Event
		case WM_INITDIALOG:{
            Main_hWnd = hWnd; // create a global copy

			// Load The Found Plug-ins (If Found)
            LoadPlugins(hWnd);

            // Load dark mode settings
            LoadSettings();
            if (g_DarkMode) {
                ApplyDarkMode(true);
                CheckMenuItem(GetMenu(hWnd), IDC_DARK_MODE, MF_CHECKED);
            }

			SetWindowText(hWnd,PVDASM);
            // Set Window Icon
			SendMessageA(hWnd,WM_SETICON,(WPARAM) 1,(LPARAM) LoadIconA(hInst,MAKEINTRESOURCE(IDI_EXE)));

			// Make the controls in main window sizable/movable
			SetWindowLongPtr(GetDlgItem(hWnd,IDC_DISASM),			GWL_USERDATA,ANCHOR_RIGHT|ANCHOR_BOTTOM);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_LIST),				GWL_USERDATA,ANCHOR_RIGHT|ANCHOR_BOTTOM|ANCHOR_NOT_TOP);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_MESSAGE1),			GWL_USERDATA,ANCHOR_BOTTOM|ANCHOR_RIGHT|ANCHOR_NOT_TOP);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_MESSAGE2),			GWL_USERDATA,ANCHOR_NOT_LEFT|ANCHOR_BOTTOM|ANCHOR_NOT_TOP);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_MESSAGE3),			GWL_USERDATA,ANCHOR_NOT_LEFT|ANCHOR_BOTTOM|ANCHOR_NOT_TOP);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_DISASM_PROGRESS),	GWL_USERDATA,ANCHOR_NOT_LEFT|ANCHOR_BOTTOM|ANCHOR_NOT_TOP);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_TOOLBAR_SEP),		GWL_USERDATA,ANCHOR_RIGHT);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_TOOLBAR_SEP2),		GWL_USERDATA,ANCHOR_RIGHT);
            SetWindowLongPtr(GetDlgItem(hWnd,IDC_SPLITTER),			GWL_USERDATA,ANCHOR_RIGHT|ANCHOR_BOTTOM|ANCHOR_NOT_TOP);

			// Initialize Debug View Control
			ZeroMemory(&LvCol,0);							// Zero Members
			LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;	// Type of mask
			LvCol.pszText="History";
			LvCol.cx=50;									// Small initial width
			SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);
			// ListView styles
			SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT|LVS_EX_ONECLICKACTIVATE|LVS_EX_DOUBLEBUFFER);
			// Auto-size column to fill width
			ListView_SetColumnWidth(GetDlgItem(hWnd,IDC_LIST), 0, LVSCW_AUTOSIZE_USEHEADER);
			
			// create the columns and initialize the list view
			InitDisasmWindow(hWnd);

			// Set list view colors (respect dark mode setting)
			if (g_DarkMode) {
				SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETBKCOLOR,0,(LPARAM)g_DarkBkColor);
				SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETTEXTBKCOLOR,0,(LPARAM)g_DarkBkColor);
				SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETTEXTCOLOR,0,(LPARAM)g_DarkTextColor);
			} else {
				SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETBKCOLOR,0,(LPARAM)RGB(255,255,255));
				SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETTEXTBKCOLOR,0,(LPARAM)RGB(255,255,255));
				SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETTEXTCOLOR,0,(LPARAM)RGB(0,0,0));
			}
			
			// Disable Menu items at first run
			EnableMenuItem(GetMenu(hWnd),ID_PE_EDIT,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_CLOSEFILE,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_GOTO_START,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_GOTO_ENTRYPOINT,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_GOTO_ADDRESS,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),ID_GOTO_JUMP,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),ID_GOTO_RETURN_JUMP,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),ID_GOTO_EXECUTECALL,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),ID_GOTO_RETURN_CALL,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_START_DISASM,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_STOP_DISASM,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_DISASM_IMP,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_STR_REFERENCES,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_SHOW_COMMENTS,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_XREF_MENU,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDM_PATCHER,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_DISASM_ADD_COMMENT,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_PVSCRIPT_ENGINE,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDM_SAVE_PROJECT,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_PRODUCE_MASM,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_PRODUCE_VIEW,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_EXPORT_MAP_PVDASM,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_VIEW_CFG,MF_GRAYED);

			// Create the ToolBar.
			hWndTB=CreateToolBar(hWnd,hInst);

            // Check if Add-in has Loaded.
			if(hRadHex==NULL){
                EnableMenuItem(GetMenu(hWnd),IDC_HEX_EDITOR,MF_GRAYED);
			}

            // Hide the Progress bar (Show only when disassembling).
            ShowWindow(GetDlgItem(hWnd,IDC_DISASM_PROGRESS),SW_HIDE);

            // Register and create the Code Map bar
            if (!g_CodeMapClassRegistered) {
                WNDCLASSA wc = {0};
                wc.lpfnWndProc = CodeMapWndProc;
                wc.hInstance = hInst;
                wc.hCursor = LoadCursor(NULL, IDC_ARROW);
                wc.lpszClassName = "PVDasmCodeMap";
                RegisterClassA(&wc);
                g_CodeMapClassRegistered = true;
            }
            {
                // Place code map right at the disassembly listview's current top
                int cmapY = GetDisasmTopY(hWnd);
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);
                HWND hCodeMap = CreateWindowExA(0, "PVDasmCodeMap", NULL,
                    WS_CHILD,  // Initially hidden
                    0, cmapY, rcClient.right, CODE_MAP_HEIGHT,
                    hWnd, (HMENU)(UINT_PTR)IDC_CODE_MAP_BAR, hInst, NULL);
                SetWindowLongPtr(hCodeMap, GWL_USERDATA, ANCHOR_RIGHT);
            }

            // Check the menu item if Code Map is enabled (bar stays hidden until disassembly completes)
            if (g_CodeMapVisible) {
                CheckMenuItem(GetMenu(hWnd), IDC_CODE_MAP, MF_CHECKED);
            }

            // Subclass the disassembly window
            LVOldWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_DISASM),GWL_WNDPROC,(LONG_PTR)ListViewSubClass);
            MsgListOldWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_LIST),GWL_WNDPROC,(LONG_PTR)MessageListSubClass);

			// Show window in normal state (not maximized)
			ShowWindow(hWnd,SW_NORMAL);

            // Update Window
            UpdateWindow(hWnd);

			// Accelerators
			hAccel = LoadAccelerators (hInst, MAKEINTRESOURCE(IDR_ACCELERATOR1));

            ////////////////////////////////////////////////// 
            //				Message Pump					//
            //////////////////////////////////////////////////

            // Get the waiting Message
            while (GetMessage(&msg, NULL, 0, 0)){
                // Handle modeless CFG viewer dialog messages
                HWND hCFGViewer = GetCFGViewerWindow();
                if (hCFGViewer && IsDialogMessage(hCFGViewer, &msg)) {
                    continue;
                }
                // Translate Accelerator Keys
				if (!TranslateAccelerator(hWnd, hAccel, &msg)){
					TranslateMessage (&msg);
				}
                // Dispatch/Process The Waiting Message
                DispatchMessage(&msg);
			}
		}
		break;

	case WM_NOTIFY:{ // Notify Messages
        switch(LOWORD(wParam)){
            //Disassembler Control (Virtual List View)
                    // Message Window ListView (Dark Mode)
                    case IDC_LIST: {
                        LPNMHDR pnmh = (LPNMHDR)lParam;
                        if (pnmh->code == NM_CUSTOMDRAW && g_DarkMode) {
                            HWND hListView = GetDlgItem(hWnd, IDC_LIST);
                            HWND hHeader = ListView_GetHeader(hListView);
                            if (pnmh->hwndFrom == hHeader) {
                                LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;
                                switch (nmcd->dwDrawStage) {
                                    case CDDS_PREPAINT:
                                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                                        return TRUE;
                                    case CDDS_ITEMPREPAINT: {
                                        HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
                                        FillRect(nmcd->hdc, &nmcd->rc, hBrush);
                                        DeleteObject(hBrush);
                                        SetTextColor(nmcd->hdc, g_DarkTextColor);
                                        SetBkMode(nmcd->hdc, TRANSPARENT);
                                        HDITEM hdi = {0};
                                        char szText[256] = {0};
                                        hdi.mask = HDI_TEXT;
                                        hdi.pszText = szText;
                                        hdi.cchTextMax = sizeof(szText);
                                        Header_GetItem(hHeader, nmcd->dwItemSpec, &hdi);
                                        RECT rc = nmcd->rc;
                                        rc.left += 6;
                                        DrawTextA(nmcd->hdc, szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, CDRF_SKIPDEFAULT);
                                        return TRUE;
                                    }
                                }
                            }
                            else if (pnmh->hwndFrom == hListView) {
                                LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
                                switch (lplvcd->nmcd.dwDrawStage) {
                                    case CDDS_PREPAINT:
                                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                                        return TRUE;
                                    case CDDS_ITEMPREPAINT:
                                        lplvcd->clrText = g_DarkTextColor;
                                        lplvcd->clrTextBk = g_DarkBkColor;
                                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, CDRF_NEWFONT);
                                        return TRUE;
                                }
                            }
                        }

                        // Right-click context menu for history listview
                        if((DWORD)((LPNMHDR)lParam)->code == (DWORD)NM_RCLICK){
                            HMENU hMenu,hPopupMenu;
                            POINT pt;
                            hMenu=LoadMenu(NULL, MAKEINTRESOURCE(IDR_HISTORY));
                            hPopupMenu=GetSubMenu(hMenu, 0);
                            GetCursorPos(&pt);
                            SetForegroundWindow(hWnd);
                            TrackPopupMenu(hPopupMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
                            DestroyMenu(hPopupMenu);
                            DestroyMenu(hMenu);
                        }
                    }
                    break;

            case IDC_DISASM:{  // Disassembly ListView Messages
                LPNMLISTVIEW	pnm		= (LPNMLISTVIEW)lParam;
                HWND			hDisasm = GetDlgItem(hWnd,IDC_DISASM);
                LV_DISPINFO*	nmdisp	= (LV_DISPINFO*)lParam;
                LV_ITEM*		pItem	= &(nmdisp)->item;

                // Prevent resizing of the last column (References)
                if (pnm->hdr.code == HDN_BEGINTRACKA || pnm->hdr.code == HDN_BEGINTRACKW ||
                    pnm->hdr.code == HDN_DIVIDERDBLCLICKA || pnm->hdr.code == HDN_DIVIDERDBLCLICKW) {
                    LPNMHEADER phdr = (LPNMHEADER)lParam;
                    HWND hHeader = ListView_GetHeader(hDisasm);
                    int columnCount = Header_GetItemCount(hHeader);
                    // Block resize of last column
                    if (phdr->iItem == columnCount - 1) {
                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
                        return TRUE;
                    }
                }

                // Change Window Style on Custom Draw
                if((DWORD)pnm->hdr.code == (DWORD)NM_CUSTOMDRAW){
                    // Check if this is from the header control
                    HWND hListView = GetDlgItem(hWnd, IDC_DISASM);
                    HWND hHeader = ListView_GetHeader(hListView);
                    if (pnm->hdr.hwndFrom == hHeader) {
                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, (LONG_PTR)ProcessHeaderCustomDraw(lParam));
                        return TRUE;
                    }
                    SetWindowLongPtr(hWnd, DWL_MSGRESULT, (LONG_PTR)ProcessCustomDraw(lParam));
                    return TRUE;
                }

                // Set the information to the list view
                if((DWORD)((LPNMHDR)lParam)->code==(DWORD)LVN_GETDISPINFO){
                    try{
						if(DisasmDataLines.size()){
							// Load Data From Memory and display in ListView
							strcpy_s(Buffer,StringLen(DisasmDataLines[nmdisp->item.iItem].GetAddress())+1 ,DisasmDataLines[nmdisp->item.iItem].GetAddress());
							strcpy_s(Buffer1,StringLen(DisasmDataLines[nmdisp->item.iItem].GetCode())+1,DisasmDataLines[nmdisp->item.iItem].GetCode());
							strcpy_s(Buffer2,StringLen(DisasmDataLines[nmdisp->item.iItem].GetMnemonic())+1,DisasmDataLines[nmdisp->item.iItem].GetMnemonic());
							strcpy_s(Buffer3,StringLen(DisasmDataLines[nmdisp->item.iItem].GetComments())+1,DisasmDataLines[nmdisp->item.iItem].GetComments());
							strcpy_s(Buffer4,StringLen(DisasmDataLines[nmdisp->item.iItem].GetReference())+1,DisasmDataLines[nmdisp->item.iItem].GetReference());
							nmdisp->item.pszText=Buffer; // item 0
	                        
							if( nmdisp->item.mask & LVIF_TEXT){
								switch(nmdisp->item.iSubItem){
									case 0:nmdisp->item.pszText=Buffer;break;
									case 1:nmdisp->item.pszText=Buffer1;break;
									case 2:nmdisp->item.pszText=Buffer2;break;
									case 3:nmdisp->item.pszText=Buffer3;break;
									case 4:nmdisp->item.pszText=Buffer4;break;
									default:nmdisp->item.pszText="";
								}
								return true;
							}
						}
                    }  
					catch(...){}
                }
                
                // Update/keep The information on the list view
                if((DWORD)((LPNMHDR)lParam)->code==(DWORD)LVN_SETDISPINFO){
                    try{
                        nmdisp->item.pszText=Buffer; // Item 0
                        if( nmdisp->item.mask & LVIF_TEXT){   
                            // rest of the sub-items
                            switch(nmdisp->item.iSubItem){
                                case 0:nmdisp->item.pszText=Buffer;break;
                                case 1:nmdisp->item.pszText=Buffer1;break;
                                case 2:nmdisp->item.pszText=Buffer2;break;
                                case 3:nmdisp->item.pszText=Buffer3;break;
								case 4:nmdisp->item.pszText=Buffer4;break;
								default:nmdisp->item.pszText="";
                            }
                            return true;
                        }
                    }  
                    catch(...){}
                }

                // Response to the Click Event on the list view
                if((DWORD)((LPNMHDR)lParam)->code == (DWORD)NM_CLICK){
                     // Set Trace Location if Jxx/Call
                     if(DisassemblerReady==TRUE){
                      GetBranchDestination(GetDlgItem(hWnd,IDC_DISASM));
                    }
                }

                // Response to the Double Click Event on the list view
                if((DWORD)((LPNMHDR)lParam)->code == (DWORD)NM_DBLCLK){
                    POINT			mpt;
                    LV_HITTESTINFO	hit;

                    GetCursorPos(&mpt);
                    ScreenToClient(hDisasm,&mpt);
                    hit.pt=mpt;
                    hit.flags=LVHT_ABOVE | LVHT_BELOW | LVHT_NOWHERE | LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON | LVHT_TOLEFT | LVHT_TORIGHT;
                    hit.iItem=0;

                    iSelected = HitColumn(hDisasm,hit.pt.x);
                    if(iSelected==-1){
                        //MessageBox(hWnd,"Can't Set Comment.","Notice",MB_OK|MB_ICONINFORMATION);
                        break;
                    }

                    // Check Which Column We Pressed on.
                    switch(iSelected) {
                        // Show XReferences for the Clicked Address.
                        case 0:{
                            if(DisassemblerReady==TRUE){
                                if(GetXReferences(GetDlgItem(hWnd,IDC_DISASM))==TRUE){
                                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_XREF), hWnd, (DLGPROC)XrefDlgProc);
                                }
                            }
                        }
                        break;

                        // Code Patcher
                        case 1:{
                            if(DisassemblerReady==TRUE){
                                DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PATCHER), hWnd, (DLGPROC)PatcherProc);
                            }
                        }
                        break;

                        // Preview Jump/Call destination (double-click shows preview window)
                        case 2:{
                            if(DisassemblerReady==TRUE){
								GetBranchDestination(GetDlgItem(hWnd,IDC_DISASM));
                                if(DCodeFlow==TRUE){
                                    // Show preview window for the branch destination
                                    if(LoadedPe64)
                                        wsprintf(TempAddress, "%08X%08X", (DWORD)((DWORD_PTR)iSelectItem>>32), (DWORD)iSelectItem);
                                    else
                                        wsprintf(TempAddress, "%08X", iSelectItem);
                                    if(SearchItemText(hDisasm, TempAddress) != -1){
                                        DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TOOLTIP), hWnd, (DLGPROC)ToolTipDialog);
                                    }
                                }
                            }
                        }
                        break;

                        // Add Comments
                        case 3:{
                            iSelected=SendMessage(hDisasm,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
                            DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_COMMENTS), hWnd, (DLGPROC)SetCommentDlgProc);
                        }
                        break;
                    }
                }
                
                // Update Data when changing between items
                if(pnm->hdr.hwndFrom == hDisasm && pnm->hdr.code == LVN_ITEMCHANGED){
                    DWORD_PTR  Offset=0;
                    DWORD_PTR  item=-1;
                    char   text[128];
                    char   Message[128];
                    
                    item=SendMessage(hDisasm,LVM_GETNEXTITEM,-1,LVNI_FOCUSED); // get selected Item index
					if(item==-1){
                        break;
					}
                    strcpy_s(text,StringLen(DisasmDataLines[item].GetAddress())+1 ,DisasmDataLines[item].GetAddress());
					
                    if(LoadedPe==TRUE){
                        // Calculate Offset in EXE
                        if(StringLen(text)!=0){
                            Offset=StringToDword(text); 
                            if(!disop.VBPCodeMode && SectionHeader){
                                Offset -= nt_hdr->OptionalHeader.ImageBase;
                                Offset = (Offset - SectionHeader->VirtualAddress)+SectionHeader->PointerToRawData;
                            }
                        }
                        
                        ////////////////////////////////////////////
                        // Show the Information For Specific Line //
                        ////////////////////////////////////////////
                        SetDlgItemText(hWnd,IDC_MESSAGE2,""); 
                        wsprintf(Message," Line: #%ld Code Address: @%s Code Offset: @%08X",item,text,Offset);
                        SetDlgItemText(hWnd,IDC_MESSAGE2,Message);
                    }
					
                    //
                    // X references
                    //
                    if(DisassemblerReady==TRUE){ 
                        char temp_msg[60];
                        static bool Show=TRUE;
                        GetBranchDestination(GetDlgItem(hWnd,IDC_DISASM));
                        if(GetXReferences(GetDlgItem(hWnd,IDC_DISASM))==TRUE){
                            if(Show==TRUE){ // Found Xref
                                // Display Message Only 1 time
                                wsprintf(temp_msg,"%08X Has XReferences, (Ctrl+Space to view)",iSelected);
                                OutDebug(Main_hWnd,temp_msg);
                                SelectLastItem(GetDlgItem(Main_hWnd,IDC_LIST));
                                SetFocus(GetDlgItem(Main_hWnd,IDC_DISASM));
                                Show=FALSE;
                            }
                        }
                        else Show=TRUE;
                    }
                    
                    return TRUE;
                }

                if((DWORD)((LPNMHDR)lParam)->code == (DWORD)NM_RCLICK){
                    if(DisassemblerReady==TRUE){
                        HMENU hMenu,hPopupMenu;
                        POINT pt;

                        hMenu=LoadMenu(NULL, MAKEINTRESOURCE (IDR_DISASM));
                        hPopupMenu=GetSubMenu (hMenu, 0);
                        // Load Menu Bitmaps
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_IMP)); 
                        SetMenuItemBitmaps(hPopupMenu, 2, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_REF)); 
                        SetMenuItemBitmaps(hPopupMenu, 3, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_XREF)); 
                        SetMenuItemBitmaps(hPopupMenu, 4, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_PATCHER)); 
                        SetMenuItemBitmaps(hPopupMenu, 6, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_HEXEDIT)); 
                        SetMenuItemBitmaps(hPopupMenu, 7, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_EDIT_PE)); 
                        SetMenuItemBitmaps(hPopupMenu, 8, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PROCESS_REFRESH)); 
                        SetMenuItemBitmaps(hPopupMenu, 9, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_FIND)); 
                        SetMenuItemBitmaps(hPopupMenu, 11, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_APPEARANCE)); 
                        SetMenuItemBitmaps(hPopupMenu, 14, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_SAVE)); 
                        SetMenuItemBitmaps(hPopupMenu, 16, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);

                        hPopupMenu=GetSubMenu(hPopupMenu, 0);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_EP)); 
                        SetMenuItemBitmaps(hPopupMenu, 0, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap); 
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_ADDR)); 
                        SetMenuItemBitmaps(hPopupMenu, 1, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap); 
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_CODE)); 
                        SetMenuItemBitmaps(hPopupMenu, 2, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap); 

                        hPopupMenu=GetSubMenu (hMenu, 0);
                        hPopupMenu=GetSubMenu(hPopupMenu, 12);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_COPY)); 
                        SetMenuItemBitmaps(hPopupMenu, 0, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_COPY_FILE));
                        SetMenuItemBitmaps(hPopupMenu, 1, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_SELECT_ALL));
                        SetMenuItemBitmaps(hPopupMenu, 3, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);

						hPopupMenu=GetSubMenu (hMenu, 0);
                        hPopupMenu=GetSubMenu(hPopupMenu, 18);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_DATA_MANAGER)); 
                        SetMenuItemBitmaps(hPopupMenu, 0, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_FUNC_MANAGER));
                        SetMenuItemBitmaps(hPopupMenu, 1, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);

						hPopupMenu=GetSubMenu(hPopupMenu, 2);
						hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_DEFINE_DATA)); 
                        SetMenuItemBitmaps(hPopupMenu, 0, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hMenuItemBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_RC_DEFINE_FUNC));
                        SetMenuItemBitmaps(hPopupMenu, 1, MF_BYPOSITION, hMenuItemBitmap, hMenuItemBitmap);
                        hPopupMenu=GetSubMenu (hMenu, 0);
                        // Show the Menu at mouse position to the left 
                        GetCursorPos (&pt);
                        SetForegroundWindow(hWnd);
                        TrackPopupMenu (hPopupMenu, TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, NULL);
                        DestroyMenu (hPopupMenu);
                        DestroyMenu (hMenu);
                    }
                }
            }
            break;
        }

        // General Notifies.
		switch(((LPNMHDR) lParam)->code){ 
            // Show ToolTips when mouse is over the.
            // ToolBar Controls.
			case TTN_GETDISPINFO:{
				LPTOOLTIPTEXT lpttt; // ToolTip struct.
				lpttt = (LPTOOLTIPTEXT) lParam; // Botton information.
				lpttt->hinst = hInst; // used in our main dialog.
				DWORD_PTR idButton = lpttt->hdr.idFrom; // Get the ID of the selected/Hovered button.

				switch (idButton){  // Check which button we hover on.
					case IDC_LOAD_FILE:		lpttt->lpszText = "Load File To Disassemble";			break;
					case IDC_SAVE_DISASM:	lpttt->lpszText = "Save Disassembly Project";			break;
					case IDC_PE_EDITOR:		lpttt->lpszText = "Edit The Pe Header";					break;
					case ID_PROCESS_SHOW:	lpttt->lpszText = "Task Manager";						break;
					case ID_EP_SHOW:		lpttt->lpszText = "Goto EntryPoint";					break;
					case ID_CODE_SHOW:		lpttt->lpszText = "Goto Start Of Code";					break;
					case ID_ADDR_SHOW:		lpttt->lpszText = "Goto Address";						break;
                    case ID_SHOW_IMPORTS:	lpttt->lpszText = "Show Imports";						break;
                    case ID_SHOW_XREF:		lpttt->lpszText = "Show String References";				break;
                    case ID_EXEC_JUMP:		lpttt->lpszText = "Execute A Jump (Jxx) Instruction";	break;
                    case ID_RET_JUMP:		lpttt->lpszText = "Return From Jump (Jxx)";				break;
                    case ID_EXEC_CALL:		lpttt->lpszText = "Execute A Call Instruction";			break;
                    case ID_RET_CALL:		lpttt->lpszText = "Return From A Call";					break;
                    case ID_XREF:			lpttt->lpszText = "List XReferences";					break;
                    case ID_DOCK_DBG:		lpttt->lpszText = "Show/Hide History";			break;
                    case ID_HEX:			lpttt->lpszText = "Hex Editor";							break;
                    case ID_SEARCH:			lpttt->lpszText = "Find In Disassembly";				break;
                    case ID_PATCH:			lpttt->lpszText = "Code Patcher";						break;
					case ID_SCRIPT:			lpttt->lpszText = "Script Editor";						break;
				} 
				break;
			} 
			default:
			break;
        }
      }
      break;
			
        case WM_LBUTTONDOWN:{
			// Check if clicking on splitter first
			POINT ptClick;
			RECT splitterRect;
			ptClick.x = LOWORD(lParam);
			ptClick.y = HIWORD(lParam);
			GetWindowRect(GetDlgItem(hWnd, IDC_SPLITTER), &splitterRect);
			MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&splitterRect, 2);
			splitterRect.top -= 2;
			splitterRect.bottom += 2;
			if (PtInRect(&splitterRect, ptClick)) {
				bSplitterDragging = true;
				nSplitterDragY = ptClick.y;
				SetCapture(hWnd);
				break;
			}
			// Otherwise, move the window with the mouse
			ReleaseCapture();
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0);
		}
		break;

	    case WM_PAINT:{ // Paint Event.
            return false;
		}
		break;
		
        // Command Events (Control's Messages)
	    case WM_COMMAND:{ // Controling the Buttons
			// selecting a plugin in the menu 
            CheckForPluginPress(wParam);

			switch (LOWORD(wParam)){ // what we pressed on?
                // Open Disassembler Options Dialog.
                case IDC_START_DISASM:{
                  Clear(GetDlgItem(hWnd,IDC_LIST));
                  IntializeDisasm(LoadedPe,LoadedPe64,LoadedNe,hWnd,OrignalData,szFileName);
                  DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DISASM), hWnd, (DLGPROC)DisasmDlgProc);
                }
                break;

                // Stop the Disassembly process.
                case IDC_STOP_DISASM:{    
                     // Terminate the Working Thread.
                     TerminateThread(hDisasmThread,0);
                     CloseHandle(hDisasmThread); // Kill Handle
                     EnableMenuItem(GetMenu(hWnd),IDC_STOP_DISASM,MF_GRAYED); 
                     EnableMenuItem(GetMenu(hWnd),IDC_DISASM_IMP,MF_GRAYED);
                     EnableMenuItem(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_GRAYED);
                     EnableMenuItem(GetMenu(hWnd),IDC_START_DISASM,MF_ENABLED);
                     CheckMenuItem(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_UNCHECKED); // Clear of checked
                     SetDlgItemText(hWnd,IDC_MESSAGE1,"Disassembly Process Stoped");
                     SetDlgItemText(hWnd,IDC_MESSAGE2,"");
                     SetDlgItemText(hWnd,IDC_MESSAGE3,"");
                     // reset the progressBar
                     SendDlgItemMessage(hWnd,IDC_DISASM_PROGRESS,PBM_SETPOS,0,0);
                }
                break;

                // Pause/resume the disassembly process
                case IDC_PAUSE_DISASM:{
                    // Check Menu Item State (Checked/Unchecked)
                    if((GetMenuState(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_BYCOMMAND)==MF_CHECKED)){
                        //resume thread if item is checked (suspeded)
                        CheckMenuItem(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_UNCHECKED);
                        ResumeThread(hDisasmThread);
                    }
	                else{
                        char text[128];
                        GetDlgItemText(hWnd,IDC_MESSAGE1,text,128);
                        lstrcat(text," - Paused");
                        SetDlgItemText(hWnd,IDC_MESSAGE1,text);
                        // Suspend if item is not checked
                        CheckMenuItem(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_CHECKED);
                        SuspendThread(hDisasmThread);
                    }
                }
                break;

				// Load File for Disassemble / edit
				case IDC_LOAD_FILE:
				case IDC_LOADFILE:{
					// Open file
					if(Open(hWnd)==false){
						OutDebug(hWnd,"---> Something Went Wrong! <---");
						ErrorHandling(hWnd);
						OutDebug(hWnd,"");
						OutDebug(hWnd,"[ Task Aborted ]");
						break;
					}
					UpdateWindow(hWnd);
				} 
				break;

                case IDC_CLOSEFILE:{
                    CloseLoadedFile(hWnd);
                }
                break;
				
                // Save Project
                case IDM_SAVE_PROJECT:
                case IDC_SAVE_DISASM:{
                    if(DisassemblerReady==TRUE){
                        SaveProject();
                    }
                }
                break;

				// Load Project
                case IDM_OPEN_PROJECT:{
                    LoadProject();
                }
                break;

                // Show About window
				case IDC_MENUABOUT:{
					// About box
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_ABOUT), hWnd, (DLGPROC)AboutDlgProc);
				}
				break;

                // Show Imports
                case ID_SHOW_IMPORTS:
                case IDM_IMPORTS:
                case IDC_DISASM_IMP:{
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_IMPORTS), hWnd, (DLGPROC)ImportsDlgProc);
                }
                break;

                // Show String References
                case ID_SHOW_XREF:
                case IDM_STR_REF:
                case IDC_STR_REFERENCES:{
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_STRING_REF), hWnd, (DLGPROC)StringRefDlgProc);
                }
                break;

                // Show Comments
                case IDC_SHOW_COMMENTS:{
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_COMMENT_REF), hWnd, (DLGPROC)CommentRefDlgProc);
                }
                break;

                // Opens the Color selection for the disassembly window
                case IDM_DISASM_APPEARANCE:
                case IDC_DISASM_APPEARANCE:{
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DISASM_COLORS), hWnd, (DLGPROC)DisasmColorsDlgProc);
                }
                break;

                // Toggle Dark Mode
                case IDC_DARK_MODE:{
                    ToggleDarkMode(hWnd);
                }
                break;

                // Toggle Code Map bar
                case IDC_CODE_MAP:{
                    HWND hBar = GetDlgItem(hWnd, IDC_CODE_MAP_BAR);
                    BOOL visible = IsWindowVisible(hBar);
                    ShowWindow(hBar, visible ? SW_HIDE : SW_SHOW);
                    CheckMenuItem(GetMenu(hWnd), IDC_CODE_MAP, visible ? MF_UNCHECKED : MF_CHECKED);
                    RepositionDisasmForCodeMap(hWnd, !visible);
                    g_CodeMapVisible = !visible;
                    SaveSettings();
                    if (g_CodeMapVisible && DisassemblerReady) {
                        BuildCodeMapData();
                        InvalidateRect(hBar, NULL, TRUE);
                    }
                }
                break;

                // Open the PE Editor
				case IDC_PE_EDITOR:{ // ToolBar Button
					if(FilesInMemory==true){
						Update(OrignalData,hWnd,hInst);
						OutDebug(hWnd,"Preparing PE For Editing...");
                        SelectLastItem(GetDlgItem(hWnd,IDC_LIST)); // Selects the Last Item
						// Show Pe Editor dialog
						DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_EDITOR), hWnd, (DLGPROC)PE_Editor);
					}
				}
				break;

				// Show Process Manager
				case ID_PROCESS_SHOW:
                case IDM_PROCESS_MANAGER:
				case ID_PROCESSES:{
					// Send main window handlers
					GetData(hWnd,hInst);
					// Call the dialog
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PROCESS), hWnd, (DLGPROC)ProcessDlgProc);
				}
				break;

				// Show PE Editor
                case IDM_PE_EDIT:
				case ID_PE_EDIT:{ // Menu
					// Update the pe editor with new information
					Update(OrignalData,hWnd,hInst);
					OutDebug(hWnd,"");
					OutDebug(hWnd,"Preparing PE For Editing...");
                    SelectLastItem(GetDlgItem(hWnd,IDC_LIST)); // Selects the Last Item
					// Call the dialog
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_EDITOR), hWnd, (DLGPROC)PE_Editor);
				}
				break;
				
				case IDC_CLEARDEBUG:{ // Clear debug information (Free memory too!!)
					Clear(GetDlgItem(hWnd,IDC_LIST));
				}
				break;
				
			    case COLOR_D_TEXT:{
					// Set debug list text color
					DWORD_PTR count=SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_GETITEMCOUNT ,0,0);
					SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETTEXTCOLOR,0,(LPARAM)SetColor(0));
					SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_REDRAWITEMS ,0,count);
					UpdateWindow(hWnd);
				}
				break;

			    case TEXT_D_BG:{
					// Set debug list text background color
					DWORD_PTR count=SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_GETITEMCOUNT ,0,0);
					SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETTEXTBKCOLOR,0,(LPARAM)SetColor(1));
					SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_REDRAWITEMS ,0,count);
					UpdateWindow(hWnd);
				}
				break;
				
			    case BG_D_COLOR:{
					// Set debug list background color
					DWORD_PTR count=SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_GETITEMCOUNT ,0,0);
					SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_SETBKCOLOR,0,(LPARAM)SetColor(2));
					SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_REDRAWITEMS ,0,count);
					UpdateWindow(hWnd);
				}
				break;
				
			    case IDC_EXIT:{
					SendMessage(hWnd,WM_CLOSE,0,0);
				}
				break;

			    // select font to the debug output window
                case IDC_DEBUG_FONT:{
					SetFont(GetDlgItem(hWnd,IDC_LIST));
				}
				break;

                // select font to the disassembly window
                case IDC_DISASM_FONT:{
                    SetFont(GetDlgItem(hWnd,IDC_DISASM));
                }
                break;

                // Goto to code start in the disassembly mode
                case ID_CODE_SHOW:
                case IDM_GOTO_CODE:
                case IDC_GOTO_START:{
                    HWND hDisasm=GetDlgItem(hWnd,IDC_DISASM); // Disasm ListView Handler
                    SelectItem(hDisasm,0);
                }
                break;

                // Goto the entry point in the disassembly mode
                case IDM_GOTO_EP:
				case ID_EP_SHOW:
                case IDC_GOTO_ENTRYPOINT:{
                    DWORD_PTR Index; // Item Index to Select
                    HWND  hDisasm=GetDlgItem(hWnd,IDC_DISASM); // Disasm ListView Handler
                    char  MessageText[20];

                    //=============================================//
                    // Find and Select (Highlight) The EntryPoint  //
                    //=============================================//
                    if(LoadedPe64){
                        ULONGLONG ep = (ULONGLONG)nt_hdr64->OptionalHeader.ImageBase + nt_hdr64->OptionalHeader.AddressOfEntryPoint;
                        wsprintf(MessageText,"%08X%08X",(DWORD)(ep>>32),(DWORD)ep);
                    } else {
                        wsprintf(MessageText,"%08X",nt_hdr->OptionalHeader.AddressOfEntryPoint+nt_hdr->OptionalHeader.ImageBase);
                    }
                    Index=SearchItemText(hDisasm,MessageText);
                    if(Index!=-1){
                        SelectItem(hDisasm,Index-1); // select and scroll to item
                        SetFocus(hDisasm);
                    }
                }
                break;

                // Select address to jump to in disassembly mode
				case ID_ADDR_SHOW:
                case IDM_GOTO_ADDR:
                case IDC_GOTO_ADDRESS:{
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_GOTO_ADDRESS), hWnd, (DLGPROC)GotoAddrressDlgProc);
                }
                break;

                // Search text in Disassembler OutPut
                case ID_SEARCH:
                case IDM_FIND_IN_DISASM:
                case IDM_FIND:{
                    if(DisassemblerReady==TRUE){
                        DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_FIND), hWnd, (DLGPROC)FindDlgProc);
                    }
                }
                break;

                // Branch/execute a jump/call instruction
                case ID_EXEC_JUMP:
                case ID_EXEC_CALL:
                case ID_GOTO_EXECUTECALL:
                case ID_GOTO_JUMP:{
                    if(DisassemblerReady==TRUE){
                        if(DCodeFlow==TRUE){
                          GotoBranch(); // We can trace
                        }
                    }
                }
                break;

                // Back Tracing from Branch Instructions
                case ID_RET_JUMP:
                case ID_RET_CALL:
                case ID_GOTO_RETURN_CALL:
                case ID_GOTO_RETURN_JUMP:{
                    if(DisassemblerReady==TRUE){
                        BackTrace();
                    }
                }
                break;

                // XReferences
                case ID_XREF:
                case IDM_XREF:
                case IDC_XREF_MENU:{
                    if(GetXReferences(GetDlgItem(hWnd,IDC_DISASM))==TRUE){
                      DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_XREF), hWnd, (DLGPROC)XrefDlgProc);
                    }
                }
                break;

                // Control Flow Graph Viewer
                case IDC_VIEW_CFG:{
                    if(DisassemblerReady==TRUE){
                        ShowCFGViewerForCurrentFunction(hWnd);
                    }
                }
                break;

                // Dock / Undock the Debug Window
                case ID_DOCK_DBG:
                case IDC_DOCK_DEBUG:{
                    HWND hWnd_Control=GetDlgItem(hWnd,IDC_LIST);
                    HWND hWnd_Disasm=GetDlgItem(hWnd,IDC_DISASM);

                    // Is the Menu Item Checked?
                    if((GetMenuState(GetMenu(hWnd),IDC_DOCK_DEBUG,MF_BYCOMMAND)==MF_CHECKED)){
                        UnDockDebug(hWnd_Disasm,hWnd_Control,hWnd);
                    }
                    else{
                        DockDebug(hWnd_Disasm,hWnd_Control,hWnd);
                    }
                }
                break;

                // Show HexEditor
                case ID_HEX:
                case IDM_HEX_EDIT:
                case IDC_HEX_EDITOR:{
                    if(hRadHex==NULL){
                       MessageBox(hWnd,"RAHexEd.dll Not Found!\nMake Sure it\'s located at AddIns Folder Next time you load PVDasm.","Error Locating DLL",MB_OK|MB_ICONEXCLAMATION);
                       return 0;
                    }
                    
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_HEXEDITOR), hWnd, (DLGPROC)HexEditorProc);
                }
                break;

                // Open Code Pacther
                case ID_PATCH:
                case IDM_CODEPATCH:
                case IDM_PATCHER:{
                    if(DisassemblerReady==TRUE){
                        DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_PATCHER), hWnd, (DLGPROC)PatcherProc);
                    }
                }
                break;

                // Add Comments
                case IDC_DISASM_ADD_COMMENT:{
                   iSelected=SendMessage(GetDlgItem(hWnd,IDC_DISASM),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
                   DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_COMMENTS), hWnd, (DLGPROC)SetCommentDlgProc);

                }
                break;

				// Copy to clipboard (memory)
				case IDM_COPY_DISASM_CLIP:
                case IDM_COPY_DISASM:{ 
                    if(DisassemblerReady==TRUE){
                        hDisasmThread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)CopyDisasmToClipboard,hWnd,0,&DisasmThreadId);
                    }
                }
                break;

				// Copy to File
				case IDM_COPY_DISASM_FILE:
                case IDM_COPY_FILE:{
                    if(DisassemblerReady==TRUE){
                        hDisasmThread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)CopyDisasmToFile,hWnd,0,&DisasmThreadId);
                    }
                }
                break;

				// Select all items in disassembly view
				case IDM_SELECT_ALL_ITEMS:
                case IDM_SELECT_ALL:{
                    SelectAllItems(GetDlgItem(hWnd,IDC_DISASM));
                }
                break;

                // Copy selected history item to clipboard
                case IDM_HISTORY_COPY:{
                    HWND hHistList = GetDlgItem(hWnd, IDC_LIST);
                    int iSel = (int)SendMessage(hHistList, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
                    if(iSel != -1){
                        char szBuf[1024] = {0};
                        LV_ITEM lvi;
                        ZeroMemory(&lvi, sizeof(lvi));
                        lvi.iItem = iSel;
                        lvi.iSubItem = 0;
                        lvi.mask = LVIF_TEXT;
                        lvi.pszText = szBuf;
                        lvi.cchTextMax = sizeof(szBuf);
                        SendMessage(hHistList, LVM_GETITEMTEXT, (WPARAM)iSel, (LPARAM)&lvi);
                        CopyToClipboard(szBuf, hWnd);
                    }
                }
                break;

                // Save all history items to file
                case IDM_HISTORY_SAVEAS:{
                    OPENFILENAME ofn;
                    char FileName[MAX_PATH] = "";
                    HWND hHistList = GetDlgItem(hWnd, IDC_LIST);

                    ZeroMemory(&ofn, sizeof(OPENFILENAME));
                    ofn.lStructSize = sizeof(OPENFILENAME);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFilter = "Text files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
                    ofn.lpstrFile = FileName;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
                    ofn.lpstrDefExt = "txt";

                    if(GetSaveFileName(&ofn)){
                        HANDLE hFile = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                        if(hFile != INVALID_HANDLE_VALUE){
                            int count = (int)SendMessage(hHistList, LVM_GETITEMCOUNT, 0, 0);
                            for(int i = 0; i < count; i++){
                                char szLine[1024] = {0};
                                LV_ITEM lvi;
                                ZeroMemory(&lvi, sizeof(lvi));
                                lvi.iItem = i;
                                lvi.iSubItem = 0;
                                lvi.mask = LVIF_TEXT;
                                lvi.pszText = szLine;
                                lvi.cchTextMax = sizeof(szLine);
                                SendMessage(hHistList, LVM_GETITEMTEXT, (WPARAM)i, (LPARAM)&lvi);
                                DWORD dwWritten;
                                WriteFile(hFile, szLine, StringLen(szLine), &dwWritten, NULL);
                                WriteFile(hFile, "\r\n", 2, &dwWritten, NULL);
                            }
                            CloseHandle(hFile);
                        }
                    }
                }
                break;

				// Load Data Segment manager
				case IDC_DEFINE_DATA_SEGS:{
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DATA_SEGMENTS), hWnd, (DLGPROC)DataSegmentsDlgProc);
				}
				break;

				// Load Function Entrypoints manager
				case IDC_DEFINE_FUNCTION_EP:{
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_FUNCTIONS_ENTRYPOINTS), hWnd, (DLGPROC)FunctionsEPSegmentsDlgProc);
				}
				break;

				case IDC_DEFINE_SELECTED_DATA:{
				   DWORD_PTR dwStart=-1,dwEnd=-1;
				   GetSelectedLines(&dwStart,&dwEnd);
				   if((dwStart!=-1 && dwEnd!=-1) && (dwEnd>dwStart)){
					   dwStart = StringToDword(DisasmDataLines[dwStart].GetAddress());
					   dwEnd   = StringToDword(DisasmDataLines[dwEnd].GetAddress());
					   DefineAddressesAsData(dwStart,dwEnd, TRUE); // define the address
                       SetFocus(GetDlgItem(hWnd,IDC_DISASM));
				   }
				}
				break;

			    case IDC_DEFINE_SELECTED_EP:{
					DWORD_PTR dwStart=-1,dwEnd=-1;
					GetSelectedLines(&dwStart,&dwEnd);
					DefineAddressesAsFunction(dwStart,dwEnd,"",TRUE); // Define the function
					SetFocus(GetDlgItem(hWnd,IDC_DISASM));
				}
				break;

				// Produce ASM File
				case IDC_PRODUCE_MASM:{
					if(DisassemblerReady==TRUE && LoadedPe==TRUE){  
						DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_WIZARD), hWnd, (DLGPROC)WizardDlgProc);
					}
				}
				break;

				case IDC_PRODUCE_VIEW:{
					if(DisassemblerReady==TRUE){
						hDisasmThread=CreateThread(0,0,(LPTHREAD_START_ROUTINE)ProduceDisasm,hWnd,0,&DisasmThreadId);
					}
				}
				break;

				case IDC_EXPORT_MAP_PVDASM:{
					if(DisassemblerReady==TRUE){
						ExportMapFile();
					}
				}
				break;

				case IDC_PVSCRIPT:
				case IDC_PVSCRIPT_ENGINE:
				case ID_SCRIPT:{
					DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_SCRIPT_EDITOR), hWnd, (DLGPROC)ScriptEditorDlgProc);
					
				}
				break;
			}
			break;
	
			case WM_CLOSE:{ // We closing PVDasm
				ExitPVDasm(hWnd);
			}
			break;
		}
		break;
	}
	return 0;
}

//===============================================================================
//=========================Functions Area========================================
//===============================================================================

// ========================
// ======= File Open ======
// ========================
/*
	   Function: 
	   ---------
	   "Open"

	   Parameters:
	   -----------
	   1. HWND hWndParent - Window Handler
	   
	   Return Values:
	   --------------
	   Result: TRUE/FALSE

	   Usage:
	   ------
	   This funtion Opens a user selected file (exe,dll,gb)
	   Maps the file to memory and checks if it has a valid
	   PE structure.

	   הפונקציה פותחת קובץ שנבחר ע"י המשתמש
	   ממפה אותו לזיכרון ובודקת את סוג הקובץ שנפתח
*/
int Open(HWND hWnd)
{  	
	// File Select dialog struct
	OPENFILENAME	ofn;
	char			debug[MAX_PATH],temp[MAX_PATH];
	HANDLE			hReadOnly=NULL;

	// Intialize struct
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME); // Set Size of Struct
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = "Executable files (*.exe, *.dll)\0*.exe;*.dll\0ROM Files (*.gb,*.gbc)\0*.gb;*.gbc\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = szFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt = "exe";
	
	// we selected a file ?
	if(GetOpenFileName(&ofn)!=0){	
		// Do we have already a file loaded ?
		if(FilesInMemory==true){
            FreeMemory(hWnd);
        }
		//צור/פתח קובץ שאנו נבחר מתוך הדיאלוג
		hFile=CreateFile(szFileName,
			GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL );

		// Delete debug text
		SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_DELETEALLITEMS,0,0); // Insert/Show the coloum
		
		OutDebug(hWnd,"[ Analyzing File ]");
		OutDebug(hWnd,"");

        // Disable Menus (Disasm related)
        HMENU hMenu=GetMenu(hWnd);
        EnableMenuItem(hMenu,IDC_GOTO_START,MF_GRAYED);
        EnableMenuItem(hMenu,IDC_GOTO_ENTRYPOINT,MF_GRAYED);
        EnableMenuItem(hMenu,IDC_GOTO_ADDRESS,MF_GRAYED);
        EnableMenuItem(hMenu,ID_GOTO_JUMP,MF_GRAYED);
        EnableMenuItem(hMenu,ID_GOTO_RETURN_JUMP,MF_GRAYED);
        EnableMenuItem(hMenu,ID_GOTO_EXECUTECALL,MF_GRAYED);
        EnableMenuItem(hMenu,ID_GOTO_RETURN_CALL,MF_GRAYED);
        EnableMenuItem(hMenu,IDC_START_DISASM,MF_ENABLED);
        EnableMenuItem(hMenu,IDC_STOP_DISASM,MF_GRAYED);
        EnableMenuItem(hMenu,IDC_PAUSE_DISASM,MF_GRAYED);
	}
	else{ 
		if(FilesInMemory!=true){
			// We pressed cancel
			// אם ביטלנו את הדיאלוג
			ErrorMsg=1;
			SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_PE_EDITOR, (LPARAM)FALSE);	// Disable Buttons
			SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_SAVE_DISASM, (LPARAM)FALSE); 
			return false;
		}

		SetWindowText(hWnd,PVDASM);
		return false;
	}
	
	if(hFile==INVALID_HANDLE_VALUE){ // Invalid file
		// Check mabye the file is Read-Only Flagged.
		hReadOnly=(int*)GetFileAttributes(szFileName);
		if( ((DWORD)hReadOnly & FILE_ATTRIBUTE_READONLY)==FILE_ATTRIBUTE_READONLY ){
			// File has the Read-Only Flag, set it back to normal.
			hReadOnly=(int*)SetFileAttributes(szFileName,FILE_ATTRIBUTE_NORMAL);
			if(!hReadOnly){
				ErrorMsg=2;
				return false;
			}

			// Open the File again for reading.
			hFile=CreateFile(szFileName,
				GENERIC_READ|GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL );
		}

		// Error on reading the file
		if(hFile==INVALID_HANDLE_VALUE){
			ErrorMsg=2;
			return false;
		}
	}
	
	wsprintf(debug,"%s - Loaded",szFileName);
	OutDebug(hWnd,debug);
	
	hFileSize=GetFileSize(hFile,NULL);	// Get file size
	if(hFileSize==INVALID_FILE_SIZE){	// Invalid File Size
		ErrorMsg=3;
		return false;
	}

	wsprintf(debug,"File Size Stored (%d bytes)",hFileSize);
    OutDebug(hWnd,debug);
	
	// קריאה לפונקציית מיפוי הקובץ לזיכרון
	// פונקציה תחזיראת המזהה לקובץ הממופה
	hFileMap=CreateFileMapping(hFile,
		NULL,
		PAGE_READWRITE,
		0,
		0,
		NULL);
	
	if(hFileMap==NULL){
		// אם לא ניתן למפות את הקובץ נצא
		ErrorMsg=4;
		return false;
	}
	
    /* 
	   Data = Pointer to the Mapped File 
	   מכיל את תחילת הקובץ הממופה בזיכרון char * מסוג Data משתנה 
	   תחזיר את כתובתו של הקובץ בזיכרון MapViewOfFile הפונקציה
	*/	
	
	Data=(char*)MapViewOfFile(hFileMap,
							FILE_MAP_ALL_ACCESS,
							0,
							0,
							0);
	
	wsprintf(debug,"File Mapped to Memory Successfuly At 0x%08X",Data);
	OutDebug(hWnd,debug);
	
	// Get caption text
	GetWindowText(hWnd,debug,256);
	// copy the loaded file + path
	lstrcpy(temp,szFileName);
	// strip the Exe Name
	GetExeName(temp);
	// set new caption
	wsprintf(debug,"%s - [%s]",debug,temp);
	SetWindowText(hWnd,debug);

	FilesInMemory=true; 	
	OrignalData=Data; 
	
	// read the dos header
	doshdr=(IMAGE_DOS_HEADER*)Data;
    nt_hdr=(IMAGE_NT_HEADERS*)(doshdr->e_lfanew+Data);

    try{
        // check if the file has the magic letters/bytes
        // בדוק חוקיות הקובץ
        if(doshdr->e_magic!=0x5A4D){ // "MZ" == 5A4D
            // MZ בדיקת אותיות אלו מעידת האם זהו קובץ בעל חתימת
            OutDebug(hWnd,"Loaded File which is not Executble!");
            SelectLastItem(GetDlgItem(hWnd,IDC_LIST)); // Selects the Last Item
            LoadedPe=false;
        }
        else{
			/*
			קטע קוד זה בודק האם קיים תחילת מבנה לקובץ
			ההרצה שאותו מיפינו, זה נעשה ע"י בדיקת
			PE תחילת המבנה אם הוא מכיל את האותיות
			0x00004550 - "PE" (IMAGE_NT_SIGNATURE)
            */

            // Check PE Signature
            if(nt_hdr->Signature==IMAGE_NT_SIGNATURE){
                LoadedPe=true;
                // האם הקובץ הוא הרצה או לא
                OutDebug(hWnd,"-> PE File Loaded!");
                // Detect Visual Basic 5 / 6 P-Code binary
                disop.VBPCodeMode = false;
                disop.VBVersion   = 0;
                {
                    int vbVer = 0;
                    if(DetectVBRuntime((const BYTE*)Data, hFileSize, &vbVer)){
                        disop.VBPCodeMode = true;
                        disop.VBVersion   = vbVer;
                        disop.CPU         = (vbVer == 5) ? vbpcode5 : vbpcode6;
                        char vbDetMsg[64];
                        wsprintf(vbDetMsg,"-> Visual Basic %d P-Code binary detected.",vbVer);
                        OutDebug(hWnd, vbDetMsg);
                    }
                }
            }
            else{
				// Check for NE existance
				ne_hdr=(IMAGE_OS2_HEADER*)(doshdr->e_lfanew+Data);
				if(ne_hdr->ne_magic==IMAGE_OS2_SIGNATURE){
					LoadedNe = true;
				}
				else{
					// אין טעם להמשיך לבדוק, ואנו משחררים את הקובץ
					// הטעון מהזיכרון
					OutDebug(hWnd," -> Uknonwn executable is not yet supported.");
					SelectLastItem(GetDlgItem(hWnd,IDC_LIST)); // Selects the Last Item
					LoadedPe=false;
					LoadedPe64=false;
					if(CloseFiles(hWnd)==false){
						return false; // something went wrong
					}
				}
            }
        }
    }
    catch(...){ // catch errors on loading file.
        OutDebug(hWnd," * Found Currupted PE structure. *");
        CloseFiles(hWnd);
        return false;
    }

	EnableID(hWnd); // Make Dump Button Enabled	
	/* Dumping Area 
	מכאן אנו מתחילים לקרוא את הקובץ ששמופה לזיכרון
	אם העלנו קובץ הרצה והוא חוקי (קיים לו מבנה מסוים) אז
	אנו נקרא לפונקציה המטפלת בכך
	אחרת אנו מציג את תוכנו של הקובץ בבתים
	*/

	if(LoadedPe==true || LoadedNe==true){ // add PE64/NE
		// אם הקובץ הוא קובץ הרצה, אז אנו נציג את נתוניו
		if(DumpPE(hWnd,Data)==false){
			return false;
		}
        LoadedNonePe=false; // PE File Loaded
        SelectLastItem(GetDlgItem(hWnd,IDC_LIST)); // Selects the Last Item
		// First time Disasm initializer
		IntializeDisasm(LoadedPe,LoadedPe64,LoadedNe,hWnd,OrignalData,szFileName);
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DISASM), hWnd, (DLGPROC)DisasmDlgProc);
	}
	else{
		LoadedPe=false;		// not PE file
		LoadedPe64=false;
        LoadedNonePe=true;	// Loaded None Pe File
        DebugNonPE(hWnd);	// EXE קובץ לא בעל מבנה
	}

	return true;    // all tasks went great in the end
}

void DebugNonPE(HWND hWnd)
{
	/*
	   Function: 
	   ---------
	   "DebugNonPE"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler
	   
	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   Procedure deals with files that has no
	   Header (PE) and probably needed to be handled
	   Different than Win32 executble files.
   */
    HMENU hMenu=GetMenu(hWnd);
    // Disable & Enable Disassembly menu items
    //EnableMenuItem ( hMenu, IDC_STOP_DISASM,     MF_GRAYED  );
    //EnableMenuItem ( hMenu, IDC_PAUSE_DISASM,    MF_GRAYED  );
    //EnableMenuItem ( hMenu, IDC_START_DISASM,    MF_GRAYED  );
    EnableMenuItem ( hMenu, IDC_GOTO_START,			MF_GRAYED  );
    EnableMenuItem ( hMenu, IDC_GOTO_ENTRYPOINT,	MF_GRAYED  );
    EnableMenuItem ( hMenu, IDC_GOTO_ADDRESS,		MF_GRAYED  );
    EnableMenuItem ( hMenu, ID_GOTO_JUMP,			MF_GRAYED  );
    EnableMenuItem ( hMenu, ID_GOTO_RETURN_JUMP,	MF_GRAYED  );
    EnableMenuItem ( hMenu, ID_GOTO_EXECUTECALL,	MF_GRAYED  );
    EnableMenuItem ( hMenu, ID_GOTO_RETURN_CALL,	MF_GRAYED  );
    EnableMenuItem ( hMenu, IDM_FIND,				MF_GRAYED  );
    EnableMenuItem ( hMenu, IDC_DISASM_IMP,			MF_GRAYED  );
    EnableMenuItem ( hMenu, IDC_STR_REFERENCES,		MF_GRAYED  );
    EnableMenuItem ( hMenu, IDC_SHOW_COMMENTS,		MF_GRAYED  );
    EnableMenuItem ( hMenu, IDC_XREF_MENU,			MF_GRAYED  );
    EnableMenuItem ( hMenu, IDM_PATCHER,			MF_GRAYED  );
    EnableMenuItem ( hMenu, ID_PE_EDIT,				MF_GRAYED  );
    
    // Disable ToolBar Buttons
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_EP_SHOW,		(LPARAM) FALSE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_CODE_SHOW,	(LPARAM) FALSE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_ADDR_SHOW,	(LPARAM) FALSE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_SHOW_XREF,	(LPARAM) FALSE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_SHOW_IMPORTS,	(LPARAM) FALSE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_SEARCH,		(LPARAM) FALSE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, ID_PATCH,		(LPARAM) FALSE );
    SendMessage ( hWndTB, TB_ENABLEBUTTON, IDC_SAVE_DISASM,	(LPARAM) FALSE );
    
    IntializeDisasm(LoadedPe,LoadedPe64,LoadedNe,hWnd,OrignalData,szFileName);
    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DISASM), hWnd, (DLGPROC)DisasmDlgProc);
    SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_PE_EDITOR, (LPARAM)FALSE);	// Disable Buttons
}

bool CloseFiles(HWND hWnd)
{
	/* 
	   Function: 
	   ---------
	   "CloseFiles"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler
	   
	   Return Values:
	   --------------
	   Result: True/False

	   Usage:
	   ------
	   פונקציה זו תשחרר מהזיכרון את כל
	   התכנית שממופת בזיכרון
	   גם כן תשחרר את הרשימה המקושרת
	   ותחזיר שקר או אמת אם עברה בהצלחה

	   Closing all file's handlers
	   From the memory
	*/

	// Unmaping File
	if(UnmapViewOfFile(OrignalData)==0){ // סגירת מיפוי הקובץ מהזיכרון
		ErrorMsg=5;
		return false;
	}
	OutDebug(hWnd,"UnMapped File Successfuly");
		
	// Close the map handle
	if(CloseHandle(hFileMap)==NULL){ // סגירת המזהה של המפה
		ErrorMsg=6;
		return false;
	}	
	OutDebug(hWnd,"Mapped File Closed Successfuly");
	
	// Close the file handle
	if(CloseHandle(hFile)==NULL){ // סגירת המזהה של הקובץ
		ErrorMsg=7;
		return false;
	}
	// no errors
	ErrorMsg=0;
	OutDebug(hWnd,"File Closed Successfuly");
	OutDebug(hWnd,"");
    FilesInMemory=false;
    LoadedNonePe=false;
    LoadedPe=false;
	LoadedPe64=false;
	LoadedNe=false;
	
	return true;
}

void ErrorHandling(HWND hWnd)
{
	/*
	   Function: 
	   ---------
	   "ErrorHandling"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler
	   
	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   This function will handle errors accured from our code
	   פונקציה זו תטפל בשגיאות שקראו במהלך התוכנית
	   ותדפיס טקסא בהתאם לסוג השגיעה
	*/

	switch(ErrorMsg){
		case 1: OutDebug(hWnd,"-> File Loading Canceled");			break;
		case 2: OutDebug(hWnd,"-> File Is Already Being Used");		break;
		case 3: OutDebug(hWnd,"-> Cannot Retrive File Size");		break;
		case 4: OutDebug(hWnd,"-> Cannot Map File To Memory");		break;
		case 5: OutDebug(hWnd,"-> Cannot UnMap File From Memory");	break;
		case 6: OutDebug(hWnd,"-> Cannot Release Map Handle");		break;
		case 7: OutDebug(hWnd,"-> Cannot Release File Handle");		break;
		case 8: OutDebug(hWnd,"-> Cannot Create Node For List");	break;
		case 9: OutDebug(hWnd,"-> Memory Pointer Out of Range !");	break;
	}
}

int DumpPE(HWND hWnd, char *memPtr)
{
	/*
	   Function:
	   ---------
	   "DumpPE"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler
	   2. char *memPtr - Pointer to the memory mapped file
	   
	   Return Values:
	   --------------
	   Result: True/False

	   Usage:
	   ------
	   Procedure Prints basic info of the PE header
	   into the output list view control.

	   פונקציה המדפיסה את הנתונים הבסיסיים של ראש הקובץ
	   לאיזור התחתון של התוכנית
	*/
	
	char debug[128];
	OutDebug(hWnd,"");
	OutDebug(hWnd,"Dumping PE Header From Memory:"); //Print into the Debug List control
	OutDebug(hWnd,"=============================="); //.....
	
	// Print PE information from File header / Optional Header
	// הדפסת נתונים מנתוניו של הקובץ עלינו עובדים(זיכרון) נתונים
	// PE אלו שייכים לראש הקובץ שניקרא
	
	doshdr=(IMAGE_DOS_HEADER*)memPtr;
	nt_hdr=(IMAGE_NT_HEADERS*)(doshdr->e_lfanew+memPtr);

	// Check for PE+ Header (Header for 64Bit OS)
	switch(nt_hdr->FileHeader.Machine){
		case IMAGE_FILE_MACHINE_IA64:
		case IMAGE_FILE_MACHINE_AMD64:{
			LoadedPe=FALSE;
			LoadedPe64=TRUE;
			nt_hdr64=(IMAGE_NT_HEADERS64*)(doshdr->e_lfanew+memPtr);
			nt_hdr=NULL;
		}
		break;
	}
	// For 64Bit PE(+) use:
	// IMAGE_NT_HEADERS64
	// define 0x8664 - AMD64
	if(LoadedPe==TRUE){
		wsprintf(debug,"Number of Section: %04X",nt_hdr->FileHeader.NumberOfSections);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Time Date Stamp: %08X",nt_hdr->FileHeader.TimeDateStamp);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Size Of Optional Header: %08X",nt_hdr->FileHeader.SizeOfOptionalHeader);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Entry Point: %08X",nt_hdr->OptionalHeader.AddressOfEntryPoint);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Image Base: %08X",nt_hdr->OptionalHeader.ImageBase);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Orignal Entry Point: %08X",nt_hdr->OptionalHeader.ImageBase+nt_hdr->OptionalHeader.AddressOfEntryPoint);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Size Of Image: %08X",nt_hdr->OptionalHeader.SizeOfImage);
		OutDebug(hWnd,debug);
	}
	else if(LoadedPe64==TRUE){
		wsprintf(debug,"Number of Section: %04X",nt_hdr64->FileHeader.NumberOfSections);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Time Date Stamp: %08X",nt_hdr64->FileHeader.TimeDateStamp);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Size Of Optional Header: %08X",nt_hdr64->FileHeader.SizeOfOptionalHeader);
		OutDebug(hWnd,debug);
		wsprintf(debug,"Entry Point: %08X",nt_hdr64->OptionalHeader.AddressOfEntryPoint);
		OutDebug(hWnd,debug);
		{
			ULONGLONG ib64 = nt_hdr64->OptionalHeader.ImageBase;
			wsprintf(debug,"Image Base: %08X%08X",(DWORD)(ib64>>32),(DWORD)ib64);
			OutDebug(hWnd,debug);
			ULONGLONG oep64 = ib64 + nt_hdr64->OptionalHeader.AddressOfEntryPoint;
			wsprintf(debug,"Orignal Entry Point: %08X%08X",(DWORD)(oep64>>32),(DWORD)oep64);
		}
		OutDebug(hWnd,debug);
		wsprintf(debug,"Size Of Image: %08X",nt_hdr64->OptionalHeader.SizeOfImage);
		OutDebug(hWnd,debug);
	}

	OutDebug(hWnd,"==========================");
	OutDebug(hWnd,"");
	return true;
}

void EnableID(HWND hWnd){
	/*
	   Function:
	   ---------
	   "EnableID"

	   Parameters:
	   -----------
	   1. HWND hWnd - Window Handler
	   
	   Return Values:
	   --------------
	   None

	   Usage:
	   ------
	   Procedure enables The Menus,
	   and the Toolbar items.
	*/

	// אפשור אובייקטים בתפריט
	// Enable Menus
	HMENU hMenu=GetMenu(hWnd);
	EnableMenuItem(hMenu,ID_PE_EDIT,MF_ENABLED);
	EnableMenuItem(hMenu,IDC_CLOSEFILE,MF_ENABLED);
	SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_PE_EDITOR, (LPARAM)TRUE);		// Disable Buttons
	SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_SAVE_DISASM, (LPARAM)TRUE);	// Disable Buttons
}

// =====================================================================================//
// ================================= DIALOGS ===========================================//
// =====================================================================================//


//======================================================================================
//=========================  GotoAddrressDlgProc =======================================
//======================================================================================
BOOL CALLBACK GotoAddrressDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message){
        case WM_INITDIALOG:{
			char Address[17];
			if(LoadedPe==TRUE){
				wsprintf(Address,"%08X",nt_hdr->OptionalHeader.ImageBase);
			}
			else if(LoadedPe64==TRUE){
				ULONGLONG ib64 = nt_hdr64->OptionalHeader.ImageBase;
				wsprintf(Address,"%08X%08X",(DWORD)(ib64>>32),(DWORD)ib64);
			}

            MaskEditControl(GetDlgItem(hWnd,IDC_ADDRESS),"0123456789abcdefABCDEF\b",TRUE);
            SendDlgItemMessage(hWnd,IDC_ADDRESS, EM_SETLIMITTEXT,(WPARAM)(LoadedPe64 ? 16 : 8),0);
            SetFocus(GetDlgItem(hWnd,IDC_ADDRESS));
			SetDlgItemText(hWnd,IDC_ADDRESS,Address);
            UpdateWindow(hWnd); // Update the window
        }
        break;
        
        case WM_PAINT:{
            return false; // No paint
        }
        break;
        
        case WM_COMMAND:{
            switch(LOWORD(wParam)) // What we press on?
            {
                case IDC_GOTO:{
                    char Text[10];
                    DWORD_PTR Index;
                    HWND hList = GetDlgItem(Main_hWnd,IDC_DISASM);
                    GetDlgItemText(hWnd,IDC_ADDRESS,Text,9);
                    if(StringLen(Text)==0){
                        MessageBox(hWnd,"No Address Entered!","Notice",MB_OK);
                        SetFocus(GetDlgItem(hWnd,IDC_ADDRESS));
                        return TRUE;
                    }

                    if(StringLen(Text)==6){ // No 00 in beginning?
                        Index=StringToDword(Text)>>8;
                        wsprintf(Text,"%08X",Index);
                    }

                    Index = SearchItemText(hList,Text); // search item
                    if(Index!=-1){ // Found index
                        SelectItem(hList,Index); // select and scroll to item
                        EndDialog(hWnd,0);
                        SetFocus(hList);
                    }
                    else{
                        MessageBox(hWnd,"Address Not Found!","Notice",MB_OK);
                        SetFocus(GetDlgItem(hWnd,IDC_ADDRESS));
                    }
                }
                break;

                // User cancel Disasm Options
                case IDC_CANCEL:{
                    EndDialog(hWnd,0);
                }
                break;
            }
        }
        break;
        
        case WM_CLOSE:{
            EndDialog(hWnd,0); // kill dialog
        }
        break;
    }
    return 0;
}


//======================================================================================
//===========================  SetCommentDlgProc =======================================
//======================================================================================
BOOL CALLBACK SetCommentDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message)
	{
        case WM_INITDIALOG:
        {
            LVITEM LvItem;
			int    lenght;
            char   Text[128];
            char   Text2[128];

            SendDlgItemMessage(hWnd,IDC_ADD_SEMI,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);
            SendDlgItemMessage(hWnd,IDC_COMMENT,EM_SETLIMITTEXT,(WPARAM)127,0);

            memset(&LvItem,0,sizeof(LvItem));
            LvItem.mask=LVIF_TEXT;
            LvItem.iSubItem=0;
            LvItem.pszText=Text;
            LvItem.cchTextMax=256;
			LvItem.iItem=(DWORD)iSelected;
            
            strcpy_s(Text2," Set Comment for Address: ");
            SendMessage(GetDlgItem(Main_hWnd,IDC_DISASM),LVM_GETITEMTEXT, iSelected, (LPARAM)&LvItem);
            lstrcat(Text2,Text);
            SetWindowText(hWnd,Text2);

            LvItem.iSubItem=3; // get comment if already exists
            SendMessage(GetDlgItem(Main_hWnd,IDC_DISASM),LVM_GETITEMTEXT, iSelected, (LPARAM)&LvItem);
            
            if(Text[1]==' '){
				lenght=StringLen(Text);
				for(int i=0;i<lenght;i++){
                    Text[i]=Text[i+2];
				}
            }

            SetDlgItemText(hWnd,IDC_COMMENT,Text);
            SetFocus(GetDlgItem(hWnd,IDC_COMMENT));
        }
        break;
        
        case WM_PAINT:{
            return false; //no paint
        }
        break;
        
        case WM_COMMAND:{
            switch(LOWORD(wParam)) // what we press on?
            {
                case IDC_SET:{
                    char Text[128];
                    char Text2[128];

					if((SendDlgItemMessage(hWnd,IDC_ADD_SEMI,BM_GETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0))==BST_CHECKED){
                        strcpy_s(Text,"; ");
					}
					else{
                        strcpy_s(Text," ");
					}

                    GetDlgItemText(hWnd,IDC_COMMENT,Text2,256);

					if(StringLen(Text2)!=0){
                        lstrcat(Text,Text2);
					}
					else{
                        strcpy_s(Text,"");
					}
                   
                    DisasmDataLines[iSelected].SetComments(Text);
                    EndDialog(hWnd,0);
                }
                break;
                
                // User cancel Disasm Options
                case IDC_CANCEL:{
                    EndDialog(hWnd,0);
                }
                break;
            }
        }
        break;
        
        case WM_CLOSE:{
            EndDialog(hWnd,0); // kill dialog
        }
        break;
    }
    return 0;
}

// Forward declarations for search helpers
void DoImportSearch(HWND hDlg, int startIndex);
void DoStrRefSearch(HWND hDlg, int startIndex);

//======================================================================================
//==============================  ImportsDlgProc =======================================
//======================================================================================
BOOL CALLBACK ImportsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message){
        case WM_INITDIALOG:{
          ItrImports	itr;
          LV_COLUMN		LvCol;
          LVITEM		LvItem;
          HWND			hList;
          DWORD			i=0,Index=0,j=0,len;
          char			Api[128],temp[128],*ptr,*DllPtr,*StartFunction;

          // Set Radio Button Checked
          SendDlgItemMessage(hWnd,IDC_CONTAIN,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);
          SetFocus(GetDlgItem(hWnd,IDC_FIND_IMPORT));
          Imp_hWnd=hWnd;
          hList=GetDlgItem(hWnd,IDC_IMPORTS_LIST);
          SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT); // Set style

          // Add Columns
          memset(&LvCol,0,sizeof(LvCol));  // Init the struct to ZERO
          LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
          LvCol.pszText="Address";
          LvCol.cx=0x50;
          SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); 
          LvCol.pszText="DLL Name";
          LvCol.cx=0x60;
          SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
          LvCol.pszText="Import Name";
          LvCol.cx=0x97;
          SendMessage(hList,LVM_INSERTCOLUMN,2,(LPARAM)&LvCol);
          LvCol.pszText="Instruction";
          LvCol.cx=0x50;
          SendMessage(hList,LVM_INSERTCOLUMN,3,(LPARAM)&LvCol);
          
          memset(&LvItem,0,sizeof(LvItem));	// Init the struct to ZERO
          LvItem.cchTextMax = 256;			// Max size of test
          
          try{
              itr=ImportsLines.begin();
              for(;itr!=ImportsLines.end();itr++,Index++){
                  temp[0]='\0';
                  Api[0]='\0';
                  i=(*itr);

                  LvItem.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE; // Styles
                  LvItem.iItem=Index;
                  LvItem.iSubItem=0;
                  LvItem.pszText = DisasmDataLines[i].GetAddress();
                  LvItem.lParam = i; // Save Position
                  SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
                  LvItem.pszText="";
                  LvItem.mask=LVIF_TEXT;

                  // Get Assembly Line
                  strcpy_s(Api,StringLen(DisasmDataLines[i].GetMnemonic())+1, DisasmDataLines[i].GetMnemonic());
                  ptr=Api;

                  // Extract the Function
				  ptr = strchr(Api, '!');
				  if(!ptr) continue; // skip entry without '!' delimiter

                  DllPtr=ptr;
                  --DllPtr;
                  ptr++;

                  // Extract the DLL
				  while(DllPtr > Api && *DllPtr!=' '){
                      DllPtr--;
				  }

                  DllPtr++;
                  while(*DllPtr && *DllPtr!='!'){
                      wsprintf(Api,"%c",*DllPtr);
                      lstrcat(temp,Api);
                      DllPtr++;
                  }
                  
                  // Copy DLL Name
                  lstrcat(temp,'\0');
                  DllPtr=temp;
                  if(temp[0]=='['){
                      DllPtr++;
                  }
                  LvItem.iItem=Index;
                  LvItem.iSubItem=1;
                  LvItem.pszText=DllPtr;
                  SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem); 

                  // Copy Function Name
                  strcpy_s(temp,StringLen(ptr)+1,ptr);
                  len=StringLen(temp);
				  if(len){
					  if(temp[len-1]==']'){
						  temp[len-1]='\0';
						  wsprintf(Api,"%s",temp);
					  }
					  else{
						  strcpy_s(Api,StringLen(temp)+1,temp);
					  }

					  LvItem.iItem=Index;
					  LvItem.iSubItem=2;
					  LvItem.pszText=Api;
					  SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);

					  StartFunction=DisasmDataLines[i].GetMnemonic();
					  temp[0]='\0';
					  while(*StartFunction!=' '){
						  wsprintf(Api,"%c",*StartFunction);
						  lstrcat(temp,Api);
						  StartFunction++;
					  }
					  lstrcat(temp,'\0');
					  LvItem.iItem=Index;
					  LvItem.iSubItem=3;
					  LvItem.pszText=temp;
					  SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);
				  }
              }

              OldWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_FIND_IMPORT),GWL_WNDPROC,(LONG_PTR)ImportSearchProc);
          }
          catch(...){  // on error do nothing
          }
        }
        break;

        case WM_PAINT:{
            return false; //no paint
        }
        break;
        
        case WM_NOTIFY: // Notify Message, Displaing the tooltips for ToolBar
	    {
           switch(LOWORD(wParam)){
			   //Disassembler Control (Virtual List View)
              case IDC_IMPORTS_LIST:{
                 if(((LPNMHDR)lParam)->code == NM_CLICK || ((LPNMHDR)lParam)->code ==LVN_ITEMCHANGED){
                     LVITEM  LvItem;
                     DWORD_PTR   dwSelectedItem;
                     HWND    hImpList=GetDlgItem(hWnd,IDC_IMPORTS_LIST);
                     
                     memset(&LvItem,0,sizeof(LvItem)); // Init the struct to ZERO
                     LvItem.mask=LVIF_PARAM|LVIF_TEXT|LVIF_STATE;	// Information Style
                     LvItem.cchTextMax = 256;			// Max size of test
                     LvItem.state=LVIS_FOCUSED|LVIS_SELECTED;
                     dwSelectedItem=SendMessage(hImpList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
                     LvItem.iItem=(DWORD)dwSelectedItem;
                     LvItem.iSubItem=0;
                     SendMessage(hImpList,LVM_GETITEM,0,(LPARAM)&LvItem);
                     dwSelectedItem=LvItem.lParam;
                     SelectItem(GetDlgItem(Main_hWnd,IDC_DISASM),dwSelectedItem);
                 }
              }
              break;
           }
        }
        break;

        case WM_COMMAND:{
            switch(LOWORD(wParam)) // what we press on?
            {
                case IDC_IMPORT_CANCEL:{
                    EndDialog(hWnd,0);
                }
                break;

                case IDC_IMPORT_FINDNEXT:{
                    HWND hList = GetDlgItem(hWnd, IDC_IMPORTS_LIST);
                    int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                    int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                    DoImportSearch(hWnd, startIdx);
                }
                break;
            }
        }
        break;

        case WM_CLOSE:{
           EndDialog(hWnd,0); // destroy dialog
        }
        break;
    }
    return 0;
}


//======================================================================================
//============================  StringRefDlgProc =======================================
//======================================================================================
BOOL CALLBACK StringRefDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message){
        case WM_INITDIALOG:{
            ItrImports	itr;
            LV_COLUMN	LvCol;
            LVITEM		LvItem;
            HWND		hList;
            DWORD		i=0,Index=0,j=0,len;
            char		Api[128],temp[128],*ptr;
            
            // Set Radio Button Checked
            SendDlgItemMessage(hWnd,IDC_REF_CONTAIN,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);
            SetFocus(GetDlgItem(hWnd,IDC_FIND_REF));
            // SubClass the edit control
            OldWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_FIND_REF),GWL_WNDPROC,(LONG_PTR)StrRefSearchProc);
            Imp_hWnd=hWnd;
            hList=GetDlgItem(hWnd,IDC_STRING_REF_LV);
            SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT); // Set style

            // Add Columns
            memset(&LvCol,0,sizeof(LvCol));  // Init the struct to ZERO
            LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
            LvCol.pszText="Address";
            LvCol.cx=0x50;
            SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol); 
            LvCol.pszText="String Reference";
            LvCol.cx=0x120;
            SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);

            memset(&LvItem,0,sizeof(LvItem));	// Init the struct to ZERO
            LvItem.cchTextMax = 256;			// Max size of test

            try{
                itr=StrRefLines.begin();
                for(;itr!=StrRefLines.end();itr++,Index++){
                    i=(*itr);
                    LvItem.iItem=Index;
                    LvItem.iSubItem=0;
                    LvItem.pszText = DisasmDataLines[i].GetAddress();
                    LvItem.lParam = i;
                    LvItem.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
                    SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
                    LvItem.mask=LVIF_TEXT;

                    strcpy_s(temp,StringLen(DisasmDataLines[i].GetComments())+1,DisasmDataLines[i].GetComments());
                    ptr=temp;
					while(*ptr!=':'){
                        ptr++;
					}
                    ptr+=2;
                    strcpy_s(Api,StringLen(ptr)+1,ptr);
					len=StringLen(Api);
					if(Api[len-1]==']')
						Api[len-1]=NULL;
                    
                    LvItem.iItem=Index;
                    LvItem.iSubItem=1;
                    LvItem.pszText=Api;
                    SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem); 

                    SendDlgItemMessage(hWnd,IDC_REF_LIST,LB_ADDSTRING,0,(LPARAM)Api);
                    // attach item number as data to the text
                    SendDlgItemMessage(hWnd,IDC_REF_LIST,LB_SETITEMDATA,(WPARAM)Index,(LPARAM)i);
                }
            }
            catch(...){ // on error do nothing
            }
   
        }
        break;
        
        case WM_PAINT:{
            PAINTSTRUCT psRepaintPictures;
            HDC dcRepaintPictures;
            dcRepaintPictures = BeginPaint(hWnd,&psRepaintPictures);
            RepaintPictures(hWnd,dcRepaintPictures);
			EndPaint(hWnd,&psRepaintPictures);
        }
        break;
        
        case WM_COMMAND:{
            switch(LOWORD(wParam)) // what we press on?
            {
                case IDC_REF_CANCEL:{
                    EndDialog(hWnd,0);
                }
                break;

                case IDC_STRREF_FINDNEXT:{
                    HWND hList = GetDlgItem(hWnd, IDC_STRING_REF_LV);
                    int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                    int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                    DoStrRefSearch(hWnd, startIdx);
                }
                break;
            }
        }
        break;

        case WM_NOTIFY: // Notify Message
	    {
           switch(LOWORD(wParam)){
			   //Disassembler Control (Virtual List View)
              case IDC_STRING_REF_LV:{
                 if(((LPNMHDR)lParam)->code == NM_CLICK || ((LPNMHDR)lParam)->code ==LVN_ITEMCHANGED){
                     LVITEM  LvItem;
                     DWORD_PTR   dwSelectedItem;
                     HWND    hRefList=GetDlgItem(hWnd,IDC_STRING_REF_LV);
                     
                     memset(&LvItem,0,sizeof(LvItem)); // Init the struct to ZERO
                     LvItem.mask=LVIF_PARAM|LVIF_TEXT|LVIF_STATE;   // Information Style
                     LvItem.cchTextMax = 256;          // Max size of test
                     LvItem.state=LVIS_FOCUSED|LVIS_SELECTED;
                     dwSelectedItem=SendMessage(hRefList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
                     LvItem.iItem=(DWORD)dwSelectedItem;
                     LvItem.iSubItem=0;
                     SendMessage(hRefList,LVM_GETITEM,0,(LPARAM)&LvItem);
                     dwSelectedItem=LvItem.lParam;
                     SelectItem(GetDlgItem(Main_hWnd,IDC_DISASM),dwSelectedItem);
                 }
              }
              break;
           }
        }
        break;

        case WM_CLOSE:{
            EndDialog(hWnd,0); // destroy dialog
        }
        break;
    }
    return 0;
}

//======================================================================================
//============================  CommentSearchProc ======================================
//======================================================================================
static HWND Comment_hWnd = NULL;
static WNDPROC OldCommentWndProc = NULL;

// Helper function to find a comment match starting from a given index
// Returns the index of the found item, or -1 if not found
int FindCommentMatch(HWND hDlg, int startIndex, bool wrapAround)
{
    char SearchText[256], Temp[256];
    HWND hList = GetDlgItem(hDlg, IDC_COMMENT_REF_LV);
    int totalItems = ListView_GetItemCount(hList);

    if(totalItems == 0) return -1;

    // Get the search text
    SendDlgItemMessage(hDlg, IDC_COMMENT_FIND, WM_GETTEXT, (WPARAM)256, (LPARAM)SearchText);
    if(StringLen(SearchText) == 0) return -1;

    bool useContains = (SendDlgItemMessage(hDlg, IDC_COMMENT_CONTAIN, BM_GETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0) == BST_CHECKED);

    // Prepare lowercase search text for case-insensitive matching
    char SearchLower[256];
    lstrcpy(SearchLower, SearchText);
    CharLower(SearchLower);

    int searchCount = wrapAround ? totalItems : (totalItems - startIndex);

    for(int i = 0; i < searchCount; i++){
        int idx = (startIndex + i) % totalItems;

        ListView_GetItemText(hList, idx, 1, Temp, 256);

        bool found = false;
        if(useContains){
            // Partial String Matching (case insensitive)
            char TempLower[256];
            lstrcpy(TempLower, Temp);
            CharLower(TempLower);
            if(strstr(TempLower, SearchLower) != NULL){
                found = true;
            }
        }
        else{
            // Exact match (case insensitive)
            if(lstrcmpi(Temp, SearchText) == 0){
                found = true;
            }
        }

        if(found){
            return idx;
        }
    }

    return -1;
}

// Helper to deselect all items in the ListView
void DeselectAllComments(HWND hList)
{
    int count = ListView_GetItemCount(hList);
    for(int i = 0; i < count; i++){
        ListView_SetItemState(hList, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

// Helper to perform search and select result
void DoCommentSearch(HWND hDlg, int startIndex)
{
    HWND hList = GetDlgItem(hDlg, IDC_COMMENT_REF_LV);
    int foundIdx = FindCommentMatch(hDlg, startIndex, true);

    if(foundIdx != -1){
        SetFocus(hList);
        SelectItem(hList, foundIdx);
        SetFocus(GetDlgItem(hDlg, IDC_COMMENT_FIND));
    }
    else{
        // No match found - deselect all
        DeselectAllComments(hList);
    }
}

LRESULT CALLBACK CommentSearchProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
        case WM_KEYDOWN:
        {
            // F3 = Find Next
            if(wParam == VK_F3){
                HWND hList = GetDlgItem(Comment_hWnd, IDC_COMMENT_REF_LV);
                int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                DoCommentSearch(Comment_hWnd, startIdx);
                return 0;
            }
        }
        break;

        case WM_KEYUP:
        {
            // Don't search on F3 release
            if(wParam == VK_F3) break;

            char SearchText[256];
            SendDlgItemMessage(Comment_hWnd, IDC_COMMENT_FIND, WM_GETTEXT, (WPARAM)256, (LPARAM)SearchText);
            if(StringLen(SearchText) == 0){
                SetWindowText(Comment_hWnd, " Show Comments");
                // Deselect all when search is empty
                DeselectAllComments(GetDlgItem(Comment_hWnd, IDC_COMMENT_REF_LV));
            }
            else{
                SetWindowText(Comment_hWnd, SearchText);
                // Search from beginning
                DoCommentSearch(Comment_hWnd, 0);
            }
        }
        break;
    }

    return CallWindowProc(OldCommentWndProc, hWnd, uMsg, wParam, lParam);
}

//======================================================================================
//============================  CommentRefDlgProc ======================================
//======================================================================================
BOOL CALLBACK CommentRefDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message){
        case WM_INITDIALOG:{
            LV_COLUMN   LvCol;
            LVITEM      LvItem;
            HWND        hList;
            DWORD       Index=0;
            char        *comment;

            // Set Radio Button Checked
            SendDlgItemMessage(hWnd,IDC_COMMENT_CONTAIN,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);
            SetFocus(GetDlgItem(hWnd,IDC_COMMENT_FIND));

            // Subclass the edit control for search
            OldCommentWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_COMMENT_FIND),GWL_WNDPROC,(LONG_PTR)CommentSearchProc);
            Comment_hWnd=hWnd;

            hList=GetDlgItem(hWnd,IDC_COMMENT_REF_LV);
            SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);

            // Add Columns
            memset(&LvCol,0,sizeof(LvCol));
            LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
            LvCol.pszText="Address";
            LvCol.cx=0x50;
            SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);
            LvCol.pszText="Comment";
            LvCol.cx=0x140;
            SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);

            memset(&LvItem,0,sizeof(LvItem));
            LvItem.cchTextMax = 256;

            try{
                // Iterate through all disassembly lines and find those with comments
                for(DWORD i=0; i < DisasmDataLines.size(); i++){
                    comment = DisasmDataLines[i].GetComments();
                    // Check if comment exists and starts with ';'
                    if(comment != NULL && comment[0] == ';'){
                        LvItem.iItem=Index;
                        LvItem.iSubItem=0;
                        LvItem.pszText = DisasmDataLines[i].GetAddress();
                        LvItem.lParam = i;  // Store the line index
                        LvItem.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
                        SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
                        LvItem.mask=LVIF_TEXT;

                        LvItem.iItem=Index;
                        LvItem.iSubItem=1;
                        LvItem.pszText=comment;
                        SendMessage(hList,LVM_SETITEM,0,(LPARAM)&LvItem);

                        Index++;
                    }
                }
            }
            catch(...){
            }
        }
        break;

        case WM_PAINT:{
            PAINTSTRUCT psRepaintPictures;
            HDC dcRepaintPictures;
            dcRepaintPictures = BeginPaint(hWnd,&psRepaintPictures);
            RepaintPictures(hWnd,dcRepaintPictures);
            EndPaint(hWnd,&psRepaintPictures);
        }
        break;

        case WM_COMMAND:{
            switch(LOWORD(wParam)){
                case IDC_COMMENT_CANCEL:{
                    EndDialog(hWnd,0);
                }
                break;

                case IDC_COMMENT_FINDNEXT:{
                    // Get currently selected item
                    HWND hList = GetDlgItem(hWnd, IDC_COMMENT_REF_LV);
                    int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);

                    // Search starting from the next item (wrap around if needed)
                    int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                    DoCommentSearch(hWnd, startIdx);
                }
                break;
            }
        }
        break;

        case WM_NOTIFY:{
            switch(LOWORD(wParam)){
                case IDC_COMMENT_REF_LV:{
                    if(((LPNMHDR)lParam)->code == NM_CLICK || ((LPNMHDR)lParam)->code == LVN_ITEMCHANGED){
                        LVITEM      LvItem;
                        DWORD_PTR   dwSelectedItem;
                        HWND        hRefList=GetDlgItem(hWnd,IDC_COMMENT_REF_LV);

                        memset(&LvItem,0,sizeof(LvItem));
                        LvItem.mask=LVIF_PARAM|LVIF_TEXT|LVIF_STATE;
                        LvItem.cchTextMax = 256;
                        LvItem.state=LVIS_FOCUSED|LVIS_SELECTED;
                        dwSelectedItem=SendMessage(hRefList,LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
                        LvItem.iItem=(DWORD)dwSelectedItem;
                        LvItem.iSubItem=0;
                        SendMessage(hRefList,LVM_GETITEM,0,(LPARAM)&LvItem);
                        dwSelectedItem=LvItem.lParam;
                        SelectItem(GetDlgItem(Main_hWnd,IDC_DISASM),dwSelectedItem);
                    }
                }
                break;
            }
        }
        break;

        case WM_CLOSE:{
            EndDialog(hWnd,0);
        }
        break;
    }
    return 0;
}

//======================================================================================
//============================  DisasmColorsDlgProc ====================================
//======================================================================================
BOOL CALLBACK DisasmColorsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    COLORREF Color;
    static bool Change=false;

    switch(Message) {
        case WM_INITDIALOG:{
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_ADDSTRING,0,(LPARAM)"SoftIce");
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETITEMDATA,0,(LPARAM)0);
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_ADDSTRING,0,(LPARAM)"IDA");
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETITEMDATA,1,(LPARAM)1);
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_ADDSTRING,0,(LPARAM)"Ollydbg");
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETITEMDATA,2,(LPARAM)2);
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_ADDSTRING,0,(LPARAM)"W32Dasm");
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETITEMDATA,3,(LPARAM)3);
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_ADDSTRING,0,(LPARAM)"Custom");
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETITEMDATA,4,(LPARAM)4);
          SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
        }
        break;
        
		// Custom Drawing
        case WM_DRAWITEM:{
            HBRUSH brush;
            PDRAWITEMSTRUCT lp=(PDRAWITEMSTRUCT)lParam;
            
            switch(lp->CtlID){
                case IDC_ADDR_COLOR:		brush = CreateSolidBrush(DisasmColors.GetAddressTextColor());			break;
                case IDC_ADDR_BG_COLOR:		brush = CreateSolidBrush(DisasmColors.GetAddressBackGroundTextColor()); break;
                case IDC_OP_COLOR:			brush = CreateSolidBrush(DisasmColors.GetOpcodesTextColor());			break;
                case IDC_OP_BG_COLOR:		brush = CreateSolidBrush(DisasmColors.GetOpcodesBackGroundTextColor()); break;                
                case IDC_ASM_COLOR:			brush = CreateSolidBrush(DisasmColors.GetAssemblyTextColor());			break;
                case IDC_ASM_BG_COLOR:		brush = CreateSolidBrush(DisasmColors.GetAssemblyBackGroundTextColor());break;
                case IDC_REM_COLOR:			brush = CreateSolidBrush(DisasmColors.GetCommentsTextColor());			break;
                case IDC_REM_BG_COLOR:		brush = CreateSolidBrush(DisasmColors.GetCommentsBackGroundTextColor());break;
                case IDC_API_COLOR:			brush = CreateSolidBrush(DisasmColors.GetResolvedApiTextColor());		break;
                case IDC_CALL_COLOR:		brush = CreateSolidBrush(DisasmColors.GetCallAddressTextColor());		break;
                case IDC_JUMP_COLOR:		brush = CreateSolidBrush(DisasmColors.GetJumpAddressTextColor());		break;
                case IDC_DISASM_BG_COLOR:	brush = CreateSolidBrush(DisasmColors.GetBackGroundColor());			break;
            }
                
            SelectObject(lp->hDC,brush);
            SetBkMode(lp->hDC, TRANSPARENT);
            FillRect(lp->hDC,&lp->rcItem,brush);
            DeleteObject(brush);
        }
        break;

        case WM_PAINT:{
           return false; //no paint
        }
        break;

        case WM_COMMAND:{
            switch(LOWORD(wParam)) // what we press on?
            {
               // List Box messages
               case IDC_COLOR_SCHEME:{
                   switch (HIWORD(wParam)){
                        case CBN_SELCHANGE:{
                            DWORD_PTR iIndex;
                            iIndex=SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_GETCURSEL,0,0);
                            ChangeDisasmScheme(iIndex); // change schem
                            Change=true;
                            //Update BK Color
                            ListView_SetBkColor(GetDlgItem(Main_hWnd,IDC_DISASM),DisasmColors.GetBackGroundColor());
                            // Fake Refresh
                            ShowWindow(hWnd,SW_HIDE);
                            ShowWindow(hWnd,SW_SHOW);
                        }
                        break;
                   }
               }
               break;

                case IDC_ADDR_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetAddressTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_ADDR_BG_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetAddressBackGroundTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_OP_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetOpcodesTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_OP_BG_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetOpcodesBackGroundTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_ASM_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetAssemblyTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_ASM_BG_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetAssemblyBackGroundTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;


                case IDC_REM_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetCommentsTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_REM_BG_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetCommentsBackGroundTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_API_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetResolvedApiTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_CALL_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetCallAddressTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_JUMP_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetJumpAddressTextColor(Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_DISASM_BG_COLOR:{
                    Color=SetColor(0);
                    if(Color!=-1){
                        Change=true;
                        DisasmColors.SetBackGroundColor(Color);
                        ListView_SetBkColor(GetDlgItem(Main_hWnd,IDC_DISASM),Color);
                        ShowWindow(hWnd,SW_HIDE);
                        ShowWindow(hWnd,SW_SHOW);
						SendDlgItemMessage(hWnd,IDC_COLOR_SCHEME,CB_SETCURSEL,(WPARAM)4,0);
                    }
                }
                break;

                case IDC_SAVE:{
                    // Update colors only when disasm window is active
                    if(Change==true && DisassemblerReady==true){
                        Change=false;
                        ShowWindow(GetDlgItem(Main_hWnd,IDC_DISASM),SW_HIDE);
                        ShowWindow(GetDlgItem(Main_hWnd,IDC_DISASM),SW_SHOW);
                    }
                    EndDialog(hWnd,0);
                }
                break;
            }
        }
        break;
        
        case WM_CLOSE:{
            EndDialog(hWnd,0); // destroy dialog
        }
        break;
    }
    return 0;
}

//====================================================================================
//================================= AboutDlgProc =====================================
//====================================================================================

BOOL CALLBACK AboutDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message) // what are we doing ?
	{ 	 
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG: {	
			About_hWnd=hWnd;
			HRSRC hrSkin = FindResource(hInst, MAKEINTRESOURCE(IDR_BINARY),"BINARY");
			if (!hrSkin) return false;
			
			// this is standard "BINARY" retrieval.
			LPRGNDATA pSkinData = (LPRGNDATA)LoadResource(hInst, hrSkin);
			if (!pSkinData) return false;
			
			// create the region using the binary data.
			HRGN rgnSkin = ExtCreateRegion(NULL, SizeofResource(NULL,hrSkin), pSkinData);
			SetWindowRgn(hWnd, rgnSkin, true);
			InvalidateRect(hWnd, NULL, TRUE);
			
			// free the allocated resource
			FreeResource(pSkinData);
			AddPicture(hWnd,IDR_REGION,-3,-3); 
			OldWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_FAKE_EDIT),GWL_WNDPROC,(LONG_PTR)AboutDialogSubClass);
			SetDlgItemText(hWnd, IDC_ABOUT_VERSION, "PVDasm v" PVDASM_VERSION);
		}
		break;

		case WM_PAINT:{
			PAINTSTRUCT psRepaintPictures;
			HDC dcRepaintPictures;
			// Start painting
			dcRepaintPictures = BeginPaint(hWnd,&psRepaintPictures);
			// Refresh the Picture
			RepaintPictures(hWnd,dcRepaintPictures);
			EndPaint(hWnd,&psRepaintPictures);
		}
		break;

		case WM_CTLCOLORSTATIC:{ // coloring the Static Controls
			HBRUSH hbrushWindow = CreateSolidBrush(RGB(125,228,253));
			HDC hdcStatic = (HDC)wParam; // Static handles
			SetTextColor(hdcStatic, RGB(0,0,0)); // Text color
			SetBkMode(hdcStatic, TRANSPARENT); // Backround mode (note: OPAQUE can be used)
			SetBkColor(hdcStatic,(LONG)hbrushWindow); //	Backround color
			return (LONG)hbrushWindow; // Paint it
		}
		break;

		case WM_LBUTTONDOWN:{
			ReleaseCapture(); 
			SendMessage(hWnd,WM_NCLBUTTONDOWN,HTCAPTION,0); 
		}
		break;
		
        case WM_COMMAND:{
            switch(LOWORD(wParam)){
               case IDC_EXIT_ABOUT:
               case IDCANCEL:{
                   RemovePictures(hWnd);
                   EndDialog(hWnd,0);
               }
               break;
            }
        }
        break;

        case WM_CLOSE:{ // We colsing the Dialog
          RemovePictures(hWnd);
          EndDialog(hWnd,0); 
        }
	    break;
	}
	return 0;
}

//======================================================================================
//============================  XrefSearchProc =========================================
//======================================================================================
static HWND XRef_hWnd = NULL;
static WNDPROC OldXrefSearchWndProc = NULL;

// Helper function to find an xref match starting from a given index
// Returns the index of the found item, or -1 if not found
int FindXrefMatch(HWND hDlg, int startIndex, bool wrapAround)
{
    char SearchText[256], Temp[512];
    HWND hList = GetDlgItem(hDlg, IDC_XREF_LV);
    int totalItems = ListView_GetItemCount(hList);

    if(totalItems == 0) return -1;

    // Get the search text
    SendDlgItemMessage(hDlg, IDC_XREF_FIND, WM_GETTEXT, (WPARAM)256, (LPARAM)SearchText);
    if(StringLen(SearchText) == 0) return -1;

    bool useContains = (SendDlgItemMessage(hDlg, IDC_XREF_CONTAIN, BM_GETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0) == BST_CHECKED);

    // Prepare lowercase search text for case-insensitive matching
    char SearchLower[256];
    lstrcpy(SearchLower, SearchText);
    CharLower(SearchLower);

    int searchCount = wrapAround ? totalItems : (totalItems - startIndex);

    for(int i = 0; i < searchCount; i++){
        int idx = (startIndex + i) % totalItems;

        // Check Address column
        ListView_GetItemText(hList, idx, 0, Temp, 256);
        char TempLower[512];
        lstrcpy(TempLower, Temp);
        CharLower(TempLower);

        bool found = false;
        if(useContains){
            if(strstr(TempLower, SearchLower) != NULL) found = true;
        }
        else{
            if(lstrcmpi(Temp, SearchText) == 0) found = true;
        }

        if(!found){
            // Check Disassembly column
            ListView_GetItemText(hList, idx, 1, Temp, 512);
            lstrcpy(TempLower, Temp);
            CharLower(TempLower);
            if(useContains){
                if(strstr(TempLower, SearchLower) != NULL) found = true;
            }
            else{
                if(lstrcmpi(Temp, SearchText) == 0) found = true;
            }
        }

        if(found) return idx;
    }

    return -1;
}

// Helper to deselect all items in the XRef ListView
void DeselectAllXrefs(HWND hList)
{
    int count = ListView_GetItemCount(hList);
    for(int i = 0; i < count; i++){
        ListView_SetItemState(hList, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

// Helper to perform search and select result
void DoXrefSearch(HWND hDlg, int startIndex)
{
    HWND hList = GetDlgItem(hDlg, IDC_XREF_LV);
    int foundIdx = FindXrefMatch(hDlg, startIndex, true);

    if(foundIdx != -1){
        SetFocus(hList);
        SelectItem(hList, foundIdx);
        SetFocus(GetDlgItem(hDlg, IDC_XREF_FIND));
    }
    else{
        DeselectAllXrefs(hList);
    }
}

LRESULT CALLBACK XrefSearchProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
        case WM_KEYDOWN:
        {
            // F3 = Find Next
            if(wParam == VK_F3){
                HWND hList = GetDlgItem(XRef_hWnd, IDC_XREF_LV);
                int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                DoXrefSearch(XRef_hWnd, startIdx);
                return 0;
            }
        }
        break;

        case WM_KEYUP:
        {
            if(wParam == VK_F3) break;

            char SearchText[256];
            SendDlgItemMessage(XRef_hWnd, IDC_XREF_FIND, WM_GETTEXT, (WPARAM)256, (LPARAM)SearchText);
            if(StringLen(SearchText) == 0){
                SetWindowText(XRef_hWnd, " XReferences");
                DeselectAllXrefs(GetDlgItem(XRef_hWnd, IDC_XREF_LV));
            }
            else{
                SetWindowText(XRef_hWnd, SearchText);
                DoXrefSearch(XRef_hWnd, 0);
            }
        }
        break;
    }

    return CallWindowProc(OldXrefSearchWndProc, hWnd, uMsg, wParam, lParam);
}

//======================================================================================
//==================================  XrefDlgProc ======================================
//======================================================================================

BOOL CALLBACK XrefDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message){
        case WM_INITDIALOG:{
            LV_COLUMN   LvCol;
            LVITEM      LvItem;
            HWND        hList;
            DWORD       Index=0;
            ItrXref     it;
            char        asmText[512];

            // Set Radio Button Checked
            SendDlgItemMessage(hWnd, IDC_XREF_CONTAIN, BM_SETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0);
            SetFocus(GetDlgItem(hWnd, IDC_XREF_FIND));

            // Subclass the edit control for search
            OldXrefSearchWndProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd, IDC_XREF_FIND), GWL_WNDPROC, (LONG_PTR)XrefSearchProc);
            XRef_hWnd = hWnd;

            // Display the target address
            char targetAddr[16];
            wsprintf(targetAddr, "%08X", (DWORD)iSelected);
            char titleText[64]; wsprintf(titleText, " XReferences to: %s", targetAddr); SetWindowText(hWnd, titleText);
            SetDlgItemText(hWnd, IDC_XREF_JUMPBACK, targetAddr);

            hList = GetDlgItem(hWnd, IDC_XREF_LV);
            SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

            // Add Columns
            memset(&LvCol, 0, sizeof(LvCol));
            LvCol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
            LvCol.pszText = "Address";
            LvCol.cx = 0x50;
            SendMessage(hList, LVM_INSERTCOLUMN, 0, (LPARAM)&LvCol);
            LvCol.pszText = "Disassembly";
            LvCol.cx = 0x180;
            SendMessage(hList, LVM_INSERTCOLUMN, 1, (LPARAM)&LvCol);

            memset(&LvItem, 0, sizeof(LvItem));
            LvItem.cchTextMax = 512;

            try{
                // Find all xrefs to the selected address
                it = XrefData.find((DWORD)iSelected);
                for(DWORD i = 0; i < XrefData.count((DWORD)iSelected); i++, it++){
                    DWORD_PTR lineIdx = (*it).second;

                    LvItem.iItem = Index;
                    LvItem.iSubItem = 0;
                    LvItem.pszText = DisasmDataLines[lineIdx].GetAddress();
                    LvItem.lParam = lineIdx;  // Store the line index
                    LvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE;
                    SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM)&LvItem);
                    LvItem.mask = LVIF_TEXT;

                    // Build disassembly text: mnemonic (the assembly instruction)
                    wsprintf(asmText, "%s", DisasmDataLines[lineIdx].GetMnemonic());
                    LvItem.iItem = Index;
                    LvItem.iSubItem = 1;
                    LvItem.pszText = asmText;
                    SendMessage(hList, LVM_SETITEM, 0, (LPARAM)&LvItem);

                    Index++;
                }
            }
            catch(...){
            }
        }
        break;

        case WM_PAINT:{
            PAINTSTRUCT psRepaintPictures;
            HDC dcRepaintPictures;
            dcRepaintPictures = BeginPaint(hWnd, &psRepaintPictures);
            RepaintPictures(hWnd, dcRepaintPictures);
            EndPaint(hWnd, &psRepaintPictures);
        }
        break;


        case WM_CTLCOLORSTATIC:{
            if((HWND)lParam == GetDlgItem(hWnd, IDC_XREF_JUMPBACK)){
                HDC hdcStatic = (HDC)wParam;
                SetTextColor(hdcStatic, RGB(0, 0, 255));
                SetBkMode(hdcStatic, TRANSPARENT);
                return (INT_PTR)GetStockObject(NULL_BRUSH);
            }
        }
        break;

        case WM_SETCURSOR:{
            if((HWND)wParam == GetDlgItem(hWnd, IDC_XREF_JUMPBACK)){
                SetCursor(LoadCursor(NULL, IDC_HAND));
                return TRUE;
            }
        }
        break;
        case WM_COMMAND:{
            switch(LOWORD(wParam)){
                case IDC_XREF_CANCEL:{
                    EndDialog(hWnd, 0);
                }
                break;

                case IDC_XREF_FINDNEXT:{
                    HWND hList = GetDlgItem(hWnd, IDC_XREF_LV);
                    int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                    int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                    DoXrefSearch(hWnd, startIdx);
                }
                break;

                case IDC_XREF_JUMPBACK:{
                    // Jump to the target address in the main disassembly
                    char targetAddr[16];
                    GetDlgItemText(hWnd, IDC_XREF_JUMPBACK, targetAddr, 16);
                    DWORD_PTR targetIndex = SearchItemText(GetDlgItem(Main_hWnd, IDC_DISASM), targetAddr);
                    if(targetIndex != -1){
                        SelectItem(GetDlgItem(Main_hWnd, IDC_DISASM), targetIndex);
                    }
                }
                break;
            }
        }
        break;

        case WM_NOTIFY:{
            switch(LOWORD(wParam)){
                case IDC_XREF_LV:{
                    if(((LPNMHDR)lParam)->code == NM_CLICK || ((LPNMHDR)lParam)->code == LVN_ITEMCHANGED){
                        LVITEM      LvItem;
                        DWORD_PTR   dwSelectedItem;
                        HWND        hRefList = GetDlgItem(hWnd, IDC_XREF_LV);

                        memset(&LvItem, 0, sizeof(LvItem));
                        LvItem.mask = LVIF_PARAM | LVIF_TEXT | LVIF_STATE;
                        LvItem.cchTextMax = 256;
                        LvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
                        dwSelectedItem = SendMessage(hRefList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                        LvItem.iItem = (DWORD)dwSelectedItem;
                        LvItem.iSubItem = 0;
                        SendMessage(hRefList, LVM_GETITEM, 0, (LPARAM)&LvItem);
                        dwSelectedItem = LvItem.lParam;
                        SelectItem(GetDlgItem(Main_hWnd, IDC_DISASM), dwSelectedItem);
                    }
                }
                break;
            }
        }
        break;

        case WM_CLOSE:{
            EndDialog(hWnd, 0);
        }
        break;
    }
    return 0;
}

//======================================================================================
//==================================  FindDlgProc ======================================
//======================================================================================

BOOL CALLBACK FindDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static bool bMatchCase=FALSE,bWholeWord=FALSE;
    HWND hListView = GetDlgItem(Main_hWnd,IDC_DISASM);
    static DWORD_PTR iItem=0;
    char TextSearch[256]="";

	switch (Message){ // what are we doing ?
		// This Window Message is the heart of the dialog //
		//================================================//
		case WM_INITDIALOG:{	
			// Intialize the Check buttons.
			if(bMatchCase==false){
                SendDlgItemMessage(hWnd,IDC_MATCH_CASE,BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);
			}
			else{
                SendDlgItemMessage(hWnd,IDC_MATCH_CASE,BM_SETCHECK,(WPARAM)BST_CHECKED,0);
			}

			if(bWholeWord==false){
                SendDlgItemMessage(hWnd,IDC_WHOLE_WORD,BM_SETCHECK,(WPARAM)BST_UNCHECKED,0);
			}
			else{
                SendDlgItemMessage(hWnd,IDC_WHOLE_WORD,BM_SETCHECK,(WPARAM)BST_CHECKED,0);
			}
			
			SetFocus(GetDlgItem(hWnd,IDC_FIND_STRING));
		}
		break;
			
        case WM_COMMAND:{
            switch(LOWORD(wParam)){
               case IDC_FIND_IT:{
                   // Get String to Search
                   GetDlgItemText(hWnd,IDC_FIND_STRING,TextSearch,256);
				   if(StringLen(TextSearch)==0){ // If blank, Don't Search
                       return 0;
				   }

                   // Check For Changes
				   if(SendDlgItemMessage(hWnd,IDC_MATCH_CASE,BM_GETCHECK,(WPARAM)BST_UNCHECKED,0)==BST_CHECKED){
                       bMatchCase=TRUE;
				   }
				   else{
                       bMatchCase=FALSE;
				   }
                   
				   if(SendDlgItemMessage(hWnd,IDC_WHOLE_WORD,BM_GETCHECK,(WPARAM)BST_UNCHECKED,0)==BST_CHECKED){
                       bWholeWord=TRUE;
				   }
				   else{
                       bWholeWord=FALSE;
				   }

                   iItem=SearchText(-1,TextSearch,hListView,bMatchCase,bWholeWord); // Search All Fields
                   if(iItem!=-1) SelectItem(hListView,iItem); else return 0;  // if Found, Select the Item & Scroll
               }
               break;

               case IDC_FIND_NEXT: {
                   // Atleast 1 Search has been done
                   // In order to find next matches.
                   if(iItem!=0){
                       // Get String to Search
                       GetDlgItemText(hWnd,IDC_FIND_STRING,TextSearch,256);
                       if(StringLen(TextSearch)==0) // If blank, Don't Search
                           return 0;

                       // Check For Changes
					   if(SendDlgItemMessage(hWnd,IDC_MATCH_CASE,BM_GETCHECK,(WPARAM)BST_UNCHECKED,0)==BST_CHECKED){
                           bMatchCase=TRUE;
					   }
					   else{
                           bMatchCase=FALSE;
					   }
                       
					   if(SendDlgItemMessage(hWnd,IDC_WHOLE_WORD,BM_GETCHECK,(WPARAM)BST_UNCHECKED,0)==BST_CHECKED){
                           bWholeWord=TRUE;
					   }
					   else{
                           bWholeWord=FALSE;
					   }
                       
                       iItem=SearchText(iItem+1,TextSearch,hListView,bMatchCase,bWholeWord); // Search All Fields
                       if(iItem!=-1) SelectItem(hListView,iItem); else return 0; // if Found, Select the Item & Scroll
                   }
               }
               break;

               case IDCANCEL:{
                   SendMessage(hWnd,WM_CLOSE,0,0);
               }
               break;
            }
        }
        break;

        case WM_CLOSE:{ // We colsing the Dialog
            // Saving Last Positions
			if(SendDlgItemMessage(hWnd,IDC_MATCH_CASE,BM_GETCHECK,(WPARAM)BST_UNCHECKED,0)==BST_CHECKED){
                bMatchCase=TRUE;
			}
			else{
                bMatchCase=FALSE;
			}
            
			if(SendDlgItemMessage(hWnd,IDC_WHOLE_WORD,BM_GETCHECK,(WPARAM)BST_UNCHECKED,0)==BST_CHECKED){
                bWholeWord=TRUE;
			}
			else{
                bWholeWord=FALSE;
			}
            
            // Close the Window
            EndDialog(hWnd,0);
        }
	    break;
	}
	return 0;
}

//================================================================//
//================== CUSTOM DRAW PROCEDURES ======================//
//================================================================//
// TODO: Add if needed.

//================================================================//
//============== Custom Draw For ListView Control ================//
//================================================================//


// Custom draw handler for ListView header (dark mode support)
LRESULT ProcessHeaderCustomDraw(LPARAM lParam)
{
    LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;

    switch(nmcd->dwDrawStage) {
        case CDDS_PREPAINT:
            if (g_DarkMode) {
                return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
            }
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT: {
            if (g_DarkMode) {
                // Fill background with dark color
                HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
                FillRect(nmcd->hdc, &nmcd->rc, hBrush);
                DeleteObject(hBrush);

                // Set text color
                SetTextColor(nmcd->hdc, g_DarkTextColor);
                SetBkMode(nmcd->hdc, TRANSPARENT);

                // Get header item text
                HWND hHeader = nmcd->hdr.hwndFrom;
                HDITEM hdi = {0};
                char szText[256] = {0};
                hdi.mask = HDI_TEXT;
                hdi.pszText = szText;
                hdi.cchTextMax = sizeof(szText);
                Header_GetItem(hHeader, nmcd->dwItemSpec, &hdi);

                // Draw text centered
                RECT rc = nmcd->rc;
                rc.left += 4;
                DrawTextA(nmcd->hdc, szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                // Draw dark divider line on right edge
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(70, 70, 70));
                HPEN hOldPen = (HPEN)SelectObject(nmcd->hdc, hPen);
                MoveToEx(nmcd->hdc, nmcd->rc.right - 1, nmcd->rc.top, NULL);
                LineTo(nmcd->hdc, nmcd->rc.right - 1, nmcd->rc.bottom);
                SelectObject(nmcd->hdc, hOldPen);
                DeleteObject(hPen);

                return CDRF_SKIPDEFAULT;
            }
            return CDRF_DODEFAULT;
        }

        case CDDS_POSTPAINT: {
            if (g_DarkMode) {
                // Fill any remaining background area the theme might have drawn
                HWND hHeader = nmcd->hdr.hwndFrom;
                RECT rcHeader;
                GetClientRect(hHeader, &rcHeader);

                // Get the right edge of the last header item
                int itemCount = Header_GetItemCount(hHeader);
                if (itemCount > 0) {
                    RECT rcLastItem;
                    Header_GetItemRect(hHeader, itemCount - 1, &rcLastItem);
                    if (rcLastItem.right < rcHeader.right) {
                        // Fill the area after the last header item
                        RECT rcFill = { rcLastItem.right, rcHeader.top, rcHeader.right, rcHeader.bottom };
                        HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
                        FillRect(nmcd->hdc, &rcFill, hBrush);
                        DeleteObject(hBrush);
                    }
                }
            }
            return CDRF_DODEFAULT;
        }
    }
    return CDRF_DODEFAULT;
}

LRESULT ProcessCustomDraw (LPARAM lParam)
{
    LPNMLVCUSTOMDRAW	lplvcd = (LPNMLVCUSTOMDRAW)lParam;
    LV_DISPINFO*		nmdisp = (LV_DISPINFO*)lParam;
    LV_ITEM*			pItem= &(nmdisp)->item;    
    
    switch(lplvcd->nmcd.dwDrawStage){
		case CDDS_PREPAINT :{ //Before the paint cycle begins
			//request notifications for individual list view items
			return CDRF_NOTIFYITEMDRAW;
		}
		break;

        case CDDS_ITEMPREPAINT:{ //Before an item is drawn
            // In dark mode, handle selection colors to avoid default blue highlight
            if (g_DarkMode && (lplvcd->nmcd.uItemState & CDIS_SELECTED)) {
                lplvcd->clrText = RGB(255, 255, 255);  // White text for selected
                lplvcd->clrTextBk = RGB(60, 60, 60);   // Dark gray background for selected
            }
            return CDRF_NOTIFYSUBITEMDRAW;
        }
        break;
        
        // Paint the List View's Items
        case CDDS_SUBITEM | CDDS_ITEMPREPAINT:{ //Before a subitem is drawn
            switch(lplvcd->iSubItem){   
				// Color the Address Field
                case 0:{
                    lplvcd->clrText   = DisasmColors.GetAddressTextColor();
					if(bToolTip==FALSE){
                       lplvcd->clrTextBk = DisasmColors.GetAddressBackGroundTextColor();
					}
					else{
                       lplvcd->clrTextBk=(COLORREF)RGB(255,255,225);
					}
					
                    return CDRF_NEWFONT;
                }
                break;
                
				// Color the Opcodes Field
                case 1:{
                    lplvcd->clrText = DisasmColors.GetOpcodesTextColor();
					if(bToolTip==FALSE){
                        lplvcd->clrTextBk = DisasmColors.GetOpcodesBackGroundTextColor();
					}
					else{
                        lplvcd->clrTextBk=(COLORREF)RGB(255,255,225);
					}

                    return CDRF_NEWFONT;
                }
                break;  
                
                case 2:{
                    int i;
                    // Set Imports Colors
                    for(i=0;i<ImportsLines.size();i++){
                        if(ImportsLines[i] == lplvcd->nmcd.dwItemSpec){
                            lplvcd->clrText = DisasmColors.GetResolvedApiTextColor();
                            return CDRF_NEWFONT;
                        }
                    }

                    // Set Jmp/Call Colors
                    for(i=0;i<DisasmCodeFlow.size();i++){
                        if(DisasmCodeFlow[i].Current_Index == lplvcd->nmcd.dwItemSpec){
							if(DisasmCodeFlow[i].BranchFlow.Call==TRUE){
                                lplvcd->clrText = DisasmColors.GetCallAddressTextColor();
							}
							else{
								if(DisasmCodeFlow[i].BranchFlow.Jump==TRUE){
									lplvcd->clrText = DisasmColors.GetJumpAddressTextColor();
								}
							}

                            return CDRF_NEWFONT;
                        }
                    }

                    // Set Normal Colors for Assembly
                    lplvcd->clrText = DisasmColors.GetAssemblyTextColor();
                    
					if(bToolTip==FALSE){
                        lplvcd->clrTextBk = DisasmColors.GetAssemblyBackGroundTextColor();
					}
					else{
                        lplvcd->clrTextBk=(COLORREF)RGB(255,255,225);
					}
					
                    return CDRF_NEWFONT;
                }
                break;
                
				// Color the Comments Field
                case 3: {
                    lplvcd->clrText = DisasmColors.GetCommentsTextColor();
                   
					if(bToolTip==FALSE){
                        lplvcd->clrTextBk = DisasmColors.GetCommentsBackGroundTextColor();
					}
					else{
                        lplvcd->clrTextBk=(COLORREF)RGB(255,255,225);
					}
					
                    return CDRF_NEWFONT;
                }
                break;

				// Color the References Field
                case 4: {
                    lplvcd->clrText = DisasmColors.GetCommentsTextColor();
                   
					if(bToolTip==FALSE){
                        lplvcd->clrTextBk = DisasmColors.GetCommentsBackGroundTextColor();
					}
					else{
                        lplvcd->clrTextBk=(COLORREF)RGB(255,255,225);
					}
					
                    return CDRF_NEWFONT;
                }
                break;
            }
        }
    }
    return CDRF_DODEFAULT;
}

//======================================================================================
//============================= SUB CLASS DIALOGS ======================================
//======================================================================================
// TODO: Add if needed.

//======================================================================================
//==================================  ImportSearchProc =================================
//======================================================================================

// Helper to deselect all items in the Imports ListView
void DeselectAllImports(HWND hList)
{
    int count = ListView_GetItemCount(hList);
    for(int i = 0; i < count; i++){
        ListView_SetItemState(hList, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

// Helper function to find an import match starting from a given index
int FindImportMatch(HWND hDlg, int startIndex, bool wrapAround)
{
    char SearchText[128], Temp[128];
    HWND hList = GetDlgItem(hDlg, IDC_IMPORTS_LIST);
    int totalItems = ListView_GetItemCount(hList);

    if(totalItems == 0) return -1;

    SendDlgItemMessage(hDlg, IDC_FIND_IMPORT, WM_GETTEXT, (WPARAM)128, (LPARAM)SearchText);
    if(StringLen(SearchText) == 0) return -1;

    bool useContains = (SendDlgItemMessage(hDlg, IDC_CONTAIN, BM_GETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0) == BST_CHECKED);

    char SearchLower[128];
    lstrcpy(SearchLower, SearchText);
    CharLower(SearchLower);

    int searchCount = wrapAround ? totalItems : (totalItems - startIndex);

    for(int i = 0; i < searchCount; i++){
        int idx = (startIndex + i) % totalItems;

        // Search in column 2 (Import Name)
        ListView_GetItemText(hList, idx, 2, Temp, 128);

        bool found = false;
        if(useContains){
            char TempLower[128];
            lstrcpy(TempLower, Temp);
            CharLower(TempLower);
            if(strstr(TempLower, SearchLower) != NULL){
                found = true;
            }
        }
        else{
            if(lstrcmpi(Temp, SearchText) == 0){
                found = true;
            }
        }

        if(found){
            return idx;
        }
    }

    return -1;
}

// Helper to perform import search and select result
void DoImportSearch(HWND hDlg, int startIndex)
{
    HWND hList = GetDlgItem(hDlg, IDC_IMPORTS_LIST);
    int foundIdx = FindImportMatch(hDlg, startIndex, true);

    if(foundIdx != -1){
        SetFocus(hList);
        SelectItem(hList, foundIdx);
        SetFocus(GetDlgItem(hDlg, IDC_FIND_IMPORT));
    }
    else{
        DeselectAllImports(hList);
    }
}

LRESULT CALLBACK ImportSearchProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
        case WM_KEYDOWN:
        {
            // F3 = Find Next
            if(wParam == VK_F3){
                HWND hList = GetDlgItem(Imp_hWnd, IDC_IMPORTS_LIST);
                int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                DoImportSearch(Imp_hWnd, startIdx);
                return 0;
            }
        }
        break;

        case WM_KEYUP:
        {
            if(wParam == VK_F3) break;

            char SearchText[128];
            SendDlgItemMessage(Imp_hWnd, IDC_FIND_IMPORT, WM_GETTEXT, (WPARAM)128, (LPARAM)SearchText);
            if(StringLen(SearchText) == 0){
                SetWindowText(Imp_hWnd, " Imported Functions");
                DeselectAllImports(GetDlgItem(Imp_hWnd, IDC_IMPORTS_LIST));
            }
            else{
                SetWindowText(Imp_hWnd, SearchText);
                DoImportSearch(Imp_hWnd, 0);
            }
        }
        break;
    }

    return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}

//======================================================================================
//===============================  StrRefSearchProc ====================================
//======================================================================================

// Helper to deselect all items in the String Refs ListView
void DeselectAllStrRefs(HWND hList)
{
    int count = ListView_GetItemCount(hList);
    for(int i = 0; i < count; i++){
        ListView_SetItemState(hList, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

// Helper function to find a string ref match starting from a given index
int FindStrRefMatch(HWND hDlg, int startIndex, bool wrapAround)
{
    char SearchText[128], Temp[128];
    HWND hList = GetDlgItem(hDlg, IDC_STRING_REF_LV);
    int totalItems = ListView_GetItemCount(hList);

    if(totalItems == 0) return -1;

    SendDlgItemMessage(hDlg, IDC_FIND_REF, WM_GETTEXT, (WPARAM)128, (LPARAM)SearchText);
    if(StringLen(SearchText) == 0) return -1;

    bool useContains = (SendDlgItemMessage(hDlg, IDC_REF_CONTAIN, BM_GETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0) == BST_CHECKED);

    char SearchLower[128];
    lstrcpy(SearchLower, SearchText);
    CharLower(SearchLower);

    int searchCount = wrapAround ? totalItems : (totalItems - startIndex);

    for(int i = 0; i < searchCount; i++){
        int idx = (startIndex + i) % totalItems;

        // Search in column 1 (String Reference)
        ListView_GetItemText(hList, idx, 1, Temp, 128);

        bool found = false;
        if(useContains){
            char TempLower[128];
            lstrcpy(TempLower, Temp);
            CharLower(TempLower);
            if(strstr(TempLower, SearchLower) != NULL){
                found = true;
            }
        }
        else{
            if(lstrcmpi(Temp, SearchText) == 0){
                found = true;
            }
        }

        if(found){
            return idx;
        }
    }

    return -1;
}

// Helper to perform string ref search and select result
void DoStrRefSearch(HWND hDlg, int startIndex)
{
    HWND hList = GetDlgItem(hDlg, IDC_STRING_REF_LV);
    int foundIdx = FindStrRefMatch(hDlg, startIndex, true);

    if(foundIdx != -1){
        SetFocus(hList);
        SelectItem(hList, foundIdx);
        SetFocus(GetDlgItem(hDlg, IDC_FIND_REF));
    }
    else{
        DeselectAllStrRefs(hList);
    }
}

LRESULT CALLBACK StrRefSearchProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
        case WM_KEYDOWN:
        {
            // F3 = Find Next
            if(wParam == VK_F3){
                HWND hList = GetDlgItem(Imp_hWnd, IDC_STRING_REF_LV);
                int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                DoStrRefSearch(Imp_hWnd, startIdx);
                return 0;
            }
        }
        break;

        case WM_KEYUP:
        {
            if(wParam == VK_F3) break;

            char SearchText[128];
            SendDlgItemMessage(Imp_hWnd, IDC_FIND_REF, WM_GETTEXT, (WPARAM)128, (LPARAM)SearchText);
            if(StringLen(SearchText) == 0){
                SetWindowText(Imp_hWnd, " String References");
                DeselectAllStrRefs(GetDlgItem(Imp_hWnd, IDC_STRING_REF_LV));
            }
            else{
                SetWindowText(Imp_hWnd, SearchText);
                DoStrRefSearch(Imp_hWnd, 0);
            }
        }
        break;
    }

    return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}

////////////////////////////////////////////////////////////////////////
// PreviewDialog: Show preview of jump/call destination on double-click //
//////////////////////////////////////////////////////////////////////////
BOOL CALLBACK ToolTipDialog(HWND hWnd,UINT Message,WPARAM wParam,LPARAM lParam)
{
	switch (Message){
		case WM_INITDIALOG:{
            LVCOLUMN LvCol;
            RECT dlgRect;
            HWND hWindow=GetDlgItem(hWnd,IDC_TOOLTIP_LIST);
            DWORD_PTR iItemIndex;
            char title[64];

            // Set window title with target address
            wsprintf(title, "Preview: %s", TempAddress);
            SetWindowText(hWnd, title);

            // Set ListView Styles
            SendMessage(hWindow,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT|LVS_EX_FLATSB|LVS_EX_DOUBLEBUFFER);

            // Setup columns
            ZeroMemory(&LvCol,0);
            LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
            LvCol.pszText="Address";
            LvCol.cx=LoadedPe64 ? 0x90 : 0x55;
            SendMessage(hWindow,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);
            LvCol.pszText="Opcodes";
            LvCol.cx=0x78;
            SendMessage(hWindow,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);
            LvCol.pszText="Disassembly";
            LvCol.cx=0x140;
            SendMessage(hWindow,LVM_INSERTCOLUMN,2,(LPARAM)&LvCol);
            LvCol.pszText="Comments";
            LvCol.cx=0x90;
            SendMessage(hWindow,LVM_INSERTCOLUMN,3,(LPARAM)&LvCol);
            LvCol.pszText="References";
            LvCol.cx=0x90;
            SendMessage(hWindow,LVM_INSERTCOLUMN,4,(LPARAM)&LvCol);

            // Apply dark mode if enabled
            if (g_DarkMode) {
                ListView_SetBkColor(hWindow, g_DarkBkColor);
                ListView_SetTextBkColor(hWindow, g_DarkBkColor);
                ListView_SetTextColor(hWindow, g_DarkTextColor);
            } else {
                ListView_SetBkColor(hWindow, (COLORREF)RGB(255,255,240));
            }

            // Center dialog on screen
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            GetWindowRect(hWnd, &dlgRect);
            int dlgWidth = dlgRect.right - dlgRect.left;
            int dlgHeight = dlgRect.bottom - dlgRect.top;
            int x = (screenWidth - dlgWidth) / 2;
            int y = (screenHeight - dlgHeight) / 2;
            SetWindowPos(hWnd, HWND_TOP, x, y, dlgWidth, dlgHeight, SWP_SHOWWINDOW);

            ToolTip_hWnd=hWnd;
            ListView_SetItemCountEx(hWindow,DisasmDataLines.size(),NULL);

            iItemIndex=SearchItemText(hWindow,TempAddress);
			if(iItemIndex!=-1){
                SelectItem(hWindow,iItemIndex);
			}

            ToolTipWndProc=(WNDPROC)SetWindowLongPtr(hWindow,GWL_WNDPROC,(LONG_PTR)ToolTipSubClass);
            UpdateWindow(hWnd);
		}
		break;

        case WM_NOTIFY:{
            switch(LOWORD(wParam)){
                case IDC_TOOLTIP_LIST:{
                    LPNMLISTVIEW	pnm		= (LPNMLISTVIEW)lParam;
                    HWND			hDisasm	= GetDlgItem(hWnd,IDC_TOOLTIP_LIST);
                    LV_DISPINFO*	nmdisp	= (LV_DISPINFO*)lParam;

                    // Custom draw for colors
                    if(pnm->hdr.code == NM_CUSTOMDRAW){
                        SetWindowLongPtr(hWnd, DWL_MSGRESULT, (LONG_PTR)ProcessCustomDraw(lParam));
                        return TRUE;
                    }

                    // Double-click to go to that address in main view
                    if(pnm->hdr.code == NM_DBLCLK){
                        DWORD_PTR sel = SendMessage(hDisasm, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                        if(sel != -1){
                            // Select in main disasm view and close preview
                            HWND hMainDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
                            SelectItem(hMainDisasm, sel);
                            EndDialog(hWnd, 0);
                        }
                        return TRUE;
                    }

                    if(((LPNMHDR)lParam)->code==LVN_GETDISPINFO){
                        try{
                            strcpy_s(Buffer,StringLen(DisasmDataLines[nmdisp->item.iItem].GetAddress())+1 ,DisasmDataLines[nmdisp->item.iItem].GetAddress());
                            strcpy_s(Buffer1,StringLen(DisasmDataLines[nmdisp->item.iItem].GetCode())+1,DisasmDataLines[nmdisp->item.iItem].GetCode());
                            strcpy_s(Buffer2,StringLen(DisasmDataLines[nmdisp->item.iItem].GetMnemonic())+1,DisasmDataLines[nmdisp->item.iItem].GetMnemonic());
                            strcpy_s(Buffer3,StringLen(DisasmDataLines[nmdisp->item.iItem].GetComments())+1,DisasmDataLines[nmdisp->item.iItem].GetComments());
                            strcpy_s(Buffer4,StringLen(DisasmDataLines[nmdisp->item.iItem].GetReference())+1,DisasmDataLines[nmdisp->item.iItem].GetReference());
                            nmdisp->item.pszText=Buffer;

                            if( nmdisp->item.mask & LVIF_TEXT){
                                switch(nmdisp->item.iSubItem){
                                    case 1:nmdisp->item.pszText=Buffer1;break;
                                    case 2:nmdisp->item.pszText=Buffer2;break;
                                    case 3:nmdisp->item.pszText=Buffer3;break;
                                    case 4:nmdisp->item.pszText=Buffer4;break;
                                }
                                return true;
                            }
                        }
                        catch(...){}
                    }

                    if(((LPNMHDR)lParam)->code==LVN_SETDISPINFO){
                        try{
                            nmdisp->item.pszText=Buffer;
                            if( nmdisp->item.mask & LVIF_TEXT){
                                switch(nmdisp->item.iSubItem){
                                    case 1:nmdisp->item.pszText=Buffer1;break;
                                    case 2:nmdisp->item.pszText=Buffer2;break;
                                    case 3:nmdisp->item.pszText=Buffer3;break;
                                    case 4:nmdisp->item.pszText=Buffer4;break;
                                }
                                return true;
                            }
                        }
                        catch(...){}
                    }
                }
                break;
            }
        }
        break;

        case WM_COMMAND:{
            switch(LOWORD(wParam)){
                case IDCANCEL:  // Escape key or close button
                    EndDialog(hWnd, 0);
                    return TRUE;
            }
        }
        break;

        case WM_CLOSE:{
		  EndDialog(hWnd,0);
        }
	    break;
	}
	return 0;
}

//======================================================================================
//===============================  AboutDialogSubClass =================================
//======================================================================================

LRESULT CALLBACK AboutDialogSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
        // Alow all keys to be processed
	    case WM_GETDLGCODE: return DLGC_WANTALLKEYS; break;
		
        // Listen to KeyDown Event
		case WM_KEYDOWN:{
			switch(wParam){
                // Esc key has been pressed, Kill the Window
				case VK_ESCAPE:{
					RemovePictures(About_hWnd);
					EndDialog(About_hWnd,0);
				}
				break;

                case VK_RETURN:{
                    EndDialog(About_hWnd,0);
                }
                break;
			}
        }
        break;
		
    }   
    return CallWindowProc(OldWndProc, hWnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////
//						TOOL TIP SUBCLASS								//
//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK ToolTipSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
		// All keys are acceptble
	    case WM_GETDLGCODE:{
			return DLGC_WANTALLKEYS;
		}
		break;

        case WM_INITDIALOG:{
            SetFocus(hWnd);
        }
        break;
        
        case WM_KEYDOWN:{
            switch(wParam){
                case VK_ESCAPE:{
                    EndDialog(ToolTip_hWnd,0);
                }
                break;
            }
        }
        break;
    }   
    return CallWindowProc(ToolTipWndProc, hWnd, uMsg, wParam, lParam);
}

//======================================================================================
//=================================  ListViewSubClass ==================================
//======================================================================================
//Branch Tracing
LRESULT CALLBACK ListViewSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
		// All keys are acceptble
	    case WM_GETDLGCODE:{
			return DLGC_WANTALLKEYS;
		}
		break;

        case WM_MOUSEMOVE:{
            POINT			mpt;
            LV_HITTESTINFO	hit;
			RECT			WindowRect;
            HWND			hDisasm=GetDlgItem(Main_hWnd,IDC_DISASM);
            int				iSelected,index=-1,i,y;

			if(DisassemblerReady==TRUE){ // disassembler finish decoding
				GetCursorPos(&mpt);
				ScreenToClient(hDisasm,&mpt);
				GetWindowRect(hDisasm,&WindowRect);
				
				hit.pt=mpt;
				hit.flags=LVHT_ABOVE | LVHT_BELOW | LVHT_NOWHERE | LVHT_ONITEMICON | LVHT_ONITEMLABEL | LVHT_ONITEMSTATEICON | LVHT_TOLEFT | LVHT_TORIGHT;
				hit.iItem=0;
				
				iSelected = HitColumnEx(hDisasm,hit.pt.x,5);
				if(iSelected==-1){
					break;
				}
				
				if(iSelected==2){ // Disassembly Column
					iSelected=HitRow(hDisasm,&hit);
					if(iSelected!=-1){ //Row Hit
						DWORD len,Address;
						
						len=StringLen(DisasmDataLines[iSelected].GetAddress());
						if(len!=0){ // only code (exclude entrypoint,procs)
							Address=StringToDword(DisasmDataLines[iSelected].GetAddress()); // Get Address
							for(i=0;i<DisasmCodeFlow.size();i++){ // Scan for code flows with current hit row
								if(DisasmCodeFlow[i].Current_Index==iSelected){
									index=i; // found code flow
									break;
								}
							}
							
							if(index!=-1){ // Show only flow
								if(DisasmCodeFlow[index].BranchFlow.Call==TRUE || DisasmCodeFlow[index].BranchFlow.Jump==TRUE){
									SCROLLINFO scrl;
									RECT rect;
									SIZE hSize;
									DWORD ColumnOffset=0,WholeText=0,addr_start=0,addr_end=0,mousepos=0;
									bool FoundImport=FALSE;
									char Target[20]="";
									
									memset(&scrl,0,sizeof(scrl));
									scrl.cbSize = sizeof(SCROLLINFO);
									scrl.fMask=SIF_ALL;

									for(y=0;y<ImportsLines.size();y++){
										if(ImportsLines[y]==DisasmCodeFlow[index].Current_Index){
											FoundImport=TRUE;
											break;
										}
									}
									
									if(FoundImport==TRUE){
										SetCursor(LoadCursor(NULL, IDC_ARROW));
										break; // don't allow imports to be viewable
									}

									// Check if destination is navigable (exists in our disassembly)
									char DestAddr[20];
									if(LoadedPe64)
										wsprintf(DestAddr,"%08X%08X",(DWORD)((DWORD_PTR)DisasmCodeFlow[index].Branch_Destination>>32),(DWORD)DisasmCodeFlow[index].Branch_Destination);
									else
										wsprintf(DestAddr,"%08X",DisasmCodeFlow[index].Branch_Destination);
									if(SearchItemText(hDisasm,DestAddr)!=-1){
										// Navigable address - show hand cursor
										SetCursor(LoadCursor(NULL, IDC_HAND));
									}
									else{
										SetCursor(LoadCursor(NULL, IDC_ARROW));
									}

									for(y=0;y<2;y++){
										ColumnOffset+=ListView_GetColumnWidth(hDisasm,y);
									}
									
									HDC hDc=GetWindowDC(hDisasm);
									ListView_GetItemRect(hDisasm,iSelected,&rect,LVIR_LABEL);
									GetTextExtentPoint32(hDc,DisasmDataLines[iSelected].GetMnemonic(),StringLen(DisasmDataLines[iSelected].GetMnemonic()),&hSize);
									WholeText=hSize.cx;
									if(LoadedPe64)
										wsprintf(Target,"%08X%08X",(DWORD)((DWORD_PTR)DisasmCodeFlow[index].Branch_Destination>>32),(DWORD)DisasmCodeFlow[index].Branch_Destination);
									else
										wsprintf(Target,"%08X",DisasmCodeFlow[index].Branch_Destination);
									GetTextExtentPoint32(hDc,Target,StringLen(Target),&hSize);
									ReleaseDC(hDisasm,hDc);
									GetScrollInfo(hDisasm,SB_HORZ,&scrl);
									// x for pixel padding
									// Check if we hover the address string only!
									mousepos=mpt.x;
									addr_start=70+ColumnOffset-(WholeText-hSize.cx);
									addr_end=45+ColumnOffset+WholeText-hSize.cx;
									if(scrl.nPos>0){
										GetCursorPos(&mpt);
										ScreenToClient(hDisasm,&mpt);
										GetWindowRect(hDisasm,&WindowRect);
										mousepos=mpt.x;
										addr_start-=scrl.nPos;
										addr_end-=scrl.nPos;
									}
									
									//
									// old code for checking if mouse is hovering over instruction
									//if( ((UINT((mpt.x+scrl.nPos)-ColumnOffset+20))>(UINT(WholeText-hSize.cx+20))) && ((UINT((mpt.x+scrl.nPos)-ColumnOffset+20))<UINT(WholeText+scrl.nPos)) )   // calculate range
									//
									// Hover tooltip disabled - use double-click on disassembly column instead
									// if(mousepos>=addr_start && mousepos<=addr_end){
									// 	strcpy_s(TempAddress,StringLen(Target)+1,Target);
									// 	if(SearchItemText(hDisasm,TempAddress)==-1){
									// 		break;
									// 	}
									// 	bToolTip=true;
									// 	DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_TOOLTIP), hWnd, (DLGPROC)ToolTipDialog);
									// }
								}
							}
							
							// ** other shows (not code flow) here **
						}
						else{
							// Not on a code flow instruction - reset cursor
							SetCursor(LoadCursor(NULL, IDC_ARROW));
						}
					}
				}
			}
        }
        break;

        case WM_KEYUP:{
			switch(wParam){
				// See if we can trace the call/jmp
                case VK_UP:
				case VK_DOWN:{   
                    // Tracing Calls/Jumps Instructions
                    if(DisassemblerReady==TRUE){
                      GetBranchDestination(hWnd);					  
					  GetXReferences(hWnd); // Check or Xreferences
                    }
				}
				break;

				// Trace the Call/Jmp
                case VK_RIGHT:{
                    // Check and see if disassembly is ready
                    if(DisassemblerReady==TRUE){
						GetBranchDestination(GetDlgItem(Main_hWnd,IDC_DISASM));
                        if(DCodeFlow==TRUE){
                          GotoBranch(); // We can trace
                        }
                    }
                }
                break;

				// Bak Trace
                case VK_LEFT:{
                    if(DisassemblerReady==TRUE){
                        BackTrace(); // Tracing Back
                    }
                }
                break;
			}
        }
        break;

        // Handle header custom draw for dark mode
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->code == NM_CUSTOMDRAW) {
                HWND hHeader = ListView_GetHeader(hWnd);
                if (pnmh->hwndFrom == hHeader && g_DarkMode) {
                    LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;
                    switch (nmcd->dwDrawStage) {
                        case CDDS_PREPAINT:
                            return CDRF_NOTIFYITEMDRAW;
                        case CDDS_ITEMPREPAINT: {
                            // Fill background
                            HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
                            FillRect(nmcd->hdc, &nmcd->rc, hBrush);
                            DeleteObject(hBrush);
                            
                            // Draw text
                            SetTextColor(nmcd->hdc, g_DarkTextColor);
                            SetBkMode(nmcd->hdc, TRANSPARENT);
                            
                            HDITEM hdi = {0};
                            char szText[256] = {0};
                            hdi.mask = HDI_TEXT;
                            hdi.pszText = szText;
                            hdi.cchTextMax = sizeof(szText);
                            Header_GetItem(hHeader, nmcd->dwItemSpec, &hdi);
                            
                            RECT rc = nmcd->rc;
                            rc.left += 6;
                            DrawTextA(nmcd->hdc, szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                            
                            return CDRF_SKIPDEFAULT;
                        }
                    }
                }
            }
        }
        break;
    }

    // Update Code Map position indicator on scroll/navigation events
    if (g_CodeMapVisible && (uMsg == WM_VSCROLL || uMsg == WM_MOUSEWHEEL || uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_LBUTTONUP)) {
        HWND hBar = GetDlgItem(Main_hWnd, IDC_CODE_MAP_BAR);
        if (hBar) {
            // Post a delayed invalidate so the listview processes the scroll first
            LRESULT result = CallWindowProc(LVOldWndProc, hWnd, uMsg, wParam, lParam);
            InvalidateRect(hBar, NULL, FALSE);
            return result;
        }
    }

    return CallWindowProc(LVOldWndProc, hWnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////
//                      MessageListSubClass                             //
//////////////////////////////////////////////////////////////////////////
// Handles header custom draw for message window in dark mode
LRESULT CALLBACK MessageListSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg) {
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->code == NM_CUSTOMDRAW && g_DarkMode) {
                HWND hHeader = ListView_GetHeader(hWnd);
                if (pnmh->hwndFrom == hHeader) {
                    LPNMCUSTOMDRAW nmcd = (LPNMCUSTOMDRAW)lParam;
                    switch (nmcd->dwDrawStage) {
                        case CDDS_PREPAINT: {
                            RECT rcHeader;
                            GetClientRect(hHeader, &rcHeader);
                            HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
                            FillRect(nmcd->hdc, &rcHeader, hBrush);
                            DeleteObject(hBrush);
                            return CDRF_NOTIFYITEMDRAW;
                        }
                        case CDDS_ITEMPREPAINT: {
                            HBRUSH hBrush = CreateSolidBrush(RGB(45, 45, 45));
                            FillRect(nmcd->hdc, &nmcd->rc, hBrush);
                            DeleteObject(hBrush);
                            SetTextColor(nmcd->hdc, g_DarkTextColor);
                            SetBkMode(nmcd->hdc, TRANSPARENT);
                            HDITEM hdi = {0};
                            char szText[256] = {0};
                            hdi.mask = HDI_TEXT;
                            hdi.pszText = szText;
                            hdi.cchTextMax = sizeof(szText);
                            Header_GetItem(hHeader, nmcd->dwItemSpec, &hdi);
                            RECT rc = nmcd->rc;
                            rc.left += 6;
                            DrawTextA(nmcd->hdc, szText, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                            return CDRF_SKIPDEFAULT;
                        }
                    }
                }
            }
        }
        break;
    }
    return CallWindowProc(MsgListOldWndProc, hWnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////////////
//							FreeMemory									//
//////////////////////////////////////////////////////////////////////////
//
// Releases the Memory of all variables and loaded files.
void FreeMemory(HWND hWnd)
{
    // ùçøåø äæéëøåï ò"é îçé÷ú ëîåú îñåééîú ùì ðúåðéí ä÷ééîéí áæéëøåï
    CloseFiles(hWnd); // close files handles.
    
    //
    // Free Disassembler Information From Memory.
    //
    
    // Free Disasm data from memory.
    DisasmDataLines.clear();
    DisasmDataArray(DisasmDataLines).swap(DisasmDataLines);
    
    // Free Imports data from memory.
    ImportsLines.clear();
    DisasmImportsArray(ImportsLines).swap(ImportsLines);
    
    // Free String ref data from memory.
    StrRefLines.clear();
    DisasmImportsArray(StrRefLines).swap(StrRefLines);
    
    // Free The Data.
    DisasmCodeFlow.clear();
    CodeBranch(DisasmCodeFlow).swap(DisasmCodeFlow);
    
    // Clear Tracing List.
    BranchTrace.clear();
    BranchData(BranchTrace).swap(BranchTrace);
    
    // Clear Xref List.
    XrefData.clear();
    XRef(XrefData).swap(XrefData);
    
    DataAddersses.clear();
    MapTree(DataAddersses).swap(DataAddersses);
        
    // clear wizard data vars.
    DataSection.clear();
    DataMapTree(DataSection).swap(DataSection);

    // clear wizard code information.
    CodeSections.clear();
    CodeSectionArray(CodeSections).swap(CodeSections);

	// Clear Function Information.
	fFunctionInfo.clear();
	FunctionInfo(fFunctionInfo).swap(fFunctionInfo);
	// Clear Export Information.
	fExportInfo.clear();
	ExportInfoArray(fExportInfo).swap(fExportInfo);
	// Clear Branch Targets.
	BranchTargets.clear();
	IntArray(BranchTargets).swap(BranchTargets);
    
    //////////////////////////////
    // Clear Wizard's variables //
    //////////////////////////////
    if(DataSection.size()!=0){
        ItrDataMap DItr=DataSection.begin();
        for(int i=0;i<DataSection.size();i++,DItr++){
            delete[] (*DItr).second; // delete allocated memory.
        }
    }
    
    WizardCodeList.clear();
    WizardList(WizardCodeList).swap(WizardCodeList);
    
    // Set FirstPass ON, on new file loading.
    LoadFirstPass=TRUE;
    
    SetWindowText(hWnd,PVDASM);
    Clear(GetDlgItem(hWnd,IDC_DISASM));
}

void ExitPVDasm(HWND hWnd)
{
	if( MessageBox(hWnd,"Exit From PVDasm Disassembler?","Notice",MB_YESNO)==IDYES){
		// Free handlers from memory
		CloseFiles(hWnd);
		PostQuitMessage(0);
		EndDialog(hWnd,0); 
	}
}

void CloseLoadedFile(HWND hWnd)
{
	// Do we have already files loaded ?
    if(FilesInMemory==true){
        //
        // Free the file handles, memory mapped handles
        //
        FreeMemory(hWnd);
        HMENU hWinMenu=GetMenu(hWnd);

        //
        // Disable Menus/Toolbar Buttons
        //
        EnableMenuItem(hWinMenu,IDC_CLOSEFILE,MF_GRAYED);

        EnableMenuItem(hWinMenu, ID_PE_EDIT, MF_GRAYED);
        EnableMenuItem(hWinMenu, IDM_FIND,   MF_GRAYED);

        EnableMenuItem(hWinMenu, IDM_COPY_DISASM_FILE, MF_GRAYED);
        EnableMenuItem(hWinMenu, IDM_COPY_DISASM_CLIP, MF_GRAYED);
        EnableMenuItem(hWinMenu, IDM_SELECT_ALL_ITEMS, MF_GRAYED);

        EnableMenuItem(hWinMenu, IDC_GOTO_START,		MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_GOTO_ENTRYPOINT,	MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_GOTO_ADDRESS,		MF_GRAYED);
        EnableMenuItem(hWinMenu, ID_GOTO_JUMP,			MF_GRAYED);
        EnableMenuItem(hWinMenu, ID_GOTO_RETURN_JUMP,	MF_GRAYED);
        EnableMenuItem(hWinMenu, ID_GOTO_EXECUTECALL,	MF_GRAYED);
        EnableMenuItem(hWinMenu, ID_GOTO_RETURN_CALL,	MF_GRAYED);

        EnableMenuItem(hWinMenu, IDC_START_DISASM,			MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_STOP_DISASM,			MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_PAUSE_DISASM,			MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_DISASM_IMP,			MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_STR_REFERENCES,		MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_SHOW_COMMENTS,			MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_XREF_MENU,				MF_GRAYED);
        EnableMenuItem(hWinMenu, IDM_PATCHER,				MF_GRAYED);
        EnableMenuItem(hWinMenu, IDM_FIND,					MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_DISASM_ADD_COMMENT,	MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_VIEW_CFG,			MF_GRAYED);

        // Hide Code Map bar when file is closed
        {
            HWND hBar = GetDlgItem(hWnd, IDC_CODE_MAP_BAR);
            if (hBar && IsWindowVisible(hBar)) {
                ShowWindow(hBar, SW_HIDE);
                RepositionDisasmForCodeMap(hWnd, FALSE);
            }
            g_CodeMapTypes.clear();
        }

        // Hide window When file is closed
	    ShowWindow(GetDlgItem(hWnd,IDC_DISASM),SW_HIDE);

        // Auto Disable buttons on startup
        SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_PE_EDITOR,		(LPARAM)FALSE);	// Disable Buttons
        SendMessage(hWndTB, TB_ENABLEBUTTON, IDC_SAVE_DISASM,	(LPARAM)FALSE);	// at first run
        
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EP_SHOW,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_CODE_SHOW,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_ADDR_SHOW,	(LPARAM)FALSE);
        
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SHOW_IMPORTS,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SHOW_XREF,		(LPARAM)FALSE);
        
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_JUMP,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EXEC_CALL,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL,	(LPARAM)FALSE);
        
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_XREF,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_CHECKBUTTON, ID_DOCK_DBG,(LPARAM)TRUE);
        
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SEARCH,	(LPARAM)FALSE);
        SendMessage(hWndTB, TB_ENABLEBUTTON, ID_PATCH,	(LPARAM)FALSE);
    }
}

//////////////////////////////////////////////////////////////////////////
//                         ListViewXrefSubClass                         //      
//////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK ListViewXrefSubClass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
		// All keys are acceptble
	    case WM_GETDLGCODE:{
			return DLGC_WANTALLKEYS;
		}
		break;
        
        case WM_KEYDOWN:{
			switch(wParam){
				// Trace the Call/Jmp
                case VK_ESCAPE:{
                    if(XRefWnd!=NULL){
                        EndDialog(XRefWnd,0);
                        XRefWnd=NULL;
                    }
                }
                break;

                case VK_RETURN:{
                    DWORD_PTR Index,data;
                    Index=SendMessage(hWnd,LB_GETCURSEL,0,0);
                    data = SendMessage(hWnd,LB_GETITEMDATA,(WPARAM)Index,0);
                    SelectItem(GetDlgItem(Main_hWnd,IDC_DISASM),data); // Select the item in disassembly window
                }
                break;
			}
        }
        break;
    }

    return CallWindowProc(LVXRefOldWndProc, hWnd, uMsg, wParam, lParam);
}


//=================================================================================
//============================== WinMain ==========================================
//=================================================================================

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	try{ // load the dialog and take care of exceptions.
		  hInst=hInstance;
          // Load HexEditor AddIn.
#ifdef _WIN64
          hRadHex = LoadLibrary("AddIns/RAHexEd64.dll");
#else
          hRadHex = LoadLibrary("AddIns/RAHexEd.dll");
#endif
          // Search For Plug-ins.
          EnumPlugins();
          // Load PVDasm's Main GUI Dialog.
		  DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, (DLGPROC)DialogProc,0);
          // Free Loaded Plug-ins.
          FreePlugins();
          // Release AddIn Handler.
		  FreeLibrary(hRadHex);
		  ExitProcess(0);
          return 0;
	}
	catch(...){ // Exception accured in PVDasm.
        //int error_no = GetLastError();
		if(hRadHex!=NULL){
            FreeLibrary(hRadHex);
		}

		// Free the Plugin's Handlers on exeptions.
        FreePlugins();
		
		// Show Error
		wsprintf(Buffer,"Unknown Error (%d) in PVDasm has been Intercepted!",GetLastError());
        MessageBox(NULL,Buffer,"Nasty Error",MB_OK|MB_ICONINFORMATION);
		ExitProcess(0);
		return 0;
	}
}