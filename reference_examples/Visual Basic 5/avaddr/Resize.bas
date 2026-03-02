Attribute VB_Name = "Module1"





' * * * * * * * * * * Caution * * * * * * * * * * * * *
' Changes made to the functions contained herein can cause VB to crash!
' SAVE YOUR CHANGES BEFORE RUNNING THIS PROGRAM IN THE VB IDE!
' DO NOT ENTER BREAK MODE! DOING SO WILL CRASH VB!
' * * * * * * * * * * Caution * * * * * * * * * * * * *









Option Explicit
Public OldWindowProc As Long  ' Original window proc

' Function to retrieve the address of the current Message-Handling routine
Declare Function GetWindowLong Lib "user32" Alias "GetWindowLongA" (ByVal hwnd As Long, ByVal nIndex As Long) As Long
' Function to define the address of the Message-Handling routine
Declare Function SetWindowLong Lib "user32" Alias "SetWindowLongA" (ByVal hwnd As Long, ByVal nIndex As Long, ByVal dwNewLong As Long) As Long
' Function to copy an object/variable/structure passed by reference onto a variable of your own
Declare Sub CopyMemory Lib "kernel32" Alias "RtlMoveMemory" (pDest As Any, pSource As Any, ByVal ByteLen As Long)
' Function to execute a function residing at a specific memory address
Declare Function CallWindowProc Lib "user32" Alias "CallWindowProcA" (ByVal lpPrevWndFunc As Long, ByVal hwnd As Long, ByVal Msg As Long, ByVal wParam As Long, ByVal lParam As Long) As Long

' This is the message constant
Public Const WM_GETMINMAXINFO = &H24

' This is a structure referenced by the MINMAXINFO structure
Type POINTAPI
     x As Long
     y As Long
End Type

' This is the structure that is passed by reference (ie an address) to your message handler
' The key items in this structure are ptMinTrackSize and ptMaxTrackSize
Type MINMAXINFO
        ptReserved As POINTAPI
        ptMaxSize As POINTAPI
        ptMaxPosition As POINTAPI
        ptMinTrackSize As POINTAPI
        ptMaxTrackSize As POINTAPI
End Type
Public Function SubClass1_WndMessage(ByVal hwnd As Long, ByVal Msg As Long, ByVal wp As Long, ByVal lp As Long) As Long
    
    ' Watch for the pertinent message to come in
    If Msg = WM_GETMINMAXINFO Then

        Dim MinMax As MINMAXINFO
        
        ' This is necessary because the structure was passed by its address and there
        ' is currently no intrinsic way to use an address in Visual Basic
        CopyMemory MinMax, ByVal lp, Len(MinMax)
        
        ' This is where you set the values of the MinX,MinY,MaxX, and MaxY
        ' The values placed in the structure must be in pixels. The values
        ' normally used in Visual Basic are in twips. The conversion is as follows:
        ' pixels = twips\twipsperpixel
        MinMax.ptMinTrackSize.x = 3975 \ Screen.TwipsPerPixelX
        MinMax.ptMinTrackSize.y = 1740 \ Screen.TwipsPerPixelY
        MinMax.ptMaxTrackSize.x = Screen.Width \ Screen.TwipsPerPixelX \ 2
        MinMax.ptMaxTrackSize.y = 3480 \ Screen.TwipsPerPixelY
        
        ' Here we copy the datastructure back up to the address passed in the parameters
        ' because Windows will look there for the information.
        CopyMemory ByVal lp, MinMax, Len(MinMax)
        
        ' This message tells Windows that the message was handled successfully
        SubClass1_WndMessage = 1
        Exit Function
                
    End If
        
    ' Here, we forward all irrelevant messages on to the default message handler.
    SubClass1_WndMessage = CallWindowProc(OldWindowProc, hwnd, Msg, wp, lp)
        
End Function
