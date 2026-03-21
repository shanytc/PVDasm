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
#include <algorithm>
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
extern MapTree				SEHAddresses;
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

// Forward declarations
BOOL CALLBACK FunctionRefDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

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
static int			g_nActiveTab=0;				// 0 = Disassembly, 1 = Graph
static bool         g_bDisasmTabVisible = true;
static bool         g_bGraphTabVisible  = true;
static WNDPROC      g_OrigTabWndProc = NULL;
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
#define CODE_MAP_BAR_HEIGHT  20  // The color-coded navigation strip
#define CODE_MAP_LEGEND_HEIGHT 16 // Legend text row below the strip
#define CODE_MAP_HEIGHT (CODE_MAP_BAR_HEIGHT + CODE_MAP_LEGEND_HEIGHT) // Total height
#define TAB_HEIGHT 24 // Height of the tab control above the disassembly/graph area
#define TAB_CLOSE_BTN_SIZE  14
#define TAB_CLOSE_BTN_PAD    4

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
// ======================  FLOW ARROWS  ===========================
// ================================================================
#define FLOW_ARROW_MAX_DEPTH    8
#define FLOW_ARROW_LANE_WIDTH   4
#define FLOW_ARROW_MAX_SPAN   500  // Skip arrows spanning > 500 lines

struct FLOW_ARROW {
    DWORD_PTR SourceIndex;
    DWORD_PTR DestIndex;
    bool      IsConditional;
    bool      IsForwardJump;
    int       NestingLane;
};
static std::vector<FLOW_ARROW> g_FlowArrows;
static bool g_FlowArrowsClassRegistered = false;
static bool g_FlowArrowSplitterDragging = false;
static int  g_FlowArrowDragStartX = 0;
static int  g_FlowArrowDragStartWidth = 0;
#define FLOW_ARROW_SPLITTER_HIT  4  // pixels from right edge to detect drag

// Reverse branch trace cycling state
static DWORD_PTR g_RevDestAddr = 0;              // destination address being cycled
static DWORD_PTR g_RevLastNavigated = (DWORD_PTR)-1; // line index we last navigated to
static int       g_RevCallerIdx = 0;             // current caller index in the cycle

// Forward declarations
LRESULT CALLBACK FlowArrowWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void BuildFlowArrowData();
static void AssignArrowLanes();
void StretchLastColumn(HWND hListView);

// ================================================================
// =================  TAB SWITCHING  ==============================
// ================================================================

// Forward declaration (defined in TAB VISIBILITY HELPERS section below)
static int LogicalToTabIndex(HWND hTab, int logicalTab);

static void SwitchTab(HWND hWnd, int tabIndex)
{
    HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
    HWND hCFGChild = GetDlgItem(hWnd, IDC_CFG_CHILD);
    HWND hTab = GetDlgItem(hWnd, IDC_TAB_MAIN);

    g_nActiveTab = tabIndex;

    // Update tab selection (resolve logical to physical index)
    if (hTab) {
        int physIdx = LogicalToTabIndex(hTab, tabIndex);
        if (physIdx >= 0) TabCtrl_SetCurSel(hTab, physIdx);
    }

    if (tabIndex == 0) {
        // Disassembly tab
        if (hDisasm) ShowWindow(hDisasm, SW_SHOW);
        if (hCFGChild) ShowWindow(hCFGChild, SW_HIDE);
        // Show flow arrows panel if enabled
        HWND hArrows = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
        if (hArrows && g_FlowArrowsVisible && DisassemblerReady)
            ShowWindow(hArrows, SW_SHOW);
    } else {
        // Graph tab
        if (hDisasm) ShowWindow(hDisasm, SW_HIDE);
        // Hide flow arrows panel on graph tab
        HWND hArrows = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
        if (hArrows) ShowWindow(hArrows, SW_HIDE);
        // Sync CFG child position to full area (disasm + flow arrows)
        if (hCFGChild && hDisasm) {
            RECT disasmRect;
            GetWindowRect(hDisasm, &disasmRect);
            MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&disasmRect, 2);
            // Expand left to cover the flow arrows panel area
            if (hArrows && g_FlowArrowsVisible && DisassemblerReady) {
                disasmRect.left -= g_FlowArrowPanelWidth;
            }
            MoveWindow(hCFGChild, disasmRect.left, disasmRect.top,
                       disasmRect.right - disasmRect.left,
                       disasmRect.bottom - disasmRect.top, TRUE);
            ShowWindow(hCFGChild, SW_SHOW);
            SetFocus(hCFGChild);
        }
        // Build graph for the function at the current cursor position
        if (DisassemblerReady) {
            LoadCFGForCurrentFunction_Embedded();
        }
    }
}

// ================================================================
// =================  TAB VISIBILITY HELPERS  =====================
// ================================================================

// Map a logical tab ID (0=Disassembly, 1=Graph) to the physical tab index in the control.
// Returns -1 if not found (tab is hidden).
static int LogicalToTabIndex(HWND hTab, int logicalTab)
{
    int count = TabCtrl_GetItemCount(hTab);
    for (int i = 0; i < count; i++) {
        TCITEM tc = {0};
        tc.mask = TCIF_PARAM;
        TabCtrl_GetItem(hTab, i, &tc);
        if ((int)tc.lParam == logicalTab)
            return i;
    }
    return -1;
}

// Map a physical tab index to a logical tab ID via lParam.
static int TabIndexToLogical(HWND hTab, int tabIndex)
{
    TCITEM tc = {0};
    tc.mask = TCIF_PARAM;
    if (TabCtrl_GetItem(hTab, tabIndex, &tc))
        return (int)tc.lParam;
    return 0;
}

// Rebuild the tab control items based on visibility flags.
static void RebuildTabs(HWND hWnd)
{
    HWND hTab = GetDlgItem(hWnd, IDC_TAB_MAIN);
    if (!hTab) return;

    // Remember current logical tab
    int curLogical = g_nActiveTab;

    TabCtrl_DeleteAllItems(hTab);

    int insertIdx = 0;
    if (g_bDisasmTabVisible) {
        TCITEM tie = {0};
        tie.mask = TCIF_TEXT | TCIF_PARAM;
        tie.pszText = (LPSTR)"Disassembly";
        tie.lParam = 0;
        TabCtrl_InsertItem(hTab, insertIdx++, &tie);
    }
    if (g_bGraphTabVisible) {
        TCITEM tie = {0};
        tie.mask = TCIF_TEXT | TCIF_PARAM;
        tie.pszText = (LPSTR)"Graph";
        tie.lParam = 1;
        TabCtrl_InsertItem(hTab, insertIdx++, &tie);
    }

    if (insertIdx == 0) {
        // No tabs visible -- shouldn't happen (protected elsewhere), show disasm as fallback
        ShowWindow(hTab, SW_HIDE);
        return;
    }
    ShowWindow(hTab, SW_SHOW);

    // Select the appropriate tab
    int physIdx = LogicalToTabIndex(hTab, curLogical);
    if (physIdx < 0) {
        // Current tab was hidden, pick the first available
        physIdx = 0;
        curLogical = TabIndexToLogical(hTab, 0);
    }
    TabCtrl_SetCurSel(hTab, physIdx);
    SwitchTab(hWnd, curLogical);
    InvalidateRect(hTab, NULL, TRUE);
}

// Get the close-button RECT for a given physical tab index.
static RECT GetTabCloseButtonRect(HWND hTab, int tabIndex)
{
    RECT itemRect;
    TabCtrl_GetItemRect(hTab, tabIndex, &itemRect);
    RECT btnRect;
    btnRect.right = itemRect.right - TAB_CLOSE_BTN_PAD;
    btnRect.left = btnRect.right - TAB_CLOSE_BTN_SIZE;
    btnRect.top = itemRect.top + (itemRect.bottom - itemRect.top - TAB_CLOSE_BTN_SIZE) / 2;
    btnRect.bottom = btnRect.top + TAB_CLOSE_BTN_SIZE;
    return btnRect;
}

// Toggle visibility of a logical tab.
static void SetTabVisible(HWND hWnd, int logicalTab, bool visible)
{
    if (logicalTab == 0)
        g_bDisasmTabVisible = visible;
    else
        g_bGraphTabVisible = visible;

    // Update menu checkmark
    UINT menuID = (logicalTab == 0) ? IDC_VIEW_DISASSEMBLY : IDC_VIEW_GRAPH;
    CheckMenuItem(GetMenu(hWnd), menuID, visible ? MF_CHECKED : MF_UNCHECKED);

    RebuildTabs(hWnd);
}

// Ensure the disassembly tab is visible and active (called from CFGViewer).
void ShowDisassemblyTab()
{
    if (!Main_hWnd) return;
    if (!g_bDisasmTabVisible)
        SetTabVisible(Main_hWnd, 0, true);
    if (g_nActiveTab != 0)
        SwitchTab(Main_hWnd, 0);
}

// Tab subclass proc: intercept mouse clicks on the close X button.
static LRESULT CALLBACK TabSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_ERASEBKGND) {
        // Fill entire tab control background (including empty area right of tabs)
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        COLORREF bgColor = g_DarkMode ? RGB(35, 35, 35) : GetSysColor(COLOR_BTNFACE);
        HBRUSH hBrush = CreateSolidBrush(bgColor);
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);
        return 1;
    }
    if (uMsg == WM_LBUTTONDOWN) {
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        int count = TabCtrl_GetItemCount(hWnd);
        // Don't allow closing if only one tab remains
        if (count > 1) {
            for (int i = 0; i < count; i++) {
                RECT closeRect = GetTabCloseButtonRect(hWnd, i);
                if (PtInRect(&closeRect, pt)) {
                    int logicalID = TabIndexToLogical(hWnd, i);
                    HWND hParent = GetParent(hWnd);
                    SetTabVisible(hParent, logicalID, false);
                    return 0; // eat the message
                }
            }
        }
    }
    return CallWindowProc(g_OrigTabWndProc, hWnd, uMsg, wParam, lParam);
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
        } else if (_strnicmp(DisasmDataLines[i].GetMnemonic(), "DB ", 3) == 0) {
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

    // Second pass: detect function bodies via prologue/epilogue patterns.
    // Many functions have PUSH EBP/RBP prologues that the auto-comment system
    // detects, but they are NOT added to fFunctionInfo. Scan the mnemonics
    // directly to find prologue->RET ranges and mark them as CMAP_FUNCTION.
    bool inFunction = false;
    for (size_t i = 0; i < count; i++) {
        char* mnem = DisasmDataLines[i].GetMnemonic();
        if (!mnem || !mnem[0]) continue;

        // Detect function banners ("; ====== Proc_" or "; ====== " from fFunctionInfo)
        if (_strnicmp(mnem, "; ======", 8) == 0) {
            inFunction = true;
            continue; // Banner line itself is not code
        }

        if (!inFunction) {
            bool isPrologue = false;
            // Classic prologue: PUSH EBP/RBP followed by MOV EBP,ESP / MOV RBP,RSP
            if (_strnicmp(mnem, "push ebp", 8) == 0 || _strnicmp(mnem, "push rbp", 8) == 0) {
                if (i + 1 < count) {
                    char* nextMnem = DisasmDataLines[i+1].GetMnemonic();
                    if (nextMnem && (_strnicmp(nextMnem, "mov ebp", 7) == 0 || _strnicmp(nextMnem, "mov rbp", 7) == 0))
                        isPrologue = true;
                }
            }
            // FPO prologue: SUB ESP/RSP, imm
            if (!isPrologue && (_strnicmp(mnem, "sub esp", 7) == 0 || _strnicmp(mnem, "sub rsp", 7) == 0))
                isPrologue = true;
            if (isPrologue) {
                inFunction = true;
                if (g_CodeMapTypes[i] == CMAP_CODE)
                    g_CodeMapTypes[i] = CMAP_FUNCTION;
            }
        } else {
            // Inside a function body — mark as FUNCTION unless it's import/data
            if (g_CodeMapTypes[i] == CMAP_CODE)
                g_CodeMapTypes[i] = CMAP_FUNCTION;

            // Check for function end: RET, RETN, RETF
            if (_strnicmp(mnem, "ret", 3) == 0) {
                // Verify it's actually a return (not "retf" prefix of something else)
                char c = mnem[3];
                if (c == '\0' || c == ' ' || c == 'n' || c == 'N' || c == 'f' || c == 'F')
                    inFunction = false;
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

// Code map zoom state: visible fraction of the total data range [0.0 .. 1.0]
static double g_CodeMapZoomStart = 0.0;
static double g_CodeMapZoomEnd   = 1.0;
static bool     g_CodeMapDragging = false;
static DWORD_PTR g_CodeMapDragIdx = 0;        // Current drag target index
static DWORD_PTR g_CodeMapLastScrollIdx = (DWORD_PTR)-1; // Last index sent to listview
static bool   g_CodeMapPanning = false;   // Middle-mouse pan state
static int    g_CodeMapPanStartX = 0;     // Mouse X at pan start
static double g_CodeMapPanStartZoomS = 0; // Zoom start at pan start
static double g_CodeMapPanStartZoomE = 0; // Zoom end at pan start
#define CODEMAP_ZOOM_FACTOR 1.3  // Each scroll step zooms by this factor

// Cached bitmap for the color bar + legend (avoids redrawing on every paint)
static HBITMAP g_CodeMapCacheBmp = NULL;
static int     g_CodeMapCacheWidth = 0;
static int     g_CodeMapCacheHeight = 0;
static double  g_CodeMapCacheZoomStart = -1.0;
static double  g_CodeMapCacheZoomEnd   = -1.0;
static size_t  g_CodeMapCacheCount = 0;
static bool    g_CodeMapCacheDark = false;

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
            int barHeight = CODE_MAP_BAR_HEIGHT;
            size_t count = DisasmDataLines.size();

            // --- Rebuild cached bitmap if needed (zoom/size/data changed) ---
            bool cacheValid = (g_CodeMapCacheBmp != NULL
                && g_CodeMapCacheWidth == totalWidth
                && g_CodeMapCacheHeight == totalHeight
                && g_CodeMapCacheZoomStart == g_CodeMapZoomStart
                && g_CodeMapCacheZoomEnd == g_CodeMapZoomEnd
                && g_CodeMapCacheCount == count
                && g_CodeMapCacheDark == g_DarkMode);

            if (!cacheValid) {
                // (Re)create the cache bitmap
                if (g_CodeMapCacheBmp) DeleteObject(g_CodeMapCacheBmp);
                HDC hdcMem = CreateCompatibleDC(hdc);
                g_CodeMapCacheBmp = CreateCompatibleBitmap(hdc, totalWidth, totalHeight);
                HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, g_CodeMapCacheBmp);

                // Fill entire background
                COLORREF bgColor = g_DarkMode ? CODEMAP_COLOR_BG_DK : CODEMAP_COLOR_BG;
                HBRUSH hBgBrush = CreateSolidBrush(bgColor);
                FillRect(hdcMem, &rc, hBgBrush);
                DeleteObject(hBgBrush);

                // Draw the color bar
                if (count > 0 && g_CodeMapTypes.size() == count && totalWidth > 0) {
                    double zStart = g_CodeMapZoomStart;
                    double zEnd   = g_CodeMapZoomEnd;
                    double zRange = zEnd - zStart;

                    COLORREF prevColor = 0;
                    int runStart = 0;

                    for (int x = 0; x <= totalWidth; x++) {
                        COLORREF color = 0;
                        if (x < totalWidth) {
                            double frac0 = (double)x / totalWidth;
                            double frac1 = (double)(x + 1) / totalWidth;
                            size_t startIdx = (size_t)((zStart + frac0 * zRange) * count);
                            size_t endIdx   = (size_t)((zStart + frac1 * zRange) * count);
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
                            FillRect(hdcMem, &fillRc, hBr);
                            DeleteObject(hBr);
                            prevColor = color;
                            runStart = x;
                        }
                    }
                }

                // Draw legend row
                int legendY = barHeight;
                HFONT hSmallFont = CreateFontA(CODE_MAP_LEGEND_HEIGHT - 1, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "MS Shell Dlg");
                HFONT hOldFont = (HFONT)SelectObject(hdcMem, hSmallFont);

                int xPos = 4;
                xPos = DrawLegendItem(hdcMem, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                    g_DarkMode ? CODEMAP_COLOR_FUNCTION_DK : CODEMAP_COLOR_FUNCTION, "Function");
                xPos = DrawLegendItem(hdcMem, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                    g_DarkMode ? CODEMAP_COLOR_IMPORT_DK : CODEMAP_COLOR_IMPORT, "Import");
                xPos = DrawLegendItem(hdcMem, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                    g_DarkMode ? CODEMAP_COLOR_DATA_DK : CODEMAP_COLOR_DATA, "Data");
                xPos = DrawLegendItem(hdcMem, xPos, legendY, CODE_MAP_LEGEND_HEIGHT,
                    g_DarkMode ? CODEMAP_COLOR_CODE_DK : CODEMAP_COLOR_CODE, "Code");

                SelectObject(hdcMem, hOldFont);
                DeleteObject(hSmallFont);

                SelectObject(hdcMem, hOldBmp);
                DeleteDC(hdcMem);

                // Update cache keys
                g_CodeMapCacheWidth = totalWidth;
                g_CodeMapCacheHeight = totalHeight;
                g_CodeMapCacheZoomStart = g_CodeMapZoomStart;
                g_CodeMapCacheZoomEnd = g_CodeMapZoomEnd;
                g_CodeMapCacheCount = count;
                g_CodeMapCacheDark = g_DarkMode;
            }

            // --- Blit cached bitmap to screen ---
            HDC hdcBlit = CreateCompatibleDC(hdc);
            HBITMAP hOldBlit = (HBITMAP)SelectObject(hdcBlit, g_CodeMapCacheBmp);
            BitBlt(hdc, 0, 0, totalWidth, totalHeight, hdcBlit, 0, 0, SRCCOPY);
            SelectObject(hdcBlit, hOldBlit);
            DeleteDC(hdcBlit);

            // --- Draw arrow indicator on top ---
            if (count > 0 && g_CodeMapTypes.size() == count && totalWidth > 0) {
                double zStart = g_CodeMapZoomStart;
                double zRange = g_CodeMapZoomEnd - zStart;
                #define IDX_TO_PX(idx) ((int)(((double)(idx) / count - zStart) / zRange * totalWidth))

                HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
                if (hDisasm) {
                    int cursorIdx;
                    if (g_CodeMapDragging) {
                        cursorIdx = (int)g_CodeMapDragIdx;
                    } else {
                        int topIndex = (int)SendMessage(hDisasm, LVM_GETTOPINDEX, 0, 0);
                        int visibleCount = (int)SendMessage(hDisasm, LVM_GETCOUNTPERPAGE, 0, 0);
                        int focusedIdx = (int)SendMessage(hDisasm, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                        if (focusedIdx >= topIndex && focusedIdx <= topIndex + visibleCount)
                            cursorIdx = focusedIdx;
                        else
                            cursorIdx = topIndex + visibleCount / 2;
                    }
                    if (cursorIdx >= 0 && cursorIdx < (int)count) {
                        int cx = IDX_TO_PX(cursorIdx);
                        if (cx >= 0 && cx < totalWidth) {
                            POINT arrow[3] = {
                                { cx - 6, 0 },
                                { cx + 6, 0 },
                                { cx, 8 }
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
                #undef IDX_TO_PX
            }

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            // If we're not on the disassembly tab, switch to it (show it first if hidden)
            if (g_nActiveTab != 0) {
                if (!g_bDisasmTabVisible)
                    SetTabVisible(Main_hWnd, 0, true);
                SwitchTab(Main_hWnd, 0);
            }
            SetCapture(hWnd);
            g_CodeMapDragging = true;
            g_CodeMapLastScrollIdx = (DWORD_PTR)-1;
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
                // Map pixel to data index using zoom range
                double zRange = g_CodeMapZoomEnd - g_CodeMapZoomStart;
                double frac = g_CodeMapZoomStart + (double)mouseX / barWidth * zRange;
                DWORD_PTR index = (DWORD_PTR)(frac * count);
                if (index >= count) index = count - 1;

                g_CodeMapDragIdx = index;

                // Paint arrow immediately BEFORE scrolling listview
                InvalidateRect(hWnd, NULL, FALSE);
                UpdateWindow(hWnd);

                // Scroll + select listview — cheap for virtual listview (just LVN_GETDISPINFO)
                if (index != g_CodeMapLastScrollIdx) {
                    g_CodeMapLastScrollIdx = index;
                    HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
                    if (hDisasm) {
                        int topIdx = (int)SendMessage(hDisasm, LVM_GETTOPINDEX, 0, 0);
                        int perPage = (int)SendMessage(hDisasm, LVM_GETCOUNTPERPAGE, 0, 0);
                        int desiredTop = (int)index - perPage / 2;
                        int deltaItems = desiredTop - topIdx;
                        if (deltaItems != 0) {
                            RECT itemRc;
                            ListView_GetItemRect(hDisasm, topIdx, &itemRc, LVIR_BOUNDS);
                            int itemHeight = itemRc.bottom - itemRc.top;
                            if (itemHeight > 0)
                                SendMessage(hDisasm, LVM_SCROLL, 0, deltaItems * itemHeight);
                        }
                        SelectItem(hDisasm, index);
                    }
                    // Repaint flow arrows panel after scroll
                    HWND hArrowPanel = GetDlgItem(Main_hWnd, IDC_FLOW_ARROWS);
                    if (hArrowPanel && IsWindowVisible(hArrowPanel)) {
                        InvalidateRect(hArrowPanel, NULL, FALSE);
                        UpdateWindow(hArrowPanel);
                    }
                }
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            ReleaseCapture();
            g_CodeMapDragging = false;

            // Final select to set focus/highlight
            HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
            if (hDisasm) {
                SelectItem(hDisasm, g_CodeMapDragIdx);
            }
            InvalidateRect(hWnd, NULL, FALSE);
            // Repaint flow arrows panel on release
            HWND hArrowPanel = GetDlgItem(Main_hWnd, IDC_FLOW_ARROWS);
            if (hArrowPanel && IsWindowVisible(hArrowPanel)) {
                InvalidateRect(hArrowPanel, NULL, FALSE);
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            // Zoom in/out centered on mouse position
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            RECT rc;
            GetClientRect(hWnd, &rc);
            int barWidth = rc.right - rc.left;

            // Get mouse X relative to our window
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            ScreenToClient(hWnd, &pt);
            int mouseX = pt.x;
            if (mouseX < 0) mouseX = 0;
            if (mouseX >= barWidth) mouseX = barWidth - 1;

            size_t count = DisasmDataLines.size();
            if (count > 0 && barWidth > 0) {
                double zRange = g_CodeMapZoomEnd - g_CodeMapZoomStart;
                // The data fraction under the mouse cursor
                double anchor = g_CodeMapZoomStart + (double)mouseX / barWidth * zRange;

                double newRange;
                if (delta > 0) // scroll up = zoom in
                    newRange = zRange / CODEMAP_ZOOM_FACTOR;
                else           // scroll down = zoom out
                    newRange = zRange * CODEMAP_ZOOM_FACTOR;

                // Clamp: can't zoom out beyond full range
                if (newRange > 1.0) newRange = 1.0;
                // Minimum zoom: at least 100 entries visible
                double minRange = 100.0 / count;
                if (minRange < 0.001) minRange = 0.001;
                if (newRange < minRange) newRange = minRange;

                // Keep the anchor point at the same pixel position
                double anchorFrac = (double)mouseX / barWidth;
                double newStart = anchor - anchorFrac * newRange;
                double newEnd   = newStart + newRange;

                // Clamp to [0, 1]
                if (newStart < 0.0) { newEnd -= newStart; newStart = 0.0; }
                if (newEnd > 1.0)   { newStart -= (newEnd - 1.0); newEnd = 1.0; }
                if (newStart < 0.0) newStart = 0.0;

                g_CodeMapZoomStart = newStart;
                g_CodeMapZoomEnd   = newEnd;
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_RBUTTONDOWN: {
            // Right-click resets zoom to default
            g_CodeMapZoomStart = 0.0;
            g_CodeMapZoomEnd   = 1.0;
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_MBUTTONDOWN: {
            // Middle-click + drag to pan when zoomed in
            double zRange = g_CodeMapZoomEnd - g_CodeMapZoomStart;
            if (zRange < 1.0) {
                SetCapture(hWnd);
                g_CodeMapPanning = true;
                g_CodeMapPanStartX = (short)LOWORD(lParam);
                g_CodeMapPanStartZoomS = g_CodeMapZoomStart;
                g_CodeMapPanStartZoomE = g_CodeMapZoomEnd;
            }
            return 0;
        }

        case WM_MBUTTONUP: {
            if (g_CodeMapPanning) {
                ReleaseCapture();
                g_CodeMapPanning = false;
            }
            return 0;
        }
    }

    // Handle pan drag in WM_MOUSEMOVE (outside switch so left-drag still works)
    if (uMsg == WM_MOUSEMOVE && g_CodeMapPanning) {
        RECT rc;
        GetClientRect(hWnd, &rc);
        int barWidth = rc.right - rc.left;
        if (barWidth > 0) {
            int mouseX = (short)LOWORD(lParam);
            int dx = mouseX - g_CodeMapPanStartX;
            double zRange = g_CodeMapPanStartZoomE - g_CodeMapPanStartZoomS;
            // Convert pixel delta to data fraction delta
            double shift = -(double)dx / barWidth * zRange;
            double newStart = g_CodeMapPanStartZoomS + shift;
            double newEnd   = g_CodeMapPanStartZoomE + shift;
            // Clamp to [0, 1]
            if (newStart < 0.0) { newEnd -= newStart; newStart = 0.0; }
            if (newEnd > 1.0)   { newStart -= (newEnd - 1.0); newEnd = 1.0; }
            if (newStart < 0.0) newStart = 0.0;
            g_CodeMapZoomStart = newStart;
            g_CodeMapZoomEnd   = newEnd;
            InvalidateRect(hWnd, NULL, FALSE);
        }
        return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void RefreshCodeMapBar()
{
    if (!g_CodeMapVisible || !Main_hWnd) return;
    BuildCodeMapData();
    // Reset zoom when new data is loaded
    g_CodeMapZoomStart = 0.0;
    g_CodeMapZoomEnd   = 1.0;
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
    HWND hTab = GetDlgItem(hWnd, IDC_TAB_MAIN);
    if (!hDisasm) return;

    RECT rc;
    GetWindowRect(hDisasm, &rc);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rc, 2);

    if (show) {
        // Get tab rect to place code map above it
        RECT rcTab;
        if (hTab) {
            GetWindowRect(hTab, &rcTab);
            MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rcTab, 2);
        }
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        // Place code map bar at the tab's current top
        if (hBar && hTab)
            MoveWindow(hBar, 0, rcTab.top, rcClient.right, CODE_MAP_HEIGHT, TRUE);
        // Move tab down by CODE_MAP_HEIGHT
        if (hTab)
            MoveWindow(hTab, 0, rcTab.top + CODE_MAP_HEIGHT,
                       rcClient.right, TAB_HEIGHT, TRUE);
        // Push listview down by CODE_MAP_HEIGHT
        MoveWindow(hDisasm, rc.left, rc.top + CODE_MAP_HEIGHT,
                   rc.right - rc.left, (rc.bottom - rc.top) - CODE_MAP_HEIGHT, TRUE);
    } else {
        // Get tab rect
        RECT rcTab;
        if (hTab) {
            GetWindowRect(hTab, &rcTab);
            MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rcTab, 2);
        }
        RECT rcClient;
        GetClientRect(hWnd, &rcClient);
        // Move tab up by CODE_MAP_HEIGHT
        if (hTab)
            MoveWindow(hTab, 0, rcTab.top - CODE_MAP_HEIGHT,
                       rcClient.right, TAB_HEIGHT, TRUE);
        // Pull listview back up
        MoveWindow(hDisasm, rc.left, rc.top - CODE_MAP_HEIGHT,
                   rc.right - rc.left, (rc.bottom - rc.top) + CODE_MAP_HEIGHT, TRUE);
    }

    // Re-initialize resize anchors so future WM_SIZE works correctly
    InitializeResizeControls(hWnd);
}

// ================================================================
// ==============  FLOW ARROWS — BUILD & PAINT  ===================
// ================================================================

static void AssignArrowLanes()
{
    // Sort arrows by span length (shortest first = innermost lane)
    std::sort(g_FlowArrows.begin(), g_FlowArrows.end(),
        [](const FLOW_ARROW& a, const FLOW_ARROW& b) {
            DWORD_PTR spanA = (a.SourceIndex > a.DestIndex) ?
                (a.SourceIndex - a.DestIndex) : (a.DestIndex - a.SourceIndex);
            DWORD_PTR spanB = (b.SourceIndex > b.DestIndex) ?
                (b.SourceIndex - b.DestIndex) : (b.DestIndex - b.SourceIndex);
            return spanA < spanB;
        });

    for (size_t i = 0; i < g_FlowArrows.size(); i++) {
        DWORD_PTR lo = min(g_FlowArrows[i].SourceIndex, g_FlowArrows[i].DestIndex);
        DWORD_PTR hi = max(g_FlowArrows[i].SourceIndex, g_FlowArrows[i].DestIndex);

        // For each candidate lane, check if any already-assigned arrow overlaps
        int lane = 0;
        for (; lane < FLOW_ARROW_MAX_DEPTH; lane++) {
            bool conflict = false;
            for (size_t j = 0; j < i; j++) {
                if (g_FlowArrows[j].NestingLane != lane) continue;
                DWORD_PTR lo2 = min(g_FlowArrows[j].SourceIndex, g_FlowArrows[j].DestIndex);
                DWORD_PTR hi2 = max(g_FlowArrows[j].SourceIndex, g_FlowArrows[j].DestIndex);
                if (lo <= hi2 && hi >= lo2) { conflict = true; break; }
            }
            if (!conflict) break;
        }
        g_FlowArrows[i].NestingLane = (lane < FLOW_ARROW_MAX_DEPTH) ? lane : -1;
    }
}

void BuildFlowArrowData()
{
    g_FlowArrows.clear();
    if (DisasmDataLines.empty() || DisasmCodeFlow.empty()) return;

    for (size_t i = 0; i < DisasmCodeFlow.size(); i++) {
        CODE_BRANCH& cb = DisasmCodeFlow[i];

        // Only visualize jumps, not calls or rets
        if (cb.BranchFlow.Call || cb.BranchFlow.Ret) continue;
        if (!cb.BranchFlow.Jump) continue;

        DWORD_PTR srcIndex = cb.Current_Index;
        DWORD_PTR destAddr = cb.Branch_Destination;
        DWORD_PTR destIndex = FindIndexByAddress(destAddr);
        if (destIndex == (DWORD_PTR)-1) continue;
        if (srcIndex >= DisasmDataLines.size()) continue;

        // Skip huge spans
        DWORD_PTR span = (srcIndex > destIndex) ? (srcIndex - destIndex) : (destIndex - srcIndex);
        if (span > FLOW_ARROW_MAX_SPAN) continue;

        // Determine if conditional by examining the mnemonic
        const char* mnemonic = DisasmDataLines[srcIndex].GetMnemonic();
        bool isCond = IsConditionalJump(mnemonic);

        FLOW_ARROW fa;
        fa.SourceIndex = srcIndex;
        fa.DestIndex = destIndex;
        fa.IsConditional = isCond;
        fa.IsForwardJump = (destIndex > srcIndex);
        fa.NestingLane = 0;
        g_FlowArrows.push_back(fa);
    }

    AssignArrowLanes();
}

LRESULT CALLBACK FlowArrowWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_ERASEBKGND:
            return 1;

        // Show left-right resize cursor when hovering over the right edge (splitter zone)
        case WM_SETCURSOR: {
            POINT pt;
            GetCursorPos(&pt);
            ScreenToClient(hWnd, &pt);
            RECT rc;
            GetClientRect(hWnd, &rc);
            if (pt.x >= rc.right - FLOW_ARROW_SPLITTER_HIT) {
                SetCursor(LoadCursor(NULL, IDC_SIZEWE));
                return TRUE;
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
            RECT rc;
            GetClientRect(hWnd, &rc);
            if (pt.x >= rc.right - FLOW_ARROW_SPLITTER_HIT) {
                g_FlowArrowSplitterDragging = true;
                // Store start position in screen coords for reliable delta
                POINT ptScreen = pt;
                ClientToScreen(hWnd, &ptScreen);
                g_FlowArrowDragStartX = ptScreen.x;
                g_FlowArrowDragStartWidth = g_FlowArrowPanelWidth;
                SetCapture(hWnd);
                return 0;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            if (g_FlowArrowSplitterDragging) {
                POINT ptScreen = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
                ClientToScreen(hWnd, &ptScreen);
                int deltaX = ptScreen.x - g_FlowArrowDragStartX;
                // Dragging right = making panel wider, dragging left = narrower
                // (panel is on the left, its right edge is the drag handle)
                int newWidth = g_FlowArrowDragStartWidth + deltaX;
                if (newWidth < FLOW_ARROW_PANEL_WIDTH_MIN) newWidth = FLOW_ARROW_PANEL_WIDTH_MIN;
                if (newWidth > FLOW_ARROW_PANEL_WIDTH_MAX) newWidth = FLOW_ARROW_PANEL_WIDTH_MAX;

                if (newWidth != g_FlowArrowPanelWidth) {
                    int widthDelta = newWidth - g_FlowArrowPanelWidth;
                    g_FlowArrowPanelWidth = newWidth;

                    // Resize: grow panel right edge, shrink ListView left edge
                    HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
                    if (hDisasm) {
                        RECT dr;
                        GetWindowRect(hDisasm, &dr);
                        MapWindowPoints(HWND_DESKTOP, Main_hWnd, (LPPOINT)&dr, 2);
                        // Shrink ListView from the left by widthDelta
                        MoveWindow(hDisasm, dr.left + widthDelta, dr.top,
                            (dr.right - dr.left) - widthDelta,
                            dr.bottom - dr.top, TRUE);
                        // Resize arrow panel to new width (same left edge)
                        RECT pr;
                        GetWindowRect(hWnd, &pr);
                        MapWindowPoints(HWND_DESKTOP, Main_hWnd, (LPPOINT)&pr, 2);
                        MoveWindow(hWnd, pr.left, pr.top,
                            g_FlowArrowPanelWidth, pr.bottom - pr.top, TRUE);

                        StretchLastColumn(hDisasm);
                        InvalidateRect(hWnd, NULL, FALSE);
                        UpdateWindow(hWnd);
                    }
                }
                return 0;
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (g_FlowArrowSplitterDragging) {
                g_FlowArrowSplitterDragging = false;
                ReleaseCapture();
                SaveSettings();
                return 0;
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            int panelW = rcClient.right - rcClient.left;
            int panelH = rcClient.bottom - rcClient.top;

            // Double-buffer
            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hBmp = CreateCompatibleBitmap(hdc, panelW, panelH);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

            // Fill background matching ListView bg
            COLORREF bgColor = g_DarkMode ? g_DarkBkColor : RGB(255, 251, 240);
            HBRUSH hBgBrush = CreateSolidBrush(bgColor);
            FillRect(hdcMem, &rcClient, hBgBrush);
            DeleteObject(hBgBrush);

            // Draw a thin separator line on the right edge (drag handle)
            COLORREF sepColor = g_DarkMode ? RGB(60, 60, 60) : RGB(180, 180, 180);
            HPEN hSepPen = CreatePen(PS_SOLID, 1, sepColor);
            HPEN hOldPen = (HPEN)SelectObject(hdcMem, hSepPen);
            MoveToEx(hdcMem, panelW - 1, 0, NULL);
            LineTo(hdcMem, panelW - 1, panelH);
            SelectObject(hdcMem, hOldPen);
            DeleteObject(hSepPen);

            // Scale lane spacing proportionally to panel width
            // At default 40px: 4px/lane.  At 120px: 12px/lane.
            int laneWidth = max(3, (panelW - 8) / max(FLOW_ARROW_MAX_DEPTH, 1));
            // Scale arrow line thickness: 1px at default, up to 2px when wide
            int penWidth = (panelW >= 80) ? 2 : 1;
            // Scale arrowhead size
            int headLen = (panelW >= 80) ? 6 : 4;
            int headSpread = (panelW >= 80) ? 4 : 3;

            // Get visible range from ListView
            HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
            if (hDisasm && !g_FlowArrows.empty()) {
                int topIndex = (int)SendMessage(hDisasm, LVM_GETTOPINDEX, 0, 0);
                int perPage = (int)SendMessage(hDisasm, LVM_GETCOUNTPERPAGE, 0, 0);
                int bottomIndex = topIndex + perPage;
                int totalItems = (int)SendMessage(hDisasm, LVM_GETITEMCOUNT, 0, 0);
                if (bottomIndex > totalItems) bottomIndex = totalItems;

                // Get item height from the first visible item
                RECT rcItem;
                ListView_GetItemRect(hDisasm, topIndex, &rcItem, LVIR_BOUNDS);
                MapWindowPoints(hDisasm, Main_hWnd, (LPPOINT)&rcItem, 2);
                RECT rcPanel;
                GetWindowRect(hWnd, &rcPanel);
                MapWindowPoints(HWND_DESKTOP, Main_hWnd, (LPPOINT)&rcPanel, 2);
                int lvTopInParent = rcItem.top;
                int panelTopInParent = rcPanel.top;
                int yOffset = lvTopInParent - panelTopInParent;

                int itemHeight = rcItem.bottom - rcItem.top;
                if (itemHeight <= 0) itemHeight = 16;

                // Arrow color palette — cycle distinct colors across lanes
                // so adjacent arrows are visually distinguishable (IDA-style)
                static const COLORREF laneColors[] = {
                    RGB(0, 100, 200),    // blue
                    RGB(180, 0, 0),      // red
                    RGB(0, 140, 60),     // green
                    RGB(160, 80, 0),     // brown/orange
                    RGB(120, 0, 160),    // purple
                    RGB(0, 140, 140),    // teal
                    RGB(160, 120, 0),    // dark yellow
                    RGB(100, 100, 100),  // gray
                };
                static const COLORREF laneColorsDk[] = {
                    RGB(80, 160, 255),   // light blue
                    RGB(230, 90, 90),    // light red
                    RGB(80, 200, 100),   // light green
                    RGB(220, 140, 50),   // orange
                    RGB(180, 100, 220),  // light purple
                    RGB(60, 200, 200),   // light teal
                    RGB(210, 180, 50),   // yellow
                    RGB(160, 160, 160),  // light gray
                };
                // Line styles — cycle per lane for extra distinction
                static const int laneStyles[] = {
                    PS_SOLID, PS_DASH, PS_DOT, PS_DASHDOT,
                    PS_SOLID, PS_DASH, PS_DOT, PS_DASHDOT,
                };
                int numLaneStyles = sizeof(laneStyles) / sizeof(laneStyles[0]);

                for (size_t i = 0; i < g_FlowArrows.size(); i++) {
                    FLOW_ARROW& fa = g_FlowArrows[i];
                    if (fa.NestingLane < 0) continue; // no lane assigned

                    DWORD_PTR lo = min(fa.SourceIndex, fa.DestIndex);
                    DWORD_PTR hi = max(fa.SourceIndex, fa.DestIndex);

                    // Skip if arrow is entirely outside visible range
                    if ((int)hi < topIndex || (int)lo > bottomIndex) continue;

                    // Calculate Y positions
                    int srcY, destY;
                    bool srcVisible = ((int)fa.SourceIndex >= topIndex && (int)fa.SourceIndex < bottomIndex);
                    bool destVisible = ((int)fa.DestIndex >= topIndex && (int)fa.DestIndex < bottomIndex);

                    if (srcVisible) {
                        srcY = yOffset + ((int)fa.SourceIndex - topIndex) * itemHeight + itemHeight / 2;
                    } else {
                        srcY = ((int)fa.SourceIndex < topIndex) ? 0 : panelH;
                    }

                    if (destVisible) {
                        destY = yOffset + ((int)fa.DestIndex - topIndex) * itemHeight + itemHeight / 2;
                    } else {
                        destY = ((int)fa.DestIndex < topIndex) ? 0 : panelH;
                    }

                    int tipRight = panelW - 2;              // arrowhead tip at panel edge
                    int stubRight = tipRight - headLen;      // stub ends where arrowhead base starts

                    // Lane X: lane 0 = rightmost (closest to ListView), left of arrowhead base
                    int laneX = (stubRight - 2) - fa.NestingLane * laneWidth;
                    if (laneX < 2) laneX = 2;

                    // Pick color and line style based on lane (IDA-style differentiation)
                    int laneIdx = fa.NestingLane % numLaneStyles;
                    COLORREF arrowColor = g_DarkMode ?
                        laneColorsDk[laneIdx] : laneColors[laneIdx];
                    int penStyle = laneStyles[laneIdx];

                    // For thick pens (width>1), GDI ignores dash styles;
                    // use ExtCreatePen with PS_GEOMETRIC for dashed thick lines
                    HPEN hArrowPen;
                    if (penWidth > 1 && penStyle != PS_SOLID) {
                        LOGBRUSH lb = {0};
                        lb.lbStyle = BS_SOLID;
                        lb.lbColor = arrowColor;
                        hArrowPen = ExtCreatePen(
                            PS_GEOMETRIC | penStyle | PS_ENDCAP_FLAT,
                            penWidth, &lb, 0, NULL);
                    } else {
                        hArrowPen = CreatePen(penStyle, penWidth, arrowColor);
                    }
                    HPEN hOldP = (HPEN)SelectObject(hdcMem, hArrowPen);

                    // Draw: horizontal stub at source -> vertical line -> horizontal stub at dest
                    // Source stub (extends to panel edge, no arrowhead here)
                    MoveToEx(hdcMem, tipRight, srcY, NULL);
                    LineTo(hdcMem, laneX, srcY);
                    // Vertical line
                    MoveToEx(hdcMem, laneX, srcY, NULL);
                    LineTo(hdcMem, laneX, destY);
                    // Dest stub
                    MoveToEx(hdcMem, laneX, destY, NULL);
                    LineTo(hdcMem, stubRight + 1, destY);

                    SelectObject(hdcMem, hOldP);
                    DeleteObject(hArrowPen);

                    // Filled triangle arrowhead at destination — crisp connection
                    if (destVisible) {
                        POINT tri[3] = {
                            { tipRight, destY },                       // tip (rightmost, at panel edge)
                            { tipRight - headLen, destY - headSpread },// upper base
                            { tipRight - headLen, destY + headSpread } // lower base
                        };
                        HBRUSH hHeadBrush = CreateSolidBrush(arrowColor);
                        HPEN hHeadPen = CreatePen(PS_SOLID, 1, arrowColor);
                        HBRUSH hOldBr = (HBRUSH)SelectObject(hdcMem, hHeadBrush);
                        HPEN hOldH = (HPEN)SelectObject(hdcMem, hHeadPen);
                        Polygon(hdcMem, tri, 3);
                        SelectObject(hdcMem, hOldH);
                        SelectObject(hdcMem, hOldBr);
                        DeleteObject(hHeadPen);
                        DeleteObject(hHeadBrush);
                    }
                }
            }

            BitBlt(hdc, 0, 0, panelW, panelH, hdcMem, 0, 0, SRCCOPY);
            SelectObject(hdcMem, hOldBmp);
            DeleteObject(hBmp);
            DeleteDC(hdcMem);

            EndPaint(hWnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
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


		// Owner-draw tab control (close X buttons)
		case WM_DRAWITEM:{
			PDRAWITEMSTRUCT pDIS = (PDRAWITEMSTRUCT)lParam;
			if (pDIS->CtlID == IDC_TAB_MAIN) {
				HWND hTab = GetDlgItem(hWnd, IDC_TAB_MAIN);
				bool isSelected = ((int)pDIS->itemID == TabCtrl_GetCurSel(hTab));

				// Background
				COLORREF bgColor, textColor;
				if (g_DarkMode) {
					bgColor = isSelected ? RGB(60, 60, 60) : RGB(35, 35, 35);
					textColor = g_DarkTextColor;
				} else {
					bgColor = isSelected ? GetSysColor(COLOR_WINDOW) : GetSysColor(COLOR_BTNFACE);
					textColor = GetSysColor(COLOR_WINDOWTEXT);
				}
				HBRUSH hBrush = CreateSolidBrush(bgColor);
				FillRect(pDIS->hDC, &pDIS->rcItem, hBrush);
				DeleteObject(hBrush);

				// Get tab text from TCITEM
				char szTabText[64] = {0};
				TCITEM tc = {0};
				tc.mask = TCIF_TEXT;
				tc.pszText = szTabText;
				tc.cchTextMax = sizeof(szTabText);
				TabCtrl_GetItem(hTab, pDIS->itemID, &tc);

				// Draw text (leave room on right for X)
				RECT textRect = pDIS->rcItem;
				textRect.left += 6;
				textRect.right -= TAB_CLOSE_BTN_SIZE + TAB_CLOSE_BTN_PAD + 4;
				SetTextColor(pDIS->hDC, textColor);
				SetBkMode(pDIS->hDC, TRANSPARENT);
				SelectObject(pDIS->hDC, GetStockObject(DEFAULT_GUI_FONT));
				DrawTextA(pDIS->hDC, szTabText, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

				// Draw X close button
				RECT closeRect = GetTabCloseButtonRect(hTab, pDIS->itemID);
				COLORREF xColor = g_DarkMode ? RGB(180, 180, 180) : RGB(120, 120, 120);
				HPEN hPen = CreatePen(PS_SOLID, 1, xColor);
				HPEN hOldPen = (HPEN)SelectObject(pDIS->hDC, hPen);
				int margin = 3;
				MoveToEx(pDIS->hDC, closeRect.left + margin, closeRect.top + margin, NULL);
				LineTo(pDIS->hDC, closeRect.right - margin, closeRect.bottom - margin);
				MoveToEx(pDIS->hDC, closeRect.right - margin - 1, closeRect.top + margin, NULL);
				LineTo(pDIS->hDC, closeRect.left + margin - 1, closeRect.bottom - margin);
				SelectObject(pDIS->hDC, hOldPen);
				DeleteObject(hPen);

				SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
				return TRUE;
			}
		}
		break;

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

			// Layout status bar across the bottom
			{
				RECT rcClient;
				GetClientRect(hWnd, &rcClient);
				int clientW = rcClient.right;

				HWND hMsg1 = GetDlgItem(hWnd, IDC_MESSAGE1);
				HWND hMsg2 = GetDlgItem(hWnd, IDC_MESSAGE2);
				HWND hProg = GetDlgItem(hWnd, IDC_DISASM_PROGRESS);

				// Use the current height of MESSAGE1 (set by dialog template / DPI)
				RECT rcMsg1;
				GetWindowRect(hMsg1, &rcMsg1);
				MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&rcMsg1, 2);
				int statusH = rcMsg1.bottom - rcMsg1.top;
				int statusY = rcClient.bottom - statusH;

				if (IsWindowVisible(hProg)) {
					// During disassembly: MESSAGE1 | MESSAGE2 | PROGRESS
					int progW  = 200;
					int msg2W  = 250;
					MoveWindow(hProg, clientW - progW, statusY, progW, statusH, TRUE);
					ShowWindow(hMsg2, SW_SHOW);
					MoveWindow(hMsg2, clientW - progW - msg2W, statusY, msg2W, statusH, TRUE);
					MoveWindow(hMsg1, -3, statusY, clientW - progW - msg2W + 3, statusH, TRUE);
				} else {
					// Idle: MESSAGE1 fills the entire width
					ShowWindow(hMsg2, SW_HIDE);
					MoveWindow(hMsg1, -3, statusY, clientW + 3, statusH, TRUE);
				}
				// Force full erase+repaint so text doesn't ghost on resize
				InvalidateRect(hMsg1, NULL, TRUE);
				InvalidateRect(hMsg2, NULL, TRUE);
				InvalidateRect(hProg, NULL, TRUE);
			}

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
			// Sync flow arrows panel position to match ListView
			{
				HWND hArrowPanel = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
				if (hArrowPanel && IsWindowVisible(hArrowPanel)) {
					HWND hDisasm2 = GetDlgItem(hWnd, IDC_DISASM);
					if (hDisasm2) {
						RECT dr;
						GetWindowRect(hDisasm2, &dr);
						MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dr, 2);
						MoveWindow(hArrowPanel, dr.left - g_FlowArrowPanelWidth, dr.top,
							g_FlowArrowPanelWidth, dr.bottom - dr.top, TRUE);
						InvalidateRect(hArrowPanel, NULL, FALSE);
					}
				}
			}
			// Stretch last column to fill width (fixes white areas in dark mode)
			StretchLastColumn(GetDlgItem(hWnd, IDC_DISASM));
			StretchLastColumn(GetDlgItem(hWnd, IDC_LIST));
			// Sync CFG child window to disasm position when graph tab is active
			if (g_nActiveTab == 1) {
				HWND hCFGChild = GetDlgItem(hWnd, IDC_CFG_CHILD);
				HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
				if (hCFGChild && hDisasm) {
					RECT dr;
					GetWindowRect(hDisasm, &dr);
					MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dr, 2);
					// Expand left to cover the hidden flow arrows panel area
					HWND hArrows = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
					if (hArrows && g_FlowArrowsVisible && DisassemblerReady) {
						dr.left -= g_FlowArrowPanelWidth;
					}
					MoveWindow(hCFGChild, dr.left, dr.top, dr.right - dr.left, dr.bottom - dr.top, TRUE);
				}
			}
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
			// Flow arrow panel right-edge drag handle
			HWND hArrowPanel = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
			if (hArrowPanel && IsWindowVisible(hArrowPanel)) {
				RECT arrowRect;
				GetWindowRect(hArrowPanel, &arrowRect);
				MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&arrowRect, 2);
				RECT hitRect = { arrowRect.right - FLOW_ARROW_SPLITTER_HIT,
					arrowRect.top, arrowRect.right + FLOW_ARROW_SPLITTER_HIT, arrowRect.bottom };
				if (PtInRect(&hitRect, pt)) {
					SetCursor(LoadCursor(NULL, IDC_SIZEWE));
					return TRUE;
				}
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
						HWND hCFGChild = GetDlgItem(hWnd, IDC_CFG_CHILD);
						HWND hArrowPanel = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
						bool hasArrows = (hArrowPanel && IsWindowVisible(hArrowPanel));
						int nWindows = 3 + (hCFGChild ? 1 : 0) + (hasArrows ? 1 : 0);
						HDWP hdwp = BeginDeferWindowPos(nWindows);
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

							// Keep CFG child in sync with disasm (expand for hidden arrows panel)
							if (hCFGChild) {
								int cfgLeft = disasmRect.left;
								int cfgWidth = disasmRect.right - disasmRect.left;
								if (g_FlowArrowsVisible && DisassemblerReady) {
									cfgLeft -= g_FlowArrowPanelWidth;
									cfgWidth += g_FlowArrowPanelWidth;
								}
								hdwp = DeferWindowPos(hdwp, hCFGChild, NULL,
									cfgLeft, disasmRect.top,
									cfgWidth,
									newDisasmHeight,
									SWP_NOZORDER);
							}

							// Keep flow arrows panel in sync with disasm
							if (hasArrows) {
								hdwp = DeferWindowPos(hdwp, hArrowPanel, NULL,
									disasmRect.left - g_FlowArrowPanelWidth,
									disasmRect.top,
									g_FlowArrowPanelWidth,
									newDisasmHeight,
									SWP_NOZORDER);
							}

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
            // STATUS BAR: MESSAGE1, MESSAGE2, and PROGRESS are laid out
            // explicitly in WM_SIZE rather than using anchor flags, so that
            // they tile the full window width without gaps or clipping.
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
            EnableMenuItem(GetMenu(hWnd),IDC_SHOW_FUNCTIONS,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_XREF_MENU,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDM_PATCHER,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_DISASM_ADD_COMMENT,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_PVSCRIPT_ENGINE,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDM_SAVE_PROJECT,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_PRODUCE_MASM,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_PRODUCE_VIEW,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_EXPORT_MAP_PVDASM,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_VIEW_CFG,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_VIEW_DISASSEMBLY,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_VIEW_GRAPH,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_CODE_MAP,MF_GRAYED);
            EnableMenuItem(GetMenu(hWnd),IDC_CONTROL_FLOW,MF_GRAYED);

			// Create the ToolBar.
			hWndTB=CreateToolBar(hWnd,hInst);

            // Check if Add-in has Loaded.
			if(hRadHex==NULL){
                EnableMenuItem(GetMenu(hWnd),IDC_HEX_EDITOR,MF_GRAYED);
			}

            // Hide the Progress bar (Show only when disassembling).
            ShowWindow(GetDlgItem(hWnd,IDC_DISASM_PROGRESS),SW_HIDE);

            // Register and create the Code Map bar (above tabs, initially hidden)
            if (!g_CodeMapClassRegistered) {
                WNDCLASSA wc = {0};
                wc.lpfnWndProc = CodeMapWndProc;
                wc.hInstance = hInst;
                wc.hCursor = LoadCursor(NULL, IDC_ARROW);
                wc.lpszClassName = "PVDasmCodeMap";
                RegisterClassA(&wc);
                g_CodeMapClassRegistered = true;
            }

            // Create tab control and content below it
            {
                HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
                RECT disasmRect;
                GetWindowRect(hDisasm, &disasmRect);
                MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&disasmRect, 2);
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);

                // Create code map bar at disasmRect.top (hidden, WS_CHILD only)
                HWND hCodeMap = CreateWindowExA(0, "PVDasmCodeMap", NULL,
                    WS_CHILD,  // Initially hidden
                    0, disasmRect.top, rcClient.right, CODE_MAP_HEIGHT,
                    hWnd, (HMENU)(UINT_PTR)IDC_CODE_MAP_BAR, hInst, NULL);
                SetWindowLongPtr(hCodeMap, GWL_USERDATA, ANCHOR_RIGHT);

                // Create tab control at disasmRect.top (code map is hidden so tab starts here)
                HWND hTab = CreateWindowEx(0, WC_TABCONTROL, NULL,
                    WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_FIXEDWIDTH | TCS_OWNERDRAWFIXED,
                    0, disasmRect.top, rcClient.right, TAB_HEIGHT,
                    hWnd, (HMENU)(UINT_PTR)IDC_TAB_MAIN, hInst, NULL);
                SendMessage(hTab, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
                TabCtrl_SetItemSize(hTab, 100, TAB_HEIGHT - 4);

                // Insert tabs with lParam storing logical ID
                TCITEM tie = {0};
                tie.mask = TCIF_TEXT | TCIF_PARAM;
                tie.pszText = (LPSTR)"Disassembly";
                tie.lParam = 0;
                TabCtrl_InsertItem(hTab, 0, &tie);
                tie.pszText = (LPSTR)"Graph";
                tie.lParam = 1;
                TabCtrl_InsertItem(hTab, 1, &tie);

                SetWindowLongPtr(hTab, GWL_USERDATA, ANCHOR_RIGHT);

                // Subclass tab control for close button click detection and background painting
                g_OrigTabWndProc = (WNDPROC)SetWindowLongPtr(hTab, GWL_WNDPROC, (LONG_PTR)TabSubClassProc);

                // Push the disasm listview down by tab height
                MoveWindow(hDisasm, disasmRect.left, disasmRect.top + TAB_HEIGHT,
                           disasmRect.right - disasmRect.left,
                           (disasmRect.bottom - disasmRect.top) - TAB_HEIGHT, TRUE);

                // Register and create CFG child window (same position as disasm, hidden initially)
                RegisterCFGChildClass(hInst);
                RECT newDisasmRect;
                GetWindowRect(hDisasm, &newDisasmRect);
                MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&newDisasmRect, 2);
                HWND hCFGChild = CreateWindowExA(0, "PVDasmCFGChild", NULL,
                    WS_CHILD | WS_CLIPSIBLINGS,
                    newDisasmRect.left, newDisasmRect.top,
                    newDisasmRect.right - newDisasmRect.left,
                    newDisasmRect.bottom - newDisasmRect.top,
                    hWnd, (HMENU)(UINT_PTR)IDC_CFG_CHILD, hInst, NULL);
                SetWindowLongPtr(hCFGChild, GWL_USERDATA, ANCHOR_RIGHT | ANCHOR_BOTTOM);
            }

            // Register and create the Flow Arrows panel (left of disasm ListView, hidden initially)
            if (!g_FlowArrowsClassRegistered) {
                WNDCLASSA wc = {0};
                wc.lpfnWndProc = FlowArrowWndProc;
                wc.hInstance = hInst;
                wc.hCursor = LoadCursor(NULL, IDC_ARROW);
                wc.lpszClassName = "PVDasmFlowArrows";
                RegisterClassA(&wc);
                g_FlowArrowsClassRegistered = true;
            }
            {
                HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
                RECT dr;
                GetWindowRect(hDisasm, &dr);
                MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dr, 2);

                // Create arrow panel at left edge of disasm (initially hidden until disassembly completes)
                HWND hArrowPanel = CreateWindowExA(0, "PVDasmFlowArrows", NULL,
                    WS_CHILD | WS_CLIPSIBLINGS,  // Hidden initially
                    dr.left - g_FlowArrowPanelWidth, dr.top,
                    g_FlowArrowPanelWidth, dr.bottom - dr.top,
                    hWnd, (HMENU)(UINT_PTR)IDC_FLOW_ARROWS, hInst, NULL);
                SetWindowLongPtr(hArrowPanel, GWL_USERDATA, ANCHOR_BOTTOM);
            }

            // Check the menu item if Code Map is enabled (bar stays hidden until disassembly completes)
            if (g_CodeMapVisible) {
                CheckMenuItem(GetMenu(hWnd), IDC_CODE_MAP, MF_CHECKED);
            }

            // Check the menu item if Control Flow is enabled
            if (g_FlowArrowsVisible) {
                CheckMenuItem(GetMenu(hWnd), IDC_CONTROL_FLOW, MF_CHECKED);
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
                    // Tab control selection change
                    case IDC_TAB_MAIN: {
                        LPNMHDR pnmh = (LPNMHDR)lParam;
                        if (pnmh->code == TCN_SELCHANGE) {
                            HWND hTab = GetDlgItem(hWnd, IDC_TAB_MAIN);
                            int sel = TabCtrl_GetCurSel(hTab);
                            int logicalID = TabIndexToLogical(hTab, sel);
                            SwitchTab(hWnd, logicalID);
                        }
                    }
                    break;

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
                        // Calculate Offset in EXE (32-bit)
                        if(StringLen(text)!=0){
                            Offset=StringToDword(text);
                            if(!disop.VBPCodeMode && SectionHeader){
                                Offset -= nt_hdr->OptionalHeader.ImageBase;
                                Offset = (Offset - SectionHeader->VirtualAddress)+SectionHeader->PointerToRawData;
                            }
                        }
                        wsprintf(Message," Line: #%ld Code Address: @%s Code Offset: @%08X",item,text,Offset);
                        SetDlgItemText(hWnd,IDC_MESSAGE1,Message);
                    }
                    else if(LoadedPe64==TRUE){
                        // Calculate Offset in EXE (64-bit)
                        if(StringLen(text)!=0){
                            ULONGLONG Addr64 = StringToQword(text);
                            ULONGLONG Offset64 = 0;
                            if(SectionHeader){
                                Offset64 = Addr64 - nt_hdr64->OptionalHeader.ImageBase;
                                Offset64 = (Offset64 - SectionHeader->VirtualAddress)+SectionHeader->PointerToRawData;
                            }
                            wsprintf(Message," Line: #%ld Code Address: @%s Code Offset: @%08X%08X",
                                     item, text, (DWORD)(Offset64>>32), (DWORD)Offset64);
                        }
                        else{
                            wsprintf(Message," Line: #%ld Code Address: @%s",item,text);
                        }
                        SetDlgItemText(hWnd,IDC_MESSAGE1,Message);
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
                     hDisasmThread = NULL;
                     EnableMenuItem(GetMenu(hWnd),IDC_STOP_DISASM,MF_GRAYED); 
                     EnableMenuItem(GetMenu(hWnd),IDC_DISASM_IMP,MF_GRAYED);
                     EnableMenuItem(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_GRAYED);
                     EnableMenuItem(GetMenu(hWnd),IDC_START_DISASM,MF_ENABLED);
                     CheckMenuItem(GetMenu(hWnd),IDC_PAUSE_DISASM,MF_UNCHECKED); // Clear of checked
                     SetDlgItemText(hWnd,IDC_MESSAGE1,"Disassembly Process Stoped");
                     SetDlgItemText(hWnd,IDC_MESSAGE2,"");
                     SetDlgItemText(hWnd,IDC_MESSAGE3,"");
                     // reset and hide the progressBar
                     SendDlgItemMessage(hWnd,IDC_DISASM_PROGRESS,PBM_SETPOS,0,0);
                     ShowWindow(GetDlgItem(hWnd,IDC_DISASM_PROGRESS),SW_HIDE);
                     // Trigger status bar re-layout
                     {RECT rc; GetClientRect(hWnd, &rc);
                     PostMessage(hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));}
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

                // Show Functions
                case IDC_SHOW_FUNCTIONS:{
                    DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_FUNCTION_REF), hWnd, (DLGPROC)FunctionRefDlgProc);
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

                // Toggle Control Flow arrows panel
                case IDC_CONTROL_FLOW:{
                    HWND hArrowPanel = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
                    HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
                    if (!hArrowPanel || !hDisasm) break;

                    g_FlowArrowsVisible = !g_FlowArrowsVisible;
                    CheckMenuItem(GetMenu(hWnd), IDC_CONTROL_FLOW,
                        g_FlowArrowsVisible ? MF_CHECKED : MF_UNCHECKED);

                    RECT dr;
                    GetWindowRect(hDisasm, &dr);
                    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dr, 2);

                    if (g_FlowArrowsVisible) {
                        // Shrink ListView from the left to make room for arrow panel
                        MoveWindow(hDisasm, dr.left + g_FlowArrowPanelWidth, dr.top,
                            (dr.right - dr.left) - g_FlowArrowPanelWidth,
                            dr.bottom - dr.top, TRUE);
                        // Position and show arrow panel
                        MoveWindow(hArrowPanel, dr.left, dr.top,
                            g_FlowArrowPanelWidth, dr.bottom - dr.top, TRUE);
                        ShowWindow(hArrowPanel, SW_SHOW);
                        if (DisassemblerReady && g_FlowArrows.empty()) {
                            BuildFlowArrowData();
                        }
                        InvalidateRect(hArrowPanel, NULL, FALSE);
                    } else {
                        // Hide arrow panel and expand ListView back
                        ShowWindow(hArrowPanel, SW_HIDE);
                        MoveWindow(hDisasm, dr.left - g_FlowArrowPanelWidth, dr.top,
                            (dr.right - dr.left) + g_FlowArrowPanelWidth,
                            dr.bottom - dr.top, TRUE);
                    }
                    StretchLastColumn(hDisasm);
                    SaveSettings();
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

                // Toggle Disassembly tab visibility (Views menu)
                case IDC_VIEW_DISASSEMBLY:{
                    if (!DisassemblerReady) break;
                    bool newState = !g_bDisasmTabVisible;
                    // Prevent hiding the last visible tab
                    if (!newState && !g_bGraphTabVisible) break;
                    SetTabVisible(hWnd, 0, newState);
                }
                break;

                // Toggle / show Graph tab (Views menu + Ctrl+G)
                case IDC_VIEW_GRAPH:
                case IDC_VIEW_CFG:{
                    if (!DisassemblerReady) break;
                    if (!g_bGraphTabVisible) {
                        // Show the graph tab, switch to it, and load CFG
                        SetTabVisible(hWnd, 1, true);
                        SwitchTab(hWnd, 1);
                        LoadCFGForCurrentFunction_Embedded();
                    } else {
                        // Already visible, just switch to it
                        SwitchTab(hWnd, 1);
                        LoadCFGForCurrentFunction_Embedded();
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

				// Copy address of selected line to clipboard
				case IDM_COPY_ADDRESS:{
                    int iSel = (int)SendMessage(GetDlgItem(hWnd,IDC_DISASM), LVM_GETNEXTITEM, (WPARAM)-1, LVNI_FOCUSED);
                    if(iSel >= 0 && iSel < (int)DisasmDataLines.size()){
                        char* addr = DisasmDataLines[iSel].GetAddress();
                        if(addr && addr[0]){
                            CopyToClipboard(addr, hWnd);
                        }
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

	// Custom message from CFG child window to switch back to Disassembly tab
	case WM_USER + 300: {
		SwitchTab(hWnd, 0);
		SetFocus(GetDlgItem(hWnd, IDC_DISASM));
		return 0;
	}
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
            // Hide code map from previous file before freeing memory
            HWND hBar = GetDlgItem(hWnd, IDC_CODE_MAP_BAR);
            if (hBar && IsWindowVisible(hBar)) {
                ShowWindow(hBar, SW_HIDE);
                RepositionDisasmForCodeMap(hWnd, FALSE);
            }
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
    EnableMenuItem ( hMenu, IDC_SHOW_FUNCTIONS,		MF_GRAYED  );
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
	OrignalData=NULL;
	Data=NULL;
	OutDebug(hWnd,"UnMapped File Successfuly");

	// Close the map handle
	if(CloseHandle(hFileMap)==NULL){ // סגירת המזהה של המפה
		ErrorMsg=6;
		return false;
	}
	hFileMap=NULL;
	OutDebug(hWnd,"Mapped File Closed Successfuly");

	// Close the file handle
	if(CloseHandle(hFile)==NULL){ // סגירת המזהה של הקובץ
		ErrorMsg=7;
		return false;
	}
	hFile=NULL;
	// no errors
	ErrorMsg=0;
	OutDebug(hWnd,"File Closed Successfuly");
	OutDebug(hWnd,"");
    FilesInMemory=false;
    LoadedNonePe=false;
    LoadedPe=false;
	LoadedPe64=false;
	LoadedNe=false;
	SectionHeader=NULL;

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
            char		Api[512],temp[512],*ptr;
            
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

                    char* comments = DisasmDataLines[i].GetComments();
                    if(!comments || !comments[0]) continue;
                    strcpy_s(temp, sizeof(temp), comments);
                    ptr = strchr(temp, ':');
                    if(!ptr) continue;
                    ptr++; // skip ':'
                    if(*ptr == ' ') ptr++; // skip space after ':'
                    strcpy_s(Api, sizeof(Api), ptr);
                    len=StringLen(Api);
                    if(len > 0 && Api[len-1]==']')
                        Api[len-1]='\0';
                    
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
//============================  FunctionRefDlgProc =====================================
//======================================================================================

static HWND Function_hWnd = NULL;
static WNDPROC OldFunctionWndProc = NULL;

// Helper function to find a function match starting from a given index
int FindFunctionMatch(HWND hDlg, int startIndex, bool wrapAround)
{
    char SearchText[256], Temp[256];
    HWND hList = GetDlgItem(hDlg, IDC_FUNCTION_REF_LV);
    int totalItems = ListView_GetItemCount(hList);

    if(totalItems == 0) return -1;

    SendDlgItemMessage(hDlg, IDC_FUNCTION_FIND, WM_GETTEXT, (WPARAM)256, (LPARAM)SearchText);
    if(StringLen(SearchText) == 0) return -1;

    bool useContains = (SendDlgItemMessage(hDlg, IDC_FUNCTION_CONTAIN, BM_GETCHECK, (WPARAM)BST_CHECKED, (LPARAM)0) == BST_CHECKED);

    char SearchLower[256];
    lstrcpy(SearchLower, SearchText);
    CharLower(SearchLower);

    int searchCount = wrapAround ? totalItems : (totalItems - startIndex);

    for(int i = 0; i < searchCount; i++){
        int idx = (startIndex + i) % totalItems;

        ListView_GetItemText(hList, idx, 1, Temp, 256);

        bool found = false;
        if(useContains){
            char TempLower[256];
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

// Helper to deselect all items in the function ListView
void DeselectAllFunctions(HWND hList)
{
    int count = ListView_GetItemCount(hList);
    for(int i = 0; i < count; i++){
        ListView_SetItemState(hList, i, 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
}

// Helper to perform search and select result
void DoFunctionSearch(HWND hDlg, int startIndex)
{
    HWND hList = GetDlgItem(hDlg, IDC_FUNCTION_REF_LV);
    int foundIdx = FindFunctionMatch(hDlg, startIndex, true);

    if(foundIdx != -1){
        SetFocus(hList);
        SelectItem(hList, foundIdx);
        SetFocus(GetDlgItem(hDlg, IDC_FUNCTION_FIND));
    }
    else{
        DeselectAllFunctions(hList);
    }
}

LRESULT CALLBACK FunctionSearchProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg){
        case WM_KEYDOWN:
        {
            if(wParam == VK_F3){
                HWND hList = GetDlgItem(Function_hWnd, IDC_FUNCTION_REF_LV);
                int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                DoFunctionSearch(Function_hWnd, startIdx);
                return 0;
            }
        }
        break;

        case WM_KEYUP:
        {
            if(wParam == VK_F3) break;

            char SearchText[256];
            SendDlgItemMessage(Function_hWnd, IDC_FUNCTION_FIND, WM_GETTEXT, (WPARAM)256, (LPARAM)SearchText);
            if(StringLen(SearchText) == 0){
                SetWindowText(Function_hWnd, " Show Functions");
                DeselectAllFunctions(GetDlgItem(Function_hWnd, IDC_FUNCTION_REF_LV));
            }
            else{
                SetWindowText(Function_hWnd, SearchText);
                DoFunctionSearch(Function_hWnd, 0);
            }
        }
        break;
    }

    return CallWindowProc(OldFunctionWndProc, hWnd, uMsg, wParam, lParam);
}

BOOL CALLBACK FunctionRefDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch(Message){
        case WM_INITDIALOG:{
            LV_COLUMN   LvCol;
            LVITEM      LvItem;
            HWND        hList;
            DWORD       Index=0;
            char        *mnemonic;

            // Set Radio Button Checked
            SendDlgItemMessage(hWnd,IDC_FUNCTION_CONTAIN,BM_SETCHECK,(WPARAM)BST_CHECKED,(LPARAM)0);
            SetFocus(GetDlgItem(hWnd,IDC_FUNCTION_FIND));

            // Subclass the edit control for search
            OldFunctionWndProc=(WNDPROC)SetWindowLongPtr(GetDlgItem(hWnd,IDC_FUNCTION_FIND),GWL_WNDPROC,(LONG_PTR)FunctionSearchProc);
            Function_hWnd=hWnd;

            hList=GetDlgItem(hWnd,IDC_FUNCTION_REF_LV);
            SendMessage(hList,LVM_SETEXTENDEDLISTVIEWSTYLE,0,LVS_EX_FULLROWSELECT);

            // Add Columns
            memset(&LvCol,0,sizeof(LvCol));
            LvCol.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_SUBITEM;
            LvCol.pszText="Address";
            LvCol.cx=0x50;
            SendMessage(hList,LVM_INSERTCOLUMN,0,(LPARAM)&LvCol);
            LvCol.pszText="Function";
            LvCol.cx=0x140;
            SendMessage(hList,LVM_INSERTCOLUMN,1,(LPARAM)&LvCol);

            memset(&LvItem,0,sizeof(LvItem));
            LvItem.cchTextMax = 256;

            try{
                // Iterate through all disassembly lines and find function banners
                for(DWORD i=0; i < DisasmDataLines.size(); i++){
                    mnemonic = DisasmDataLines[i].GetMnemonic();
                    // Function banners start with "; ======"
                    if(mnemonic != NULL && strncmp(mnemonic, "; ======", 8) == 0){
                        // Extract function name from banner
                        // Format 1: "; ====== FuncName ======"
                        // Format 2: "; ===========[ Program Entry Point ]==========="
                        char funcName[256];
                        const char* openBracket = strchr(mnemonic, '[');
                        if(openBracket != NULL){
                            // Format 2: extract text between [ and ]
                            const char* closeBracket = strchr(openBracket, ']');
                            if(closeBracket != NULL && (closeBracket - openBracket - 1) < 255){
                                int len = (int)(closeBracket - openBracket - 1);
                                memcpy(funcName, openBracket + 1, len);
                                funcName[len] = '\0';
                            }
                            else{
                                lstrcpyn(funcName, openBracket + 1, 255);
                            }
                        }
                        else{
                            // Format 1: extract text between "; ====== " and " ======"
                            const char* start = mnemonic + 9; // skip "; ====== "
                            const char* end = strstr(start, " ======");
                            if(end != NULL && (end - start) < 255){
                                int len = (int)(end - start);
                                memcpy(funcName, start, len);
                                funcName[len] = '\0';
                            }
                            else{
                                lstrcpyn(funcName, start, 255);
                            }
                        }

                        // Use the next line's address (the actual function start)
                        DWORD nextIdx = i + 1;
                        char* addr = "";
                        if(nextIdx < DisasmDataLines.size()){
                            addr = DisasmDataLines[nextIdx].GetAddress();
                        }

                        LvItem.iItem=Index;
                        LvItem.iSubItem=0;
                        LvItem.pszText = addr;
                        LvItem.lParam = nextIdx;  // Store the line index of the first instruction
                        LvItem.mask=LVIF_TEXT|LVIF_PARAM|LVIF_STATE;
                        SendMessage(hList,LVM_INSERTITEM,0,(LPARAM)&LvItem);
                        LvItem.mask=LVIF_TEXT;

                        LvItem.iItem=Index;
                        LvItem.iSubItem=1;
                        LvItem.pszText=funcName;
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
                case IDC_FUNCTION_CANCEL:{
                    EndDialog(hWnd,0);
                }
                break;

                case IDC_FUNCTION_FINDNEXT:{
                    HWND hList = GetDlgItem(hWnd, IDC_FUNCTION_REF_LV);
                    int currentSel = (int)SendMessage(hList, LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
                    int startIdx = (currentSel >= 0) ? currentSel + 1 : 0;
                    DoFunctionSearch(hWnd, startIdx);
                }
                break;
            }
        }
        break;

        case WM_NOTIFY:{
            switch(LOWORD(wParam)){
                case IDC_FUNCTION_REF_LV:{
                    if(((LPNMHDR)lParam)->code == NM_CLICK || ((LPNMHDR)lParam)->code == LVN_ITEMCHANGED){
                        LVITEM      LvItem;
                        DWORD_PTR   dwSelectedItem;
                        HWND        hRefList=GetDlgItem(hWnd,IDC_FUNCTION_REF_LV);

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
                        HWND hLV = GetDlgItem(Main_hWnd, IDC_DISASM);
                        DWORD_PTR sel = SendMessage(hLV, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_FOCUSED|LVNI_SELECTED);
                        bool reverseFound = false;

                        if (sel != (DWORD_PTR)-1 && sel < DisasmDataLines.size()) {
                            // Check if we're continuing a previous cycle
                            bool continuing = (g_RevDestAddr != 0 && sel == g_RevLastNavigated);

                            DWORD_PTR lookupAddr;
                            if (continuing) {
                                // Continue cycling callers of the same destination
                                lookupAddr = g_RevDestAddr;
                                g_RevCallerIdx++;
                            } else {
                                // Fresh lookup on current line's address
                                const char* addrStr = DisasmDataLines[sel].GetAddress();
                                lookupAddr = LoadedPe64 ? (DWORD_PTR)StringToQword((char*)addrStr)
                                                        : (DWORD_PTR)StringToDword((char*)addrStr);
                                g_RevCallerIdx = 0;
                            }

                            // Collect all callers targeting this address
                            std::vector<DWORD_PTR> callers;
                            for (unsigned int i = 0; i < DisasmCodeFlow.size(); i++) {
                                if (DisasmCodeFlow[i].Branch_Destination == lookupAddr)
                                    callers.push_back(DisasmCodeFlow[i].Current_Index);
                            }

                            if (!callers.empty()) {
                                // Wrap around
                                g_RevCallerIdx = g_RevCallerIdx % (int)callers.size();
                                DWORD_PTR srcIndex = callers[g_RevCallerIdx];

                                // Clear stale trace history
                                BranchTrace.clear();
                                HMENU hMenu = GetMenu(Main_hWnd);
                                EnableMenuItem(hMenu, ID_GOTO_RETURN_JUMP, MF_GRAYED);
                                EnableMenuItem(hMenu, ID_GOTO_RETURN_CALL, MF_GRAYED);
                                SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_JUMP, (LPARAM)FALSE);
                                SendMessage(hWndTB, TB_ENABLEBUTTON, ID_RET_CALL, (LPARAM)FALSE);

                                // Update cycling state
                                g_RevDestAddr = lookupAddr;
                                g_RevLastNavigated = srcIndex;

                                // Navigate to the source branch instruction
                                char dbg[128];
                                if (callers.size() > 1)
                                    wsprintf(dbg, "Tracing Back To -> %s (%d of %d)",
                                             DisasmDataLines[srcIndex].GetAddress(),
                                             g_RevCallerIdx + 1, (int)callers.size());
                                else
                                    wsprintf(dbg, "Tracing Back To -> %s",
                                             DisasmDataLines[srcIndex].GetAddress());
                                OutDebug(Main_hWnd, dbg);
                                SelectLastItem(GetDlgItem(Main_hWnd, IDC_LIST));
                                SetFocus(hLV);
                                SelectItem(hLV, srcIndex);
                                GetBranchDestination(hLV);
                                reverseFound = true;
                            }
                        }
                        // If current line isn't a branch destination, try normal back-trace
                        if (!reverseFound) {
                            g_RevDestAddr = 0;
                            g_RevLastNavigated = (DWORD_PTR)-1;
                            g_RevCallerIdx = 0;
                            BackTrace();
                        }
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

    // Update Code Map and Flow Arrows on scroll/navigation events
    if (uMsg == WM_VSCROLL || uMsg == WM_MOUSEWHEEL || uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_LBUTTONUP) {
        HWND hBar = g_CodeMapVisible ? GetDlgItem(Main_hWnd, IDC_CODE_MAP_BAR) : NULL;
        HWND hArrowPanel = g_FlowArrowsVisible ? GetDlgItem(Main_hWnd, IDC_FLOW_ARROWS) : NULL;
        if (hBar || (hArrowPanel && IsWindowVisible(hArrowPanel))) {
            LRESULT result = CallWindowProc(LVOldWndProc, hWnd, uMsg, wParam, lParam);
            if (hBar) {
                InvalidateRect(hBar, NULL, FALSE);
                UpdateWindow(hBar);
            }
            if (hArrowPanel && IsWindowVisible(hArrowPanel)) {
                InvalidateRect(hArrowPanel, NULL, FALSE);
                UpdateWindow(hArrowPanel);
            }
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
    // Prevent handlers from accessing data while we free it.
    DisassemblerReady=FALSE;

    // Wait for the disassembly worker thread to finish before freeing data.
    // The thread may still be running post-processing (LocateXrefs, etc.)
    // even after setting DisassemblerReady=TRUE.
    // We pump messages while waiting to prevent deadlock: the worker thread
    // uses SendMessage for UI updates, which blocks until the UI thread
    // processes them. Without message pumping, both threads would block.
    if(hDisasmThread){
        // DisassemblerReady is FALSE — worker thread checks this flag
        // in LocateXrefs/LoadApiSignature and exits quickly.
        // Pump messages while waiting so SendMessage calls from the
        // worker thread don't deadlock.
        while(TRUE){
            DWORD result = MsgWaitForMultipleObjects(1, &hDisasmThread, FALSE, INFINITE, QS_ALLINPUT);
            if(result == WAIT_OBJECT_0) break;      // Thread finished
            if(result == WAIT_OBJECT_0 + 1){         // Message arrived
                MSG msg;
                while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)){
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            else break; // WAIT_FAILED or other error
        }
        CloseHandle(hDisasmThread);
        hDisasmThread = NULL;
    }

    // IMPORTANT: Reset the listview BEFORE clearing data vectors.
    // This prevents NM_CUSTOMDRAW / LVN_GETDISPINFO callbacks from
    // accessing freed DisasmDataLines, ImportsLines, or DisasmCodeFlow.
    HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
    ShowWindow(hDisasm, SW_HIDE);
    SendMessage(hDisasm, LVM_SETITEMCOUNT, 0, 0);
    Clear(hDisasm);

    // Close file handles and unmap memory.
    CloseFiles(hWnd);

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

    // Clear SEH address data.
    SEHAddresses.clear();
    MapTree(SEHAddresses).swap(SEHAddresses);

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

    WizardCodeList.clear();
    WizardList(WizardCodeList).swap(WizardCodeList);

    // Set FirstPass ON, on new file loading.
    LoadFirstPass=TRUE;

    SetWindowText(hWnd,PVDASM);
}

void ExitPVDasm(HWND hWnd)
{
	if( MessageBox(hWnd,"Exit From PVDasm Disassembler?","Notice",MB_YESNO)==IDYES){
		// Wait for worker thread before exit
		if(hDisasmThread){
			WaitForSingleObject(hDisasmThread, 5000);
			CloseHandle(hDisasmThread);
			hDisasmThread = NULL;
		}
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
        EnableMenuItem(hWinMenu, IDC_SHOW_FUNCTIONS,		MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_XREF_MENU,				MF_GRAYED);
        EnableMenuItem(hWinMenu, IDM_PATCHER,				MF_GRAYED);
        EnableMenuItem(hWinMenu, IDM_FIND,					MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_DISASM_ADD_COMMENT,	MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_VIEW_CFG,			MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_VIEW_DISASSEMBLY,	MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_VIEW_GRAPH,		MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_CODE_MAP,			MF_GRAYED);
        EnableMenuItem(hWinMenu, IDC_CONTROL_FLOW,		MF_GRAYED);

        // Reset tab visibility to defaults
        g_bDisasmTabVisible = true;
        g_bGraphTabVisible  = true;
        CheckMenuItem(GetMenu(hWnd), IDC_VIEW_DISASSEMBLY, MF_CHECKED);
        CheckMenuItem(GetMenu(hWnd), IDC_VIEW_GRAPH, MF_CHECKED);

        // Clear embedded CFG and switch back to disassembly tab
        ClearEmbeddedCFG();
        RebuildTabs(hWnd);
        SwitchTab(hWnd, 0);

        // Hide Code Map bar when file is closed
        {
            HWND hBar = GetDlgItem(hWnd, IDC_CODE_MAP_BAR);
            if (hBar && IsWindowVisible(hBar)) {
                ShowWindow(hBar, SW_HIDE);
                RepositionDisasmForCodeMap(hWnd, FALSE);
            }
            g_CodeMapTypes.clear();
            if (g_CodeMapCacheBmp) { DeleteObject(g_CodeMapCacheBmp); g_CodeMapCacheBmp = NULL; }
        }

        // Hide Flow Arrows panel when file is closed
        {
            HWND hArrowPanel = GetDlgItem(hWnd, IDC_FLOW_ARROWS);
            HWND hDisasm = GetDlgItem(hWnd, IDC_DISASM);
            if (hArrowPanel && IsWindowVisible(hArrowPanel) && hDisasm) {
                ShowWindow(hArrowPanel, SW_HIDE);
                // Expand ListView back
                RECT dr;
                GetWindowRect(hDisasm, &dr);
                MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&dr, 2);
                MoveWindow(hDisasm, dr.left - g_FlowArrowPanelWidth, dr.top,
                    (dr.right - dr.left) + g_FlowArrowPanelWidth,
                    dr.bottom - dr.top, TRUE);
            }
            g_FlowArrows.clear();
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