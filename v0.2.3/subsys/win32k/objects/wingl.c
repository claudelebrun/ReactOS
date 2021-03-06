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
/* $Id: wingl.c,v 1.8 2004/05/10 17:07:20 weiden Exp $ */
#include <w32k.h>

INT
STDCALL
NtGdiChoosePixelFormat(HDC  hDC,
                           CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}


INT
STDCALL
NtGdiDescribePixelFormat(HDC  hDC,
                             INT  PixelFormat,
                             UINT  BufSize,
                             LPPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetEnhMetaFilePixelFormat(HENHMETAFILE  hEMF,
                                    DWORD  BufSize, 
                                    CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

INT
STDCALL
NtGdiGetPixelFormat(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiSetPixelFormat(HDC  hDC,
                         INT  PixelFormat,
                         CONST PPIXELFORMATDESCRIPTOR  pfd)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
NtGdiSwapBuffers(HDC  hDC)
{
  UNIMPLEMENTED;
}

/* EOF */
