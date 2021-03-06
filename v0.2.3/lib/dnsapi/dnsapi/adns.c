/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS system libraries
 * FILE:        lib/dnsapi/dnsapi/context.c
 * PURPOSE:     ADNS translation.
 * PROGRAMER:   Art Yerkes
 * UPDATE HISTORY:
 *              12/15/03 -- Created
 */

#include <windows.h>
#include <WinError.h>
#include <WinDNS.h>
#include <internal/windns.h>

DNS_STATUS DnsIntTranslateAdnsToDNS_STATUS( int Status ) {
  switch( Status ) {
  case adns_s_ok:
    return ERROR_SUCCESS;
  case adns_s_nomemory:
  case adns_s_systemfail:
  default: /* There really aren't any general errors in the dns part. */
    return DNS_ERROR_NO_MEMORY;
  }
}

