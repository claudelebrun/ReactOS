/*

 */

#ifndef __INTERNAL_HAL_BUS_H
#define __INTERNAL_HAL_BUS_H


typedef NTSTATUS STDCALL_FUNC
(*pAdjustResourceList)(IN PBUS_HANDLER BusHandler,
		       IN ULONG BusNumber,
		       IN OUT PCM_RESOURCE_LIST Resources);

typedef NTSTATUS STDCALL_FUNC
(*pAssignSlotResources)(IN PBUS_HANDLER BusHandler,
			IN ULONG BusNumber,
			IN PUNICODE_STRING RegistryPath,
			IN PUNICODE_STRING DriverClassName,
			IN PDRIVER_OBJECT DriverObject,
			IN PDEVICE_OBJECT DeviceObject,
			IN ULONG SlotNumber,
			IN OUT PCM_RESOURCE_LIST *AllocatedResources);

typedef ULONG STDCALL_FUNC
(*pGetSetBusData)(IN PBUS_HANDLER BusHandler,
		  IN ULONG BusNumber,
		  IN ULONG SlotNumber,
		  OUT PVOID Buffer,
		  IN ULONG Offset,
		  IN ULONG Length);

typedef ULONG STDCALL_FUNC
(*pGetInterruptVector)(IN PBUS_HANDLER BusHandler,
		       IN ULONG BusNumber,
		       IN ULONG BusInterruptLevel,
		       IN ULONG BusInterruptVector,
		       OUT PKIRQL Irql,
		       OUT PKAFFINITY Affinity);

typedef ULONG STDCALL_FUNC
(*pTranslateBusAddress)(IN PBUS_HANDLER BusHandler,
			IN ULONG BusNumber,
			IN PHYSICAL_ADDRESS BusAddress,
			IN OUT PULONG AddressSpace,
			OUT PPHYSICAL_ADDRESS TranslatedAddress);

typedef struct _BUS_HANDLER
{
  LIST_ENTRY Entry;
  INTERFACE_TYPE InterfaceType;
  BUS_DATA_TYPE BusDataType;
  ULONG BusNumber;
  ULONG RefCount;

  pGetSetBusData	GetBusData;
  pGetSetBusData	SetBusData;
  pAdjustResourceList	AdjustResourceList;
  pAssignSlotResources	AssignSlotResources;
  pGetInterruptVector	GetInterruptVector;
  pTranslateBusAddress	TranslateBusAddress;
} BUS_HANDLER;


/* FUNCTIONS *****************************************************************/

/* bus.c */
PBUS_HANDLER
HalpAllocateBusHandler(INTERFACE_TYPE InterfaceType,
		       BUS_DATA_TYPE BusDataType,
		       ULONG BusNumber);

/* sysbus.h */
ULONG STDCALL
HalpGetSystemInterruptVector(PVOID BusHandler,
			     ULONG BusNumber,
			     ULONG BusInterruptLevel,
			     ULONG BusInterruptVector,
			     PKIRQL Irql,
			     PKAFFINITY Affinity);

BOOLEAN STDCALL
HalpTranslateSystemBusAddress(PBUS_HANDLER BusHandler,
			      ULONG BusNumber,
			      PHYSICAL_ADDRESS BusAddress,
			      PULONG AddressSpace,
			      PPHYSICAL_ADDRESS TranslatedAddress);

/* isa.c */
ULONG STDCALL
HalpGetIsaInterruptVector(PVOID BusHandler,
			  ULONG BusNumber,
			  ULONG BusInterruptLevel,
			  ULONG BusInterruptVector,
			  PKIRQL Irql,
			  PKAFFINITY Affinity);

BOOLEAN STDCALL
HalpTranslateIsaBusAddress(PBUS_HANDLER BusHandler,
			   ULONG BusNumber,
			   PHYSICAL_ADDRESS BusAddress,
			   PULONG AddressSpace,
			   PPHYSICAL_ADDRESS TranslatedAddress);

/* time.c */
ULONG STDCALL
HalpGetCmosData(PBUS_HANDLER BusHandler,
		ULONG BusNumber,
		ULONG SlotNumber,
		PVOID Buffer,
		ULONG Offset,
		ULONG Length);

ULONG STDCALL
HalpSetCmosData(PBUS_HANDLER BusHandler,
		ULONG BusNumber,
		ULONG SlotNumber,
		PVOID Buffer,
		ULONG Offset,
		ULONG Length);

/* mca.h */
ULONG STDCALL
HalpGetMicroChannelData(PBUS_HANDLER BusHandler,
			ULONG BusNumber,
			ULONG SlotNumber,
			PVOID Buffer,
			ULONG Offset,
			ULONG Length);

#endif /* __INTERNAL_HAL_BUS_H */

/* EOF */

