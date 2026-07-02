# How It Works Today

Modern audio systems are usually built around DMA, ring buffers, mixers, and
operating-system audio servers. Applications rarely write directly to a
hardware buffer and then command the device in the style of a tiny lab runtime.

## What Changed

Applications often talk to APIs such as WASAPI, Core Audio, ALSA, PulseAudio,
PipeWire, SDL audio, or browser Web Audio. The operating system mixes multiple
applications, handles device changes, negotiates formats, and tries to balance
latency with power use and stability.

Professional audio software may care about very low latency and stable buffer
scheduling. Everyday applications often care more about compatibility and not
glitching when the system is busy. Both concerns are built on the same basic
fact: audio is time-sensitive sample data.

## What Stayed Useful

The lab still teaches sample-rate and duration conversion, byte counts versus
sample counts, buffer ownership, clipping and amplitude, and deterministic audio
artifacts for tests. The music maker example applies the same idea at a higher
level: application state becomes PCM samples.
