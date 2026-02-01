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
  "Src/Kernel/Net/**.cpp",
}

-- Custom Toolchain for Kernel
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

-- TARGET: FKernel
target("FKernel")
  set_kind("binary")
  set_toolchains("FKernel_Compiling")
  set_default(true)
  set_filename("FKernel.bin")
  set_license("BSD-3-Clause")
  set_warnings("allextra", "error")
  add_includedirs("Include")

  add_cxflags(flags.general.cxx, { force = true })
  add_asflags(flags.general.asm, { force = true })
  add_ldflags(flags.general.ld, { force = true })

  if is_mode("debug") then
    set_symbols("debug")
    set_optimize("fast")
    add_defines("FKERNEL_DEBUG")
    if is_arch("x86_64", "x64") then
      add_cxflags(flags.x86_64.cxx)
    end
  end

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

  after_link(function (target)
    os.execv("lua", {"Meta/x86_64-tools/mount_mockos.lua"})
  end)

  on_run(function (target)
    os.execv("lua", {"Meta/x86_64-tools/run_mockos.lua"})
  end)
target_end()

-- TARGET: LibC_Testing (Static library with renamed symbols)
target("LibC_Testing")
  set_kind("static")
  set_toolchains("clang")
  add_includedirs("Include")
  add_cxflags("-fno-builtin", "-g")
  add_defines(
    "memcpy=kernel_memcpy",
    "memset=kernel_memset",
    "memmove=kernel_memmove",
    "memcmp=kernel_memcmp",
    "strlen=kernel_strlen",
    "strcpy=kernel_strcpy",
    "strncpy=kernel_strncpy",
    "strcmp=kernel_strcmp",
    "strncmp=kernel_strncmp",
    "strcat=kernel_strcat",
    "strchr=kernel_strchr",
    "strrchr=kernel_strrchr",
    "atoi=kernel_atoi",
    "itoa=kernel_itoa",
    "stol=kernel_stol",
    "snprintf=kernel_snprintf",
    "vsnprintf=kernel_vsnprintf"
  )
  add_files("Src/LibC/string/*.c")
  add_files("Src/LibC/ctype.c")
target_end()

-- TARGET: Test (Host executable)
target("Test")
  set_kind("binary")
  set_default(false)
  set_toolchains("clang")
  add_deps("LibC_Testing")
  add_includedirs("Include", ".")
  set_languages("cxx20")
  add_defines("FKERNEL_TEST")
  add_cxflags("-g")
  
  -- Add test files
  add_files("tests/main.cpp")
  add_files("tests/LibC/test_string_memory_comprehensive.cpp")
  add_files("tests/LibFK/test_circular_buffer.cpp")
  add_files("tests/test_mock.c")

  on_run(function(target)
    local test_binary = target:targetfile()
    print(">>> Running tests: " .. test_binary)
    local result = os.execv(test_binary, {})
    if result ~= 0 then
      os.raise("Tests failed with exit code: " .. tostring(result))
    else
      print("All tests passed!")
    end
  end)
target_end()
