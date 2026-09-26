# Chatpad Porting References

These references were identified during the Windows 10 port investigation.

## Xbox 360

- [xbox360wirelesschatpad](https://github.com/Kytech/xbox360wirelesschatpad)
  - User-mode Xbox 360 Chatpad protocol reference.
  - Investigate `src/` for packet polling, accessory-bus state, and key translation.
  - This is the preferred architectural direction for Windows 10: user-mode USB
    communication plus a virtual HID/input mechanism, avoiding a custom kernel
    filter for packet parsing.
- [chatpad-super-driver](https://github.com/GAFBlizzard/chatpad-super-driver)
  - Historical source corresponding to this workspace.
  - Useful for protocol details and configuration behavior, but its kernel filter
    caused a Windows 10 bug check during reboot testing.

## Xbox One and Later

- [xone](https://github.com/medusalix/xone)
  - Active Linux reference for Xbox GIP and Xbox One Chatpad handling.
  - `driver/chatpad.c` covers accessory initialization and input translation.
  - `driver/gip.c` and `driver/gip.h` document GIP framing and routing.
- [xonedo](https://github.com/OpenGamingCollective/xonedo)
  - Community-maintained xone-related reference.

The xone repositories target Xbox One/Series GIP hardware and are not a direct
implementation for this Xbox 360 USB controller. They may still clarify the
difference between the 360 accessory bus and later GIP multiplexing.

## Recommended Next Architecture

Do not continue installing the custom kernel filter on the physical Windows 10
machine. Treat the legacy driver as a protocol reference and investigate:

1. User-mode WinUSB/libusb access to the wired Xbox 360 controller.
2. Packet parsing and Chatpad accessory initialization in a desktop process.
3. Keyboard event output through a virtual HID or another supported user-mode
   input mechanism.
4. A small signed helper only if Windows requires one for the selected virtual
   HID approach.

User-mode parsing keeps malformed packets and state-machine bugs from causing a
system-wide kernel bug check.

## Historical Windows 10 Report

A community report described a 64-bit Windows 10 installation where the
Chatpad backlight worked but keyboard input did not. Removing and reinstalling
the drivers reportedly restored typing; rumble reportedly remained unsupported.
This is an anecdotal result and was not reproduced here. It does not establish
that the legacy kernel filter is safe on current Windows 10 builds; this
workspace's rebuilt filter still caused a blue screen during reboot testing.

The report used driver-signature workarounds. The correctly spelled BCDEdit
setting is:

```bat
bcdedit /set loadoptions DISABLE_INTEGRITY_CHECKS
bcdedit /set testsigning on
```

`DDISABLE_INTEGRITY_CHECKS` is a typo. Do not add the integrity-checks
workaround to the normal installation guide: Test Mode alone is preferable for
controlled development, and neither setting makes an unsafe kernel driver
compatible with Windows 10. Disable Test Mode and restore normal boot settings
after testing.
