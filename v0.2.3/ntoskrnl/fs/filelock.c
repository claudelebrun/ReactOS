/* $Id: filelock.c,v 1.12 2003/10/12 17:05:44 hbirr Exp $
 *
 * reactos/ntoskrnl/fs/filelock.c
 *
 */
#include <ddk/ntddk.h>
#include <internal/ifs.h>
#include <ddk/ntifs.h>
#include <ntos.h>

#define NDEBUG
#include <internal/debug.h>

/*
NOTE:
I'm not using resource syncronization here, since FsRtlFastCheckLockForRead/Write
are allowed to be called at DISPATCH_LEVEL. Must therefore use nonpaged memory for
the lists.
*/

#define LOCK_START_OFF(Lock)  ((Lock).StartingByte.QuadPart)
#define LOCK_END_OFF(Lock)    (((Lock).StartingByte.QuadPart) + ((Lock).Length.QuadPart) - 1)
#define REQUEST_START_OFF     (FileOffset->QuadPart)
#define REQUEST_END_OFF       ((FileOffset->QuadPart) + (Length->QuadPart) - 1)

FAST_MUTEX              LockTocMutex;
NPAGED_LOOKASIDE_LIST   GrantedLookaside;
NPAGED_LOOKASIDE_LIST   LockTocLookaside;
PAGED_LOOKASIDE_LIST    LockLookaside;

/**********************************************************************
 * NAME							PRIVATE
 *	FsRtlpInitFileLockingImplementation
 *
 */
VOID
STDCALL INIT_FUNCTION
FsRtlpInitFileLockingImplementation(VOID)
{
   ExInitializeNPagedLookasideList( &LockTocLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FILE_LOCK_TOC),
                                    IFS_POOL_TAG,
                                    0
                                    );

   ExInitializeNPagedLookasideList( &GrantedLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FILE_LOCK_GRANTED),
                                    IFS_POOL_TAG,
                                    0
                                    );

   ExInitializePagedLookasideList(  &LockLookaside,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(FILE_LOCK),
                                    IFS_POOL_TAG,
                                    0
                                    );

   ExInitializeFastMutex(&LockTocMutex);
}

/**********************************************************************
 * NAME							PRIVATE
 *	FsRtlpFileLockCancelRoutine
 *
 */
VOID
STDCALL 
FsRtlpFileLockCancelRoutine(
   IN PDEVICE_OBJECT DeviceObject, 
   IN PIRP Irp
   )
{
   KIRQL                         oldIrql;
   PKSPIN_LOCK                   SpinLock;
   PCOMPLETE_LOCK_IRP_ROUTINE    CompleteLockIrpRoutine;

   //don't need this since we have our own sync. protecting irp cancellation
   IoReleaseCancelSpinLock(Irp->CancelIrql); 

   SpinLock = &((PFILE_LOCK_TOC)Irp->Tail.Overlay.DriverContext[1])->SpinLock;

   KeAcquireSpinLock(SpinLock, &oldIrql);
   RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
   KeReleaseSpinLock(SpinLock, oldIrql);

   Irp->IoStatus.Status = STATUS_CANCELLED;

   CompleteLockIrpRoutine = ((PFILE_LOCK)Irp->Tail.Overlay.DriverContext[0])->CompleteLockIrpRoutine;
   if (CompleteLockIrpRoutine)
   {
      CompleteLockIrpRoutine(Irp->Tail.Overlay.DriverContext[2], Irp);
   }
   else
   {
      IofCompleteRequest(Irp, IO_NO_INCREMENT);
   }
   
}

/**********************************************************************
 * NAME							PRIVATE
 *	FsRtlpCheckLockForReadOrWriteAccess
 *
 */
BOOLEAN
FASTCALL
FsRtlpCheckLockForReadOrWriteAccess(
   IN PFILE_LOCK           FileLock,
   IN PLARGE_INTEGER       FileOffset,
   IN PLARGE_INTEGER       Length,
   IN ULONG                Key,
   IN PFILE_OBJECT         FileObject,
   IN PEPROCESS            Process,
   IN BOOLEAN              Read
   )
{
   KIRQL                oldirql;
   PFILE_LOCK_TOC       LockToc;
   PFILE_LOCK_GRANTED   Granted;
   PLIST_ENTRY          EnumEntry;

   assert(FileLock);
   LockToc = FileLock->LockInformation;

   if (LockToc == NULL || Length->QuadPart == 0) 
   {
      return TRUE;
   }

   KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

   EnumEntry = LockToc->GrantedListHead.Flink;
   while ( EnumEntry != &LockToc->GrantedListHead)
   {
      Granted = CONTAINING_RECORD(EnumEntry, FILE_LOCK_GRANTED , ListEntry );
      //if overlapping
      if(!(REQUEST_START_OFF > LOCK_END_OFF(Granted->Lock) || 
         REQUEST_END_OFF < LOCK_START_OFF(Granted->Lock))) 
      {
         //No read conflict if (shared lock) OR (exclusive + our lock)
         //No write conflict if exclusive lock AND our lock
         if ((Read && !Granted->Lock.ExclusiveLock) ||
            (Granted->Lock.ExclusiveLock &&	
            Granted->Lock.Process == Process &&
            Granted->Lock.FileObject == FileObject &&
            Granted->Lock.Key == Key ) ) 
         {
            //AND if lock surround request region, stop searching and grant
            if (REQUEST_START_OFF >= LOCK_START_OFF(Granted->Lock) && 
               REQUEST_END_OFF <= LOCK_END_OFF(Granted->Lock))
            {
               EnumEntry = &LockToc->GrantedListHead;//indicate no conflict
               break;
            }
            //else continue searching for conflicts
         }
         else //conflict
         {
            break;
         }
      }
      EnumEntry = EnumEntry->Flink;
   }

   KeReleaseSpinLock(&LockToc->SpinLock, oldirql);

   if (EnumEntry == &LockToc->GrantedListHead) 
   { //no conflict
      return TRUE;
   }

   return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCheckLockForReadAccess
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlCheckLockForReadAccess (
   IN PFILE_LOCK  FileLock,
   IN PIRP        Irp
   )
{
   PIO_STACK_LOCATION   Stack;
   LARGE_INTEGER        LocalLength;

   Stack = IoGetCurrentIrpStackLocation(Irp);

   LocalLength.u.LowPart = Stack->Parameters.Read.Length;
   LocalLength.u.HighPart = 0;

   return FsRtlpCheckLockForReadOrWriteAccess(  FileLock,
                                                &Stack->Parameters.Read.ByteOffset,
                                                &LocalLength,
                                                Stack->Parameters.Read.Key,
                                                Stack->FileObject,
                                                IoGetRequestorProcess(Irp),
                                                TRUE//Read?
                                                );
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlCheckLockForWriteAccess
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlCheckLockForWriteAccess (
   IN PFILE_LOCK  FileLock,
   IN PIRP        Irp
   )
{
   PIO_STACK_LOCATION   Stack;
   LARGE_INTEGER		   LocalLength;

   Stack = IoGetCurrentIrpStackLocation(Irp);

   LocalLength.u.LowPart = Stack->Parameters.Read.Length;
   LocalLength.u.HighPart = 0;

   return FsRtlpCheckLockForReadOrWriteAccess(  FileLock,
                                                &Stack->Parameters.Write.ByteOffset,
                                                &LocalLength,
                                                Stack->Parameters.Write.Key,
                                                Stack->FileObject,
                                                IoGetRequestorProcess(Irp),
                                                FALSE//Read?
                                                );

}




/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastCheckLockForRead
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlFastCheckLockForRead (
   IN PFILE_LOCK           FileLock,
   IN PLARGE_INTEGER       FileOffset,
   IN PLARGE_INTEGER       Length,
   IN ULONG                Key,
   IN PFILE_OBJECT         FileObject,
   IN PEPROCESS            Process
   )
{
   return FsRtlpCheckLockForReadOrWriteAccess(  FileLock,
                                                FileOffset,
                                                Length,
                                                Key,
                                                FileObject,
                                                Process,
                                                TRUE//Read?
                                                );
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastCheckLockForWrite
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlFastCheckLockForWrite (
   IN PFILE_LOCK           FileLock,
   IN PLARGE_INTEGER       FileOffset,
   IN PLARGE_INTEGER       Length,
   IN ULONG                Key,
   IN PFILE_OBJECT         FileObject,
   IN PEPROCESS            Process
   )
{
   return FsRtlpCheckLockForReadOrWriteAccess(  FileLock,
                                                FileOffset,
                                                Length,
                                                Key,
                                                FileObject,
                                                Process,
                                                FALSE//Read?
                                                );
}



/**********************************************************************
 * NAME							PRIVATE
 *	FsRtlpFastUnlockAllByKey
 *
 */
NTSTATUS
FASTCALL
FsRtlpFastUnlockAllByKey(
   IN PFILE_LOCK           FileLock,
   IN PFILE_OBJECT         FileObject,
   IN PEPROCESS            Process,
   IN DWORD                Key,      /* FIXME: guess */
   IN BOOLEAN              UseKey,   /* FIXME: guess */
   IN PVOID                Context OPTIONAL
   )
{
   KIRQL				      oldirql;
   PFILE_LOCK_TOC		   LockToc;
   PLIST_ENTRY			   EnumEntry;
   PFILE_LOCK_GRANTED	Granted;
   BOOLEAN				   Unlock = FALSE;
   //must make local copy since FILE_LOCK struct is allowed to be paged
   PUNLOCK_ROUTINE		GotUnlockRoutine;

   assert(FileLock);
   LockToc = FileLock->LockInformation;

   if (LockToc == NULL)
   {
      return STATUS_RANGE_NOT_LOCKED;
   }

   GotUnlockRoutine = FileLock->UnlockRoutine;
   KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

   EnumEntry = LockToc->GrantedListHead.Flink;
   while (EnumEntry != &LockToc->GrantedListHead ) 
   {
      Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED, ListEntry);
      EnumEntry = EnumEntry->Flink;

      if (Granted->Lock.Process == Process &&
         Granted->Lock.FileObject == FileObject &&
         (!UseKey || (UseKey && Granted->Lock.Key == Key)) )
      {
         RemoveEntryList(&Granted->ListEntry);
         Unlock = TRUE;

         if (GotUnlockRoutine) 
         {
            /*
            Put on unlocked list and call unlock routine for them afterwards.
            This way we don't have to restart enum after each call
            */
            InsertHeadList(&LockToc->UnlockedListHead,&Granted->ListEntry);
         }
         else 
         {
            ExFreeToNPagedLookasideList(&GrantedLookaside,Granted);	
         }
      }
   }

   if (Unlock)
   {
      //call unlock routine for each unlocked lock (if any)
      while (!IsListEmpty(&LockToc->UnlockedListHead)) 
      {
         EnumEntry = RemoveTailList(&LockToc->UnlockedListHead);
         Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED, ListEntry);
         KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
         FileLock->UnlockRoutine(Context,&Granted->Lock);
         ExFreeToNPagedLookasideList(&GrantedLookaside,Granted);
         KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);
      }

      //NOTE: holding spinlock while calling this
      FsRtlpCompletePendingLocks(FileLock, LockToc, &oldirql);

      if (IsListEmpty(&LockToc->GrantedListHead)) 
      {
         KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
         FsRtlAreThereCurrentFileLocks(FileLock) = FALSE;
      }
      else
      {
         KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
      }
      return STATUS_SUCCESS;
   }

   KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
   return STATUS_RANGE_NOT_LOCKED;
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastUnlockAll
 *
 * @implemented
 */
NTSTATUS
STDCALL
FsRtlFastUnlockAll /*ByProcess*/ (
   IN PFILE_LOCK           FileLock,
   IN PFILE_OBJECT         FileObject,
   IN PEPROCESS            Process,
   IN PVOID                Context OPTIONAL
   )
{
   return FsRtlpFastUnlockAllByKey( FileLock,
                                    FileObject,
                                    Process,
                                    0,     /* Key */
                                    FALSE, /* Do NOT use Key */
                                    Context
                                    );
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastUnlockAllByKey
 *
 * @implemented
 */
NTSTATUS
STDCALL
FsRtlFastUnlockAllByKey (
   IN PFILE_LOCK           FileLock,
   IN PFILE_OBJECT         FileObject,
   IN PEPROCESS            Process,
   IN ULONG                Key,
   IN PVOID                Context OPTIONAL
   )
{
   return FsRtlpFastUnlockAllByKey( FileLock,
                                    FileObject,
                                    Process,
                                    Key,
                                    TRUE, /* Use Key */
                                    Context
                                    );
}


/**********************************************************************
 * NAME							PRIVATE
 *	FsRtlpAddLock
 *
 * NOTE
 *  Spinlock held at entry !!
 */
NTSTATUS
FASTCALL
FsRtlpAddLock(
   IN PFILE_LOCK_TOC		   LockToc,
   IN PFILE_OBJECT         FileObject,
   IN PLARGE_INTEGER       FileOffset,
   IN PLARGE_INTEGER       Length,
   IN PEPROCESS            Process,
   IN ULONG                Key,
   IN BOOLEAN              ExclusiveLock
   )
{
   PLIST_ENTRY          EnumEntry;
   PFILE_LOCK_GRANTED   Granted;

   EnumEntry = LockToc->GrantedListHead.Flink;
   while (EnumEntry != &LockToc->GrantedListHead) 
   {
      Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED, ListEntry);
      //if overlapping
      if(!(REQUEST_START_OFF > LOCK_END_OFF(Granted->Lock) || 
         REQUEST_END_OFF < LOCK_START_OFF(Granted->Lock))) 
      {
         //never conflict if shared lock and we want to add a shared lock
         if (!Granted->Lock.ExclusiveLock && !ExclusiveLock) 
         {
            //AND if lock surround region, stop searching and insert lock
            if (REQUEST_START_OFF >= LOCK_START_OFF(Granted->Lock) && 
               REQUEST_END_OFF <= LOCK_END_OFF(Granted->Lock))
            {
               EnumEntry = &LockToc->GrantedListHead;
               break;
            }
            //else keep locking for conflicts
         }
         else 
         {//conflict if we want share access to excl. lock OR exlc. access to shared lock
            break;//FAIL
         }
      }
      EnumEntry = EnumEntry->Flink;
   }

   if (EnumEntry == &LockToc->GrantedListHead) 
   {//no conflict
      Granted = ExAllocateFromNPagedLookasideList(&GrantedLookaside);

      Granted->Lock.StartingByte = *FileOffset;
      Granted->Lock.Length = *Length;
      Granted->Lock.ExclusiveLock = ExclusiveLock;
      Granted->Lock.Key = Key;
      Granted->Lock.FileObject = FileObject;
      Granted->Lock.Process = Process;
      Granted->Lock.EndingByte.QuadPart = REQUEST_END_OFF;

      InsertHeadList(&LockToc->GrantedListHead,&Granted->ListEntry);
      return TRUE;
   }

   return FALSE;

}



/**********************************************************************
 * NAME							PRIVATE
 *	FsRtlpCompletePendingLocks
 *
 * NOTE
 *  Spinlock held at entry !!
 */
VOID
FASTCALL
FsRtlpCompletePendingLocks(
   IN       PFILE_LOCK     FileLock,
   IN       PFILE_LOCK_TOC LockToc,
   IN OUT   PKIRQL         oldirql
   )
{
   //walk pending list, FIFO order, try 2 complete locks
   PLIST_ENTRY                   EnumEntry;
   PIRP                          Irp;
   PEXTENDED_IO_STACK_LOCATION            Stack;

   EnumEntry = LockToc->PendingListHead.Blink;
   while (EnumEntry != &LockToc->PendingListHead) 
   {
      Irp = CONTAINING_RECORD(EnumEntry,IRP, Tail.Overlay.ListEntry);

      Stack = (PEXTENDED_IO_STACK_LOCATION) IoGetCurrentIrpStackLocation(Irp);
      if (FsRtlpAddLock(LockToc,
                        Stack->FileObject,
                        &Stack->Parameters.LockControl.ByteOffset,
                        Stack->Parameters.LockControl.Length,
                        IoGetRequestorProcess(Irp),
                        Stack->Parameters.LockControl.Key,
                        Stack->Flags & SL_EXCLUSIVE_LOCK
                        ) ) 
      {
         RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

         if (!IoSetCancelRoutine(Irp, NULL))
         {
            /*
            Cancel routine WILL be called after we release the spinlock. It will try to remove 
            the irp from the list and cancel/complete this irp. Since we allready removed it, 
            make its ListEntry point to itself.
            */
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
         }
         else
         {
            /*
            Cancel routine will NOT be called, canceled or not.

            Put on completed list and complete them all afterwards.
            This way we don't have to restart enum after each completion.
            */
            Irp->IoStatus.Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = 0;
            InsertHeadList(&LockToc->CompletedListHead,&Irp->Tail.Overlay.ListEntry);
         }
      }
      EnumEntry = EnumEntry->Blink;
   }

   //complete irp's (if any)
   while (!IsListEmpty(&LockToc->CompletedListHead)) 
   {
      EnumEntry = RemoveTailList(&LockToc->CompletedListHead);
      KeReleaseSpinLock(&LockToc->SpinLock, *oldirql);//fires cancel routine
      Irp = CONTAINING_RECORD(EnumEntry, IRP, Tail.Overlay.ListEntry);

      if (FileLock->CompleteLockIrpRoutine)
      {
         FileLock->CompleteLockIrpRoutine(Irp->Tail.Overlay.DriverContext[2], Irp);
      }
      else
      {
         IofCompleteRequest(Irp, IO_NO_INCREMENT);
      }

      KeAcquireSpinLock(&LockToc->SpinLock, oldirql);
   }

}



/**********************************************************************
 * NAME							PRIVATE
 *	FsRtlpUnlockSingle
 *
 */
NTSTATUS
FASTCALL
FsRtlpUnlockSingle(
   IN PFILE_LOCK           FileLock,
   IN PFILE_OBJECT         FileObject,
   IN PLARGE_INTEGER       FileOffset,
   IN PLARGE_INTEGER       Length,
   IN PEPROCESS            Process,
   IN ULONG                Key,
   IN PVOID                Context OPTIONAL,
   IN BOOLEAN              AlreadySynchronized,
   IN BOOLEAN              CallUnlockRoutine
   )
{
   KIRQL                oldirql;
   PFILE_LOCK_TOC       LockToc;
   PFILE_LOCK_GRANTED   Granted;
   PLIST_ENTRY          EnumEntry;

   assert(FileLock);
   LockToc = FileLock->LockInformation;

   if (LockToc == NULL || Length->QuadPart == 0)
   {
      return STATUS_RANGE_NOT_LOCKED;
   }

   KeAcquireSpinLock(&LockToc->SpinLock, &oldirql );

   EnumEntry = LockToc->GrantedListHead.Flink;
   while (EnumEntry != &LockToc->GrantedListHead) 
   {
      Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED,ListEntry);

      //must be exact match
      if (FileOffset->QuadPart == Granted->Lock.StartingByte.QuadPart &&
         Length->QuadPart == Granted->Lock.Length.QuadPart &&
         Granted->Lock.Process == Process &&
         Granted->Lock.FileObject == FileObject &&
         Granted->Lock.Key == Key) 
      {
         RemoveEntryList(&Granted->ListEntry);
         FsRtlpCompletePendingLocks(FileLock, LockToc, &oldirql);

         if (IsListEmpty(&LockToc->GrantedListHead))
         {
            KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
            FsRtlAreThereCurrentFileLocks(FileLock) = FALSE;
         }
         else
         {
            KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
         }

         if (FileLock->UnlockRoutine && CallUnlockRoutine)
         {
            FileLock->UnlockRoutine(Context,&Granted->Lock);
         }

         ExFreeToNPagedLookasideList(&GrantedLookaside,Granted);

         return STATUS_SUCCESS;
      }
      EnumEntry = EnumEntry->Flink;
   }

   KeReleaseSpinLock(&LockToc->SpinLock, oldirql);

   return STATUS_RANGE_NOT_LOCKED;

}



/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFastUnlockSingle
 *
 * @implemented
 */
NTSTATUS
STDCALL
FsRtlFastUnlockSingle (
   IN PFILE_LOCK           FileLock,
   IN PFILE_OBJECT         FileObject,
   IN PLARGE_INTEGER       FileOffset,
   IN PLARGE_INTEGER       Length,
   IN PEPROCESS            Process,
   IN ULONG                Key,
   IN PVOID                Context OPTIONAL,
   IN BOOLEAN              AlreadySynchronized
   )
{
   return FsRtlpUnlockSingle( FileLock,
                              FileObject,
                              FileOffset,
                              Length,
                              Process,
                              Key,
                              Context,
                              AlreadySynchronized,
                              TRUE//CallUnlockRoutine
                              );
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlpDumpFileLocks
 *
 *	NOTE: used for testing and debugging
 */
VOID
FASTCALL
FsRtlpDumpFileLocks(
   IN PFILE_LOCK  FileLock
   )
{
   KIRQL                oldirql;
   PFILE_LOCK_TOC       LockToc;
   PFILE_LOCK_GRANTED   Granted;
   PIRP                 Irp;
   PLIST_ENTRY          EnumEntry;
   PEXTENDED_IO_STACK_LOCATION   Stack;

   assert(FileLock);
   LockToc = FileLock->LockInformation;

   if (LockToc == NULL) 
   {
      DPRINT1("No file locks\n");
      return;
   }

   DPRINT1("Dumping granted file locks, FIFO order\n");

   KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

   EnumEntry = LockToc->GrantedListHead.Blink;
   while ( EnumEntry != &LockToc->GrantedListHead)
   {
      Granted = CONTAINING_RECORD(EnumEntry, FILE_LOCK_GRANTED , ListEntry );

      DPRINT1("%s, start: %i, len: %i, end: %i, key: %i, proc: 0x%X, fob: 0x%X\n",
         Granted->Lock.ExclusiveLock ? "EXCL" : "SHRD",
         Granted->Lock.StartingByte.QuadPart,
         Granted->Lock.Length.QuadPart,
         Granted->Lock.EndingByte.QuadPart,
         Granted->Lock.Key,
         Granted->Lock.Process,
         Granted->Lock.FileObject
         );

      EnumEntry = EnumEntry->Blink;
   }

   DPRINT1("Dumping pending file locks, FIFO order\n");

   EnumEntry = LockToc->PendingListHead.Blink;
   while ( EnumEntry != &LockToc->PendingListHead)
   {
      Irp = CONTAINING_RECORD(EnumEntry, IRP , Tail.Overlay.ListEntry );

      Stack = (PEXTENDED_IO_STACK_LOCATION) IoGetCurrentIrpStackLocation(Irp);

      DPRINT1("%s, start: %i, len: %i, end: %i, key: %i, proc: 0x%X, fob: 0x%X\n",
         (Stack->Flags & SL_EXCLUSIVE_LOCK) ? "EXCL" : "SHRD",
         Stack->Parameters.LockControl.ByteOffset.QuadPart,
         Stack->Parameters.LockControl.Length->QuadPart,
         Stack->Parameters.LockControl.ByteOffset.QuadPart + Stack->Parameters.LockControl.Length->QuadPart - 1,
         Stack->Parameters.LockControl.Key,
         IoGetRequestorProcess(Irp),
         Stack->FileObject
         );

      EnumEntry = EnumEntry->Blink;
   }

   KeReleaseSpinLock(&LockToc->SpinLock, oldirql);
}



/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlGetNextFileLock
 *
 * RETURN VALUE
 *	NULL if no more locks.
 *
 * @implemented
 */
PFILE_LOCK_INFO
STDCALL
FsRtlGetNextFileLock (
   IN PFILE_LOCK   FileLock,
   IN BOOLEAN      Restart
   )
{
   /*
   Messy enumeration of granted locks.
   What our last ptr. in LastReturnedLock points at, might have been freed between 
   calls, so we have to scan thru the list every time, searching for our last lock.
   If it's not there anymore, restart the enumeration...
   */
   KIRQL                oldirql;
   PLIST_ENTRY          EnumEntry;
   PFILE_LOCK_GRANTED   Granted;
   PFILE_LOCK_TOC       LockToc;
   BOOLEAN              FoundPrevious = FALSE;
   //must make local copy since FILE_LOCK struct is allowed to be in paged mem
   FILE_LOCK_INFO       LocalLastReturnedLockInfo;
   PVOID                LocalLastReturnedLock;

   assert(FileLock);
   LockToc = FileLock->LockInformation;
   if (LockToc == NULL)
   {
      return NULL;
   }

   LocalLastReturnedLock = FileLock->LastReturnedLock;

   KeAcquireSpinLock(&LockToc->SpinLock,&oldirql);

restart:;

   EnumEntry = LockToc->GrantedListHead.Flink;

   if (Restart)
   {
      if (EnumEntry != &LockToc->GrantedListHead)
      {
         Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED,ListEntry);
         LocalLastReturnedLockInfo = Granted->Lock;
         KeReleaseSpinLock(&LockToc->SpinLock,oldirql);

         FileLock->LastReturnedLockInfo = LocalLastReturnedLockInfo;
         FileLock->LastReturnedLock = EnumEntry;
         return &FileLock->LastReturnedLockInfo;
      }
      else 
      {
         KeReleaseSpinLock(&LockToc->SpinLock,oldirql);
         return NULL;
      }
   }

   //else: continue enum
   while (EnumEntry != &LockToc->GrantedListHead) 
   {
      //found previous lock?
      if (EnumEntry == LocalLastReturnedLock) 
      {
         FoundPrevious = TRUE;
         //get next
         EnumEntry = EnumEntry->Flink;
         if (EnumEntry != &LockToc->GrantedListHead)
         {
            Granted = CONTAINING_RECORD(EnumEntry,FILE_LOCK_GRANTED,ListEntry);
            LocalLastReturnedLockInfo = Granted->Lock;
            KeReleaseSpinLock(&LockToc->SpinLock,oldirql);

            FileLock->LastReturnedLockInfo = LocalLastReturnedLockInfo;
            FileLock->LastReturnedLock = EnumEntry;
            return &FileLock->LastReturnedLockInfo;
         }
         break;
      }
      EnumEntry = EnumEntry->Flink;
   }

   if (!FoundPrevious) 
   {
      //got here? uh no, didn't find our last lock..must have been freed...restart
      Restart = TRUE;
      goto restart;
   }

   KeReleaseSpinLock(&LockToc->SpinLock,oldirql);

   return NULL;//no (more) locks
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlInitializeFileLock
 *
 * NOTE
 *  Called when creating/allocating/initializing FCB
 *
 * @implemented
 */
VOID
STDCALL
FsRtlInitializeFileLock (
   IN PFILE_LOCK                   FileLock,
   IN PCOMPLETE_LOCK_IRP_ROUTINE   CompleteLockIrpRoutine OPTIONAL,
   IN PUNLOCK_ROUTINE              UnlockRoutine OPTIONAL
   )
{

   FsRtlAreThereCurrentFileLocks(FileLock) = FALSE;
   FileLock->CompleteLockIrpRoutine = CompleteLockIrpRoutine;
   FileLock->UnlockRoutine = UnlockRoutine;
   FileLock->LockInformation = NULL;

}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlPrivateLock
 *
 * @implemented
 */
BOOLEAN
STDCALL
FsRtlPrivateLock (
   IN PFILE_LOCK           FileLock,
   IN PFILE_OBJECT         FileObject,
   IN PLARGE_INTEGER       FileOffset,
   IN PLARGE_INTEGER       Length,
   IN PEPROCESS            Process,
   IN ULONG                Key,
   IN BOOLEAN              FailImmediately, //seems meaningless for fast io
   IN BOOLEAN              ExclusiveLock,
   OUT PIO_STATUS_BLOCK    IoStatus,
   IN PIRP                 Irp OPTIONAL,
   IN PVOID                Context,
   IN BOOLEAN              AlreadySynchronized
   )
{
   PFILE_LOCK_TOC       LockToc;
   KIRQL                oldirql;

   assert(FileLock);
   if (FileLock->LockInformation == NULL) 
   {
      ExAcquireFastMutex(&LockTocMutex);
      //still NULL?
      if (FileLock->LockInformation == NULL)
      {
         FileLock->LockInformation = ExAllocateFromNPagedLookasideList(&LockTocLookaside);
         LockToc =  FileLock->LockInformation;
         KeInitializeSpinLock(&LockToc->SpinLock);
         InitializeListHead(&LockToc->GrantedListHead);
         InitializeListHead(&LockToc->PendingListHead);
         InitializeListHead(&LockToc->CompletedListHead);
         InitializeListHead(&LockToc->UnlockedListHead);
      }
      ExReleaseFastMutex(&LockTocMutex);
   }

   LockToc =  FileLock->LockInformation;
   KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

   //try add new lock (while holding spin lock)
   if (FsRtlpAddLock(LockToc,
                     FileObject,
                     FileOffset,
                     Length,
                     Process,
                     Key,
                     ExclusiveLock
                     ) ) 
   {
      IoStatus->Status = STATUS_SUCCESS;
   }
   else if (Irp && !FailImmediately) 
   {	//failed + irp + no fail = mk. pending
      //for our cancel routine
      Irp->Tail.Overlay.DriverContext[0] = (PVOID)FileLock;
      Irp->Tail.Overlay.DriverContext[1] = (PVOID)LockToc;
      Irp->Tail.Overlay.DriverContext[2] = Context;

      IoSetCancelRoutine(Irp, FsRtlpFileLockCancelRoutine);

      if (Irp->Cancel) 
      {
         //irp canceled even before we got to queue it
         if (IoSetCancelRoutine(Irp, NULL))
         {  //Cancel routine will NOT be called: cancel it here
            IoStatus->Status = STATUS_CANCELLED; 
         }
         else
         {  //Cancel routine WILL be called. When we release the lock it will complete the irp
            //Return pending since we are not completing the irp here
            Irp->IoStatus.Status = IoStatus->Status = STATUS_PENDING;
            Irp->IoStatus.Information = 0;
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);
         }
         
      }
      else 
      {  //not cancelled: queue irp
         IoMarkIrpPending(Irp);
         Irp->IoStatus.Status = IoStatus->Status = STATUS_PENDING;
         Irp->IoStatus.Information = 0;
         InsertHeadList(&LockToc->PendingListHead,&Irp->Tail.Overlay.ListEntry);
      }

   }
   else 
   {
      IoStatus->Status = STATUS_LOCK_NOT_GRANTED;
   }

   KeReleaseSpinLock(&LockToc->SpinLock, oldirql);	//fires cancel routine

   //never pending if no irp;-)
   assert(!(IoStatus->Status == STATUS_PENDING && !Irp));

   if (IoStatus->Status != STATUS_PENDING) 
   {
      if (IoStatus->Status == STATUS_SUCCESS) 
      {
         FsRtlAreThereCurrentFileLocks(FileLock) = TRUE;
      }

      if (Irp) 
      {
         Irp->IoStatus.Status = IoStatus->Status;
         Irp->IoStatus.Information = 0;

         if (FileLock->CompleteLockIrpRoutine) 
         { //complete irp routine

            if (!NT_SUCCESS(FileLock->CompleteLockIrpRoutine(Context,Irp))) 
            {
               //CompleteLockIrpRoutine complain: revert changes
               FsRtlpUnlockSingle(  FileLock,
                                    FileObject,
                                    FileOffset,
                                    Length,
                                    Process,
                                    Key,
                                    Context,
                                    AlreadySynchronized,
                                    FALSE//CallUnlockRoutine
                                    );
            }
         }
         else 
         {//std irp completion
            IofCompleteRequest(Irp, IO_NO_INCREMENT);
         }
      }
   }

   //NOTE: only fast io seems to care about this return value
   return (IoStatus->Status == STATUS_SUCCESS || FailImmediately);

}



/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlProcessFileLock
 *
 * @implemented
 */
NTSTATUS
STDCALL
FsRtlProcessFileLock (
   IN PFILE_LOCK   FileLock,
   IN PIRP         Irp,
   IN PVOID        Context OPTIONAL
   )
{
   PEXTENDED_IO_STACK_LOCATION   Stack;
   NTSTATUS             Status;
   IO_STATUS_BLOCK      LocalIoStatus;

   assert(FileLock);
   Stack = (PEXTENDED_IO_STACK_LOCATION) IoGetCurrentIrpStackLocation(Irp);
   Irp->IoStatus.Information = 0;

   switch(Stack->MinorFunction)
   {
      case IRP_MN_LOCK:
         //ret: BOOLEAN
         FsRtlPrivateLock( FileLock,
                           Stack->FileObject,
                           &Stack->Parameters.LockControl.ByteOffset, //not pointer!
                           Stack->Parameters.LockControl.Length,
                           IoGetRequestorProcess(Irp),
                           Stack->Parameters.LockControl.Key,
                           Stack->Flags & SL_FAIL_IMMEDIATELY,
                           Stack->Flags & SL_EXCLUSIVE_LOCK,
                           &LocalIoStatus,
                           Irp,
                           Context,
                           FALSE);

         return LocalIoStatus.Status;

      case IRP_MN_UNLOCK_SINGLE:
         Status = FsRtlFastUnlockSingle ( FileLock,
                                          Stack->FileObject,
                                          &Stack->Parameters.LockControl.ByteOffset,
                                          Stack->Parameters.LockControl.Length,
                                          IoGetRequestorProcess(Irp),
                                          Stack->Parameters.LockControl.Key,
                                          Context,
                                          FALSE);
         break;

      case IRP_MN_UNLOCK_ALL:
         Status = FsRtlFastUnlockAll(  FileLock,
                                       Stack->FileObject,
                                       IoGetRequestorProcess(Irp),
                                       Context);
         break;

      case IRP_MN_UNLOCK_ALL_BY_KEY:
         Status = FsRtlFastUnlockAllByKey (  FileLock,
                                             Stack->FileObject,
                                             IoGetRequestorProcess(Irp),
                                             Stack->Parameters.LockControl.Key,
                                             Context);

         break;

      default:
         Irp->IoStatus.Status = Status = STATUS_INVALID_DEVICE_REQUEST;
         IofCompleteRequest(Irp, IO_NO_INCREMENT);
         return Status;
   }

   Irp->IoStatus.Status = Status;

   if (FileLock->CompleteLockIrpRoutine )
   {
      FileLock->CompleteLockIrpRoutine(Context,Irp);
   }
   else
   {
      IofCompleteRequest(Irp,IO_NO_INCREMENT);
   }

   return Status;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlUninitializeFileLock
 *
 * @implemented
 */
VOID
STDCALL
FsRtlUninitializeFileLock (
   IN PFILE_LOCK FileLock
   )
{
   PFILE_LOCK_TOC       LockToc;
   PIRP                 Irp;
   PFILE_LOCK_GRANTED   Granted;
   PLIST_ENTRY          EnumEntry;
   KIRQL                oldirql;

   assert(FileLock);
   if (FileLock->LockInformation == NULL)
   {
      return;
   }

   LockToc = FileLock->LockInformation;

   KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);

   //remove and free granted locks
   while (!IsListEmpty(&LockToc->GrantedListHead)) 
   {
      EnumEntry = RemoveTailList(&LockToc->GrantedListHead);
      Granted = CONTAINING_RECORD(EnumEntry, FILE_LOCK_GRANTED, ListEntry);
      ExFreeToNPagedLookasideList(&GrantedLookaside, Granted);
   }

   //remove, complete and free all pending locks
   while (!IsListEmpty(&LockToc->PendingListHead)) 
   {
      EnumEntry = RemoveTailList(&LockToc->PendingListHead);
      Irp = CONTAINING_RECORD(EnumEntry, IRP, Tail.Overlay.ListEntry);

      if (!IoSetCancelRoutine(Irp, NULL))
      {  
         //The cancel routine will be called. When we release the lock it will complete the irp.
         InitializeListHead(&Irp->Tail.Overlay.ListEntry);
      }
      else
      {
         /*
         Cancel routine will NOT be called, even though the irp might have been canceled.
         Don't care since we'l complete it faster than the cancel routine would have.
         */
         KeReleaseSpinLock(&LockToc->SpinLock, oldirql);//fires cancel routine	

         Irp->IoStatus.Status = STATUS_RANGE_NOT_LOCKED;
   
         if (FileLock->CompleteLockIrpRoutine)
         {
            FileLock->CompleteLockIrpRoutine(Irp->Tail.Overlay.DriverContext[2], Irp);
         }
         else
         {
            IofCompleteRequest(Irp, IO_NO_INCREMENT);
         }

         KeAcquireSpinLock(&LockToc->SpinLock, &oldirql);
      }
   }

   KeReleaseSpinLock(&LockToc->SpinLock, oldirql);

   ExFreeToNPagedLookasideList(&LockTocLookaside, LockToc);

   FsRtlAreThereCurrentFileLocks(FileLock) = FALSE;
   FileLock->LockInformation = NULL;

}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlAllocateFileLock
 *
 * NOTE
 * 	Only present in NT 5.0 or later.
 *	FCB FILE_LOCK struct should/is acording to DDK allocated from paged pool!
 *
 * @implemented
 */
PFILE_LOCK
STDCALL
FsRtlAllocateFileLock(
   IN PCOMPLETE_LOCK_IRP_ROUTINE    CompleteLockIrpRoutine OPTIONAL,
   IN PUNLOCK_ROUTINE               UnlockRoutine OPTIONAL
   )
{
   PFILE_LOCK  FileLock;

   FileLock = ExAllocateFromPagedLookasideList(&LockLookaside);

   FsRtlInitializeFileLock(FileLock,
                           CompleteLockIrpRoutine,
                           UnlockRoutine
                           );

   return FileLock;
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlFreeFileLock
 *
 * NOTE
 * 	Only present in NT 5.0 or later.
 *	FCB FILE_LOCK struct should/is acording to DDK allocated from paged pool!
 *
 * @implemented
 */
VOID
STDCALL
FsRtlFreeFileLock(
   IN PFILE_LOCK FileLock
   )
{
   assert(FileLock);

   FsRtlUninitializeFileLock(FileLock);
   ExFreeToPagedLookasideList(&LockLookaside, FileLock);
}

/* EOF */
