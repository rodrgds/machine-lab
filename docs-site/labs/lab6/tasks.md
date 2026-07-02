# Lab 6 Tasks

Work in `labs/audio/`. The goal is to produce a valid audio buffer and hand it
to the runtime with the correct byte count. Treat the buffer as device-facing
memory: bounds, format, and units matter.

## Requested Functions

- `int audio_map_buffer(void)`
- `int audio_fill_square_wave(uint32_t hz, uint32_t ms)`
- `int audio_play(size_t byte_count)`
- `int audio_stop(void)`

## Guided Gaps

Reject zero frequency or duration, compute samples per half-period, pick an
amplitude that does not clip, keep byte count and sample count separate, and
make `audio_stop` safe when nothing is playing. The square-wave function should
be small enough that someone can verify the period arithmetic directly.

> [!IMPORTANT]
> Do not write past the mapped audio buffer. Duration, sample rate, channels,
> and bytes per sample must agree before writing.

## Common Mistakes

Common mistakes include using frequency where period is required, treating
milliseconds as samples, writing one channel when stereo is expected, and
starting playback before filling the buffer. If the output file has the wrong
duration, inspect unit conversion before changing the playback call.

Next: [checks and references](/labs/lab6/check).
