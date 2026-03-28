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

    // If paused, resume before detaching
    if (g_DbgState == DBG_STATE_PAUSED) {
        s_dwContinueStatus = DBG_CONTINUE;
        g_DbgState = DBG_STATE_RUNNING;
        SetEvent(g_hContinueEvent);
    }

    DebugActiveProcessStop(g_DbgProcess.dwProcessId);
    DbgCleanup();
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

        switch (debugEvent.dwDebugEventCode) {

        case CREATE_PROCESS_DEBUG_EVENT: {
            CREATE_PROCESS_DEBUG_INFO* cpdi = &debugEvent.u.CreateProcessInfo;

            g_DbgProcess.lpBaseOfImage = cpdi->lpBaseOfImage;
            g_DbgProcess.dwEntryPoint = (DWORD_PTR)cpdi->lpStartAddress;

            // Compute ASLR rebase delta: actual load address vs PE preferred ImageBase
            {
                DWORD_PTR preferredBase = 0;
                if (LoadedPe64 && NTheader64)
                    preferredBase = (DWORD_PTR)NTheader64->OptionalHeader.ImageBase;
                else if (LoadedPe && NTheader)
                    preferredBase = (DWORD_PTR)NTheader->OptionalHeader.ImageBase;

                g_dwRebaseDelta = (DWORD_PTR)cpdi->lpBaseOfImage - preferredBase;
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
                    } else {
                        // Remember to re-enable after single step
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
    if (g_DbgState != DBG_STATE_PAUSED) {
        char msg[64];
        wsprintf(msg, "StepInto: not paused (state=%d)", g_DbgState);
        OutDebug(s_hMainWnd, msg);
        return FALSE;
    }

    // Always use the main thread for stepping
    // (the attach breakpoint thread is temporary and exits immediately)
    HANDLE hThread = g_DbgProcess.hMainThread;
    if (!hThread) {
        OutDebug(s_hMainWnd, "StepInto: no main thread handle");
        return FALSE;
    }

    // Set Trap Flag (TF) for single step
    // Use native CONTEXT directly to avoid truncating 64-bit registers
#ifdef _M_X64
    if (g_DbgProcess.bIsWow64) {
        WOW64_CONTEXT ctx = {0};
        ctx.ContextFlags = WOW64_CONTEXT_CONTROL;
        if (!Wow64GetThreadContext(hThread, &ctx)) return FALSE;
        ctx.EFlags |= 0x100;
        Wow64SetThreadContext(hThread, &ctx);
    } else {
        CONTEXT ctx64 = {0};
        ctx64.ContextFlags = CONTEXT_CONTROL;
        if (!GetThreadContext(hThread, &ctx64)) return FALSE;
        ctx64.EFlags |= 0x100;
        SetThreadContext(hThread, &ctx64);
    }
#else
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_CONTROL;
    if (!GetThreadContext(hThread, &ctx)) return FALSE;
    ctx.EFlags |= 0x100;
    SetThreadContext(hThread, &ctx);
#endif

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

void DbgUpdateRegisterDialog()
{
    if (!g_hRegisterDlg || !IsWindow(g_hRegisterDlg))
        return;

    char buf[32];

    wsprintf(buf, "%08X", g_DbgRegisters.Eax); SetDlgItemText(g_hRegisterDlg, IDC_REG_EAX, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Ebx); SetDlgItemText(g_hRegisterDlg, IDC_REG_EBX, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Ecx); SetDlgItemText(g_hRegisterDlg, IDC_REG_ECX, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Edx); SetDlgItemText(g_hRegisterDlg, IDC_REG_EDX, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Esi); SetDlgItemText(g_hRegisterDlg, IDC_REG_ESI, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Edi); SetDlgItemText(g_hRegisterDlg, IDC_REG_EDI, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Ebp); SetDlgItemText(g_hRegisterDlg, IDC_REG_EBP, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Esp); SetDlgItemText(g_hRegisterDlg, IDC_REG_ESP, buf);
    wsprintf(buf, "%08X", g_DbgRegisters.Eip); SetDlgItemText(g_hRegisterDlg, IDC_REG_EIP, buf);
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
            data.SetComments((char*)"");
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
            EnableMenuItem(hMenu, IDC_VIEW_GRAPH,         MF_ENABLED);
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
        break;

    case DBG_STATE_PAUSED:
    case DBG_STATE_STEPPING_INTO:
        EnableMenuItem(hMenu, IDM_DBG_START, MF_ENABLED);    // F9 = Resume
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
        DbgUpdateRegisterDialog();

        // Apply dark mode
        if (g_DarkMode) {
            // Dark mode will be applied via WM_CTLCOLOR* messages in the parent
        }
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

void DbgSwitchToThread(DWORD dwThreadId)
{
    if (!g_bDebuggerActive || g_DbgState != DBG_STATE_PAUSED)
        return;

    s_dwPausedThreadId = dwThreadId;
    memcpy(&g_DbgPrevRegisters, &g_DbgRegisters, sizeof(DEBUG_REGISTERS));
    DbgReadRegisters();
    DbgUpdateRegisterDialog();
    DbgDisassembleAtEIP();

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
