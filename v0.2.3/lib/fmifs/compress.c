/* $Id: compress.c,v 1.2 2004/02/23 11:55:12 ekohl Exp $
 *
 * COPYING:	See the top level directory
 * PROJECT:	ReactOS 
 * FILE:	reactos/lib/fmifs/compress.c
 * DESCRIPTION:	File management IFS utility functions
 * PROGRAMMER:	Emanuele Aliberti
 * UPDATED
 * 	1999-02-16 (Emanuele Aliberti)
 * 		Entry points added.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <fmifs.h>


/* FMIFS.4 */
BOOLEAN STDCALL
EnableVolumeCompression (PWCHAR DriveRoot,
			 BOOLEAN Enable)
{
  return FALSE;
}

/* EOF */
