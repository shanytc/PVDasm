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


 File: LoadPic.cpp (main)
   This program was written by Shany Golan, Student at :
                    Ruppin, department of computer science and engineering University.
*/

#include <windows.h>
#include "olestd.h"
#include <olectl.h>
#include <comdef.h>
#include <stdio.h>

struct Pictures
{
  IPicture *Picture; // pointer to the picture
  long PictureWitdh; // picture witdh (in pixels)
  long PictureHeight; // picture height (in pixels)
  int PositionX; // the X coordinate of the picture on the Window
  int PositionY; // the Y coordinate of the picture on the Window
  HWND hWnd; // the handle of the window where the picture is printed on
  OLE_XSIZE_HIMETRIC cx; // Amount which will be used for copy horizontally in source picture
  OLE_YSIZE_HIMETRIC cy; // Amount which will be used for copy vertically in source picture
  struct Pictures *NextPicture; // the pointer to the next picture (if there is one)
  struct Pictures *PrevPicture; // the pointer to the previous picture (if there is one)
} *pFirstPicture = NULL;

int PictureCount = 0;

IPicture *LoadPicture(HWND hWnd, const unsigned char *data, size_t len,long *ret_w, long *ret_h, OLE_XSIZE_HIMETRIC *cx, OLE_YSIZE_HIMETRIC *cy)
{
  HDC dcPictureLoad;
  IPicture *pic = NULL;
  HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, len);
  LPVOID pvData = GlobalLock( hGlobal );
  memcpy(pvData,data,len);
  GlobalUnlock(hGlobal);
  LPSTREAM pStream = NULL;
  CreateStreamOnHGlobal( hGlobal, TRUE, &pStream );
  OleLoadPicture(pStream, 0, FALSE,IID_IPicture, (void **)&pic);
  pStream->Release();
  pic->get_Width(cx);
  pic->get_Height(cy);
  dcPictureLoad = GetDC(hWnd);
  *ret_w = MAP_LOGHIM_TO_PIX(*cx, GetDeviceCaps(dcPictureLoad, LOGPIXELSX));
  *ret_h = MAP_LOGHIM_TO_PIX(*cy, GetDeviceCaps(dcPictureLoad, LOGPIXELSX));
  ReleaseDC(hWnd,dcPictureLoad);
  return pic;
}

void RemovePictures(HWND hWnd)
{
  int iCounter;
  int LoopCount = PictureCount;
  struct Pictures *pPicture = pFirstPicture;
  for (iCounter=0; iCounter<LoopCount; iCounter++)
  {
    if (pPicture->hWnd==hWnd)
    {
      struct Pictures *pPictureTemp = pPicture->NextPicture;
      if (pPicture==pFirstPicture)
      {
        pFirstPicture=pPicture->NextPicture;
      } else {
        pPicture->PrevPicture->NextPicture=pPicture->NextPicture;
        if (pPicture->NextPicture != NULL)
        {
          pPicture->NextPicture->PrevPicture=pPicture->PrevPicture;
        }
      }
      IPicture *freepic = pPicture->Picture;
      freepic->Release();
      free(pPicture);
      PictureCount--;
      pPicture=pPictureTemp;  
    }
    else
    {
      pPicture=pPicture->NextPicture;
    }
  }
}

void RepaintPictures(HWND hWnd, HDC dcRepaintPictures)
{
  int iCounter;
  struct Pictures *pPicture = pFirstPicture;
  for (iCounter=0; iCounter<PictureCount; iCounter++)
  {
    if (pPicture->hWnd==hWnd)
    {
      RECT bounds;
      bounds.top = pPicture->PositionY;
      bounds.bottom = pPicture->PositionY + pPicture->PictureHeight;
      bounds.left = pPicture->PositionX;
      bounds.right = pPicture->PositionX + pPicture->PictureWitdh;
      pPicture->Picture->Render(dcRepaintPictures, bounds.left, bounds.bottom, bounds.right - bounds.left,
                                bounds.top - bounds.bottom, 0, 0, pPicture->cx, pPicture->cy, NULL);
    }
    pPicture=pPicture->NextPicture;
  }
}

void AddPicture(HWND hWnd,int ResourceHandle,int PositionX,int PositionY)
{
  HRSRC res = FindResource(GetModuleHandle(NULL),MAKEINTRESOURCE(ResourceHandle),"BINARY");
  if (res) 
  {
    struct Pictures *pPicture = (struct Pictures*) malloc (sizeof(struct Pictures));
    HGLOBAL mem = LoadResource(GetModuleHandle(NULL), res);
    void *data = LockResource(mem);
    size_t sz = SizeofResource(GetModuleHandle(NULL), res);
    pPicture->Picture = LoadPicture(hWnd, (u_char*)data, sz, &pPicture->PictureWitdh, 
	                                	&pPicture->PictureHeight, &pPicture->cx, &pPicture->cy);
    if (pPicture->Picture != NULL)
    {
      pPicture->PositionX=PositionX;
      pPicture->PositionY=PositionY;
      pPicture->hWnd=hWnd;
      pPicture->NextPicture=pFirstPicture;
      if (pFirstPicture != NULL)
      {
        pFirstPicture->PrevPicture=pPicture;
      }
      pFirstPicture=pPicture;
      PictureCount++;
      HDC dcWindowUpdate = GetDC(hWnd);
			RepaintPictures(hWnd,dcWindowUpdate);
			ReleaseDC(hWnd,dcWindowUpdate);
    }
  }
}
