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
