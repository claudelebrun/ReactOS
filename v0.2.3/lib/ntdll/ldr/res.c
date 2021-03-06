/* $Id: res.c,v 1.6 2003/12/20 21:43:27 navaraf Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/ldr/res.c
 * PURPOSE:         Resource access for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 *                  Robert Dickenson (robd@mok.lvcm.com)
 */

/*
 * TODO:
 *  - any comments ??
 */

/* INCLUDES *****************************************************************/

#include <reactos/config.h>
#include <ddk/ntddk.h>
#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <ntdll/ldr.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* PROTOTYPES ****************************************************************/



/* FUNCTIONS *****************************************************************/

/*
	Status = LdrFindResource_U (hModule,
				    &ResourceInfo,
				    RESOURCE_DATA_LEVEL,
				    &ResourceDataEntry);
 */
NTSTATUS STDCALL
LdrFindResource_U(PVOID BaseAddress,
                  PLDR_RESOURCE_INFO ResourceInfo,
                  ULONG Level,
                  PIMAGE_RESOURCE_DATA_ENTRY* ResourceDataEntry)
{
    PIMAGE_RESOURCE_DIRECTORY ResDir;
    PIMAGE_RESOURCE_DIRECTORY ResBase;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
    NTSTATUS Status = STATUS_SUCCESS;
    PWCHAR ws;
    ULONG i;
    ULONG Id;
    LONG low, high, mid, result;

    DPRINT("LdrFindResource_U(%08x, %08x, %d, %08x)\n", BaseAddress, ResourceInfo, Level, ResourceDataEntry);

    /* Get the pointer to the resource directory */
    ResDir = (PIMAGE_RESOURCE_DIRECTORY)RtlImageDirectoryEntryToData(BaseAddress,
                      TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &i);
    if (ResDir == NULL) {
        return STATUS_RESOURCE_DATA_NOT_FOUND;
    }

    DPRINT("ResourceDirectory: %x  Size: %d\n", (ULONG)ResDir, (int)i);

    ResBase = ResDir;

    /* Let's go into resource tree */
    for (i = 0; i < Level; i++) {
        DPRINT("ResDir: %x  Level: %d\n", (ULONG)ResDir, i);

        Id = ((PULONG)ResourceInfo)[i];
//	ResourceInfo.Type = (ULONG)lpType;
//	ResourceInfo.Name = (ULONG)lpName;
//	ResourceInfo.Language = (ULONG)wLanguage;

        if (Id & 0xFFFF0000) {
            /* Resource name is a unicode string */
            ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1);
            DPRINT("ResEntry %x - Resource name is a unicode string\n", (ULONG)ResEntry);
            DPRINT("EntryCount %d\n", (ULONG)ResDir->NumberOfNamedEntries);

            low = 0;
            high = ResDir->NumberOfNamedEntries - 1;
            mid = high/2;
            while( low <= high ) {
               /* Does we need check if it's named entry, think not */
               ws = (PWCHAR)((ULONG)ResBase + (ResEntry[mid].Name & 0x7FFFFFFF));
               result = _wcsnicmp((PWCHAR)Id, ws + 1, *ws);
               /* Need double check for lexical & length */
               if(result == 0) {
                  result = (wcslen((PWCHAR)Id) - (int)*ws);
                  if(result == 0) goto found;
               }
               if(result < 0)
                  high = mid - 1;
               else
                  low = mid + 1;
				
               mid = (low + high)/2;
            }
        } else {
            /* We use ID number instead of string */
            ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1) + ResDir->NumberOfNamedEntries;
            DPRINT("ResEntry %x - Resource ID number instead of string\n", (ULONG)ResEntry);
            DPRINT("EntryCount %d\n", (ULONG)ResDir->NumberOfIdEntries);

            low = 0;
            high = ResDir->NumberOfIdEntries - 1;
            mid = high/2;
            while( low <= high ) {
               result = Id - ResEntry[mid].Name;
               if(result == 0) goto found;
               if(result < 0)
                  high = mid - 1;
               else
                  low = mid + 1;
				
               mid = (low + high)/2;
             }
        }

        switch (i) {
        case 0:
            DPRINT("Error %lu - STATUS_RESOURCE_TYPE_NOT_FOUND\n", i);
            return STATUS_RESOURCE_TYPE_NOT_FOUND;
        case 1:
            DPRINT("Error %lu - STATUS_RESOURCE_NAME_NOT_FOUND\n", i);
            return STATUS_RESOURCE_NAME_NOT_FOUND;
        case 2:
            if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries) {
                /* Use the first available language */
                ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
                break;
            }
            DPRINT("Error %lu - STATUS_RESOURCE_LANG_NOT_FOUND\n", i);
            return STATUS_RESOURCE_LANG_NOT_FOUND;
         case 3:
            DPRINT("Error %lu - STATUS_RESOURCE_DATA_NOT_FOUND\n", i);
            return STATUS_RESOURCE_DATA_NOT_FOUND;
         default:
            DPRINT("Error %lu - STATUS_INVALID_PARAMETER\n", i);
            return STATUS_INVALID_PARAMETER;
        }
found:;
        ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResBase +
                     (ResEntry[mid].OffsetToData & 0x7FFFFFFF));
    }
    DPRINT("ResourceDataEntry: %x\n", (ULONG)ResDir);

    if (ResourceDataEntry) {
        *ResourceDataEntry = (PVOID)ResDir;
    }
    return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrAccessResource(IN  PVOID BaseAddress,
                  IN  PIMAGE_RESOURCE_DATA_ENTRY ResourceDataEntry,
                  OUT PVOID* Resource OPTIONAL,
                  OUT PULONG Size OPTIONAL)
{
    PIMAGE_SECTION_HEADER Section;
    PIMAGE_NT_HEADERS NtHeader;
    ULONG SectionRva;
    ULONG SectionVa;
    ULONG DataSize;
    ULONG Offset = 0;
    ULONG Data;

    if(!ResourceDataEntry)
        return STATUS_RESOURCE_DATA_NOT_FOUND;

    Data = (ULONG)RtlImageDirectoryEntryToData(BaseAddress,
                           TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &DataSize);
    if (Data == 0) {
        return STATUS_RESOURCE_DATA_NOT_FOUND;
    }
    if ((ULONG)BaseAddress & 1) {
        /* loaded as ordinary file */
        NtHeader = RtlImageNtHeader((PVOID)((ULONG)BaseAddress & ~1UL));
        Offset = (ULONG)BaseAddress - Data + NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress;
        Section = RtlImageRvaToSection(NtHeader, BaseAddress, NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE].VirtualAddress);
        if (Section == NULL) {
            return STATUS_RESOURCE_DATA_NOT_FOUND;
        }
        if (Section->Misc.VirtualSize < ResourceDataEntry->OffsetToData) {
            SectionRva = RtlImageRvaToSection (NtHeader, BaseAddress, ResourceDataEntry->OffsetToData)->VirtualAddress;
            SectionVa = RtlImageRvaToVa(NtHeader, BaseAddress, SectionRva, NULL);
            Offset = SectionRva - SectionVa + Data - Section->VirtualAddress;
        }
    }
    if (Resource) {
        *Resource = (PVOID)(ResourceDataEntry->OffsetToData - Offset + (ULONG)BaseAddress);
    }
    if (Size) {
        *Size = ResourceDataEntry->Size;
    }
    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
LdrFindResourceDirectory_U(IN PVOID BaseAddress,
                           WCHAR** name,
                           DWORD level,
                           OUT PVOID* addr)
{
    PIMAGE_RESOURCE_DIRECTORY ResDir;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY ResEntry;
    ULONG EntryCount;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;
    WCHAR* ws;

    /* Get the pointer to the resource directory */
    ResDir = (PIMAGE_RESOURCE_DIRECTORY)
    RtlImageDirectoryEntryToData(BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_RESOURCE, &i);
    if (ResDir == NULL) {
        return STATUS_RESOURCE_DATA_NOT_FOUND;
    }

    /* Let's go into resource tree */
    for (i = 0; i < level; i++, name++) {
        EntryCount = ResDir->NumberOfNamedEntries;
        ResEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(ResDir + 1);
        if ((ULONG)(*name) & 0xFFFF0000) {
            /* Resource name is a unicode string */
            for (; EntryCount--; ResEntry++) {
                /* Scan entries for equal name */
                if (ResEntry->Name & 0x80000000) {
                    ws = (WCHAR*)((ULONG)ResDir + (ResEntry->Name & 0x7FFFFFFF));
                    if (!wcsncmp(*name, ws + 1, *ws) && wcslen(*name) == (int)*ws) {
                        goto found;
                    }
                }
            }
        } else {
            /* We use ID number instead of string */
            ResEntry += EntryCount;
            EntryCount = ResDir->NumberOfIdEntries;
            for (; EntryCount--; ResEntry++) {
                /* Scan entries for equal name */
                if (ResEntry->Name == (ULONG)(*name))
                    goto found;
            }
        }
        switch (i) {
        case 0:
            return STATUS_RESOURCE_TYPE_NOT_FOUND;
        case 1:
            return STATUS_RESOURCE_NAME_NOT_FOUND;
        case 2:
            Status = STATUS_RESOURCE_LANG_NOT_FOUND;
            /* Just use first language entry */
            if (ResDir->NumberOfNamedEntries || ResDir->NumberOfIdEntries) {
                ResEntry = (IMAGE_RESOURCE_DIRECTORY_ENTRY*)(ResDir + 1);
                break;
            }
            return Status;
        case 3:
            return STATUS_RESOURCE_DATA_NOT_FOUND;
        default:
            return STATUS_INVALID_PARAMETER;
        }
found:;
        ResDir = (PIMAGE_RESOURCE_DIRECTORY)((ULONG)ResDir + ResEntry->OffsetToData);
    }
    if (addr) {
        *addr = (PVOID)ResDir;
    }
    return Status;
}

/* EOF */
