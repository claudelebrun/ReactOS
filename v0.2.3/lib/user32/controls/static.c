/* $Id: static.c,v 1.13 2004/05/16 19:31:06 navaraf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS User32
 * PURPOSE:          Static control
 * FILE:             lib/user32/controls/static.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 * REVISION HISTORY: 2003/05/28 GvG Created
 * NOTES:            Adapted from Wine
 */

#include "windows.h"
#include "user32/regcontrol.h"

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintBitmapfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, DWORD style );
static LRESULT CALLBACK StaticWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
static LRESULT CALLBACK StaticWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static COLORREF color_black, color_gray, color_white;

/* offsets for GetWindowLong for static private information */
#define HFONT_GWL_OFFSET    0
#define HICON_GWL_OFFSET    (sizeof(HFONT))
#define STATIC_EXTRA_BYTES  (HICON_GWL_OFFSET + sizeof(HICON))

typedef void (*pfPaint)( HWND hwnd, HDC hdc, DWORD style );

static pfPaint staticPaintFunc[SS_TYPEMASK+1] =
{
    STATIC_PaintTextfn,      /* SS_LEFT */
    STATIC_PaintTextfn,      /* SS_CENTER */
    STATIC_PaintTextfn,      /* SS_RIGHT */
    STATIC_PaintIconfn,      /* SS_ICON */
    STATIC_PaintRectfn,      /* SS_BLACKRECT */
    STATIC_PaintRectfn,      /* SS_GRAYRECT */
    STATIC_PaintRectfn,      /* SS_WHITERECT */
    STATIC_PaintRectfn,      /* SS_BLACKFRAME */
    STATIC_PaintRectfn,      /* SS_GRAYFRAME */
    STATIC_PaintRectfn,      /* SS_WHITEFRAME */
    NULL,                    /* SS_USERITEM */
    STATIC_PaintTextfn,      /* SS_SIMPLE */
    STATIC_PaintTextfn,      /* SS_LEFTNOWORDWRAP */
    STATIC_PaintOwnerDrawfn, /* SS_OWNERDRAW */
    STATIC_PaintBitmapfn,    /* SS_BITMAP */
    NULL,                    /* SS_ENHMETAFILE */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDHORIZ */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDVERT */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDFRAME */
};


/*********************************************************************
 * static class descriptor
 */
const struct builtin_class_descr STATIC_builtin_class =
{
    L"Static",            /* name */
    CS_DBLCLKS | CS_PARENTDC, /* style  */
    (WNDPROC) StaticWndProcW,                  /* procW */
    (WNDPROC) StaticWndProcA,                  /* procA */
    STATIC_EXTRA_BYTES,                        /* extra */
    (LPCWSTR) IDC_ARROW,                        /* cursor */ /* FIXME Wine uses IDC_ARROWA */
    0                                          /* brush */
};


/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
static HICON STATIC_SetIcon( HWND hwnd, HICON hicon, DWORD style )
{
#if 0 /* FIXME */
    HICON prevIcon;
    CURSORICONINFO *info = hicon?(CURSORICONINFO *) GlobalLock16(HICON_16(hicon)):NULL;

    if ((style & SS_TYPEMASK) != SS_ICON) return 0;
    if (hicon && !info) {
	ERR("huh? hicon!=0, but info=0???\n");
    	return 0;
    }
    prevIcon = (HICON)SetWindowLongA( hwnd, HICON_GWL_OFFSET, (LONG)hicon );
    if (hicon)
    {
        SetWindowPos( hwnd, 0, 0, 0, info->nWidth, info->nHeight,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        GlobalUnlock16(HICON_16(hicon));
    }
    return prevIcon;
#else
    HICON prevIcon;

    if ((style & SS_TYPEMASK) != SS_ICON) return 0;
    prevIcon = (HICON)SetWindowLongA( hwnd, HICON_GWL_OFFSET, (LONG)hicon );
    if (hicon)
    {
        ICONINFO info;
        BITMAP bm;

        if (!GetIconInfo(hicon, &info))
        {
            return 0;
        }
        if (!GetObjectW(info.hbmColor, sizeof(BITMAP), &bm))
        {
            return 0;
        }
        SetWindowPos( hwnd, 0, 0, 0, bm.bmWidth, bm.bmHeight,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }
    return prevIcon;
#endif
}

/***********************************************************************
 *           STATIC_SetBitmap
 *
 * Set the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_SetBitmap( HWND hwnd, HBITMAP hBitmap, DWORD style )
{
    HBITMAP hOldBitmap;
    LONG full_style = GetWindowLongA( hwnd, GWL_STYLE );

    if ((style & SS_TYPEMASK) != SS_BITMAP) return 0;
    if (hBitmap && GetObjectType(hBitmap) != OBJ_BITMAP) {
	OutputDebugStringA("huh? hBitmap!=0, but not bitmap\n");
    	return 0;
    }
    hOldBitmap = (HBITMAP)SetWindowLongA( hwnd, HICON_GWL_OFFSET, (LONG)hBitmap );
    if (hBitmap && !(full_style & SS_REALSIZEIMAGE))
    {
        BITMAP bm;
        GetObjectW(hBitmap, sizeof(bm), &bm);
        SetWindowPos( hwnd, 0, 0, 0, bm.bmWidth, bm.bmHeight,
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }
    return hOldBitmap;
}

/***********************************************************************
 *           STATIC_LoadIconW
 *
 * Load the icon for an SS_ICON control.
 */
static HICON STATIC_LoadIconW( HWND hwnd, LPCWSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HICON hicon = LoadIconW( hInstance, name );
    if (!hicon) hicon = LoadIconW( 0, name );
    return hicon;
}

/***********************************************************************
 *           STATIC_LoadIconA
 *
 * Load the icon for an SS_ICON control.
 */
static HICON STATIC_LoadIconA( HWND hwnd, LPCSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HICON hicon = LoadIconA( hInstance, name );
    if (!hicon) hicon = LoadIconA( 0, name );
    return hicon;
}

/***********************************************************************
 *           STATIC_LoadBitmapW
 *
 * Load the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_LoadBitmapW( HWND hwnd, LPCWSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HBITMAP hbitmap = LoadBitmapW( hInstance, name );
    if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
        hbitmap = LoadBitmapW( 0, name );
    return hbitmap;
}

/***********************************************************************
 *           STATIC_LoadBitmapA
 *
 * Load the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_LoadBitmapA( HWND hwnd, LPCSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HBITMAP hbitmap = LoadBitmapA( hInstance, name );
    if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
        hbitmap = LoadBitmapA( 0, name );
    return hbitmap;
}

/***********************************************************************
 *           STATIC_TryPaintFcn
 *
 * Try to immediately paint the control.
 */
static VOID STATIC_TryPaintFcn(HWND hwnd, LONG full_style)
{
    LONG style = full_style & SS_TYPEMASK;
    RECT rc;

    GetClientRect( hwnd, &rc );
    if (!IsRectEmpty(&rc) && IsWindowVisible(hwnd) && staticPaintFunc[style])
    {
	HDC hdc;
	hdc = GetDC( hwnd );
	(staticPaintFunc[style])( hwnd, hdc, full_style );
	ReleaseDC( hwnd, hdc );
    }
}

/***********************************************************************
 *           StaticWndProc_common
 */
static LRESULT CALLBACK StaticWndProc_common( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    LRESULT lResult = 0;
    LONG full_style = GetWindowLongA( hwnd, GWL_STYLE );
    LONG style = full_style & SS_TYPEMASK;

    if (!IsWindow( hwnd )) return 0;
    switch (uMsg)
    {
    case WM_CREATE:
        if (style < 0L || style > SS_TYPEMASK)
        {
            OutputDebugStringA("Unknown style\n");
            return -1;
        }
        /* initialise colours */
        color_black = GetSysColor(COLOR_3DDKSHADOW);
        color_gray = GetSysColor(COLOR_3DSHADOW);
        color_white  = GetSysColor(COLOR_3DHILIGHT);
        break;

    case WM_NCDESTROY:
        if (style == SS_ICON) {
/*
 * FIXME
 *           DestroyIcon32( STATIC_SetIcon( wndPtr, 0 ) );
 *
 * We don't want to do this yet because DestroyIcon32 is broken. If the icon
 * had already been loaded by the application the last thing we want to do is
 * GlobalFree16 the handle.
 */
            break;
        }
        else
	    return unicode ? DefWindowProcW(hwnd, uMsg, wParam, lParam) :
				 DefWindowProcA(hwnd, uMsg, wParam, lParam);

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            if (staticPaintFunc[style])
                (staticPaintFunc[style])( hwnd, ps.hdc, full_style );
            EndPaint(hwnd, &ps);
        }
        break;

    case WM_ENABLE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_SYSCOLORCHANGE:
        color_black = GetSysColor(COLOR_3DDKSHADOW);
        color_gray = GetSysColor(COLOR_3DSHADOW);
        color_white  = GetSysColor(COLOR_3DHILIGHT);
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_NCCREATE:
	if (full_style & SS_SUNKEN)
            SetWindowLongA( hwnd, GWL_EXSTYLE,
                            GetWindowLongA( hwnd, GWL_EXSTYLE ) | WS_EX_STATICEDGE );

	lParam = (LPARAM)(((LPCREATESTRUCTW)lParam)->lpszName);
	/* fall through */
    case WM_SETTEXT:
	switch (style) {
	case SS_ICON:
	{
	    HICON hIcon;
	    if (unicode)
		hIcon = STATIC_LoadIconW(hwnd, (LPCWSTR)lParam);
	    else
		hIcon = STATIC_LoadIconA(hwnd, (LPCSTR)lParam);
            /* FIXME : should we also return the previous hIcon here ??? */
            STATIC_SetIcon(hwnd, hIcon, style);
	    break;
	}
        case SS_BITMAP:
	{
	    HBITMAP hBitmap;
	    if (unicode)
		hBitmap = STATIC_LoadBitmapW(hwnd, (LPCWSTR)lParam);
	    else
		hBitmap = STATIC_LoadBitmapA(hwnd, (LPCSTR)lParam);
            STATIC_SetBitmap(hwnd, hBitmap, style);
	    break;
	}
	case SS_LEFT:
	case SS_CENTER:
	case SS_RIGHT:
	case SS_SIMPLE:
	case SS_LEFTNOWORDWRAP:
        {
	    if (HIWORD(lParam))
	    {
		lResult = unicode? DefWindowProcW(hwnd, WM_SETTEXT, wParam, lParam) :
				 DefWindowProcA(hwnd, uMsg, wParam, lParam);
	    }
	    if (uMsg == WM_SETTEXT)
		STATIC_TryPaintFcn( hwnd, full_style );
	    break;
	}
	default:
	    if (HIWORD(lParam))
	    {
		lResult = DefWindowProcW( hwnd, WM_SETTEXT, wParam, lParam );
	    }
	    if(uMsg == WM_SETTEXT)
		InvalidateRect(hwnd, NULL, TRUE);
	}
        return 1; /* success. FIXME: check text length */

    case WM_SETFONT:
        if ((style == SS_ICON) || (style == SS_BITMAP)) return 0;
        SetWindowLongA( hwnd, HFONT_GWL_OFFSET, wParam );
        if (LOWORD(lParam))
            InvalidateRect( hwnd, NULL, TRUE );
        break;

    case WM_GETFONT:
        return GetWindowLongA( hwnd, HFONT_GWL_OFFSET );

    case WM_NCHITTEST:
        if (full_style & SS_NOTIFY)
           return HTCLIENT;
        else
           return HTTRANSPARENT;

    case WM_GETDLGCODE:
        return DLGC_STATIC;

    case WM_LBUTTONDOWN:
	if (! (full_style & SS_NOTIFY)) return 0;
	if (unicode)
	    SendMessageW(GetParent(hwnd), WM_COMMAND,
			 MAKEWPARAM((WORD) GetWindowLongW(hwnd, GWL_ID), STN_CLICKED), (LPARAM) hwnd);
	else
	    SendMessageA(GetParent(hwnd), WM_COMMAND,
			 MAKEWPARAM((WORD) GetWindowLongA(hwnd, GWL_ID), STN_CLICKED), (LPARAM) hwnd);
	return 0;

    case WM_LBUTTONDBLCLK:
	if (! (full_style & SS_NOTIFY)) return 0;
	if (unicode)
	    SendMessageW(GetParent(hwnd), WM_COMMAND,
	             MAKEWPARAM((WORD) GetWindowLongW(hwnd, GWL_ID), STN_DBLCLK), (LPARAM) hwnd);
	else
	    SendMessageA(GetParent(hwnd), WM_COMMAND,
	             MAKEWPARAM((WORD) GetWindowLongA(hwnd, GWL_ID), STN_DBLCLK), (LPARAM) hwnd);
	return 0;

    case STM_GETIMAGE:
    case STM_GETICON:
        return GetWindowLongA( hwnd, HICON_GWL_OFFSET );

    case STM_SETIMAGE:
        switch(wParam) {
	case IMAGE_BITMAP:
	    lResult = (LRESULT)STATIC_SetBitmap( hwnd, (HBITMAP)lParam, style );
	    break;
	case IMAGE_ICON:
	    lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)lParam, style );
	    break;
	default:
	    OutputDebugStringA("STM_SETIMAGE: Unhandled type\n");
	    break;
	}
        InvalidateRect( hwnd, NULL, TRUE );
	break;

    case STM_SETICON:
        lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)wParam, style );
        InvalidateRect( hwnd, NULL, TRUE );
        break;

    default:
	return unicode ? DefWindowProcW(hwnd, uMsg, wParam, lParam) :
				 DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }

    return lResult;
}


/*********************************************************************
 *
 *	StaticWndProcW   (USER32.@)
 */
LRESULT CALLBACK StaticWndProcW(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return StaticWndProc_common(hWnd, uMsg, wParam, lParam, TRUE);
}

/*********************************************************************
 *
 *	StaticWndProc   (USER32.@)
 */
LRESULT CALLBACK StaticWndProcA(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return StaticWndProc_common(hWnd, uMsg, wParam, lParam, FALSE);
}

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, DWORD style )
{
  DRAWITEMSTRUCT dis;
  LONG id = GetWindowLongA( hwnd, GWL_ID );

  dis.CtlType    = ODT_STATIC;
  dis.CtlID      = id;
  dis.itemID     = 0;
  dis.itemAction = ODA_DRAWENTIRE;
  dis.itemState  = 0;
  dis.hwndItem   = hwnd;
  dis.hDC        = hdc;
  dis.itemData   = 0;
  GetClientRect( hwnd, &dis.rcItem );

  SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd );
  SendMessageW( GetParent(hwnd), WM_DRAWITEM, id, (LPARAM)&dis );
}

static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hBrush;
    HFONT hFont;
    UINT wFormat;
    INT len;
    WCHAR *text;
    HPEN ShadowPen, HighlightPen;

    GetClientRect( hwnd, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_LEFT:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_CENTER:
	wFormat = DT_CENTER | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_RIGHT:
	wFormat = DT_RIGHT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_SIMPLE:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP;
	break;

    case SS_LEFTNOWORDWRAP:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_VCENTER;
	break;

    default:
        return;
    }

    if (style & SS_NOPREFIX)
	wFormat |= DT_NOPREFIX;
    switch(style & SS_ELLIPSISMASK)
    {
    case SS_WORDELLIPSIS:
	wFormat |= DT_WORD_ELLIPSIS | DT_SINGLELINE;
	break;
    case SS_ENDELLIPSIS:
	wFormat |= DT_END_ELLIPSIS;
	wFormat &= ~DT_WORDBREAK;
	break;
    case SS_PATHELLIPSIS:
	wFormat |= DT_PATH_ELLIPSIS;
	wFormat &= ~DT_WORDBREAK;
	break;
    }

    if ((hFont = (HFONT)GetWindowLongA( hwnd, HFONT_GWL_OFFSET ))) SelectObject( hdc, hFont );

    if ((style & SS_NOPREFIX) || ((style & SS_TYPEMASK) != SS_SIMPLE))
    {
        hBrush = (HBRUSH)SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC,
				       (WPARAM)hdc, (LPARAM)hwnd );
        if (!hBrush) /* did the app forget to call defwindowproc ? */
            hBrush = (HBRUSH)DefWindowProcW(GetParent(hwnd), WM_CTLCOLORSTATIC,
					    (WPARAM)hdc, (LPARAM)hwnd);
        FillRect( hdc, &rc, hBrush );
    }
    if (!IsWindowEnabled(hwnd)) SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
    SetBkMode(hdc, TRANSPARENT);

    if (style & SS_SUNKEN)
    {
	ShadowPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
	SelectObject(hdc, ShadowPen);
	MoveToEx(hdc, 0, rc.bottom - 2, NULL);
	LineTo(hdc, 0, 0);
	LineTo(hdc, rc.right - 1, 0);
	HighlightPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
	SelectObject(hdc, HighlightPen);
	LineTo(hdc, rc.right - 1, rc.bottom - 1);
	LineTo(hdc, -1, rc.bottom - 1);
	SelectObject(hdc, GetStockObject(WHITE_PEN));
	DeleteObject(HighlightPen);
	DeleteObject(ShadowPen);
	rc.left++;
	rc.right--;
	rc.top++;
	rc.bottom--;
    }

    if (!(len = SendMessageW( hwnd, WM_GETTEXTLENGTH, 0, 0 ))) return;
    if (!(text = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return;
    SendMessageW( hwnd, WM_GETTEXT, len + 1, (LPARAM)text );
    DrawTextW( hdc, text, -1, &rc, wFormat );
    HeapFree( GetProcessHeap(), 0, text );
}

static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hBrush;

    GetClientRect( hwnd, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush(color_black);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYRECT:
	hBrush = CreateSolidBrush(color_gray);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_WHITERECT:
	hBrush = CreateSolidBrush(color_white);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_BLACKFRAME:
	hBrush = CreateSolidBrush(color_black);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYFRAME:
	hBrush = CreateSolidBrush(color_gray);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_WHITEFRAME:
	hBrush = CreateSolidBrush(color_white);
        FrameRect( hdc, &rc, hBrush );
	break;
    default:
        return;
    }
    DeleteObject( hBrush );
}


static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hbrush;
    HICON hIcon;

    GetClientRect( hwnd, &rc );
    hbrush = (HBRUSH)SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC,
				   (WPARAM)hdc, (LPARAM)hwnd );
    FillRect( hdc, &rc, hbrush );
    if ((hIcon = (HICON)GetWindowLongA( hwnd, HICON_GWL_OFFSET )))
        DrawIcon( hdc, rc.left, rc.top, hIcon );
}

static void STATIC_PaintBitmapfn(HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hbrush;
    HDC hMemDC;
    HBITMAP hBitmap, oldbitmap;
    LONG full_style = GetWindowLongA( hwnd, GWL_STYLE );

    GetClientRect( hwnd, &rc );
    hbrush = (HBRUSH)SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC,
				   (WPARAM)hdc, (LPARAM)hwnd );
    FillRect( hdc, &rc, hbrush );

    if ((hBitmap = (HBITMAP)GetWindowLongA( hwnd, HICON_GWL_OFFSET )))
    {
        BITMAP bm;

        if(GetObjectType(hBitmap) != OBJ_BITMAP) return;
        if (!(hMemDC = CreateCompatibleDC( hdc ))) return;
	GetObjectW(hBitmap, sizeof(bm), &bm);
	oldbitmap = SelectObject(hMemDC, hBitmap);
        if (full_style & SS_CENTERIMAGE)
        {
            BitBlt(hdc, (rc.right - bm.bmWidth) >> 1,
                   (rc.bottom - bm.bmHeight) >> 1,
                   bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);
        }
        else
        {
            BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0,
                   SRCCOPY);
        }
	SelectObject(hMemDC, oldbitmap);
	DeleteDC(hMemDC);
    }
}


static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;

    GetClientRect( hwnd, &rc );
    switch (style & SS_TYPEMASK)
    {
	case SS_ETCHEDHORZ:
#if 0 /* This is what seems logical */
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_TOP|BF_BOTTOM);
#else /* But this is what NT, Win2k and WinXP actually do */
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_TOP);
#endif
	    break;
	case SS_ETCHEDVERT:
#if 0 /* This is what seems logical */
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_LEFT|BF_RIGHT);
#else /* But this is what NT, Win2k and WinXP actually do */
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_LEFT);
#endif
	    break;
	case SS_ETCHEDFRAME:
	    DrawEdge (hdc, &rc, EDGE_ETCHED, BF_RECT);
	    break;
    }
}
