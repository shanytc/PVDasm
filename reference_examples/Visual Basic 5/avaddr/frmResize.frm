VERSION 5.00
Begin VB.Form frmResize 
   Caption         =   "AddressOf - Limiting Resize"
   ClientHeight    =   1050
   ClientLeft      =   3390
   ClientTop       =   3885
   ClientWidth     =   3855
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   ScaleHeight     =   1050
   ScaleWidth      =   3855
   Begin VB.Label Label2 
      Alignment       =   2  'Center
      Caption         =   "An Example of Using VB5's 'AddressOf' Operater to Limit the Size of Resizing Permitted.  This Only works on VB5 or Better."
      BeginProperty Font 
         Name            =   "Arial"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   855
      Left            =   120
      TabIndex        =   1
      Top             =   120
      Width           =   3615
   End
   Begin VB.Label Label1 
      Alignment       =   2  'Center
      Caption         =   "http://www.members.tripod.com/~access_vb/"
      Height          =   495
      Left            =   0
      TabIndex        =   0
      Top             =   3840
      Width           =   3855
   End
End
Attribute VB_Name = "frmResize"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit
Private Const GWL_WNDPROC = (-4)
Private Sub Form_Load()
    
    ' First, we need to store the address of the existing Message Handler
    OldWindowProc = GetWindowLong(Me.hwnd, GWL_WNDPROC)
    
    ' Now we can tell windows to forward all messages to out own Message Handler
    Call SetWindowLong(Me.hwnd, GWL_WNDPROC, AddressOf SubClass1_WndMessage)
    
End Sub
Private Sub Form_Unload(Cancel As Integer)
    
    ' We must return control of the messages back to windows before the program exits
    Call SetWindowLong(Me.hwnd, GWL_WNDPROC, OldWindowProc)

End Sub
