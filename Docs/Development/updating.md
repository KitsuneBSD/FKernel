# Developing Updates for FKernel

## Adding a New Subsystem
1. Create the header in `Include/Kernel/SubsystemName/`.
2. Implement the logic in `Src/Kernel/SubsystemName/`.
3. Add the directory to `kernel_non_architecture_related` in `xmake.lua`.
4. Initialize the subsystem in `Src/Kernel/Init/init.cpp`.

## Adding a System Call
1. Add the number to `Include/LibFK/Syscalls/numbers.h`.
2. Implement the handler in `Src/Kernel/Syscall/syscall_list/<Domain>/` — one `sys_*` handler per file, file name = handler name minus the `sys_` prefix (shared support files with zero handlers are allowed).
3. Register it in `Src/Kernel/Syscall/syscall.cpp`.
4. Run `xmake check-syscalls` to verify the one-handler-per-file rule.

## Modifying Architecture Specific Code
Architecture specific code resides in `Src/Kernel/Arch/x86_64/`. When updating these:
- Maintain SystemV ABI compatibility.
- Ensure any assembly changes are matched with C++ declarations.
- Update documentation in `Docs/Architecture/` if changes affect the memory layout or task switching.

## Testing Against Validation Tooling (BusyBox, musl)
To validate syscall compatibility:
1.  **Toolchain**: Use scripts in `Toolchain/` to download and patch projects.
2.  **LibC**: Musl is the preferred validation library. Patch it to use FKernel's `syscall` interface.
3.  **BusyBox**: Provides `ash` and standard tools (`ls`, `cat`, `mkdir`). Compiled statically against Musl for MockOS test ISO.
