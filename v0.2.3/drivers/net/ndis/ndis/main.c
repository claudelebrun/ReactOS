/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/main.c
 * PURPOSE:     Driver entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   20 Aug 2003 Vizzini - NDIS4/5 revisions
 *   3  Oct 2003 Vizzini - formatting and minor bugfixing
 */
#include <ndissys.h>
#include <protocol.h>
#include <miniport.h>


#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

/* see miniport.c */
extern KSPIN_LOCK OrphanAdapterListLock;
extern LIST_ENTRY OrphanAdapterListHead;


VOID STDCALL MainUnload(
    PDRIVER_OBJECT DriverObject)
/*
 * FUNCTION: Unloads the driver
 * ARGUMENTS:
 *     DriverObject = Pointer to driver object created by the system
 */
{
  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


NTSTATUS
STDCALL
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Main driver entry point
 * ARGUMENTS:
 *     DriverObject = Pointer to a driver object for this driver
 *     RegistryPath = Registry node for configuration parameters
 * RETURNS:
 *     Status of driver initialization
 */
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  InitializeListHead(&ProtocolListHead);
  KeInitializeSpinLock(&ProtocolListLock);

  InitializeListHead(&MiniportListHead);
  KeInitializeSpinLock(&MiniportListLock);

  InitializeListHead(&AdapterListHead);
  KeInitializeSpinLock(&AdapterListLock);

  InitializeListHead(&OrphanAdapterListHead);
  KeInitializeSpinLock(&OrphanAdapterListLock);

  DriverObject->DriverUnload = MainUnload;

  /* 
   * until we have PNP support, query the enum key and NdisFindDevice() each one
   * NOTE- this will load and start other services before this one returns STATUS_SUCCESS.
   * I hope there aren't code reentrancy problems. :) 
   */
  //NdisStartDevices();

  return STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
CDECL
NdisWriteErrorLogEntry(
    IN  NDIS_HANDLE     NdisAdapterHandle,
    IN  NDIS_ERROR_CODE ErrorCode,
    IN  ULONG           NumberOfErrorValues,
    ...) 
/*
 * FUNCTION: Write a syslog error
 * ARGUMENTS:
 *     NdisAdapterHandle:  Handle passed into MiniportInitialize
 *     ErrorCode:  32-bit error code to be logged
 *     NumberOfErrorValues: number of errors to log
 *     Variable: list of log items
 * NOTES:
 *     - THIS IS >CDECL<
 *     - This needs to be fixed to do var args
 *     - FIXME - this needs to be properly implemented once we have an event log
 */
{
  NDIS_DbgPrint(MIN_TRACE, ("ERROR: ErrorCode 0x%x\n", ErrorCode));
  ASSERT(0);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeReadWriteLock(
    IN  PNDIS_RW_LOCK   Lock)
/*
 * FUNCTION: Initialize a NDIS_RW_LOCK
 * ARGUMENTS:
 *     Lock: pointer to the lock to initialize
 * NOTES:
 *    NDIS 5.0
 */
{
  memset(Lock,0,sizeof(NDIS_RW_LOCK));
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisWriteEventLogEntry(
    IN  PVOID       LogHandle,
    IN  NDIS_STATUS EventCode,
    IN  ULONG       UniqueEventValue,
    IN  USHORT      NumStrings,
    IN  PVOID       StringsList OPTIONAL,
    IN  ULONG       DataSize,
    IN  PVOID       Data        OPTIONAL)
/*
 * FUNCTION: Log an event in the system event log
 * ARGUMENTS:
 *     LogHandle: pointer to the driver object of the protocol logging the event
 *     EventCode: NDIS_STATUS_XXX describing the event
 *     UniqueEventValue: identifiees this instance of the error value
 *     NumStrings: number of strings in StringList
 *     StringList: list of strings to log
 *     DataSize: number of bytes in Data
 *     Data: binary dump data to help analyzing the event
 * NOTES:
 *     - STDCALL, not CDECL like WriteError...
 *     - FIXME Needs to use the real log interface, once there is one
 */
{
  /*
   * just returning true until we have an event log
   */
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  return NDIS_STATUS_SUCCESS;
}

/* EOF */

