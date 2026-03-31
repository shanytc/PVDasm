/*
     8888888b.                  888     888 d8b
     888   Y88b                 888     888 Y8P
     888    888                 888     888
     888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888
     8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888
     888        888     888  888  Y88o88P   888 88888888 888  888  888
     888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P
     888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"

    Control Flow Graph (CFG) Viewer
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Written by Shany Golan, 2026.
    Displays function control flow as a graph with basic blocks.

    File: CFGViewer.h
*/

#ifndef __CFG_VIEWER_H__
#define __CFG_VIEWER_H__

#include <windows.h>
#include <vector>
#include <map>
#include <set>
#include <queue>
#include "x64.h"

// ================================================================
// ====================  DATA STRUCTURES  =========================
// ================================================================

// Edge types for visual differentiation
typedef enum CFGEdgeType {
    EDGE_UNCONDITIONAL = 0,    // JMP (always taken)
    EDGE_CONDITIONAL_TRUE,     // Jcc when condition is true (branch taken)
    EDGE_CONDITIONAL_FALSE,    // Fall-through when condition is false
    EDGE_CALL                  // CALL instruction edge (blue/purple)
} CFG_EDGE_TYPE;

// Represents a single basic block in the CFG
typedef struct CFGBasicBlock {
    DWORD_PTR   BlockID;           // Unique block identifier
    DWORD_PTR   StartAddress;      // First instruction address
    DWORD_PTR   EndAddress;        // Last instruction address
    DWORD_PTR   StartIndex;        // Index in DisasmDataLines for first instruction
    DWORD_PTR   EndIndex;          // Index in DisasmDataLines for last instruction

    // Layout information (in graph coordinates)
    int         X;                 // X position in graph space
    int         Y;                 // Y position in graph space
    int         Width;             // Calculated width based on text
    int         Height;            // Calculated height based on instruction count
    int         Layer;             // Hierarchical layer (for layout algorithm)
    int         PositionInLayer;   // Position within the layer (left to right)

    // Block properties
    bool        IsEntryBlock;      // Function entry point
    bool        IsExitBlock;       // Contains RET or no successors
    bool        IsSelected;        // Currently selected by user

    // Function label
    char        FunctionLabel[64]; // Function/proc name if this is a function entry

    // Cached rendering data
    COLORREF    CachedBgColor;     // Pre-computed background color

    // Comment alignment (0 = not aligned, >0 = pixel width for mnemonic column)
    int         AlignedCommentCol;

} CFG_BASIC_BLOCK;

// Represents an edge between two basic blocks
typedef struct CFGEdge {
    DWORD_PTR       SourceBlockID;     // Source block ID
    DWORD_PTR       TargetBlockID;     // Target block ID
    CFG_EDGE_TYPE   Type;              // Edge type for visual differentiation
    bool            IsBackEdge;        // Loop back edge (target layer < source layer)

} CFG_EDGE;

// Cached edge route for rendering (computed once after layout, reused per frame)
typedef struct CFGEdgeRoute {
    POINT Points[12];              // Polyline vertices
    int   PointCount;              // Number of valid points
    int   ArrowFromX, ArrowFromY;  // Penultimate point (for arrowhead angle)
    int   ArrowToX, ArrowToY;     // Arrowhead tip
    int   LabelX, LabelY;         // T/F/C label position
    int   StartPortOffsetX;        // startX - srcBlock.X (preserved during drag)
    int   EndPortOffsetX;          // endX - dstBlock.X   (preserved during drag)
} CFG_EDGE_ROUTE;

// Main CFG data structure
typedef struct CFGGraph {
    std::vector<CFG_BASIC_BLOCK>        Blocks;
    std::vector<CFG_EDGE>               Edges;

    // Function metadata
    DWORD_PTR                           FunctionStart;
    DWORD_PTR                           FunctionEnd;
    char                                FunctionName[64];

    // Graph layout bounds (in graph coordinates)
    int                                 GraphWidth;
    int                                 GraphHeight;

    // Maps for quick lookup
    std::map<DWORD_PTR, size_t>         AddressToBlockIndex;   // StartAddress -> index in Blocks
    std::map<DWORD_PTR, size_t>         BlockIDToIndex;        // BlockID -> index in Blocks

    // Cached rendering data
    std::vector<CFG_EDGE_ROUTE>         EdgeRoutes;            // Parallel to Edges[]
    bool                                EdgeRoutesDirty;       // Recompute before next render

} CFG_GRAPH;

// View state for zoom and pan
typedef struct CFGViewState {
    double      ZoomLevel;         // 0.25 to 4.0 (25% to 400%)
    double      PanOffsetX;        // Horizontal pan in screen pixels
    double      PanOffsetY;        // Vertical pan in screen pixels

    // Mouse interaction state
    bool        IsPanning;         // Dragging to pan the view
    bool        IsDraggingBlock;   // Dragging a block to move it
    POINT       DragStartPoint;
    double      DragStartPanX;
    double      DragStartPanY;
    int         DragBlockStartX;   // Block's original X when drag started
    int         DragBlockStartY;   // Block's original Y when drag started

    // Selected block
    DWORD_PTR   SelectedBlockID;   // (DWORD_PTR)-1 if none selected
    DWORD_PTR   DraggingBlockID;   // Block being dragged, (DWORD_PTR)-1 if none

    // Overview minimap
    bool        ShowOverview;        // Display the minimap
    bool        IsDraggingOverview;  // Dragging on overview to pan

} CFG_VIEW_STATE;

// ================================================================
// ====================  LAYOUT CONSTANTS  ========================
// ================================================================

#define CFG_BLOCK_H_SPACING     50      // Horizontal spacing between blocks
#define CFG_BLOCK_V_SPACING     40      // Vertical spacing between layers
#define CFG_LINE_HEIGHT         16      // Height per instruction line
#define CFG_BLOCK_PADDING       8       // Padding inside blocks
#define CFG_MIN_BLOCK_WIDTH     120     // Minimum block width
#define CFG_ARROW_SIZE          10      // Arrowhead size

#define CFG_MIN_ZOOM            0.05
#define CFG_MAX_ZOOM            4.0
#define CFG_ZOOM_FACTOR         1.15

#define CFG_OVERVIEW_WIDTH      200
#define CFG_OVERVIEW_HEIGHT     160
#define CFG_OVERVIEW_MARGIN     10

// ================================================================
// ====================  FUNCTION PROTOTYPES  =====================
// ================================================================

// Graph Building
BOOL BuildCFGFromFunction(DWORD_PTR funcStart, DWORD_PTR funcEnd, const char* funcName, CFG_GRAPH* outGraph);
BOOL BuildCFGByTracing(DWORD_PTR startIndex, CFG_GRAPH* outGraph);
void FreeCFGGraph(CFG_GRAPH* graph);
void ClearCFGGraph(CFG_GRAPH* graph);

// Layout
void LayoutCFG(CFG_GRAPH* graph);
void CalculateBlockDimensions(HDC hDC, CFG_GRAPH* graph);

// Rendering
void RenderCFG(HWND hWnd, HDC hDC, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState);
void RenderBlocks(HDC hDC, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState, RECT* visibleRect = NULL);
void RenderEdges(HDC hDC, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState, RECT* visibleRect = NULL);
void DrawArrowhead(HDC hDC, int fromX, int fromY, int toX, int toY, COLORREF color);
void ComputeEdgeRoutes(CFG_GRAPH* graph);
void ComputeBlockColors(CFG_GRAPH* graph);

// Interaction
POINT ScreenToGraph(CFG_VIEW_STATE* viewState, POINT screenPt);
POINT GraphToScreen(CFG_VIEW_STATE* viewState, POINT graphPt);
DWORD_PTR HitTestBlocks(CFG_GRAPH* graph, POINT graphPt);
DWORD_PTR HitTestInstruction(CFG_BASIC_BLOCK* block, POINT graphPt);

// View State
void InitCFGViewState(CFG_VIEW_STATE* state);
void CenterGraphInView(HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* state);
void ZoomAtPoint(CFG_VIEW_STATE* viewState, POINT screenPt, double zoomFactor);

// Overview minimap
void RenderOverview(HDC hDC, HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState, RECT* clientRect);
BOOL HitTestOverview(POINT screenPt, RECT* clientRect);
void PanFromOverviewClick(CFG_VIEW_STATE* viewState, CFG_GRAPH* graph, POINT screenPt, RECT* clientRect);

// Dialog
BOOL CALLBACK CFGViewerDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
void ShowCFGViewerForCurrentFunction(HWND hParent);
HWND GetCFGViewerWindow();  // Returns handle for message loop integration

// Embedded child window support
extern bool g_CFGGraphValid;
void RegisterCFGChildClass(HINSTANCE hInst);
void LoadCFGForCurrentFunction_Embedded();
void RefreshCFGLabels();
void ClearEmbeddedCFG();
LRESULT CALLBACK CFGChildWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Helper functions
DWORD_PTR GetAddressAtIndex(DWORD_PTR index);
DWORD_PTR FindIndexByAddress(DWORD_PTR address);
bool IsConditionalJump(const char* mnemonic);
bool IsUnconditionalJump(const char* mnemonic);
bool IsReturnInstruction(const char* mnemonic);
bool IsCallInstruction(const char* mnemonic);

#endif // __CFG_VIEWER_H__
