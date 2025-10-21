#!/usr/bin/env lua
local io = io
local os = os
local print = print

local IMG = './gpt.img'
local SIZE = '256M'

print(string.format('Creating GPT image: %s (%s)', IMG, SIZE))
os.execute(string.format('qemu-img create -f raw "%s" "%s"', IMG, SIZE))

local rc = os.execute(string.format('sgdisk -o "%s"', IMG))
if not rc then error('sgdisk -o failed') end

rc = os.execute(string.format('sgdisk -n 1:2048:32767 -t 1:8300 -c 1:"primary1" "%s"', IMG))
if not rc then error('sgdisk -n 1 failed') end
rc = os.execute(string.format('sgdisk -n 2:32768:65535 -t 2:8300 -c 2:"primary2" "%s"', IMG))
if not rc then error('sgdisk -n 2 failed') end

local function command_exists(cmd)
  local f = io.popen('command -v ' .. cmd .. ' >/dev/null 2>&1; echo $?')
  local out = f:read('*l')
  f:close()
  return out == '0'
end

if command_exists('losetup') and command_exists('mkfs.ext4') then
  local loop = io.popen('losetup --show -f -P ' .. IMG):read('*l')
  if loop and loop ~= '' then
    os.execute(string.format('mkfs.ext4 -F %sp1', loop))
    os.execute(string.format('mkfs.ext4 -F %sp2', loop))
    os.execute('losetup -d ' .. loop)
  else
    print('losetup failed to attach image; filesystems not created')
  end
else
  print('losetup or mkfs.ext4 not available — GPT image created without filesystems')
end

print('Done: ' .. IMG)
