# QEMU Virtio Lab

A minimal, reproducible lab on an Apple Silicon Mac for tracing Virtio end to end — from the QEMU device model to the Linux guest driver.

## Workflow

1. `./scripts/host-check.sh` — check macOS, Homebrew, Docker, and required tools.
2. `./scripts/fetch-sources.sh` — fetch the pinned QEMU source on macOS.
3. `./scripts/build-qemu.sh` — build `qemu-system-aarch64` natively on macOS.
4. `./scripts/build-guest-docker.sh` — build the Linux `Image` and Buildroot rootfs inside a Docker container.
5. `./scripts/run.sh` — boot the guest and log in as `root` (empty password).
6. Read [`docs/virtio-data-flow.md`](docs/virtio-data-flow.md), then work through the custom device experiment in [`patches/`](patches/README.md).

## Requirements

- macOS on Apple Silicon (arm64)
- Homebrew (`brew install qemu meson ninja pkg-config`)
- Docker Desktop (only needed for step 4)
- Network access for the first source download

> [!NOTE]
> The Linux guest cannot be built natively on macOS: the kernel's host tools (`modpost`) conflict with Darwin headers, and the Linux tree has filenames that collide on case-insensitive APFS. The guest is therefore always built in Docker. Running `build-guest.sh` directly on macOS prints a hint to use `build-guest-docker.sh` instead.

## Quick start

Versions are pinned in `config/versions.env` (QEMU v9.2.0, Linux v6.12, Buildroot 2025.02).

```sh
./scripts/host-check.sh
./scripts/fetch-sources.sh
./scripts/build-qemu.sh
./scripts/build-guest-docker.sh
./scripts/run.sh
```

At the `virtio-guest login:` prompt, enter `root` (no password). To quit QEMU, press `Ctrl-a`, release, then `x`.

`run.sh` only uses files that already exist on macOS, so Docker Desktop does not need to be running to boot the guest.

## Docker guest build

`build-guest-docker.sh` uses an Ubuntu 24.04 arm64 container (see `Dockerfile`) and runs `fetch-sources.sh` and `build-guest.sh` inside it:

| Location | Contents |
|---|---|
| Docker volume `qemu-virtio-lab-build` | Linux / Buildroot sources and build trees |
| Docker volume `qemu-virtio-lab-output` | Full guest output, including Buildroot intermediates |
| `output/guest/` on macOS | `Image` and `rootfs.ext4`, copied out after the build |

`.docker-sources-ready` marks that the sources in the volume have been fetched. Delete it to force a fresh fetch on the next run.

Rebuild the guest whenever you change `config/linux-aarch64.config`, `config/buildroot-aarch64.fragment`, or add the guest driver to the kernel.

## Layout

| Path | Purpose |
|---|---|
| `build/qemu/` | QEMU source and build tree (macOS) |
| `output/qemu/` | Installed `qemu-system-aarch64` |
| `output/guest/` | Guest `Image` and `rootfs.ext4` |
| `config/` | Pinned versions, kernel config fragment, Buildroot fragment |
| `docs/` | Virtio data-flow notes |
| `patches/` | Plan for the custom QEMU Virtio device |
| `guest-driver/` | Linux `virtio_driver` for the custom device |
| `open_issue.md` / `close_issue.md` | Open and resolved issues |

The custom QEMU device is not added to the boot command by default. After applying the patch, boot it with:

```sh
CUSTOM_DEVICE=1 ./scripts/run.sh
```
