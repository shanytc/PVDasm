VERSION 5.00
Begin VB.Form frmMenu 
   Caption         =   "Menu subclassing example"
   ClientHeight    =   3165
   ClientLeft      =   165
   ClientTop       =   480
   ClientWidth     =   4680
   Icon            =   "frmMenu.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3165
   ScaleWidth      =   4680
   StartUpPosition =   2  'CenterScreen
   Begin VB.Label lblSelItem 
      Height          =   615
      Left            =   120
      TabIndex        =   0
      Top             =   2280
      Width           =   3975
   End
   Begin VB.Menu mnu_top 
      Caption         =   "Top menu"
      Begin VB.Menu mnu_top_1 
         Caption         =   "Item 1"
      End
      Begin VB.Menu mnu_top_2 
         Caption         =   "Item 2"
      End
      Begin VB.Menu mnu_top_3 
         Caption         =   "Item 3"
      End
   End
   Begin VB.Menu mnu_top2 
      Caption         =   "Another top menu"
      Begin VB.Menu mnu_top2_item1 
         Caption         =   "Item 1"
      End
      Begin VB.Menu mnu_top2_submenu 
         Caption         =   "Submenu"
         Begin VB.Menu mnu_top2_sub_1 
            Caption         =   "Item 1"
         End
         Begin VB.Menu mnu_top2_sub_2 
            Caption         =   "Item 2"
         End
      End
      Begin VB.Menu mnu_top2_2 
         Caption         =   "Item 2"
      End
   End
End
Attribute VB_Name = "frmMenu"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Sub Form_Load()
HookWindow Me
End Sub

Private Sub Form_Unload(Cancel As Integer)
UnHookWindow
End Sub
