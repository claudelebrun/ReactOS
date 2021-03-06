/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
/* $Id: apc.c,v 1.7 2002/09/08 10:23:04 chorns Exp $
 *
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode APC support
 * FILE:              lib/ntdll/rtl/apc.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <string.h>

/* FUNCTIONS ***************************************************************/

VOID STDCALL 
KiUserApcDispatcher(PIO_APC_ROUTINE ApcRoutine,
		    PVOID ApcContext,
		    PIO_STATUS_BLOCK Iosb,
		    ULONG Reserved,
		    PCONTEXT Context)
{
   /*
    * Call the APC 
    */
   ApcRoutine(ApcContext,
	      Iosb,
	      Reserved);
   /*
    * Switch back to the interrupted context
    */
   NtContinue(Context, 1);
}

