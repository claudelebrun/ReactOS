/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/*
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/i386/exp.c
 * PURPOSE:              Handling exceptions
 * PROGRAMMERS:          David Welch (welch@cwcom.net)
 *                       Skywing (skywing@valhallalegends.com)
 * REVISION HISTORY:
 *              ??/??/??: Created
 *              09/12/03: KeRaiseUserException added (Skywing).
 */

/* INCLUDES *****************************************************************/

#include <roscfg.h>
#include <ddk/ntddk.h>
#include <internal/ntoskrnl.h>
#include <internal/ke.h>
#include <internal/i386/segment.h>
#include <internal/i386/mm.h>
#include <internal/module.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/trap.h>
#include <ntdll/ldr.h>
#include <internal/safe.h>
#include <internal/kd.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

#define FLAG_IF (1<<9)

#define _STR(x) #x
#define STR(x) _STR(x)

extern void interrupt_handler2e(void);
extern void interrupt_handler2d(void);

extern VOID KiTrap0(VOID);
extern VOID KiTrap1(VOID);
extern VOID KiTrap2(VOID);
extern VOID KiTrap3(VOID);
extern VOID KiTrap4(VOID);
extern VOID KiTrap5(VOID);
extern VOID KiTrap6(VOID);
extern VOID KiTrap7(VOID);
extern VOID KiTrap8(VOID);
extern VOID KiTrap9(VOID);
extern VOID KiTrap10(VOID);
extern VOID KiTrap11(VOID);
extern VOID KiTrap12(VOID);
extern VOID KiTrap13(VOID);
extern VOID KiTrap14(VOID);
extern VOID KiTrap15(VOID);
extern VOID KiTrap16(VOID);
extern VOID KiTrapUnknown(VOID);

extern ULONG init_stack;
extern ULONG init_stack_top;

static char *ExceptionTypeStrings[] = 
  {
    "Divide Error",
    "Debug Trap",
    "NMI",
    "Breakpoint",
    "Overflow",
    "BOUND range exceeded",
    "Invalid Opcode",
    "No Math Coprocessor",
    "Double Fault",
    "Unknown(9)",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection",
    "Page Fault",
    "Math Fault",
    "Alignment Check",
    "Machine Check"
  };

static NTSTATUS ExceptionToNtStatus[] = 
  {
    STATUS_INTEGER_DIVIDE_BY_ZERO,
    STATUS_SINGLE_STEP,
    STATUS_ACCESS_VIOLATION,
    STATUS_BREAKPOINT,
    STATUS_INTEGER_OVERFLOW,
    STATUS_ARRAY_BOUNDS_EXCEEDED,
    STATUS_ILLEGAL_INSTRUCTION,
    STATUS_ACCESS_VIOLATION, /* STATUS_FLT_INVALID_OPERATION */
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_STACK_OVERFLOW,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION,
    STATUS_ACCESS_VIOLATION, /* STATUS_FLT_INVALID_OPERATION */
    STATUS_DATATYPE_MISALIGNMENT,
    STATUS_ACCESS_VIOLATION
  };

/* FUNCTIONS ****************************************************************/

#ifdef KDBG
BOOLEAN STDCALL
KeRosPrintAddress(PVOID address)
{
  KdbPrintAddress(address);
  return TRUE;
}
#else /* KDBG */
BOOLEAN STDCALL
KeRosPrintAddress(PVOID address)
{
   PLIST_ENTRY current_entry;
   MODULE_TEXT_SECTION* current;
   extern LIST_ENTRY ModuleTextListHead;
   ULONG_PTR RelativeAddress;

   current_entry = ModuleTextListHead.Flink;
   
   while (current_entry != &ModuleTextListHead &&
	  current_entry != NULL)
     {
	current = 
	  CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);

	if (address >= (PVOID)current->Base &&
	    address < (PVOID)(current->Base + current->Length))
	  {
            RelativeAddress = (ULONG_PTR) address - current->Base;
	    DbgPrint("<%ws: %x>", current->Name, RelativeAddress);
	    return(TRUE);
	  }
	current_entry = current_entry->Flink;
     }
   return(FALSE);
}
#endif /* KDBG */

ULONG
KiKernelTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr, PVOID Cr2)
{
  EXCEPTION_RECORD Er;

  Er.ExceptionFlags = 0;
  Er.ExceptionRecord = NULL;
  Er.ExceptionAddress = (PVOID)Tf->Eip;

  if (ExceptionNr == 14)
    {
      Er.ExceptionCode = STATUS_ACCESS_VIOLATION;
      Er.NumberParameters = 2;
      Er.ExceptionInformation[0] = Tf->ErrorCode & 0x1;
      Er.ExceptionInformation[1] = (ULONG)Cr2;
    }
  else
    {
      if (ExceptionNr < 16)
	{
	  Er.ExceptionCode = ExceptionToNtStatus[ExceptionNr];
	}
      else
	{
	  Er.ExceptionCode = STATUS_ACCESS_VIOLATION;
	}
      Er.NumberParameters = 0;
    }

  Er.ExceptionFlags = ((NTSTATUS) STATUS_SINGLE_STEP == (NTSTATUS) Er.ExceptionCode
    || (NTSTATUS) STATUS_BREAKPOINT == (NTSTATUS) Er.ExceptionCode) ?
    EXCEPTION_NONCONTINUABLE : 0;

  KiDispatchException(&Er, 0, Tf, KernelMode, TRUE);

  return(0);
}

ULONG
KiDoubleFaultHandler(VOID)
{
  unsigned int cr2_;
  ULONG StackLimit;
  ULONG StackBase;
  ULONG Esp0;
  ULONG ExceptionNr = 8;
  KTSS* OldTss;
  PULONG Frame;
  ULONG OldCr3;
#if 0
  ULONG i, j;
  static PVOID StackTrace[MM_STACK_SIZE / sizeof(PVOID)];
  static ULONG StackRepeatCount[MM_STACK_SIZE / sizeof(PVOID)];
  static ULONG StackRepeatLength[MM_STACK_SIZE / sizeof(PVOID)];
  ULONG TraceLength;
  BOOLEAN FoundRepeat;
#endif
  
  OldTss = KeGetCurrentKPCR()->TSS;
  Esp0 = OldTss->Esp;

  /* Get CR2 */
#if defined(__GNUC__)
  __asm__("movl %%cr2,%0\n\t" : "=d" (cr2_));
#elif defined(_MSC_VER)
  __asm mov eax, cr2;
  __asm mov cr2_, eax;
#else
#error Unknown compiler for inline assembler
#endif

  if (PsGetCurrentThread() != NULL &&
      PsGetCurrentThread()->ThreadsProcess != NULL)
    {
      OldCr3 = (ULONG)
	PsGetCurrentThread()->ThreadsProcess->Pcb.DirectoryTableBase.QuadPart;
    }
  else
    {
      OldCr3 = 0xBEADF0AL;
    }
   
   /*
    * Check for stack underflow
    */
   if (PsGetCurrentThread() != NULL &&
       Esp0 < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DbgPrint("Stack underflow (tf->esp %x Limit %x)\n",
		 Esp0, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	ExceptionNr = 12;
     }
   
   /*
    * Print out the CPU registers
    */
   if (ExceptionNr < 19)
     {
       DbgPrint("%s Exception: %d(%x)\n", ExceptionTypeStrings[ExceptionNr],
		ExceptionNr, 0);
     }
   else
     {
       DbgPrint("Exception: %d(%x)\n", ExceptionNr, 0);
     }
   DbgPrint("CS:EIP %x:%x ", OldTss->Cs, OldTss->Eip);
   KeRosPrintAddress((PVOID)OldTss->Eip);
   DbgPrint("\n");
   DbgPrint("cr2 %x cr3 %x ", cr2_, OldCr3);
   DbgPrint("Proc: %x ",PsGetCurrentProcess());
   if (PsGetCurrentProcess() != NULL)
     {
	DbgPrint("Pid: %x <", PsGetCurrentProcess()->UniqueProcessId);
	DbgPrint("%.8s> ", PsGetCurrentProcess()->ImageFileName);
     }
   if (PsGetCurrentThread() != NULL)
     {
	DbgPrint("Thrd: %x Tid: %x",
		 PsGetCurrentThread(),
		 PsGetCurrentThread()->Cid.UniqueThread);
     }
   DbgPrint("\n");
   DbgPrint("DS %x ES %x FS %x GS %x\n", OldTss->Ds, OldTss->Es,
	    OldTss->Fs, OldTss->Gs);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", OldTss->Eax, OldTss->Ebx, 
	    OldTss->Ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x\n   ESP: %.8x", OldTss->Edx, 
	    OldTss->Ebp, OldTss->Esi, Esp0);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x ", OldTss->Edi, OldTss->Eflags);
   if (OldTss->Cs == KERNEL_CS)
     {
	DbgPrint("kESP %.8x ", Esp0);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.StackLimit);
		      	     
	  }
     }
   else
     {
	DbgPrint("User ESP %.8x\n", OldTss->Esp);
     }
  if ((OldTss->Cs & 0xffff) == KERNEL_CS)
    {
      if (PsGetCurrentThread() != NULL)
	{
	  StackLimit = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
	  StackBase = (ULONG)PsGetCurrentThread()->Tcb.StackLimit;
	}
      else
	{
	  StackLimit = (ULONG)&init_stack_top;
	  StackBase = (ULONG)&init_stack;
	}

      /* 
	 Change to an #if 0 to reduce the amount of information printed on
	 a recursive stack trace. 
      */      
#if 1
      DbgPrint("Frames: ");
      Frame = (PULONG)OldTss->Ebp;
      while (Frame != NULL && (ULONG)Frame >= StackBase)
	{
	  KeRosPrintAddress((PVOID)Frame[1]);
	  Frame = (PULONG)Frame[0];
          DbgPrint(" ");
	}
#else
      DbgPrint("Frames: ");
      i = 0;
      Frame = (PULONG)OldTss->Ebp;
      while (Frame != NULL && (ULONG)Frame >= StackBase)
	{
	  StackTrace[i] = (PVOID)Frame[1];
	  Frame = (PULONG)Frame[0];
	  i++;
	}
      TraceLength = i;

      i = 0;
      while (i < TraceLength)
	{
	  StackRepeatCount[i] = 0;
	  j = i + 1;
	  FoundRepeat = FALSE;
	  while ((j - i) <= (TraceLength - j) && FoundRepeat == FALSE)
	    {
	      if (memcmp(&StackTrace[i], &StackTrace[j], 
			 (j - i) * sizeof(PVOID)) == 0)
		{
		  StackRepeatCount[i] = 2;
		  StackRepeatLength[i] = j - i;
		  FoundRepeat = TRUE;
		}
	      else
		{
		  j++;
		}
	    }
	  if (FoundRepeat == FALSE)
	    {
	      i++;
	      continue;
	    }
	  j = j + StackRepeatLength[i];
	  while ((TraceLength - j) >= StackRepeatLength[i] && 
		 FoundRepeat == TRUE)
	    {
	      if (memcmp(&StackTrace[i], &StackTrace[j], 
			 StackRepeatLength[i] * sizeof(PVOID)) == 0)
		{
		  StackRepeatCount[i]++;
		  j = j + StackRepeatLength[i];
		}
	      else
		{
		  FoundRepeat = FALSE;
		}
	    }
	  i = j;
	}

      i = 0;
      while (i < TraceLength)
	{
	  if (StackRepeatCount[i] == 0)
	    {
	      KeRosPrintAddress(StackTrace[i]);
	      i++;
	    }
	  else
	    {
	      DbgPrint("{");
	      if (StackRepeatLength[i] == 0)
		{
		  for(;;);
		}
	      for (j = 0; j < StackRepeatLength[i]; j++)
		{
		  KeRosPrintAddress(StackTrace[i + j]);
		}
	      DbgPrint("}*%d", StackRepeatCount[i]);
	      i = i + StackRepeatLength[i] * StackRepeatCount[i];
	    }
	}
#endif
    }
   
   DbgPrint("\n");
   for(;;);
   return 0;
}

VOID
KiDumpTrapFrame(PKTRAP_FRAME Tf, ULONG Parameter1, ULONG Parameter2)
{
  ULONG cr3_;
  ULONG i;
  ULONG StackLimit;
  PULONG Frame;
  ULONG Esp0;
  ULONG ExceptionNr = (ULONG)Tf->DebugArgMark;
  ULONG cr2 = (ULONG)Tf->DebugPointer;

  Esp0 = (ULONG)Tf;
  
   /*
    * Print out the CPU registers
    */
   if (ExceptionNr < 19)
     {
	DbgPrint("%s Exception: %d(%x)\n", ExceptionTypeStrings[ExceptionNr],
		 ExceptionNr, Tf->ErrorCode&0xffff);
     }
   else
     {
	DbgPrint("Exception: %d(%x)\n", ExceptionNr, Tf->ErrorCode&0xffff);
     }
   DbgPrint("Processor: %d CS:EIP %x:%x ", KeGetCurrentProcessorNumber(),
	    Tf->Cs&0xffff, Tf->Eip);
   KeRosPrintAddress((PVOID)Tf->Eip);
   DbgPrint("\n");
   Ke386GetPageTableDirectory(cr3_);
   DbgPrint("cr2 %x cr3 %x ", cr2, cr3_);
   DbgPrint("Proc: %x ",PsGetCurrentProcess());
   if (PsGetCurrentProcess() != NULL)
     {
	DbgPrint("Pid: %x <", PsGetCurrentProcess()->UniqueProcessId);
	DbgPrint("%.8s> ", PsGetCurrentProcess()->ImageFileName);
     }
   if (PsGetCurrentThread() != NULL)
     {
	DbgPrint("Thrd: %x Tid: %x",
		 PsGetCurrentThread(),
		 PsGetCurrentThread()->Cid.UniqueThread);
     }
   DbgPrint("\n");
   DbgPrint("DS %x ES %x FS %x GS %x\n", Tf->Ds&0xffff, Tf->Es&0xffff,
	    Tf->Fs&0xffff, Tf->Gs&0xfff);
   DbgPrint("EAX: %.8x   EBX: %.8x   ECX: %.8x\n", Tf->Eax, Tf->Ebx, Tf->Ecx);
   DbgPrint("EDX: %.8x   EBP: %.8x   ESI: %.8x   ESP: %.8x\n", Tf->Edx, 
	    Tf->Ebp, Tf->Esi, Esp0);
   DbgPrint("EDI: %.8x   EFLAGS: %.8x ", Tf->Edi, Tf->Eflags);
   if ((Tf->Cs&0xffff) == KERNEL_CS)
     {
	DbgPrint("kESP %.8x ", Esp0);
	if (PsGetCurrentThread() != NULL)
	  {
	     DbgPrint("kernel stack base %x\n",
		      PsGetCurrentThread()->Tcb.StackLimit);
		      	     
	  }
     }

   if (PsGetCurrentThread() != NULL)
     {
       StackLimit = (ULONG)PsGetCurrentThread()->Tcb.StackBase;
     }
   else
     {
       StackLimit = (ULONG)&init_stack_top;
     }
   
   /*
    * Dump the stack frames
    */
   DbgPrint("Frames: ");
   /* Change to an #if 0 if no frames are printed because of fpo. */
#if 1
   i = 1;
   Frame = (PULONG)Tf->Ebp;
   while (Frame != NULL)
     {
       NTSTATUS Status;
       PVOID Eip;
       Status = MmSafeCopyFromUser(&Eip, Frame + 1, sizeof(Eip));
       if (!NT_SUCCESS(Status))
	 {
	   DbgPrint("<INVALID>");
	   break;
	 }
       if (!KeRosPrintAddress(Eip))
	 {
	   DbgPrint("<%X>", Eip);
	   break;
	 }
       Status = MmSafeCopyFromUser(&Frame, Frame, sizeof(Frame));
       if (!NT_SUCCESS(Status))
	 {
	   break;
	 }
       i++;
       DbgPrint(" ");
     }
#else
   i = 1;
   Frame = (PULONG)((ULONG_PTR)Esp0 + KTRAP_FRAME_EFLAGS);
   while (Frame < (PULONG)PsGetCurrentThread()->Tcb.StackBase && i < 50)
     {
	 ULONG Address = *Frame;
	 if (KeRosPrintAddress((PVOID)Address))
	   {
	     i++;
	   }
	 Frame++;
     }
#endif
}

ULONG
KiTrapHandler(PKTRAP_FRAME Tf, ULONG ExceptionNr)
/*
 * FUNCTION: Called by the lowlevel execption handlers to print an amusing 
 * message and halt the computer
 * ARGUMENTS:
 *        Complete CPU context
 */
{
   unsigned int cr2_;
   NTSTATUS Status;
   ULONG Esp0;

   /* Store the exception number in an unused field in the trap frame. */
   Tf->DebugArgMark = (PVOID)ExceptionNr;

   /* Use the address of the trap frame as approximation to the ring0 esp */
   Esp0 = (ULONG)&Tf->Eip;
  
   /* Get CR2 */
#if defined(__GNUC__)
   __asm__("movl %%cr2,%0\n\t" : "=d" (cr2_));
#elif defined(_MSC_VER)
  __asm mov eax, cr2;
  __asm mov cr2_, eax;
#else
#error Unknown compiler for inline assembler
#endif
   Tf->DebugPointer = (PVOID)cr2_;
   
   /*
    * If this was a V86 mode exception then handle it specially
    */
   if (Tf->Eflags & (1 << 17))
     {
       return(KeV86Exception(ExceptionNr, Tf, cr2_));
     }

   /*
    * Check for stack underflow, this may be obsolete
    */
   if (PsGetCurrentThread() != NULL &&
       Esp0 < (ULONG)PsGetCurrentThread()->Tcb.StackLimit)
     {
	DbgPrint("Stack underflow (tf->esp %x Limit %x)\n",
		 Esp0, (ULONG)PsGetCurrentThread()->Tcb.StackLimit);
	ExceptionNr = 12;
     }

   /*
    * Maybe handle the page fault and return
    */
   if (ExceptionNr == 14)
     {
        if (Tf->Eflags & FLAG_IF)
	{
	  Ke386EnableInterrupts();
	}
	Status = MmPageFault(Tf->Cs&0xffff,
			     &Tf->Eip,
			     &Tf->Eax,
			     cr2_,
			     Tf->ErrorCode);
	if (NT_SUCCESS(Status))
	  {
	     return(0);
	  }
     }

   /*
    * Check for a breakpoint that was only for the attention of the debugger.
    */
   if (ExceptionNr == 3 && Tf->Eip == ((ULONG)DbgBreakPointNoBugCheck) + 1)
     {
       /* 
	  EIP is already adjusted by the processor to point to the instruction
	  after the breakpoint.
       */
       return(0);
     }

   /*
    * Handle user exceptions differently
    */
   if ((Tf->Cs & 0xFFFF) == USER_CS)
     {
       return(KiUserTrapHandler(Tf, ExceptionNr, (PVOID)cr2_));
     }
   else
    {
      return(KiKernelTrapHandler(Tf, ExceptionNr, (PVOID)cr2_));
    }
}

VOID 
KeDumpStackFrames(PULONG Frame)
{
	ULONG i;

	DbgPrint("Frames: ");
	i = 1;
	while ( MmIsAddressValid(Frame) )
	{
		if (!KeRosPrintAddress((PVOID)Frame[1]))
		{
			DbgPrint("<%X>", (PVOID)Frame[1]);
		}
		Frame = (PULONG)Frame[0];
		i++;
		DbgPrint(" ");
	}
}

VOID STDCALL
KeRosDumpStackFrames ( PULONG Frame, ULONG FrameCount )
{
	ULONG i;

	DbgPrint("Frames: ");
	if ( !Frame )
	{
		__asm__("mov %%ebp, %%ebx" : "=b" (Frame) : );
		Frame = (PULONG)Frame[0]; // step out of KeRosDumpStackFrames
	}
	i = 1;
	while ( MmIsAddressValid(Frame) && i++ < FrameCount )
	{
		if (!KeRosPrintAddress((PVOID)Frame[1]))
		{
			DbgPrint("<%X>", (PVOID)Frame[1]);
		}
		Frame = (PULONG)Frame[0];
		DbgPrint(" ");
	}
}

static void set_system_call_gate(unsigned int sel, unsigned int func)
{
   DPRINT("sel %x %d\n",sel,sel);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0xef00 + (((int)func)&0xffff0000);
   DPRINT("idt[sel].b %x\n",KiIdt[sel].b);
}

static void set_interrupt_gate(unsigned int sel, unsigned int func)
{
   DPRINT("set_interrupt_gate(sel %d, func %x)\n",sel,func);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0x8e00 + (((int)func)&0xffff0000);         
}

static void set_trap_gate(unsigned int sel, unsigned int func, unsigned int dpl)
{
   DPRINT("set_trap_gate(sel %d, func %x, dpl %d)\n",sel, func, dpl);
   assert(dpl <= 3);
   KiIdt[sel].a = (((int)func)&0xffff) +
     (KERNEL_CS << 16);
   KiIdt[sel].b = 0x8f00 + (dpl << 13) + (((int)func)&0xffff0000);         
}

static void
set_task_gate(unsigned int sel, unsigned task_sel)
{
  KiIdt[sel].a = task_sel << 16;
  KiIdt[sel].b = 0x8500;
}

VOID INIT_FUNCTION
KeInitExceptions(VOID)
/*
 * FUNCTION: Initalize CPU exception handling
 */
{
   int i;

   DPRINT("KeInitExceptions()\n");

   /*
    * Set up the other gates
    */
   set_trap_gate(0, (ULONG)KiTrap0, 0);
   set_trap_gate(1, (ULONG)KiTrap1, 0);
   set_trap_gate(2, (ULONG)KiTrap2, 0);
   set_trap_gate(3, (ULONG)KiTrap3, 3);
   set_trap_gate(4, (ULONG)KiTrap4, 0);
   set_trap_gate(5, (ULONG)KiTrap5, 0);
   set_trap_gate(6, (ULONG)KiTrap6, 0);
   set_trap_gate(7, (ULONG)KiTrap7, 0);
   set_task_gate(8, TRAP_TSS_SELECTOR);
   set_trap_gate(9, (ULONG)KiTrap9, 0);
   set_trap_gate(10, (ULONG)KiTrap10, 0);
   set_trap_gate(11, (ULONG)KiTrap11, 0);
   set_trap_gate(12, (ULONG)KiTrap12, 0);
   set_trap_gate(13, (ULONG)KiTrap13, 0);
   set_interrupt_gate(14, (ULONG)KiTrap14);
   set_trap_gate(15, (ULONG)KiTrap15, 0);
   set_trap_gate(16, (ULONG)KiTrap16, 0);
   
   for (i=17;i<256;i++)
        {
	   set_trap_gate(i,(int)KiTrapUnknown, 0);
        }
   
   set_system_call_gate(0x2d,(int)interrupt_handler2d);
   set_system_call_gate(0x2e,(int)interrupt_handler2e);
}

/*
 * @implemented
 */

VOID STDCALL
KeRaiseUserException(IN NTSTATUS ExceptionCode)
{
   /* FIXME: This needs SEH */

   PKTHREAD Thread = KeGetCurrentThread();

   ProbeForWrite(&Thread->Teb->ExceptionCode, sizeof(NTSTATUS), sizeof(NTSTATUS)); /* NT doesn't check this -- bad? */
   Thread->TrapFrame->Eip = (ULONG_PTR)LdrpGetSystemDllRaiseExceptionDispatcher();
   Thread->Teb->ExceptionCode = ExceptionCode;
}
