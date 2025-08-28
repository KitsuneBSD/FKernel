# FKernel

A small hybrid kernel for x86_64 with a freestanding, bare-metal focus. It is inspired by MINIX, seL4, Mach, the BSDs, and SerenityOS.

## Highlights

- Hybrid kernel design, influenced by MINIX, seL4, Mach, the BSDs, and SerenityOS
- Runs directly on hardware (bare-metal), written in modern C++20 without the usual C++ runtime
- Boots with GRUB using the Multiboot2 standard
- Target platform: x86_64 PCs (for now)
- Includes early boot, basic memory management, a simple virtual file system, and essential drivers (timer, interrupts, serial port, ATA disk)

## Prerequisites

Build host: Linux (x86_64 recommended)

Required tools:

- Toolchain: `clang`/`clang++` (C++20), `ld.lld`
- Assembler: `nasm`
- Build system: `xmake`
- Boot image tooling: `grub-mkrescue` (or `grub2-mkrescue`), plus `xorriso` and `mtools`
- Emulator: `qemu-system-x86_64` (and `qemu-img`)

Notes:

- Some distros ship `grub2-mkrescue` instead of `grub-mkrescue`.
- `grub-mkrescue` typically requires `xorriso` and `mtools` packages.

## Linux

Follow the steps for your distro, then build and run.

1) Install dependencies

- Debian/Ubuntu:

  ```bash
  sudo apt update
  sudo apt install -y \
    clang lld nasm xmake \
    qemu-system-x86 qemu-utils \
    grub-pc-bin grub-common \
    xorriso mtools \
    jq clang-format clang-tidy cppcheck
  ```

- Arch Linux:

  ```bash
  sudo pacman -Syu --needed \
    clang lld nasm xmake \
    qemu grub xorriso mtools \
    jq clang-format clang-tidy cppcheck
  ```

- Fedora:

  ```bash
  sudo dnf install -y \
    clang lld nasm xmake \
    qemu-system-x86 qemu-img \
    grub2-tools-extra xorriso mtools \
    jq clang-tools-extra cppcheck
  ```

2) Get the code

```bash
git clone https://github.com/KitsuneBSD/FKernel.git
cd FKernel
```

3) Build, run, clean

```bash
xmake          # build
xmake run      # launch QEMU with the generated ISO
# (quit QEMU with Ctrl+A, X if using the QEMU monitor on stdio)
xmake clean    # remove build artifacts
```

Artifacts created:

- Kernel: `build/FKernel.bin`
- ISO: `build/FKernel-MockOS.iso`
- Disk (qcow2): `build/FKernel-HDA.qcow2`

If ISO creation fails, ensure GRUB i386-pc modules are installed (e.g., `grub-pc-bin` on Debian/Ubuntu) and the path `/usr/lib/grub/i386-pc/` exists, or adjust `Meta/mounting_mockos.sh`.

## DevPod:  (Docker/Podman)

Use the provided DevContainer. The image is built from `.devcontainer/Dockerfile` (FROM `ubuntu:24.04`) and already includes all required tools: clang/lld, nasm, xmake, qemu-system-x86_64/qemu-img, grub-mkrescue, xorriso, mtools, and optional dev tools (jq, clang-format/clang-tidy, cppcheck). No extra install inside the container is needed.

Steps:

1) Start a DevPod for this repo:
   - Docker: `devpod up . --provider=docker`
   - Podman: `devpod up . --provider=podman`
2) Open the workspace in your editor (VS Code or DevPod UI).
3) Inside the DevPod terminal:
   - Build: `xmake`
   - Run: `xmake run`
   - Optional checks: `bash Meta/run_continuous_integration.sh`

If you maintain your own template/image, ensure the same toolset is present.

## Troubleshooting

- `grub-mkrescue` not found: Use `grub2-mkrescue` or install GRUB utilities, plus `xorriso` and `mtools`.
- QEMU not found: Install `qemu-system-x86_64` (and `qemu-img`/`qemu-utils`).
- ISO creation fails: Ensure `/usr/lib/grub/i386-pc/` exists on your distro or adjust the path in `Meta/mounting_mockos.sh`.
- LSP not seeing includes: Generate `compile_commands.json` with `Meta/generate_compile_commands.sh` and point your tools to it.

## Contributing

Issues and PRs are welcome. Please run `Meta/run_continuous_integration.sh` before submitting changes.

## License

See `LICENSE`.
