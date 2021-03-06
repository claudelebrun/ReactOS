/* $Id: misc.c,v 1.9 2003/11/17 20:35:46 sedwards Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Various functions
 * FILE:              lib/ntdll/rtl/misc.c
 * PROGRAMER:         Eric Kohl <ekohl@zr-online.de>
 * REVISION HISTORY:
 *                    10/08/2000: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

/* GLOBALS ******************************************************************/

extern ULONG NtGlobalFlag;

/* FUNCTIONS ****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetNtGlobalFlags
 *
 * DESCRIPTION
 *	Retrieves the global os flags.
 *
 * ARGUMENTS
 *	None
 *
 * RETURN VALUE
 *	global flags
 *
 * REVISIONS
 * 	2000-08-10 ekohl
 */

ULONG STDCALL
RtlGetNtGlobalFlags(VOID)
{
  return(NtGlobalFlag);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	RtlGetNtProductType
 *
 * DESCRIPTION
 *	Retrieves the OS product type.
 *
 * ARGUMENTS
 *	ProductType	Pointer to the product type variable.
 *
 * RETURN VALUE
 *	TRUE if successful, otherwise FALSE
 *
 * NOTE
 *	ProductType can be one of the following values:
 *	  1	Workstation (Winnt)
 *	  2	Server (Lanmannt)
 *	  3	Advanced Server (Servernt)
 *
 * REVISIONS
 * 	2000-08-10 ekohl
 *
 * @implemented
 */

BOOLEAN STDCALL
RtlGetNtProductType(PNT_PRODUCT_TYPE ProductType)
{
  *ProductType = SharedUserData->NtProductType;
  return(TRUE);
}

/**********************************************************************
 * NAME							EXPORTED
 *	RtlGetNtVersionNumbers
 *
 * DESCRIPTION
 *	Get the version numbers of the run time library.
 *
 * ARGUMENTS
 *	major [OUT]	Destination for the Major version
 *	minor [OUT]	Destination for the Minor version
 *	build [OUT]	Destination for the Build version
 *
 * RETURN VALUE
 *	Nothing.
 *
 * NOTE
 *	Introduced in Windows XP (NT5.1)
 *
 * @implemented
 */

void STDCALL
RtlGetNtVersionNumbers(LPDWORD major, LPDWORD minor, LPDWORD build)
{
	PPEB pPeb = NtCurrentPeb();

	if (major)
	{
		/* msvcrt.dll as released with XP Home fails in DLLMain() if the
		 * major version is not 5. So, we should never set a version < 5 ...
		 * This makes sense since this call didn't exist before XP anyway.
		 */
		*major = pPeb->OSMajorVersion < 5 ? 5 : pPeb->OSMajorVersion;
	}

	if (minor)
	{
		*minor = pPeb->OSMinorVersion;
	}

	if (build)
	{
		/* FIXME: Does anybody know the real formula? */
		*build = (0xF0000000 | pPeb->OSBuildNumber);
	}
}
