/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/time.c
 * PURPOSE:     Time related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   Vizzini 08-Oct-2003  Formatting, commenting, and ASSERTs
 *
 * NOTES:
 *     - Although the standard kernel-mode M.O. is to trust the caller
 *       to not provide bad arguments, we have added lots of argument
 *       validation to assist in the effort to get third-party binaries
 *       working.  It is easiest to track bugs when things break quickly
 *       and badly.
 *     - Nearly this entire file is PAGED_CODE (with the exception of the
 *       MiniportTimerDpc() function)
 */
#include <ndissys.h>


VOID STDCALL
MiniportTimerDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2)
/*
 * FUNCTION: Scheduled by the SetTimer family of functions
 * ARGUMENTS:
 *     Dpc: Pointer to the DPC Object being executed
 *     DeferredContext: Pointer to a NDIS_MINIPORT_TIMER object
 *     SystemArgument1: Unused.
 *     SystemArgument2: Unused.
 * NOTES:
 *     - runs at IRQL = DISPATCH_LEVEL
 */
{
  PNDIS_MINIPORT_TIMER Timer;

  Timer = (PNDIS_MINIPORT_TIMER)DeferredContext;

  ASSERT(Timer->MiniportTimerFunction);

  Timer->MiniportTimerFunction (NULL, Timer->MiniportTimerContext, NULL, NULL);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisCancelTimer(
    IN  PNDIS_TIMER Timer,
    OUT PBOOLEAN    TimerCancelled)
/*
 * FUNCTION: Cancels a scheduled NDIS timer
 * ARGUMENTS:
 *     Timer: pointer to an NDIS_TIMER object to cancel
 *     TimerCancelled: boolean that returns cancellation status
 * NOTES:
 *     - call at IRQL <= DISPATCH_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(Timer);

  *TimerCancelled = KeCancelTimer (&Timer->Timer);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisGetCurrentSystemTime (
    IN  OUT PLARGE_INTEGER   pSystemTime)
/*
 * FUNCTION: Retrieve the current system time
 * ARGUMENTS:
 *     pSystemTime: pointer to the returned system time
 * NOTES:
 *     - call at IRQL <= DISPATCH_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(pSystemTime);

  KeQuerySystemTime (pSystemTime);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeTimer(
    IN OUT  PNDIS_TIMER             Timer,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
/*
 * FUNCTION: Set up an NDIS_TIMER for later use
 * ARGUMENTS:
 *     Timer: pointer to caller-allocated storage to receive an NDIS_TIMER
 *     TimerFunction: function pointer to routine to run when timer expires
 *     FunctionContext: context (param 2) to be passed to the timer function when it runs
 * NOTES:
 *     - TimerFunction will be called at DISPATCH_LEVEL
 *     - call at IRQL = PASSIVE_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(Timer);

  KeInitializeTimer (&Timer->Timer);

  KeInitializeDpc (&Timer->Dpc, (PKDEFERRED_ROUTINE)TimerFunction, FunctionContext);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMCancelTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    OUT PBOOLEAN                TimerCancelled)
/*
 * FUNCTION: cancel a scheduled NDIS_MINIPORT_TIMER
 * ARGUMENTS:
 *     Timer: timer object to cancel
 *     TimerCancelled: status of cancel operation
 * NOTES:
 *     - call at IRQL <= DISPATCH_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(TimerCancelled);
  ASSERT(Timer);

  *TimerCancelled = KeCancelTimer (&Timer->Timer);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMInitializeTimer(
    IN OUT  PNDIS_MINIPORT_TIMER    Timer,
    IN      NDIS_HANDLE             MiniportAdapterHandle,
    IN      PNDIS_TIMER_FUNCTION    TimerFunction,
    IN      PVOID                   FunctionContext)
/*
 * FUNCTION: Initialize an NDIS_MINIPORT_TIMER
 * ARGUMENTS:
 *     Timer: Timer object to initialize
 *     MiniportAdapterHandle: Handle to the miniport, passed in to MiniportInitialize
 *     TimerFunction: function to be executed when the timer expires
 *     FunctionContext: argument passed to TimerFunction when it is called
 * NOTES:
 *     - TimerFunction is called at IRQL = DISPATCH_LEVEL
 *     - call at IRQL = PASSIVE_LEVEL
 */
{
  PAGED_CODE();
  ASSERT(Timer);
  KeInitializeTimer (&Timer->Timer);

  KeInitializeDpc (&Timer->Dpc, MiniportTimerDpc, (PVOID) Timer);

  Timer->MiniportTimerFunction = TimerFunction;
  Timer->MiniportTimerContext = FunctionContext;
  Timer->Miniport = MiniportAdapterHandle;
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSetPeriodicTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsPeriod)
/*
 * FUNCTION: Set a timer to go off periodically
 * ARGUMENTS:
 *     Timer: pointer to the timer object to set
 *     MillisecondsPeriod: period of the timer
 * NOTES:
 *     - Minimum predictible interval is ~10ms
 *     - Must be called at IRQL <= DISPATCH_LEVEL)
 */
{
  LARGE_INTEGER Timeout;

  PAGED_CODE();
  ASSERT(Timer);

  /* relative delays are negative, absolute are positive; resolution is 100ns */
  Timeout.QuadPart = MillisecondsPeriod * -10000;

  KeSetTimerEx (&Timer->Timer, Timeout, MillisecondsPeriod, &Timer->Dpc);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisMSetTimer(
    IN  PNDIS_MINIPORT_TIMER    Timer,
    IN  UINT                    MillisecondsToDelay)
/*
 * FUNCTION: Set a NDIS_MINIPORT_TIMER so that it goes off
 * ARGUMENTS:
 *     Timer: timer object to set
 *     MillisecondsToDelay: time to wait for the timer to expire
 * NOTES:
 *     - Minimum predictible interval is ~10ms
 *     - Must be called at IRQL <= DISPATCH_LEVEL)
 */
{
  LARGE_INTEGER Timeout;

  PAGED_CODE();
  ASSERT(Timer);

  /* relative delays are negative, absolute are positive; resolution is 100ns */
  Timeout.QuadPart = MillisecondsToDelay * -10000;

  KeSetTimer (&Timer->Timer, Timeout, &Timer->Dpc);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisSetTimer(
    IN  PNDIS_TIMER Timer,
    IN  UINT        MillisecondsToDelay)
/*
 * FUNCTION: Set an NDIS_TIMER so that it goes off
 * ARGUMENTS:
 *     Timer: timer object to set
 *     MillisecondsToDelay: time to wait for the timer to expire
 * NOTES:
 *     - Minimum predictible interval is ~10ms
 *     - Must be called at IRQL <= DISPATCH_LEVEL)
 */
{
  LARGE_INTEGER Timeout;

  PAGED_CODE();
  ASSERT(Timer);

  /* relative delays are negative, absolute are positive; resolution is 100ns */
  Timeout.QuadPart = MillisecondsToDelay * -10000;

  KeSetTimer (&Timer->Timer, Timeout, &Timer->Dpc);
}

/* EOF */

