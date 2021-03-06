/*
 *	IEnumIDList
 *
 *	Copyright 1998	Juergen Schmied <juergen.schmied@metronet.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "undocshell.h"
#include "shlwapi.h"
#include "winerror.h"
#include "objbase.h"

#include "pidl.h"
#include "shlguid.h"
#include "enumidlist.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct tagENUMLIST
{
	struct tagENUMLIST	*pNext;
	LPITEMIDLIST		pidl;

} ENUMLIST, *LPENUMLIST;

typedef struct
{
	ICOM_VFIELD(IEnumIDList);
	DWORD				ref;
	LPENUMLIST			mpFirst;
	LPENUMLIST			mpLast;
	LPENUMLIST			mpCurrent;

} IEnumIDListImpl;

static struct ICOM_VTABLE(IEnumIDList) eidlvt;

/**************************************************************************
 *  AddToEnumList()
 */
BOOL AddToEnumList(
	IEnumIDList * iface,
	LPITEMIDLIST pidl)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPENUMLIST  pNew;

	TRACE("(%p)->(pidl=%p)\n",This,pidl);

    if (!iface || !pidl)
        return FALSE;

	pNew = (LPENUMLIST)SHAlloc(sizeof(ENUMLIST));
	if(pNew)
	{
	  /*set the next pointer */
	  pNew->pNext = NULL;
	  pNew->pidl = pidl;

	  /*is This the first item in the list? */
	  if(!This->mpFirst)
	  {
	    This->mpFirst = pNew;
	    This->mpCurrent = pNew;
	  }

	  if(This->mpLast)
	  {
	    /*add the new item to the end of the list */
	    This->mpLast->pNext = pNew;
	  }

	  /*update the last item pointer */
	  This->mpLast = pNew;
	  TRACE("-- (%p)->(first=%p, last=%p)\n",This,This->mpFirst,This->mpLast);
	  return TRUE;
	}
	return FALSE;
}

/**************************************************************************
 *  CreateFolderEnumList()
 */
BOOL CreateFolderEnumList(
	IEnumIDList *list,
	LPCSTR lpszPath,
	DWORD dwFlags)
{
    LPITEMIDLIST pidl=NULL;
    WIN32_FIND_DATAA stffile;
    HANDLE hFile;
    CHAR  szPath[MAX_PATH];
    BOOL succeeded = TRUE;

    TRACE("(%p)->(path=%s flags=0x%08lx) \n",list,debugstr_a(lpszPath),dwFlags);

    if(!lpszPath || !lpszPath[0]) return FALSE;

    strcpy(szPath, lpszPath);
    PathAddBackslashA(szPath);
    strcat(szPath,"*.*");

    hFile = FindFirstFileA(szPath,&stffile);
    if ( hFile != INVALID_HANDLE_VALUE )
    {
        BOOL findFinished = FALSE;

        do
        {
            if ( !(stffile.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) 
             || (dwFlags & SHCONTF_INCLUDEHIDDEN) )
            {
                if ( (stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                 dwFlags & SHCONTF_FOLDERS &&
                 strcmp (stffile.cFileName, ".") && strcmp (stffile.cFileName, ".."))
                {
                    pidl = _ILCreateFromFindDataA(&stffile);
                    succeeded = succeeded && AddToEnumList(list, pidl);
                }
                else if (!(stffile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                 && dwFlags & SHCONTF_NONFOLDERS)
                {
                    pidl = _ILCreateFromFindDataA(&stffile);
                    succeeded = succeeded && AddToEnumList(list, pidl);
                }
            }
            if (succeeded)
            {
                if (!FindNextFileA(hFile, &stffile))
                {
                    if (GetLastError() == ERROR_NO_MORE_FILES)
                        findFinished = TRUE;
                    else
                        succeeded = FALSE;
                }
            }
        } while (succeeded && !findFinished);
        FindClose(hFile);
    }
    return succeeded;
}

/**************************************************************************
*   DeleteList()
*/
static BOOL DeleteList(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	LPENUMLIST  pDelete;

	TRACE("(%p)->()\n",This);

	while(This->mpFirst)
	{ pDelete = This->mpFirst;
	  This->mpFirst = pDelete->pNext;
	  SHFree(pDelete->pidl);
	  SHFree(pDelete);
	}
	This->mpFirst = This->mpLast = This->mpCurrent = NULL;
	return TRUE;
}

/**************************************************************************
 *  IEnumIDList_Folder_Constructor
 *
 */

IEnumIDList * IEnumIDList_Constructor(void)
{
    IEnumIDListImpl *lpeidl = (IEnumIDListImpl*)HeapAlloc(GetProcessHeap(),
     HEAP_ZERO_MEMORY, sizeof(IEnumIDListImpl));

    if (lpeidl)
    {
        lpeidl->ref = 1;
        lpeidl->lpVtbl = &eidlvt;
    }
    TRACE("-- (%p)->()\n",lpeidl);

    return (IEnumIDList*)lpeidl;
}

/**************************************************************************
 *  EnumIDList_QueryInterface
 */
static HRESULT WINAPI IEnumIDList_fnQueryInterface(
	IEnumIDList * iface,
	REFIID riid,
	LPVOID *ppvObj)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This;
	}
	else if(IsEqualIID(riid, &IID_IEnumIDList))  /*IEnumIDList*/
	{    *ppvObj = (IEnumIDList*)This;
	}

	if(*ppvObj)
	{ IEnumIDList_AddRef((IEnumIDList*)*ppvObj);
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}

	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/******************************************************************************
 * IEnumIDList_fnAddRef
 */
static ULONG WINAPI IEnumIDList_fnAddRef(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);
	TRACE("(%p)->(%lu)\n",This,This->ref);
	return ++(This->ref);
}
/******************************************************************************
 * IEnumIDList_fnRelease
 */
static ULONG WINAPI IEnumIDList_fnRelease(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->(%lu)\n",This,This->ref);

	if (!--(This->ref)) {
	  TRACE(" destroying IEnumIDList(%p)\n",This);
	  DeleteList((IEnumIDList*)This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

/**************************************************************************
 *  IEnumIDList_fnNext
 */

static HRESULT WINAPI IEnumIDList_fnNext(
	IEnumIDList * iface,
	ULONG celt,
	LPITEMIDLIST * rgelt,
	ULONG *pceltFetched)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	ULONG    i;
	HRESULT  hr = S_OK;
	LPITEMIDLIST  temp;

	TRACE("(%p)->(%ld,%p, %p)\n",This,celt,rgelt,pceltFetched);

/* It is valid to leave pceltFetched NULL when celt is 1. Some of explorer's
 * subsystems actually use it (and so may a third party browser)
 */
	if(pceltFetched)
	  *pceltFetched = 0;

	*rgelt=0;

	if(celt > 1 && !pceltFetched)
	{ return E_INVALIDARG;
	}

	if(celt > 0 && !This->mpCurrent)
	{ return S_FALSE;
	}

	for(i = 0; i < celt; i++)
	{ if(!(This->mpCurrent))
	    break;

	  temp = ILClone(This->mpCurrent->pidl);
	  rgelt[i] = temp;
	  This->mpCurrent = This->mpCurrent->pNext;
	}
	if(pceltFetched)
	{  *pceltFetched = i;
	}

	return hr;
}

/**************************************************************************
*  IEnumIDList_fnSkip
*/
static HRESULT WINAPI IEnumIDList_fnSkip(
	IEnumIDList * iface,ULONG celt)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	DWORD    dwIndex;
	HRESULT  hr = S_OK;

	TRACE("(%p)->(%lu)\n",This,celt);

	for(dwIndex = 0; dwIndex < celt; dwIndex++)
	{ if(!This->mpCurrent)
	  { hr = S_FALSE;
	    break;
	  }
	  This->mpCurrent = This->mpCurrent->pNext;
	}
	return hr;
}
/**************************************************************************
*  IEnumIDList_fnReset
*/
static HRESULT WINAPI IEnumIDList_fnReset(
	IEnumIDList * iface)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)\n",This);
	This->mpCurrent = This->mpFirst;
	return S_OK;
}
/**************************************************************************
*  IEnumIDList_fnClone
*/
static HRESULT WINAPI IEnumIDList_fnClone(
	IEnumIDList * iface,LPENUMIDLIST * ppenum)
{
	ICOM_THIS(IEnumIDListImpl,iface);

	TRACE("(%p)->() to (%p)->() E_NOTIMPL\n",This,ppenum);
	return E_NOTIMPL;
}

/**************************************************************************
 *  IEnumIDList_fnVTable
 */
static ICOM_VTABLE (IEnumIDList) eidlvt =
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IEnumIDList_fnQueryInterface,
	IEnumIDList_fnAddRef,
	IEnumIDList_fnRelease,
	IEnumIDList_fnNext,
	IEnumIDList_fnSkip,
	IEnumIDList_fnReset,
	IEnumIDList_fnClone,
};
