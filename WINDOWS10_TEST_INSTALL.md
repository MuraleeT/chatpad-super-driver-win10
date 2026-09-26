# Windows 10 Test Installation

This package has been prepared for Windows 10 x64 Test Mode.

Completed:

- Added Windows 10 amd64 and x86 sections to `chatpad_filter.inf`.
- Added catalog declarations to the filter, keyboard, and mouse INFs.
- Generated `chatpad_filter.cat`, `chatpad_keyboard.cat`, and `chatpad_mouse.cat`.
- Test-signed the amd64 driver binaries and catalogs.
- Created the local test certificate `chatpad-test.cer`.
- Verified all eight amd64 driver and catalog signatures.

Only the `amd64` driver files are signed. Use these instructions on 64-bit Windows 10.

## Before Restarting

Open **Command Prompt as Administrator** and run:

```bat
bcdedit /set testsigning on
```

The command should report:

```text
The operation completed successfully.
```

Restart Windows:

```bat
shutdown /r /t 0
```

Windows should display `Test Mode` after restarting. Secure Boot may need to be disabled if Test Mode does not activate.

## After Restarting

Open **Command Prompt as Administrator** and run these commands:

```bat
cd /d D:\xboxchatpad\chatpad_0_0_3a_win7
certutil -addstore -f Root chatpad-test.cer
certutil -addstore -f TrustedPublisher chatpad-test.cer
pnputil /add-driver chatpad_filter.inf /install
pnputil /add-driver chatpad_keyboard.inf /install
```

The mouse package is optional and is not needed for Chatpad keyboard input.
Leave `chatpad_mouse.inf` uninstalled.

Connect the Xbox 360 controller and chatpad before installing the filter driver. If the original installer is still needed for device creation, run this as Administrator after the commands above:

```bat
chatpad_installer_amd64.exe
```

## Disable Test Mode

When finished testing, open **Command Prompt as Administrator** and run:

```bat
bcdedit /set testsigning off
shutdown /r /t 0
```

This package uses a local test certificate. It is suitable for local testing only and is not a production Microsoft-signed driver package.

## Rebuilt Driver Test Package

The rebuilt package is in `win10-test-package`. It contains drivers built with
the Windows 10 target and modern WDK tooling. The package passed Inf2Cat and
all five amd64 driver signatures plus three catalog signatures were verified.

Do not install it until the old packages have been removed. Test the filter
first, then add the keyboard package after a stable reboot. Leave the mouse
package uninstalled.

From an Administrator Command Prompt:

```bat
bcdedit /set testsigning on
shutdown /r /t 0
```

After reboot, install the test certificate and filter package:

```bat
cd /d D:\xboxchatpad\chatpad_0_0_3a_win7
certutil -addstore -f Root chatpad-test.cer
certutil -addstore -f TrustedPublisher chatpad-test.cer
cd /d D:\xboxchatpad\chatpad_0_0_3a_win7\win10-test-package
pnputil /add-driver chatpad_filter.inf /install
```

Reboot and observe the system before installing any other rebuilt driver. If
the machine blue-screens, boot Safe Mode and remove the published Chatpad INF
reported by `pnputil /enum-drivers`:

```bat
pnputil /delete-driver oem###.inf /uninstall /force
bcdedit /set testsigning off
shutdown /r /t 0
```

D:\Visual Studio 2026\18\Community\Common7\Tools