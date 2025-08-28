# Building FKernel on a Native Machine

> [!NOTE]
> Currently, this guide is focused on **Linux** systems.  
> Support for **Windows** is planned, but not yet fully implemented.  
> If you're on Windows, it's recommended to use **WSL2** or a **virtual machine** for now.

## Requirements

To build FKernel, the following tools are required:

- **Assembler:** [NASM](https://en.wikipedia.org/wiki/Netwide_Assembler)  
- **Compiler:** [Clang](https://en.wikipedia.org/wiki/Clang), [G++](https://en.wikipedia.org/wiki/GNU_Compiler_Collection), or [MSVC](https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B)  
- **Linker:** [ld.lld](https://en.wikipedia.org/wiki/LLVM#Linker) or [GNU ld](https://en.wikipedia.org/wiki/GNU_ld)  
- **Build System:** [xmake](https://xmake.io)

---

## Step 1: Configure XMake

FKernel comes with a preconfigured `.xmake` project. However, the first time you build, there will be no cached toolchain configuration.

Run the following command to configure the build system:

```bash
xmake f --mode=release --toolchain=FKernel_Compiling
````

This sets up the toolchain path cache so the project can compile successfully.

---

## Step 2: Build the Kernel

To compile the project, simply run:

```bash
xmake
```

For a verbose build output, use:

```bash
xmake -v
```

---

## Step 3: Run FKernel (Optional)

To run FKernel in a virtual machine environment, you will need:

* [mtools](https://en.wikipedia.org/wiki/Mtools)
* [xorriso](https://en.wikipedia.org/wiki/Libburnia#Xorriso)
* [grub](https://en.wikipedia.org/wiki/GNU_GRUB)
* [qemu](https://en.wikipedia.org/wiki/QEMU)

By default, `xmake run` will:

1. Create a virtual disk image called `FKernel-HDA` in the `build` directory.
2. Set up a minimal mock OS environment.
3. Launch QEMU in **non-graphics** mode with serial output.

Run:

```bash
xmake run
```
> [!NOTE]
> These steps are currently handled via shell scripts, which makes Windows compatibility problematic.
> A future rewrite may introduce cross-platform support.
> Contributions are welcome.
