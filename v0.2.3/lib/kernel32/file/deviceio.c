/* $Id: deviceio.c,v 1.15 2004/01/23 21:16:03 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/deviceio.c
 * PURPOSE:         Device I/O and Overlapped Result functions
 * PROGRAMMER:      Ariadne (ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/*
 * @implemented
 */
BOOL
STDCALL
DeviceIoControl(
		HANDLE hDevice,
		DWORD dwIoControlCode,
		LPVOID lpInBuffer,
		DWORD nInBufferSize,
		LPVOID lpOutBuffer,
		DWORD nOutBufferSize,
		LPDWORD lpBytesReturned,
		LPOVERLAPPED lpOverlapped
		)
{
	NTSTATUS errCode = 0;
	HANDLE hEvent = NULL;
	PIO_STATUS_BLOCK IoStatusBlock;
	IO_STATUS_BLOCK IIosb;

	BOOL bFsIoControlCode = FALSE;

    DPRINT("DeviceIoControl(hDevice %x dwIoControlCode %d lpInBuffer %x "
          "nInBufferSize %d lpOutBuffer %x nOutBufferSize %d "
          "lpBytesReturned %x lpOverlapped %x)\n",
          hDevice,dwIoControlCode,lpInBuffer,nInBufferSize,lpOutBuffer,
          nOutBufferSize,lpBytesReturned,lpOverlapped);

	if (lpBytesReturned == NULL)
	{
        DPRINT("DeviceIoControl() - returning STATUS_INVALID_PARAMETER\n");
		SetLastErrorByStatus (STATUS_INVALID_PARAMETER);
		return FALSE;
	}
	//
	// TODO: Review and approve this change by RobD. IoCtrls for Serial.sys were 
	//       going to NtFsControlFile instead of NtDeviceIoControlFile.
	//		 Don't know at this point if anything else is affected by this change.
	//
	// if (((dwIoControlCode >> 16) & FILE_DEVICE_FILE_SYSTEM) == FILE_DEVICE_FILE_SYSTEM) {
	//

	if ((dwIoControlCode >> 16) == FILE_DEVICE_FILE_SYSTEM) {

		bFsIoControlCode = TRUE;
        DPRINT("DeviceIoControl() - FILE_DEVICE_FILE_SYSTEM == TRUE %x %x\n", dwIoControlCode, dwIoControlCode >> 16);
	} else {
		bFsIoControlCode = FALSE;
        DPRINT("DeviceIoControl() - FILE_DEVICE_FILE_SYSTEM == FALSE %x %x\n", dwIoControlCode, dwIoControlCode >> 16);
	}

	if(lpOverlapped  != NULL)
	{
		hEvent = lpOverlapped->hEvent;
		lpOverlapped->Internal = STATUS_PENDING;
		IoStatusBlock = (PIO_STATUS_BLOCK)lpOverlapped;
	}
	else
	{
		IoStatusBlock = &IIosb;
	}

	if (bFsIoControlCode == TRUE)
	{
		errCode = NtFsControlFile (hDevice,
		                           hEvent,
		                           NULL,
		                           NULL,
		                           IoStatusBlock,
		                           dwIoControlCode,
		                           lpInBuffer,
		                           nInBufferSize,
		                           lpOutBuffer,
		                           nOutBufferSize);
	}
	else
	{
		errCode = NtDeviceIoControlFile (hDevice,
		                                 hEvent,
		                                 NULL,
		                                 NULL,
		                                 IoStatusBlock,
		                                 dwIoControlCode,
		                                 lpInBuffer,
		                                 nInBufferSize,
		                                 lpOutBuffer,
		                                 nOutBufferSize);
	}

	if (errCode == STATUS_PENDING)
	{
        DPRINT("DeviceIoControl() - STATUS_PENDING\n");
		if (NtWaitForSingleObject(hDevice,FALSE,NULL) < 0)
		{
			*lpBytesReturned = IoStatusBlock->Information;
			SetLastErrorByStatus (errCode);
            DPRINT("DeviceIoControl() - STATUS_PENDING wait failed.\n");
			return FALSE;
		}
	}
	else if (!NT_SUCCESS(errCode))
	{
		SetLastErrorByStatus (errCode);
        DPRINT("DeviceIoControl() - ERROR: %x\n", errCode);
		return FALSE;
	}

	if (lpOverlapped)
		*lpBytesReturned = lpOverlapped->InternalHigh;
	else
		*lpBytesReturned = IoStatusBlock->Information;

	return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetOverlappedResult (
  IN HANDLE   hFile,
	IN LPOVERLAPPED	lpOverlapped,
	OUT LPDWORD		lpNumberOfBytesTransferred,
	IN BOOL		bWait
	)
{
	DWORD WaitStatus;
  HANDLE hObject;

  if (lpOverlapped->Internal == STATUS_PENDING)
  {
    if (!bWait)
    {
      /* can't use SetLastErrorByStatus(STATUS_PENDING) here, 
      since STATUS_PENDING translates to ERROR_IO_PENDING */
      SetLastError(ERROR_IO_INCOMPLETE);
      return FALSE;
    }
    
    hObject = lpOverlapped->hEvent ? lpOverlapped->hEvent : hFile;

    /* Wine delivers pending APC's while waiting, but Windows does
    not, nor do we... */
    WaitStatus = WaitForSingleObject(hObject, INFINITE);
    
    if (WaitStatus == WAIT_FAILED)
    {
      DPRINT("Wait failed!\n");
      /* WaitForSingleObjectEx sets the last error */
      return FALSE;
    }
  }

  *lpNumberOfBytesTransferred = lpOverlapped->InternalHigh;
  
  if (!NT_SUCCESS(lpOverlapped->Internal))
  {
    SetLastErrorByStatus(lpOverlapped->Internal);
    return FALSE;
  }

	return TRUE;
}

/* EOF */


