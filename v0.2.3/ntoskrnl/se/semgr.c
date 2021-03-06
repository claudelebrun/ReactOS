/* $Id: semgr.c,v 1.30 2004/05/20 12:42:11 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/semgr.c
 * PROGRAMER:         ?
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>
#include <internal/se.h>

#include <internal/debug.h>

#define TAG_SXPT   TAG('S', 'X', 'P', 'T')


/* GLOBALS ******************************************************************/

PSE_EXPORTS EXPORTED SeExports = NULL;


/* PROTOTYPES ***************************************************************/

static BOOLEAN SepInitExports(VOID);

/* FUNCTIONS ****************************************************************/


BOOLEAN INIT_FUNCTION
SeInit1(VOID)
{
  SepInitLuid();

  if (!SepInitSecurityIDs())
    return FALSE;

  if (!SepInitDACLs())
    return FALSE;

  if (!SepInitSDs())
    return FALSE;

  SepInitPrivileges();

  if (!SepInitExports())
    return FALSE;

  return TRUE;
}


BOOLEAN INIT_FUNCTION
SeInit2(VOID)
{
  SepInitializeTokenImplementation();

  return TRUE;
}


BOOLEAN
SeInitSRM(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING Name;
  HANDLE DirectoryHandle;
  HANDLE EventHandle;
  NTSTATUS Status;

  /* Create '\Security' directory */
  RtlInitUnicodeString(&Name,
		       L"\\Security");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     0,
			     NULL);
  Status = NtCreateDirectoryObject(&DirectoryHandle,
				   DIRECTORY_ALL_ACCESS,
				   &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create 'Security' directory!\n");
      return FALSE;
    }

  /* Create 'LSA_AUTHENTICATION_INITALIZED' event */
  RtlInitUnicodeString(&Name,
		       L"\\LSA_AUTHENTICATION_INITALIZED");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT,
			     DirectoryHandle,
			     SePublicDefaultSd);
  Status = NtCreateEvent(&EventHandle,
			 EVENT_ALL_ACCESS,
			 &ObjectAttributes,
			 FALSE,
			 FALSE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Failed to create 'LSA_AUTHENTICATION_INITALIZED' event!\n");
      NtClose(DirectoryHandle);
      return FALSE;
    }

  NtClose(EventHandle);
  NtClose(DirectoryHandle);

  /* FIXME: Create SRM port and listener thread */

  return TRUE;
}


static BOOLEAN INIT_FUNCTION
SepInitExports(VOID)
{
  SeExports = ExAllocatePoolWithTag(NonPagedPool,
				    sizeof(SE_EXPORTS),
				    TAG_SXPT);
  if (SeExports == NULL)
    return FALSE;

  SeExports->SeCreateTokenPrivilege = SeCreateTokenPrivilege;
  SeExports->SeAssignPrimaryTokenPrivilege = SeAssignPrimaryTokenPrivilege;
  SeExports->SeLockMemoryPrivilege = SeLockMemoryPrivilege;
  SeExports->SeIncreaseQuotaPrivilege = SeIncreaseQuotaPrivilege;
  SeExports->SeUnsolicitedInputPrivilege = SeUnsolicitedInputPrivilege;
  SeExports->SeTcbPrivilege = SeTcbPrivilege;
  SeExports->SeSecurityPrivilege = SeSecurityPrivilege;
  SeExports->SeTakeOwnershipPrivilege = SeTakeOwnershipPrivilege;
  SeExports->SeLoadDriverPrivilege = SeLoadDriverPrivilege;
  SeExports->SeCreatePagefilePrivilege = SeCreatePagefilePrivilege;
  SeExports->SeIncreaseBasePriorityPrivilege = SeIncreaseBasePriorityPrivilege;
  SeExports->SeSystemProfilePrivilege = SeSystemProfilePrivilege;
  SeExports->SeSystemtimePrivilege = SeSystemtimePrivilege;
  SeExports->SeProfileSingleProcessPrivilege = SeProfileSingleProcessPrivilege;
  SeExports->SeCreatePermanentPrivilege = SeCreatePermanentPrivilege;
  SeExports->SeBackupPrivilege = SeBackupPrivilege;
  SeExports->SeRestorePrivilege = SeRestorePrivilege;
  SeExports->SeShutdownPrivilege = SeShutdownPrivilege;
  SeExports->SeDebugPrivilege = SeDebugPrivilege;
  SeExports->SeAuditPrivilege = SeAuditPrivilege;
  SeExports->SeSystemEnvironmentPrivilege = SeSystemEnvironmentPrivilege;
  SeExports->SeChangeNotifyPrivilege = SeChangeNotifyPrivilege;
  SeExports->SeRemoteShutdownPrivilege = SeRemoteShutdownPrivilege;

  SeExports->SeNullSid = SeNullSid;
  SeExports->SeWorldSid = SeWorldSid;
  SeExports->SeLocalSid = SeLocalSid;
  SeExports->SeCreatorOwnerSid = SeCreatorOwnerSid;
  SeExports->SeCreatorGroupSid = SeCreatorGroupSid;
  SeExports->SeNtAuthoritySid = SeNtAuthoritySid;
  SeExports->SeDialupSid = SeDialupSid;
  SeExports->SeNetworkSid = SeNetworkSid;
  SeExports->SeBatchSid = SeBatchSid;
  SeExports->SeInteractiveSid = SeInteractiveSid;
  SeExports->SeLocalSystemSid = SeLocalSystemSid;
  SeExports->SeAliasAdminsSid = SeAliasAdminsSid;
  SeExports->SeAliasUsersSid = SeAliasUsersSid;
  SeExports->SeAliasGuestsSid = SeAliasGuestsSid;
  SeExports->SeAliasPowerUsersSid = SeAliasPowerUsersSid;
  SeExports->SeAliasAccountOpsSid = SeAliasAccountOpsSid;
  SeExports->SeAliasSystemOpsSid = SeAliasSystemOpsSid;
  SeExports->SeAliasPrintOpsSid = SeAliasPrintOpsSid;
  SeExports->SeAliasBackupOpsSid = SeAliasBackupOpsSid;

  return TRUE;
}


VOID SepReferenceLogonSession(PLUID AuthenticationId)
{
   UNIMPLEMENTED;
}

VOID SepDeReferenceLogonSession(PLUID AuthenticationId)
{
   UNIMPLEMENTED;
}



/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtAllocateUuids(PULARGE_INTEGER Time,
		PULONG Range,
		PULONG Sequence)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	      IN HANDLE ClientToken,
	      IN ACCESS_MASK DesiredAccess,
	      IN PGENERIC_MAPPING GenericMapping,
	      OUT PPRIVILEGE_SET PrivilegeSet,
	      OUT PULONG ReturnLength,
	      OUT PULONG GrantedAccess,
	      OUT PBOOLEAN AccessStatus)
{
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @implemented
 */
VOID STDCALL
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  PEPROCESS Process;
  BOOLEAN CopyOnOpen;
  BOOLEAN EffectiveOnly;

  Process = PsGetCurrentThread ()->ThreadsProcess;

  SubjectContext->ProcessAuditId = Process;
  SubjectContext->ClientToken = 
    PsReferenceImpersonationToken (PsGetCurrentThread(),
				   &CopyOnOpen,
				   &EffectiveOnly,
				   &SubjectContext->ImpersonationLevel);
  SubjectContext->PrimaryToken = PsReferencePrimaryToken (Process);
}


/*
 * @unimplemented
 */
VOID STDCALL
SeLockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  ObDereferenceObject (SubjectContext->PrimaryToken);
  if (SubjectContext->ClientToken != NULL)
    {
      ObDereferenceObject (SubjectContext->ClientToken);
    }
}


/*
 * @unimplemented
 */
VOID STDCALL
SeUnlockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
SeDeassignSecurity(PSECURITY_DESCRIPTOR* SecurityDescriptor)
{
  if ((*SecurityDescriptor) != NULL)
    {
      ExFreePool(*SecurityDescriptor);
      (*SecurityDescriptor) = NULL;
    }
  return(STATUS_SUCCESS);
}


#if 0
VOID SepGetDefaultsSubjectContext(PSECURITY_SUBJECT_CONTEXT SubjectContext,
				  PSID* Owner,
				  PSID* PrimaryGroup,
				  PSID* ProcessOwner,
				  PSID* ProcessPrimaryGroup,
				  PACL* DefaultDacl)
{
   PACCESS_TOKEN Token;
   
   if (SubjectContext->ClientToken != NULL)
     {
	Token = SubjectContext->ClientToken;
     }
   else
     {
	Token = SubjectContext->PrimaryToken;
     }
   *Owner = Token->UserAndGroups[Token->DefaultOwnerIndex].Sid;
   *PrimaryGroup = Token->PrimaryGroup;
   *DefaultDacl = Token->DefaultDacl;
   *ProcessOwner = SubjectContext->PrimaryToken->
     UserAndGroups[Token->DefaultOwnerIndex].Sid;
   *ProcessPrimaryGroup = SubjectContext->PrimaryToken->PrimaryGroup;
}

NTSTATUS SepInheritAcl(PACL Acl,
		       BOOLEAN IsDirectoryObject,
		       PSID Owner,
		       PSID PrimaryGroup,
		       PACL DefaultAcl,
		       PSID ProcessOwner,
		       PSID ProcessGroup,
		       PGENERIC_MAPPING GenericMapping)
{
   if (Acl == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   if (Acl->AclRevision != 2 &&
       Acl->AclRevision != 3 )
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
}
#endif

/*
 * @unimplemented
 */
NTSTATUS STDCALL
SeAssignSecurity(PSECURITY_DESCRIPTOR ParentDescriptor,
		 PSECURITY_DESCRIPTOR ExplicitDescriptor,
		 PSECURITY_DESCRIPTOR* NewDescriptor,
		 BOOLEAN IsDirectoryObject,
		 PSECURITY_SUBJECT_CONTEXT SubjectContext,
		 PGENERIC_MAPPING GenericMapping,
		 POOL_TYPE PoolType)
{
#if 0
   PSECURITY_DESCRIPTOR Descriptor;
   PSID Owner;
   PSID PrimaryGroup;
   PACL DefaultDacl;
   PSID ProcessOwner;
   PSID ProcessPrimaryGroup;
   PACL Sacl;
   
   if (ExplicitDescriptor == NULL)
     {
	RtlCreateSecurityDescriptor(&Descriptor, 1);
     }
   else
     {
	Descriptor = ExplicitDescriptor;
     }
   SeLockSubjectContext(SubjectContext);
   SepGetDefaultsSubjectContext(SubjectContext,
				&Owner,
				&PrimaryGroup,
				&DefaultDacl,
				&ProcessOwner,
				&ProcessPrimaryGroup);
   if (Descriptor->Control & SE_SACL_PRESENT ||
       Descriptor->Control & SE_SACL_DEFAULTED)
     {
	if (ParentDescriptor == NULL)
	  {
	  }
	if (Descriptor->Control & SE_SACL_PRESENT ||
	    Descriptor->Sacl == NULL ||)
	  {
	     Sacl = NULL;
	  }
	else
	  {
	     Sacl = Descriptor->Sacl;
	     if (Descriptor->Control & SE_SELF_RELATIVE)
	       {
		  Sacl = (PACL)(((PVOID)Sacl) + (PVOID)Descriptor);
	       }
	  }
	SepInheritAcl(Sacl,
		      IsDirectoryObject,
		      Owner,
		      PrimaryGroup,
		      DefaultDacl,
		      ProcessOwner,
		      GenericMapping);
     }
#else
  UNIMPLEMENTED;
  return(STATUS_NOT_IMPLEMENTED);
#endif
}

BOOLEAN SepSidInToken(PACCESS_TOKEN Token,
		      PSID Sid)
{
   ULONG i;
   
   if (Token->UserAndGroupCount == 0)
     {
	return(FALSE);
     }
   
   for (i=0; i<Token->UserAndGroupCount; i++)
     {
	if (RtlEqualSid(Sid, Token->UserAndGroups[i].Sid))
	  {
	     if (i == 0 ||
		 (!(Token->UserAndGroups[i].Attributes & SE_GROUP_ENABLED)))
	       {
		  return(TRUE);
	       }
	     return(FALSE);
	  }
     }
   return(FALSE);
}


/*
 * @implemented
 */
BOOLEAN STDCALL
SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
	      IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
	      IN BOOLEAN SubjectContextLocked,
	      IN ACCESS_MASK DesiredAccess,
	      IN ACCESS_MASK PreviouslyGrantedAccess,
	      OUT PPRIVILEGE_SET* Privileges,
	      IN PGENERIC_MAPPING GenericMapping,
	      IN KPROCESSOR_MODE AccessMode,
	      OUT PACCESS_MODE GrantedAccess,
	      OUT PNTSTATUS AccessStatus)
/*
 * FUNCTION: Determines whether the requested access rights can be granted
 * to an object protected by a security descriptor and an object owner
 * ARGUMENTS:
 *      SecurityDescriptor = Security descriptor protecting the object
 *      SubjectSecurityContext = Subject's captured security context
 *      SubjectContextLocked = Indicates the user's subject context is locked
 *      DesiredAccess = Access rights the caller is trying to acquire
 *      PreviouslyGrantedAccess = Specified the access rights already granted
 *      Privileges = ?
 *      GenericMapping = Generic mapping associated with the object
 *      AccessMode = Access mode used for the check
 *      GrantedAccess (OUT) = On return specifies the access granted
 *      AccessStatus (OUT) = Status indicating why access was denied
 * RETURNS: If access was granted, returns TRUE
 */
{
   ULONG i;
   PACL Dacl;
   BOOLEAN Present;
   BOOLEAN Defaulted;
   NTSTATUS Status;
   PACE CurrentAce;
   PSID Sid;
   ACCESS_MASK CurrentAccess;

   CurrentAccess = PreviouslyGrantedAccess;

   /*
    * Ignore the SACL for now
    */

   /*
    * Check the DACL
    */
   Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
					 &Present,
					 &Dacl,
					 &Defaulted);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   CurrentAce = (PACE)(Dacl + 1);
   for (i = 0; i < Dacl->AceCount; i++)
     {
	Sid = (PSID)(CurrentAce + 1);
	if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
	  {
	     if (SepSidInToken(SubjectSecurityContext->ClientToken, Sid))
	       {
		  *AccessStatus = STATUS_ACCESS_DENIED;
		  *GrantedAccess = 0;
		  return(STATUS_SUCCESS);
	       }
	  }
	if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
	  {
	     if (SepSidInToken(SubjectSecurityContext->ClientToken, Sid))
	       {
		  CurrentAccess = CurrentAccess | 
		    CurrentAce->AccessMask;
	       }
	  }
     }
   if (!(CurrentAccess & DesiredAccess) &&
       !((~CurrentAccess) & DesiredAccess))
     {
	*AccessStatus = STATUS_ACCESS_DENIED;
     }
   else
     {
	*AccessStatus = STATUS_SUCCESS;
     }
   *GrantedAccess = CurrentAccess;

   return(STATUS_SUCCESS);
}

/* EOF */
