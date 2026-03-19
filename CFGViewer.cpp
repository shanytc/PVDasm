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

    File: CFGViewer.cpp
*/

#include "CFGViewer.h"
#include "MappedFile.h"
#include "CDisasmData.h"
#include "Disasm.h"
#include "resource\resource.h"
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// ================================================================
// ====================  EXTERNAL REFERENCES  =====================
// ================================================================

extern DisasmDataArray      DisasmDataLines;
extern CodeBranch           DisasmCodeFlow;
extern FunctionInfo         fFunctionInfo;
extern std::set<DWORD_PTR> CallTargets;
extern HWND                 Main_hWnd;
extern bool                 g_DarkMode;
extern COLORREF             g_DarkBkColor;
extern COLORREF             g_DarkTextColor;
extern IMAGE_NT_HEADERS*    nt_hdr;
extern bool                 LoadedPe64;

// ================================================================
// ====================  GLOBAL STATE  ============================
// ================================================================

static CFG_GRAPH        g_CurrentGraph;
static CFG_VIEW_STATE   g_ViewState;
static HFONT            g_hCFGFont = NULL;
static HWND             g_hCFGViewerWnd = NULL;  // Handle to modeless CFG viewer window
static ULONG_PTR        g_GdiplusToken = 0;

// Cached GDI objects for edge rendering (created once in WM_INITDIALOG)
static HPEN     g_EdgePens[5] = {};       // Width-2 pens: uncond, true, false, backedge, call
static HPEN     g_ArrowPens[5] = {};      // Width-1 pens for arrowhead outlines
static HBRUSH   g_ArrowBrushes[5] = {};   // Solid brushes for arrowhead fill
static COLORREF g_EdgeColors[5] = {};     // Color values for text labels

// ================================================================
// ====================  HELPER FUNCTIONS  ========================
// ================================================================

DWORD_PTR GetAddressAtIndex(DWORD_PTR index)
{
    if (index >= DisasmDataLines.size()) return 0;
    return StringToDword(DisasmDataLines[index].GetAddress());
}

DWORD_PTR FindIndexByAddress(DWORD_PTR address)
{
    // Binary search since addresses are sorted
    size_t left = 0;
    size_t right = DisasmDataLines.size();

    while (left < right) {
        size_t mid = (left + right) / 2;
        DWORD_PTR midAddr = GetAddressAtIndex(mid);

        if (midAddr == address) return mid;
        else if (midAddr < address) left = mid + 1;
        else right = mid;
    }

    // If exact match not found, return closest
    if (left < DisasmDataLines.size()) return left;
    return (DWORD_PTR)-1;
}

// Helper to extract just the opcode from mnemonic string (e.g., "jmp 00401234" -> "jmp")
static void ExtractOpcode(const char* mnemonic, char* opcode, size_t opcodeSize)
{
    if (!mnemonic || !opcode || opcodeSize == 0) return;

    opcode[0] = '\0';

    // Skip leading whitespace
    while (*mnemonic && (*mnemonic == ' ' || *mnemonic == '\t')) mnemonic++;

    // Copy until space or end
    size_t i = 0;
    while (*mnemonic && *mnemonic != ' ' && *mnemonic != '\t' && i < opcodeSize - 1) {
        opcode[i++] = *mnemonic++;
    }
    opcode[i] = '\0';

    // Convert to lowercase
    _strlwr(opcode);
}

bool IsConditionalJump(const char* mnemonic)
{
    if (!mnemonic || mnemonic[0] == '\0') return false;

    // Extract just the opcode
    char opcode[32];
    ExtractOpcode(mnemonic, opcode, sizeof(opcode));

    // Skip if it's just "jmp"
    if (strcmp(opcode, "jmp") == 0) return false;

    // Check for conditional jumps: ja, jae, jb, jbe, jc, je, jg, jge, jl, jle,
    // jnae, jnb, jnbe, jnc, jne, jng, jnge, jnl, jnle, jno, jnp, jns, jnz,
    // jo, jp, jpe, jpo, js, jz, jecxz, jcxz, loop, loope, loopne, loopnz, loopz
    if (opcode[0] == 'j' && strlen(opcode) > 1) return true;
    if (strncmp(opcode, "loop", 4) == 0) return true;

    return false;
}

bool IsUnconditionalJump(const char* mnemonic)
{
    if (!mnemonic || mnemonic[0] == '\0') return false;

    char opcode[32];
    ExtractOpcode(mnemonic, opcode, sizeof(opcode));

    return (strcmp(opcode, "jmp") == 0);
}

bool IsReturnInstruction(const char* mnemonic)
{
    if (!mnemonic || mnemonic[0] == '\0') return false;

    char opcode[32];
    ExtractOpcode(mnemonic, opcode, sizeof(opcode));

    // ret, retn, retf
    return (strncmp(opcode, "ret", 3) == 0);
}

bool IsCallInstruction(const char* mnemonic)
{
    if (!mnemonic || mnemonic[0] == '\0') return false;

    char opcode[32];
    ExtractOpcode(mnemonic, opcode, sizeof(opcode));

    return (strcmp(opcode, "call") == 0);
}

// ================================================================
// ====================  GRAPH BUILDING  ==========================
// ================================================================

BOOL BuildCFGFromFunction(DWORD_PTR funcStart, DWORD_PTR funcEnd, const char* funcName, CFG_GRAPH* outGraph)
{
    if (!outGraph) return FALSE;

    ClearCFGGraph(outGraph);

    // Store function info
    outGraph->FunctionStart = funcStart;
    outGraph->FunctionEnd = funcEnd;
    if (funcName) {
        strncpy(outGraph->FunctionName, funcName, 63);
        outGraph->FunctionName[63] = '\0';
    } else {
        if(LoadedPe64)
            wsprintf(outGraph->FunctionName, "sub_%08X%08X", (DWORD)(funcStart>>32), (DWORD)funcStart);
        else
            wsprintf(outGraph->FunctionName, "sub_%08X", (DWORD)funcStart);
    }

    // Find start and end indices in DisasmDataLines
    DWORD_PTR startIndex = FindIndexByAddress(funcStart);
    DWORD_PTR endIndex = FindIndexByAddress(funcEnd);

    if (startIndex == (DWORD_PTR)-1 || endIndex == (DWORD_PTR)-1) {
        return FALSE;
    }

    // Ensure endIndex is within bounds and actually within function
    if (endIndex >= DisasmDataLines.size()) {
        endIndex = DisasmDataLines.size() - 1;
    }

    // Step 1: Collect all block start addresses
    std::set<DWORD_PTR> blockStartAddresses;
    std::map<DWORD_PTR, std::vector<DWORD_PTR>> successors;  // srcAddr -> list of target addresses
    std::map<DWORD_PTR, CFG_EDGE_TYPE> edgeTypes;  // (srcAddr << 32 | targetAddr) -> type

    // Entry point is always a block start
    blockStartAddresses.insert(funcStart);

    // Scan instructions in function to find branches
    for (DWORD_PTR i = startIndex; i <= endIndex && i < DisasmDataLines.size(); i++) {
        DWORD_PTR addr = GetAddressAtIndex(i);
        if (addr > funcEnd) break;

        const char* mnemonic = DisasmDataLines[i].GetMnemonic();

        // Check for branch instructions
        bool isCondJump = IsConditionalJump(mnemonic);
        bool isUncondJump = IsUnconditionalJump(mnemonic);
        bool isRet = IsReturnInstruction(mnemonic);

        if (isCondJump || isUncondJump) {
            // Find branch destination in DisasmCodeFlow
            for (size_t j = 0; j < DisasmCodeFlow.size(); j++) {
                if (DisasmCodeFlow[j].Current_Index == i) {
                    DWORD_PTR destAddr = DisasmCodeFlow[j].Branch_Destination;

                    // If destination is within function, add as block start
                    if (destAddr >= funcStart && destAddr <= funcEnd) {
                        blockStartAddresses.insert(destAddr);
                        successors[addr].push_back(destAddr);

                        // Mark edge type
                        if (isCondJump) {
                            edgeTypes[(addr << 16) | (destAddr & 0xFFFF)] = EDGE_CONDITIONAL_TRUE;
                        } else {
                            edgeTypes[(addr << 16) | (destAddr & 0xFFFF)] = EDGE_UNCONDITIONAL;
                        }
                    }
                    break;
                }
            }

            // For conditional jumps, fall-through is also a successor
            if (isCondJump && i + 1 <= endIndex) {
                DWORD_PTR nextAddr = GetAddressAtIndex(i + 1);
                if (nextAddr >= funcStart && nextAddr <= funcEnd) {
                    blockStartAddresses.insert(nextAddr);
                    successors[addr].push_back(nextAddr);
                    edgeTypes[(addr << 16) | (nextAddr & 0xFFFF)] = EDGE_CONDITIONAL_FALSE;
                }
            }
        }
    }

    // Step 2: Create basic blocks from sorted addresses
    std::vector<DWORD_PTR> sortedStarts(blockStartAddresses.begin(), blockStartAddresses.end());
    std::sort(sortedStarts.begin(), sortedStarts.end());

    for (size_t i = 0; i < sortedStarts.size(); i++) {
        CFG_BASIC_BLOCK block;
        memset(&block, 0, sizeof(block));

        block.BlockID = i;
        block.StartAddress = sortedStarts[i];
        block.StartIndex = FindIndexByAddress(sortedStarts[i]);

        // End address is just before next block start, or function end
        if (i + 1 < sortedStarts.size()) {
            // Find the instruction just before the next block
            DWORD_PTR nextBlockStart = sortedStarts[i + 1];
            DWORD_PTR nextBlockStartIndex = FindIndexByAddress(nextBlockStart);

            if (nextBlockStartIndex > 0 && nextBlockStartIndex > block.StartIndex) {
                block.EndIndex = nextBlockStartIndex - 1;
                block.EndAddress = GetAddressAtIndex(block.EndIndex);
            } else {
                block.EndIndex = block.StartIndex;
                block.EndAddress = block.StartAddress;
            }
        } else {
            block.EndIndex = endIndex;
            block.EndAddress = funcEnd;
        }

        block.IsEntryBlock = (block.StartAddress == funcStart);

        // Check if block ends with RET
        if (block.EndIndex < DisasmDataLines.size()) {
            const char* lastMnemonic = DisasmDataLines[block.EndIndex].GetMnemonic();
            block.IsExitBlock = IsReturnInstruction(lastMnemonic);
        }

        // Check if this block starts a known function (fFunctionInfo or CallTargets)
        block.FunctionLabel[0] = '\0';
        bool foundLabel = false;
        for (size_t fi = 0; fi < fFunctionInfo.size(); fi++) {
            if (fFunctionInfo[fi].FunctionStart == block.StartAddress) {
                if (fFunctionInfo[fi].FunctionName[0] != '\0')
                    strncpy(block.FunctionLabel, fFunctionInfo[fi].FunctionName, 63);
                else
                    if(LoadedPe64)
                        wsprintf(block.FunctionLabel, "Proc_%08X%08X", (DWORD)(block.StartAddress>>32), (DWORD)block.StartAddress);
                    else
                        wsprintf(block.FunctionLabel, "Proc_%08X", (DWORD)block.StartAddress);
                block.FunctionLabel[63] = '\0';
                foundLabel = true;
                break;
            }
        }
        if (!foundLabel && CallTargets.count(block.StartAddress)) {
            if(LoadedPe64)
                wsprintf(block.FunctionLabel, "Proc_%08X%08X", (DWORD)(block.StartAddress>>32), (DWORD)block.StartAddress);
            else
                wsprintf(block.FunctionLabel, "Proc_%08X", (DWORD)block.StartAddress);
        }

        outGraph->Blocks.push_back(block);
        outGraph->AddressToBlockIndex[block.StartAddress] = outGraph->Blocks.size() - 1;
        outGraph->BlockIDToIndex[block.BlockID] = outGraph->Blocks.size() - 1;
    }

    // Step 3: Create edges
    for (size_t i = 0; i < outGraph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = outGraph->Blocks[i];
        DWORD_PTR lastAddr = block.EndAddress;

        // Check if this address has explicit successors
        if (successors.count(lastAddr)) {
            for (DWORD_PTR target : successors[lastAddr]) {
                if (outGraph->AddressToBlockIndex.count(target)) {
                    CFG_EDGE edge;
                    edge.SourceBlockID = block.BlockID;
                    edge.TargetBlockID = outGraph->Blocks[outGraph->AddressToBlockIndex[target]].BlockID;
                    edge.IsBackEdge = false;

                    // Get edge type
                    DWORD_PTR key = (lastAddr << 16) | (target & 0xFFFF);
                    if (edgeTypes.count(key)) {
                        edge.Type = edgeTypes[key];
                    } else {
                        edge.Type = EDGE_UNCONDITIONAL;
                    }

                    outGraph->Edges.push_back(edge);
                }
            }
        } else if (!block.IsExitBlock) {
            // Implicit fall-through to next block
            if (i + 1 < outGraph->Blocks.size()) {
                // Check if last instruction is not an unconditional jump
                const char* lastMnemonic = DisasmDataLines[block.EndIndex].GetMnemonic();
                if (!IsUnconditionalJump(lastMnemonic) && !IsReturnInstruction(lastMnemonic)) {
                    CFG_EDGE edge;
                    edge.SourceBlockID = block.BlockID;
                    edge.TargetBlockID = outGraph->Blocks[i + 1].BlockID;
                    edge.Type = EDGE_UNCONDITIONAL;
                    edge.IsBackEdge = false;
                    outGraph->Edges.push_back(edge);
                }
            }
        }
    }

    // Add loc_ labels for jump target blocks that don't already have a function label
    std::set<DWORD_PTR> jumpTargetAddrs;
    for (auto& pair : successors) {
        for (DWORD_PTR target : pair.second) {
            jumpTargetAddrs.insert(target);
        }
    }
    for (size_t i = 0; i < outGraph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = outGraph->Blocks[i];
        if (block.FunctionLabel[0] == '\0' && !block.IsEntryBlock &&
            jumpTargetAddrs.count(block.StartAddress)) {
            if(LoadedPe64)
                wsprintf(block.FunctionLabel, "loc_%08X%08X", (DWORD)(block.StartAddress>>32), (DWORD)block.StartAddress);
            else
                wsprintf(block.FunctionLabel, "loc_%08X", (DWORD)block.StartAddress);
        }
    }

    return (outGraph->Blocks.size() > 0);
}

void ClearCFGGraph(CFG_GRAPH* graph)
{
    if (!graph) return;

    graph->Blocks.clear();
    graph->Edges.clear();
    graph->EdgeRoutes.clear();
    graph->EdgeRoutesDirty = true;
    graph->AddressToBlockIndex.clear();
    graph->BlockIDToIndex.clear();
    graph->FunctionStart = 0;
    graph->FunctionEnd = 0;
    graph->FunctionName[0] = '\0';
    graph->GraphWidth = 0;
    graph->GraphHeight = 0;
}

void FreeCFGGraph(CFG_GRAPH* graph)
{
    ClearCFGGraph(graph);
}

// ================================================================
// ====================  LAYOUT ALGORITHM  ========================
// ================================================================

void CalculateBlockDimensions(HDC hDC, CFG_GRAPH* graph)
{
    if (!graph || !hDC) return;

    HFONT hOldFont = NULL;
    if (g_hCFGFont) {
        hOldFont = (HFONT)SelectObject(hDC, g_hCFGFont);
    }

    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = graph->Blocks[i];

        // Calculate width based on longest line
        int maxWidth = CFG_MIN_BLOCK_WIDTH;
        int numLines = 0;

        for (DWORD_PTR idx = block.StartIndex; idx <= block.EndIndex && idx < DisasmDataLines.size(); idx++) {
            char line[512];
            char* comment = DisasmDataLines[idx].GetComments();
            if (comment && comment[0] != '\0') {
                wsprintf(line, "%s  %s  %s",
                         DisasmDataLines[idx].GetAddress(),
                         DisasmDataLines[idx].GetMnemonic(),
                         comment);
            } else {
                wsprintf(line, "%s  %s",
                         DisasmDataLines[idx].GetAddress(),
                         DisasmDataLines[idx].GetMnemonic());
            }

            SIZE textSize;
            GetTextExtentPoint32(hDC, line, lstrlen(line), &textSize);

            if (textSize.cx + 2 * CFG_BLOCK_PADDING > maxWidth) {
                maxWidth = textSize.cx + 2 * CFG_BLOCK_PADDING;
            }
            numLines++;
        }

        // Account for function label header
        if (block.FunctionLabel[0] != '\0') {
            SIZE labelSize;
            GetTextExtentPoint32(hDC, block.FunctionLabel, lstrlen(block.FunctionLabel), &labelSize);
            if (labelSize.cx + 2 * CFG_BLOCK_PADDING > maxWidth) {
                maxWidth = labelSize.cx + 2 * CFG_BLOCK_PADDING;
            }
            numLines++;  // Extra line for the header
        }

        block.Width = maxWidth;
        block.Height = numLines * CFG_LINE_HEIGHT + 2 * CFG_BLOCK_PADDING;

        // Minimum height
        if (block.Height < 30) block.Height = 30;
    }

    if (hOldFont) {
        SelectObject(hDC, hOldFont);
    }
}

void LayoutCFG(CFG_GRAPH* graph)
{
    if (!graph || graph->Blocks.empty()) return;

    // Step 1: Assign layers using BFS from entry block
    std::map<DWORD_PTR, int> blockLayer;
    std::queue<DWORD_PTR> bfsQueue;

    // Find entry block
    DWORD_PTR entryBlockID = 0;
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        if (graph->Blocks[i].IsEntryBlock) {
            entryBlockID = graph->Blocks[i].BlockID;
            break;
        }
    }

    blockLayer[entryBlockID] = 0;
    bfsQueue.push(entryBlockID);

    while (!bfsQueue.empty()) {
        DWORD_PTR current = bfsQueue.front();
        bfsQueue.pop();
        int currentLayer = blockLayer[current];

        // Find all successor blocks
        for (size_t i = 0; i < graph->Edges.size(); i++) {
            if (graph->Edges[i].SourceBlockID == current) {
                DWORD_PTR target = graph->Edges[i].TargetBlockID;

                if (blockLayer.find(target) == blockLayer.end()) {
                    blockLayer[target] = currentLayer + 1;
                    bfsQueue.push(target);
                } else if (blockLayer[target] <= currentLayer) {
                    // This is a back edge (loop)
                    graph->Edges[i].IsBackEdge = true;
                }
            }
        }
    }

    // Assign layer to any blocks not reached (disconnected)
    int maxLayer = 0;
    for (auto& pair : blockLayer) {
        if (pair.second > maxLayer) maxLayer = pair.second;
    }
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        if (blockLayer.find(graph->Blocks[i].BlockID) == blockLayer.end()) {
            blockLayer[graph->Blocks[i].BlockID] = maxLayer + 1;
        }
        graph->Blocks[i].Layer = blockLayer[graph->Blocks[i].BlockID];
    }

    // Recalculate max layer
    maxLayer = 0;
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        if (graph->Blocks[i].Layer > maxLayer) {
            maxLayer = graph->Blocks[i].Layer;
        }
    }

    // Step 2: Group blocks by layer
    std::map<int, std::vector<size_t>> layerBlocks;
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        layerBlocks[graph->Blocks[i].Layer].push_back(i);
    }

    // Step 3: Calculate block dimensions
    HDC hDC = GetDC(NULL);
    CalculateBlockDimensions(hDC, graph);
    ReleaseDC(NULL, hDC);

    // Step 4: Assign coordinates
    int currentY = CFG_BLOCK_V_SPACING;
    int maxLayerWidth = 0;
    std::map<int, int> layerWidths;

    for (int layer = 0; layer <= maxLayer; layer++) {
        if (layerBlocks.find(layer) == layerBlocks.end()) continue;

        std::vector<size_t>& blocksInLayer = layerBlocks[layer];

        // Calculate total width and max height of this layer
        int layerWidth = 0;
        int maxHeight = 0;

        for (size_t idx : blocksInLayer) {
            CFG_BASIC_BLOCK& block = graph->Blocks[idx];
            layerWidth += block.Width + CFG_BLOCK_H_SPACING;
            if (block.Height > maxHeight) maxHeight = block.Height;
        }
        layerWidth -= CFG_BLOCK_H_SPACING;  // Remove trailing spacing
        layerWidths[layer] = layerWidth;

        if (layerWidth > maxLayerWidth) maxLayerWidth = layerWidth;

        // Assign X coordinates (left-aligned initially, will center later)
        int currentX = CFG_BLOCK_H_SPACING;
        int posInLayer = 0;

        for (size_t idx : blocksInLayer) {
            CFG_BASIC_BLOCK& block = graph->Blocks[idx];
            block.X = currentX;
            block.Y = currentY;
            block.PositionInLayer = posInLayer++;
            currentX += block.Width + CFG_BLOCK_H_SPACING;
        }

        currentY += maxHeight + CFG_BLOCK_V_SPACING;
    }

    // Step 5: Center each layer horizontally (diamond/tree shape)
    for (int layer = 0; layer <= maxLayer; layer++) {
        if (layerBlocks.find(layer) == layerBlocks.end()) continue;

        int offset = (maxLayerWidth - layerWidths[layer]) / 2;
        if (offset > 0) {
            for (size_t idx : layerBlocks[layer]) {
                graph->Blocks[idx].X += offset;
            }
        }
    }

    graph->GraphWidth = maxLayerWidth + 2 * CFG_BLOCK_H_SPACING;
    graph->GraphHeight = currentY;
}

// ================================================================
// ====================  VIEW STATE  ==============================
// ================================================================

void InitCFGViewState(CFG_VIEW_STATE* state)
{
    if (!state) return;

    state->ZoomLevel = 1.0;
    state->PanOffsetX = 0;
    state->PanOffsetY = 0;
    state->IsPanning = false;
    state->IsDraggingBlock = false;
    state->DragStartPoint.x = 0;
    state->DragStartPoint.y = 0;
    state->DragStartPanX = 0;
    state->DragStartPanY = 0;
    state->DragBlockStartX = 0;
    state->DragBlockStartY = 0;
    state->SelectedBlockID = (DWORD_PTR)-1;
    state->DraggingBlockID = (DWORD_PTR)-1;
}

void CenterGraphInView(HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* state)
{
    if (!hWnd || !graph || !state) return;

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);

    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // Calculate zoom to fit
    double zoomX = (double)clientWidth / (double)graph->GraphWidth;
    double zoomY = (double)clientHeight / (double)graph->GraphHeight;
    double fitZoom = min(zoomX, zoomY) * 0.9;  // 90% to leave margin

    // Clamp zoom
    if (fitZoom < CFG_MIN_ZOOM) fitZoom = CFG_MIN_ZOOM;
    if (fitZoom > CFG_MAX_ZOOM) fitZoom = CFG_MAX_ZOOM;

    state->ZoomLevel = fitZoom;

    // Center the graph
    double scaledWidth = graph->GraphWidth * fitZoom;
    double scaledHeight = graph->GraphHeight * fitZoom;

    state->PanOffsetX = (clientWidth - scaledWidth) / 2.0;
    state->PanOffsetY = (clientHeight - scaledHeight) / 2.0;
}

// Focus on the entry block at a readable zoom level.
// Falls back to CenterGraphInView if no entry block exists.
static void FocusOnEntryBlock(HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* state)
{
    if (!hWnd || !graph || !state) return;

    // Find entry block
    int entryIdx = -1;
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        if (graph->Blocks[i].IsEntryBlock) {
            entryIdx = (int)i;
            break;
        }
    }

    if (entryIdx < 0) {
        // No entry block — fall back to fit-all view
        CenterGraphInView(hWnd, graph, state);
        return;
    }

    CFG_BASIC_BLOCK& entry = graph->Blocks[entryIdx];

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // Choose zoom: try 1.0, but shrink if the entry block doesn't fit
    double zoom = 1.0;
    if (entry.Width * zoom > clientWidth * 0.9)
        zoom = (clientWidth * 0.9) / entry.Width;
    if (entry.Height * zoom > clientHeight * 0.9)
        zoom = (clientHeight * 0.9) / entry.Height;
    if (zoom < CFG_MIN_ZOOM) zoom = CFG_MIN_ZOOM;
    if (zoom > CFG_MAX_ZOOM) zoom = CFG_MAX_ZOOM;

    state->ZoomLevel = zoom;

    // Center the entry block in the viewport
    double blockCenterX = entry.X + entry.Width / 2.0;
    double blockCenterY = entry.Y + entry.Height / 2.0;
    state->PanOffsetX = clientWidth / 2.0 - blockCenterX * zoom;
    state->PanOffsetY = clientHeight / 2.0 - blockCenterY * zoom;
}

POINT ScreenToGraph(CFG_VIEW_STATE* viewState, POINT screenPt)
{
    POINT graphPt;
    graphPt.x = (LONG)((screenPt.x - viewState->PanOffsetX) / viewState->ZoomLevel);
    graphPt.y = (LONG)((screenPt.y - viewState->PanOffsetY) / viewState->ZoomLevel);
    return graphPt;
}

POINT GraphToScreen(CFG_VIEW_STATE* viewState, POINT graphPt)
{
    POINT screenPt;
    screenPt.x = (LONG)(graphPt.x * viewState->ZoomLevel + viewState->PanOffsetX);
    screenPt.y = (LONG)(graphPt.y * viewState->ZoomLevel + viewState->PanOffsetY);
    return screenPt;
}

void ZoomAtPoint(CFG_VIEW_STATE* viewState, POINT screenPt, double zoomFactor)
{
    double newZoom = viewState->ZoomLevel * zoomFactor;

    // Clamp zoom level
    if (newZoom < CFG_MIN_ZOOM) newZoom = CFG_MIN_ZOOM;
    if (newZoom > CFG_MAX_ZOOM) newZoom = CFG_MAX_ZOOM;

    if (newZoom != viewState->ZoomLevel) {
        // Zoom centered on mouse position
        POINT graphPt = ScreenToGraph(viewState, screenPt);
        viewState->ZoomLevel = newZoom;
        viewState->PanOffsetX = screenPt.x - graphPt.x * newZoom;
        viewState->PanOffsetY = screenPt.y - graphPt.y * newZoom;
    }
}

DWORD_PTR HitTestBlocks(CFG_GRAPH* graph, POINT graphPt)
{
    if (!graph) return (DWORD_PTR)-1;

    // Test in reverse order so topmost blocks are hit first
    for (int i = (int)graph->Blocks.size() - 1; i >= 0; i--) {
        CFG_BASIC_BLOCK& block = graph->Blocks[i];

        if (graphPt.x >= block.X && graphPt.x <= block.X + block.Width &&
            graphPt.y >= block.Y && graphPt.y <= block.Y + block.Height) {
            return block.BlockID;
        }
    }

    return (DWORD_PTR)-1;
}

// ================================================================
// ====================  RENDERING  ===============================
// ================================================================

void DrawArrowhead(HDC hDC, int fromX, int fromY, int toX, int toY, COLORREF color)
{
    double angle = atan2((double)(toY - fromY), (double)(toX - fromX));

    POINT arrowPts[3];
    arrowPts[0].x = toX;
    arrowPts[0].y = toY;
    arrowPts[1].x = toX - (int)(CFG_ARROW_SIZE * cos(angle - 0.4));
    arrowPts[1].y = toY - (int)(CFG_ARROW_SIZE * sin(angle - 0.4));
    arrowPts[2].x = toX - (int)(CFG_ARROW_SIZE * cos(angle + 0.4));
    arrowPts[2].y = toY - (int)(CFG_ARROW_SIZE * sin(angle + 0.4));

    HBRUSH hBrush = CreateSolidBrush(color);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, hBrush);
    HPEN hPen = CreatePen(PS_SOLID, 1, color);
    HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

    Polygon(hDC, arrowPts, 3);

    SelectObject(hDC, hOldPen);
    SelectObject(hDC, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
}

// Fast arrowhead drawing using pre-cached GDI objects (pen index 0-4)
static void DrawArrowheadCached(HDC hDC, int fromX, int fromY, int toX, int toY, int penIdx)
{
    double angle = atan2((double)(toY - fromY), (double)(toX - fromX));

    POINT arrowPts[3];
    arrowPts[0].x = toX;
    arrowPts[0].y = toY;
    arrowPts[1].x = toX - (int)(CFG_ARROW_SIZE * cos(angle - 0.4));
    arrowPts[1].y = toY - (int)(CFG_ARROW_SIZE * sin(angle - 0.4));
    arrowPts[2].x = toX - (int)(CFG_ARROW_SIZE * cos(angle + 0.4));
    arrowPts[2].y = toY - (int)(CFG_ARROW_SIZE * sin(angle + 0.4));

    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, g_ArrowBrushes[penIdx]);
    HPEN hOldPen = (HPEN)SelectObject(hDC, g_ArrowPens[penIdx]);

    Polygon(hDC, arrowPts, 3);

    SelectObject(hDC, hOldPen);
    SelectObject(hDC, hOldBrush);
}

// Compute a spread X position for a port along a block edge.
// portIndex is 0-based, totalPorts is the count of ports on that edge.
static int ComputePortX(CFG_BASIC_BLOCK& block, int portIndex, int totalPorts)
{
    if (totalPorts <= 1)
        return block.X + block.Width / 2;

    const int margin = 12;
    int usable = block.Width - 2 * margin;
    if (usable < 0) usable = 0;
    return block.X + margin + (portIndex * usable) / (totalPorts - 1);
}

// ================================================================
// ====================  EDGE ROUTE CACHING  ======================
// ================================================================

void ComputeEdgeRoutes(CFG_GRAPH* graph)
{
    if (!graph) return;

    graph->EdgeRoutes.resize(graph->Edges.size());
    graph->EdgeRoutesDirty = false;

    // ── Pre-pass: build per-block port assignments ──────────────────
    std::map<DWORD_PTR, std::vector<size_t>> outgoingEdges;
    std::map<DWORD_PTR, std::vector<size_t>> incomingEdges;
    std::set<size_t> backEdgeSet;

    for (size_t i = 0; i < graph->Edges.size(); i++) {
        CFG_EDGE& edge = graph->Edges[i];

        if (graph->BlockIDToIndex.find(edge.SourceBlockID) == graph->BlockIDToIndex.end() ||
            graph->BlockIDToIndex.find(edge.TargetBlockID) == graph->BlockIDToIndex.end()) {
            graph->EdgeRoutes[i].PointCount = 0;
            continue;
        }

        if (edge.IsBackEdge) {
            backEdgeSet.insert(i);
        } else {
            outgoingEdges[edge.SourceBlockID].push_back(i);
        }
        incomingEdges[edge.TargetBlockID].push_back(i);
    }

    // Sort outgoing edges by target block X (left-to-right) to reduce crossings
    for (auto& pair : outgoingEdges) {
        std::sort(pair.second.begin(), pair.second.end(),
            [&](size_t a, size_t b) {
                CFG_BASIC_BLOCK& da = graph->Blocks[graph->BlockIDToIndex[graph->Edges[a].TargetBlockID]];
                CFG_BASIC_BLOCK& db = graph->Blocks[graph->BlockIDToIndex[graph->Edges[b].TargetBlockID]];
                return da.X < db.X;
            });
    }

    // Sort incoming edges by source block X (left-to-right) to reduce crossings
    for (auto& pair : incomingEdges) {
        std::sort(pair.second.begin(), pair.second.end(),
            [&](size_t a, size_t b) {
                CFG_BASIC_BLOCK& sa = graph->Blocks[graph->BlockIDToIndex[graph->Edges[a].SourceBlockID]];
                CFG_BASIC_BLOCK& sb = graph->Blocks[graph->BlockIDToIndex[graph->Edges[b].SourceBlockID]];
                return sa.X < sb.X;
            });
    }

    // Build port index lookups
    std::map<size_t, int> outPortIndex, inPortIndex;
    std::map<size_t, int> outPortTotal, inPortTotal;

    for (auto& pair : outgoingEdges) {
        int total = (int)pair.second.size();
        for (int k = 0; k < total; k++) {
            outPortIndex[pair.second[k]] = k;
            outPortTotal[pair.second[k]] = total;
        }
    }
    for (auto& pair : incomingEdges) {
        int total = (int)pair.second.size();
        for (int k = 0; k < total; k++) {
            inPortIndex[pair.second[k]] = k;
            inPortTotal[pair.second[k]] = total;
        }
    }

    // Collision tracking
    struct UsedHSeg { int y; int minX; int maxX; };
    std::vector<UsedHSeg> usedHorizontalSegs;
    struct UsedVSeg { int x; int minY; int maxY; };
    std::vector<UsedVSeg> usedVerticalSegs;

    // ── Compute route for each edge ─────────────────────────────────
    for (size_t i = 0; i < graph->Edges.size(); i++) {
        CFG_EDGE& edge = graph->Edges[i];
        CFG_EDGE_ROUTE& route = graph->EdgeRoutes[i];

        if (graph->BlockIDToIndex.find(edge.SourceBlockID) == graph->BlockIDToIndex.end() ||
            graph->BlockIDToIndex.find(edge.TargetBlockID) == graph->BlockIDToIndex.end()) {
            route.PointCount = 0;
            continue;
        }

        CFG_BASIC_BLOCK& srcBlock = graph->Blocks[graph->BlockIDToIndex[edge.SourceBlockID]];
        CFG_BASIC_BLOCK& dstBlock = graph->Blocks[graph->BlockIDToIndex[edge.TargetBlockID]];

        // Calculate start and end points
        int startX, startY, endX, endY;

        if (edge.IsBackEdge) {
            startX = srcBlock.X;
            startY = srcBlock.Y + srcBlock.Height / 2;
            endX = ComputePortX(dstBlock, inPortIndex[i], inPortTotal[i]);
            endY = dstBlock.Y;
        } else {
            startX = ComputePortX(srcBlock, outPortIndex[i], outPortTotal[i]);
            endX = ComputePortX(dstBlock, inPortIndex[i], inPortTotal[i]);

            if (srcBlock.Y >= dstBlock.Y + dstBlock.Height) {
                startY = srcBlock.Y;
                endY = dstBlock.Y + dstBlock.Height;
            } else {
                startY = srcBlock.Y + srcBlock.Height;
                endY = dstBlock.Y;
            }
        }

        // Store port offsets relative to block position (preserved during drag)
        route.StartPortOffsetX = startX - srcBlock.X;
        route.EndPortOffsetX = endX - dstBlock.X;

        if (edge.IsBackEdge) {
            // 4-segment orthogonal route for back edges

            int backEdgeIndex = 0;
            for (size_t be : backEdgeSet) {
                if (be >= i) break;
                backEdgeIndex++;
            }

            int channelX = min(srcBlock.X, endX) - 30 - (backEdgeIndex * 15);
            int aboveY = dstBlock.Y - CFG_BLOCK_V_SPACING / 2;

            // Obstacle avoidance
            for (size_t bi = 0; bi < graph->Blocks.size(); bi++) {
                CFG_BASIC_BLOCK& blk = graph->Blocks[bi];
                if (blk.BlockID == edge.SourceBlockID || blk.BlockID == edge.TargetBlockID)
                    continue;

                if (channelX >= blk.X - 5 && channelX <= blk.X + blk.Width + 5) {
                    int segMinY = min(startY, aboveY);
                    int segMaxY = max(startY, aboveY);
                    if (segMaxY >= blk.Y && segMinY <= blk.Y + blk.Height) {
                        channelX = blk.X - 20;
                    }
                }

                int hMinX = min(channelX, endX);
                int hMaxX = max(channelX, endX);
                if (aboveY >= blk.Y - 5 && aboveY <= blk.Y + blk.Height + 5) {
                    if (hMaxX >= blk.X && hMinX <= blk.X + blk.Width) {
                        aboveY = blk.Y - 15;
                    }
                }
            }

            // Stagger horizontal segment
            {
                int hMinX = min(channelX, endX);
                int hMaxX = max(channelX, endX);
                for (int iter = 0; iter < 20; iter++) {
                    bool collision = false;
                    for (size_t s = 0; s < usedHorizontalSegs.size(); s++) {
                        if (abs(usedHorizontalSegs[s].y - aboveY) < 6 &&
                            hMaxX >= usedHorizontalSegs[s].minX && hMinX <= usedHorizontalSegs[s].maxX) {
                            aboveY -= 8;
                            collision = true;
                            break;
                        }
                    }
                    if (!collision) break;
                }
                UsedHSeg seg = { aboveY, hMinX, hMaxX };
                usedHorizontalSegs.push_back(seg);
            }

            // Store route points
            route.Points[0].x = startX;   route.Points[0].y = startY;
            route.Points[1].x = channelX; route.Points[1].y = startY;
            route.Points[2].x = channelX; route.Points[2].y = aboveY;
            route.Points[3].x = endX;     route.Points[3].y = aboveY;
            route.Points[4].x = endX;     route.Points[4].y = endY;
            route.PointCount = 5;
            route.ArrowFromX = endX;  route.ArrowFromY = aboveY;
            route.ArrowToX = endX;    route.ArrowToY = endY;
            route.LabelX = channelX - 5;
            route.LabelY = (startY + aboveY) / 2;

            // Record segments for collision avoidance
            {
                int vMinY = min(startY, aboveY);
                int vMaxY = max(startY, aboveY);
                usedVerticalSegs.push_back({ channelX, vMinY, vMaxY });
                vMinY = min(aboveY, endY);
                vMaxY = max(aboveY, endY);
                usedVerticalSegs.push_back({ endX, vMinY, vMaxY });
            }
        } else {
            // 3-segment orthogonal route for normal edges
            int midY = (startY + endY) / 2;

            // Obstacle avoidance
            for (size_t bi = 0; bi < graph->Blocks.size(); bi++) {
                CFG_BASIC_BLOCK& blk = graph->Blocks[bi];
                if (blk.BlockID == edge.SourceBlockID || blk.BlockID == edge.TargetBlockID)
                    continue;

                int hMinX = min(startX, endX);
                int hMaxX = max(startX, endX);
                if (hMaxX >= blk.X && hMinX <= blk.X + blk.Width) {
                    if (midY >= blk.Y - 5 && midY <= blk.Y + blk.Height + 5) {
                        midY = blk.Y - 15;
                    }
                }
            }

            // Clamp midY
            if (startY < endY) {
                if (midY < startY + 5) midY = startY + 5;
                if (midY > endY - 5) midY = endY - 5;
            } else if (startY > endY) {
                if (midY > startY - 5) midY = startY - 5;
                if (midY < endY + 5) midY = endY + 5;
            }

            // Stagger horizontal segment
            {
                int hMinX = min(startX, endX);
                int hMaxX = max(startX, endX);
                for (int iter = 0; iter < 20; iter++) {
                    bool collision = false;
                    for (size_t s = 0; s < usedHorizontalSegs.size(); s++) {
                        if (abs(usedHorizontalSegs[s].y - midY) < 6 &&
                            hMaxX >= usedHorizontalSegs[s].minX && hMinX <= usedHorizontalSegs[s].maxX) {
                            midY += 8;
                            collision = true;
                            break;
                        }
                    }
                    if (!collision) break;
                }
                UsedHSeg seg = { midY, hMinX, hMaxX };
                usedHorizontalSegs.push_back(seg);
            }

            // Compute vertical jog offsets
            int drawStartX = startX;
            int drawEndX = endX;
            {
                int vMinY = min(startY, midY);
                int vMaxY = max(startY, midY);
                if (vMaxY - vMinY > 5) {
                    for (int iter = 0; iter < 20; iter++) {
                        bool collision = false;
                        for (size_t s = 0; s < usedVerticalSegs.size(); s++) {
                            if (abs(usedVerticalSegs[s].x - drawStartX) < 6 &&
                                vMaxY >= usedVerticalSegs[s].minY && vMinY <= usedVerticalSegs[s].maxY) {
                                drawStartX += 8;
                                collision = true;
                                break;
                            }
                        }
                        if (!collision) break;
                    }
                }
                usedVerticalSegs.push_back({ drawStartX, vMinY, vMaxY });

                vMinY = min(midY, endY);
                vMaxY = max(midY, endY);
                if (vMaxY - vMinY > 5) {
                    for (int iter = 0; iter < 20; iter++) {
                        bool collision = false;
                        for (size_t s = 0; s < usedVerticalSegs.size(); s++) {
                            if (abs(usedVerticalSegs[s].x - drawEndX) < 6 &&
                                vMaxY >= usedVerticalSegs[s].minY && vMinY <= usedVerticalSegs[s].maxY) {
                                drawEndX += 8;
                                collision = true;
                                break;
                            }
                        }
                        if (!collision) break;
                    }
                }
                usedVerticalSegs.push_back({ drawEndX, vMinY, vMaxY });
            }

            // Store route points
            int pc = 0;
            route.Points[pc].x = startX; route.Points[pc].y = startY; pc++;
            if (drawStartX != startX) {
                route.Points[pc].x = drawStartX; route.Points[pc].y = startY; pc++;
            }
            route.Points[pc].x = drawStartX; route.Points[pc].y = midY; pc++;
            route.Points[pc].x = drawEndX;   route.Points[pc].y = midY; pc++;

            if (drawEndX != endX) {
                int jogY = (midY < endY) ? endY - 12 : endY + 12;
                route.Points[pc].x = drawEndX; route.Points[pc].y = jogY; pc++;
                route.Points[pc].x = endX;     route.Points[pc].y = jogY; pc++;
                route.Points[pc].x = endX;     route.Points[pc].y = endY; pc++;
                route.ArrowFromX = endX; route.ArrowFromY = jogY;
            } else {
                route.Points[pc].x = endX; route.Points[pc].y = endY; pc++;
                route.ArrowFromX = endX; route.ArrowFromY = midY;
            }
            route.PointCount = pc;
            route.ArrowToX = endX; route.ArrowToY = endY;
            route.LabelX = startX + 4;
            route.LabelY = (startY < endY) ? startY + 8 : startY - 16;
        }
    }
}

// ================================================================
// ====================  BLOCK COLOR CACHING  =====================
// ================================================================

void ComputeBlockColors(CFG_GRAPH* graph)
{
    if (!graph) return;

    COLORREF blockBg      = g_DarkMode ? RGB(50, 50, 50) : RGB(255, 255, 245);
    COLORREF trueBranchBg = g_DarkMode ? RGB(40, 70, 40) : RGB(220, 255, 220);
    COLORREF falseBranchBg= g_DarkMode ? RGB(70, 40, 40) : RGB(255, 220, 220);
    COLORREF exitBlockBg  = g_DarkMode ? RGB(40, 50, 70) : RGB(210, 230, 255);

    // Single pass over edges to collect incoming info per block
    std::map<DWORD_PTR, int> inCount;
    std::map<DWORD_PTR, CFG_EDGE_TYPE> inType;
    for (size_t e = 0; e < graph->Edges.size(); e++) {
        DWORD_PTR target = graph->Edges[e].TargetBlockID;
        inCount[target]++;
        inType[target] = graph->Edges[e].Type;
    }

    // Single pass over blocks to assign colors
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = graph->Blocks[i];
        COLORREF bgColor = blockBg;

        if (block.IsExitBlock) {
            bgColor = exitBlockBg;
        } else if (inCount[block.BlockID] == 1) {
            if (inType[block.BlockID] == EDGE_CONDITIONAL_TRUE)
                bgColor = trueBranchBg;
            else if (inType[block.BlockID] == EDGE_CONDITIONAL_FALSE)
                bgColor = falseBranchBg;
        }
        block.CachedBgColor = bgColor;
    }
}

// Lightweight route update for edges connected to a single block (used during drag).
// Preserves the original port spread offsets so T/F edges stay separated.
static void UpdateRoutesForDraggedBlock(CFG_GRAPH* graph, DWORD_PTR blockID)
{
    if (!graph || graph->EdgeRoutes.size() != graph->Edges.size()) return;

    for (size_t i = 0; i < graph->Edges.size(); i++) {
        CFG_EDGE& edge = graph->Edges[i];
        if (edge.SourceBlockID != blockID && edge.TargetBlockID != blockID) continue;

        CFG_EDGE_ROUTE& route = graph->EdgeRoutes[i];

        if (graph->BlockIDToIndex.find(edge.SourceBlockID) == graph->BlockIDToIndex.end() ||
            graph->BlockIDToIndex.find(edge.TargetBlockID) == graph->BlockIDToIndex.end()) {
            route.PointCount = 0;
            continue;
        }

        CFG_BASIC_BLOCK& srcBlock = graph->Blocks[graph->BlockIDToIndex[edge.SourceBlockID]];
        CFG_BASIC_BLOCK& dstBlock = graph->Blocks[graph->BlockIDToIndex[edge.TargetBlockID]];

        // Recompute start/end using the saved port offsets (preserves T/F spread)
        int startX = srcBlock.X + route.StartPortOffsetX;
        int startY, endY;
        int endX = dstBlock.X + route.EndPortOffsetX;

        if (edge.IsBackEdge) {
            startY = srcBlock.Y + srcBlock.Height / 2;
            endY = dstBlock.Y;

            int channelX = min(srcBlock.X, endX) - 30;
            int aboveY = dstBlock.Y - CFG_BLOCK_V_SPACING / 2;

            route.Points[0].x = startX;   route.Points[0].y = startY;
            route.Points[1].x = channelX; route.Points[1].y = startY;
            route.Points[2].x = channelX; route.Points[2].y = aboveY;
            route.Points[3].x = endX;     route.Points[3].y = aboveY;
            route.Points[4].x = endX;     route.Points[4].y = endY;
            route.PointCount = 5;
            route.ArrowFromX = endX;  route.ArrowFromY = aboveY;
            route.ArrowToX = endX;    route.ArrowToY = endY;
            route.LabelX = channelX - 5;
            route.LabelY = (startY + aboveY) / 2;
        } else {
            if (srcBlock.Y >= dstBlock.Y + dstBlock.Height) {
                startY = srcBlock.Y;
                endY = dstBlock.Y + dstBlock.Height;
            } else {
                startY = srcBlock.Y + srcBlock.Height;
                endY = dstBlock.Y;
            }

            int midY = (startY + endY) / 2;

            route.Points[0].x = startX; route.Points[0].y = startY;
            route.Points[1].x = startX; route.Points[1].y = midY;
            route.Points[2].x = endX;   route.Points[2].y = midY;
            route.Points[3].x = endX;   route.Points[3].y = endY;
            route.PointCount = 4;
            route.ArrowFromX = endX; route.ArrowFromY = midY;
            route.ArrowToX = endX;   route.ArrowToY = endY;
            route.LabelX = startX + 4;
            route.LabelY = (startY < endY) ? startY + 8 : startY - 16;
        }
    }
}

// ================================================================
// ====================  EDGE & BLOCK RENDERING  ==================
// ================================================================

// Map edge type + back-edge flag to pen cache index (0-4)
static int GetEdgePenIndex(const CFG_EDGE& edge)
{
    if (edge.IsBackEdge) return 3;
    switch (edge.Type) {
        case EDGE_CONDITIONAL_TRUE:  return 1;
        case EDGE_CONDITIONAL_FALSE: return 2;
        case EDGE_CALL:              return 4;
        default:                     return 0;
    }
}

void RenderEdges(HDC hDC, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState, RECT* visibleRect)
{
    if (!graph || !viewState) return;

    // Ensure routes are computed
    if (graph->EdgeRoutesDirty || graph->EdgeRoutes.size() != graph->Edges.size()) {
        ComputeEdgeRoutes(graph);
    }

    for (size_t i = 0; i < graph->Edges.size(); i++) {
        CFG_EDGE& edge = graph->Edges[i];
        CFG_EDGE_ROUTE& route = graph->EdgeRoutes[i];

        if (route.PointCount < 2) continue;

        // Viewport culling: compute bounding box of route polyline
        if (visibleRect) {
            int minRX = route.Points[0].x, maxRX = route.Points[0].x;
            int minRY = route.Points[0].y, maxRY = route.Points[0].y;
            for (int p = 1; p < route.PointCount; p++) {
                if (route.Points[p].x < minRX) minRX = route.Points[p].x;
                if (route.Points[p].x > maxRX) maxRX = route.Points[p].x;
                if (route.Points[p].y < minRY) minRY = route.Points[p].y;
                if (route.Points[p].y > maxRY) maxRY = route.Points[p].y;
            }
            RECT routeRect = { minRX - CFG_ARROW_SIZE, minRY - CFG_ARROW_SIZE,
                               maxRX + CFG_ARROW_SIZE, maxRY + CFG_ARROW_SIZE };
            RECT dummy;
            if (!IntersectRect(&dummy, &routeRect, visibleRect)) continue;
        }

        int penIdx = GetEdgePenIndex(edge);

        // Draw polyline using cached pen
        HPEN hOldPen = (HPEN)SelectObject(hDC, g_EdgePens[penIdx]);
        Polyline(hDC, route.Points, route.PointCount);
        SelectObject(hDC, hOldPen);

        // Draw arrowhead using cached brush/pen
        DrawArrowheadCached(hDC, route.ArrowFromX, route.ArrowFromY,
                            route.ArrowToX, route.ArrowToY, penIdx);

        // Draw T/F/C label
        if (edge.Type == EDGE_CONDITIONAL_TRUE || edge.Type == EDGE_CONDITIONAL_FALSE || edge.Type == EDGE_CALL) {
            const char* label = (edge.Type == EDGE_CONDITIONAL_TRUE) ? "T" :
                                (edge.Type == EDGE_CONDITIONAL_FALSE) ? "F" : "C";
            SetBkMode(hDC, TRANSPARENT);
            SetTextColor(hDC, g_EdgeColors[penIdx]);
            TextOut(hDC, route.LabelX, route.LabelY, label, 1);
        }
    }
}

void RenderBlocks(HDC hDC, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState, RECT* visibleRect)
{
    if (!graph || !viewState) return;

    // Colors based on dark mode
    COLORREF blockBorder = g_DarkMode ? RGB(100, 100, 100) : RGB(0, 0, 0);
    COLORREF selectedBorder = RGB(0, 120, 215);
    COLORREF textColor = g_DarkMode ? g_DarkTextColor : RGB(0, 0, 0);
    COLORREF addrColor = g_DarkMode ? RGB(130, 130, 130) : RGB(100, 100, 100);

    bool drawText = true;

    HFONT hOldFont = NULL;
    if (g_hCFGFont) {
        hOldFont = (HFONT)SelectObject(hDC, g_hCFGFont);
    }

    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = graph->Blocks[i];

        RECT blockRect = {block.X, block.Y, block.X + block.Width, block.Y + block.Height};

        // Viewport culling
        if (visibleRect) {
            RECT dummy;
            if (!IntersectRect(&dummy, &blockRect, visibleRect)) continue;
        }

        // Use pre-computed background color
        HBRUSH hBgBrush = CreateSolidBrush(block.CachedBgColor);
        FillRect(hDC, &blockRect, hBgBrush);
        DeleteObject(hBgBrush);

        // Draw border
        bool isSelected = (block.BlockID == viewState->SelectedBlockID);
        HPEN hPen = CreatePen(PS_SOLID, isSelected ? 3 : 1, isSelected ? selectedBorder : blockBorder);
        HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);
        HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, hNullBrush);

        Rectangle(hDC, blockRect.left, blockRect.top, blockRect.right, blockRect.bottom);

        SelectObject(hDC, hOldBrush);
        SelectObject(hDC, hOldPen);
        DeleteObject(hPen);

        // Skip text rendering at low zoom (LOD optimization)
        if (!drawText) continue;

        // Draw instructions inside block
        SetBkMode(hDC, TRANSPARENT);
        int y = block.Y + CFG_BLOCK_PADDING;

        // Draw function label header if present
        if (block.FunctionLabel[0] != '\0') {
            COLORREF headerColor = g_DarkMode ? RGB(100, 160, 255) : RGB(0, 50, 160);
            SetTextColor(hDC, headerColor);
            TextOut(hDC, block.X + CFG_BLOCK_PADDING, y, block.FunctionLabel, lstrlen(block.FunctionLabel));
            y += CFG_LINE_HEIGHT;

            // Draw separator line below header
            HPEN hSepPen = CreatePen(PS_SOLID, 1, g_DarkMode ? RGB(80, 80, 80) : RGB(180, 180, 180));
            HPEN hOldSepPen = (HPEN)SelectObject(hDC, hSepPen);
            MoveToEx(hDC, block.X + 2, y - 2, NULL);
            LineTo(hDC, block.X + block.Width - 2, y - 2);
            SelectObject(hDC, hOldSepPen);
            DeleteObject(hSepPen);
        }

        for (DWORD_PTR idx = block.StartIndex; idx <= block.EndIndex && idx < DisasmDataLines.size(); idx++) {
            char* addrText = DisasmDataLines[idx].GetAddress();
            char* asmText = DisasmDataLines[idx].GetMnemonic();

            // Draw address in subdued color
            SetTextColor(hDC, addrColor);
            TextOut(hDC, block.X + CFG_BLOCK_PADDING, y, addrText, lstrlen(addrText));

            // Draw mnemonic
            SetTextColor(hDC, textColor);
            TextOut(hDC, block.X + CFG_BLOCK_PADDING + 75, y, asmText, lstrlen(asmText));

            // Draw comment if present
            char* comment = DisasmDataLines[idx].GetComments();
            if (comment && comment[0] != '\0') {
                // Measure mnemonic width to position comment after it
                SIZE asmSize;
                GetTextExtentPoint32(hDC, asmText, lstrlen(asmText), &asmSize);
                int commentX = block.X + CFG_BLOCK_PADDING + 75 + asmSize.cx;

                char commentBuf[256];
                wsprintf(commentBuf, "  %s", comment);
                COLORREF commentColor = g_DarkMode ? RGB(87, 166, 74) : RGB(0, 128, 0);
                SetTextColor(hDC, commentColor);
                TextOut(hDC, commentX, y, commentBuf, lstrlen(commentBuf));
            }

            y += CFG_LINE_HEIGHT;
        }
    }

    if (hOldFont) {
        SelectObject(hDC, hOldFont);
    }
}

// ================================================================
// ====================  SAVE GRAPH TO FILE  ======================
// ================================================================

static int GetEncoderCLSID(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0, size = 0;
    Gdiplus::GetImageEncodersSize(&num, &size);
    if (size == 0) return -1;

    Gdiplus::ImageCodecInfo* pInfo = (Gdiplus::ImageCodecInfo*)malloc(size);
    if (!pInfo) return -1;

    Gdiplus::GetImageEncoders(num, size, pInfo);
    for (UINT i = 0; i < num; i++) {
        if (wcscmp(pInfo[i].MimeType, format) == 0) {
            *pClsid = pInfo[i].Clsid;
            free(pInfo);
            return i;
        }
    }
    free(pInfo);
    return -1;
}

static void SaveGraphToFile(HWND hWnd, CFG_GRAPH* graph)
{
    if (!graph || graph->Blocks.empty()) return;

    // Show Save dialog with both formats
    char szFile[MAX_PATH] = "";
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "PNG Image (*.png)\0*.png\0JPEG Image (*.jpg)\0*.jpg\0";
    ofn.lpstrDefExt = "png";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileName(&ofn)) return;

    // Determine format from file extension
    BOOL isPNG = TRUE;
    const char* ext = strrchr(szFile, '.');
    if (ext && (_stricmp(ext, ".jpg") == 0 || _stricmp(ext, ".jpeg") == 0)) {
        isPNG = FALSE;
    }

    // Compute graph bounds with margin
    int margin = 20;
    int imgWidth = graph->GraphWidth + margin * 2;
    int imgHeight = graph->GraphHeight + margin * 2;
    if (imgWidth < 100) imgWidth = 100;
    if (imgHeight < 100) imgHeight = 100;

    // Find graph origin (minimum X, Y across all blocks)
    int minX = INT_MAX, minY = INT_MAX;
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        if (graph->Blocks[i].X < minX) minX = graph->Blocks[i].X;
        if (graph->Blocks[i].Y < minY) minY = graph->Blocks[i].Y;
    }

    // Create memory DC
    HDC hScreenDC = GetDC(hWnd);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    HBITMAP hBitmap;
    if (isPNG) {
        // 32-bit ARGB for transparency
        BITMAPINFO bmi;
        ZeroMemory(&bmi, sizeof(bmi));
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = imgWidth;
        bmi.bmiHeader.biHeight = -imgHeight;  // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        void* pBits = NULL;
        hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
        // DIB section is zero-initialized (fully transparent)
    } else {
        hBitmap = CreateCompatibleBitmap(hScreenDC, imgWidth, imgHeight);
    }

    HBITMAP hOldBmp = (HBITMAP)SelectObject(hMemDC, hBitmap);

    // For JPEG, fill with white background
    if (!isPNG) {
        RECT fillRect = {0, 0, imgWidth, imgHeight};
        HBRUSH hWhite = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hMemDC, &fillRect, hWhite);
        DeleteObject(hWhite);
    }

    // Set up transform: 1:1 zoom, offset to place graph at margin
    SetGraphicsMode(hMemDC, GM_ADVANCED);
    XFORM xform;
    xform.eM11 = 1.0f;
    xform.eM12 = 0;
    xform.eM21 = 0;
    xform.eM22 = 1.0f;
    xform.eDx = (FLOAT)(margin - minX);
    xform.eDy = (FLOAT)(margin - minY);
    SetWorldTransform(hMemDC, &xform);

    // Select font and render
    HFONT hOldFont = NULL;
    if (g_hCFGFont) {
        hOldFont = (HFONT)SelectObject(hMemDC, g_hCFGFont);
    }

    // Create a temporary view state for rendering (not used for transform, but needed by API)
    CFG_VIEW_STATE exportView;
    InitCFGViewState(&exportView);

    RenderEdges(hMemDC, graph, &exportView, NULL);
    RenderBlocks(hMemDC, graph, &exportView, NULL);

    if (hOldFont) {
        SelectObject(hMemDC, hOldFont);
    }

    // Reset transform
    ModifyWorldTransform(hMemDC, NULL, MWT_IDENTITY);

    // Save using GDI+
    Gdiplus::Bitmap* gdipBmp = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);
    if (gdipBmp) {
        // Convert filename to wide string
        WCHAR wFile[MAX_PATH];
        MultiByteToWideChar(CP_ACP, 0, szFile, -1, wFile, MAX_PATH);

        CLSID clsid;
        if (isPNG) {
            GetEncoderCLSID(L"image/png", &clsid);
            gdipBmp->Save(wFile, &clsid, NULL);
        } else {
            GetEncoderCLSID(L"image/jpeg", &clsid);
            // Set JPEG quality to 95
            Gdiplus::EncoderParameters encoderParams;
            encoderParams.Count = 1;
            encoderParams.Parameter[0].Guid = Gdiplus::EncoderQuality;
            encoderParams.Parameter[0].Type = Gdiplus::EncoderParameterValueTypeLong;
            encoderParams.Parameter[0].NumberOfValues = 1;
            ULONG quality = 95;
            encoderParams.Parameter[0].Value = &quality;
            gdipBmp->Save(wFile, &clsid, &encoderParams);
        }
        delete gdipBmp;
    }

    SelectObject(hMemDC, hOldBmp);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(hWnd, hScreenDC);
}

void RenderCFG(HWND hWnd, HDC hDC, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState)
{
    if (!hWnd || !hDC || !graph || !viewState) return;

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);

    // Create off-screen buffer for flicker-free rendering
    HDC hdcMem = CreateCompatibleDC(hDC);
    HBITMAP hbmMem = CreateCompatibleBitmap(hDC, clientRect.right, clientRect.bottom);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

    // Fill background
    COLORREF bgColor = g_DarkMode ? g_DarkBkColor : RGB(255, 255, 255);
    HBRUSH hBgBrush = CreateSolidBrush(bgColor);
    FillRect(hdcMem, &clientRect, hBgBrush);
    DeleteObject(hBgBrush);

    // Set up coordinate transformation for zoom/pan
    SetGraphicsMode(hdcMem, GM_ADVANCED);
    XFORM xform;
    xform.eM11 = (FLOAT)viewState->ZoomLevel;
    xform.eM12 = 0;
    xform.eM21 = 0;
    xform.eM22 = (FLOAT)viewState->ZoomLevel;
    xform.eDx = (FLOAT)viewState->PanOffsetX;
    xform.eDy = (FLOAT)viewState->PanOffsetY;
    SetWorldTransform(hdcMem, &xform);

    // Compute visible rectangle in graph coordinates for viewport culling
    RECT visibleRect;
    visibleRect.left   = (int)(-viewState->PanOffsetX / viewState->ZoomLevel);
    visibleRect.top    = (int)(-viewState->PanOffsetY / viewState->ZoomLevel);
    visibleRect.right  = visibleRect.left + (int)(clientRect.right / viewState->ZoomLevel);
    visibleRect.bottom = visibleRect.top + (int)(clientRect.bottom / viewState->ZoomLevel);

    // Draw edges first (behind blocks)
    RenderEdges(hdcMem, graph, viewState, &visibleRect);

    // Draw blocks
    RenderBlocks(hdcMem, graph, viewState, &visibleRect);

    // Reset transform
    ModifyWorldTransform(hdcMem, NULL, MWT_IDENTITY);

    // Draw zoom indicator
    char zoomText[32];
    wsprintf(zoomText, "Zoom: %d%%", (int)(viewState->ZoomLevel * 100));
    SetBkMode(hdcMem, TRANSPARENT);
    SetTextColor(hdcMem, g_DarkMode ? RGB(180, 180, 180) : RGB(80, 80, 80));
    TextOut(hdcMem, 10, clientRect.bottom - 20, zoomText, lstrlen(zoomText));

    // Blit to screen
    BitBlt(hDC, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

// ================================================================
// ====================  DIALOG PROCEDURE  ========================
// ================================================================

BOOL CALLBACK CFGViewerDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
        case WM_INITDIALOG: {
            // Initialize GDI+
            Gdiplus::GdiplusStartupInput gdipStartup;
            Gdiplus::GdiplusStartup(&g_GdiplusToken, &gdipStartup, NULL);

            // Initialize view state
            InitCFGViewState(&g_ViewState);

            // Create font for drawing
            if (g_hCFGFont) {
                DeleteObject(g_hCFGFont);
            }
            g_hCFGFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas");

            // Create cached GDI objects for edge rendering
            g_EdgeColors[0] = g_DarkMode ? RGB(150, 150, 150) : RGB(100, 100, 100);  // unconditional
            g_EdgeColors[1] = RGB(0, 180, 0);      // true (green)
            g_EdgeColors[2] = RGB(220, 50, 50);    // false (red)
            g_EdgeColors[3] = RGB(0, 120, 220);    // back-edge (blue)
            g_EdgeColors[4] = RGB(100, 50, 200);   // call (purple)
            for (int c = 0; c < 5; c++) {
                g_EdgePens[c] = CreatePen(PS_SOLID, 2, g_EdgeColors[c]);
                g_ArrowPens[c] = CreatePen(PS_SOLID, 1, g_EdgeColors[c]);
                g_ArrowBrushes[c] = CreateSolidBrush(g_EdgeColors[c]);
            }

            // Get function bounds from lParam
            DWORD_PTR* funcBounds = (DWORD_PTR*)lParam;
            BOOL buildSuccess = FALSE;

            if (funcBounds) {
                DWORD_PTR funcStart = funcBounds[0];
                DWORD_PTR funcEnd = funcBounds[1];
                const char* funcName = (const char*)funcBounds[2];
                DWORD_PTR startIndex = funcBounds[3];

                // Build CFG - use tracing mode (trace from entry point)
                if (funcStart == 0) {
                    // Tracing mode - trace control flow from entry point
                    buildSuccess = BuildCFGByTracing(startIndex, &g_CurrentGraph);
                } else {
                    // Function mode - use predefined boundaries
                    buildSuccess = BuildCFGFromFunction(funcStart, funcEnd, funcName, &g_CurrentGraph);
                }

                if (buildSuccess) {
                    LayoutCFG(&g_CurrentGraph);
                    ComputeEdgeRoutes(&g_CurrentGraph);
                    ComputeBlockColors(&g_CurrentGraph);
                    FocusOnEntryBlock(hWnd, &g_CurrentGraph, &g_ViewState);

                    // Set window title
                    char title[128];
                    wsprintf(title, "CFG Viewer - %s", g_CurrentGraph.FunctionName);
                    SetWindowText(hWnd, title);
                } else {
                    MessageBox(hWnd, "Failed to build control flow graph.\n\n"
                               "Make sure there is disassembled code available.",
                               "CFG Viewer", MB_OK | MB_ICONERROR);
                    DestroyWindow(hWnd);
                    g_hCFGViewerWnd = NULL;
                    return FALSE;
                }
            }

            return TRUE;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);
            RenderCFG(hWnd, hDC, &g_CurrentGraph, &g_ViewState);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;  // Prevent flicker

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ScreenToClient(hWnd, &pt);

            double zoomFactor = (delta > 0) ? CFG_ZOOM_FACTOR : (1.0 / CFG_ZOOM_FACTOR);
            ZoomAtPoint(&g_ViewState, pt, zoomFactor);

            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            // Check if clicked on a block
            POINT graphPt = ScreenToGraph(&g_ViewState, pt);
            DWORD_PTR clickedBlock = HitTestBlocks(&g_CurrentGraph, graphPt);

            if (clickedBlock != (DWORD_PTR)-1) {
                // Block clicked - select it and start dragging the block
                g_ViewState.SelectedBlockID = clickedBlock;
                g_ViewState.DraggingBlockID = clickedBlock;
                g_ViewState.IsDraggingBlock = TRUE;
                g_ViewState.DragStartPoint = pt;

                // Store block's original position
                if (g_CurrentGraph.BlockIDToIndex.count(clickedBlock)) {
                    CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[clickedBlock]];
                    g_ViewState.DragBlockStartX = block.X;
                    g_ViewState.DragBlockStartY = block.Y;
                }

                SetCapture(hWnd);
                InvalidateRect(hWnd, NULL, FALSE);
            } else {
                // Start dragging to pan
                g_ViewState.IsPanning = TRUE;
                g_ViewState.DragStartPoint = pt;
                g_ViewState.DragStartPanX = g_ViewState.PanOffsetX;
                g_ViewState.DragStartPanY = g_ViewState.PanOffsetY;
                SetCapture(hWnd);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            if (g_ViewState.IsDraggingBlock) {
                // Move the block
                if (g_CurrentGraph.BlockIDToIndex.count(g_ViewState.DraggingBlockID)) {
                    CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[g_ViewState.DraggingBlockID]];

                    // Calculate delta in graph coordinates
                    int deltaX = (int)((pt.x - g_ViewState.DragStartPoint.x) / g_ViewState.ZoomLevel);
                    int deltaY = (int)((pt.y - g_ViewState.DragStartPoint.y) / g_ViewState.ZoomLevel);

                    block.X = g_ViewState.DragBlockStartX + deltaX;
                    block.Y = g_ViewState.DragBlockStartY + deltaY;

                    // Update only the edges connected to this block (fast)
                    UpdateRoutesForDraggedBlock(&g_CurrentGraph, g_ViewState.DraggingBlockID);

                    InvalidateRect(hWnd, NULL, FALSE);
                }
            } else if (g_ViewState.IsPanning) {
                // Pan the view
                g_ViewState.PanOffsetX = g_ViewState.DragStartPanX + (pt.x - g_ViewState.DragStartPoint.x);
                g_ViewState.PanOffsetY = g_ViewState.DragStartPanY + (pt.y - g_ViewState.DragStartPoint.y);

                InvalidateRect(hWnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (g_ViewState.IsDraggingBlock) {
                g_ViewState.IsDraggingBlock = FALSE;
                g_ViewState.DraggingBlockID = (DWORD_PTR)-1;
                ReleaseCapture();
                // Recompute edge routes after block was moved
                ComputeEdgeRoutes(&g_CurrentGraph);
                InvalidateRect(hWnd, NULL, FALSE);
            }
            if (g_ViewState.IsPanning) {
                g_ViewState.IsPanning = FALSE;
                ReleaseCapture();
            }
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            // Double-click on block navigates to it in main disassembly
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            POINT graphPt = ScreenToGraph(&g_ViewState, pt);
            DWORD_PTR blockID = HitTestBlocks(&g_CurrentGraph, graphPt);

            if (blockID != (DWORD_PTR)-1 && g_CurrentGraph.BlockIDToIndex.count(blockID)) {
                CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[blockID]];

                // Navigate to this block in main disassembly
                HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
                if (hDisasm) {
                    ListView_SetItemState(hDisasm, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
                    ListView_SetItemState(hDisasm, (int)block.StartIndex,
                                          LVIS_SELECTED | LVIS_FOCUSED,
                                          LVIS_SELECTED | LVIS_FOCUSED);
                    ListView_EnsureVisible(hDisasm, (int)block.StartIndex, FALSE);
                }
            }
            return 0;
        }

        case WM_CONTEXTMENU: {
            POINT pt;
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);

            // If invoked via keyboard (Shift+F10), use center of window
            if (pt.x == -1 && pt.y == -1) {
                RECT rc;
                GetClientRect(hWnd, &rc);
                pt.x = rc.right / 2;
                pt.y = rc.bottom / 2;
                ClientToScreen(hWnd, &pt);
            }

            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, IDM_CFG_FIT_GRAPH, "Fit Graph to Screen");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, IDM_CFG_SAVE_IMAGE, "Save as Image...");

            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hMenu);
            return 0;
        }

        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case IDM_CFG_FIT_GRAPH:
                    CenterGraphInView(hWnd, &g_CurrentGraph, &g_ViewState);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case IDM_CFG_SAVE_IMAGE:
                    SaveGraphToFile(hWnd, &g_CurrentGraph);
                    break;
            }
            return 0;
        }

        case WM_SIZE: {
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_HOME:
                    // Reset view to fit graph
                    CenterGraphInView(hWnd, &g_CurrentGraph, &g_ViewState);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;

                case VK_ESCAPE:
                    DestroyWindow(hWnd);
                    break;

                case VK_ADD:
                case VK_OEM_PLUS: {
                    // Zoom in
                    RECT rect;
                    GetClientRect(hWnd, &rect);
                    POINT center = {rect.right / 2, rect.bottom / 2};
                    ZoomAtPoint(&g_ViewState, center, CFG_ZOOM_FACTOR);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;
                }

                case VK_SUBTRACT:
                case VK_OEM_MINUS: {
                    // Zoom out
                    RECT rect;
                    GetClientRect(hWnd, &rect);
                    POINT center = {rect.right / 2, rect.bottom / 2};
                    ZoomAtPoint(&g_ViewState, center, 1.0 / CFG_ZOOM_FACTOR);
                    InvalidateRect(hWnd, NULL, FALSE);
                    break;
                }
            }
            return 0;
        }

        case WM_CLOSE: {
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_DESTROY: {
            FreeCFGGraph(&g_CurrentGraph);
            if (g_hCFGFont) {
                DeleteObject(g_hCFGFont);
                g_hCFGFont = NULL;
            }
            // Clean up cached GDI objects
            for (int c = 0; c < 5; c++) {
                if (g_EdgePens[c])     { DeleteObject(g_EdgePens[c]);     g_EdgePens[c] = NULL; }
                if (g_ArrowPens[c])    { DeleteObject(g_ArrowPens[c]);    g_ArrowPens[c] = NULL; }
                if (g_ArrowBrushes[c]) { DeleteObject(g_ArrowBrushes[c]); g_ArrowBrushes[c] = NULL; }
            }
            if (g_GdiplusToken) {
                Gdiplus::GdiplusShutdown(g_GdiplusToken);
                g_GdiplusToken = 0;
            }
            g_hCFGViewerWnd = NULL;
            return 0;
        }
    }

    return FALSE;
}

// ================================================================
// ====================  PUBLIC INTERFACE  ========================
// ================================================================

// Build CFG by tracing control flow from a starting address
// This works without predefined function boundaries
// Helper to extract address from mnemonic like "jmp 00401234" or "call dword ptr [00401234]"
static DWORD_PTR ExtractAddressFromMnemonic(const char* mnemonic)
{
    if (!mnemonic) return 0;

    // Skip past the opcode to find the operand
    const char* p = mnemonic;

    // Skip leading whitespace
    while (*p && (*p == ' ' || *p == '\t')) p++;

    // Skip the opcode
    while (*p && *p != ' ' && *p != '\t') p++;

    // Skip whitespace after opcode
    while (*p && (*p == ' ' || *p == '\t')) p++;

    // Now we're at the operand - look for hex addresses
    // Skip "short", "near", "far", "dword ptr", etc.
    while (*p) {
        // Skip non-hex characters, but look for hex digits
        while (*p && !isxdigit(*p)) {
            // Skip past keywords
            if (strncmp(p, "short", 5) == 0 || strncmp(p, "SHORT", 5) == 0) {
                p += 5;
                continue;
            }
            if (strncmp(p, "near", 4) == 0 || strncmp(p, "NEAR", 4) == 0) {
                p += 4;
                continue;
            }
            if (strncmp(p, "far", 3) == 0 || strncmp(p, "FAR", 3) == 0) {
                p += 3;
                continue;
            }
            if (strncmp(p, "dword", 5) == 0 || strncmp(p, "DWORD", 5) == 0) {
                p += 5;
                continue;
            }
            if (strncmp(p, "ptr", 3) == 0 || strncmp(p, "PTR", 3) == 0) {
                p += 3;
                continue;
            }
            p++;
        }
        if (!*p) break;

        // Found hex digits - count them
        const char* start = p;
        int hexCount = 0;
        while (isxdigit(*p)) {
            hexCount++;
            p++;
        }

        // Valid addresses are typically 4-16 hex digits (16-bit to 64-bit)
        if (hexCount >= 4 && hexCount <= 16) {
            char addrStr[20];
            int copyLen = (hexCount < 19) ? hexCount : 19;
            strncpy(addrStr, start, copyLen);
            addrStr[copyLen] = '\0';
            return StringToDword(addrStr);
        }
    }
    return 0;
}

BOOL BuildCFGByTracing(DWORD_PTR startIndex, CFG_GRAPH* outGraph)
{
    if (!outGraph || startIndex >= DisasmDataLines.size()) return FALSE;

    ClearCFGGraph(outGraph);

    DWORD_PTR startAddr = GetAddressAtIndex(startIndex);
    if(LoadedPe64)
        wsprintf(outGraph->FunctionName, "Code at %08X%08X", (DWORD)(startAddr>>32), (DWORD)startAddr);
    else
        wsprintf(outGraph->FunctionName, "Code at %08X", (DWORD)startAddr);
    outGraph->FunctionStart = startAddr;

    // Set to track visited addresses and addresses to visit
    std::set<DWORD_PTR> visitedIndices;
    std::set<DWORD_PTR> toVisit;
    std::set<DWORD_PTR> blockStartIndices;
    std::map<DWORD_PTR, std::vector<DWORD_PTR>> successors;  // index -> successor indices
    std::map<DWORD_PTR, CFG_EDGE_TYPE> edgeTypes;

    // Start tracing from the selected instruction
    toVisit.insert(startIndex);
    blockStartIndices.insert(startIndex);

    // Trace all reachable code
    while (!toVisit.empty()) {
        DWORD_PTR currentIndex = *toVisit.begin();
        toVisit.erase(toVisit.begin());

        if (visitedIndices.count(currentIndex) || currentIndex >= DisasmDataLines.size()) {
            continue;
        }

        // Trace this block until we hit a branch or end
        DWORD_PTR idx = currentIndex;
        while (idx < DisasmDataLines.size()) {
            if (visitedIndices.count(idx)) break;

            // Skip proc marker lines (empty address = comment/separator, not real instruction)
            const char* addrStr = DisasmDataLines[idx].GetAddress();
            if (!addrStr || addrStr[0] == '\0') {
                idx++;
                continue;
            }

            visitedIndices.insert(idx);

            const char* mnemonic = DisasmDataLines[idx].GetMnemonic();
            bool isCondJump = IsConditionalJump(mnemonic);
            bool isUncondJump = IsUnconditionalJump(mnemonic);
            bool isRet = IsReturnInstruction(mnemonic);
            bool isCall = IsCallInstruction(mnemonic);

            if (isRet) {
                // End of this path
                break;
            }

            if (isCall) {
                // Try to find call destination
                DWORD_PTR destAddr = 0;
                bool foundInCodeFlow = false;

                for (size_t j = 0; j < DisasmCodeFlow.size(); j++) {
                    if (DisasmCodeFlow[j].Current_Index == idx) {
                        destAddr = DisasmCodeFlow[j].Branch_Destination;
                        foundInCodeFlow = true;
                        break;
                    }
                }

                if (!foundInCodeFlow || destAddr == 0) {
                    destAddr = ExtractAddressFromMnemonic(mnemonic);
                }

                // Check if target is a numeric local address (not an API call like KERNEL32!ExitProcess)
                // API calls contain '!' in mnemonic or resolve to address 0
                bool isLocalCall = (destAddr != 0);
                if (isLocalCall) {
                    // Check if mnemonic contains '!' (API call indicator)
                    const char* p = mnemonic;
                    while (*p) {
                        if (*p == '!') { isLocalCall = false; break; }
                        p++;
                    }
                }

                if (isLocalCall) {
                    DWORD_PTR destIndex = FindIndexByAddress(destAddr);

                    if (destIndex != (DWORD_PTR)-1 && destIndex < DisasmDataLines.size()) {
                        // Call target starts a new block
                        blockStartIndices.insert(destIndex);
                        // Return address (next instruction) starts a new block
                        if (idx + 1 < DisasmDataLines.size()) {
                            blockStartIndices.insert(idx + 1);
                        }

                        // CALL edge to target
                        successors[idx].push_back(destIndex);
                        edgeTypes[(idx << 16) | (destIndex & 0xFFFF)] = EDGE_CALL;

                        // Fall-through edge to return address
                        if (idx + 1 < DisasmDataLines.size()) {
                            successors[idx].push_back(idx + 1);
                            edgeTypes[(idx << 16) | ((idx + 1) & 0xFFFF)] = EDGE_UNCONDITIONAL;

                            if (!visitedIndices.count(idx + 1)) {
                                toVisit.insert(idx + 1);
                            }
                        }

                        // Trace the call target
                        if (!visitedIndices.count(destIndex)) {
                            toVisit.insert(destIndex);
                        }

                        break;  // End current block (CALL splits the block)
                    }
                }

                // If not a local call, just continue (fall-through)
                idx++;
                continue;
            }

            if (isCondJump || isUncondJump) {
                // Try to find branch destination from DisasmCodeFlow first
                DWORD_PTR destAddr = 0;
                bool foundInCodeFlow = false;

                for (size_t j = 0; j < DisasmCodeFlow.size(); j++) {
                    if (DisasmCodeFlow[j].Current_Index == idx) {
                        destAddr = DisasmCodeFlow[j].Branch_Destination;
                        foundInCodeFlow = true;
                        break;
                    }
                }

                // Fallback: extract address from mnemonic text
                if (!foundInCodeFlow || destAddr == 0) {
                    destAddr = ExtractAddressFromMnemonic(mnemonic);
                }

                if (destAddr != 0) {
                    DWORD_PTR destIndex = FindIndexByAddress(destAddr);

                    if (destIndex != (DWORD_PTR)-1 && destIndex < DisasmDataLines.size()) {
                        // Only jump TARGETS create new blocks
                        blockStartIndices.insert(destIndex);
                        successors[idx].push_back(destIndex);

                        if (isCondJump) {
                            // Jump target = branch TAKEN = TRUE (Green)
                            edgeTypes[(idx << 16) | (destIndex & 0xFFFF)] = EDGE_CONDITIONAL_TRUE;
                        } else {
                            edgeTypes[(idx << 16) | (destIndex & 0xFFFF)] = EDGE_UNCONDITIONAL;
                        }

                        if (!visitedIndices.count(destIndex)) {
                            toVisit.insert(destIndex);
                        }
                    }
                }

                // For conditional jumps, fall-through is also a successor AND a new block
                // (because the Jcc ends the current block)
                if (isCondJump && idx + 1 < DisasmDataLines.size()) {
                    blockStartIndices.insert(idx + 1);
                    successors[idx].push_back(idx + 1);
                    // Fall-through = branch NOT taken = FALSE (Red)
                    edgeTypes[(idx << 16) | ((idx + 1) & 0xFFFF)] = EDGE_CONDITIONAL_FALSE;

                    if (!visitedIndices.count(idx + 1)) {
                        toVisit.insert(idx + 1);
                    }
                }
                break;  // End of this block
            }

            // Continue to next instruction (fall-through)
            idx++;
        }
    }

    // Find the range of traced code
    if (visitedIndices.empty()) return FALSE;

    DWORD_PTR minIndex = *visitedIndices.begin();
    DWORD_PTR maxIndex = *visitedIndices.rbegin();
    outGraph->FunctionEnd = GetAddressAtIndex(maxIndex);

    // Create basic blocks
    std::vector<DWORD_PTR> sortedStarts(blockStartIndices.begin(), blockStartIndices.end());
    std::sort(sortedStarts.begin(), sortedStarts.end());

    for (size_t i = 0; i < sortedStarts.size(); i++) {
        DWORD_PTR blockStartIdx = sortedStarts[i];

        // Skip if this block start wasn't actually visited
        if (!visitedIndices.count(blockStartIdx)) continue;

        CFG_BASIC_BLOCK block;
        memset(&block, 0, sizeof(block));

        block.BlockID = outGraph->Blocks.size();
        block.StartIndex = blockStartIdx;
        block.StartAddress = GetAddressAtIndex(blockStartIdx);

        // Find end of this block
        DWORD_PTR endIdx = blockStartIdx;
        for (DWORD_PTR idx = blockStartIdx; idx <= maxIndex && visitedIndices.count(idx); idx++) {
            // Check if next instruction is a new block start
            if (idx > blockStartIdx && blockStartIndices.count(idx)) {
                break;
            }
            endIdx = idx;

            // Check if this is a block-ending instruction
            const char* mnemonic = DisasmDataLines[idx].GetMnemonic();
            if (IsConditionalJump(mnemonic) || IsUnconditionalJump(mnemonic) || IsReturnInstruction(mnemonic)) {
                break;
            }
            // CALL also ends the block if the next instruction starts a new block
            if (IsCallInstruction(mnemonic) && blockStartIndices.count(idx + 1)) {
                break;
            }
        }

        block.EndIndex = endIdx;
        block.EndAddress = GetAddressAtIndex(endIdx);
        block.IsEntryBlock = (blockStartIdx == startIndex);

        const char* lastMnemonic = DisasmDataLines[endIdx].GetMnemonic();
        block.IsExitBlock = IsReturnInstruction(lastMnemonic);

        // Check if this block starts a known function (fFunctionInfo or CallTargets)
        block.FunctionLabel[0] = '\0';
        bool foundLabel = false;
        for (size_t fi = 0; fi < fFunctionInfo.size(); fi++) {
            if (fFunctionInfo[fi].FunctionStart == block.StartAddress) {
                if (fFunctionInfo[fi].FunctionName[0] != '\0')
                    strncpy(block.FunctionLabel, fFunctionInfo[fi].FunctionName, 63);
                else
                    if(LoadedPe64)
                        wsprintf(block.FunctionLabel, "Proc_%08X%08X", (DWORD)(block.StartAddress>>32), (DWORD)block.StartAddress);
                    else
                        wsprintf(block.FunctionLabel, "Proc_%08X", (DWORD)block.StartAddress);
                block.FunctionLabel[63] = '\0';
                foundLabel = true;
                break;
            }
        }
        if (!foundLabel && CallTargets.count(block.StartAddress)) {
            if(LoadedPe64)
                wsprintf(block.FunctionLabel, "Proc_%08X%08X", (DWORD)(block.StartAddress>>32), (DWORD)block.StartAddress);
            else
                wsprintf(block.FunctionLabel, "Proc_%08X", (DWORD)block.StartAddress);
        }

        outGraph->Blocks.push_back(block);
        outGraph->AddressToBlockIndex[block.StartAddress] = outGraph->Blocks.size() - 1;
        outGraph->BlockIDToIndex[block.BlockID] = outGraph->Blocks.size() - 1;
    }

    // Create edges directly from each block's ending instruction
    // This is more reliable than using the successors map
    std::set<DWORD_PTR> actualJumpTargets;

    for (size_t i = 0; i < outGraph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = outGraph->Blocks[i];
        DWORD_PTR lastIdx = block.EndIndex;
        const char* lastMnemonic = DisasmDataLines[lastIdx].GetMnemonic();

        if (IsUnconditionalJump(lastMnemonic)) {
            // JMP instruction - create edge to jump target
            DWORD_PTR destAddr = ExtractAddressFromMnemonic(lastMnemonic);
            if (destAddr != 0 && outGraph->AddressToBlockIndex.count(destAddr)) {
                actualJumpTargets.insert(destAddr);
                CFG_EDGE edge;
                edge.SourceBlockID = block.BlockID;
                edge.TargetBlockID = outGraph->Blocks[outGraph->AddressToBlockIndex[destAddr]].BlockID;
                edge.Type = EDGE_UNCONDITIONAL;
                edge.IsBackEdge = false;
                outGraph->Edges.push_back(edge);
            }
        } else if (IsConditionalJump(lastMnemonic)) {
            // Jcc instruction - create edge to jump target (TRUE) and fall-through (FALSE)
            DWORD_PTR destAddr = ExtractAddressFromMnemonic(lastMnemonic);
            if (destAddr != 0 && outGraph->AddressToBlockIndex.count(destAddr)) {
                actualJumpTargets.insert(destAddr);
                CFG_EDGE edge;
                edge.SourceBlockID = block.BlockID;
                edge.TargetBlockID = outGraph->Blocks[outGraph->AddressToBlockIndex[destAddr]].BlockID;
                edge.Type = EDGE_CONDITIONAL_TRUE;  // Jump target = TRUE (Green)
                edge.IsBackEdge = false;
                outGraph->Edges.push_back(edge);
            }

            // Fall-through edge
            DWORD_PTR nextIdx = lastIdx + 1;
            DWORD_PTR nextAddr = GetAddressAtIndex(nextIdx);
            if (outGraph->AddressToBlockIndex.count(nextAddr)) {
                actualJumpTargets.insert(nextAddr);
                CFG_EDGE edge;
                edge.SourceBlockID = block.BlockID;
                edge.TargetBlockID = outGraph->Blocks[outGraph->AddressToBlockIndex[nextAddr]].BlockID;
                edge.Type = EDGE_CONDITIONAL_FALSE;  // Fall-through = FALSE (Red)
                edge.IsBackEdge = false;
                outGraph->Edges.push_back(edge);
            }
        } else if (IsCallInstruction(lastMnemonic)) {
            // CALL instruction - create edge to call target and fall-through
            DWORD_PTR destAddr = ExtractAddressFromMnemonic(lastMnemonic);
            if (destAddr != 0 && outGraph->AddressToBlockIndex.count(destAddr)) {
                actualJumpTargets.insert(destAddr);
                CFG_EDGE edge;
                edge.SourceBlockID = block.BlockID;
                edge.TargetBlockID = outGraph->Blocks[outGraph->AddressToBlockIndex[destAddr]].BlockID;
                edge.Type = EDGE_CALL;
                edge.IsBackEdge = false;
                outGraph->Edges.push_back(edge);
            }

            // Fall-through edge (return address)
            DWORD_PTR nextIdx = lastIdx + 1;
            DWORD_PTR nextAddr = GetAddressAtIndex(nextIdx);
            if (outGraph->AddressToBlockIndex.count(nextAddr)) {
                actualJumpTargets.insert(nextAddr);
                CFG_EDGE edge;
                edge.SourceBlockID = block.BlockID;
                edge.TargetBlockID = outGraph->Blocks[outGraph->AddressToBlockIndex[nextAddr]].BlockID;
                edge.Type = EDGE_UNCONDITIONAL;
                edge.IsBackEdge = false;
                outGraph->Edges.push_back(edge);
            }
        } else if (!IsReturnInstruction(lastMnemonic) && !block.IsExitBlock) {
            // Not a branch, call, or return - create fall-through edge
            DWORD_PTR nextIdx = lastIdx + 1;
            DWORD_PTR nextAddr = GetAddressAtIndex(nextIdx);
            if (outGraph->AddressToBlockIndex.count(nextAddr)) {
                CFG_EDGE edge;
                edge.SourceBlockID = block.BlockID;
                edge.TargetBlockID = outGraph->Blocks[outGraph->AddressToBlockIndex[nextAddr]].BlockID;
                edge.Type = EDGE_UNCONDITIONAL;
                edge.IsBackEdge = false;
                outGraph->Edges.push_back(edge);
            }
        }
        // RET instructions have no outgoing edges
    }

    // Add loc_ labels for jump target blocks that don't already have a function label
    for (size_t i = 0; i < outGraph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = outGraph->Blocks[i];
        if (block.FunctionLabel[0] == '\0' && !block.IsEntryBlock &&
            actualJumpTargets.count(block.StartAddress)) {
            if(LoadedPe64)
                wsprintf(block.FunctionLabel, "loc_%08X%08X", (DWORD)(block.StartAddress>>32), (DWORD)block.StartAddress);
            else
                wsprintf(block.FunctionLabel, "loc_%08X", (DWORD)block.StartAddress);
        }
    }

    // Post-process: Merge blocks that are NOT actual jump targets
    // (they only have fall-through from previous block and previous block doesn't end with branch)
    bool merged = true;
    while (merged) {
        merged = false;
        for (size_t i = 1; i < outGraph->Blocks.size(); i++) {
            CFG_BASIC_BLOCK& block = outGraph->Blocks[i];

            // Skip if this block is an actual jump target
            if (actualJumpTargets.count(block.StartAddress)) continue;

            // Skip if this is the entry block
            if (block.IsEntryBlock) continue;

            // Find the previous block (by address)
            CFG_BASIC_BLOCK* prevBlock = nullptr;
            size_t prevIdx = 0;
            for (size_t j = 0; j < outGraph->Blocks.size(); j++) {
                if (j == i) continue;
                CFG_BASIC_BLOCK& candidate = outGraph->Blocks[j];
                if (candidate.EndIndex + 1 == block.StartIndex) {
                    // Check if previous block ends with a control flow instruction
                    const char* lastMnemonic = DisasmDataLines[candidate.EndIndex].GetMnemonic();
                    if (!IsConditionalJump(lastMnemonic) && !IsUnconditionalJump(lastMnemonic) && !IsReturnInstruction(lastMnemonic) && !IsCallInstruction(lastMnemonic)) {
                        prevBlock = &candidate;
                        prevIdx = j;
                        break;
                    }
                }
            }

            if (prevBlock) {
                // Merge: extend previous block to include this block
                prevBlock->EndIndex = block.EndIndex;
                prevBlock->EndAddress = block.EndAddress;
                if (block.IsExitBlock) prevBlock->IsExitBlock = true;

                // Remove the merged block
                DWORD_PTR mergedBlockID = block.BlockID;
                outGraph->Blocks.erase(outGraph->Blocks.begin() + i);

                // Update edges: redirect any edges pointing to merged block to previous block
                for (size_t e = 0; e < outGraph->Edges.size(); ) {
                    if (outGraph->Edges[e].TargetBlockID == mergedBlockID) {
                        // This edge pointed to the merged block
                        if (outGraph->Edges[e].SourceBlockID == prevBlock->BlockID) {
                            // Self-edge from merge, remove it
                            outGraph->Edges.erase(outGraph->Edges.begin() + e);
                            continue;
                        }
                        outGraph->Edges[e].TargetBlockID = prevBlock->BlockID;
                    }
                    if (outGraph->Edges[e].SourceBlockID == mergedBlockID) {
                        outGraph->Edges[e].SourceBlockID = prevBlock->BlockID;
                    }
                    e++;
                }

                // Remove fall-through edge between the two blocks
                for (size_t e = 0; e < outGraph->Edges.size(); ) {
                    if (outGraph->Edges[e].SourceBlockID == prevBlock->BlockID &&
                        outGraph->Edges[e].TargetBlockID == prevBlock->BlockID) {
                        outGraph->Edges.erase(outGraph->Edges.begin() + e);
                        continue;
                    }
                    e++;
                }

                // Rebuild lookup maps
                outGraph->AddressToBlockIndex.clear();
                outGraph->BlockIDToIndex.clear();
                for (size_t j = 0; j < outGraph->Blocks.size(); j++) {
                    outGraph->AddressToBlockIndex[outGraph->Blocks[j].StartAddress] = j;
                    outGraph->BlockIDToIndex[outGraph->Blocks[j].BlockID] = j;
                }

                merged = true;
                break;  // Restart the loop
            }
        }
    }

    return (outGraph->Blocks.size() > 0);
}

void ShowCFGViewerForCurrentFunction(HWND hParent)
{
    // If window already exists, bring it to front
    if (g_hCFGViewerWnd && IsWindow(g_hCFGViewerWnd)) {
        SetForegroundWindow(g_hCFGViewerWnd);
        return;
    }

    // Get current selection in disassembly
    HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
    if (!hDisasm) {
        MessageBox(hParent, "No disassembly view available.", "CFG Viewer", MB_OK | MB_ICONWARNING);
        return;
    }

    if (DisasmDataLines.empty()) {
        MessageBox(hParent, "No disassembly data available.", "CFG Viewer", MB_OK | MB_ICONWARNING);
        return;
    }

    // Get entry point address
    DWORD_PTR entryPointAddr = 0;
    if (nt_hdr) {
        entryPointAddr = nt_hdr->OptionalHeader.AddressOfEntryPoint + nt_hdr->OptionalHeader.ImageBase;
    }

    // Find the entry point in the disassembly
    DWORD_PTR startIndex = 0;
    if (entryPointAddr != 0) {
        startIndex = FindIndexByAddress(entryPointAddr);
        if (startIndex == (DWORD_PTR)-1) {
            startIndex = 0;  // Fall back to first instruction
        }
    }

    // Pass start index to dialog for tracing mode
    static DWORD_PTR funcBounds[4];
    funcBounds[0] = 0;  // No predefined function start
    funcBounds[1] = 0;  // No predefined function end
    funcBounds[2] = 0;  // No function name
    funcBounds[3] = startIndex;  // Start tracing from entry point

    // Create modeless dialog (non-blocking)
    g_hCFGViewerWnd = CreateDialogParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CFG_VIEWER),
                                         hParent, (DLGPROC)CFGViewerDlgProc, (LPARAM)funcBounds);

    if (g_hCFGViewerWnd) {
        ShowWindow(g_hCFGViewerWnd, SW_SHOW);
    }
}

// Returns the CFG viewer window handle for message loop integration
HWND GetCFGViewerWindow()
{
    return g_hCFGViewerWnd;
}
