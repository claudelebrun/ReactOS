/* $Id: sd.c,v 1.14 2004/05/20 12:42:11 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Security manager
 * FILE:              kernel/se/sd.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 * REVISION HISTORY:
 *                 26/07/98: Added stubs for security functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/se.h>

#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PSECURITY_DESCRIPTOR SePublicDefaultSd = NULL;
PSECURITY_DESCRIPTOR SePublicDefaultUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenSd = NULL;
PSECURITY_DESCRIPTOR SePublicOpenUnrestrictedSd = NULL;
PSECURITY_DESCRIPTOR SeSystemDefaultSd = NULL;
PSECURITY_DESCRIPTOR SeUnrestrictedSd = NULL;

/* FUNCTIONS ***************************************************************/

BOOLEAN INIT_FUNCTION
SepInitSDs(VOID)
{
  /* Create PublicDefaultSd */
  SePublicDefaultSd = ExAllocatePool(NonPagedPool,
				     sizeof(SECURITY_DESCRIPTOR));
  if (SePublicDefaultSd == NULL)
    return(FALSE);

  RtlCreateSecurityDescriptor(SePublicDefaultSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicDefaultSd,
			       TRUE,
			       SePublicDefaultDacl,
			       FALSE);

  /* Create PublicDefaultUnrestrictedSd */
  SePublicDefaultUnrestrictedSd = ExAllocatePool(NonPagedPool,
						 sizeof(SECURITY_DESCRIPTOR));
  if (SePublicDefaultUnrestrictedSd == NULL)
    return(FALSE);

  RtlCreateSecurityDescriptor(SePublicDefaultUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicDefaultUnrestrictedSd,
			       TRUE,
			       SePublicDefaultUnrestrictedDacl,
			       FALSE);

  /* Create PublicOpenSd */
  SePublicOpenSd = ExAllocatePool(NonPagedPool,
				  sizeof(SECURITY_DESCRIPTOR));
  if (SePublicOpenSd == NULL)
    return(FALSE);

  RtlCreateSecurityDescriptor(SePublicOpenSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicOpenSd,
			       TRUE,
			       SePublicOpenDacl,
			       FALSE);

  /* Create PublicOpenUnrestrictedSd */
  SePublicOpenUnrestrictedSd = ExAllocatePool(NonPagedPool,
					      sizeof(SECURITY_DESCRIPTOR));
  if (SePublicOpenUnrestrictedSd == NULL)
    return(FALSE);

  RtlCreateSecurityDescriptor(SePublicOpenUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SePublicOpenUnrestrictedSd,
			       TRUE,
			       SePublicOpenUnrestrictedDacl,
			       FALSE);

  /* Create SystemDefaultSd */
  SeSystemDefaultSd = ExAllocatePool(NonPagedPool,
				     sizeof(SECURITY_DESCRIPTOR));
  if (SeSystemDefaultSd == NULL)
    return(FALSE);

  RtlCreateSecurityDescriptor(SeSystemDefaultSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SeSystemDefaultSd,
			       TRUE,
			       SeSystemDefaultDacl,
			       FALSE);

  /* Create UnrestrictedSd */
  SeUnrestrictedSd = ExAllocatePool(NonPagedPool,
				    sizeof(SECURITY_DESCRIPTOR));
  if (SeUnrestrictedSd == NULL)
    return(FALSE);

  RtlCreateSecurityDescriptor(SeUnrestrictedSd,
			      SECURITY_DESCRIPTOR_REVISION);
  RtlSetDaclSecurityDescriptor(SeUnrestrictedSd,
			       TRUE,
			       SeUnrestrictedDacl,
			       FALSE);

  return(TRUE);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlCreateSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			    ULONG Revision)
{
  if (Revision != SECURITY_DESCRIPTOR_REVISION)
    return(STATUS_UNSUCCESSFUL);

  SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;
  SecurityDescriptor->Sbz1 = 0;
  SecurityDescriptor->Control = 0;
  SecurityDescriptor->Owner = NULL;
  SecurityDescriptor->Group = NULL;
  SecurityDescriptor->Sacl = NULL;
  SecurityDescriptor->Dacl = NULL;

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
ULONG STDCALL
RtlLengthSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  PSID Owner;
  PSID Group;
  ULONG Length;
  PACL Dacl;
  PACL Sacl;

  Length = sizeof(SECURITY_DESCRIPTOR);

  if (SecurityDescriptor->Owner != NULL)
    {
      Owner = SecurityDescriptor->Owner;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Owner = (PSID)((ULONG)Owner +
			 (ULONG)SecurityDescriptor);
	}
      Length = Length + ((sizeof(SID) + (Owner->SubAuthorityCount - 1) *
			 sizeof(ULONG) + 3) & 0xfc);
    }

  if (SecurityDescriptor->Group != NULL)
    {
      Group = SecurityDescriptor->Group;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Group = (PSID)((ULONG)Group + (ULONG)SecurityDescriptor);
	}
      Length = Length + ((sizeof(SID) + (Group->SubAuthorityCount - 1) *
			 sizeof(ULONG) + 3) & 0xfc);
    }

  if (SecurityDescriptor->Control & SE_DACL_PRESENT &&
      SecurityDescriptor->Dacl != NULL)
    {
      Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Dacl = (PACL)((ULONG)Dacl + (char*)SecurityDescriptor);
	}
      Length = Length + ((Dacl->AclSize + 3) & 0xfc);
    }

  if (SecurityDescriptor->Control & SE_SACL_PRESENT &&
      SecurityDescriptor->Sacl != NULL)
    {
      Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Sacl = (PACL)((ULONG)Sacl + (char*)SecurityDescriptor);
	}
      Length = Length + ((Sacl->AclSize + 3) & 0xfc);
    }

  return(Length);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     PBOOLEAN DaclPresent,
			     PACL* Dacl,
			     PBOOLEAN DaclDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (!(SecurityDescriptor->Control & SE_DACL_PRESENT))
    {
      *DaclPresent = FALSE;
      return(STATUS_SUCCESS);
    }
  *DaclPresent = TRUE;

  if (SecurityDescriptor->Dacl == NULL)
    {
      *Dacl = NULL;
    }
  else
    {
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Dacl = (PACL)((ULONG)SecurityDescriptor->Dacl +
			 (char*)SecurityDescriptor);
	}
      else
	{
	  *Dacl = SecurityDescriptor->Dacl;
	}
    }

  if (SecurityDescriptor->Control & SE_DACL_DEFAULTED)
    {
      *DaclDefaulted = TRUE;
    }
  else
    {
      *DaclDefaulted = FALSE;
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetDaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     BOOLEAN DaclPresent,
			     PACL Dacl,
			     BOOLEAN DaclDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (!DaclPresent)
    {
      SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_PRESENT);
      return(STATUS_SUCCESS);
    }

  SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_PRESENT;
  SecurityDescriptor->Dacl = Dacl;
  SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_DACL_DEFAULTED);

  if (DaclDefaulted)
    {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_DACL_DEFAULTED;
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
BOOLEAN STDCALL
RtlValidSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;

  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(FALSE);
    }

  Owner = SecurityDescriptor->Owner;
  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      Owner = (PSID)((ULONG)Owner + (ULONG)SecurityDescriptor);
    }

  if (!RtlValidSid(Owner))
    {
      return(FALSE);
    }

  Group = SecurityDescriptor->Group;
  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      Group = (PSID)((ULONG)Group + (ULONG)SecurityDescriptor);
    }

  if (!RtlValidSid(Group))
    {
      return(FALSE);
    }

  if (SecurityDescriptor->Control & SE_DACL_PRESENT &&
      SecurityDescriptor->Dacl != NULL)
    {
      Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Dacl = (PACL)((ULONG)Dacl + (ULONG)SecurityDescriptor);
	}

      if (!RtlValidAcl(Dacl))
	{
	  return(FALSE);
	}
    }

  if (SecurityDescriptor->Control & SE_SACL_PRESENT &&
      SecurityDescriptor->Sacl != NULL)
    {
      Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  Sacl = (PACL)((ULONG)Sacl + (ULONG)SecurityDescriptor);
	}

      if (!RtlValidAcl(Sacl))
	{
	  return(FALSE);
	}
    }

  return(TRUE);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID Owner,
			      BOOLEAN OwnerDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  SecurityDescriptor->Owner = Owner;
  SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_OWNER_DEFAULTED);

  if (OwnerDefaulted)
    {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_OWNER_DEFAULTED;
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetOwnerSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID* Owner,
			      PBOOLEAN OwnerDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (SecurityDescriptor->Owner != NULL)
    {
	if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	  {
	     *Owner = (PSID)((ULONG)SecurityDescriptor->Owner +
			     (char*)SecurityDescriptor);
	  }
	else
	  {
	     *Owner = SecurityDescriptor->Owner;
	  }
    }
  else
    {
	*Owner = NULL;
    }
   if (SecurityDescriptor->Control & SE_OWNER_DEFAULTED)
     {
	*OwnerDefaulted = 1;
     }
   else
     {
	*OwnerDefaulted = 0;
     }
   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID Group,
			      BOOLEAN GroupDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  SecurityDescriptor->Group = Group;
  SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_GROUP_DEFAULTED);

  if (GroupDefaulted)
    {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_GROUP_DEFAULTED;
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetGroupSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			      PSID* Group,
			      PBOOLEAN GroupDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (SecurityDescriptor->Group != NULL)
    {
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Group = (PSID)((ULONG)SecurityDescriptor->Group +
			  (char*)SecurityDescriptor);
	}
      else
	{
	  *Group = SecurityDescriptor->Group;
	}
    }
  else
    {
      *Group = NULL;
    }

  if (SecurityDescriptor->Control & SE_GROUP_DEFAULTED)
    {
      *GroupDefaulted = TRUE;
    }
  else
    {
      *GroupDefaulted = FALSE;
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlGetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     PBOOLEAN SaclPresent,
			     PACL *Sacl,
			     PBOOLEAN SaclDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (!(SecurityDescriptor->Control & SE_SACL_PRESENT))
    {
      *SaclPresent = FALSE;
      return(STATUS_SUCCESS);
    }
  *SaclPresent = TRUE;

  if (SecurityDescriptor->Sacl == NULL)
    {
      *Sacl = NULL;
    }
  else
    {
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Sacl = (PACL)((ULONG)SecurityDescriptor->Sacl +
			 (char*)SecurityDescriptor);
	}
      else
	{
	  *Sacl = SecurityDescriptor->Sacl;
	}
    }

  if (SecurityDescriptor->Control & SE_SACL_DEFAULTED)
    {
      *SaclDefaulted = TRUE;
    }
  else
    {
      *SaclDefaulted = FALSE;
    }

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetSaclSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			     BOOLEAN SaclPresent,
			     PACL Sacl,
			     BOOLEAN SaclDefaulted)
{
  if (SecurityDescriptor->Revision != SECURITY_DESCRIPTOR_REVISION)
    {
      return(STATUS_UNSUCCESSFUL);
    }
  if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  if (!SaclPresent)
    {
      SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_PRESENT);
      return(STATUS_SUCCESS);
    }

  SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_PRESENT;
  SecurityDescriptor->Sacl = Sacl;
  SecurityDescriptor->Control = SecurityDescriptor->Control & ~(SE_SACL_DEFAULTED);

  if (SaclDefaulted)
    {
      SecurityDescriptor->Control = SecurityDescriptor->Control | SE_SACL_DEFAULTED;
    }

  return(STATUS_SUCCESS);
}


static VOID
RtlpQuerySecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,
			    PSID* Owner,
			    PULONG OwnerLength,
			    PSID* Group,
			    PULONG GroupLength,
			    PACL* Dacl,
			    PULONG DaclLength,
			    PACL* Sacl,
			    PULONG SaclLength)
{
  if (SecurityDescriptor->Owner != NULL)
    {
      *Owner = SecurityDescriptor->Owner;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Owner = (PSID)((ULONG)*Owner + (ULONG)SecurityDescriptor);
	}
    }
  else
    {
      *Owner = NULL;
    }

  if (*Owner != NULL)
    {
      *OwnerLength = (RtlLengthSid(*Owner) + 3) & ~3;
    }
  else
    {
      *OwnerLength = 0;
    }

  if ((SecurityDescriptor->Control & SE_DACL_PRESENT) &&
      SecurityDescriptor->Dacl != NULL)
    {
      *Dacl = SecurityDescriptor->Dacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Dacl = (PACL)((ULONG)*Dacl + (ULONG)SecurityDescriptor);
	}
    }
  else
    {
      *Dacl = NULL;
    }

  if (*Dacl != NULL)
    {
      *DaclLength = ((*Dacl)->AclSize + 3) & ~3;
    }
  else
    {
      *DaclLength = 0;
    }

  if (SecurityDescriptor->Group != NULL)
    {
      *Group = SecurityDescriptor->Group;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Group = (PSID)((ULONG)*Group + (ULONG)SecurityDescriptor);
	}
    }
  else
    {
      *Group = NULL;
    }

  if (*Group != NULL)
    {
      *GroupLength = (RtlLengthSid(*Group) + 3) & ~3;
    }
  else
    {
      *GroupLength = 0;
    }

  if ((SecurityDescriptor->Control & SE_SACL_PRESENT) &&
      SecurityDescriptor->Sacl != NULL)
    {
      *Sacl = SecurityDescriptor->Sacl;
      if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
	{
	  *Sacl = (PACL)((ULONG)*Sacl + (ULONG)SecurityDescriptor);
	}
    }
  else
    {
      *Sacl = NULL;
    }

  if (*Sacl != NULL)
    {
      *SaclLength = ((*Sacl)->AclSize + 3) & ~3;
    }
  else
    {
      *SaclLength = 0;
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlAbsoluteToSelfRelativeSD(PSECURITY_DESCRIPTOR AbsSD,
			    PSECURITY_DESCRIPTOR RelSD,
			    PULONG BufferLength)
{
  PSID Owner;
  PSID Group;
  PACL Sacl;
  PACL Dacl;
  ULONG OwnerLength;
  ULONG GroupLength;
  ULONG SaclLength;
  ULONG DaclLength;
  ULONG TotalLength;
  ULONG Current;

  if (AbsSD->Control & SE_SELF_RELATIVE)
    {
      return(STATUS_BAD_DESCRIPTOR_FORMAT);
    }

  RtlpQuerySecurityDescriptor(AbsSD,
			      &Owner,
			      &OwnerLength,
			      &Group,
			      &GroupLength,
			      &Dacl,
			      &DaclLength,
			      &Sacl,
			      &SaclLength);

  TotalLength = OwnerLength + GroupLength + SaclLength +
		DaclLength + sizeof(SECURITY_DESCRIPTOR);
  if (*BufferLength < TotalLength)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }

  RtlZeroMemory(RelSD,
		TotalLength);
  memmove(RelSD,
	  AbsSD,
	  sizeof(SECURITY_DESCRIPTOR));
  Current = (ULONG)RelSD + sizeof(SECURITY_DESCRIPTOR);

  if (SaclLength != 0)
    {
      memmove((PVOID)Current,
	      Sacl,
	      SaclLength);
      RelSD->Sacl = (PACL)((ULONG)Current - (ULONG)RelSD);
      Current += SaclLength;
    }

  if (DaclLength != 0)
    {
      memmove((PVOID)Current,
	      Dacl,
	      DaclLength);
      RelSD->Dacl = (PACL)((ULONG)Current - (ULONG)RelSD);
      Current += DaclLength;
    }

  if (OwnerLength != 0)
    {
      memmove((PVOID)Current,
	      Owner,
	      OwnerLength);
      RelSD->Owner = (PSID)((ULONG)Current - (ULONG)RelSD);
      Current += OwnerLength;
    }

  if (GroupLength != 0)
    {
      memmove((PVOID)Current,
	      Group,
	      GroupLength);
      RelSD->Group = (PSID)((ULONG)Current - (ULONG)RelSD);
    }

  RelSD->Control |= SE_SELF_RELATIVE;

  return(STATUS_SUCCESS);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
SeQuerySecurityDescriptorInfo(IN PSECURITY_INFORMATION SecurityInformation,
			      OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
			      IN OUT PULONG Length,
			      IN PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
SeSetSecurityDescriptorInfo(IN PVOID Object OPTIONAL,
			    IN PSECURITY_INFORMATION SecurityInformation,
			    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
			    IN OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
			    IN POOL_TYPE PoolType,
			    IN PGENERIC_MAPPING GenericMapping)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
BOOLEAN STDCALL
SeValidSecurityDescriptor(IN ULONG Length,
			  IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
