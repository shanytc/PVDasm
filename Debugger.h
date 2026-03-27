#ifndef __DEBUGGER_H__
#define __DEBUGGER_H__

#include <windows.h>
#include <vector>

// ================================================================
// =======================  ENUMERATIONS  =========================
// ================================================================

typedef enum DebuggerState {
    DBG_STATE_IDLE = 0,         // No process attached
    DBG_STATE_RUNNING,          // Process running, debug loop waiting
    DBG_STATE_PAUSED,           // Stopped at breakpoint/exception/step
    DBG_STATE_STEPPING_INTO,    // Single-stepping (TF set)
    DBG_STATE_STEPPING_OVER,    // Temp breakpoint placed after instruction
    DBG_STATE_STEPPING_OUT,     // Running to return address
    DBG_STATE_RUN_TO_CURSOR     // Running to specific address
} DEBUGGER_STATE;

// ================================================================
// ========================  STRUCTURES  ==========================
// ================================================================

typedef struct DebugProcessInfo {
    DWORD               dwProcessId;
    DWORD               dwMainThreadId;
    HANDLE              hProcess;
    HANDLE              hMainThread;
    LPVOID              lpBaseOfImage;
    DWORD_PTR           dwEntryPoint;
    char                szExePath[MAX_PATH];
    char                szCmdLine[MAX_PATH];
    char                szWorkDir[MAX_PATH];
    bool                bAttached;      // true = attached, false = created
} DEBUG_PROCESS_INFO;

typedef struct DebugThreadInfo {
    DWORD   dwThreadId;
    HANDLE  hThread;
    LPVOID  lpStartAddress;
    LPVOID  lpTlsBase;
} DEBUG_THREAD_INFO;

typedef struct DebugModuleInfo {
    LPVOID  lpBaseOfDll;
    DWORD   dwSizeOfImage;
    char    szModuleName[MAX_PATH];
} DEBUG_MODULE_INFO;

typedef struct DebugBreakpoint {
    DWORD_PTR   dwAddress;          // Virtual address of breakpoint
    BYTE        bOriginalByte;      // Saved original byte (replaced by 0xCC)
    bool        bEnabled;           // Is breakpoint active?
    bool        bOneShot;           // Temporary breakpoint (remove after hit)
    int         nHitCount;          // Number of times hit
} DEBUG_BREAKPOINT;

typedef struct DebugRegisters {
    // General purpose
    DWORD   Eax, Ebx, Ecx, Edx;
    DWORD   Esi, Edi, Ebp, Esp;
    DWORD   Eip;
    // Flags
    DWORD   EFlags;
    // Segments
    WORD    SegCs, SegDs, SegEs, SegFs, SegGs, SegSs;
    // Debug registers
    DWORD   Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;
} DEBUG_REGISTERS;

typedef struct DebugMemorySnapshot {
    DWORD_PTR   dwBaseAddress;
    DWORD       dwSize;
    BYTE*       pData;
    SYSTEMTIME  stTimestamp;
} DEBUG_MEMORY_SNAPSHOT;

// ================================================================
// ===================  CUSTOM WINDOW MESSAGES  ===================
// ================================================================

#define WM_DBG_EVENT_BASE       (WM_USER + 400)
#define WM_DBG_BREAKPOINT_HIT   (WM_DBG_EVENT_BASE + 0)  // wParam = threadId, lParam = address
#define WM_DBG_STEP_COMPLETE    (WM_DBG_EVENT_BASE + 1)  // wParam = threadId, lParam = address
#define WM_DBG_EXCEPTION        (WM_DBG_EVENT_BASE + 2)  // wParam = exceptionCode, lParam = address
#define WM_DBG_PROCESS_EXIT     (WM_DBG_EVENT_BASE + 3)  // wParam = exitCode
#define WM_DBG_DLL_LOAD         (WM_DBG_EVENT_BASE + 4)  // lParam = pointer to module name string
#define WM_DBG_DLL_UNLOAD       (WM_DBG_EVENT_BASE + 5)
#define WM_DBG_THREAD_CREATE    (WM_DBG_EVENT_BASE + 6)
#define WM_DBG_THREAD_EXIT      (WM_DBG_EVENT_BASE + 7)
#define WM_DBG_OUTPUT_STRING    (WM_DBG_EVENT_BASE + 8)  // lParam = pointer to string (UI frees)
#define WM_DBG_STATE_CHANGED    (WM_DBG_EVENT_BASE + 9)  // wParam = new state

// ================================================================
// ======================  GLOBAL EXTERNS  ========================
// ================================================================

extern DEBUGGER_STATE                       g_DbgState;
extern DEBUG_PROCESS_INFO                   g_DbgProcess;
extern std::vector<DEBUG_THREAD_INFO>       g_DbgThreads;
extern std::vector<DEBUG_MODULE_INFO>       g_DbgModules;
extern std::vector<DEBUG_BREAKPOINT>        g_DbgBreakpoints;
extern std::vector<DEBUG_MEMORY_SNAPSHOT>   g_DbgSnapshots;
extern DEBUG_REGISTERS                      g_DbgRegisters;
extern DEBUG_REGISTERS                      g_DbgPrevRegisters;
extern HANDLE                               g_hDebugThread;
extern DWORD                                g_dwDebugThreadId;
extern HWND                                 g_hRegisterDlg;
extern DWORD_PTR                            g_dwCurrentEIP;
extern bool                                 g_bDebuggerActive;
extern HANDLE                               g_hContinueEvent;
extern CRITICAL_SECTION                     g_csBreakpoints;
extern DWORD_PTR                            g_dwRebaseDelta;    // actualBase - preferredBase (for ASLR)

// ================================================================
// ==================  FUNCTION PROTOTYPES  =======================
// ================================================================

// Process Control
BOOL DbgStartProcess(HWND hMainWnd, const char* szExePath, const char* szCmdLine, const char* szWorkDir);
BOOL DbgAttachToProcess(HWND hMainWnd, DWORD dwProcessId);
BOOL DbgDetachFromProcess();
BOOL DbgTerminateProcess();
BOOL DbgPauseProcess();
BOOL DbgResumeProcess();
void DbgCleanup();

// Debug Event Loop (thread function)
DWORD WINAPI DebugEventLoop(LPVOID lpParam);

// Breakpoint Management
BOOL DbgSetBreakpoint(DWORD_PTR dwAddress, bool bOneShot);
BOOL DbgRemoveBreakpoint(DWORD_PTR dwAddress);
BOOL DbgToggleBreakpoint(DWORD_PTR dwAddress);
BOOL DbgEnableBreakpoint(DWORD_PTR dwAddress, bool bEnable);
DEBUG_BREAKPOINT* DbgFindBreakpoint(DWORD_PTR dwAddress);

// Stepping
BOOL DbgStepInto();
BOOL DbgStepOver();
BOOL DbgRunUntilReturn();
BOOL DbgRunToCursor(DWORD_PTR dwAddress);

// Register Operations
BOOL DbgReadRegisters();
BOOL DbgWriteRegisters();
void DbgFormatEFlags(DWORD eflags, char* buffer, size_t bufLen);
void DbgUpdateRegisterDialog();

// Memory Operations
BOOL DbgReadMemory(DWORD_PTR dwAddress, LPVOID lpBuffer, SIZE_T nSize, SIZE_T* pBytesRead);
BOOL DbgWriteMemory(DWORD_PTR dwAddress, LPCVOID lpBuffer, SIZE_T nSize);
BOOL DbgTakeMemorySnapshot(DWORD_PTR dwBaseAddress, DWORD dwSize);
void DbgRefreshMemory();

// Disassembly Sync
BOOL DbgDisassembleAtEIP();

// UI
void DbgUpdateMenuState(HWND hWnd);
void DbgInitMenuState(HWND hWnd);

// Dialog Procedures
BOOL CALLBACK DbgRegisterDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DbgAttachDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DbgOptionsDlgProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);

#endif // __DEBUGGER_H__
