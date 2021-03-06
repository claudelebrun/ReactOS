/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: winpos.c,v 1.116.4.1 2004/06/20 22:27:54 gvg Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  NtGdid
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define MINMAX_NOSWP  (0x00010000)

#define SWP_EX_NOCOPY 0x0001
#define SWP_EX_PAINTSELF 0x0002

#define  SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define  SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define  SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL
IntGetClientOrigin(HWND hWnd, LPPOINT Point)
{
  PWINDOW_OBJECT WindowObject;
  
  WindowObject = IntGetWindowObject((hWnd ? hWnd : IntGetDesktopWindow()));
  if (WindowObject == NULL)
    {
      Point->x = Point->y = 0;
      return FALSE;
    }
  Point->x = WindowObject->ClientRect.left;
  Point->y = WindowObject->ClientRect.top;
  
  IntReleaseWindowObject(WindowObject);
  return TRUE;
}

BOOL STDCALL
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point)
{
  BOOL Ret;
  POINT pt;
  NTSTATUS Status;
  
  if(!Point)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Ret = IntGetClientOrigin(hWnd, &pt);
  
  if(!Ret)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  
  Status = MmCopyToCaller(Point, &pt, sizeof(POINT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }
  
  return Ret;
}

/*******************************************************************
 *         WinPosActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
VOID FASTCALL
WinPosActivateOtherWindow(PWINDOW_OBJECT Window)
{
  PWINDOW_OBJECT Wnd, Old;
  int TryTopmost;
  
  if (!Window || IntIsDesktopWindow(Window))
  {
    IntSetFocusMessageQueue(NULL);
    return;
  }
  Wnd = Window;
  for(;;)
  {
    HWND *List, *phWnd;
    
    Old = Wnd;
    Wnd = IntGetParentObject(Wnd);
    if(Old != Window)
    {
      IntReleaseWindowObject(Old);
    }
    if(!Wnd)
    {
      IntSetFocusMessageQueue(NULL);
      return;
    }
    
    if((List = IntWinListChildren(Wnd)))
    {
      for(TryTopmost = 0; TryTopmost <= 1; TryTopmost++)
      {
        for(phWnd = List; *phWnd; phWnd++)
        {
          PWINDOW_OBJECT Child;
        
          if((*phWnd) == Window->Self)
          {
            continue;
          }
        
          if((Child = IntGetWindowObject(*phWnd)))
          {
            if(((! TryTopmost && (0 == (Child->ExStyle & WS_EX_TOPMOST)))
                || (TryTopmost && (0 != (Child->ExStyle & WS_EX_TOPMOST))))
               && IntSetForegroundWindow(Child))
            {
              ExFreePool(List);
              IntReleaseWindowObject(Wnd);
              IntReleaseWindowObject(Child);
              return;
            }
            IntReleaseWindowObject(Child);
          }
        }
      }
      ExFreePool(List);
    }
  }
  IntReleaseWindowObject(Wnd);
}

VOID STATIC FASTCALL
WinPosFindIconPos(PWINDOW_OBJECT Window, POINT *Pos)
{
  /* FIXME */
}

PINTERNALPOS FASTCALL
WinPosInitInternalPos(PWINDOW_OBJECT WindowObject, POINT *pt, PRECT RestoreRect)
{
  PWINDOW_OBJECT Parent;
  INT XInc, YInc;
  
  if (WindowObject->InternalPos == NULL)
    {
      RECT WorkArea;
      PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop; /* Or rather get it from the window? */
      
      Parent = IntGetParentObject(WindowObject);
      if(Parent)
      {
        if(IntIsDesktopWindow(Parent))
          IntGetDesktopWorkArea(Desktop, &WorkArea);
        else
          WorkArea = Parent->ClientRect;
        IntReleaseWindowObject(Parent);
      }
      else
        IntGetDesktopWorkArea(Desktop, &WorkArea);
      
      WindowObject->InternalPos = ExAllocatePoolWithTag(PagedPool, sizeof(INTERNALPOS), TAG_WININTLIST);
      if(!WindowObject->InternalPos)
      {
        DPRINT1("Failed to allocate INTERNALPOS structure for window 0x%x\n", WindowObject->Self);
        return NULL;
      }
      WindowObject->InternalPos->NormalRect = WindowObject->WindowRect;
      IntGetWindowBorderMeasures(WindowObject, &XInc, &YInc);
      WindowObject->InternalPos->MaxPos.x = WorkArea.left - XInc;
      WindowObject->InternalPos->MaxPos.y = WorkArea.top - YInc;
      WindowObject->InternalPos->IconPos.x = WorkArea.left;
      WindowObject->InternalPos->IconPos.y = WorkArea.bottom - NtUserGetSystemMetrics(SM_CYMINIMIZED);
    }
  if (WindowObject->Style & WS_MINIMIZE)
    {
      WindowObject->InternalPos->IconPos = *pt;
    }
  else if (WindowObject->Style & WS_MAXIMIZE)
    {
      WindowObject->InternalPos->MaxPos = *pt;
    }
  else if (RestoreRect != NULL)
    {
      WindowObject->InternalPos->NormalRect = *RestoreRect;
    }
  return(WindowObject->InternalPos);
}

UINT FASTCALL
WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos)
{
  POINT Size;
  PINTERNALPOS InternalPos;
  UINT SwpFlags = 0;

  Size.x = WindowObject->WindowRect.left;
  Size.y = WindowObject->WindowRect.top;
  InternalPos = WinPosInitInternalPos(WindowObject, &Size, 
				      &WindowObject->WindowRect); 

  if (InternalPos)
    {
      if (WindowObject->Style & WS_MINIMIZE)
	{
	  if (!IntSendMessage(WindowObject->Self, WM_QUERYOPEN, 0, 0))
	    {
	      return(SWP_NOSIZE | SWP_NOMOVE);
	    }
	  SwpFlags |= SWP_NOCOPYBITS;
	}
      switch (ShowFlag)
	{
	case SW_MINIMIZE:
	  {
	    if (WindowObject->Style & WS_MAXIMIZE)
	      {
		WindowObject->Flags |= WINDOWOBJECT_RESTOREMAX;
		WindowObject->Style &= ~WS_MAXIMIZE;
	      }
	    else
	      {
		WindowObject->Flags &= ~WINDOWOBJECT_RESTOREMAX;
	      }
	    IntRedrawWindow(WindowObject, NULL, 0, RDW_VALIDATE | RDW_NOERASE |
	       RDW_NOINTERNALPAINT);
	    WindowObject->Style |= WS_MINIMIZE;
	    WinPosFindIconPos(WindowObject, &InternalPos->IconPos);
	    NtGdiSetRect(NewPos, InternalPos->IconPos.x, InternalPos->IconPos.y,
			NtUserGetSystemMetrics(SM_CXMINIMIZED),
			NtUserGetSystemMetrics(SM_CYMINIMIZED));
	    SwpFlags |= SWP_NOCOPYBITS;
	    break;
	  }

	case SW_MAXIMIZE:
	  {
	    WinPosGetMinMaxInfo(WindowObject, &Size, &InternalPos->MaxPos, 
				NULL, NULL);
	    DPRINT("Maximize: %d,%d %dx%d\n",
	       InternalPos->MaxPos.x, InternalPos->MaxPos.y, Size.x, Size.y);
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WindowObject->Style &= ~WS_MINIMIZE;
	      }
	    WindowObject->Style |= WS_MAXIMIZE;
	    NtGdiSetRect(NewPos, InternalPos->MaxPos.x, InternalPos->MaxPos.y,
			Size.x, Size.y);
	    break;
	  }

	case SW_RESTORE:
	  {
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WindowObject->Style &= ~WS_MINIMIZE;
		if (WindowObject->Flags & WINDOWOBJECT_RESTOREMAX)
		  {
		    WinPosGetMinMaxInfo(WindowObject, &Size,
					&InternalPos->MaxPos, NULL, NULL);
		    WindowObject->Style |= WS_MAXIMIZE;
		    NtGdiSetRect(NewPos, InternalPos->MaxPos.x,
				InternalPos->MaxPos.y, Size.x, Size.y);
		    break;
		  }
		else
		  {
		    *NewPos = InternalPos->NormalRect;
		    NewPos->right -= NewPos->left;
		    NewPos->bottom -= NewPos->top;
		    break;
		  }
	      }
	    else
	      {
		if (!(WindowObject->Style & WS_MAXIMIZE))
		  {
		    return 0;
		  }
		WindowObject->Style &= ~WS_MAXIMIZE;
		*NewPos = InternalPos->NormalRect;
		NewPos->right -= NewPos->left;
		NewPos->bottom -= NewPos->top;
		break;
	      }
	  }
	}
    }
  else
    {
      SwpFlags |= SWP_NOSIZE | SWP_NOMOVE;
    }
  return(SwpFlags);
}

VOID FASTCALL
WinPosFillMinMaxInfoStruct(PWINDOW_OBJECT Window, MINMAXINFO *Info)
{
  INT XInc, YInc;
  RECT WorkArea;
  PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop; /* Or rather get it from the window? */
  
  IntGetDesktopWorkArea(Desktop, &WorkArea);
  
  /* Get default values. */
  Info->ptMaxSize.x = WorkArea.right - WorkArea.left;
  Info->ptMaxSize.y = WorkArea.bottom - WorkArea.top;
  Info->ptMinTrackSize.x = NtUserGetSystemMetrics(SM_CXMINTRACK);
  Info->ptMinTrackSize.y = NtUserGetSystemMetrics(SM_CYMINTRACK);
  Info->ptMaxTrackSize.x = Info->ptMaxSize.x;
  Info->ptMaxTrackSize.y = Info->ptMaxSize.y;

  IntGetWindowBorderMeasures(Window, &XInc, &YInc);
  Info->ptMaxSize.x += 2 * XInc;
  Info->ptMaxSize.y += 2 * YInc;

  if (Window->InternalPos != NULL)
    {
      Info->ptMaxPosition = Window->InternalPos->MaxPos;
    }
  else
    {
      Info->ptMaxPosition.x -= WorkArea.left + XInc;
      Info->ptMaxPosition.y -= WorkArea.top + YInc;
    }
}

UINT FASTCALL
WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack)
{
  MINMAXINFO MinMax;
  
  WinPosFillMinMaxInfoStruct(Window, &MinMax);
  
  IntSendMessage(Window->Self, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax);

  MinMax.ptMaxTrackSize.x = max(MinMax.ptMaxTrackSize.x,
				MinMax.ptMinTrackSize.x);
  MinMax.ptMaxTrackSize.y = max(MinMax.ptMaxTrackSize.y,
				MinMax.ptMinTrackSize.y);

  if (MaxSize) *MaxSize = MinMax.ptMaxSize;
  if (MaxPos) *MaxPos = MinMax.ptMaxPosition;
  if (MinTrack) *MinTrack = MinMax.ptMinTrackSize;
  if (MaxTrack) *MaxTrack = MinMax.ptMaxTrackSize;

  return 0; //FIXME: what does it return?
}

STATIC VOID FASTCALL
FixClientRect(PRECT ClientRect, PRECT WindowRect)
{
  if (ClientRect->left < WindowRect->left)
    {
      ClientRect->left = WindowRect->left;
    }
  else if (WindowRect->right < ClientRect->left)
    {
      ClientRect->left = WindowRect->right;
    }
  if (ClientRect->right < WindowRect->left)
    {
      ClientRect->right = WindowRect->left;
    }
  else if (WindowRect->right < ClientRect->right)
    {
      ClientRect->right = WindowRect->right;
    }
  if (ClientRect->top < WindowRect->top)
    {
      ClientRect->top = WindowRect->top;
    }
  else if (WindowRect->bottom < ClientRect->top)
    {
      ClientRect->top = WindowRect->bottom;
    }
  if (ClientRect->bottom < WindowRect->top)
    {
      ClientRect->bottom = WindowRect->top;
    }
  else if (WindowRect->bottom < ClientRect->bottom)
    {
      ClientRect->bottom = WindowRect->bottom;
    }
}

LONG STATIC FASTCALL
WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PWINDOWPOS WinPos,
		   RECT* WindowRect, RECT* ClientRect)
{
  PWINDOW_OBJECT Parent;
  UINT wvrFlags = 0;

  /* Send WM_NCCALCSIZE message to get new client area */
  if ((WinPos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE)
    {
      NCCALCSIZE_PARAMS params;
      WINDOWPOS winposCopy;

      params.rgrc[0] = *WindowRect;
      params.rgrc[1] = Window->WindowRect;
      params.rgrc[2] = Window->ClientRect;
      Parent = IntGetParentObject(Window);
      if (0 != (Window->Style & WS_CHILD) && Parent)
	{
	  NtGdiOffsetRect(&(params.rgrc[0]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	  NtGdiOffsetRect(&(params.rgrc[1]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	  NtGdiOffsetRect(&(params.rgrc[2]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	}
      params.lppos = &winposCopy;
      winposCopy = *WinPos;

      wvrFlags = IntSendMessage(Window->Self, WM_NCCALCSIZE, TRUE, (LPARAM) &params);

      /* If the application send back garbage, ignore it */
      if (params.rgrc[0].left <= params.rgrc[0].right &&
          params.rgrc[0].top <= params.rgrc[0].bottom)
	{
          *ClientRect = params.rgrc[0];
	  if ((Window->Style & WS_CHILD) && Parent)
	    {
	      NtGdiOffsetRect(ClientRect, Parent->ClientRect.left,
	                      Parent->ClientRect.top);
	    }
          FixClientRect(ClientRect, WindowRect);
	}

       /* FIXME: WVR_ALIGNxxx */

      if (ClientRect->left != Window->ClientRect.left ||
          ClientRect->top != Window->ClientRect.top)
	{
          WinPos->flags &= ~SWP_NOCLIENTMOVE;
	}

      if ((ClientRect->right - ClientRect->left !=
           Window->ClientRect.right - Window->ClientRect.left) ||
          (ClientRect->bottom - ClientRect->top !=
           Window->ClientRect.bottom - Window->ClientRect.top))
	{
          WinPos->flags &= ~SWP_NOCLIENTSIZE;
	}
	  if(Parent)
	    IntReleaseWindowObject(Parent);
    }
  else
    {
      if (! (WinPos->flags & SWP_NOMOVE)
          && (ClientRect->left != Window->ClientRect.left ||
              ClientRect->top != Window->ClientRect.top))
	{
          WinPos->flags &= ~SWP_NOCLIENTMOVE;
	}
    }

  return wvrFlags;
}

BOOL FASTCALL
WinPosDoWinPosChanging(PWINDOW_OBJECT WindowObject,
		       PWINDOWPOS WinPos,
		       PRECT WindowRect,
		       PRECT ClientRect)
{
  INT X, Y;

  if (!(WinPos->flags & SWP_NOSENDCHANGING))
    {
      IntSendMessage(WindowObject->Self, WM_WINDOWPOSCHANGING, 0, (LPARAM) WinPos);
    }
  
  *WindowRect = WindowObject->WindowRect;
  *ClientRect = WindowObject->ClientRect;

  if (!(WinPos->flags & SWP_NOSIZE))
    {
      WindowRect->right = WindowRect->left + WinPos->cx;
      WindowRect->bottom = WindowRect->top + WinPos->cy;
    }

  if (!(WinPos->flags & SWP_NOMOVE))
    {
      PWINDOW_OBJECT Parent;
      X = WinPos->x;
      Y = WinPos->y;
      Parent = IntGetParentObject(WindowObject);
      if ((0 != (WindowObject->Style & WS_CHILD)) && Parent)
	{
	  X += Parent->ClientRect.left;
	  Y += Parent->ClientRect.top;
	}
	  if(Parent)
	    IntReleaseWindowObject(Parent);
      WindowRect->left = X;
      WindowRect->top = Y;
      WindowRect->right += X - WindowObject->WindowRect.left;
      WindowRect->bottom += Y - WindowObject->WindowRect.top;
      NtGdiOffsetRect(ClientRect,
        X - WindowObject->WindowRect.left,
        Y - WindowObject->WindowRect.top);
    }

  WinPos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

  return TRUE;
}

/*
 * Fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 */
HWND FASTCALL
WinPosDoOwnedPopups(HWND hWnd, HWND hWndInsertAfter)
{
   HWND *List = NULL;
   HWND Owner = NtUserGetWindow(hWnd, GW_OWNER);
   LONG Style = NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE);
   PWINDOW_OBJECT DesktopWindow, ChildObject;
   int i;

   if ((Style & WS_POPUP) && Owner)
   {
      /* Make sure this popup stays above the owner */
      HWND hWndLocalPrev = HWND_TOPMOST;

      if (hWndInsertAfter != HWND_TOPMOST)
      {
         DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());
         List = IntWinListChildren(DesktopWindow);
         IntReleaseWindowObject(DesktopWindow);
         if (List != NULL)
         {
            for (i = 0; List[i]; i++)
            {
               if (List[i] == Owner) break;
               if (HWND_TOP == hWndInsertAfter)
               {
                 ChildObject = IntGetWindowObject(List[i]);
                 if (NULL != ChildObject)
                 {
                   if (0 == (ChildObject->ExStyle & WS_EX_TOPMOST))
                   {
                     IntReleaseWindowObject(ChildObject);
                     break;
                   }
                   IntReleaseWindowObject(ChildObject);
                 }
               }
               if (List[i] != hWnd) hWndLocalPrev = List[i];
               if (hWndLocalPrev == hWndInsertAfter) break;
            }
            hWndInsertAfter = hWndLocalPrev;
         }
      }
   }
   else if (Style & WS_CHILD)
   {
      return hWndInsertAfter;
   }

   if (!List)
   {
      DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);
      IntReleaseWindowObject(DesktopWindow);
   }
   if (List != NULL)
   {
      for (i = 0; List[i]; i++)
      {
         if (List[i] == hWnd)
            break;
         if ((NtUserGetWindowLong(List[i], GWL_STYLE, FALSE) & WS_POPUP) &&
             NtUserGetWindow(List[i], GW_OWNER) == hWnd)
         {
            WinPosSetWindowPos(List[i], hWndInsertAfter, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            hWndInsertAfter = List[i];
         }
      }
      ExFreePool(List);
   }

   return hWndInsertAfter;
}

/***********************************************************************
 *	     WinPosInternalMoveWindow
 *
 * Update WindowRect and ClientRect of Window and all of its children
 * We keep both WindowRect and ClientRect in screen coordinates internally
 */
VOID STATIC FASTCALL
WinPosInternalMoveWindow(PWINDOW_OBJECT Window, INT MoveX, INT MoveY)
{
   PWINDOW_OBJECT Child;

   Window->WindowRect.left += MoveX;
   Window->WindowRect.right += MoveX;
   Window->WindowRect.top += MoveY;
   Window->WindowRect.bottom += MoveY;

   Window->ClientRect.left += MoveX;
   Window->ClientRect.right += MoveX;
   Window->ClientRect.top += MoveY;
   Window->ClientRect.bottom += MoveY;
   
   IntLockRelatives(Window);
   for(Child = Window->FirstChild; Child; Child = Child->NextSibling)
   {
     WinPosInternalMoveWindow(Child, MoveX, MoveY);
   }
   IntUnLockRelatives(Window);
}

/*
 * WinPosFixupSWPFlags
 *
 * Fix redundant flags and values in the WINDOWPOS structure.
 */

BOOL FASTCALL
WinPosFixupFlags(WINDOWPOS *WinPos, PWINDOW_OBJECT Window)
{
   if (Window->Style & WS_VISIBLE)
   {
      WinPos->flags &= ~SWP_SHOWWINDOW;
   }
   else
   {
      WinPos->flags &= ~SWP_HIDEWINDOW;
      if (!(WinPos->flags & SWP_SHOWWINDOW))
         WinPos->flags |= SWP_NOREDRAW;
   }

   WinPos->cx = max(WinPos->cx, 0);
   WinPos->cy = max(WinPos->cy, 0);

   /* Check for right size */
   if (Window->WindowRect.right - Window->WindowRect.left == WinPos->cx &&
       Window->WindowRect.bottom - Window->WindowRect.top == WinPos->cy)
   {
      WinPos->flags |= SWP_NOSIZE;    
   }

   /* Check for right position */
   if (Window->WindowRect.left == WinPos->x &&
       Window->WindowRect.top == WinPos->y)
   {
      WinPos->flags |= SWP_NOMOVE;    
   }

   if (WinPos->hwnd == NtUserGetForegroundWindow())
   {
      WinPos->flags |= SWP_NOACTIVATE;   /* Already active */
   }
   else
   if ((Window->Style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
   {
      /* Bring to the top when activating */
      if (!(WinPos->flags & SWP_NOACTIVATE)) 
      {
         WinPos->flags &= ~SWP_NOZORDER;
         WinPos->hwndInsertAfter = (0 != (Window->ExStyle & WS_EX_TOPMOST) ?
                                    HWND_TOPMOST : HWND_TOP);
         return TRUE;
      }
   }

   /* Check hwndInsertAfter */
   if (!(WinPos->flags & SWP_NOZORDER))
   {
      /* Fix sign extension */
      if (WinPos->hwndInsertAfter == (HWND)0xffff)
      {
         WinPos->hwndInsertAfter = HWND_TOPMOST;
      }
      else if (WinPos->hwndInsertAfter == (HWND)0xfffe)
      {
         WinPos->hwndInsertAfter = HWND_NOTOPMOST;
      }

      if (WinPos->hwndInsertAfter == HWND_NOTOPMOST)
      {
         WinPos->hwndInsertAfter = HWND_TOP;
      }
      else if (HWND_TOP == WinPos->hwndInsertAfter
               && 0 != (Window->ExStyle & WS_EX_TOPMOST))
      {
         /* Keep it topmost when it's already topmost */
         WinPos->hwndInsertAfter = HWND_TOPMOST;
      }

      /* hwndInsertAfter must be a sibling of the window */
      if (HWND_TOPMOST != WinPos->hwndInsertAfter
          && HWND_TOP != WinPos->hwndInsertAfter
          && HWND_NOTOPMOST != WinPos->hwndInsertAfter
          && HWND_BOTTOM != WinPos->hwndInsertAfter)
      {
         PWINDOW_OBJECT Parent = IntGetParentObject(Window);
         if (NtUserGetAncestor(WinPos->hwndInsertAfter, GA_PARENT) !=
             (Parent ? Parent->Self : NULL))
         {
            if(Parent)
              IntReleaseWindowObject(Parent);
            return FALSE;
         }
         else
         {
            if(Parent)
              IntReleaseWindowObject(Parent);
            /*
             * We don't need to change the Z order of hwnd if it's already
             * inserted after hwndInsertAfter or when inserting hwnd after
             * itself.
             */
            if ((WinPos->hwnd == WinPos->hwndInsertAfter) ||
                (WinPos->hwnd == NtUserGetWindow(WinPos->hwndInsertAfter, GW_HWNDNEXT)))
            {
               WinPos->flags |= SWP_NOZORDER;
            }
         }
      }
   }

   return TRUE;
}

/* x and y are always screen relative */
BOOLEAN FASTCALL
WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags)
{
   PWINDOW_OBJECT Window;
   WINDOWPOS WinPos;
   RECT NewWindowRect;
   RECT NewClientRect;
   PROSRGNDATA VisRgn;
   HRGN VisBefore = NULL;
   HRGN VisAfter = NULL;
   HRGN DirtyRgn = NULL;
   HRGN ExposedRgn = NULL;
   HRGN CopyRgn = NULL;
   ULONG WvrFlags = 0;
   RECT OldWindowRect, OldClientRect;
   int RgnType;
   HDC Dc;
   RECT CopyRect;
   RECT TempRect;

   /* FIXME: Get current active window from active queue. */

   Window = IntGetWindowObject(Wnd);
   if (!Window)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   /*
    * Only allow CSRSS to mess with the desktop window
    */
   if (Wnd == IntGetDesktopWindow() &&
       Window->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
   {
      IntReleaseWindowObject(Window);
      return FALSE;
   }

   WinPos.hwnd = Wnd;
   WinPos.hwndInsertAfter = WndInsertAfter;
   WinPos.x = x;
   WinPos.y = y;
   WinPos.cx = cx;
   WinPos.cy = cy;
   WinPos.flags = flags;

   WinPosDoWinPosChanging(Window, &WinPos, &NewWindowRect, &NewClientRect);

   /* Fix up the flags. */
   if (!WinPosFixupFlags(&WinPos, Window))
   {
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   /* Does the window still exist? */
   if (!IntIsWindow(WinPos.hwnd))
   {
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if ((WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
       SWP_NOZORDER &&
       NtUserGetAncestor(WinPos.hwnd, GA_PARENT) == IntGetDesktopWindow())
   {
      WinPos.hwndInsertAfter = WinPosDoOwnedPopups(WinPos.hwnd, WinPos.hwndInsertAfter);
   }
  
   /* Compute the visible region before the window position is changed */
   if (!(WinPos.flags & (SWP_NOREDRAW | SWP_SHOWWINDOW)) &&
       (WinPos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | 
                        SWP_HIDEWINDOW | SWP_FRAMECHANGED)) != 
       (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER))
   {
      VisBefore = VIS_ComputeVisibleRegion(Window, FALSE, FALSE, TRUE);
      VisRgn = NULL;

      if (VisBefore != NULL && (VisRgn = (PROSRGNDATA)RGNDATA_LockRgn(VisBefore)) &&
          UnsafeIntGetRgnBox(VisRgn, &TempRect) == NULLREGION)
      {
         RGNDATA_UnlockRgn(VisBefore);
         NtGdiDeleteObject(VisBefore);
         VisBefore = NULL;
      }
      else if(VisRgn)
      {
         RGNDATA_UnlockRgn(VisBefore);
      }
   }

   WvrFlags = WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect, &NewClientRect);

   /* Relink windows. (also take into account shell window in hwndShellWindow) */
   if (!(WinPos.flags & SWP_NOZORDER) && WinPos.hwnd != NtUserGetShellWindow())
   {
      PWINDOW_OBJECT ParentWindow;
      PWINDOW_OBJECT Sibling;
      PWINDOW_OBJECT InsertAfterWindow;

      if ((ParentWindow = IntGetParentObject(Window)))
      {
         if (HWND_TOPMOST == WinPos.hwndInsertAfter)
         {
            InsertAfterWindow = NULL;
         }
         else if (HWND_TOP == WinPos.hwndInsertAfter
                  || HWND_NOTOPMOST == WinPos.hwndInsertAfter)
         {
            InsertAfterWindow = NULL;
            IntLockRelatives(ParentWindow);
            Sibling = ParentWindow->FirstChild;
            while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
            {
               InsertAfterWindow = Sibling;
               Sibling = Sibling->NextSibling;
            }
            if (NULL != InsertAfterWindow)
            {
              IntReferenceWindowObject(InsertAfterWindow);
            }
            IntUnLockRelatives(ParentWindow);
         }
         else if (WinPos.hwndInsertAfter == HWND_BOTTOM)
         {
            IntLockRelatives(ParentWindow);
            if(ParentWindow->LastChild)
            {
               IntReferenceWindowObject(ParentWindow->LastChild);
               InsertAfterWindow = ParentWindow->LastChild;
            }
            else
              InsertAfterWindow = NULL;
            IntUnLockRelatives(ParentWindow);
         }
         else
            InsertAfterWindow = IntGetWindowObject(WinPos.hwndInsertAfter);
         /* Do nothing if hwndInsertAfter is HWND_BOTTOM and Window is already
            the last window */
         if (InsertAfterWindow != Window)
         {
             IntUnlinkWindow(Window);
             IntLinkWindow(Window, ParentWindow, InsertAfterWindow);
         }
         if (InsertAfterWindow != NULL)
            IntReleaseWindowObject(InsertAfterWindow);
         if ((HWND_TOPMOST == WinPos.hwndInsertAfter)
             || (0 != (Window->ExStyle & WS_EX_TOPMOST)
                 && NULL != Window->PrevSibling
                 && 0 != (Window->PrevSibling->ExStyle & WS_EX_TOPMOST))
             || (NULL != Window->NextSibling
                 && 0 != (Window->NextSibling->ExStyle & WS_EX_TOPMOST)))
         {
            Window->ExStyle |= WS_EX_TOPMOST;
         }
         else
         {
            Window->ExStyle &= ~ WS_EX_TOPMOST;
         }
         
         IntReleaseWindowObject(ParentWindow);
      }
   }

   OldWindowRect = Window->WindowRect;
   OldClientRect = Window->ClientRect;

   if (OldClientRect.bottom - OldClientRect.top ==
       NewClientRect.bottom - NewClientRect.top)
   {
      WvrFlags &= ~WVR_VREDRAW;
   }

   if (OldClientRect.right - OldClientRect.left ==
       NewClientRect.right - NewClientRect.left)
   {
      WvrFlags &= ~WVR_HREDRAW;
   }

   /* FIXME: Actually do something with WVR_VALIDRECTS */

   if (NewClientRect.left != OldClientRect.left ||
       NewClientRect.top != OldClientRect.top)
   {
      WinPosInternalMoveWindow(Window,
                               NewClientRect.left - OldClientRect.left,
                               NewClientRect.top - OldClientRect.top);
   }

   Window->WindowRect = NewWindowRect;
   Window->ClientRect = NewClientRect;

   if (!(WinPos.flags & SWP_SHOWWINDOW) && (WinPos.flags & SWP_HIDEWINDOW))
   {
      /* Clear the update region */
      IntRedrawWindow(Window, NULL, 0, RDW_VALIDATE | RDW_NOFRAME |
                      RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ALLCHILDREN);
      Window->Style &= ~WS_VISIBLE;
   }
   else if (WinPos.flags & SWP_SHOWWINDOW)
   {
      Window->Style |= WS_VISIBLE;
   }

   DceResetActiveDCEs(Window,
                      NewWindowRect.left - OldWindowRect.left,
                      NewWindowRect.top - OldWindowRect.top);

   /* Determine the new visible region */
   VisAfter = VIS_ComputeVisibleRegion(Window, FALSE, FALSE, TRUE);
   VisRgn = NULL;

   if (VisAfter != NULL && (VisRgn = (PROSRGNDATA)RGNDATA_LockRgn(VisAfter)) &&
       UnsafeIntGetRgnBox(VisRgn, &TempRect) == NULLREGION)
   {
      RGNDATA_UnlockRgn(VisAfter);
      NtGdiDeleteObject(VisAfter);
      VisAfter = NULL;
   }
   else if(VisRgn)
   {
      RGNDATA_UnlockRgn(VisAfter);
   }

   /*
    * Determine which pixels can be copied from the old window position
    * to the new. Those pixels must be visible in both the old and new
    * position. Also, check the class style to see if the windows of this
    * class need to be completely repainted on (horizontal/vertical) size
    * change.
    */
   if (VisBefore != NULL && VisAfter != NULL && !(WinPos.flags & SWP_NOCOPYBITS) &&
       ((WinPos.flags & SWP_NOSIZE) || !(WvrFlags & WVR_REDRAW)))
   {
      CopyRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      RgnType = NtGdiCombineRgn(CopyRgn, VisAfter, VisBefore, RGN_AND);

      /*
       * If this is (also) a window resize, the whole nonclient area
       * needs to be repainted. So we limit the copy to the client area,
       * 'cause there is no use in copying it (would possibly cause
       * "flashing" too). However, if the copy region is already empty,
       * we don't have to crop (can't take anything away from an empty
       * region...)
       */
      if (!(WinPos.flags & SWP_NOSIZE) && RgnType != ERROR &&
          RgnType != NULLREGION)
      {
         RECT ORect = OldClientRect;
         RECT NRect = NewClientRect;
         NtGdiOffsetRect(&ORect, - OldWindowRect.left, - OldWindowRect.top);
         NtGdiOffsetRect(&NRect, - NewWindowRect.left, - NewWindowRect.top);
         NtGdiIntersectRect(&CopyRect, &ORect, &NRect);
         REGION_CropRgn(CopyRgn, CopyRgn, &CopyRect, NULL);
      }

      /* No use in copying bits which are in the update region. */
      if (Window->UpdateRegion != NULL)
      {
         NtGdiCombineRgn(CopyRgn, CopyRgn, Window->UpdateRegion, RGN_DIFF);
      }
      if (Window->NCUpdateRegion != NULL)
      {
         NtGdiCombineRgn(CopyRgn, CopyRgn, Window->NCUpdateRegion, RGN_DIFF);
      }
		  
      /*
       * Now, get the bounding box of the copy region. If it's empty
       * there's nothing to copy. Also, it's no use copying bits onto
       * themselves.
       */
      VisRgn = NULL;
      if ((VisRgn = (PROSRGNDATA)RGNDATA_LockRgn(CopyRgn)) && 
          UnsafeIntGetRgnBox(VisRgn, &CopyRect) == NULLREGION)
      {
         /* Nothing to copy, clean up */
         RGNDATA_UnlockRgn(CopyRgn);
         NtGdiDeleteObject(CopyRgn);
         CopyRgn = NULL;
      }
      else if (OldWindowRect.left != NewWindowRect.left ||
               OldWindowRect.top != NewWindowRect.top)
      {
         if(VisRgn)
         {
            RGNDATA_UnlockRgn(CopyRgn);
         }
         /*
          * Small trick here: there is no function to bitblt a region. So
          * we set the region as the clipping region, take the bounding box
          * of the region and bitblt that. Since nothing outside the clipping
          * region is copied, this has the effect of bitblt'ing the region.
          *
          * Since NtUserGetDCEx takes ownership of the clip region, we need
          * to create a copy of CopyRgn and pass that. We need CopyRgn later 
          */
         HRGN ClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

         NtGdiCombineRgn(ClipRgn, CopyRgn, NULL, RGN_COPY);
         Dc = NtUserGetDCEx(Wnd, ClipRgn, DCX_WINDOW | DCX_CACHE |
            DCX_INTERSECTRGN | DCX_CLIPSIBLINGS);
         NtGdiBitBlt(Dc,
            CopyRect.left, CopyRect.top, CopyRect.right - CopyRect.left,
            CopyRect.bottom - CopyRect.top, Dc,
            CopyRect.left + (OldWindowRect.left - NewWindowRect.left),
            CopyRect.top + (OldWindowRect.top - NewWindowRect.top), SRCCOPY);
         NtUserReleaseDC(Wnd, Dc);
         IntValidateParent(Window, CopyRgn);
      }
      else if(VisRgn)
      {
         RGNDATA_UnlockRgn(CopyRgn);
      }
   }
   else
   {
      CopyRgn = NULL;
   }

   /* We need to redraw what wasn't visible before */
   if (VisAfter != NULL)
   {
      DirtyRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      if (CopyRgn != NULL)
      {
         RgnType = NtGdiCombineRgn(DirtyRgn, VisAfter, CopyRgn, RGN_DIFF);
      }
      else
      {
         RgnType = NtGdiCombineRgn(DirtyRgn, VisAfter, 0, RGN_COPY);
      }
      if (RgnType != ERROR && RgnType != NULLREGION)
      {
         NtGdiOffsetRgn(DirtyRgn,
            Window->WindowRect.left - Window->ClientRect.left,
            Window->WindowRect.top - Window->ClientRect.top);
         IntRedrawWindow(Window, NULL, DirtyRgn,
            RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
      }
      NtGdiDeleteObject(DirtyRgn);
   }

   if (CopyRgn != NULL)
   {
      NtGdiDeleteObject(CopyRgn);
   }

   /* Expose what was covered before but not covered anymore */
   if (VisBefore != NULL)
   {
      ExposedRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      NtGdiCombineRgn(ExposedRgn, VisBefore, NULL, RGN_COPY);
      NtGdiOffsetRgn(ExposedRgn, OldWindowRect.left - NewWindowRect.left,
                     OldWindowRect.top - NewWindowRect.top);
      if (VisAfter != NULL)
         RgnType = NtGdiCombineRgn(ExposedRgn, ExposedRgn, VisAfter, RGN_DIFF);
      else
         RgnType = SIMPLEREGION;

      if (RgnType != ERROR && RgnType != NULLREGION)
      {
         VIS_WindowLayoutChanged(Window, ExposedRgn);
      }
      NtGdiDeleteObject(ExposedRgn);
      NtGdiDeleteObject(VisBefore);
   }

   if (VisAfter != NULL)
   {
      NtGdiDeleteObject(VisAfter);
   }

   if (!(WinPos.flags & SWP_NOREDRAW))
   {
      IntRedrawWindow(Window, NULL, 0, RDW_ALLCHILDREN | RDW_ERASENOW);
   }

   if (!(WinPos.flags & SWP_NOACTIVATE))
   {
      if ((Window->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
      {
         IntSendMessage(WinPos.hwnd, WM_CHILDACTIVATE, 0, 0);
      }
      else
      {
         IntSetForegroundWindow(Window);
      }
   }

   if ((WinPos.flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE)
      IntSendMessage(WinPos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM) &WinPos);

   IntReleaseWindowObject(Window);

   return TRUE;
}

LRESULT FASTCALL
WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect)
{
  LRESULT Result;

  *ClientRect = *WindowRect;
  Result = IntSendMessage(Wnd, WM_NCCALCSIZE, FALSE, (LPARAM) ClientRect);

  FixClientRect(ClientRect, WindowRect);

  return Result;
}

BOOLEAN FASTCALL
WinPosShowWindow(HWND Wnd, INT Cmd)
{
  BOOLEAN WasVisible;
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  UINT Swp = 0;
  RECT NewPos;
  BOOLEAN ShowFlag;
//  HRGN VisibleRgn;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       Wnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }
  
  WasVisible = (Window->Style & WS_VISIBLE) != 0;

  switch (Cmd)
    {
    case SW_HIDE:
      {
	if (!WasVisible)
	  {
	    ObmDereferenceObject(Window);
	    return(FALSE);
	  }
	Swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE |
	  SWP_NOZORDER;
	break;
      }

    case SW_SHOWMINNOACTIVE:
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOWMINIMIZED:
      Swp |= SWP_SHOWWINDOW;
      /* Fall through. */
    case SW_MINIMIZE:
      {
	Swp |= SWP_FRAMECHANGED | SWP_NOACTIVATE;
	if (!(Window->Style & WS_MINIMIZE))
	  {
	    Swp |= WinPosMinMaximize(Window, SW_MINIMIZE, &NewPos);
	  }
	else
	  {
	    Swp |= SWP_NOSIZE | SWP_NOMOVE;
	  }
	break;
      }

    case SW_SHOWMAXIMIZED:
      {
	Swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
	if (!(Window->Style & WS_MAXIMIZE))
	  {
	    Swp |= WinPosMinMaximize(Window, SW_MAXIMIZE, &NewPos);
	  }
	else
	  {
	    Swp |= SWP_NOSIZE | SWP_NOMOVE;
	  }
	break;
      }

    case SW_SHOWNA:
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOW:
      Swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
      /* Don't activate the topmost window. */
      break;

    case SW_SHOWNOACTIVATE:
      Swp |= SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOWNORMAL:
    case SW_SHOWDEFAULT:
    case SW_RESTORE:
      Swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
      if (Window->Style & (WS_MINIMIZE | WS_MAXIMIZE))
	{
	  Swp |= WinPosMinMaximize(Window, SW_RESTORE, &NewPos);	 
	}
      else
	{
	  Swp |= SWP_NOSIZE | SWP_NOMOVE;
	}
      break;
    }

  ShowFlag = (Cmd != SW_HIDE);
  if (ShowFlag != WasVisible)
    {
      IntSendMessage(Wnd, WM_SHOWWINDOW, ShowFlag, 0);
      /* 
       * FIXME: Need to check the window wasn't destroyed during the 
       * window procedure. 
       */
    }

  /* We can't activate a child window */
  if ((Window->Style & WS_CHILD) &&
      !(Window->ExStyle & WS_EX_MDICHILD))
    {
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
    }

  WinPosSetWindowPos(Window->Self, 0 != (Window->ExStyle & WS_EX_TOPMOST)
                                   ? HWND_TOPMOST : HWND_TOP,
                     NewPos.left, NewPos.top, NewPos.right, NewPos.bottom, LOWORD(Swp));

  if (Cmd == SW_HIDE)
    {
      /* FIXME: This will cause the window to be activated irrespective
       * of whether it is owned by the same thread. Has to be done
       * asynchronously.
       */

      if (Window->Self == NtUserGetActiveWindow())
        {
          WinPosActivateOtherWindow(Window);
        }

      /* Revert focus to parent */
      if (Wnd == IntGetThreadFocusWindow() ||
          IntIsChildWindow(Wnd, IntGetThreadFocusWindow()))
        {
          NtUserSetFocus(Window->Parent);
        }
    }

  /* FIXME: Check for window destruction. */

  if (Window->Flags & WINDOWOBJECT_NEED_SIZE)
    {
      WPARAM wParam = SIZE_RESTORED;

      Window->Flags &= ~WINDOWOBJECT_NEED_SIZE;
      if (Window->Style & WS_MAXIMIZE)
	{
	  wParam = SIZE_MAXIMIZED;
	}
      else if (Window->Style & WS_MINIMIZE)
	{
	  wParam = SIZE_MINIMIZED;
	}

      IntSendMessage(Wnd, WM_SIZE, wParam,
                     MAKELONG(Window->ClientRect.right - 
                              Window->ClientRect.left,
                              Window->ClientRect.bottom -
                              Window->ClientRect.top));
      IntSendMessage(Wnd, WM_MOVE, 0,
                     MAKELONG(Window->ClientRect.left,
                              Window->ClientRect.top));
    }

  /* Activate the window if activation is not requested and the window is not minimized */
/*
  if (!(Swp & (SWP_NOACTIVATE | SWP_HIDEWINDOW)) && !(Window->Style & WS_MINIMIZE))
    {
      WinPosChangeActiveWindow(Wnd, FALSE);
    }
*/

  ObmDereferenceObject(Window);
  return(WasVisible);
}

STATIC VOID FASTCALL
WinPosSearchChildren(
   PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *Point,
   PWINDOW_OBJECT* Window, USHORT *HitTest)
{
   PWINDOW_OBJECT Current;
   HWND *List, *phWnd;

   if ((List = IntWinListChildren(ScopeWin)))
   {
      for (phWnd = List; *phWnd; ++phWnd)
      {
         if (!(Current = IntGetWindowObject(*phWnd)))
            continue;
      
         if (!(Current->Style & WS_VISIBLE))
         {
            IntReleaseWindowObject(Current);
	    continue;
         }

         if ((Current->Style & (WS_POPUP | WS_CHILD | WS_DISABLED)) ==
             (WS_CHILD | WS_DISABLED))
         {
            IntReleaseWindowObject(Current);
            continue;
         }

         if (!IntPtInWindow(Current, Point->x, Point->y))
         {
            IntReleaseWindowObject(Current);
            continue;
         }

         if (*Window)
	    IntReleaseWindowObject(*Window);
         *Window = Current;

         if (Current->Style & WS_MINIMIZE)
         {
            *HitTest = HTCAPTION;
            break;
         }

         if (Current->Style & WS_DISABLED)
         {
            *HitTest = HTERROR;
            break;
         }
        
         if (OnlyHitTests && (Current->MessageQueue == OnlyHitTests))
         {
            *HitTest = IntSendMessage(Current->Self, WM_NCHITTEST, 0,
                                      MAKELONG(Point->x, Point->y));
            if ((*HitTest) == (USHORT)HTTRANSPARENT)
               continue;
         }
         else
            *HitTest = HTCLIENT;
        
         if (Point->x >= Current->ClientRect.left &&
             Point->x < Current->ClientRect.right &&
             Point->y >= Current->ClientRect.top &&
             Point->y < Current->ClientRect.bottom)
         {
            WinPosSearchChildren(Current, OnlyHitTests, Point, Window, HitTest);
         }        

         break;
      }
      ExFreePool(List);
   }
}

USHORT FASTCALL
WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *WinPoint, 
		      PWINDOW_OBJECT* Window)
{
  HWND DesktopWindowHandle;
  PWINDOW_OBJECT DesktopWindow;
  POINT Point = *WinPoint;
  USHORT HitTest;

  *Window = NULL;
  
  if(!ScopeWin)
  {
    DPRINT1("WinPosWindowFromPoint(): ScopeWin == NULL!\n");
    return(HTERROR);
  }

  if (ScopeWin->Style & WS_DISABLED)
    {
      return(HTERROR);
    }

  /* Translate the point to the space of the scope window. */
  DesktopWindowHandle = IntGetDesktopWindow();
  if((DesktopWindowHandle != ScopeWin->Self) &&
     (DesktopWindow = IntGetWindowObject(DesktopWindowHandle)))
  {
    Point.x += ScopeWin->ClientRect.left - DesktopWindow->ClientRect.left;
    Point.y += ScopeWin->ClientRect.top - DesktopWindow->ClientRect.top;
    IntReleaseWindowObject(DesktopWindow);
  }
  
  HitTest = HTNOWHERE;
  
  WinPosSearchChildren(ScopeWin, OnlyHitTests, &Point, Window, &HitTest);

  return ((*Window) ? HitTest : HTNOWHERE);
}

BOOL
STDCALL
NtUserGetMinMaxInfo(
  HWND hwnd,
  MINMAXINFO *MinMaxInfo,
  BOOL SendMessage)
{
  POINT Size;
  PINTERNALPOS InternalPos;
  PWINDOW_OBJECT Window;
  MINMAXINFO SafeMinMax;
  NTSTATUS Status;
  
  Window = IntGetWindowObject(hwnd);
  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }

  Size.x = Window->WindowRect.left;
  Size.y = Window->WindowRect.top;
  InternalPos = WinPosInitInternalPos(Window, &Size, 
				      &Window->WindowRect); 
  if(InternalPos)
  {
    if(SendMessage)
    {
      WinPosGetMinMaxInfo(Window, &SafeMinMax.ptMaxSize, &SafeMinMax.ptMaxPosition, 
                          &SafeMinMax.ptMinTrackSize, &SafeMinMax.ptMaxTrackSize);
    }
    else
    {
      WinPosFillMinMaxInfoStruct(Window, &SafeMinMax);
    }
    Status = MmCopyToCaller(MinMaxInfo, &SafeMinMax, sizeof(MINMAXINFO));
    if(!NT_SUCCESS(Status))
    {
      IntReleaseWindowObject(Window);
      SetLastNtError(Status);
      return FALSE;
    }
    IntReleaseWindowObject(Window);
    return TRUE;
  }
  
  IntReleaseWindowObject(Window);
  return FALSE;
}

/* EOF */
