
/* $Id: rw.c,v 1.66 2004/05/15 23:00:02 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/rw.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Hartmut Birr
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntos/kefuncs.h>
#include <wchar.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NextCluster(PDEVICE_EXTENSION DeviceExt,
	    ULONG FirstCluster,
	    PULONG CurrentCluster,
	    BOOLEAN Extend)
     /*
      * Return the next cluster in a FAT chain, possibly extending the chain if
      * necessary
      */
{
 if (FirstCluster == 1)
    {
      (*CurrentCluster) += DeviceExt->FatInfo.SectorsPerCluster;
      return(STATUS_SUCCESS);
    }
  else
    {
      /* 
       * CN: FIXME: Real bug here or in dirwr, where CurrentCluster isn't 
       * initialized when 0
       */
      if (FirstCluster == 0)
	{
	  NTSTATUS Status;
	  
	  Status = GetNextCluster(DeviceExt, 0, CurrentCluster,
				  Extend);
	  return(Status);
	}
      else
	{
	  NTSTATUS Status;
	  
	  Status = GetNextCluster(DeviceExt, (*CurrentCluster), CurrentCluster,
				  Extend);
	  return(Status);
	}
    }
}

NTSTATUS
OffsetToCluster(PDEVICE_EXTENSION DeviceExt,
		ULONG FirstCluster,
		ULONG FileOffset,
		PULONG Cluster,
		BOOLEAN Extend)
     /*
      * Return the cluster corresponding to an offset within a file,
      * possibly extending the file if necessary
      */
{
  ULONG CurrentCluster;
  ULONG i;
  NTSTATUS Status;
/*
  DPRINT("OffsetToCluster(DeviceExt %x, Fcb %x, FirstCluster %x,"
         " FileOffset %x, Cluster %x, Extend %d)\n", DeviceExt, 
         Fcb, FirstCluster, FileOffset, Cluster, Extend);
*/
  if (FirstCluster == 0)
  {
    DbgPrint("OffsetToCluster is called with FirstCluster = 0!\n");
    KEBUGCHECK(0);
  }

  if (FirstCluster == 1)
    {
      /* root of FAT16 or FAT12 */
      *Cluster = DeviceExt->FatInfo.rootStart + FileOffset
	/ (DeviceExt->FatInfo.BytesPerCluster) * DeviceExt->FatInfo.SectorsPerCluster;
      return(STATUS_SUCCESS);
    }
  else
    {
      CurrentCluster = FirstCluster;
      for (i = 0; i < FileOffset / DeviceExt->FatInfo.BytesPerCluster; i++)
	{
	  Status = GetNextCluster (DeviceExt, CurrentCluster, &CurrentCluster,
				   Extend);
	  if (!NT_SUCCESS(Status))
	    {
	      return(Status);
	    }
	}
      *Cluster = CurrentCluster;
      return(STATUS_SUCCESS);
    }
}

NTSTATUS
VfatReadFileData (PVFAT_IRP_CONTEXT IrpContext, 
                  ULONG Length, 
		  LARGE_INTEGER ReadOffset, 
		  PULONG LengthRead)
/*
 * FUNCTION: Reads data from a file
 */
{
  ULONG CurrentCluster;
  ULONG FirstCluster;
  ULONG StartCluster;
  ULONG ClusterCount;
  LARGE_INTEGER StartOffset;
  PDEVICE_EXTENSION DeviceExt;
  BOOLEAN First = TRUE;
  PVFATFCB Fcb;
  PVFATCCB Ccb;
  NTSTATUS Status;
  ULONG BytesDone;
  ULONG BytesPerSector;
  ULONG BytesPerCluster;
  ULONG Count;

  /* PRECONDITION */
  assert (IrpContext);
  DeviceExt = IrpContext->DeviceExt;
  assert (DeviceExt);
  assert (DeviceExt->FatInfo.BytesPerCluster);
  assert (IrpContext->FileObject);
  assert (IrpContext->FileObject->FsContext2 != NULL);

  DPRINT("VfatReadFileData(DeviceExt %x, FileObject %x, "
	 "Length %d, ReadOffset 0x%I64x)\n", DeviceExt,
	 IrpContext->FileObject, Length, ReadOffset.QuadPart);

  *LengthRead = 0;

  Ccb = (PVFATCCB)IrpContext->FileObject->FsContext2;
  Fcb = IrpContext->FileObject->FsContext;
  BytesPerSector = DeviceExt->FatInfo.BytesPerSector;
  BytesPerCluster = DeviceExt->FatInfo.BytesPerCluster;

  assert(ReadOffset.QuadPart + Length <= ROUND_UP(Fcb->RFCB.FileSize.QuadPart, BytesPerSector));
  assert(ReadOffset.u.LowPart % BytesPerSector == 0);
  assert(Length % BytesPerSector == 0);

  /* Is this a read of the FAT? */
  if (Fcb->Flags & FCB_IS_FAT)
  {
    ReadOffset.QuadPart += DeviceExt->FatInfo.FATStart * BytesPerSector;
    Status = VfatReadDiskPartial(IrpContext, &ReadOffset, Length, 0, TRUE);

    if (NT_SUCCESS(Status))
    {
      *LengthRead = Length;
    }
    else
    {
      DPRINT1("FAT reading failed, Status %x\n", Status);
    }
    return Status;
  }
  /* Is this a read of the Volume ? */
  if (Fcb->Flags & FCB_IS_VOLUME)
  {
    Status = VfatReadDiskPartial(IrpContext, &ReadOffset, Length, 0, TRUE);
    if (NT_SUCCESS(Status))
    {
      *LengthRead = Length;
    }
    else
    {
      DPRINT1("Volume reading failed, Status %x\n", Status);
    }
    return Status;
  }

  /*
   * Find the first cluster
   */
  FirstCluster = CurrentCluster =
    vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);

  if (FirstCluster == 1)
  {
    // Directory of FAT12/16 needs a special handling
    CHECKPOINT;
    if (ReadOffset.u.LowPart + Length > DeviceExt->FatInfo.rootDirectorySectors * BytesPerSector)
    {
      Length = DeviceExt->FatInfo.rootDirectorySectors * BytesPerSector - ReadOffset.u.LowPart;
    }
    ReadOffset.u.LowPart += DeviceExt->FatInfo.rootStart * BytesPerSector;

    // Fire up the read command
    
    Status = VfatReadDiskPartial (IrpContext, &ReadOffset, Length, 0, TRUE);
    if (NT_SUCCESS(Status))
    {
      *LengthRead = Length;
    }
    return Status;
  }
  /*
   * Find the cluster to start the read from
   */
  if (Ccb->LastCluster > 0 && ReadOffset.u.LowPart > Ccb->LastOffset)
  {
    CurrentCluster = Ccb->LastCluster;
  }
  Status = OffsetToCluster(DeviceExt, FirstCluster,
			   ROUND_DOWN(ReadOffset.u.LowPart, BytesPerCluster),
			   &CurrentCluster, FALSE);
  if (!NT_SUCCESS(Status))
  {
    return(Status);
  }

  Ccb->LastCluster = CurrentCluster;
  Ccb->LastOffset = ROUND_DOWN (ReadOffset.u.LowPart, BytesPerCluster);

  KeInitializeEvent(&IrpContext->Event, NotificationEvent, FALSE);
  IrpContext->RefCount = 1;
  Count = 0;

  while (Length > 0 && CurrentCluster != 0xffffffff)
  {
    StartCluster = CurrentCluster;
    StartOffset.QuadPart = ClusterToSector(DeviceExt, StartCluster) * BytesPerSector;
    BytesDone = 0;
    ClusterCount = 0;

    do
    {
      ClusterCount++;
      if (First)
      {
        BytesDone =  min (Length, BytesPerCluster - (ReadOffset.u.LowPart % BytesPerCluster));
      	StartOffset.QuadPart += ReadOffset.u.LowPart % BytesPerCluster;
      	First = FALSE;
      }
      else
      {
      	if (Length - BytesDone > BytesPerCluster)
      	{
      	  BytesDone += BytesPerCluster;
      	}
      	else
      	{
      	  BytesDone = Length;
      	}
      }
      Status = NextCluster(DeviceExt, FirstCluster, &CurrentCluster, FALSE);
    }
    while (StartCluster + ClusterCount == CurrentCluster && NT_SUCCESS(Status) && Length > BytesDone);
    DPRINT("start %08x, next %08x, count %d\n",
           StartCluster, CurrentCluster, ClusterCount);

    Ccb->LastCluster = StartCluster + (ClusterCount - 1);
    Ccb->LastOffset = ReadOffset.u.LowPart + (ClusterCount - 1) * BytesPerCluster;

    Count++;

    // Fire up the read command
    Status = VfatReadDiskPartial (IrpContext, &StartOffset, BytesDone, *LengthRead, FALSE);
    if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
      {
        break;
      }
    *LengthRead += BytesDone;
    Length -= BytesDone;
    ReadOffset.u.LowPart += BytesDone;
  }
  if (0 != InterlockedDecrement((PLONG)&IrpContext->RefCount))
    {
      KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
    }
  if (NT_SUCCESS(Status) || Status == STATUS_PENDING)
    {
      if (Length > 0)
        {
	  Status = STATUS_UNSUCCESSFUL;
	}
      else
        {
          Status = IrpContext->Irp->IoStatus.Status;
	}
    }
  return Status;
}

NTSTATUS 
VfatWriteFileData(PVFAT_IRP_CONTEXT IrpContext,
		  ULONG Length,
		  LARGE_INTEGER WriteOffset)
{
   PDEVICE_EXTENSION DeviceExt;
   PVFATFCB Fcb;
   PVFATCCB Ccb;
   ULONG Count;
   ULONG FirstCluster;
   ULONG CurrentCluster;
   ULONG BytesDone;
   ULONG StartCluster;
   ULONG ClusterCount;
   NTSTATUS Status = STATUS_SUCCESS;
   BOOLEAN First = TRUE;
   ULONG BytesPerSector;
   ULONG BytesPerCluster;
   LARGE_INTEGER StartOffset;
   ULONG BufferOffset;

   /* PRECONDITION */
   assert (IrpContext);
   DeviceExt = IrpContext->DeviceExt;
   assert (DeviceExt);
   assert (DeviceExt->FatInfo.BytesPerCluster);
   assert (IrpContext->FileObject);
   assert (IrpContext->FileObject->FsContext2 != NULL);

   Ccb = (PVFATCCB)IrpContext->FileObject->FsContext2;
   Fcb = IrpContext->FileObject->FsContext;
   BytesPerCluster = DeviceExt->FatInfo.BytesPerCluster;
   BytesPerSector = DeviceExt->FatInfo.BytesPerSector;

   DPRINT("VfatWriteFileData(DeviceExt %x, FileObject %x, "
	  "Length %d, WriteOffset 0x%I64x), '%wZ'\n", DeviceExt,
	  IrpContext->FileObject, Length, WriteOffset,
	  &Fcb->PathNameU);

   assert(WriteOffset.QuadPart + Length <= Fcb->RFCB.AllocationSize.QuadPart);
   assert(WriteOffset.u.LowPart % BytesPerSector == 0);
   assert(Length % BytesPerSector == 0)

   // Is this a write of the volume ?
   if (Fcb->Flags & FCB_IS_VOLUME)
   {
      Status = VfatWriteDiskPartial(IrpContext, &WriteOffset, Length, 0, TRUE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Volume writing failed, Status %x\n", Status);
      }
      return Status;
   }

   // Is this a write to the FAT ?
   if (Fcb->Flags & FCB_IS_FAT)
   {
      WriteOffset.u.LowPart += DeviceExt->FatInfo.FATStart * BytesPerSector;
      IrpContext->RefCount = 1;
      for (Count = 0; Count < DeviceExt->FatInfo.FATCount; Count++)
      {
         Status = VfatWriteDiskPartial(IrpContext, &WriteOffset, Length, 0, FALSE);
         if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
         {
            DPRINT1("FAT writing failed, Status %x\n", Status);
	    break;
         }
	 WriteOffset.u.LowPart += Fcb->RFCB.FileSize.u.LowPart;
      }
      if (0 != InterlockedDecrement((PLONG)&IrpContext->RefCount))
        {
	  KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
	}
      if (NT_SUCCESS(Status) || Status == STATUS_PENDING)
        {
	  Status = IrpContext->Irp->IoStatus.Status;
	}
      return Status;
   }

   /*
    * Find the first cluster
    */
   FirstCluster = CurrentCluster =
      vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);

   if (FirstCluster == 1)
   {
      assert(WriteOffset.u.LowPart + Length <= DeviceExt->FatInfo.rootDirectorySectors * BytesPerSector);
      // Directory of FAT12/16 needs a special handling
      WriteOffset.u.LowPart += DeviceExt->FatInfo.rootStart * BytesPerSector;
      // Fire up the write command
      Status = VfatWriteDiskPartial (IrpContext, &WriteOffset, Length, 0, TRUE);
      return Status;
   }

   /*
    * Find the cluster to start the write from
    */
   if (Ccb->LastCluster > 0 && WriteOffset.u.LowPart > Ccb->LastOffset)
   {
      CurrentCluster = Ccb->LastCluster;
   }

   Status = OffsetToCluster(DeviceExt, FirstCluster,
			    ROUND_DOWN(WriteOffset.u.LowPart, BytesPerCluster),
			    &CurrentCluster, FALSE);

   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   Ccb->LastCluster = CurrentCluster;
   Ccb->LastOffset = ROUND_DOWN (WriteOffset.u.LowPart, BytesPerCluster);

   IrpContext->RefCount = 1;
   Count = 0;
   BufferOffset = 0;

   while (Length > 0 && CurrentCluster != 0xffffffff)
   {
      StartCluster = CurrentCluster;
      StartOffset.QuadPart = ClusterToSector(DeviceExt, StartCluster) * BytesPerSector;
      BytesDone = 0;
      ClusterCount = 0;

      do
      {
         ClusterCount++;
         if (First)
         {
            BytesDone =  min (Length, BytesPerCluster - (WriteOffset.u.LowPart % BytesPerCluster));
	    StartOffset.QuadPart += WriteOffset.u.LowPart % BytesPerCluster;
      	    First = FALSE;
         }
         else
         {
      	    if (Length - BytesDone > BytesPerCluster)
      	    {
      	       BytesDone += BytesPerCluster;
      	    }
      	    else
      	    {
      	       BytesDone = Length;
      	    }
         }
         Status = NextCluster(DeviceExt, FirstCluster, &CurrentCluster, FALSE);
      }
      while (StartCluster + ClusterCount == CurrentCluster && NT_SUCCESS(Status) && Length > BytesDone);
      DPRINT("start %08x, next %08x, count %d\n",
             StartCluster, CurrentCluster, ClusterCount);

      Ccb->LastCluster = StartCluster + (ClusterCount - 1);
      Ccb->LastOffset = WriteOffset.u.LowPart + (ClusterCount - 1) * BytesPerCluster;

      // Fire up the write command
      Status = VfatWriteDiskPartial (IrpContext, &StartOffset, BytesDone, BufferOffset, FALSE);
      Count++;
      if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
        {
	  break;
	}
      BufferOffset += BytesDone;
      Length -= BytesDone;
      WriteOffset.u.LowPart += BytesDone;
   }
   if (0 != InterlockedDecrement((PLONG)&IrpContext->RefCount))
     {
       KeWaitForSingleObject(&IrpContext->Event, Executive, KernelMode, FALSE, NULL);
     }
   if (NT_SUCCESS(Status) || Status == STATUS_PENDING)
     {
       if (Length > 0)
         {
	   Status = STATUS_UNSUCCESSFUL;
	 }
       else
         {
	   Status = IrpContext->Irp->IoStatus.Status;
	 }
     }
   return Status;
}

NTSTATUS
VfatRead(PVFAT_IRP_CONTEXT IrpContext)
{
   NTSTATUS Status;
   PVFATFCB Fcb;
   ULONG Length = 0;
   ULONG ReturnedLength = 0;
   PERESOURCE Resource = NULL;
   LARGE_INTEGER ByteOffset;
   PVOID Buffer;
   PDEVICE_OBJECT DeviceToVerify;
   ULONG BytesPerSector;

   assert(IrpContext);

   DPRINT("VfatRead(IrpContext %x)\n", IrpContext);

   assert(IrpContext->DeviceObject);

   // This request is not allowed on the main device object
   if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
   {
      DPRINT("VfatRead is called with the main device object.\n");
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
   }

   assert(IrpContext->DeviceExt);
   assert(IrpContext->FileObject);
   Fcb = IrpContext->FileObject->FsContext;
   assert(Fcb);

   DPRINT("<%wZ>\n", &Fcb->PathNameU);

   if (Fcb->Flags & FCB_IS_PAGE_FILE)
   {
      PFATINFO FatInfo = &IrpContext->DeviceExt->FatInfo;
      IrpContext->Stack->Parameters.Read.ByteOffset.QuadPart += FatInfo->dataStart * FatInfo->BytesPerSector;
      IoSkipCurrentIrpStackLocation(IrpContext->Irp);
      DPRINT("Read from page file, disk offset %I64x\n", IrpContext->Stack->Parameters.Read.ByteOffset.QuadPart);
      Status = IoCallDriver(IrpContext->DeviceExt->StorageDevice, IrpContext->Irp);
      VfatFreeIrpContext(IrpContext);
      return Status;
   }

   ByteOffset = IrpContext->Stack->Parameters.Read.ByteOffset;
   Length = IrpContext->Stack->Parameters.Read.Length;
   BytesPerSector = IrpContext->DeviceExt->FatInfo.BytesPerSector;

   /* fail if file is a directory and no paged read */
   if (Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY && !(IrpContext->Irp->Flags & IRP_PAGING_IO))
   {
      Status = STATUS_INVALID_PARAMETER;
      goto ByeBye;
   }


   DPRINT("'%wZ', Offset: %d, Length %d\n", &Fcb->PathNameU, ByteOffset.u.LowPart, Length);

   if (ByteOffset.u.HighPart && !(Fcb->Flags & FCB_IS_VOLUME))
   {
      Status = STATUS_INVALID_PARAMETER;
      goto ByeBye;
   }
   if (ByteOffset.QuadPart >= Fcb->RFCB.FileSize.QuadPart)
   {
      IrpContext->Irp->IoStatus.Information = 0;
      Status = STATUS_END_OF_FILE;
      goto ByeBye;
   }
   if (IrpContext->Irp->Flags & (IRP_PAGING_IO | IRP_NOCACHE) || (Fcb->Flags & FCB_IS_VOLUME))
   {
      if (ByteOffset.u.LowPart % BytesPerSector != 0 || Length % BytesPerSector != 0)
      {
         DPRINT("%d %d\n", ByteOffset.u.LowPart, Length);
         // non cached read must be sector aligned
         Status = STATUS_INVALID_PARAMETER;
         goto ByeBye;
      }
   }
   if (Length == 0)
   {
      IrpContext->Irp->IoStatus.Information = 0;
      Status = STATUS_SUCCESS;
      goto ByeBye;
   }
   
   if (Fcb->Flags & FCB_IS_VOLUME)
   {
      Resource = &IrpContext->DeviceExt->DirResource;
   }
   else if (IrpContext->Irp->Flags & IRP_PAGING_IO)
   {
      Resource = &Fcb->PagingIoResource;
   }
   else
   {
      Resource = &Fcb->MainResource;
   }
   if (!ExAcquireResourceSharedLite(Resource,
                                    (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
   {
      Resource = NULL;
      Status = STATUS_PENDING;
      goto ByeBye;
   }

   if (!(IrpContext->Irp->Flags & IRP_PAGING_IO) &&
      FsRtlAreThereCurrentFileLocks(&Fcb->FileLock))
   {
      if (!FsRtlCheckLockForReadAccess(&Fcb->FileLock, IrpContext->Irp)) 
      {
         Status = STATUS_FILE_LOCK_CONFLICT;
         goto ByeBye;
      }
   }

   Buffer = VfatGetUserBuffer(IrpContext->Irp);
   if (!Buffer)
     {
       Status = STATUS_INVALID_USER_BUFFER;
       goto ByeBye;
     }

   if (!(IrpContext->Irp->Flags & (IRP_NOCACHE|IRP_PAGING_IO)) &&
     !(Fcb->Flags & (FCB_IS_PAGE_FILE|FCB_IS_VOLUME)))
   {
      // cached read
      CHECKPOINT;
      Status = STATUS_SUCCESS;
      if (ByteOffset.u.LowPart + Length > Fcb->RFCB.FileSize.u.LowPart)
      {
         Length = Fcb->RFCB.FileSize.u.LowPart - ByteOffset.u.LowPart;
         Status = /*STATUS_END_OF_FILE*/STATUS_SUCCESS;
      }

      CHECKPOINT;
      if (IrpContext->FileObject->PrivateCacheMap == NULL)
      {
	  ULONG CacheSize;
	  CacheSize = IrpContext->DeviceExt->FatInfo.BytesPerCluster;
	  if (CacheSize < PAGE_SIZE)
	  {
	     CacheSize = PAGE_SIZE;
	  }
	  CcRosInitializeFileCache(IrpContext->FileObject, CacheSize);
      }
      if (!CcCopyRead(IrpContext->FileObject, &ByteOffset, Length,
                      (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT), Buffer,
                      &IrpContext->Irp->IoStatus))
      {
         Status = STATUS_PENDING;
         goto ByeBye;
      }
      CHECKPOINT;
      if (!NT_SUCCESS(IrpContext->Irp->IoStatus.Status))
      {
        Status = IrpContext->Irp->IoStatus.Status;
      }
   }
   else
   {
      // non cached read
      CHECKPOINT;
      if (ByteOffset.QuadPart + Length > ROUND_UP(Fcb->RFCB.FileSize.QuadPart, BytesPerSector))
      {
         Length = (ULONG)(ROUND_UP(Fcb->RFCB.FileSize.QuadPart, BytesPerSector) - ByteOffset.QuadPart);
      }

      Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoWriteAccess);
      if (!NT_SUCCESS(Status))
        {
          goto ByeBye;
        }

      Status = VfatReadFileData(IrpContext, Length, ByteOffset, &ReturnedLength);
/**/
      if (Status == STATUS_VERIFY_REQUIRED)
      {
         DPRINT("VfatReadFile returned STATUS_VERIFY_REQUIRED\n");
         DeviceToVerify = IoGetDeviceToVerify((struct _ETHREAD*)KeGetCurrentThread());
         IoSetDeviceToVerify((struct _ETHREAD*)KeGetCurrentThread(), NULL);
         Status = IoVerifyVolume (DeviceToVerify, FALSE);

         if (NT_SUCCESS(Status))
         {
            Status = VfatReadFileData(IrpContext, Length,
                                      ByteOffset, &ReturnedLength);
         }

      }
/**/
      if (NT_SUCCESS(Status))
      {
         IrpContext->Irp->IoStatus.Information = ReturnedLength;
      }
   }

ByeBye:
   if (Resource)
   {
      ExReleaseResourceLite(Resource);
   }

   if (Status == STATUS_PENDING)
   {
      Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoWriteAccess);
      if (NT_SUCCESS(Status))
      {
         Status = VfatQueueRequest(IrpContext);
      }
      else
      {
         IrpContext->Irp->IoStatus.Status = Status;
         IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
         VfatFreeIrpContext(IrpContext);
      }
   }
   else
   {
      IrpContext->Irp->IoStatus.Status = Status;
      if (IrpContext->FileObject->Flags & FO_SYNCHRONOUS_IO &&
          !(IrpContext->Irp->Flags & IRP_PAGING_IO) &&
          (NT_SUCCESS(Status) || Status==STATUS_END_OF_FILE))
      {
         IrpContext->FileObject->CurrentByteOffset.QuadPart =
           ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
      }

      IoCompleteRequest(IrpContext->Irp,
                        (CCHAR)(NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
      VfatFreeIrpContext(IrpContext);
   }
   DPRINT("%x\n", Status);
   return Status;
}

NTSTATUS VfatWrite (PVFAT_IRP_CONTEXT IrpContext)
{
   PVFATFCB Fcb;
   PERESOURCE Resource = NULL;
   LARGE_INTEGER ByteOffset;
   LARGE_INTEGER OldFileSize;
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG Length = 0;
   ULONG OldAllocationSize;
   PVOID Buffer;
   ULONG BytesPerSector;

   assert (IrpContext);

   DPRINT("VfatWrite(IrpContext %x)\n", IrpContext);

   assert(IrpContext->DeviceObject);

   // This request is not allowed on the main device object
   if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
   {
      DPRINT("VfatWrite is called with the main device object.\n");
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
   }

   assert(IrpContext->DeviceExt);
   assert(IrpContext->FileObject);
   Fcb = IrpContext->FileObject->FsContext;
   assert(Fcb);

   DPRINT("<%wZ>\n", &Fcb->PathNameU);

   if (Fcb->Flags & FCB_IS_PAGE_FILE)
   {
      PFATINFO FatInfo = &IrpContext->DeviceExt->FatInfo;
      IrpContext->Stack->Parameters.Write.ByteOffset.QuadPart += FatInfo->dataStart * FatInfo->BytesPerSector;
      IoSkipCurrentIrpStackLocation(IrpContext->Irp);
      DPRINT("Write to page file, disk offset %I64x\n", IrpContext->Stack->Parameters.Write.ByteOffset.QuadPart);
      Status = IoCallDriver(IrpContext->DeviceExt->StorageDevice, IrpContext->Irp);
      VfatFreeIrpContext(IrpContext);
      return Status;
   }

  /* fail if file is a directory and no paged read */
   if (Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY && !(IrpContext->Irp->Flags & IRP_PAGING_IO))
   {
      Status = STATUS_INVALID_PARAMETER;
      goto ByeBye;
   }

   ByteOffset = IrpContext->Stack->Parameters.Write.ByteOffset;
   Length = IrpContext->Stack->Parameters.Write.Length;
   BytesPerSector = IrpContext->DeviceExt->FatInfo.BytesPerSector;

   if (ByteOffset.u.HighPart && !(Fcb->Flags & FCB_IS_VOLUME))
   {
      Status = STATUS_INVALID_PARAMETER;
      goto ByeBye;
   }

   if (Fcb->Flags & (FCB_IS_FAT | FCB_IS_VOLUME) ||
       1 == vfatDirEntryGetFirstCluster (IrpContext->DeviceExt, &Fcb->entry))
   {
      if (ByteOffset.QuadPart + Length > Fcb->RFCB.FileSize.QuadPart)
      {
         // we can't extend the FAT, the volume or the root on FAT12/FAT16
         Status = STATUS_END_OF_FILE;
         goto ByeBye;
      }
   }

   if (IrpContext->Irp->Flags & (IRP_PAGING_IO|IRP_NOCACHE) || (Fcb->Flags & FCB_IS_VOLUME))
   {
      if (ByteOffset.u.LowPart % BytesPerSector != 0 || Length % BytesPerSector != 0)
      {
         // non cached write must be sector aligned
         Status = STATUS_INVALID_PARAMETER;
         goto ByeBye;
      }
   }

   if (Length == 0)
   {
      /* FIXME:
       *   Update last write time
       */
      IrpContext->Irp->IoStatus.Information = 0;
      Status = STATUS_SUCCESS;
      goto ByeBye;
   }

   if (IrpContext->Irp->Flags & IRP_PAGING_IO)
   {
      if (ByteOffset.u.LowPart + Length > Fcb->RFCB.AllocationSize.u.LowPart)
      {
         Status = STATUS_INVALID_PARAMETER;
         goto ByeBye;
      }
      if (ByteOffset.u.LowPart + Length > ROUND_UP(Fcb->RFCB.AllocationSize.u.LowPart, BytesPerSector))
      {
         Length =  ROUND_UP(Fcb->RFCB.FileSize.u.LowPart, BytesPerSector) - ByteOffset.u.LowPart;
      }
   }

   if (Fcb->Flags & FCB_IS_VOLUME)
   {
      Resource = &IrpContext->DeviceExt->DirResource;
   }
   else if (IrpContext->Irp->Flags & IRP_PAGING_IO)
   {
      Resource = &Fcb->PagingIoResource;
   }
   else
   {
      Resource = &Fcb->MainResource;
   }

   if (Fcb->Flags & FCB_IS_PAGE_FILE)
   {
      if (!ExAcquireResourceSharedLite(Resource,
                                       (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
      {
         Resource = NULL;
         Status = STATUS_PENDING;
         goto ByeBye;
      }
   }
   else
   {
      if (!ExAcquireResourceExclusiveLite(Resource,
                                          (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
      {
         Resource = NULL;
         Status = STATUS_PENDING;
         goto ByeBye;
      }
   }

   if (!(IrpContext->Irp->Flags & IRP_PAGING_IO) &&
      FsRtlAreThereCurrentFileLocks(&Fcb->FileLock))
   {
      if (!FsRtlCheckLockForWriteAccess(&Fcb->FileLock, IrpContext->Irp)) 
      {
         Status = STATUS_FILE_LOCK_CONFLICT;
         goto ByeBye;
       }
    }

   if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT) && !(Fcb->Flags & FCB_IS_VOLUME))
   {
      if (ByteOffset.u.LowPart + Length > Fcb->RFCB.AllocationSize.u.LowPart)
      {
        Status = STATUS_PENDING;
        goto ByeBye;
      }
   }

   OldFileSize = Fcb->RFCB.FileSize;
   OldAllocationSize = Fcb->RFCB.AllocationSize.u.LowPart;

   Buffer = VfatGetUserBuffer(IrpContext->Irp);
   if (!Buffer)
     {
       Status = STATUS_INVALID_USER_BUFFER;
       goto ByeBye;
     }


   if (!(Fcb->Flags & (FCB_IS_FAT|FCB_IS_VOLUME)) && 
       !(IrpContext->Irp->Flags & IRP_PAGING_IO) &&
       ByteOffset.u.LowPart + Length > Fcb->RFCB.FileSize.u.LowPart)
   {
      LARGE_INTEGER AllocationSize;
      AllocationSize.QuadPart = ByteOffset.u.LowPart + Length;
      Status = VfatSetAllocationSizeInformation(IrpContext->FileObject, Fcb,
	                                        IrpContext->DeviceExt, &AllocationSize);
      if (!NT_SUCCESS (Status))
      {
         CHECKPOINT;
         goto ByeBye;
      }
   }


   if (!(IrpContext->Irp->Flags & (IRP_NOCACHE|IRP_PAGING_IO)) &&
      !(Fcb->Flags & (FCB_IS_PAGE_FILE|FCB_IS_VOLUME)))
   {
      // cached write
      CHECKPOINT;

      if (IrpContext->FileObject->PrivateCacheMap == NULL)
      {
	  ULONG CacheSize;
	  CacheSize = IrpContext->DeviceExt->FatInfo.BytesPerCluster;
	  if (CacheSize < PAGE_SIZE)
	  {
	     CacheSize = PAGE_SIZE;
	  }
	  CcRosInitializeFileCache(IrpContext->FileObject, CacheSize);
      }
      if (ByteOffset.QuadPart > OldFileSize.QuadPart)
      {
          CcZeroData(IrpContext->FileObject, &OldFileSize, &ByteOffset, TRUE);
      }
      if (CcCopyWrite(IrpContext->FileObject, &ByteOffset, Length,
                      1 /*IrpContext->Flags & IRPCONTEXT_CANWAIT*/, Buffer))
      {
      	 IrpContext->Irp->IoStatus.Information = Length;
         Status = STATUS_SUCCESS;
      }
      else
      {
         Status = STATUS_UNSUCCESSFUL;
      }
      CHECKPOINT;
   }
   else
   {
      // non cached write
      CHECKPOINT;

      if (ByteOffset.QuadPart > OldFileSize.QuadPart)
      {
         CcZeroData(IrpContext->FileObject, &OldFileSize, &ByteOffset, TRUE);
      }

      Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoReadAccess);
      if (!NT_SUCCESS(Status))
        {
	  goto ByeBye;
	}

      Status = VfatWriteFileData(IrpContext, Length, ByteOffset);
      if (NT_SUCCESS(Status))
      {
         IrpContext->Irp->IoStatus.Information = Length;
      }
   }

   if (!(IrpContext->Irp->Flags & IRP_PAGING_IO) &&
      !(Fcb->Flags & (FCB_IS_FAT|FCB_IS_VOLUME)))
   {
      if(!(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
      {
         LARGE_INTEGER SystemTime, LocalTime;
         // set dates and times
         KeQuerySystemTime (&SystemTime);
         ExSystemTimeToLocalTime (&SystemTime, &LocalTime);
         FsdFileTimeToDosDateTime ((TIME*)&LocalTime, &Fcb->entry.UpdateDate,
		    	           &Fcb->entry.UpdateTime);
         Fcb->entry.AccessDate = Fcb->entry.UpdateDate;
         /* set date and times to dirty */
	 Fcb->Flags |= FCB_IS_DIRTY;
      }
   }

ByeBye:
   if (Resource)
   {
      ExReleaseResourceLite(Resource);
   }

   if (Status == STATUS_PENDING)
   {
      Status = VfatLockUserBuffer(IrpContext->Irp, Length, IoReadAccess);
      if (NT_SUCCESS(Status))
      {
         Status = VfatQueueRequest(IrpContext);
      }
      else
      {
         IrpContext->Irp->IoStatus.Status = Status;
         IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
         VfatFreeIrpContext(IrpContext);
      }
   }
   else
   {
      IrpContext->Irp->IoStatus.Status = Status;
      if (IrpContext->FileObject->Flags & FO_SYNCHRONOUS_IO &&
          !(IrpContext->Irp->Flags & IRP_PAGING_IO) && NT_SUCCESS(Status))
      {
         IrpContext->FileObject->CurrentByteOffset.QuadPart =
           ByteOffset.QuadPart + IrpContext->Irp->IoStatus.Information;
      }

      IoCompleteRequest(IrpContext->Irp,
                        (CCHAR)(NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
      VfatFreeIrpContext(IrpContext);
   }
   DPRINT("%x\n", Status);
   return Status;
}


