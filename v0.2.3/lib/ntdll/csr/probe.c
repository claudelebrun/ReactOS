/* $Id: probe.c,v 1.4 2003/07/11 13:50:22 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/csr/probe.c
 * PURPOSE:         CSRSS address range access probing API
 * AUTHOR:          Eric Kohl
 * DATE:            2001-06-17
 */
#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <ntdll/rtl.h>

/*
 * @implemented
 */
VOID STDCALL
CsrProbeForRead(IN CONST PVOID Address,
		IN ULONG Length,
		IN ULONG Alignment)
{
   PUCHAR Pointer;
   UCHAR Data;

   if (Length == 0)
     return;

   if ((ULONG)Address & (Alignment - 1))
     RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);

   Pointer = (PUCHAR)Address;
   Data = *Pointer;
   Pointer = (PUCHAR)((ULONG)Address + Length -1);
   Data = *Pointer;
}

/*
 * @implemented
 */
VOID STDCALL
CsrProbeForWrite(IN CONST PVOID Address,
		 IN ULONG Length,
		 IN ULONG Alignment)
{
   PUCHAR Pointer;
   UCHAR Data;

   if (Length == 0)
     return;

   if ((ULONG)Address & (Alignment - 1))
     RtlRaiseStatus(STATUS_DATATYPE_MISALIGNMENT);

//   if (Address >= MmUserProbeAddress)
//     RtlRaiseStatus(STATUS_ACCESS_VIOLATION);

   Pointer = (PUCHAR)Address;
   Data = *Pointer;
   *Pointer = Data;
   Pointer = (PUCHAR)((ULONG)Address + Length -1);
   Data = *Pointer;
   *Pointer = Data;
}

/* EOF */
