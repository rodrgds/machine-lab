# PCM Audio

PCM stores sound as a sequence of numeric sample values. Each sample is the
amplitude of the waveform at one point in time. If the sample rate is 48 kHz,
the device expects 48,000 samples per second for each channel.

| Term | Meaning |
| --- | --- |
| sample rate | samples per second |
| channel | mono/stereo layout |
| frame | one sample per channel |
| buffer | memory region sent to the device |
| amplitude | numeric signal level |

The device consumes bytes, not musical intent. Your code must translate
frequency and duration into sample counts, frame counts, and byte counts. A
small mistake in those units can make the audio too short, too long, distorted,
or silent.

## Square Wave

A square wave alternates between positive and negative amplitude. It is easy to
generate and easy to test:

```text
positive for half period
negative for half period
repeat
```

It is not a beautiful sound, but it is an excellent first waveform because the
expected buffer is easy to inspect. Once the square wave works, a sine wave,
envelope, melody, or tracker pattern is a higher-level transformation that still
ends in PCM samples.
