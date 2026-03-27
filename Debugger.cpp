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
extern IMAGE_NT_HEADERS*    NTheader;
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

// Parameters passed from UI thread to debug thread
static char     s_szStartCmdLine[MAX_PATH * 2] = {0};
static char     s_szStartWorkDir[MAX_PATH] = {0};

// ================================================================
// ===================  PROCESS CONTROL  ==========================
// ================================================================

BOOL DbgStartProcess(HWND hMainWnd, const char* szExePath, const char* szCmdLine, const char* szWorkDir)
{
    if (g_bDebuggerActive)
        return FALSE;

    // Clean up temp files from previous sessions (silently skips locked ones)
    DbgCleanupOldTempFiles();

    // Copy the EXE to a temp file to avoid sharing violation with PVDasm's
    // PAGE_READWRITE file mapping that locks the original file.
    char tempDir[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);

    const char* exeName = strrchr(szExePath, '\\');
    if (!exeName) exeName = strrchr(szExePath, '/');
    exeName = exeName ? exeName + 1 : szExePath;

    wsprintf(s_szTempExePath, "%spvdasm_dbg_%u_%s", tempDir, GetTickCount(), exeName);

    // Write mapped file data to temp (avoids sharing violation entirely)
    if (!OrignalData || hFileSize == 0) {
        OutDebug(hMainWnd, "No file mapped in memory");
        s_szTempExePath[0] = 0;
        return FALSE;
    }

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
        char err[256];
        wsprintf(err, "Failed to write temp file: error %d", GetLastError());
        OutDebug(hMainWnd, err);
        DeleteFileA(s_szTempExePath);
        s_szTempExePath[0] = 0;
        return FALSE;
    }

    // Prepare command line and working directory for the debug thread
    if (szCmdLine && szCmdLine[0]) {
        wsprintf(s_szStartCmdLine, "\"%s\" %s", s_szTempExePath, szCmdLine);
    } else {
        wsprintf(s_szStartCmdLine, "\"%s\"", s_szTempExePath);
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

BOOL DbgAttachToProcess(HWND hMainWnd, DWORD dwProcessId)
{
    if (g_bDebuggerActive)
        return FALSE;

    if (!DebugActiveProcess(dwProcessId)) {
        char err[128];
        wsprintf(err, "DebugActiveProcess failed: error %d", GetLastError());
        OutDebug(hMainWnd, err);
        return FALSE;
    }

    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
    if (!hProcess) {
        DebugActiveProcessStop(dwProcessId);
        OutDebug(hMainWnd, "OpenProcess failed");
        return FALSE;
    }

    memset(&g_DbgProcess, 0, sizeof(g_DbgProcess));
    g_DbgProcess.dwProcessId = dwProcessId;
    g_DbgProcess.hProcess = hProcess;
    g_DbgProcess.bAttached = true;

    s_hMainWnd = hMainWnd;
    s_nInitialBreakpoints = 2;
    s_dwReEnableBPAddr = 0;
    g_bDebuggerActive = true;
    g_DbgState = DBG_STATE_RUNNING;

    if (!g_hContinueEvent)
        g_hContinueEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ResetEvent(g_hContinueEvent);

    InitializeCriticalSection(&g_csBreakpoints);

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
static void DbgPauseAtEvent(DWORD dwThreadId, DWORD_PTR dwAddress, UINT uMsg)
{
    g_DbgState = DBG_STATE_PAUSED;
    s_dwPausedThreadId = dwThreadId;
    g_dwCurrentEIP = dwAddress;

    // Save previous registers for change detection
    memcpy(&g_DbgPrevRegisters, &g_DbgRegisters, sizeof(DEBUG_REGISTERS));
    DbgReadRegisters();

    PostMessage(s_hMainWnd, uMsg, (WPARAM)dwThreadId, (LPARAM)dwAddress);
}

// ================================================================
// ====================  DEBUG EVENT LOOP  ========================
// ================================================================

DWORD WINAPI DebugEventLoop(LPVOID lpParam)
{
    DEBUG_EVENT debugEvent;

    // CreateProcess must happen on the SAME thread as WaitForDebugEvent.
    // Windows delivers debug events only to the thread that started debugging.
    if (!g_DbgProcess.bAttached) {
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

        OutDebug(s_hMainWnd, "Process created, waiting for debug events...");
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

            DbgReadModuleName(g_DbgProcess.hProcess, cpdi->hFile,
                              cpdi->lpBaseOfImage, cpdi->lpImageName,
                              cpdi->fUnicode, g_DbgProcess.szExePath, MAX_PATH);

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

                // Initial system breakpoints (ntdll + WOW64 ntdll32)
                if (s_nInitialBreakpoints > 0) {
                    s_nInitialBreakpoints--;

                    // Set entry point BP only once (on the first initial breakpoint)
                    if (g_DbgProcess.dwEntryPoint != 0 && !DbgFindBreakpoint(g_DbgProcess.dwEntryPoint)) {
                        BOOL bpSet = DbgSetBreakpoint(g_DbgProcess.dwEntryPoint, true);
                        char msg[128];
                        wsprintf(msg, "Entry point BP at %08X: %s", (DWORD)g_DbgProcess.dwEntryPoint, bpSet ? "OK" : "FAILED");
                        OutDebug(s_hMainWnd, msg);
                    }

                    s_dwContinueStatus = DBG_CONTINUE;
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

                    // Set EIP back to the breakpoint address (INT3 advances EIP past it)
                    {
                        HANDLE hThread = DbgGetThreadHandle(debugEvent.dwThreadId);
                        if (hThread) {
#ifdef _M_X64
                            WOW64_CONTEXT ctx = {0};
                            ctx.ContextFlags = WOW64_CONTEXT_CONTROL;
                            Wow64GetThreadContext(hThread, &ctx);
                            ctx.Eip = (DWORD)bp->dwAddress;
                            Wow64SetThreadContext(hThread, &ctx);
#else
                            CONTEXT ctx;
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
    if (g_DbgState != DBG_STATE_PAUSED)
        return FALSE;

    HANDLE hThread = DbgGetThreadHandle(s_dwPausedThreadId);
    if (!hThread)
        return FALSE;

    // Set Trap Flag (TF) for single step
#ifdef _M_X64
    // 64-bit PVDasm debugging 32-bit process: use WOW64 context
    WOW64_CONTEXT ctx = {0};
    ctx.ContextFlags = WOW64_CONTEXT_CONTROL;
    if (!Wow64GetThreadContext(hThread, &ctx))
        return FALSE;
    ctx.EFlags |= 0x100;
    Wow64SetThreadContext(hThread, &ctx);
#else
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL;
    if (!GetThreadContext(hThread, &ctx))
        return FALSE;
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

#ifdef _M_X64
    // 64-bit PVDasm debugging 32-bit process: use WOW64 context
    WOW64_CONTEXT ctx = {0};
    ctx.ContextFlags = WOW64_CONTEXT_FULL | WOW64_CONTEXT_DEBUG_REGISTERS;
    if (!Wow64GetThreadContext(hThread, &ctx))
        return FALSE;

    g_DbgRegisters.Eax = ctx.Eax;
    g_DbgRegisters.Ebx = ctx.Ebx;
    g_DbgRegisters.Ecx = ctx.Ecx;
    g_DbgRegisters.Edx = ctx.Edx;
    g_DbgRegisters.Esi = ctx.Esi;
    g_DbgRegisters.Edi = ctx.Edi;
    g_DbgRegisters.Ebp = ctx.Ebp;
    g_DbgRegisters.Esp = ctx.Esp;
    g_DbgRegisters.Eip = ctx.Eip;
    g_DbgRegisters.EFlags = ctx.EFlags;
    g_DbgRegisters.SegCs = (WORD)ctx.SegCs;
    g_DbgRegisters.SegDs = (WORD)ctx.SegDs;
    g_DbgRegisters.SegEs = (WORD)ctx.SegEs;
    g_DbgRegisters.SegFs = (WORD)ctx.SegFs;
    g_DbgRegisters.SegGs = (WORD)ctx.SegGs;
    g_DbgRegisters.SegSs = (WORD)ctx.SegSs;
    g_DbgRegisters.Dr0 = ctx.Dr0;
    g_DbgRegisters.Dr1 = ctx.Dr1;
    g_DbgRegisters.Dr2 = ctx.Dr2;
    g_DbgRegisters.Dr3 = ctx.Dr3;
    g_DbgRegisters.Dr6 = ctx.Dr6;
    g_DbgRegisters.Dr7 = ctx.Dr7;
#else
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS;
    if (!GetThreadContext(hThread, &ctx))
        return FALSE;

    g_DbgRegisters.Eax = ctx.Eax;
    g_DbgRegisters.Ebx = ctx.Ebx;
    g_DbgRegisters.Ecx = ctx.Ecx;
    g_DbgRegisters.Edx = ctx.Edx;
    g_DbgRegisters.Esi = ctx.Esi;
    g_DbgRegisters.Edi = ctx.Edi;
    g_DbgRegisters.Ebp = ctx.Ebp;
    g_DbgRegisters.Esp = ctx.Esp;
    g_DbgRegisters.Eip = ctx.Eip;
    g_DbgRegisters.EFlags = ctx.EFlags;
    g_DbgRegisters.SegCs = (WORD)ctx.SegCs;
    g_DbgRegisters.SegDs = (WORD)ctx.SegDs;
    g_DbgRegisters.SegEs = (WORD)ctx.SegEs;
    g_DbgRegisters.SegFs = (WORD)ctx.SegFs;
    g_DbgRegisters.SegGs = (WORD)ctx.SegGs;
    g_DbgRegisters.SegSs = (WORD)ctx.SegSs;
    g_DbgRegisters.Dr0 = ctx.Dr0;
    g_DbgRegisters.Dr1 = ctx.Dr1;
    g_DbgRegisters.Dr2 = ctx.Dr2;
    g_DbgRegisters.Dr3 = ctx.Dr3;
    g_DbgRegisters.Dr6 = ctx.Dr6;
    g_DbgRegisters.Dr7 = ctx.Dr7;
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

#ifdef _M_X64
    WOW64_CONTEXT ctx = {0};
    ctx.ContextFlags = WOW64_CONTEXT_FULL;
    if (!Wow64GetThreadContext(hThread, &ctx))
        return FALSE;

    ctx.Eax = g_DbgRegisters.Eax;
    ctx.Ebx = g_DbgRegisters.Ebx;
    ctx.Ecx = g_DbgRegisters.Ecx;
    ctx.Edx = g_DbgRegisters.Edx;
    ctx.Esi = g_DbgRegisters.Esi;
    ctx.Edi = g_DbgRegisters.Edi;
    ctx.Ebp = g_DbgRegisters.Ebp;
    ctx.Esp = g_DbgRegisters.Esp;
    ctx.Eip = g_DbgRegisters.Eip;
    ctx.EFlags = g_DbgRegisters.EFlags;
    ctx.SegCs = g_DbgRegisters.SegCs;
    ctx.SegDs = g_DbgRegisters.SegDs;
    ctx.SegEs = g_DbgRegisters.SegEs;
    ctx.SegFs = g_DbgRegisters.SegFs;
    ctx.SegGs = g_DbgRegisters.SegGs;
    ctx.SegSs = g_DbgRegisters.SegSs;

    return Wow64SetThreadContext(hThread, &ctx);
#else
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(hThread, &ctx))
        return FALSE;

    ctx.Eax = g_DbgRegisters.Eax;
    ctx.Ebx = g_DbgRegisters.Ebx;
    ctx.Ecx = g_DbgRegisters.Ecx;
    ctx.Edx = g_DbgRegisters.Edx;
    ctx.Esi = g_DbgRegisters.Esi;
    ctx.Edi = g_DbgRegisters.Edi;
    ctx.Ebp = g_DbgRegisters.Ebp;
    ctx.Esp = g_DbgRegisters.Esp;
    ctx.Eip = g_DbgRegisters.Eip;
    ctx.EFlags = g_DbgRegisters.EFlags;
    ctx.SegCs = g_DbgRegisters.SegCs;
    ctx.SegDs = g_DbgRegisters.SegDs;
    ctx.SegEs = g_DbgRegisters.SegEs;
    ctx.SegFs = g_DbgRegisters.SegFs;
    ctx.SegGs = g_DbgRegisters.SegGs;
    ctx.SegSs = g_DbgRegisters.SegSs;

    return SetThreadContext(hThread, &ctx);
#endif
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
        // Map runtime EIP back to static disassembly address (subtract ASLR delta)
        DWORD_PTR staticAddr = g_dwCurrentEIP - g_dwRebaseDelta;
        char addrStr[16];
        wsprintf(addrStr, "%08X", (DWORD)staticAddr);

        // Search through DisasmDataLines directly (safer than ListView search)
        for (DWORD_PTR i = 0; i < DisasmDataLines.size(); i++) {
            char* itemAddr = DisasmDataLines[i].GetAddress();
            if (itemAddr && lstrcmp(itemAddr, addrStr) == 0) {
                SelectItem(hListView, i);
                InvalidateRect(hListView, NULL, FALSE);
                return TRUE;
            }
        }
    }

    // Fallback: EIP is not in the existing disassembly (e.g. jumped to a DLL),
    // read live memory and decode from EIP
    const DWORD readSize = 4096;
    char* buffer = (char*)malloc(readSize);
    if (!buffer)
        return FALSE;

    SIZE_T bytesRead;
    if (!ReadProcessMemory(g_DbgProcess.hProcess, (LPVOID)g_dwCurrentEIP, buffer, readSize, &bytesRead)) {
        free(buffer);
        return FALSE;
    }

    DisasmDataLines.clear();

    DISASSEMBLY disasm;
    DWORD_PTR offset = 0;
    DWORD_PTR addr = g_dwCurrentEIP;
    DWORD lineCount = 0;
    const DWORD maxLines = 200;

    while (offset < bytesRead && lineCount < maxLines) {
        DWORD_PTR startOffset = offset;

        FlushDecoded(&disasm);
        disasm.Address = addr;
        Decode(&disasm, buffer, &offset);

        DWORD_PTR instrSize = disasm.OpcodeSize + disasm.PrefixSize;
        if (instrSize == 0) instrSize = 1;

        offset++;

        CDisasmData data;

        char addrStr[16];
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

        DisasmDataLines.push_back(data);

        addr += instrSize;
        lineCount++;
    }

    free(buffer);

    SendMessage(hListView, LVM_SETITEMCOUNT, DisasmDataLines.size(), 0);
    SelectItem(hListView, 0);
    InvalidateRect(hListView, NULL, FALSE);

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

BOOL CALLBACK DbgAttachDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HWND hProcessList = NULL;

    switch (Message) {
    case WM_INITDIALOG: {
        hProcessList = GetDlgItem(hWnd, IDC_DBG_PROCESS_LIST);
        if (!hProcessList)
            break;

        // Set up ListView columns
        SendMessage(hProcessList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
                    LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMN lvc;
        lvc.mask = LVCF_TEXT | LVCF_WIDTH;

        lvc.pszText = (LPSTR)"PID";
        lvc.cx = 60;
        ListView_InsertColumn(hProcessList, 0, &lvc);

        lvc.pszText = (LPSTR)"Process Name";
        lvc.cx = 200;
        ListView_InsertColumn(hProcessList, 1, &lvc);

        // Enumerate processes
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnap != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(pe32);
            int idx = 0;

            if (Process32First(hSnap, &pe32)) {
                do {
                    char pidStr[16];
                    wsprintf(pidStr, "%d", pe32.th32ProcessID);

                    LVITEM lvi;
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = idx;
                    lvi.iSubItem = 0;
                    lvi.pszText = pidStr;
                    ListView_InsertItem(hProcessList, &lvi);

                    ListView_SetItemText(hProcessList, idx, 1, pe32.szExeFile);
                    idx++;
                } while (Process32Next(hSnap, &pe32));
            }
            CloseHandle(hSnap);
        }

        // Apply dark mode
        if (g_DarkMode) {
            ListView_SetBkColor(hProcessList, g_DarkBkColor);
            ListView_SetTextBkColor(hProcessList, g_DarkBkColor);
            ListView_SetTextColor(hProcessList, g_DarkTextColor);
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
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
