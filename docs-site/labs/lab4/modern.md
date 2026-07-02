# How It Works Today

Modern pointing devices usually use USB HID, Bluetooth HID, touchpads, or
integrated input controllers rather than PS/2 packet streams.

## What Changed

Applications often receive higher-level pointer events that already contain
position, button state, scroll wheels, gestures, touch contacts, pressure, or
device identity. A desktop application may never see the raw packet format that
the physical device used internally.

Operating systems also apply policy above the raw device layer. Pointer
acceleration, coordinate transformation, palm rejection, gesture recognition,
and compositor routing usually happen before application code receives an event.

## What Stayed Useful

Packet thinking still matters. Input arrives incrementally, synchronization can
be lost, relative movement differs from absolute position, and low-level parsing
should be separated from UI policy. These lessons apply to modern gamepad input,
tablet reports, network messages, and recorded replay streams.

Machine Lab keeps the PS/2-shaped mouse because it is small enough to inspect
while still being rich enough to teach synchronization and signed deltas. Modern
input stacks add comfort and capability, but they do not remove the need for
careful parsing at the layer that receives device reports.
