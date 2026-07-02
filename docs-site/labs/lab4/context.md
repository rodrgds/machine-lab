# Lab 4 Context: Why Pointing Devices Matter

The mouse changed personal computing because it made direct manipulation
practical. Windows, menus, drawing tools, games, graphical editors, and
desktop-style file managers all depend on continuous pointing input.

At the controller level, a mouse is a stream of packets. The packet format is
small, but it contains several ideas that recur in protocols: synchronization
bits, signed movement fields, button state, overflow flags, and explicit
enable/disable commands. Unlike a keyboard scancode, a mouse packet also
describes movement. The program must decide how relative deltas affect a cursor,
camera, selection box, or game object.

## From Keyboard To Mouse

The mouse still goes through the i8042-style controller in this lab, but the
data model is richer than keyboard scancodes. A keyboard event can be one or two
bytes. A mouse event is usually a packet whose bytes are meaningful only as a
group.

This makes Lab 4 a bridge between input and graphics. You are not required to
draw a full pointer UI yet, but the parser you write should be good enough for a
final project to build one. Keep the low-level packet decoder independent from
screen coordinates so that later applications can choose their own coordinate
system, sensitivity, and clipping behavior.
