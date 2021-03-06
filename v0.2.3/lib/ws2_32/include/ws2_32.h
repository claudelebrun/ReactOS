/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        include/ws2_32.h
 * PURPOSE:     WinSock 2 DLL header
 */
#ifndef __WS2_32_H
#define __WS2_32_H

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ntos.h>
#include <napi/teb.h>
#include <winsock2.h>
#include <ws2spi.h>
#include <ws2tcpip.h>
#include <windows.h>
#undef assert
#include <debug.h>

/* Exported by ntdll.dll, but where is the prototype? */
unsigned long strtoul(const char *nptr, char **endptr, int base);


#define EXPORT STDCALL

extern HANDLE GlobalHeap;
extern BOOL Initialized;	/* TRUE if WSAStartup() has been successfully called */
extern WSPUPCALLTABLE UpcallTable;

#define WS2_INTERNAL_MAX_ALIAS 16

typedef struct _WINSOCK_GETSERVBYNAME_CACHE {
    UINT Size;
    SERVENT ServerEntry;
    PCHAR Aliases[WS2_INTERNAL_MAX_ALIAS];
    CHAR Data[1];
} WINSOCK_GETSERVBYNAME_CACHE, *PWINSOCK_GETSERVBYNAME_CACHE;

typedef struct _WINSOCK_THREAD_BLOCK {
    INT LastErrorValue;     /* Error value from last function that failed */
    CHAR Intoa[16];         /* Buffer for inet_ntoa() */
    PWINSOCK_GETSERVBYNAME_CACHE 
    Getservbyname;          /* Buffer used by getservbyname */
} WINSOCK_THREAD_BLOCK, *PWINSOCK_THREAD_BLOCK;


/* Macros */

#define WSAINITIALIZED (Initialized)

#define WSASETINITIALIZED (Initialized = TRUE)


#ifdef LE

/* DWORD network to host byte order conversion for little endian machines */
#define DN2H(dw) \
  ((((dw) & 0xFF000000L) >> 24) | \
   (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* DWORD host to network byte order conversion for little endian machines */
#define DH2N(dw) \
	((((dw) & 0xFF000000L) >> 24) | \
	 (((dw) & 0x00FF0000L) >> 8) | \
	 (((dw) & 0x0000FF00L) << 8) | \
	 (((dw) & 0x000000FFL) << 24))

/* WORD network to host order conversion for little endian machines */
#define WN2H(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

/* WORD host to network byte order conversion for little endian machines */
#define WH2N(w) \
	((((w) & 0xFF00) >> 8) | \
	 (((w) & 0x00FF) << 8))

#else /* LE */

/* DWORD network to host byte order conversion for big endian machines */
#define DN2H(dw) \
    (dw)

/* DWORD host to network byte order conversion big endian machines */
#define DH2N(dw) \
    (dw)

/* WORD network to host order conversion for big endian machines */
#define WN2H(w) \
    (w)

/* WORD host to network byte order conversion for big endian machines */
#define WH2N(w) \
    (w)

#endif /* LE */

#endif /* __WS2_32_H */

/* EOF */
