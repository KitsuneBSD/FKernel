# Getting Started with FKernel Development

## Environment Setup
To develop for FKernel, you need:
- `xmake` (Build system)
- `nasm` (Assembler)
- `llvm/lld` (Linker and compiler)
- `qemu-system-x86_64` (Emulator)
- `xorriso` and `grub-mkrescue` (ISO creation)

## Build and Run
```bash
xmake build FKernel
xmake run FKernel
```

## Object Calisthenics
We enforce strict coding rules:
1. One level of indentation per method.
2. Don't use the `ELSE` keyword.
3. Wrap all primitives and strings.
4. First class collections.
5. One dot per line.
6. Don't abbreviate.
7. Keep all entities small (Classes < 200 lines).
8. No classes with more than two instance variables.
9. No getters/setters/properties.
