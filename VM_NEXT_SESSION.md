# Xbox Chatpad Windows 10 VM Handoff

This document is the complete handoff for continuing the Xbox 360 Chatpad Windows 10 investigation in a virtual machine.

## Project

This workspace began as the `chatpad_0_0_3a_win7` binary release of GAFBlizzard's Xbox 360 Chatpad Super Driver. The original project targeted Windows 7 and used old KMDF 1.9-era code.

The original upstream project was hosted on Google Code:

- https://code.google.com/p/chatpad-super-driver/

The personal GitHub repository is:

- https://github.com/MuraleeT/chatpad-super-driver-win10
- Branch: `win10-port`
- Latest documented commit: `e8135e5`

The workspace was imported into Git because the original archive was not a Git checkout. Generated build output, the local test certificate, catalogs, and the staged test package are excluded by `.gitignore`.

## Hardware

- Wired Xbox 360 controller for Windows.
- Xbox 360 Chatpad attached to the controller.
- Expected controller hardware ID:

```text
USB\VID_045E&PID_028E
```

- The known physical controller instance used during testing was:

```text
USB\VID_045E&PID_028E\09F2B2B
```

The legacy driver is for a wired controller. Wireless controller support is a separate problem.

## What Happened

1. The original `chatpad_installer_amd64.exe` failed with `0xE000022F`.
2. Inspection showed the original package had no active catalog declaration and no valid driver signature.
3. Windows 10 rejected the package as unsigned.
4. The original INF only targeted Windows 7 sections and used a 2010 driver date.
5. Windows Test Mode and a local test certificate were used for investigation.
6. The original filter was installed on physical Windows 10 and caused a blue screen during reboot.
7. The old packages were removed and Test Mode was disabled.
8. The source archive was found in `source_release_0_0_4a.zip`.
9. Modern WDK project files were added for all five driver components.
10. The rebuilt drivers compiled and test-signed.
11. The rebuilt filter initially installed but was outranked by Microsoft's `xusb22.inf` because its INF still had a 2010 `DriverVer`.
12. The filter INF was updated to version `1.0.1.5` dated `09/26/2026`; it then became `Best Ranked / Installed`.
13. The rebuilt filter again caused a blue screen during reboot on the physical Windows 10 installation.
14. The physical machine was cleaned up. Do not test the filter on the physical machine again.

The most likely remaining defect is in runtime USB request, cancellation, power-transition, surprise-removal, or control-device lifetime handling. Signing and INF ranking were separate issues and are not the root cause of the crash.

## Source Layout

Useful source is under:

```text
source_release_0_0_4a/source_release_0_0_4a/
```

Components:

- `filter/` - Xbox controller USB filter and control device.
- `chatpad_keyboard_kmdf/` - virtual keyboard KMDF layer.
- `hid_keyboard_interface/` - virtual keyboard HID interface.
- `chatpad_mouse_kmdf/` - virtual mouse KMDF layer.
- `hid_mouse_interface/` - virtual mouse HID interface.
- `include/` - IOCTL headers.
- `installer/` - old installer source.
- `chatpad_control/` - user-mode control utility source.

Modern project files added:

- `filter/chatpad_filter.vcxproj`
- `chatpad_keyboard_kmdf/chatpad_keyboard_kmdf.vcxproj`
- `hid_keyboard_interface/chatpad_keyboard.vcxproj`
- `chatpad_mouse_kmdf/chatpad_mouse_kmdf.vcxproj`
- `hid_mouse_interface/chatpad_mouse.vcxproj`

## Source Changes Already Made

The port contains these changes:

- Removed legacy `__DATE__` and `__TIME__` compiler usage.
- Initialized `WDFDEVICE` and `WDFREQUEST` handles before conditional creation.
- Replaced raw IRP stack pointer arithmetic with `IoGetNextIrpStackLocation`.
- Replaced `ExAllocatePoolWithTag` wrappers with `ExAllocatePool2` using non-paged pool.
- Changed filter device-add to return its real initialization status instead of always returning success.
- Added Windows 10 INF sections.
- Added catalog declarations.
- Updated the rebuilt filter `DriverVer` to `09/26/2026,1.0.1.5`.

These changes are not sufficient to prove runtime safety. The next debugging target is the filter's request lifecycle and power/remove handling.

## Toolchain

The successful local build used:

- Visual Studio 2026 Community.
- Windows SDK `10.0.28000.2526`.
- WDK `10.0.28000.2526`.
- MSBuild:

```text
D:\Visual Studio 2026\18\Community\MSBuild\Current\Bin\MSBuild.exe
```

- WDK `Inf2Cat`:

```text
C:\Program Files (x86)\Windows Kits\10\bin\10.0.28000.0\x86\Inf2Cat.exe
```

- SDK/WDK `signtool` used locally:

```text
C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe
```

.NET and `dotnet add package` are not required. These are native WDK/MSBuild projects.

## Build Commands

Open a Developer Command Prompt for Visual Studio 2026, then build each project:

```bat
set MSBUILD=D:\Visual Studio 2026\18\Community\MSBuild\Current\Bin\MSBuild.exe

"%MSBUILD%" source_release_0_0_4a\source_release_0_0_4a\filter\chatpad_filter.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64
"%MSBUILD%" source_release_0_0_4a\source_release_0_0_4a\chatpad_keyboard_kmdf\chatpad_keyboard_kmdf.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64
"%MSBUILD%" source_release_0_0_4a\source_release_0_0_4a\hid_keyboard_interface\chatpad_keyboard.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64
"%MSBUILD%" source_release_0_0_4a\source_release_0_0_4a\chatpad_mouse_kmdf\chatpad_mouse_kmdf.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64
"%MSBUILD%" source_release_0_0_4a\source_release_0_0_4a\hid_mouse_interface\chatpad_mouse.vcxproj /m /t:Build /p:Configuration=Debug /p:Platform=x64
```

Outputs are written to:

```text
build\Debug\x64\
```

Warnings about duplicate WDK imports are present in the generated projects but did not prevent compilation or signing. Clean those project imports later if desired.

## Research References

See `RESEARCH_REFERENCES.md` for links and architectural notes.

Important references:

- https://github.com/Kytech/xbox360wirelesschatpad - Xbox 360 user-mode protocol reference.
- https://github.com/GAFBlizzard/chatpad-super-driver - historical driver source.
- https://github.com/medusalix/xone - Xbox One/Series GIP reference; not a direct Xbox 360 implementation.
- https://github.com/OpenGamingCollective/xonedo - related GIP reference.

The recommended long-term architecture is a user-mode WinUSB/libusb parser with virtual keyboard output. Avoid custom kernel packet parsing if possible.

## VM Setup

Use VMware or VirtualBox for USB passthrough. Hyper-V is less convenient for passing through a USB controller.

Recommended VM:

- Windows 10 x64, preferably fully patched.
- Take a clean snapshot before installing any Chatpad driver.
- Install Visual Studio/WDK only if rebuilding inside the VM is needed; building can normally happen on the host.
- Pass through the wired Xbox 360 controller USB device.
- Keep the controller disconnected until the snapshot is ready.
- Never test first on the physical host.

The VM should be disposable. If a driver causes a bug check, revert the snapshot instead of repairing the VM manually.

## Driver Verifier Plan

Do not enable Driver Verifier on the physical host. In the VM, after the test certificate is trusted and Test Mode is enabled, identify the installed Chatpad filter binary and enable verification only for that binary.

Use the graphical `verifier.exe` interface if possible:

1. Create standard settings.
2. Select specific driver names.
3. Select only `chatpad_filter.sys`.
4. Reboot.
5. Reproduce controller plug/unplug, reboot, and Chatpad initialization.
6. If the VM bug-checks, collect the dump from `C:\Windows\Minidump`.
7. Disable Verifier from Safe Mode or Recovery with:

```bat
verifier /reset
```

Analyze the dump with WinDbg. The useful commands are:

```text
!analyze -v
lmvm chatpad_filter
kv
!wdfkd.wdflogdump chatpad_filter
```

The investigation should focus on USB URB completion, request reuse, cancellation, `D0Exit`, surprise removal, and control-device lifetime.

## Keyboard-Only Goal

The practical target is Chatpad keyboard input. Mouse movement from the controller is not required.

For future testing:

- Test the filter alone first.
- Then test the keyboard package.
- Leave `chatpad_mouse.inf` uninstalled.
- Do not run the original `chatpad_installer_amd64.exe` against the rebuilt package.

## Test Signing Notes

The local test certificate used previously was `chatpad-test.cer` with SHA-1 thumbprint:

```text
985A8E0AC5CC4C2BCC8C9F8A9AB99FB84CE726C7
```

The certificate was intentionally excluded from Git. Create or import a test certificate inside the VM instead.

For controlled VM testing only:

```bat
bcdedit /set testsigning on
shutdown /r /t 0
```

After testing:

```bat
bcdedit /set testsigning off
shutdown /r /t 0
```

Do not use `DISABLE_INTEGRITY_CHECKS` as a normal workaround. It weakens boot integrity and does not make an unsafe driver safe.

## Recovery

If the VM will not boot:

1. Revert to the clean VM snapshot, or use Windows Recovery Command Prompt.
2. Disable Test Mode:

```bat
bcdedit /set {default} testsigning off
```

3. In Safe Mode or an Administrator Command Prompt, remove the published Chatpad package reported by `pnputil /enum-drivers`:

```bat
pnputil /delete-driver oem###.inf /uninstall /force
sc.exe delete ChatpadFilter
```

Never guess the `oem###.inf` number on a different VM; enumerate it first.

## Current Stop Point

The personal GitHub branch contains the source port and documentation, but no driver package is considered safe for normal Windows 10 use. The next session should start with a VM snapshot and crash-dump instrumentation, not another physical-machine installation.
