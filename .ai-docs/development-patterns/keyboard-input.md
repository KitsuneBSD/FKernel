# Keyboard Input Pipeline

## Architecture

The keyboard input pipeline transforms hardware scancodes into characters, control signals, and terminal events.

### Flow

```
PS/2 Hardware → IRQ1 → handle_scancode() → KeymapManager::translate() → TerminalManager::handle_input() → VGATerminal::on_char()
```

### Key Files

| File | Role |
|------|------|
| `Src/Kernel/Driver/Keyboard/ps2_keyboard.cpp` | PS/2 driver, scancode processing, modifier tracking |
| `Src/Kernel/Driver/Keyboard/keymap_manager.cpp` | Layout tables, character translation, dead key state machine |
| `Src/Kernel/Driver/Terminal/vga_terminal.cpp` | Input buffering, canonical/raw mode, signal delivery |
| `Include/Kernel/Driver/Keyboard/keymap_manager.h` | KeyboardLayout enum, KeymapManager interface |

## Modifier Keys

PS2Keyboard tracks three modifiers via scancode byte 0x80 (release bit):

| Modifier | Scancode (press) | Scancode (release) |
|----------|-------------------|---------------------|
| Left/Right Shift | 0x2A / 0x36 | 0xAA / 0xB6 |
| Left Alt | 0x38 | 0xB8 |
| Left Ctrl | 0x1D | 0x9D |

## Control Characters (Ctrl modifier)

When `ctrl_pressed` is true, `KeymapManager::translate()` produces control characters directly, bypassing the layout map:

| Key | Scancode | Control Char |
|-----|----------|-------------|
| Ctrl+A through Ctrl+Z | 0x1E..0x39 | \x01..\x1A |
| Ctrl+[ | 0x1A | \x1B (ESC) |
| Ctrl+\ | 0x2B | \x1C (SIGQUIT) |
| Ctrl+] | 0x1B | \x1D |

## Keyboard Layouts

### Supported Layouts

| Layout | Enum | Default | Notes |
|--------|------|---------|-------|
| US | `KeyboardLayout::US` | No | Standard QWERTY, no dead keys |
| US International | `KeyboardLayout::US_INTL` | Yes | Dead keys for accented characters |
| ABNT2 | `KeyboardLayout::ABNT2` | No | Brazilian layout |

### Layout Switching

Runtime layout switching via ioctl on any terminal FD:

```c
// Set layout (0=US, 1=US_INTL, 2=ABNT2)
sys_ioctl(fd, 0x4B01 /* KBDIO_SETLAYOUT */, (void*)1);

// Get current layout
int layout = sys_ioctl(fd, 0x4B02 /* KBDIO_GETLAYOUT */, 0);

// Toggle compose mode (dead keys on/off)
sys_ioctl(fd, 0x4B03 /* KBDIO_SETCOMPOSE */, (void*)1);
```

### FKMAP File Format

Binary keymap files can be loaded from VFS:

```
Offset 0: "FKMAP" (5 bytes header)
Offset 8: Normal map (128 bytes)
Offset 136: Shift map (128 bytes)
Offset 264: Alt/AltGr map (128 bytes)
```

## Dead Key State Machine (US_INTL)

Dead keys (backtick, apostrophe, Shift+6=circumflex, Shift+`=tilde) use a 3-state machine:

```
IDLE → [dead key pressed] → DEAD_BUFFERED → [next key pressed]
  ├─ if combine → return accented char → IDLE
  ├─ if space → return dead key literal → IDLE
  └─ if no combine → return dead key literal, buffer current key → PENDING_FLUSH
PENDING_FLUSH → return buffered key → IDLE
```

### Dead Key Combinations

| Dead Key | Combinations | Result |
|----------|-------------|--------|
| `` ` `` | a/e/i/o/u | à/è/ì/ò/ù |
| `'` | a/e/i/o/u, c/C | á/é/í/ó/ú, ç/Ç |
| `^` | a/e/i/o/u | â/ê/î/ô/û |
| `~` | a/n/o | ã/ñ/õ |

## Terminal Modes

### Canonical Mode (default)
- Input buffered until newline
- Line editing (backspace) handled by terminal
- Ctrl+C/Z/\ deliver signals
- Ctrl+D signals EOF when queue empty

### Raw Mode
- Characters passed through immediately
- No signal delivery from Ctrl keys
- No line editing
- Used by programs that handle their own input (e.g., vi, less)

## Signal Delivery from Terminal

When ISIG is enabled in termios (default) and a foreground process group exists:

| Control Char | Signal | Effect |
|-------------|--------|--------|
| \x03 (Ctrl+C) | SIGINT | Terminate foreground group |
| \x1C (Ctrl+\) | SIGQUIT | Terminate with core dump |
| \x1A (Ctrl+Z) | SIGTSTP | Stop foreground group |
| \x04 (Ctrl+D) | EOF | Return 0 from read() |
