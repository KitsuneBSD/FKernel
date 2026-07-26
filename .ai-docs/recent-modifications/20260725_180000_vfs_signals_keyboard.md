# 2026-07-25: VFS Fix, Signal Chain, US_INTL Keyboard Layout

## Changes

### VFS: /tmp directory creation
- Added `/tmp` directory creation in `VirtualFileSystem::initialize()` (`Src/Kernel/Fs/Vfs/virtual_filesystem.cpp:46`)
- Added `TmpFsNode::truncate()` override (`Src/Kernel/Fs/Virtual/TmpFs/tmp_fs.cpp:47-50`)
- Fixes 4 failing VFS regression tests that relied on `/tmp` existing

### Signal chain: Ctrl+C/Z/\ completion
- Added `ctrl_pressed` tracking to PS2Keyboard driver (`Include/Kernel/Driver/Keyboard/ps2_keyboard.h:25`)
- Modified `KeymapManager::translate()` to accept `ctrl` parameter and produce control characters (`Src/Kernel/Driver/Keyboard/keymap_manager.cpp:235`)
- Added Ctrl+D (EOF) handling in VGATerminal: empty queue sets `eof_pending` flag (`Src/Kernel/Driver/Terminal/vga_terminal.cpp:49-54`)
- Init process now calls `TIOCSCTTY` after `setsid()` to set foreground process group (`Src/Userland/init/main.c:29`)
- Shell sets SIGINT/SIGQUIT/SIGTSTP to SIG_IGN, restores SIG_DFL in children (`Src/Userland/shell/main.c:36-47`)

### Keyboard: US International layout
- Added `US_INTL` to `KeyboardLayout` enum (`Include/Kernel/Driver/Keyboard/keymap_manager.h:10`)
- Added US_INTL layout tables with dead key markers (`Src/Kernel/Driver/Keyboard/keymap_manager.cpp:93-140`)
- Implemented dead key state machine with `resolve_dead_key()` combining tables (`Src/Kernel/Driver/Keyboard/keymap_manager.cpp:142-200`)
- Added `compose_mode` flag to toggle dead keys on/off (`Include/Kernel/Driver/Keyboard/keymap_manager.h:47`)
- Default layout changed from ABNT2 to US_INTL (`Src/Kernel/Driver/Keyboard/ps2_keyboard.cpp:78`)
- Added runtime layout switching via custom ioctls KBDIO_SETLAYOUT/GETLAYOUT/SETCOMPOSE (`Src/Kernel/Driver/Terminal/vga_terminal.cpp:321-335`)

### Documentation
- Updated `Docs/Domains/process-scheduling.md` with terminal signal delivery flowchart
- Created `.ai-docs/development-patterns/keyboard-input.md` documenting the full input pipeline
