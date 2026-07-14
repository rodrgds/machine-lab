# Public Demo Asset

`machine-lab-demo.mp4` is the 54-second public demo linked from the README. It shows:

- generating a student workspace;
- running a lab test;
- the word processor example;
- the music maker example;
- final project artifacts.

The Remotion edit uses deterministic `word-processor.mp4` and
`music-maker.mp4` captures. Its soundtrack is the sequencer's own
`music-maker.wav` output, so the public artifact also exercises the audio path.

Regenerate it with:

```sh
devenv shell -- sh scripts/render_public_demo.sh
```

The video is licensed with the rest of the course/docs media under CC BY 4.0.
