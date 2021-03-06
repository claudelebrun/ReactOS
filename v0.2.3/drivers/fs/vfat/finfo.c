/* $Id: finfo.c,v 1.35 2004/05/23 13:34:32 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/finfo.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Hartmut Birr
 *
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
VfatGetStandardInformation(PVFATFCB FCB,
			   PFILE_STANDARD_INFORMATION StandardInfo,
			   PULONG BufferLength)
/*
 * FUNCTION: Retrieve the standard file information
 */
{

  if (*BufferLength < sizeof(FILE_STANDARD_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  /* PRECONDITION */
  assert (StandardInfo != NULL);
  assert (FCB != NULL);

  if (vfatFCBIsDirectory(FCB))
    {
      StandardInfo->AllocationSize.QuadPart = 0LL;
      StandardInfo->EndOfFile.QuadPart = 0LL;
      StandardInfo->Directory = TRUE;
    }
  else
    {
      StandardInfo->AllocationSize = FCB->RFCB.AllocationSize;
      StandardInfo->EndOfFile = FCB->RFCB.FileSize;
      StandardInfo->Directory = FALSE;
    }
  StandardInfo->NumberOfLinks = 0;
  StandardInfo->DeletePending = FCB->Flags & FCB_DELETE_PENDING ? TRUE : FALSE;

  *BufferLength -= sizeof(FILE_STANDARD_INFORMATION);
  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatSetPositionInformation(PFILE_OBJECT FileObject,
			   PFILE_POSITION_INFORMATION PositionInfo)
{
  DPRINT ("FsdSetPositionInformation()\n");

  DPRINT ("PositionInfo %x\n", PositionInfo);
  DPRINT ("Setting position %d\n", PositionInfo->CurrentByteOffset.u.LowPart);
  memcpy (&FileObject->CurrentByteOffset, &PositionInfo->CurrentByteOffset,
	  sizeof (LARGE_INTEGER));

  return (STATUS_SUCCESS);
}

static NTSTATUS
VfatGetPositionInformation(PFILE_OBJECT FileObject,
			   PVFATFCB FCB,
			   PDEVICE_OBJECT DeviceObject,
			   PFILE_POSITION_INFORMATION PositionInfo,
			   PULONG BufferLength)
{
  DPRINT ("VfatGetPositionInformation()\n");

  if (*BufferLength < sizeof(FILE_POSITION_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  PositionInfo->CurrentByteOffset.QuadPart =
    FileObject->CurrentByteOffset.QuadPart;

  DPRINT("Getting position %I64x\n",
	 PositionInfo->CurrentByteOffset.QuadPart);

  *BufferLength -= sizeof(FILE_POSITION_INFORMATION);
  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatSetBasicInformation(PFILE_OBJECT FileObject,
			PVFATFCB FCB,
			PDEVICE_EXTENSION DeviceExt,
			PFILE_BASIC_INFORMATION BasicInfo)
{
  DPRINT("VfatSetBasicInformation()\n");

  assert (NULL != FileObject);
  assert (NULL != FCB);
  assert (NULL != DeviceExt);
  assert (NULL != BasicInfo);
  /* Check volume label bit */
  assert(0 == (FCB->entry.Attrib & 0x08));

  FsdFileTimeToDosDateTime((TIME *)&(BasicInfo->CreationTime),
                           &(FCB->entry.CreationDate),  
                           &(FCB->entry.CreationTime));
  FsdFileTimeToDosDateTime((TIME *)&(BasicInfo->LastAccessTime),
                           &(FCB->entry.AccessDate),  
                           NULL);
  FsdFileTimeToDosDateTime((TIME *)&(BasicInfo->LastWriteTime),
                           &(FCB->entry.UpdateDate),
                           &(FCB->entry.UpdateTime));

  FCB->entry.Attrib = (unsigned char)((FCB->entry.Attrib &
                       (FILE_ATTRIBUTE_DIRECTORY | 0x48)) |
                      (BasicInfo->FileAttributes &
                       (FILE_ATTRIBUTE_ARCHIVE |
                        FILE_ATTRIBUTE_SYSTEM |
                        FILE_ATTRIBUTE_HIDDEN |
                        FILE_ATTRIBUTE_READONLY)));
  DPRINT("Setting attributes 0x%02x\n", FCB->entry.Attrib);

  VfatUpdateEntry(FCB);

  return(STATUS_SUCCESS);
}

static NTSTATUS
VfatGetBasicInformation(PFILE_OBJECT FileObject,
			PVFATFCB FCB,
			PDEVICE_OBJECT DeviceObject,
			PFILE_BASIC_INFORMATION BasicInfo,
			PULONG BufferLength)
{
  DPRINT("VfatGetBasicInformation()\n");

  if (*BufferLength < sizeof(FILE_BASIC_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;

  FsdDosDateTimeToFileTime(FCB->entry.CreationDate,
			   FCB->entry.CreationTime,
			   (TIME *)&BasicInfo->CreationTime);
  FsdDosDateTimeToFileTime(FCB->entry.AccessDate,
			   0,
			   (TIME *)&BasicInfo->LastAccessTime);
  FsdDosDateTimeToFileTime(FCB->entry.UpdateDate,
			   FCB->entry.UpdateTime,
			   (TIME *)&BasicInfo->LastWriteTime);
  BasicInfo->ChangeTime = BasicInfo->LastWriteTime;

  BasicInfo->FileAttributes = FCB->entry.Attrib & 0x3f;
  /* Synthesize FILE_ATTRIBUTE_NORMAL */
  if (0 == (BasicInfo->FileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_ARCHIVE |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN |
                                         FILE_ATTRIBUTE_READONLY)))
  {
    BasicInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
  }
  DPRINT("Getting attributes 0x%02x\n", BasicInfo->FileAttributes);

  *BufferLength -= sizeof(FILE_BASIC_INFORMATION);
  return(STATUS_SUCCESS);
}


static NTSTATUS
VfatSetDispositionInformation(PFILE_OBJECT FileObject,
			      PVFATFCB FCB,
			      PDEVICE_OBJECT DeviceObject,
			      PFILE_DISPOSITION_INFORMATION DispositionInfo)
{
  NTSTATUS Status = STATUS_SUCCESS;
  int count;

  PDEVICE_EXTENSION DeviceExt = DeviceObject->DeviceExtension;

  DPRINT ("FsdSetDispositionInformation()\n");

  assert (DeviceExt != NULL);
  assert (DeviceExt->FatInfo.BytesPerCluster != 0);
  assert (FCB != NULL);

  if (vfatFCBIsRoot(FCB) || 
     (FCB->LongNameU.Length == sizeof(WCHAR) && FCB->LongNameU.Buffer[0] == L'.') ||
     (FCB->LongNameU.Length == 2 * sizeof(WCHAR) && FCB->LongNameU.Buffer[0] == L'.' && FCB->LongNameU.Buffer[1] == L'.'))
    {
      // we cannot delete a '.', '..' or the root directory
      return STATUS_ACCESS_DENIED;
    }
  if (DispositionInfo->DoDeleteFile)
    {
      if (MmFlushImageSection (FileObject->SectionObjectPointer, MmFlushForDelete))
        {
          count = FCB->RefCount;
          if (FCB->RefCount > 1)
            {
	      DPRINT1("%d %x\n", FCB->RefCount, CcGetFileObjectFromSectionPtrs(FileObject->SectionObjectPointer));
              Status = STATUS_ACCESS_DENIED;
            }
          else
            {
              FCB->Flags |= FCB_DELETE_PENDING;
              FileObject->DeletePending = TRUE;
            }
        }
      else
        {
          DPRINT1("MmFlushImageSection returned FALSE\n");
          Status = STATUS_ACCESS_DENIED;
        }
      DPRINT("RefCount:%d\n", count);
      if (NT_SUCCESS(Status) && vfatFCBIsDirectory(FCB))
        {
          if (!VfatIsDirectoryEmpty(FCB))
            {
              Status = STATUS_DIRECTORY_NOT_EMPTY;
              FCB->Flags &= ~FCB_DELETE_PENDING;
              FileObject->DeletePending = FALSE;
            }
          else
            {
              Status = STATUS_SUCCESS;
            }
        }
     }
   else
     {
       FileObject->DeletePending = FALSE;
     }
  return Status;
}

static NTSTATUS
VfatGetNameInformation(PFILE_OBJECT FileObject,
		       PVFATFCB FCB,
		       PDEVICE_OBJECT DeviceObject,
		       PFILE_NAME_INFORMATION NameInfo,
		       PULONG BufferLength)
/*
 * FUNCTION: Retrieve the file name information
 */
{

  assert (NameInfo != NULL);
  assert (FCB != NULL);

  if (*BufferLength < sizeof(FILE_NAME_INFORMATION) + FCB->PathNameU.Length + sizeof(WCHAR))
    return STATUS_BUFFER_OVERFLOW;

  NameInfo->FileNameLength = FCB->PathNameU.Length;
  memcpy(NameInfo->FileName, FCB->PathNameU.Buffer, FCB->PathNameU.Length);
  NameInfo->FileName[FCB->PathNameU.Length / sizeof(WCHAR)] = 0;

  *BufferLength -= (sizeof(FILE_NAME_INFORMATION) + FCB->PathNameU.Length + sizeof(WCHAR));

  return STATUS_SUCCESS;
}

static NTSTATUS
VfatGetInternalInformation(PVFATFCB Fcb,
			   PFILE_INTERNAL_INFORMATION InternalInfo,
			   PULONG BufferLength)
{
  assert (InternalInfo);
  assert (Fcb);

  if (*BufferLength < sizeof(FILE_INTERNAL_INFORMATION))
    return STATUS_BUFFER_OVERFLOW;
  // FIXME: get a real index, that can be used in a create operation
  InternalInfo->IndexNumber.QuadPart = 0;
  *BufferLength -= sizeof(FILE_INTERNAL_INFORMATION);
  return STATUS_SUCCESS;
}


static NTSTATUS
VfatGetNetworkOpenInformation(PVFATFCB Fcb,
			      PFILE_NETWORK_OPEN_INFORMATION NetworkInfo,
			      PULONG BufferLength)
/*
 * FUNCTION: Retrieve the file network open information
 */
{
  assert (NetworkInfo);
  assert (Fcb);

  if (*BufferLength < sizeof(FILE_NETWORK_OPEN_INFORMATION))
    return(STATUS_BUFFER_OVERFLOW);

  FsdDosDateTimeToFileTime(Fcb->entry.CreationDate,
			   Fcb->entry.CreationTime,
			   &NetworkInfo->CreationTime);
  FsdDosDateTimeToFileTime(Fcb->entry.AccessDate,
			   0,
			   &NetworkInfo->LastAccessTime);
  FsdDosDateTimeToFileTime(Fcb->entry.UpdateDate,
			   Fcb->entry.UpdateTime,
			   &NetworkInfo->LastWriteTime);
  NetworkInfo->ChangeTime = NetworkInfo->LastWriteTime;
  if (vfatFCBIsDirectory(Fcb))
    {
      NetworkInfo->EndOfFile.QuadPart = 0L;
      NetworkInfo->AllocationSize.QuadPart = 0L;
    }
  else
    {
      NetworkInfo->AllocationSize = Fcb->RFCB.AllocationSize;
      NetworkInfo->EndOfFile = Fcb->RFCB.FileSize;
    }
  NetworkInfo->FileAttributes = Fcb->entry.Attrib & 0x3f;

  *BufferLength -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
  return STATUS_SUCCESS;
}


static NTSTATUS
VfatGetAllInformation(PFILE_OBJECT FileObject,
		      PVFATFCB Fcb,
		      PFILE_ALL_INFORMATION Info,
		      PULONG BufferLength)
/*
 * FUNCTION: Retrieve the all file information
 */
{

  assert (Info);
  assert (Fcb);

  if (*BufferLength < sizeof(FILE_ALL_INFORMATION) + Fcb->PathNameU.Length + sizeof(WCHAR))
    return(STATUS_BUFFER_OVERFLOW);

  /* Basic Information */
  FsdDosDateTimeToFileTime(Fcb->entry.CreationDate,
			   Fcb->entry.CreationTime,
			   (TIME *)&Info->BasicInformation.CreationTime);
  FsdDosDateTimeToFileTime(Fcb->entry.AccessDate,
			   0,
			   (TIME *)&Info->BasicInformation.LastAccessTime);
  FsdDosDateTimeToFileTime(Fcb->entry.UpdateDate,
			   Fcb->entry.UpdateTime,
			   (TIME *)&Info->BasicInformation.LastWriteTime);
  Info->BasicInformation.ChangeTime = Info->BasicInformation.LastWriteTime;
  Info->BasicInformation.FileAttributes = Fcb->entry.Attrib & 0x3f;

  /* Standard Information */
  if (vfatFCBIsDirectory(Fcb))
    {
      Info->StandardInformation.AllocationSize.QuadPart = 0LL;
      Info->StandardInformation.EndOfFile.QuadPart = 0LL;
      Info->StandardInformation.Directory = TRUE;
    }
  else
    {
      Info->StandardInformation.AllocationSize = Fcb->RFCB.AllocationSize;
      Info->StandardInformation.EndOfFile = Fcb->RFCB.FileSize;
      Info->StandardInformation.Directory = FALSE;
    }
  Info->StandardInformation.NumberOfLinks = 0;
  Info->StandardInformation.DeletePending = Fcb->Flags & FCB_DELETE_PENDING ? TRUE : FALSE;

  /* Internal Information */
  /* FIXME: get a real index, that can be used in a create operation */
  Info->InternalInformation.IndexNumber.QuadPart = 0;

  /* EA Information */
  Info->EaInformation.EaSize = 0;

  /* Access Information */
  /* The IO-Manager adds this information */

  /* Position Information */
  Info->PositionInformation.CurrentByteOffset.QuadPart = FileObject->CurrentByteOffset.QuadPart;

  /* Mode Information */
  /* The IO-Manager adds this information */

  /* Alignment Information */
  /* The IO-Manager adds this information */

  /* Name Information */
  Info->NameInformation.FileNameLength = Fcb->PathNameU.Length;
  RtlCopyMemory(Info->NameInformation.FileName, Fcb->PathNameU.Buffer, Fcb->PathNameU.Length);
  Info->NameInformation.FileName[Fcb->PathNameU.Length / sizeof(WCHAR)] = 0;

  *BufferLength -= (sizeof(FILE_ALL_INFORMATION) + Fcb->PathNameU.Length + sizeof(WCHAR));

  return STATUS_SUCCESS;
}

VOID UpdateFileSize(PFILE_OBJECT FileObject, PVFATFCB Fcb, ULONG Size, ULONG ClusterSize)
{
   if (Size > 0)
   {
      Fcb->RFCB.AllocationSize.QuadPart = ROUND_UP(Size, ClusterSize);
   }
   else
   {
      Fcb->RFCB.AllocationSize.QuadPart = (LONGLONG)0;
   }
   if (!vfatFCBIsDirectory(Fcb))
   {
      Fcb->entry.FileSize = Size;  
   }
   Fcb->RFCB.FileSize.QuadPart = Size;
   Fcb->RFCB.ValidDataLength.QuadPart = Size;

   if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
   {
      CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize);
   }
}

NTSTATUS
VfatSetAllocationSizeInformation(PFILE_OBJECT FileObject, 
				 PVFATFCB Fcb,
				 PDEVICE_EXTENSION DeviceExt,
				 PLARGE_INTEGER AllocationSize)
{
  ULONG OldSize;
  ULONG Cluster, FirstCluster;
  NTSTATUS Status;

  ULONG ClusterSize = DeviceExt->FatInfo.BytesPerCluster;
  ULONG NewSize = AllocationSize->u.LowPart;
  ULONG NCluster;
  BOOL AllocSizeChanged = FALSE;

  DPRINT("VfatSetAllocationSizeInformation()\n");

  OldSize = Fcb->entry.FileSize;
  if (AllocationSize->u.HighPart > 0)
  {
    return STATUS_INVALID_PARAMETER;
  }
  if (OldSize == NewSize)
    {
      return(STATUS_SUCCESS);
    }

  FirstCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Fcb->entry);
  
  if (NewSize > Fcb->RFCB.AllocationSize.u.LowPart)
  {
    AllocSizeChanged = TRUE;
    if (FirstCluster == 0)
    {
      Status = NextCluster (DeviceExt, FirstCluster, &FirstCluster, TRUE);
      if (!NT_SUCCESS(Status))
      {
	DPRINT1("NextCluster failed. Status = %x\n", Status);
	return Status;
      }
      if (FirstCluster == 0xffffffff)
      {
         return STATUS_DISK_FULL;
      }
      Status = OffsetToCluster(DeviceExt, FirstCluster, 
	         ROUND_DOWN(NewSize - 1, ClusterSize),
                 &NCluster, TRUE);
      if (NCluster == 0xffffffff || !NT_SUCCESS(Status))
      {
	 /* disk is full */
         NCluster = Cluster = FirstCluster;
	 Status = STATUS_SUCCESS;
         while (NT_SUCCESS(Status) && Cluster != 0xffffffff && Cluster > 1)
	 {
	    Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
            WriteCluster (DeviceExt, Cluster, 0);
	    Cluster = NCluster;
	 }
	 return STATUS_DISK_FULL;
      }
      Fcb->entry.FirstCluster = (unsigned short)(FirstCluster & 0x0000FFFF);
      Fcb->entry.FirstClusterHigh = (unsigned short)((FirstCluster & 0xFFFF0000) >> 16);
    }
    else
    {
       Status = OffsetToCluster(DeviceExt, FirstCluster, 
	          Fcb->RFCB.AllocationSize.u.LowPart - ClusterSize,
		  &Cluster, FALSE);
       /* FIXME: Check status */
       /* Cluster points now to the last cluster within the chain */
       Status = OffsetToCluster(DeviceExt, FirstCluster, 
	         ROUND_DOWN(NewSize - 1, ClusterSize),
                 &NCluster, TRUE);
       if (NCluster == 0xffffffff || !NT_SUCCESS(Status))
       {
	  /* disk is full */
	  NCluster = Cluster; 
          Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
	  WriteCluster(DeviceExt, Cluster, 0xffffffff);
	  Cluster = NCluster;
          while (NT_SUCCESS(Status) && Cluster != 0xffffffff && Cluster > 1)
	  {
	    Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
            WriteCluster (DeviceExt, Cluster, 0);
	    Cluster = NCluster;
	  }
	  return STATUS_DISK_FULL;
       }
    }
    UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize);
  }
  else if (NewSize + ClusterSize <= Fcb->RFCB.AllocationSize.u.LowPart)
  {
    AllocSizeChanged = TRUE;
    UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize);
    if (NewSize > 0)
    {
      Status = OffsetToCluster(DeviceExt, FirstCluster, 
	          ROUND_DOWN(NewSize - 1, ClusterSize),
		  &Cluster, FALSE);

      NCluster = Cluster;
      Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
      WriteCluster(DeviceExt, Cluster, 0xffffffff);
      Cluster = NCluster;
    }
    else
    {
      Fcb->entry.FirstCluster = 0;
      Fcb->entry.FirstClusterHigh = 0;

      NCluster = Cluster = FirstCluster;
      Status = STATUS_SUCCESS;
    }
    while (NT_SUCCESS(Status) && 0xffffffff != Cluster && Cluster > 1)
    {
       Status = NextCluster (DeviceExt, FirstCluster, &NCluster, FALSE);
       WriteCluster (DeviceExt, Cluster, 0);
       Cluster = NCluster;
    }
  }
  else
  {
     UpdateFileSize(FileObject, Fcb, NewSize, ClusterSize);
  }
  /* Update the on-disk directory entry */
  Fcb->Flags |= FCB_IS_DIRTY;
  if (AllocSizeChanged)
    {
      VfatUpdateEntry(Fcb);
    }
  return STATUS_SUCCESS;
}

NTSTATUS VfatQueryInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PVFATFCB FCB = NULL;

  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;
  ULONG BufferLength;

  /* PRECONDITION */
  assert (IrpContext);

  /* INITIALIZATION */
  FileInformationClass = IrpContext->Stack->Parameters.QueryFile.FileInformationClass;
  FCB = (PVFATFCB) IrpContext->FileObject->FsContext;

  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;
  BufferLength = IrpContext->Stack->Parameters.QueryFile.Length;

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     if (!ExAcquireResourceSharedLite(&FCB->MainResource,
                                      (BOOLEAN)(IrpContext->Flags & IRPCONTEXT_CANWAIT)))
     {
        return VfatQueueRequest (IrpContext);
     }
  }


  switch (FileInformationClass)
    {
    case FileStandardInformation:
      RC = VfatGetStandardInformation(FCB,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FilePositionInformation:
      RC = VfatGetPositionInformation(IrpContext->FileObject,
				      FCB,
				      IrpContext->DeviceObject,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileBasicInformation:
      RC = VfatGetBasicInformation(IrpContext->FileObject,
				   FCB,
				   IrpContext->DeviceObject,
				   SystemBuffer,
				   &BufferLength);
      break;
    case FileNameInformation:
      RC = VfatGetNameInformation(IrpContext->FileObject,
				  FCB,
				  IrpContext->DeviceObject,
				  SystemBuffer,
				  &BufferLength);
      break;
    case FileInternalInformation:
      RC = VfatGetInternalInformation(FCB,
				      SystemBuffer,
				      &BufferLength);
      break;
    case FileNetworkOpenInformation:
      RC = VfatGetNetworkOpenInformation(FCB,
					 SystemBuffer,
					 &BufferLength);
      break;
    case FileAllInformation:
      RC = VfatGetAllInformation(IrpContext->FileObject,
				 FCB,
				 SystemBuffer,
				 &BufferLength);
      break;

    case FileAlternateNameInformation:
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      RC = STATUS_NOT_SUPPORTED;
    }

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     ExReleaseResourceLite(&FCB->MainResource);
  }
  IrpContext->Irp->IoStatus.Status = RC;
  if (NT_SUCCESS(RC))
    IrpContext->Irp->IoStatus.Information =
      IrpContext->Stack->Parameters.QueryFile.Length - BufferLength;
  else
    IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return RC;
}

NTSTATUS VfatSetInformation(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Retrieve the specified file information
 */
{
  FILE_INFORMATION_CLASS FileInformationClass;
  PVFATFCB FCB = NULL;
  NTSTATUS RC = STATUS_SUCCESS;
  PVOID SystemBuffer;
  BOOL CanWait = IrpContext->Flags & IRPCONTEXT_CANWAIT;
  
  /* PRECONDITION */
  assert(IrpContext);
  
  DPRINT("VfatSetInformation(IrpContext %x)\n", IrpContext);
  
  /* INITIALIZATION */
  FileInformationClass = 
    IrpContext->Stack->Parameters.SetFile.FileInformationClass;
  FCB = (PVFATFCB) IrpContext->FileObject->FsContext;
  SystemBuffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;
  
  DPRINT("FileInformationClass %d\n", FileInformationClass);
  DPRINT("SystemBuffer %x\n", SystemBuffer);

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
    {
      if (!ExAcquireResourceExclusiveLite(&FCB->MainResource,
                                          (BOOLEAN)CanWait))
	{
	  return(VfatQueueRequest (IrpContext));
	}
    }

  switch (FileInformationClass)
    {
    case FilePositionInformation:
      RC = VfatSetPositionInformation(IrpContext->FileObject,
				      SystemBuffer);
      break;
    case FileDispositionInformation:
      RC = VfatSetDispositionInformation(IrpContext->FileObject,
					 FCB,
					 IrpContext->DeviceObject,
					 SystemBuffer);
      break;
    case FileAllocationInformation:    
    case FileEndOfFileInformation:
      RC = VfatSetAllocationSizeInformation(IrpContext->FileObject,
					    FCB,
					    IrpContext->DeviceExt,
					    (PLARGE_INTEGER)SystemBuffer);
      break;    
    case FileBasicInformation:
      RC = VfatSetBasicInformation(IrpContext->FileObject,
				   FCB,
				   IrpContext->DeviceExt,
				   SystemBuffer);
      break;
    case FileRenameInformation:
      RC = STATUS_NOT_IMPLEMENTED;
      break;
    default:
      RC = STATUS_NOT_SUPPORTED;
    }

  if (!(FCB->Flags & FCB_IS_PAGE_FILE))
  {
     ExReleaseResourceLite(&FCB->MainResource);
  }

  IrpContext->Irp->IoStatus.Status = RC;
  IrpContext->Irp->IoStatus.Information = 0;
  IoCompleteRequest(IrpContext->Irp, IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);

  return RC;
}

/* EOF */
