/* $Id: ncache.c,v 1.28 2004/04/10 22:35:25 gdalsnes Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/cont.c
 * PURPOSE:         Manages non-cached memory
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


/**********************************************************************
 * NAME       EXPORTED
 *  MmAllocateNonCachedMemory@4
 *
 * DESCRIPTION
 *  Allocates a virtual address range of noncached and cache
 * aligned memory.
 * 
 * ARGUMENTS
 * NumberOfBytes
 *  Size of region to allocate.
 *  
 * RETURN VALUE
 * The base address of the range on success;
 * NULL on failure.
 *
 * NOTE
 *  Description taken from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
PVOID STDCALL
MmAllocateNonCachedMemory(IN ULONG NumberOfBytes)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Attributes;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;
   MmLockAddressSpace(MmGetKernelAddressSpace());
   Result = NULL;
   Status = MmCreateMemoryArea (NULL,
                                MmGetKernelAddressSpace(),
                                MEMORY_AREA_NO_CACHE,
                                &Result,
                                NumberOfBytes,
                                0,
                                &marea,
                                FALSE,
                                FALSE,
                                BoundaryAddressMultiple);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(Status))
   {
      return (NULL);
   }
   Attributes = PAGE_READWRITE | PAGE_SYSTEM | PAGE_NOCACHE |
                PAGE_WRITETHROUGH;
   for (i = 0; i < (PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE); i++)
   {
      PHYSICAL_ADDRESS NPage;

      Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &NPage);
      MmCreateVirtualMapping (NULL,
                              (char*)Result + (i * PAGE_SIZE),
                              Attributes,
                              NPage,
                              TRUE);
   }
   return ((PVOID)Result);
}

VOID STATIC
MmFreeNonCachedPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                    PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry,
                    BOOLEAN Dirty)
{
   assert(SwapEntry == 0);
   if (PhysAddr.QuadPart != 0)
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PhysAddr);
   }
}

/**********************************************************************
 * NAME       EXPORTED
 * MmFreeNonCachedMemory@8
 *
 * DESCRIPTION
 * Releases a range of noncached memory allocated with 
 * MmAllocateNonCachedMemory.
 * 
 * ARGUMENTS
 * BaseAddress
 *  Virtual address to be freed;
 *  
 * NumberOfBytes
 *  Size of the region to be freed.
 *  
 * RETURN VALUE
 *  None.
 *
 * NOTE
 *  Description taken from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
VOID STDCALL MmFreeNonCachedMemory (IN PVOID BaseAddress,
                                    IN ULONG NumberOfBytes)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryArea (MmGetKernelAddressSpace(),
                     BaseAddress,
                     NumberOfBytes,
                     MmFreeNonCachedPage,
                     NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

/* EOF */
