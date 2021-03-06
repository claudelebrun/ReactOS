/* $Id: access.c,v 1.4 2003/07/11 13:50:23 royce Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Access rights handling functions
 * FILE:              lib/ntdll/rtl/access.c
 * PROGRAMER:         Eric Kohl <ekohl@rz-online.de>
 * REVISION HISTORY:
 *                    06/04/2000: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <ntdll/ntdll.h>

/* FUNCTIONS ***************************************************************/

BOOLEAN
STDCALL
RtlAreAllAccessesGranted (
	ACCESS_MASK	GrantedAccess,
	ACCESS_MASK	DesiredAccess
	)
{
	return ((GrantedAccess & DesiredAccess) == DesiredAccess);
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlAreAnyAccessesGranted (
	ACCESS_MASK	GrantedAccess,
	ACCESS_MASK	DesiredAccess
	)
{
	return ((GrantedAccess & DesiredAccess) != 0);
}


/*
 * @implemented
 */
VOID
STDCALL
RtlMapGenericMask (
	PACCESS_MASK		AccessMask,
	PGENERIC_MAPPING	GenericMapping
	)
{
	if (*AccessMask & GENERIC_READ)
		*AccessMask |= GenericMapping->GenericRead;

	if (*AccessMask & GENERIC_WRITE)
		*AccessMask |= GenericMapping->GenericWrite;

	if (*AccessMask & GENERIC_EXECUTE)
		*AccessMask |= GenericMapping->GenericExecute;

	if (*AccessMask & GENERIC_ALL)
		*AccessMask |= GenericMapping->GenericAll;

	*AccessMask &= 0x0FFFFFFF;
}

/* EOF */
