/* $Id: lang.c,v 1.19 2004/06/13 20:04:56 navaraf Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/lang.c
 * AUTHOR   : ???
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FIXME:  these are included in winnls.h, however including this file causes alot of 
           conflicting type errors. */

#define LOCALE_SYEARMONTH 0x1006
#define LOCALE_IPAPERSIZE 0x100A
#define LOCALE_RETURN_NUMBER 0x20000000
#define LOCALE_USE_CP_ACP 0x40000000
#define LOCALE_LOCALEINFOFLAGSMASK (LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP|LOCALE_RETURN_NUMBER)

static LCID SystemLocale = MAKELCID(LANG_ENGLISH, SORT_DEFAULT);

//#define _OLE2NLS_IN_BUILD_

/*
 * @implemented
 */
LCID
STDCALL
ConvertDefaultLocale (
    LCID    Locale
    )
{
  switch(Locale)
  {
    case LOCALE_SYSTEM_DEFAULT:
      return GetSystemDefaultLCID();
    
    case LOCALE_USER_DEFAULT:
      return GetUserDefaultLCID();
    
    /*case LOCALE_NEUTRAL:
      return MAKELCID(LANG_NEUTRAL, SUBLANG_NEUTRAL);*/
  }
  
  /* ported from wine, is that right? */
  return MAKELANGID(PRIMARYLANGID(Locale), SUBLANG_NEUTRAL);
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumCalendarInfoW (
    CALINFO_ENUMPROCW lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumCalendarInfoA (
    CALINFO_ENUMPROCA lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumCalendarInfoExA(
    CALINFO_ENUMPROCEXA lpCalInfoEnumProcEx,
    LCID                Locale,
    CALID               Calendar,
    CALTYPE             CalType)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumCalendarInfoExW(
    CALINFO_ENUMPROCEXW lpCalInfoEnumProcEx,
    LCID                Locale,
    CALID               Calendar,
    CALTYPE             CalType)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumDateFormatsW (
    DATEFMT_ENUMPROCW  lpDateFmtEnumProc,
    LCID               Locale,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumDateFormatsA (
    DATEFMT_ENUMPROCA  lpDateFmtEnumProc,
    LCID               Locale,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumDateFormatsExA(
    DATEFMT_ENUMPROCEXA lpDateFmtEnumProcEx,
    LCID                Locale,
    DWORD               dwFlags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumDateFormatsExW(
    DATEFMT_ENUMPROCEXW lpDateFmtEnumProcEx,
    LCID                Locale,
    DWORD               dwFlags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumLanguageGroupLocalesA(
    LANGGROUPLOCALE_ENUMPROCA lpLangGroupLocaleEnumProc,
    LGRPID                    LanguageGroup,
    DWORD                     dwFlags,
    LONG_PTR                  lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumLanguageGroupLocalesW(
    LANGGROUPLOCALE_ENUMPROCW lpLangGroupLocaleEnumProc,
    LGRPID                    LanguageGroup,
    DWORD                     dwFlags,
    LONG_PTR                  lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemCodePagesW (
    CODEPAGE_ENUMPROCW  lpCodePageEnumProc,
    DWORD               dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemCodePagesA (
    CODEPAGE_ENUMPROCA lpCodePageEnumProc,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemGeoID(
    GEOCLASS        GeoClass,
    GEOID           ParentGeoId,
    GEO_ENUMPROC    lpGeoEnumProc)
{
  if(!lpGeoEnumProc)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  switch(GeoClass)
  {
    case GEOCLASS_NATION:
      /*RtlEnterCriticalSection(&DllLock);
      
        FIXME - Get GEO IDs calling Csr
      
      RtlLeaveCriticalSection(&DllLock);*/
      
      SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
      break;
    
    default:
      SetLastError(ERROR_INVALID_FLAGS);
      return FALSE;
  }
 
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemLanguageGroupsA(
    LANGUAGEGROUP_ENUMPROCA lpLanguageGroupEnumProc,
    DWORD                   dwFlags,
    LONG_PTR                lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemLocalesW (
    LOCALE_ENUMPROCW lpLocaleEnumProc,
    DWORD            dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemLocalesA (
    LOCALE_ENUMPROCA lpLocaleEnumProc,
    DWORD            dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumTimeFormatsW (
    TIMEFMT_ENUMPROCW    lpTimeFmtEnumProc,
    LCID            Locale,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumTimeFormatsA (
    TIMEFMT_ENUMPROCA  lpTimeFmtEnumProc,
    LCID               Locale,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumUILanguagesA(
    UILANGUAGE_ENUMPROCA lpUILanguageEnumProc,
    DWORD                dwFlags,
    LONG_PTR             lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
EnumUILanguagesW(
    UILANGUAGE_ENUMPROCW lpUILanguageEnumProc,
    DWORD                dwFlags,
    LONG_PTR             lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
UINT
STDCALL
GetACP (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 1252;
}

#endif


/*
 * @unimplemented
 */
int
STDCALL
GetCalendarInfoA(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPSTR   lpCalData,
    int      cchData,
    LPDWORD  lpValue)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetCalendarInfoW(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPWSTR   lpCalData,
    int      cchData,
    LPDWORD  lpValue)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
BOOL
STDCALL
GetCPInfo (
    UINT        CodePage,
    LPCPINFO    CodePageInfo
    )
{
    unsigned i;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    CodePageInfo->MaxCharSize = 1;
    CodePageInfo->DefaultChar[0] = '?';
    for (i = 1; i < MAX_DEFAULTCHAR; i++)
	{
	CodePageInfo->DefaultChar[i] = 0;
	}
    for (i = 0; i < MAX_LEADBYTES; i++)
	{
	CodePageInfo->LeadByte[i] = 0;
	}

    return TRUE;
}

#endif


/*
 * @unimplemented
 */
BOOL
STDCALL
GetCPInfoExW(
    UINT          CodePage,
    DWORD         dwFlags,
    LPCPINFOEXW  lpCPInfoEx)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetCPInfoExA(
    UINT          CodePage,
    DWORD         dwFlags,
    LPCPINFOEXA  lpCPInfoEx)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetCurrencyFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    LPCWSTR         lpValue,
    CONST CURRENCYFMTW   * lpFormat,
    LPWSTR          lpCurrencyStr,
    int         cchCurrency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetCurrencyFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    LPCSTR          lpValue,
    CONST CURRENCYFMTA   * lpFormat,
    LPSTR           lpCurrencyStr,
    int         cchCurrency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
int
STDCALL
GetDateFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpDate,
    LPCWSTR         lpFormat,
    LPWSTR          lpDateStr,
    int         cchDate
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetDateFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpDate,
    LPCSTR          lpFormat,
    LPSTR           lpDateStr,
    int         cchDate
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

#endif


/*
 * @unimplemented
 */
int
STDCALL
GetGeoInfoW(
    GEOID       Location,
    GEOTYPE     GeoType,
    LPWSTR     lpGeoData,
    int         cchData,
    LANGID      LangId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetGeoInfoA(
    GEOID       Location,
    GEOTYPE     GeoType,
    LPSTR     lpGeoData,
    int         cchData,
    LANGID      LangId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

const WCHAR *RosGetLocaleValueName( DWORD lctype )
{
    switch (lctype & ~LOCALE_LOCALEINFOFLAGSMASK)
    {
    /* These values are used by SetLocaleInfo and GetLocaleInfo, and
     * the values are stored in the registry, confirmed under Windows.
     */
    case LOCALE_ICALENDARTYPE:    return L"iCalendarType";
    case LOCALE_ICURRDIGITS:      return L"iCurrDigits";
    case LOCALE_ICURRENCY:        return L"iCurrency";
    case LOCALE_IDIGITS:          return L"iDigits";
    case LOCALE_IFIRSTDAYOFWEEK:  return L"iFirstDayOfWeek";
    case LOCALE_IFIRSTWEEKOFYEAR: return L"iFirstWeekOfYear";
    case LOCALE_ILZERO:           return L"iLZero";
    case LOCALE_IMEASURE:         return L"iMeasure";
    case LOCALE_INEGCURR:         return L"iNegCurr";
    case LOCALE_INEGNUMBER:       return L"iNegNumber";
    case LOCALE_IPAPERSIZE:       return L"iPaperSize";
    case LOCALE_ITIME:            return L"iTime";
    case LOCALE_S1159:            return L"s1159";
    case LOCALE_S2359:            return L"s2359";
    case LOCALE_SCURRENCY:        return L"sCurrency";
    case LOCALE_SDATE:            return L"sDate";
    case LOCALE_SDECIMAL:         return L"sDecimal";
    case LOCALE_SGROUPING:        return L"sGrouping";
    case LOCALE_SLIST:            return L"sList";
    case LOCALE_SLONGDATE:        return L"sLongDate";
    case LOCALE_SMONDECIMALSEP:   return L"sMonDecimalSep";
    case LOCALE_SMONGROUPING:     return L"sMonGrouping";
    case LOCALE_SMONTHOUSANDSEP:  return L"sMonThousandSep";
    case LOCALE_SNEGATIVESIGN:    return L"sNegativeSign";
    case LOCALE_SPOSITIVESIGN:    return L"sPositiveSign";
    case LOCALE_SSHORTDATE:       return L"sShortDate";
    case LOCALE_STHOUSAND:        return L"sThousand";
    case LOCALE_STIME:            return L"sTime";
    case LOCALE_STIMEFORMAT:      return L"sTimeFormat";
    case LOCALE_SYEARMONTH:       return L"sYearMonth";

    /* The following are not listed under MSDN as supported,
     * but seem to be used and also stored in the registry.
     */
    case LOCALE_ICOUNTRY:         return L"iCountry";
    case LOCALE_IDATE:            return L"iDate";
    case LOCALE_ILDATE:           return L"iLDate";
    case LOCALE_ITLZERO:          return L"iTLZero";
    case LOCALE_SCOUNTRY:         return L"sCountry";
    case LOCALE_SLANGUAGE:        return L"sLanguage";
    }
    return NULL;
}

HKEY RosCreateRegistryKey(void)
{
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING nameW;
    HANDLE hKey;

    if (RtlOpenCurrentUser( KEY_ALL_ACCESS, &hKey ) != STATUS_SUCCESS) return 0;

    objAttr.Length = sizeof(objAttr);
    objAttr.RootDirectory = hKey;
    objAttr.ObjectName = &nameW;
    objAttr.Attributes = 0;
    objAttr.SecurityDescriptor = NULL;
    objAttr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, L"Control Panel\\International");

    if (NtCreateKey( &hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 0, NULL ) != STATUS_SUCCESS) hKey = 0;
    NtClose( objAttr.RootDirectory );
    return hKey;
}

INT RosGetRegistryLocaleInfo( LPCWSTR lpValue, LPWSTR lpBuffer, INT nLen )
{
    DWORD dwSize;
    HKEY hKey;
	INT nRet;
    NTSTATUS ntStatus;
    UNICODE_STRING usNameW;
    KEY_VALUE_PARTIAL_INFORMATION *kvpiInfo;
    const int nInfoSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    if (!(hKey = RosCreateRegistryKey())) return -1;

    RtlInitUnicodeString( &usNameW, lpValue );
    dwSize = nInfoSize + nLen * sizeof(WCHAR);

    if (!(kvpiInfo = HeapAlloc( GetProcessHeap(), 0, dwSize )))
    {
        NtClose( hKey );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    ntStatus = NtQueryValueKey( hKey, &usNameW, KeyValuePartialInformation, kvpiInfo, dwSize, &dwSize );
    if (ntStatus == STATUS_BUFFER_OVERFLOW && !lpBuffer) ntStatus = 0;

    if (!ntStatus)
    {
        nRet = (dwSize - nInfoSize) / sizeof(WCHAR);
        
        if (!nRet || ((WCHAR *)kvpiInfo->Data)[nRet - 1])
        {
            if (nRet < nLen || !lpBuffer) nRet++;
            else
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                nRet = 0;
            }
        }
        if (nRet && lpBuffer)
        {
            memcpy( lpBuffer, kvpiInfo->Data, (nRet - 1) * sizeof(WCHAR) );
            lpBuffer[nRet - 1] = 0;
        }
    }
    else
    {
        if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND) nRet = -1;
        else
        {
            SetLastError( RtlNtStatusToDosError(ntStatus) );
            nRet = 0;
        }
    }
    NtClose( hKey );
    HeapFree( GetProcessHeap(), 0, kvpiInfo );
    return nRet;
}

/*
 * @implemented
 */
int
STDCALL
GetLocaleInfoW (
	LCID Locale,
    LCTYPE  LCType,
    LPWSTR  lpLCData,
    int cchData
    )
{
    LANGID liLangID;
    HRSRC hRsrc;
    HGLOBAL hMem;
    HMODULE hModule;
    INT nRet;
    UINT uiFlags;
    const WCHAR *ch;
    int i;

    if (cchData < 0 || (cchData && !lpLCData))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!cchData) lpLCData = NULL;

    if (Locale == LOCALE_NEUTRAL || Locale == LOCALE_SYSTEM_DEFAULT) Locale = GetSystemDefaultLCID();
    else if (Locale == LOCALE_USER_DEFAULT) Locale = GetUserDefaultLCID();

    uiFlags = LCType & LOCALE_LOCALEINFOFLAGSMASK;
    LCType &= ~LOCALE_LOCALEINFOFLAGSMASK;

    if (!(uiFlags & LOCALE_NOUSEROVERRIDE) && Locale == GetUserDefaultLCID())
    {
        const WCHAR *value = RosGetLocaleValueName(LCType);

        if (value && ((nRet = RosGetRegistryLocaleInfo( value, lpLCData, cchData )) != -1)) return nRet;
    }

    liLangID = LANGIDFROMLCID( Locale );

    if (SUBLANGID(liLangID) == SUBLANG_NEUTRAL)
        liLangID = MAKELANGID(PRIMARYLANGID(liLangID), SUBLANG_DEFAULT);

    hModule = GetModuleHandleW( L"kernel32.dll" );
    if (!(hRsrc = FindResourceExW( hModule, (LPWSTR)RT_STRING, (LPCWSTR)((LCType >> 4) + 1), liLangID )))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!(hMem = LoadResource( hModule, hRsrc )))
        return 0;

    ch = LockResource( hMem );
    for (i = 0; i < (LCType & 0x0f); i++) ch += *ch + 1;

    if (uiFlags & LOCALE_RETURN_NUMBER) nRet = sizeof(UINT) / sizeof(WCHAR);
    else nRet = (LCType == LOCALE_FONTSIGNATURE) ? *ch : *ch + 1;

    if (!lpLCData) return nRet;

    if (nRet > cchData)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (uiFlags & LOCALE_RETURN_NUMBER)
    {
        UINT uiNum;
        WCHAR *chEnd, *chTmp = HeapAlloc( GetProcessHeap(), 0, (*ch + 1) * sizeof(WCHAR) );

        if (!chTmp) 
			return 0;

        memcpy( chTmp, ch + 1, *ch * sizeof(WCHAR) );
        chTmp[*ch] = L'\0';
        uiNum = wcstol( chTmp, &chEnd, 10 );

        if (!*chEnd)
            memcpy( lpLCData, &uiNum, sizeof(uiNum) );
        else
        {
            SetLastError( ERROR_INVALID_FLAGS );
            nRet = 0;
        }
        HeapFree( GetProcessHeap(), 0, chTmp );
    }
    else
    {
        memcpy( lpLCData, ch + 1, *ch * sizeof(WCHAR) );
        if (LCType != LOCALE_FONTSIGNATURE) lpLCData[nRet-1] = 0;
    }
    return nRet;
}



/***********************************************************************
 *    get_lcid_codepage
 *
 * Retrieve the ANSI codepage for a given locale.
 */
inline static UINT get_lcid_codepage( LCID lcid )
{
    UINT ret;
    if (!GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE|LOCALE_RETURN_NUMBER, (WCHAR *)&ret,
                         sizeof(ret)/sizeof(WCHAR) )) ret = 0;
    return ret;
}



/*
 * @implemented
 *
 * Get information about an aspect of a locale.
 *
 * PARAMS
 *  lcid   [I] LCID of the locale
 *  lctype [I] LCTYPE_ flags from "winnls.h"
 *  buffer [O] Destination for the information
 *  len    [I] Length of buffer in characters
 *
 * RETURNS
 *  Success: The size of the data requested. If buffer is non-NULL, it is filled
 *           with the information.
 *  Failure: 0. Use GetLastError() to determine the cause.
 *
 * NOTES
 *  - LOCALE_NEUTRAL is equal to LOCALE_SYSTEM_DEFAULT
 *  - The string returned is NUL terminated, except for LOCALE_FONTSIGNATURE,
 *    which is a bit string.
 */
INT STDCALL GetLocaleInfoA( LCID lcid, LCTYPE lctype, LPSTR buffer, INT len )
{
    WCHAR *bufferW;
    INT lenW, ret;

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!len) buffer = NULL;

    if (!(lenW = GetLocaleInfoW( lcid, lctype, NULL, 0 ))) return 0;

    if (!(bufferW = HeapAlloc( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    if ((ret = GetLocaleInfoW( lcid, lctype, bufferW, lenW )))
    {
        if ((lctype & LOCALE_RETURN_NUMBER) ||
            ((lctype & ~LOCALE_LOCALEINFOFLAGSMASK) == LOCALE_FONTSIGNATURE))
        {
            /* it's not an ASCII string, just bytes */
            ret *= sizeof(WCHAR);
            if (buffer)
            {
                if (ret <= len) memcpy( buffer, bufferW, ret );
                else
                {
                    SetLastError( ERROR_INSUFFICIENT_BUFFER );
                    ret = 0;
                }
            }
        }
        else
        {
            UINT codepage = CP_ACP;
            if (!(lctype & LOCALE_USE_CP_ACP)) codepage = get_lcid_codepage( lcid );
            ret = WideCharToMultiByte( codepage, 0, bufferW, ret, buffer, len, NULL, NULL );
        }
    }
    HeapFree( GetProcessHeap(), 0, bufferW );
    return ret;
}


/*
 * @unimplemented
 */
int
STDCALL
GetNumberFormatW (
    LCID        Locale,
    DWORD       dwFlags,
    LPCWSTR     lpValue,
    CONST NUMBERFMTW * lpFormat,
    LPWSTR      lpNumberStr,
    int     cchNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetNumberFormatA (
    LCID        Locale,
    DWORD       dwFlags,
    LPCSTR      lpValue,
    CONST NUMBERFMTA * lpFormat,
    LPSTR       lpNumberStr,
    int     cchNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetOEMCP (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 437; /* FIXME: call csrss.exe */
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
LANGID
STDCALL
GetSystemDefaultLangID (VOID)
{
     /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELANGID(
        LANG_ENGLISH,
        SUBLANG_ENGLISH_US
        );
}


/*
 * @implemented
 */
LCID
STDCALL
GetSystemDefaultLCID (VOID)
{
  return SystemLocale;
}

#endif


/*
 * @unimplemented
 */
LANGID
STDCALL
GetSystemDefaultUILanguage(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @implemented
 */
LCID
STDCALL
GetThreadLocale (VOID)
{
  return NtCurrentTeb()->CurrentLocale;
}

#endif

INT RosGetTimeFormat(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime)
{
	INT nPos = 0, nLastFormatPos = 0;
	BOOL bDrop = FALSE;

	while( *lpFormat )
	{
		if (*lpFormat == L'\'')
		{
			lpFormat++;

			while(*lpFormat)
			{
				if (*lpFormat == L'\'')
				{
					lpFormat++;
					if(*lpFormat != L'\'')
						break;
				}
				if (!cchTime)
					nPos++;
				else if(nPos > cchTime)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}
				else
				{
					if(!bDrop)
					{
						lpTimeStr[nPos] = *lpFormat;
						nPos++;
					}
				}
				*lpFormat++;
			}
		}
		else if(*lpFormat==L'H' || *lpFormat==L'h' || *lpFormat==L'm' || *lpFormat==L's' || *lpFormat==L't' )
		{
			int nCount, nBufLen;
			WCHAR nType = *lpFormat;
			WCHAR Buffer[40];

			bDrop = FALSE;

			Buffer[0] = 0;

			for(nCount = 1; *lpFormat == nType; lpFormat++)
				nCount++;

			switch(nType)
			{
			case L'h':
				{
					if(!(dwFlags & TIME_FORCE24HOURFORMAT))
					{
						swprintf( Buffer, L"%.*d", nCount > 2 ? 2 : nCount,
							lpTime->wHour == 0 ? 12 : (lpTime->wHour - 1) % 12 + 1);
						break;
					}
				}
			case L'H':
				{
					swprintf( Buffer, L"%.*d", nCount > 2 ? 2 : nCount, lpTime->wHour );
					break;
				}
			case L'm':
				{
					if(!(dwFlags & TIME_NOMINUTESORSECONDS))
					{
						swprintf( Buffer, L"%.*d", nCount > 2 ? 2 : nCount, lpTime->wMinute );
					}
					else
						nPos = nLastFormatPos;

					break;
				}
			case L's':
				{
					if(!(dwFlags & (TIME_NOSECONDS|TIME_NOMINUTESORSECONDS)))
					{
					    swprintf( Buffer, L"%.*d", nCount > 2 ? 2 : nCount, lpTime->wSecond );				
					}
					else
						nPos = nLastFormatPos;

					break;
				}
			case L't':
				{
					if(!(dwFlags & TIME_NOTIMEMARKER))
					{
						GetLocaleInfoW(Locale, (lpTime->wHour < 12) ? LOCALE_S1159 : LOCALE_S2359, Buffer, sizeof(Buffer) );
						if(nCount == 1)
							Buffer[1] = 0;
					}
					else
					{
						nPos = nLastFormatPos;
						bDrop = TRUE;
					}
					break;
				}
			}
			nBufLen = wcslen(Buffer);

			if(!cchTime)
			{
				/* wine does nothing here?!? */
			}
			else if(nPos + nBufLen < cchTime)
				wcscpy( lpTimeStr + nPos, Buffer );
			else
			{
				lstrcpynW( lpTimeStr + nPos, Buffer, cchTime - nPos );
				
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return 0;
			}
			nPos += nBufLen;
			nLastFormatPos = nPos;
		}
		else
		{
			if(!cchTime)
				nPos++;
			else if(nPos > cchTime)
			{
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return 0;
			}
			else
			{
				if(!bDrop)
				{
					lpTimeStr[nPos] = *lpFormat;
					nPos++;
				}
			}
		lpFormat++;
		}
	}

	/* Are we not only counting? */
	if (cchTime)
	{
		if (nPos >= cchTime)
		{
			SetLastError(ERROR_INSUFFICIENT_BUFFER);
			return 0;
		}
		else
			lpTimeStr[nPos] = L'\0';
	}

	nPos++;
	return nPos;
}

/*
 * @implemented
 */
INT
STDCALL
GetTimeFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME* lpTime,
    LPCWSTR         lpFormat,
    LPWSTR          lpTimeStr,
    int             cchTime
    )
{
	WCHAR Buffer[40];
	SYSTEMTIME t;

    if (!Locale)
		Locale = LOCALE_SYSTEM_DEFAULT;
        
	Locale = ConvertDefaultLocale( Locale );

	if (lpFormat == NULL)
	{
	  if (dwFlags & LOCALE_NOUSEROVERRIDE)
		  Locale = GetSystemDefaultLCID();

	  if (!GetLocaleInfoW(Locale, LOCALE_STIMEFORMAT, Buffer, 40))
		return 0;

	  lpFormat = Buffer;
	}
	if (dwFlags & LOCALE_NOUSEROVERRIDE)
    {
		SetLastError(ERROR_INVALID_FLAGS);
	    return 0;
    }
	if (lpTime == NULL)
	{
		GetLocalTime(&t);
		lpTime = &t;
	}
	if((lpTime->wHour > 24) || (lpTime->wMinute >= 60) || (lpTime->wSecond >= 60))
    {
		SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
	return RosGetTimeFormat(Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);
}


/*
 * @implemented
 */
int
STDCALL
GetTimeFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME* lpTime,
    LPCSTR          lpFormat,
    LPSTR           lpTimeStr,
    int             cchTime
    )
{
	LPWSTR lpFormatU = NULL;
	LPWSTR lpTimeStrU;
	int numCharsU;
	int retVal = 0;

	if (lpFormat != NULL) {
		/* First just determine the number of necessary bytes 
		for the unicode string */
		int numBytes = MultiByteToWideChar(CP_ACP, 0, lpFormat, -1, NULL, 0);

		if (numBytes > 0) {
			lpFormatU = HeapAlloc(GetProcessHeap(), 0, numBytes);

			if (lpFormatU != NULL) 
				MultiByteToWideChar(CP_ACP, 0, lpFormat, -1, lpFormatU, numBytes);
		}
	}

	/* Just get the number of characters needed to store the 
	   Unicode output */
	numCharsU = GetTimeFormatW(Locale, dwFlags, lpTime, lpFormatU, NULL, 0);

	if (numCharsU != 0) {
		lpTimeStrU = HeapAlloc(GetProcessHeap(), 0, numCharsU*sizeof(WCHAR));

		if (lpTimeStrU != NULL) {
			if (GetTimeFormatW(Locale, dwFlags, lpTime, lpFormatU, lpTimeStrU, numCharsU))
				/* Convert the output string to ANSI */
				retVal = WideCharToMultiByte(CP_ACP, 0, lpTimeStrU, numCharsU, lpTimeStr, cchTime, NULL, NULL);

			HeapFree(GetProcessHeap(), 0, lpTimeStrU);
		}
	}

	if (lpFormatU != NULL)
		HeapFree(GetProcessHeap(), 0, lpFormatU);

	return retVal;
}



#ifndef _OLE2NLS_IN_BUILD_

/*
 * @implemented
 */
LANGID
STDCALL
GetUserDefaultLangID (VOID)
{
    return LANGIDFROMLCID(GetUserDefaultLCID());
}


/*
 * @implemented
 */
LCID
STDCALL
GetUserDefaultLCID (VOID)
{
    LCID lcid;
    NtQueryDefaultLocale(TRUE, &lcid);
    return lcid;
}

#endif


/*
 * @unimplemented
 */
LANGID
STDCALL
GetUserDefaultUILanguage(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
GEOID
STDCALL
GetUserGeoID(
    GEOCLASS    GeoClass)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
IsValidCodePage (
    UINT    CodePage
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
IsValidLanguageGroup(
    LGRPID  LanguageGroup,
    DWORD   dwFlags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
IsValidLocale (
    LCID    Locale,
    DWORD   dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
int
STDCALL
LCMapStringA (
    LCID    Locale,
    DWORD   dwMapFlags,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPSTR   lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
LCMapStringW (
    LCID    Locale,
    DWORD   dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR  lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

#endif


/*
 * @unimplemented
 */
BOOL
STDCALL
SetCalendarInfoA(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPCSTR  lpCalData)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetCalendarInfoW(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPCWSTR  lpCalData)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
BOOL
STDCALL
SetLocaleInfoA (
    LCID    Locale,
    LCTYPE  LCType,
    LPCSTR  lpLCData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetLocaleInfoW (
    LCID    Locale,
    LCTYPE  LCType,
    LPCWSTR lpLCData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
SetThreadLocale (
    LCID    Locale
    )
{
  /* FIXME - Check if locale is valid */
  
  NtCurrentTeb()->CurrentLocale = Locale;
  return TRUE;
}

#endif


/*
 * @unimplemented
 */
BOOL
STDCALL
SetUserGeoID(
    GEOID       GeoId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
