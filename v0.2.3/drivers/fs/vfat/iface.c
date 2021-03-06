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
/* $Id: iface.c,v 1.74 2004/02/10 16:22:56 navaraf Exp $
 *
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/iface.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Hartmut Birr
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* GLOBALS *****************************************************************/

PVFAT_GLOBAL_DATA VfatGlobalData;

/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
DriverEntry(PDRIVER_OBJECT DriverObject,
	    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Called by the system to initalize the driver
 * ARGUMENTS:
 *           DriverObject = object describing this driver
 *           RegistryPath = path to our configuration entries
 * RETURNS: Success or failure
 */
{
   PDEVICE_OBJECT DeviceObject;
   UNICODE_STRING DeviceName = ROS_STRING_INITIALIZER(L"\\Fat");
   NTSTATUS Status;

   Status = IoCreateDevice(DriverObject,
			   sizeof(VFAT_GLOBAL_DATA),
			   &DeviceName,
			   FILE_DEVICE_DISK_FILE_SYSTEM,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
     {
	return (Status);
     }
   VfatGlobalData = DeviceObject->DeviceExtension;
   RtlZeroMemory (VfatGlobalData, sizeof(VFAT_GLOBAL_DATA));
   VfatGlobalData->DriverObject = DriverObject;
   VfatGlobalData->DeviceObject = DeviceObject;

   DeviceObject->Flags = DO_DIRECT_IO;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_CREATE] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_READ] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_WRITE] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_FILE_SYSTEM_CONTROL] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_DIRECTORY_CONTROL] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_QUERY_VOLUME_INFORMATION] = 
     VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_SET_VOLUME_INFORMATION] = 
     VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = VfatShutdown;
   DriverObject->MajorFunction[IRP_MJ_LOCK_CONTROL] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VfatBuildRequest;
   DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = VfatBuildRequest;

   DriverObject->DriverUnload = NULL;

   ExInitializeNPagedLookasideList(&VfatGlobalData->FcbLookasideList, 
                                   NULL, NULL, 0, sizeof(VFATFCB), TAG_FCB, 0);
   ExInitializeNPagedLookasideList(&VfatGlobalData->CcbLookasideList, 
                                   NULL, NULL, 0, sizeof(VFATCCB), TAG_CCB, 0);
   ExInitializeNPagedLookasideList(&VfatGlobalData->IrpContextLookasideList, 
                                   NULL, NULL, 0, sizeof(VFAT_IRP_CONTEXT), TAG_IRP, 0);

   ExInitializeResourceLite(&VfatGlobalData->VolumeListLock);
   InitializeListHead(&VfatGlobalData->VolumeListHead);
   IoRegisterFileSystem(DeviceObject);
   return(STATUS_SUCCESS);
}

/* EOF */

