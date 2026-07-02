# Ninjix

Ninjix is the larger game port included with Machine Lab. It came from a
previous Minix-style final project and is useful because it shows that the labs
lead to real application structure, not only isolated device functions.

![Ninjix menu screenshot](/assets/screenshots/ninjix.png)

## What It Exercises

- keyboard and mouse input;
- framebuffer rendering;
- menus, overlays, panels, and HUD text;
- audio hooks;
- profiling and frame timing;
- reusable device libraries;
- a UART multiplayer protocol;
- paired runtime execution.

## Why It Matters

Small examples prove one device at a time. Ninjix proves that the device pieces
can support a full reactive program:

- input is translated into application events;
- rendering is separated from game state;
- timers drive progression;
- serial messages coordinate two hosted programs;
- deterministic smoke scripts make a large project testable.

## Run It

```sh
machinelab run build/examples/ninjix
```

For multiplayer:

```sh
machinelab run-pair build/examples/ninjix --right build/examples/ninjix
```

In interactive multiplayer, select Multiplayer in both windows, then choose
opposite ATTACK and DEFEND roles.

For a fast deterministic smoke test:

```sh
machinelab run-pair --headless build/examples/ninjix \
  --right build/examples/ninjix --max-ticks 900
```

For a longer authored demo:

```sh
machinelab run --headless --script scripts/ninjix_level1_demo.mlabscript \
  --dump-frame build/ninjix.ppm -- build/examples/ninjix
```

## Teaching Notes

Ninjix is a good final-project reference when students ask:

- how large should a state machine be?
- where should input parsing stop and application logic begin?
- how should rendering helpers be organized?
- what does a serial protocol look like inside a game?
- how can a project remain demoable and testable?

It should not be treated as the starter template. It is evidence of the target
scale, not the minimum expected project.
