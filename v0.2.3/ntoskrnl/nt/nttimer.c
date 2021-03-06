/* $Id: nttimer.c,v 1.23 2003/12/30 18:52:05 fireball Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/nttimer.c
 * PURPOSE:         User-mode timers
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <ntos/synch.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <limits.h>
#include <internal/pool.h>
#include <internal/safe.h>

#include <internal/debug.h>


/* TYPES ********************************************************************/

typedef struct _NTTIMER
{
   KTIMER Timer;
   KDPC Dpc;
   KAPC Apc;
   BOOLEAN Running;
} NTTIMER, *PNTTIMER;


/* GLOBALS ******************************************************************/

POBJECT_TYPE ExTimerType = NULL;

static GENERIC_MAPPING ExpTimerMapping = {
	STANDARD_RIGHTS_READ | TIMER_QUERY_STATE,
	STANDARD_RIGHTS_WRITE | TIMER_MODIFY_STATE,
	STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
	TIMER_ALL_ACCESS};


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
NtpCreateTimer(PVOID ObjectBody,
	       PVOID Parent,
	       PWSTR RemainingPath,
	       POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("NtpCreateTimer(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody, Parent, RemainingPath);

  if (RemainingPath != NULL && wcschr(RemainingPath+1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  return(STATUS_SUCCESS);
}


VOID STDCALL
NtpDeleteTimer(PVOID ObjectBody)
{
   KIRQL OldIrql;
   PNTTIMER Timer = ObjectBody;

   DPRINT("NtpDeleteTimer()\n");

   OldIrql = KeRaiseIrqlToDpcLevel();

   KeCancelTimer(&Timer->Timer);
   KeRemoveQueueDpc(&Timer->Dpc);
   KeRemoveQueueApc(&Timer->Apc);
   Timer->Running = FALSE;

   KeLowerIrql(OldIrql);
}


VOID STDCALL
NtpTimerDpcRoutine(PKDPC Dpc,
		   PVOID DeferredContext,
		   PVOID SystemArgument1,
		   PVOID SystemArgument2)
{
   PNTTIMER Timer;

   DPRINT("NtpTimerDpcRoutine()\n");

   Timer = (PNTTIMER)DeferredContext;

   if ( Timer->Running )
     {
       KeInsertQueueApc(&Timer->Apc,
			SystemArgument1,
			SystemArgument2,
			IO_NO_INCREMENT);
     }
}


VOID STDCALL
NtpTimerApcKernelRoutine(PKAPC Apc,
			 PKNORMAL_ROUTINE* NormalRoutine,
			 PVOID* NormalContext,
			 PVOID* SystemArgument1,
			 PVOID* SystemArguemnt2)
{
   DPRINT("NtpTimerApcKernelRoutine()\n");

}


VOID INIT_FUNCTION
NtInitializeTimerImplementation(VOID)
{
   assert(!ExTimerType)
   ExTimerType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

   RtlCreateUnicodeString(&ExTimerType->TypeName, L"Timer");

   ExTimerType->Tag = TAG('T', 'I', 'M', 'T');
   ExTimerType->MaxObjects = ULONG_MAX;
   ExTimerType->MaxHandles = ULONG_MAX;
   ExTimerType->TotalObjects = 0;
   ExTimerType->TotalHandles = 0;
   ExTimerType->PagedPoolCharge = 0;
   ExTimerType->NonpagedPoolCharge = sizeof(NTTIMER);
   ExTimerType->Mapping = &ExpTimerMapping;
   ExTimerType->Dump = NULL;
   ExTimerType->Open = NULL;
   ExTimerType->Close = NULL;
   ExTimerType->Delete = NtpDeleteTimer;
   ExTimerType->Parse = NULL;
   ExTimerType->Security = NULL;
   ExTimerType->QueryName = NULL;
   ExTimerType->OkayToClose = NULL;
   ExTimerType->Create = NtpCreateTimer;
   ExTimerType->DuplicationNotify = NULL;

   ObpCreateTypeObject(ExTimerType);
}


NTSTATUS STDCALL
NtCancelTimer(IN HANDLE TimerHandle,
	      OUT PBOOLEAN CurrentState OPTIONAL)
{
   PNTTIMER Timer;
   NTSTATUS Status;
   BOOLEAN State;
   KIRQL OldIrql;

   DPRINT("NtCancelTimer()\n");
   Status = ObReferenceObjectByHandle(TimerHandle,
				      TIMER_ALL_ACCESS,
				      ExTimerType,
				      UserMode,
				      (PVOID*)&Timer,
				      NULL);
   if (!NT_SUCCESS(Status))
     return Status;

   OldIrql = KeRaiseIrqlToDpcLevel();

   State = KeCancelTimer(&Timer->Timer);
   KeRemoveQueueDpc(&Timer->Dpc);
   KeRemoveQueueApc(&Timer->Apc);
   Timer->Running = FALSE;

   KeLowerIrql(OldIrql);
   ObDereferenceObject(Timer);

   if (CurrentState != NULL)
     {
	*CurrentState = State;
     }

   return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtCreateTimer(OUT PHANDLE TimerHandle,
	      IN ACCESS_MASK DesiredAccess,
	      IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	      IN TIMER_TYPE TimerType)
{
   PNTTIMER Timer;
   NTSTATUS Status;

   DPRINT("NtCreateTimer()\n");
   Status = ObCreateObject(ExGetPreviousMode(),
			   ExTimerType,
			   ObjectAttributes,
			   ExGetPreviousMode(),
			   NULL,
			   sizeof(NTTIMER),
			   0,
			   0,
			   (PVOID*)&Timer);
   if (!NT_SUCCESS(Status))
     return Status;

   KeInitializeTimerEx(&Timer->Timer,
		       TimerType);

   KeInitializeDpc(&Timer->Dpc,
		   &NtpTimerDpcRoutine,
		   Timer);

   Timer->Running = FALSE;

   Status = ObInsertObject ((PVOID)Timer,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    TimerHandle);

   ObDereferenceObject(Timer);

   return Status;
}


NTSTATUS STDCALL
NtOpenTimer(OUT PHANDLE TimerHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;

   Status = ObOpenObjectByName(ObjectAttributes,
			       ExTimerType,
			       NULL,
			       UserMode,
			       DesiredAccess,
			       NULL,
			       TimerHandle);
   return Status;
}


NTSTATUS STDCALL
NtQueryTimer(IN HANDLE TimerHandle,
	     IN CINT TimerInformationClass,
	     OUT PVOID UnsafeTimerInformation,
	     IN ULONG Length,
	     OUT PULONG UnsafeResultLength)
{
  PNTTIMER Timer;
  TIMER_BASIC_INFORMATION TimerInformation;
  ULONG ResultLength;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle(TimerHandle,
				     TIMER_QUERY_STATE,
				     ExTimerType,
				     (KPROCESSOR_MODE)KeGetPreviousMode(),
				     (PVOID*)&Timer,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  if (TimerInformationClass != TimerBasicInformation)
    {
      ObDereferenceObject(Timer);
      return(STATUS_INVALID_INFO_CLASS);
    }
  if (Length < sizeof(TIMER_BASIC_INFORMATION))
    {
      ObDereferenceObject(Timer);
      return(STATUS_INFO_LENGTH_MISMATCH);
    }

  memcpy(&TimerInformation.TimeRemaining, &Timer->Timer.DueTime,
	 sizeof(LARGE_INTEGER));
  TimerInformation.SignalState = (BOOLEAN)Timer->Timer.Header.SignalState;
  ResultLength = sizeof(TIMER_BASIC_INFORMATION);

  Status = MmCopyToCaller(UnsafeTimerInformation, &TimerInformation,
			  sizeof(TIMER_BASIC_INFORMATION));
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(Timer);
      return(Status);
    }

  if (UnsafeResultLength != NULL)
    {
      Status = MmCopyToCaller(UnsafeResultLength, &ResultLength,
			      sizeof(ULONG));
      if (!NT_SUCCESS(Status))
	{
	  ObDereferenceObject(Timer);
	  return(Status);
	}
    }
  ObDereferenceObject(Timer);
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSetTimer(IN HANDLE TimerHandle,
	   IN PLARGE_INTEGER DueTime,
	   IN PTIMERAPCROUTINE TimerApcRoutine,
	   IN PVOID TimerContext,
	   IN BOOL WakeTimer,
	   IN ULONG Period OPTIONAL,
	   OUT PBOOLEAN PreviousState OPTIONAL)
{
   PNTTIMER Timer;
   NTSTATUS Status;
   BOOLEAN Result;
   BOOLEAN State;

   DPRINT("NtSetTimer()\n");

   Status = ObReferenceObjectByHandle(TimerHandle,
				      TIMER_ALL_ACCESS,
				      ExTimerType,
				      (KPROCESSOR_MODE)KeGetPreviousMode(),
				      (PVOID*)&Timer,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
       return Status;
     }

   State = KeReadStateTimer(&Timer->Timer);

   if (Timer->Running == TRUE)
     {
	/* cancel running timer */
	const KIRQL OldIrql = KeRaiseIrqlToDpcLevel();
	KeCancelTimer(&Timer->Timer);
	KeRemoveQueueDpc(&Timer->Dpc);
	KeRemoveQueueApc(&Timer->Apc);
	Timer->Running = FALSE;
	KeLowerIrql(OldIrql);
     }

   if (TimerApcRoutine)
     {
       KeInitializeApc(&Timer->Apc,
		       KeGetCurrentThread(),
		       OriginalApcEnvironment,
		       &NtpTimerApcKernelRoutine,
		       (PKRUNDOWN_ROUTINE)NULL,
		       (PKNORMAL_ROUTINE)TimerApcRoutine,
		       (KPROCESSOR_MODE)KeGetPreviousMode(),
		       TimerContext);
     }

   Result = KeSetTimerEx(&Timer->Timer,
			 *DueTime,
			 Period,
			 TimerApcRoutine ? &Timer->Dpc : 0 );
   if (Result == TRUE)
     {
	ObDereferenceObject(Timer);
	DPRINT1( "KeSetTimer says the timer was already running, this shouldn't be\n" );
	return STATUS_UNSUCCESSFUL;
     }

   Timer->Running = TRUE;

   ObDereferenceObject(Timer);

   if (PreviousState != NULL)
     {
	*PreviousState = State;
     }

   return STATUS_SUCCESS;
}

/* EOF */
