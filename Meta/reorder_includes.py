#!/usr/bin/env python3
"""
Reorders #include lines in Kernel .cpp files to match the canonical layer order:
  LibC → LibFK → Kernel → Other

Rules:
- Only touches the initial "include block" at the top of each file.
- Stops at the first #ifdef/#ifndef/#if/#else/#endif line (keeps conditional includes
  and everything after them unchanged).
- Blank lines and // comment lines inside the block are consumed and regenerated.
- Groups are separated by a single blank line in the output; a trailing blank line
  is added before the rest of the file.

Usage:
  python3 Meta/reorder_includes.py [--check] [files...]

  With --check: report files that need reordering without writing.
  Without files: processes all Src/Kernel/**/*.cpp
"""

import sys
import os
import re
from pathlib import Path

ROOT = Path(__file__).parent.parent


def classify(line):
    m = re.match(r'\s*#include\s*[<"]([^>"]+)[>"]', line)
    if not m:
        return None, None
    path = m.group(1)
    if path.startswith("LibC/"):
        return 0, line.strip()
    if path.startswith("LibFK/"):
        return 1, line.strip()
    if path.startswith("Kernel/"):
        return 2, line.strip()
    return 3, line.strip()


def process_file(path, check_only=False):
    text = path.read_text()
    lines = text.splitlines(keepends=True)

    # Collect the initial include block: #include, blank, // comment lines.
    # Stop at first preprocessor conditional or code line.
    block_end = 0
    for i, line in enumerate(lines):
        stripped = line.strip()
        if stripped == "" or stripped.startswith("//"):
            block_end = i + 1
            continue
        if re.match(r"#include\s", stripped):
            block_end = i + 1
            continue
        # Any other line (code, #ifdef, #pragma, #define, etc.) ends the block
        break

    # Extract block lines and rest
    block_lines = lines[:block_end]
    rest_lines = lines[block_end:]

    # Separate includes from blank/comment lines in block
    groups = {0: [], 1: [], 2: [], 3: []}
    has_any_include = False
    for line in block_lines:
        stripped = line.strip()
        if not stripped or stripped.startswith("//"):
            continue  # discard blanks/comments; we'll regenerate spacing
        grp, inc = classify(line)
        if inc is None:
            continue  # shouldn't happen, but skip non-include non-blank lines
        groups[grp].append(inc)
        has_any_include = True

    if not has_any_include:
        return False  # nothing to do

    # Check if already in correct order (no LibFK before LibC, no Kernel before LibFK)
    original_includes = [l.strip() for l in block_lines if classify(l)[0] is not None]
    reordered = []
    for grp in sorted(groups.keys()):
        reordered.extend(groups[grp])
    if original_includes == reordered:
        return False  # already correct

    if check_only:
        print(f"NEEDS REORDER: {path}")
        return True

    # Build new block
    new_block_lines = []
    for grp in sorted(groups.keys()):
        if not groups[grp]:
            continue
        if new_block_lines:
            new_block_lines.append("\n")
        for inc in groups[grp]:
            new_block_lines.append(inc + "\n")

    # Ensure one blank line before rest of file (if rest starts with non-blank)
    rest_text = "".join(rest_lines)
    if rest_text and not rest_text.startswith("\n"):
        new_block_lines.append("\n")

    new_text = "".join(new_block_lines) + rest_text
    if new_text != text:
        path.write_text(new_text)
        return True
    return False


def main():
    args = sys.argv[1:]
    check_only = "--check" in args
    args = [a for a in args if a != "--check"]

    if args:
        files = [Path(a) for a in args]
    else:
        files = list((ROOT / "Src" / "Kernel").rglob("*.cpp"))

    changed = 0
    for f in sorted(files):
        if process_file(f, check_only=check_only):
            changed += 1
            if not check_only:
                print(f"REORDERED: {f.relative_to(ROOT)}")

    print(f"\n{'Would reorder' if check_only else 'Reordered'}: {changed} / {len(files)} files")


if __name__ == "__main__":
    main()
