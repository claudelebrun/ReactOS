/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: msgqueue.c,v 1.100 2004/05/22 17:51:08 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Message queues
 * FILE:             subsys/win32k/ntuser/msgqueue.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <win32k/debug1.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

#define SYSTEM_MESSAGE_QUEUE_SIZE           (256)

static MSG SystemMessageQueue[SYSTEM_MESSAGE_QUEUE_SIZE];
static ULONG SystemMessageQueueHead = 0;
static ULONG SystemMessageQueueTail = 0;
static ULONG SystemMessageQueueCount = 0;
static ULONG SystemMessageQueueMouseMove = -1;
static KSPIN_LOCK SystemMessageQueueLock;

static ULONG volatile HardwareMessageQueueStamp = 0;
static LIST_ENTRY HardwareMessageQueueHead;
static KMUTEX HardwareMessageQueueLock;

static KEVENT HardwareMessageEvent;

static PAGED_LOOKASIDE_LIST MessageLookasideList;

#define IntLockSystemMessageQueue(OldIrql) \
  KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql)

#define IntUnLockSystemMessageQueue(OldIrql) \
  KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql)

#define IntUnLockSystemHardwareMessageQueueLock(Wait) \
  KeReleaseMutex(&HardwareMessageQueueLock, Wait)

/* FUNCTIONS *****************************************************************/

/* set some queue bits */
inline VOID MsqSetQueueBits( PUSER_MESSAGE_QUEUE Queue, WORD Bits )
{
    Queue->WakeBits |= Bits;
    Queue->ChangedBits |= Bits;
    if (MsqIsSignaled( Queue )) KeSetEvent(&Queue->NewMessages, IO_NO_INCREMENT, FALSE);
}

/* clear some queue bits */
inline VOID MsqClearQueueBits( PUSER_MESSAGE_QUEUE Queue, WORD Bits )
{
    Queue->WakeBits &= ~Bits;
    Queue->ChangedBits &= ~Bits;
}

VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
  IntLockMessageQueue(Queue);
  Queue->PaintCount++;
  Queue->PaintPosted = TRUE;
  KeSetEvent(&Queue->NewMessages, IO_NO_INCREMENT, FALSE);
  IntUnLockMessageQueue(Queue);
}

VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
  IntLockMessageQueue(Queue);
  Queue->PaintCount--;
  if (Queue->PaintCount == 0)
    {
      Queue->PaintPosted = FALSE;
    }
  IntUnLockMessageQueue(Queue);
}


NTSTATUS FASTCALL
MsqInitializeImpl(VOID)
{
  /*CurrentFocusMessageQueue = NULL;*/
  InitializeListHead(&HardwareMessageQueueHead);
  KeInitializeEvent(&HardwareMessageEvent, NotificationEvent, 0);
  KeInitializeSpinLock(&SystemMessageQueueLock);
  KeInitializeMutex(&HardwareMessageQueueLock, 0);

  ExInitializePagedLookasideList(&MessageLookasideList,
				 NULL,
				 NULL,
				 0,
				 sizeof(USER_MESSAGE),
				 0,
				 256);

  return(STATUS_SUCCESS);
}

VOID FASTCALL
MsqInsertSystemMessage(MSG* Msg)
{
  LARGE_INTEGER LargeTickCount;
  KIRQL OldIrql;
  ULONG mmov = (ULONG)-1;
  
  KeQueryTickCount(&LargeTickCount);
  Msg->time = LargeTickCount.u.LowPart;

  IntLockSystemMessageQueue(OldIrql);

  /* only insert WM_MOUSEMOVE messages if not already in system message queue */
  if(Msg->message == WM_MOUSEMOVE)
    mmov = SystemMessageQueueMouseMove;

  if(mmov != (ULONG)-1)
  {
    /* insert message at the queue head */
    while (mmov != SystemMessageQueueHead )
    {
      ULONG prev = mmov ? mmov - 1 : SYSTEM_MESSAGE_QUEUE_SIZE - 1;
      ASSERT((LONG) mmov >= 0);
      ASSERT(mmov < SYSTEM_MESSAGE_QUEUE_SIZE);
      SystemMessageQueue[mmov] = SystemMessageQueue[prev];
      mmov = prev;
    }
    SystemMessageQueue[SystemMessageQueueHead] = *Msg;
  }
  else
  {
    if (SystemMessageQueueCount == SYSTEM_MESSAGE_QUEUE_SIZE)
    {
      IntUnLockSystemMessageQueue(OldIrql);
      return;
    }
    SystemMessageQueue[SystemMessageQueueTail] = *Msg;
    if(Msg->message == WM_MOUSEMOVE)
      SystemMessageQueueMouseMove = SystemMessageQueueTail;
    SystemMessageQueueTail =
      (SystemMessageQueueTail + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
    SystemMessageQueueCount++;
  }
  IntUnLockSystemMessageQueue(OldIrql);
  KeSetEvent(&HardwareMessageEvent, IO_NO_INCREMENT, FALSE);
}

BOOL FASTCALL
MsqIsDblClk(LPMSG Msg, BOOL Remove)
{
  PWINSTATION_OBJECT WinStaObject;
  PSYSTEM_CURSORINFO CurInfo;
  NTSTATUS Status;
  LONG dX, dY;
  BOOL Res;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                          KernelMode,
                                          0,
                                          &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    return FALSE;
  }
  CurInfo = IntGetSysCursorInfo(WinStaObject);
  Res = (Msg->hwnd == (HWND)CurInfo->LastClkWnd) && 
        ((Msg->time - CurInfo->LastBtnDown) < CurInfo->DblClickSpeed);
  if(Res)
  {
    
    dX = CurInfo->LastBtnDownX - Msg->pt.x;
    dY = CurInfo->LastBtnDownY - Msg->pt.y;
    if(dX < 0) dX = -dX;
    if(dY < 0) dY = -dY;
    
    Res = (dX <= CurInfo->DblClickWidth) &&
          (dY <= CurInfo->DblClickHeight);
  }

  if(Remove)
  {
    if (Res)
    {
      CurInfo->LastBtnDown = 0;
      CurInfo->LastBtnDownX = Msg->pt.x;
      CurInfo->LastBtnDownY = Msg->pt.y;
      CurInfo->LastClkWnd = NULL;
    }
    else
    {
      CurInfo->LastBtnDownX = Msg->pt.x;
      CurInfo->LastBtnDownY = Msg->pt.y;
      CurInfo->LastClkWnd = (HANDLE)Msg->hwnd;
      CurInfo->LastBtnDown = Msg->time;
    }
  }
  
  ObDereferenceObject(WinStaObject);
  return Res;
}

BOOL STATIC STDCALL
MsqTranslateMouseMessage(PUSER_MESSAGE_QUEUE MessageQueue, HWND hWnd, UINT FilterLow, UINT FilterHigh,
			 PUSER_MESSAGE Message, BOOL Remove, PBOOL Freed, 
			 PWINDOW_OBJECT ScopeWin, PPOINT ScreenPoint, BOOL FromGlobalQueue)
{
  USHORT Msg = Message->Msg.message;
  PWINDOW_OBJECT Window = NULL;
  HWND CaptureWin;
  
  CaptureWin = IntGetCaptureWindow();
  if (CaptureWin == NULL)
  {
    if(Msg == WM_MOUSEWHEEL)
    {
      Window = IntGetWindowObject(IntGetFocusWindow());
    }
    else
    {
      WinPosWindowFromPoint(ScopeWin, NULL, &Message->Msg.pt, &Window);
      if(Window == NULL)
      {
        Window = ScopeWin;
        IntReferenceWindowObject(Window);
      }
    }
  }
  else
  {
    /* FIXME - window messages should go to the right window if no buttons are
               pressed */
    Window = IntGetWindowObject(CaptureWin);
  }

  if (Window == NULL)
  {
    if(!FromGlobalQueue)
    {
      RemoveEntryList(&Message->ListEntry);
      if(MessageQueue->MouseMoveMsg == Message)
      {
        MessageQueue->MouseMoveMsg = NULL;
      }
    }
    ExFreePool(Message);
    *Freed = TRUE;
    return(FALSE);
  }
  
  if (Window->MessageQueue != MessageQueue)
  {
    if (! FromGlobalQueue)
    {
      DPRINT("Moving msg between private queues\n");
      /* This message is already queued in a private queue, but we need
       * to move it to a different queue, perhaps because a new window
       * was created which now covers the screen area previously taken
       * by another window. To move it, we need to take it out of the
       * old queue. Note that we're already holding the lock mutexes of the
       * old queue */
      RemoveEntryList(&Message->ListEntry);
      
      /* remove the pointer for the current WM_MOUSEMOVE message in case we
         just removed it */
      if(MessageQueue->MouseMoveMsg == Message)
      {
        MessageQueue->MouseMoveMsg = NULL;
      }
    }
    
    /* lock the destination message queue, so we don't get in trouble with other
       threads, messing with it at the same time */
    IntLockHardwareMessageQueue(Window->MessageQueue);
    InsertTailList(&Window->MessageQueue->HardwareMessagesListHead,
                   &Message->ListEntry);
    if(Message->Msg.message == WM_MOUSEMOVE)
    {
      if(Window->MessageQueue->MouseMoveMsg)
      {
        /* remove the old WM_MOUSEMOVE message, we're processing a more recent
           one */
        RemoveEntryList(&Window->MessageQueue->MouseMoveMsg->ListEntry);
        ExFreePool(Window->MessageQueue->MouseMoveMsg);
      }
      /* save the pointer to the WM_MOUSEMOVE message in the new queue */
      Window->MessageQueue->MouseMoveMsg = Message;
    }
    IntUnLockHardwareMessageQueue(Window->MessageQueue);
    
    KeSetEvent(&Window->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
    *Freed = FALSE;
    IntReleaseWindowObject(Window);
    return(FALSE);
  }
  
  /* From here on, we're in the same message queue as the caller! */
  
  *ScreenPoint = Message->Msg.pt;
  
  if((hWnd != NULL && Window->Self != hWnd) ||
     ((FilterLow != 0 || FilterLow != 0) && (Msg < FilterLow || Msg > FilterHigh)))
  {
    /* Reject the message because it doesn't match the filter */
    
    if(FromGlobalQueue)
    {
      /* Lock the message queue so no other thread can mess with it.
         Our own message queue is not locked while fetching from the global
         queue, so we have to make sure nothing interferes! */
      IntLockHardwareMessageQueue(Window->MessageQueue);
      /* if we're from the global queue, we need to add our message to our
         private queue so we don't loose it! */
      InsertTailList(&Window->MessageQueue->HardwareMessagesListHead,
                     &Message->ListEntry);
    }
    
    if (Message->Msg.message == WM_MOUSEMOVE)
    {
      if(Window->MessageQueue->MouseMoveMsg &&
         (Window->MessageQueue->MouseMoveMsg != Message))
      {
        /* delete the old message */
        RemoveEntryList(&Window->MessageQueue->MouseMoveMsg->ListEntry);
        ExFreePool(Window->MessageQueue->MouseMoveMsg);
      }
      /* always save a pointer to this WM_MOUSEMOVE message here because we're
         sure that the message is in the private queue */
      Window->MessageQueue->MouseMoveMsg = Message;
    }
    if(FromGlobalQueue)
    {
      IntUnLockHardwareMessageQueue(Window->MessageQueue);
    }
    
    IntReleaseWindowObject(Window);
    *Freed = FALSE;
    return(FALSE);
  }
  
  /* FIXME - only assign if removing? */
  Message->Msg.hwnd = Window->Self;
  Message->Msg.message = Msg;
  Message->Msg.lParam = MAKELONG(Message->Msg.pt.x, Message->Msg.pt.y);
  
  /* remove the reference to the current WM_(NC)MOUSEMOVE message, if this message
     is it */
  if (Message->Msg.message == WM_MOUSEMOVE ||
      Message->Msg.message == WM_NCMOUSEMOVE)
  {
    if(FromGlobalQueue)
    {
      /* Lock the message queue so no other thread can mess with it.
         Our own message queue is not locked while fetching from the global
         queue, so we have to make sure nothing interferes! */
      IntLockHardwareMessageQueue(Window->MessageQueue);
      if(Window->MessageQueue->MouseMoveMsg)
      {
        /* delete the WM_(NC)MOUSEMOVE message in the private queue, we're dealing
           with one that's been sent later */
        RemoveEntryList(&Window->MessageQueue->MouseMoveMsg->ListEntry);
        ExFreePool(Window->MessageQueue->MouseMoveMsg);
        /* our message is not in the private queue so we can remove the pointer
           instead of setting it to the current message we're processing */
        Window->MessageQueue->MouseMoveMsg = NULL;
      }
      IntUnLockHardwareMessageQueue(Window->MessageQueue);
    }
    else if(Window->MessageQueue->MouseMoveMsg == Message)
    {
      Window->MessageQueue->MouseMoveMsg = NULL;
    }
  }
  
  IntReleaseWindowObject(Window);
  *Freed = FALSE;
  return(TRUE);
}

BOOL STDCALL
MsqPeekHardwareMessage(PUSER_MESSAGE_QUEUE MessageQueue, HWND hWnd,
		       UINT FilterLow, UINT FilterHigh, BOOL Remove,
		       PUSER_MESSAGE* Message)
{
  KIRQL OldIrql;
  POINT ScreenPoint;
  BOOL Accept, Freed;
  PLIST_ENTRY CurrentEntry;
  PWINDOW_OBJECT DesktopWindow;
  PVOID WaitObjects[2];
  NTSTATUS WaitStatus;

  if( !IntGetScreenDC() || 
      PsGetWin32Thread()->MessageQueue == W32kGetPrimitiveMessageQueue() ) 
  {
    return FALSE;
  }

  WaitObjects[1] = &MessageQueue->NewMessages;
  WaitObjects[0] = &HardwareMessageQueueLock;
  do
    {
      WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                            UserMode, TRUE, NULL, NULL);
      while (MsqDispatchOneSentMessage(MessageQueue))
        {
          ;
        }
    }
  while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);

  DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());

  /* Process messages in the message queue itself. */
  IntLockHardwareMessageQueue(MessageQueue);
  CurrentEntry = MessageQueue->HardwareMessagesListHead.Flink;
  while (CurrentEntry != &MessageQueue->HardwareMessagesListHead)
    {
      PUSER_MESSAGE Current =
	CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      if (Current->Msg.message >= WM_MOUSEFIRST &&
	  Current->Msg.message <= WM_MOUSELAST)
	{
	  Accept = MsqTranslateMouseMessage(MessageQueue, hWnd, FilterLow, FilterHigh,
					    Current, Remove, &Freed, 
					    DesktopWindow, &ScreenPoint, FALSE);
	  if (Accept)
	    {
	      if (Remove)
		{
		  RemoveEntryList(&Current->ListEntry);
		}
	      IntUnLockHardwareMessageQueue(MessageQueue);
              IntUnLockSystemHardwareMessageQueueLock(FALSE);
	      *Message = Current;
	      IntReleaseWindowObject(DesktopWindow);
	      return(TRUE);
	    }

	}
    }
  IntUnLockHardwareMessageQueue(MessageQueue);

  /* Now try the global queue. */

  /* Transfer all messages from the DPC accessible queue to the main queue. */
  IntLockSystemMessageQueue(OldIrql);
  while (SystemMessageQueueCount > 0)
    {
      PUSER_MESSAGE UserMsg;
      MSG Msg;

      ASSERT(SystemMessageQueueHead < SYSTEM_MESSAGE_QUEUE_SIZE);
      Msg = SystemMessageQueue[SystemMessageQueueHead];
      SystemMessageQueueHead =
	(SystemMessageQueueHead + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
      SystemMessageQueueCount--;
      IntUnLockSystemMessageQueue(OldIrql);
      UserMsg = ExAllocateFromPagedLookasideList(&MessageLookasideList);
      /* What to do if out of memory? For now we just panic a bit in debug */
      ASSERT(UserMsg);
      UserMsg->FreeLParam = FALSE;
      UserMsg->Msg = Msg;
      InsertTailList(&HardwareMessageQueueHead, &UserMsg->ListEntry);
      IntLockSystemMessageQueue(OldIrql);
    }
  /*
   * we could set this to -1 conditionally if we find one, but
   * this is more efficient and just as effective.
   */
  SystemMessageQueueMouseMove = -1;
  HardwareMessageQueueStamp++;
  IntUnLockSystemMessageQueue(OldIrql);

  /* Process messages in the queue until we find one to return. */
  CurrentEntry = HardwareMessageQueueHead.Flink;
  while (CurrentEntry != &HardwareMessageQueueHead)
    {
      PUSER_MESSAGE Current =
	CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      RemoveEntryList(&Current->ListEntry);
      HardwareMessageQueueStamp++;
      if (Current->Msg.message >= WM_MOUSEFIRST &&
	  Current->Msg.message <= WM_MOUSELAST)
	{
	  const ULONG ActiveStamp = HardwareMessageQueueStamp;
	  /* Translate the message. */
	  Accept = MsqTranslateMouseMessage(MessageQueue, hWnd, FilterLow, FilterHigh,
					    Current, Remove, &Freed, 
					    DesktopWindow, &ScreenPoint, TRUE);
	  if (Accept)
	    {
	      /* Check for no more messages in the system queue. */
	      IntLockSystemMessageQueue(OldIrql);
	      if (SystemMessageQueueCount == 0 &&
		  IsListEmpty(&HardwareMessageQueueHead))
		{
		  KeClearEvent(&HardwareMessageEvent);
		}
	      IntUnLockSystemMessageQueue(OldIrql);

	      /*
		 If we aren't removing the message then add it to the private
		 queue.
	      */
	      if (!Remove)
		{
                  IntLockHardwareMessageQueue(MessageQueue);
		  if(Current->Msg.message == WM_MOUSEMOVE)
		  {
		    if(MessageQueue->MouseMoveMsg)
		    {
		      RemoveEntryList(&MessageQueue->MouseMoveMsg->ListEntry);
		      ExFreePool(MessageQueue->MouseMoveMsg);
		    }
		    MessageQueue->MouseMoveMsg = Current;
		  }
		  InsertTailList(&MessageQueue->HardwareMessagesListHead,
				 &Current->ListEntry);
                  IntUnLockHardwareMessageQueue(MessageQueue);
		}
	      IntUnLockSystemHardwareMessageQueueLock(FALSE);
	      *Message = Current;
	      IntReleaseWindowObject(DesktopWindow);
	      return(TRUE);
	    }
	  /* If the contents of the queue changed then restart processing. */
	  if (HardwareMessageQueueStamp != ActiveStamp)
	    {
	      CurrentEntry = HardwareMessageQueueHead.Flink;
	      continue;
	    }
	}
    }
  IntReleaseWindowObject(DesktopWindow);
  /* Check if the system message queue is now empty. */
  IntLockSystemMessageQueue(OldIrql);
  if (SystemMessageQueueCount == 0 && IsListEmpty(&HardwareMessageQueueHead))
    {
      KeClearEvent(&HardwareMessageEvent);
    }
  IntUnLockSystemMessageQueue(OldIrql);
  IntUnLockSystemHardwareMessageQueueLock(FALSE);

  return(FALSE);
}

VOID FASTCALL
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PUSER_MESSAGE_QUEUE FocusMessageQueue;
  MSG Msg;

  DPRINT("MsqPostKeyboardMessage(uMsg 0x%x, wParam 0x%x, lParam 0x%x)\n",
    uMsg, wParam, lParam);

  Msg.hwnd = 0;
  Msg.message = uMsg;
  Msg.wParam = wParam;
  Msg.lParam = lParam;
  /* FIXME: Initialize time and point. */
  
  FocusMessageQueue = IntGetFocusMessageQueue();
  if( !IntGetScreenDC() ) {
    if( W32kGetPrimitiveMessageQueue() ) {
      MsqPostMessage(W32kGetPrimitiveMessageQueue(), &Msg, FALSE);
    }
  } else {
    if (FocusMessageQueue == NULL)
      {
	DPRINT("No focus message queue\n");
	return;
      }
    
    if (FocusMessageQueue->FocusWindow != (HWND)0)
      {
	Msg.hwnd = FocusMessageQueue->FocusWindow;
        DPRINT("Msg.hwnd = %x\n", Msg.hwnd);
	MsqPostMessage(FocusMessageQueue, &Msg, FALSE);
      }
    else
      {
	DPRINT("Invalid focus window handle\n");
      }
  }
}

VOID FASTCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  PWINDOW_OBJECT Window;
  PW32THREAD Win32Thread;
  PW32PROCESS Win32Process;
  MSG Mesg;
  NTSTATUS Status;

  Status = ObReferenceObjectByPointer (Thread,
				       THREAD_ALL_ACCESS,
				       PsThreadType,
				       KernelMode);
  if (!NT_SUCCESS(Status))
    return;

  Win32Thread = ((PETHREAD)Thread)->Win32Thread;
  if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
    {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
    }

  Win32Process = ((PETHREAD)Thread)->ThreadsProcess->Win32Process;
  if (Win32Process == NULL || Win32Process->WindowStation == NULL)
    {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
    }

  Status = ObmReferenceObjectByHandle(Win32Process->WindowStation->HandleTable,
                                      hWnd, otWindow, (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
    }

  Mesg.hwnd = hWnd;
  Mesg.message = WM_HOTKEY;
  Mesg.wParam = wParam;
  Mesg.lParam = lParam;
//      Mesg.pt.x = PsGetWin32Process()->WindowStation->SystemCursor.x;
//      Mesg.pt.y = PsGetWin32Process()->WindowStation->SystemCursor.y;
//      KeQueryTickCount(&LargeTickCount);
//      Mesg.time = LargeTickCount.u.LowPart;
  MsqPostMessage(Window->MessageQueue, &Mesg, FALSE);
  ObmDereferenceObject(Window);
  ObDereferenceObject (Thread);

//  IntLockMessageQueue(pThread->MessageQueue);
//  InsertHeadList(&pThread->MessageQueue->PostedMessagesListHead,
//		 &Message->ListEntry);
//  KeSetEvent(&pThread->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
//  IntUnLockMessageQueue(pThread->MessageQueue);

}

PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg, BOOLEAN FreeLParam)
{
  PUSER_MESSAGE Message;

  Message = ExAllocateFromPagedLookasideList(&MessageLookasideList);
  if (!Message)
    {
      return NULL;
    }

  Message->FreeLParam = FreeLParam;
  RtlMoveMemory(&Message->Msg, Msg, sizeof(MSG));

  return Message;
}

VOID FASTCALL
MsqDestroyMessage(PUSER_MESSAGE Message)
{
  ExFreeToPagedLookasideList(&MessageLookasideList, Message);
}

VOID FASTCALL
MsqDispatchSentNotifyMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
  PLIST_ENTRY ListEntry;
  PUSER_SENT_MESSAGE_NOTIFY Message;

  IntLockMessageQueue(MessageQueue);
  while (!IsListEmpty(&MessageQueue->SentMessagesListHead))
  {
    ListEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
    Message = CONTAINING_RECORD(ListEntry, USER_SENT_MESSAGE_NOTIFY,
				ListEntry);
    IntUnLockMessageQueue(MessageQueue);

    IntCallSentMessageCallback(Message->CompletionCallback,
				Message->hWnd,
				Message->Msg,
				Message->CompletionCallbackContext,
				Message->Result);
    
    IntLockMessageQueue(MessageQueue);
  }
  IntUnLockMessageQueue(MessageQueue);
}

BOOLEAN FASTCALL
MsqPeekSentMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
  return(!IsListEmpty(&MessageQueue->SentMessagesListHead));
}

BOOLEAN FASTCALL
MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue)
{
  PUSER_SENT_MESSAGE Message;
  PLIST_ENTRY Entry;
  LRESULT Result;
  BOOL Freed;
  PUSER_SENT_MESSAGE_NOTIFY NotifyMessage;

  IntLockMessageQueue(MessageQueue);
  if (IsListEmpty(&MessageQueue->SentMessagesListHead))
    {
      IntUnLockMessageQueue(MessageQueue);
      return(FALSE);
    }
  
  /* remove it from the list of pending messages */
  Entry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
  Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);
  
  /* insert it to the list of messages that are currently dispatched by this
     message queue */
  InsertTailList(&MessageQueue->LocalDispatchingMessagesHead,
		 &Message->ListEntry);
  
  IntUnLockMessageQueue(MessageQueue);
  
  /* Call the window procedure. */
  Result = IntSendMessage(Message->Msg.hwnd,
                          Message->Msg.message,
                          Message->Msg.wParam,
                          Message->Msg.lParam);
  
  /* remove the message from the local dispatching list, because it doesn't need
     to be cleaned up on thread termination anymore */
  IntLockMessageQueue(MessageQueue);
  RemoveEntryList(&Message->ListEntry);
  IntUnLockMessageQueue(MessageQueue);

  /* remove the message from the dispatching list, so lock the sender's message queue */
  IntLockMessageQueue(Message->SenderQueue);
  
  Freed = (Message->DispatchingListEntry.Flink == NULL);
  if(!Freed)
  {
    /* only remove it from the dispatching list if not already removed by a timeout */
    RemoveEntryList(&Message->DispatchingListEntry);
  }
  /* still keep the sender's message queue locked, so the sender can't exit the
     MsqSendMessage() function (if timed out) */
  
  /* Let the sender know the result. */
  if (Message->Result != NULL)
    {
      *Message->Result = Result;
    }

  /* Notify the sender. */
  if (Message->CompletionEvent != NULL)
    {
      KeSetEvent(Message->CompletionEvent, IO_NO_INCREMENT, FALSE);
    }
  
  /* unlock the sender's message queue, the safe operation is done */
  IntUnLockMessageQueue(Message->SenderQueue);

  /* Notify the sender if they specified a callback. */
  if (!Freed && Message->CompletionCallback != NULL)
    {
      if(!(NotifyMessage = ExAllocatePoolWithTag(NonPagedPool,
					         sizeof(USER_SENT_MESSAGE_NOTIFY), TAG_USRMSG)))
      {
        DPRINT1("MsqDispatchOneSentMessage(): Not enough memory to create a callback notify message\n");
        goto Notified;
      }
      NotifyMessage->CompletionCallback =
	Message->CompletionCallback;
      NotifyMessage->CompletionCallbackContext =
	Message->CompletionCallbackContext;
      NotifyMessage->Result = Result;
      NotifyMessage->hWnd = Message->Msg.hwnd;
      NotifyMessage->Msg = Message->Msg.message;
      MsqSendNotifyMessage(Message->SenderQueue, NotifyMessage);
    }

Notified:
  if(!Freed)
  {
    /* only dereference our message queue if the message has not been timed out */
    IntDereferenceMessageQueue(MessageQueue);
  }
  
  /* only free the message if not freed already */
  ExFreePool(Message);
  return(TRUE);
}

VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
		     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage)
{
  IntLockMessageQueue(MessageQueue);
  InsertTailList(&MessageQueue->NotifyMessagesListHead,
		 &NotifyMessage->ListEntry);
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  IntUnLockMessageQueue(MessageQueue);
}

NTSTATUS FASTCALL
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
               UINT uTimeout, BOOL Block, ULONG_PTR *uResult)
{
  PUSER_SENT_MESSAGE Message;
  KEVENT CompletionEvent;
  NTSTATUS WaitStatus;
  LRESULT Result;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  LARGE_INTEGER Timeout;
  PLIST_ENTRY Entry;

  if(!(Message = ExAllocatePoolWithTag(PagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
  {
    DPRINT1("MsqSendMessage(): Not enough memory to allocate a message");
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  
  KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);
  
  ThreadQueue = PsGetWin32Thread()->MessageQueue;
  ASSERT(ThreadQueue != MessageQueue);
  
  Timeout.QuadPart = uTimeout * -10000;
  
  /* FIXME - increase reference counter of sender's message queue here */
  
  Result = 0;
  Message->Msg.hwnd = Wnd;
  Message->Msg.message = Msg;
  Message->Msg.wParam = wParam;
  Message->Msg.lParam = lParam;
  Message->CompletionEvent = &CompletionEvent;
  Message->Result = &Result;
  Message->SenderQueue = ThreadQueue;
  Message->CompletionCallback = NULL;
  
  IntReferenceMessageQueue(MessageQueue);
  
  /* add it to the list of pending messages */
  IntLockMessageQueue(ThreadQueue);
  InsertTailList(&ThreadQueue->DispatchingMessagesHead, &Message->DispatchingListEntry);
  IntUnLockMessageQueue(ThreadQueue);
  
  /* queue it in the destination's message queue */
  IntLockMessageQueue(MessageQueue);
  InsertTailList(&MessageQueue->SentMessagesListHead, &Message->ListEntry);
  IntUnLockMessageQueue(MessageQueue);
  
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  
  /* we can't access the Message anymore since it could have already been deleted! */
  
  if(Block)
  {
    /* don't process messages sent to the thread */
    WaitStatus = KeWaitForSingleObject(&CompletionEvent, UserRequest, UserMode, 
                                       FALSE, (uTimeout ? &Timeout : NULL));
    if(WaitStatus == STATUS_TIMEOUT)
      {
        /* look up if the message has not yet dispatched, if so
           make sure it can't pass a result and it must not set the completion event anymore */
	IntLockMessageQueue(MessageQueue);
        Entry = MessageQueue->SentMessagesListHead.Flink;
        while (Entry != &MessageQueue->SentMessagesListHead)
          {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                == Message)
              {
                /* we can access Message here, it's secure because the message queue is locked
                   and the message is still hasn't been dispatched */
		Message->CompletionEvent = NULL;
                Message->Result = NULL;
                break;
              }
            Entry = Entry->Flink;
          }
        IntUnLockMessageQueue(MessageQueue);
	
	/* remove from the local dispatching list so the other thread knows,
	   it can't pass a result and it must not set the completion event anymore */
	IntLockMessageQueue(ThreadQueue);
        Entry = ThreadQueue->DispatchingMessagesHead.Flink;
        while (Entry != &ThreadQueue->DispatchingMessagesHead)
          {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                == Message)
              {
                /* we can access Message here, it's secure because the sender's message is locked
                   and the message has definitely not yet been destroyed, otherwise it would
                   have been removed from this list by the dispatching routine right after
		   dispatching the message */
		Message->CompletionEvent = NULL;
                Message->Result = NULL;
                RemoveEntryList(&Message->DispatchingListEntry);
                IntDereferenceMessageQueue(MessageQueue);
                break;
              }
            Entry = Entry->Flink;
          }
	IntUnLockMessageQueue(ThreadQueue);
	
	DPRINT("MsqSendMessage (blocked) timed out\n");
      }
    while (MsqDispatchOneSentMessage(ThreadQueue));
  }
  else
  {
    PVOID WaitObjects[2];
    
    WaitObjects[0] = &CompletionEvent;
    WaitObjects[1] = &ThreadQueue->NewMessages;
    do
      {
        WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                              UserMode, FALSE, (uTimeout ? &Timeout : NULL), NULL);
        if(WaitStatus == STATUS_TIMEOUT)
          {
            /* look up if the message has not yet been dispatched, if so
               make sure it can't pass a result and it must not set the completion event anymore */
	    IntLockMessageQueue(MessageQueue);
            Entry = MessageQueue->SentMessagesListHead.Flink;
            while (Entry != &MessageQueue->SentMessagesListHead)
              {
                if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                    == Message)
                  {
                    /* we can access Message here, it's secure because the message queue is locked
                       and the message is still hasn't been dispatched */
		    Message->CompletionEvent = NULL;
                    Message->Result = NULL;
                    break;
                  }
                Entry = Entry->Flink;
              }
	    IntUnLockMessageQueue(MessageQueue);
	    
	    /* remove from the local dispatching list so the other thread knows,
	       it can't pass a result and it must not set the completion event anymore */
	    IntLockMessageQueue(ThreadQueue);
            Entry = ThreadQueue->DispatchingMessagesHead.Flink;
            while (Entry != &ThreadQueue->DispatchingMessagesHead)
              {
                if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                    == Message)
                  {
                    /* we can access Message here, it's secure because the sender's message is locked
                       and the message has definitely not yet been destroyed, otherwise it would
                       have been removed from this list by the dispatching routine right after
		       dispatching the message */
		    Message->CompletionEvent = NULL;
                    Message->Result = NULL;
                    RemoveEntryList(&Message->DispatchingListEntry);
                    IntDereferenceMessageQueue(MessageQueue);
                    break;
                  }
                Entry = Entry->Flink;
              }
	    IntUnLockMessageQueue(ThreadQueue);
	    
	    DPRINT("MsqSendMessage timed out\n");
            break;
          }
        while (MsqDispatchOneSentMessage(ThreadQueue));
      }
    while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);
  }
  
  if(WaitStatus != STATUS_TIMEOUT)
    *uResult = (STATUS_WAIT_0 == WaitStatus ? Result : -1);
  
  return WaitStatus;
}

VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue, MSG* Msg, BOOLEAN FreeLParam)
{
  PUSER_MESSAGE Message;
  
  if(!(Message = MsqCreateMessage(Msg, FreeLParam)))
  {
    return;
  }
  IntLockMessageQueue(MessageQueue);
  InsertTailList(&MessageQueue->PostedMessagesListHead,
		 &Message->ListEntry);
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  IntUnLockMessageQueue(MessageQueue);
}

VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode)
{
  IntLockMessageQueue(MessageQueue);
  MessageQueue->QuitPosted = TRUE;
  MessageQueue->QuitExitCode = ExitCode;
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  IntUnLockMessageQueue(MessageQueue);
}

BOOLEAN STDCALL
MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	       IN BOOLEAN Hardware,
	       IN BOOLEAN Remove,
	       IN HWND Wnd,
	       IN UINT MsgFilterLow,
	       IN UINT MsgFilterHigh,
	       OUT PUSER_MESSAGE* Message)
{
  PLIST_ENTRY CurrentEntry;
  PUSER_MESSAGE CurrentMessage;
  PLIST_ENTRY ListHead;

  if (Hardware)
    {
      return(MsqPeekHardwareMessage(MessageQueue, Wnd,
				    MsgFilterLow, MsgFilterHigh,
				    Remove, Message));
    }

  IntLockMessageQueue(MessageQueue);
  CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
  ListHead = &MessageQueue->PostedMessagesListHead;
  while (CurrentEntry != ListHead)
    {
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
					 ListEntry);
      if ((Wnd == 0 || Wnd == CurrentMessage->Msg.hwnd) &&
	  ((MsgFilterLow == 0 && MsgFilterHigh == 0) ||
	   (MsgFilterLow <= CurrentMessage->Msg.message &&
	    MsgFilterHigh >= CurrentMessage->Msg.message)))
	{
	  if (Remove)
	    {
	      RemoveEntryList(&CurrentMessage->ListEntry);
	    }
	  IntUnLockMessageQueue(MessageQueue);
	  *Message = CurrentMessage;
	  return(TRUE);
	}
      CurrentEntry = CurrentEntry->Flink;
    }
  IntUnLockMessageQueue(MessageQueue);
  return(FALSE);
}

NTSTATUS FASTCALL
MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
  PVOID WaitObjects[2] = {&MessageQueue->NewMessages, &HardwareMessageEvent};
  return(KeWaitForMultipleObjects(2,
				  WaitObjects,
				  WaitAny,
				  Executive,
				  UserMode,
				  TRUE,
				  NULL,
				  NULL));
}

BOOL FASTCALL
MsqIsHung(PUSER_MESSAGE_QUEUE MessageQueue)
{
  LARGE_INTEGER LargeTickCount;
  
  KeQueryTickCount(&LargeTickCount);
  return ((LargeTickCount.u.LowPart - MessageQueue->LastMsgRead) > MSQ_HUNG);
}

VOID FASTCALL
MsqInitializeMessageQueue(struct _ETHREAD *Thread, PUSER_MESSAGE_QUEUE MessageQueue)
{
  LARGE_INTEGER LargeTickCount;
  
  MessageQueue->Thread = Thread;
  MessageQueue->CaretInfo = (PTHRDCARETINFO)(MessageQueue + 1);
  InitializeListHead(&MessageQueue->PostedMessagesListHead);
  InitializeListHead(&MessageQueue->SentMessagesListHead);
  InitializeListHead(&MessageQueue->HardwareMessagesListHead);
  InitializeListHead(&MessageQueue->DispatchingMessagesHead);
  InitializeListHead(&MessageQueue->LocalDispatchingMessagesHead);
  KeInitializeMutex(&MessageQueue->HardwareLock, 0);
  ExInitializeFastMutex(&MessageQueue->Lock);
  MessageQueue->QuitPosted = FALSE;
  MessageQueue->QuitExitCode = 0;
  KeInitializeEvent(&MessageQueue->NewMessages, SynchronizationEvent, FALSE);
  KeQueryTickCount(&LargeTickCount);
  MessageQueue->LastMsgRead = LargeTickCount.u.LowPart;
  MessageQueue->FocusWindow = NULL;
  MessageQueue->PaintPosted = FALSE;
  MessageQueue->PaintCount = 0;
}

VOID FASTCALL
MsqCleanupMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
  PLIST_ENTRY CurrentEntry;
  PUSER_MESSAGE CurrentMessage;
  PUSER_SENT_MESSAGE CurrentSentMessage;
  
  IntLockMessageQueue(MessageQueue);
  
  /* cleanup posted messages */
  while (!IsListEmpty(&MessageQueue->PostedMessagesListHead))
    {
      CurrentEntry = RemoveHeadList(&MessageQueue->PostedMessagesListHead);
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
					 ListEntry);
      MsqDestroyMessage(CurrentMessage);
    }
  
  /* remove the messages that have not yet been dispatched */
  while (!IsListEmpty(&MessageQueue->SentMessagesListHead))
    {
      CurrentEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE, 
                                             ListEntry);
      
      DPRINT("Notify the sender and remove a message from the queue that had not been dispatched\n");
      
      /* remove the message from the dispatching list */
      if(CurrentSentMessage->DispatchingListEntry.Flink != NULL)
      {
        RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }
      
      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
        KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }
      
      /* dereference our message queue */
      IntDereferenceMessageQueue(MessageQueue);
      
      /* free the message */
      ExFreePool(CurrentSentMessage);
    }
  
  /* notify senders of dispatching messages. This needs to be cleaned up if e.g.
     ExitThread() was called in a SendMessage() umode callback */
  while (!IsListEmpty(&MessageQueue->LocalDispatchingMessagesHead))
    {
      CurrentEntry = RemoveHeadList(&MessageQueue->LocalDispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             ListEntry);
      
      DPRINT("Notify the sender, the thread has been terminated while dispatching a message!\n");
      
      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
        KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }
      
      /* dereference our message queue */
      IntDereferenceMessageQueue(MessageQueue);
      
      /* free the message */
      ExFreePool(CurrentSentMessage);
    }
  
  IntUnLockMessageQueue(MessageQueue);
}

PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(struct _ETHREAD *Thread)
{
  PUSER_MESSAGE_QUEUE MessageQueue;

  MessageQueue = (PUSER_MESSAGE_QUEUE)ExAllocatePoolWithTag(PagedPool,
				   sizeof(USER_MESSAGE_QUEUE) + sizeof(THRDCARETINFO),
				   TAG_MSGQ);
  
  if (!MessageQueue)
    {
      return NULL;
    }

  RtlZeroMemory(MessageQueue, sizeof(USER_MESSAGE_QUEUE) + sizeof(THRDCARETINFO));
  /* hold at least one reference until it'll be destroyed */
  IntReferenceMessageQueue(MessageQueue);
  /* initialize the queue */
  MsqInitializeMessageQueue(Thread, MessageQueue);

  return MessageQueue;
}

VOID FASTCALL
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
  MsqCleanupMessageQueue(MessageQueue);
  /* decrease the reference counter, if it hits zero, the queue will be freed */
  IntDereferenceMessageQueue(MessageQueue);
}

PHOOKTABLE FASTCALL
MsqGetHooks(PUSER_MESSAGE_QUEUE Queue)
{
  return Queue->Hooks;
}

VOID FASTCALL
MsqSetHooks(PUSER_MESSAGE_QUEUE Queue, PHOOKTABLE Hooks)
{
  Queue->Hooks = Hooks;
}

LPARAM FASTCALL
MsqSetMessageExtraInfo(LPARAM lParam)
{
  LPARAM Ret;
  PUSER_MESSAGE_QUEUE MessageQueue;
  
  MessageQueue = PsGetWin32Thread()->MessageQueue;
  if(!MessageQueue)
  {
    return 0;
  }
  
  Ret = MessageQueue->ExtraInfo;
  MessageQueue->ExtraInfo = lParam;
  
  return Ret;
}

LPARAM FASTCALL
MsqGetMessageExtraInfo(VOID)
{
  PUSER_MESSAGE_QUEUE MessageQueue;
  
  MessageQueue = PsGetWin32Thread()->MessageQueue;
  if(!MessageQueue)
  {
    return 0;
  }
  
  return MessageQueue->ExtraInfo;
}

HWND FASTCALL
MsqSetStateWindow(PUSER_MESSAGE_QUEUE MessageQueue, ULONG Type, HWND hWnd)
{
  HWND Prev;
  
  switch(Type)
  {
    case MSQ_STATE_CAPTURE:
      Prev = MessageQueue->CaptureWindow;
      MessageQueue->CaptureWindow = hWnd;
      return Prev;
    case MSQ_STATE_ACTIVE:
      Prev = MessageQueue->ActiveWindow;
      MessageQueue->ActiveWindow = hWnd;
      return Prev;
    case MSQ_STATE_FOCUS:
      Prev = MessageQueue->FocusWindow;
      MessageQueue->FocusWindow = hWnd;
      return Prev;
    case MSQ_STATE_MENUOWNER:
      Prev = MessageQueue->MenuOwner;
      MessageQueue->MenuOwner = hWnd;
      return Prev;
    case MSQ_STATE_MOVESIZE:
      Prev = MessageQueue->MoveSize;
      MessageQueue->MoveSize = hWnd;
      return Prev;
    case MSQ_STATE_CARET:
      ASSERT(MessageQueue->CaretInfo);
      Prev = MessageQueue->CaretInfo->hWnd;
      MessageQueue->CaretInfo->hWnd = hWnd;
      return Prev;
  }
  
  return NULL;
}

/* EOF */
