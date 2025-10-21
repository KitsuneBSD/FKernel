#!/usr/bin/env lua
local io = io
local os = os
local print = print

local IMG = './mbr.img'
local SIZE = '64M'

print(string.format('Creating MBR image: %s (%s)', IMG, SIZE))
os.execute(string.format('qemu-img create -f raw "%s" "%s"', IMG, SIZE))

local sfdisk_cmd = string.format("sfdisk --force %s <<'EOF'\nlabel: dos\nlabel-id: 0x12345678\nunit: sectors\n\n1 : start=2048, size=, type=83\nEOF", IMG)
print('+ ' .. sfdisk_cmd)
local rc = os.execute(sfdisk_cmd)
if not rc then error('sfdisk failed') end

-- Optionally format partition if tools available
local function command_exists(cmd)
  local f = io.popen('command -v ' .. cmd .. ' >/dev/null 2>&1; echo $?')
  local out = f:read('*l')
  f:close()
  return out == '0'
end

if command_exists('losetup') and command_exists('mkfs.ext4') then
  local loop = io.popen('losetup --show -f -P ' .. IMG):read('*l')
  if loop and loop ~= '' then
    local part = loop .. 'p1'
    print('Formatting partition ' .. part)
    os.execute(string.format('mkfs.ext4 -F "%s"', part))
    os.execute('losetup -d ' .. loop)
  else
    print('losetup failed to create loop device')
  end
else
  print('losetup or mkfs.ext4 not available — image created without a filesystem')
end

print('Done: ' .. IMG)
