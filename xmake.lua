add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

set_targetdir("build")
set_objectdir("build/objs")

set_languages("c++23")

local clang_flags = {
	"-g",
	"-fno-threadsafe-statics",
	"-fno-exceptions",
	"-fno-rtti",
	"-fno-stack-protector",
}
local nasm_flags = {
	"-f elf64",
	"-g",
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

set_warnings(all)

add_cxflags(clang_flags)
add_asflags(nasm_flags)
add_ldflags(lld_flags)

before_build(function(target)
	os.execv("bash Meta/run_cppcheck.sh")
end)

after_link(function(target)
	os.execv("bash Meta/mounting_mockos.sh")
end)

on_run(function(target)
	os.execv("bash Meta/run_mockOS.sh")
	--os.execv("bash Meta/run_kernel.sh")
end)

add_includedirs("Include", "Include/Kernel", "Include/Lib")
add_files("Src/Kernel/**.asm")
add_files("Src/Kernel/**.cpp")
add_files("Src/Lib/**.cpp")
