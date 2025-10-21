#!/usr/bin/env lua
local io = io
local os = os
local print = print

local IMG = './ebr.img'
local SIZE = '128M'

print(string.format('Creating EBR (extended MBR) image: %s (%s)', IMG, SIZE))
os.execute(string.format('qemu-img create -f raw "%s" "%s"', IMG, SIZE))

local sfdisk_cmd = string.format("sfdisk --force %s <<'EOF'\nlabel: dos\nlabel-id: 0x87654321\nunit: sectors\n\n1 : start=2048, size=32768, type=83\n2 : start=34816, size=, type=5\nEOF", IMG)
print('+ ' .. sfdisk_cmd)
local rc = os.execute(sfdisk_cmd)
if not rc then error('sfdisk failed') end

local function command_exists(cmd)
  local f = io.popen('command -v ' .. cmd .. ' >/dev/null 2>&1; echo $?')
  local out = f:read('*l')
  f:close()
  return out == '0'
end

if command_exists('losetup') then
  local f = io.popen('losetup --show -f -P ' .. IMG)
  local loop = f:read('*l')
  f:close()
  if loop and loop ~= '' then
    -- Try to append logical partitions via sfdisk
    local append_cmd = string.format("sfdisk --no-reread --force %s <<'EOF'\n,10240,83\n,10240,83\nEOF", IMG)
    print('+ ' .. append_cmd)
    os.execute(append_cmd)
    os.execute('losetup -d ' .. loop)
  else
    print('losetup failed to attach image; logical partitions not created')
  end
else
  print('losetup not available — created extended partition entry but not logical partitions')
end

print('Done: ' .. IMG)
