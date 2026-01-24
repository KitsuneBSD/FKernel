local RunCommand = require("Meta.Lib.run_command")
local PrintMessage = require("Meta.Lib.print_message")
local Compiler = require("Meta.Lib.userland_compiler")

local InitrdBuilder = {}

function InitrdBuilder.setup_staging(path)
  RunCommand("rm -rf " .. path)
  RunCommand("mkdir -p " .. path .. "/bin")
  RunCommand("mkdir -p " .. path .. "/sbin")
  RunCommand("mkdir -p " .. path .. "/etc")
end

function InitrdBuilder.build_component(name, config, staging_path)
  local comp_src = "Src/Userland/" .. name
  local obj_dir = "build/userland/obj"
  RunCommand("mkdir -p " .. obj_dir)

  local c_main = comp_src .. "/main.c"
  local f_c = io.open(c_main, "r")
  if f_c then
    f_c:close()
    local obj_c = obj_dir .. "/" .. name .. ".o"
    local crt0_obj = obj_dir .. "/crt0.o"
    local bin = (name == "init") and (staging_path .. "/sbin/init") or (staging_path .. "/bin/" .. name)

    Compiler.compile_asm("Src/Userland/lib/crt0.asm", crt0_obj)

    if Compiler.compile_c(c_main, obj_c) then
      RunCommand("rm -f " .. bin)
      return Compiler.link_elf({ crt0_obj, obj_c, "build/userland/obj/syscalls.o" }, bin, "Src/Userland/linker.ld")
    end
  end

  local asm_main = comp_src .. "/main.asm"
  local f = io.open(asm_main, "r")
  if f then
    f:close()
    local obj = obj_dir .. "/" .. name .. ".o"
    local bin = (name == "init") and (staging_path .. "/sbin/init") or (staging_path .. "/bin/" .. name)

    if Compiler.compile_asm(asm_main, obj) then
      RunCommand("rm -f " .. bin)
      return Compiler.link_elf({ obj, "build/userland/obj/syscalls.o" }, bin, "Src/Userland/linker.ld")
    end
  end
  return false
end

function InitrdBuilder.pack_tar(staging_path, output_tar)
  local cmd = string.format("tar -cf %s -C %s .", output_tar, staging_path)
  if RunCommand(cmd) then
    PrintMessage(false, "Packaged initrd: " .. output_tar)
    return true
  end
  return false
end

return InitrdBuilder
