/* $Id: event.c,v 1.18 2004/01/23 21:16:04 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/synch/event.c
 * PURPOSE:         Local string functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
HANDLE STDCALL
CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes,
	     BOOL bManualReset,
	     BOOL bInitialState,
	     LPCSTR lpName)
{
   UNICODE_STRING EventNameU;
   ANSI_STRING EventName;
   HANDLE EventHandle;

   RtlInitUnicodeString (&EventNameU, NULL);

   if (lpName)
     {
	RtlInitAnsiString(&EventName,
			  (LPSTR)lpName);
	RtlAnsiStringToUnicodeString(&EventNameU,
				     &EventName,
				     TRUE);
     }

   EventHandle = CreateEventW(lpEventAttributes,
			      bManualReset,
			      bInitialState,
			      EventNameU.Buffer);

   if (lpName)
     {
	RtlFreeUnicodeString(&EventNameU);
     }

   return EventHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes,
	     BOOL bManualReset,
	     BOOL bInitialState,
	     LPCWSTR lpName)
{
   NTSTATUS Status;
   HANDLE hEvent;
   UNICODE_STRING EventNameString;
   OBJECT_ATTRIBUTES ObjectAttributes;

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = hBaseDir;
   ObjectAttributes.ObjectName = NULL;
   ObjectAttributes.Attributes = 0;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;

   if (NULL != lpEventAttributes)
     {
       if (sizeof(SECURITY_ATTRIBUTES) < lpEventAttributes->nLength)
         {
           SetLastError(ERROR_INVALID_PARAMETER);
           return NULL;
         }
       ObjectAttributes.SecurityDescriptor = lpEventAttributes->lpSecurityDescriptor;
       ObjectAttributes.Attributes = lpEventAttributes->bInheritHandle ? OBJ_INHERIT : 0;
     }

   if (lpName != NULL)
     {
	RtlInitUnicodeString(&EventNameString, (LPWSTR)lpName);
	ObjectAttributes.ObjectName = &EventNameString;
     }

   Status = NtCreateEvent(&hEvent,
			  STANDARD_RIGHTS_ALL|EVENT_READ_ACCESS|EVENT_WRITE_ACCESS,
			  &ObjectAttributes,
			  bManualReset,
			  bInitialState);
   DPRINT( "Called\n" );
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return hEvent;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenEventA(DWORD dwDesiredAccess,
	   BOOL bInheritHandle,
	   LPCSTR lpName)
{
   UNICODE_STRING EventNameU;
   ANSI_STRING EventName;
   HANDLE EventHandle;

   RtlInitUnicodeString(&EventNameU,
			NULL);

   if (lpName)
     {
	RtlInitAnsiString(&EventName,
			  (LPSTR)lpName);
	RtlAnsiStringToUnicodeString(&EventNameU,
				     &EventName,
				     TRUE);
    }

   EventHandle = OpenEventW(dwDesiredAccess,
			    bInheritHandle,
			    EventNameU.Buffer);

   if (lpName)
     {
	RtlFreeUnicodeString(&EventNameU);
     }

   return EventHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenEventW(DWORD dwDesiredAccess,
	   BOOL bInheritHandle,
	   LPCWSTR lpName)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING EventNameString;
   NTSTATUS Status;
   HANDLE hEvent = NULL;

   if (lpName == NULL)
     {
	SetLastError(ERROR_INVALID_PARAMETER);
	return NULL;
     }

   RtlInitUnicodeString(&EventNameString, (LPWSTR)lpName);

   ObjectAttributes.Length = sizeof(OBJECT_ATTRIBUTES);
   ObjectAttributes.RootDirectory = hBaseDir;
   ObjectAttributes.ObjectName = &EventNameString;
   ObjectAttributes.Attributes = 0;
   ObjectAttributes.SecurityDescriptor = NULL;
   ObjectAttributes.SecurityQualityOfService = NULL;
   if (bInheritHandle == TRUE)
     {
	ObjectAttributes.Attributes |= OBJ_INHERIT;
     }

   Status = NtOpenEvent(&hEvent,
			dwDesiredAccess,
			&ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }

   return hEvent;
}


/*
 * @implemented
 */
BOOL STDCALL
PulseEvent(HANDLE hEvent)
{
   ULONG Count;
   NTSTATUS Status;

   Status = NtPulseEvent(hEvent,
			 &Count);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return FALSE;
     }

   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
ResetEvent(HANDLE hEvent)
{
   NTSTATUS Status;
   ULONG Count;

   Status = NtResetEvent(hEvent,
			 &Count);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetEvent(HANDLE hEvent)
{
   NTSTATUS Status;
   ULONG Count;

   Status = NtSetEvent(hEvent,
		       &Count);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}

/* EOF */
