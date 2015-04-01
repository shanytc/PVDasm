# Microsoft Developer Studio Project File - Name="PVDasm" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=PVDasm - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "FileMap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "FileMap.mak" CFG="PVDasm - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "PVDasm - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "PVDasm - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "PVDasm - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Fp"Release/PVDasm.pch" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "NDEBUG"
# ADD RSC /l 0x40d /fo"Release/PVDasm.rsrc.res" /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Release/PVDasm.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib psapi.lib /nologo /subsystem:windows /machine:I386 /out:"Release/PVDasm.exe"
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "PVDasm - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /I "C:\Program Files\Microsoft Visual Studio\VC98\Include" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /Fp"Debug/PVDasm.pch" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x40d /d "_DEBUG"
# ADD RSC /l 0x40d /fo"Debug/PVDasm.rsrc.res" /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo /o"Debug/PVDasm.bsc"
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib comctl32.lib psapi.lib /nologo /subsystem:windows /debug /machine:I386 /out:"Debug/PVDasm.exe" /pdbtype:sept
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "PVDasm - Win32 Release"
# Name "PVDasm - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "Functions.c"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=.\Resize\AnchorResizing.cpp
# End Source File
# Begin Source File

SOURCE=.\Functions.cpp
# End Source File
# End Group
# Begin Group "Pe Editor.c"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=.\PE_Editor.cpp
# End Source File
# Begin Source File

SOURCE=.\Sections.cpp
# End Source File
# End Group
# Begin Group "Main.c"

# PROP Default_Filter "c"
# Begin Source File

SOURCE=.\FileMap.cpp
# End Source File
# End Group
# Begin Group "Processes.c"

# PROP Default_Filter "*.c"
# Begin Source File

SOURCE=.\Process.cpp
# End Source File
# End Group
# Begin Group "Disassembler.c"

# PROP Default_Filter ""
# Begin Group "Disasm_Data.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CDisasmColor.cpp
# End Source File
# Begin Source File

SOURCE=.\CDisasmData.cpp
# End Source File
# End Group
# Begin Group "chip8.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Chip8.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\Disasm.cpp
# End Source File
# Begin Source File

SOURCE=.\Disasm_Options.cpp
# End Source File
# Begin Source File

SOURCE=.\Dsasm_Functions.cpp
# End Source File
# End Group
# Begin Group "LoadPic.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\loadpic\loadpic.cpp
# End Source File
# End Group
# Begin Group "HexEditor.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\HexEditor.cpp
# End Source File
# End Group
# Begin Group "Patcher.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Patcher.cpp
# End Source File
# End Group
# Begin Group "Project.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Project.cpp
# End Source File
# Begin Source File

SOURCE=.\Wizard.cpp
# End Source File
# End Group
# Begin Group "SDK.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SDK\SDK.CPP
# End Source File
# End Group
# Begin Group "PVScript.c"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\PVScript\pvscript.cpp
# End Source File
# End Group
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "Functions.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Resize\AnchorResizing.h
# End Source File
# Begin Source File

SOURCE=.\Functions.h
# End Source File
# End Group
# Begin Group "Pe_Editor.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\PE_Editor.h
# End Source File
# Begin Source File

SOURCE=.\PE_Sections.h
# End Source File
# End Group
# Begin Group "Main.h"

# PROP Default_Filter "h"
# Begin Source File

SOURCE=.\MappedFile.h
# End Source File
# End Group
# Begin Group "Processes.h"

# PROP Default_Filter "*.h"
# Begin Source File

SOURCE=.\Process.h
# End Source File
# End Group
# Begin Group "Disassembler.h"

# PROP Default_Filter ""
# Begin Group "Disasm_Data.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\CDisasmData.h
# End Source File
# End Group
# Begin Group "Chip8.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Chip8.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\Disasm.h
# End Source File
# End Group
# Begin Group "LoadPic.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\loadpic\loadpic.h
# End Source File
# End Group
# Begin Group "HexEditor.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\HexEditor.h
# End Source File
# End Group
# Begin Group "Patcher.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Patcher.h
# End Source File
# End Group
# Begin Group "Project.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\Project.h
# End Source File
# Begin Source File

SOURCE=.\Wizard.h
# End Source File
# End Group
# Begin Group "SDK.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\SDK\SDK.H
# End Source File
# End Group
# Begin Group "PVScript.h"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\PVScript\pvscript.h
# End Source File
# End Group
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\resource\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00003.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00004.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00005.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00006.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00007.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\bmp00008.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\default1.bin
# End Source File
# Begin Source File

SOURCE=.\resource\exports.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\FileMap.rsrc.rc
# End Source File
# Begin Source File

SOURCE=.\resource\function.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\resource\Imports.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\LIST.BMP
# End Source File
# Begin Source File

SOURCE=.\resource\logo.gif
# End Source File
# Begin Source File

SOURCE=.\resource\pic.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\pic.jpg
# End Source File
# Begin Source File

SOURCE=.\resource\rc_addr.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_appear.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_code.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_copy1.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_data_manager.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_define_data.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_define_func.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_ep.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_ep1.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_find.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_func_manager.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_hex.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_imp.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_patch.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_ref.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_save.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_save1.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\rc_xref.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\ToolBar.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\ToolBarHot.bmp
# End Source File
# Begin Source File

SOURCE=.\resource\W95MBX03.ICO
# End Source File
# Begin Source File

SOURCE=.\resource\W95MBX04.ICO
# End Source File
# Begin Source File

SOURCE=.\resource\xref.jpg
# End Source File
# End Group
# Begin Source File

SOURCE=.\resource\pic.rgn
# End Source File
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# Begin Source File

SOURCE=.\resource\xref.rgn
# End Source File
# End Target
# End Project
