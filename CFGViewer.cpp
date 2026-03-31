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
void ShowDisassemblyTab();  // Defined in FileMap.cpp

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
static bool     g_OverviewEnabled = TRUE; // Overview minimap enabled by default
static bool     g_CommentsAligned = false; // Comment alignment toggle
bool            g_CFGGraphValid = false; // True when embedded CFG child has a valid graph

// ================================================================
// ====================  HELPER FUNCTIONS  ========================
// ================================================================

// Forward declarations for functions defined later in the file
static DWORD_PTR ExtractAddressFromMnemonic(const char* mnemonic);
static void FocusOnBlock(HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* state, int blockIdx);
static void FocusOnEntryBlock(HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* state);
static void UpdateRoutesForDraggedBlock(CFG_GRAPH* graph, DWORD_PTR blockID);

DWORD_PTR GetAddressAtIndex(DWORD_PTR index)
{
    if (index >= DisasmDataLines.size()) return 0;
    return (DWORD_PTR)StringToQword(DisasmDataLines[index].GetAddress());
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

        // Check if this block starts a known function
        block.FunctionLabel[0] = '\0';
        bool foundLabel = false;

        // First, check fFunctionInfo
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

        // Fallback: check for a banner line in DisasmDataLines right before this block
        if (!foundLabel && block.StartIndex > 0) {
            DWORD_PTR prevIdx = block.StartIndex - 1;
            const char* prevMnem = DisasmDataLines[prevIdx].GetMnemonic();
            if (prevMnem && strncmp(prevMnem, "; ======", 8) == 0) {
                const char* s = prevMnem + 9; // skip "; ====== "
                const char* e = s;
                while (*e && strncmp(e, " ======", 7) != 0) e++;
                size_t len = e - s;
                if (len > 0 && len < 64) {
                    memcpy(block.FunctionLabel, s, len);
                    block.FunctionLabel[len] = '\0';
                }
                foundLabel = true;
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
            char* addr = DisasmDataLines[idx].GetAddress();
            char* mnemonic = DisasmDataLines[idx].GetMnemonic();
            char* comment = DisasmDataLines[idx].GetComments();

            // Measure address + gap + mnemonic base width
            SIZE addrSize, mnemonicSize;
            GetTextExtentPoint32(hDC, addr, lstrlen(addr), &addrSize);
            GetTextExtentPoint32(hDC, mnemonic, lstrlen(mnemonic), &mnemonicSize);
            int lineWidth = addrSize.cx + 8 + mnemonicSize.cx; // 8 = addr-mnemonic gap

            if (comment && comment[0] != '\0') {
                SIZE commentSize;
                char commentBuf[256];
                wsprintf(commentBuf, "  %s", comment);
                GetTextExtentPoint32(hDC, commentBuf, lstrlen(commentBuf), &commentSize);

                if (block.AlignedCommentCol > 0) {
                    // With alignment: comment starts at aligned column
                    lineWidth = addrSize.cx + 8 + block.AlignedCommentCol + commentSize.cx;
                } else {
                    lineWidth += commentSize.cx;
                }
            }

            if (lineWidth + 2 * CFG_BLOCK_PADDING > maxWidth) {
                maxWidth = lineWidth + 2 * CFG_BLOCK_PADDING;
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

    size_t N = graph->Blocks.size();

    // ================================================================
    // Phase 0: Calculate block dimensions
    // ================================================================
    HDC hDC = GetDC(NULL);
    CalculateBlockDimensions(hDC, graph);
    ReleaseDC(NULL, hDC);

    // ================================================================
    // Phase 1: Layer Assignment (DFS back-edges + longest path)
    // ================================================================

    // Build adjacency (by block index, with edge indices)
    std::map<DWORD_PTR, size_t> idToIdx;
    for (size_t i = 0; i < N; i++)
        idToIdx[graph->Blocks[i].BlockID] = i;

    std::vector<std::vector<std::pair<size_t, size_t>>> outEdges(N); // (target, edge_idx)
    for (size_t e = 0; e < graph->Edges.size(); e++) {
        auto itS = idToIdx.find(graph->Edges[e].SourceBlockID);
        auto itT = idToIdx.find(graph->Edges[e].TargetBlockID);
        if (itS != idToIdx.end() && itT != idToIdx.end())
            outEdges[itS->second].push_back({ itT->second, e });
    }

    size_t entryIdx = 0;
    for (size_t i = 0; i < N; i++) {
        if (graph->Blocks[i].IsEntryBlock) { entryIdx = i; break; }
    }

    // DFS to identify true back edges (edges to ancestors on DFS stack)
    std::vector<bool> isBackEdge(graph->Edges.size(), false);
    {
        std::vector<int> color(N, 0); // 0=white, 1=gray (on stack), 2=black
        std::vector<std::pair<size_t, int>> stk; // (node, child_cursor)

        auto runDfs = [&](size_t start) {
            color[start] = 1;
            stk.push_back({ start, 0 });
            while (!stk.empty()) {
                auto& top = stk.back();
                size_t u = top.first;
                if (top.second >= (int)outEdges[u].size()) {
                    color[u] = 2;
                    stk.pop_back();
                    continue;
                }
                size_t v = outEdges[u][top.second].first;
                size_t e = outEdges[u][top.second].second;
                top.second++;
                if (color[v] == 1) { // ancestor on stack = back edge
                    isBackEdge[e] = true;
                    graph->Edges[e].IsBackEdge = true;
                } else if (color[v] == 0) {
                    color[v] = 1;
                    stk.push_back({ v, 0 });
                }
                // color==2 (cross/forward edge) → not a back edge, skip
            }
        };
        runDfs(entryIdx);
        for (size_t i = 0; i < N; i++)
            if (color[i] == 0) runDfs(i);
    }

    // Build forward-edge DAG (excluding back edges)
    std::vector<std::vector<size_t>> fwdSuccs(N), fwdPreds(N);
    for (size_t e = 0; e < graph->Edges.size(); e++) {
        if (isBackEdge[e]) continue;
        auto itS = idToIdx.find(graph->Edges[e].SourceBlockID);
        auto itT = idToIdx.find(graph->Edges[e].TargetBlockID);
        if (itS != idToIdx.end() && itT != idToIdx.end()) {
            fwdSuccs[itS->second].push_back(itT->second);
            fwdPreds[itT->second].push_back(itS->second);
        }
    }

    // Longest path on forward DAG via topological sort (Kahn's algorithm)
    // This gives the deepest possible layer → tall/narrow like IDA
    std::vector<int> layer(N, 0);
    std::vector<int> inDeg(N, 0);
    for (size_t i = 0; i < N; i++)
        inDeg[i] = (int)fwdPreds[i].size();

    std::queue<size_t> topoQ;
    for (size_t i = 0; i < N; i++)
        if (inDeg[i] == 0) topoQ.push(i);

    while (!topoQ.empty()) {
        size_t u = topoQ.front(); topoQ.pop();
        for (size_t v : fwdSuccs[u]) {
            if (layer[u] + 1 > layer[v])
                layer[v] = layer[u] + 1;
            if (--inDeg[v] == 0)
                topoQ.push(v);
        }
    }

    int maxLayer = 0;
    for (size_t i = 0; i < N; i++) {
        if (layer[i] > maxLayer) maxLayer = layer[i];
        graph->Blocks[i].Layer = layer[i];
    }

    // Mark back edges on blocks (layer comparison after longest-path assignment)
    for (size_t e = 0; e < graph->Edges.size(); e++) {
        if (isBackEdge[e]) continue; // already marked by DFS
        auto itS = idToIdx.find(graph->Edges[e].SourceBlockID);
        auto itT = idToIdx.find(graph->Edges[e].TargetBlockID);
        if (itS != idToIdx.end() && itT != idToIdx.end()) {
            if (layer[itT->second] <= layer[itS->second]) {
                isBackEdge[e] = true;
                graph->Edges[e].IsBackEdge = true;
            }
        }
    }

    // ================================================================
    // Phase 2: Crossing Minimization (Barycenter heuristic)
    // ================================================================

    // Group block indices by layer
    std::vector<std::vector<size_t>> layerBlocks(maxLayer + 1);
    for (size_t i = 0; i < N; i++)
        layerBlocks[layer[i]].push_back(i);

    // Identify primary (fall-through) successor per block
    std::vector<size_t> primarySucc(N, (size_t)-1);
    for (size_t e = 0; e < graph->Edges.size(); e++) {
        if (isBackEdge[e]) continue;
        auto itS = idToIdx.find(graph->Edges[e].SourceBlockID);
        auto itT = idToIdx.find(graph->Edges[e].TargetBlockID);
        if (itS != idToIdx.end() && itT != idToIdx.end()) {
            size_t s = itS->second, t = itT->second;
            CFG_EDGE_TYPE etype = graph->Edges[e].Type;
            if (etype == EDGE_CONDITIONAL_FALSE ||
                (etype == EDGE_UNCONDITIONAL && primarySucc[s] == (size_t)-1)) {
                primarySucc[s] = t;
            }
        }
    }
    for (size_t i = 0; i < N; i++) {
        if (primarySucc[i] == (size_t)-1 && fwdSuccs[i].size() == 1)
            primarySucc[i] = fwdSuccs[i][0];
    }

    // DFS ordering: follow primary (fall-through) first for initial layer ordering
    // This creates IDA-like spine: fall-through path stays in the center
    std::vector<bool> visited(N, false);
    std::vector<size_t> dfsOrder;
    dfsOrder.reserve(N);
    {
        std::vector<std::pair<size_t, int>> stk; // (node, child_index)
        // Push entry's successors: primary first (will be visited first)
        stk.push_back({ entryIdx, -1 });
        visited[entryIdx] = true;
        dfsOrder.push_back(entryIdx);

        while (!stk.empty()) {
            auto& top = stk.back();
            size_t node = top.first;
            top.second++;

            // Build ordered children: primary first, then others
            std::vector<size_t>& succs = fwdSuccs[node];
            std::vector<size_t> ordered;
            if (primarySucc[node] != (size_t)-1) {
                ordered.push_back(primarySucc[node]);
            }
            for (size_t s : succs) {
                if (s != primarySucc[node]) ordered.push_back(s);
            }

            if (top.second >= (int)ordered.size()) {
                stk.pop_back();
                continue;
            }

            size_t child = ordered[top.second];
            if (!visited[child]) {
                visited[child] = true;
                dfsOrder.push_back(child);
                stk.push_back({ child, -1 });
            }
        }
        // Add any unvisited blocks
        for (size_t i = 0; i < N; i++) {
            if (!visited[i]) dfsOrder.push_back(i);
        }
    }

    // Re-order blocks within each layer by DFS visit order
    std::vector<int> dfsRank(N, 0);
    for (int i = 0; i < (int)dfsOrder.size(); i++) dfsRank[dfsOrder[i]] = i;

    for (int L = 0; L <= maxLayer; L++) {
        std::sort(layerBlocks[L].begin(), layerBlocks[L].end(),
            [&](size_t a, size_t b) { return dfsRank[a] < dfsRank[b]; });
    }

    // Initialize PositionInLayer
    std::vector<int> posInLayer(N, 0);
    for (int L = 0; L <= maxLayer; L++) {
        for (int p = 0; p < (int)layerBlocks[L].size(); p++) {
            posInLayer[layerBlocks[L][p]] = p;
        }
    }

    // Barycenter sweeps (4 passes) to refine ordering
    for (int pass = 0; pass < 4; pass++) {
        if (pass % 2 == 0) {
            for (int L = 1; L <= maxLayer; L++) {
                std::vector<std::pair<double, size_t>> baryOrder;
                for (size_t idx : layerBlocks[L]) {
                    double bary = -1.0;
                    int count = 0; double sum = 0.0;
                    for (size_t pred : fwdPreds[idx]) {
                        if (layer[pred] < L) { sum += posInLayer[pred]; count++; }
                    }
                    if (count > 0) bary = sum / count;
                    baryOrder.push_back({ bary, idx });
                }
                std::stable_sort(baryOrder.begin(), baryOrder.end(),
                    [](const std::pair<double, size_t>& a, const std::pair<double, size_t>& b) {
                        if (a.first < 0) return false;
                        if (b.first < 0) return true;
                        return a.first < b.first;
                    });
                layerBlocks[L].clear();
                for (int p = 0; p < (int)baryOrder.size(); p++) {
                    layerBlocks[L].push_back(baryOrder[p].second);
                    posInLayer[baryOrder[p].second] = p;
                }
            }
        } else {
            for (int L = maxLayer - 1; L >= 0; L--) {
                std::vector<std::pair<double, size_t>> baryOrder;
                for (size_t idx : layerBlocks[L]) {
                    double bary = -1.0;
                    int count = 0; double sum = 0.0;
                    for (size_t succ : fwdSuccs[idx]) {
                        if (layer[succ] > L) { sum += posInLayer[succ]; count++; }
                    }
                    if (count > 0) bary = sum / count;
                    baryOrder.push_back({ bary, idx });
                }
                std::stable_sort(baryOrder.begin(), baryOrder.end(),
                    [](const std::pair<double, size_t>& a, const std::pair<double, size_t>& b) {
                        if (a.first < 0) return false;
                        if (b.first < 0) return true;
                        return a.first < b.first;
                    });
                layerBlocks[L].clear();
                for (int p = 0; p < (int)baryOrder.size(); p++) {
                    layerBlocks[L].push_back(baryOrder[p].second);
                    posInLayer[baryOrder[p].second] = p;
                }
            }
        }
    }

    // Write PositionInLayer back to blocks
    for (int L = 0; L <= maxLayer; L++) {
        for (int p = 0; p < (int)layerBlocks[L].size(); p++) {
            graph->Blocks[layerBlocks[L][p]].PositionInLayer = p;
        }
    }

    // ================================================================
    // Phase 3: Coordinate Assignment (fall-through spine alignment)
    // ================================================================

    // Y coordinates
    std::vector<int> layerY(maxLayer + 1, 0);
    int currentY = CFG_BLOCK_V_SPACING;
    for (int L = 0; L <= maxLayer; L++) {
        layerY[L] = currentY;
        int maxHeight = 0;
        for (size_t idx : layerBlocks[L]) {
            if (graph->Blocks[idx].Height > maxHeight)
                maxHeight = graph->Blocks[idx].Height;
        }
        currentY += maxHeight + CFG_BLOCK_V_SPACING;
    }

    // X coordinates: top-down, center each block under its predecessors
    // DFS ordering ensures fall-through child is first in each layer,
    // so the overlap resolver naturally pushes branch targets to the side.
    std::vector<int> finalX(N, -1);
    finalX[entryIdx] = 400;

    for (int L = 0; L <= maxLayer; L++) {
        // Place blocks: center under median predecessor
        for (size_t idx : layerBlocks[L]) {
            if (finalX[idx] >= 0) continue;

            std::vector<int> predCenters;
            for (size_t pred : fwdPreds[idx]) {
                if (finalX[pred] >= 0)
                    predCenters.push_back(finalX[pred] + graph->Blocks[pred].Width / 2);
            }

            if (!predCenters.empty()) {
                std::sort(predCenters.begin(), predCenters.end());
                int median = predCenters[predCenters.size() / 2];
                finalX[idx] = median - graph->Blocks[idx].Width / 2;
            } else {
                finalX[idx] = 400;
            }
        }

        // Resolve overlaps in this layer (left-to-right)
        for (int p = 0; p < (int)layerBlocks[L].size(); p++) {
            size_t idx = layerBlocks[L][p];
            if (finalX[idx] < CFG_BLOCK_H_SPACING)
                finalX[idx] = CFG_BLOCK_H_SPACING;
            if (p > 0) {
                size_t prevIdx = layerBlocks[L][p - 1];
                int minX = finalX[prevIdx] + graph->Blocks[prevIdx].Width + CFG_BLOCK_H_SPACING;
                if (finalX[idx] < minX)
                    finalX[idx] = minX;
            }
        }
    }

    // Bottom-up refinement (3 passes): shift parents to center over children,
    // then top-down re-align children under parents
    for (int pass = 0; pass < 3; pass++) {
        // Bottom-up: center parents over children
        for (int L = maxLayer - 1; L >= 0; L--) {
            for (size_t idx : layerBlocks[L]) {
                if (fwdSuccs[idx].empty()) continue;
                int sumCenter = 0, cnt = 0;
                for (size_t s : fwdSuccs[idx]) {
                    sumCenter += finalX[s] + graph->Blocks[s].Width / 2;
                    cnt++;
                }
                if (cnt == 0) continue;
                int targetX = sumCenter / cnt - graph->Blocks[idx].Width / 2;
                if (targetX < CFG_BLOCK_H_SPACING) targetX = CFG_BLOCK_H_SPACING;
                finalX[idx] = targetX;
            }
            // Re-resolve overlaps
            for (int p = 0; p < (int)layerBlocks[L].size(); p++) {
                size_t idx = layerBlocks[L][p];
                if (finalX[idx] < CFG_BLOCK_H_SPACING)
                    finalX[idx] = CFG_BLOCK_H_SPACING;
                if (p > 0) {
                    size_t prevIdx = layerBlocks[L][p - 1];
                    int minX = finalX[prevIdx] + graph->Blocks[prevIdx].Width + CFG_BLOCK_H_SPACING;
                    if (finalX[idx] < minX)
                        finalX[idx] = minX;
                }
            }
        }

        // Top-down: re-align children under parents (fall-through priority)
        for (int L = 1; L <= maxLayer; L++) {
            for (size_t idx : layerBlocks[L]) {
                // If this block is someone's primary (fall-through) successor,
                // pull it toward that parent's center
                int targetCenter = -1;
                for (size_t pred : fwdPreds[idx]) {
                    if (primarySucc[pred] == idx && finalX[pred] >= 0) {
                        targetCenter = finalX[pred] + graph->Blocks[pred].Width / 2;
                        break;
                    }
                }
                if (targetCenter >= 0) {
                    int targetX = targetCenter - graph->Blocks[idx].Width / 2;
                    if (targetX < CFG_BLOCK_H_SPACING) targetX = CFG_BLOCK_H_SPACING;
                    finalX[idx] = targetX;
                }
            }
            // Re-resolve overlaps
            for (int p = 0; p < (int)layerBlocks[L].size(); p++) {
                size_t idx = layerBlocks[L][p];
                if (finalX[idx] < CFG_BLOCK_H_SPACING)
                    finalX[idx] = CFG_BLOCK_H_SPACING;
                if (p > 0) {
                    size_t prevIdx = layerBlocks[L][p - 1];
                    int minX = finalX[prevIdx] + graph->Blocks[prevIdx].Width + CFG_BLOCK_H_SPACING;
                    if (finalX[idx] < minX)
                        finalX[idx] = minX;
                }
            }
        }
    }

    // Normalize: shift so leftmost block starts at margin
    int globalMinX = INT_MAX, globalMaxRight = 0;
    for (size_t i = 0; i < N; i++) {
        if (finalX[i] < globalMinX) globalMinX = finalX[i];
        int right = finalX[i] + graph->Blocks[i].Width;
        if (right > globalMaxRight) globalMaxRight = right;
    }
    int shiftX = CFG_BLOCK_H_SPACING - globalMinX;
    for (size_t i = 0; i < N; i++)
        finalX[i] += shiftX;

    // Center the graph around the entry block's spine:
    // make left and right padding symmetric around the entry block center
    int contentWidth = (globalMaxRight - globalMinX) + 2 * CFG_BLOCK_H_SPACING;
    int entryCenterX = finalX[entryIdx] + graph->Blocks[entryIdx].Width / 2;
    int leftExtent = entryCenterX;
    int rightExtent = contentWidth - entryCenterX;
    int maxExtent = max(leftExtent, rightExtent);
    int graphWidth = maxExtent * 2;
    int centerShift = graphWidth / 2 - entryCenterX;
    for (size_t i = 0; i < N; i++)
        finalX[i] += centerShift;

    // Write final coordinates
    for (size_t i = 0; i < N; i++) {
        graph->Blocks[i].X = finalX[i];
        graph->Blocks[i].Y = layerY[layer[i]];
    }

    graph->GraphWidth = graphWidth;
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
    state->ShowOverview = g_OverviewEnabled;
    state->IsDraggingOverview = false;
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
static void FocusOnBlock(HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* state, int blockIdx)
{
    if (!hWnd || !graph || !state) return;
    if (blockIdx < 0 || blockIdx >= (int)graph->Blocks.size()) {
        CenterGraphInView(hWnd, graph, state);
        return;
    }

    CFG_BASIC_BLOCK& block = graph->Blocks[blockIdx];

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // Choose zoom: try 1.0, but shrink if the block doesn't fit
    double zoom = 1.0;
    if (block.Width * zoom > clientWidth * 0.9)
        zoom = (clientWidth * 0.9) / block.Width;
    if (block.Height * zoom > clientHeight * 0.9)
        zoom = (clientHeight * 0.9) / block.Height;
    if (zoom < CFG_MIN_ZOOM) zoom = CFG_MIN_ZOOM;
    if (zoom > CFG_MAX_ZOOM) zoom = CFG_MAX_ZOOM;

    state->ZoomLevel = zoom;

    // Center the block in the viewport
    double blockCenterX = block.X + block.Width / 2.0;
    double blockCenterY = block.Y + block.Height / 2.0;
    state->PanOffsetX = clientWidth / 2.0 - blockCenterX * zoom;
    state->PanOffsetY = clientHeight / 2.0 - blockCenterY * zoom;
}

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

    FocusOnBlock(hWnd, graph, state, entryIdx);
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

// Given a graph-space point inside a block, returns the DisasmDataLines index
// of the clicked instruction line. Returns (DWORD_PTR)-1 if click is on header,
// padding, or out of range.
DWORD_PTR HitTestInstruction(CFG_BASIC_BLOCK* block, POINT graphPt)
{
    if (!block) return (DWORD_PTR)-1;

    // Check horizontal bounds
    if (graphPt.x < block->X || graphPt.x > block->X + block->Width)
        return (DWORD_PTR)-1;

    // Relative Y within block content area
    int relY = graphPt.y - (block->Y + CFG_BLOCK_PADDING);
    if (relY < 0) return (DWORD_PTR)-1;

    int lineIndex = relY / CFG_LINE_HEIGHT;

    // Account for function label header line
    int headerLines = (block->FunctionLabel[0] != '\0') ? 1 : 0;
    int instrOffset = lineIndex - headerLines;

    if (instrOffset < 0) return (DWORD_PTR)-1;  // Clicked on header

    DWORD_PTR instrIndex = block->StartIndex + instrOffset;
    if (instrIndex > block->EndIndex || instrIndex >= DisasmDataLines.size())
        return (DWORD_PTR)-1;

    return instrIndex;
}

// Resolve the target address for a branch/call instruction.
// First checks DisasmCodeFlow for the matching index, then falls back to
// parsing the address from the mnemonic text.
// Returns 0 for register-indirect jumps/calls (e.g., jmp eax, call [ebx+4]).
static DWORD_PTR ResolveTargetAddress(DWORD_PTR instrIndex)
{
    // Step 1: Scan DisasmCodeFlow for matching Current_Index
    for (size_t i = 0; i < DisasmCodeFlow.size(); i++) {
        if (DisasmCodeFlow[i].Current_Index == instrIndex) {
            return DisasmCodeFlow[i].Branch_Destination;
        }
    }

    // Step 2: Fallback - parse address from mnemonic text
    if (instrIndex < DisasmDataLines.size()) {
        const char* mnemonic = DisasmDataLines[instrIndex].GetMnemonic();
        return ExtractAddressFromMnemonic(mnemonic);
    }

    return 0;
}

// Pan the view to center a target block at the current zoom level.
// Selects the block.
static void PanToBlock(HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* state, size_t blockIndex)
{
    if (!graph || !state || blockIndex >= graph->Blocks.size()) return;

    CFG_BASIC_BLOCK& block = graph->Blocks[blockIndex];

    // Deselect all, select target
    state->SelectedBlockID = block.BlockID;

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int clientWidth = clientRect.right - clientRect.left;
    int clientHeight = clientRect.bottom - clientRect.top;

    // Center the block in the viewport at current zoom
    double blockCenterX = block.X + block.Width / 2.0;
    double blockCenterY = block.Y + block.Height / 2.0;
    state->PanOffsetX = clientWidth / 2.0 - blockCenterX * state->ZoomLevel;
    state->PanOffsetY = clientHeight / 2.0 - blockCenterY * state->ZoomLevel;

    InvalidateRect(hWnd, NULL, FALSE);
}

// Navigate the main disassembly listview to a specific DisasmDataLines index.
static void NavigateMainDisasmTo(DWORD_PTR instrIndex)
{
    HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
    if (!hDisasm) return;

    ListView_SetItemState(hDisasm, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(hDisasm, (int)instrIndex,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(hDisasm, (int)instrIndex, FALSE);
}

// Navigation history entry - stores a complete graph snapshot + view state
struct CFGNavEntry {
    CFG_GRAPH     Graph;
    CFG_VIEW_STATE ViewState;
};

static std::vector<CFGNavEntry> g_NavHistory;
static const size_t CFG_NAV_HISTORY_MAX = 16;

// Navigate into a called function: save current state, rebuild CFG for target.
// If the target address can't be resolved to a disassembly index, beep.
static void NavigateToFunction(HWND hWnd, DWORD_PTR targetAddress)
{
    if (targetAddress == 0) {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    // Find the target index in disassembly
    DWORD_PTR targetIndex = FindIndexByAddress(targetAddress);
    if (targetIndex == (DWORD_PTR)-1 || targetIndex >= DisasmDataLines.size()) {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    // Verify the target is actually code (not just closest match)
    DWORD_PTR actualAddr = GetAddressAtIndex(targetIndex);
    if (actualAddr != targetAddress) {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    // Walk backwards from the target to find the function start (banner line)
    DWORD_PTR startIndex = targetIndex;
    for (DWORD_PTR i = targetIndex; ; i--) {
        const char* mnemonic = DisasmDataLines[i].GetMnemonic();
        if (mnemonic && strncmp(mnemonic, "; ======", 8) == 0) {
            startIndex = i + 1;
            break;
        }
        if (i == 0) break;
    }

    // Save current state to navigation history
    if (g_NavHistory.size() >= CFG_NAV_HISTORY_MAX)
        g_NavHistory.erase(g_NavHistory.begin());

    CFGNavEntry entry;
    entry.Graph = g_CurrentGraph;       // Copy current graph
    entry.ViewState = g_ViewState;      // Copy current view state
    g_NavHistory.push_back(entry);

    // Clear current graph (don't free - we moved data to history)
    g_CurrentGraph.Blocks.clear();
    g_CurrentGraph.Edges.clear();
    g_CurrentGraph.EdgeRoutes.clear();
    g_CurrentGraph.AddressToBlockIndex.clear();
    g_CurrentGraph.BlockIDToIndex.clear();

    // Build new CFG from the target function
    BOOL success = BuildCFGByTracing(startIndex, &g_CurrentGraph);
    if (success) {
        LayoutCFG(&g_CurrentGraph);
        ComputeEdgeRoutes(&g_CurrentGraph);
        ComputeBlockColors(&g_CurrentGraph);
        FocusOnEntryBlock(hWnd, &g_CurrentGraph, &g_ViewState);

        // Update window title
        char title[128];
        wsprintf(title, "CFG Viewer - %s", g_CurrentGraph.FunctionName);
        SetWindowText(hWnd, title);

        InvalidateRect(hWnd, NULL, FALSE);
    } else {
        // Restore from history on failure
        if (!g_NavHistory.empty()) {
            CFGNavEntry& back = g_NavHistory.back();
            g_CurrentGraph = back.Graph;
            g_ViewState = back.ViewState;
            g_NavHistory.pop_back();
        }
        MessageBeep(MB_ICONEXCLAMATION);
    }
}

// Navigate back in history (called by Backspace key)
static void NavigateBack(HWND hWnd)
{
    if (g_NavHistory.empty()) {
        MessageBeep(MB_ICONEXCLAMATION);
        return;
    }

    // Free current graph and restore from history
    ClearCFGGraph(&g_CurrentGraph);

    CFGNavEntry& back = g_NavHistory.back();
    g_CurrentGraph = back.Graph;
    g_ViewState = back.ViewState;
    g_NavHistory.pop_back();

    // Update window title
    char title[128];
    wsprintf(title, "CFG Viewer - %s", g_CurrentGraph.FunctionName);
    SetWindowText(hWnd, title);

    InvalidateRect(hWnd, NULL, FALSE);
}

// Hover state for hand cursor on navigable instructions
static bool g_HoverOnNavigable = false;

// Context menu state: the block that was right-clicked
static DWORD_PTR g_ContextMenuBlockID = (DWORD_PTR)-1;

// Find the source block index of a conditional (T/F) edge targeting the given block.
// Returns the index into Blocks[], or (size_t)-1 if none found.
static size_t FindConditionalCaller(CFG_GRAPH* graph, DWORD_PTR targetBlockID)
{
    if (!graph) return (size_t)-1;

    for (size_t i = 0; i < graph->Edges.size(); i++) {
        CFG_EDGE& edge = graph->Edges[i];
        if (edge.TargetBlockID == targetBlockID &&
            (edge.Type == EDGE_CONDITIONAL_TRUE || edge.Type == EDGE_CONDITIONAL_FALSE)) {
            if (graph->BlockIDToIndex.count(edge.SourceBlockID))
                return graph->BlockIDToIndex[edge.SourceBlockID];
        }
    }
    return (size_t)-1;
}

// Find the first block (entry block) index. Returns (size_t)-1 if not found.
static size_t FindEntryBlockIndex(CFG_GRAPH* graph)
{
    if (!graph) return (size_t)-1;
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        if (graph->Blocks[i].IsEntryBlock)
            return i;
    }
    // Fallback: first block
    return graph->Blocks.empty() ? (size_t)-1 : 0;
}

// Find the last block (exit block or lowest-address exit) index.
// Returns (size_t)-1 if not found.
static size_t FindExitBlockIndex(CFG_GRAPH* graph)
{
    if (!graph || graph->Blocks.empty()) return (size_t)-1;

    // Prefer an exit block
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        if (graph->Blocks[i].IsExitBlock)
            return i;
    }
    // Fallback: last block in the vector
    return graph->Blocks.size() - 1;
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
            startY = srcBlock.Y + srcBlock.Height - CFG_BLOCK_PADDING - CFG_LINE_HEIGHT / 2;
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
                // Avoid other edge segments
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

                // Avoid blocks on first vertical segment (drawStartX, startY→midY)
                for (size_t bi = 0; bi < graph->Blocks.size(); bi++) {
                    CFG_BASIC_BLOCK& blk = graph->Blocks[bi];
                    if (blk.BlockID == edge.SourceBlockID || blk.BlockID == edge.TargetBlockID)
                        continue;
                    if (drawStartX >= blk.X - 5 && drawStartX <= blk.X + blk.Width + 5 &&
                        vMaxY >= blk.Y && vMinY <= blk.Y + blk.Height) {
                        int leftX = blk.X - 15;
                        int rightX = blk.X + blk.Width + 15;
                        drawStartX = (abs(startX - leftX) <= abs(startX - rightX)) ? leftX : rightX;
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

                // Avoid blocks on second vertical segment (drawEndX, midY→endY)
                for (size_t bi = 0; bi < graph->Blocks.size(); bi++) {
                    CFG_BASIC_BLOCK& blk = graph->Blocks[bi];
                    if (blk.BlockID == edge.SourceBlockID || blk.BlockID == edge.TargetBlockID)
                        continue;
                    if (drawEndX >= blk.X - 5 && drawEndX <= blk.X + blk.Width + 5 &&
                        vMaxY >= blk.Y && vMinY <= blk.Y + blk.Height) {
                        int leftX = blk.X - 15;
                        int rightX = blk.X + blk.Width + 15;
                        drawEndX = (abs(endX - leftX) <= abs(endX - rightX)) ? leftX : rightX;
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

        // Measure address column width from first line in block
        int addrColWidth = 75; // default for x86
        if (block.StartIndex < DisasmDataLines.size()) {
            char* firstAddr = DisasmDataLines[block.StartIndex].GetAddress();
            if (firstAddr && firstAddr[0]) {
                SIZE addrSize;
                GetTextExtentPoint32(hDC, firstAddr, lstrlen(firstAddr), &addrSize);
                addrColWidth = addrSize.cx + 8; // add gap between address and mnemonic
            }
        }

        for (DWORD_PTR idx = block.StartIndex; idx <= block.EndIndex && idx < DisasmDataLines.size(); idx++) {
            char* addrText = DisasmDataLines[idx].GetAddress();
            char* asmText = DisasmDataLines[idx].GetMnemonic();

            // Draw address in subdued color
            SetTextColor(hDC, addrColor);
            TextOut(hDC, block.X + CFG_BLOCK_PADDING, y, addrText, lstrlen(addrText));

            // Draw mnemonic
            SetTextColor(hDC, textColor);
            TextOut(hDC, block.X + CFG_BLOCK_PADDING + addrColWidth, y, asmText, lstrlen(asmText));

            // Draw comment if present
            char* comment = DisasmDataLines[idx].GetComments();
            if (comment && comment[0] != '\0') {
                int commentX;
                if (block.AlignedCommentCol > 0) {
                    // Use aligned column: all comments start at the same X position
                    commentX = block.X + CFG_BLOCK_PADDING + addrColWidth + block.AlignedCommentCol;
                } else {
                    // Natural position: right after the mnemonic
                    SIZE asmSize;
                    GetTextExtentPoint32(hDC, asmText, lstrlen(asmText), &asmSize);
                    commentX = block.X + CFG_BLOCK_PADDING + addrColWidth + asmSize.cx;
                }

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

    // Draw overview minimap
    if (viewState->ShowOverview)
        RenderOverview(hdcMem, hWnd, graph, viewState, &clientRect);

    // Blit to screen
    BitBlt(hDC, 0, 0, clientRect.right, clientRect.bottom, hdcMem, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

// ================================================================
// ====================  OVERVIEW MINIMAP  ========================
// ================================================================

// Computes the overview panel rectangle and graph-to-overview transform
static void GetOverviewRect(RECT* clientRect, RECT* outPanel, double* outScale, double* outOffX, double* outOffY,
                            int graphW, int graphH)
{
    outPanel->right  = clientRect->right - CFG_OVERVIEW_MARGIN;
    outPanel->bottom = clientRect->bottom - CFG_OVERVIEW_MARGIN;
    outPanel->left   = outPanel->right - CFG_OVERVIEW_WIDTH;
    outPanel->top    = outPanel->bottom - CFG_OVERVIEW_HEIGHT;

    // Available drawing area inside panel (with 10px inner padding)
    int innerW = CFG_OVERVIEW_WIDTH - 20;
    int innerH = CFG_OVERVIEW_HEIGHT - 20;

    if (graphW <= 0 || graphH <= 0) {
        *outScale = 1.0;
        *outOffX = outPanel->left + 10;
        *outOffY = outPanel->top + 10;
        return;
    }

    double scaleX = (double)innerW / graphW;
    double scaleY = (double)innerH / graphH;
    *outScale = (scaleX < scaleY) ? scaleX : scaleY;

    // Center the graph within the overview panel
    double scaledW = graphW * (*outScale);
    double scaledH = graphH * (*outScale);
    *outOffX = outPanel->left + 10 + (innerW - scaledW) / 2.0;
    *outOffY = outPanel->top + 10 + (innerH - scaledH) / 2.0;
}

void RenderOverview(HDC hDC, HWND hWnd, CFG_GRAPH* graph, CFG_VIEW_STATE* viewState, RECT* clientRect)
{
    if (!graph || graph->Blocks.empty()) return;

    RECT panel;
    double scale, offX, offY;
    GetOverviewRect(clientRect, &panel, &scale, &offX, &offY, graph->GraphWidth, graph->GraphHeight);

    // Fill background
    COLORREF bgCol = g_DarkMode ? RGB(30, 30, 30) : RGB(240, 240, 240);
    HBRUSH hBg = CreateSolidBrush(bgCol);
    FillRect(hDC, &panel, hBg);
    DeleteObject(hBg);

    // Draw 1px border
    COLORREF borderCol = g_DarkMode ? RGB(80, 80, 80) : RGB(160, 160, 160);
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, borderCol);
    HPEN hOldPen = (HPEN)SelectObject(hDC, hBorderPen);
    HBRUSH hNullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDC, hNullBrush);
    Rectangle(hDC, panel.left, panel.top, panel.right, panel.bottom);

    // Draw edges as simple 1px lines
    for (size_t i = 0; i < graph->Edges.size(); i++) {
        CFG_EDGE& edge = graph->Edges[i];
        if (!graph->BlockIDToIndex.count(edge.SourceBlockID) || !graph->BlockIDToIndex.count(edge.TargetBlockID))
            continue;

        CFG_BASIC_BLOCK& src = graph->Blocks[graph->BlockIDToIndex[edge.SourceBlockID]];
        CFG_BASIC_BLOCK& dst = graph->Blocks[graph->BlockIDToIndex[edge.TargetBlockID]];

        int sx = (int)(offX + (src.X + src.Width / 2) * scale);
        int sy = (int)(offY + (src.Y + src.Height / 2) * scale);
        int dx = (int)(offX + (dst.X + dst.Width / 2) * scale);
        int dy = (int)(offY + (dst.Y + dst.Height / 2) * scale);

        // Determine edge color
        int penIdx = 0;
        if (edge.IsBackEdge)           penIdx = 3;
        else if (edge.Type == EDGE_CONDITIONAL_TRUE)  penIdx = 1;
        else if (edge.Type == EDGE_CONDITIONAL_FALSE) penIdx = 2;
        else if (edge.Type == EDGE_CALL)              penIdx = 4;

        HPEN hEdgePen = CreatePen(PS_SOLID, 1, g_EdgeColors[penIdx]);
        SelectObject(hDC, hEdgePen);
        MoveToEx(hDC, sx, sy, NULL);
        LineTo(hDC, dx, dy);
        SelectObject(hDC, hBorderPen);
        DeleteObject(hEdgePen);
    }

    // Draw blocks as small rectangles
    for (size_t i = 0; i < graph->Blocks.size(); i++) {
        CFG_BASIC_BLOCK& blk = graph->Blocks[i];
        int bx = (int)(offX + blk.X * scale);
        int by = (int)(offY + blk.Y * scale);
        int bw = (int)(blk.Width * scale);
        int bh = (int)(blk.Height * scale);
        if (bw < 2) bw = 2;
        if (bh < 2) bh = 2;

        HBRUSH hBlkBrush = CreateSolidBrush(blk.CachedBgColor);
        RECT blkRect = { bx, by, bx + bw, by + bh };
        FillRect(hDC, &blkRect, hBlkBrush);
        DeleteObject(hBlkBrush);

        // 1px border around block
        COLORREF blkBorderCol = g_DarkMode ? RGB(120, 120, 120) : RGB(80, 80, 80);
        HPEN hBlkPen = CreatePen(PS_SOLID, 1, blkBorderCol);
        SelectObject(hDC, hBlkPen);
        SelectObject(hDC, hNullBrush);
        Rectangle(hDC, bx, by, bx + bw, by + bh);
        SelectObject(hDC, hBorderPen);
        DeleteObject(hBlkPen);
    }

    // Draw viewport rectangle
    // Compute visible area in graph coords
    double visL = -viewState->PanOffsetX / viewState->ZoomLevel;
    double visT = -viewState->PanOffsetY / viewState->ZoomLevel;
    double visR = visL + clientRect->right / viewState->ZoomLevel;
    double visB = visT + clientRect->bottom / viewState->ZoomLevel;

    // Map to overview coords
    int vpL = (int)(offX + visL * scale);
    int vpT = (int)(offY + visT * scale);
    int vpR = (int)(offX + visR * scale);
    int vpB = (int)(offY + visB * scale);

    // Clamp to panel bounds
    if (vpL < panel.left) vpL = panel.left;
    if (vpT < panel.top) vpT = panel.top;
    if (vpR > panel.right) vpR = panel.right;
    if (vpB > panel.bottom) vpB = panel.bottom;

    // Draw with bright 2px pen
    HPEN hVpPen = CreatePen(PS_SOLID, 2, RGB(255, 200, 50));
    SelectObject(hDC, hVpPen);
    SelectObject(hDC, hNullBrush);
    Rectangle(hDC, vpL, vpT, vpR, vpB);

    // Cleanup
    SelectObject(hDC, hOldPen);
    SelectObject(hDC, hOldBrush);
    DeleteObject(hBorderPen);
    DeleteObject(hVpPen);
}

BOOL HitTestOverview(POINT screenPt, RECT* clientRect)
{
    RECT panel;
    double scale, offX, offY;
    GetOverviewRect(clientRect, &panel, &scale, &offX, &offY, 1, 1); // graph dims don't matter for panel rect

    return (screenPt.x >= panel.left && screenPt.x <= panel.right &&
            screenPt.y >= panel.top  && screenPt.y <= panel.bottom);
}

void PanFromOverviewClick(CFG_VIEW_STATE* viewState, CFG_GRAPH* graph, POINT screenPt, RECT* clientRect)
{
    if (!graph || graph->Blocks.empty()) return;

    RECT panel;
    double scale, offX, offY;
    GetOverviewRect(clientRect, &panel, &scale, &offX, &offY, graph->GraphWidth, graph->GraphHeight);

    if (scale <= 0) return;

    // Convert screen point within overview to graph coordinates
    double graphX = (screenPt.x - offX) / scale;
    double graphY = (screenPt.y - offY) / scale;

    // Center the main viewport on that graph point
    viewState->PanOffsetX = clientRect->right / 2.0 - graphX * viewState->ZoomLevel;
    viewState->PanOffsetY = clientRect->bottom / 2.0 - graphY * viewState->ZoomLevel;
}

// ================================================================
// ====================  GDI RESOURCE HELPERS  =====================
// ================================================================

static void InitCFGGDIResources()
{
    // Initialize GDI+
    if (!g_GdiplusToken) {
        Gdiplus::GdiplusStartupInput gdipStartup;
        Gdiplus::GdiplusStartup(&g_GdiplusToken, &gdipStartup, NULL);
    }

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
        if (g_EdgePens[c])     DeleteObject(g_EdgePens[c]);
        if (g_ArrowPens[c])    DeleteObject(g_ArrowPens[c]);
        if (g_ArrowBrushes[c]) DeleteObject(g_ArrowBrushes[c]);
        g_EdgePens[c] = CreatePen(PS_SOLID, 2, g_EdgeColors[c]);
        g_ArrowPens[c] = CreatePen(PS_SOLID, 1, g_EdgeColors[c]);
        g_ArrowBrushes[c] = CreateSolidBrush(g_EdgeColors[c]);
    }
}

static void CleanupCFGGDIResources()
{
    g_HoverOnNavigable = false;
    if (g_hCFGFont) {
        DeleteObject(g_hCFGFont);
        g_hCFGFont = NULL;
    }
    for (int c = 0; c < 5; c++) {
        if (g_EdgePens[c])     { DeleteObject(g_EdgePens[c]);     g_EdgePens[c] = NULL; }
        if (g_ArrowPens[c])    { DeleteObject(g_ArrowPens[c]);    g_ArrowPens[c] = NULL; }
        if (g_ArrowBrushes[c]) { DeleteObject(g_ArrowBrushes[c]); g_ArrowBrushes[c] = NULL; }
    }
    if (g_GdiplusToken) {
        Gdiplus::GdiplusShutdown(g_GdiplusToken);
        g_GdiplusToken = 0;
    }
}

// ================================================================
// ====================  SHARED MESSAGE HANDLERS  ==================
// ================================================================

static LRESULT HandleCFG_MouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
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

static LRESULT HandleCFG_LButtonDown(HWND hWnd, LPARAM lParam)
{
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    // Check if clicked on overview minimap
    if (g_ViewState.ShowOverview) {
        RECT cr;
        GetClientRect(hWnd, &cr);
        if (HitTestOverview(pt, &cr)) {
            g_ViewState.IsDraggingOverview = TRUE;
            SetCapture(hWnd);
            PanFromOverviewClick(&g_ViewState, &g_CurrentGraph, pt, &cr);
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }
    }

    // Check if clicked on a block
    POINT graphPt = ScreenToGraph(&g_ViewState, pt);
    DWORD_PTR clickedBlock = HitTestBlocks(&g_CurrentGraph, graphPt);

    if (clickedBlock != (DWORD_PTR)-1) {
        g_ViewState.SelectedBlockID = clickedBlock;
        g_ViewState.DraggingBlockID = clickedBlock;
        g_ViewState.IsDraggingBlock = TRUE;
        g_ViewState.DragStartPoint = pt;

        if (g_CurrentGraph.BlockIDToIndex.count(clickedBlock)) {
            CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[clickedBlock]];
            g_ViewState.DragBlockStartX = block.X;
            g_ViewState.DragBlockStartY = block.Y;
        }

        SetCapture(hWnd);
        InvalidateRect(hWnd, NULL, FALSE);
    } else {
        g_ViewState.IsPanning = TRUE;
        g_ViewState.DragStartPoint = pt;
        g_ViewState.DragStartPanX = g_ViewState.PanOffsetX;
        g_ViewState.DragStartPanY = g_ViewState.PanOffsetY;
        SetCapture(hWnd);
    }
    return 0;
}

static LRESULT HandleCFG_MouseMove(HWND hWnd, LPARAM lParam)
{
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    if (g_ViewState.IsDraggingOverview) {
        RECT cr;
        GetClientRect(hWnd, &cr);
        PanFromOverviewClick(&g_ViewState, &g_CurrentGraph, pt, &cr);
        InvalidateRect(hWnd, NULL, FALSE);
        return 0;
    }

    if (g_ViewState.IsDraggingBlock) {
        if (g_CurrentGraph.BlockIDToIndex.count(g_ViewState.DraggingBlockID)) {
            CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[g_ViewState.DraggingBlockID]];
            int deltaX = (int)((pt.x - g_ViewState.DragStartPoint.x) / g_ViewState.ZoomLevel);
            int deltaY = (int)((pt.y - g_ViewState.DragStartPoint.y) / g_ViewState.ZoomLevel);
            block.X = g_ViewState.DragBlockStartX + deltaX;
            block.Y = g_ViewState.DragBlockStartY + deltaY;
            UpdateRoutesForDraggedBlock(&g_CurrentGraph, g_ViewState.DraggingBlockID);
            InvalidateRect(hWnd, NULL, FALSE);
        }
    } else if (g_ViewState.IsPanning) {
        g_ViewState.PanOffsetX = g_ViewState.DragStartPanX + (pt.x - g_ViewState.DragStartPoint.x);
        g_ViewState.PanOffsetY = g_ViewState.DragStartPanY + (pt.y - g_ViewState.DragStartPoint.y);
        InvalidateRect(hWnd, NULL, FALSE);
    } else {
        // Not dragging/panning - check for hover over navigable instruction
        bool wasHover = g_HoverOnNavigable;
        g_HoverOnNavigable = false;

        POINT graphPt = ScreenToGraph(&g_ViewState, pt);
        DWORD_PTR blockID = HitTestBlocks(&g_CurrentGraph, graphPt);

        if (blockID != (DWORD_PTR)-1 && g_CurrentGraph.BlockIDToIndex.count(blockID)) {
            CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[blockID]];
            DWORD_PTR instrIndex = HitTestInstruction(&block, graphPt);

            if (instrIndex != (DWORD_PTR)-1) {
                const char* mnemonic = DisasmDataLines[instrIndex].GetMnemonic();
                if (IsConditionalJump(mnemonic) || IsUnconditionalJump(mnemonic) || IsCallInstruction(mnemonic)) {
                    g_HoverOnNavigable = true;
                }
            }
        }

        if (wasHover != g_HoverOnNavigable) {
            SetCursor(LoadCursor(NULL, g_HoverOnNavigable ? IDC_HAND : IDC_ARROW));
        }
    }
    return 0;
}

static LRESULT HandleCFG_LButtonUp(HWND hWnd)
{
    if (g_ViewState.IsDraggingOverview) {
        g_ViewState.IsDraggingOverview = FALSE;
        ReleaseCapture();
        return 0;
    }

    if (g_ViewState.IsDraggingBlock) {
        g_ViewState.IsDraggingBlock = FALSE;
        g_ViewState.DraggingBlockID = (DWORD_PTR)-1;
        ReleaseCapture();
        ComputeEdgeRoutes(&g_CurrentGraph);
        InvalidateRect(hWnd, NULL, FALSE);
    }
    if (g_ViewState.IsPanning) {
        g_ViewState.IsPanning = FALSE;
        ReleaseCapture();
    }
    return 0;
}

static LRESULT HandleCFG_LButtonDblClk(HWND hWnd, LPARAM lParam)
{
    // Cancel any drag state started by the preceding WM_LBUTTONDOWN
    if (g_ViewState.IsDraggingBlock) {
        if (g_CurrentGraph.BlockIDToIndex.count(g_ViewState.DraggingBlockID)) {
            CFG_BASIC_BLOCK& dragBlock = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[g_ViewState.DraggingBlockID]];
            dragBlock.X = g_ViewState.DragBlockStartX;
            dragBlock.Y = g_ViewState.DragBlockStartY;
            UpdateRoutesForDraggedBlock(&g_CurrentGraph, g_ViewState.DraggingBlockID);
        }
        g_ViewState.IsDraggingBlock = FALSE;
        g_ViewState.DraggingBlockID = (DWORD_PTR)-1;
        ReleaseCapture();
    }
    if (g_ViewState.IsPanning) {
        g_ViewState.IsPanning = FALSE;
        ReleaseCapture();
    }

    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    POINT graphPt = ScreenToGraph(&g_ViewState, pt);
    DWORD_PTR blockID = HitTestBlocks(&g_CurrentGraph, graphPt);

    if (blockID != (DWORD_PTR)-1 && g_CurrentGraph.BlockIDToIndex.count(blockID)) {
        CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[g_CurrentGraph.BlockIDToIndex[blockID]];
        DWORD_PTR instrIndex = HitTestInstruction(&block, graphPt);

        if (instrIndex != (DWORD_PTR)-1) {
            const char* mnemonic = DisasmDataLines[instrIndex].GetMnemonic();

            if (IsConditionalJump(mnemonic) || IsUnconditionalJump(mnemonic)) {
                DWORD_PTR targetAddr = ResolveTargetAddress(instrIndex);
                if (targetAddr != 0 && g_CurrentGraph.AddressToBlockIndex.count(targetAddr)) {
                    size_t targetIdx = g_CurrentGraph.AddressToBlockIndex[targetAddr];
                    PanToBlock(hWnd, &g_CurrentGraph, &g_ViewState, targetIdx);
                } else {
                    NavigateMainDisasmTo(instrIndex);
                }
            } else if (IsCallInstruction(mnemonic)) {
                DWORD_PTR targetAddr = ResolveTargetAddress(instrIndex);
                if (targetAddr != 0) {
                    NavigateToFunction(hWnd, targetAddr);
                } else {
                    MessageBeep(MB_ICONEXCLAMATION);
                }
            } else {
                NavigateMainDisasmTo(instrIndex);
            }
        } else {
            NavigateMainDisasmTo(block.StartIndex);
        }
    }
    return 0;
}

static LRESULT HandleCFG_ContextMenu(HWND hWnd, LPARAM lParam)
{
    POINT pt;
    pt.x = GET_X_LPARAM(lParam);
    pt.y = GET_Y_LPARAM(lParam);

    if (pt.x == -1 && pt.y == -1) {
        RECT rc;
        GetClientRect(hWnd, &rc);
        pt.x = rc.right / 2;
        pt.y = rc.bottom / 2;
        ClientToScreen(hWnd, &pt);
    }

    POINT clientPt = pt;
    ScreenToClient(hWnd, &clientPt);
    POINT graphPt = ScreenToGraph(&g_ViewState, clientPt);
    g_ContextMenuBlockID = HitTestBlocks(&g_CurrentGraph, graphPt);

    bool hasConditionalCaller = false;
    if (g_ContextMenuBlockID != (DWORD_PTR)-1) {
        hasConditionalCaller = (FindConditionalCaller(&g_CurrentGraph, g_ContextMenuBlockID) != (size_t)-1);
    }

    bool hasBlock = (g_ContextMenuBlockID != (DWORD_PTR)-1);

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, hasBlock ? MF_STRING : MF_STRING | MF_GRAYED,
               IDM_CFG_SHOW_DISASM, "Show Disassembly");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, IDM_CFG_GOTO_START, "Goto Start");
    AppendMenu(hMenu, MF_STRING, IDM_CFG_GOTO_END, "Goto End");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, hasConditionalCaller ? MF_STRING : MF_STRING | MF_GRAYED,
               IDM_CFG_GOTO_CALLER, "Goto Caller");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, IDM_CFG_FIT_GRAPH, "Fit Graph to Screen");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, IDM_CFG_SAVE_IMAGE, "Save as Image...");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, g_CommentsAligned ? MF_STRING | MF_CHECKED : MF_STRING,
               IDM_CFG_ALIGN_COMMENTS, "Align Comments");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, g_OverviewEnabled ? MF_STRING | MF_CHECKED : MF_STRING,
               IDM_CFG_TOGGLE_OVERVIEW, "Show Overview");

    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);
    return 0;
}

static LRESULT HandleCFG_Command(HWND hWnd, WPARAM wParam)
{
    switch (LOWORD(wParam)) {
        case IDM_CFG_SHOW_DISASM: {
            if (g_ContextMenuBlockID != (DWORD_PTR)-1 &&
                g_CurrentGraph.BlockIDToIndex.count(g_ContextMenuBlockID)) {
                size_t idx = g_CurrentGraph.BlockIDToIndex[g_ContextMenuBlockID];
                DWORD_PTR startIndex = g_CurrentGraph.Blocks[idx].StartIndex;
                NavigateMainDisasmTo(startIndex);
                ShowDisassemblyTab();
            }
            break;
        }

        case IDM_CFG_FIT_GRAPH:
            CenterGraphInView(hWnd, &g_CurrentGraph, &g_ViewState);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case IDM_CFG_SAVE_IMAGE:
            SaveGraphToFile(hWnd, &g_CurrentGraph);
            break;

        case IDM_CFG_GOTO_CALLER: {
            if (g_ContextMenuBlockID != (DWORD_PTR)-1) {
                size_t callerIdx = FindConditionalCaller(&g_CurrentGraph, g_ContextMenuBlockID);
                if (callerIdx != (size_t)-1) {
                    PanToBlock(hWnd, &g_CurrentGraph, &g_ViewState, callerIdx);
                }
            }
            break;
        }

        case IDM_CFG_GOTO_START: {
            size_t entryIdx = FindEntryBlockIndex(&g_CurrentGraph);
            if (entryIdx != (size_t)-1) {
                PanToBlock(hWnd, &g_CurrentGraph, &g_ViewState, entryIdx);
            }
            break;
        }

        case IDM_CFG_GOTO_END: {
            size_t exitIdx = FindExitBlockIndex(&g_CurrentGraph);
            if (exitIdx != (size_t)-1) {
                PanToBlock(hWnd, &g_CurrentGraph, &g_ViewState, exitIdx);
            }
            break;
        }

        case IDM_CFG_ALIGN_COMMENTS: {
            g_CommentsAligned = !g_CommentsAligned;

            HDC hDC = GetDC(hWnd);
            HFONT hOldFont = NULL;
            if (g_hCFGFont)
                hOldFont = (HFONT)SelectObject(hDC, g_hCFGFont);

            if (g_CommentsAligned) {
                // Compute aligned comment column for each block
                for (size_t i = 0; i < g_CurrentGraph.Blocks.size(); i++) {
                    CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[i];
                    int maxMnemonicWidth = 0;

                    for (DWORD_PTR idx = block.StartIndex; idx <= block.EndIndex && idx < DisasmDataLines.size(); idx++) {
                        char* comment = DisasmDataLines[idx].GetComments();
                        if (comment && comment[0] != '\0') {
                            char* asmText = DisasmDataLines[idx].GetMnemonic();
                            SIZE asmSize;
                            GetTextExtentPoint32(hDC, asmText, lstrlen(asmText), &asmSize);
                            if (asmSize.cx > maxMnemonicWidth)
                                maxMnemonicWidth = asmSize.cx;
                        }
                    }

                    if (maxMnemonicWidth > 0)
                        block.AlignedCommentCol = maxMnemonicWidth;
                }
            } else {
                // Clear alignment for all blocks
                for (size_t i = 0; i < g_CurrentGraph.Blocks.size(); i++)
                    g_CurrentGraph.Blocks[i].AlignedCommentCol = 0;
            }

            CalculateBlockDimensions(hDC, &g_CurrentGraph);
            g_CurrentGraph.EdgeRoutesDirty = true;

            if (hOldFont)
                SelectObject(hDC, hOldFont);
            ReleaseDC(hWnd, hDC);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        case IDM_CFG_TOGGLE_OVERVIEW:
            g_OverviewEnabled = !g_OverviewEnabled;
            g_ViewState.ShowOverview = g_OverviewEnabled;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
    }
    return 0;
}

static LRESULT HandleCFG_KeyDown(HWND hWnd, WPARAM wParam, bool isChildWindow)
{
    switch (wParam) {
        case VK_HOME:
            CenterGraphInView(hWnd, &g_CurrentGraph, &g_ViewState);
            InvalidateRect(hWnd, NULL, FALSE);
            break;

        case VK_ESCAPE:
            if (isChildWindow) {
                // Embedded child: send message to main window to switch back to tab 0
                SendMessage(Main_hWnd, WM_USER + 300, 0, 0);
            } else {
                DestroyWindow(hWnd);
            }
            break;

        case VK_BACK:
            NavigateBack(hWnd);
            break;

        case VK_ADD:
        case VK_OEM_PLUS: {
            RECT rect;
            GetClientRect(hWnd, &rect);
            POINT center = {rect.right / 2, rect.bottom / 2};
            ZoomAtPoint(&g_ViewState, center, CFG_ZOOM_FACTOR);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        case VK_SUBTRACT:
        case VK_OEM_MINUS: {
            RECT rect;
            GetClientRect(hWnd, &rect);
            POINT center = {rect.right / 2, rect.bottom / 2};
            ZoomAtPoint(&g_ViewState, center, 1.0 / CFG_ZOOM_FACTOR);
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        case 'M':
            g_OverviewEnabled = !g_OverviewEnabled;
            g_ViewState.ShowOverview = g_OverviewEnabled;
            InvalidateRect(hWnd, NULL, FALSE);
            break;
    }
    return 0;
}

// ================================================================
// ====================  DIALOG PROCEDURE  ========================
// ================================================================

BOOL CALLBACK CFGViewerDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
        case WM_INITDIALOG: {
            InitCFGGDIResources();
            InitCFGViewState(&g_ViewState);

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
            return 1;

        case WM_MOUSEWHEEL:
            return (BOOL)HandleCFG_MouseWheel(hWnd, wParam, lParam);

        case WM_LBUTTONDOWN:
            return (BOOL)HandleCFG_LButtonDown(hWnd, lParam);

        case WM_MOUSEMOVE:
            return (BOOL)HandleCFG_MouseMove(hWnd, lParam);

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(LoadCursor(NULL, g_HoverOnNavigable ? IDC_HAND : IDC_ARROW));
                return TRUE;
            }
            break;
        }

        case WM_LBUTTONUP:
            return (BOOL)HandleCFG_LButtonUp(hWnd);

        case WM_LBUTTONDBLCLK:
            return (BOOL)HandleCFG_LButtonDblClk(hWnd, lParam);

        case WM_CONTEXTMENU:
            return (BOOL)HandleCFG_ContextMenu(hWnd, lParam);

        case WM_COMMAND:
            return (BOOL)HandleCFG_Command(hWnd, wParam);

        case WM_SIZE: {
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;
        }

        case WM_KEYDOWN:
            return (BOOL)HandleCFG_KeyDown(hWnd, wParam, false);

        case WM_CLOSE: {
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_DESTROY: {
            FreeCFGGraph(&g_CurrentGraph);
            for (size_t i = 0; i < g_NavHistory.size(); i++) {
                ClearCFGGraph(&g_NavHistory[i].Graph);
            }
            g_NavHistory.clear();
            CleanupCFGGDIResources();
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
            return (DWORD_PTR)StringToQword(addrStr);
        }
    }
    return 0;
}

BOOL BuildCFGByTracing(DWORD_PTR startIndex, CFG_GRAPH* outGraph)
{
    if (!outGraph || startIndex >= DisasmDataLines.size()) return FALSE;

    ClearCFGGraph(outGraph);

    DWORD_PTR startAddr = GetAddressAtIndex(startIndex);
    outGraph->FunctionStart = startAddr;

    // Try to resolve function name from fFunctionInfo
    bool nameFound = false;
    for (size_t fi = 0; fi < fFunctionInfo.size(); fi++) {
        if (fFunctionInfo[fi].FunctionStart == startAddr) {
            if (fFunctionInfo[fi].FunctionName[0] != '\0') {
                strncpy(outGraph->FunctionName, fFunctionInfo[fi].FunctionName, 63);
                outGraph->FunctionName[63] = '\0';
            } else {
                if (LoadedPe64)
                    wsprintf(outGraph->FunctionName, "Proc_%08X%08X", (DWORD)(startAddr>>32), (DWORD)startAddr);
                else
                    wsprintf(outGraph->FunctionName, "Proc_%08X", (DWORD)startAddr);
            }
            nameFound = true;
            break;
        }
    }
    // Fallback: check for a banner line right before startIndex in DisasmDataLines
    if (!nameFound && startIndex > 0) {
        const char* prevMnem = DisasmDataLines[startIndex - 1].GetMnemonic();
        if (prevMnem && strncmp(prevMnem, "; ======", 8) == 0) {
            const char* s = prevMnem + 9;
            const char* e = s;
            while (*e && strncmp(e, " ======", 7) != 0) e++;
            size_t len = e - s;
            if (len > 0 && len < 64) {
                memcpy(outGraph->FunctionName, s, len);
                outGraph->FunctionName[len] = '\0';
            }
            nameFound = true;
        }
    }
    if (!nameFound) {
        if (LoadedPe64)
            wsprintf(outGraph->FunctionName, "Code at %08X%08X", (DWORD)(startAddr>>32), (DWORD)startAddr);
        else
            wsprintf(outGraph->FunctionName, "Code at %08X", (DWORD)startAddr);
    }

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

            // Check for proc marker lines (empty address = comment/separator)
            const char* addrStr = DisasmDataLines[idx].GetAddress();
            if (!addrStr || addrStr[0] == '\0') {
                // If this is a function banner, stop — we've crossed into another function
                const char* markerText = DisasmDataLines[idx].GetMnemonic();
                if (markerText && strncmp(markerText, "; ======", 8) == 0) {
                    break;  // Hit a function boundary, stop tracing
                }
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
                // CALL does NOT split blocks — the called function returns
                // and execution continues at the next instruction.
                // Only noreturn functions end the block.
                if (strstr(mnemonic, "ExitProcess") || strstr(mnemonic, "ExitThread") ||
                    strstr(mnemonic, "TerminateProcess") || strstr(mnemonic, "TerminateThread") ||
                    strstr(mnemonic, "FatalExit") || strstr(mnemonic, "abort")) {
                    break;  // Noreturn — stop tracing this path
                }

                // Regular call (local or API) — just continue
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
        }

        block.EndIndex = endIdx;
        block.EndAddress = GetAddressAtIndex(endIdx);
        block.IsEntryBlock = (blockStartIdx == startIndex);

        const char* lastMnemonic = DisasmDataLines[endIdx].GetMnemonic();
        block.IsExitBlock = IsReturnInstruction(lastMnemonic) ||
            (IsCallInstruction(lastMnemonic) && (strstr(lastMnemonic, "ExitProcess") ||
             strstr(lastMnemonic, "ExitThread") || strstr(lastMnemonic, "TerminateProcess") ||
             strstr(lastMnemonic, "TerminateThread") || strstr(lastMnemonic, "FatalExit") ||
             strstr(lastMnemonic, "abort")));

        // Check if this block starts a known function
        block.FunctionLabel[0] = '\0';
        bool foundLabel = false;

        // First, check fFunctionInfo
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

        // Fallback: check for a banner line in DisasmDataLines right before this block
        if (!foundLabel && block.StartIndex > 0) {
            DWORD_PTR prevIdx = block.StartIndex - 1;
            const char* prevMnem = DisasmDataLines[prevIdx].GetMnemonic();
            if (prevMnem && strncmp(prevMnem, "; ======", 8) == 0) {
                const char* s = prevMnem + 9; // skip "; ====== "
                const char* e = s;
                while (*e && strncmp(e, " ======", 7) != 0) e++;
                size_t len = e - s;
                if (len > 0 && len < 64) {
                    memcpy(block.FunctionLabel, s, len);
                    block.FunctionLabel[len] = '\0';
                }
                foundLabel = true;
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
    // If window already exists, destroy it so we rebuild for the new function
    if (g_hCFGViewerWnd && IsWindow(g_hCFGViewerWnd)) {
        DestroyWindow(g_hCFGViewerWnd);
        g_hCFGViewerWnd = NULL;
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

    // Get the currently selected line in the disassembly listview
    DWORD_PTR selectedItem = SendMessage(hDisasm, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_FOCUSED);
    if (selectedItem == (DWORD_PTR)-1 || selectedItem >= DisasmDataLines.size()) {
        selectedItem = 0;
    }

    // Walk backwards from the selected line to find the function start (banner line)
    DWORD_PTR startIndex = selectedItem;
    for (DWORD_PTR i = selectedItem; ; i--) {
        const char* mnemonic = DisasmDataLines[i].GetMnemonic();
        if (mnemonic && strncmp(mnemonic, "; ======", 8) == 0) {
            // Found a function banner — the function starts at the next line
            startIndex = i + 1;
            break;
        }
        if (i == 0) break;  // unsigned — check after processing index 0
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

// Returns the CFG viewer window handle for message loop integration.
// Now returns NULL since the embedded child window doesn't need IsDialogMessage.
HWND GetCFGViewerWindow()
{
    return NULL;
}

// ================================================================
// ====================  EMBEDDED CHILD WINDOW  ====================
// ================================================================

static bool g_CFGChildClassRegistered = false;

void RegisterCFGChildClass(HINSTANCE hInst)
{
    if (g_CFGChildClassRegistered) return;

    WNDCLASSA wc = {0};
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CFGChildWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "PVDasmCFGChild";
    RegisterClassA(&wc);
    g_CFGChildClassRegistered = true;
}

LRESULT CALLBACK CFGChildWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
        case WM_CREATE:
            InitCFGGDIResources();
            InitCFGViewState(&g_ViewState);
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hDC = BeginPaint(hWnd, &ps);
            if (g_CFGGraphValid) {
                RenderCFG(hWnd, hDC, &g_CurrentGraph, &g_ViewState);
            } else {
                // Draw placeholder
                RECT rc;
                GetClientRect(hWnd, &rc);
                COLORREF bgColor = g_DarkMode ? g_DarkBkColor : GetSysColor(COLOR_WINDOW);
                COLORREF txColor = g_DarkMode ? RGB(140, 140, 140) : RGB(128, 128, 128);
                HBRUSH hBg = CreateSolidBrush(bgColor);
                FillRect(hDC, &rc, hBg);
                DeleteObject(hBg);
                SetBkMode(hDC, TRANSPARENT);
                SetTextColor(hDC, txColor);
                HFONT hOld = (HFONT)SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
                DrawText(hDC, "No graph loaded - use View > Control Flow Graph", -1, &rc,
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(hDC, hOld);
            }
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_MOUSEWHEEL:
            if (g_CFGGraphValid) return HandleCFG_MouseWheel(hWnd, wParam, lParam);
            break;

        case WM_LBUTTONDOWN:
            SetFocus(hWnd);
            if (g_CFGGraphValid) return HandleCFG_LButtonDown(hWnd, lParam);
            break;

        case WM_MOUSEMOVE:
            if (g_CFGGraphValid) return HandleCFG_MouseMove(hWnd, lParam);
            break;

        case WM_LBUTTONUP:
            if (g_CFGGraphValid) return HandleCFG_LButtonUp(hWnd);
            break;

        case WM_LBUTTONDBLCLK:
            if (g_CFGGraphValid) return HandleCFG_LButtonDblClk(hWnd, lParam);
            break;

        case WM_CONTEXTMENU:
            if (g_CFGGraphValid) return HandleCFG_ContextMenu(hWnd, lParam);
            break;

        case WM_COMMAND:
            if (g_CFGGraphValid) return HandleCFG_Command(hWnd, wParam);
            break;

        case WM_SETCURSOR:
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(LoadCursor(NULL, g_HoverOnNavigable ? IDC_HAND : IDC_ARROW));
                return TRUE;
            }
            break;

        case WM_SIZE:
            InvalidateRect(hWnd, NULL, FALSE);
            return 0;

        case WM_KEYDOWN:
            return HandleCFG_KeyDown(hWnd, wParam, true);

        case WM_DESTROY:
            CleanupCFGGDIResources();
            return 0;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void LoadCFGForCurrentFunction_Embedded()
{
    extern bool DisassemblerReady;
    if (!DisassemblerReady) return;
    if (DisasmDataLines.empty()) return;

    // Get currently selected line from disassembly listview
    HWND hDisasm = GetDlgItem(Main_hWnd, IDC_DISASM);
    if (!hDisasm) return;

    DWORD_PTR selectedItem = SendMessage(hDisasm, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_FOCUSED);
    if (selectedItem == (DWORD_PTR)-1 || selectedItem >= DisasmDataLines.size()) {
        selectedItem = 0;
    }

    // Walk backwards to find function banner
    DWORD_PTR startIndex = selectedItem;
    for (DWORD_PTR i = selectedItem; ; i--) {
        const char* mnemonic = DisasmDataLines[i].GetMnemonic();
        if (mnemonic && strncmp(mnemonic, "; ======", 8) == 0) {
            startIndex = i + 1;
            break;
        }
        if (i == 0) break;
    }

    // Free previous graph
    FreeCFGGraph(&g_CurrentGraph);
    // Clear navigation history
    for (size_t i = 0; i < g_NavHistory.size(); i++) {
        ClearCFGGraph(&g_NavHistory[i].Graph);
    }
    g_NavHistory.clear();

    BOOL success = BuildCFGByTracing(startIndex, &g_CurrentGraph);
    if (success) {
        LayoutCFG(&g_CurrentGraph);
        ComputeEdgeRoutes(&g_CurrentGraph);
        ComputeBlockColors(&g_CurrentGraph);
        g_CFGGraphValid = true;

        // Find the block containing the selected disassembly line
        int focusBlockIdx = -1;
        for (size_t i = 0; i < g_CurrentGraph.Blocks.size(); i++) {
            if (selectedItem >= g_CurrentGraph.Blocks[i].StartIndex &&
                selectedItem <= g_CurrentGraph.Blocks[i].EndIndex) {
                focusBlockIdx = (int)i;
                break;
            }
        }
        // Fall back to entry block if selected line isn't in any block
        if (focusBlockIdx < 0) {
            for (size_t i = 0; i < g_CurrentGraph.Blocks.size(); i++) {
                if (g_CurrentGraph.Blocks[i].IsEntryBlock) {
                    focusBlockIdx = (int)i;
                    break;
                }
            }
        }

        // Select the focused block (blue border highlight)
        if (focusBlockIdx >= 0)
            g_ViewState.SelectedBlockID = g_CurrentGraph.Blocks[focusBlockIdx].BlockID;

        HWND hChild = GetDlgItem(Main_hWnd, IDC_CFG_CHILD);
        if (hChild) {
            FocusOnBlock(hChild, &g_CurrentGraph, &g_ViewState, focusBlockIdx);
            InvalidateRect(hChild, NULL, FALSE);
        }
    } else {
        g_CFGGraphValid = false;
        HWND hChild = GetDlgItem(Main_hWnd, IDC_CFG_CHILD);
        if (hChild) InvalidateRect(hChild, NULL, FALSE);
    }
}

void RefreshCFGLabels()
{
    if (!g_CFGGraphValid) return;

    // Update FunctionLabel for all blocks from fFunctionInfo
    for (size_t i = 0; i < g_CurrentGraph.Blocks.size(); i++) {
        CFG_BASIC_BLOCK& block = g_CurrentGraph.Blocks[i];
        for (size_t fi = 0; fi < fFunctionInfo.size(); fi++) {
            if (fFunctionInfo[fi].FunctionStart == block.StartAddress) {
                if (fFunctionInfo[fi].FunctionName[0] != '\0')
                    strncpy(block.FunctionLabel, fFunctionInfo[fi].FunctionName, 63);
                else if (LoadedPe64)
                    wsprintf(block.FunctionLabel, "Proc_%08X%08X", (DWORD)(block.StartAddress>>32), (DWORD)block.StartAddress);
                else
                    wsprintf(block.FunctionLabel, "Proc_%08X", (DWORD)block.StartAddress);
                block.FunctionLabel[63] = '\0';
                break;
            }
        }
    }

    // Also update graph-level function name
    for (size_t fi = 0; fi < fFunctionInfo.size(); fi++) {
        if (fFunctionInfo[fi].FunctionStart == g_CurrentGraph.FunctionStart) {
            if (fFunctionInfo[fi].FunctionName[0] != '\0') {
                strncpy(g_CurrentGraph.FunctionName, fFunctionInfo[fi].FunctionName, 63);
                g_CurrentGraph.FunctionName[63] = '\0';
            } else {
                if (LoadedPe64)
                    wsprintf(g_CurrentGraph.FunctionName, "Proc_%08X%08X", (DWORD)(g_CurrentGraph.FunctionStart>>32), (DWORD)g_CurrentGraph.FunctionStart);
                else
                    wsprintf(g_CurrentGraph.FunctionName, "Proc_%08X", (DWORD)g_CurrentGraph.FunctionStart);
            }
            break;
        }
    }

    // Invalidate graph child to repaint with updated labels
    HWND hChild = GetDlgItem(Main_hWnd, IDC_CFG_CHILD);
    if (hChild) InvalidateRect(hChild, NULL, FALSE);
}

void ClearEmbeddedCFG()
{
    FreeCFGGraph(&g_CurrentGraph);
    for (size_t i = 0; i < g_NavHistory.size(); i++) {
        ClearCFGGraph(&g_NavHistory[i].Graph);
    }
    g_NavHistory.clear();
    g_CFGGraphValid = false;
    g_HoverOnNavigable = false;

    HWND hChild = GetDlgItem(Main_hWnd, IDC_CFG_CHILD);
    if (hChild) InvalidateRect(hChild, NULL, FALSE);
}
