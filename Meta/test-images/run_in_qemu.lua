#!/usr/bin/env lua
local io = io
local os = os
local print = print

local args = {}
for i=1,#arg do args[#args+1] = arg[i] end

local DISK = ''
local ISO = 'build/FKernel-MockOS.iso'
local QEMU_ARGS = ''

local i = 1
while i <= #args do
  local a = args[i]
  if a == '--disk' then DISK = args[i+1]; i = i + 2
  elseif a == '--iso' then ISO = args[i+1]; i = i + 2
  elseif a == '--qemu-args' then QEMU_ARGS = args[i+1]; i = i + 2
  else print('Unknown arg: ' .. a); os.exit(1) end
end

if DISK == '' then print('No disk specified. Use --disk ./mbr.img'); os.exit(1) end
if not (io.open(DISK)) then print('Disk not found: ' .. DISK); os.exit(1) end
if not (io.open(ISO)) then print('ISO not found: ' .. ISO); print('Build the kernel and ISO first with: xmake'); os.exit(1) end

local qemu_cmd = string.format('qemu-system-x86_64 -drive file="%s",format=raw,if=ide,index=0 -cdrom "%s" -m 2G -smp 2 -nographic -serial mon:stdio %s', DISK, ISO, QEMU_ARGS)
print('+ ' .. qemu_cmd)
os.execute(qemu_cmd)
