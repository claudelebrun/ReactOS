/* $Id: conio.h,v 1.3 2004/03/14 17:53:27 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/include/conio.h
 * PURPOSE:         CSRSS internal console I/O interface
 */

#ifndef CONIO_H_INCLUDED
#define CONIO_H_INCLUDED

#include "api.h"
#include "win32csr.h"

/* Object type magic numbers */

#define CONIO_CONSOLE_MAGIC         0x00000001
#define CONIO_SCREEN_BUFFER_MAGIC   0x00000002

/************************************************************************
 * Screen buffer structure represents the win32 screen buffer object.   *
 * Internally, the portion of the buffer being shown CAN loop past the  *
 * bottom of the virtual buffer and wrap around to the top.  Win32 does *
 * not do this.  I decided to do this because it eliminates the need to *
 * do a massive memcpy() to scroll the contents of the buffer up to     *
 * scroll the screen on output, instead I just shift down the position  *
 * to be displayed, and let it wrap around to the top again.            *
 * The VirtualX member keeps track of the top X coord that win32        *
 * clients THINK is currently being displayed, because they think that  *
 * when the display reaches the bottom of the buffer and another line   *
 * being printed causes another line to scroll down, that the buffer IS *
 * memcpy()'s up, and the bottom of the buffer is still displayed, but  *
 * internally, I just wrap back to the top of the buffer.               *
 ***********************************************************************/

typedef struct tagCSRSS_SCREEN_BUFFER
{
  Object_t Header;                 /* Object header */
  BYTE *Buffer;                    /* pointer to screen buffer */
  USHORT MaxX, MaxY;               /* size of the entire scrollback buffer */
  USHORT ShowX, ShowY;             /* beginning offset for the actual display area */
  ULONG CurrentX;                  /* Current X cursor position */
  ULONG CurrentY;                  /* Current Y cursor position */
  BYTE DefaultAttrib;              /* default char attribute */
  USHORT VirtualX;                 /* top row of buffer being displayed, reported to callers */
  CONSOLE_CURSOR_INFO CursorInfo;
  USHORT Mode;
} CSRSS_SCREEN_BUFFER, *PCSRSS_SCREEN_BUFFER;

typedef struct tagCSRSS_CONSOLE_VTBL
{
  VOID (STDCALL *InitScreenBuffer)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer);
  VOID (STDCALL *WriteStream)(PCSRSS_CONSOLE Console, RECT *Block, UINT CursorStartX, UINT CursorStartY,
                              UINT ScrolledLines, CHAR *Buffer, UINT Length);
  VOID (STDCALL *DrawRegion)(PCSRSS_CONSOLE Console, RECT *Region);
  BOOL (STDCALL *SetCursorInfo)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer);
  BOOL (STDCALL *SetScreenInfo)(PCSRSS_CONSOLE Console, PCSRSS_SCREEN_BUFFER ScreenBuffer,
                                UINT OldCursorX, UINT OldCursorY);
  BOOL (STDCALL *ChangeTitle)(PCSRSS_CONSOLE Console);
  VOID (STDCALL *CleanupConsole)(PCSRSS_CONSOLE Console);
  BOOL (STDCALL *ChangeIcon)(PCSRSS_CONSOLE Console);
} CSRSS_CONSOLE_VTBL, *PCSRSS_CONSOLE_VTBL;

typedef struct tagCSRSS_CONSOLE
{
  Object_t Header;                      /* Object header */
  PCSRSS_CONSOLE Prev, Next;            /* Next and Prev consoles in console wheel */
  HANDLE ActiveEvent;
  LIST_ENTRY InputEvents;               /* List head for input event queue */
  WORD WaitingChars;
  WORD WaitingLines;                    /* number of chars and lines in input queue */
  PCSRSS_SCREEN_BUFFER ActiveBuffer;    /* Pointer to currently active screen buffer */
  WORD Mode;                            /* Console mode flags */
  WORD EchoCount;                       /* count of chars to echo, in line buffered mode */
  UNICODE_STRING Title;                 /* Title of console */
  struct				/* active code pages */
    {
      UINT Input;
      UINT Output;
    } CodePageId;
  BOOL EarlyReturn;                     /* wake client and return data, even if we are in line buffered mode, and we don't have a complete line */
  DWORD HardwareState;                  /* _GDI_MANAGED, _DIRECT */
  HWND hWindow;
  HICON hWindowIcon;
  COORD Size;
  PVOID PrivateData;
  PCSRSS_CONSOLE_VTBL Vtbl;
  LIST_ENTRY ProcessList;
} CSRSS_CONSOLE;

VOID STDCALL ConioDeleteConsole(Object_t *Object);
VOID STDCALL ConioDeleteScreenBuffer(Object_t *Buffer);
void STDCALL ConioProcessKey(MSG *msg, PCSRSS_CONSOLE Console, BOOL TextMode);
void FASTCALL ConioPhysicalToLogical(PCSRSS_SCREEN_BUFFER Buff,
                                     ULONG PhysicalX,
                                     ULONG PhysicalY,
                                     LONG *LogicalX,
                                     LONG *LogicalY);
VOID FASTCALL ConioDrawConsole(PCSRSS_CONSOLE Console);

/* api/conio.c */
CSR_API(CsrWriteConsole);
CSR_API(CsrAllocConsole);
CSR_API(CsrFreeConsole);
CSR_API(CsrReadConsole);
CSR_API(CsrConnectProcess);
CSR_API(CsrGetScreenBufferInfo);
CSR_API(CsrSetCursor);
CSR_API(CsrFillOutputChar);
CSR_API(CsrReadInputEvent);
CSR_API(CsrWriteConsoleOutputChar);
CSR_API(CsrWriteConsoleOutputAttrib);
CSR_API(CsrFillOutputAttrib);
CSR_API(CsrGetCursorInfo);
CSR_API(CsrSetCursorInfo);
CSR_API(CsrSetTextAttrib);
CSR_API(CsrSetConsoleMode);
CSR_API(CsrGetConsoleMode);
CSR_API(CsrCreateScreenBuffer);
CSR_API(CsrSetScreenBuffer);
CSR_API(CsrSetTitle);
CSR_API(CsrGetTitle);
CSR_API(CsrWriteConsoleOutput);
CSR_API(CsrFlushInputBuffer);
CSR_API(CsrScrollConsoleScreenBuffer);
CSR_API(CsrReadConsoleOutputChar);
CSR_API(CsrReadConsoleOutputAttrib);
CSR_API(CsrGetNumberOfConsoleInputEvents);
CSR_API(CsrPeekConsoleInput);
CSR_API(CsrReadConsoleOutput);
CSR_API(CsrWriteConsoleInput);
CSR_API(CsrHardwareStateProperty);
CSR_API(CsrGetConsoleWindow);
CSR_API(CsrSetConsoleIcon);

#define ConioInitScreenBuffer(Console, Buff) (Console)->Vtbl->InitScreenBuffer((Console), (Buff))
#define ConioDrawRegion(Console, Region) (Console)->Vtbl->DrawRegion((Console), (Region))
#define ConioWriteStream(Console, Block, CurStartX, CurStartY, ScrolledLines, Buffer, Length) \
          (Console)->Vtbl->WriteStream((Console), (Block), (CurStartX), (CurStartY), \
                                       (ScrolledLines), (Buffer), (Length))
#define ConioSetCursorInfo(Console, Buff) (Console)->Vtbl->SetCursorInfo((Console), (Buff))
#define ConioSetScreenInfo(Console, Buff, OldCursorX, OldCursorY) \
          (Console)->Vtbl->SetScreenInfo((Console), (Buff), (OldCursorX), (OldCursorY))
#define ConioChangeTitle(Console) (Console)->Vtbl->ChangeTitle(Console)
#define ConioCleanupConsole(Console) (Console)->Vtbl->CleanupConsole(Console)

#define ConioRectHeight(Rect) \
    (((Rect)->top) > ((Rect)->bottom) ? 0 : ((Rect)->bottom) - ((Rect)->top) + 1)
#define ConioRectWidth(Rect) \
    (((Rect)->left) > ((Rect)->right) ? 0 : ((Rect)->right) - ((Rect)->left) + 1)

#define ConioLockConsole(ProcessData, Handle, Ptr) \
    Win32CsrLockObject((ProcessData), (Handle), (Object_t **)(Ptr), CONIO_CONSOLE_MAGIC)
#define ConioUnlockConsole(Console) \
    Win32CsrUnlockObject((Object_t *) Console)
#define ConioLockScreenBuffer(ProcessData, Handle, Ptr) \
    Win32CsrLockObject((ProcessData), (Handle), (Object_t **)(Ptr), CONIO_SCREEN_BUFFER_MAGIC)
#define ConioUnlockScreenBuffer(Buff) \
    Win32CsrUnlockObject((Object_t *) Buff)
#define ConioChangeIcon(Console) (Console)->Vtbl->ChangeIcon(Console)

#endif /* CONIO_H_INCLUDED */

/* EOF */

