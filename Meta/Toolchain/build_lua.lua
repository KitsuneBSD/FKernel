local BuildUtils = require("Meta.Lib.build_utils")
local RunCommand = require("Meta.Lib.run_command")
local Toolchain = require("Meta.Lib.toolchain")
local PrintMessage = require("Meta.Lib.print_message")

print(">>> Building Custom Lua <<<")

os.execute("mkdir -p " .. Toolchain.BIN_DIR)
os.execute("mkdir -p build")

if not os.exists("build/lua") then
    RunCommand("git clone https://github.com/lua/lua build/lua --depth 1")
end

local src_dir = "build/lua"
if BuildUtils.make(src_dir) then
    os.execute("cp " .. src_dir .. "/lua " .. Toolchain.BIN_DIR .. "/fkernel-lua")
    PrintMessage(false, "Lua successfully installed in " .. Toolchain.BIN_DIR)
end
