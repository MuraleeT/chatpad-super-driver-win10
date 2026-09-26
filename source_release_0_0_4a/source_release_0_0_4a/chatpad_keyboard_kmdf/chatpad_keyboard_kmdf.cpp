//
// Chatpad virtual keyboard KMDF driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

// This driver was hand-written by following the hidkmdf example from the 7600.16385.1 WinDDK.

#include "chatpad_keyboard_kmdf.h"


//***
// Global variables.
//***

// Lock for synchronization.  This protects the below global variables.
// TODO do I need to worry about deleting this (and if so how), or is just letting the
//   system handle it upon driver unload okay?
WDFWAITLOCK                      chatpadKeyboardKMDFDeviceCollectionLock;

// How many handles are open on the device.  This value should never be greater than one.
ULONG                            numOpenHandles = 0;
// Handle to the control device.
WDFDEVICE                        chatpadKeyboardKMDFControlDevice = NULL;
// Whether the control device is available.  If the device is unplugged,
// the control device may remain in existence, but it will be marked
// as unavailable.
BOOLEAN                          chatpadKeyboardKMDFControlDeviceAvailable = FALSE;

// Collection of objects for non-control device instances.
WDFCOLLECTION                    chatpadKeyboardKMDFDeviceCollection;


//***
// HID descriptors
//***

// The Device Class Definition for Human Interface Devices (HID) document, version 1.11,
// and the HID Descriptor Tool (DT) were used to create the HID descriptors below.

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

// TODO do I need to concern myself with the boot protocol?
//   (see http://msdn.microsoft.com/en-us/library/ff543350%28v=VS.85%29.aspx)

// Make sure this structure is byte-aligned and not padded to match word boundaries.
#pragma pack(1)

// Note that this structure must exactly match the report described by the
// descriptor below.
typedef struct _CHATPAD_KEYBOARD_HID_INPUT_REPORT
{
   // TODO NEXT add reference to where modified byte is defined
   // TODO NEXT research how repeat works...seems to be automatic?
   BYTE     modifierByte;
   BYTE     reservedByte;
   // Note that LED data is not included here since the LED report is an output rather than an input.

   // USB key codes (see USB HID Usage Table specification, section 10) of keys
   // that are currently held down.
   BYTE     keyCodes[6];
} CHATPAD_KEYBOARD_HID_INPUT_REPORT, *PCHATPAD_KEYBOARD_HID_INPUT_REPORT;

#pragma pack()

// HID report descriptor (keyboard).
const HID_REPORT_DESCRIPTOR chatpadKeyboardHidReportDescriptor[] =
{
   0x05, 0x01,    // Usage page (generic desktop).
   0x09, 0x06,    // Usage (keyboard).
   0xA1, 0x01,    // Collection (application).

   0x05, 0x07,    // Usage page (key codes).
   0x19, 0xE0,    // Usage minimum (224).
   0x29, 0xE7,    // Usage maximum (231).
   0x15, 0x00,    // Logical minimum (0).
   0x25, 0x01,    // Logical maximum (1).
   0x75, 0x01,    // Report size (1).
   0x95, 0x08,    // Report count (8).
   0x81, 0x02,    // Input (data, variable, absolute):  Modifier byte.

   0x95, 0x01,    // Report count (1).
   0x75, 0x08,    // Report size (8).
//   0x81, 0x03,    // Input (constant, variable, absolute):  Reserved byte.
   0x81, 0x01,    // Input (constant):  Reserved byte.

// TODO can I omit the LED information since I do not need it?
   0x95, 0x05,    // Report count (5).
   0x75, 0x01,    // Report size (1).
   0x05, 0x08,    // Usage page (page# for LEDs).
   0x19, 0x01,    // Usage minimum (1).
   0x29, 0x05,    // Usage maximum (5).
   0x91, 0x02,    // Output (data, variable, absolute):  LED report.

   0x95, 0x01,    // Report count (1).
   0x75, 0x03,    // Report size (3).
//   0x91, 0x03,    // Output (constant, variable, absolute) LED report padding to fill up a byte.
   0x91, 0x01,    // Output (constant) LED report padding to fill up a byte.

   0x95, 0x06,    // Report count (6).
   0x75, 0x08,    // Report size (8).
   0x15, 0x00,    // Logical minimum (0).
//   0x25, 0x65,    // Logical maximum (101).
   0x25, 0xFF,    // Logical maximum (255).
   0x05, 0x07,    // Usage page (key codes).
   0x19, 0x00,    // Usage minimum (0).
//   0x29, 0x65,    // Usage maximum (101).
   0x29, 0xFF,    // Usage maximum (255).
   0x81, 0x00,    // Input (data, array):  Key array, 6 bytes.

   0xC0           // End collection
}; // end chatpadKeyboardHidReportDescriptor

// HID descriptor (keyboard).
CONST HID_DESCRIPTOR chatpadKeyboardHidDefaultDescriptor =
{
   0x09,    // Length of HID descriptor.
   0x21,    // Descriptor type (HID 0x21, assigned by USB?).
   0x0101,  // HID spec release (1.01).
   0x00,    // Country code (not specified).
   0x01,    // Number of HID class descriptors.
   { 0x22,                                         // Descriptor type (report descriptor).
     sizeof(chatpadKeyboardHidReportDescriptor) }  // Total length of report descriptor.
   // TODO verify that the report descriptor size is 0x3F
}; // end chatpadKeyboardHidDefaultDescriptor 

//***
// Function prototypes/forward declarations so that OACR/preFAST doesn't complain.
//***

NTSTATUS ChatpadKeyboardKMDFCreateControlDevice(WDFDEVICE device);
VOID ChatpadKeyboardKMDFDeleteControlDevice(VOID);

NTSTATUS ChatpadKeyboardKMDFGetHidDescriptor(WDFDEVICE device, WDFREQUEST request);
NTSTATUS ChatpadKeyboardKMDFGetHidDeviceAttributes(WDFREQUEST request);
NTSTATUS ChatpadKeyboardKMDFGetHidReportDescriptor(WDFDEVICE device, WDFREQUEST request);
NTSTATUS ChatpadKeyboardKMDFGetHidString(WDFREQUEST request);


EVT_WDF_DRIVER_UNLOAD                        DriverUnload;
EVT_WDF_DEVICE_PREPARE_HARDWARE              ChatpadKeyboardKMDFEvtPrepareHardware;
EVT_WDF_DEVICE_D0_ENTRY                      ChatpadKeyboardKMDFD0Entry;
EVT_WDF_DEVICE_D0_EXIT                       ChatpadKeyboardKMDFD0Exit;
EVT_WDF_DRIVER_DEVICE_ADD                    ChatpadKeyboardKMDFEvtDeviceAdd;
EVT_WDFDEVICE_WDM_IRP_PREPROCESS             ChatpadKeyboardKMDFEvtWdmPreprocessMnQueryId;
EVT_WDF_DEVICE_FILE_CREATE                   ChatpadKeyboardKMDFControlDeviceCreateHandler;
EVT_WDF_FILE_CLOSE                           ChatpadKeyboardKMDFControlDeviceCloseHandler;
EVT_WDF_OBJECT_CONTEXT_CLEANUP               ChatpadKeyboardKMDFDriverContextCleanup;
EVT_WDF_DEVICE_CONTEXT_CLEANUP               ChatpadKeyboardKMDFContextCleanup;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL  ChatpadKeyboardKMDFInternalIOCTLHandler;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL           ChatpadKeyboardKMDFIOCTLHandler;

extern "C"
DRIVER_INITIALIZE DriverEntry;


// Called when driver is first loaded.
extern "C"
NTSTATUS
DriverEntry(
  __in PDRIVER_OBJECT  driverObject,
  __in PUNICODE_STRING registryPath)
{
   NTSTATUS                retStatus = STATUS_SUCCESS;
   WDF_DRIVER_CONFIG       driverConfig;
   WDF_OBJECT_ATTRIBUTES   objectAttributes;

   ChatpadTrace(("XBox 360 Controller Chatpad Virtual Keyboard KMDF Layer by GAFBlizzard.\n"));
   ChatpadTrace(("Build metadata unavailable.\n"));

   // This is just an information printout so I can check the size of a DEVICE_CONTEXT structure
   // (only the structure itself, not any additional allocated memory).
   ChatpadTrace(("sizeof(DEVICE_CONTEXT) == %u\n", sizeof(DEVICE_CONTEXT)));
   ChatpadTrace(("sizeof(CHATPAD_KEYBOARD_HID_INPUT_REPORT)  == %u\n", sizeof(CHATPAD_KEYBOARD_HID_INPUT_REPORT)));
// TODO NEXT REMOVE BELOW
ChatpadTrace(("report descriptor size is %u\n", sizeof(chatpadKeyboardHidReportDescriptor))); 

   WDF_DRIVER_CONFIG_INIT(&driverConfig,
                          ChatpadKeyboardKMDFEvtDeviceAdd);

   // Set up the driver unload function.
   driverConfig.EvtDriverUnload = DriverUnload;

   // Register a cleanup callback.
   WDF_OBJECT_ATTRIBUTES_INIT(&objectAttributes);
   objectAttributes.EvtCleanupCallback = ChatpadKeyboardKMDFDriverContextCleanup;

   // Create framework driver object (FDO).
   retStatus = WdfDriverCreate(
     driverObject,
     registryPath,
     &objectAttributes,
     &driverConfig,
     WDF_NO_HANDLE);
   if(STATUS_SUCCESS != retStatus)
   {
      ChatpadTrace(("WdfDriverCreate failed:  0x%0x\n", retStatus));
   }

   // Create collection for different instances of the top-level (non-control) device.
   if(STATUS_SUCCESS == retStatus)
   {
      retStatus = WdfCollectionCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        &chatpadKeyboardKMDFDeviceCollection);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfCollectionCreate failed:  0x%x\n", retStatus));
      }
   }

   if(STATUS_SUCCESS == retStatus)
   {
      retStatus = WdfWaitLockCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        &chatpadKeyboardKMDFDeviceCollectionLock);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfWaitLockCreate failed:  0x%x\n", retStatus));
      }
   }

   return retStatus;
} // end DriverEntry

// Unloads the driver.
VOID
DriverUnload(
  IN WDFDRIVER driver)
{
   ChatpadTrace(("Unloading chatpad virtual keyboard KMDF layer.\n"));

   // Clean up any global data.
   // TODO add code here?
   return;
} // end DriverUnload

// TODO move this code?
NTSTATUS
ChatpadKeyboardKMDFEvtPrepareHardware(
  IN WDFDEVICE                   wdfDevice,
  IN WDFCMRESLIST                resourcesRaw,
  IN WDFCMRESLIST                resourcesTranslated)
{
   NTSTATUS                      retStatus = STATUS_SUCCESS;
   PDEVICE_CONTEXT               devContext = NULL;

   ChatpadTrace(("Prepare hardware callback called.\n"));

   devContext = ChatpadKeyboardKMDFGetDeviceContext(wdfDevice);
   if(NULL != devContext)
   {
      // TODO is there anything I need to do here?
   }
   else
   {
      retStatus = STATUS_INTERNAL_ERROR;
   }

   return retStatus;
} // end ChatpadKeyboardKMDFEvtPrepareHardware

// NOTE:  This function must not do anything that could cause a page fault, according to the osrusbfx2 example.
NTSTATUS
ChatpadKeyboardKMDFD0Entry(
  IN WDFDEVICE                wdfDevice,
  IN WDF_POWER_DEVICE_STATE   previousState)
{
   NTSTATUS          retStatus   = STATUS_SUCCESS;
   PDEVICE_CONTEXT   devContext  = NULL;

   devContext = ChatpadKeyboardKMDFGetDeviceContext(wdfDevice);
   if(NULL != devContext)
   {
      ChatpadTrace(("D0 entry occurred.\n"));

      WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);
      chatpadKeyboardKMDFControlDeviceAvailable = TRUE;
      WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);
   } // end if(NULL != devContext)
   else
   {
      ChatpadTrace(("Unable to obtain device context.\n"));
      retStatus = STATUS_INTERNAL_ERROR;
   }

   return retStatus;
} // end ChatpadKeyboardKMDFD0Entry

// NOTE:  This function must not do anything that could cause a page fault, according to the osrusbfx2 example.
NTSTATUS
ChatpadKeyboardKMDFD0Exit(
  WDFDEVICE                wdfDevice,
  WDF_POWER_DEVICE_STATE   targetState)
{
   NTSTATUS retStatus = STATUS_SUCCESS;
   PDEVICE_CONTEXT devContext  = NULL;

   devContext = ChatpadKeyboardKMDFGetDeviceContext(wdfDevice);
   if(NULL != devContext)
   {
      ChatpadTrace(("D0 exit occurred.\n"));
   }
   else
   {
      retStatus = STATUS_INTERNAL_ERROR;
   }

   return retStatus;
} // end ChatpadKeyboardKMDFD0Exit

NTSTATUS
ChatpadKeyboardKMDFEvtDeviceAdd(
  IN WDFDRIVER       driver,
  IN PWDFDEVICE_INIT deviceInit)
{
   NTSTATUS                      retStatus = STATUS_SUCCESS;
   WDF_IO_QUEUE_CONFIG           ioQueueConfig;
   WDF_OBJECT_ATTRIBUTES         objectAttributes;
   WDFDEVICE                     wdfDevice = NULL;
   PDEVICE_CONTEXT               devContext = NULL;
   WDFQUEUE                      queue;
   WDF_PNPPOWER_EVENT_CALLBACKS  powerCallbacks;
   UCHAR                         minorFunction;

   if(STATUS_SUCCESS == retStatus)
   {
      // Mark ourselves as a filter, causing requests we don't care about to be
      // passed along etc.
      WdfFdoInitSetFilter(deviceInit);

      // Mark our device type as unknown.
      // TODO this was from another example, I think, but the hidusbfx2 example does not have it.
      //   Is there something I should understand about what it does and whether I need it?
      // TODO should this be set eventually since this driver is not generic, or is the
      //      automatic stuff fine?
//      WdfDeviceInitSetDeviceType(deviceInit, FILE_DEVICE_UNKNOWN);

      // Override IRP_MN_QUERY_ID, like vmulti does.
      // TODO is overriding this IRP necessary?  It may only be necessary for multifunction drivers.
      minorFunction = IRP_MN_QUERY_ID;
      retStatus = WdfDeviceInitAssignWdmIrpPreprocessCallback(
        deviceInit,
        ChatpadKeyboardKMDFEvtWdmPreprocessMnQueryId,
        IRP_MJ_PNP,        // Major function.
        &minorFunction,    // Array of minor functions.
        1);                // Number of minor functions.
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfDeviceInitAssignWdmIrpPreprocessCallback failed:  0x%x\n", retStatus));
      }
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // Register a prepare hardware callback to create a WDF USB device object.
      WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&powerCallbacks);
      powerCallbacks.EvtDevicePrepareHardware = ChatpadKeyboardKMDFEvtPrepareHardware;
      powerCallbacks.EvtDeviceD0Entry = ChatpadKeyboardKMDFD0Entry;
      powerCallbacks.EvtDeviceD0Exit = ChatpadKeyboardKMDFD0Exit;

      WdfDeviceInitSetPnpPowerEventCallbacks(deviceInit, &powerCallbacks);

      WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&objectAttributes, DEVICE_CONTEXT);

      // Register for cleanup notification.
      objectAttributes.EvtCleanupCallback = ChatpadKeyboardKMDFContextCleanup;

      // Create framework device object, attach to the lower stack etc.
      retStatus = WdfDeviceCreate(&deviceInit, &objectAttributes, &wdfDevice);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfDeviceCreate failed:  0x%x\n", retStatus));
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      devContext = ChatpadKeyboardKMDFGetDeviceContext(wdfDevice);
      if(NULL != devContext)
      {
         // Initialize DEVICE_CONTEXT values.
         ChatpadTrace(("Initializing device context.\n"));

         // TODO is it possible for a driver to read from the registry and detect whether to start up enabled?
         //   (apparently so, see http://flylib.com/books/en/3.141.1.102/1/)
         devContext->keyboardEnabled = FALSE;

         if(STATUS_SUCCESS == retStatus)
         {
            // Initialize default queue config structure.
            WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchParallel);

            // Select what we want from the queue.
            // This should catch IOCTL_INTERNAL_USB_SUBMIT_URB.
            ioQueueConfig.EvtIoInternalDeviceControl =
              ChatpadKeyboardKMDFInternalIOCTLHandler;

            // Create the default queue.
            retStatus = WdfIoQueueCreate(
              wdfDevice,
              &ioQueueConfig,
              WDF_NO_OBJECT_ATTRIBUTES,
              NULL);
            if(STATUS_SUCCESS == retStatus)
            {
               ChatpadTrace(("Created chatpad keyboard KMDF layer default queue successfully.\n"));
            }
            else
            {
               ChatpadTrace(("WdfIoQueueCreate failed:  0x%x\n", retStatus));
            }
         } // end if(STATUS_SUCCESS == retStatus)

         if(STATUS_SUCCESS == retStatus)
         {
            // Create manual I/O queue for handling HID read requests from
            // above us.
            WDF_IO_QUEUE_CONFIG hidReadRequestQueueConfig;

            WDF_IO_QUEUE_CONFIG_INIT(&hidReadRequestQueueConfig, WdfIoQueueDispatchManual);
            hidReadRequestQueueConfig.PowerManaged = WdfFalse;

            retStatus =
              WdfIoQueueCreate(
                wdfDevice,
                &hidReadRequestQueueConfig,
                WDF_NO_OBJECT_ATTRIBUTES,
                &(devContext->hidReadRequestQueue));
            if(STATUS_SUCCESS == retStatus)
            {
               ChatpadTrace(("Created HID read request queue successfully.\n"));
            }
            else
            {
               ChatpadTrace(("WdfIoQueueCreate failed:  0x%x\n", retStatus));
            }
         } // end if(STATUS_SUCCESS == retStatus)
      } // end if(NULL != devContext)
      else
      {
         retStatus = STATUS_INTERNAL_ERROR;
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      // Add the device to the collection.
      WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);

      retStatus = WdfCollectionAdd(
        chatpadKeyboardKMDFDeviceCollection,
        wdfDevice);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfCollectionAdd failed:  0x%x\n", retStatus));
      }

      WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);
   }

   // Create the control device.
   if(STATUS_SUCCESS == retStatus)
   {
      retStatus = ChatpadKeyboardKMDFCreateControlDevice(wdfDevice);
      // ChatpadKeyboardKMDFCreateControlDevice should print an error if it fails.
   }

   return retStatus;
} // end ChatpadKeyboardKMDFEvtDeviceAdd

// This is just a wrapper for ExAllocatePoolWithTag, since OACR refuses to stop complaining
// about the memory possibly being leaked otherwise, and I do not know if there
// is a way to add the __drv_aliasesMem thing without a wrapper function.
__drv_aliasesMem
PVOID
ExAllocatePoolWithTagWrapper(
  __in __drv_strictTypeMatch(__drv_typeExpr) POOL_TYPE PoolType,
  __in SIZE_T NumberOfBytes,
  __in ULONG Tag)
{
   UNREFERENCED_PARAMETER(PoolType);
   return ExAllocatePool2(POOL_FLAG_NON_PAGED, NumberOfBytes, Tag);
} // end ExAllocatePoolWithTagWrapper

// This code is hand-written but is patterned after VMultiEvtWdmPreprocessMnQueryID
// from vmulti (http://code.google.com/p/vmulti/).
NTSTATUS
ChatpadKeyboardKMDFEvtWdmPreprocessMnQueryId(
  WDFDEVICE       device,
  PIRP            irp)
{
   NTSTATUS             retStatus               = STATUS_SUCCESS;
   PIO_STACK_LOCATION   currentStackLocation    = NULL;
   PIO_STACK_LOCATION   previousStackLocation   = NULL;
   PDEVICE_OBJECT       deviceObject            = NULL;
   PWCHAR               buffer                  = NULL;

   currentStackLocation = IoGetCurrentIrpStackLocation(irp);
   deviceObject = WdfDeviceWdmGetDeviceObject(device);

   // According to vmulti, this check is required to filter out QUERY_IDs
   // forwarded by the HIDCLASS for the parent framework device object.
   // These IDs, says vmulti, are sent by the PNP manager for the parent
   // framework device object if you root-enumerate this driver.
   // TODO study the documentation to fully understand how all this works!
   // TODO also study how stack locations work to fully understand the pointer arithmetic.
   previousStackLocation = IoGetNextIrpStackLocation(irp);
   if(NULL == previousStackLocation)
   {
      ChatpadTrace(("Error trying to get previous stack pointer.\n"));
      retStatus = STATUS_INTERNAL_ERROR;
   }

   if(STATUS_SUCCESS == retStatus)
   {
      if(previousStackLocation->DeviceObject == deviceObject)
      {
         // The vmulti code says that filtering out this basically prevents the
         // found new hardware popup for a root-enumerated device (on reboot
         // only?).
         // TODO study how this works
         retStatus = irp->IoStatus.Status;
      }
      else
      {
         if((BusQueryDeviceID == currentStackLocation->Parameters.QueryId.IdType) ||
            (BusQueryHardwareIDs == currentStackLocation->Parameters.QueryId.IdType))
         {
            // In this case, according to vmulti, HIDClass is asking for a child
            // device ID and hardware IDs.
            // TODO study how this works regarding the device/hardware IDs

            // TODO NEXT do we ever need to free this, or is that handled
            //   automatically by the system, or when the driver is unloaded?
            buffer = static_cast<PWCHAR>(ExAllocatePoolWithTagWrapper(
              NonPagedPool,
              CHATPADKEYBOARDKMDF_HARDWARE_IDS_LENGTH,
              CHATPADKEYBOARDKMDF_POOL_TAG));

            if(NULL != buffer)
            {
               RtlCopyMemory(
                 buffer,
                 CHATPADKEYBOARDKMDF_HARDWARE_IDS,
                 CHATPADKEYBOARDKMDF_HARDWARE_IDS_LENGTH);
               // TODO why do we have to cast to a ULONG_PTR?  And why ULONG_PTR, not PULONG?
               irp->IoStatus.Information = reinterpret_cast<ULONG_PTR>(buffer);
            }
            else
            {
               ChatpadTrace(("Failed to allocate memory for hardware IDs string.\n"));
               retStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            irp->IoStatus.Status = retStatus;
         }
         else
         {
            retStatus = irp->IoStatus.Status;
         }
      } // end case where previous stack location's device object is not the current device object

      // TODO NEXT WIP is this invalid?  See http://msdn.microsoft.com/en-us/library/ff560205%28v=VS.85%29.aspx, I/O error 0x22E
      IoCompleteRequest(irp, IO_NO_INCREMENT);
   } // end if(STATUS_SUCCESS == retStatus)

   ChatpadTrace(("ChatpadKeyboardKMDFEvtWdmPreprocessMnQueryId returning status 0x%x\n", retStatus));

   return retStatus;
} // end ChatpadKeyboardKMDFEvtWdmPreprocessMnQueryId

VOID
ChatpadKeyboardKMDFControlDeviceCreateHandler(
  WDFDEVICE       device,
  WDFREQUEST      request,
  WDFFILEOBJECT   fileObject)
{
   NTSTATUS apiStatus = STATUS_SUCCESS;

   // Increment the open count.
   WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);

   // Since I have a wait lock, can I safely just modify it like this?
   if(numOpenHandles < 1)
   {
      numOpenHandles++;
   }
   else
   {
      ChatpadTrace(("Error, there was already an open handle to the control device.\n"));
      apiStatus = STATUS_INTERNAL_ERROR;
   }

   ChatpadTrace(("There are now %lu open handles to the chatpad keyboard KMDF control device.\n", numOpenHandles));

   if(STATUS_SUCCESS == apiStatus)
   {
      // TODO there is probably a cleaner way to do this than having one extension
      //   have a pointer to the other one etc...maybe?
      PCHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION controlExtension  = NULL;
      PDEVICE_CONTEXT                                 devContext        = NULL;

      if(STATUS_SUCCESS == apiStatus)
      {
         controlExtension = ChatpadKeyboardKMDFControlGetData(chatpadKeyboardKMDFControlDevice);

         if(NULL == controlExtension)
         {
            ChatpadTrace(("Failed to get control device extension in open handler.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         devContext = controlExtension->keyboardDeviceContext;
         if(NULL == devContext)
         {
            ChatpadTrace(("Failed to get device context in open handler.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         // Mark the virtual keyboard as enabled when the application connects.
         // TODO could messing with the device context here cause synchronization issues?
         devContext->keyboardEnabled = TRUE;
         ChatpadTrace(("Virtual keyboard enabled on application connect.\n"));
      }
   }

   WdfRequestComplete(request, apiStatus);

   WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);
} // end ChatpadKeyboardKMDFControlDeviceCreateHandler

VOID
ChatpadKeyboardKMDFControlDeviceCloseHandler(
  WDFFILEOBJECT   fileObject)
{
   NTSTATUS apiStatus = STATUS_SUCCESS;

   ChatpadTrace(("The IRQL in the keyboard close handler is %u.\n", KeGetCurrentIrql()));

   // Decrement the open count.
   WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);

   // Since I have a wait lock, can I safely just modify it like this?
   if(numOpenHandles > 0)
   {
      numOpenHandles--;
   }
   else
   {
      ChatpadTrace(("Error, there are supposedly 0 open handles but the close handler was called...\n"));
   }

   ChatpadTrace(("There are now %lu open handles to the chatpad keyboard KMDF control device.\n", numOpenHandles));

   if(0 == numOpenHandles)
   {
      // TODO there is probably a cleaner way to do this than having one extension
      //   have a pointer to the other one etc...maybe?
      //   I want to make sure the no-keys-pressed HID response gets sent, though.
      PCHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION controlExtension  = NULL;
      PDEVICE_CONTEXT                                 devContext        = NULL;
      WDFREQUEST                                      requestFromAbove = NULL;

      if(NULL == chatpadKeyboardKMDFControlDevice)
      {
         ChatpadTrace(("Error, control device handle is NULL.\n"));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         controlExtension = ChatpadKeyboardKMDFControlGetData(chatpadKeyboardKMDFControlDevice);

         if(NULL == controlExtension)
         {
            ChatpadTrace(("Failed to get control device extension in close handler.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         devContext = controlExtension->keyboardDeviceContext;
         if(NULL == devContext)
         {
            ChatpadTrace(("Failed to get device context in close handler.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         // Mark the virtual keyboard as disabled when the application disconnects.
         // TODO could messing with the device context here cause synchronization issues?
         devContext->keyboardEnabled = FALSE;
         ChatpadTrace(("Virtual keyboard disabled on application disconnect.\n"));

         apiStatus = WdfIoQueueRetrieveNextRequest(
           devContext->hidReadRequestQueue,
           &requestFromAbove);
         if(STATUS_SUCCESS != apiStatus)
         {
            ChatpadTrace(("No top-level HID read report request available.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         PCHATPAD_KEYBOARD_HID_INPUT_REPORT  inputReport                   = NULL;
         ULONG                               bytesTransferred              = 0;
         size_t                              bytesToCopy                   = 0;
         size_t                              requestFromAboveBufferLength  = 0;

         bytesToCopy = sizeof(CHATPAD_KEYBOARD_HID_INPUT_REPORT);
         apiStatus = WdfRequestRetrieveOutputBuffer(
           requestFromAbove,
           bytesToCopy,
           reinterpret_cast<PVOID*>(&inputReport),
           &requestFromAboveBufferLength);
         if((STATUS_SUCCESS == apiStatus)                   &&
            (NULL != inputReport)                           &&
            (requestFromAboveBufferLength >= bytesToCopy))
         {
            // Fill in the report fields to indicate that no keys are pressed, so the virtual keyboard
            // does not end up in a state where keys are repeating when the control application disconnects.
            inputReport->modifierByte  = 0x00;
            inputReport->reservedByte  = 0x00;
            inputReport->keyCodes[0]   = 0x00;
            inputReport->keyCodes[1]   = 0x00;
            inputReport->keyCodes[2]   = 0x00;
            inputReport->keyCodes[3]   = 0x00;
            inputReport->keyCodes[4]   = 0x00;
            inputReport->keyCodes[5]   = 0x00;

            // Indicate to the requester how many bytes were transferred.
            bytesTransferred = bytesToCopy;
         }
         else
         {
            ChatpadTrace(("Unable to get valid buffer from top-level HID read report request:  0x%x\n",
                          apiStatus));
            apiStatus = STATUS_INTERNAL_ERROR;
         }

         // Return the report to the requester.
         WdfRequestCompleteWithInformation(requestFromAbove, apiStatus, bytesTransferred);

/*
         if(STATUS_SUCCESS == apiStatus)
         {
            ChatpadTrace(("Successfully sent HID response indicating all keys are released.\n"));
         }
*/
      } // end if(STATUS_SUCCESS == apiStatus)
   } // end if(0 == numOpenHandles)

   WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);
} // end ChatpadKeyboardKMDFControlDeviceCloseHandler

VOID
ChatpadKeyboardKMDFDriverContextCleanup(
  IN WDFOBJECT       object)
{
   NTSTATUS apiStatus = STATUS_SUCCESS;

   ChatpadTrace(("Driver context cleanup handler called.\n"));
   // TODO do I need to do anything here?  Right now I am letting the other context cleanup callback handle things.
} // end ChatpadKeyboardKMDFDriverContextCleanup

VOID
ChatpadKeyboardKMDFContextCleanup(
  IN WDFOBJECT       object)
{
   NTSTATUS apiStatus = STATUS_SUCCESS;
   // TODO is static_cast or reinterpret_cast preferred here?  I'm doing an
   //      oldstyle cast because that's what the suggestion on osronline
   //      said to do.  :)
   //      Normally, for void* to something* I'd do static_cast, and from
   //      thingA* to thingB* I'd do reinterpret_cast.
   WDFDEVICE device = (WDFDEVICE) object;
   PDEVICE_CONTEXT devContext = NULL;
   ULONG deviceCount;

   ChatpadTrace(("Normal context cleanup handler called.\n"));

   devContext = ChatpadKeyboardKMDFGetDeviceContext(device);
   if((NULL != device) && (NULL != devContext))
   {
      WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);

      // Clean up device context resources.

      deviceCount = WdfCollectionGetCount(chatpadKeyboardKMDFDeviceCollection);
      if(deviceCount == 1)
      {
         // If we are the last instance, then delete the control device so that
         // the driver can unload.  Apparently there was a bug in WDF 1.0, but 
         // apparently this sort of thing is fine in WDF 1.9 (and even earlier?).
         //
         // According to the toaster KMDF sideband filter example, the deletion
         // below has to be done while the lock on the collection is acquired
         // since it protects the global device variable, and another thread
         // cannot try to create while we are deleting the device.

         if(0 == numOpenHandles)
         {
            ChatpadKeyboardKMDFDeleteControlDevice();
         }
         else
         {
            ChatpadTrace(("The device was not deleted since there are still %lu open handles to it.\n",
                          numOpenHandles));
         }
      } // end if(deviceCount == 1)

      // Even if the control device was not deleted, however, we mark it as unavailable.
      chatpadKeyboardKMDFControlDeviceAvailable = FALSE;

      WdfCollectionRemove(chatpadKeyboardKMDFDeviceCollection, device);

      WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);
   } // end if((NULL != device) && (NULL != devContext))
   else
   {
      // This should never happen.
      ChatpadTrace(("Error, device and/or devContext are NULL while cleaning up.\n"));
   }
}

// Creates a control device object so that an application can talk to the filter driver.
NTSTATUS
ChatpadKeyboardKMDFCreateControlDevice(
  WDFDEVICE device)
{
   NTSTATUS                retStatus = STATUS_SUCCESS;
   BOOLEAN                 createDevice = FALSE;
   PWDFDEVICE_INIT         deviceInit = NULL;
   WDF_FILEOBJECT_CONFIG   fileObjectConfig;
   WDFDEVICE               newChatpadKeyboardKMDFControlDevice = NULL;
   WDF_OBJECT_ATTRIBUTES   controlAttributes;
   DECLARE_CONST_UNICODE_STRING(ntDeviceName, CHATPAD_KEYBOARD_KMDF_CONTROL_NAME);
   DECLARE_CONST_UNICODE_STRING(symbolicLinkName, CHATPAD_KEYBOARD_KMDF_CONTROL_LINK);
   WDFQUEUE                newQueue;
   WDF_IO_QUEUE_CONFIG     ioQueueConfig;

   // Determine whether we should create the device, or if it already exists.
   WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);

   // TODO REMOVE after testing
   ChatpadTrace(("Collection device count is %d.\n", WdfCollectionGetCount(chatpadKeyboardKMDFDeviceCollection)));
   // TODO is this a safe way to synchronize things?
   if(NULL == chatpadKeyboardKMDFControlDevice)
   {
      createDevice = TRUE;
   }

   WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);

   if(!createDevice)
   {
      ChatpadTrace(("Chatpad keyboard KMDF control device was already created.\n"));
   }
   else
   {
      ChatpadTrace(("Creating chatpad keyboard KMDF control device.\n"));

      // Allocate and initialize a WDFDEVICE_INIT structure for the control device.
      deviceInit = WdfControlDeviceInitAllocate(
        WdfDeviceGetDriver(device),
        // Permissions:  Allow kernel, system, and admin control over the device,
        //   but do not allow any other users to access it.
        // TODO does this work with XP?
        &SDDL_DEVOBJ_SYS_ALL_ADM_ALL);
      if(NULL == deviceInit)
      {
         retStatus = STATUS_INSUFFICIENT_RESOURCES;
      }

      if(STATUS_SUCCESS == retStatus)
      {
         // Make the control device exclusive.  I only want one application talking to it at a time.
         WdfDeviceInitSetExclusive(deviceInit, TRUE);
         // Get device name.
         retStatus = WdfDeviceInitAssignName(deviceInit, &ntDeviceName);
      }

      // Set up callbacks so we can keep track of whether user mode has the
      // control device open.
      WDF_FILEOBJECT_CONFIG_INIT(
        &fileObjectConfig,
        ChatpadKeyboardKMDFControlDeviceCreateHandler,
        ChatpadKeyboardKMDFControlDeviceCloseHandler,
        WDF_NO_EVENT_CALLBACK);

      WdfDeviceInitSetFileObjectConfig(
        deviceInit,
        &fileObjectConfig,
        WDF_NO_OBJECT_ATTRIBUTES);

      if(STATUS_SUCCESS == retStatus)
      {
         WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
           &controlAttributes,
           CHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION);
         // Create the control device.
         retStatus = WdfDeviceCreate(
           &deviceInit,
           &controlAttributes,
           &newChatpadKeyboardKMDFControlDevice);
      }

      // Initialize control extension.
      PDEVICE_CONTEXT devContext = ChatpadKeyboardKMDFGetDeviceContext(device);
      if(NULL != devContext)
      {
         PCHATPAD_KEYBOARD_KMDF_CONTROL_DEVICE_EXTENSION controlExtension =
           ChatpadKeyboardKMDFControlGetData(newChatpadKeyboardKMDFControlDevice);

         if(NULL != controlExtension)
         {
            ChatpadTrace(("Initializing control device extension.\n"));
            controlExtension->keyboardDeviceContext = devContext;
         }
         else
         {
            ChatpadTrace(("Error, could not initialize control device extension.\n"));
            retStatus = STATUS_INTERNAL_ERROR;
         }
      }
      else
      {
         retStatus = STATUS_INTERNAL_ERROR;
      }

      if(STATUS_SUCCESS == retStatus)
      {
         // Create symbolic link so user mode can open the device.
         retStatus = WdfDeviceCreateSymbolicLink(
           newChatpadKeyboardKMDFControlDevice,
           &symbolicLinkName);
      }

      if(STATUS_SUCCESS == retStatus)
      {
         // Configure default queue so that requests to the control device will be parallel.
         WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
           &ioQueueConfig,
//           WdfIoQueueDispatchSequential);
           WdfIoQueueDispatchParallel);

         // Select what we want from the queue.

         // This should catch application-command IOCTLs.
         ioQueueConfig.EvtIoDeviceControl =
           ChatpadKeyboardKMDFIOCTLHandler;

         // Create the queue.
         retStatus = WdfIoQueueCreate(
           newChatpadKeyboardKMDFControlDevice,
           &ioQueueConfig,
           WDF_NO_OBJECT_ATTRIBUTES, // TODO what does this parameter do?
           &newQueue);
      } // end if(STATUS_SUCCESS == retStatus)

      if(STATUS_SUCCESS == retStatus)
      {
         WdfControlFinishInitializing(newChatpadKeyboardKMDFControlDevice);
         WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);
         chatpadKeyboardKMDFControlDevice = newChatpadKeyboardKMDFControlDevice;
         WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);

         ChatpadTrace(("Chatpad keyboard KMDF control device created successfully.\n"));
      }
      else
      {
         ChatpadTrace(("WdfIoQueueCreate failed:  0x%x\n", retStatus));
      }
   } // end if creating the control device

   // Handle error cases.
   if(STATUS_SUCCESS != retStatus)
   {
      ChatpadTrace(("Failed to create chatpad keyboard KMDF control device:  0x%x\n", retStatus));

      if(NULL != deviceInit)
      {
         WdfDeviceInitFree(deviceInit);
         deviceInit = NULL;
      }

      if(NULL != newChatpadKeyboardKMDFControlDevice)
      {
         // Release the newly created, but uninitialized, object.
         WdfObjectDelete(newChatpadKeyboardKMDFControlDevice);
         newChatpadKeyboardKMDFControlDevice = NULL;
      }
   }

   return retStatus;
} // end ChatpadKeyboardKMDFCreateControlDevice

// Deletes the control device object.
//
// IMPORTANT NOTE:  This device should only be called if a wait lock is
// currently acquired on chatpadKeyboardKMDFDeviceCollectionLock.
VOID
ChatpadKeyboardKMDFDeleteControlDevice(VOID)
{
   ChatpadTrace(("Deleting chatpad control device.\n"));

   if(chatpadKeyboardKMDFControlDevice)
   {
      WdfObjectDelete(chatpadKeyboardKMDFControlDevice);
      chatpadKeyboardKMDFControlDevice = NULL;
   }
} // end ChatpadKeyboardKMDFDeleteControlDevice

VOID
ChatpadKeyboardKMDFInternalIOCTLHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       outputBufferLength,
  IN size_t       inputBufferLength,
  IN ULONG        ioControlCode)
{
   NTSTATUS                   apiStatus               = STATUS_SUCCESS;
   WDFDEVICE                  wdfDevice;
   PDEVICE_CONTEXT            devContext              = NULL;
   // Whether this handler should complete the request.
   BOOLEAN                    completeRequestNow      = TRUE;

//   ChatpadTrace(("Chatpad keyboard KMDF layer internal IOCTL handler called with ioControlCode == %x\n",
//                 ioControlCode));

   wdfDevice = WdfIoQueueGetDevice(queue);

   // Get device context so we know where to forward the request.
   devContext = ChatpadKeyboardKMDFGetDeviceContext(wdfDevice);

// TODO what control codes do I need to support for a keyboard?
   if(IOCTL_HID_GET_DEVICE_DESCRIPTOR == ioControlCode)
   {
      apiStatus = ChatpadKeyboardKMDFGetHidDescriptor(wdfDevice, request);
   }
   else if(IOCTL_HID_GET_DEVICE_ATTRIBUTES == ioControlCode)
   {
// TODO NEXT do I need to support this?  If so I guess I have to provide placeholder vendor/product IDs.
      apiStatus = ChatpadKeyboardKMDFGetHidDeviceAttributes(request);
   }
   else if(IOCTL_HID_GET_REPORT_DESCRIPTOR == ioControlCode)
   {
      apiStatus = ChatpadKeyboardKMDFGetHidReportDescriptor(wdfDevice, request);
   }
   else if(IOCTL_HID_GET_STRING == ioControlCode)
   {
      apiStatus = ChatpadKeyboardKMDFGetHidString(request);
   }
   else if(IOCTL_HID_SET_FEATURE == ioControlCode)
   {
ChatpadTrace(("Received HID set feature request.\n"));
// TODO NEXT do I need to support this request?
apiStatus = STATUS_NOT_SUPPORTED; // TODO NEXT REMOVE
   }
   else if(IOCTL_HID_GET_FEATURE == ioControlCode)
   {
ChatpadTrace(("Received HID get feature request.\n"));
// TODO NEXT do I need to support this request?
apiStatus = STATUS_NOT_SUPPORTED; // TODO NEXT REMOVE
   }
   else if(IOCTL_HID_WRITE_REPORT == ioControlCode)
   {
ChatpadTrace(("Received HID write report request.\n"));
// TODO NEXT do I need to support this request?  Apparently so, if I want to track the system caps lock state.
apiStatus = STATUS_NOT_SUPPORTED; // TODO NEXT REMOVE
   }
   else if(IOCTL_HID_READ_REPORT == ioControlCode)
   {
      // Queue the request to a manual queue so that it can be handled when we
      // receive data from the control device.
      // TODO NEXT do I need to handle IOCTL_HID_GET_INPUT_REPORT as well?
      //   If only handling IOCTL_HID_READ_REPORT works on XP, Vista, and Windows 7, then apparently I don't.
      apiStatus = WdfRequestForwardToIoQueue(request, devContext->hidReadRequestQueue);
      if(STATUS_SUCCESS == apiStatus)
      {
         completeRequestNow = FALSE;
      }
      else
      {
         ChatpadTrace(("WdfRequestForwardToIoQueue failed:  0x%x\n", apiStatus));
      }
   }
   else
   {
      ChatpadTrace(("Unsupported internal IOCTL code 0x%x.\n", ioControlCode));
      apiStatus = STATUS_NOT_SUPPORTED;
   }

   if(TRUE == completeRequestNow)
   {
      WdfRequestComplete(request, apiStatus);
   }

   return;
} // end ChatpadKeyboardKMDFInternalIOCTLHandler

// This function retrieves the keyboard HID descriptor.
NTSTATUS
ChatpadKeyboardKMDFGetHidDescriptor(
  IN WDFDEVICE    device,
  IN WDFREQUEST   request)
{
   NTSTATUS    retStatus      = STATUS_SUCCESS;
   size_t      bytesToCopy    = 0;
   WDFMEMORY   outputMemory;

   ChatpadTrace(("Received HID get \"device\" descriptor request.\n"));

   retStatus = WdfRequestRetrieveOutputMemory(request, &outputMemory);
   if(STATUS_SUCCESS != retStatus)
   {
      ChatpadTrace(("WdfRequestRetrieveOutputMemory failed:  0x%x\n", retStatus));
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // Use hardcoded HID descriptor.
      // TODO is it okay to use this descriptor even though the thing we are handling is a
      //   request for a "device HID descriptor", and according to the HID spec that is
      //   a different descriptor?
      bytesToCopy = chatpadKeyboardHidDefaultDescriptor.bLength;

      if(0 == bytesToCopy)
      {
         ChatpadTrace(("Error, invalid internal HID descriptor.\n"));
         retStatus = STATUS_INVALID_DEVICE_STATE;
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      retStatus = WdfMemoryCopyFromBuffer(
        outputMemory,
        0,
        const_cast<HID_DESCRIPTOR*>(&chatpadKeyboardHidDefaultDescriptor),
        bytesToCopy);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfMemoryCopyFromBuffer failed:  0x%x\n", retStatus));
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      // Report how many bytes were copied.
      ChatpadTrace(("Reporting %lu bytes copied.\n", bytesToCopy));
      WdfRequestSetInformation(request, bytesToCopy);
   } // end if(STATUS_SUCCESS == retStatus)

   return retStatus;
} // end ChatpadKeyboardKMDFGetHidDescriptor

// This function retrieves device attributes.
NTSTATUS
ChatpadKeyboardKMDFGetHidDeviceAttributes(
  IN WDFREQUEST   request)
{
   NTSTATUS                retStatus               = STATUS_SUCCESS;
   PHID_DEVICE_ATTRIBUTES  deviceAttributes        = NULL;
   PVOID                   deviceAttributesAsVoid  = NULL;
   PDEVICE_CONTEXT         devContext              = NULL;

   devContext = ChatpadKeyboardKMDFGetDeviceContext(WdfIoQueueGetDevice(WdfRequestGetIoQueue(request)));

   ChatpadTrace(("Received HID get device attributes request.\n"));

   // Get a pointer to where we can write the device attributes.
   retStatus = WdfRequestRetrieveOutputBuffer(
     request,
     sizeof(HID_DEVICE_ATTRIBUTES),
     &deviceAttributesAsVoid,
     NULL);
   if(STATUS_SUCCESS != retStatus)
   {
      ChatpadTrace(("WdfRequestRetrieveOutputBuffer failed:  0x%x\n", retStatus));
   }

   if(STATUS_SUCCESS == retStatus)
   {
      deviceAttributes = static_cast<PHID_DEVICE_ATTRIBUTES>(deviceAttributesAsVoid);
      if(NULL != deviceAttributes)
      {
         // Use hardcoded values for vendor ID, product ID, and version number.
         // TODO are these reasonable, safe values?
         // TODO NEXT convert into constants and put in a header file somewhere.
         deviceAttributes->Size           = sizeof(HID_DEVICE_ATTRIBUTES);
         deviceAttributes->VendorID       = 0x00FE;
         deviceAttributes->ProductID      = 0x00ED;
         deviceAttributes->VersionNumber  = 0x0001;
      }
      else
      {
         ChatpadTrace(("Error, invalid device attributes pointer.\n"));
         retStatus = STATUS_INTERNAL_ERROR;
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      // Report size of structure.
      ChatpadTrace(("Reporting %lu bytes.\n", sizeof(HID_DEVICE_ATTRIBUTES)));
      WdfRequestSetInformation(request, sizeof(HID_DEVICE_ATTRIBUTES));
   } // end if(STATUS_SUCCESS == retStatus)

   return retStatus;
} // end ChatpadKeyboardKMDFGetHidDeviceAttributes

// This function retrieves the keyboard HID descriptor.
NTSTATUS
ChatpadKeyboardKMDFGetHidReportDescriptor(
  IN WDFDEVICE    device,
  IN WDFREQUEST   request)
{
   NTSTATUS    retStatus      = STATUS_SUCCESS;
   size_t      bytesToCopy    = 0;
   WDFMEMORY   outputMemory;

//   ChatpadTrace(("Received HID get report descriptor request.\n"));

   retStatus = WdfRequestRetrieveOutputMemory(request, &outputMemory);
   if(STATUS_SUCCESS != retStatus)
   {
      ChatpadTrace(("WdfRequestRetrieveOutputMemory failed:  0x%x\n", retStatus));
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // Use hardcoded HID descriptor.
      // TODO is it okay to use this descriptor even though the thing we are handling is a
      //   request for a "device HID descriptor", and according to the HID spec that is
      //   a different descriptor?
      bytesToCopy = chatpadKeyboardHidDefaultDescriptor.DescriptorList[0].wReportLength;

      if(0 == bytesToCopy)
      {
         ChatpadTrace(("Error, invalid internal HID report descriptor.\n"));
         retStatus = STATUS_INVALID_DEVICE_STATE;
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      retStatus = WdfMemoryCopyFromBuffer(
        outputMemory,
        0,
        const_cast<PHID_REPORT_DESCRIPTOR>(chatpadKeyboardHidReportDescriptor),
        bytesToCopy);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfMemoryCopyFromBuffer failed:  0x%x\n", retStatus));
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      // Report how many bytes were copied.
//      ChatpadTrace(("Reporting %lu bytes copied.\n", bytesToCopy));
      WdfRequestSetInformation(request, bytesToCopy);
   } // end if(STATUS_SUCCESS == retStatus)

   return retStatus;
} // end ChatpadKeyboardKMDFGetHidReportDescriptor

// This function retrieves a keyboard HID string.
// This code is hand-written but is patterned after VMultiGetString from vmulti.
NTSTATUS
ChatpadKeyboardKMDFGetHidString(
  IN WDFREQUEST   request)
{
   NTSTATUS                retStatus                  = STATUS_SUCCESS;
   PWSTR                   hidString                  = NULL;
   size_t                  lengthToReturn             = 0;
   PVOID                   outputStringBuffer         = NULL;
   size_t                  outputStringBufferLength   = 0;
   WDF_REQUEST_PARAMETERS  requestParameters;
   ULONG_PTR               whichStringRequested;

   ChatpadTrace(("Received HID get string request.\n"));

   WDF_REQUEST_PARAMETERS_INIT(&requestParameters);
   WdfRequestGetParameters(request, &requestParameters);

   // TODO NEXT study how this works to get the actual string requested
   // TODO NEXT this is probably a stupid thing...but move the strings below to be global constants,
   //   or at LEAST review the rules on whether such constants are always available and thus it is
   //   safe to assign them to pointers...in this case, they would just need to be safe for the lifetime
   //   of the function, I suppose.
   whichStringRequested =
     ((reinterpret_cast<ULONG_PTR>(requestParameters.Parameters.DeviceIoControl.Type3InputBuffer)) &
      0xFFFF);
   switch(whichStringRequested)
   {
      case HID_STRING_ID_IMANUFACTURER:
      {
         hidString = L"GAFBlizzard\0";
         break;
      }

      case HID_STRING_ID_IPRODUCT:
      {
         hidString = L"Chatpad Virtual Keyboard\0";
         break;
      }

      case HID_STRING_ID_ISERIALNUMBER:
      {
         hidString = L"42\0";
         break;
      }

      default:
      {
         ChatpadTrace((
           "Unknown value for getting a HID string:  0x%x\n",
           whichStringRequested));
         hidString = NULL;
         break;
      }
   }

   if(NULL != hidString)
   {
      lengthToReturn = ((wcslen(hidString) * sizeof(WCHAR)) +
                        sizeof(UNICODE_NULL));
   }
   else
   {
      lengthToReturn = 0;
      retStatus = STATUS_INVALID_PARAMETER;
   }

   if(STATUS_SUCCESS == retStatus)
   {
      retStatus = WdfRequestRetrieveOutputBuffer(
        request,
        lengthToReturn,
        &outputStringBuffer,
        &outputStringBufferLength);
      if((STATUS_SUCCESS != retStatus) ||
         (outputStringBufferLength < lengthToReturn))
      {
         ChatpadTrace(("WdfRequestRetrieveOutputBuffer failed:  0x%x\n", retStatus));
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      // Copy the HID string.
      RtlCopyMemory(outputStringBuffer, hidString, lengthToReturn);
      WdfRequestSetInformation(request, lengthToReturn);
   } // end if(STATUS_SUCCESS == retStatus)

   ChatpadTrace(("ChatpadKeyboardKMDFGetHidString returning status 0x%x\n", retStatus));

   return retStatus;
} // end ChatpadKeyboardKMDFGetHidString

// This function runs at PASSIVE IRQL.
VOID
ChatpadKeyboardKMDFIOCTLHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       outputBufferLength,
  IN size_t       inputBufferLength,
  IN ULONG        ioControlCode)
{
   NTSTATUS                   apiStatus                           = STATUS_SUCCESS;
   ULONG                      numDriverInstances                  = 0;
   // Device context for a particular filter driver instance.
   PDEVICE_CONTEXT            devContext                          = NULL;
   PVOID                      dataFromUserAsVoid                  = NULL;
   PUCHAR                     dataFromUser                        = NULL;
   // The following pointer can point at either a receive buffer or a send buffer.
   ULONG                      bytesToCopy                         = 0;
   ULONG                      bytesTransferred                    = 0;
   BOOLEAN                    immediatelyFailRequest              = FALSE;
   WDFREQUEST                 requestFromAbove;

//   ChatpadTrace(("The IRQL in the chatpad keyboard KMDF IOCTL handler is %u.\n", KeGetCurrentIrql()));

   WdfWaitLockAcquire(chatpadKeyboardKMDFDeviceCollectionLock, NULL);

   if(TRUE == chatpadKeyboardKMDFControlDeviceAvailable)
   {
      // Get the number of filter driver instances.
      numDriverInstances = WdfCollectionGetCount(chatpadKeyboardKMDFDeviceCollection);
      // Get device context.
      if(numDriverInstances >= 1)
      {
         devContext = ChatpadKeyboardKMDFGetDeviceContext(WdfCollectionGetItem(chatpadKeyboardKMDFDeviceCollection, 0));
      }
   }
   else
   {
      // If there is no control device available, then we want to return immediately to avoid
      // crashing.  This situation can happen if a user program has the driver open
      // and the device is suddenly unplugged.
      immediatelyFailRequest = TRUE;
   }

   WdfWaitLockRelease(chatpadKeyboardKMDFDeviceCollectionLock);

   if(TRUE == immediatelyFailRequest)
   {
      ChatpadTrace(("Ignoring control device IOCTL since the device is unavailable.\n"));
      WdfRequestCompleteWithInformation(request, STATUS_INTERNAL_ERROR, 0);
      return;
   }

   if(IOCTL_CHATPAD_KEYBOARD_KMDF_SET_ENABLED == ioControlCode)
   {
      // TODO add constant for how long the buffer should be
      if(inputBufferLength != 1)
      {
         ChatpadTrace(("Error, input buffer length is %u.\n", inputBufferLength));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         // Get input buffer data.
         apiStatus = WdfRequestRetrieveInputBuffer(request, inputBufferLength, &dataFromUserAsVoid, NULL);
         // This ugly cast is probably avoidable, but it works for now.
         dataFromUser = static_cast<PUCHAR>(dataFromUserAsVoid);
         if((STATUS_SUCCESS != apiStatus) || (NULL == dataFromUser))
         {
            ChatpadTrace(("WdfRequestRetrieveInputBuffer failed:  0x%x\n", apiStatus));
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         if(CHATPAD_KEYBOARD_KMDF_ENABLE == dataFromUser[0])
         {
            devContext->keyboardEnabled = TRUE;
            ChatpadTrace(("Virtual keyboard enabled due to IOCTL command.\n"));
         }
         else if(CHATPAD_KEYBOARD_KMDF_DISABLE == dataFromUser[0])
         {
            devContext->keyboardEnabled = FALSE;
            ChatpadTrace(("Virtual keyboard disabled due to IOCTL command.\n"));
         }
         else
         {
            ChatpadTrace(("Unknown keyboard enable code %d.\n", dataFromUser[0]));
         }
      } // end if(STATUS_SUCCESS == apiStatus)
   } // end if(IOCTL_CHATPAD_KEYBOARD_KMDF_SET_ENABLED == ioControlCode)
   else if(IOCTL_CHATPAD_KEYBOARD_KMDF_SEND_DATA == ioControlCode)
   {
      // TODO add constant for how long the buffer should be at minimum
      if(inputBufferLength < 7)
      {
         ChatpadTrace(("Error, input buffer length is %u.\n", inputBufferLength));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         // Get input buffer data.
         apiStatus = WdfRequestRetrieveInputBuffer(request, inputBufferLength, &dataFromUserAsVoid, NULL);
         // This ugly cast is probably avoidable, but it works for now.
         dataFromUser = static_cast<PUCHAR>(dataFromUserAsVoid);
         if((STATUS_SUCCESS != apiStatus) || (NULL == dataFromUser))
         {
            ChatpadTrace(("WdfRequestRetrieveInputBuffer failed:  0x%x\n", apiStatus));
         }
      }

      // Do not send data out from the virtual keyboard device if the device is disabled.
      if((STATUS_SUCCESS == apiStatus)             &&
         (TRUE == devContext->keyboardEnabled))
      {
/*
         ChatpadTrace((
           "Received bytes 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x\n",
           dataFromUser[0],
           dataFromUser[1],
           dataFromUser[2],
           dataFromUser[3],
           dataFromUser[4],
           dataFromUser[5],
           dataFromUser[6]));
*/

         apiStatus = WdfIoQueueRetrieveNextRequest(
           devContext->hidReadRequestQueue,
           &requestFromAbove);

         if(STATUS_SUCCESS == apiStatus)
         {
            PCHATPAD_KEYBOARD_HID_INPUT_REPORT  inputReport                   = NULL;
            size_t                              requestFromAboveBufferLength  = 0;

            bytesToCopy = sizeof(CHATPAD_KEYBOARD_HID_INPUT_REPORT);
            apiStatus = WdfRequestRetrieveOutputBuffer(
              requestFromAbove,
              bytesToCopy,
              reinterpret_cast<PVOID*>(&inputReport),
              &requestFromAboveBufferLength);
            if((STATUS_SUCCESS == apiStatus)                   &&
               (NULL != inputReport)                           &&
               (requestFromAboveBufferLength >= bytesToCopy))
            {
               // Fill in the report fields.
               inputReport->modifierByte  = dataFromUser[0];
               inputReport->reservedByte  = 0x00;
               // See section 10 in the HID usage table spec for the USB key code reference.
               inputReport->keyCodes[0]   = dataFromUser[1];
               inputReport->keyCodes[1]   = dataFromUser[2];
               inputReport->keyCodes[2]   = dataFromUser[3];
               inputReport->keyCodes[3]   = dataFromUser[4];
               inputReport->keyCodes[4]   = dataFromUser[5];
               inputReport->keyCodes[5]   = dataFromUser[6];

               // Indicate to the requester how many bytes were transferred.
               bytesTransferred = bytesToCopy;
            }
            else
            {
               ChatpadTrace(("Unable to get valid buffer from top-level HID read report request:  0x%x\n",
                             apiStatus));
               apiStatus = STATUS_INTERNAL_ERROR;
            }

            // Return the report to the requester.
            WdfRequestCompleteWithInformation(requestFromAbove, apiStatus, bytesTransferred);
         } // end if(STATUS_SUCCESS == apiStatus)
         else
         {
            ChatpadTrace(("No top-level HID read report request available.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      } // end if successful and the virtual keyboard device is enabled
      else if((STATUS_SUCCESS == apiStatus) &&
              (FALSE == devContext->keyboardEnabled))
      {
         ChatpadTrace(("Ignoring keyboard data since virtual keyboard device is disabled.\n"));
      }
   } // end if(IOCTL_CHATPAD_KEYBOARD_KMDF_SEND_DATA == ioControlCode)
   else
   {
      ChatpadTrace(("Unknown ioctl control code 0x%x.\n", ioControlCode));
      // TODO does this status make the driver/device die, or just return an error
      //   to DeviceIoControl?
      apiStatus = STATUS_INTERNAL_ERROR;
   }

//   ChatpadTrace(("Completing ChatpadKeyboardKMDFIOCTLHandler with bytesTransferred == %lu\n", bytesTransferred));

   // Completing with the length as the information here seems to work, but I cannot seem to find
   // in the API or an example where that is specified as actually always happening for IOCTLs.
   // TODO find that information at some point?
   WdfRequestCompleteWithInformation(request, apiStatus, bytesTransferred);

   return;
} // end ChatpadKeyboardKMDFIOCTLHandler

