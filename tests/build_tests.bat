@echo off
REM ============================================================
REM  PVDasm Test Binary Builder
REM  Compiles test .c files into 32-bit PE executables for
REM  stress-testing PVDasm's disassembly engine.
REM
REM  Usage: Just run build_tests.bat from any command prompt.
REM         The script auto-detects Visual Studio and sets up
REM         the x86 compiler environment.
REM ============================================================

echo ========================================
echo  PVDasm Test Binary Builder
echo ========================================
echo.

REM Check if cl.exe is already available (e.g. VS command prompt)
where cl >nul 2>&1
if not errorlevel 1 goto :have_cl

REM --- Auto-detect Visual Studio vcvarsall.bat ---
echo Searching for Visual Studio...
set "VCVARS="

REM Try vswhere (VS 2017+) first
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VCVARS=%%i\VC\Auxiliary\Build\vcvarsall.bat"
    )
)
if defined VCVARS if exist "%VCVARS%" goto :found_vcvars

REM Fallback: search common paths (newest first)
for %%Y in (2022 2019 2017) do (
    for %%E in (Professional Enterprise Community BuildTools) do (
        for %%P in ("%ProgramFiles%" "%ProgramFiles(x86)%") do (
            if exist "%%~P\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
                set "VCVARS=%%~P\Microsoft Visual Studio\%%Y\%%E\VC\Auxiliary\Build\vcvarsall.bat"
                goto :found_vcvars
            )
        )
    )
)

REM Try VS 2015 (v14)
if exist "%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" (
    set "VCVARS=%ProgramFiles(x86)%\Microsoft Visual Studio 14.0\VC\vcvarsall.bat"
    goto :found_vcvars
)

echo ERROR: Could not find Visual Studio with C++ tools.
echo        Install Visual Studio with "Desktop development with C++" workload.
exit /b 1

:found_vcvars
echo Found: %VCVARS%
echo Setting up x86 environment...
call "%VCVARS%" x86 >nul 2>&1
if errorlevel 1 (
    echo ERROR: vcvarsall.bat failed.
    exit /b 1
)

REM Verify cl.exe is now available
where cl >nul 2>&1
if errorlevel 1 (
    echo ERROR: cl.exe still not found after running vcvarsall.bat.
    exit /b 1
)

:have_cl
echo Using: & where cl
echo.

set CFLAGS=/nologo /W3 /Od /GS- /MD

echo --- Building x86.exe (basic 32-bit x86) ---
cl %CFLAGS% /Fe:x86.exe x86.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building x87_fpu.exe (x87 FPU) ---
cl %CFLAGS% /Fe:x87_fpu.exe x87_fpu.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building mmx.exe (MMX) ---
cl %CFLAGS% /Fe:mmx.exe mmx.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building sse.exe (SSE) ---
cl %CFLAGS% /arch:SSE /Fe:sse.exe sse.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building sse2.exe (SSE2) ---
cl %CFLAGS% /arch:SSE2 /Fe:sse2.exe sse2.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building sse3.exe (SSE3 + SSSE3) ---
cl %CFLAGS% /arch:SSE2 /Fe:sse3.exe sse3.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building sse4.exe (SSE4.1 + SSE4.2) ---
cl %CFLAGS% /arch:SSE2 /Fe:sse4.exe sse4.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building avx.exe (AVX) ---
cl %CFLAGS% /arch:AVX /Fe:avx.exe avx.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building avx2.exe (AVX2) ---
cl %CFLAGS% /arch:AVX2 /Fe:avx2.exe avx2.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building avx512.exe (AVX-512F) ---
cl %CFLAGS% /arch:AVX512 /Fe:avx512.exe avx512.c /link /SUBSYSTEM:CONSOLE
echo.

echo --- Building chip8_gen.exe and generating chip8.bin ---
cl %CFLAGS% /Fe:chip8_gen.exe chip8_gen.c /link /SUBSYSTEM:CONSOLE
if exist chip8_gen.exe (
    chip8_gen.exe
    echo    chip8.bin generated.
)
echo.

echo --- Building vb_pcode_gen.exe and generating vb_pcode.exe ---
cl %CFLAGS% /Fe:vb_pcode_gen.exe vb_pcode_gen.c /link /SUBSYSTEM:CONSOLE
if exist vb_pcode_gen.exe (
    vb_pcode_gen.exe
    echo    vb_pcode.exe generated.
)
echo.

REM --- Clean up build artifacts ---
del /q *.obj *.pdb *.ilk 2>nul

echo ========================================
echo  Build complete!
echo ========================================
echo.
echo  Test files produced:
echo    x86.exe        - Basic 32-bit x86 instructions
echo    x87_fpu.exe    - x87 FPU instructions
echo    mmx.exe        - MMX + 3DNow! instructions
echo    sse.exe         - SSE instructions
echo    sse2.exe       - SSE2 instructions
echo    sse3.exe       - SSE3 + SSSE3 instructions
echo    sse4.exe       - SSE4.1 + SSE4.2 instructions
echo    avx.exe        - AVX instructions (VEX 128/256-bit)
echo    avx2.exe       - AVX2 + FMA3 instructions
echo    avx512.exe     - AVX-512F instructions (EVEX, zmm, opmask)
echo    chip8.bin      - Chip8/SCHIP ROM (all opcodes)
echo    vb_pcode.exe   - VB6 P-Code PE (select VB P-Code CPU mode)
echo.
echo  Load each file into PVDasm and verify disassembly output.
echo ========================================
