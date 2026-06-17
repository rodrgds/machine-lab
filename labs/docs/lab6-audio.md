# Lab 6: AC97-lite PCM Audio

Goal: map the virtual AC97-lite PCM buffer, write signed 16-bit stereo samples,
program the PCM output sample rate and byte count registers, and start/stop
playback through the bus-master control register. In headless mode, use
`--audio-wav` to write the PCM stream to a `.wav` file for testing.

Reference:

```sh
build/lcom run --headless --audio-wav build/tone.wav -- build/examples/audio_tone
```
