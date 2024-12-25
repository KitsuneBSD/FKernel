# Toolchain

FKernel has a simple choose of languages and tools to build the kernel.

The kernel has some dependencies that need to be installed before building the kernel:

- [GNU/Make](https://pt.wikipedia.org/wiki/Make) maybe being portable from BSD/Make
- [NASM](https://pt.wikipedia.org/wiki/NASM)

But, NASM is default choose in makefile you can choose between FASM or NASM.

## Probably future tools

- [RUSTC](https://rustc-dev-guide.rust-lang.org/) from build the microkernel
- [LLVM](https://llvm.org/) from build the monolithic kernel

## Testing the builded FKernel

From now, the make build only a legacy bios bootable image.

But you can test it with [QEMU](https://www.qemu.org/).

### To build the kernel

run in root directory:

```bash
make
```

And from execute the bootloader, run in root directory:

```bash
make qemu
```
