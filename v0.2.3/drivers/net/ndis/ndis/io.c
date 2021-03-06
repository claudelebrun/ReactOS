/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/io.c
 * PURPOSE:     I/O related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   20 Aug 2003 Vizzini - DMA support
 *   3  Oct 2003 Vizzini - Formatting and minor bugfixes
 */
#include <ndissys.h>
#include <miniport.h>


VOID STDCALL HandleDeferredProcessing(
    IN  PKDPC   Dpc,
    IN  PVOID   DeferredContext,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2)
/*
 * FUNCTION: Deferred interrupt processing routine
 * ARGUMENTS:
 *     Dpc             = Pointer to DPC object
 *     DeferredContext = Pointer to context information (LOGICAL_ADAPTER)
 *     SystemArgument1 = Unused
 *     SystemArgument2 = Unused
 */
{
  BOOLEAN WasBusy;
  PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(DeferredContext);

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

  /* XXX try to grok WasBusy */
  KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    {
      WasBusy = Adapter->MiniportBusy;
      Adapter->MiniportBusy = TRUE;
    }
  KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

  /* Call the deferred interrupt service handler for this adapter */
  (*Adapter->Miniport->Chars.HandleInterruptHandler)(
      Adapter->NdisMiniportBlock.MiniportAdapterContext);

  KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    {
      if ((!WasBusy) && (Adapter->WorkQueueHead)) 
        {
          KeInsertQueueDpc(&Adapter->MiniportDpc, NULL, NULL);
        } 
      else 
        {
          Adapter->MiniportBusy = WasBusy;
        }
    }
  KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

  /* re-enable the interrupt */
  NDIS_DbgPrint(MAX_TRACE, ("re-enabling the interrupt\n"));
  if(Adapter->Miniport->Chars.EnableInterruptHandler)
    (*Adapter->Miniport->Chars.EnableInterruptHandler)(
        Adapter->NdisMiniportBlock.MiniportAdapterContext);

  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


BOOLEAN STDCALL ServiceRoutine(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       ServiceContext)
/*
 * FUNCTION: Interrupt service routine
 * ARGUMENTS:
 *     Interrupt      = Pointer to interrupt object
 *     ServiceContext = Pointer to context information (LOGICAL_ADAPTER)
 * RETURNS
 *     TRUE if a miniport controlled device generated the interrupt
 */
{
  BOOLEAN InterruptRecognized;
  BOOLEAN QueueMiniportHandleInterrupt;
  PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(ServiceContext);

  NDIS_DbgPrint(MAX_TRACE, ("Called. Adapter (0x%X)\n", Adapter));

  (*Adapter->Miniport->Chars.ISRHandler)(&InterruptRecognized,
      &QueueMiniportHandleInterrupt,
      Adapter->NdisMiniportBlock.MiniportAdapterContext);

  if (QueueMiniportHandleInterrupt) 
    {
      NDIS_DbgPrint(MAX_TRACE, ("Queueing DPC.\n"));
      KeInsertQueueDpc(&Adapter->NdisMiniportBlock.Interrupt->InterruptDpc, NULL, NULL);
    }

  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return InterruptRecognized;
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisCompleteDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    NdisDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisFlushBuffer(
    IN  PNDIS_BUFFER    Buffer,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
ULONG
EXPORT
NdisGetCacheFillSize(
    VOID)
{
    UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateReadPortUchar(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PUCHAR      Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  *Data = READ_PORT_UCHAR((PUCHAR)Port); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateReadPortUlong(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PULONG      Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  *Data = READ_PORT_ULONG((PULONG)Port); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateReadPortUshort(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    OUT PUSHORT     Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  *Data = READ_PORT_USHORT((PUSHORT)Port); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateWritePortUchar(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  UCHAR       Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  WRITE_PORT_UCHAR((PUCHAR)Port, Data); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @implemented
 */
VOID
EXPORT
NdisImmediateWritePortUlong(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  ULONG       Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  WRITE_PORT_ULONG((PULONG)Port, Data); // FIXME: What to do with WrapperConfigurationContext?
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisImmediateWritePortUshort(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       Port,
    IN  USHORT      Data)
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  WRITE_PORT_USHORT((PUSHORT)Port, Data); // FIXME: What to do with WrapperConfigurationContext?
}


IO_ALLOCATION_ACTION STDCALL NdisMapRegisterCallback (
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP            Irp,
    IN PVOID           MapRegisterBase,
    IN PVOID           Context)
/*
 * FUNCTION: Called back during reservation of map registers
 * ARGUMENTS:
 *     DeviceObject: Device object of the deivce setting up DMA
 *     Irp: Reserved; must be ignored
 *     MapRegisterBase: Map registers assigned for transfer
 *     Context: LOGICAL_ADAPTER object of the requesting miniport
 * NOTES:       
 *     - Called once per BaseMapRegister (see NdisMAllocateMapRegisters)
 *     - Called at IRQL = DISPATCH_LEVEL
 */
{
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)Context;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  Adapter->NdisMiniportBlock.MapRegisters[Adapter->NdisMiniportBlock.CurrentMapRegister].MapRegister = MapRegisterBase;

  NDIS_DbgPrint(MAX_TRACE, ("setting event and leaving.\n"));

  KeSetEvent(&Adapter->DmaEvent, 0, FALSE);

  /* this is only the thing to do for busmaster NICs */
  return DeallocateObjectKeepRegisters;
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMAllocateMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        DmaChannel,
    IN  BOOLEAN     DmaSize,
    IN  ULONG       BaseMapRegistersNeeded,
    IN  ULONG       MaximumBufferSize)
/*
 * FUNCTION: Allocate map registers for use in DMA transfers
 * ARGUMENTS:
 *     MiniportAdapterHandle: Passed in to MiniportInitialize
 *     DmaChannel: DMA channel to use
 *     DmaSize: bit width of DMA transfers
 *     BaseMapRegistersNeeded: number of base map registers requested
 *     MaximumBufferSize: largest single buffer transferred
 * RETURNS:
 *     NDIS_STATUS_SUCCESS on success
 *     NDIS_STATUS_RESOURCES on failure
 * NOTES: 
 *     - the win2k ddk and the nt4 ddk have conflicting prototypes for this.
 *       I'm implementing the 2k one.
 *     - do not confuse a "base map register" with a "map register" - they
 *       are different.  Only NDIS seems to use the base concept.  The idea
 *       is that a miniport supplies the number of base map registers it will
 *       need, which is equal to the number of DMA send buffers it manages.
 *       NDIS then allocates a number of map registers to go with each base
 *       map register, so that a driver just has to send the base map register
 *       number during dma operations and NDIS can find the group of real
 *       map registers that represent the transfer.
 *     - Because of the above sillyness, you can only specify a few base map
 *       registers at most.  a 1514-byte packet is two map registers at 4k
 *       page size.  
 *     - NDIS limits the total number of allocated map registers to 64,
 *       which (in the case of the above example) limits the number of base
 *       map registers to 32.
 */
{
  DEVICE_DESCRIPTION Description;
  PADAPTER_OBJECT    AdapterObject = 0;
  UINT               MapRegistersRequired = 0;
  UINT               MapRegistersPerBaseRegister = 0;
  ULONG              AvailableMapRegisters;
  NTSTATUS           NtStatus;
  PLOGICAL_ADAPTER   Adapter = 0;
  PDEVICE_OBJECT     DeviceObject = 0;
  KIRQL              OldIrql;

  NDIS_DbgPrint(MAX_TRACE, ("called: Handle 0x%x, DmaChannel 0x%x, DmaSize 0x%x, BaseMapRegsNeeded: 0x%x, MaxBuffer: 0x%x.\n",
                            MiniportAdapterHandle, DmaChannel, DmaSize, BaseMapRegistersNeeded, MaximumBufferSize));

  memset(&Description,0,sizeof(Description));

  Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;

  ASSERT(Adapter);

  /* only bus masters may call this routine */
  ASSERT(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER);
  if(!Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER)
    return NDIS_STATUS_SUCCESS;

  DeviceObject = Adapter->NdisMiniportBlock.DeviceObject;

  KeInitializeEvent(&Adapter->DmaEvent, NotificationEvent, FALSE);
  KeInitializeSpinLock(&Adapter->DmaLock);

  /*
  * map registers correlate to physical pages.  ndis documents a
  * maximum of 64 map registers that it will return.  
  * at 4k pages, a 1514-byte buffer can span not more than 2 pages.
  *
  * the number of registers required for a given physical mapping
  * is (first register + last register + one per page size), 
  * given that physical mapping is > 2.
  */

  /* unhandled corner case: {1,2}-byte max buffer size */
  ASSERT(MaximumBufferSize > 2);
  MapRegistersPerBaseRegister = ((MaximumBufferSize-2) / PAGE_SIZE) + 2;
  MapRegistersRequired = BaseMapRegistersNeeded * MapRegistersPerBaseRegister;

  if(MapRegistersRequired > 64)
    {
      NDIS_DbgPrint(MID_TRACE, ("Request for too many map registers: %d\n", MapRegistersRequired));
      return NDIS_STATUS_RESOURCES;
    }

  Description.Version = DEVICE_DESCRIPTION_VERSION;
  Description.Master = TRUE;                         /* implied by calling this function */
  Description.ScatterGather = TRUE;                  /* XXX UNTRUE: All BM DMA are S/G (ms seems to do this) */
  Description.Dma32BitAddresses = DmaSize;
  Description.BusNumber = Adapter->BusNumber;
  Description.InterfaceType = Adapter->BusType;
  Description.DmaChannel = DmaChannel;
  Description.MaximumLength = MaximumBufferSize;

  if(Adapter->NdisMiniportBlock.AdapterType == Isa)
    {
      /* system dma */
      if(DmaChannel < 4)
        Description.DmaWidth = Width8Bits;
      else
        Description.DmaWidth = Width16Bits;

      Description.DmaSpeed = Compatible;
    }
  else if(Adapter->NdisMiniportBlock.AdapterType == PCIBus)
    {
      if(DmaSize == NDIS_DMA_64BITS)
        Description.Dma64BitAddresses = TRUE;
      else
        Description.Dma32BitAddresses = TRUE;
    }
  else
    {
      NDIS_DbgPrint(MIN_TRACE, ("Unsupported bus type\n"));
      ASSERT(0);
    }

  AvailableMapRegisters = MapRegistersRequired;
  AdapterObject = HalGetAdapter(&Description, &AvailableMapRegisters);

  if(!AdapterObject)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Unable to allocate an adapter object; bailing out\n"));
      return NDIS_STATUS_RESOURCES;
    }

  Adapter->NdisMiniportBlock.SystemAdapterObject = AdapterObject;

  if(AvailableMapRegisters < MapRegistersRequired)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Didn't get enough map registers from hal - requested 0x%x, got 0x%x\n", 
          MapRegistersRequired, AvailableMapRegisters));

      return NDIS_STATUS_RESOURCES;
    }

  /* allocate & zero space in the miniport block for the registers */
  Adapter->NdisMiniportBlock.MapRegisters = ExAllocatePool(NonPagedPool, BaseMapRegistersNeeded * sizeof(MAP_REGISTER_ENTRY));
  if(!Adapter->NdisMiniportBlock.MapRegisters)
    {
      NDIS_DbgPrint(MIN_TRACE, ("insufficient resources.\n"));
      return NDIS_STATUS_RESOURCES;
    }

  memset(Adapter->NdisMiniportBlock.MapRegisters, 0, BaseMapRegistersNeeded * sizeof(MAP_REGISTER_ENTRY));

  while(BaseMapRegistersNeeded)
    {
      NDIS_DbgPrint(MAX_TRACE, ("iterating, basemapregistersneeded = %d, IoAlloc = 0x%x\n", BaseMapRegistersNeeded, IoAllocateAdapterChannel));

      BaseMapRegistersNeeded--;
      Adapter->NdisMiniportBlock.CurrentMapRegister = BaseMapRegistersNeeded;
      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        {
          NtStatus = IoAllocateAdapterChannel(AdapterObject, DeviceObject, 
              MapRegistersPerBaseRegister, NdisMapRegisterCallback, Adapter);
        }
      KeLowerIrql(OldIrql);

      if(!NT_SUCCESS(NtStatus))
        {
          NDIS_DbgPrint(MIN_TRACE, ("IoAllocateAdapterChannel failed: 0x%x\n", NtStatus));
          return NDIS_STATUS_RESOURCES;
        }

      NDIS_DbgPrint(MAX_TRACE, ("waiting on event\n"));

      NtStatus = KeWaitForSingleObject(&Adapter->DmaEvent, Executive, KernelMode, FALSE, 0);

      if(!NT_SUCCESS(NtStatus))
        {
          NDIS_DbgPrint(MIN_TRACE, ("KeWaitForSingleObject failed: 0x%x\n", NtStatus));
          return NDIS_STATUS_RESOURCES;
        }

      NDIS_DbgPrint(MAX_TRACE, ("resetting event\n"));

      KeResetEvent(&Adapter->DmaEvent);
    }

  NDIS_DbgPrint(MAX_TRACE, ("returning success\n"));
  return NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMStartBufferPhysicalMapping(
    IN  NDIS_HANDLE                 MiniportAdapterHandle,
    IN  PNDIS_BUFFER                Buffer,
    IN  ULONG                       PhysicalMapRegister,
    IN  BOOLEAN                     WriteToDevice,
    OUT PNDIS_PHYSICAL_ADDRESS_UNIT	PhysicalAddressArray,
    OUT PUINT                       ArraySize)
/*
 * FUNCTION: Sets up map registers for a bus-master DMA transfer
 * ARGUMENTS:
 *     MiniportAdapterHandle: handle originally input to MiniportInitialize
 *     Buffer: data to be transferred 
 *     PhysicalMapRegister: specifies the map register to set up
 *     WriteToDevice: if true, data is being written to the device; else it is being read
 *     PhysicalAddressArray: list of mapped ranges suitable for DMA with the device
 *     ArraySize: number of elements in PhysicalAddressArray
 * NOTES:
 *     - Must be called at IRQL <= DISPATCH_LEVEL
 *     - The basic idea:  call IoMapTransfer() in a loop as many times as it takes
 *       in order to map all of the virtual memory to physical memoroy readable
 *       by the device
 *     - The caller supplies storage for the physical address array.  
 */
{
  PLOGICAL_ADAPTER Adapter = 0;
  VOID *CurrentVa;
  ULONG TotalLength;
  PHYSICAL_ADDRESS ReturnedAddress;
  UINT LoopCount =  0;

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  ASSERT(MiniportAdapterHandle && Buffer && PhysicalAddressArray && ArraySize);

  Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  CurrentVa = MmGetMdlVirtualAddress(Buffer);
  TotalLength = MmGetMdlByteCount(Buffer);

  while(TotalLength)
    {
      ULONG Length = TotalLength;

      ReturnedAddress = IoMapTransfer(Adapter->NdisMiniportBlock.SystemAdapterObject, Buffer, 
          Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].MapRegister, 
          CurrentVa, &Length, WriteToDevice);

      Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].WriteToDevice = WriteToDevice;

      PhysicalAddressArray[LoopCount].PhysicalAddress = ReturnedAddress;
      PhysicalAddressArray[LoopCount].Length = Length;

      TotalLength -= Length;
      CurrentVa += Length;

      LoopCount++;
    }

  *ArraySize = LoopCount;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMCompleteBufferPhysicalMapping(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           PhysicalMapRegister)
/*
 * FUNCTION: Complete dma action started by NdisMStartBufferPhysicalMapping
 * ARGUMENTS:
 *     - MiniportAdapterHandle: handle originally input to MiniportInitialize
 *     - Buffer: NDIS_BUFFER to complete the mapping on
 *     - PhyscialMapRegister: the chosen map register
 * NOTES:
 *     - May be called at IRQL <= DISPATCH_LEVEL
 */
{
  PLOGICAL_ADAPTER Adapter = 0;
  VOID *CurrentVa;
  ULONG Length;

  ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
  ASSERT(MiniportAdapterHandle && Buffer);

  Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  CurrentVa = MmGetMdlVirtualAddress(Buffer);
  Length = MmGetMdlByteCount(Buffer);

  IoFlushAdapterBuffers(Adapter->NdisMiniportBlock.SystemAdapterObject, Buffer, 
      Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].MapRegister,
      CurrentVa, Length, 
      Adapter->NdisMiniportBlock.MapRegisters[PhysicalMapRegister].WriteToDevice);
}



/*
 * @unimplemented
 */
VOID
EXPORT
NdisMCompleteDmaTransfer(
    OUT PNDIS_STATUS    Status,
    IN  PNDIS_HANDLE    MiniportDmaHandle,
    IN  PNDIS_BUFFER    Buffer,
    IN  ULONG           Offset,
    IN  ULONG           Length,
    IN  BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMDeregisterDmaChannel(
    IN  PNDIS_HANDLE    MiniportDmaHandle)
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMDeregisterInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt)
/*
 * FUNCTION: Releases an interrupt vector
 * ARGUMENTS:
 *     Interrupt = Pointer to interrupt object
 */
{
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
    IoDisconnectInterrupt(Interrupt->InterruptObject);
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMDeregisterIoPortRange(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts,
    IN  PVOID       PortOffset)
/*
 * FUNCTION: Releases a register mapping to I/O ports
 * ARGUMENTS:
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InitialPort           = Bus-relative base port address of a range to be mapped
 *     NumberOfPorts         = Specifies number of ports to be mapped
 *     PortOffset            = Pointer to mapped base port address
 */
{
  NDIS_DbgPrint(MAX_TRACE, ("called - IMPLEMENT ME.\n"));
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMFreeMapRegisters(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION: Free previously allocated map registers
 * ARGUMENTS:
 *     MiniportAdapterHandle:  Handle originally passed in to MiniportInitialize
 * NOTES:
 */
{
  KIRQL            OldIrql;
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
  PADAPTER_OBJECT  AdapterObject;
  UINT             MapRegistersPerBaseRegister;
  UINT             i;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(Adapter);

  /* only bus masters may call this routine */
  ASSERT(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER);
  if(!Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_BUS_MASTER)
    return;

  MapRegistersPerBaseRegister = ((Adapter->NdisMiniportBlock.MaximumPhysicalMapping - 2) / PAGE_SIZE) + 2;

  AdapterObject = Adapter->NdisMiniportBlock.SystemAdapterObject;

  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    {
      for(i = 0; i < Adapter->NdisMiniportBlock.PhysicalMapRegistersNeeded; i++)
        {
          IoFreeMapRegisters(Adapter->NdisMiniportBlock.SystemAdapterObject, 
              Adapter->NdisMiniportBlock.MapRegisters[i].MapRegister, MapRegistersPerBaseRegister);
        }
    }
 KeLowerIrql(OldIrql);

 ExFreePool(Adapter->NdisMiniportBlock.MapRegisters);
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMMapIoSpace(
    OUT PVOID                   *VirtualAddress,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  NDIS_PHYSICAL_ADDRESS   PhysicalAddress,
    IN  UINT                    Length)
/*
 * FUNCTION: Maps a bus-relative address to a system-wide virtual address
 * ARGUMENTS:
 *     VirtualAddress: receives virtual address of mapping
 *     MiniportAdapterHandle: Handle originally input to MiniportInitialize
 *     PhysicalAddress: bus-relative address to map
 *     Length: Number of bytes to map
 * RETURNS:
 *     NDIS_STATUS_SUCCESS: the operation completed successfully
 *     NDIS_STATUS_RESOURCE_CONFLICT: the physical address range is already claimed
 *     NDIS_STATUS_RESOURCES: insufficient resources to complete the mapping
 *     NDIS_STATUS_FAILURE: a general failure has occured
 * NOTES:
 *     - Must be called at IRQL = PASSIVE_LEVEL
 * BUGS:
 *     - Only supports things that MmMapIoSpace internally supports - what
 *       about considering bus type, etc?
 *     - doesn't track resources allocated...
 */
{
  PAGED_CODE();
  ASSERT(VirtualAddress && MiniportAdapterHandle);

  *VirtualAddress = MmMapIoSpace(PhysicalAddress, Length, MmNonCached);

  if(!*VirtualAddress)
    return NDIS_STATUS_RESOURCES;

  return NDIS_STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
ULONG
EXPORT
NdisMReadDmaCounter(
    IN  NDIS_HANDLE MiniportDmaHandle)
{
    UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterDmaChannel(
    OUT PNDIS_HANDLE            MiniportDmaHandle,
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  UINT                    DmaChannel,
    IN  BOOLEAN                 Dma32BitAddresses,
    IN  PNDIS_DMA_DESCRIPTION   DmaDescription,
    IN  ULONG                   MaximumLength)
{
    UNIMPLEMENTED

  return NDIS_STATUS_FAILURE;
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterInterrupt(
    OUT PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  NDIS_HANDLE                 MiniportAdapterHandle,
    IN  UINT                        InterruptVector,
    IN  UINT                        InterruptLevel,
    IN  BOOLEAN	                    RequestIsr,
    IN  BOOLEAN                     SharedInterrupt,
    IN  NDIS_INTERRUPT_MODE         InterruptMode)
/*
 * FUNCTION: Claims access to an interrupt vector
 * ARGUMENTS:
 *     Interrupt             = Address of interrupt object to initialize
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InterruptVector       = Specifies bus-relative vector to register
 *     InterruptLevel        = Specifies bus-relative DIRQL vector for interrupt
 *     RequestIsr            = TRUE if MiniportISR should always be called
 *     SharedInterrupt       = TRUE if other devices may use the same interrupt
 *     InterruptMode         = Specifies type of interrupt
 * RETURNS:
 *     Status of operation
 */
{
  NTSTATUS Status;
  ULONG MappedIRQ;
  KIRQL DIrql;
  KAFFINITY Affinity;
  PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

  NDIS_DbgPrint(MAX_TRACE, ("Called. InterruptVector (0x%X)  InterruptLevel (0x%X)  "
      "SharedInterrupt (%d)  InterruptMode (0x%X)\n",
      InterruptVector, InterruptLevel, SharedInterrupt, InterruptMode));

  RtlZeroMemory(Interrupt, sizeof(NDIS_MINIPORT_INTERRUPT));

  KeInitializeSpinLock(&Interrupt->DpcCountLock);

  KeInitializeDpc(&Interrupt->InterruptDpc, HandleDeferredProcessing, Adapter);

  KeInitializeEvent(&Interrupt->DpcsCompletedEvent, NotificationEvent, FALSE);

  Interrupt->SharedInterrupt = SharedInterrupt;

  Adapter->NdisMiniportBlock.Interrupt = Interrupt;

  MappedIRQ = HalGetInterruptVector(Adapter->BusType, Adapter->BusNumber, InterruptLevel, InterruptVector, &DIrql, &Affinity);

  NDIS_DbgPrint(MAX_TRACE, ("Connecting to interrupt vector (0x%X)  Affinity (0x%X).\n", MappedIRQ, Affinity));

  Status = IoConnectInterrupt(&Interrupt->InterruptObject, ServiceRoutine, Adapter, &Interrupt->DpcCountLock, MappedIRQ,
      DIrql, DIrql, InterruptMode, SharedInterrupt, Affinity, FALSE);

  NDIS_DbgPrint(MAX_TRACE, ("Leaving. Status (0x%X).\n", Status));

  if (NT_SUCCESS(Status))
    return NDIS_STATUS_SUCCESS;

  if (Status == STATUS_INSUFFICIENT_RESOURCES) 
    {
        /* FIXME: Log error */
      NDIS_DbgPrint(MIN_TRACE, ("Resource conflict!\n"));
      return NDIS_STATUS_RESOURCE_CONFLICT;
    }

  NDIS_DbgPrint(MIN_TRACE, ("Function failed\n"));
  return NDIS_STATUS_FAILURE;
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterIoPortRange(
    OUT PVOID       *PortOffset,
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  UINT        InitialPort,
    IN  UINT        NumberOfPorts)
/*
 * FUNCTION: Sets up driver access to device I/O ports
 * ARGUMENTS:
 *     PortOffset            = Address of buffer to place mapped base port address
 *     MiniportAdapterHandle = Specifies handle input to MiniportInitialize
 *     InitialPort           = Bus-relative base port address of a range to be mapped
 *     NumberOfPorts         = Specifies number of ports to be mapped
 * RETURNS:
 *     Status of operation
 */
{
  PHYSICAL_ADDRESS PortAddress, TranslatedAddress;
  PLOGICAL_ADAPTER Adapter  = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);
  ULONG            AddressSpace = 1;    /* FIXME The HAL handles this wrong atm */

  NDIS_DbgPrint(MAX_TRACE, ("Called - InitialPort 0x%x, NumberOfPorts 0x%x\n", InitialPort, NumberOfPorts));

  memset(&PortAddress, 0, sizeof(PortAddress));

  /* this might be a hack - ndis5 miniports seem to specify 0 */
  if(InitialPort)
      PortAddress = RtlConvertUlongToLargeInteger(InitialPort);
  else
      PortAddress = Adapter->BaseIoAddress;

  NDIS_DbgPrint(MAX_TRACE, ("Translating address 0x%x 0x%x\n", PortAddress.u.HighPart, PortAddress.u.LowPart));

  /* FIXME: hard-coded bus number */
  if(!HalTranslateBusAddress(Adapter->BusType, 0, PortAddress, &AddressSpace, &TranslatedAddress))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Unable to translate address\n"));
      return NDIS_STATUS_RESOURCES;
    }

  NDIS_DbgPrint(MAX_TRACE, ("Hal returned AddressSpace=0x%x TranslatedAddress=0x%x 0x%x\n",
                            AddressSpace, TranslatedAddress.u.HighPart, TranslatedAddress.u.LowPart));

  if(AddressSpace)
    {
      ASSERT(TranslatedAddress.u.HighPart == 0);
      *PortOffset = (PVOID) TranslatedAddress.u.LowPart;
      NDIS_DbgPrint(MAX_TRACE, ("Returning 0x%x\n", *PortOffset));
      return NDIS_STATUS_SUCCESS;
    }

  NDIS_DbgPrint(MAX_TRACE, ("calling MmMapIoSpace\n"));

  *PortOffset = 0;
  *PortOffset = MmMapIoSpace(TranslatedAddress, NumberOfPorts, MmNonCached);
  NDIS_DbgPrint(MAX_TRACE, ("Returning 0x%x for port range\n", *PortOffset));

  if(!*PortOffset)
    return NDIS_STATUS_RESOURCES;

  return NDIS_STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID
EXPORT
NdisMSetupDmaTransfer(
    OUT	PNDIS_STATUS    Status,
    IN	PNDIS_HANDLE    MiniportDmaHandle,
    IN	PNDIS_BUFFER    Buffer,
    IN	ULONG           Offset,
    IN	ULONG           Length,
    IN	BOOLEAN         WriteToDevice)
{
    UNIMPLEMENTED
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMUnmapIoSpace(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  PVOID       VirtualAddress,
    IN  UINT        Length)
/*
 * FUNCTION: Un-maps space previously mapped with NdisMMapIoSpace
 * ARGUMENTS:
 *     MiniportAdapterHandle: handle originally passed into MiniportInitialize
 *     VirtualAddress: Address to un-map
 *     Length: length of the mapped memory space
 * NOTES:
 *     - Must be called at IRQL = PASSIVE_LEVEL
 *     - Must only be called from MiniportInitialize and MiniportHalt
 *     - See also: NdisMMapIoSpace
 * BUGS:
 *     - Depends on MmUnmapIoSpace to Do The Right Thing in all cases
 */
{
  PAGED_CODE();

  ASSERT(MiniportAdapterHandle);

  MmUnmapIoSpace(VirtualAddress, Length);
}

/* EOF */

