/* $Id: ldr.c,v 1.21 2004/06/13 20:04:56 navaraf Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/ldr.c
 * AUTHOR   : Boudewijn Dekker
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

typedef struct tagLOADPARMS32 {
  LPSTR lpEnvAddress;
  LPSTR lpCmdLine;
  LPSTR lpCmdShow;
  DWORD dwReserved;
} LOADPARMS32;

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
DisableThreadLibraryCalls (
	HMODULE	hLibModule
	)
{
	NTSTATUS Status;

	Status = LdrDisableThreadCalloutsForDll ((PVOID)hLibModule);
	if (!NT_SUCCESS (Status))
	{
		SetLastErrorByStatus (Status);
		return FALSE;
	}
	return TRUE;
}


/*
 * @implemented
 */
HINSTANCE
STDCALL
LoadLibraryA (
	LPCSTR	lpLibFileName
	)
{
	return LoadLibraryExA (lpLibFileName, 0, 0);
}


/*
 * @implemented
 */
HINSTANCE
STDCALL
LoadLibraryExA (
	LPCSTR	lpLibFileName,
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	UNICODE_STRING LibFileNameU;
	ANSI_STRING LibFileName;
	HINSTANCE hInst;
	NTSTATUS Status;

        (void)hFile;

	RtlInitAnsiString (&LibFileName,
	                   (LPSTR)lpLibFileName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&LibFileNameU,
		                              &LibFileName,
		                              TRUE);
	else
		RtlOemStringToUnicodeString (&LibFileNameU,
		                             &LibFileName,
		                             TRUE);

	Status = LdrLoadDll(NULL,
			    dwFlags,
			    &LibFileNameU,
			    (PVOID*)&hInst);

	RtlFreeUnicodeString (&LibFileNameU);

	if ( !NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return hInst;
}


/*
 * @implemented
 */
HINSTANCE
STDCALL
LoadLibraryW (
	LPCWSTR	lpLibFileName
	)
{
	return LoadLibraryExW (lpLibFileName, 0, 0);
}


/*
 * @implemented
 */
HINSTANCE
STDCALL
LoadLibraryExW (
	LPCWSTR	lpLibFileName,
	HANDLE	hFile,
	DWORD	dwFlags
	)
{
	UNICODE_STRING DllName;
	HINSTANCE hInst;
	NTSTATUS Status;

        (void)hFile;

	if ( lpLibFileName == NULL )
		return NULL;

	dwFlags &= 
	  DONT_RESOLVE_DLL_REFERENCES |
	  LOAD_LIBRARY_AS_DATAFILE |
	  LOAD_WITH_ALTERED_SEARCH_PATH;

	RtlInitUnicodeString (&DllName, (LPWSTR)lpLibFileName);
	Status = LdrLoadDll(NULL, dwFlags, &DllName, (PVOID*)&hInst);
	if ( !NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}
	
	return hInst;
}


/*
 * @implemented
 */
FARPROC
STDCALL
GetProcAddress( HMODULE hModule, LPCSTR lpProcName )
{
	ANSI_STRING ProcedureName;
	FARPROC fnExp = NULL;

	if (HIWORD(lpProcName) != 0)
	{
		RtlInitAnsiString (&ProcedureName,
		                   (LPSTR)lpProcName);
		LdrGetProcedureAddress ((PVOID)hModule,
		                        &ProcedureName,
		                        0,
		                        (PVOID*)&fnExp);
	}
	else
	{
		LdrGetProcedureAddress ((PVOID)hModule,
		                        NULL,
		                        (ULONG)lpProcName,
		                        (PVOID*)&fnExp);
	}

	return fnExp;
}


/*
 * @implemented
 */
BOOL
STDCALL
FreeLibrary( HMODULE hLibModule )
{
	LdrUnloadDll(hLibModule);
	return TRUE;
}


/*
 * @implemented
 */
VOID
STDCALL
FreeLibraryAndExitThread (
	HMODULE	hLibModule,
	DWORD	dwExitCode
	)
{
	if ( FreeLibrary(hLibModule) )
		ExitThread(dwExitCode);
	for (;;)
		;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetModuleFileNameA (
	HINSTANCE	hModule,
	LPSTR		lpFilename,
	DWORD		nSize
	)
{
	ANSI_STRING FileName;
	PLIST_ENTRY ModuleListHead;
	PLIST_ENTRY Entry;
	PLDR_MODULE Module;
	PPEB Peb;
	ULONG Length = 0;

	Peb = NtCurrentPeb ();
	RtlEnterCriticalSection (Peb->LoaderLock);

	if (hModule == NULL)
		hModule = Peb->ImageBaseAddress;

	ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
	Entry = ModuleListHead->Flink;

	while (Entry != ModuleListHead)
	{
		Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);
		if (Module->BaseAddress == (PVOID)hModule)
		{
			if (nSize * sizeof(WCHAR) < Module->FullDllName.Length)
			{
				SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
			}
			else
			{
				FileName.Length = 0;
				FileName.MaximumLength = nSize * sizeof(WCHAR);
				FileName.Buffer = lpFilename;

				/* convert unicode string to ansi (or oem) */
				if (bIsFileApiAnsi)
					RtlUnicodeStringToAnsiString (&FileName,
					                              &Module->FullDllName,
					                              FALSE);
				else
					RtlUnicodeStringToOemString (&FileName,
					                             &Module->FullDllName,
					                             FALSE);
				Length = Module->FullDllName.Length / sizeof(WCHAR);
			}

			RtlLeaveCriticalSection (Peb->LoaderLock);
			return Length;
		}

		Entry = Entry->Flink;
	}

	SetLastErrorByStatus (STATUS_DLL_NOT_FOUND);
	RtlLeaveCriticalSection (Peb->LoaderLock);

	return 0;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetModuleFileNameW (
	HINSTANCE	hModule,
	LPWSTR		lpFilename,
	DWORD		nSize
	)
{
	UNICODE_STRING FileName;
	PLIST_ENTRY ModuleListHead;
	PLIST_ENTRY Entry;
	PLDR_MODULE Module;
	PPEB Peb;
	ULONG Length = 0;

	Peb = NtCurrentPeb ();
	RtlEnterCriticalSection (Peb->LoaderLock);

	if (hModule == NULL)
		hModule = Peb->ImageBaseAddress;

	ModuleListHead = &Peb->Ldr->InLoadOrderModuleList;
	Entry = ModuleListHead->Flink;
	while (Entry != ModuleListHead)
	{
		Module = CONTAINING_RECORD(Entry, LDR_MODULE, InLoadOrderModuleList);

		if (Module->BaseAddress == (PVOID)hModule)
		{
			if (nSize * sizeof(WCHAR) < Module->FullDllName.Length)
			{
				SetLastErrorByStatus (STATUS_BUFFER_TOO_SMALL);
			}
			else
			{
				FileName.Length = 0;
				FileName.MaximumLength = nSize * sizeof(WCHAR);
				FileName.Buffer = lpFilename;

				RtlCopyUnicodeString (&FileName,
				                      &Module->FullDllName);
				Length = Module->FullDllName.Length / sizeof(WCHAR);
			}

			RtlLeaveCriticalSection (Peb->LoaderLock);
			return Length;
		}

		Entry = Entry->Flink;
	}

	SetLastErrorByStatus (STATUS_DLL_NOT_FOUND);
	RtlLeaveCriticalSection (Peb->LoaderLock);

	return 0;
}


/*
 * @implemented
 */
HMODULE
STDCALL
GetModuleHandleA ( LPCSTR lpModuleName )
{
	UNICODE_STRING UnicodeName;
	ANSI_STRING ModuleName;
	PVOID BaseAddress;
	NTSTATUS Status;

	if (lpModuleName == NULL)
		return ((HMODULE)NtCurrentPeb()->ImageBaseAddress);
	RtlInitAnsiString (&ModuleName,
	                   (LPSTR)lpModuleName);

	/* convert ansi (or oem) string to unicode */
	if (bIsFileApiAnsi)
		RtlAnsiStringToUnicodeString (&UnicodeName,
					      &ModuleName,
					      TRUE);
	else
		RtlOemStringToUnicodeString (&UnicodeName,
					     &ModuleName,
					     TRUE);

	Status = LdrGetDllHandle (0,
				  0,
				  &UnicodeName,
				  &BaseAddress);

	RtlFreeUnicodeString (&UnicodeName);

	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return ((HMODULE)BaseAddress);
}


/*
 * @implemented
 */
HMODULE
STDCALL
GetModuleHandleW (LPCWSTR lpModuleName)
{
	UNICODE_STRING ModuleName;
	PVOID BaseAddress;
	NTSTATUS Status;

	if (lpModuleName == NULL)
		return ((HMODULE)NtCurrentPeb()->ImageBaseAddress);

	RtlInitUnicodeString (&ModuleName,
			      (LPWSTR)lpModuleName);

	Status = LdrGetDllHandle (0,
				  0,
				  &ModuleName,
				  &BaseAddress);
	if (!NT_SUCCESS(Status))
	{
		SetLastErrorByStatus (Status);
		return NULL;
	}

	return ((HMODULE)BaseAddress);
}


/*
 * @implemented
 */
DWORD
STDCALL
LoadModule (
    LPCSTR  lpModuleName,
    LPVOID  lpParameterBlock
    )
{
  STARTUPINFOA StartupInfo;
  PROCESS_INFORMATION ProcessInformation;
  LOADPARMS32 *LoadParams;
  char FileName[MAX_PATH];
  char *CommandLine, *t;
  BYTE Length;
  
  LoadParams = (LOADPARMS32*)lpParameterBlock;
  if(!lpModuleName || !LoadParams || (((WORD*)LoadParams->lpCmdShow)[0] != 2))
  {
    /* windows doesn't check parameters, we do */
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  
  if(!SearchPathA(NULL, lpModuleName, ".exe", MAX_PATH, FileName, NULL) &&
     !SearchPathA(NULL, lpModuleName, NULL, MAX_PATH, FileName, NULL))
  {
    return ERROR_FILE_NOT_FOUND;
  }
  
  Length = (BYTE)LoadParams->lpCmdLine[0];
  if(!(CommandLine = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                               strlen(lpModuleName) + Length + 2)))
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
  }
  
  /* Create command line string */
  strcpy(CommandLine, lpModuleName);
  t = CommandLine + strlen(CommandLine);
  *(t++) = ' ';
  memcpy(t, LoadParams->lpCmdLine + 1, Length);
  
  /* Build StartupInfo */
  RtlZeroMemory(&StartupInfo, sizeof(STARTUPINFOA));
  StartupInfo.cb = sizeof(STARTUPINFOA);
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = ((WORD*)LoadParams->lpCmdShow)[1];
  
  if(!CreateProcessA(FileName, CommandLine, NULL, NULL, FALSE, 0, LoadParams->lpEnvAddress,
                     NULL, &StartupInfo, &ProcessInformation))
  {
    DWORD Error;
    
    HeapFree(GetProcessHeap(), 0, CommandLine);
    /* return the right value */
    Error = GetLastError();
    switch(Error)
    {
      case ERROR_BAD_EXE_FORMAT:
      {
        return ERROR_BAD_FORMAT;
      }
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
      {
        return Error;
      }
    }
    return 0;
  }
  
  HeapFree(GetProcessHeap(), 0, CommandLine);
  
  /* Wait up to 15 seconds for the process to become idle */
  WaitForInputIdle(ProcessInformation.hProcess, 15000);
  
  CloseHandle(ProcessInformation.hThread);
  CloseHandle(ProcessInformation.hProcess);
  
  return 33;
}

/* EOF */
