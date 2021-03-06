#ifndef __NTOSKRNL_INCLUDE_INTERNAL_SAFE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_SAFE_H

NTSTATUS MmSafeCopyFromUser(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);
NTSTATUS MmSafeCopyToUser(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);

NTSTATUS STDCALL
MmCopyFromCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);
NTSTATUS STDCALL
MmCopyToCaller(PVOID Dest, const VOID *Src, ULONG NumberOfBytes);

NTSTATUS
RtlCaptureUnicodeString(PUNICODE_STRING Dest,
			PUNICODE_STRING UnsafeSrc);

#endif /* __NTOSKRNL_INCLUDE_INTERNAL_SAFE_Hb */
