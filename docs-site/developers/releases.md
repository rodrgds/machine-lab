# Releases

Machine Lab publishes release archives for Linux, macOS, and Windows through
GitHub Actions.

## Release Contents

Release archives should contain:

- `machinelab` primary CLI;
- `lcom` and `lowlab` compatibility CLIs;
- `libmachinelab.a` and compatibility static libraries;
- public SDK headers;
- examples and replay scripts;
- course docs and the public docs-site source.

## Workflows

| Workflow | Purpose |
| --- | --- |
| `.github/workflows/release.yml` | build/test/package platform archives |
| `.github/workflows/docs.yml` | build/deploy the VitePress website |

The release workflow builds on:

- Ubuntu 24.04;
- macOS x86_64;
- macOS arm64;
- Windows/MSYS2.

## Local Preflight

```sh
devenv shell -- machinelab-test
npm run docs:check
git diff --check
```

If a release changes lab behavior, verify a generated student workspace:

```sh
machinelab setup build/student-smoke --force
make -C build/student-smoke
machinelab test rtc --project build/student-smoke
```

Fresh stubs should compile and fail the predefined checks.
