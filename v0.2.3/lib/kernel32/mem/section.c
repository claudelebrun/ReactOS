/* $Id: section.c,v 1.25 2004/05/25 20:04:14 navaraf Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/kernel32/mem/section.c
 * PURPOSE:              Implementing file mapping
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FUNCTIONS *****************************************************************/

#define MASK_PAGE_FLAGS (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY)
#define MASK_SEC_FLAGS  (SEC_COMMIT | SEC_IMAGE | SEC_NOCACHE | SEC_RESERVE)

/*
 * @implemented
 */
HANDLE STDCALL
CreateFileMappingA(HANDLE hFile,
		   LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
		   DWORD flProtect,
		   DWORD dwMaximumSizeHigh,
		   DWORD dwMaximumSizeLow,
		   LPCSTR lpName)
{
   NTSTATUS Status;
   HANDLE SectionHandle;
   LARGE_INTEGER MaximumSize;
   OBJECT_ATTRIBUTES ObjectAttributes;
   ANSI_STRING AnsiName;
   UNICODE_STRING UnicodeName;
   PSECURITY_DESCRIPTOR SecurityDescriptor;
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_STANDARD_INFORMATION FileStandard;

   if ((flProtect & (MASK_PAGE_FLAGS | MASK_SEC_FLAGS)) != flProtect)
     {
        DPRINT1("Invalid flProtect 0x%08x\n", flProtect);
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
     }
   if (lpFileMappingAttributes)
     {
        SecurityDescriptor = lpFileMappingAttributes->lpSecurityDescriptor;
     }
   else
     {
        SecurityDescriptor = NULL;
     }

   if((dwMaximumSizeLow==0)&&(dwMaximumSizeHigh==0))
   {
	   // MSDN: If dwMaximumSizeLow and dwMaximumSizeHigh are zero, the maximum size of the 
	   // file-mapping object is equal to the current size of the file identified by hFile. 
	   Status = NtQueryInformationFile(hFile,
		   &IoStatusBlock,
		   &FileStandard,
		   sizeof(FILE_STANDARD_INFORMATION),
		   FileStandardInformation);
	   if (!NT_SUCCESS(Status))
	   {
		   DPRINT("Status 0x%08x obtaining FileStandardInformation for source\n", Status);
		   SetLastErrorByStatus(Status);
		   return NULL;
	   }
	   MaximumSize = FileStandard.EndOfFile;
   } else {
	   MaximumSize.u.LowPart = dwMaximumSizeLow;
	   MaximumSize.u.HighPart = dwMaximumSizeHigh;
   }
   RtlInitAnsiString(&AnsiName,
		     (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&UnicodeName,
				&AnsiName,
				TRUE);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      0,
			      hBaseDir,
			      SecurityDescriptor);
   Status = NtCreateSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    &ObjectAttributes,
			    &MaximumSize,
			    flProtect & MASK_PAGE_FLAGS,
			    flProtect & MASK_SEC_FLAGS,
			    hFile==INVALID_HANDLE_VALUE ? NULL : hFile);
   RtlFreeUnicodeString(&UnicodeName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }
   return SectionHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
CreateFileMappingW(HANDLE hFile,
		   LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
		   DWORD flProtect,
		   DWORD dwMaximumSizeHigh,
		   DWORD dwMaximumSizeLow,
		   LPCWSTR lpName)
{
   NTSTATUS Status;
   HANDLE SectionHandle;
   LARGE_INTEGER MaximumSize;
   PLARGE_INTEGER MaximumSizePointer;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING UnicodeName;
   PSECURITY_DESCRIPTOR SecurityDescriptor;
   IO_STATUS_BLOCK IoStatusBlock;
   FILE_STANDARD_INFORMATION FileStandard;

   if ((flProtect & (MASK_PAGE_FLAGS | MASK_SEC_FLAGS)) != flProtect)
     {
        DPRINT1("Invalid flProtect 0x%08x\n", flProtect);
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
     }
   if (lpFileMappingAttributes)
     {
        SecurityDescriptor = lpFileMappingAttributes->lpSecurityDescriptor;
     }
   else
     {
        SecurityDescriptor = NULL;
     }

   if ((dwMaximumSizeLow == 0) && (dwMaximumSizeHigh == 0))
     {
	   // MSDN: If dwMaximumSizeLow and dwMaximumSizeHigh are zero, the maximum size of the 
	   // file-mapping object is equal to the current size of the file identified by hFile. 
	   Status = NtQueryInformationFile(hFile,
		   &IoStatusBlock,
		   &FileStandard,
		   sizeof(FILE_STANDARD_INFORMATION),
		   FileStandardInformation);
	   if (!NT_SUCCESS(Status))
	   {
		   DPRINT("Status 0x%08x obtaining FileStandardInformation for source\n", Status);
		   SetLastErrorByStatus(Status);
		   return NULL;
	   }
	   MaximumSize = FileStandard.EndOfFile;
       MaximumSizePointer = &MaximumSize;
     }
   else
     {
       MaximumSize.u.LowPart = dwMaximumSizeLow;
       MaximumSize.u.HighPart = dwMaximumSizeHigh;
       MaximumSizePointer = &MaximumSize;
     }
   RtlInitUnicodeString(&UnicodeName,
			lpName);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      0,
			      hBaseDir,
			      SecurityDescriptor);
   Status = NtCreateSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    &ObjectAttributes,
			    MaximumSizePointer,
			    flProtect & MASK_PAGE_FLAGS,
			    flProtect & MASK_SEC_FLAGS,
			    hFile==INVALID_HANDLE_VALUE ? NULL : hFile);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }
   return SectionHandle;
}


/*
 * @implemented
 */
LPVOID STDCALL
MapViewOfFileEx(HANDLE hFileMappingObject,
		DWORD dwDesiredAccess,
		DWORD dwFileOffsetHigh,
		DWORD dwFileOffsetLow,
		DWORD dwNumberOfBytesToMap,
		LPVOID lpBaseAddress)
{
   NTSTATUS Status;
   LARGE_INTEGER SectionOffset;
   ULONG ViewSize;
   ULONG Protect;
   LPVOID BaseAddress;

   SectionOffset.u.LowPart = dwFileOffsetLow;
   SectionOffset.u.HighPart = dwFileOffsetHigh;

   if ( ( dwDesiredAccess & FILE_MAP_WRITE) == FILE_MAP_WRITE)
	Protect  = PAGE_READWRITE;
   else if ((dwDesiredAccess & FILE_MAP_READ) == FILE_MAP_READ)
	Protect = PAGE_READONLY;
   else if ((dwDesiredAccess & FILE_MAP_ALL_ACCESS) == FILE_MAP_ALL_ACCESS)
	Protect  = PAGE_READWRITE;
   else if ((dwDesiredAccess & FILE_MAP_COPY) == FILE_MAP_COPY)
	Protect = PAGE_WRITECOPY;
   else
	Protect = PAGE_READWRITE;
   
   if (lpBaseAddress == NULL)
     {
	BaseAddress = NULL;
     }
   else
     {
	BaseAddress = lpBaseAddress;
     }

   ViewSize = (ULONG) dwNumberOfBytesToMap;

   Status = ZwMapViewOfSection(hFileMappingObject,
			       NtCurrentProcess(),
			       &BaseAddress,
			       0,
			       dwNumberOfBytesToMap,
			       &SectionOffset,
			       &ViewSize,
			       ViewShare,
			       0,
			       Protect);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }
   return BaseAddress;
}


/*
 * @implemented
 */
LPVOID STDCALL
MapViewOfFile(HANDLE hFileMappingObject,
	      DWORD dwDesiredAccess,
	      DWORD dwFileOffsetHigh,
	      DWORD dwFileOffsetLow,
	      DWORD dwNumberOfBytesToMap)
{
   return MapViewOfFileEx(hFileMappingObject,
			  dwDesiredAccess,
			  dwFileOffsetHigh,
			  dwFileOffsetLow,
			  dwNumberOfBytesToMap,
			  NULL);
}


/*
 * @implemented
 */
BOOL STDCALL
UnmapViewOfFile(LPVOID lpBaseAddress)
{
   NTSTATUS Status;

   Status = NtUnmapViewOfSection(NtCurrentProcess(),
				 lpBaseAddress);
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
HANDLE STDCALL
OpenFileMappingA(DWORD dwDesiredAccess,
		 BOOL bInheritHandle,
		 LPCSTR lpName)
{
   NTSTATUS Status;
   HANDLE SectionHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   ANSI_STRING AnsiName;
   UNICODE_STRING UnicodeName;

   ULONG Attributes = 0;

   if (bInheritHandle)
     {
	Attributes = OBJ_INHERIT;
     }

   RtlInitAnsiString(&AnsiName,
		     (LPSTR)lpName);
   RtlAnsiStringToUnicodeString(&UnicodeName,
				&AnsiName,
				TRUE);

   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      Attributes,
			      hBaseDir,
			      NULL);
   Status = NtOpenSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    &ObjectAttributes);
   RtlFreeUnicodeString (&UnicodeName);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus (Status);
	return NULL;
     }
   return SectionHandle;
}


/*
 * @implemented
 */
HANDLE STDCALL
OpenFileMappingW(DWORD dwDesiredAccess,
		 BOOL bInheritHandle,
		 LPCWSTR lpName)
{
   NTSTATUS Status;
   HANDLE SectionHandle;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING UnicodeName;
   ULONG Attributes = 0;

   if (bInheritHandle)
     {
	Attributes = OBJ_INHERIT;
     }

   RtlInitUnicodeString(&UnicodeName,
			lpName);
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeName,
			      Attributes,
			      hBaseDir,
			      NULL);
   Status = ZwOpenSection(&SectionHandle,
			    SECTION_ALL_ACCESS,
			    &ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return NULL;
     }
   return SectionHandle;
}


/*
 * @implemented
 */
BOOL STDCALL
FlushViewOfFile(LPCVOID lpBaseAddress,
		DWORD dwNumberOfBytesToFlush)
{
   NTSTATUS Status;
   ULONG NumberOfBytesFlushed;

   Status = NtFlushVirtualMemory(NtCurrentProcess(),
				 (LPVOID)lpBaseAddress,
				 dwNumberOfBytesToFlush,
				 &NumberOfBytesFlushed);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   return TRUE;
}

/* EOF */
