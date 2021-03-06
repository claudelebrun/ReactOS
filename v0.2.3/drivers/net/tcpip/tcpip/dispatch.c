/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/dispatch.h
 * PURPOSE:     TDI dispatch routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 * TODO:        Validate device object in all dispatch routines
 */
#include <roscfg.h>
#include <tcpip.h>
#include <dispatch.h>
#include <routines.h>
#include <datagram.h>
#include <info.h>


NTSTATUS DispPrepareIrpForCancel(
    PTRANSPORT_CONTEXT Context,
    PIRP Irp,
    PDRIVER_CANCEL CancelRoutine)
/*
 * FUNCTION: Prepare an IRP for cancellation
 * ARGUMENTS:
 *     Context       = Pointer to context information
 *     Irp           = Pointer to an I/O request packet
 *     CancelRoutine = Routine to be called when I/O request is cancelled
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IoAcquireCancelSpinLock(&OldIrql);

    if (!Irp->Cancel) {
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, CancelRoutine);
        Context->RefCount++;
        IoReleaseCancelSpinLock(OldIrql);

        TI_DbgPrint(DEBUG_IRP, ("Leaving (IRP at 0x%X can now be cancelled).\n", Irp));

        return STATUS_SUCCESS;
    }

    /* IRP has already been cancelled */
 
    IoReleaseCancelSpinLock(OldIrql);

    Irp->IoStatus.Status      = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    TI_DbgPrint(DEBUG_IRP, ("Leaving (IRP was already cancelled).\n"));

    return IRPFinish(Irp, STATUS_CANCELLED);
}


VOID DispCancelComplete(
    PVOID Context)
/*
 * FUNCTION: Completes a cancel request
 * ARGUMENTS:
 *     Context = Pointer to context information (FILE_OBJECT)
 */
{
    KIRQL OldIrql;
    PFILE_OBJECT FileObject;
    PTRANSPORT_CONTEXT TranContext;
    
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    FileObject  = (PFILE_OBJECT)Context;
    TranContext = (PTRANSPORT_CONTEXT)FileObject->FsContext;
    
    IoAcquireCancelSpinLock(&OldIrql);

    /* Remove the reference taken by the cancel routine */
    TranContext->RefCount--;

    if (TranContext->RefCount == 0) {
        TI_DbgPrint(DEBUG_IRP, ("Setting TranContext->CleanupEvent to signaled.\n"));
        /* Set the cleanup event */
        KeSetEvent(&TranContext->CleanupEvent, 0, FALSE);
    }

    TI_DbgPrint(DEBUG_REFCOUNT, ("TranContext->RefCount (%d).\n", TranContext->RefCount));

    IoReleaseCancelSpinLock(OldIrql);

    TI_DbgPrint(DEBUG_IRP, ("Leaving.\n"));
}


VOID DispCancelRequest(
    PDEVICE_OBJECT Device,
    PIRP Irp)
/*
 * FUNCTION: Cancels an IRP
 * ARGUMENTS:
 *     Device = Pointer to device object
 *     Irp    = Pointer to an I/O request packet
 */
{
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    PFILE_OBJECT FileObject;
    UCHAR MinorFunction;
    NTSTATUS Status = STATUS_SUCCESS;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp         = IoGetCurrentIrpStackLocation(Irp);
    FileObject    = IrpSp->FileObject;
    TranContext   = (PTRANSPORT_CONTEXT)FileObject->FsContext;
    MinorFunction = IrpSp->MinorFunction;

    TI_DbgPrint(DEBUG_IRP, ("IRP at (0x%X)  MinorFunction (0x%X)  IrpSp (0x%X).\n", Irp, MinorFunction, IrpSp));

#ifdef DBG
    if (!Irp->Cancel)
        TI_DbgPrint(MIN_TRACE, ("Irp->Cancel is FALSE, should be TRUE.\n"));
#endif

    /* Increase reference count to prevent accidential closure
       of the object while inside the cancel routine */
    TranContext->RefCount++;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    /* Try canceling the request */
    switch(MinorFunction) {
    case TDI_SEND:

    case TDI_RECEIVE:
        /* FIXME: Close connection */
        break;

    case TDI_SEND_DATAGRAM:
        if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            TI_DbgPrint(MIN_TRACE, ("TDI_SEND_DATAGRAM, but no address file.\n"));
            break;
        }

        DGCancelSendRequest(TranContext->Handle.AddressHandle, Irp);
        break;

    case TDI_RECEIVE_DATAGRAM:
        if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
            TI_DbgPrint(MIN_TRACE, ("TDI_RECEIVE_DATAGRAM, but no address file.\n"));
            break;
        }

        DGCancelReceiveRequest(TranContext->Handle.AddressHandle, Irp);
        break;

    default:
        TI_DbgPrint(MIN_TRACE, ("Unknown IRP. MinorFunction (0x%X).\n", MinorFunction));
        break;
    }

    if (Status != STATUS_PENDING)
        DispCancelComplete(FileObject);

    TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID DispDataRequestComplete(
    PVOID Context,
    NTSTATUS Status,
    ULONG Count)
/*
 * FUNCTION: Completes a send/receive IRP
 * ARGUMENTS:
 *     Context = Pointer to context information (IRP)
 *     Status  = Status of the request
 *     Count   = Number of bytes sent or received
 */
{
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    PTRANSPORT_CONTEXT TranContext;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    Irp         = (PIRP)Context;
    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

    IoAcquireCancelSpinLock(&OldIrql);

    IoSetCancelRoutine(Irp, NULL);
    TranContext->RefCount--;
    TI_DbgPrint(DEBUG_REFCOUNT, ("TranContext->RefCount (%d).\n", TranContext->RefCount));
    if (TranContext->RefCount == 0) {
        TI_DbgPrint(DEBUG_IRP, ("Setting TranContext->CleanupEvent to signaled.\n"));

        KeSetEvent(&TranContext->CleanupEvent, 0, FALSE);
    }

    if (Irp->Cancel || TranContext->CancelIrps) {
        /* The IRP has been cancelled */

        TI_DbgPrint(DEBUG_IRP, ("IRP is cancelled.\n"));

        Status = STATUS_CANCELLED;
        Count  = 0;
    }

    IoReleaseCancelSpinLock(OldIrql);

    Irp->IoStatus.Status      = Status;
    Irp->IoStatus.Information = Count;

    TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

    IRPFinish(Irp, STATUS_SUCCESS);
}


NTSTATUS DispTdiAccept(
  PIRP Irp)
/*
 * FUNCTION: TDI_ACCEPT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiAssociateAddress(
    PIRP Irp)
/*
 * FUNCTION: TDI_ASSOCIATE_ADDRESS handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_REQUEST_KERNEL_ASSOCIATE Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  PCONNECTION_ENDPOINT Connection;
  PFILE_OBJECT FileObject;
  PADDRESS_FILE AddrFile;
  NTSTATUS Status;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (!Connection) {
    TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  if (Connection->AddressFile) {
    TI_DbgPrint(MID_TRACE, ("An address file is already asscociated.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Parameters = (PTDI_REQUEST_KERNEL_ASSOCIATE)&IrpSp->Parameters;

  Status = ObReferenceObjectByHandle(
    Parameters->AddressHandle,
    0,
    IoFileObjectType,
    KernelMode,
    (PVOID*)&FileObject,
    NULL);
  if (!NT_SUCCESS(Status)) {
    TI_DbgPrint(MID_TRACE, ("Bad address file object handle (0x%X).\n",
      Parameters->AddressHandle));
    return STATUS_INVALID_PARAMETER;
  }

  if (FileObject->FsContext2 != (PVOID)TDI_TRANSPORT_ADDRESS_FILE) {
    ObDereferenceObject(FileObject);
    TI_DbgPrint(MID_TRACE, ("Bad address file object. Magic (0x%X).\n",
      FileObject->FsContext2));
    return STATUS_INVALID_PARAMETER;
  }

  /* Get associated address file object. Quit if none exists */

  TranContext = FileObject->FsContext;
  if (!TranContext) {
    ObDereferenceObject(FileObject);
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
  if (!AddrFile) {
    ObDereferenceObject(FileObject);
    TI_DbgPrint(MID_TRACE, ("No address file object.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  /* The connection endpoint references the address file object */
  ReferenceObject(AddrFile);
  Connection->AddressFile = AddrFile;

  /* Add connection endpoint to the address file */
  AddrFile->Connection = Connection;

  /* FIXME: Maybe do this in DispTdiDisassociateAddress() instead? */
  ObDereferenceObject(FileObject);

  return STATUS_SUCCESS;
}


NTSTATUS DispTdiConnect(
  PIRP Irp)
/*
 * FUNCTION: TDI_CONNECT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PCONNECTION_ENDPOINT Connection;
  PTDI_REQUEST_KERNEL Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  TDI_REQUEST Request;
  NTSTATUS Status;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (!Connection) {
    TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  Parameters = (PTDI_REQUEST_KERNEL)&IrpSp->Parameters;

  /* Initialize a connect request */
  Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
  Request.RequestNotifyObject      = DispDataRequestComplete;
  Request.RequestContext           = Irp;

  /* XXX Handle connected UDP, etc... */
  Status = TCPConnect(
    &Request,
    Parameters->RequestConnectionInformation,
    Parameters->ReturnConnectionInformation);

  TI_DbgPrint(MAX_TRACE, ("TCP Connect returned %08x\n", Status));

  return Status;
}


NTSTATUS DispTdiDisassociateAddress(
  PIRP Irp)
/*
 * FUNCTION: TDI_DISASSOCIATE_ADDRESS handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PCONNECTION_ENDPOINT Connection;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (!Connection) {
    TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  if (!Connection->AddressFile) {
    TI_DbgPrint(MID_TRACE, ("No address file is asscociated.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  /* Remove the reference put on the address file object */
  KeAcquireSpinLock(&Connection->Lock, &OldIrql);
  DereferenceObject(Connection->AddressFile);
  KeReleaseSpinLock(&Connection->Lock, OldIrql);

  return STATUS_SUCCESS;
}


NTSTATUS DispTdiDisconnect(
  PIRP Irp)
/*
 * FUNCTION: TDI_DISCONNECT handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_REQUEST_KERNEL_QUERY_INFORMATION Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  Parameters = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&IrpSp->Parameters;

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  switch (Parameters->QueryType)
  {
    case TDI_QUERY_ADDRESS_INFO:
      {
        PTDI_ADDRESS_INFO AddressInfo;
        PADDRESS_FILE AddrFile;
        PTA_IP_ADDRESS Address;

        AddressInfo = (PTDI_ADDRESS_INFO)MmGetSystemAddressForMdl(Irp->MdlAddress);

        switch ((ULONG)IrpSp->FileObject->FsContext2) {
          case TDI_TRANSPORT_ADDRESS_FILE:
            AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
            break;

          case TDI_CONNECTION_FILE:
            AddrFile = ((PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext)->AddressFile;
            break;

          default:
            TI_DbgPrint(MIN_TRACE, ("Invalid transport context\n"));
            return STATUS_INVALID_PARAMETER;
        }

        if (!AddrFile) {
          TI_DbgPrint(MID_TRACE, ("No address file object.\n"));
          return STATUS_INVALID_PARAMETER;
        }

        if (MmGetMdlByteCount(Irp->MdlAddress) <
            (sizeof(TDI_ADDRESS_INFO) + sizeof(TDI_ADDRESS_IP))) {
          TI_DbgPrint(MID_TRACE, ("MDL buffer too small.\n"));
          return STATUS_BUFFER_OVERFLOW;
        }

        /* FIXME: Is this count really the one we should return? */
        AddressInfo->ActivityCount = AddrFile->RefCount;

        Address = (PTA_IP_ADDRESS)&AddressInfo->Address;
        Address->TAAddressCount = 1;
        Address->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        Address->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        Address->Address[0].Address[0].sin_port = AddrFile->Port;
        Address->Address[0].Address[0].in_addr = AddrFile->ADE->Address->Address.IPv4Address;        
        RtlZeroMemory(
          &Address->Address[0].Address[0].sin_zero,
          sizeof(Address->Address[0].Address[0].sin_zero));

        return STATUS_SUCCESS;
      }
  }

  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiListen(
  PIRP Irp)
/*
 * FUNCTION: TDI_LISTEN handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PCONNECTION_ENDPOINT Connection;
  PTDI_REQUEST_KERNEL Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  PTDI_REQUEST Request;
  NTSTATUS Status;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated connection endpoint file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (TranContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  Connection = (PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext;
  if (Connection == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  Parameters = (PTDI_REQUEST_KERNEL)&IrpSp->Parameters;

  Status = TCPListen( Request, 1024 /* BACKLOG */ );

  return Status;
}


NTSTATUS DispTdiQueryInformation(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: TDI_QUERY_INFORMATION handler
 * ARGUMENTS:
 *     DeviceObject = Pointer to device object structure
 *     Irp          = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_REQUEST_KERNEL_QUERY_INFORMATION Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  Parameters = (PTDI_REQUEST_KERNEL_QUERY_INFORMATION)&IrpSp->Parameters;

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_CONNECTION;
  }

  switch (Parameters->QueryType)
  {
    case TDI_QUERY_ADDRESS_INFO:
      {
        PTDI_ADDRESS_INFO AddressInfo;
        PADDRESS_FILE AddrFile;
        PTA_IP_ADDRESS Address;

        AddressInfo = (PTDI_ADDRESS_INFO)MmGetSystemAddressForMdl(Irp->MdlAddress);

        switch ((ULONG)IrpSp->FileObject->FsContext2) {
          case TDI_TRANSPORT_ADDRESS_FILE:
            AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
            break;

          case TDI_CONNECTION_FILE:
            AddrFile = ((PCONNECTION_ENDPOINT)TranContext->Handle.ConnectionContext)->AddressFile;
            break;

          default:
            TI_DbgPrint(MIN_TRACE, ("Invalid transport context\n"));
            return STATUS_INVALID_PARAMETER;
        }

        if (!AddrFile) {
          TI_DbgPrint(MID_TRACE, ("No address file object.\n"));
          return STATUS_INVALID_PARAMETER;
        }

        if (MmGetMdlByteCount(Irp->MdlAddress) <
            (sizeof(TDI_ADDRESS_INFO) + sizeof(TDI_ADDRESS_IP))) {
          TI_DbgPrint(MID_TRACE, ("MDL buffer too small.\n"));
          return STATUS_BUFFER_OVERFLOW;
        }

        /* FIXME: Is this count really the one we should return? */
        AddressInfo->ActivityCount = AddrFile->RefCount;

        Address = (PTA_IP_ADDRESS)&AddressInfo->Address;
        Address->TAAddressCount = 1;
        Address->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
        Address->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
        Address->Address[0].Address[0].sin_port = AddrFile->Port;
        Address->Address[0].Address[0].in_addr = AddrFile->ADE->Address->Address.IPv4Address;        
        RtlZeroMemory(
          &Address->Address[0].Address[0].sin_zero,
          sizeof(Address->Address[0].Address[0].sin_zero));

        return STATUS_SUCCESS;
      }
  }

  return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS DispTdiReceive(
  PIRP Irp)
/*
 * FUNCTION: TDI_RECEIVE handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PIO_STACK_LOCATION IrpSp;
  PTDI_REQUEST_KERNEL_RECEIVE ReceiveInfo;
  PTRANSPORT_CONTEXT TranContext;
  TDI_REQUEST Request;
  NTSTATUS Status;
  ULONG BytesReceived;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);
  ReceiveInfo = (PTDI_REQUEST_KERNEL_RECEIVE)&(IrpSp->Parameters);

  TranContext = IrpSp->FileObject->FsContext;
  if (TranContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  if (TranContext->Handle.ConnectionContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("No connection endpoint file object.\n"));
      return STATUS_INVALID_CONNECTION;
    }

  /* Initialize a receive request */
  Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
  Request.RequestNotifyObject = DispDataRequestComplete;
  Request.RequestContext = Irp;
  Status = DispPrepareIrpForCancel(
    IrpSp->FileObject->FsContext,
    Irp,
    (PDRIVER_CANCEL)DispCancelRequest);
  if (NT_SUCCESS(Status))
    {
      Status = TCPReceiveData(
        &Request,
        (PNDIS_BUFFER)Irp->MdlAddress,
        ReceiveInfo->ReceiveLength,
        ReceiveInfo->ReceiveFlags,
        &BytesReceived);
      if (Status != STATUS_PENDING)
      {
          DispDataRequestComplete(Irp, Status, BytesReceived);
      }
    }

  if (Status != STATUS_PENDING)
    {
      IrpSp->Control &= ~SL_PENDING_RETURNED;
    }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

  return Status;
}


NTSTATUS DispTdiReceiveDatagram(
    PIRP Irp)
/*
 * FUNCTION: TDI_RECEIVE_DATAGRAM handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PIO_STACK_LOCATION IrpSp;
  PTDI_REQUEST_KERNEL_RECEIVEDG DgramInfo;
  PTRANSPORT_CONTEXT TranContext;
  TDI_REQUEST Request;
  NTSTATUS Status;
  ULONG BytesReceived;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp     = IoGetCurrentIrpStackLocation(Irp);
  DgramInfo = (PTDI_REQUEST_KERNEL_RECEIVEDG)&(IrpSp->Parameters);

  TranContext = IrpSp->FileObject->FsContext;
  if (TranContext == NULL)
    {
      TI_DbgPrint(MID_TRACE, ("Bad transport context.\n"));
      return STATUS_INVALID_ADDRESS;
    }

  /* Initialize a receive request */
  Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
  Request.RequestNotifyObject  = DispDataRequestComplete;
  Request.RequestContext       = Irp;
  Status = DispPrepareIrpForCancel(
    IrpSp->FileObject->FsContext,
    Irp,
    (PDRIVER_CANCEL)DispCancelRequest);
  if (NT_SUCCESS(Status))
    {
      Status = UDPReceiveDatagram(
        &Request,
        DgramInfo->ReceiveDatagramInformation,
        (PNDIS_BUFFER)Irp->MdlAddress,
        DgramInfo->ReceiveLength,
        DgramInfo->ReceiveFlags,
        DgramInfo->ReturnDatagramInformation,
        &BytesReceived);
      if (Status != STATUS_PENDING)
        {
          DispDataRequestComplete(Irp, Status, BytesReceived);
        }
    }

  if (Status != STATUS_PENDING)
    {
      IrpSp->Control &= ~SL_PENDING_RETURNED;
    }

  TI_DbgPrint(DEBUG_IRP, ("Leaving. Status is (0x%X)\n", Status));

  return Status;
}


NTSTATUS DispTdiSend(
    PIRP Irp)
/*
 * FUNCTION: TDI_SEND handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    PIO_STACK_LOCATION IrpSp;
    TDI_REQUEST Request;
    PTDI_REQUEST_KERNEL_SEND SendInfo;
    PTRANSPORT_CONTEXT TranContext;
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    SendInfo    = (PTDI_REQUEST_KERNEL_SEND)&(IrpSp->Parameters);
    TranContext = IrpSp->FileObject->FsContext;

    /* Initialize a send request */
    Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
    Request.RequestNotifyObject  = DispDataRequestComplete;
    Request.RequestContext       = Irp;

    Status = DispPrepareIrpForCancel(
        IrpSp->FileObject->FsContext,
        Irp,
        (PDRIVER_CANCEL)DispCancelRequest);
    if (NT_SUCCESS(Status)) {

        /* FIXME: DgramInfo->SendDatagramInformation->RemoteAddress 
           must be of type PTDI_ADDRESS_IP */

        Status = (*((PADDRESS_FILE)Request.Handle.AddressHandle)->Send)(
            &Request, NULL, 
	    (PNDIS_BUFFER)Irp->MdlAddress, SendInfo->SendLength);
        if (Status != STATUS_PENDING) {
            DispDataRequestComplete(Irp, Status, 0);
            /* Return STATUS_PENDING because DispPrepareIrpForCancel
               marks Irp as pending */
            Status = STATUS_PENDING;
        }
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving.\n"));

    return Status;
}


NTSTATUS DispTdiSendDatagram(
    PIRP Irp)
/*
 * FUNCTION: TDI_SEND_DATAGRAM handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    PIO_STACK_LOCATION IrpSp;
    TDI_REQUEST Request;
    PTDI_REQUEST_KERNEL_SENDDG DgramInfo;
    PTRANSPORT_CONTEXT TranContext;
    NTSTATUS Status;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    IrpSp       = IoGetCurrentIrpStackLocation(Irp);
    DgramInfo   = (PTDI_REQUEST_KERNEL_SENDDG)&(IrpSp->Parameters);
    TranContext = IrpSp->FileObject->FsContext;

    /* Initialize a send request */
    Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
    Request.RequestNotifyObject  = DispDataRequestComplete;
    Request.RequestContext       = Irp;

    Status = DispPrepareIrpForCancel(
        IrpSp->FileObject->FsContext,
        Irp,
        (PDRIVER_CANCEL)DispCancelRequest);
    if (NT_SUCCESS(Status)) {

        /* FIXME: DgramInfo->SendDatagramInformation->RemoteAddress 
           must be of type PTDI_ADDRESS_IP */

        Status = (*((PADDRESS_FILE)Request.Handle.AddressHandle)->Send)(
            &Request, DgramInfo->SendDatagramInformation,
            (PNDIS_BUFFER)Irp->MdlAddress, DgramInfo->SendLength);
        if (Status != STATUS_PENDING) {
            DispDataRequestComplete(Irp, Status, 0);
            /* Return STATUS_PENDING because DispPrepareIrpForCancel
               marks Irp as pending */
            Status = STATUS_PENDING;
        }
    }

    TI_DbgPrint(DEBUG_IRP, ("Leaving.\n"));

    return Status;
}


NTSTATUS DispTdiSetEventHandler(
  PIRP Irp)
/*
 * FUNCTION: TDI_SET_EVENT_HANDER handler
 * ARGUMENTS:
 *     Irp = Pointer to a I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PTDI_REQUEST_KERNEL_SET_EVENT Parameters;
  PTRANSPORT_CONTEXT TranContext;
  PIO_STACK_LOCATION IrpSp;
  PADDRESS_FILE AddrFile;
  NTSTATUS Status;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  /* Get associated address file object. Quit if none exists */

  TranContext = IrpSp->FileObject->FsContext;
  if (!TranContext) {
    TI_DbgPrint(MIN_TRACE, ("Bad transport context.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  AddrFile = (PADDRESS_FILE)TranContext->Handle.AddressHandle;
  if (!AddrFile) {
    TI_DbgPrint(MIN_TRACE, ("No address file object.\n"));
    return STATUS_INVALID_PARAMETER;
  }

  Parameters = (PTDI_REQUEST_KERNEL_SET_EVENT)&IrpSp->Parameters;
  Status     = STATUS_SUCCESS;
  
  KeAcquireSpinLock(&AddrFile->Lock, &OldIrql);

  /* Set the event handler. if an event handler is associated with
     a specific event, it's flag (RegisteredXxxHandler) is TRUE.
     If an event handler is not used it's flag is FALSE */
  switch (Parameters->EventType) {
  case TDI_EVENT_CONNECT:
    if (!Parameters->EventHandler) {
      AddrFile->ConnectHandlerContext    = NULL;
      AddrFile->RegisteredConnectHandler = FALSE;
    } else {
      AddrFile->ConnectHandler =
        (PTDI_IND_CONNECT)Parameters->EventHandler;
      AddrFile->ConnectHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredConnectHandler = TRUE;
    }
    break;

  case TDI_EVENT_DISCONNECT:
    if (!Parameters->EventHandler) {
      AddrFile->DisconnectHandlerContext    = NULL;
      AddrFile->RegisteredDisconnectHandler = FALSE;
    } else {
      AddrFile->DisconnectHandler =
        (PTDI_IND_DISCONNECT)Parameters->EventHandler;
      AddrFile->DisconnectHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredDisconnectHandler = TRUE;
    }
    break;

    case TDI_EVENT_ERROR:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ErrorHandlerContext    = NULL;
      AddrFile->RegisteredErrorHandler = FALSE;
    } else {
      AddrFile->ErrorHandler =
        (PTDI_IND_ERROR)Parameters->EventHandler;
      AddrFile->ErrorHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredErrorHandler = TRUE;
    }
    break;

  case TDI_EVENT_RECEIVE:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ReceiveHandlerContext    = NULL;
      AddrFile->RegisteredReceiveHandler = FALSE;
    } else {
      AddrFile->ReceiveHandler =
        (PTDI_IND_RECEIVE)Parameters->EventHandler;
      AddrFile->ReceiveHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredReceiveHandler = TRUE;
    }
    break;

  case TDI_EVENT_RECEIVE_DATAGRAM:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ReceiveDatagramHandlerContext    = NULL;
      AddrFile->RegisteredReceiveDatagramHandler = FALSE;
    } else {
      AddrFile->ReceiveDatagramHandler =
        (PTDI_IND_RECEIVE_DATAGRAM)Parameters->EventHandler;
      AddrFile->ReceiveDatagramHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredReceiveDatagramHandler = TRUE;
    }
    break;

  case TDI_EVENT_RECEIVE_EXPEDITED:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ExpeditedReceiveHandlerContext    = NULL;
      AddrFile->RegisteredExpeditedReceiveHandler = FALSE;
    } else {
      AddrFile->ExpeditedReceiveHandler =
        (PTDI_IND_RECEIVE_EXPEDITED)Parameters->EventHandler;
      AddrFile->ExpeditedReceiveHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredExpeditedReceiveHandler = TRUE;
    }
    break;

  case TDI_EVENT_CHAINED_RECEIVE:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ChainedReceiveHandlerContext    = NULL;
      AddrFile->RegisteredChainedReceiveHandler = FALSE;
    } else {
      AddrFile->ChainedReceiveHandler =
        (PTDI_IND_CHAINED_RECEIVE)Parameters->EventHandler;
      AddrFile->ChainedReceiveHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredChainedReceiveHandler = TRUE;
    }
    break;

  case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ChainedReceiveDatagramHandlerContext    = NULL;
      AddrFile->RegisteredChainedReceiveDatagramHandler = FALSE;
    } else {
      AddrFile->ChainedReceiveDatagramHandler =
        (PTDI_IND_CHAINED_RECEIVE_DATAGRAM)Parameters->EventHandler;
      AddrFile->ChainedReceiveDatagramHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredChainedReceiveDatagramHandler = TRUE;
    }
    break;

  case TDI_EVENT_CHAINED_RECEIVE_EXPEDITED:
    if (Parameters->EventHandler == NULL) {
      AddrFile->ChainedReceiveExpeditedHandlerContext    = NULL;
      AddrFile->RegisteredChainedReceiveExpeditedHandler = FALSE;
    } else {
      AddrFile->ChainedReceiveExpeditedHandler =
        (PTDI_IND_CHAINED_RECEIVE_EXPEDITED)Parameters->EventHandler;
      AddrFile->ChainedReceiveExpeditedHandlerContext    = Parameters->EventContext;
      AddrFile->RegisteredChainedReceiveExpeditedHandler = TRUE;
    }
    break;

  default:
    TI_DbgPrint(MIN_TRACE, ("Unknown event type (0x%X).\n",
      Parameters->EventType));

    Status = STATUS_INVALID_PARAMETER;
  }

  KeReleaseSpinLock(&AddrFile->Lock, OldIrql);

  return Status;
}


NTSTATUS DispTdiSetInformation(
    PIRP Irp)
/*
 * FUNCTION: TDI_SET_INFORMATION handler
 * ARGUMENTS:
 *     Irp = Pointer to an I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

	return STATUS_NOT_IMPLEMENTED;
}


VOID DispTdiQueryInformationExComplete(
    PVOID Context,
    ULONG Status,
    UINT ByteCount)
/*
 * FUNCTION: Completes a TDI QueryInformationEx request
 * ARGUMENTS:
 *     Context   = Pointer to the IRP for the request
 *     Status    = TDI status of the request
 *     ByteCount = Number of bytes returned in output buffer
 */
{
    PTI_QUERY_CONTEXT QueryContext;
    UINT Count = 0;

    QueryContext = (PTI_QUERY_CONTEXT)Context;
    if (NT_SUCCESS(Status)) {
        Count = CopyBufferToBufferChain(
            QueryContext->InputMdl,
            FIELD_OFFSET(TCP_REQUEST_QUERY_INFORMATION_EX, Context),
            (PUCHAR)&QueryContext->QueryInfo.Context,
            CONTEXT_SIZE);
    }

    MmUnlockPages(QueryContext->InputMdl);
    IoFreeMdl(QueryContext->InputMdl);
    MmUnlockPages(QueryContext->OutputMdl);
    IoFreeMdl(QueryContext->OutputMdl);

    QueryContext->Irp->IoStatus.Information = ByteCount;
    QueryContext->Irp->IoStatus.Status      = Status;

    ExFreePool(QueryContext);
}


NTSTATUS DispTdiQueryInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: TDI QueryInformationEx handler
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    PTCP_REQUEST_QUERY_INFORMATION_EX InputBuffer;
    PTRANSPORT_CONTEXT TranContext;
    PTI_QUERY_CONTEXT QueryContext;
    PVOID OutputBuffer;
    TDI_REQUEST Request;
    UINT Size;
    UINT InputBufferLength;
    UINT OutputBufferLength;
    BOOLEAN InputMdlLocked  = FALSE;
    BOOLEAN OutputMdlLocked = FALSE;
    PMDL InputMdl           = NULL;
    PMDL OutputMdl          = NULL;
    NTSTATUS Status         = STATUS_SUCCESS;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;

    switch ((ULONG)IrpSp->FileObject->FsContext2) {
    case TDI_TRANSPORT_ADDRESS_FILE:
        Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        Request.Handle.ControlChannel = TranContext->Handle.ControlChannel;
        break;

    default:
        TI_DbgPrint(MIN_TRACE, ("Invalid transport context\n"));
        return STATUS_INVALID_PARAMETER;
    }

    InputBufferLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputBufferLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    /* Validate parameters */
    if ((InputBufferLength == sizeof(TCP_REQUEST_QUERY_INFORMATION_EX)) &&
        (OutputBufferLength != 0)) {

        InputBuffer = (PTCP_REQUEST_QUERY_INFORMATION_EX)
            IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        OutputBuffer = Irp->UserBuffer;

        QueryContext = ExAllocatePool(NonPagedPool, sizeof(TI_QUERY_CONTEXT));
        if (QueryContext) {
#ifdef _MSC_VER
            try {
#endif
                InputMdl = IoAllocateMdl(InputBuffer,
                    sizeof(TCP_REQUEST_QUERY_INFORMATION_EX),
                    FALSE, TRUE, NULL);

                OutputMdl = IoAllocateMdl(OutputBuffer,
                    OutputBufferLength, FALSE, TRUE, NULL);

                if (InputMdl && OutputMdl) {

                    MmProbeAndLockPages(InputMdl, Irp->RequestorMode,
                        IoModifyAccess);

                    InputMdlLocked = TRUE;

                    MmProbeAndLockPages(OutputMdl, Irp->RequestorMode,
                        IoWriteAccess);

                    OutputMdlLocked = TRUE;

                    RtlCopyMemory(&QueryContext->QueryInfo,
                        InputBuffer, sizeof(TCP_REQUEST_QUERY_INFORMATION_EX));

                } else
                    Status = STATUS_INSUFFICIENT_RESOURCES;
#ifdef _MSC_VER
            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
            }
#endif
            if (NT_SUCCESS(Status)) {
                Size = MmGetMdlByteCount(OutputMdl);

                QueryContext->Irp       = Irp;
                QueryContext->InputMdl  = InputMdl;
                QueryContext->OutputMdl = OutputMdl;

                Request.RequestNotifyObject = DispTdiQueryInformationExComplete;
                Request.RequestContext      = QueryContext;
                Status = InfoTdiQueryInformationEx(&Request,
                    &QueryContext->QueryInfo.ID, OutputMdl,
                    &Size, &QueryContext->QueryInfo.Context);
                DispTdiQueryInformationExComplete(QueryContext, Status, Size);

                TI_DbgPrint(MAX_TRACE, ("Leaving. Status = (0x%X)\n", Status));

                return Status;
            }

            /* An error occurred if we get here */

            if (InputMdl) {
                if (InputMdlLocked)
                    MmUnlockPages(InputMdl);
                IoFreeMdl(InputMdl);
            }

            if (OutputMdl) {
                if (OutputMdlLocked)
                    MmUnlockPages(OutputMdl);
                IoFreeMdl(OutputMdl);
            }

            ExFreePool(QueryContext);
        } else
            Status = STATUS_INSUFFICIENT_RESOURCES;
    } else
        Status = STATUS_INVALID_PARAMETER;

    TI_DbgPrint(MIN_TRACE, ("Leaving. Status = (0x%X)\n", Status));

    return Status;
}


NTSTATUS DispTdiSetInformationEx(
    PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
/*
 * FUNCTION: TDI SetInformationEx handler
 * ARGUMENTS:
 *     Irp   = Pointer to I/O request packet
 *     IrpSp = Pointer to current stack location of Irp
 * RETURNS:
 *     Status of operation
 */
{
    PTRANSPORT_CONTEXT TranContext;
    PTCP_REQUEST_SET_INFORMATION_EX Info;
    TDI_REQUEST Request;
    TDI_STATUS Status;
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IRP, ("Called.\n"));

    TranContext = (PTRANSPORT_CONTEXT)IrpSp->FileObject->FsContext;
    Info        = (PTCP_REQUEST_SET_INFORMATION_EX)Irp->AssociatedIrp.SystemBuffer;

    switch ((ULONG)IrpSp->FileObject->FsContext2) {
    case TDI_TRANSPORT_ADDRESS_FILE:
        Request.Handle.AddressHandle = TranContext->Handle.AddressHandle;
        break;

    case TDI_CONNECTION_FILE:
        Request.Handle.ConnectionContext = TranContext->Handle.ConnectionContext;
        break;

    case TDI_CONTROL_CHANNEL_FILE:
        Request.Handle.ControlChannel = TranContext->Handle.ControlChannel;
        break;

    default:
        Irp->IoStatus.Status      = STATUS_INVALID_PARAMETER;
        Irp->IoStatus.Information = 0;

        TI_DbgPrint(DEBUG_IRP, ("Completing IRP at (0x%X).\n", Irp));

        return IRPFinish(Irp, STATUS_INVALID_PARAMETER);
    }

    Status = DispPrepareIrpForCancel(TranContext, Irp, NULL);
    if (NT_SUCCESS(Status)) {
        Request.RequestNotifyObject = DispDataRequestComplete;
        Request.RequestContext      = Irp;

        Status = InfoTdiSetInformationEx(&Request, &Info->ID,
            &Info->Buffer, Info->BufferSize);

        if (Status != STATUS_PENDING) {
            IoAcquireCancelSpinLock(&OldIrql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(OldIrql);
        }
    }

    return Status;
}

/* EOF */
