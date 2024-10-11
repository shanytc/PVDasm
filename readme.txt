
          	        8888888b.                  888     888 d8b
     		        888   Y88b                 888     888 Y8P
    		        888    888                 888     888
   			888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888
  		        8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888
 	                888        888     888  888  Y88o88P   888 88888888 888  888  888
    		        888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P
    		        888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"
                   	===============================================[Shany Golan 2024]


                  	??????????????????????????????????????????????????????????????
                   	?                                                            ?
                   	?  ProView is : 32Bit Multi Disassembler & PE|PE+ Editor     ?
                   	?  Build Version: v1.7e                                      ?
                   	?  Copyrights by Shany Golan (R) 2003-2024                   ?
                   	?                                                            ?
                   	??????????????????????????????????????????????????????????????


                                               [General Overview]

PVDasm project is a fully written from scratch disassembler.
The disassembler engine has been written by me, and it is free for usage by anyone to use.
Proview (PVDasm) is my attempt to make my own disassembler and for CPU architecture knowledge.
PVDasm is written fully in C (IDE: VC++ 6,2005,2010), C++, and STL Templates.
Proview also includes a simplified versions of a PE/PE+ editor, process manager, Plugin SDK, 
MASM Source code generator (wizard), and many more features.
Thank you all for supporting the PVDasm.

                                                [History & Change Log]

-> 09.12.2022:
                * Fixed a nasty crash associated with subclassing
                  edit text inputs on new operating systems (Windows 10/11).
                * Fixed LoadPic library to compile standalone.
-> 01.04.2015:  * PVDasm is now open sourced!
-> 14.08.2011:
        * Fixed the ToolBar in PVDasm 64Bit ;)
		  The magic line for 64Bit compatibility: 
		  SendMessage(hWndTool, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
		* Task Manager is working on PVDasm 64Bit.
		  The root cause for task manager not to show is due to LoadLibraryA("VDMDBG.DLL");
		  this is a 16bit Virtual DOS Machine Debug Information .dll that could not be loaded
		  into a 64Bit process, even though it is located in Windows/SysWOW64/ it seems it is
		  compiled for 32Bit. Dependency Walker loaded it, but than reported a wrong CPU compile error.
		  To make TaskManager back again, I disabled LoadLibraryA("VDMDBG.DLL"), 
		  and let it load PSAPI.dll only.
		* The HexEditor .dll that PVDasm 64Bit loads is a 3rd party dll that belongs to RadASM project,
		  and to enable it for PVDasm 64Bit, KetilO need to recompile it for x64 :(.


-> 13.08.2011:	
        * Fixed the bug in the status bar, when clicking on a disassembled line 
		  did not showed the actual Code Address / Code Offset.
		* Fixed a nasty crash when PVDasm tried to resolve an APIs calls way outside
		  of the current disassebled section. (Bad Pointer, access violation)
		* Fixed a nasty crash when PVDasm tried to access a dword instead of
		  a word from a memory location, which caused a memory acceess violation crash.
		* Increased buffer size in XRef resolving, which caused a stack run time error.
		* Fixed PVDasm (64bit) Import table resolve!! PVDasm 64 crashed due to memory
		  access violation when tried accessing the import table using PIMAGE_THUNK_DATA64
		  instead of PIMAGE_THUNK_DATA32 for normal PE files.
		  this is done by adding #define PIMAGE_THUNK_DATA PIMAGE_THUNK_DATA32 for x64 build.
		* PE/PE+ Editor fixed in PVDasm 64Bit!!
		* Menu bar re-ordered.

		; Thanks to the bug fixes above, old 16bit binaries loads much safer now.
		; PVDasm is now fully 32/64 bit compatible!
		; PVDasm doesn't support Win7 process task manager (yet?), so it may crash,
		  when attempting to read a process.
		; PVDasm 64bit has an issue with Common Controls 6 manifest configuration,
		  enabling the manifest causes the virtual list view to die (notifications not receiving),
		  disabling manifest causes a defect in the tool bar control...
		  only sulution is to try and rewrite it.

-> 27.03.2011:	
        * Fixed a nasty bug in the PE Import scanner which caused PVDasm to crash.
		* Known Issues:
			1. in x64 PVDasm, the menu items are bad! I know.. I'm trying to figure it out,
			   It seems VisualStudio doesn't load the BMP list correctly.
			2. PVdasm may on several PCs not run, due to missing Distributibal x86/x64 vc++ packages,
			   In case it happens, please let me know!

-> 20.02.2010:
        * Fixed a buffer accees violation bug in the PE Imports scanner, thanks 'SecMAM' for the bug report.
		* Increased buffers' size for extra caution.
		* Changed MoveWindow() to SetWindowPos() for better compability and speed.
		* Changed SetWindowLong to SetWindowLongPtr (32Bit/64Bit Compability) - thanks 'jstorme'.

-> 28.03.2009:
        * Fixed a bug that caused PVDasm to crash when,
		  You disassemble the same file (After it was already disassembled).
		  Sometimes Re-Disassembly is needed (in case of defining Function entries / Data entries),
		  and the Database need to match the view.

-> 20.03.2009:
        * Fixed the currupted resources (I.e: Menu bar icons), for some reason,
		  last versions of VS (or my PC) currupted them.

-> 2008-2009:
        * Gui changes / addons / regrouped
		* 64Bit of PVDasm has been compiled and tested under WinXP/Win7 64bit,
		* No support for disassembling PE64 Image files, however PE+ Header (64Bit) is supported.

-> 08.11.2006:
        * Added support for loading NE Executable,
		  File Format, for now, PVDasm will let you,
		  load the file, but it will not disassemble,
		  or let you edit the NE Header.

-> 04.11.2006:
        * Added "ret" flag to the CodeFlow array,
		  (For those who uses plugin messages, it will
		  be filled in the DISASSEMBLY struct.)

-> 28.10.2006:
        * Updated the Plugin SDK to include new 3 Plugin Messaeges,
		  Which deals with Exports:
		  1. PI_GET_NUMBER_OF_EXPORTS
		  2. PI_GET_EXPORT_NAME
		  3. PI_GET_EXPORT_DASM_INDEX
		  Also updated the disassembly struct which was not updated,
		  and caused an overflow of the data.
		* Updated PVDasm to supprt the 3 new Plugin Messages.
		* Goto Address box will now include the image base (for you lazy guys :))

-> 19.09.2006:
        * PVScript Engine v1 Added to PVDasm,
		  the engine allows you to create custom commands
		  and execute them in PVDasm without the need for any
		  gui commands.
		  For basic help:
		  commands are seperated by ";", and executed by
		  pressing the Ctrl+Enter keys.
		  basic command names: close, exit, new, define
		  see pvdasm.hlp to learn about the PVScript commands.

-> 12.05.2006:	* GUI Changes.

-> 06.05.2006:	
        * PVdasm now supports editing a PE+ (64Bit),
		  executable files.
		* Fixed a bug in the masm wizard - code editor,
		  causing pvdasm to crash.

-> 04.03.2006:
        * Copy to clipboard/file functions has been,
		  recoded, and uses dynamic memory allocating.
		  this should bring faster copying to memory.
		  and eventually to the clipboard.

-> 21.08.2005:
        * Improved the mouse hovering bug,
		  over a jxx/call instructions.
		  It seems that when we horizontly scroll,
		  the headers shifts, and the code was unable,
		  to detect the position.
		  I improved the code, but yet after a few H-scrolls,
		  you will not be able to get a pop up window.
		  though there shouldn't be much of a need to
		  Horizontly scroll :)

-> 21.06.2005:	* PVDasm was unable to load Read-Only Files,
		  now, pvdasm checks for the read-only flag,
		  and if it found that the file is read-only,
		  it will set the file as normal (non read-only, archive).


-> 21.05.2005:	* Fixed a decode bug,
		  I missed counting/displaying 2 bytes,
		  When decoding instructions with 0x67 prefix.


-> 23.04.2005:	* ToolTip Helper window will now close when moving,
		  the mouse out of the window area.
		  Note: it closes better usually when pointing to the border,
		  of the window, since i am using the no active message.
		  sometimes it doesn't capture the in-activity when mouse is out,
		  of the window, so just move mouse slower near the border,
		  and the window will close.

-> 08.04.2005:	* Some bugs fixed in the function EntryPoint editor,
		  Which allowed address with addresses like '4010'
		  to be inserted/updated in a function.
		  PVDasm will now accept only Dword length strings, i.e: '00401000'


-> 24.03.2005:  * It seems some has Team has reported about a security buffer overflow,
		  and yes that is correct, the problem was at 2 places where i did not
		  used the MAX_PATH in 3 buffers, which caused pvdasm to crash.
		  this version should fix this problem.


-> 12.02.2005:	* Added File Map Export (File->Produce->Map File).
		* Updating a Function's Name, will now,
		  Be changed in PVDasm without ReDisassembly.
		* updated IDA2PV.idc.

-> 01.02.2005:
		* updated IDA2PV.idc file to export the function's Names.
		  With the proper functions names.
		* MAP Importing now supports reading the Function name from the .map file.


-> 29.01.2005:
		* Updated The MASM Wizard tool,
		  Lines which points to a string data,
		  Defined in the .DATA Section by PVDasm,
		  Will now be fixed from: e.g:
		  'PUSH 12345678'
		  TO:
		  'PUSH OFFSET <MY_STR_NAME>'
		* Added some Exception Handlers for various code places.
		* Added a missing code in the MASM Wizard.
		* Added Duplicate .DATA Lines Remover, created by the MASM Wizard.
		* Added more default Libs/Incs created by the MASM Wizard.
		* Fixed a bug when Wizard sees 0x0A, the output is a new line.
		* Fixed a bug when loading a Project and,
		  Saving the project back again.

-> 25.01.2005:
		* Added A fix for latest reported Buffer Overflow,
	 	  when reading the Imports.
		* Added more Plugin Messages (By Special Request)
		* Save/Load project now support function names.

-> 22.01.2005:	
		* Fixed more general Bugs
		* Functions can now be renamed.
		* Masm Wizard now supports Functions with names.
		  This means, the source create will now have:
		  Call "MyFunc" insted of Call XXXXXXXX.
		  Note: function will be shown only if a function,
                  has a name, else it will remain Call XXXXXXXX

-> 13.01.2005: 
		* Fixed Reported Disasm bugs by 'mcoder'
		  To the Disasm Engine Core.
		  Thanks for the bugs.
		* Missing not handled Opcodes,
		  in the Disasm engine core added.

-> 27.11.2004: 
	       * 2 major bugs has been discovered,
                 and i had to release this as a new version,
                 to keep confuse our of users.
               * Fixed a nasty error,
                 an invalid entrypoint pointer was defined,
                 and caused a nasty crash.
                 Note: it has nothing related with the 1.5e new code,
                 it's regarding the FirstPass anaylzer.
               * nasty freez has been fixed,
                 it has been told to me by various members [thanks elfz!],
                 it seems that when the mouse is leaving the disasm client,
                 window and using the drag bar for the listview,
                 pvdasm freezes and die.
                 this is because the tooltip code is based on the,
                 mouse_movie event, and leaving the client still,
                 trigger the event and the calculation just freezes PVDasm.
                 so i set a restrictive border between the bar and the mouse.
                 It should work fine now.

-> 23.11.2004:
               * Added 2 more Plugin Messages.

               * Fix List for User's comfrot (As reported by 'elfZ'):
                 1. "Decode new file dialog's tab order is quite messed up" - *Corrected*
                 2. "Ctrl-Space (references) opens a window,  
                     but I can't navigate to any of address using keyboard,
                     (dblclick is req-d). Also, the "esc" doesn't close xref window" - *Added*
                 3. "Ctrl-C as a shortcut to 'go to start of code' isn't really the best choice,
                     I'd expect it to be 'copy selected text to clipboard'. - *Corrected + More Fixes*
                 4. Tab Order fixes in all dialogs.
                 5. More ShurtCuts has been added to support the keyboard users.
                    Note: the 'Plugin' menu is dynamicly created by pvdasm, 
                    so it can't be associated with 'Alt+P' shurtcut.
                 6. Fixed some typos, added 'Add Comment' to menu and shortcut key.
                 7. Added a 'Show String' for instructions like: LEA ESI,DWORD PTR [00403012]
                    it will be shows as, e.g: LEA ESI,DWORD PTR [00403012] ASCIIZ "My String",0
               * ToolTip Helper: When Hovering over address of instructions like: (CALL XXXXXXXX/JXX XXXXXXXX),
                 a window will pop up and will show the 'target' jump code. (tooltip alike help).
                 Note: 'ESC' key to close the window.  
               * MAP Reader :
                 PVDasm can load a Specific MAP file generated by IDA from the script: "Map/pvmap.idc",
                 This will create a much more analyzed disassembly code that is correct and readable. 
                 Note: See tutorial Section on how to work with MAP files.

-> 26.08.2004:                 
                * Added 6 More Plugin Messages.
                * End of a function is now marked in the last
                  Instruction of the function boundry as a part of a remark.
                  Example: RET ; proc_XXXXXXXX endp
                * Loaded File can be released via Menu ('Close File')
                * Added some bug fixes (after file is closed).
                * Fixed decoding of the Opcode sequence: '8C3F', 
                  "Old Display: MOV WORD PTR DS:[EDI],+", // pointer passed the segment array onto the next one
                  "New Display: MOV WORD PTR DS:[EDI],SEG?",

-> 25.07.2004:  * Fixed a nasty 'out of range pointer',
                  exception in the latest built (1.5b) on big files, 
                  regarding the visual xreferences.
                  in the latest build the disasm will be currupted 
                  when loading big files (>300k+-) and trying,
                  to access the data which is invalid.
                  visual xrefferences will now be added,
                  after the disasm process, so while disasm will,
                  be visible, xref will be added afterward.
		  there will be times where there wont be any xref
                  due to an internal exception caught by pvdasm,
                  mostly accur on big file with bad disassembly and
                  mis-aligned code.
                  note: the addresses references to the address will not
                  be shown since 'Ctrl+Space' will display them all,
                  or via the tool-bar button.
                * Disassembly Bug fixed, an register was added where it shouldn't.
                  Corrected "[ESP+ESP+0Xh]" -> "[ESP+0Xh]",
                  Thanks to 'CoDe_InSide' for the bug report.
		* Optimized code: MMX version of StringHex->Hex convertion algorithm.
                  Optimized code: MMX version of lstrlen(); function
                  note: if there are users with non MMX CPU enabled (support),
                  please tell me so i will release pvdasm,
                  version with a backward compability.
                * 2 Bug fixes in the MASM Wizard, when deleting,
	          an Data string member, the string pointer got mixed up.
                  it will now show the good pointers so after deleting,
                  everything will be shown properly.
                  and another bug when creating a new,
                  function entry point than the data pointer got lost,
                  for each new 'disassembly' listing the,
                  data pointer will be recalculated.
                * Export / Import Function Database added to the subroutine,
                  Maker dialog in the Masm Wizard.
                  this is because for any disassembly action,
                  the dataBase is deleted.
		  exporting the database could save allot of hand redefining.
                  Note: if dataBase exported and afterward,
                  a new function is defined within pvdasm, it does not exist,
                  in the exported database, there for either add the new addition,
                  to the dataBase or redefine all from beginning.
                  exported information is in this format:
		
		  [Export File Begin .xpr]
                  ; each seperated line must contain "\r\n"
                  number of functions  
                  function_1 proc...
                  function_1 proto...
                  function_1 endp
                  function_1 Start address
	          function_1 End address
                  function_1 index  ; this means the index in the comboBox (starting from 0)
                  [ EOF ]
 
                * Added a 'Auto Search' button to the MASM code builder,
                  if the selected fucntion is an entrypoint and the caller,
                  is a 'CALL' instruction, than the auto search will find the number,
                  of parameters sent to the function and will automatically insert the,
                  PROC / PROTO lines in the edit boxes.
                  Note: it does not 'create' the function, only defines it,
                  User will have to press the 'Create' button,
                  to enable it in the source output.
	        * Crash fixed on disassembly view save operation when,
                  User canceled the operation.

-> 02.07.2004:  * Masm Wizard tool Functionality Enhanced!!
                  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		  Here's what new:
                  1. It will now clean: LEAVE instructions in procs.
                  2. it will fix RET XXXX -> RET instructions in procs.
                  3. Jumps and Calls are now automaticly refers to their lables!
                     e.g: 
                          'jnz 12345678' -> 'jnz ref_12345678'
                          ..code here..
                          ref_12345678:
                          'call 87654321' -> 'call proc_87654321'

                     Note: Call to an api imports stays the same! i.e: 'Call MessageBoxA'

                  4. Procedure StackFrame clean-up! wizard will remove,
                     The preceding "PUSH EBP" / "MOV EBP,ESP" signatures from a proc.
                     e.g:
		         before: 
                                 proc_12345678 proc ...
                                 push ebp     ; setup,
                                 mov ebp,esp  ; procedure stack frame,
                                 add esp,xxh  ; for local variables.
                                 ; uneffected code like more 'push ebp' 
                                 ; or 'mov ebp,esp' repeated instructions.
				 ret			
                     
                         after:
                                 proc_12345678 proc ...
                                 add esp,xxh  ; leave room for local variables.
				 ret

		     This gives us a compilable source code (in most cases),
                     but not executable image! since we need to make hand modifications,
                     For stuff like 'global' vars..etc.

                  5.
                    removed LEAVE instruction and 'RET XXXX' changes to 'RET'.
                  
		* Added a preceding zero (0) for a decoded instruction,
                  e.g: 'add esp,FFFFFFFFh' -> 'add esp,0FFFFFFFFh'
                  note: i haven't covered all the possibilities, so it will take a while.

-> 01.07.2004:  * Added a small bug fix in the Disassembly engine.
                * Save/Load project supports 'Data In Code' & 'Functions EntryPoint' information,
                  next time loading a project it will automaticly relocate your analyzed work.
		* Bug Fixes in the Masm wizard.	
                * Conditional or Unconditional jumps as references are,
                  shown in a new column in the disassembly view.
                  		

-> 21.03.2004:  * Corrected an Disassembly bug (hopefully,
                  Won't damage other decodings), this is the change:
                  'PUSH DWORD PTR SS:[EBP+00]' -> 'PUSH DWORD PTR SS:[EBP+EAX+00]'
                  Thanks to _Seven_ for the bug report.
                * Added a MASM Source Code Wizard Creator,
                  Check pvdasm.hlp to see how to use it.

-> 14.02.2004:  * While DblClick on an CALL<API> Instruction,
                  PVDasm will jump to it's "JMP<API>" jump table.
                * Api Recognition (engine) Added.
                  PVDasm will attempt to add API Parameters,
                  In order to make disassembly more easy to read.
                  Recognition DataBase is saved in the \sig\msapi.sig file. 
                  The File originally collected by Pegasus - Thnx!,
                  I'v altered it abit so pvdasm can read some missing,
                  Functions that aren't in the original build.
                  It can be altered anytime for your own need anytime.
                  the file can be renamed to any name.
                  only 1 (main) sig file is supported at this moment.
                * Few add-ons here and there.
                * You can now transform/Define address range to Data/Function EntryPoint by
                  Selecting the addresses (in the disassembly view) and
                  press either: 
                  Ctrl+Insert: #Define Data blocks.
                  Alt+Insert: #Define a Function(s) EntryPoint(s).
                  or using the contex menu for both missions.
                * Data/Function EntryPoint Managers can be accesses using,
                  Contex Menu (right click).
 

-> 10.02.2004:  * Added a New Plugin Message for Developers.
                * Updated Help File.
                * Added an Option to Edit/Add/Remove Function's
                  Start/End Addresses Analyzed by the FirstPass.
                  This Gives Flexibility for the users to modify 
                  The Bad Analisis created by the Incomplete FirstPass.
 

-> 26.01.2004:  * Api Calls/Jmps , Jmps,Calls Highlighting Added,
                  Use the Appearance Dialog to select your colors.
                * Right Click On Disasm window pops up a Menu. (For faster access)
                * Copy to File/Clipboard added, found in the right click popup menu.
                * Some Visual Fixes here and there.
                * Added a new Plugin to the site (Chip8 Emulator)


-> 15.12.2003:  * 2 new SDK Messages has been added, see the pvdasm.hlp file
                  at the PluginSDK.zip to review them.
                * FAQ added.
                * Some Bug Fixes.

-> 13.12.2003:  * Simple! FirstPass analyzer has been added.
                  if program uses FirstPass, you will be able
                  to modify/add/delete Bad/Good data addresses found,
                  by the analyzer when perssing 'data segments' for the next disasm process. 
                  No Herueistics in the analyzer yet.

-> 12.12.2003:  * Chip8 CPU Added to the CPU Category.
                  You can disassemble with added/New CPUs only 
                  if they are not Win32 Images (MZ/PE).
                  if Image Loaded is Binary, Select The CPU and press on 'Set',
                  finally press ok.
                  No need to set CPU for loaded Win32 images, they are automaticly
                  slects x86 CPU.


-> 02.12.2003:  * CodePatcher bug currected, where instructions,
                  With Prefixes were missing the last bytes (prefix size was not added).	
                * Added ability to create a custom data sections (segments),
                  if you want to treat code as data, add rva start and rva end.
                  

-> 11.11.2003:  * Load/Save Project Options Added.
	          There are 7 files for each saved project.
                  (yeah i know kidna allot, but it's easier to handle rather than 1 bif file.)

                * Plugin SDK Added!:
                  you can code your own plugins for PVDasm now.
                  Put your plugin at the \Plugin\ dir and ur done.
                  note: plugin will be executed only if a file has loaded into Pvdasm,
                  (no disassemble process require to execute the plugin).

                  PVDasm for now ships with this dll(s) list:
                  -------------------------------------------
                  1. "Command Line Disassembler"
                     CLD.DLL - Plugin coded by me, the plugin gives you option to preview disassemble
                     Opcode Vector from a command line alike mini-tool.
                     Source for plugin included.


-> 01.11.2003:  * Debug Window is now Dock-able (Via ToolBar or Menu).
                * Searching Withing the Disassembled code is no availble (be sure to check 'Match Case').
                * XReferences is not supported, if an address is being Refferences from another,
                  ToolBar Control will be availble, or a message will be writetn in the DebugWindow.
                  Press Ctrl+Space, ToolBar Control  or double click the Address to view XReferences to selected line,
                  The Window of xreferences will be opened accouring to your Mouse Pointer Position!,
                  idea came from the intellicase window, I kinda like it ;).
                * HexEditor - AddIn Created for RadASM (By KetilO) Has Been 'Converted' By Me to,
                  be Used Inside VisualC++ (For VC Example Check: http://radasm.visualassembler.com/projects/CustDemo.zip).
                  If AddIn Dll is not found in the AddIns\ Directory, you will not be able to access it via Proview (Run-Time).
                * String Refferences & Import Refferences Dialogs has been Changed, now you can perform,
                  Better Search Within Them, and view 'more' Information rather than using a simple ListBox ;).
                * Disasm Bugs Fixes (Those who has been reported.) 
                * Added Few More Seh Frames to avoid Crashes.
                * CodePatcher - Added Inline code patcher with Assembly Preview (After Patch) in same window.
                  After Patch has been complete you can or not ReDisassemble your Project in order to see,
                  Changes, iv done it beccause i want to avoid MisData information when patching new bytes,
                  So better keep stuff linear insted of curved ;) (PV is pretty fast to do ReDisassemble anyway hehe).
                  Access it by Double Click on Opcodes Culumn, ToolBar or Menu.
                * Gui Fixes/Edits (also fixed the bug in the disassembly appearance for the background color)
                 

-> 12.10.2003:  * Branch Tracing/Back Tracing has been added Using <- or -> Arrow Keys, opr trace in by,
                  Double Click Jxx instruction in disassembly Widnow (Column).
                  You can trace jxx/calls and return from them the same Way u traced them (No Metter how deep u traced!).
                  Fast Tracing / Ret from tracing with left/right arrow keys, tool bar or from the menu.
                * Tool Bar Has been upgraded, more option added


-> 01.10.2003:  * 100% Disassembly Speed!, you will notice a *huge!*,
                  Speed difference from the last build.
                  I think PVDasm can compete with the big boys now ;) (disasm speed)
                  Although there might be still false disasm (still in testing mode)
                * Added Colors Schmes (softice/ida/ollydbg/w32asm/custom) to the disasm window.
                * Goto.. (address/entrypoint/code start) options added.
                * x8 Speed optimizations to the disasm engine core.
                * Known Bug: if file is over 5mb PV might be not responding for a while
                  Because PV uses memory to store the information insted of a temp File (e.g: w32dasm).
                  but it doesn't mean it doesn't work, after a while u will get result,
                  so if you have more than 256mb mem, u will be fine during big files :) .
                * WinXP Theme (Manifest) Added to PV'S Gui (when XP theme shell is being used only).
                * Added String References with search dialog.     
                * Added a custom dialog About =) just to play with skinnble dialogs.           
                  

-> 27.08.2003:  * Import resolving added.
                * Imports dialog with searching added.
                * PV Now Uses Virtual ListView to hold big amount of data.
                * PV Will Auto Allocate memory based on the disassembled code data.
                * Double Click a disassembled line will allow to Add/Edit comments.

-> 15.08.2003:  * Disasm Engine Complete!! (except bugs i will find later :) )
                  it isn't the fastest engine, but it suites me fine now (as a student :))
                * Here we go.. Full support for 0F Set.
                  Meaning: MMX / 3DNow! / SSE / SSE2 instructions + prefixes support
                * Bug Fixes in disasm engine.
                * Disasm from EP option added.

-> 10.08.2003:  * added Options to the disassembler menu 
                * progress bar /percent added
                * force disasm's bytes from ep is now user defined (0-50 bytes).
                  Note: Smaller number of byte can cause few instructions to be not well decoded.

-> 09.08.2003:  * added 1/4 support for 0F Instruction set (JXX & 1 byte opcodes set)
                * added another opcode support (forgot to add it).

-> 08.08.2003:  * Fixed some problems, add a forgotten opcode :D
                * Added option to force disassembly before EP (lenght not yet user defined)
                * Auto jump to EP, code start, address added.
                * added option to restart disassembly

-> 07.08.2003:  * Opcode 0x0F remains to complete the Disasm engine.
                  So you will get from time to time some gaps if your
                  Exe is using its Set of instructions.
                * Process Viewer/Dumper supports 9x/2k/XP  


-> 18.05.2003:  * Disasembler implemented.

-> 10.02.2003:  * Added Expotrs Viewer

-> 05.02.2003:  * Added Import viewer at the
                  Pe editor/Directory viewer

-> 03.02.2003:  * Added a pe rebuilder
		  fixed the alignment and headers size,
                  as well as the sections [vsize=rsize / vaddr=raddr]
                  i cannot say it will rebuild it successfuly,
                  notpad did worked though =)
                  and i am opened to suggestions.
		* Added a partial process dumper
                  you can choose how maby bytes to dump
                  and what address to start from.
		  notes: full/partial dump does not work under
                  win9x, due to: i dont have this OS installed :).


			??????????????????????????????????????????????????????????????????????
               	 	?                                                                    ?
			? PVDasm.exe (32Bit) MD5 hash    : f93424e75595740e47f795579a5ac41b  ?
			? PVDasm.exe (64Bit) MD5 hash    : 0b056281912e528c86b25dc61d550247  ?
                	?                                                                    ?
                	? If This is not the Hash you get, the exe has been altered!         ?
			?                                                                    ?
			??????????????????????????????????????????????????????????????????????
