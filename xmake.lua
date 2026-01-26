add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)
set_targetdir("build")
set_objectdir("build/objs")

set_languages("cxx20", "c17")

local flags = {
  general = {
    cxx = {
      "-ffreestanding",
      "-fno-threadsafe-statics",
      "-fno-exceptions",
      "-fno-rtti",
      "-fno-stack-protector",
      "-fno-use-cxa-atexit",
      "-fno-pic",
      "-fno-omit-frame-pointer",
      "-nostdlib",
      "-nostdinc",
      "-D__fkernel__",
      "-U__linux__",
      "-U__linux",
      "-Ulinux",
      "-Wno-constant-conversion",
      "-Wno-c++11-narrowing",
    },

    asm = {
      "-w-label-orphan",
      "-w-implicit-abs-deprecated",
      "-w-other",
    },

    ld = {
      "-T Config/linker.ld",
      "-nostdlib",
      "-z max-page-size=0x1000",
    },
  },

  x86_64 = {
    cxx = {
      "--target=x86_64-fkernel",
      "-mcmodel=kernel",
      "-mno-sse",
      "-mno-avx",
    },

    asm = {
      "-f elf64",
    },
  },
}

local kernel_non_architecture_related = {
  "Src/Kernel/Boot/**.cpp",
  "Src/Kernel/Clock/**.cpp",
  "Src/Kernel/Driver/**.cpp",
  "Src/Kernel/Hardware/**.cpp",
  "Src/Kernel/Init/**.cpp",
  "Src/Kernel/Memory/**.cpp",
  "Src/Kernel/Fs/**.cpp",
  "Src/Kernel/Ipc/**.cpp",
  "Src/Kernel/Posix/**.cpp",
  "Src/Kernel/Loader/**.cpp",
  "Src/Kernel/Scheduler/**.cpp",
  "Src/Kernel/Syscall/**.cpp",
}

toolchain("FKernel_Compiling")
set_kind("standalone")

local toolchain_bin = os.getenv("HOME") .. "/.fkernel/toolchain/bin/"

if os.exists(toolchain_bin .. "x86_64-fkernel-clang") then
  set_toolset("cc", toolchain_bin .. "x86_64-fkernel-clang")
  set_toolset("cxx", toolchain_bin .. "x86_64-fkernel-clang")
else
  set_toolset("cc", "clang", "gcc")
  set_toolset("cxx", "clang++", "g++")
end

if os.exists(toolchain_bin .. "x86_64-fkernel-ld.lld") then
  set_toolset("ld", toolchain_bin .. "x86_64-fkernel-ld.lld")
else
  set_toolset("ld", "ld.lld", "ld")
end

if os.exists(toolchain_bin .. "fkernel-nasm") then
  set_toolset("as", toolchain_bin .. "fkernel-nasm")
else
  set_toolset("as", "nasm")
end

toolchain_end()

option("initrd_mode")
    set_default("busybox")
    set_values("busybox", "openrc")
    set_description("Select initrd system style")

target("FKernel")
set_kind("binary")
set_toolchains("FKernel_Compiling")

before_build(function(target)
  local clang = path.join(toolchain_bin, "x86_64-fkernel-clang")
  local lld = path.join(toolchain_bin, "x86_64-fkernel-ld.lld")

  if not os.exists(clang) or not os.exists(lld) then
    print(">>> Custom FKernel toolchain (clang/lld) not found in " .. toolchain_bin)
    print(">>> Starting automatic toolchain build (this may take a long time)...")

    os.execv("lua", { "Meta/Toolchain/build_clang.lua" })
    os.execv("lua", { "Meta/Toolchain/build_lld.lua" })
    os.execv("lua", { "Meta/Toolchain/build_nasm.lua" })
    os.execv("lua", { "Meta/Toolchain/build_lua.lua" })

    print(">>> Toolchain built successfully!")
    print(">>> Please run 'xmake config -c' to ensure the new tools are correctly detected.")
    os.raise("Toolchain was missing and has been built. Re-run xmake to continue.")
  end
end)

set_default(true)
set_filename("FKernel.bin")

set_license("BSD-3-Clause")
set_warnings("allextra", "error")
add_includedirs("Include")

if is_mode("debug") then
  set_symbols("debug")
  set_optimize("fast")
  add_defines("FKERNEL_DEBUG")

  if is_arch("x86_64", "x64") then
    add_cxflags(flags.x86_64.cxx)
  end

  --TODO: add tests load on the kernel if this mode is setted
end

if is_mode("release") then
  set_symbols("hidden")
  set_optimize("faster")
  set_strip("all")
end

add_cxflags(flags.general.cxx, { force = true })
add_asflags(flags.general.asm, { force = true })
add_ldflags(flags.general.ld, { force = true })

add_files("Src/LibC/**.c")
add_files("Src/LibC/**.cpp")
add_files("Src/LibFK/**.cpp")

if is_arch("x86_64", "x64") then
  add_cxflags(flags.x86_64.cxx)
  add_asflags(flags.x86_64.asm)

  add_files("Src/Kernel/Arch/x86_64/**.asm")
  add_files("Src/Kernel/Arch/x86_64/**.cpp")
end

add_files(kernel_non_architecture_related)
add_defines("FKERNEL_DEBUG")

if is_arch("x86_64", "x64") then
  after_link(function(target)
    os.execv("lua Meta/x86_64-tools/mount_mockos.lua")
  end)

  on_run(function(target)
    os.execv("lua Meta/x86_64-tools/run_mockos.lua")
  end)
end

on_clean(function(target)
  os.execv("rm -rf Build")
  os.execv("rm -rf build")
  os.execv("rm -rf logs/")
end)

target_end()

task("setup-hda")
set_menu({
  usage = "xmake setup-hda",
  description = "Create and format the FKernel-HDA.qcow2 disk image with MBR and FAT32",
})
on_run(function()
  os.execv("lua Meta/x86_64-tools/create_hda.lua")
end)
task_end()

task("build-initrd")
set_menu({
  usage = "xmake build-initrd",
  description = "Compile and package the initrd (BusyBox or OpenRC)",
})
on_run(function()
  os.execv("lua", {"Meta/x86_64-tools/mount_mockos.lua", "--only-initrd"})
end)
task_end()

task("config-initrd")
set_menu({
  usage = "xmake config-initrd",
  description = "Configure items to include in the initrd.tar using an interactive menu",
})
on_run(function()
  os.execv("lua Meta/x86_64-tools/configure_initrd.lua")
end)
task_end()

task("analyze")
set_category("plugin")

set_menu({
  usage = "xmake analyze",
  description = "Run the script to analyze the kernel runtime",
})

on_run(function()
  os.execv("lua Meta/x86_64-tools/analyze_kernel_runtime.lua")
end)
task_end()
