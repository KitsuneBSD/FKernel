local Toolchain = {}

Toolchain.TRIPLE = "x86_64-fkernel"
Toolchain.BASE_DIR = os.getenv("HOME") .. "/.fkernel/toolchain"
Toolchain.BIN_DIR = Toolchain.BASE_DIR .. "/bin"

function Toolchain.get_tool(name, fallback)
    local local_tool = Toolchain.BIN_DIR .. "/" .. name
    local f = io.open(local_tool, "r")
    if f then
        f:close()
        return local_tool
    end
    return fallback
end

function Toolchain.get_clang()
    return Toolchain.get_tool("x86_64-fkernel-clang", "clang")
end

function Toolchain.get_ld()
    return Toolchain.get_tool("x86_64-fkernel-ld.lld", "ld.lld")
end

function Toolchain.get_nasm()
    return Toolchain.get_tool("fkernel-nasm", "nasm")
end

return Toolchain
