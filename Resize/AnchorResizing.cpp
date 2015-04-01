/*
     8888888b.                  888     888 d8b
     888   Y88b                 888     888 Y8P
     888    888                 888     888
     888   d88P 888d888  .d88b. Y88b   d88P 888  .d88b.  888  888  888
     8888888P"  888P"   d88""88b Y88b d88P  888 d8P  Y8b 888  888  888
     888        888     888  888  Y88o88P   888 88888888 888  888  888
     888        888     Y88..88P   Y888P    888 Y8b.     Y88b 888 d88P
     888        888      "Y88P"     Y8P     888  "Y8888   "Y8888888P"


             PE Editor & Dissasembler & File Identifier
             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   
 Written by Shany Golan.
 In januar, 2003.
 I have investigated P.E. file format as thoroughly as possible,
 But I cannot claim that I am an expert yet, so some of its information  
 May give you wrong results.

 Language used: Visual C++ 6.0
 Date of creation: July 06, 2002

 Date of first release: unknown ??, 2003

 You can contact me: e-mail address: Shanytc@yahoo.com

 Copyright (C) 2011. By Shany Golan.

 Permission is granted to make and distribute verbatim copies of this
 Program provided the copyright notice and this permission notice are
 Preserved on all copies.


 File: AnchorResizing.cpp (main)
   This program was written by Shany Golan, Student at :
		Ruppin, department of computer science and engineering University.
*/

#include "AnchorResizing.h"

// This rect will be used for storing the last size of the main window 
RECT MainWindowRect;

// The struct we use to store the negative coordinates when a control disappears
typedef struct tagNegativeCoordinates {
	unsigned short x;
	unsigned short y;
} NegativeCoordinates;

// We need to calculate from the DWORD the negative coordinate values
NegativeCoordinates GetNegativeCoordinates(long dwData) 
{
	/*
		Function:
		---------
		"GetNegativeCoordinates"

	    Parameters:
		-----------
		1. long dwData - coordinate to navigate

		Return Values:
		--------------
		NegativeCoordinates struct filled with negative number
	*/

	NegativeCoordinates ncControl; // negative struct
	ncControl.x = (unsigned short)((dwData >> 4) >> 14);
	ncControl.y = (unsigned short)((dwData >> 4) & 0x00003FFF);

	// return the struct
	return ncControl;
}

// Here we will set the coordinate x
void SetNegativeCoordinateX(long *dwData, unsigned short x) 
{
	long dwTemp;

	dwTemp = 0;
	dwTemp = (long)x;
	
	// Clear the x coordinates in the DWORD value
	*dwData &= 0x0003FFFF;
	
	*dwData |= dwTemp << 18;
}

// Here we will set the coordinate y
void SetNegativeCoordinateY(long *dwData, unsigned short y) 
{
	long dwTemp;

	dwTemp = 0;
	dwTemp = (long)y;
	
	// Clear the y coordinates in the DWORD value
	*dwData &= 0xFFFC000F;
	*dwData |= dwTemp << 4;
}

// Find Window and get its information
BOOL CALLBACK EnumerateAndResizeControls(HWND hWndControl, LPARAM lParam) 
{ 
	RECT NewSizeRect;
	RECT ControlRect;
	POINT ptNewControlTopLeft;
	HWND hWndMainWindow = (HWND)lParam;
	LONG dwControlAnchors;
	int iNewControlHeight;
	int iNewControlWidth;

	// Retrieve the current anchor settings
	dwControlAnchors = GetWindowLongPtr(hWndControl,GWL_USERDATA);
	
	// If there are no anchor settings, just ignore this
	if (dwControlAnchors != NULL)
	{
		// Get the difference of the main window size (height, width or both)
		GetClientRect(hWndMainWindow,&NewSizeRect);
		int iHeightDifference = NewSizeRect.bottom - MainWindowRect.bottom;
		int iWidthDifference = NewSizeRect.right - MainWindowRect.right;

		// Now get the current width and height of the control
		GetWindowRect(hWndControl,&ControlRect);
		iNewControlHeight = ControlRect.bottom - ControlRect.top;
		iNewControlWidth = ControlRect.right - ControlRect.left;

		// We need to get the upperleft coordinate in coordinates on the main window
		ptNewControlTopLeft.x = ControlRect.left;
		ptNewControlTopLeft.y = ControlRect.top;
		ScreenToClient(hWndMainWindow,&ptNewControlTopLeft);

		// Check if the control should not be anchored to the left
		if ((dwControlAnchors & ANCHOR_NOT_LEFT) > 0)
		{
			// The topleft coordinate moves to the right
			ptNewControlTopLeft.x += iWidthDifference;

		// So we are anchored to the left, then it is possible we need to increase
		// the size of the control if we also are anchored to the right
		} else if ((dwControlAnchors & ANCHOR_RIGHT) > 0) {

			// Calculate the new width
			iNewControlWidth += iWidthDifference - GetNegativeCoordinates(dwControlAnchors).x;
			
			// If the control disappears we must save the negative coordinates
			if (iNewControlWidth<0)
			{
				// Register the new negative coordinates
				SetNegativeCoordinateX(&dwControlAnchors,(unsigned short)0-iNewControlWidth);
			} else {
				// We are visible again, so no negative coordinates are used
				SetNegativeCoordinateX(&dwControlAnchors,0);
			}
		}

		// Check if the control should not be anchored to the top
		if ((dwControlAnchors & ANCHOR_NOT_TOP) > 0)
		{
			// The topleft coordinate moves to the bottom
			ptNewControlTopLeft.y += iHeightDifference;

		// So we are anchored to the top, then it is possible we need to increase
		// the size of the control if we also are anchored to the bottom
		} else if ((dwControlAnchors & ANCHOR_BOTTOM) > 0) {

			// Calculate the new height
			iNewControlHeight += iHeightDifference - GetNegativeCoordinates(dwControlAnchors).y;

			// If the control disappears we must save the negative coordinates
			if (iNewControlHeight<0)
			{
				// Set the new negative coordinates
				SetNegativeCoordinateY(&dwControlAnchors,(unsigned short)0-iNewControlHeight);
			} else {
				// We are visible again, so no negative coordinates are used
				SetNegativeCoordinateY(&dwControlAnchors,0);
			}
		}

		// Save the new anchor settings (this is with negative coordinates)
		SetWindowLongPtr(hWndControl,GWL_USERDATA,dwControlAnchors);

		// And finally move the control to the new place, and increase in size if necessary
		MoveWindow(hWndControl,ptNewControlTopLeft.x,ptNewControlTopLeft.y,iNewControlWidth,iNewControlHeight,TRUE);
	}

	// We must call true to get the next control
	return TRUE;
}

void ResizeControls(HWND hWnd)
 {

	// Let us walk through all the controls on the window (enumeration)
	EnumChildWindows(hWnd,EnumerateAndResizeControls,(LPARAM)hWnd);

	// We should store the new size of the main window in the main window rect
	GetClientRect(hWnd,&MainWindowRect);

	// Don't ask me why, but this will fix some painting probs
	InvalidateRect(hWnd,&MainWindowRect,NULL);
}

void InitializeResizeControls(HWND hWnd) 
{
	// Store the main 
	GetClientRect(hWnd,&MainWindowRect);
}