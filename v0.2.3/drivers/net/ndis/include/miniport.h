/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.h
 * PURPOSE:     Definitions for routines used by NDIS miniport drivers
 */

#ifndef __MINIPORT_H
#define __MINIPORT_H

#include <ndissys.h>


typedef struct _HARDWARE_ADDRESS {
    union {
        UCHAR Medium802_3[ETH_LENGTH_OF_ADDRESS];
    } Type;
} HARDWARE_ADDRESS, *PHARDWARE_ADDRESS;

/* Information about a miniport */
typedef struct _MINIPORT_DRIVER {
    LIST_ENTRY                      ListEntry;          /* Entry on global list */
    KSPIN_LOCK                      Lock;               /* Protecting spin lock */
    ULONG                           RefCount;           /* Reference count */
    NDIS_MINIPORT_CHARACTERISTICS   Chars;              /* Miniport characteristics */
    WORK_QUEUE_ITEM                 WorkItem;           /* Work item */
    PDRIVER_OBJECT                  DriverObject;       /* Driver object of miniport */
    LIST_ENTRY                      AdapterListHead;    /* Adapters created by miniport */
    PUNICODE_STRING                 RegistryPath;       /* SCM Registry key */
} MINIPORT_DRIVER, *PMINIPORT_DRIVER;

/* resources allocated on behalf on the miniport */
#define MINIPORT_RESOURCE_TYPE_MEMORY 0
typedef struct _MINIPORT_RESOURCE {
    LIST_ENTRY     ListEntry;
    ULONG          ResourceType;
    PVOID          Resource;
} MINIPORT_RESOURCE, *PMINIPORT_RESOURCE;

/* Configuration context */
typedef struct _MINIPORT_CONFIGURATION_CONTEXT {
    NDIS_HANDLE    Handle;
    LIST_ENTRY     ResourceListHead;
    KSPIN_LOCK     ResourceLock;
} MINIPORT_CONFIGURATION_CONTEXT, *PMINIPORT_CONFIGURATION_CONTEXT;

/* Bugcheck callback context */
typedef struct _MINIPORT_BUGCHECK_CONTEXT {
    PVOID                       DriverContext;
    ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler;
    PKBUGCHECK_CALLBACK_RECORD  CallbackRecord;
} MINIPORT_BUGCHECK_CONTEXT, *PMINIPORT_BUGCHECK_CONTEXT;

/* a miniport's shared memory */
typedef struct _MINIPORT_SHARED_MEMORY {
    PADAPTER_OBJECT   AdapterObject;
    ULONG             Length;
    PHYSICAL_ADDRESS  PhysicalAddress;
    PVOID             VirtualAddress;
    BOOLEAN           Cached;
} MINIPORT_SHARED_MEMORY, *PMINIPORT_SHARED_MEMORY;

#define GET_MINIPORT_DRIVER(Handle)((PMINIPORT_DRIVER)Handle)

/* detected adapters that are driverless */
typedef struct _ORPHAN_ADATER {
    LIST_ENTRY        ListEntry;
    NDIS_STRING       RegistryPath;
    INTERFACE_TYPE    BusType;
    ULONG             BusNumber;
    ULONG             SlotNumber;
} ORPHAN_ADAPTER, *PORPHAN_ADAPTER;

/* Information about a logical adapter */
typedef struct _LOGICAL_ADAPTER 
{
    NDIS_MINIPORT_BLOCK         NdisMiniportBlock;      /* NDIS defined fields */
    KDPC                        MiniportDpc;            /* DPC routine for adapter */
    BOOLEAN                     MiniportBusy;           /* A MiniportXxx routine is executing */
    NDIS_HANDLE                 MiniportAdapterBinding; /* Binding handle for current caller */
    ULONG                       WorkQueueLevel;         /* Number of used work item buffers */
    NDIS_MINIPORT_WORK_ITEM     WorkQueue[NDIS_MINIPORT_WORK_QUEUE_SIZE];
    PNDIS_MINIPORT_WORK_ITEM    WorkQueueHead;          /* Head of work queue */
    PNDIS_MINIPORT_WORK_ITEM    WorkQueueTail;          /* Tail of work queue */
    LIST_ENTRY                  ListEntry;              /* Entry on global list */
    LIST_ENTRY                  MiniportListEntry;      /* Entry on miniport driver list */
    LIST_ENTRY                  ProtocolListHead;       /* List of bound protocols */
    ULONG                       RefCount;               /* Reference count */
    PMINIPORT_DRIVER            Miniport;               /* Miniport owning this adapter */
    UNICODE_STRING              DeviceName;             /* Device name of this adapter */
    ULONG                       Attributes;             /* Attributes of adapter */
    BOOLEAN                     AttributesSet;          /* Whether NdisMSetAttributes(Ex) has been called */
    PVOID                       QueryBuffer;            /* Buffer to use for queries */
    ULONG                       QueryBufferLength;      /* Length of QueryBuffer */
    ULONG                       MediumHeaderSize;       /* Size of medium header */
    HARDWARE_ADDRESS            Address;                /* Hardware address of adapter */
    ULONG                       AddressLength;          /* Length of hardware address */
    PUCHAR                      LookaheadBuffer;        /* Pointer to lookahead buffer */
    ULONG                       LookaheadLength;        /* Length of lookahead buffer */
    ULONG                       CurLookaheadLength;     /* Current (selected) length of lookahead buffer */
    ULONG                       MaxLookaheadLength;     /* Maximum length of lookahead buffer */
    PNDIS_PACKET                PacketQueueHead;        /* Head of packet queue */
    PNDIS_PACKET                PacketQueueTail;        /* Head of packet queue */
    PNDIS_PACKET                LoopPacket;             /* Current packet beeing looped */
    PMINIPORT_BUGCHECK_CONTEXT  BugcheckContext;        /* Adapter's shutdown handler */
    KEVENT                      DmaEvent;               /* Event to support DMA register allocation */
    KSPIN_LOCK                  DmaLock;                /* Spinlock to protect the dma list */
    UINT                        BusNumber;              /* The bus number of the adapter  */
    INTERFACE_TYPE              BusType;                /* The bus type of the adapter */
    UINT                        SlotNumber;             /* The slot number of the adapter*/
    ULONG                       Irql;                   /* The Irql assigned to the adapter */
    ULONG                       Vector;                 /* The interrupt vector assigned to the adapter */
    KAFFINITY                   Affinity;               /* The processor affinity of the adapter */
    PHYSICAL_ADDRESS            BaseIoAddress;          /* The base IO address of the adapter */
    PHYSICAL_ADDRESS            BaseMemoryAddress;      /* The base memory address of mapped memory for the adapter */
    ULONG                       DmaChannel;             /* The DMA channel number of the adapter */
    ULONG                       DmaPort;                /* The DMA port number of the adapter */
    PNDIS_MINIPORT_TIMER        Timer;                  /* The timer  object of the adapter */
} LOGICAL_ADAPTER, *PLOGICAL_ADAPTER;

#define GET_LOGICAL_ADAPTER(Handle)((PLOGICAL_ADAPTER)Handle)

extern LIST_ENTRY MiniportListHead;
extern KSPIN_LOCK MiniportListLock;
extern LIST_ENTRY AdapterListHead;
extern KSPIN_LOCK AdapterListLock;


#ifdef DBG
VOID
MiniDisplayPacket(
    PNDIS_PACKET Packet);
#endif /* DBG */

VOID
MiniIndicateData(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_HANDLE         MacReceiveContext,
    PVOID               HeaderBuffer,
    UINT                HeaderBufferSize,
    PVOID               LookaheadBuffer,
    UINT                LookaheadBufferSize,
    UINT                PacketSize);

BOOLEAN
MiniAdapterHasAddress(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet);

PLOGICAL_ADAPTER
MiniLocateDevice(
    PNDIS_STRING AdapterName);

NDIS_STATUS
MiniQueryInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PULONG              BytesWritten);

NDIS_STATUS
FASTCALL
MiniQueueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE WorkItemType,
    PVOID               WorkItemContext,
    NDIS_HANDLE         Initiator);

NDIS_STATUS
FASTCALL
MiniDequeueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE *WorkItemType,
    PVOID               *WorkItemContext,
    NDIS_HANDLE         *Initiator);

NDIS_STATUS
MiniDoRequest(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_REQUEST NdisRequest);

BOOLEAN 
NdisFindDevice(
    UINT   VendorID, 
    UINT   DeviceID, 
    PUINT  BusNumber, 
    PUINT  SlotNumber);

VOID
NdisStartDevices();

#endif /* __MINIPORT_H */

/* EOF */

