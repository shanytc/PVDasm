Attribute VB_Name = "modMenu"
Option Explicit

Public Const MF_BYCOMMAND = &H0&
Public Const MF_BYPOSITION = &H400&
Public Const MF_POPUP = &H10&
Declare Function GetMenuString Lib "user32" Alias "GetMenuStringA" (ByVal hMenu As Long, ByVal wIDItem As Long, ByVal lpString As String, ByVal nMaxCount As Long, ByVal wFlag As Long) As Long

Private Declare Function SetWindowLong Lib "user32" Alias "SetWindowLongA" (ByVal hWnd As Long, ByVal nIndex As Long, ByVal dwNewLong As Long) As Long
Private Declare Function GetWindowLong Lib "user32" Alias "GetWindowLongA" (ByVal hWnd As Long, ByVal nIndex As Long) As Long
Private Declare Function CallWindowProc Lib "user32" Alias "CallWindowProcA" (ByVal lpPrevWndFunc As Long, ByVal hWnd As Long, ByVal Msg As Long, ByVal wParam As Long, ByVal lParam As Long) As Long
Declare Function GetSystemMenu Lib "user32" (ByVal hWnd As Long, ByVal bRevert As Long) As Long

Private oldwndproc As Long
Private subclassedhWnd As Long

Public Const WM_MENUSELECT = &H11F
Public Const WM_NCDESTROY = &H82
Public Const GWL_WNDPROC = (-4)

Public Sub HookWindow(SubClassForm As Form)

' if something is already subclassed, don't subclass anything else
If oldwndproc <> 0 Then Exit Sub

subclassedhWnd = SubClassForm.hWnd

'Get the handle for the old window procedure so it can be replaced and used later
oldwndproc = GetWindowLong(SubClassForm.hWnd, GWL_WNDPROC)

'Install custom window procedure for this window
SetWindowLong SubClassForm.hWnd, GWL_WNDPROC, AddressOf WndProc

End Sub

Private Function WndProc(ByVal hWnd As Long, ByVal Msg As Long, ByVal wParam As Long, ByVal lParam As Long) As Long
'Does control want this message?
If Msg = WM_MENUSELECT Then
   
  ' This occurs when the menu is being closed
  If lParam = 0 Then Exit Function
  
  Dim MenuItemStr As String * 128
  Dim MenuHandle As Integer
  
  ' Get the low word from wParam: this contains the command ID or position of the menu entry
  MenuHandle = GetLowWord(wParam)
  
  'If the highlighted menu is the top of a poup menu, pass menu item by position
  If (GetHighWord(wParam) And MF_POPUP) = MF_POPUP Then
    
    'Get the caption of the menu item
    If GetMenuString(lParam, MenuHandle, MenuItemStr, 127, MF_BYPOSITION) = 0 Then Exit Function
  
  Else    ' Otherwise pass it by command ID
    
    'Get the caption of the menu item
    If GetMenuString(lParam, MenuHandle, MenuItemStr, 127, MF_BYCOMMAND) = 0 Then Exit Function
  
  End If
  
  ' Add status bar message here!
  frmMenu.lblSelItem = Trim$(MenuItemStr)

Else
  
  'Otherwise, just call default window handler
  WndProc = CallWindowProc(oldwndproc, hWnd, Msg, wParam, lParam)

End If

'Unhook this window if it is being destroyed
If Msg = WM_NCDESTROY Then
  UnHookWindow
End If
End Function

Public Sub UnHookWindow()
' If there is nothing subclassed, there is nothing to unsubclass!
If oldwndproc = 0 Then Exit Sub

'Return to default window handler
SetWindowLong subclassedhWnd, GWL_WNDPROC, oldwndproc
oldwndproc = 0
End Sub

Public Function GetLowWord(Word As Long)
GetLowWord = CInt("&H" & Right$(Hex$(Word), 4))
End Function

Public Function GetHighWord(Word As Long)
GetHighWord = CInt("&H" & Left$(Hex$(Word), 4))
End Function

