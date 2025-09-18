add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

set_targetdir("build")
set_objectdir("build/objs")

set_languages("cxx20")

local clang_flags = {
	"-ffreestanding",
	"-fno-threadsafe-statics",
	"-fno-exceptions",
	"-fno-rtti",
	"-fno-stack-protector",
	"-nostdlib",
	"-nostdinc",
	"-mcmodel=kernel",
	"-mno-sse",
	"-mno-avx",
	"-fno-pic",
}

local nasm_flags = {
	"-f elf64",
	"-w-label-orphan",
	"-w-other",
}

local lld_flags = {
	"-T Config/linker.ld",
	"-nostdlib",
}

toolchain("FKernel_Compiling")
set_kind("standalone")

set_toolset("cc", "clang", "tcc", "cl", "gcc")
set_toolset("cxx", "clang++", "cl", "g++")
set_toolset("ld", "ld.lld", "gold", "link", "ld")
set_toolset("as", "nasm", "yasm", "ml")

toolchain_end()

target("FKernel")
set_kind("binary")
set_default(true)
set_filename("FKernel.bin")

if is_mode("debug") then
	set_symbols("debug")
	set_optimize("fast")
	add_defines("FKERNEL_DEBUG")
end

if is_mode("release") then
	set_symbols("hidden")
	set_optimize("faster")
end

after_link(function(target)
	os.execv("lua Meta/mounting_mockos.lua")
end)

on_clean(function(target)
	os.execv("rm -rf build")
end)

on_run(function(target)
	os.execv("lua Meta/run_mockos.lua")
end)

set_warnings("allextra", "error")

add_cxflags(clang_flags, { force = true })
add_asflags(nasm_flags, { force = true })
add_ldflags(lld_flags, { force = true })

add_includedirs("Include")

if is_arch("x86_64", "x64") then
	add_files("Src/Kernel/Arch/x86_64/**.asm")
	add_files("Src/Kernel/Arch/x86_64/**.cpp")
end

add_files("Src/Kernel/Init/**.cpp")
add_files("Src/Kernel/Driver/**.cpp")

add_files("Src/LibC/**.c")
add_files("Src/LibC/**.cpp")
add_files("Src/LibFK/**.cpp")
target_end()

target("Test")
set_default(false)
set_kind("binary")

set_toolchains("gcc", "clang")

add_includedirs("Include", "/usr/include/")
add_files("Test/test_runner.c")
add_files("Test/LibC/**.c")
add_files("Test/LibFK/**.cpp")
add_files("Src/LibC/**.c")
add_files("Src/LibFK/**.cpp")

add_defines("FKERNEL_TEST")

set_languages("c17", "cxx20")

target_end()
