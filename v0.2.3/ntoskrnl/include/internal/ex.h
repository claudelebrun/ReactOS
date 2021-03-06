/*
 * internal executive prototypes
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H

#define NTOS_MODE_KERNEL
#include <ntos.h>

typedef struct _WINSTATION_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  KSPIN_LOCK Lock;
  UNICODE_STRING Name;
  LIST_ENTRY DesktopListHead;
  PRTL_ATOM_TABLE AtomTable;
  PVOID HandleTable;
  HANDLE SystemMenuTemplate;
  PVOID SystemCursor;
  UINT CaretBlinkRate;
  HANDLE ShellWindow;
  HANDLE ShellListView;
  ULONG Flags;
  struct _DESKTOP_OBJECT* ActiveDesktop;
  /* FIXME: Clipboard */
  LIST_ENTRY HotKeyListHead;
  FAST_MUTEX HotKeyListLock;
} WINSTATION_OBJECT, *PWINSTATION_OBJECT;

typedef struct _DESKTOP_OBJECT
{
  CSHORT Type;
  CSHORT Size;
  LIST_ENTRY ListEntry;
  KSPIN_LOCK Lock;
  UNICODE_STRING Name;
  /* Pointer to the associated window station. */
  struct _WINSTATION_OBJECT *WindowStation;
  /* Pointer to the active queue. */
  PVOID ActiveMessageQueue;
  /* Rectangle of the work area */
#ifdef __WIN32K__
  RECT WorkArea;
#else
  LONG WorkArea[4];
#endif
  /* Handle of the desktop window. */
  HANDLE DesktopWindow;
  HANDLE PrevActiveWindow;
  /* Thread blocking input */
  PVOID BlockInputThread;
} DESKTOP_OBJECT, *PDESKTOP_OBJECT;


typedef VOID (*PLOOKASIDE_MINMAX_ROUTINE)(
  POOL_TYPE PoolType,
  ULONG Size,
  PUSHORT MinimumDepth,
  PUSHORT MaximumDepth);

/* GLOBAL VARIABLES *********************************************************/

TIME_ZONE_INFORMATION _SystemTimeZoneInfo;
extern POBJECT_TYPE ExEventPairObjectType;


/* INITIALIZATION FUNCTIONS *************************************************/

VOID
ExpWin32kInit(VOID);

VOID 
ExInit2 (VOID);
VOID
ExInit3 (VOID);
VOID 
ExInitTimeZoneInfo (VOID);
VOID 
ExInitializeWorkerThreads(VOID);
VOID
ExpInitLookasideLists(VOID);

/* OTHER FUNCTIONS **********************************************************/

VOID
ExpSwapThreadEventPair(
	IN struct _ETHREAD* Thread,
	IN struct _KEVENT_PAIR* EventPair
	);


#endif /* __NTOSKRNL_INCLUDE_INTERNAL_EXECUTIVE_H */
