local Compiler = require("Meta.Lib.userland_compiler")
local RunCommand = require("Meta.Lib.run_command")
local PrintMessage = require("Meta.Lib.print_message")

local ComponentManager = {}

function ComponentManager.build(name, type, staging_path)
    local src_path = "Src/Userland/" .. name
    local obj_dir = "build/userland/obj"
    RunCommand("mkdir -p " .. obj_dir)

    -- Prioridade 1: Projeto Externo (Se tiver um build.lua na pasta)
    local build_script = src_path .. "/build.lua"
    if io.open(build_script, "r") then
        PrintMessage(false, "Running external build script for: " .. name)
        return RunCommand("lua " .. build_script .. " " .. staging_path)
    end

    -- Prioridade 2: C Nativo
    local c_main = src_path .. "/main.c"
    if io.open(c_main, "r") then
        local obj = obj_dir .. "/" .. name .. ".o"
        local bin = (name == "init") and (staging_path .. "/sbin/init") or (staging_path .. "/bin/" .. name)
        if Compiler.compile_c(c_main, obj) then
            RunCommand("rm -f " .. bin)
            return Compiler.link_elf({"build/userland/obj/crt0.o", obj, "build/userland/obj/syscalls.o"}, bin, "Src/Userland/linker.ld")
        end
    end

    -- Prioridade 3: Assembly Nativo
    local asm_main = src_path .. "/main.asm"
    if io.open(asm_main, "r") then
        local obj = obj_dir .. "/" .. name .. ".o"
        local bin = (name == "init") and (staging_path .. "/sbin/init") or (staging_path .. "/bin/" .. name)
        if Compiler.compile_asm(asm_main, obj) then
            RunCommand("rm -f " .. bin)
            return Compiler.link_elf({obj, "build/userland/obj/syscalls.o"}, bin, "Src/Userland/linker.ld")
        end
    end

    return false
end

return ComponentManager
