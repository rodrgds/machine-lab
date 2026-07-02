# Reactive Graphical Applications

A graphical program is usually an event loop:

```text
while running:
  wait for timer/input events
  update application state
  draw current state
  present frame
```

The final project should not draw randomly from many places. It should have a
clear state model and a rendering pass. Input changes the state. Time advances
the state. Rendering reads the state and draws a frame. This sounds like a
style preference, but it quickly becomes a correctness issue once projects have
menus, animations, moving objects, text, sound, and replay scripts.

## A Useful Separation

Device helpers should own scancodes, mouse packets, timer ticks, framebuffer
writes, and audio buffers. Application state should own menus, entities,
cursors, documents, or song patterns. The renderer should draw that state into
pixels. Replay and test code should provide deterministic input timelines.

When those responsibilities are mixed together, every change becomes risky. A
keyboard shortcut should not know how a sprite is clipped. A rectangle drawing
helper should not know which game level is active. A replay script should not
depend on real human typing speed.

## Why This Matters

This separation makes projects easier to test, record, debug, and divide among
teammates. It also makes final demos more stable. A reactive graphical program
should be able to run from live input or from a scripted input trace and produce
the same visible behavior.
