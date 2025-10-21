#!/usr/bin/env lua
local io = io
local os = os
local print = print

local IMG = './bsd.img'
local SIZE = '64M'

print(string.format('Creating BSD disklabel (best-effort) image: %s (%s)', IMG, SIZE))
os.execute(string.format('qemu-img create -f raw "%s" "%s"', IMG, SIZE))

local sfdisk_cmd = string.format("sfdisk --force %s <<'EOF'\nlabel: dos\nlabel-id: 0xdeadbeef\nunit: sectors\n\n1 : start=2048, size=, type=83\nEOF", IMG)
print('+ ' .. sfdisk_cmd)
local rc = os.execute(sfdisk_cmd)
if not rc then error('sfdisk failed') end

-- Write a zeroed sector at offset 1 to mimic some BSD label heuristics
local dd_cmd = string.format("printf '\\0' | dd of=\"%s\" bs=512 seek=1 count=1 conv=notrunc >/dev/null 2>&1 || true", IMG)
os.execute(dd_cmd)

print('Note: BSD labels created this way are synthetic. For a canonical BSD disklabel use a BSD host or bsdlabel tools.')
print('Done: ' .. IMG)
