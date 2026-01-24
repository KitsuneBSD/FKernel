local RunCommand = require("Meta.Lib.run_command")
local PrintMessage = require("Meta.Lib.print_message")
local Toolchain = require("Meta.Lib.toolchain")

local UserlandCompiler = {}

function UserlandCompiler.compile_c(src, obj, sysroot)
    local cc = Toolchain.get_clang()
    print("  [CC]  " .. src .. " -> " .. obj)
    local include_path = sysroot and ("-I " .. sysroot .. "/usr/include") or "-I Src/Userland/lib"
    local cmd = string.format("%s --target=%s -ffreestanding -nostdlib -fno-stack-protector -O2 %s -D__fkernel__ -U__linux__ -U__linux -Ulinux -c %s -o %s", 
                               cc, Toolchain.TRIPLE, include_path, src, obj)
    if RunCommand(cmd) then
        return true
    end
    PrintMessage(true, "Failed to compile C: " .. src)
    return false
end

function UserlandCompiler.compile_asm(src, obj)
    local as = Toolchain.get_nasm()
    print("  [ASM] " .. src .. " -> " .. obj)
    local cmd = string.format("%s -f elf64 %s -o %s", as, src, obj)
    if RunCommand(cmd) then
        return true
    end
    PrintMessage(true, "Failed to compile: " .. src)
    return false
end

function UserlandCompiler.link_elf(objs, output, linker_script)
    local ld = Toolchain.get_ld()
    print("  [LD]  Linking -> " .. output)
    local obj_list = table.concat(objs, " ")
    local cmd = string.format("%s -T %s %s -o %s", ld, linker_script, obj_list, output)
    if RunCommand(cmd) then
        return true
    end
    PrintMessage(true, "Failed to link: " .. output)
    return false
end

return UserlandCompiler