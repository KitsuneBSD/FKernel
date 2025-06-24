add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

set_targetdir("build")
set_objectdir("build/objs")

set_languages("cxx20")

local clang_flags = {
	"-fno-threadsafe-statics",
	"-fno-exceptions",
	"-fno-rtti",
	"-Wno-gnu-line-marker",
}
local nasm_flags = {
	"-f elf64",
}

local lld_flags = {
	"-T Config/linker.ld",
}
toolchain("FKernel_Compiling")
set_kind("standalone")
set_toolset("cc", "clang")
set_toolset("cxx", "clang++")
set_toolset("ld", "ld.lld")
set_toolset("as", "nasm")
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

	before_build(function(target)
		os.execv("bash Meta/run_continuous_integration.sh")
	end)
end

after_link(function(target)
	os.execv("bash Meta/mounting_mockos.sh")
end)

on_clean(function(target)
	os.execv("rm -rf build")
end)

on_run(function(target)
	os.execv("bash Meta/run_mockos.sh")
end)

set_warnings("everything")

add_cxflags(clang_flags)
add_asflags(nasm_flags)
add_ldflags(lld_flags)

add_includedirs("Include")

if is_arch("x86_64") then
	add_files("Src/Kernel/Arch/x86_64/Boot/**.asm")
	add_files("Src/Kernel/Arch/x86_64/Section/**.asm")
end

add_files("Src/Kernel/Init/**.cpp")

target_end()
