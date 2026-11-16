# SDK versions

Each SDK version has a number (`sdk.json`, for example `1.0.0`). Creator Lab comes with one
SDK, and every project remembers the exact SDK it was made with, so a project keeps building
the same way after you update Lab.

## How a project keeps its SDK

A new project has a small file, `resona.sdk.lock`: the SDK version and a checksum of its
files. Lab keeps each SDK version once, in a shared folder on your computer, and every
project uses the one its lock names. Keep the lock in your source control. Lab never swaps
in a different version on its own: if a project's SDK is missing, it says so.

## Updating a project to a newer SDK

**Project SDK… → Update to installed SDK** in Lab (or `resona-lab sdk sync <project>`)
moves that one project to the SDK that came with Lab. Other projects are not touched.
Read the [changelog](../CHANGELOG.md), then build and test again.

## Getting a missing SDK

A project from someone else may name an SDK version you do not have. Then:

- **Project SDK… → Download SDK** (or `resona-lab sdk fetch <project>`) downloads that
  version from the SDK's releases, checks it against the project's lock, and adds it.
  Set `RESONA_SDK_RELEASES` to where the releases are.
- Or get the SDK folder itself and run `resona-lab sdk install <folder>`.

## Sharing a project

**Export project…** makes a ZIP of your project. Tick **Bundle the pinned SDK** to put the
SDK inside too, so the other person can build it without downloading anything. Build output,
Git history and credential files are left out.

## Commands

```sh
resona-lab sdk status <project>     # which SDK, which version, where
resona-lab sdk sync <project>       # move this project to Lab's SDK
resona-lab sdk fetch <project>      # download the SDK this project names
resona-lab sdk install <folder>     # add an SDK folder you already have
resona-lab export <project> --out project.zip [--bundle-sdk]
```

`RESONA_LAB_SDK` points Lab at a different SDK folder (for working on the SDK itself), and
`RESONA_LAB_SDK_CACHE` moves the shared folder.

## Releases

Each version is a tag `v<version>` with a `resona-sdk-<version>.tar.gz` archive and its
`.sha256`, made by `tools/release.py`. Changes are listed in [CHANGELOG.md](../CHANGELOG.md).
