# Developer Guide

This page is for people building Machine Lab itself: runtime code, lab
scaffolding, docs, tests, examples, and releases.

## Audience Boundaries

| Audience | Stable surface | Keep out |
| --- | --- | --- |
| Students | generated workspace, SDK headers, lab handouts, `machinelab test` | CMake, runtime C++, CI, reference solutions |
| Instructors | labs, examples, scripts, adoption docs, selected tests | accidental protocol internals |
| Developers | whole repository | unclear student-facing copy |

## Source Map

| Path | Ownership |
| --- | --- |
| `runtime/` | C++ host runtime and devices |
| `runtime/cli/` | setup, lab catalog, tests, CLI support |
| `sdk/include/lcom/` | public C headers copied into workspaces |
| `sdk/client/` | C client linked into student programs |
| `course/labs/templates/` | starter contracts |
| `course/labs/docs/` | source lab handouts |
| `docs-site/` | public website |
| `dev/lab-solutions/` | reference implementations |
| `dev/lab-tests/` | predefined lab checks |

## Test Gate

```sh
devenv shell -- machinelab-test
npm run docs:check
git diff --check
```

## Docs Site

The public site uses VitePress:

```sh
npm install
npm run docs:dev
npm run docs:build
npm run deploy:docs
```

Cloudflare Pages deploys from `docs-site/.vitepress/dist`. Push-to-deploy uses
`.github/workflows/docs.yml` and requires `CLOUDFLARE_API_TOKEN` plus
`CLOUDFLARE_ACCOUNT_ID` repository secrets.
