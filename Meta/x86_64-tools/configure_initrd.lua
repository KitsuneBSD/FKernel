#!/usr/bin/env lua

local userland_dir = "Src/Userland"
local config_file = "build/initrd_config.txt"
os.execute("mkdir -p build")

local function run_dialog(cmd)
  local handle = io.popen(cmd .. " 2>&1")
  local result = handle:read("*a")
  handle:close()
  return result
end

local components = {}
local p = io.popen("ls -d " .. userland_dir .. "/*/")
for line in p:lines() do
  local name = line:match("Userland/([^/]+)/$")
  if name and name ~= "lib" and name ~= "libbsd" and name ~= "init" then
    table.insert(components, name)
  end
end
p:close()

local sys_type = run_dialog("dialog --stdout --title \"System Architecture\" --menu \"Choose Base System:\" 15 60 5 " ..
  "minimal \"Native Shell (Minimalist)\" " ..
  "standard \"Standard (Musl + BusyBox)\" " ..
  "advanced \"Advanced (OpenRC + Full Suite)\" ")

if sys_type == "" then os.exit(0) end

local checklist_cmd = "dialog --stdout --separate-output --checklist \"Select Userland Components:\" 20 60 10 "
checklist_cmd = checklist_cmd .. "init \"System Bootstrapper (Required)\" on "

for _, comp in ipairs(components) do
  checklist_cmd = checklist_cmd .. string.format("%s \"%s Utility\" on ", comp, comp:upper())
end

local selected_items = run_dialog(checklist_cmd)

local f = io.open(config_file, "w")
if f then
  f:write("SYSTEM_TYPE=" .. sys_type .. "\n")
  f:write("COMPONENTS=" .. selected_items:gsub("\n", ",") .. "\n")
  f:close()
  local PrintMessage = require("Meta.Lib.print_message")
  PrintMessage(false, "Deep configuration saved to " .. config_file)
else
  local PrintMessage = require("Meta.Lib.print_message")
  PrintMessage(true, "Failed to save configuration.")
end
