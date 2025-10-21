#!/usr/bin/env lua
local exec = os.execute

local function run(cmd)
  print("+ ", cmd)
  local ok, _, code = os.execute(cmd)
  if not ok then
    error(string.format("Command failed: %s (exit %s)", cmd, tostring(code)))
  end
end

run('./make_mbr.lua')
run('./make_ebr.lua')
run('./make_gpt.lua')
run('./make_bsd_img.lua')

print('All images created in ' .. io.popen('pwd'):read('*l'))
