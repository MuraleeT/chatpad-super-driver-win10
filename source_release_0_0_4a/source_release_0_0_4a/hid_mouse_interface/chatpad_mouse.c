//
// Chatpad virtual mouse driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

// This driver was hand-written by following the hidkmdf example from the 7600.16385.1 WinDDK.
// This file is in C rather than C++ because for some reason, building with C++ results in a
// link error about HidRegisterMinidriver.

#include <wdm.h>

// TODO do I need to disable warnings here?
#include <hidport.h>

// TODO what does this do?  Does it get the next device object down the stack?  It is from the hidkmdf example.
#define GET_NEXT_DEVICE_OBJECT(DO) \
  (((PHID_DEVICE_EXTENSION)(DO)->DeviceExtension)->NextDeviceObject)

// TODO REMOVE if debug output is not desired
#define CHATPAD_MOUSE_DBG

// Support debug tracing.
#ifdef CHATPAD_MOUSE_DBG
// TODO add this above the other DbgPrint if desired for more information in DebugView.  Make sure to add a '\' at the end.
/*
   DbgPrint("CHATPAD!"__FUNCTION__ ": ");
*/
#define ChatpadTrace(_MSG_) \
{ \
   DbgPrint _MSG_; \
}
#else
#define ChatpadTrace(__MSG__) \
{ \
}
#endif

__drv_dispatchType_other
  DRIVER_DISPATCH ChatpadMousePassThrough;
__drv_dispatchType_other
  DRIVER_DISPATCH ChatpadMousePowerPassThrough;
DRIVER_UNLOAD DriverUnload;
DRIVER_ADD_DEVICE ChatpadMouseAddDevice;

DRIVER_INITIALIZE DriverEntry;

// Called when driver is first loaded.
NTSTATUS
DriverEntry(
  __in PDRIVER_OBJECT   driverObject,
  __in PUNICODE_STRING  registryPath)
{
   NTSTATUS                      retStatus = STATUS_SUCCESS;
   HID_MINIDRIVER_REGISTRATION   hidMinidriverRegistration;
   ULONG functionIndex = 0;

   ChatpadTrace(("XBox 360 Controller Chatpad Virtual Mouse Device by GAFBlizzard.\n"));
   ChatpadTrace(("Build metadata unavailable.\n"));

   // Initialize the dispatch table to pass through all the IRPs by default.
   for(functionIndex = 0; functionIndex <= IRP_MJ_MAXIMUM_FUNCTION; functionIndex++)
   {
      driverObject->MajorFunction[functionIndex] = ChatpadMousePassThrough;
   }

   // Set up special case for IRP_MJ_POWER.
   driverObject->MajorFunction[IRP_MJ_POWER] = ChatpadMousePowerPassThrough;

   // Set up add device and driver unload functions.
   driverObject->DriverExtension->AddDevice = ChatpadMouseAddDevice;
   driverObject->DriverUnload = DriverUnload;

   RtlZeroMemory(&hidMinidriverRegistration, sizeof(hidMinidriverRegistration));
   hidMinidriverRegistration.Revision              = HID_REVISION;
   hidMinidriverRegistration.DriverObject          = driverObject;
   hidMinidriverRegistration.RegistryPath          = registryPath;
   hidMinidriverRegistration.DeviceExtensionSize   = 0;
   hidMinidriverRegistration.DevicesArePolled      = FALSE;

   // Register with hidclass.
   retStatus = HidRegisterMinidriver(&hidMinidriverRegistration);
   if(STATUS_SUCCESS == retStatus)
   {
      ChatpadTrace(("HidRegisterMinidriver succeeded.\n"));
   }
   else
   {
      ChatpadTrace(("HidRegisterMinidriver failed:  0x%0x\n", retStatus));
   }

   return retStatus;
} // end DriverEntry

// Unloads the driver.
VOID
DriverUnload(
  __in PDRIVER_OBJECT   driverObject)
{
   ChatpadTrace(("Unloading chatpad mouse driver.\n"));

   return;
} // end DriverUnload

NTSTATUS
ChatpadMouseAddDevice(
  __in PDRIVER_OBJECT   driverObject,
  __in PDEVICE_OBJECT   functionalDeviceObject)
{
   // TODO what does this do?  This is from the hidkmdf example and presumably handles
   //   initialization in some fashion.
   functionalDeviceObject->Flags &= (~DO_DEVICE_INITIALIZING);

   ChatpadTrace(("ChatpadMouseAddDevice returning STATUS_SUCCESS.\n"));
   return STATUS_SUCCESS;
} // end ChatpadMouseAddDevice

// This function passes through non-power IRPs.
NTSTATUS
ChatpadMousePassThrough(
  __in PDEVICE_OBJECT   deviceObject,
  __in PIRP             irp)
{
   NTSTATUS             retStatus      = STATUS_SUCCESS;
   PIO_STACK_LOCATION   stackLocation  = NULL;

   stackLocation = IoGetCurrentIrpStackLocation(irp);
   if(NULL != stackLocation)
   {
//      ChatpadTrace(("Major function code %u, minor function code %u.\n",
//                    stackLocation->MajorFunction,
//                    stackLocation->MinorFunction));
   }
   else
   {
      ChatpadTrace(("Error, stack location pointer is NULL.\n"));
   }

   // Copy the stack to the next level.
   IoCopyCurrentIrpStackLocationToNext(irp);

   retStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(deviceObject), irp);
//   ChatpadTrace(("ChatpadMousePassThrough returning retStatus 0x%x\n", retStatus));

   return retStatus;
} // end ChatpadMousePassThrough

// This function contains special handling code for IRP_MJ_POWER.
NTSTATUS
ChatpadMousePowerPassThrough(
  __in PDEVICE_OBJECT   deviceObject,
  __in PIRP             irp)
{
   NTSTATUS retStatus = STATUS_SUCCESS;

   // TODO what does this do?  The hidkmdf example seems to say that we must start
   //   the next power IRP before skipping to the next stack location.
   PoStartNextPowerIrp(irp);

   // Copy the stack to the next level.
   IoCopyCurrentIrpStackLocationToNext(irp);
   retStatus = PoCallDriver(GET_NEXT_DEVICE_OBJECT(deviceObject), irp);

//   ChatpadTrace(("ChatpadMousePowerPassThrough returning retStatus 0x%x\n", retStatus));

   return retStatus;
} // end ChatpadMousePowerPassThrough

