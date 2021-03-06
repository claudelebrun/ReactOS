#ifndef _SUBSYS_WIN32K_INCLUDE_CLEANUP_H
#define _SUBSYS_WIN32K_INCLUDE_CLEANUP_H

NTSTATUS FASTCALL InitCleanupImpl(VOID);

NTSTATUS FASTCALL
IntSafeCopyUnicodeString(PUNICODE_STRING Dest,
                         PUNICODE_STRING Source);

NTSTATUS FASTCALL
IntSafeCopyUnicodeStringTerminateNULL(PUNICODE_STRING Dest,
                                      PUNICODE_STRING Source);

NTSTATUS FASTCALL
IntUnicodeStringToNULLTerminated(PWSTR *Dest, PUNICODE_STRING Src);

void FASTCALL
IntFreeNULLTerminatedFromUnicodeString(PWSTR NullTerminated, PUNICODE_STRING UnicodeString);

#endif /* ndef _SUBSYS_WIN32K_INCLUDE_CLEANUP_H */
