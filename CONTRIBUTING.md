# Contributing

Machine Lab is both a teaching project and a runtime project. Contributions are
welcome, but changes need to preserve the separation between student-facing
course material, instructor material, and developer internals.

## Audience Boundaries

| Audience | Stable surface | Keep out |
| --- | --- | --- |
| Students | generated workspace, public SDK headers, lab handouts, `machinelab test` | reference solutions, CMake, CI, runtime protocol internals |
| Instructors | adoption guide, instructor guide, lab map, examples, replay scripts, selected grading checks | accidental spoilers that solve the whole lab |
| Developers | this repository, runtime code, release workflow, test fixtures | unclear public copy or duplicated course truth |

When a patch changes one surface, check the adjacent surface too. Example: a new
lab function usually needs an SDK/header update, a generated workspace update,
lab docs, reference solution, predefined test, CLI lab catalog entry, and README
mention.

## Local Setup

Preferred path:

```sh
devenv shell
machinelab-test
```

Fallback path:

```sh
cmake -S . -B build -G Ninja -DMACHINE_LAB_WITH_SDL=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

SDL, FFmpeg, and packaging tools are used for interactive demos, video capture,
and release artifacts. Core headless tests should still pass without requiring a
desktop session.

## Change Checklist

- Keep generated student workspaces simple: plain `make`, C files, headers, and
  `machinelab test`.
- Keep public C headers in `sdk/include/lcom/` C-only and free of runtime C++
  types.
- Keep lab contracts aligned across `course/labs/function-requests.json`,
  `runtime/cli/`, templates, tests, solutions, and docs.
- Keep reference implementations under `dev/lab-solutions/`.
- Keep predefined lab checks under `dev/lab-tests/`.
- Keep public docs in `docs/` and student lab handouts in `course/labs/docs/`.
- Prefer small deterministic examples over demos that only work interactively.
- Add or update integration checks when you add a public artifact.

## Docs Style

- Be explicit about what the student should implement and what they should not.
- Use admonitions for risk and guidance:
  - `> [!NOTE]` for context.
  - `> [!TIP]` for useful technique.
  - `> [!IMPORTANT]` for correctness or safety requirements.
- Link to external references when they help, but keep the lab self-contained.
- Do not paste complete solutions into student-facing docs.
- Keep prose direct. The goal is to teach the machine model, not to decorate it.

## Testing

Run the full gate before opening a pull request:

```sh
devenv shell -- machinelab-test
git diff --check
```

Useful narrower checks:

```sh
devenv shell -- ctest --test-dir build -R integration --output-on-failure
devenv shell -- machinelab setup build/student-smoke --force
devenv shell -- machinelab test rtc --project build/student-smoke
devenv shell -- machinelab docs cli
```

Fresh generated lab stubs should compile and fail the predefined tests until a
student implements the missing functions.

## Pull Requests

A good pull request includes:

- the user-facing reason for the change;
- the affected audience: student, instructor, developer, or release user;
- screenshots, WAVs, videos, traces, or replay scripts when behavior is visual
  or interactive;
- test commands and their results;
- notes about any intentionally skipped surface.

By contributing, you agree that code contributions are licensed under MIT and
course/documentation contributions are licensed under CC BY 4.0.
