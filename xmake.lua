add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)
set_targetdir("build")
set_objectdir("build/objs")

set_languages("cxx20")

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
			"--target=x86_64-unknown-none-elf",
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
	"Src/Kernel/Block/**.cpp",
	"Src/Kernel/Driver/**.cpp",
	"Src/Kernel/FileSystem/**.cpp",
	"Src/Kernel/Hardware/**.cpp",
	"Src/Kernel/Init/**.cpp",
	"Src/Kernel/MemoryManager/**.cpp",
	"Src/Kernel/Posix/**.cpp",
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

set_license("BSD-3-Clause")
set_warnings("allextra", "error")

if is_mode("debug") then
	set_symbols("debug")
	set_optimize("fast")
	add_defines("FKERNEL_DEBUG")

	--TODO: add tests load on the kernel if this mode is setted
end

if is_mode("release") then
	set_symbols("hidden")
	set_optimize("faster")
	set_strip("all")
end

add_includedirs("Include")

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

if is_arch("x86_64", "x64") then
	after_link(function(target)
		os.execv("lua Meta/x86_64-tools/mount_mockos.lua")
	end)

	on_run(function(target)
		os.execv("lua Meta/x86_64-tools/run_mockos.lua")
	end)
end

target_end()
