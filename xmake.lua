add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

set_languages("cxx20")

set_targetdir("build")

toolchain("FKernel_Compiling")
set_kind("standalone")
set_toolset("cc", "clang", "tcc", "gcc")
set_toolset("cxx", "clang++", "g++")
set_toolset("ld", "mold", "ld.lld", "clang++", "ld")
set_toolset("as", "nasm", "fasm", "yasm")
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
	"-fno-omit-frame-pointer ",
	"-Wno-gnu-line-marker",
}

-- NOTE: We need enforcing the elf binary first
local nasm_flags = {
	"-f elf64",
	"-w-other",
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
	set_optimize("none")
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

add_includedirs("Include")

-- Compile: LibC
add_files("Src/LibC/**.cpp")

if is_arch("x86_64") then
	add_files("Src/Kernel/Arch/x86_64/**.asm")
end

before_build(function(target)
	os.exec("bash Meta/run_continuous_integration.sh")
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
