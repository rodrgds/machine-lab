# Machine Lab Docs

Machine Lab is organized as a public course kit. The canonical public docs are
the VitePress website in [../docs-site](../docs-site), deployed at
<https://machine-lab-docs.pages.dev/>.

This folder remains useful as package-local reference material, but new
student, instructor, and developer documentation should be added to the website
first.

## Student-facing

Use this path when you are taking the course or following it independently.

- [Public docs website source](../docs-site): the VitePress site intended for public browsing.
- [Student guide](student-guide.md): workspace setup, lab loop, final project
  loop, and what to ignore.
- [Course overview](../course/README.md): lab sequence and project studio.
- [Lab 1: Bitwise Helpers And RTC](../course/labs/docs/lab1-bitwise-rtc.md)
- [Lab 2: PIT And Timer IRQs](../course/labs/docs/lab2-timer.md)
- [Lab 3: Keyboard And i8042](../course/labs/docs/lab3-keyboard.md)
- [Lab 4: PS/2 Mouse Packets](../course/labs/docs/lab4-mouse.md)
- [Lab 5: VBE Framebuffer And XPM](../course/labs/docs/lab5-video.md)
- [Lab 6: AC97-lite PCM Audio](../course/labs/docs/lab6-audio.md)
- [Lab 7: UART And Serial Ports](../course/labs/docs/lab7-uart.md)
- [Examples](../examples/README.md): word processor, music maker, games, and
  project seeds.

## Teacher-facing

Use this path when you want to run the material in a class, club, workshop, or
university lab.

- [Adoption guide](adoption.md): how to use Machine Lab in one week.
- [Instructor guide](instructor-guide.md): syllabus map, lab operations,
  grading, project rubric, and staff workflow.
- [Video demo script](video-demo.md): short sequence for a class presentation.
- [Course license](../course/LICENSE.md): CC BY 4.0 reuse terms for handouts.

## Developer-facing

Use this path when you are building Machine Lab itself.

- [Docs site source](../docs-site): canonical public VitePress site deployed to Cloudflare Pages.
- [Developer guide](developer-guide.md): architecture boundaries, tests, release
  workflow, docs conventions, and change rules.
- [Architecture](architecture.md): runtime process model and virtual devices.
- [Runtime and CLI](runtime-and-cli.md): execution, replay, capture, bundles.
- [From Minix to Machine Lab](from-minix.md): what was retained and what was
  made portable.
- [Roadmap](roadmap.md): implemented scope and future work.
- [Contributing](../CONTRIBUTING.md): contribution rules and review checklist.

## Licenses

- Code: [MIT License](../LICENSE).
- Course, docs, screenshots, and demo media: [CC BY 4.0](../course/LICENSE.md).
