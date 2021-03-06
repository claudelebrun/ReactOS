/* $Id: extypes.h,v 1.21 2003/11/14 15:19:35 ekohl Exp $ */

#ifndef __INCLUDE_DDK_EXTYPES_H
#define __INCLUDE_DDK_EXTYPES_H

typedef ULONG INTERLOCKED_RESULT;
typedef ULONG WORK_QUEUE_TYPE;

typedef ULONG_PTR ERESOURCE_THREAD, *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY
{
   ERESOURCE_THREAD OwnerThread;
   union
     {
	LONG OwnerCount;
	ULONG TableSize;
     }; /* anon */
} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE
{
   LIST_ENTRY SystemResourcesList;
   POWNER_ENTRY OwnerTable;
   SHORT ActiveCount;
   USHORT Flag;
   PKSEMAPHORE SharedWaiters;
   PKEVENT ExclusiveWaiters;
   OWNER_ENTRY OwnerThreads[2];
   ULONG ContentionCount;
   USHORT NumberOfSharedWaiters;
   USHORT NumberOfExclusiveWaiters;
   union
     {
	PVOID Address;
	ULONG CreatorBackTraceIndex;
     }; /* anon */
   KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;


typedef struct 
{
   LONG Count;
   struct _KTHREAD* Owner;
   ULONG Contention;
   KEVENT Event;
   ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

typedef struct _ZONE_HEADER
{
   SINGLE_LIST_ENTRY FreeList;
   SINGLE_LIST_ENTRY SegmentList;
   ULONG BlockSize;
   ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;

typedef struct _ZONE_SEGMENT_HEADER
{
   SINGLE_LIST_ENTRY SegmentList; /* was Entry */
   PVOID Reserved;  /* was ULONG Size; */
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;



typedef VOID STDCALL_FUNC
(*PWORKER_THREAD_ROUTINE)(PVOID Parameter);


/* Modified by Andrew Greenwood, 16th July 2003: */

typedef struct _WORK_QUEUE_ITEM
{
   LIST_ENTRY List;
   PWORKER_THREAD_ROUTINE WorkerRoutine;
   PVOID Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;

typedef PVOID STDCALL_FUNC
(*PALLOCATE_FUNCTION)(POOL_TYPE PoolType,
		      ULONG NumberOfBytes,
		      ULONG Tag);

typedef VOID STDCALL_FUNC
(*PFREE_FUNCTION)(PVOID Buffer);

typedef union _SLIST_HEADER
{
   ULONGLONG Alignment;
   struct
     {
	SINGLE_LIST_ENTRY Next;
	USHORT Depth;
	USHORT Sequence;	
     }; /* now anonymous */
} SLIST_HEADER, *PSLIST_HEADER;

typedef struct _NPAGED_LOOKASIDE_LIST
{
   SLIST_HEADER ListHead;
   USHORT Depth;
   USHORT MaximumDepth;
   ULONG TotalAllocates;
   ULONG AllocateMisses;
   ULONG TotalFrees;
   ULONG FreeMisses;
   POOL_TYPE Type;
   ULONG Tag;
   ULONG Size;
   PALLOCATE_FUNCTION Allocate;
   PFREE_FUNCTION Free;
   LIST_ENTRY ListEntry;
   ULONG LastTotalAllocates;
   ULONG LastAllocateMisses;
   ULONG Pad[2];
   KSPIN_LOCK Obsoleted;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;

typedef struct _PAGED_LOOKASIDE_LIST
{
   SLIST_HEADER ListHead;
   USHORT Depth;
   USHORT MaximumDepth;
   ULONG TotalAllocates;
   ULONG AllocateMisses;
   ULONG TotalFrees;
   ULONG FreeMisses;
   POOL_TYPE Type;
   ULONG Tag;
   ULONG Size;
   PALLOCATE_FUNCTION Allocate;
   PFREE_FUNCTION Free;
   LIST_ENTRY ListEntry;
   ULONG LastTotalAllocates;
   ULONG LastAllocateMisses;
   FAST_MUTEX Obsoleted;
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;


/* callback object (not functional in NT4)*/

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef VOID STDCALL_FUNC
(*PCALLBACK_FUNCTION)(PVOID CallbackContext,
		      PVOID Argument1,
		      PVOID Argument2);

#endif /* __INCLUDE_DDK_EXTYPES_H */

/* EOF */
