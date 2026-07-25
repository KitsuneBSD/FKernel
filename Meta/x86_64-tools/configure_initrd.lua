
local config_file = "build/initrd_config.txt"
os.execute("mkdir -p build")

local function run_dialog(cmd)
  local handle = io.popen(cmd .. " 2>&1")
  local result = handle:read("*a")
  handle:close()
  return result
end

local choice = run_dialog("dialog --stdout --title \"FKernel Boot Selection\" --menu \"Choose System Style:\" 10 50 2 " ..
  "busybox \"BusyBox (Standard)\" " ..
  "openrc \"OpenRC (Advanced)\" ")

if choice == "" then os.exit(0) end

local f = io.open(config_file, "w")
if f then
  if choice == "openrc" then
    f:write("SYSTEM_TYPE=advanced\n")
  else
    f:write("SYSTEM_TYPE=standard\n")
  end
  f:write("COMPONENTS=\n") -- Nenhuma aplicação userland nativa necessária
  f:close()
  print(">>> Boot style set to: " .. choice)
else
  print(">>> Error: Failed to save configuration.")
end

