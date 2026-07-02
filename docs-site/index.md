---
layout: home

hero:
  name: Machine Lab
  text: Portable C labs for machine-facing programming
  tagline: Teach device-shaped systems work without making students fight a VM first.
  image:
    src: /assets/screenshots/word-processor.png
    alt: Machine Lab word processor example
  actions:
    - theme: brand
      text: Start the labs
      link: /labs/
    - theme: alt
      text: Adopt in a class
      link: /instructors/adoption
    - theme: alt
      text: GitHub
      link: https://github.com/rodrgds/machine-lab

features:
  - title: Device-shaped C
    details: Ports, IRQs, RTC, timers, keyboard, mouse, framebuffers, audio buffers, and UART.
  - title: Student-first workflow
    details: Generate a workspace, run make, run machinelab test, inspect deterministic artifacts.
  - title: Course-ready
    details: Lab guides, instructor notes, rubrics, examples, release builds, and Cloudflare-hosted docs.
---

<div class="demo-video">
  <video controls preload="metadata" src="/assets/demo/machine-lab-90s.mp4"></video>
</div>

Machine Lab is public courseware for computer architecture, operating systems,
and systems programming classes that want low-level ideas without requiring a
full OS lab setup.

It keeps the useful part of older PC lab work:

- bytes, masks, registers, and status bits;
- interrupt-shaped event loops;
- keyboard and mouse packet parsing;
- framebuffers and PCM audio;
- serial protocols and paired programs;
- final projects that produce visible artifacts.

It removes the part that usually burns the first week:

- fragile VM images;
- service-management ceremony;
- platform-specific display/audio setup;
- undocumented grading paths.

<div class="role-grid">
  <div class="role-card">
    <h3>Students</h3>
    <p>Use a generated workspace, follow the lab guides, and build a final C project.</p>
    <p><a href="/students/">Student guide</a></p>
  </div>
  <div class="role-card">
    <h3>Instructors</h3>
    <p>Adopt the material in one week, choose labs, publish a rubric, and run project studio.</p>
    <p><a href="/instructors/">Instructor guide</a></p>
  </div>
  <div class="role-card">
    <h3>Maintainers</h3>
    <p>Work on the runtime, SDK, lab templates, release artifacts, and docs site.</p>
    <p><a href="/developers/">Developer guide</a></p>
  </div>
</div>

<div class="demo-grid">
  <img src="/assets/screenshots/music-maker.png" alt="Machine Lab music maker example">
  <img src="/assets/screenshots/breakout.png" alt="Machine Lab breakout example">
</div>

## What You Can Send People

- Lab track: [start here](/labs/).
- One-week adoption: [instructor adoption guide](/instructors/adoption).
- Project examples: [examples](/examples/).
- Source and releases: [GitHub](https://github.com/rodrgds/machine-lab).
