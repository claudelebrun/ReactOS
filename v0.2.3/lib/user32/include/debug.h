/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS user32.dll
 * FILE:        include/debug.h
 * PURPOSE:     Debugging support macros
 * DEFINES:     DBG     - Enable debug output
 *              NASSERT - Disable assertions
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#define NORMAL_MASK    0x000000FF
#define SPECIAL_MASK   0xFFFFFF00
#define MIN_TRACE      0x00000001
#define MID_TRACE      0x00000002
#define MAX_TRACE      0x00000003

#define DEBUG_CHECK    0x00000100
#define DEBUG_OBJECT   0x00000200
#define DEBUG_WINDOW   0x00000400
#define DEBUG_ULTRA    0xFFFFFFFF

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef DBG

extern DWORD DebugTraceLevel;

#define D(_t_, _x_) \
    if (((DebugTraceLevel & NORMAL_MASK) >= _t_) || \
        ((DebugTraceLevel & _t_) > NORMAL_MASK)) { \
        DbgPrint("(%hS:%d)(%hS) ", __FILE__, __LINE__, __FUNCTION__); \
		    DbgPrint _x_; \
    }

#ifdef NASSERT
#define ASSERT(x)
#else /* NASSERT */
#define ASSERT(x) if (!(x)) { D(MIN_TRACE, ("Assertion "#x" failed at %s:%d\n", __FILE__, __LINE__)); KeBugCheck(0); }
#endif /* NASSERT */

#define ASSERT_IRQL(x) ASSERT(KeGetCurrentIrql() <= (x))

#else /* DBG */

#define D(_t_, _x_)

#define ASSERT_IRQL(x)
#define ASSERT(x)

#endif /* DBG */

#ifdef assert
#undef assert
#endif
#define assert(x) ASSERT(x)
#define assert_irql(x) ASSERT_IRQL(x)


#define UNIMPLEMENTED \
    D(MIN_TRACE, ("is unimplemented, please try again later.\n"));

#define CHECKPOINT \
    D(DEBUG_CHECK, ("\n"));

#define DPRINT(X...) D(DEBUG_CHECK, (X))

#define CP CHECKPOINT

#endif /* __DEBUG_H */

/* EOF */
