# How It Works Today

Modern keyboard stacks usually go through USB HID, Bluetooth HID, operating
system input layers, keymaps, input methods, and accessibility services before
applications see input.

## What Changed

Applications usually receive key events from APIs such as SDL, browser events,
Cocoa, Win32, X11, Wayland, or terminal input modes. Those APIs often translate
device reports into richer events that include logical keys, text input,
modifier state, repeat behavior, focus, and layout-dependent characters.

## What Stayed Useful

The low-level lessons remain. Byte streams need careful decoding, command
responses must not be confused with data, and low-level input should be
separated from UI behavior. These ideas apply whether the report came from PS/2,
USB HID, Bluetooth, a terminal emulator, or a virtual input backend.

Machine Lab uses the older keyboard shape because it is visible enough to teach.
Modern stacks are more capable, especially for international text and
accessibility, but they are also harder to inspect end to end in a first systems
course.
