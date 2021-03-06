#ifndef _WIN32K_TAGS_H
#define _WIN32K_TAGS_H

#define TAG_STRING	TAG('S', 'T', 'R', ' ') /* string */

/* ntuser */
#define TAG_MOUSE	TAG('M', 'O', 'U', 'S') /* mouse */
#define TAG_ACCEL	TAG('A', 'C', 'C', 'L') /* accelerator */
#define TAG_HOOK	TAG('W', 'N', 'H', 'K') /* hook */
#define TAG_HOTKEY	TAG('H', 'O', 'T', 'K') /* hotkey */
#define TAG_MENUITEM	TAG('M', 'E', 'N', 'I') /* menu item */
#define TAG_MSG		TAG('M', 'E', 'S', 'G') /* message */
#define TAG_MSGQ	TAG('M', 'S', 'G', 'Q') /* message queue */
#define TAG_USRMSG	TAG('U', 'M', 'S', 'G') /* user message */
#define TAG_WNDPROP	TAG('W', 'P', 'R', 'P') /* window property */
#define TAG_WNAM	TAG('W', 'N', 'A', 'M') /* window name */
#define TAG_WINLIST	TAG('W', 'N', 'L', 'S') /* window handle list */
#define TAG_WININTLIST	TAG('W', 'N', 'I', 'P') /* window internal pos */
#define TAG_WINPROCLST	TAG('W', 'N', 'P', 'L') /* window proc list */
#define TAG_SBARINFO	TAG('S', 'B', 'I', 'N') /* scrollbar info */
#define TAG_TIMER	TAG('T', 'I', 'M', 'R') /* timer entry */
#define TAG_TIMERTD	TAG('T', 'I', 'M', 'T') /* timer thread dereference list */
#define TAG_TIMERBMP	TAG('T', 'I', 'M', 'B') /* timers bitmap */
#define TAG_CALLBACK	TAG('C', 'B', 'C', 'K') /* callback memory */

/* objects */
#define TAG_BEZIER	TAG('B', 'E', 'Z', 'R') /* bezier */
#define TAG_BITMAP	TAG('B', 'T', 'M', 'P') /* bitmap */
#define TAG_PATBLT	TAG('P', 'B', 'L', 'T') /* patblt */
#define TAG_CLIP	TAG('C', 'L', 'I', 'P') /* clipping */
#define TAG_COORD	TAG('C', 'O', 'R', 'D') /* coords */
#define TAG_DC		TAG('D', 'C', 'D', 'C') /* dc */
#define TAG_GDIOBJ	TAG('G', 'D', 'I', 'O') /* gdi obj */
#define TAG_GDIHNDTBLE	TAG('G', 'D', 'I', 'H') /* gdi handle table */
#define TAG_DIB		TAG('D', 'I', 'B', ' ') /* dib */
#define TAG_COLORMAP	TAG('C', 'O', 'L', 'M') /* color map */
#define TAG_SHAPE	TAG('S', 'H', 'A', 'P') /* shape */
#define TAG_PALETTE	TAG('P', 'A', 'L', 'E') /* palette */
#define TAG_PALETTEMAP	TAG('P', 'A', 'L', 'M') /* palette mapping */
#define TAG_PATH	TAG('P', 'A', 'T', 'H') /* path */
#define TAG_PRINT	TAG('P', 'R', 'N', 'T') /* print */
#define TAG_REGION	TAG('R', 'G', 'N', 'O') /* region */
#define TAG_GDITEXT	TAG('T', 'X', 'T', 'O') /* text */
#define TAG_FONT	TAG('F', 'N', 'T', 'O') /* font entry */

/* misc */
#define TAG_DRIVER  TAG('G', 'D', 'R', 'V') /* video drivers */

#endif /* _WIN32K_TAGS_H */
