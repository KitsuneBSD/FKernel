local Toolchain = require("Meta.Lib.toolchain")
local PrintMessage = require("Meta.Lib.print_message")

print(">>> Checking LLD Toolchain <<<")

local lld_binary = Toolchain.BIN_DIR .. "/" .. Toolchain.TRIPLE .. "-ld.lld"
local f = io.open(lld_binary, "r")
if f then
    f:close()
    PrintMessage(false, "LLD is already built and installed (unified with Clang).")
    return
end

-- If for some reason it's not there, we'd need to build it, 
-- but since build_clang.lua now builds both, this shouldn't happen 
-- unless the user runs this script manually before build_clang.lua.
-- For safety, we could call build_clang.lua here, but let's just 
-- warn the user.
PrintMessage(true, "LLD not found. It should be built by build_clang.lua.")
PrintMessage(true, "Please run build_clang.lua first for a unified build.")
os.exit(1)

