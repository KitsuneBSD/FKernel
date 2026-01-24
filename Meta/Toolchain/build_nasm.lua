local BuildUtils = require("Meta.Lib.build_utils")
local RunCommand = require("Meta.Lib.run_command")
local Toolchain = require("Meta.Lib.toolchain")
local PrintMessage = require("Meta.Lib.print_message")

print(">>> Building Custom NASM <<<")

os.execute("mkdir -p " .. Toolchain.BIN_DIR)
os.execute("mkdir -p build")

if not os.execute("test -d build/nasm") then
    RunCommand("git clone https://github.com/netwide-assembler/nasm build/nasm --depth 1")
end

local src_dir = "build/nasm"
if os.execute("cd " .. src_dir .. " && ./autogen.sh && ./configure") then
    if BuildUtils.make(src_dir) then
        os.execute("cp " .. src_dir .. "/nasm " .. Toolchain.BIN_DIR .. "/fkernel-nasm")
        PrintMessage(false, "NASM successfully installed in " .. Toolchain.BIN_DIR)
    end
end
