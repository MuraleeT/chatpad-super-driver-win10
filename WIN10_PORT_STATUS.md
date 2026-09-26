# Windows 10 Port Status

This is a personal working port of the original Xbox 360 Chatpad Super Driver.
The upstream project was published on Google Code and targeted Windows 7/KMDF
1.9. It is not a direct fork of a GitHub repository.

## Completed

- Extracted `source_release_0_0_4a.zip`.
- Added modern Visual Studio/WDK project files for all five amd64 driver components.
- Built and test-signed the filter, keyboard, keyboard KMDF, mouse, and mouse KMDF drivers.
- Removed legacy `__DATE__` and `__TIME__` compiler usage.
- Initialized device/request handles before conditional creation.
- Replaced unsafe IRP stack pointer arithmetic with `IoGetNextIrpStackLocation`.
- Replaced `ExAllocatePoolWithTag` wrappers with `ExAllocatePool2`.
- Changed the filter device-add path to return real initialization failures.
- Generated and verified Windows 10 test catalogs.

## Current Blocker

The rebuilt filter still caused a blue screen during reboot on the physical
Windows 10 machine. The crash is therefore in the filter's runtime request,
power, cancellation, or device-removal behavior, not package signing or INF
selection.

Do not reinstall the filter on the physical machine until the crash is analyzed
in a VM or from a kernel crash dump. The keyboard and mouse packages should stay
uninstalled during filter debugging.

## Next Investigation

1. Test in a Windows 10 VM with USB passthrough and a snapshot.
2. Enable Driver Verifier only for the Chatpad filter binary.
3. Reproduce the reboot/plug-unplug failure.
4. Analyze the dump in WinDbg, focusing on URB completion, request reuse,
   `D0Exit`, surprise removal, and control-device lifetime.
5. Rebuild and repeat before packaging a keyboard-only release.
