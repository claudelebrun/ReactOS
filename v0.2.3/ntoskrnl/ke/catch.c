/*
 *  ReactOS kernel
 *  Copyright (C) 2000  ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: catch.c,v 1.42 2004/05/07 05:12:10 royce Exp $
 *
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/catch.c
 * PURPOSE:              Exception handling
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 *                       Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <reactos/bugcodes.h>
#include <roscfg.h>
#include <internal/ke.h>
#include <internal/ldr.h>
#include <internal/ps.h>
#include <internal/kd.h>
#include <internal/safe.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

ULONG
RtlpDispatchException(IN PEXCEPTION_RECORD  ExceptionRecord,
	IN PCONTEXT  Context);

VOID 
KiDispatchException(PEXCEPTION_RECORD ExceptionRecord,
		    PCONTEXT Context,
		    PKTRAP_FRAME Tf,
		    KPROCESSOR_MODE PreviousMode,
		    BOOLEAN SearchFrames)
{
  EXCEPTION_DISPOSITION Value;
  CONTEXT TContext;
  KD_CONTINUE_TYPE Action = kdContinue;

  DPRINT("KiDispatchException() called\n");

  /* PCR->KeExceptionDispatchCount++; */

  if (Context == NULL)
    {
      TContext.ContextFlags = CONTEXT_FULL;
      if (PreviousMode == UserMode)
	{
	  TContext.ContextFlags = TContext.ContextFlags | CONTEXT_DEBUGGER;
	}
  
      KeTrapFrameToContext(Tf, &TContext);

      Context = &TContext;
    }

#if 0
  if (ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) 
    {
      Context->Eip--;
    }
#endif
      
  if (KdDebuggerEnabled && KdDebugState & KD_DEBUG_GDB)
    {
      Action = KdEnterDebuggerException (ExceptionRecord, Context, Tf);
    }
#ifdef KDBG
  else if (KdDebuggerEnabled && KdDebugState & KD_DEBUG_KDB)
    {
      Action = KdbEnterDebuggerException (ExceptionRecord, Context, Tf);
      if (Action == kdContinue)
	{
	  return;
	}
    }
#endif /* KDBG */
  if (Action != kdHandleException)
    {
      if (PreviousMode == UserMode)
	{
	  if (SearchFrames)
	    {
	      PULONG Stack;
	      ULONG CDest;
	      char temp_space[12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)]; // FIXME: HACKHACK
	      PULONG pNewUserStack = (PULONG)(Tf->Esp - (12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)));
	      NTSTATUS StatusOfCopy;

	      /* FIXME: Forward exception to user mode debugger */

	      /* FIXME: Check user mode stack for enough space */
	  
	      /*
	       * Let usermode try and handle the exception
	       */
	      Stack = (PULONG)temp_space;
	      CDest = 3 + (ROUND_UP(sizeof(EXCEPTION_RECORD), 4) / 4);
	      /* Return address */
	      Stack[0] = 0;    
	      /* Pointer to EXCEPTION_RECORD structure */
	      Stack[1] = (ULONG)&pNewUserStack[3];
	      /* Pointer to CONTEXT structure */
	      Stack[2] = (ULONG)&pNewUserStack[CDest];
	      memcpy(&Stack[3], ExceptionRecord, sizeof(EXCEPTION_RECORD));
	      memcpy(&Stack[CDest], Context, sizeof(CONTEXT));

	      StatusOfCopy = MmCopyToCaller(pNewUserStack,
 	                                    temp_space,
 	                                    (12 + sizeof(EXCEPTION_RECORD) + sizeof(CONTEXT)));
	      if (NT_SUCCESS(StatusOfCopy))
	        {
	          Tf->Esp = (ULONG)pNewUserStack;
	        }
	      else
	        {
	          // Now it really hit the ventilation device. Sorry,
	          // can do nothing but kill the sucker.
	          ZwTerminateThread(NtCurrentThread(), ExceptionRecord->ExceptionCode);
	          DPRINT1("User-mode stack was invalid. Terminating target thread\nn");
	        }
	      Tf->Eip = (ULONG)LdrpGetSystemDllExceptionDispatcher();
	      return;
	    }

	  /* FIXME: Forward the exception to the debugger */

	  /* FIXME: Forward the exception to the process exception port */

	  /* Terminate the offending thread */
	  DPRINT1("Unhandled UserMode exception, terminating thread\n");
	  ZwTerminateThread(NtCurrentThread(), ExceptionRecord->ExceptionCode);

	  /* If that fails then bugcheck */
	  DPRINT1("Could not terminate thread\n");
	  KEBUGCHECK(KMODE_EXCEPTION_NOT_HANDLED);
	}
      else
	{
	  /* PreviousMode == KernelMode */
	  Value = RtlpDispatchException (ExceptionRecord, Context);
	  
	  DPRINT("RtlpDispatchException() returned with 0x%X\n", Value);
	  /* 
	   * If RtlpDispatchException() does not handle the exception then 
	   * bugcheck 
	   */
	  if (Value != ExceptionContinueExecution ||
	      0 != (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE))
	    {
	      DPRINT("ExceptionRecord->ExceptionAddress = 0x%x\n",
		     ExceptionRecord->ExceptionAddress );
              KEBUGCHECKWITHTF(KMODE_EXCEPTION_NOT_HANDLED, 0, 0, 0, 0, Tf);	      
	    }
	}
    }
  else
    {
      KeContextToTrapFrame (Context, KeGetCurrentThread()->TrapFrame);
    }
}

/*
 * @implemented
 */
VOID STDCALL
ExRaiseAccessViolation (VOID)
{
  ExRaiseStatus (STATUS_ACCESS_VIOLATION);
}

/*
 * @implemented
 */
VOID STDCALL
ExRaiseDatatypeMisalignment (VOID)
{
  ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
}

/*
 * @implemented
 */
VOID STDCALL
ExRaiseStatus (IN NTSTATUS Status)
{
  EXCEPTION_RECORD ExceptionRecord;

  DPRINT("ExRaiseStatus(%x)\n", Status);

  ExceptionRecord.ExceptionRecord = NULL;
  ExceptionRecord.NumberParameters = 0;
  ExceptionRecord.ExceptionCode = Status;
  ExceptionRecord.ExceptionFlags = 0;

  RtlRaiseException(&ExceptionRecord);
}


NTSTATUS STDCALL
NtRaiseException (IN PEXCEPTION_RECORD ExceptionRecord,
		  IN PCONTEXT Context,
		  IN BOOLEAN SearchFrames)
{
  KiDispatchException(ExceptionRecord,
		      Context,
		      PsGetCurrentThread()->Tcb.TrapFrame,
		      (KPROCESSOR_MODE)ExGetPreviousMode(),
		      SearchFrames);
  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord)
{
  ZwRaiseException(ExceptionRecord, NULL, TRUE);
}

/* EOF */
