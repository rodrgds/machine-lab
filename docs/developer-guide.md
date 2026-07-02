# Developer Guide

This guide is for people maintaining Machine Lab itself: runtime code, lab
scaffolding, docs, tests, examples, and releases.

The project has two jobs:

- provide a stable public course kit;
- provide a maintainable host runtime that makes the course portable.

Treat both jobs as first-class. Runtime changes that break the generated
student experience are not complete. Docs changes that drift from the runtime
are not complete either.

## Audience Boundaries

| Audience | Stable surface | Do not leak |
| --- | --- | --- |
| Students | generated workspace, `include/lcom/`, lab handouts, `machinelab test` | CMake, runtime C++, CI, reference solutions |
| Instructors | labs, examples, scripts, adoption docs, selected tests | accidental internal protocol details |
| Developers | whole repository | unclear student-facing copy |

When a change crosses boundaries, update the related docs and tests in the same
patch.

## Source Map

| Path | Ownership |
| --- | --- |
| `runtime/` | C++ host runtime, devices, backends |
| `runtime/cli/` | CLI helpers, course catalog, setup/test support |
| `common/` | process protocol shared by runtime and C client |
| `sdk/client/` | C client linked into student programs |
| `sdk/include/lcom/` | public student SDK headers copied as `include/lcom/` |
| `course/labs/templates/` | APIs copied into generated workspaces |
| `course/labs/docs/` | student-facing handouts |
| `course/labs/function-requests.json` | structured lab truth for CLI/help |
| `docs-site/` | VitePress public website |
| `dev/lab-solutions/` | reference implementations for tests and review |
| `dev/lab-tests/` | predefined student-lab checks |
| `examples/` | examples and project seeds |
| `scripts/` | deterministic replay scripts |
| `tests/integration.sh` | end-to-end repo contract |

## Change Rules

- Keep `sdk/include/lcom/` small, C-only, and free of host backend types.
- Keep generated workspace text student-facing and short.
- Keep CLI strings, lab aliases, and generated workspace layout aligned with
  `course/labs/function-requests.json` and `runtime/main.cpp`.
- Keep `lcom`/`lowlab` compatibility aliases unless a release plan removes them.
- Do not make students learn repo internals to pass labs.
- Prefer one shared helper over repeated generated text.
- Add public docs and integration checks when adding public features.
- Keep binary demo artifacts reproducible from scripts or documented commands.

## Runtime Architecture

At a high level:

1. A student C program links against the Machine Lab C client.
2. The client speaks a small protocol to the host runtime.
3. The runtime exposes virtual devices: PIT, RTC, keyboard/mouse, VBE, audio,
   UART, IRQ controller, and related support.
4. SDL backends provide interactive display, input, and audio where available.
5. Headless backends provide deterministic tests, frame dumps, WAVs, traces, and
   videos for CI and grading.

The runtime should model device-facing constraints without pretending to be a
full operating system or hardware emulator.

## Adding Or Changing A Lab

Update these surfaces together:

1. `course/labs/function-requests.json`
2. `course/labs/templates/labN/include/`
3. generated starter source in `runtime/cli/FileOps.*` if needed
4. `dev/lab-solutions/labN/`
5. `dev/lab-tests/labN_test.c`
6. `course/labs/docs/labN-*.md`
7. `course/README.md`
8. README and role guides if the public course map changes
9. integration tests

Fresh stubs should compile and fail meaningfully. Completed reference solutions
should pass the predefined checks.

## Adding Or Changing A Device

Check:

- port map and register names;
- public SDK header shape;
- runtime device state;
- IRQ behavior;
- script/replay support, if input driven;
- headless artifact support, if visual or audio;
- example program;
- lab or project use case;
- docs and external references.

Avoid exposing runtime internals through public headers just because it is
convenient for one example.

## Test Gate

```sh
devenv shell -- machinelab-test
npm run docs:check
git diff --check
```

Useful narrower checks:

```sh
devenv shell -- ctest --test-dir build -R integration --output-on-failure
devenv shell -- machinelab docs cli
devenv shell -- machinelab setup student --force
devenv shell -- machinelab test rtc --project student
```

For visual/audio changes, also capture artifacts:

```sh
devenv shell -- machinelab run --headless \
  --script scripts/music_maker_demo.mlabscript \
  --audio-wav build/music.wav \
  --dump-frame build/music.ppm \
  -- build/examples/music_maker
```

## Release Surface

Release archives should contain:

- `machinelab` primary CLI;
- `lcom` and `lowlab` compatibility CLIs;
- `libmachinelab.a` and compatibility static libraries;
- public headers;
- examples and scripts useful for demos;
- docs or links that explain student setup.

GitHub Actions should build Linux, macOS, and Windows artifacts. Release testing
should prove that generated student workspaces compile and that the primary
examples run headlessly.

## Documentation Discipline

Root README is the public landing page. Keep detailed workflows in focused docs:

- public index: `docs/index.md`;
- public website: `docs-site/`;
- students: `docs/student-guide.md`;
- instructors: `docs/instructor-guide.md`;
- adoption: `docs/adoption.md`;
- developers: this file;
- lab handouts: `course/labs/docs/`;
- runtime behavior: `docs/runtime-and-cli.md`;
- architecture: `docs/architecture.md`.

Student-facing docs should explain what to implement while leaving real gaps.
Instructor docs can discuss grading and operation. Developer docs can mention
internal files, release workflows, and test contracts.

## Licensing

Code is MIT licensed. Course and documentation material is CC BY 4.0. Keep new
files aligned with that split unless there is a specific reason to do otherwise.

## Public Website

The public website is built with VitePress and deployed to Cloudflare Pages:

```sh
npm run docs:dev
npm run docs:check
npm run deploy:docs
```

Production URL: <https://machine-lab-docs.pages.dev/>

Assets are copied from `docs/assets/` into `docs-site/public/assets/` during the
build. Push-to-deploy is configured in `.github/workflows/docs.yml` and requires
`CLOUDFLARE_API_TOKEN` plus `CLOUDFLARE_ACCOUNT_ID` repository secrets.
