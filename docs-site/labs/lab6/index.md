# Lab 6: AC97-lite PCM Audio

Generated folder: `labs/audio/`

Lab 6 introduces audio as a stream of samples. Instead of drawing pixels into a
framebuffer, you fill an audio buffer with PCM data and ask the runtime to play
it.

The analogy with graphics is useful. A framebuffer is memory that eventually
becomes an image. An audio buffer is memory that eventually becomes pressure
changes in the air. In both cases, the device consumes bytes according to a
format, and your program is responsible for producing those bytes consistently.

| Concept | Meaning |
| --- | --- |
| PCM sample | numeric amplitude at one point in time |
| sample rate | samples per second |
| channel count | mono or stereo layout |
| buffer | mapped memory for samples |
| byte count | amount the device should consume |

Start by learning [PCM audio](/labs/lab6/pcm), then complete the
[implementation tasks](/labs/lab6/tasks), run the [checks](/labs/lab6/check),
and compare the lab model with [modern audio stacks](/labs/lab6/modern).

> [!TIP]
> Audio bugs are often unit bugs: samples, frames, channels, and bytes are not
> interchangeable.
