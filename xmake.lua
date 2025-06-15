add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

set_languages("cxx20")

set_targetdir("build")

toolchain("FKernel_Compiling")
set_kind("standalone")
set_toolset("cc", "clang")
set_toolset("cxx", "clang++")
set_toolset("ld", "ld.lld")
set_toolset("as", "nasm")
toolchain_end()

-- NOTE: This flags isn't used in default xmake so, we need enforcing them
local cxxflags_osdev = {
	"-ffreestanding",
	"-nostdinc",
	"-nostdlib",
	"-fno-threadsafe-statics",
	"-fno-exceptions",
	"-fno-rtti",
	"-fno-stack-protector",
	"-Wno-gnu-line-marker",
}

-- NOTE: We need enforcing the elf binary first
local nasm_flags = {
	"-f elf64",
}

-- NOTE: Enforcing the use of Config/Linker.ld file
local linker_flags = {
	"-T Config/linker.ld",
	"-nostdlib",
}

target("FKernel")
set_kind("binary")
set_default(true)
set_filename("FKernel.bin")

set_warnings("everything")

if is_mode("release") then
	set_optimize("faster")
	set_symbols("hidden")
end

if is_mode("debug") then
	set_optimize("none")
	set_symbols("debug")
	add_defines("FKERNEL_DEBUG")
end

add_cxxflags(cxxflags_osdev)
add_asflags(nasm_flags)
add_ldflags(linker_flags)

add_includedirs("Include", "Include/Kernel", "Include/LibFK", "Include/LibC")

add_files("Src/Kernel/Boot/**.cpp")
add_files("Src/Kernel/Driver/**.cpp")
add_files("Src/Kernel/Init/**.cpp")
add_files("Src/LibFK/**.cpp")
add_files("Src/LibC/**.cpp")

if is_arch("x86_64") then
	add_files("Src/Kernel/Boot/Arch/x86_64/**.asm")
	add_files("Src/Kernel/Arch/x86_64/**.cpp")
	add_files("Src/Kernel/Arch/x86_64/**.asm")

	add_includedirs("Src/Kernel/Arch/x86_64")
end

before_build(function(target)
	os.exec("bash Meta/run_cppcheck.sh")
end)

after_link(function(target)
	os.exec("bash Meta/mounting_mockos.sh")
end)

on_run(function(target)
	os.exec("bash Meta/run_mockos.sh")
end)

on_clean(function(target)
	os.exec("rm -rf build")
end)

target_end()
