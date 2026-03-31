/*
    Debugger.cpp - Win32 Debug API integration for PVDasm

    Implements process debugging: start/attach, breakpoints, stepping,
    register inspection, and memory operations using the Windows Debug API.

    Written by Shany Golan.
    Copyright (C) 2011-2026. By Shany Golan.
*/

#include "MappedFile.h"
#include "Debugger.h"
#include <tlhelp32.h>

// External globals defined in FileMap.cpp / Functions.cpp
extern DisasmDataArray      DisasmDataLines;
extern bool                 g_DarkMode;
extern HBRUSH               g_hDarkBrush;
extern COLORREF             g_DarkBkColor;
extern COLORREF             g_DarkTextColor;
extern bool                 LoadedPe;
extern bool                 LoadedPe64;
extern bool                 DisassemblerReady;
extern bool                 DisasmIsWorking;
extern DISASM_OPTIONS       disop;
extern IMAGE_NT_HEADERS*    NTheader;
extern HWND                 hWndTB;
extern bool                 g_CodeMapVisible;
extern bool                 g_FlowArrowsVisible;

// Functions from FileMap.cpp / Disasm.cpp
extern void CloseLoadedFile(HWND hWnd);
extern bool FilesInMemory;
extern IMAGE_NT_HEADERS64*  NTheader64;
extern HANDLE               hFile, hFileMap;
extern char*                OrignalData;
extern char*                Data;
extern DWORD_PTR            hFileSize;
extern char                 szFileName[MAX_PATH];

// ================================================================
// =======================  GLOBAL VARIABLES  =====================
// ================================================================

DEBUGGER_STATE                      g_DbgState = DBG_STATE_IDLE;
DEBUG_PROCESS_INFO                  g_DbgProcess = {0};
std::vector<DEBUG_THREAD_INFO>      g_DbgThreads;
std::vector<DEBUG_MODULE_INFO>      g_DbgModules;
std::vector<DEBUG_BREAKPOINT>       g_DbgBreakpoints;
std::vector<DEBUG_MEMORY_SNAPSHOT>  g_DbgSnapshots;
DEBUG_REGISTERS                     g_DbgRegisters = {0};
DEBUG_REGISTERS                     g_DbgPrevRegisters = {0};
HANDLE                              g_hDebugThread = NULL;
DWORD                               g_dwDebugThreadId = 0;
HWND                                g_hRegisterDlg = NULL;
HWND                                g_hThreadsDlg = NULL;
HWND                                g_hBreakpointsDlg = NULL;
DWORD_PTR                           g_dwCurrentEIP = 0;
bool                                g_bDebuggerActive = false;
HANDLE                              g_hContinueEvent = NULL;
CRITICAL_SECTION                    g_csBreakpoints;
DWORD_PTR                           g_dwRebaseDelta = 0;

// Clean up leftover temp files from previous debug sessions
static void DbgCleanupOldTempFiles()
{
    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);

    char pattern[MAX_PATH];
    wsprintf(pattern, "%spvdasm_dbg_*", tempDir);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        char fullPath[MAX_PATH];
        wsprintf(fullPath, "%s%s", tempDir, fd.cFileName);
        // DeleteFile will fail silently if the file is still locked by a running process
        DeleteFileA(fullPath);
    } while (FindNextFileA(hFind, &fd));

    FindClose(hFind);
}

// Internal state
static HWND     s_hMainWnd = NULL;
static int      s_nInitialBreakpoints = 2;      // WOW64 fires 2 system breakpoints (ntdll64 + ntdll32)
static DWORD_PTR s_dwReEnableBPAddr = 0;        // Address of breakpoint to re-enable after single step
static DWORD    s_dwContinueStatus = DBG_CONTINUE;
static DWORD    s_dwPausedThreadId = 0;         // Thread ID of the thread that caused the pause
static bool     s_bDetachRequested = false;     // Signal debug thread to detach
static char     s_szTempExePath[MAX_PATH] = {0}; // Temp copy of EXE for debugging
// Cached thread display info (populated once at pause time)
struct THREAD_CACHE_ENTRY {
    DWORD       dwThreadId;
    DWORD_PTR   dwEip;
    char        szName[64];
    char        szModule[64];
};
static std::vector<THREAD_CACHE_ENTRY> s_threadCache;

// Parameters passed from UI thread to debug thread
static char     s_szStartCmdLine[MAX_PATH * 2] = {0};
static char     s_szStartWorkDir[MAX_PATH] = {0};
static DWORD    s_dwAttachPid = 0;  // PID to attach to (0 = start process instead)

// ================================================================
// ===================  PROCESS CONTROL  ==========================
// ================================================================

BOOL DbgStartProcess(HWND hMainWnd, const char* szExePath, const char* szCmdLine, const char* szWorkDir)
{
    if (g_bDebuggerActive)
        return FALSE;

    // Clean up temp files from previous sessions (silently skips locked ones)
    DbgCleanupOldTempFiles();

    s_szTempExePath[0] = 0;

    // If the file is mapped in memory (loaded+disassembled), copy it to a temp
    // file to avoid sharing violation with PVDasm's PAGE_READWRITE mapping.
    // If not mapped, launch the EXE directly from its original path.
    if (OrignalData && hFileSize > 0) {
        char tempDir[MAX_PATH];
        GetTempPathA(MAX_PATH, tempDir);

        const char* exeName = strrchr(szExePath, '\\');
        if (!exeName) exeName = strrchr(szExePath, '/');
        exeName = exeName ? exeName + 1 : szExePath;

        wsprintf(s_szTempExePath, "%spvdasm_dbg_%u_%s", tempDir, GetTickCount(), exeName);

        HANDLE hTempFile = CreateFileA(s_szTempExePath, GENERIC_WRITE, 0, NULL,
                                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hTempFile == INVALID_HANDLE_VALUE) {
            char err[256];
            wsprintf(err, "Failed to create temp file: error %d", GetLastError());
            OutDebug(hMainWnd, err);
            s_szTempExePath[0] = 0;
            return FALSE;
        }

        DWORD bytesWritten;
        BOOL written = WriteFile(hTempFile, OrignalData, (DWORD)hFileSize, &bytesWritten, NULL);
        CloseHandle(hTempFile);

        if (!written || bytesWritten != (DWORD)hFileSize) {
            OutDebug(hMainWnd, "Failed to write temp file");
            DeleteFileA(s_szTempExePath);
            s_szTempExePath[0] = 0;
            return FALSE;
        }
    }

    // Use temp copy if available, otherwise launch from original path
    const char* launchPath = s_szTempExePath[0] ? s_szTempExePath : szExePath;

    // Prepare command line and working directory for the debug thread
    if (szCmdLine && szCmdLine[0]) {
        wsprintf(s_szStartCmdLine, "\"%s\" %s", launchPath, szCmdLine);
    } else {
        wsprintf(s_szStartCmdLine, "\"%s\"", launchPath);
    }

    // Use original exe's directory as working directory if none specified
    s_szStartWorkDir[0] = 0;
    if (szWorkDir && szWorkDir[0]) {
        lstrcpyn(s_szStartWorkDir, szWorkDir, MAX_PATH);
    } else {
        lstrcpyn(s_szStartWorkDir, szExePath, MAX_PATH);
        char* lastSlash = strrchr(s_szStartWorkDir, '\\');
        if (!lastSlash) lastSlash = strrchr(s_szStartWorkDir, '/');
        if (lastSlash) *(lastSlash + 1) = '\0';
        else s_szStartWorkDir[0] = 0;
    }

    // Store info
    memset(&g_DbgProcess, 0, sizeof(g_DbgProcess));
    g_DbgProcess.bAttached = false;
    lstrcpyn(g_DbgProcess.szExePath, szExePath, MAX_PATH);
    if (szCmdLine) lstrcpyn(g_DbgProcess.szCmdLine, szCmdLine, MAX_PATH);
    if (szWorkDir) lstrcpyn(g_DbgProcess.szWorkDir, szWorkDir, MAX_PATH);

    s_hMainWnd = hMainWnd;
    s_nInitialBreakpoints = 2;
    s_dwReEnableBPAddr = 0;
    g_bDebuggerActive = true;
    g_DbgState = DBG_STATE_RUNNING;

    if (!g_hContinueEvent)
        g_hContinueEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_hContinueEvent);

    InitializeCriticalSection(&g_csBreakpoints);

    // CreateProcess MUST be called from the debug thread (same thread as WaitForDebugEvent)
    g_hDebugThread = CreateThread(NULL, 0, DebugEventLoop, NULL, 0, &g_dwDebugThreadId);

    PostMessage(hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)g_DbgState, 0);
    return TRUE;
}

// Enable SeDebugPrivilege so we can attach to processes we don't own
static void DbgEnableDebugPrivilege()
{
    HANDLE hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        TOKEN_PRIVILEGES tp;
        if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
            tp.PrivilegeCount = 1;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
        }
        CloseHandle(hToken);
    }
}

BOOL DbgAttachToProcess(HWND hMainWnd, DWORD dwProcessId)
{
    if (g_bDebuggerActive)
        return FALSE;

    DbgEnableDebugPrivilege();

    memset(&g_DbgProcess, 0, sizeof(g_DbgProcess));
    g_DbgProcess.dwProcessId = dwProcessId;
    g_DbgProcess.bAttached = true;
    s_dwAttachPid = dwProcessId;

    // Try to get EXE path early (before debug thread starts)
    {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwProcessId);
        if (hProc) {
            DWORD size = MAX_PATH;
            QueryFullProcessImageNameA(hProc, 0, g_DbgProcess.szExePath, &size);
            CloseHandle(hProc);
        }
    }

    s_hMainWnd = hMainWnd;
    s_nInitialBreakpoints = 2;
    s_dwReEnableBPAddr = 0;
    g_bDebuggerActive = true;
    g_DbgState = DBG_STATE_RUNNING;

    if (!g_hContinueEvent)
        g_hContinueEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_hContinueEvent);

    InitializeCriticalSection(&g_csBreakpoints);

    // DebugActiveProcess must be called on the same thread as WaitForDebugEvent
    g_hDebugThread = CreateThread(NULL, 0, DebugEventLoop, NULL, 0, &g_dwDebugThreadId);

    PostMessage(hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)g_DbgState, 0);
    return TRUE;
}

BOOL DbgDetachFromProcess()
{
    if (!g_bDebuggerActive)
        return FALSE;

    // Remove all breakpoints before detaching
    EnterCriticalSection(&g_csBreakpoints);
    for (size_t i = 0; i < g_DbgBreakpoints.size(); i++) {
        if (g_DbgBreakpoints[i].bEnabled) {
            BYTE orig = g_DbgBreakpoints[i].bOriginalByte;
            SIZE_T written;
            WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)g_DbgBreakpoints[i].dwAddress, &orig, 1, &written);
            FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)g_DbgBreakpoints[i].dwAddress, 1);
        }
    }
    g_DbgBreakpoints.clear();
    LeaveCriticalSection(&g_csBreakpoints);

    // Signal the debug thread to detach (it must call DebugActiveProcessStop
    // on the same thread that called DebugActiveProcess/CreateProcess)
    s_bDetachRequested = true;

    // If paused, resume the debug loop so it can process the detach
    if (g_DbgState == DBG_STATE_PAUSED) {
        s_dwContinueStatus = DBG_CONTINUE;
        g_DbgState = DBG_STATE_RUNNING;
        SetEvent(g_hContinueEvent);
    }

    return TRUE;
}

BOOL DbgTerminateProcess()
{
    if (!g_bDebuggerActive)
        return FALSE;

    // If paused, resume the debug loop so it can process the exit event
    if (g_DbgState == DBG_STATE_PAUSED) {
        s_dwContinueStatus = DBG_CONTINUE;
        g_DbgState = DBG_STATE_RUNNING;
        SetEvent(g_hContinueEvent);
    }

    TerminateProcess(g_DbgProcess.hProcess, 0);
    // The debug loop will receive EXIT_PROCESS_DEBUG_EVENT and call DbgCleanup
    return TRUE;
}

BOOL DbgPauseProcess()
{
    if (g_DbgState != DBG_STATE_RUNNING)
        return FALSE;

    return DebugBreakProcess(g_DbgProcess.hProcess);
}

BOOL DbgResumeProcess()
{
    if (g_DbgState != DBG_STATE_PAUSED)
        return FALSE;

    // Re-enable any permanent breakpoint that was temporarily disabled
    if (s_dwReEnableBPAddr != 0) {
        EnterCriticalSection(&g_csBreakpoints);
        DEBUG_BREAKPOINT* bp = DbgFindBreakpoint(s_dwReEnableBPAddr);
        if (bp && bp->bEnabled) {
            BYTE int3 = 0xCC;
            SIZE_T written;
            WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)bp->dwAddress, &int3, 1, &written);
            FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)bp->dwAddress, 1);
        }
        LeaveCriticalSection(&g_csBreakpoints);
        s_dwReEnableBPAddr = 0;
    }

    s_dwContinueStatus = DBG_CONTINUE;
    g_DbgState = DBG_STATE_RUNNING;
    g_dwCurrentEIP = 0;
    SetEvent(g_hContinueEvent);

    PostMessage(s_hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)g_DbgState, 0);
    return TRUE;
}

void DbgCleanup()
{
    g_bDebuggerActive = false;
    g_DbgState = DBG_STATE_IDLE;
    g_dwCurrentEIP = 0;
    g_dwRebaseDelta = 0;
    s_dwReEnableBPAddr = 0;
    s_dwPausedThreadId = 0;
    s_bDetachRequested = false;
    s_threadCache.clear();

    // Reset tab width and window title
    if (s_hMainWnd) {
        HWND hTab = GetDlgItem(s_hMainWnd, IDC_TAB_MAIN);
        if (hTab) {
            SendMessage(hTab, TCM_SETITEMSIZE, 0, MAKELPARAM(100, 20));
            TCITEM tie = {0};
            tie.mask = TCIF_TEXT;
            tie.pszText = (LPSTR)"Disassembly";
            TabCtrl_SetItem(hTab, 0, &tie);
            InvalidateRect(hTab, NULL, TRUE);
        }
        SetWindowText(s_hMainWnd, PVDASM);
    }

    if (g_DbgProcess.hProcess) {
        CloseHandle(g_DbgProcess.hProcess);
    }
    if (g_DbgProcess.hMainThread) {
        CloseHandle(g_DbgProcess.hMainThread);
    }
    memset(&g_DbgProcess, 0, sizeof(g_DbgProcess));

    g_DbgThreads.clear();
    g_DbgModules.clear();

    EnterCriticalSection(&g_csBreakpoints);
    g_DbgBreakpoints.clear();
    LeaveCriticalSection(&g_csBreakpoints);
    DeleteCriticalSection(&g_csBreakpoints);

    // Free memory snapshots
    for (size_t i = 0; i < g_DbgSnapshots.size(); i++) {
        if (g_DbgSnapshots[i].pData)
            free(g_DbgSnapshots[i].pData);
    }
    g_DbgSnapshots.clear();

    memset(&g_DbgRegisters, 0, sizeof(g_DbgRegisters));
    memset(&g_DbgPrevRegisters, 0, sizeof(g_DbgPrevRegisters));

    if (g_hDebugThread) {
        CloseHandle(g_hDebugThread);
        g_hDebugThread = NULL;
    }

    // Delete temp copy of EXE
    if (s_szTempExePath[0]) {
        DeleteFileA(s_szTempExePath);
        s_szTempExePath[0] = 0;
    }

    if (s_hMainWnd)
        PostMessage(s_hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)DBG_STATE_IDLE, 0);
}

// ================================================================
// ====================  HELPER FUNCTIONS  ========================
// ================================================================

// Get thread handle from thread ID
static HANDLE DbgGetThreadHandle(DWORD dwThreadId)
{
    if (dwThreadId == g_DbgProcess.dwMainThreadId)
        return g_DbgProcess.hMainThread;

    for (size_t i = 0; i < g_DbgThreads.size(); i++) {
        if (g_DbgThreads[i].dwThreadId == dwThreadId)
            return g_DbgThreads[i].hThread;
    }
    return NULL;
}

// Helper: get 32-bit context (works for both WOW64 and native 32-bit targets)
static BOOL DbgGet32Context(HANDLE hThread, WOW64_CONTEXT* ctx)
{
#ifdef _M_X64
    if (g_DbgProcess.bIsWow64) {
        ctx->ContextFlags = WOW64_CONTEXT_FULL | WOW64_CONTEXT_DEBUG_REGISTERS;
        return Wow64GetThreadContext(hThread, ctx);
    } else {
        // 64-bit process: use regular CONTEXT, copy to WOW64_CONTEXT for uniform API
        CONTEXT ctx64 = {0};
        ctx64.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
        if (!GetThreadContext(hThread, &ctx64)) return FALSE;
        ctx->Eax = (DWORD)ctx64.Rax; ctx->Ebx = (DWORD)ctx64.Rbx;
        ctx->Ecx = (DWORD)ctx64.Rcx; ctx->Edx = (DWORD)ctx64.Rdx;
        ctx->Esi = (DWORD)ctx64.Rsi; ctx->Edi = (DWORD)ctx64.Rdi;
        ctx->Ebp = (DWORD)ctx64.Rbp; ctx->Esp = (DWORD)ctx64.Rsp;
        ctx->Eip = (DWORD)ctx64.Rip;
        ctx->EFlags = (DWORD)ctx64.EFlags;
        ctx->SegCs = (WORD)ctx64.SegCs; ctx->SegDs = (WORD)ctx64.SegDs;
        ctx->SegEs = (WORD)ctx64.SegEs; ctx->SegFs = (WORD)ctx64.SegFs;
        ctx->SegGs = (WORD)ctx64.SegGs; ctx->SegSs = (WORD)ctx64.SegSs;
        ctx->Dr0 = (DWORD)ctx64.Dr0; ctx->Dr1 = (DWORD)ctx64.Dr1;
        ctx->Dr2 = (DWORD)ctx64.Dr2; ctx->Dr3 = (DWORD)ctx64.Dr3;
        ctx->Dr6 = (DWORD)ctx64.Dr6; ctx->Dr7 = (DWORD)ctx64.Dr7;
        return TRUE;
    }
#else
    CONTEXT* nativeCtx = (CONTEXT*)ctx; // Same struct on 32-bit
    nativeCtx->ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    return GetThreadContext(hThread, nativeCtx);
#endif
}

static BOOL DbgSet32Context(HANDLE hThread, WOW64_CONTEXT* ctx)
{
#ifdef _M_X64
    if (g_DbgProcess.bIsWow64) {
        ctx->ContextFlags = WOW64_CONTEXT_FULL;
        return Wow64SetThreadContext(hThread, ctx);
    } else {
        // 64-bit process: only safe to write back EFlags (registers are truncated)
        CONTEXT ctx64 = {0};
        ctx64.ContextFlags = CONTEXT_CONTROL;
        if (!GetThreadContext(hThread, &ctx64)) return FALSE;
        ctx64.EFlags = ctx->EFlags;
        return SetThreadContext(hThread, &ctx64);
    }
#else
    CONTEXT* nativeCtx = (CONTEXT*)ctx;
    nativeCtx->ContextFlags = CONTEXT_FULL;
    return SetThreadContext(hThread, nativeCtx);
#endif
}

// Read a module name from the debuggee's address space
static void DbgReadModuleName(HANDLE hProcess, HANDLE hFile, LPVOID lpBaseOfDll, LPVOID lpImageName, WORD fUnicode, char* out, size_t outLen)
{
    out[0] = 0;

    // Try reading the name pointer from the debuggee
    if (lpImageName) {
        LPVOID namePtr = NULL;
        SIZE_T bytesRead;
        if (ReadProcessMemory(hProcess, lpImageName, &namePtr, sizeof(namePtr), &bytesRead) && namePtr) {
            if (fUnicode) {
                WCHAR wName[MAX_PATH] = {0};
                if (ReadProcessMemory(hProcess, namePtr, wName, sizeof(wName) - 2, &bytesRead)) {
                    WideCharToMultiByte(CP_ACP, 0, wName, -1, out, (int)outLen, NULL, NULL);
                }
            } else {
                ReadProcessMemory(hProcess, namePtr, out, outLen - 1, &bytesRead);
            }
        }
    }

    // Fallback: use GetFinalPathNameByHandle if we have a file handle
    if (out[0] == 0 && hFile && hFile != INVALID_HANDLE_VALUE) {
        GetFinalPathNameByHandleA(hFile, out, (DWORD)outLen, FILE_NAME_NORMALIZED);
    }
}

// Pause the debugger: update state, read registers, notify UI
// Add one thread to cache
static void DbgCacheAddThread(DWORD tid, HANDLE hThread, const char* name)
{
    THREAD_CACHE_ENTRY entry;
    memset(&entry, 0, sizeof(entry));
    entry.dwThreadId = tid;
    lstrcpyn(entry.szName, name, 64);

    if (hThread) {
        WOW64_CONTEXT ctx = {0};
        if (DbgGet32Context(hThread, &ctx))
            entry.dwEip = ctx.Eip;

        const char* mod = DbgFindModuleForAddress(entry.dwEip);
        if (mod) lstrcpyn(entry.szModule, mod, 64);
    }

    s_threadCache.push_back(entry);
}

// Build complete thread cache (called once when pausing)
static void DbgCacheThreadEIPs()
{
    s_threadCache.clear();

    // Main thread
    if (g_DbgProcess.dwMainThreadId != 0) {
        char name[64];
        char* exeName = g_DbgProcess.szExePath;
        char* slash = strrchr(exeName, '\\');
        if (!slash) slash = strrchr(exeName, '/');
        if (slash) exeName = slash + 1;
        if (strncmp(exeName, "\\\\?\\", 4) == 0) exeName += 4;
        lstrcpyn(name, exeName[0] ? exeName : "Main", 64);
        DbgCacheAddThread(g_DbgProcess.dwMainThreadId, g_DbgProcess.hMainThread, name);
    }

    // Other threads
    for (size_t i = 0; i < g_DbgThreads.size(); i++) {
        if (g_DbgThreads[i].dwThreadId == g_DbgProcess.dwMainThreadId)
            continue;
        char name[32];
        wsprintf(name, "%08X", (DWORD)(DWORD_PTR)g_DbgThreads[i].lpStartAddress);
        DbgCacheAddThread(g_DbgThreads[i].dwThreadId, g_DbgThreads[i].hThread, name);
    }
}

static void DbgPauseAtEvent(DWORD dwThreadId, DWORD_PTR dwAddress, UINT uMsg)
{
    g_DbgState = DBG_STATE_PAUSED;
    s_dwPausedThreadId = dwThreadId;
    g_dwCurrentEIP = dwAddress;

    // Save previous registers for change detection
    memcpy(&g_DbgPrevRegisters, &g_DbgRegisters, sizeof(DEBUG_REGISTERS));
    DbgReadRegisters();

    // Cache all thread EIPs once (dialog reads from cache)
    DbgCacheThreadEIPs();

    PostMessage(s_hMainWnd, uMsg, (WPARAM)dwThreadId, (LPARAM)dwAddress);
}

// ================================================================
// ====================  DEBUG EVENT LOOP  ========================
// ================================================================

DWORD WINAPI DebugEventLoop(LPVOID lpParam)
{
    DEBUG_EVENT debugEvent;

    // Don't kill the process when we detach
    DebugSetProcessKillOnExit(FALSE);

    // Both CreateProcess and DebugActiveProcess must happen on the SAME thread
    // as WaitForDebugEvent. Windows delivers debug events only to the thread
    // that started debugging.
    if (g_DbgProcess.bAttached && s_dwAttachPid != 0) {
        // Attach to existing process
        if (!DebugActiveProcess(s_dwAttachPid)) {
            char err[128];
            wsprintf(err, "DebugActiveProcess failed: error %d", GetLastError());
            OutDebug(s_hMainWnd, err);
            g_bDebuggerActive = false;
            g_DbgState = DBG_STATE_IDLE;
            PostMessage(s_hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)DBG_STATE_IDLE, 0);
            return 1;
        }

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, s_dwAttachPid);
        if (!hProcess) {
            char err[128];
            wsprintf(err, "OpenProcess failed: error %d", GetLastError());
            OutDebug(s_hMainWnd, err);
            DebugActiveProcessStop(s_dwAttachPid);
            g_bDebuggerActive = false;
            g_DbgState = DBG_STATE_IDLE;
            PostMessage(s_hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)DBG_STATE_IDLE, 0);
            return 1;
        }

        g_DbgProcess.hProcess = hProcess;
        s_dwAttachPid = 0;

        // Detect if target is 32-bit (WOW64)
        BOOL wow64 = FALSE;
        IsWow64Process(g_DbgProcess.hProcess, &wow64);
        g_DbgProcess.bIsWow64 = (wow64 != FALSE);

        {
            char msg[64];
            wsprintf(msg, "Attached to %s process", g_DbgProcess.bIsWow64 ? "32-bit (WOW64)" : "64-bit");
            OutDebug(s_hMainWnd, msg);
        }
    } else {
        // Start new process
        STARTUPINFOA si = {0};
        PROCESS_INFORMATION pi = {0};
        si.cb = sizeof(si);

        const char* workDir = s_szStartWorkDir[0] ? s_szStartWorkDir : NULL;

        if (!CreateProcessA(NULL, s_szStartCmdLine, NULL, NULL, FALSE,
                            DEBUG_PROCESS | DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_CONSOLE,
                            NULL, workDir, &si, &pi)) {
            char err[128];
            wsprintf(err, "CreateProcess failed: error %d", GetLastError());
            OutDebug(s_hMainWnd, err);
            DeleteFileA(s_szTempExePath);
            s_szTempExePath[0] = 0;
            g_bDebuggerActive = false;
            g_DbgState = DBG_STATE_IDLE;
            PostMessage(s_hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)DBG_STATE_IDLE, 0);
            return 1;
        }

        g_DbgProcess.dwProcessId = pi.dwProcessId;
        g_DbgProcess.dwMainThreadId = pi.dwThreadId;
        g_DbgProcess.hProcess = pi.hProcess;
        g_DbgProcess.hMainThread = pi.hThread;

        // Detect if target is 32-bit (WOW64)
        BOOL wow64 = FALSE;
        IsWow64Process(g_DbgProcess.hProcess, &wow64);
        g_DbgProcess.bIsWow64 = (wow64 != FALSE);

        {
            char msg[64];
            wsprintf(msg, "Started %s process", g_DbgProcess.bIsWow64 ? "32-bit (WOW64)" : "64-bit");
            OutDebug(s_hMainWnd, msg);
        }
    }

    while (g_bDebuggerActive) {
        if (!WaitForDebugEvent(&debugEvent, 100)) {
            if (GetLastError() == ERROR_SEM_TIMEOUT)
                continue;
            break;
        }

        s_dwContinueStatus = DBG_CONTINUE;

        // Log all events when stepping
        if (g_DbgState == DBG_STATE_STEPPING_INTO || g_DbgState == DBG_STATE_STEPPING_OVER) {
            char evtMsg[128];
            wsprintf(evtMsg, "DBG_EVENT: code=%d tid=%d state=%d",
                     debugEvent.dwDebugEventCode, debugEvent.dwThreadId, g_DbgState);
            OutDebug(s_hMainWnd, evtMsg);
        }

        switch (debugEvent.dwDebugEventCode) {

        case CREATE_PROCESS_DEBUG_EVENT: {
            CREATE_PROCESS_DEBUG_INFO* cpdi = &debugEvent.u.CreateProcessInfo;

            g_DbgProcess.lpBaseOfImage = cpdi->lpBaseOfImage;
            g_DbgProcess.dwEntryPoint = (DWORD_PTR)cpdi->lpStartAddress;

            // Compute ASLR rebase delta: actual load address vs PE preferred ImageBase
            // Only meaningful when a file was loaded and disassembled (static addresses)
            {
                DWORD_PTR preferredBase = 0;
                if (LoadedPe64 && NTheader64)
                    preferredBase = (DWORD_PTR)NTheader64->OptionalHeader.ImageBase;
                else if (LoadedPe && NTheader)
                    preferredBase = (DWORD_PTR)NTheader->OptionalHeader.ImageBase;

                if (preferredBase != 0)
                    g_dwRebaseDelta = (DWORD_PTR)cpdi->lpBaseOfImage - preferredBase;
                else
                    g_dwRebaseDelta = 0;  // No file loaded, addresses are already runtime
            }

            // Always use the thread handle from the debug event
            g_DbgProcess.hMainThread = cpdi->hThread;
            g_DbgProcess.dwMainThreadId = debugEvent.dwThreadId;

            // Only read module name if we don't already have the original EXE path
            // (Start Process sets it before launch; don't overwrite with temp copy path)
            if (g_DbgProcess.szExePath[0] == '\0') {
                DbgReadModuleName(g_DbgProcess.hProcess, cpdi->hFile,
                                  cpdi->lpBaseOfImage, cpdi->lpImageName,
                                  cpdi->fUnicode, g_DbgProcess.szExePath, MAX_PATH);
            }

            if (cpdi->hFile)
                CloseHandle(cpdi->hFile);

            {
                char msg[256];
                wsprintf(msg, "Process created: base=%08X entry=%08X delta=%08X",
                         (DWORD)(DWORD_PTR)cpdi->lpBaseOfImage,
                         (DWORD)g_DbgProcess.dwEntryPoint,
                         (DWORD)g_dwRebaseDelta);
                OutDebug(s_hMainWnd, msg);
            }

            char* nameCopy = _strdup(g_DbgProcess.szExePath);
            PostMessage(s_hMainWnd, WM_DBG_DLL_LOAD, 0, (LPARAM)nameCopy);
            break;
        }

        case EXIT_PROCESS_DEBUG_EVENT: {
            DWORD exitCode = debugEvent.u.ExitProcess.dwExitCode;
            ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, DBG_CONTINUE);
            PostMessage(s_hMainWnd, WM_DBG_PROCESS_EXIT, (WPARAM)exitCode, 0);
            DbgCleanup();
            return 0;
        }

        case CREATE_THREAD_DEBUG_EVENT: {
            DEBUG_THREAD_INFO ti;
            ti.dwThreadId = debugEvent.dwThreadId;
            ti.hThread = debugEvent.u.CreateThread.hThread;
            ti.lpStartAddress = debugEvent.u.CreateThread.lpStartAddress;
            ti.lpTlsBase = debugEvent.u.CreateThread.lpThreadLocalBase;
            g_DbgThreads.push_back(ti);

            PostMessage(s_hMainWnd, WM_DBG_THREAD_CREATE, (WPARAM)debugEvent.dwThreadId, 0);
            break;
        }

        case EXIT_THREAD_DEBUG_EVENT: {
            for (size_t i = 0; i < g_DbgThreads.size(); i++) {
                if (g_DbgThreads[i].dwThreadId == debugEvent.dwThreadId) {
                    g_DbgThreads.erase(g_DbgThreads.begin() + i);
                    break;
                }
            }
            // Don't spam thread exit messages during stepping
            if (g_DbgState != DBG_STATE_STEPPING_INTO && g_DbgState != DBG_STATE_STEPPING_OVER)
                PostMessage(s_hMainWnd, WM_DBG_THREAD_EXIT, (WPARAM)debugEvent.dwThreadId, 0);
            break;
        }

        case LOAD_DLL_DEBUG_EVENT: {
            LOAD_DLL_DEBUG_INFO* lddi = &debugEvent.u.LoadDll;

            DEBUG_MODULE_INFO mod;
            memset(&mod, 0, sizeof(mod));
            mod.lpBaseOfDll = lddi->lpBaseOfDll;

            DbgReadModuleName(g_DbgProcess.hProcess, lddi->hFile,
                              lddi->lpBaseOfDll, lddi->lpImageName,
                              lddi->fUnicode, mod.szModuleName, MAX_PATH);

            if (lddi->hFile)
                CloseHandle(lddi->hFile);

            g_DbgModules.push_back(mod);

            char* nameCopy = _strdup(mod.szModuleName);
            PostMessage(s_hMainWnd, WM_DBG_DLL_LOAD, 0, (LPARAM)nameCopy);
            break;
        }

        case UNLOAD_DLL_DEBUG_EVENT: {
            LPVOID base = debugEvent.u.UnloadDll.lpBaseOfDll;
            for (size_t i = 0; i < g_DbgModules.size(); i++) {
                if (g_DbgModules[i].lpBaseOfDll == base) {
                    g_DbgModules.erase(g_DbgModules.begin() + i);
                    break;
                }
            }
            PostMessage(s_hMainWnd, WM_DBG_DLL_UNLOAD, 0, 0);
            break;
        }

        case OUTPUT_DEBUG_STRING_EVENT: {
            OUTPUT_DEBUG_STRING_INFO* odsi = &debugEvent.u.DebugString;
            SIZE_T bytesRead;

            if (odsi->fUnicode) {
                WCHAR* wBuf = (WCHAR*)malloc(odsi->nDebugStringLength * sizeof(WCHAR));
                if (wBuf) {
                    ReadProcessMemory(g_DbgProcess.hProcess, odsi->lpDebugStringData, wBuf, odsi->nDebugStringLength * sizeof(WCHAR), &bytesRead);
                    int len = WideCharToMultiByte(CP_ACP, 0, wBuf, -1, NULL, 0, NULL, NULL);
                    char* aBuf = (char*)malloc(len);
                    if (aBuf) {
                        WideCharToMultiByte(CP_ACP, 0, wBuf, -1, aBuf, len, NULL, NULL);
                        PostMessage(s_hMainWnd, WM_DBG_OUTPUT_STRING, 0, (LPARAM)aBuf);
                    }
                    free(wBuf);
                }
            } else {
                char* buf = (char*)malloc(odsi->nDebugStringLength + 1);
                if (buf) {
                    memset(buf, 0, odsi->nDebugStringLength + 1);
                    ReadProcessMemory(g_DbgProcess.hProcess, odsi->lpDebugStringData, buf, odsi->nDebugStringLength, &bytesRead);
                    PostMessage(s_hMainWnd, WM_DBG_OUTPUT_STRING, 0, (LPARAM)buf);
                }
            }
            break;
        }

        case EXCEPTION_DEBUG_EVENT: {
            EXCEPTION_RECORD* er = &debugEvent.u.Exception.ExceptionRecord;
            DWORD_PTR exAddr = (DWORD_PTR)er->ExceptionAddress;

            switch (er->ExceptionCode) {

            case 0x4000001F:  // STATUS_WX86_BREAKPOINT (WOW64 32-bit on 64-bit Windows)
            case EXCEPTION_BREAKPOINT: {
                {
                    char msg[128];
                    wsprintf(msg, "EXCEPTION_BREAKPOINT at %08X (initial_remaining=%d)", (DWORD)exAddr, s_nInitialBreakpoints);
                    OutDebug(s_hMainWnd, msg);
                }

                // Initial system breakpoints
                if (s_nInitialBreakpoints > 0) {
                    s_nInitialBreakpoints--;

                    if (g_DbgProcess.bAttached) {
                        // For attach: pause and read the main thread's context
                        // (the attach BP thread is temporary and will exit)
                        s_nInitialBreakpoints = 0;
                        s_dwPausedThreadId = g_DbgProcess.dwMainThreadId;

                        // Read main thread's actual EIP
                        g_DbgState = DBG_STATE_PAUSED;
                        memcpy(&g_DbgPrevRegisters, &g_DbgRegisters, sizeof(DEBUG_REGISTERS));
                        DbgReadRegisters();
                        DbgCacheThreadEIPs();

                        PostMessage(s_hMainWnd, WM_DBG_BREAKPOINT_HIT,
                                    (WPARAM)g_DbgProcess.dwMainThreadId, (LPARAM)g_dwCurrentEIP);

                        ResetEvent(g_hContinueEvent);
                        WaitForSingleObject(g_hContinueEvent, INFINITE);
                        s_dwContinueStatus = DBG_CONTINUE;
                    } else {
                        // For start: set entry point BP and continue past system breakpoints
                        if (g_DbgProcess.dwEntryPoint != 0 && !DbgFindBreakpoint(g_DbgProcess.dwEntryPoint)) {
                            BOOL bpSet = DbgSetBreakpoint(g_DbgProcess.dwEntryPoint, true);
                            char msg[128];
                            wsprintf(msg, "Entry point BP at %08X: %s", (DWORD)g_DbgProcess.dwEntryPoint, bpSet ? "OK" : "FAILED");
                            OutDebug(s_hMainWnd, msg);
                        }
                        s_dwContinueStatus = DBG_CONTINUE;
                    }
                    break;
                }

                // Check if this is a user breakpoint
                // ExceptionAddress may point AT the INT3 or one byte AFTER it
                // depending on the platform/WOW64, so check both
                EnterCriticalSection(&g_csBreakpoints);
                DEBUG_BREAKPOINT* bp = DbgFindBreakpoint(exAddr);
                bool bpAtExAddr = true;
                if (!bp) {
                    bp = DbgFindBreakpoint(exAddr - 1);
                    bpAtExAddr = false;
                }
                if (bp) {
                    // Restore original byte
                    SIZE_T written;
                    WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)bp->dwAddress, &bp->bOriginalByte, 1, &written);
                    FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)bp->dwAddress, 1);

                    // Set EIP/RIP back to the breakpoint address (INT3 advances past it)
                    {
                        HANDLE hThread = DbgGetThreadHandle(debugEvent.dwThreadId);
                        if (hThread) {
#ifdef _M_X64
                            if (g_DbgProcess.bIsWow64) {
                                WOW64_CONTEXT ctx = {0};
                                ctx.ContextFlags = WOW64_CONTEXT_CONTROL;
                                Wow64GetThreadContext(hThread, &ctx);
                                ctx.Eip = (DWORD)bp->dwAddress;
                                Wow64SetThreadContext(hThread, &ctx);
                            } else {
                                CONTEXT ctx64 = {0};
                                ctx64.ContextFlags = CONTEXT_CONTROL;
                                GetThreadContext(hThread, &ctx64);
                                ctx64.Rip = (DWORD64)bp->dwAddress;
                                SetThreadContext(hThread, &ctx64);
                            }
#else
                            CONTEXT ctx = {0};
                            ctx.ContextFlags = CONTEXT_CONTROL;
                            GetThreadContext(hThread, &ctx);
                            ctx.Eip = (DWORD)bp->dwAddress;
                            SetThreadContext(hThread, &ctx);
#endif
                        }
                    }

                    bp->nHitCount++;
                    bool isOneShot = bp->bOneShot;
                    DWORD_PTR bpAddr = bp->dwAddress;

                    if (isOneShot) {
                        // Remove one-shot breakpoint
                        for (size_t i = 0; i < g_DbgBreakpoints.size(); i++) {
                            if (g_DbgBreakpoints[i].dwAddress == bpAddr) {
                                g_DbgBreakpoints.erase(g_DbgBreakpoints.begin() + i);
                                break;
                            }
                        }

                        // Re-enable any permanent breakpoint that was temporarily disabled
                        // (since we use one-shot BPs for stepping, not TF/single-step)
                        if (s_dwReEnableBPAddr != 0) {
                            DEBUG_BREAKPOINT* rebp = DbgFindBreakpoint(s_dwReEnableBPAddr);
                            if (rebp && rebp->bEnabled) {
                                BYTE int3 = 0xCC;
                                SIZE_T w;
                                WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)rebp->dwAddress, &int3, 1, &w);
                                FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)rebp->dwAddress, 1);
                            }
                            s_dwReEnableBPAddr = 0;
                        }
                    } else {
                        // Permanent breakpoint - remember to re-enable after stepping past it
                        s_dwReEnableBPAddr = bpAddr;
                    }
                    LeaveCriticalSection(&g_csBreakpoints);

                    DbgPauseAtEvent(debugEvent.dwThreadId, bpAddr, WM_DBG_BREAKPOINT_HIT);

                    ResetEvent(g_hContinueEvent);
                    WaitForSingleObject(g_hContinueEvent, INFINITE);
                    s_dwContinueStatus = DBG_CONTINUE;
                } else {
                    LeaveCriticalSection(&g_csBreakpoints);
                    // Not our breakpoint, pass to application
                    if (debugEvent.u.Exception.dwFirstChance)
                        s_dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                    else
                        s_dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                }
                break;
            }

            case 0x4000001E:  // STATUS_WX86_SINGLE_STEP (WOW64)
            case EXCEPTION_SINGLE_STEP: {
                // Re-enable breakpoint if needed
                if (s_dwReEnableBPAddr != 0) {
                    EnterCriticalSection(&g_csBreakpoints);
                    DEBUG_BREAKPOINT* bp = DbgFindBreakpoint(s_dwReEnableBPAddr);
                    if (bp && bp->bEnabled) {
                        BYTE int3 = 0xCC;
                        SIZE_T written;
                        WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)bp->dwAddress, &int3, 1, &written);
                        FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)bp->dwAddress, 1);
                    }
                    LeaveCriticalSection(&g_csBreakpoints);
                    s_dwReEnableBPAddr = 0;

                    // If we were just re-enabling a BP and the user wants to continue running
                    if (g_DbgState == DBG_STATE_RUNNING) {
                        break;  // Continue running
                    }
                }

                // Step complete - pause
                DbgPauseAtEvent(debugEvent.dwThreadId, exAddr, WM_DBG_STEP_COMPLETE);

                ResetEvent(g_hContinueEvent);
                WaitForSingleObject(g_hContinueEvent, INFINITE);
                s_dwContinueStatus = DBG_CONTINUE;
                break;
            }

            default: {
                // Other exceptions
                PostMessage(s_hMainWnd, WM_DBG_EXCEPTION,
                           (WPARAM)er->ExceptionCode, (LPARAM)exAddr);

                if (debugEvent.u.Exception.dwFirstChance) {
                    // First chance - let the application handle it
                    s_dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
                } else {
                    // Second chance - pause the debugger
                    DbgPauseAtEvent(debugEvent.dwThreadId, exAddr, WM_DBG_EXCEPTION);
                    ResetEvent(g_hContinueEvent);
                    WaitForSingleObject(g_hContinueEvent, INFINITE);
                    s_dwContinueStatus = DBG_CONTINUE;
                }
                break;
            }

            } // switch ExceptionCode
            break;
        }

        case RIP_EVENT:
            break;

        } // switch dwDebugEventCode

        ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, s_dwContinueStatus);

        // Check if detach was requested
        if (s_bDetachRequested) {
            s_bDetachRequested = false;
            DebugActiveProcessStop(g_DbgProcess.dwProcessId);
            PostMessage(s_hMainWnd, WM_DBG_PROCESS_EXIT, 0, 0);
            DbgCleanup();
            return 0;
        }
    }

    return 0;
}

// ================================================================
// ==================  BREAKPOINT MANAGEMENT  =====================
// ================================================================

BOOL DbgSetBreakpoint(DWORD_PTR dwAddress, bool bOneShot)
{
    if (!g_bDebuggerActive)
        return FALSE;

    EnterCriticalSection(&g_csBreakpoints);

    // Check if breakpoint already exists
    DEBUG_BREAKPOINT* existing = DbgFindBreakpoint(dwAddress);
    if (existing) {
        LeaveCriticalSection(&g_csBreakpoints);
        return FALSE;
    }

    DEBUG_BREAKPOINT bp;
    memset(&bp, 0, sizeof(bp));
    bp.dwAddress = dwAddress;
    bp.bOneShot = bOneShot;
    bp.bEnabled = true;
    bp.nHitCount = 0;

    // Read original byte
    SIZE_T bytesRead;
    if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwAddress, &bp.bOriginalByte, 1, &bytesRead)) {
        LeaveCriticalSection(&g_csBreakpoints);
        return FALSE;
    }

    // Write INT3
    BYTE int3 = 0xCC;
    SIZE_T written;
    if (!WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwAddress, &int3, 1, &written)) {
        LeaveCriticalSection(&g_csBreakpoints);
        return FALSE;
    }
    FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)dwAddress, 1);

    g_DbgBreakpoints.push_back(bp);
    LeaveCriticalSection(&g_csBreakpoints);
    return TRUE;
}

BOOL DbgRemoveBreakpoint(DWORD_PTR dwAddress)
{
    if (!g_bDebuggerActive)
        return FALSE;

    EnterCriticalSection(&g_csBreakpoints);
    for (size_t i = 0; i < g_DbgBreakpoints.size(); i++) {
        if (g_DbgBreakpoints[i].dwAddress == dwAddress) {
            if (g_DbgBreakpoints[i].bEnabled) {
                // Restore original byte
                SIZE_T written;
                WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwAddress, &g_DbgBreakpoints[i].bOriginalByte, 1, &written);
                FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)dwAddress, 1);
            }
            g_DbgBreakpoints.erase(g_DbgBreakpoints.begin() + i);
            LeaveCriticalSection(&g_csBreakpoints);
            return TRUE;
        }
    }
    LeaveCriticalSection(&g_csBreakpoints);
    return FALSE;
}

BOOL DbgToggleBreakpoint(DWORD_PTR dwAddress)
{
    EnterCriticalSection(&g_csBreakpoints);
    DEBUG_BREAKPOINT* bp = DbgFindBreakpoint(dwAddress);
    if (bp) {
        DWORD_PTR addr = bp->dwAddress;
        LeaveCriticalSection(&g_csBreakpoints);
        return DbgRemoveBreakpoint(addr);
    }
    LeaveCriticalSection(&g_csBreakpoints);
    return DbgSetBreakpoint(dwAddress, false);
}

BOOL DbgEnableBreakpoint(DWORD_PTR dwAddress, bool bEnable)
{
    if (!g_bDebuggerActive)
        return FALSE;

    EnterCriticalSection(&g_csBreakpoints);
    DEBUG_BREAKPOINT* bp = DbgFindBreakpoint(dwAddress);
    if (!bp) {
        LeaveCriticalSection(&g_csBreakpoints);
        return FALSE;
    }

    if (bEnable && !bp->bEnabled) {
        BYTE int3 = 0xCC;
        SIZE_T written;
        WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwAddress, &int3, 1, &written);
        FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)dwAddress, 1);
        bp->bEnabled = true;
    } else if (!bEnable && bp->bEnabled) {
        SIZE_T written;
        WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwAddress, &bp->bOriginalByte, 1, &written);
        FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)dwAddress, 1);
        bp->bEnabled = false;
    }

    LeaveCriticalSection(&g_csBreakpoints);
    return TRUE;
}

// Note: caller must hold g_csBreakpoints or be in debug loop context
DEBUG_BREAKPOINT* DbgFindBreakpoint(DWORD_PTR dwAddress)
{
    for (size_t i = 0; i < g_DbgBreakpoints.size(); i++) {
        if (g_DbgBreakpoints[i].dwAddress == dwAddress)
            return &g_DbgBreakpoints[i];
    }
    return NULL;
}

// ================================================================
// =========================  STEPPING  ===========================
// ================================================================

BOOL DbgStepInto()
{
    if (g_DbgState != DBG_STATE_PAUSED)
        return FALSE;

    // Read current instruction
    BYTE instrBuf[16];
    SIZE_T bytesRead;
    if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)g_dwCurrentEIP, instrBuf, sizeof(instrBuf), &bytesRead))
        return FALSE;

    DISASSEMBLY disasm;
    memset(&disasm, 0, sizeof(disasm));
    DWORD_PTR index = 0;
    FlushDecoded(&disasm);
    Decode(&disasm, (char*)instrBuf, &index);

    DWORD_PTR instrSize = disasm.OpcodeSize + disasm.PrefixSize;
    if (instrSize == 0) instrSize = 1;

    DWORD_PTR targetAddr = 0;
    bool isConditionalJump = false;

    // Handle RET instructions - read return address from [ESP]
    if (disasm.CodeFlow.Ret) {
        DWORD retAddr32 = 0;
        SIZE_T br;
        if (ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)(DWORD_PTR)g_DbgRegisters.Esp, &retAddr32, sizeof(DWORD), &br)) {
            DbgSetBreakpoint((DWORD_PTR)retAddr32, true);

            g_DbgState = DBG_STATE_STEPPING_INTO;
            s_dwContinueStatus = DBG_CONTINUE;
            SetEvent(g_hContinueEvent);
            return TRUE;
        }
        // If ReadProcessMemory fails, fall through to normal next-instruction step
    }

    if (disasm.CodeFlow.Call || disasm.CodeFlow.Jump) {
        BYTE op = instrBuf[disasm.PrefixSize];

        if (op == 0xE8 || op == 0xE9) {
            // Near CALL rel32 / JMP rel32
            INT32 rel = *(INT32*)(instrBuf + disasm.PrefixSize + 1);
            targetAddr = g_dwCurrentEIP + instrSize + (DWORD_PTR)(INT_PTR)rel;
        }
        else if (op == 0xEB) {
            // Short JMP rel8
            INT8 rel = (INT8)instrBuf[disasm.PrefixSize + 1];
            targetAddr = g_dwCurrentEIP + instrSize + (DWORD_PTR)(INT_PTR)rel;
        }
        else if (op == 0xFF) {
            BYTE modrm = instrBuf[disasm.PrefixSize + 1];
            BYTE reg = (modrm >> 3) & 7;  // reg field: 2=CALL, 4=JMP

            if (reg == 2 || reg == 4) {
                BYTE mod = (modrm >> 6) & 3;
                BYTE rm = modrm & 7;

                if (mod == 0 && rm == 5) {
                    // FF 15/25 disp32 = CALL/JMP [disp32] (indirect via memory)
                    DWORD indirectAddr = *(DWORD*)(instrBuf + disasm.PrefixSize + 2);
                    DWORD callTarget = 0;
                    ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)(DWORD_PTR)indirectAddr, &callTarget, sizeof(callTarget), &bytesRead);
                    if (callTarget != 0)
                        targetAddr = callTarget;
                }
                else if (mod == 3) {
                    // FF D0-D7 / FF E0-E7 = CALL/JMP reg (indirect via register)
                    DWORD regVal = 0;
                    switch (rm) {
                        case 0: regVal = g_DbgRegisters.Eax; break;
                        case 1: regVal = g_DbgRegisters.Ecx; break;
                        case 2: regVal = g_DbgRegisters.Edx; break;
                        case 3: regVal = g_DbgRegisters.Ebx; break;
                        case 4: regVal = g_DbgRegisters.Esp; break;
                        case 5: regVal = g_DbgRegisters.Ebp; break;
                        case 6: regVal = g_DbgRegisters.Esi; break;
                        case 7: regVal = g_DbgRegisters.Edi; break;
                    }
                    if (regVal != 0)
                        targetAddr = regVal;
                }

                // Force the flow flags so the target is used
                if (targetAddr != 0) {
                    if (reg == 2) disasm.CodeFlow.Call = TRUE;
                    else disasm.CodeFlow.Jump = TRUE;
                }
            }
        }
        else if ((op & 0xF0) == 0x70) {
            // Short conditional jump rel8 (0x70-0x7F)
            INT8 rel = (INT8)instrBuf[disasm.PrefixSize + 1];
            targetAddr = g_dwCurrentEIP + instrSize + (DWORD_PTR)(INT_PTR)rel;
            isConditionalJump = true;
        }
        else if (op == 0xE3 || op == 0xE2 || op == 0xE1 || op == 0xE0) {
            // E3=JECXZ, E2=LOOP, E1=LOOPE/LOOPZ, E0=LOOPNE/LOOPNZ
            INT8 rel = (INT8)instrBuf[disasm.PrefixSize + 1];
            targetAddr = g_dwCurrentEIP + instrSize + (DWORD_PTR)(INT_PTR)rel;
            isConditionalJump = true;
        }
        else if (op == 0x0F) {
            BYTE op2 = instrBuf[disasm.PrefixSize + 1];
            if ((op2 & 0xF0) == 0x80) {
                // Near conditional jump rel32 (0x0F 0x80-0x8F)
                INT32 rel = *(INT32*)(instrBuf + disasm.PrefixSize + 2);
                targetAddr = g_dwCurrentEIP + instrSize + (DWORD_PTR)(INT_PTR)rel;
                isConditionalJump = true;
            }
        }
    }

    if (isConditionalJump && targetAddr != 0) {
        // Evaluate the condition using EFLAGS to determine which way the branch goes
        DWORD eflags = g_DbgRegisters.EFlags;
        bool CF = (eflags >> 0) & 1;
        bool PF = (eflags >> 2) & 1;
        bool ZF = (eflags >> 6) & 1;
        bool SF = (eflags >> 7) & 1;
        bool OF = (eflags >> 11) & 1;

        // Get the condition code (low nibble of opcode)
        BYTE cc;
        BYTE op = instrBuf[disasm.PrefixSize];
        if (op == 0x0F)
            cc = instrBuf[disasm.PrefixSize + 1] & 0x0F;
        else if (op >= 0xE0 && op <= 0xE3)
            cc = op; // Special: E0=LOOPNE, E1=LOOPE, E2=LOOP, E3=JECXZ
        else
            cc = op & 0x0F;

        bool taken = false;
        switch (cc) {
            case 0x0: taken = OF; break;                    // JO
            case 0x1: taken = !OF; break;                   // JNO
            case 0x2: taken = CF; break;                    // JB/JC/JNAE
            case 0x3: taken = !CF; break;                   // JNB/JNC/JAE
            case 0x4: taken = ZF; break;                    // JZ/JE
            case 0x5: taken = !ZF; break;                   // JNZ/JNE
            case 0x6: taken = CF || ZF; break;              // JBE/JNA
            case 0x7: taken = !CF && !ZF; break;            // JA/JNBE
            case 0x8: taken = SF; break;                    // JS
            case 0x9: taken = !SF; break;                   // JNS
            case 0xA: taken = PF; break;                    // JP/JPE
            case 0xB: taken = !PF; break;                   // JNP/JPO
            case 0xC: taken = SF != OF; break;              // JL/JNGE
            case 0xD: taken = SF == OF; break;              // JGE/JNL
            case 0xE: taken = ZF || (SF != OF); break;      // JLE/JNG
            case 0xF: taken = !ZF && (SF == OF); break;     // JG/JNLE
            case 0xE0: taken = ((g_DbgRegisters.Ecx - 1) != 0 && !ZF); break; // LOOPNE/LOOPNZ
            case 0xE1: taken = ((g_DbgRegisters.Ecx - 1) != 0 && ZF); break;  // LOOPE/LOOPZ
            case 0xE2: taken = ((g_DbgRegisters.Ecx - 1) != 0); break;        // LOOP
            case 0xE3: taken = (g_DbgRegisters.Ecx == 0); break;              // JECXZ
        }

        if (taken) {
            DbgSetBreakpoint(targetAddr, true);
        } else {
            DbgSetBreakpoint(g_dwCurrentEIP + instrSize, true);
        }
    }
    else if (targetAddr != 0 && (disasm.CodeFlow.Call || disasm.CodeFlow.Jump)) {
        // Unconditional CALL/JMP - follow the target
        DbgSetBreakpoint(targetAddr, true);
    } else {
        // Normal instruction - break at next instruction
        DbgSetBreakpoint(g_dwCurrentEIP + instrSize, true);
    }

    g_DbgState = DBG_STATE_STEPPING_INTO;
    s_dwContinueStatus = DBG_CONTINUE;
    SetEvent(g_hContinueEvent);

    return TRUE;
}

BOOL DbgStepOver()
{
    if (g_DbgState != DBG_STATE_PAUSED)
        return FALSE;

    HANDLE hThread = DbgGetThreadHandle(s_dwPausedThreadId);
    if (!hThread)
        return FALSE;

    // Read current instruction to check if it's a CALL
    BYTE instrBuf[16];
    SIZE_T bytesRead;
    if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)g_dwCurrentEIP, instrBuf, sizeof(instrBuf), &bytesRead))
        return DbgStepInto();  // Fallback to step into

    // Decode the instruction using PVDasm's decode engine
    DISASSEMBLY disasm;
    memset(&disasm, 0, sizeof(disasm));
    DWORD_PTR index = 0;
    FlushDecoded(&disasm);
    Decode(&disasm, (char*)instrBuf, &index);

    // Check if instruction is a CALL (CodeFlow.Call flag)
    bool isCall = (disasm.CodeFlow.Call == 1);
    DWORD_PTR instrSize = disasm.OpcodeSize + disasm.PrefixSize;
    if (instrSize == 0) instrSize = 1;

    if (isCall) {
        // Set one-shot breakpoint after the CALL instruction
        DWORD_PTR nextAddr = g_dwCurrentEIP + instrSize;
        DbgSetBreakpoint(nextAddr, true);

        g_DbgState = DBG_STATE_STEPPING_OVER;
        s_dwContinueStatus = DBG_CONTINUE;
        SetEvent(g_hContinueEvent);
    } else {
        // Not a CALL - just single step
        return DbgStepInto();
    }

    return TRUE;
}

BOOL DbgRunUntilReturn()
{
    if (g_DbgState != DBG_STATE_PAUSED)
        return FALSE;

    // Read return address from [ESP]
    DWORD_PTR retAddr = 0;
    SIZE_T bytesRead;

    #ifdef _M_X64
    if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)(DWORD_PTR)g_DbgRegisters.Esp, &retAddr, sizeof(DWORD_PTR), &bytesRead))
        return FALSE;
    #else
    DWORD retAddr32 = 0;
    if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)(DWORD_PTR)g_DbgRegisters.Esp, &retAddr32, sizeof(DWORD), &bytesRead))
        return FALSE;
    retAddr = retAddr32;
    #endif

    if (retAddr == 0)
        return FALSE;

    // Set one-shot breakpoint at return address
    DbgSetBreakpoint(retAddr, true);

    g_DbgState = DBG_STATE_STEPPING_OUT;
    s_dwContinueStatus = DBG_CONTINUE;
    SetEvent(g_hContinueEvent);

    PostMessage(s_hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)g_DbgState, 0);
    return TRUE;
}

BOOL DbgRunToCursor(DWORD_PTR dwAddress)
{
    if (g_DbgState != DBG_STATE_PAUSED)
        return FALSE;

    // Set one-shot breakpoint at target address
    DbgSetBreakpoint(dwAddress, true);

    g_DbgState = DBG_STATE_RUN_TO_CURSOR;
    s_dwContinueStatus = DBG_CONTINUE;
    SetEvent(g_hContinueEvent);

    PostMessage(s_hMainWnd, WM_DBG_STATE_CHANGED, (WPARAM)g_DbgState, 0);
    return TRUE;
}

// ================================================================
// ==================  REGISTER OPERATIONS  =======================
// ================================================================

BOOL DbgReadRegisters()
{
    if (!g_bDebuggerActive || s_dwPausedThreadId == 0)
        return FALSE;

    HANDLE hThread = DbgGetThreadHandle(s_dwPausedThreadId);
    if (!hThread)
        return FALSE;

    WOW64_CONTEXT ctx = {0};
    if (!DbgGet32Context(hThread, &ctx))
        return FALSE;

    g_DbgRegisters.Eax = ctx.Eax; g_DbgRegisters.Ebx = ctx.Ebx;
    g_DbgRegisters.Ecx = ctx.Ecx; g_DbgRegisters.Edx = ctx.Edx;
    g_DbgRegisters.Esi = ctx.Esi; g_DbgRegisters.Edi = ctx.Edi;
    g_DbgRegisters.Ebp = ctx.Ebp; g_DbgRegisters.Esp = ctx.Esp;
    g_DbgRegisters.Eip = ctx.Eip;
    g_DbgRegisters.EFlags = ctx.EFlags;
    g_DbgRegisters.SegCs = (WORD)ctx.SegCs; g_DbgRegisters.SegDs = (WORD)ctx.SegDs;
    g_DbgRegisters.SegEs = (WORD)ctx.SegEs; g_DbgRegisters.SegFs = (WORD)ctx.SegFs;
    g_DbgRegisters.SegGs = (WORD)ctx.SegGs; g_DbgRegisters.SegSs = (WORD)ctx.SegSs;
    g_DbgRegisters.Dr0 = ctx.Dr0; g_DbgRegisters.Dr1 = ctx.Dr1;
    g_DbgRegisters.Dr2 = ctx.Dr2; g_DbgRegisters.Dr3 = ctx.Dr3;
    g_DbgRegisters.Dr6 = ctx.Dr6; g_DbgRegisters.Dr7 = ctx.Dr7;

    // For 64-bit targets, g_DbgRegisters.Eip is truncated to 32 bits.
    // Read the full 64-bit RIP directly for address operations.
#ifdef _M_X64
    if (!g_DbgProcess.bIsWow64) {
        HANDLE hThread2 = DbgGetThreadHandle(s_dwPausedThreadId);
        if (hThread2) {
            CONTEXT ctx64 = {0};
            ctx64.ContextFlags = CONTEXT_CONTROL;
            if (GetThreadContext(hThread2, &ctx64)) {
                g_dwCurrentEIP = (DWORD_PTR)ctx64.Rip;
                return TRUE;
            }
        }
    }
#endif
    g_dwCurrentEIP = g_DbgRegisters.Eip;
    return TRUE;
}

BOOL DbgWriteRegisters()
{
    if (!g_bDebuggerActive || s_dwPausedThreadId == 0)
        return FALSE;

    HANDLE hThread = DbgGetThreadHandle(s_dwPausedThreadId);
    if (!hThread)
        return FALSE;

    WOW64_CONTEXT ctx = {0};
    if (!DbgGet32Context(hThread, &ctx))
        return FALSE;

    ctx.Eax = g_DbgRegisters.Eax; ctx.Ebx = g_DbgRegisters.Ebx;
    ctx.Ecx = g_DbgRegisters.Ecx; ctx.Edx = g_DbgRegisters.Edx;
    ctx.Esi = g_DbgRegisters.Esi; ctx.Edi = g_DbgRegisters.Edi;
    ctx.Ebp = g_DbgRegisters.Ebp; ctx.Esp = g_DbgRegisters.Esp;
    ctx.Eip = g_DbgRegisters.Eip;
    ctx.EFlags = g_DbgRegisters.EFlags;
    ctx.SegCs = g_DbgRegisters.SegCs; ctx.SegDs = g_DbgRegisters.SegDs;
    ctx.SegEs = g_DbgRegisters.SegEs; ctx.SegFs = g_DbgRegisters.SegFs;
    ctx.SegGs = g_DbgRegisters.SegGs; ctx.SegSs = g_DbgRegisters.SegSs;

    return DbgSet32Context(hThread, &ctx);
}

void DbgFormatEFlags(DWORD eflags, char* buffer, size_t bufLen)
{
    wsprintf(buffer, "CF=%d PF=%d AF=%d ZF=%d SF=%d TF=%d IF=%d DF=%d OF=%d",
             (eflags >> 0) & 1,   // CF
             (eflags >> 2) & 1,   // PF
             (eflags >> 4) & 1,   // AF
             (eflags >> 6) & 1,   // ZF
             (eflags >> 7) & 1,   // SF
             (eflags >> 8) & 1,   // TF
             (eflags >> 9) & 1,   // IF
             (eflags >> 10) & 1,  // DF
             (eflags >> 11) & 1); // OF
}

// Generate IDA-style annotation for a register value
static void DbgAnnotateRegisterValue(DWORD regValue, char* out, size_t outSize)
{
    out[0] = 0;
    if (!g_bDebuggerActive || regValue == 0) return;

    extern FunctionInfo fFunctionInfo;

    // Check if value points to a known function start
    for (size_t i = 0; i < fFunctionInfo.size(); i++) {
        if (regValue == (DWORD)fFunctionInfo[i].FunctionStart) {
            // Read and decode first instruction
            BYTE codeBuf[16];
            SIZE_T br;
            if (DbgReadMemory((DWORD_PTR)regValue, codeBuf, 16, &br) && br > 0) {
                DISASSEMBLY dis;
                DWORD_PTR idx = 0;
                FlushDecoded(&dis);
                dis.Address = regValue;
                Decode(&dis, (char*)codeBuf, &idx);
                wsprintf(out, "%s -> %s", fFunctionInfo[i].FunctionName, dis.Assembly);
            } else {
                lstrcpyn(out, fFunctionInfo[i].FunctionName, (int)outSize);
            }
            return;
        }
    }

    // Check if value is inside a known function
    for (size_t i = 0; i < fFunctionInfo.size(); i++) {
        if (regValue > (DWORD)fFunctionInfo[i].FunctionStart &&
            regValue <= (DWORD)fFunctionInfo[i].FunctionEnd) {
            DWORD offset = regValue - (DWORD)fFunctionInfo[i].FunctionStart;
            wsprintf(out, "%s+%Xh", fFunctionInfo[i].FunctionName, offset);
            return;
        }
    }

    // Check if value is near ESP (stack pointer)
    if (g_DbgRegisters.Esp != 0) {
        INT_PTR stackOff = (INT_PTR)regValue - (INT_PTR)g_DbgRegisters.Esp;
        if (stackOff >= -0x10000 && stackOff <= 0x10000 && stackOff != 0) {
            if (stackOff >= 0)
                wsprintf(out, "Stack[ESP+%Xh]", (DWORD)stackOff);
            else
                wsprintf(out, "Stack[ESP-%Xh]", (DWORD)-stackOff);
            return;
        }
    }

    // Check if the value contains printable ASCII bytes
    {
        char bytes[4];
        bytes[0] = (char)(regValue & 0xFF);
        bytes[1] = (char)((regValue >> 8) & 0xFF);
        bytes[2] = (char)((regValue >> 16) & 0xFF);
        bytes[3] = (char)((regValue >> 24) & 0xFF);
        char ascii[5] = {0};
        int count = 0;
        for (int i = 0; i < 4; i++) {
            if (bytes[i] >= 0x20 && bytes[i] <= 0x7E)
                ascii[count++] = bytes[i];
        }
        ascii[count] = 0;
        if (count >= 2) {
            wsprintf(out, "'%s'", ascii);
            return;
        }
    }

    // Check if value is in a known module
    const char* mod = DbgFindModuleForAddress((DWORD_PTR)regValue);
    if (mod) {
        lstrcpyn(out, mod, (int)outSize);
        return;
    }
}

void DbgUpdateRegisterDialog()
{
    if (!g_hRegisterDlg || !IsWindow(g_hRegisterDlg))
        return;

    char buf[32];

    // Update register values and annotations
    struct { DWORD value; int editId; int annotId; } regs[] = {
        { g_DbgRegisters.Eax, IDC_REG_EAX, IDC_REG_EAX_ANNOT },
        { g_DbgRegisters.Ebx, IDC_REG_EBX, IDC_REG_EBX_ANNOT },
        { g_DbgRegisters.Ecx, IDC_REG_ECX, IDC_REG_ECX_ANNOT },
        { g_DbgRegisters.Edx, IDC_REG_EDX, IDC_REG_EDX_ANNOT },
        { g_DbgRegisters.Esi, IDC_REG_ESI, IDC_REG_ESI_ANNOT },
        { g_DbgRegisters.Edi, IDC_REG_EDI, IDC_REG_EDI_ANNOT },
        { g_DbgRegisters.Ebp, IDC_REG_EBP, IDC_REG_EBP_ANNOT },
        { g_DbgRegisters.Esp, IDC_REG_ESP, IDC_REG_ESP_ANNOT },
        { g_DbgRegisters.Eip, IDC_REG_EIP, IDC_REG_EIP_ANNOT },
    };

    char annot[256];
    for (int i = 0; i < 9; i++) {
        wsprintf(buf, "%08X", regs[i].value);
        SetDlgItemText(g_hRegisterDlg, regs[i].editId, buf);
        DbgAnnotateRegisterValue(regs[i].value, annot, sizeof(annot));
        SetDlgItemText(g_hRegisterDlg, regs[i].annotId, annot);
    }

    wsprintf(buf, "%08X", g_DbgRegisters.EFlags); SetDlgItemText(g_hRegisterDlg, IDC_REG_EFLAGS, buf);

    wsprintf(buf, "%04X", g_DbgRegisters.SegCs); SetDlgItemText(g_hRegisterDlg, IDC_REG_CS, buf);
    wsprintf(buf, "%04X", g_DbgRegisters.SegDs); SetDlgItemText(g_hRegisterDlg, IDC_REG_DS, buf);
    wsprintf(buf, "%04X", g_DbgRegisters.SegEs); SetDlgItemText(g_hRegisterDlg, IDC_REG_ES, buf);
    wsprintf(buf, "%04X", g_DbgRegisters.SegFs); SetDlgItemText(g_hRegisterDlg, IDC_REG_FS, buf);
    wsprintf(buf, "%04X", g_DbgRegisters.SegGs); SetDlgItemText(g_hRegisterDlg, IDC_REG_GS, buf);
    wsprintf(buf, "%04X", g_DbgRegisters.SegSs); SetDlgItemText(g_hRegisterDlg, IDC_REG_SS, buf);

    // Format expanded flags
    char flagsBuf[128];
    DbgFormatEFlags(g_DbgRegisters.EFlags, flagsBuf, sizeof(flagsBuf));
    SetDlgItemText(g_hRegisterDlg, IDC_REG_FLAGS_TEXT, flagsBuf);
}

// ================================================================
// ==================  MEMORY OPERATIONS  =========================
// ================================================================

BOOL DbgReadMemory(DWORD_PTR dwAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* pBytesRead)
{
    if (!g_bDebuggerActive)
        return FALSE;
    return ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwAddress, lpBuffer, nSize, pBytesRead);
}

BOOL DbgWriteMemory(DWORD_PTR dwAddress, LPCVOID lpBuffer, SIZE_T nSize)
{
    if (!g_bDebuggerActive)
        return FALSE;
    SIZE_T written;
    BOOL result = WriteProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwAddress, lpBuffer, nSize, &written);
    if (result)
        FlushInstructionCache(g_DbgProcess.hProcess, (LPVOID)dwAddress, nSize);
    return result;
}

BOOL DbgTakeMemorySnapshot(DWORD_PTR dwBaseAddress, DWORD dwSize)
{
    if (!g_bDebuggerActive)
        return FALSE;

    BYTE* pData = (BYTE*)malloc(dwSize);
    if (!pData)
        return FALSE;

    SIZE_T bytesRead;
    if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)dwBaseAddress, pData, dwSize, &bytesRead)) {
        free(pData);
        return FALSE;
    }

    DEBUG_MEMORY_SNAPSHOT snap;
    snap.dwBaseAddress = dwBaseAddress;
    snap.dwSize = (DWORD)bytesRead;
    snap.pData = pData;
    GetSystemTime(&snap.stTimestamp);

    g_DbgSnapshots.push_back(snap);
    return TRUE;
}

void DbgRefreshMemory()
{
    if (g_DbgState == DBG_STATE_PAUSED) {
        DbgDisassembleAtEIP();
    }
}

// ================================================================
// =================  DISASSEMBLY SYNC  ===========================
// ================================================================

// Resolve memory references in instruction to show values in comments
static void DbgResolveMemoryComment(const char* assembly, char* outComment, size_t outSize)
{
    outComment[0] = 0;
    if (!g_bDebuggerActive || !assembly) return;

    const char* ptrPos = strstr(assembly, "ptr ");
    if (!ptrPos) ptrPos = strstr(assembly, "Ptr ");
    if (!ptrPos) ptrPos = strstr(assembly, "PTR ");

    if (!ptrPos) {
        // No memory reference - check for immediate address (e.g. "PUSH 0040303FH")
        // Only for instructions that typically reference addresses
        if (_strnicmp(assembly, "push ", 5) != 0 &&
            _strnicmp(assembly, "mov ", 4) != 0 &&
            _strnicmp(assembly, "lea ", 4) != 0)
            return;

        const char* p = assembly;
        // Skip mnemonic
        while (*p && *p != ' ') p++;
        while (*p == ' ') p++;
        if (!*p) return;

        // For MOV/LEA, skip to the second operand (after comma)
        if (_strnicmp(assembly, "push ", 5) != 0) {
            p = strchr(p, ',');
            if (!p) return;
            p++;
            while (*p == ' ') p++;
            if (!*p) return;
        }

        // Check if the operand is a pure hex immediate with 'H' suffix
        const char* start = p;
        bool isHex = true;
        int hexLen = 0;
        while (*p && *p != ',' && *p != ' ' && *p != 'H' && *p != 'h') {
            char c = *p;
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                isHex = false;
                break;
            }
            hexLen++;
            p++;
        }
        if (!isHex || hexLen < 5 || hexLen > 8) return;  // At least 5 hex digits for valid address
        if (*p != 'H' && *p != 'h') return;  // Must end with H

        char hexBuf[16] = {0};
        lstrcpyn(hexBuf, start, hexLen + 1);
        DWORD_PTR immAddr = (DWORD_PTR)strtoul(hexBuf, NULL, 16);
        if (immAddr < 0x10000) return;  // Too low to be a valid address

        // Try to read a string at this address
        char strBuf[64] = {0};
        SIZE_T br;
        if (DbgReadMemory(immAddr, strBuf, 63, &br) && br > 0) {
            strBuf[br] = 0;
            // Check if it looks like a readable string (at least 2 printable chars)
            int printCount = 0;
            for (int i = 0; i < (int)br && strBuf[i]; i++) {
                if (strBuf[i] >= 0x20 && strBuf[i] <= 0x7E) printCount++;
                else break;
            }
            if (printCount >= 2) {
                strBuf[40] = 0;  // Truncate long strings
                wsprintf(outComment, "-> \"%s\"", strBuf);
            } else {
                // Not a string, show as DWORD value
                DWORD val = 0;
                if (DbgReadMemory(immAddr, &val, 4, &br) && br == 4) {
                    wsprintf(outComment, "-> %08X", val);
                }
            }
        }
        return;
    }

    int readSize = 4;
    if (ptrPos > assembly + 5) {
        // Check longer prefixes first to avoid "Word" matching tail of "Dword"
        if (_strnicmp(ptrPos - 6, "Dword ", 6) == 0) readSize = 4;
        else if (_strnicmp(ptrPos - 6, "Qword ", 6) == 0) readSize = 8;
        else if (_strnicmp(ptrPos - 5, "Byte ", 5) == 0) readSize = 1;
        else if (_strnicmp(ptrPos - 5, "Word ", 5) == 0) readSize = 2;
    } else if (ptrPos > assembly + 4) {
        if (_strnicmp(ptrPos - 5, "Byte ", 5) == 0) readSize = 1;
        else if (_strnicmp(ptrPos - 5, "Word ", 5) == 0) readSize = 2;
    }

    const char* bracket = strchr(ptrPos, '[');
    if (!bracket) return;
    const char* bracketEnd = strchr(bracket, ']');
    if (!bracketEnd) return;

    char addrContent[64];
    int len = (int)(bracketEnd - bracket - 1);
    if (len <= 0 || len >= 64) return;
    lstrcpyn(addrContent, bracket + 1, len + 1);

    // Evaluate the address expression: can be pure hex, or reg+hex, reg*scale+hex, etc.
    // Examples: "00403560h", "EAX+0040351Fh", "EBP-50h", "EBX*2+ESP-33h"
    // Strategy: replace register names with their values, then evaluate simple +/- expressions

    // Register name to value lookup
    struct { const char* name; DWORD value; } regMap[] = {
        {"EAX", g_DbgRegisters.Eax}, {"EBX", g_DbgRegisters.Ebx},
        {"ECX", g_DbgRegisters.Ecx}, {"EDX", g_DbgRegisters.Edx},
        {"ESI", g_DbgRegisters.Esi}, {"EDI", g_DbgRegisters.Edi},
        {"EBP", g_DbgRegisters.Ebp}, {"ESP", g_DbgRegisters.Esp},
        {"EAX", g_DbgRegisters.Eax}, // lowercase handled via _strnicmp
    };

    // Evaluate: scan tokens separated by + and -
    DWORD_PTR addr = 0;
    char expr[64];
    lstrcpyn(expr, addrContent, 64);

    // Replace '*' scaled index with computed value (e.g. "EBX*2" -> value)
    // Simple approach: split on +/-, resolve each token
    char* p = expr;
    bool valid = true;
    int sign = 1;

    while (*p && valid) {
        // Skip whitespace
        while (*p == ' ') p++;
        if (!*p) break;

        // Check sign
        if (*p == '+') { sign = 1; p++; continue; }
        if (*p == '-') { sign = -1; p++; continue; }

        // Extract token until next +, -, or end
        char token[32] = {0};
        int ti = 0;
        while (*p && *p != '+' && *p != '-' && *p != ' ' && ti < 31) {
            token[ti++] = *p++;
        }
        token[ti] = 0;
        if (ti == 0) { valid = false; break; }

        // Strip trailing 'h'/'H' from hex values
        int tlen = lstrlen(token);
        if (tlen > 0 && (token[tlen-1] == 'h' || token[tlen-1] == 'H'))
            token[tlen-1] = '\0';

        // Check if token contains '*' (scaled index like "EBX*2")
        char* star = strchr(token, '*');
        if (star) {
            *star = '\0';
            char* regPart = token;
            char* scalePart = star + 1;
            DWORD regVal = 0;
            bool foundReg = false;
            for (int r = 0; r < 8; r++) {
                if (_stricmp(regPart, regMap[r].name) == 0) {
                    regVal = regMap[r].value;
                    foundReg = true;
                    break;
                }
            }
            if (foundReg) {
                DWORD scale = (DWORD)strtoul(scalePart, NULL, 10);
                addr += sign * (DWORD_PTR)(regVal * scale);
            } else {
                valid = false;
            }
            sign = 1;
            continue;
        }

        // Try register name
        bool foundReg = false;
        for (int r = 0; r < 8; r++) {
            if (_stricmp(token, regMap[r].name) == 0) {
                addr += sign * (DWORD_PTR)regMap[r].value;
                foundReg = true;
                break;
            }
        }
        if (foundReg) { sign = 1; continue; }

        // Try hex number
        bool isHex = true;
        for (int i = 0; token[i]; i++) {
            char c = token[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                isHex = false; break;
            }
        }
        if (isHex && token[0]) {
            DWORD_PTR val = (DWORD_PTR)strtoul(token, NULL, 16);
            addr += sign * val;
            sign = 1;
            continue;
        }

        // Unknown token - can't resolve
        valid = false;
    }

    if (!valid || addr == 0) return;

    BYTE buf[8] = {0};
    SIZE_T bytesRead;
    if (!DbgReadMemory(addr, buf, readSize, &bytesRead) || bytesRead == 0)
        return;

    // Format the value
    char valStr[32];
    switch (readSize) {
        case 1: wsprintf(valStr, "%02X", buf[0]); break;
        case 2: wsprintf(valStr, "%04X", *(WORD*)buf); break;
        case 4: wsprintf(valStr, "%08X", *(DWORD*)buf); break;
        case 8: wsprintf(valStr, "%08X%08X", *(DWORD*)(buf+4), *(DWORD*)buf); break;
        default: valStr[0] = 0; break;
    }

    // Check for printable ASCII in the value bytes
    char asciiStr[16] = {0};
    int asciiLen = 0;
    for (int i = 0; i < readSize; i++) {
        if (buf[i] >= 0x20 && buf[i] <= 0x7E)
            asciiStr[asciiLen++] = (char)buf[i];
    }
    asciiStr[asciiLen] = 0;

    if (asciiLen >= 2) {
        wsprintf(outComment, "= %s '%s'", valStr, asciiStr);
    } else {
        wsprintf(outComment, "= %s", valStr);
    }
}

// Public wrapper for FileMap.cpp to call
void DbgResolveMemoryCommentPublic(const char* assembly, char* outComment, size_t outSize)
{
    DbgResolveMemoryComment(assembly, outComment, outSize);
}

BOOL DbgDisassembleAtEIP()
{
    if (!g_bDebuggerActive || g_dwCurrentEIP == 0 || !s_hMainWnd)
        return FALSE;

    HWND hListView = GetDlgItem(s_hMainWnd, IDC_DISASM);
    if (!hListView)
        return FALSE;

    // If the file is already disassembled, navigate to EIP in the existing view
    if (DisasmDataLines.size() > 0) {
        // Try both raw EIP and rebased address (static disassembly uses preferred base,
        // live decode uses runtime addresses)
        char addrStr[20], addrStr2[20];
        if (g_dwCurrentEIP > 0xFFFFFFFF) {
            wsprintf(addrStr, "%08X%08X", (DWORD)(g_dwCurrentEIP >> 32), (DWORD)g_dwCurrentEIP);
            DWORD_PTR sa = g_dwCurrentEIP - g_dwRebaseDelta;
            wsprintf(addrStr2, "%08X%08X", (DWORD)(sa >> 32), (DWORD)sa);
        } else {
            wsprintf(addrStr, "%08X", (DWORD)g_dwCurrentEIP);
            wsprintf(addrStr2, "%08X", (DWORD)(g_dwCurrentEIP - g_dwRebaseDelta));
        }

        for (DWORD_PTR i = 0; i < DisasmDataLines.size(); i++) {
            char* itemAddr = DisasmDataLines[i].GetAddress();
            if (itemAddr && (lstrcmp(itemAddr, addrStr) == 0 || lstrcmp(itemAddr, addrStr2) == 0)) {
                SelectItem(hListView, i);
                InvalidateRect(hListView, NULL, FALSE);
                return TRUE;
            }
        }
    }

    // Read the code around EIP from process memory and run full disassembly
    {
        // Find the module that contains EIP by scanning loaded modules
        LPVOID moduleBase = NULL;
        for (size_t m = 0; m < g_DbgModules.size(); m++) {
            // Try each module's base - read its SizeOfImage to check range
            IMAGE_DOS_HEADER dh = {0};
            SIZE_T br2;
            if (ReadProcessMemory(g_DbgProcess.hProcess, g_DbgModules[m].lpBaseOfDll, &dh, sizeof(dh), &br2) &&
                dh.e_magic == IMAGE_DOS_SIGNATURE) {
                BYTE nt[4096] = {0};
                if (ReadProcessMemory(g_DbgProcess.hProcess, (LPBYTE)g_DbgModules[m].lpBaseOfDll + dh.e_lfanew, nt, sizeof(nt), &br2)) {
                    IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)nt;
                    DWORD soi = pNt->OptionalHeader.SizeOfImage;
                    DWORD_PTR modStart = (DWORD_PTR)g_DbgModules[m].lpBaseOfDll;
                    if (g_dwCurrentEIP >= modStart && g_dwCurrentEIP < modStart + soi) {
                        moduleBase = g_DbgModules[m].lpBaseOfDll;
                        break;
                    }
                }
            }
        }
        // Also check the main EXE
        if (!moduleBase && g_DbgProcess.lpBaseOfImage) {
            moduleBase = g_DbgProcess.lpBaseOfImage;
        }

        if (!moduleBase) {
            OutDebug(s_hMainWnd, "Could not find module for EIP");
            return FALSE;
        }

        // Read DOS header from the module containing EIP
        IMAGE_DOS_HEADER dosHdr = {0};
        SIZE_T br;
        if (!ReadProcessMemory(g_DbgProcess.hProcess, moduleBase, &dosHdr, sizeof(dosHdr), &br) ||
            dosHdr.e_magic != IMAGE_DOS_SIGNATURE) {
            OutDebug(s_hMainWnd, "Failed to read DOS header");
            return FALSE;
        }

        // Read NT headers
        BYTE ntBuf[4096] = {0};
        if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPBYTE)moduleBase + dosHdr.e_lfanew, ntBuf, sizeof(ntBuf), &br)) {
            OutDebug(s_hMainWnd, "Failed to read NT headers");
            return FALSE;
        }

        IMAGE_NT_HEADERS* pNt32 = (IMAGE_NT_HEADERS*)ntBuf;
        IMAGE_NT_HEADERS64* pNt64 = (IMAGE_NT_HEADERS64*)ntBuf;
        bool isPe64 = (pNt32->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64);

        WORD numSections = pNt32->FileHeader.NumberOfSections;
        IMAGE_SECTION_HEADER* sections;

        if (isPe64) {
            sections = (IMAGE_SECTION_HEADER*)((BYTE*)&pNt64->OptionalHeader + pNt64->FileHeader.SizeOfOptionalHeader);
        } else {
            sections = (IMAGE_SECTION_HEADER*)((BYTE*)&pNt32->OptionalHeader + pNt32->FileHeader.SizeOfOptionalHeader);
        }

        // Find the section containing EIP
        DWORD_PTR actualBase = (DWORD_PTR)moduleBase;
        DWORD_PTR sectionVA = 0;
        DWORD sectionSize = 0;
        bool foundSection = false;

        for (int s = 0; s < numSections; s++) {
            DWORD_PTR secStart = actualBase + sections[s].VirtualAddress;
            DWORD_PTR secEnd = secStart + sections[s].Misc.VirtualSize;
            if (g_dwCurrentEIP >= secStart && g_dwCurrentEIP < secEnd) {
                sectionVA = secStart;
                sectionSize = sections[s].Misc.VirtualSize;
                foundSection = true;
                char msg[128];
                wsprintf(msg, "EIP in section \"%.8s\" at %08X, size=%d bytes",
                         sections[s].Name, (DWORD)sectionVA, sectionSize);
                OutDebug(s_hMainWnd, msg);
                break;
            }
        }

        if (!foundSection || sectionSize == 0) {
            OutDebug(s_hMainWnd, "No section found for EIP, reading 64KB around it");
            sectionVA = g_dwCurrentEIP & ~0xFFF;  // page-align down
            sectionSize = 65536;
        }

        if (sectionSize > 16 * 1024 * 1024) sectionSize = 16 * 1024 * 1024;

        // Read the entire code section from process memory
        char* buffer = (char*)VirtualAlloc(NULL, sectionSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!buffer) return FALSE;

        SIZE_T bytesRead = 0;
        ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)sectionVA, buffer, sectionSize, &bytesRead);
        if (bytesRead == 0) {
            VirtualFree(buffer, 0, MEM_RELEASE);
            return FALSE;
        }

        {
            char msg[128];
            wsprintf(msg, "Read %d bytes from code section at %08X", (DWORD)bytesRead, (DWORD)sectionVA);
            OutDebug(s_hMainWnd, msg);
        }

        DisasmDataLines.clear();

        // Set correct CPU mode
        DWORD savedCPU = disop.CPU;
        bool savedPe = LoadedPe, savedPe64 = LoadedPe64;
        disop.CPU = isPe64 ? 6 : 0;
        LoadedPe = !isPe64;
        LoadedPe64 = isPe64;
        disop.ShowAddr = 1;
        disop.ShowOpcodes = 1;
        disop.ShowAssembly = 1;
        disop.ShowRemarks = 1;

        // Run FirstPass for function/branch detection
        DWORD_PTR epAddress = g_dwCurrentEIP;
        disop.FirstPass = 1;
        try {
            FirstPass(
                (DWORD_PTR)(g_dwCurrentEIP - sectionVA),  // EP offset within section
                bytesRead,                                  // bytes to analyze
                g_dwCurrentEIP,                             // EP address
                buffer,                                     // code buffer
                sectionVA,                                  // section start
                sectionVA + bytesRead                       // section end
            );
            OutDebug(s_hMainWnd, "FirstPass analysis complete");
        } catch (...) {
            OutDebug(s_hMainWnd, "FirstPass failed");
        }

        // Start decoding from EIP (not section start) to ensure correct instruction alignment
        DISASSEMBLY disasm;
        DWORD_PTR eipOffset = g_dwCurrentEIP - sectionVA;
        if (eipOffset >= bytesRead) eipOffset = 0;
        DWORD_PTR offset = eipOffset;
        DWORD_PTR addr = sectionVA + eipOffset;

        while (offset < bytesRead) {
            DWORD_PTR startOffset = offset;
            FlushDecoded(&disasm);
            disasm.Address = addr;
            Decode(&disasm, buffer, &offset);

            DWORD_PTR instrSize = disasm.OpcodeSize + disasm.PrefixSize;
            if (instrSize == 0) instrSize = 1;
            offset++;
            if (offset <= startOffset) offset = startOffset + 1;

            CDisasmData data;
            char addrStr[20];
            if (addr > 0xFFFFFFFF)
                wsprintf(addrStr, "%08X%08X", (DWORD)(addr >> 32), (DWORD)addr);
            else
                wsprintf(addrStr, "%08X", (DWORD)addr);
            data.SetAddress(addrStr);

            char opcodeStr[64] = {0};
            for (DWORD_PTR j = 0; j < instrSize && j < 15; j++) {
                char byteStr[4];
                wsprintf(byteStr, "%02X ", (BYTE)buffer[startOffset + j]);
                lstrcat(opcodeStr, byteStr);
            }
            data.SetCode(opcodeStr);
            data.SetMnemonic(disasm.Assembly);
            // Resolve memory references to show values in comments
            char memComment[128] = {0};
            if (g_DbgState == DBG_STATE_PAUSED)
                DbgResolveMemoryComment(disasm.Assembly, memComment, sizeof(memComment));
            data.SetComments(memComment[0] ? memComment : (char*)"");
            data.SetReference((char*)"");

            // Use SaveDecoded for proper analysis (function prologue, etc.)
            DisasmDataLines.push_back(data);

            addr += instrSize;
        }

        VirtualFree(buffer, 0, MEM_RELEASE);

        // Build flow arrow and code map data from the decoded instructions
        BuildFlowArrowData();
        BuildCodeMapData();

        disop.CPU = savedCPU;
        LoadedPe = savedPe;
        LoadedPe64 = savedPe64;

        if (!DisassemblerReady) {
            DisassemblerReady = TRUE;
            FilesInMemory = true;  // So CloseLoadedFile cleans up properly
            ShowWindow(hListView, SW_SHOW);

            // Enable toolbar buttons
            SendMessage(hWndTB, TB_ENABLEBUTTON, ID_EP_SHOW,   (LPARAM)TRUE);
            SendMessage(hWndTB, TB_ENABLEBUTTON, ID_CODE_SHOW, (LPARAM)TRUE);
            SendMessage(hWndTB, TB_ENABLEBUTTON, ID_ADDR_SHOW, (LPARAM)TRUE);
            SendMessage(hWndTB, TB_ENABLEBUTTON, ID_SEARCH,    (LPARAM)TRUE);

            // Enable menus
            HMENU hMenu = GetMenu(s_hMainWnd);
            EnableMenuItem(hMenu, IDC_START_DISASM,       MF_ENABLED);
            EnableMenuItem(hMenu, IDC_GOTO_START,         MF_ENABLED);
            EnableMenuItem(hMenu, IDC_GOTO_ENTRYPOINT,    MF_ENABLED);
            EnableMenuItem(hMenu, IDC_GOTO_ADDRESS,       MF_ENABLED);
            EnableMenuItem(hMenu, IDM_FIND,               MF_ENABLED);
            EnableMenuItem(hMenu, IDC_VIEW_DISASSEMBLY,   MF_ENABLED);
            EnableMenuItem(hMenu, IDC_VIEW_GRAPH_TAB,     MF_ENABLED);
            EnableMenuItem(hMenu, IDC_VIEW_GRAPH_DOCK,    MF_ENABLED);
            EnableMenuItem(hMenu, IDC_CODE_MAP,           MF_ENABLED);
            EnableMenuItem(hMenu, IDC_CONTROL_FLOW,       MF_ENABLED);
            EnableMenuItem(hMenu, IDM_COPY_DISASM_CLIP,   MF_ENABLED);
            EnableMenuItem(hMenu, IDM_SELECT_ALL_ITEMS,   MF_ENABLED);
            EnableMenuItem(hMenu, IDC_SHOW_COMMENTS,      MF_ENABLED);
            EnableMenuItem(hMenu, IDC_SHOW_FUNCTIONS,     MF_ENABLED);

            // Show code map bar if enabled
            RefreshCodeMapBar();

            // Show flow arrows panel if enabled
            if (g_FlowArrowsVisible) {
                HWND hArrowPanel = GetDlgItem(s_hMainWnd, IDC_FLOW_ARROWS);
                if (hArrowPanel && !IsWindowVisible(hArrowPanel)) {
                    RECT dr;
                    GetWindowRect(hListView, &dr);
                    MapWindowPoints(HWND_DESKTOP, s_hMainWnd, (LPPOINT)&dr, 2);
                    MoveWindow(hListView, dr.left + g_FlowArrowPanelWidth, dr.top,
                        (dr.right - dr.left) - g_FlowArrowPanelWidth,
                        dr.bottom - dr.top, TRUE);
                    GetWindowRect(hListView, &dr);
                    MapWindowPoints(HWND_DESKTOP, s_hMainWnd, (LPPOINT)&dr, 2);
                    MoveWindow(hArrowPanel, dr.left - g_FlowArrowPanelWidth, dr.top,
                        g_FlowArrowPanelWidth, dr.bottom - dr.top, TRUE);
                    ShowWindow(hArrowPanel, SW_SHOW);
                    InvalidateRect(hArrowPanel, NULL, FALSE);
                }
            }

            // Switch to Disassembly tab and recalculate layout
            SendMessage(s_hMainWnd, WM_USER + 300, 0, 0);
            RECT rc;
            GetClientRect(s_hMainWnd, &rc);
            SendMessage(s_hMainWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rc.right, rc.bottom));
        }

        {
            char msg[64];
            wsprintf(msg, "Decoded %d instructions", (int)DisasmDataLines.size());
            OutDebug(s_hMainWnd, msg);
        }

        SendMessage(hListView, LVM_SETITEMCOUNT, DisasmDataLines.size(), 0);

        // Navigate to EIP (addresses are runtime addresses, no delta needed)
        char eipStr[20];
        if (g_dwCurrentEIP > 0xFFFFFFFF)
            wsprintf(eipStr, "%08X%08X", (DWORD)(g_dwCurrentEIP >> 32), (DWORD)g_dwCurrentEIP);
        else
            wsprintf(eipStr, "%08X", (DWORD)g_dwCurrentEIP);

        bool found = false;
        for (DWORD_PTR i = 0; i < DisasmDataLines.size(); i++) {
            char* itemAddr = DisasmDataLines[i].GetAddress();
            if (itemAddr && lstrcmp(itemAddr, eipStr) == 0) {
                SelectItem(hListView, i);
                found = true;
                break;
            }
        }

        if (!found) {
            char msg[64];
            wsprintf(msg, "EIP %s not found in disassembly", eipStr);
            OutDebug(s_hMainWnd, msg);
            // Scroll to first item as fallback
            SelectItem(hListView, 0);
        }

        InvalidateRect(hListView, NULL, TRUE);
        UpdateWindow(hListView);
    }

    return TRUE;
}

// ================================================================
// =========================  UI  =================================
// ================================================================

void DbgInitMenuState(HWND hWnd)
{
    HMENU hMenu = GetMenu(hWnd);
    EnableMenuItem(hMenu, IDM_DBG_START, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DBG_ATTACH, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DBG_OPTIONS, MF_ENABLED);
    EnableMenuItem(hMenu, IDM_DBG_PAUSE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_TERMINATE, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_DETACH, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_REFRESH_MEM, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_SNAPSHOT, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_STEP_INTO, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_STEP_OVER, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_RUN_UNTIL_RET, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_RUN_TO_CURSOR, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_TOGGLE_BREAKPOINT, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_VIEW_REGISTERS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_VIEW_THREADS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_VIEW_BREAKPOINTS, MF_GRAYED);
    EnableMenuItem(hMenu, IDM_DBG_RESUME, MF_GRAYED);
}

void DbgUpdateMenuState(HWND hWnd)
{
    HMENU hMenu = GetMenu(hWnd);

    switch (g_DbgState) {
    case DBG_STATE_IDLE:
        EnableMenuItem(hMenu, IDM_DBG_START, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_ATTACH, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_OPTIONS, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_PAUSE, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_TERMINATE, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_DETACH, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_REFRESH_MEM, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_SNAPSHOT, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_STEP_INTO, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_STEP_OVER, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_RUN_UNTIL_RET, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_RUN_TO_CURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_TOGGLE_BREAKPOINT, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_REGISTERS, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_THREADS, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_BREAKPOINTS, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_RESUME, MF_GRAYED);
        break;

    case DBG_STATE_RUNNING:
    case DBG_STATE_STEPPING_OVER:
    case DBG_STATE_STEPPING_OUT:
    case DBG_STATE_RUN_TO_CURSOR:
        EnableMenuItem(hMenu, IDM_DBG_START, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_ATTACH, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_OPTIONS, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_PAUSE, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_TERMINATE, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_DETACH, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_REFRESH_MEM, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_SNAPSHOT, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_STEP_INTO, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_STEP_OVER, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_RUN_UNTIL_RET, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_RUN_TO_CURSOR, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_TOGGLE_BREAKPOINT, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_REGISTERS, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_THREADS, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_BREAKPOINTS, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_RESUME, MF_GRAYED);
        break;

    case DBG_STATE_PAUSED:
    case DBG_STATE_STEPPING_INTO:
        EnableMenuItem(hMenu, IDM_DBG_START, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_ATTACH, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_OPTIONS, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_PAUSE, MF_GRAYED);
        EnableMenuItem(hMenu, IDM_DBG_TERMINATE, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_DETACH, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_REFRESH_MEM, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_SNAPSHOT, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_STEP_INTO, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_STEP_OVER, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_RUN_UNTIL_RET, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_RUN_TO_CURSOR, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_TOGGLE_BREAKPOINT, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_REGISTERS, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_THREADS, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_VIEW_BREAKPOINTS, MF_ENABLED);
        EnableMenuItem(hMenu, IDM_DBG_RESUME, MF_ENABLED);
        break;
    }
}

// ================================================================
// ===================  DIALOG PROCEDURES  ========================
// ================================================================

BOOL CALLBACK DbgRegisterDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
    case WM_INITDIALOG: {
        // Set early so DbgUpdateRegisterDialog can find us
        g_hRegisterDlg = hWnd;
        DbgUpdateRegisterDialog();
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_REG_APPLY: {
            char buf[16];

            GetDlgItemText(hWnd, IDC_REG_EAX, buf, 16); g_DbgRegisters.Eax = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_EBX, buf, 16); g_DbgRegisters.Ebx = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_ECX, buf, 16); g_DbgRegisters.Ecx = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_EDX, buf, 16); g_DbgRegisters.Edx = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_ESI, buf, 16); g_DbgRegisters.Esi = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_EDI, buf, 16); g_DbgRegisters.Edi = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_EBP, buf, 16); g_DbgRegisters.Ebp = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_ESP, buf, 16); g_DbgRegisters.Esp = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_EIP, buf, 16); g_DbgRegisters.Eip = StringToDword(buf);
            GetDlgItemText(hWnd, IDC_REG_EFLAGS, buf, 16); g_DbgRegisters.EFlags = StringToDword(buf);

            DbgWriteRegisters();
            g_dwCurrentEIP = g_DbgRegisters.Eip;

            if (s_hMainWnd) {
                DbgDisassembleAtEIP();
            }
            return TRUE;
        }

        case IDC_REG_REFRESH:
            DbgReadRegisters();
            DbgUpdateRegisterDialog();
            return TRUE;

        case IDCANCEL:
            DestroyWindow(hWnd);
            g_hRegisterDlg = NULL;
            return TRUE;
        }
        break;

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        if (g_DarkMode) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_DarkTextColor);
            SetBkColor(hdc, g_DarkBkColor);
            return (INT_PTR)g_hDarkBrush;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        g_hRegisterDlg = NULL;
        return TRUE;
    }
    return FALSE;
}

// Populate the process list, optionally filtering by name
static void DbgPopulateProcessList(HWND hList, const char* filter)
{
    ListView_DeleteAllItems(hList);

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(pe32);
    int idx = 0;

    if (Process32First(hSnap, &pe32)) {
        do {
            if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == 4) continue;

            // Apply filter (case-insensitive substring match on process name or PID)
            if (filter && filter[0]) {
                char nameLower[MAX_PATH], filterLower[MAX_PATH], pidStr[16];
                lstrcpyn(nameLower, pe32.szExeFile, MAX_PATH);
                lstrcpyn(filterLower, filter, MAX_PATH);
                CharLowerA(nameLower);
                CharLowerA(filterLower);
                wsprintf(pidStr, "%d", pe32.th32ProcessID);
                if (!strstr(nameLower, filterLower) && !strstr(pidStr, filter))
                    continue;
            }

            char pidStr[16];
            wsprintf(pidStr, "%d", pe32.th32ProcessID);

            LVITEM lvi;
            lvi.mask = LVIF_TEXT;
            lvi.iItem = idx;
            lvi.iSubItem = 0;
            lvi.pszText = pidStr;
            ListView_InsertItem(hList, &lvi);

            ListView_SetItemText(hList, idx, 1, pe32.szExeFile);

            char* elevStr = (char*)"?";
            char* archStr = (char*)"?";

            HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe32.th32ProcessID);
            if (hProc) {
                HANDLE hToken = NULL;
                if (OpenProcessToken(hProc, TOKEN_QUERY, &hToken)) {
                    TOKEN_ELEVATION elev = {0};
                    DWORD size = sizeof(elev);
                    if (GetTokenInformation(hToken, TokenElevation, &elev, sizeof(elev), &size)) {
                        elevStr = elev.TokenIsElevated ? (char*)"Yes" : (char*)"No";
                    }
                    CloseHandle(hToken);
                }

                BOOL isWow64 = FALSE;
                if (IsWow64Process(hProc, &isWow64)) {
                    archStr = isWow64 ? (char*)"32" : (char*)"64";
                }

                CloseHandle(hProc);
            } else {
                elevStr = (char*)"Yes";
            }

            ListView_SetItemText(hList, idx, 2, elevStr);
            ListView_SetItemText(hList, idx, 3, archStr);
            idx++;
        } while (Process32Next(hSnap, &pe32));
    }
    CloseHandle(hSnap);
}

BOOL CALLBACK DbgAttachDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HWND hProcessList = NULL;

    switch (Message) {
    case WM_INITDIALOG: {
        hProcessList = GetDlgItem(hWnd, IDC_DBG_PROCESS_LIST);
        if (!hProcessList)
            break;

        SendMessage(hProcessList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                    LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;

        lvc.pszText = (LPSTR)"PID";
        lvc.cx = 55;
        ListView_InsertColumn(hProcessList, 0, &lvc);

        lvc.pszText = (LPSTR)"Process Name";
        lvc.cx = 160;
        ListView_InsertColumn(hProcessList, 1, &lvc);

        lvc.pszText = (LPSTR)"Elevated";
        lvc.cx = 55;
        ListView_InsertColumn(hProcessList, 2, &lvc);

        lvc.pszText = (LPSTR)"Arch";
        lvc.cx = 40;
        ListView_InsertColumn(hProcessList, 3, &lvc);

        DbgPopulateProcessList(hProcessList, NULL);

        // Apply dark mode
        if (g_DarkMode) {
            ListView_SetBkColor(hProcessList, g_DarkBkColor);
            ListView_SetTextBkColor(hProcessList, g_DarkBkColor);
            ListView_SetTextColor(hProcessList, g_DarkTextColor);
        }

        // Focus the filter edit
        SetFocus(GetDlgItem(hWnd, IDC_DBG_FILTER));
        return FALSE;  // FALSE because we set focus manually
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_DBG_FILTER: {
            if (HIWORD(wParam) == EN_CHANGE) {
                char filter[128] = {0};
                GetDlgItemText(hWnd, IDC_DBG_FILTER, filter, 128);
                DbgPopulateProcessList(hProcessList, filter);

                // Re-apply dark mode colors after repopulating
                if (g_DarkMode) {
                    ListView_SetBkColor(hProcessList, g_DarkBkColor);
                    ListView_SetTextBkColor(hProcessList, g_DarkBkColor);
                    ListView_SetTextColor(hProcessList, g_DarkTextColor);
                }
            }
            return TRUE;
        }
        case IDOK: {
            // Get selected process PID
            int sel = ListView_GetNextItem(hProcessList, -1, LVNI_SELECTED);
            if (sel == -1) {
                MessageBox(hWnd, "Please select a process.", "Attach", MB_OK | MB_ICONINFORMATION);
                return TRUE;
            }

            char pidStr[16];
            ListView_GetItemText(hProcessList, sel, 0, pidStr, 16);
            DWORD pid = (DWORD)atoi(pidStr);

            HWND hParent = GetParent(hWnd);
            EndDialog(hWnd, IDOK);

            if (pid > 0) {
                DbgAttachToProcess(hParent, pid);
            }
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        if (g_DarkMode) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_DarkTextColor);
            SetBkColor(hdc, g_DarkBkColor);
            return (INT_PTR)g_hDarkBrush;
        }
        break;
    }
    return FALSE;
}

BOOL CALLBACK DbgOptionsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
    case WM_INITDIALOG: {
        // Pre-fill with current process options
        SetDlgItemText(hWnd, IDC_DBG_EXE_PATH, g_DbgProcess.szExePath);
        SetDlgItemText(hWnd, IDC_DBG_CMD_LINE, g_DbgProcess.szCmdLine);
        SetDlgItemText(hWnd, IDC_DBG_WORK_DIR, g_DbgProcess.szWorkDir);

        if (g_DarkMode) {
            // Dark mode via WM_CTLCOLOR
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_DBG_BROWSE: {
            OPENFILENAMEA ofn;
            char szFile[MAX_PATH] = {0};
            memset(&ofn, 0, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = "Executable Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

            if (GetOpenFileNameA(&ofn)) {
                SetDlgItemText(hWnd, IDC_DBG_EXE_PATH, szFile);
            }
            return TRUE;
        }

        case IDOK: {
            GetDlgItemText(hWnd, IDC_DBG_EXE_PATH, g_DbgProcess.szExePath, MAX_PATH);
            GetDlgItemText(hWnd, IDC_DBG_CMD_LINE, g_DbgProcess.szCmdLine, MAX_PATH);
            GetDlgItemText(hWnd, IDC_DBG_WORK_DIR, g_DbgProcess.szWorkDir, MAX_PATH);
            EndDialog(hWnd, IDOK);
            return TRUE;
        }

        case IDCANCEL:
            EndDialog(hWnd, IDCANCEL);
            return TRUE;
        }
        break;

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
        if (g_DarkMode) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_DarkTextColor);
            SetBkColor(hdc, g_DarkBkColor);
            return (INT_PTR)g_hDarkBrush;
        }
        break;
    }
    return FALSE;
}

// ================================================================
// =================  THREADS DIALOG  =============================
// ================================================================

// Find which module an address belongs to
const char* DbgFindModuleForAddress(DWORD_PTR addr)
{
    for (size_t i = 0; i < g_DbgModules.size(); i++) {
        DWORD_PTR base = (DWORD_PTR)g_DbgModules[i].lpBaseOfDll;
        if (addr >= base && addr < base + 0x1000000) {
            char* name = g_DbgModules[i].szModuleName;
            char* slash = strrchr(name, '\\');
            if (!slash) slash = strrchr(name, '/');
            if (slash) return slash + 1;
            return name;
        }
    }
    return NULL;
}

// Display cached thread info - no context reads, instant
void DbgUpdateThreadsDialog()
{
    // g_hThreadsDlg may not be set yet during WM_INITDIALOG,
    // so also accept if called with a valid window directly
    HWND hDlg = g_hThreadsDlg;
    if (!hDlg || !IsWindow(hDlg))
        return;

    HWND hList = GetDlgItem(hDlg, IDC_DBG_THREAD_LIST);
    if (!hList) return;

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    for (int i = 0; i < (int)s_threadCache.size(); i++) {
        THREAD_CACHE_ENTRY& e = s_threadCache[i];
        char tidDec[16], tidHex[16], state[16], eipStr[20];
        wsprintf(tidDec, "%d", e.dwThreadId);
        wsprintf(tidHex, "%X", e.dwThreadId);
        lstrcpy(state, g_DbgState == DBG_STATE_PAUSED ? "Suspended" : "Running");
        wsprintf(eipStr, "%08X", (DWORD)e.dwEip);

        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = tidDec;
        ListView_InsertItem(hList, &lvi);
        ListView_SetItemText(hList, i, 1, tidHex);
        ListView_SetItemText(hList, i, 2, state);
        ListView_SetItemText(hList, i, 3, e.szName);
        ListView_SetItemText(hList, i, 4, eipStr);
        ListView_SetItemText(hList, i, 5, e.szModule);
    }

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hList, NULL, TRUE);

    if (g_DarkMode) {
        ListView_SetBkColor(hList, g_DarkBkColor);
        ListView_SetTextBkColor(hList, g_DarkBkColor);
        ListView_SetTextColor(hList, g_DarkTextColor);
    }
}

void DbgUpdateDisasmTabName()
{
    if (!s_hMainWnd || !g_bDebuggerActive) return;

    const char* modName = DbgFindModuleForAddress(g_dwCurrentEIP);

    // Update Disassembly tab text with module name
    HWND hTab = GetDlgItem(s_hMainWnd, IDC_TAB_MAIN);
    if (!hTab) return;

    char tabText[128];
    if (modName)
        wsprintf(tabText, "Disassembly (%s)", modName);
    else
        lstrcpy(tabText, "Disassembly");

    // Calculate required tab width based on text
    HDC hdc = GetDC(hTab);
    HFONT hFont = (HFONT)SendMessage(hTab, WM_GETFONT, 0, 0);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SIZE textSize;
    GetTextExtentPoint32(hdc, tabText, lstrlen(tabText), &textSize);
    SelectObject(hdc, hOldFont);
    ReleaseDC(hTab, hdc);

    // Add padding for close button and margins
    int tabWidth = textSize.cx + 30;
    if (tabWidth < 100) tabWidth = 100;

    // Resize tabs: set first tab wider, keep Graph at 100
    SendMessage(hTab, TCM_SETITEMSIZE, 0, MAKELPARAM(tabWidth, 20));

    TCITEM tie = {0};
    tie.mask = TCIF_TEXT;
    tie.pszText = tabText;
    TabCtrl_SetItem(hTab, 0, &tie);

    InvalidateRect(hTab, NULL, TRUE);

    // Also update window title
    char title[256];
    if (modName)
        wsprintf(title, "%s - [Debugging: %s]", PVDASM, modName);
    else
        wsprintf(title, "%s - [Debugging]", PVDASM);
    SetWindowText(s_hMainWnd, title);
}

void DbgSwitchToThread(DWORD dwThreadId)
{
    if (!g_bDebuggerActive || g_DbgState != DBG_STATE_PAUSED)
        return;

    s_dwPausedThreadId = dwThreadId;
    memcpy(&g_DbgPrevRegisters, &g_DbgRegisters, sizeof(DEBUG_REGISTERS));
    DbgReadRegisters();
    DbgUpdateRegisterDialog();
    DbgDisassembleAtEIP();

    // Update Disassembly tab name with module
    DbgUpdateDisasmTabName();

    if (s_hMainWnd) {
        char msg[64];
        wsprintf(msg, "Switched to thread %d (EIP: %08X)", dwThreadId, (DWORD)g_dwCurrentEIP);
        OutDebug(s_hMainWnd, msg);
        SetDlgItemText(s_hMainWnd, IDC_MESSAGE1, msg);
    }
}

BOOL CALLBACK DbgThreadsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
    case WM_INITDIALOG: {
        // Set early so DbgUpdateThreadsDialog can find us
        g_hThreadsDlg = hWnd;

        HWND hList = GetDlgItem(hWnd, IDC_DBG_THREAD_LIST);
        if (!hList) break;

        SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                    LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;

        lvc.pszText = (LPSTR)"Decimal";
        lvc.cx = 55;
        ListView_InsertColumn(hList, 0, &lvc);

        lvc.pszText = (LPSTR)"Hex";
        lvc.cx = 45;
        ListView_InsertColumn(hList, 1, &lvc);

        lvc.pszText = (LPSTR)"State";
        lvc.cx = 60;
        ListView_InsertColumn(hList, 2, &lvc);

        lvc.pszText = (LPSTR)"Name";
        lvc.cx = 70;
        ListView_InsertColumn(hList, 3, &lvc);

        lvc.pszText = (LPSTR)"EIP";
        lvc.cx = 70;
        ListView_InsertColumn(hList, 4, &lvc);

        lvc.pszText = (LPSTR)"Module";
        lvc.cx = 80;
        ListView_InsertColumn(hList, 5, &lvc);

        DbgUpdateThreadsDialog();
        return TRUE;
    }

    case WM_NOTIFY: {
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm->idFrom == IDC_DBG_THREAD_LIST && pnm->code == NM_DBLCLK) {
            HWND hList = GetDlgItem(hWnd, IDC_DBG_THREAD_LIST);
            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (sel != -1) {
                char tidStr[16];
                ListView_GetItemText(hList, sel, 0, tidStr, 16);
                DWORD tid = (DWORD)atoi(tidStr);
                if (tid != 0) {
                    DbgSwitchToThread(tid);
                    DbgUpdateThreadsDialog();
                }
            }
        }
        break;
    }

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        if (g_DarkMode) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_DarkTextColor);
            SetBkColor(hdc, g_DarkBkColor);
            return (INT_PTR)g_hDarkBrush;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        g_hThreadsDlg = NULL;
        return TRUE;
    }
    return FALSE;
}

// ================================================================
// ===============  BREAKPOINTS DIALOG  ===========================
// ================================================================

void DbgUpdateBreakpointsDialog()
{
    if (!g_hBreakpointsDlg || !IsWindow(g_hBreakpointsDlg))
        return;

    HWND hList = GetDlgItem(g_hBreakpointsDlg, IDC_DBG_BP_LIST);
    if (!hList) return;

    SendMessage(hList, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(hList);

    EnterCriticalSection(&g_csBreakpoints);
    for (int i = 0; i < (int)g_DbgBreakpoints.size(); i++) {
        char addrStr[20], hitsStr[16], stateStr[16], moduleStr[64];
        wsprintf(addrStr, "%08X", (DWORD)g_DbgBreakpoints[i].dwAddress);
        wsprintf(hitsStr, "%d", g_DbgBreakpoints[i].nHitCount);
        lstrcpy(stateStr, g_DbgBreakpoints[i].bEnabled ? "Enabled" : "Disabled");

        const char* mod = DbgFindModuleForAddress(g_DbgBreakpoints[i].dwAddress);
        lstrcpyn(moduleStr, mod ? mod : "", 64);

        LVITEM lvi = {0};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = i;
        lvi.pszText = addrStr;
        ListView_InsertItem(hList, &lvi);
        ListView_SetItemText(hList, i, 1, stateStr);
        ListView_SetItemText(hList, i, 2, hitsStr);
        ListView_SetItemText(hList, i, 3, moduleStr);
    }
    LeaveCriticalSection(&g_csBreakpoints);

    SendMessage(hList, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(hList, NULL, TRUE);

    if (g_DarkMode) {
        ListView_SetBkColor(hList, g_DarkBkColor);
        ListView_SetTextBkColor(hList, g_DarkBkColor);
        ListView_SetTextColor(hList, g_DarkTextColor);
    }
}

BOOL CALLBACK DbgBreakpointsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message) {
    case WM_INITDIALOG: {
        g_hBreakpointsDlg = hWnd;

        HWND hList = GetDlgItem(hWnd, IDC_DBG_BP_LIST);
        if (!hList) break;

        SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                    LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;

        lvc.pszText = (LPSTR)"Address";
        lvc.cx = 75;
        ListView_InsertColumn(hList, 0, &lvc);

        lvc.pszText = (LPSTR)"State";
        lvc.cx = 55;
        ListView_InsertColumn(hList, 1, &lvc);

        lvc.pszText = (LPSTR)"Hits";
        lvc.cx = 40;
        ListView_InsertColumn(hList, 2, &lvc);

        lvc.pszText = (LPSTR)"Module";
        lvc.cx = 80;
        ListView_InsertColumn(hList, 3, &lvc);

        DbgUpdateBreakpointsDialog();
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_DBG_BP_REMOVE: {
            HWND hList = GetDlgItem(hWnd, IDC_DBG_BP_LIST);
            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (sel != -1) {
                char addrStr[20];
                ListView_GetItemText(hList, sel, 0, addrStr, 20);
                DWORD_PTR addr = (DWORD_PTR)StringToDword(addrStr);
                DbgRemoveBreakpoint(addr);
                DbgUpdateBreakpointsDialog();
                // Repaint disassembly to remove red highlight
                if (s_hMainWnd)
                    InvalidateRect(GetDlgItem(s_hMainWnd, IDC_DISASM), NULL, TRUE);
            }
            return TRUE;
        }

        case IDC_DBG_BP_CLEAR_ALL: {
            EnterCriticalSection(&g_csBreakpoints);
            // Restore all original bytes
            for (size_t i = 0; i < g_DbgBreakpoints.size(); i++) {
                if (g_DbgBreakpoints[i].bEnabled) {
                    SIZE_T written;
                    WriteProcessMemory(g_DbgProcess.hProcess,
                        (LPVOID)g_DbgBreakpoints[i].dwAddress,
                        &g_DbgBreakpoints[i].bOriginalByte, 1, &written);
                    FlushInstructionCache(g_DbgProcess.hProcess,
                        (LPVOID)g_DbgBreakpoints[i].dwAddress, 1);
                }
            }
            g_DbgBreakpoints.clear();
            LeaveCriticalSection(&g_csBreakpoints);

            DbgUpdateBreakpointsDialog();
            if (s_hMainWnd)
                InvalidateRect(GetDlgItem(s_hMainWnd, IDC_DISASM), NULL, TRUE);
            return TRUE;
        }
        }
        break;

    case WM_NOTIFY: {
        NMHDR* pnm = (NMHDR*)lParam;
        if (pnm->idFrom == IDC_DBG_BP_LIST && pnm->code == NM_DBLCLK) {
            // Double-click: jump to breakpoint in disassembly
            HWND hList = GetDlgItem(hWnd, IDC_DBG_BP_LIST);
            int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (sel != -1 && s_hMainWnd) {
                char addrStr[20];
                ListView_GetItemText(hList, sel, 0, addrStr, 20);

                // Search in disassembly and navigate
                HWND hDisasm = GetDlgItem(s_hMainWnd, IDC_DISASM);
                if (hDisasm) {
                    // Search with both raw address and minus delta
                    DWORD_PTR bpAddr = (DWORD_PTR)StringToDword(addrStr);
                    char searchStr[20], searchStr2[20];
                    wsprintf(searchStr, "%08X", (DWORD)bpAddr);
                    wsprintf(searchStr2, "%08X", (DWORD)(bpAddr - g_dwRebaseDelta));

                    for (DWORD_PTR i = 0; i < DisasmDataLines.size(); i++) {
                        char* itemAddr = DisasmDataLines[i].GetAddress();
                        if (itemAddr && (lstrcmp(itemAddr, searchStr) == 0 ||
                                         lstrcmp(itemAddr, searchStr2) == 0)) {
                            SendMessage(s_hMainWnd, WM_USER + 300, 0, 0);
                            SelectItem(hDisasm, i);
                            SetFocus(hDisasm);
                            break;
                        }
                    }
                }
            }
        }
        break;
    }

    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        if (g_DarkMode) {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_DarkTextColor);
            SetBkColor(hdc, g_DarkBkColor);
            return (INT_PTR)g_hDarkBrush;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hWnd);
        g_hBreakpointsDlg = NULL;
        return TRUE;
    }
    return FALSE;
}
