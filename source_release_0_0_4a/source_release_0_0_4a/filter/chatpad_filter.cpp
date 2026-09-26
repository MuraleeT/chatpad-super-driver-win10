//
// Chatpad KMDF USB filter driver, by GAFBlizzard A.K.A. Blizzard A.K.A. AgentElrond, copyright 2010.
//
// This code is released under the MIT license.  See LICENSE.TXT for details.
//

// The awesome guide at http://www.osronline.com/article.cfm?id=446 was
// used for information on how to put this driver together, along with
// the WinDDK toaster filter example(s), but the code in this driver was all
// written by hand.

#include "chatpad_filter.h"

// Global variables.

// Lock for synchronization.  This protects the below global variables.
// TODO do I need to worry about deleting this (and if so how), or is just letting the
//   system handle it upon driver unload okay?
WDFWAITLOCK                      chatpadDeviceCollectionLock;

// How many handles are open on the device.  This value should never be greater than one.
ULONG                            numOpenHandles = 0;
// Handle to the control device.
WDFDEVICE                        chatpadControlDevice = NULL;
// Whether the control device is available.  If the device is unplugged,
// the control device may remain in existence, but it will be marked
// as unavailable.
BOOLEAN                          chatpadControlDeviceAvailable = FALSE;

// Collection of objects for different physical device instances.
WDFCOLLECTION                    chatpadDeviceCollection;

// Forward declarations so that functions can be referenced before they are defined.
EVT_WDF_REQUEST_COMPLETION_ROUTINE ControlsReadCompletion;
EVT_WDF_REQUEST_COMPLETION_ROUTINE ChatpadReadCompletion;

extern "C"
DRIVER_INITIALIZE DriverEntry;

// Called when driver is first loaded.
extern "C"
NTSTATUS
DriverEntry(
  PDRIVER_OBJECT  driverObject,
  PUNICODE_STRING registryPath)
{
   NTSTATUS                retStatus = STATUS_SUCCESS;
   WDF_DRIVER_CONFIG       driverConfig;

   ChatpadTrace(("XBox 360 Controller Chatpad Super Driver by GAFBlizzard.\n"));
   ChatpadTrace(("Build metadata unavailable.\n"));

   // This is just an information printout so I can check the size of a DEVICE_CONTEXT structure
   // (only the structure itself, not any additional allocated memory).
   ChatpadTrace(("sizeof(DEVICE_CONTEXT) == %u\n", sizeof(DEVICE_CONTEXT)));

   WDF_DRIVER_CONFIG_INIT(&driverConfig,
                          ChatpadFilterEvtDeviceAdd);
   // Set up the driver unload function.
   driverConfig.EvtDriverUnload = DriverUnload;

   retStatus = WdfDriverCreate(
     driverObject,
     registryPath,
     WDF_NO_OBJECT_ATTRIBUTES,
     &driverConfig,
     NULL);
   if(!NT_SUCCESS(retStatus))
   {
      // TODO why double parentheses?  Macro issues?
      ChatpadTrace(("WdfDriverCreate failed:  0x%0x\n", retStatus));
   }

   // Create collection for different instances of the physical device.
   if(NT_SUCCESS(retStatus))
   {
      retStatus = WdfCollectionCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        &chatpadDeviceCollection);
      if(!NT_SUCCESS(retStatus))
      {
         ChatpadTrace(("WdfCollectionCreate failed:  0x%x\n", retStatus));
      }
   }

   if(NT_SUCCESS(retStatus))
   {
      retStatus = WdfWaitLockCreate(
        WDF_NO_OBJECT_ATTRIBUTES,
        &chatpadDeviceCollectionLock);
      if(!NT_SUCCESS(retStatus))
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
   // TODO why double parentheses?
   ChatpadTrace(("Unloading chatpad filter driver.\n"));

   // Clean up any global data.
   // TODO NEXT add code here?
   return;
} // end DriverUnload

// Utility method to free any allocated usbdInterfaceInformation pointers
// and mark the number of interfaces as 0 in devContext.
//
// IMPORTANT NOTE:  Any needed synchronization must be done outside of
//   this function.  Let the caller beware, and not call this in multiple
//   threads simultaneously or anything like that.
// TODO is there any risk of synchronization issues?  I assume not
//   since other things should not be called once the device context
//   cleanup function is called, but I may want to make sure.
NTSTATUS
ReleaseCachedInterfaceInformation(PDEVICE_CONTEXT devContext)
{
   NTSTATUS retStatus = STATUS_SUCCESS;

   for(BYTE i = 0; i < devContext->numInterfaces; i++)
   {
      if(NULL != devContext->usbdInterfaceInformation[i])
      {
         ExFreePoolWithTag(devContext->usbdInterfaceInformation[i], 'zilB');
         devContext->usbdInterfaceInformation[i] = NULL;
      }
   }

   return retStatus;
} // end ReleaseCachedInterfaceInformation

EVT_WDF_DEVICE_PREPARE_HARDWARE ChatpadFilterEvtPrepareHardware;

NTSTATUS
ChatpadFilterEvtPrepareHardware(
  IN WDFDEVICE                   wdfDevice,
  IN WDFCMRESLIST                resourcesRaw,
  IN WDFCMRESLIST                resourcesTranslated)
{
   NTSTATUS                      retStatus = STATUS_SUCCESS;
   PDEVICE_CONTEXT               devContext = NULL;

   devContext = ChatpadFilterGetDeviceContext(wdfDevice);
   if(NULL != devContext)
   {
      if(NULL == devContext->usbDevice)
      {
         retStatus = WdfUsbTargetDeviceCreate(
           wdfDevice,
           WDF_NO_OBJECT_ATTRIBUTES,
           &(devContext->usbDevice));
      }
   }
   else
   {
      retStatus = STATUS_INTERNAL_ERROR;
   }

   return retStatus;
} // end ChatpadFilterEvtPrepareHardware

EVT_WDF_DEVICE_QUERY_REMOVE ChatpadFilterEvtQueryRemove;

NTSTATUS
ChatpadFilterEvtQueryRemove(
  IN WDFDEVICE wdfDevice)
{
   NTSTATUS    retStatus   = STATUS_SUCCESS;

   ChatpadTrace(("ChatpadFilterEvtQueryRemove called with IRQL %u\n", KeGetCurrentIrql()));
   return retStatus;
} // end ChatpadFilterEvtQueryRemove

EVT_WDF_DEVICE_SURPRISE_REMOVAL ChatpadFilterEvtSurpriseRemoval;

VOID
ChatpadFilterEvtSurpriseRemoval(
  IN WDFDEVICE wdfDevice)
{
   ChatpadTrace(("ChatpadFilterEvtSurpriseRemoval called with IRQL %u\n", KeGetCurrentIrql()));
} // end ChatpadFilterEvtSurpriseRemoval

EVT_WDF_DEVICE_D0_ENTRY ChatpadFilterD0Entry;

// NOTE:  This function must not do anything that could cause a page fault, according to the osrusbfx2 example.
NTSTATUS
ChatpadFilterD0Entry(
  IN WDFDEVICE                wdfDevice,
  IN WDF_POWER_DEVICE_STATE   previousState)
{
   NTSTATUS          retStatus   = STATUS_SUCCESS;
   PDEVICE_CONTEXT   devContext  = NULL;

   devContext = ChatpadFilterGetDeviceContext(wdfDevice);
   if(NULL != devContext)
   {
      ChatpadTrace(("D0 entry occurred.\n"));

      // TODO is this safe?  Are there any synchronizations problem with using the below lines?
      WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);
      chatpadControlDeviceAvailable = TRUE;
      WdfWaitLockRelease(chatpadDeviceCollectionLock);
   } // end if(NULL != devContext)
   else
   {
      ChatpadTrace(("Unable to obtain device context.\n"));
      retStatus = STATUS_INTERNAL_ERROR;
   }

   return retStatus;
} // end ChatpadFilterD0Entry

EVT_WDF_DEVICE_D0_EXIT ChatpadFilterD0Exit;

// NOTE:  This function must not do anything that could cause a page fault, according to the osrusbfx2 example.
NTSTATUS
ChatpadFilterD0Exit(
  WDFDEVICE                wdfDevice,
  WDF_POWER_DEVICE_STATE   targetState)
{
   NTSTATUS retStatus = STATUS_SUCCESS;
   PDEVICE_CONTEXT devContext  = NULL;

   // TODO I assume I do not need the PAGED_CODE() call from the osrusbfx2 example?

   devContext = ChatpadFilterGetDeviceContext(wdfDevice);
   if(NULL != devContext)
   {
      ChatpadTrace(("D0 exit occurred.\n"));
   }
   else
   {
      retStatus = STATUS_INTERNAL_ERROR;
   }

   return retStatus;
} // end ChatpadFilterD0Exit

NTSTATUS
ChatpadFilterEvtDeviceAdd(
  IN WDFDRIVER       driver,
  IN PWDFDEVICE_INIT deviceInit)
{
   NTSTATUS                      retStatus = STATUS_SUCCESS;
   PDEVICE_CONTEXT               devContext = NULL;
   //ULONG                         serialNumber = 0;
   //ULONG                         returnSize = 0;
   WDF_OBJECT_ATTRIBUTES         wdfObjectAttr;
   WDFDEVICE                     wdfDevice = NULL;
   WDF_IO_QUEUE_CONFIG           ioQueueConfig;
   WDF_PNPPOWER_EVENT_CALLBACKS  powerCallbacks;

   // TODO:  Find some way to get the serial number, or otherwise identify the device, since trying
   //        to get the DevicePropertyUINumber fails (STATUS_OBJECT_NAME_NOT_FOUND) on my Vista 64 system, at least.
   /*
   // Get device serial number.
   retStatus = WdfFdoInitQueryProperty(
     deviceInit,
     DevicePropertyUINumber,
     sizeof(serialNumber),
     &serialNumber,
     &returnSize);
   */

   if(STATUS_SUCCESS == retStatus)
   {
      // Mark ourselves as a filter, causing requests we don't care about to be
      // passed along etc.
      // 
      // By the way, apparently FDO means "Framework Device Object".
      WdfFdoInitSetFilter(deviceInit);

      // Mark our device type as unknown.
      // TODO should this be set eventually since this driver is not generic, or is the
      //      automatic stuff fine?
      WdfDeviceInitSetDeviceType(deviceInit, FILE_DEVICE_UNKNOWN);

      // Register a prepare hardware callback to create a WDF USB device object.
      WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&powerCallbacks);
      powerCallbacks.EvtDevicePrepareHardware = ChatpadFilterEvtPrepareHardware;
      powerCallbacks.EvtDeviceD0Entry = ChatpadFilterD0Entry;
      powerCallbacks.EvtDeviceD0Exit = ChatpadFilterD0Exit;
      powerCallbacks.EvtDeviceSurpriseRemoval = ChatpadFilterEvtSurpriseRemoval;
      powerCallbacks.EvtDeviceQueryRemove = ChatpadFilterEvtQueryRemove;

      WdfDeviceInitSetPnpPowerEventCallbacks(
        deviceInit,
        &powerCallbacks);

      WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&wdfObjectAttr, DEVICE_CONTEXT);

      // Register for cleanup notification.
      wdfObjectAttr.EvtCleanupCallback = ChatpadFilterEvtDeviceContextCleanup;

      // Create framework device object, attach to the lower stack etc.
      retStatus = WdfDeviceCreate(
        &deviceInit,
        &wdfObjectAttr,
        &wdfDevice);
   }
   /*
   else
   {
      ChatpadTrace(("Failed to get serial number:  0x%x\n", retStatus));
   }
   */

   if(STATUS_SUCCESS == retStatus)
   {
      devContext = ChatpadFilterGetDeviceContext(wdfDevice);
      if(NULL != devContext)
      {
         // Initialize DEVICE_CONTEXT values.
         ChatpadTrace(("Initializing device context.\n"));

         devContext->magicNumber = DEVICE_CONTEXT_MAGIC_NUMBER;
         // TODO if I ever support multiple devices, this probably needs to be unique per device!
         //   It would be based on serial number or something similar.
         //devContext->serialNumber = serialNumber;
         devContext->serialNumber = 0x00000042;
         devContext->targetForRequests = WdfDeviceGetIoTarget(wdfDevice);
         devContext->usbDevice = NULL;
         devContext->usbConfigSelected = FALSE;
         devContext->msInitFinished = FALSE;
         devContext->chatpadInitFinished = FALSE;
         // Filter initially until the Microsoft driver controller init is finished.
         devContext->filterMode = FILTER_MODE_FILTERED;
         devContext->numInterfaces = 0;

         for(BYTE i = 0; i < CHATPAD_DRIVER_MAX_SUPPORTED_NUM_INTERFACES; i++)
         {
            devContext->usbdInterfaceInformation[i] = NULL;
         }

         devContext->numPipes = 0;

         for(BYTE i = 0; i < CHATPAD_DRIVER_MAX_SUPPORTED_NUM_PIPES; i++)
         {
            WDF_USB_PIPE_INFORMATION_INIT(&(devContext->wdfUsbPipeInfo[i]));
            devContext->wdfUsbPipe[i] = NULL;
         }

         // Create requests and memory buffers for writing to and reading from endpoints.
         // I am not sure if these have to be non-pageable, but I am making all request
         // storage non-pageable to be on the safe side especially since the total amount
         // of data I am allocating is presumably pretty small.

         WDF_OBJECT_ATTRIBUTES requestAttributes;

         WDF_OBJECT_ATTRIBUTES_INIT(&requestAttributes);

         // Make the I/O target the parent object of these requests so that
         // they will get deleted when the I/O target is deleted.
         // TODO I may want to do more research and verify that these get
         //   deleted properly, if possible.  Could they ever hang the
         //   driver on shutdown or driver upgrade if they are not pending?
         requestAttributes.ParentObject = devContext->targetForRequests;

         if(STATUS_SUCCESS == retStatus)
         {
            retStatus = WdfMemoryCreate(
              WDF_NO_OBJECT_ATTRIBUTES,
              NonPagedPool,
              0,
              CONTROLS_WRITE_BUFFER_LENGTH,
              &(devContext->controlsWriteBufferMemory),
              NULL);
            if(STATUS_SUCCESS != retStatus)
            {
               ChatpadTrace(("Allocation failed for controls write buffer memory.\n"));
            }
         }

         if(STATUS_SUCCESS == retStatus)
         {
            retStatus = WdfMemoryCreate(
              WDF_NO_OBJECT_ATTRIBUTES,
              NonPagedPool,
              0,
              sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
              &(devContext->controlsWriteUrbMemory),
              NULL);
            if(STATUS_SUCCESS != retStatus)
            {
               ChatpadTrace(("Allocation failed for controls write URB memory.\n"));
            }
         }

         if(STATUS_SUCCESS == retStatus)
         {
            retStatus = WdfRequestCreate(
              &requestAttributes,
              devContext->targetForRequests,
              &(devContext->controlsWriteRequest));
            if(STATUS_SUCCESS != retStatus)
            {
               ChatpadTrace(("WdfRequestCreate failed:  0x%x\n", retStatus));
            }
         }

         if(STATUS_SUCCESS == retStatus)
         {
            PURB controlsWriteUrb =
              static_cast<PURB>(WdfMemoryGetBuffer(devContext->controlsWriteUrbMemory, NULL));
            if(NULL != controlsWriteUrb)
            {
               // TODO move this code somewhere common so it will not be redundant and prone to bugs if
               //   one side changes?
               controlsWriteUrb->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
               controlsWriteUrb->UrbHeader.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
               controlsWriteUrb->UrbBulkOrInterruptTransfer.PipeHandle = NULL;
               controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferFlags =
                 USBD_TRANSFER_DIRECTION_OUT;
               controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBufferLength =
                 CONTROLS_WRITE_BUFFER_LENGTH;
               controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
               controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBuffer =
                 WdfMemoryGetBuffer(devContext->controlsWriteBufferMemory, NULL);
               controlsWriteUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
            }
            else
            {
               ChatpadTrace(("Failed to get controls write URB memory buffer.\n"));
               retStatus = STATUS_INTERNAL_ERROR;
            }
         }

         for(int requestIndex = 0;
             ((STATUS_SUCCESS == retStatus) &&
              (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
             requestIndex++)
         {
            if(STATUS_SUCCESS == retStatus)
            {
               retStatus = WdfMemoryCreate(
                 WDF_NO_OBJECT_ATTRIBUTES,
                 NonPagedPool,
                 0,
                 CONTROLS_READ_BUFFER_LENGTH,
                 &(devContext->controlsReadBufferMemory[requestIndex]),
                 NULL);
               if(STATUS_SUCCESS != retStatus)
               {
                  ChatpadTrace(("Allocation failed for controls read buffer memory.\n"));
               }
            }

            if(STATUS_SUCCESS == retStatus)
            {
               retStatus = WdfMemoryCreate(
                 WDF_NO_OBJECT_ATTRIBUTES,
                 NonPagedPool,
                 0,
                 sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                 &(devContext->controlsReadUrbMemory[requestIndex]),
                 NULL);
               if(STATUS_SUCCESS != retStatus)
               {
                  ChatpadTrace(("Allocation failed for controls read URB memory.\n"));
               }
            }

            if(STATUS_SUCCESS == retStatus)
            {
               PURB controlsReadUrb =
                 static_cast<PURB>(WdfMemoryGetBuffer(devContext->controlsReadUrbMemory[requestIndex], NULL));
               if(NULL != controlsReadUrb)
               {
                  // TODO move this code somewhere common so it will not be redundant and prone to bugs if
                  //   one side changes?
                  controlsReadUrb->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
                  controlsReadUrb->UrbHeader.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
                  controlsReadUrb->UrbBulkOrInterruptTransfer.PipeHandle = NULL;
                  // TODO could the short transfer flag ever cause problems on some controllers and/or hubs?
                  controlsReadUrb->UrbBulkOrInterruptTransfer.TransferFlags =
                    USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
                  controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength =
                    CONTROLS_READ_BUFFER_LENGTH;
                  controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
                  controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer =
                    WdfMemoryGetBuffer(devContext->controlsReadBufferMemory[requestIndex], NULL);
                  controlsReadUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
               }
               else
               {
                  ChatpadTrace(("Failed to get controls read URB memory buffer.\n"));
                  retStatus = STATUS_INTERNAL_ERROR;
               }
            }

            if(STATUS_SUCCESS == retStatus)
            {
               devContext->controlsReadRequestsInUse[requestIndex] = FALSE;

               retStatus = WdfRequestCreate(
                 &requestAttributes,
                 devContext->targetForRequests,
                 &(devContext->controlsReadRequests[requestIndex]));
               if(STATUS_SUCCESS != retStatus)
               {
                  ChatpadTrace(("WdfRequestCreate failed:  0x%x\n", retStatus));
               }
            }

            if(STATUS_SUCCESS == retStatus)
            {
               retStatus = WdfMemoryCreate(
                 WDF_NO_OBJECT_ATTRIBUTES,
                 NonPagedPool,
                 0,
                 CHATPAD_READ_BUFFER_LENGTH,
                 &(devContext->chatpadReadBufferMemory[requestIndex]),
                 NULL);
               if(STATUS_SUCCESS != retStatus)
               {
                  ChatpadTrace(("Allocation failed for chatpad read buffer memory.\n"));
               }
            }

            if(STATUS_SUCCESS == retStatus)
            {
               retStatus = WdfMemoryCreate(
                 WDF_NO_OBJECT_ATTRIBUTES,
                 NonPagedPool,
                 0,
                 sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
                 &(devContext->chatpadReadUrbMemory[requestIndex]),
                 NULL);
               if(STATUS_SUCCESS != retStatus)
               {
                  ChatpadTrace(("Allocation failed for chatpad read URB memory.\n"));
               }
            }

            if(STATUS_SUCCESS == retStatus)
            {
               PURB chatpadReadUrb =
                 static_cast<PURB>(WdfMemoryGetBuffer(devContext->chatpadReadUrbMemory[requestIndex], NULL));
               if(NULL != chatpadReadUrb)
               {
                  // TODO move this code somewhere common so it will not be redundant and prone to bugs if
                  //   one side changes?
                  chatpadReadUrb->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
                  chatpadReadUrb->UrbHeader.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
                  chatpadReadUrb->UrbBulkOrInterruptTransfer.PipeHandle = NULL;
                  // TODO could the short transfer flag ever cause problems on some controllers and/or hubs?
                  chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferFlags =
                    USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
                  chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength =
                    CHATPAD_READ_BUFFER_LENGTH;
                  chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
                  chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer =
                    WdfMemoryGetBuffer(devContext->chatpadReadBufferMemory[requestIndex], NULL);
                  chatpadReadUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;
               }
               else
               {
                  ChatpadTrace(("Failed to get chatpad read URB memory buffer.\n"));
                  retStatus = STATUS_INTERNAL_ERROR;
               }
            }

            if(STATUS_SUCCESS == retStatus)
            {
               devContext->chatpadReadRequestsInUse[requestIndex] = FALSE;

               retStatus = WdfRequestCreate(
                 &requestAttributes,
                 devContext->targetForRequests,
                 &(devContext->chatpadReadRequests[requestIndex]));
               if(STATUS_SUCCESS != retStatus)
               {
                  ChatpadTrace(("WdfRequestCreate failed:  0x%x\n", retStatus));
               }
            }
         } // end looping through chatpad filter driver request indices

         if(STATUS_SUCCESS == retStatus)
         {
            // Initialize default queue config structure.
            WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
              &ioQueueConfig,
              WdfIoQueueDispatchParallel);

            // Select what we want from the queue.

            // This should catch IOCTL_INTERNAL_USB_SUBMIT_URB.
            ioQueueConfig.EvtIoInternalDeviceControl =
              ChatpadFilterInternalIOCTLHandler;

            // Create the default queue.
            retStatus = WdfIoQueueCreate(
              wdfDevice,
              &ioQueueConfig,
              WDF_NO_OBJECT_ATTRIBUTES,
              NULL);
            if(STATUS_SUCCESS == retStatus)
            {
               ChatpadTrace(("Created device filter default queue successfully.\n"));
            }
            else
            {
               ChatpadTrace(("WdfIoQueueCreate failed:  0x%x\n", retStatus));
            }
         } // end if(STATUS_SUCCESS == retStatus)

         if(STATUS_SUCCESS == retStatus)
         {
            // Create manual I/O queue for parking controls endpoint read requests
            // from the top-level driver.
            WDF_IO_QUEUE_CONFIG controlsParkedDriverReadRequestQueueConfig;

            WDF_IO_QUEUE_CONFIG_INIT(&controlsParkedDriverReadRequestQueueConfig,
                                     WdfIoQueueDispatchManual);
            retStatus =
              WdfIoQueueCreate(
                wdfDevice,
                &controlsParkedDriverReadRequestQueueConfig,
                WDF_NO_OBJECT_ATTRIBUTES,
                &(devContext->controlsParkedDriverReadRequestQueue));
            if(STATUS_SUCCESS == retStatus)
            {
               ChatpadTrace(("Created controls driver read request parking queue successfully.\n"));
            }
            else
            {
               ChatpadTrace(("WdfIoQueueCreate failed:  0x%x\n", retStatus));
            }
         } // end if(STATUS_SUCCESS == retStatus)

         if(STATUS_SUCCESS == retStatus)
         {
            // Create manual I/O queue for parking controls endpoint read requests
            // from the user.
            WDF_IO_QUEUE_CONFIG controlsParkedUserReadRequestQueueConfig;

            WDF_IO_QUEUE_CONFIG_INIT(&controlsParkedUserReadRequestQueueConfig,
                                     WdfIoQueueDispatchManual);
            retStatus =
              WdfIoQueueCreate(
                wdfDevice,
                &controlsParkedUserReadRequestQueueConfig,
                WDF_NO_OBJECT_ATTRIBUTES,
                &(devContext->controlsParkedUserReadRequestQueue));
            if(STATUS_SUCCESS == retStatus)
            {
               ChatpadTrace(("Created controls user read request parking queue successfully.\n"));
            }
            else
            {
               ChatpadTrace(("WdfIoQueueCreate failed:  0x%x\n", retStatus));
            }
         } // end if(STATUS_SUCCESS == retStatus)

         if(STATUS_SUCCESS == retStatus)
         {
            // Create manual I/O queue for parking chatpad endpoint read requests
            // from the user.
            WDF_IO_QUEUE_CONFIG chatpadParkedUserReadRequestQueueConfig;

            WDF_IO_QUEUE_CONFIG_INIT(&chatpadParkedUserReadRequestQueueConfig,
                                     WdfIoQueueDispatchManual);
            retStatus =
              WdfIoQueueCreate(
                wdfDevice,
                &chatpadParkedUserReadRequestQueueConfig,
                WDF_NO_OBJECT_ATTRIBUTES,
                &(devContext->chatpadParkedUserReadRequestQueue));
            if(STATUS_SUCCESS == retStatus)
            {
               ChatpadTrace(("Created chatpad user read request parking queue successfully.\n"));
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
   else
   {
      ChatpadTrace(("WdfDeviceCreate failed:  0x%x\n", retStatus));
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // Add the device to the collection.
      WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);

      retStatus = WdfCollectionAdd(
        chatpadDeviceCollection,
        wdfDevice);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfCollectionAdd failed:  0x%x\n", retStatus));
      }

      WdfWaitLockRelease(chatpadDeviceCollectionLock);
   }

   // Create the control device.
   if(STATUS_SUCCESS == retStatus)
   {
      retStatus = ChatpadCreateControlDevice(wdfDevice);
      // For now, the return status here is ignored since
      // ChatpadCreateControlDevice should print an error if it fails, anyway.
   }

   return retStatus;
} // end ChatpadFilterEvtDeviceAdd

// This method sends the controls data write request specified by the given
// device context and request index.
NTSTATUS SendControlsWriteRequest(
  IN PDEVICE_CONTEXT devContext,
  PUCHAR             inputBuffer,
  ULONG              bytesToWrite,
  PULONG             bytesTransferred)
{
   NTSTATUS                   retStatus         = STATUS_SUCCESS;
   PURB                       controlsWriteUrb  = NULL;
   WDF_REQUEST_SEND_OPTIONS   sendOptions;

   if((NULL == devContext)          ||
      (NULL == inputBuffer)         ||
      (0 == bytesToWrite)           ||
      (NULL == bytesTransferred))
   {
      ChatpadTrace(("Invalid parameters to SendControlsWriteRequest.\n"));
      retStatus = STATUS_INTERNAL_ERROR;
   }

   if(STATUS_SUCCESS == retStatus)
   {
      RtlFillMemory(WdfMemoryGetBuffer(devContext->controlsWriteBufferMemory, NULL),
                    0x00,
                    CONTROLS_WRITE_BUFFER_LENGTH);

      // Reinitialize the URB since its contents may have been adjusted during
      // the most recent transfer.
      controlsWriteUrb =
        static_cast<PURB>(WdfMemoryGetBuffer(devContext->controlsWriteUrbMemory, NULL));
      if(NULL != controlsWriteUrb)
      {
         // TODO move this code somewhere common so it will not be redundant and prone to bugs if
         //   one side changes?
         controlsWriteUrb->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
         controlsWriteUrb->UrbHeader.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
         controlsWriteUrb->UrbBulkOrInterruptTransfer.PipeHandle =
           devContext->usbdInterfaceInformation[0]->Pipes[1].PipeHandle;
         controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferFlags =
           USBD_TRANSFER_DIRECTION_OUT;
         controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBufferLength =
           CONTROLS_WRITE_BUFFER_LENGTH;
         controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
         controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBuffer =
           WdfMemoryGetBuffer(devContext->controlsWriteBufferMemory, NULL);
         controlsWriteUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

         // Copy the data to send and the data length into the URB.
         RtlCopyMemory(
           controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBuffer,
           inputBuffer,
           bytesToWrite);
         controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = bytesToWrite;
      }
      else
      {
         ChatpadTrace(("Error, failed to get controls write URB memory buffer.\n"));
         retStatus = STATUS_INTERNAL_ERROR;
      }
   } // end if(STATUS_SUCCESS == retStatus)

   if(STATUS_SUCCESS == retStatus)
   {
      // TODO make enums or some other system for pipe indices?
      // Format the request for sending an URB.
      retStatus = WdfUsbTargetPipeFormatRequestForUrb(
        devContext->wdfUsbPipe[0],
        devContext->controlsWriteRequest,
        devContext->controlsWriteUrbMemory,
        NULL);
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // Set up a timeout.
      WDF_REQUEST_SEND_OPTIONS_INIT(
        &sendOptions,
        WDF_REQUEST_SEND_OPTION_SYNCHRONOUS | WDF_REQUEST_SEND_OPTION_TIMEOUT);
      sendOptions.Timeout = WDF_REL_TIMEOUT_IN_SEC(CHATPAD_DRIVER_SYNCHRONOUS_REQUEST_TIMEOUT_LENGTH);
      retStatus = WdfRequestAllocateTimer(devContext->controlsWriteRequest);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfRequestAllocateTimer failed:  0x%x\n", retStatus));
      }
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // Send the request synchronously.
      if(WdfRequestSend(devContext->controlsWriteRequest,
                        devContext->targetForRequests,
                        &sendOptions))
      {
//         ChatpadTrace(("*** Sent controls write request successfully. ***\n"));
         retStatus = WdfRequestGetStatus(devContext->controlsWriteRequest);
         // I presumably should never have a value other than STATUS_SUCCESS since
         // I am sending the request synchronously, so all values are treated as errors.
         if(STATUS_SUCCESS != retStatus)
         {
            ChatpadTrace(("(WdfRequestGetStatus returns 0x%x)\n", retStatus));
         }
      }
      else
      {
         ChatpadTrace(("*** Failed to send write request. ***\n"));
         retStatus = STATUS_INTERNAL_ERROR;
      }

      // Make sure that the IOCTL sees the proper number of bytes transferred.
      *bytesTransferred = controlsWriteUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;

      WDF_REQUEST_REUSE_PARAMS reuseParams;

      WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);
      retStatus = WdfRequestReuse(
        devContext->controlsWriteRequest,
        &reuseParams);
      if(STATUS_SUCCESS != retStatus)
      {
         ChatpadTrace(("WdfRequestReuse failed:  0x%x\n", retStatus));
      }
   } // end if(STATUS_SUCCESS == retStatus)
   else
   {
      ChatpadTrace(("WdfUsbTargetPipeFormatRequestForUrb failed:  0x%x\n", retStatus));
   }

   return retStatus;
} // end SendControlsWriteRequest

// This method sends the controls data read request specified by the given
// device context and request index.
NTSTATUS SendControlsReadRequest(
  IN PDEVICE_CONTEXT devContext,
  IN ULONG           requestIndex)
{
   NTSTATUS                   retStatus      = STATUS_SUCCESS;
   WDF_REQUEST_SEND_OPTIONS   sendOptions;

   RtlFillMemory(WdfMemoryGetBuffer(devContext->controlsReadBufferMemory[requestIndex], NULL),
                 0x00,
                 CONTROLS_READ_BUFFER_LENGTH);

   // Reinitialize the URB since its contents may have been adjusted during
   // the most recent transfer.
   PURB controlsReadUrb =
     static_cast<PURB>(WdfMemoryGetBuffer(devContext->controlsReadUrbMemory[requestIndex], NULL));
   if(NULL != controlsReadUrb)
   {
      // TODO move this code somewhere common so it will not be redundant and prone to bugs if
      //   one side changes?
      controlsReadUrb->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
      controlsReadUrb->UrbHeader.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
      controlsReadUrb->UrbBulkOrInterruptTransfer.PipeHandle =
        devContext->usbdInterfaceInformation[0]->Pipes[0].PipeHandle;
      // TODO could the short transfer flag ever cause problems on some controllers and/or hubs?
      controlsReadUrb->UrbBulkOrInterruptTransfer.TransferFlags =
        USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
      controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength =
        CONTROLS_READ_BUFFER_LENGTH;
      controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
      controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer =
        WdfMemoryGetBuffer(devContext->controlsReadBufferMemory[requestIndex], NULL);
      controlsReadUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

      // This is just code that I am saving in case I ever want to
      // print bulk or interrupt transfer URB information again.
/*
      ChatpadTrace((
        "!!! %u %u 0x%x 0x%x %u 0x%x 0x%x 0x%x\n",
        controlsReadUrb->UrbHeader.Function,
        controlsReadUrb->UrbHeader.Length,
        controlsReadUrb->UrbBulkOrInterruptTransfer.PipeHandle,
        controlsReadUrb->UrbBulkOrInterruptTransfer.TransferFlags,
        controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength,
        controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL,
        controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer,
        controlsReadUrb->UrbBulkOrInterruptTransfer.UrbLink));
*/
   }
   else
   {
      ChatpadTrace(("Error, failed to get controls read URB memory buffer.\n"));
      retStatus = STATUS_INTERNAL_ERROR;
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // TODO make enums or some other system for pipe indices?
      // Format the request for sending an URB.
      retStatus = WdfUsbTargetPipeFormatRequestForUrb(
        devContext->wdfUsbPipe[0],
        devContext->controlsReadRequests[requestIndex],
        devContext->controlsReadUrbMemory[requestIndex],
        NULL);
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // TODO eventually figure out, if possible, why the URB (Argument1) value is going
      //   away before the completion routine gets called.  If I still had that, I could
      //   use a single completion routine instead of two separate ones for endpoint read
      //   data.  UPDATE:  Doron Holan says (http://www.osronline.com/showthread.cfm?link=195082)
      //   that stack locations are zeroed as the IRP is completed up the stack.
      //   TODO am I safe using the completion parameters then?
      WdfRequestSetCompletionRoutine(
        devContext->controlsReadRequests[requestIndex],
        ControlsReadCompletion,
        devContext);

      // Send the request.
      if(WdfRequestSend(devContext->controlsReadRequests[requestIndex],
                        devContext->targetForRequests,
                        WDF_NO_SEND_OPTIONS))
      {
//         ChatpadTrace(("*** Sent controls read request successfully. ***\n"));
         // TODO do I need to do this here?
         retStatus = WdfRequestGetStatus(devContext->controlsReadRequests[requestIndex]);
         if(STATUS_SUCCESS != retStatus)
         {
            // 0x00000103 means pending and is presumably normal.
//            ChatpadTrace(("(WdfRequestGetStatus returns 0x%x)\n", retStatus));
            // Ignore the normal pending value so that we continue executing successfully.
            if(STATUS_PENDING == retStatus)
            {
               retStatus = STATUS_SUCCESS;
            }
         }
      }
      else
      {
         ChatpadTrace(("*** Failed to send read request. ***\n"));
         retStatus = STATUS_INTERNAL_ERROR;
      }
   } // end if(STATUS_SUCCESS == retStatus)
   else
   {
      ChatpadTrace(("WdfUsbTargetPipeFormatRequestForUrb failed:  0x%x\n", retStatus));
   }

   return retStatus;
} // end SendControlsReadRequest

// This method sends the chatpad data read request specified by the given
// device context and request index.
NTSTATUS SendChatpadReadRequest(
  IN PDEVICE_CONTEXT devContext,
  IN ULONG           requestIndex)
{
   NTSTATUS retStatus = STATUS_SUCCESS;

   RtlFillMemory(WdfMemoryGetBuffer(devContext->chatpadReadBufferMemory[requestIndex], NULL),
                 0x00,
                 CHATPAD_READ_BUFFER_LENGTH);

//ChatpadTrace((".\n")); // TODO REMOVE
   // Reinitialize the URB since its contents may have been adjusted during
   // the most recent transfer.
   PURB chatpadReadUrb =
     static_cast<PURB>(WdfMemoryGetBuffer(devContext->chatpadReadUrbMemory[requestIndex], NULL));
   if(NULL != chatpadReadUrb)
   {
      // TODO move this code somewhere common so it will not be redundant and prone to bugs if
      //   one side changes?
      chatpadReadUrb->UrbHeader.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
      chatpadReadUrb->UrbHeader.Length = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
      chatpadReadUrb->UrbBulkOrInterruptTransfer.PipeHandle =
        devContext->usbdInterfaceInformation[2]->Pipes[0].PipeHandle;
      // TODO could the short transfer flag ever cause problems on some controllers and/or hubs?
      chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferFlags =
        USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK;
      chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength =
        CHATPAD_READ_BUFFER_LENGTH;
      chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL = NULL;
      chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer =
        WdfMemoryGetBuffer(devContext->chatpadReadBufferMemory[requestIndex], NULL);
      chatpadReadUrb->UrbBulkOrInterruptTransfer.UrbLink = NULL;

      // This is just code that I am saving in case I ever want to
      // print bulk or interrupt transfer URB information again.
/*
      ChatpadTrace((
        "!!! %u %u 0x%x 0x%x %u 0x%x 0x%x 0x%x\n",
        chatpadReadUrb->UrbHeader.Function,
        chatpadReadUrb->UrbHeader.Length,
        chatpadReadUrb->UrbBulkOrInterruptTransfer.PipeHandle,
        chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferFlags,
        chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength,
        chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBufferMDL,
        chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer,
        chatpadReadUrb->UrbBulkOrInterruptTransfer.UrbLink));
*/
   }
   else
   {
      ChatpadTrace(("Error, failed to get chatpad read URB memory buffer.\n"));
      retStatus = STATUS_INTERNAL_ERROR;
   }

   if(STATUS_SUCCESS == retStatus)
   {
      // TODO make enums or some other system for pipe indices?
      // Format the request for sending an URB.
      retStatus = WdfUsbTargetPipeFormatRequestForUrb(
        devContext->wdfUsbPipe[0],
        devContext->chatpadReadRequests[requestIndex],
        devContext->chatpadReadUrbMemory[requestIndex],
        NULL);
   }

   if(STATUS_SUCCESS == retStatus)
   {
      WdfRequestSetCompletionRoutine(
        devContext->chatpadReadRequests[requestIndex],
        ChatpadReadCompletion,
        devContext);

      // Send the request.
      if(WdfRequestSend(devContext->chatpadReadRequests[requestIndex],
                        devContext->targetForRequests,
                        WDF_NO_SEND_OPTIONS))
      {
//         ChatpadTrace(("*** Sent chatpad read request successfully. ***\n"));
/*
         retStatus = WdfRequestGetStatus(devContext->chatpadReadRequests[requestIndex]);
         if(STATUS_SUCCESS != retStatus)
         {
            // 0x00000103 means pending and is presumably normal.
//            ChatpadTrace(("(for chatpad, WdfRequestGetStatus returns 0x%x)\n", retStatus));
            // Ignore the normal pending value so that we continue executing successfully.
            if(STATUS_PENDING == retStatus)
            {
               retStatus = STATUS_SUCCESS;
            }
         }
*/
      }
      else
      {
         ChatpadTrace(("*** Failed to send read request. ***\n"));
         retStatus = STATUS_INTERNAL_ERROR;
      }
   } // end if(STATUS_SUCCESS == retStatus)
   else
   {
      ChatpadTrace(("WdfUsbTargetPipeFormatRequestForUrb failed:  0x%x\n", retStatus));
   }

   return retStatus;
} // end SendChatpadReadRequest

EVT_WDF_DEVICE_FILE_CREATE ChatpadControlDeviceCreateHandler;

VOID
ChatpadControlDeviceCreateHandler(
  WDFDEVICE       device,
  WDFREQUEST      request,
  WDFFILEOBJECT   fileObject)
{
   NTSTATUS apiStatus = STATUS_SUCCESS;

   // Increment the open count.
   WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);

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

   ChatpadTrace(("There are now %lu open handles to the chatpad filter device.\n", numOpenHandles));
   WdfRequestComplete(request, apiStatus);

   WdfWaitLockRelease(chatpadDeviceCollectionLock);
} // end ChatpadControlDeviceCreateHandler

EVT_WDF_FILE_CLOSE ChatpadControlDeviceCloseHandler;

VOID
ChatpadControlDeviceCloseHandler(
  WDFFILEOBJECT   fileObject)
{
   NTSTATUS apiStatus = STATUS_SUCCESS;

   // Decrement the open count.
   WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);

   // Since I have a wait lock, can I safely just modify it like this?
   if(numOpenHandles > 0)
   {
      numOpenHandles--;
   }
   else
   {
      ChatpadTrace(("Error, there are supposedly 0 open handles but the close handler was called...\n"));
   }

   ChatpadTrace(("There are now %lu open handles to the chatpad filter device.\n", numOpenHandles));

   if(0 == numOpenHandles)
   {
      // TODO there is probably a cleaner way to do this than having one extension
      //   have a pointer to the other one etc...maybe?  I want to make sure
      //   that filter mode gets disabled if the application disconnects,
      //   though.
      PCHATPAD_CONTROL_DEVICE_EXTENSION   controlExtension  = NULL;
      PDEVICE_CONTEXT                     devContext        = NULL;

      if(NULL == chatpadControlDevice)
      {
         ChatpadTrace(("Error, control device handle is NULL.\n"));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         controlExtension = ChatpadControlGetData(chatpadControlDevice);

         if(NULL == controlExtension)
         {
            ChatpadTrace(("Failed to get control device extension in chatpad filter close handler.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         devContext = controlExtension->chatpadFilterDeviceContext;
         if(NULL == devContext)
         {
            ChatpadTrace(("Failed to get device context in close handler.\n"));
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      }

      if(STATUS_SUCCESS == apiStatus)
      {
         // Set the filter mode to unfiltered when the application disconnects.
         // TODO could messing with the device context here cause synchronization issues?
         devContext->filterMode = FILTER_MODE_UNFILTERED;
         ChatpadTrace(("Filter mode set to unfiltered on application disconnect.\n"));

         // If any available controls read requests are not already pending,
         // then send them.  This is done because the top-level driver
         // should always have read requests pending, so we can just
         // start completing them as soon as we get data.
         for(int requestIndex = 0;
             ((STATUS_SUCCESS == apiStatus) &&
              (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
             requestIndex++)
         {
            // TODO do I need synchronization here?
            if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
            {
               devContext->controlsReadRequestsInUse[requestIndex] = TRUE;
               apiStatus = SendControlsReadRequest(devContext, requestIndex);
            } // if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
         } // end looping through chatpad filter driver request indices
      } // end if(STATUS_SUCCESS == apiStatus)
   } // end if(0 == numOpenHandles)

   WdfWaitLockRelease(chatpadDeviceCollectionLock);
} // end ChatpadControlDeviceCloseHandler

VOID
ChatpadFilterEvtDeviceContextCleanup(
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

   ChatpadTrace(("Cleaning up device context.\n"));

   devContext = ChatpadFilterGetDeviceContext(device);
   if((NULL != device) && (NULL != devContext))
   {
      WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);

      // Clean up device context resources.
      // TODO is this the appropriate place to do the cleanup, inside the lock
      //   and before the device gets deleted?
      ReleaseCachedInterfaceInformation(devContext);
      for(int requestIndex = 0;
          ((STATUS_SUCCESS == apiStatus) &&
           (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
          requestIndex++)
      {
         // TODO is it safe to release request memory here, or is there any chance
         //   that it could still be in use by the system/requests?

         if(NULL != devContext->controlsWriteBufferMemory)
         {
            WdfObjectDelete(devContext->controlsWriteBufferMemory);
            devContext->controlsWriteBufferMemory = NULL;
         }

         if(NULL != devContext->controlsWriteUrbMemory)
         {
            WdfObjectDelete(devContext->controlsWriteUrbMemory);
            devContext->controlsWriteUrbMemory = NULL;
         }

         if(NULL != devContext->controlsReadBufferMemory[requestIndex])
         {
            WdfObjectDelete(devContext->controlsReadBufferMemory[requestIndex]);
            devContext->controlsReadBufferMemory[requestIndex] = NULL;
         }

         if(NULL != devContext->controlsReadUrbMemory[requestIndex])
         {
            WdfObjectDelete(devContext->controlsReadUrbMemory[requestIndex]);
            devContext->controlsReadUrbMemory[requestIndex] = NULL;
         }

         if(NULL != devContext->chatpadReadBufferMemory[requestIndex])
         {
            WdfObjectDelete(devContext->chatpadReadBufferMemory[requestIndex]);
            devContext->chatpadReadBufferMemory[requestIndex] = NULL;
         }

         if(NULL != devContext->chatpadReadUrbMemory[requestIndex])
         {
            WdfObjectDelete(devContext->chatpadReadUrbMemory[requestIndex]);
            devContext->chatpadReadUrbMemory[requestIndex] = NULL;
         }
      } // end looping through chatpad filter driver request indices

      deviceCount = WdfCollectionGetCount(chatpadDeviceCollection);
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

         // I added an open count, protected by chatpadDeviceCollectionLock,
         // which means that if the device is unplugged while the user still
         // has a handle open, the device should remain open until it is
         // replugged and unplugged after that point.
         // TODO make the above behavior nicer if possible?  Is it possible
         //   to delete the control device from the close handler?
         if(0 == numOpenHandles)
         {
            ChatpadDeleteControlDevice();
         }
         else
         {
            ChatpadTrace(("The device was not deleted since there are still %lu open handles to it.\n",
                          numOpenHandles));
         }
      }

      // Even if the control device was not deleted, however, we mark it as unavailable since the cable
      // was pulled or whatnot.
      chatpadControlDeviceAvailable = FALSE;

      WdfCollectionRemove(chatpadDeviceCollection, device);

      WdfWaitLockRelease(chatpadDeviceCollectionLock);
   } // end if((NULL != device) && (NULL != devContext))
   else
   {
      // This should never happen.
      ChatpadTrace(("Error, device and/or devContext are NULL while cleaning up.\n"));
   }
} // end ChatpadFilterEvtDeviceContextCleanup

// Creates a control device object so that an application can talk to the filter driver.
// TODO how does this thing link to the regular filter device and its queues?
NTSTATUS
ChatpadCreateControlDevice(
  WDFDEVICE device)
{
   NTSTATUS                retStatus = STATUS_SUCCESS;
   BOOLEAN                 createDevice = FALSE;
   PWDFDEVICE_INIT         deviceInit = NULL;
   WDF_FILEOBJECT_CONFIG   fileObjectConfig;
   WDFDEVICE               newChatpadControlDevice = NULL;
   WDF_OBJECT_ATTRIBUTES   controlAttributes;
   DECLARE_CONST_UNICODE_STRING(ntDeviceName, CHATPAD_FILTER_CONTROL_NAME);
   DECLARE_CONST_UNICODE_STRING(symbolicLinkName, CHATPAD_FILTER_CONTROL_LINK);
   WDFQUEUE                newQueue;
   WDF_IO_QUEUE_CONFIG     ioQueueConfig;

   // Determine whether we should create the device, or if it already exists.
   WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);

// TODO NEXT REMOVE after testing
ChatpadTrace(("Collection device count is %d.\n", WdfCollectionGetCount(chatpadDeviceCollection)));
   // TODO is this a safe way to synchronize things?
   if(NULL == chatpadControlDevice)
   {
      createDevice = TRUE;
   }

   WdfWaitLockRelease(chatpadDeviceCollectionLock);

   if(!createDevice)
   {
      ChatpadTrace(("Chatpad control device was already created.\n"));
   }
   else
   {
      ChatpadTrace(("Creating chatpad control device.\n"));

      // Allocate and initialize a WDFDEVICE_INIT structure for the control device.
      deviceInit = WdfControlDeviceInitAllocate(
        WdfDeviceGetDriver(device),
        // TODO what does this ugly thing govern -- permissions?  Based off of toaster sideband filter example.
        &SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R);
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
        ChatpadControlDeviceCreateHandler,
        ChatpadControlDeviceCloseHandler,
        WDF_NO_EVENT_CALLBACK);

      WdfDeviceInitSetFileObjectConfig(
        deviceInit,
        &fileObjectConfig,
        WDF_NO_OBJECT_ATTRIBUTES);

      if(STATUS_SUCCESS == retStatus)
      {
         WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(
           &controlAttributes,
           CHATPAD_CONTROL_DEVICE_EXTENSION);
         // Create the control device.
         retStatus = WdfDeviceCreate(
           &deviceInit,
           &controlAttributes,
           &newChatpadControlDevice);
      }

      // Initialize control extension.
      PDEVICE_CONTEXT devContext = ChatpadFilterGetDeviceContext(device);
      if(NULL != devContext)
      {
         PCHATPAD_CONTROL_DEVICE_EXTENSION controlExtension =
           ChatpadControlGetData(newChatpadControlDevice);

         if(NULL != controlExtension)
         {
            ChatpadTrace(("Initializing chatpad filter control device extension.\n"));
            controlExtension->chatpadFilterDeviceContext = devContext;
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
           newChatpadControlDevice,
           &symbolicLinkName);
      }

      if(STATUS_SUCCESS == retStatus)
      {
         // Configure default queue so that requests to the control device will be parallel.
         // TODO is this okay?  The idea here is that I want to be able to do read and
         //   write requests at the same time depending on how blocking works.
         WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
           &ioQueueConfig,
           //WdfIoQueueDispatchSequential);
           WdfIoQueueDispatchParallel);

         // Select what we want from the queue.

         // This should catch application-command IOCTLs.
         ioQueueConfig.EvtIoDeviceControl =
           ChatpadFilterIOCTLHandler;

         // Create the queue.
         retStatus = WdfIoQueueCreate(
           newChatpadControlDevice,
           &ioQueueConfig,
           WDF_NO_OBJECT_ATTRIBUTES, // TODO what does this parameter do?
           &newQueue);
      } // end if(STATUS_SUCCESS == retStatus)

      if(STATUS_SUCCESS == retStatus)
      {
         WdfControlFinishInitializing(newChatpadControlDevice);
         WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);
         chatpadControlDevice = newChatpadControlDevice;
         WdfWaitLockRelease(chatpadDeviceCollectionLock);

         ChatpadTrace(("Chatpad control device created successfully.\n"));
      }
      else
      {
         ChatpadTrace(("WdfIoQueueCreate failed:  0x%x\n", retStatus));
      }
   } // end if creating the control device

   // Handle error cases.
   if(STATUS_SUCCESS != retStatus)
   {
      ChatpadTrace(("Failed to create chatpad control device:  0x%x\n", retStatus));

      if(NULL != deviceInit)
      {
         WdfDeviceInitFree(deviceInit);
         deviceInit = NULL;
      }

      if(NULL != newChatpadControlDevice)
      {
         // Release the newly created, but uninitialized, object.
         WdfObjectDelete(newChatpadControlDevice);
         newChatpadControlDevice = NULL;
      }
   }

   return retStatus;
} // end ChatpadCreateControlDevice

// Deletes the control device object.
//
// IMPORTANT NOTE:  This device should only be called if a wait lock is
// currently acquired on chatpadDeviceCollectionLock.
VOID
ChatpadDeleteControlDevice(VOID)
{
   ChatpadTrace(("Deleting chatpad control device.\n"));

   if(chatpadControlDevice)
   {
      WdfObjectDelete(chatpadControlDevice);
      chatpadControlDevice = NULL;
   }
} // end ChatpadDeleteControlDevice

// This function normally runs at DISPATCH IRQL.  It might run at PASSIVE if
// something gets messed up with requests.
VOID
ControlsReadCompletion(
  IN WDFREQUEST                     request,
  IN WDFIOTARGET                    target,
  IN PWDF_REQUEST_COMPLETION_PARAMS params,
  IN WDFCONTEXT                     context)
{
   PDEVICE_CONTEXT   devContext           = reinterpret_cast<PDEVICE_CONTEXT>(context);
   NTSTATUS          apiStatus            = STATUS_SUCCESS;
   WDFMEMORY         urbMemory            = NULL;
   PUCHAR            transferBuffer       = NULL;
   ULONG             bytesTransferred     = 0;
   BOOLEAN           wasRequestCancelled  = FALSE;

   // These variables are used for completing controls data read requests.
   WDFREQUEST        driverRequestFromAbove = NULL;
   WDFREQUEST        userRequestFromAbove = NULL;
   NTSTATUS          driverCompletionStatusForAboveCode  = STATUS_SUCCESS;
   NTSTATUS          userCompletionStatusForAboveCode    = STATUS_SUCCESS;
   BOOLEAN           driverCompleteRequestToAboveCode    = FALSE;
   BOOLEAN           userCompleteRequestToAboveCode      = FALSE;

   ASSERT(devContext);
   ASSERT(params);

//   ChatpadTrace(("Controls read completion callback called with IRQL %u\n", KeGetCurrentIrql()));
//   ChatpadTrace(("I/O status:  0x%x\n", params->IoStatus.Status));

   // Get the status.  If the request was cancelled, then we want to be careful
   // not to complete requests from above.  Otherwise, there was apparently
   // some sort of infinite loop due to the top-level requests.
   // TODO why was this the case?
   if(STATUS_SUCCESS == apiStatus)
   {
      apiStatus = WdfRequestGetStatus(request);
//      ChatpadTrace(("WdfRequestGetStatus returns 0x%x\n", apiStatus));

      if(STATUS_CANCELLED == apiStatus)
      {
         ChatpadTrace(("Controls read request was cancelled.\n"));
         wasRequestCancelled = TRUE;
      }
   }

   // Only do this processing if the request was not cancelled.
   if(FALSE == wasRequestCancelled)
   {
//      ChatpadTrace(("USBD status 0x%x, request type %d\n",
//                    params->Parameters.Usb.Completion->UsbdStatus,
//                    params->Parameters.Usb.Completion->Type));
      PWDF_USB_REQUEST_COMPLETION_PARAMS completionParams =
        params->Parameters.Usb.Completion;
      if(NULL != completionParams)
      {
         if(WdfUsbRequestTypePipeUrb == completionParams->Type)
         {
            urbMemory = completionParams->Parameters.PipeUrb.Buffer;
//            ChatpadTrace(("WDFMEMORY handle is 0x%x\n", urbMemory));
            if(NULL != urbMemory)
            {
               PURB controlsReadUrb =
                 static_cast<PURB>(WdfMemoryGetBuffer(urbMemory, NULL));
               if(NULL != controlsReadUrb)
               {
                  transferBuffer =
                    static_cast<PUCHAR>(controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer);
                  bytesTransferred = controlsReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;
//                  ChatpadTrace(("Buffer address is 0x%x with data length %lu\n", transferBuffer, bytesTransferred));
                  if(NULL != transferBuffer)
                  {
/*
                     ChatpadTrace(("### Buffer data (length %lu):  %02x %02x %02x %02x %02x %02x %02x %02x ... ###\n",
                                   bytesTransferred,
                                   transferBuffer[0],
                                   transferBuffer[1],
                                   transferBuffer[2],
                                   transferBuffer[3],
                                   transferBuffer[4],
                                   transferBuffer[5],
                                   transferBuffer[6],
                                   transferBuffer[7]));
*/

                     // Only forward data to the top-level driver if we are not intercepting the data.
                     if(FILTER_MODE_INTERCEPTED != devContext->filterMode)
                     {
                        apiStatus = WdfIoQueueRetrieveNextRequest(
                          devContext->controlsParkedDriverReadRequestQueue,
                          &driverRequestFromAbove);

                        if(STATUS_SUCCESS == apiStatus)
                        {
                           PURB topLevelUrb =
                             static_cast<PURB>(
                               URB_FROM_IRP(
                                 WdfRequestWdmGetIrp(
                                   driverRequestFromAbove)));

                           if(NULL != topLevelUrb)
                           {
                              RtlCopyMemory(static_cast<unsigned char*>(
                                              topLevelUrb->UrbBulkOrInterruptTransfer.TransferBuffer),
                                            transferBuffer,
                                            bytesTransferred);
                              topLevelUrb->UrbBulkOrInterruptTransfer.TransferBufferLength = bytesTransferred;

                              driverCompleteRequestToAboveCode       = TRUE;
                              driverCompletionStatusForAboveCode     = apiStatus;
                           }
                           else
                           {
                              ChatpadTrace(("Unable to get pointer to URB from top-level driver request.\n"));
                              apiStatus = STATUS_INTERNAL_ERROR;
                           }
                        } // end if(STATUS_SUCCESS == apiStatus)
                        else
                        {
                           ChatpadTrace(("No top-level controls read request available.\n"));
                           apiStatus = STATUS_INTERNAL_ERROR;
                        }
                     } // end if(FILTER_MODE_INTERCEPTED != devContext->filterMode)

                     // Only send data to the user if we are filtering or intercepting the data.
                     if((FILTER_MODE_FILTERED      == devContext->filterMode)    ||
                        (FILTER_MODE_INTERCEPTED   == devContext->filterMode))
                     {
                        PVOID bufferToUserAsVoid   = NULL;
                        PUCHAR bufferToUser        = NULL;

                        apiStatus = WdfIoQueueRetrieveNextRequest(
                          devContext->controlsParkedUserReadRequestQueue,
                          &userRequestFromAbove);
                        if(STATUS_SUCCESS != apiStatus)
                        {
                           ChatpadTrace(("No user-provided controls read request available.\n"));
                        }

                        if(STATUS_SUCCESS == apiStatus)
                        {
//                           ChatpadTrace(("Got a user-provided read request from the queue.\n"));

                           apiStatus = WdfRequestRetrieveOutputBuffer(
                             userRequestFromAbove,
                             bytesTransferred,
                             &bufferToUserAsVoid,
                             NULL);
                           // This ugly cast is probably avoidable, but it works for now.
                           bufferToUser = static_cast<PUCHAR>(bufferToUserAsVoid);
                           if(STATUS_SUCCESS != apiStatus)
                           {
                              ChatpadTrace(("WdfRequestRetrieveOutputBuffer failed:  0x%x\n", apiStatus));
                           }

                           // Just in case, check the pointer.
                           if(NULL == bufferToUser)
                           {
                              ChatpadTrace(("Failed to obtain buffer for writing controls data back to user.\n"));
                              apiStatus = STATUS_INTERNAL_ERROR;
                           }
                        }

                        if(STATUS_SUCCESS == apiStatus)
                        {
                           RtlCopyMemory(bufferToUser,
                                         transferBuffer,
                                         bytesTransferred);

                           userCompleteRequestToAboveCode         = TRUE;
                           userCompletionStatusForAboveCode       = apiStatus;
                        } // end if(STATUS_SUCCESS == apiStatus)
                     } // end if(FILTER_MODE_UNFILTERED != devContext->filterMode)
                  } // end if(NULL != transferBuffer)
                  else
                  {
                     apiStatus = STATUS_INTERNAL_ERROR;
                  }
               } // end if(NULL != controlsReadUrb)
               else
               {
                  apiStatus = STATUS_INTERNAL_ERROR;
               }
            } // end if(NULL != urbMemory)
            else
            {
               apiStatus = STATUS_INTERNAL_ERROR;
            }
         } // end if(WdfUsbRequestTypePipeUrb == completionParams->Type)
         else
         {
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      } // end if(NULL != completionParams)
      else
      {
         ChatpadTrace(("Error, completion parameter pointer is NULL.\n"));
         apiStatus = STATUS_INTERNAL_ERROR;
      }
   } // end if(FALSE == wasRequestCancelled)

   // The request reuse code is called even if an error like STATUS_CANCELLED
   // or STATUS_INTERNAL_ERROR occurred so that the driver can keep operating.
   WDF_REQUEST_REUSE_PARAMS reuseParams;

   WDF_REQUEST_REUSE_PARAMS_INIT(
     &reuseParams,
     WDF_REQUEST_REUSE_NO_FLAGS,
     STATUS_SUCCESS);
   apiStatus = WdfRequestReuse(
     request,
     &reuseParams);
   if(STATUS_SUCCESS != apiStatus)
   {
      ChatpadTrace(("WdfRequestReuse failed:  0x%x\n", apiStatus));
   }

   // Find which request index corresponded to this request, and mark it
   // as available once again.

   int requestIndex = 0;

   // TODO do I need to use synchronization here?
   for(requestIndex = 0;
       requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS;
       requestIndex++)
   {
      if((NULL != devContext) &&
         (request == devContext->controlsReadRequests[requestIndex]))
      {
         // Only mark the request as unused if the filtering mode is unfiltered, since in
         // that case this request is a leftover proxy request and we will not automatically
         // be sending more proxy requests until the filtering mode is changed.
         if(FILTER_MODE_UNFILTERED == devContext->filterMode)
         {
            devContext->controlsReadRequestsInUse[requestIndex] = FALSE;
         }
         break;
      }
   }

   if(CHATPAD_MAX_NUM_PENDING_USB_REQUESTS == requestIndex)
   {
      ChatpadTrace(("Failed to find this request in the chatpad filter driver's controls request array.\n"));
      apiStatus = STATUS_INTERNAL_ERROR;
   }

   // Now that the original request is marked available again, complete a
   // request back to the above code if needed.  Waiting until the
   // previous request is marked available will hopefully prevent any problems
   // that could result from requests coming down so quickly that we do not
   // have time to mark proxy requests as available.
   // TODO should I use a WDFQUEUE to store the proxy requests as well?

   if(TRUE == driverCompleteRequestToAboveCode)
   {
      // Completing with the length as the information here seems to work, but I cannot seem to find
      // in the API or an example where that is specified as actually always happening for IOCTLs.
      // In this case, I am completing with that information for both an URB and an IOCTL request
      // from two different queues.
      // TODO is this safe?
      WdfRequestCompleteWithInformation(driverRequestFromAbove,
                                        driverCompletionStatusForAboveCode,
                                        bytesTransferred);
//      ChatpadTrace(("Sent data back up to top-level driver.\n"));
   }

   if(TRUE == userCompleteRequestToAboveCode)
   {
      // Completing with the length as the information here seems to work, but I cannot seem to find
      // in the API or an example where that is specified as actually always happening for IOCTLs.
      // In this case, I am completing with that information for both an URB and an IOCTL request
      // from two different queues.
      // TODO is this safe?
      WdfRequestCompleteWithInformation(userRequestFromAbove,
                                        userCompletionStatusForAboveCode,
                                        bytesTransferred);
//      ChatpadTrace(("Sent data back up to user code.\n"));
   }

   // Do not try to resend a request if filtering has been disabled, since actual top-level requests
   // will simply be forwarded on in that case and we do not want to use proxy requests.
   if((FILTER_MODE_UNFILTERED != devContext->filterMode)       &&
      (CHATPAD_MAX_NUM_PENDING_USB_REQUESTS != requestIndex)   &&
      (FALSE == wasRequestCancelled))
   {
      // Note that the request is only sent if the original request is not
      // cancelled.  Otherwise, the system crashes when the controller is
      // unplugged.
      SendControlsReadRequest(devContext, requestIndex);
   } // end if(TRUE == completeRequestToAboveCode)

//   ChatpadTrace(("Controls read completion callback finished, with apiStatus == 0x%x\n", apiStatus));
} // end ControlsReadCompletion

NTSTATUS SendChatpadReadRequest(
  IN PDEVICE_CONTEXT devContext,
  IN ULONG           requestIndex);

// This function normally runs at DISPATCH IRQL.  It might run at PASSIVE if
// something gets messed up with requests.
VOID
ChatpadReadCompletion(
  IN WDFREQUEST                     request,
  IN WDFIOTARGET                    target,
  IN PWDF_REQUEST_COMPLETION_PARAMS params,
  IN WDFCONTEXT                     context)
{
   PDEVICE_CONTEXT   devContext        = reinterpret_cast<PDEVICE_CONTEXT>(context);
   NTSTATUS          apiStatus         = STATUS_SUCCESS;
   WDFMEMORY         urbMemory         = NULL;
   PUCHAR            transferBuffer    = NULL;
   ULONG             bytesTransferred  = 0;

   // These variables are used for completing chatpad data read requests back
   // to higher-level code.
   WDFREQUEST        requestFromAbove = NULL;
   NTSTATUS          completionStatusForAboveCode        = STATUS_SUCCESS;
   BOOLEAN           wasRequestCancelled                 = FALSE;
   BOOLEAN           completeRequestToAboveCode          = FALSE;

   ASSERT(devContext);
   ASSERT(params);

//   ChatpadTrace(("Chatpad read completion callback called with IRQL %u\n", KeGetCurrentIrql()));
//   ChatpadTrace(("I/O status:  0x%x\n", params->IoStatus.Status));

   // Get the status.  If the request was cancelled, then we want to be careful
   // not to complete requests from above.  Otherwise, there might apparently
   // be some sort of infinite loop due to higher-level requests.  I am not
   // sure if this would actually happen in this case, but this code seems to
   // work so I am keeping it.
   if(STATUS_SUCCESS == apiStatus)
   {
      apiStatus = WdfRequestGetStatus(request);
//      ChatpadTrace(("WdfRequestGetStatus returns 0x%x\n", apiStatus));

      if(STATUS_CANCELLED == apiStatus)
      {
         ChatpadTrace(("Chatpad read request was cancelled.\n"));
         wasRequestCancelled = TRUE;
      }
   }

   // Only do this processing if the request was not cancelled.
   if(FALSE == wasRequestCancelled)
   {
//      ChatpadTrace(("USBD status 0x%x, request type %d\n",
//                    params->Parameters.Usb.Completion->UsbdStatus,
//                    params->Parameters.Usb.Completion->Type));
      PWDF_USB_REQUEST_COMPLETION_PARAMS completionParams =
        params->Parameters.Usb.Completion;
      if(NULL != completionParams)
      {
         if(WdfUsbRequestTypePipeUrb == completionParams->Type)
         {
            urbMemory = completionParams->Parameters.PipeUrb.Buffer;
//            ChatpadTrace(("WDFMEMORY handle is 0x%x\n", urbMemory));
            if(NULL != urbMemory)
            {
               PURB chatpadReadUrb =
                 static_cast<PURB>(WdfMemoryGetBuffer(urbMemory, NULL));
               if(NULL != chatpadReadUrb)
               {
                  transferBuffer =
                    static_cast<PUCHAR>(chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBuffer);
                  bytesTransferred = chatpadReadUrb->UrbBulkOrInterruptTransfer.TransferBufferLength;
//                  ChatpadTrace(("Buffer address is 0x%x with data length %lu\n", transferBuffer, bytesTransferred));
                  if(NULL != transferBuffer)
                  {
/*
                     ChatpadTrace(("### Buffer data (length %lu):  %02x %02x %02x %02x %02x %02x %02x %02x ... ###\n",
                                   bytesTransferred,
                                   transferBuffer[0],
                                   transferBuffer[1],
                                   transferBuffer[2],
                                   transferBuffer[3],
                                   transferBuffer[4],
                                   transferBuffer[5],
                                   transferBuffer[6],
                                   transferBuffer[7]));
*/

                     PVOID bufferToUserAsVoid   = NULL;
                     PUCHAR bufferToUser        = NULL;

                     apiStatus = WdfIoQueueRetrieveNextRequest(
                       devContext->chatpadParkedUserReadRequestQueue,
                       &requestFromAbove);
                     if(STATUS_SUCCESS != apiStatus)
                     {
                        ChatpadTrace(("No user-provided chatpad read request available.\n"));
                     }

                     if(STATUS_SUCCESS == apiStatus)
                     {
//                        ChatpadTrace(("Got a user-provided read request from the queue.\n"));

                        apiStatus = WdfRequestRetrieveOutputBuffer(
                          requestFromAbove,
                          bytesTransferred,
                          &bufferToUserAsVoid,
                          NULL);
                        // This ugly cast is probably avoidable, but it works for now.
                        bufferToUser = static_cast<PUCHAR>(bufferToUserAsVoid);
                        if(STATUS_SUCCESS != apiStatus)
                        {
                           ChatpadTrace(("WdfRequestRetrieveOutputBuffer failed:  0x%x\n", apiStatus));
                        }

                        // Just in case, check the pointer.
                        if(NULL == bufferToUser)
                        {
                           ChatpadTrace(("Failed to obtain buffer for writing chatpad data back to user.\n"));
                           apiStatus = STATUS_INTERNAL_ERROR;
                        }
                     }

                     if(STATUS_SUCCESS == apiStatus)
                     {
                        RtlCopyMemory(bufferToUser,
                                      transferBuffer,
                                      bytesTransferred);

                        completeRequestToAboveCode          = TRUE;
                        completionStatusForAboveCode        = apiStatus;
                     } // end if(STATUS_SUCCESS == apiStatus)
                  } // end if(NULL != transferBuffer)
                  else
                  {
                     apiStatus = STATUS_INTERNAL_ERROR;
                  }
               } // end if(NULL != chatpadReadUrb)
               else
               {
                  apiStatus = STATUS_INTERNAL_ERROR;
               }
            } // end if(NULL != urbMemory)
            else
            {
               apiStatus = STATUS_INTERNAL_ERROR;
            }
         } // end if(WdfUsbRequestTypePipeUrb == completionParams->Type)
         else
         {
            apiStatus = STATUS_INTERNAL_ERROR;
         }
      } // end if(NULL != completionParams)
      else
      {
         ChatpadTrace(("Error, completion parameter pointer is NULL.\n"));
         apiStatus = STATUS_INTERNAL_ERROR;
      }
   } // end if(FALSE == wasRequestCancelled)

   // The request reuse code is called even if an error like STATUS_CANCELLED
   // or STATUS_INTERNAL_ERROR occurred so that the driver can keep operating.
   WDF_REQUEST_REUSE_PARAMS reuseParams;

   WDF_REQUEST_REUSE_PARAMS_INIT(
     &reuseParams,
     WDF_REQUEST_REUSE_NO_FLAGS,
     STATUS_SUCCESS);
   apiStatus = WdfRequestReuse(
     request,
     &reuseParams);
   if(STATUS_SUCCESS != apiStatus)
   {
      ChatpadTrace(("WdfRequestReuse failed:  0x%x\n", apiStatus));
   }

   // Find which request index corresponded to this request, and mark it
   // as available once again.

   int requestIndex = 0;

   // TODO do I need to use synchronization here?
   for(requestIndex = 0;
       requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS;
       requestIndex++)
   {
      if((NULL != devContext) &&
         (request == devContext->chatpadReadRequests[requestIndex]))
      {
         break;
      }
   }

   if(CHATPAD_MAX_NUM_PENDING_USB_REQUESTS == requestIndex)
   {
      ChatpadTrace(("Failed to find this request in the chatpad filter driver's chatpad request array.\n"));
      apiStatus = STATUS_INTERNAL_ERROR;
   }

   // Now that the original request is marked available again, complete a
   // request back to the above code if needed.  Waiting until the
   // previous request is marked available will hopefully prevent any problems
   // that could result from requests coming down so quickly that we do not
   // have time to mark proxy requests as available.
   // TODO should I use a WDFQUEUE to store the proxy requests as well?

   if(TRUE == completeRequestToAboveCode)
   {
      // Completing with the length as the information here seems to work, but I cannot seem to find
      // in the API or an example where that is specified as actually always happening for IOCTLs.
      // In this case, I am completing with that information for both an URB and an IOCTL request
      // from two different queues.
      // TODO is this safe?
      WdfRequestCompleteWithInformation(requestFromAbove,
                                        completionStatusForAboveCode,
                                        bytesTransferred);
//      ChatpadTrace(("Sent data back up to top-level driver.\n"));
   }

   // According to the WinDDK toastmon example, one should not resend
   // requests here because it can result in recursion if a lower-level
   // driver completes the request synchronously.
   // UPDATE:  According to Tim Roberts this should be okay, it seems.
   if((CHATPAD_MAX_NUM_PENDING_USB_REQUESTS != requestIndex) &&
      (FALSE == wasRequestCancelled))
   {
      // Note that the request is only sent if the original request is not
      // cancelled.  Otherwise, the system crashes when the controller is
      // unplugged.
      SendChatpadReadRequest(devContext, requestIndex);
   } // end if(TRUE == completeRequestToAboveCode)

//   ChatpadTrace(("Chatpad read completion callback finished, with apiStatus == 0x%x\n", apiStatus));
} // end ChatpadReadCompletion

EVT_WDF_REQUEST_COMPLETION_ROUTINE ChatpadFilterSubmitURBCompletion;

// This function, at least for bulk and interrupt requests, runs at DISPATCH IRQL.
VOID
ChatpadFilterSubmitURBCompletion(
  IN WDFREQUEST                     request,
  IN WDFIOTARGET                    target,
  IN PWDF_REQUEST_COMPLETION_PARAMS params,
  IN WDFCONTEXT                     context)
{
   PDEVICE_CONTEXT   devContext        = reinterpret_cast<PDEVICE_CONTEXT>(context);
   ULONG             ioControlCode;

   ASSERT(params);

/*ChatpadTrace(("params->Type == 0x%x  ioctl code == 0x%x\n",
              params->Type,
              params->Parameters.Ioctl.IoControlCode));*/
// TODO why are the parameters invalid?  Invalid unless they're explicitly setup somehow by my own driver?
//      For now, I am simply making this function ONLY get called for completion on IOCTL_INTERNAL_USB_SUBMIT_URB
//      internal IOCTLs, rather than having to figure the parameter stuff out.  I got a BSOD the first time I tried.  :-D
/*   if((WdfRequestTypeDeviceControlInternal == params->Type)    &&
      (IOCTL_INTERNAL_USB_SUBMIT_URB == params->Parameters.Ioctl.IoControlCode))*/
//   ChatpadTrace(("Internal IOCTL completion callback called with IRQL %u\n", KeGetCurrentIrql()));
//   ChatpadTrace(("I/O status:  0x%x\n", params->IoStatus.Status));

   // Get the URB.
   PURB existingURB = static_cast<PURB>(URB_FROM_IRP(WdfRequestWdmGetIrp(request)));

   if(NULL != existingURB)
   {
//      ChatpadTrace(("[completion]  URB function:  %d\n", existingURB->UrbHeader.Function));
      if(URB_FUNCTION_CONTROL_TRANSFER == existingURB->UrbHeader.Function)
      {
         // TODO move this stuff below, seems useless.
         // Interesting, get descriptor data comes back as a control transfer.
         // TODO do I need a different completion routine?  I will if more than one type of control transfer response
         //      needs to be handled.  For now, I am assuming only get config descriptor requests get completion handled
         //      through this code.
         if(153 == existingURB->UrbControlTransfer.TransferBufferLength)
         {
            ChatpadTrace(("[completion]  Received config descriptor.\n"));
         }
         else
         {
            // This doesn't seem to be a get config descriptor response, so we don't know what it is...
            ChatpadTrace(("[completion]  Control transfer:  pipe 0x%x, flags 0x%x, buffer length %lu.\n",
                          existingURB->UrbControlTransfer.PipeHandle,
                          existingURB->UrbControlTransfer.TransferFlags,
                          existingURB->UrbControlTransfer.TransferBufferLength));
         }
      } // end if(URB_FUNCTION_CONTROL_TRANSFER == existingURB->UrbHeader.Function)
      else if(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == existingURB->UrbHeader.Function)
      {
         // TODO do checking against pipe handle?  will it always be constant?
/*
         ChatpadTrace((
           "[completion]  pipe handle 0x%x, %s device, flags 0x%x, buffer length %lu\n",
           existingURB->UrbBulkOrInterruptTransfer.PipeHandle,
           (existingURB->UrbBulkOrInterruptTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN) ? "from" : "to",
           existingURB->UrbBulkOrInterruptTransfer.TransferFlags,
           existingURB->UrbBulkOrInterruptTransfer.TransferBufferLength));
*/
         if(NULL != (existingURB->UrbBulkOrInterruptTransfer.TransferBuffer))
         {
            UCHAR bufferData[80];
            RtlFillMemory(bufferData, 0x00, 80 * sizeof(UCHAR));
            RtlCopyMemory(bufferData,
                          static_cast<unsigned char*>(existingURB->UrbBulkOrInterruptTransfer.TransferBuffer),
                          (existingURB->UrbBulkOrInterruptTransfer.TransferBufferLength));
/*
            ChatpadTrace(("[completion]  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
              bufferData[0],
              bufferData[1],
              bufferData[2],
              bufferData[3],
              bufferData[4],
              bufferData[5],
              bufferData[6],
              bufferData[7],
              bufferData[8],
              bufferData[9],
              bufferData[10],
              bufferData[11],
              bufferData[12],
              bufferData[13],
              bufferData[14],
              bufferData[15],
              bufferData[16],
              bufferData[17],
              bufferData[18],
              bufferData[19]));
*/

            // Detect if the 0x01 0x03 0x06 hex sequence was encountered.  On two
            // presumably different driver versions, on two different OSes,
            // this sequence was detected.  The final sequence on Vista 64
            // was 0x05 0x03 0x00, but this did not show up on the 32-bit
            // XP machine so I am not using that sequence.
            // TODO what does this sequence mean?
            // TODO since someone else on Vista 64 had problems with this sequence working, I'm just ignoring it for now
            //   and marking the controller as initialized as soon as the configuration is selected.
            /*
            if((0x01 == bufferData[0]) &&
               (0x03 == bufferData[1]) &&
               (0x06 == bufferData[2]))
            {
               ChatpadTrace(("End of Microsoft init detected.  Chatpad init can now begin.\n"));
               devContext->msInitFinished = TRUE;
               // Turn filtering off until further notice.
               devContext->filterMode = FILTER_MODE_UNFILTERED;
            }
            */
         } // end if(NULL != (existingURB->UrbBulkOrInterruptTransfer.TransferBuffer))
      } // end if(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == existingURB->UrbHeader.Function)
      else
      {
         ChatpadTrace(("Unknown URB function 0x%x.\n", existingURB->UrbHeader.Function));
      }
   } // end if(NULL != existingURB)
   else
   {
      ChatpadTrace(("[completion] Error, existingURB is NULL.\n"));
   }

   WdfRequestComplete(request, params->IoStatus.Status);
} // end ChatpadFilterSubmitURBCompletion

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

VOID
ChatpadFilterInternalIOCTLHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       outputBufferLength,
  IN size_t       inputBufferLength,
  IN ULONG        ioControlCode)
{
   PIRP                       ioctlIRP                = NULL;
   PDEVICE_CONTEXT            devContext              = NULL;
   NTSTATUS                   apiStatus               = STATUS_SUCCESS;
   BOOLEAN                    messageAlreadyHandled   = FALSE;
   BOOLEAN                    handleCompletion        = FALSE;
   PURB                       existingURB             = NULL;
   WDF_USB_DEVICE_SELECT_CONFIG_PARAMS usbDevConfigurationParams;
   WDF_REQUEST_SEND_OPTIONS   sendOptions;

   //ChatpadTrace(("Internal IOCTL received.\n"));

   // Get device context so we know where to forward the request.
   devContext = ChatpadFilterGetDeviceContext(WdfIoQueueGetDevice(queue));
   // TODO is a failed assertion going to do anything horrible?
   ASSERT(IS_DEVICE_CONTEXT(devContext));

   if(IOCTL_INTERNAL_USB_SUBMIT_URB == ioControlCode)
   {
/*
      ChatpadTrace((
        "ChatpadFilter:  inbuf len %u, outbuf len %u, \n",
        inputBufferLength,
        outputBufferLength));
*/

      // Get the URB.
      existingURB = static_cast<PURB>(URB_FROM_IRP(WdfRequestWdmGetIrp(request)));

      if(NULL != existingURB)
      {
         if(URB_FUNCTION_SELECT_CONFIGURATION == existingURB->UrbHeader.Function)
         {
            ChatpadTrace(("ChatpadFilter:  Selecting configuration.\n"));
            // TODO:  Does WdfUsbTargetDeviceSelectConfig HAVE to be called from the prepare hardware section,
            //        or is that part of the MSDN documentation not strictly true?
            // Use the existing URB.  This presumably means the return
            // information gets properly stored in the URB where the top-level
            // driver can see it.
            // TODO some day I could add some trace statements before WdfUsbTargetDeviceSelectConfig
            //   and make sure that some of the URB values are 0 beforehand and set afterwards?
            WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_URB(&usbDevConfigurationParams, existingURB);
            apiStatus = WdfUsbTargetDeviceSelectConfig(
              devContext->usbDevice,
              WDF_NO_OBJECT_ATTRIBUTES,
              &usbDevConfigurationParams);
            if(STATUS_SUCCESS == apiStatus)
            {
               ChatpadTrace((
                 "ChatpadFilter:  Configuration %u selected.\n",
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue));
               ChatpadTrace((
                 "hdr %u %u %u 0x%x %lu\n",
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Hdr.Length,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Hdr.Function,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Hdr.Status,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Hdr.UsbdDeviceHandle,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Hdr.UsbdFlags));
               ChatpadTrace((
                 "descriptor 0x%x handle 0x%x #interfaces %u configvalue %u interface 0x%x\n",
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.ConfigurationDescriptor,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.ConfigurationHandle,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue,
                 usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface));
/*
   ChatpadTrace((
     "Interface info:  len %u num %u altset %u class %u sub %u prot %u res %u handle 0x%x numpipes %lu\n",
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.Length,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.InterfaceNumber,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.AlternateSetting,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.Class,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.SubClass,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.Protocol,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.Reserved,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.InterfaceHandle,
     usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.NumberOfPipes));
   PUSBD_PIPE_INFORMATION pipeInfo;
   for(ULONG i = 0;
       i < usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.NumberOfPipes;
       i++)
   {
      pipeInfo = &(usbDevConfigurationParams.Types.Urb.Urb->UrbSelectConfiguration.Interface.Pipes[i]);
      ChatpadTrace((
        "Pipe:  mps %u addr %u intv %u type 0x%x handle 0x%x mts %lu flags %lu\n",
        pipeInfo->MaximumPacketSize,
        pipeInfo->EndpointAddress,
        pipeInfo->Interval,
        pipeInfo->PipeType,
        pipeInfo->PipeHandle,
        pipeInfo->MaximumTransferSize,
        pipeInfo->PipeFlags));
   }
*/

               if((usbDevConfigurationParams.Types.Urb.
                     Urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces) >
                  CHATPAD_DRIVER_MAX_SUPPORTED_NUM_INTERFACES)
               {
                  ChatpadTrace(("Error, unsupported number of interfaces:  %u\n",
                                usbDevConfigurationParams.Types.Urb.Urb->
                                  UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces));
                  apiStatus = STATUS_INTERNAL_ERROR;
               }

               if(STATUS_SUCCESS == apiStatus)
               {
                  ULONG_PTR curInterfacePointerOffset = 0;
                  WDFUSBINTERFACE curInterface = NULL;

                  // Reset the pipe count to zero before enumerating pipes,
                  // just in case they have been enumerated before.
                  devContext->numPipes = 0;

                  // Release all saved interface information, if any, so we can save new information.
                  ReleaseCachedInterfaceInformation(devContext);
                  for(UCHAR interfaceIndex = 0;
                      ((STATUS_SUCCESS == apiStatus) &&
                       (interfaceIndex < usbDevConfigurationParams.Types.Urb.Urb->
                                           UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces));
                      interfaceIndex++)
                  {
                     // Copy the USBD_INTERFACE_INFORMATION information so we
                     // can handle URB_FUNCTION_SELECT_INTERFACE later, by
                     // simply returning the saved information (since the
                     // controller presumably does not need any alternate
                     // settings selected).  The data is copied instead of just
                     // saving a pointer here, in case the structure from the
                     // top-level driver is freed at some point.

                     // Allocate enough memory to store the structure.  The tag
                     // used is 'Bliz' backwards, and hopefully no one else
                     // uses that tag.  :P Note that single quotes are
                     // apparently used here.
                     PUSBD_INTERFACE_INFORMATION
                        originalUsbdInterfaceInformation =
                       reinterpret_cast<PUSBD_INTERFACE_INFORMATION>
                         (reinterpret_cast<PUCHAR>(&(usbDevConfigurationParams.Types.Urb.Urb->
                                                       UrbSelectConfiguration.Interface)) +
                          curInterfacePointerOffset);
                     if(NULL == originalUsbdInterfaceInformation)
                     {
                        ChatpadTrace(("Error trying to get original usbd interface information.\n"));
                        apiStatus = STATUS_INTERNAL_ERROR;
                     }

                     if(STATUS_SUCCESS == apiStatus)
                     {
                        // TODO if possible, figure out the right combination of magic keywords
                        // (see http://msdn.microsoft.com/en-us/library/ff549130%28VS.85%29.aspx
                        //  and http://flylib.com/books/en/3.141.1.177/1/) to get rid of the OACR
                        //  warning about possibly leaking memory.  This memory should be freed
                        //  later in a ReleaseCachedInterfaceInformation() call.
                        devContext->usbdInterfaceInformation[interfaceIndex] =
                          static_cast<PUSBD_INTERFACE_INFORMATION>(
                            ExAllocatePoolWithTagWrapper(
                              PagedPool,
                              originalUsbdInterfaceInformation->Length,
                              'zilB'));
                        if(NULL == devContext->usbdInterfaceInformation[interfaceIndex])
                        {
                           ChatpadTrace(("Error allocating memory for interface information.\n"));
                           apiStatus = STATUS_INTERNAL_ERROR;
                        }
                     } // end if(STATUS_SUCCESS == apiStatus)

                     if(STATUS_SUCCESS == apiStatus)
                     {
                        // Note that the pointer is reinterpreted as a PUCHAR so the pointer arithmetic will work properly.
                        // TODO will this cause problems on 32- vs. 64-bit systems?
                        RtlCopyMemory(
                          devContext->usbdInterfaceInformation[interfaceIndex],
                          originalUsbdInterfaceInformation,
                          originalUsbdInterfaceInformation->Length);

                        // Update the pointer offset to get information on the next interface, if any.
                        curInterfacePointerOffset += (devContext->usbdInterfaceInformation[interfaceIndex])->Length;

                        if(NULL != devContext->usbdInterfaceInformation[interfaceIndex])
                        {
                           ChatpadTrace(("Interface with index %u, number %u, and %u pipes\n",
                                         interfaceIndex,
                                         (devContext->usbdInterfaceInformation[interfaceIndex])->InterfaceNumber,
                                         (devContext->usbdInterfaceInformation[interfaceIndex])->NumberOfPipes));
                        }
                        else
                        {
                           // This should never happen as long as the pointer is dynamically allocated
                           // and checked above, but it is left in here anyway in case other code is
                           // ever changed, to make this code robust.
                           ChatpadTrace(("Error, interface pointer is NULL.\n"));
                           apiStatus = STATUS_INTERNAL_ERROR;
                        }
                     } // end if(STATUS_SUCCESS == apiStatus)

                     // Save pointers for all endpoint pipes.
                     if(STATUS_SUCCESS == apiStatus)
                     {
                        curInterface =
                          WdfUsbTargetDeviceGetInterface(devContext->usbDevice, interfaceIndex);
                        if(NULL == curInterface)
                        {
                           ChatpadTrace(("WdfUsbTargetDeviceGetInterface failed.\n"));
                           apiStatus = STATUS_INTERNAL_ERROR;
                        }
                     } // end if(STATUS_SUCCESS == apiStatus)

                     if(STATUS_SUCCESS == apiStatus)
                     {
                        BYTE selectedSettingIndex =
                          WdfUsbInterfaceGetConfiguredSettingIndex(curInterface);
                        BYTE numEndpoints = WdfUsbInterfaceGetNumEndpoints(curInterface, selectedSettingIndex);
                        ChatpadTrace(("The interface with index %u has alternate setting %u selected, and %u endpoints.\n",
                                      interfaceIndex,
                                      selectedSettingIndex,
                                      numEndpoints));
                        BYTE numConfiguredPipes = WdfUsbInterfaceGetNumConfiguredPipes(curInterface);
                        ChatpadTrace(("The interface also has %u pipes configured.\n",
                                      numConfiguredPipes));
                        if(numEndpoints != numConfiguredPipes)
                        {
                           ChatpadTrace(("Error, the number of endpoints does not equal the number of configured pipes.\n"));
                           apiStatus = STATUS_INTERNAL_ERROR;
                        }

                        for(UCHAR pipeIndex = 0;
                            ((STATUS_SUCCESS == apiStatus) &&
                             (pipeIndex < numConfiguredPipes));
                            pipeIndex++)
                        {
                           // Get pipe information.
                           WDF_USB_PIPE_INFORMATION pipeInfo;

                           WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
                           WDFUSBPIPE curPipe =
                             WdfUsbInterfaceGetConfiguredPipe(curInterface, pipeIndex, &pipeInfo);
                           if(NULL != curPipe)
                           {
                              // Tell the framework that it is okay to read
                              // (and write?) less than the maximum packet
                              // size.
                              WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(curPipe);
                              // TODO is it okay to set this for all pipes?
                              // Save the WDF pipe information.
                              WDF_USB_PIPE_INFORMATION_INIT(&(devContext->wdfUsbPipeInfo[pipeIndex]));
                              RtlCopyMemory(
                                &(devContext->wdfUsbPipeInfo[pipeIndex]),
                                &pipeInfo,
                                pipeInfo.Size);
                              // Save the WDF pipe handle.
                              devContext->wdfUsbPipe[pipeIndex] = curPipe;
                              devContext->numPipes++;

                              ChatpadTrace(("Pipe endpoint address %u, handle 0x%x",
                                            devContext->wdfUsbPipeInfo[pipeIndex].EndpointAddress,
                                            devContext->wdfUsbPipe[pipeIndex]));
                           }
                           else
                           {
                              ChatpadTrace(("WdfUsbInterfaceGetConfiguredPipe failed.\n"));
                              apiStatus = STATUS_INTERNAL_ERROR;
                           }
                        } // end looping through configured pipes
                     } // end if(STATUS_SUCCESS == apiStatus)
                  } // end looping through interfaces
               } // end if(STATUS_SUCCESS == apiStatus)

               // Finish initializing the URB for the write request.

               if(STATUS_SUCCESS == apiStatus)
               {
                  PURB controlsWriteUrb =
                    static_cast<PURB>(WdfMemoryGetBuffer(devContext->controlsWriteUrbMemory, NULL));
                  // TODO create enums or some other method for indexing interfaces and pipes?
                  if((NULL != devContext->usbdInterfaceInformation[0])                       &&
                     (NULL != devContext->usbdInterfaceInformation[0]->Pipes[1].PipeHandle))
                  {
                     controlsWriteUrb->UrbBulkOrInterruptTransfer.PipeHandle =
                       devContext->usbdInterfaceInformation[0]->Pipes[1].PipeHandle;
                     ChatpadTrace(("For writing controls data, using USBD handle 0x%x\n",
                                   controlsWriteUrb->UrbBulkOrInterruptTransfer.PipeHandle));
                  }
                  else
                  {
                     ChatpadTrace(("Failed to get controls data interface pointer and pipe handle.\n"));
                     apiStatus = STATUS_INTERNAL_ERROR;
                  }
               } // end if(STATUS_SUCCESS == apiStatus)

               // Finish initializing the URBs for read requests.

               for(int requestIndex = 0;
                   ((STATUS_SUCCESS == apiStatus) &&
                    (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
                   requestIndex++)
               {
                  PURB controlsReadUrb =
                    static_cast<PURB>(WdfMemoryGetBuffer(devContext->controlsReadUrbMemory[requestIndex], NULL));
                  // TODO create enums or some other method for indexing interfaces and pipes?
                  if((NULL != devContext->usbdInterfaceInformation[0])                       &&
                     (NULL != devContext->usbdInterfaceInformation[0]->Pipes[0].PipeHandle))
                  {
                     controlsReadUrb->UrbBulkOrInterruptTransfer.PipeHandle =
                       devContext->usbdInterfaceInformation[0]->Pipes[0].PipeHandle;
                     ChatpadTrace(("For reading controls data, using USBD handle 0x%x\n",
                                   controlsReadUrb->UrbBulkOrInterruptTransfer.PipeHandle));
                  }
                  else
                  {
                     ChatpadTrace(("Failed to get controls data interface pointer and pipe handle.\n"));
                     apiStatus = STATUS_INTERNAL_ERROR;
                  }

                  PURB chatpadReadUrb =
                    static_cast<PURB>(WdfMemoryGetBuffer(devContext->chatpadReadUrbMemory[requestIndex], NULL));
                  // TODO create enums or some other method for indexing interfaces and pipes?
                  if((NULL != devContext->usbdInterfaceInformation[2])                       &&
                     (NULL != devContext->usbdInterfaceInformation[2]->Pipes[0].PipeHandle))
                  {
                     chatpadReadUrb->UrbBulkOrInterruptTransfer.PipeHandle =
                       devContext->usbdInterfaceInformation[2]->Pipes[0].PipeHandle;
                     ChatpadTrace(("For reading chatpad data, using USBD handle 0x%x\n",
                                   chatpadReadUrb->UrbBulkOrInterruptTransfer.PipeHandle));
                  }
                  else
                  {
                     ChatpadTrace(("Failed to get chatpad data interface pointer and pipe handle.\n"));
                     apiStatus = STATUS_INTERNAL_ERROR;
                  }
               } // end looping through chatpad filter driver request indices

               // TODO eventually I might properly detect when the Microsoft driver has finished
               //   initializing the controller.  Since these messages seem to vary between
               //   systems/hardware, I am just setting it to TRUE for now as soon as
               //   the USB device configuration is selected.
               devContext->msInitFinished = TRUE;
               // Turn filtering off until further notice.
               devContext->filterMode = FILTER_MODE_UNFILTERED;

               devContext->usbConfigSelected = TRUE;
            } // end if(STATUS_SUCCESS == apiStatus)
            else
            {
               ChatpadTrace(("WdfUsbTargetDeviceSelectConfig failed:  0x%x\n", apiStatus));
            }
            WdfRequestComplete(request, apiStatus);
            messageAlreadyHandled = TRUE;
         } // end if(URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE == existingURB->UrbHeader.Function)
         else if(URB_FUNCTION_SELECT_INTERFACE == existingURB->UrbHeader.Function)
         {
            UCHAR interfaceNumber = existingURB->UrbSelectInterface.Interface.InterfaceNumber;
            UCHAR numInterfaces = WdfUsbTargetDeviceGetNumInterfaces(devContext->usbDevice);
            WDFUSBINTERFACE usbInterface = NULL;
            UCHAR interfaceIndex = 0;

            ChatpadTrace(("ChatpadFilter:  Selecting interface setting for interface number %u.\n", interfaceNumber));

            if(TRUE == devContext->usbConfigSelected)
            {
               // Just in case interface number is ever not the same as
               // interface index, I am doing this the annoying way by manually
               // searching through the interfaces.
               for(interfaceIndex = 0; interfaceIndex < numInterfaces; interfaceIndex++)
               {
                  if(interfaceNumber == (devContext->usbdInterfaceInformation[interfaceIndex])->InterfaceNumber)
                  {
                     // IMPORTANT NOTE:  For now, I am not calling
                     // WdfUsbInterfaceSelectSetting, since the original
                     // WdfUsbTargetDeviceSelectConfig call should use
                     // alternate setting 0 for all interfaces by default.
                     //
                     // If WdfUsbInterfaceSelectSetting is called, and the
                     // official XBox 360 controller driver is trying to use
                     // a non-zero setting (which it does not, as far as I
                     // know), then calling WdfUsbInterfaceSelectSetting
                     // might in theory delete or change interface and/or pipe
                     // data that has been cached, without filling in the
                     // URB to get new handles.  So, right now the filter
                     // driver just saves the original interface data and
                     // passes it back up.
                     //
                     // An alternative would be ditching the Wdf API and using
                     // annoying lower-level interfaces.  If there is a way to
                     // fill in the select setting URB for the return trip
                     // properly using Wdf, that would be great, but I do not
                     // know one.

                     if(NULL != devContext->usbdInterfaceInformation[interfaceIndex])
                     {
                        // TODO NEXT COMMENT OUT BELOW
                        ChatpadTrace(("Found interface to copy with index %u, number %u, and %u pipes\n",
                                      interfaceIndex,
                                      (devContext->usbdInterfaceInformation[interfaceIndex])->InterfaceNumber,
                                      (devContext->usbdInterfaceInformation[interfaceIndex])->NumberOfPipes));
                        // Copy saved interface information into the URB.
                        RtlCopyMemory(
                          &(existingURB->UrbSelectInterface.Interface),
                          devContext->usbdInterfaceInformation[interfaceIndex],
                          devContext->usbdInterfaceInformation[interfaceIndex]->Length);

                        ChatpadTrace((
                          "hdr %u %u %u 0x%x %lu\n",
                          existingURB->UrbSelectInterface.Hdr.Length,
                          existingURB->UrbSelectInterface.Hdr.Function,
                          existingURB->UrbSelectInterface.Hdr.Status,
                          existingURB->UrbSelectInterface.Hdr.UsbdDeviceHandle,
                          existingURB->UrbSelectInterface.Hdr.UsbdFlags));
                        ChatpadTrace((
                          "handle 0x%x interface 0x%x\n",
                          existingURB->UrbSelectInterface.ConfigurationHandle,
                          existingURB->UrbSelectInterface.Interface));
                        ChatpadTrace((
                          "Interface info:  len %u num %u altset %u class %u sub %u\n",
                          existingURB->UrbSelectInterface.Interface.Length,
                          existingURB->UrbSelectInterface.Interface.InterfaceNumber,
                          existingURB->UrbSelectInterface.Interface.AlternateSetting,
                          existingURB->UrbSelectInterface.Interface.Class,
                          existingURB->UrbSelectInterface.Interface.SubClass));
                        ChatpadTrace(("  prot %u res %u handle 0x%x numpipes %lu\n",
                          existingURB->UrbSelectInterface.Interface.Protocol,
                          existingURB->UrbSelectInterface.Interface.Reserved,
                          existingURB->UrbSelectInterface.Interface.InterfaceHandle,
                          existingURB->UrbSelectInterface.Interface.NumberOfPipes));
                        PUSBD_PIPE_INFORMATION pipeInfo;
                        for(ULONG i = 0;
                            i < existingURB->
                                  UrbSelectInterface.Interface.NumberOfPipes;
                            i++)
                        {
                           pipeInfo = &(existingURB->
                                          UrbSelectInterface.Interface.Pipes[i]);
                           ChatpadTrace((
                             "Pipe:  mps %u addr %u intv %u type 0x%x handle 0x%x mts %lu flags %lu\n",
                             pipeInfo->MaximumPacketSize,
                             pipeInfo->EndpointAddress,
                             pipeInfo->Interval,
                             pipeInfo->PipeType,
                             pipeInfo->PipeHandle,
                             pipeInfo->MaximumTransferSize,
                             pipeInfo->PipeFlags));
                        }

                        // Stop searching since the interface was found.
                        break;
                     } // end if(NULL != devContext->usbdInterfaceInformation[interfaceIndex])
                     else
                     {
                        ChatpadTrace(("Error, cached interface pointer is NULL.\n"));
                        apiStatus = STATUS_INTERNAL_ERROR;
                     }
                  } // end if interface number matches
               } // end for(interfaceIndex = 0; interfaceIndex < numInterfaces; interfaceIndex++)

               // Fail if the interface number was not found.
               if(interfaceIndex == numInterfaces)
               {
                  ChatpadTrace(("Error, could not find interface with number %d.\n", interfaceNumber));
                  apiStatus = STATUS_INTERNAL_ERROR;
               }

               // Complete the request, now that the URB has the interface information filled in.
               // (If there was a failure or something, apiStatus should not be STATUS_SUCCESS
               //  at this point anyway, so completing the request here should not result in
               //  a BSOD.)
               ChatpadTrace(("Status at end of handling select interface is 0x%x.\n", apiStatus));
               WdfRequestComplete(request, apiStatus);
               messageAlreadyHandled = TRUE;
            } // end if(TRUE == devContext->usbConfigSelected)
            else
            {
               ChatpadTrace(("Error, device not already configured.\n"));
               apiStatus = STATUS_INTERNAL_ERROR;
            }
         } // end if(URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE == existingURB->UrbHeader.Function)
// TODO NEXT:  Finish implementing chatpad read functionality in driver.
// TODO NEXT:  Implement chatpad read functionality up through userspace.
// TODO NEXT:  Implement something like the original chatpad app if possible, with 1f/1e timed loop.
// TODO NEXT:  Test automatic controller code detection, to support plain wired 360 controllers, in theory.
// TODO NEXT:  Make nice installer package that ideally includes all files for x86, amd64, and ia64,
//             for XP, Vista, and Windows 7.
// TODO NEXT:  Include an explicit source code license and reference it from all .cpp, .h, etc. files.
         else if(URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE == existingURB->UrbHeader.Function)
         {
            ChatpadTrace((
              "ChatpadFilter:  Getting descriptor (type 0x%x) from device.\n",
              existingURB->UrbControlDescriptorRequest.DescriptorType));
            // Set up the completion handler for this packet.
            //handleCompletion = TRUE;
         } // end if(URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE == existingURB->UrbHeader.Function)
         else if(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == existingURB->UrbHeader.Function)
         {
//ChatpadTrace((".")); // TODO remove if desired, this is just for testing timing in DebugView
            if(TRUE == devContext->usbConfigSelected)
            {
               int numOutstandingProxyRequests = 0;
               int requestIndex = 0;

               // Count the number of outstanding proxy requests.
               for(requestIndex = 0;
                   ((STATUS_SUCCESS == apiStatus) &&
                    (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
                   requestIndex++)
               {
                  // TODO do I need to use synchronization here?
                  if(TRUE == devContext->controlsReadRequestsInUse[requestIndex])
                  {
                     numOutstandingProxyRequests++;
                  } // if(TRUE == devContext->controlsReadRequestsInUse[requestIndex])
               } // end looping through chatpad filter driver request indices

               // Only support completion handling if filtering or intercepting controls data,
               // or if there are still outstanding proxy controls data read requests.
               if((FILTER_MODE_FILTERED      == devContext->filterMode)    ||
                  (FILTER_MODE_INTERCEPTED   == devContext->filterMode)    ||
                  (numOutstandingProxyRequests > 0))
               {
                  if(existingURB->UrbBulkOrInterruptTransfer.PipeHandle ==
                     devContext->usbdInterfaceInformation[0]->Pipes[0].PipeHandle)
                  {
                     // We park driver-provided controls data read requests in a
                     // manual queue so that we can complete them later.  This
                     // apparently lets the framework automatically handle
                     // cancellation for us, so the driver should not freeze when
                     // it is upgraded or the computer shuts down.

                     // NOTE:  Apparently we do not need to format the request here to forward it to the queue.
                     // TODO research why that is the case?
                     apiStatus = WdfRequestForwardToIoQueue(request, devContext->controlsParkedDriverReadRequestQueue);
                     if(STATUS_SUCCESS != apiStatus)
                     {
                        ChatpadTrace(("Failed to forward request:  0x%x\n", apiStatus));
                        apiStatus = STATUS_INTERNAL_ERROR;
                     }

                     // Initialize and send a read request to the controls data endpoint.
                     if((FILTER_MODE_FILTERED      == devContext->filterMode)    ||
                        (FILTER_MODE_INTERCEPTED   == devContext->filterMode))
                     {
                        for(requestIndex = 0;
                            ((STATUS_SUCCESS == apiStatus) &&
                             (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
                            requestIndex++)
                        {
                           // TODO do I need to use synchronization here?
                           if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
                           {
                              devContext->controlsReadRequestsInUse[requestIndex] = TRUE;
                              apiStatus = SendControlsReadRequest(devContext, requestIndex);
                              break;
                           } // if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
                        } // end looping through chatpad filter driver request indices
                     } // end if filtering or intercepting controls data

                     // For now, I am always treating the message as handled even
                     // if I do not have new read requests to send, since there
                     // may already be pending read requests.
                     // TODO could this ever cause problems?
                     messageAlreadyHandled = TRUE;
   /*
                     if(requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS)
                     {
                        messageAlreadyHandled = TRUE;
                     }
                     else
                     {
                        ChatpadTrace(("Error, failed to find an available proxy request to send.\n"));
                        apiStatus = STATUS_INTERNAL_ERROR;
                     }
   */
                  } // end if this is a read request to the controls data pipe
                  else
                  {
   /*
                     ChatpadTrace((
                       "ChatpadFilter:  pipe handle 0x%x, %s device, buffer length %lu\n",
                       existingURB->UrbBulkOrInterruptTransfer.PipeHandle,
                       (existingURB->UrbBulkOrInterruptTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN) ? "from" : "to",
                       existingURB->UrbBulkOrInterruptTransfer.TransferBufferLength));
   */
                     if(NULL != (existingURB->UrbBulkOrInterruptTransfer.TransferBuffer))
                     {
                        // Set up the completion handler for this packet.
                        handleCompletion = TRUE;

                        UCHAR bufferData[80];
                        RtlFillMemory(bufferData, 0x00, 80 * sizeof(UCHAR));
                        RtlCopyMemory(bufferData,
                                      static_cast<unsigned char*>(existingURB->UrbBulkOrInterruptTransfer.TransferBuffer),
                                      (existingURB->UrbBulkOrInterruptTransfer.TransferBufferLength));
   /*
                        ChatpadTrace(("%02x %02x %02x %02x %02x %02x %02x %02x ...\n",
                                     bufferData[0],
                                     bufferData[1],
                                     bufferData[2],
                                     bufferData[3],
                                     bufferData[4],
                                     bufferData[5],
                                     bufferData[6],
                                     bufferData[7]));
   */
                     }
                  } // end if this is not a read request to the controls data pipe
               } // end if filtering or intercepting controls data, or if there are
                 // outstanding proxy controls data read requests
            } // end if(TRUE == devContext->usbConfigSelected)
            else
            {
               ChatpadTrace(("Error, received a bulk or interrupt transfer but the device is not yet configured.\n"));
               apiStatus = STATUS_INTERNAL_ERROR;
            }
         } // end if(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER == existingURB->UrbHeader.Function)
         else if(URB_FUNCTION_CONTROL_TRANSFER == existingURB->UrbHeader.Function)
         {
            ChatpadTrace(("Control transfer detected.\n"));
            // TODO print out details on control transfers for informational purposes?
         }
         else
         {
            ChatpadTrace(("Unknown URB function 0x%x.\n", existingURB->UrbHeader.Function));
         }
      } // end if(NULL != existingURB)
   } // end if(IOCTL_INTERNAL_USB_SUBMIT_URB == ioControlCode)
   else
   {
      ChatpadTrace(("Unknown control code 0x%x.\n", ioControlCode));
   }

   if(!messageAlreadyHandled)
   {
      // Forward the request unmodified.
      WdfRequestFormatRequestUsingCurrentType(request);

      if(!handleCompletion)
      {
         WDF_REQUEST_SEND_OPTIONS_INIT(
           &sendOptions,
           WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

         // Try to forward the request.
         if(!WdfRequestSend(request,
                            devContext->targetForRequests,
                            &sendOptions))
         {
            apiStatus = WdfRequestGetStatus(request);
            ChatpadTrace(("WdfRequestSend failed:  0x%x\n", apiStatus));
            // Just complete the request ourselves since something went horribly wrong.
            WdfRequestComplete(request, apiStatus);
         }
      } // end if not handling completion tracking
      else
      {
         if(NULL != existingURB)
         {
            // Set up completion routine.
            WdfRequestSetCompletionRoutine(
              request,
              ChatpadFilterSubmitURBCompletion,
              devContext);
            // Try to forward the request.
            if(!WdfRequestSend(request,
                               devContext->targetForRequests,
                               WDF_NO_SEND_OPTIONS))
            {
               apiStatus = WdfRequestGetStatus(request);
               ChatpadTrace(("WdfRequestSend failed:  0x%x\n", apiStatus));
               WdfRequestComplete(request, apiStatus);
            }
         }
         else
         {
            ChatpadTrace(("Error, non-URB internal IOCTLs not supported for completion tracking.\n"));
         }
      } // end if handling completion tracking
   } // end if(!messageAlreadyHandled)

   return;
} // end ChatpadFilterInternalIOCTLHandler

// This function runs at PASSIVE IRQL.
VOID
ChatpadFilterIOCTLHandler(
  IN WDFQUEUE     queue,
  IN WDFREQUEST   request,
  IN size_t       outputBufferLength,
  IN size_t       inputBufferLength,
  IN ULONG        ioControlCode)
{
   PIRP                       ioctlIRP                            = NULL;
   ULONG                      numDriverInstances                  = 0;
   // Device context for a particular filter driver instance.
   PDEVICE_CONTEXT            devContext                          = NULL;
   NTSTATUS                   apiStatus                           = STATUS_SUCCESS;
   PVOID                      dataFromUserAsVoid                  = NULL;
   PUCHAR                     dataFromUser                        = NULL;
   PVOID                      dataFromDeviceBuffer                = NULL;
   // The following pointer can point at either a receive buffer or a send buffer.
   PVOID                      controlTransferExtraBufferPointer   = NULL;
   ULONG                      controlTransferExtraBufferLength    = 0;
   ULONG                      bytesTransferred                    = 0;
   BOOLEAN                    messageAlreadyHandled               = FALSE;
   BOOLEAN                    immediatelyFailRequest              = FALSE;

//   ChatpadTrace(("The IRQL in the filter IOCTL handler is %u.\n", KeGetCurrentIrql()));

   WdfWaitLockAcquire(chatpadDeviceCollectionLock, NULL);

   if(TRUE == chatpadControlDeviceAvailable)
   {
      // Get the number of filter driver instances.
      numDriverInstances = WdfCollectionGetCount(chatpadDeviceCollection);
      // Get device context.
      if(numDriverInstances >= 1)
      {
         devContext = ChatpadFilterGetDeviceContext(WdfCollectionGetItem(chatpadDeviceCollection, 0));
         // TODO is a failed assertion going to do anything horrible in a kernel driver?
         ASSERT(IS_DEVICE_CONTEXT(devContext));
      }
   }
   else
   {
      // If there is no control device available, then we want to return immediately to avoid
      // crashing.  This situation can happen if a user program has the driver open
      // and the device is suddenly unplugged.
      immediatelyFailRequest = TRUE;
   }

   WdfWaitLockRelease(chatpadDeviceCollectionLock);

   if(TRUE == immediatelyFailRequest)
   {
      ChatpadTrace(("Ignoring control device IOCTL since the device is unavailable.\n"));
      WdfRequestCompleteWithInformation(request, STATUS_INTERNAL_ERROR, 0);
      return;
   }

   if(FALSE == devContext->usbConfigSelected)
   {
      ChatpadTrace(("IOCTL received for filter driver, but the USB device is not yet configured.\n"));
      apiStatus = STATUS_INTERNAL_ERROR;
   }
   else if(IOCTL_CHATPAD_IS_MS_INIT_DONE == ioControlCode)
   {
      if(FALSE == devContext->msInitFinished)
      {
         ChatpadTrace(("Error, Microsoft initialization may not have finished yet.\n"));
         apiStatus = STATUS_INTERNAL_ERROR;
      }
      else
      {
         ChatpadTrace(("Microsoft initialization seems to be finished.\n"));
      }
   } // end if(IOCTL_CHATPAD_IS_MS_INIT_DONE == ioControlCode)
   else if(FALSE == devContext->msInitFinished)
   {
      ChatpadTrace(("IOCTL received for filter driver, but Microsoft initialization does not seem to be finished yet.\n"));
      apiStatus = STATUS_INTERNAL_ERROR;
   }
   else if(IOCTL_CHATPAD_SEND_CONTROL_TRANSFER == ioControlCode)
   {
      // Make sure the user provided us with enough data.
      // We require at least 9 bytes.  (TODO make that a symbolic constant)
      if(inputBufferLength >= 9)
      {
//         ChatpadTrace(("Input buffer length is %u.\n", inputBufferLength));
      }
      else
      {
         ChatpadTrace(("Error, input buffer length is only %u.\n", inputBufferLength));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      // Get output buffer pointer (to return data back from the device), if a buffer is provided.
      if((STATUS_SUCCESS == apiStatus) &&
         (outputBufferLength > 0))
      {
         // Make sure the user provided us with a buffer that is large enough
         // for a reasonably large return message.  We require at least 32
         // bytes since the 360 controller should not return more than that
         // amount.
         // TODO Make the 32 a symbolic constant.
         // TODO Research the behavior if a device returns more data than is
         //   available in a buffer?  Would an error simply be returned?
         if(outputBufferLength >= 32)
         {
            ChatpadTrace(("Output buffer length is %u.\n", outputBufferLength));
         }
         else
         {
            ChatpadTrace(("Error, output buffer length is only %u.\n", outputBufferLength));
            apiStatus = STATUS_INTERNAL_ERROR;
         }

         if(STATUS_SUCCESS == apiStatus)
         {
            apiStatus = WdfRequestRetrieveOutputBuffer(request, outputBufferLength, &dataFromDeviceBuffer, NULL);
            if((STATUS_SUCCESS != apiStatus) || (NULL == dataFromDeviceBuffer))
            {
               ChatpadTrace(("WdfRequestRetrieveOutputBuffer failed:  0x%x\n", apiStatus));
            }
         }
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
/*
         ChatpadTrace(("In theory, we should send a control transfer to interface %u.\n", dataFromUser[0]));
         ChatpadTrace(("Raw data to send:  0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                       dataFromUser[1],
                       dataFromUser[2],
                       dataFromUser[3],
                       dataFromUser[4],
                       dataFromUser[5],
                       dataFromUser[6],
                       dataFromUser[7],
                       dataFromUser[8]));
*/

         // TODO eventually print out serial numbers, and/or use them to identify the
         //      proper device?  For now, we always default to the first one.

         // TODO is it safe to do something outside the lock here?

//         ChatpadTrace(("Sending control transfer synchronously to interface index 0.\n"));

         // Actually send the control transfer.
         WDF_USB_CONTROL_SETUP_PACKET setupPacket;
         // Copy starting at the byte past the interface number.
         RtlCopyMemory(setupPacket.Generic.Bytes, &(dataFromUser[1]), 8);
         if(0x80 == (0x80 & dataFromUser[1]))
         {
            // In this case, we should receive data if any is available.
            if(outputBufferLength > 0)
            {
               controlTransferExtraBufferPointer = dataFromDeviceBuffer;
               controlTransferExtraBufferLength = outputBufferLength;
            }
         }
         else
         {
            // In this case, we should send data if any is available.
            if(inputBufferLength > 9)
            {
               controlTransferExtraBufferPointer = &(dataFromUser[9]);
               controlTransferExtraBufferLength = inputBufferLength - 9;
            }
         }
         // Note that we can just use a local memory structure since the
         // control transfer is sent synchronously.
         WDF_MEMORY_DESCRIPTOR memoryDescriptor;
         WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(
           &memoryDescriptor,
           controlTransferExtraBufferPointer,
           controlTransferExtraBufferLength);

         WDF_REQUEST_SEND_OPTIONS sendOptions;

         // Set up a timeout.
         WDF_REQUEST_SEND_OPTIONS_INIT(
           &sendOptions,
           WDF_REQUEST_SEND_OPTION_TIMEOUT);
         sendOptions.Timeout = WDF_REL_TIMEOUT_IN_SEC(CHATPAD_DRIVER_SYNCHRONOUS_REQUEST_TIMEOUT_LENGTH);
         // TODO should I create the control transfer request myself, so that I can call
         //   WdfRequestAllocateTimer()?  Hopefully there is not really a need for that.

         // Even without specifying a specific interface, this just seems to work.  Yay.
         apiStatus = WdfUsbTargetDeviceSendControlTransferSynchronously(
           devContext->usbDevice,
           WDF_NO_HANDLE,
           &sendOptions,
           &setupPacket,
           &memoryDescriptor,
           &bytesTransferred);
         if(!NT_SUCCESS(apiStatus))
         {
            ChatpadTrace(("Control transfer failed:  0x%x\n", apiStatus));
            ChatpadTrace(("(buf 0x%x  buflen %lu  numbytes %lu)\n",
                          controlTransferExtraBufferPointer,
                          controlTransferExtraBufferLength,
                          bytesTransferred));
         }
         else
         {
//            ChatpadTrace(("Control transfer succeeded with %lu additional bytes transferred.\n",
//                          bytesTransferred));
         }
      } // end if(STATUS_SUCCESS == apiStatus)
   } // end else if(IOCTL_CHATPAD_SEND_CONTROL_TRANSFER == ioControlCode)
   else if(IOCTL_CHATPAD_WRITE_TO_CONTROLS_ENDPOINT == ioControlCode)
   {
      if(inputBufferLength > 0)
      {
//         ChatpadTrace(("Input buffer length is %u.\n", inputBufferLength));
      }
      else
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
         ChatpadTrace(("Writing data synchronously to controls endpoint.\n"));

         apiStatus = SendControlsWriteRequest(devContext, dataFromUser, inputBufferLength, &bytesTransferred);
      } // end if(STATUS_SUCCESS == apiStatus)
   } // end else if(IOCTL_CHATPAD_WRITE_TO_CONTROLS_ENDPOINT == ioControlCode)
   else if(IOCTL_CHATPAD_READ_FROM_CONTROLS_ENDPOINT == ioControlCode)
   {
      // Note that if controls data is not currently being intercepted, this
      // request will go into a queue but simply sit there forever until
      // interception mode becomes active or the driver is shut down.

      // We park user-provided controls data read requests in a manual queue so
      // that we can complete them later.  This apparently lets the framework
      // automatically handle cancellation for us, so the driver should not
      // freeze when it is upgraded or the computer shuts down.

      // NOTE:  Apparently we do not need to format the request here to forward it to the queue.
      // TODO research why that is the case?
      apiStatus = WdfRequestForwardToIoQueue(request, devContext->controlsParkedUserReadRequestQueue);
      if(STATUS_SUCCESS == apiStatus)
      {
//         ChatpadTrace(("Forwarded user-provided controls endpoint read request to a waiting queue.\n"));
      }
      else
      {
         ChatpadTrace(("Failed to forward request:  0x%x\n", apiStatus));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      int requestIndex = 0;

      // Initialize and send a read request to the controls data endpoint.
      for(requestIndex = 0;
          ((STATUS_SUCCESS == apiStatus) &&
           (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
          requestIndex++)
      {
         // TODO do I need to use synchronization here?
         if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
         {
            devContext->controlsReadRequestsInUse[requestIndex] = TRUE;
            apiStatus = SendControlsReadRequest(devContext, requestIndex);
            break;
         } // if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
      } // end looping through chatpad filter driver request indices

      // This makes the request NOT be completed in this function.  If it was,
      // then when the device is removed (or perhaps when data comes through)
      // the request would be completed for a second time causing a system
      // crash.  The request should only be completed when data becomes
      // available or when it is cancelled.
      messageAlreadyHandled = TRUE;

      // For now, I am always treating the message as handled even
      // if I do not have new read requests to send, since there
      // may already be pending read requests.
      // TODO could this practice ever cause problems?
   } // end else if(IOCTL_CHATPAD_READ_FROM_CONTROLS_ENDPOINT == ioControlCode)
   else if(IOCTL_CHATPAD_READ_FROM_CHATPAD_ENDPOINT == ioControlCode)
   {
      // We park user-provided chatpad data read requests in a manual queue so
      // that we can complete them later.  This apparently lets the framework
      // automatically handle cancellation for us, so the driver should not
      // freeze when it is upgraded or the computer shuts down.

      // NOTE:  Apparently we do not need to format the request here to forward it to the queue.
      // TODO research why that is the case?
      apiStatus = WdfRequestForwardToIoQueue(request, devContext->chatpadParkedUserReadRequestQueue);
      if(STATUS_SUCCESS == apiStatus)
      {
//         ChatpadTrace(("Forwarded user-provided chatpad endpoint read request to a waiting queue.\n"));
      }
      else
      {
         ChatpadTrace(("Failed to forward request:  0x%x\n", apiStatus));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      int requestIndex = 0;

      // Initialize and send a read request to the chatpad data endpoint.
      for(requestIndex = 0;
          ((STATUS_SUCCESS == apiStatus) &&
           (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
          requestIndex++)
      {
         // TODO do I need to use synchronization here?
         if(FALSE == devContext->chatpadReadRequestsInUse[requestIndex])
         {
            devContext->chatpadReadRequestsInUse[requestIndex] = TRUE;
            apiStatus = SendChatpadReadRequest(devContext, requestIndex);
            break;
         } // if(FALSE == devContext->chatpadReadRequestsInUse[requestIndex])
      } // end looping through chatpad filter driver request indices

      // This makes the request NOT be completed in this function.  If it was,
      // then when the device is removed (or perhaps when data comes through)
      // the request would be completed for a second time causing a system
      // crash.  The request should only be completed when data becomes
      // available or when it is cancelled.
      messageAlreadyHandled = TRUE;

      // For now, I am always treating the message as handled even
      // if I do not have new read requests to send, since there
      // may already be pending read requests.
      // TODO could this practice ever cause problems?
   } // end else if(IOCTL_CHATPAD_READ_FROM_CHATPAD_ENDPOINT == ioControlCode)
   else if(IOCTL_CHATPAD_SET_CONTROLS_MAPPINGS == ioControlCode)
   {
ChatpadTrace(("TODO NEXT implement setting controls mappings.\n"));
   } // end else if(IOCTL_CHATPAD_SET_CONTROLS_MAPPINGS == ioControlCode)
   else if(IOCTL_CHATPAD_SET_CONTROLS_FILTER_MODE == ioControlCode)
   {
      // TODO add constant for how long the buffer should be
      if(inputBufferLength != 1)
      {
         ChatpadTrace(("Error, set controls filter mode input buffer length is %u.\n", inputBufferLength));
         apiStatus = STATUS_INTERNAL_ERROR;
      }

      // Get input buffer data.
      apiStatus = WdfRequestRetrieveInputBuffer(request, inputBufferLength, &dataFromUserAsVoid, NULL);
      // This ugly cast is probably avoidable, but it works for now.
      dataFromUser = static_cast<PUCHAR>(dataFromUserAsVoid);
      if((STATUS_SUCCESS != apiStatus) || (NULL == dataFromUser))
      {
         ChatpadTrace(("WdfRequestRetrieveInputBuffer failed:  0x%x\n", apiStatus));
      }

      if((FILTER_MODE_UNFILTERED    == dataFromUser[0])  ||
         (FILTER_MODE_FILTERED      == dataFromUser[0])  ||
         (FILTER_MODE_INTERCEPTED   == dataFromUser[0]))
      {
         devContext->filterMode = dataFromUser[0];
         ChatpadTrace(("Changed filter mode to %u.\n", devContext->filterMode));

         if(FILTER_MODE_INTERCEPTED != devContext->filterMode)
         {
            // If any available controls read requests are not already pending,
            // then send them.  This is done because the top-level driver
            // should always have read requests pending, so we can just
            // start completing them as soon as we get data.
            for(int requestIndex = 0;
                ((STATUS_SUCCESS == apiStatus) &&
                 (requestIndex < CHATPAD_MAX_NUM_PENDING_USB_REQUESTS));
                requestIndex++)
            {
               // TODO do I need synchronization here?
               if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
               {
                  devContext->controlsReadRequestsInUse[requestIndex] = TRUE;
                  apiStatus = SendControlsReadRequest(devContext, requestIndex);
               } // if(FALSE == devContext->controlsReadRequestsInUse[requestIndex])
            } // end looping through chatpad filter driver request indices
         } // end if(FILTER_MODE_INTERCEPTED != devContext->filterMode)
      } // end if new filter mode is valid
      else
      {
         ChatpadTrace(("Error, invalid filter mode %u.\n", dataFromUser[0]));
         apiStatus = STATUS_INTERNAL_ERROR;
      } // end handling case where new filter mode is not valid
   } // end else if(IOCTL_CHATPAD_SET_CONTROLS_FILTER_MODE == ioControlCode)
   else
   {
      ChatpadTrace(("Unknown ioctl control code 0x%x.\n", ioControlCode));
      // TODO does this status make the driver/device die, or just return an error
      //   to DeviceIoControl?
      apiStatus = STATUS_INTERNAL_ERROR;
   }

   if(FALSE == messageAlreadyHandled)
   {
      // Completing with the length as the information here seems to work, but I cannot seem to find
      // in the API or an example where that is specified as actually always happening for IOCTLs.
      // TODO find that information at some point?
      WdfRequestCompleteWithInformation(request, apiStatus, bytesTransferred);
   }

   return;
} // end ChatpadFilterIOCTLHandler

