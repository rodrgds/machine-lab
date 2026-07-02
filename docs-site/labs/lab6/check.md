# Lab 6 Check

Run the audio tests before judging the result by ear. The tests can verify byte
counts, buffer bounds, and deterministic sample output even when your local
machine or browser would play the sound differently.

```sh
machinelab test audio
machinelab run --headless --audio-wav build/tone.wav -- build/examples/audio_tone
```

## Discussion Prompts

When reviewing the lab, discuss what changes if you generate sine waves instead
of square waves, how a tracker could represent notes, tempo, and instruments,
and why audio APIs often use ring buffers.

## External Reading

For additional background, read OSDev's [sound overview](https://wiki.osdev.org/Sound),
the general [PCM audio overview](https://en.wikipedia.org/wiki/Pulse-code_modulation),
and the OSDev [AC97 notes](https://wiki.osdev.org/AC97).
