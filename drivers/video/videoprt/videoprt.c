/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: videoprt.c,v 1.2 2004/02/10 16:22:56 navaraf Exp $
 */

#include "videoprt.h"

BOOLEAN CsrssInitialized = FALSE;
PEPROCESS Csrss = NULL;
PVIDEO_PORT_DEVICE_EXTENSION ResetDisplayParametersDeviceExtension = NULL;

//  -------------------------------------------------------  Public Interface

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS  

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
  DPRINT("DriverEntry()\n");
  return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
VOID
VideoPortDebugPrint(IN VIDEO_DEBUG_LEVEL DebugPrintLevel,
                    IN PCHAR DebugMessage, ...)
{
	char Buffer[256];
	va_list ap;

/*
	if (DebugPrintLevel > InternalDebugLevel)
		return;
*/
	va_start (ap, DebugMessage);
	vsprintf (Buffer, DebugMessage, ap);
	va_end (ap);

	DbgPrint (Buffer);
}


/*
 * @unimplemented
 */
VP_STATUS 
STDCALL
VideoPortDisableInterrupt(IN PVOID  HwDeviceExtension)
{
  DPRINT("VideoPortDisableInterrupt\n");
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
VP_STATUS 
STDCALL
VideoPortEnableInterrupt(IN PVOID  HwDeviceExtension)
{
  DPRINT("VideoPortEnableInterrupt\n");
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
VOID 
STDCALL
VideoPortFreeDeviceBase(IN PVOID  HwDeviceExtension, 
                        IN PVOID  MappedAddress)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortFreeDeviceBase\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  InternalUnmapMemory(DeviceExtension, MappedAddress);
}


/*
 * @implemented
 */
ULONG 
STDCALL
VideoPortGetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    OUT PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortGetBusData\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  return HalGetBusDataByOffset(BusDataType, 
                               DeviceExtension->SystemIoBusNumber, 
                               SlotNumber, 
                               Buffer, 
                               Offset, 
                               Length);
}


/*
 * @implemented
 */
UCHAR 
STDCALL
VideoPortGetCurrentIrql(VOID)
{
  DPRINT("VideoPortGetCurrentIrql\n");
  return KeGetCurrentIrql();
}


/*
 * @implemented
 */
PVOID 
STDCALL
VideoPortGetDeviceBase(IN PVOID  HwDeviceExtension,
                       IN PHYSICAL_ADDRESS  IoAddress,
                       IN ULONG  NumberOfUchars,
                       IN UCHAR  InIoSpace)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortGetDeviceBase\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  return InternalMapMemory(DeviceExtension, IoAddress, NumberOfUchars, InIoSpace);
}


/*
 * @unimplemented
 */
VP_STATUS 
STDCALL
VideoPortGetDeviceData(IN PVOID  HwDeviceExtension,
                       IN VIDEO_DEVICE_DATA_TYPE  DeviceDataType,
                       IN PMINIPORT_QUERY_DEVICE_ROUTINE  CallbackRoutine,
                       IN PVOID Context)
{
  DPRINT("VideoPortGetDeviceData\n");
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
VP_STATUS 
STDCALL
VideoPortGetAccessRanges(IN PVOID  HwDeviceExtension,
                         IN ULONG  NumRequestedResources,
                         IN PIO_RESOURCE_DESCRIPTOR  RequestedResources OPTIONAL,
                         IN ULONG  NumAccessRanges,
                         IN PVIDEO_ACCESS_RANGE  AccessRanges,
                         IN PVOID  VendorId,
                         IN PVOID  DeviceId,
                         IN PULONG  Slot)
{
  PCI_SLOT_NUMBER PciSlotNumber;
  BOOLEAN FoundDevice;
  ULONG FunctionNumber;
  PCI_COMMON_CONFIG Config;
  PCM_RESOURCE_LIST AllocatedResources;
  NTSTATUS Status;
  UINT AssignedCount;
  CM_FULL_RESOURCE_DESCRIPTOR *FullList;
  CM_PARTIAL_RESOURCE_DESCRIPTOR *Descriptor;
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortGetAccessRanges\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  if (0 == NumRequestedResources && PCIBus == DeviceExtension->AdapterInterfaceType)
    {
      DPRINT("Looking for VendorId 0x%04x DeviceId 0x%04x\n", (int)*((USHORT *) VendorId),
             (int)*((USHORT *) DeviceId));
      FoundDevice = FALSE;
      PciSlotNumber.u.AsULONG = *Slot;
      for (FunctionNumber = 0; ! FoundDevice && FunctionNumber < 8; FunctionNumber++)
	{
	  PciSlotNumber.u.bits.FunctionNumber = FunctionNumber;
	  if (sizeof(PCI_COMMON_CONFIG) ==
	      HalGetBusDataByOffset(PCIConfiguration, DeviceExtension->SystemIoBusNumber,
	                            PciSlotNumber.u.AsULONG,&Config, 0,
	                            sizeof(PCI_COMMON_CONFIG)))
	    {
	      DPRINT("Slot 0x%02x (Device %d Function %d) VendorId 0x%04x DeviceId 0x%04x\n",
	             PciSlotNumber.u.AsULONG, PciSlotNumber.u.bits.DeviceNumber,
	             PciSlotNumber.u.bits.FunctionNumber, Config.VendorID, Config.DeviceID);
	      FoundDevice = (Config.VendorID == *((USHORT *) VendorId) &&
	                     Config.DeviceID == *((USHORT *) DeviceId));
	    }
	}
      if (! FoundDevice)
	{
	  return STATUS_UNSUCCESSFUL;
	}
      Status = HalAssignSlotResources(NULL, NULL, NULL, NULL,
                                      DeviceExtension->AdapterInterfaceType,
                                      DeviceExtension->SystemIoBusNumber,
                                      PciSlotNumber.u.AsULONG, &AllocatedResources);
      if (! NT_SUCCESS(Status))
	{
	  return Status;
	}
      AssignedCount = 0;
      for (FullList = AllocatedResources->List;
           FullList < AllocatedResources->List + AllocatedResources->Count;
           FullList++)
	{
	  assert(FullList->InterfaceType == PCIBus &&
	         FullList->BusNumber == DeviceExtension->SystemIoBusNumber &&
	         1 == FullList->PartialResourceList.Version &&
	         1 == FullList->PartialResourceList.Revision);
	  for (Descriptor = FullList->PartialResourceList.PartialDescriptors;
	       Descriptor < FullList->PartialResourceList.PartialDescriptors + FullList->PartialResourceList.Count;
	       Descriptor++)
	    {
              if ((CmResourceTypeMemory == Descriptor->Type
                   || CmResourceTypePort == Descriptor->Type)
                  && NumAccessRanges <= AssignedCount)
		{
                  DPRINT1("Too many access ranges found\n");
                  ExFreePool(AllocatedResources);
                  return STATUS_UNSUCCESSFUL;
                }
	      if (CmResourceTypeMemory == Descriptor->Type)
		{
                  if (NumAccessRanges <= AssignedCount)
                    {
                      DPRINT1("Too many access ranges found\n");
                      ExFreePool(AllocatedResources);
                      return STATUS_UNSUCCESSFUL;
                    }
		  DPRINT("Memory range starting at 0x%08x length 0x%08x\n",
		         Descriptor->u.Memory.Start.u.LowPart, Descriptor->u.Memory.Length);
		  AccessRanges[AssignedCount].RangeStart = Descriptor->u.Memory.Start;
		  AccessRanges[AssignedCount].RangeLength = Descriptor->u.Memory.Length;
		  AccessRanges[AssignedCount].RangeInIoSpace = 0;
		  AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
		  AccessRanges[AssignedCount].RangeShareable =
		    (CmResourceShareShared == Descriptor->ShareDisposition);
		  AssignedCount++;
		}
	      else if (CmResourceTypePort == Descriptor->Type)
		{
		  DPRINT("Port range starting at 0x%04x length %d\n",
		         Descriptor->u.Memory.Start.u.LowPart, Descriptor->u.Memory.Length);
		  AccessRanges[AssignedCount].RangeStart = Descriptor->u.Port.Start;
		  AccessRanges[AssignedCount].RangeLength = Descriptor->u.Port.Length;
		  AccessRanges[AssignedCount].RangeInIoSpace = 1;
		  AccessRanges[AssignedCount].RangeVisible = 0; /* FIXME: Just guessing */
		  AccessRanges[AssignedCount].RangeShareable = 0;
		  AssignedCount++;
		}
              else if (CmResourceTypeInterrupt == Descriptor->Type)
                {
                  DeviceExtension->InterruptLevel = Descriptor->u.Interrupt.Level;
                  DeviceExtension->InterruptVector = Descriptor->u.Interrupt.Vector;
                }
	    }
	}
      ExFreePool(AllocatedResources);
    }
  else
    {
    UNIMPLEMENTED
    }

  return STATUS_SUCCESS;
}

typedef struct QueryRegistryCallbackContext
{
  PVOID HwDeviceExtension;
  PVOID HwContext;
  PMINIPORT_GET_REGISTRY_ROUTINE HwGetRegistryRoutine;
} QUERY_REGISTRY_CALLBACK_CONTEXT, *PQUERY_REGISTRY_CALLBACK_CONTEXT;

static NTSTATUS STDCALL
QueryRegistryCallback(IN PWSTR ValueName,
                      IN ULONG ValueType,
                      IN PVOID ValueData,
                      IN ULONG ValueLength,
                      IN PVOID Context,
                      IN PVOID EntryContext)
{
  PQUERY_REGISTRY_CALLBACK_CONTEXT CallbackContext = (PQUERY_REGISTRY_CALLBACK_CONTEXT) Context;

  DPRINT("Found registry value for name %S: type %d, length %d\n",
         ValueName, ValueType, ValueLength);
  return (*(CallbackContext->HwGetRegistryRoutine))(CallbackContext->HwDeviceExtension,
                                                    CallbackContext->HwContext,
                                                    ValueName,
                                                    ValueData,
                                                    ValueLength);
}

/*
 * @unimplemented
 */
VP_STATUS 
STDCALL
VideoPortGetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ParameterName,
                               IN UCHAR  IsParameterFileName,
                               IN PMINIPORT_GET_REGISTRY_ROUTINE  GetRegistryRoutine,
                               IN PVOID  HwContext)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  QUERY_REGISTRY_CALLBACK_CONTEXT Context;
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortGetRegistryParameters ParameterName %S\n", ParameterName);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  if (IsParameterFileName)
    {
      UNIMPLEMENTED;
    }

  DPRINT("ParameterName %S\n", ParameterName);

  Context.HwDeviceExtension = HwDeviceExtension;
  Context.HwContext = HwContext;
  Context.HwGetRegistryRoutine = GetRegistryRoutine;

  QueryTable[0].QueryRoutine = QueryRegistryCallback;
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[0].Name = ParameterName;
  QueryTable[0].EntryContext = NULL;
  QueryTable[0].DefaultType = REG_NONE;
  QueryTable[0].DefaultData = NULL;
  QueryTable[0].DefaultLength = 0;

  QueryTable[1].QueryRoutine = NULL;
  QueryTable[1].Name = NULL;

  return NT_SUCCESS(RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                           DeviceExtension->RegistryPath.Buffer,
                                           QueryTable, &Context, NULL))
         ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
}

static BOOLEAN
VPInterruptRoutine(IN struct _KINTERRUPT *Interrupt,
                   IN PVOID ServiceContext)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
  
  DeviceExtension = ServiceContext;
  assert(NULL != DeviceExtension->HwInterrupt);

  return DeviceExtension->HwInterrupt(&DeviceExtension->MiniPortDeviceExtension);
}

static VOID STDCALL
VPTimerRoutine(IN PDEVICE_OBJECT DeviceObject,
               IN PVOID Context)
{
  PVIDEO_PORT_DEVICE_EXTENSION  DeviceExtension;
  
  DeviceExtension = Context;
  assert(DeviceExtension == DeviceObject->DeviceExtension
         && NULL != DeviceExtension->HwTimer);

  DeviceExtension->HwTimer(&DeviceExtension->MiniPortDeviceExtension);
}

/*
 * @implemented
 */
ULONG STDCALL
VideoPortInitialize(IN PVOID  Context1,
                    IN PVOID  Context2,
                    IN PVIDEO_HW_INITIALIZATION_DATA  HwInitializationData,
                    IN PVOID  HwContext)
{
  PUNICODE_STRING RegistryPath;
  UCHAR Again;
  WCHAR DeviceBuffer[20];
  WCHAR SymlinkBuffer[20];
  WCHAR DeviceVideoBuffer[20];
  NTSTATUS Status;
  PDRIVER_OBJECT MPDriverObject = (PDRIVER_OBJECT) Context1;
  PDEVICE_OBJECT MPDeviceObject;
  VIDEO_PORT_CONFIG_INFO ConfigInfo;
  PVIDEO_PORT_DEVICE_EXTENSION  DeviceExtension;
  ULONG DeviceNumber = 0;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;
  ULONG MaxBus;
  ULONG MaxLen;
  KIRQL IRQL;
  KAFFINITY Affinity;
  ULONG InterruptVector;

  DPRINT("VideoPortInitialize\n");

  RegistryPath = (PUNICODE_STRING) Context2;

  /*  Build Dispatch table from passed data  */
  MPDriverObject->DriverStartIo = (PDRIVER_STARTIO) HwInitializationData->HwStartIO;

  /*  Create a unicode device name  */
  Again = FALSE;
  do
    {
      swprintf(DeviceBuffer, L"\\Device\\Video%lu", DeviceNumber);
      RtlInitUnicodeString(&DeviceName, DeviceBuffer);

      /*  Create the device  */
      Status = IoCreateDevice(MPDriverObject,
                              HwInitializationData->HwDeviceExtensionSize +
                                sizeof(VIDEO_PORT_DEVICE_EXTENSION),
                              &DeviceName,
                              FILE_DEVICE_VIDEO,
                              0,
                              TRUE,
                              &MPDeviceObject);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IoCreateDevice call failed with status 0x%08x\n", Status);
          return Status;
        }

      MPDriverObject->DeviceObject = MPDeviceObject;

      /* Initialize the miniport drivers dispatch table */
      MPDriverObject->MajorFunction[IRP_MJ_CREATE] = VidDispatchOpen;
      MPDriverObject->MajorFunction[IRP_MJ_CLOSE] = VidDispatchClose;
      MPDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VidDispatchDeviceControl;

      /* Initialize our device extension */
      DeviceExtension = 
        (PVIDEO_PORT_DEVICE_EXTENSION) MPDeviceObject->DeviceExtension;
      DeviceExtension->DeviceObject = MPDeviceObject;
      DeviceExtension->HwInitialize = HwInitializationData->HwInitialize;
      DeviceExtension->HwResetHw = HwInitializationData->HwResetHw;
      DeviceExtension->AdapterInterfaceType = HwInitializationData->AdapterInterfaceType;
      DeviceExtension->SystemIoBusNumber = 0;
      MaxLen = (wcslen(RegistryPath->Buffer) + 10) * sizeof(WCHAR);
      DeviceExtension->RegistryPath.MaximumLength = MaxLen;
      DeviceExtension->RegistryPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                                   MaxLen,
                                                                   TAG_VIDEO_PORT);
      swprintf(DeviceExtension->RegistryPath.Buffer, L"%s\\Device%d",
               RegistryPath->Buffer, DeviceNumber);
      DeviceExtension->RegistryPath.Length = wcslen(DeviceExtension->RegistryPath.Buffer) *
                                             sizeof(WCHAR);

      MaxBus = (DeviceExtension->AdapterInterfaceType == PCIBus) ? 8 : 1;
      DPRINT("MaxBus: %lu\n", MaxBus);
      InitializeListHead(&DeviceExtension->AddressMappingListHead);
      
      /*  Set the buffering strategy here...  */
      /*  If you change this, remember to change VidDispatchDeviceControl too */
      MPDeviceObject->Flags |= DO_BUFFERED_IO;

      do
	{
	  RtlZeroMemory(&DeviceExtension->MiniPortDeviceExtension, 
	                HwInitializationData->HwDeviceExtensionSize);
	  DPRINT("Searching on bus %d\n", DeviceExtension->SystemIoBusNumber);
	  /* Setup configuration info */
	  RtlZeroMemory(&ConfigInfo, sizeof(VIDEO_PORT_CONFIG_INFO));
	  ConfigInfo.Length = sizeof(VIDEO_PORT_CONFIG_INFO);
	  ConfigInfo.AdapterInterfaceType = DeviceExtension->AdapterInterfaceType;
	  ConfigInfo.SystemIoBusNumber = DeviceExtension->SystemIoBusNumber;
	  ConfigInfo.InterruptMode = (PCIBus == DeviceExtension->AdapterInterfaceType) ?
	                              LevelSensitive : Latched;

	  /*  Call HwFindAdapter entry point  */
	  /* FIXME: Need to figure out what string to pass as param 3  */
	  Status = HwInitializationData->HwFindAdapter(&DeviceExtension->MiniPortDeviceExtension,
	                                               Context2,
	                                               NULL,
	                                               &ConfigInfo,
	                                               &Again);
	  if (NO_ERROR != Status)
	    {
	      DPRINT("HwFindAdapter call failed with error %d\n", Status);
	      DeviceExtension->SystemIoBusNumber++;
	    }
	}
      while (NO_ERROR != Status && DeviceExtension->SystemIoBusNumber < MaxBus);

      if (NO_ERROR != Status)
        {
	  RtlFreeUnicodeString(&DeviceExtension->RegistryPath);
          IoDeleteDevice(MPDeviceObject);

          return  Status;
        }
      DPRINT("Found adapter\n");

      /* create symbolic link "\??\DISPLAYx" */
      swprintf(SymlinkBuffer, L"\\??\\DISPLAY%lu", DeviceNumber+1);
      RtlInitUnicodeString (&SymlinkName,
                            SymlinkBuffer);
      IoCreateSymbolicLink (&SymlinkName,
                            &DeviceName);

      /* Add entry to DEVICEMAP\VIDEO key in registry */
      swprintf(DeviceVideoBuffer, L"\\Device\\Video%d", DeviceNumber);
      RtlWriteRegistryValue(RTL_REGISTRY_DEVICEMAP,
                            L"VIDEO",
                            DeviceVideoBuffer,
                            REG_SZ,
                            DeviceExtension->RegistryPath.Buffer,
                            DeviceExtension->RegistryPath.Length + sizeof(WCHAR));

      /* FIXME: Allocate hardware resources for device  */

      /*  Allocate interrupt for device  */
      DeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;
      if (0 == ConfigInfo.BusInterruptVector)
        {
          ConfigInfo.BusInterruptVector = DeviceExtension->InterruptVector;
        }
      if (0 == ConfigInfo.BusInterruptLevel)
        {
          ConfigInfo.BusInterruptLevel = DeviceExtension->InterruptLevel;
        }
      if (NULL != HwInitializationData->HwInterrupt)
        {
          InterruptVector = 
            HalGetInterruptVector(ConfigInfo.AdapterInterfaceType,
                                  ConfigInfo.SystemIoBusNumber,
                                  ConfigInfo.BusInterruptLevel,
                                  ConfigInfo.BusInterruptVector,
                                  &IRQL,
                                  &Affinity);
          if (0 == InterruptVector)
            {
              DPRINT1("HalGetInterruptVector failed\n");
              IoDeleteDevice(MPDeviceObject);
              
              return STATUS_INSUFFICIENT_RESOURCES;
            }
          KeInitializeSpinLock(&DeviceExtension->InterruptSpinLock);
          Status = IoConnectInterrupt(&DeviceExtension->InterruptObject,
                                      VPInterruptRoutine,
                                      DeviceExtension,
                                      &DeviceExtension->InterruptSpinLock,
                                      InterruptVector,
                                      IRQL,
                                      IRQL,
                                      ConfigInfo.InterruptMode,
                                      FALSE,
                                      Affinity,
                                      FALSE);
          if (!NT_SUCCESS(Status))
            {
              DPRINT1("IoConnectInterrupt failed with status 0x%08x\n", Status);
              IoDeleteDevice(MPDeviceObject);
              
              return Status;
            }
        }
      DeviceNumber++;
    }
  while (Again);

  DeviceExtension->HwTimer = HwInitializationData->HwTimer;
  if (HwInitializationData->HwTimer != NULL)
    {
      DPRINT("Initializing timer\n");
      Status = IoInitializeTimer(MPDeviceObject,
                                 VPTimerRoutine,
                                 DeviceExtension);
      if (!NT_SUCCESS(Status))
        {
          DPRINT("IoInitializeTimer failed with status 0x%08x\n", Status);
          
          if (HwInitializationData->HwInterrupt != NULL)
            {
              IoDisconnectInterrupt(DeviceExtension->InterruptObject);
            }
          IoDeleteDevice(MPDeviceObject);
          
          return Status;
        }
    }

  return  STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
VOID 
STDCALL
VideoPortLogError(IN PVOID  HwDeviceExtension,
                  IN PVIDEO_REQUEST_PACKET  Vrp OPTIONAL,
                  IN VP_STATUS  ErrorCode,
                  IN ULONG  UniqueId)
{
  DPRINT1("VideoPortLogError ErrorCode %d (0x%x) UniqueId %lu (0x%lx)\n",
          ErrorCode, ErrorCode, UniqueId, UniqueId);
  if (NULL != Vrp)
    {
      DPRINT1("Vrp->IoControlCode %lu (0x%lx)\n", Vrp->IoControlCode, Vrp->IoControlCode);
    }
}


/*
 * @unimplemented
 */
VP_STATUS 
STDCALL
VideoPortMapBankedMemory(IN PVOID  HwDeviceExtension,
                         IN PHYSICAL_ADDRESS  PhysicalAddress,
                         IN PULONG  Length,
                         IN PULONG  InIoSpace,
                         OUT PVOID  *VirtualAddress,
                         IN ULONG  BankLength,
                         IN UCHAR  ReadWriteBank,
                         IN PBANKED_SECTION_ROUTINE  BankRoutine,
                         IN PVOID  Context)
{
  DPRINT("VideoPortMapBankedMemory\n");
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
VP_STATUS 
STDCALL
VideoPortMapMemory(IN PVOID  HwDeviceExtension,
                   IN PHYSICAL_ADDRESS  PhysicalAddress,
                   IN PULONG  Length,
                   IN PULONG  InIoSpace,
                   OUT PVOID  *VirtualAddress)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortMapMemory\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  *VirtualAddress = InternalMapMemory(DeviceExtension, PhysicalAddress,
                                      *Length, *InIoSpace);

  return NULL == *VirtualAddress ? STATUS_NO_MEMORY : STATUS_SUCCESS;
}


/*
 * @implemented
 */
BOOLEAN 
STDCALL
VideoPortScanRom(IN PVOID  HwDeviceExtension, 
                 IN PUCHAR  RomBase,
                 IN ULONG  RomLength,
                 IN PUCHAR  String)
{
  ULONG StringLength;
  BOOLEAN Found;
  PUCHAR SearchLocation;

  DPRINT("VideoPortScanRom RomBase %p RomLength 0x%x String %s\n", RomBase, RomLength, String);

  StringLength = strlen(String);
  Found = FALSE;
  SearchLocation = RomBase;
  for (SearchLocation = RomBase;
       ! Found && SearchLocation < RomBase + RomLength - StringLength;
       SearchLocation++)
    {
      Found = (RtlCompareMemory(SearchLocation, String, StringLength) == StringLength);
      if (Found)
	{
	  DPRINT("Match found at %p\n", SearchLocation);
	}
    }

  return Found;
}


/*
 * @implemented
 */
ULONG 
STDCALL
VideoPortSetBusData(IN PVOID  HwDeviceExtension,
                    IN BUS_DATA_TYPE  BusDataType,
                    IN ULONG  SlotNumber,
                    IN PVOID  Buffer,
                    IN ULONG  Offset,
                    IN ULONG  Length)
{
  DPRINT("VideoPortSetBusData\n");
  return  HalSetBusDataByOffset(BusDataType,
                                0,
                                SlotNumber,
                                Buffer,
                                Offset,
                                Length);
}


/*
 * @implemented
 */
VP_STATUS 
STDCALL
VideoPortSetRegistryParameters(IN PVOID  HwDeviceExtension,
                               IN PWSTR  ValueName,
                               IN PVOID  ValueData,
                               IN ULONG  ValueLength)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoSetRegistryParameters\n");

  assert_irql(PASSIVE_LEVEL);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  return RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                               DeviceExtension->RegistryPath.Buffer,
                               ValueName,
                               REG_BINARY,
                               ValueData,
                               ValueLength);
}


/*
 * @unimplemented
 */
VP_STATUS 
STDCALL
VideoPortSetTrappedEmulatorPorts(IN PVOID  HwDeviceExtension,
                                 IN ULONG  NumAccessRanges,
                                 IN PVIDEO_ACCESS_RANGE  AccessRange)
{
  DPRINT("VideoPortSetTrappedEmulatorPorts\n");
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
VOID 
STDCALL
VideoPortStartTimer(IN PVOID  HwDeviceExtension)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortStartTimer\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  IoStartTimer(DeviceExtension->DeviceObject);
}


/*
 * @implemented
 */
VOID 
STDCALL
VideoPortStopTimer(IN PVOID  HwDeviceExtension)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortStopTimer\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  IoStopTimer(DeviceExtension->DeviceObject);
}


/*
 * @implemented
 */
BOOLEAN 
STDCALL
VideoPortSynchronizeExecution(IN PVOID  HwDeviceExtension,
                              IN VIDEO_SYNCHRONIZE_PRIORITY  Priority,
                              IN PMINIPORT_SYNCHRONIZE_ROUTINE  SynchronizeRoutine,
                              OUT PVOID  Context)
{
  BOOLEAN Ret;
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
  KIRQL OldIrql;

  DPRINT("VideoPortSynchronizeExecution\n");

  switch(Priority)
    {
    case VpLowPriority:
      Ret = (*SynchronizeRoutine)(Context);
      break;
    case VpMediumPriority:
      DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
                                          VIDEO_PORT_DEVICE_EXTENSION,
                                          MiniPortDeviceExtension);
      if (NULL == DeviceExtension->InterruptObject)
	{
	  Ret = (*SynchronizeRoutine)(Context);
	}
      else
	{
	  Ret = KeSynchronizeExecution(DeviceExtension->InterruptObject,
	                               SynchronizeRoutine,
	                               Context);
	}
      break;
    case VpHighPriority:
      OldIrql = KeGetCurrentIrql();
      if (OldIrql < SYNCH_LEVEL)
	{
	  OldIrql = KfRaiseIrql(SYNCH_LEVEL);
	}
      Ret = (*SynchronizeRoutine)(Context);
      if (OldIrql < SYNCH_LEVEL)
	{
	  KfLowerIrql(OldIrql);
	}
      break;
    default:
      Ret = FALSE;
    }

  return Ret;
}


/*
 * @implemented
 */
VP_STATUS 
STDCALL
VideoPortUnmapMemory(IN PVOID  HwDeviceExtension,
                     IN PVOID  VirtualAddress,
                     IN HANDLE  ProcessHandle)
{
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VideoPortFreeDeviceBase\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      VIDEO_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  InternalUnmapMemory(DeviceExtension, VirtualAddress);

  return STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VP_STATUS 
STDCALL
VideoPortVerifyAccessRanges(IN PVOID  HwDeviceExtension,
                            IN ULONG  NumAccessRanges,
                            IN PVIDEO_ACCESS_RANGE  AccessRanges)
{
  DPRINT1("VideoPortVerifyAccessRanges not implemented\n");
  return NO_ERROR;
}


/*
 * Reset display to blue screen
 */
static BOOLEAN STDCALL
VideoPortResetDisplayParameters(Columns, Rows)
{
  if (NULL == ResetDisplayParametersDeviceExtension)
    {
      return(FALSE);
    }
  if (NULL == ResetDisplayParametersDeviceExtension->HwResetHw)
    {
      return(FALSE);
    }
  if (!ResetDisplayParametersDeviceExtension->HwResetHw(&ResetDisplayParametersDeviceExtension->MiniPortDeviceExtension,
							Columns, Rows))
    {
      return(FALSE);
    }

  ResetDisplayParametersDeviceExtension = NULL;

  return TRUE;
}

/*
 * @implemented
 */

PVOID
STDCALL
VideoPortAllocatePool(
   IN PVOID HwDeviceExtension,
   IN VP_POOL_TYPE PoolType,
   IN SIZE_T NumberOfBytes,
   IN ULONG Tag)
{
   return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */

VOID
STDCALL
VideoPortFreePool(
   IN PVOID HwDeviceExtension,
   IN PVOID Ptr)
{
   ExFreePool(Ptr);
}

//    VidDispatchOpen
//
//  DESCRIPTION:
//    Answer requests for Open calls
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

NTSTATUS STDCALL
VidDispatchOpen(IN PDEVICE_OBJECT pDO,
                IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpStack;
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("VidDispatchOpen() called\n");

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  if (! CsrssInitialized)
    {
      DPRINT("Referencing CSRSS\n");
      Csrss = PsGetCurrentProcess();
      DPRINT("Csrss %p\n", Csrss);
    }
  else
    {
      DeviceExtension = (PVIDEO_PORT_DEVICE_EXTENSION) pDO->DeviceExtension;
      if (DeviceExtension->HwInitialize(&DeviceExtension->MiniPortDeviceExtension))
	{
	  Irp->IoStatus.Status = STATUS_SUCCESS;
	  /* Storing the device extension pointer in a static variable is an ugly
	   * hack. Unfortunately, we need it in VideoPortResetDisplayParameters
	   * and HalAcquireDisplayOwnership doesn't allow us to pass a userdata
           * parameter. On the bright side, the DISPLAY device is opened
	   * exclusively, so there can be only one device extension active at
	   * any point in time. */
	  ResetDisplayParametersDeviceExtension = DeviceExtension;
	  HalAcquireDisplayOwnership(VideoPortResetDisplayParameters);
	}
      else
	{
	  Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	}
    }

  Irp->IoStatus.Information = FILE_OPENED;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

//    VidDispatchClose
//
//  DESCRIPTION:
//    Answer requests for Close calls
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

NTSTATUS STDCALL
VidDispatchClose(IN PDEVICE_OBJECT pDO,
                 IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpStack;

  DPRINT("VidDispatchClose() called\n");

  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  if (! CsrssInitialized)
    {
      CsrssInitialized = TRUE;
    }
  else
    {
      HalReleaseDisplayOwnership();
    }

  Irp->IoStatus.Status = STATUS_SUCCESS;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

//    VidDispatchDeviceControl
//
//  DESCRIPTION:
//    Answer requests for device control calls
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    Standard dispatch arguments
//
//  RETURNS:
//    NTSTATUS
//

NTSTATUS STDCALL
VidDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                         IN PIRP Irp)
{
  PIO_STACK_LOCATION IrpStack;
  PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
  PVIDEO_REQUEST_PACKET vrp;

  DPRINT("VidDispatchDeviceControl\n");
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = DeviceObject->DeviceExtension;

  /* Translate the IRP to a VRP */
  vrp = ExAllocatePool(PagedPool, sizeof(VIDEO_REQUEST_PACKET));
  if (NULL == vrp)
    {
    return STATUS_NO_MEMORY;
    }
  vrp->StatusBlock        = (PSTATUS_BLOCK) &(Irp->IoStatus);
  vrp->IoControlCode      = IrpStack->Parameters.DeviceIoControl.IoControlCode;

  /* We're assuming METHOD_BUFFERED */
  vrp->InputBuffer        = Irp->AssociatedIrp.SystemBuffer;
  vrp->InputBufferLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
  vrp->OutputBuffer       = Irp->AssociatedIrp.SystemBuffer;
  vrp->OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

  /* Call the Miniport Driver with the VRP */
  ((PDRIVER_STARTIO)DeviceObject->DriverObject->DriverStartIo)((PVOID) &DeviceExtension->MiniPortDeviceExtension, (PIRP)vrp);

  /* Free the VRP */
  ExFreePool(vrp);

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

PVOID STDCALL
InternalMapMemory(IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
                  IN PHYSICAL_ADDRESS IoAddress,
                  IN ULONG NumberOfUchars,
                  IN UCHAR InIoSpace)
{
  PHYSICAL_ADDRESS TranslatedAddress;
  PVIDEO_PORT_ADDRESS_MAPPING AddressMapping;
  ULONG AddressSpace;
  PVOID MappedAddress;
  PLIST_ENTRY Entry;

  if (0 != (InIoSpace & VIDEO_MEMORY_SPACE_P6CACHE))
    {
      DPRINT("VIDEO_MEMORY_SPACE_P6CACHE not supported, turning off\n");
      InIoSpace &= ~VIDEO_MEMORY_SPACE_P6CACHE;
    }
  if (! IsListEmpty(&DeviceExtension->AddressMappingListHead))
    {
      Entry = DeviceExtension->AddressMappingListHead.Flink;
      while (Entry != &DeviceExtension->AddressMappingListHead)
	{
	  AddressMapping = CONTAINING_RECORD(Entry,
	                                     VIDEO_PORT_ADDRESS_MAPPING,
	                                     List);
	  if (IoAddress.QuadPart == AddressMapping->IoAddress.QuadPart &&
	      NumberOfUchars <= AddressMapping->NumberOfUchars)
	    {
	      AddressMapping->MappingCount++;
	      return AddressMapping->MappedAddress;
	    }
	  Entry = Entry->Flink;
	}
    }

  AddressSpace = (ULONG)InIoSpace;
  if (HalTranslateBusAddress(DeviceExtension->AdapterInterfaceType,
			     DeviceExtension->SystemIoBusNumber,
			     IoAddress,
			     &AddressSpace,
			     &TranslatedAddress) == FALSE)
    return NULL;

  /* i/o space */
  if (AddressSpace != 0)
    {
    assert(0 == TranslatedAddress.u.HighPart);
    return (PVOID) TranslatedAddress.u.LowPart;
    }

  MappedAddress = MmMapIoSpace(TranslatedAddress,
			       NumberOfUchars,
			       FALSE);

  AddressMapping = ExAllocatePoolWithTag(PagedPool,
			                 sizeof(VIDEO_PORT_ADDRESS_MAPPING),
                                         TAG_VIDEO_PORT);
  if (AddressMapping == NULL)
    return MappedAddress;

  AddressMapping->MappedAddress = MappedAddress;
  AddressMapping->NumberOfUchars = NumberOfUchars;
  AddressMapping->IoAddress = IoAddress;
  AddressMapping->SystemIoBusNumber = DeviceExtension->SystemIoBusNumber;
  AddressMapping->MappingCount = 1;

  InsertHeadList(&DeviceExtension->AddressMappingListHead,
		 &AddressMapping->List);

  return MappedAddress;
}

VOID STDCALL
InternalUnmapMemory(IN PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension,
                    IN PVOID MappedAddress)
{
  PVIDEO_PORT_ADDRESS_MAPPING AddressMapping;
  PLIST_ENTRY Entry;

  Entry = DeviceExtension->AddressMappingListHead.Flink;
  while (Entry != &DeviceExtension->AddressMappingListHead)
    {
      AddressMapping = CONTAINING_RECORD(Entry,
				         VIDEO_PORT_ADDRESS_MAPPING,
				         List);
      if (AddressMapping->MappedAddress == MappedAddress)
	{
	  assert(0 <= AddressMapping->MappingCount);
	  AddressMapping->MappingCount--;
	  if (0 == AddressMapping->MappingCount)
	    {
	      MmUnmapIoSpace(AddressMapping->MappedAddress,
	                     AddressMapping->NumberOfUchars);
	      RemoveEntryList(Entry);
	      ExFreePool(AddressMapping);

	      return;
	    }
	}

      Entry = Entry->Flink;
    }
}
