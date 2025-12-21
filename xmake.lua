add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)

set_targetdir("build")
set_objectdir("build/objs")

set_languages("cxx20", "c17")

local flags = {
	general = {
		c = {
			"-ffreestanding",
			"-fno-stack-protector",
			"-nostdlib",
			"-nostdinc",
		},
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
		common = {
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

toolchain("FKernel_Compiling")
	set_kind("standalone")
	set_toolset("cc", "clang", "gcc")
	set_toolset("cxx", "clang++", "g++")
	set_toolset("ld", "ld.lld", "ld")
	set_toolset("as", "nasm")
toolchain_end()

target("FKernel_Prekernel")
	set_kind("object")
	set_toolchains("FKernel_Compiling")

	add_cflags(flags.general.c, { force = true })
	add_cxxflags(flags.general.cxx, {force = true})
    add_asflags(flags.general.asm, { force = true })

	if is_arch("x86_64", "x64") then
		add_cflags(flags.x86_64.common)
		add_asflags(flags.x86_64.asm)

        add_files("Src/PreKernel/Arch/x86_64/**.asm")
	end
target_end()

target("FKernel")
	set_kind("binary")
	set_default(true)
	set_filename("FKernel.bin")
	set_toolchains("FKernel_Compiling")

	add_deps("FKernel_Prekernel")

	set_license("BSD-3-Clause")
	set_warnings("allextra", "error")
	add_includedirs("Include")

	local kernel_non_architecture_related = {
        "Src/Kernel/Boot/**.cpp",
	    "Src/Kernel/Clock/**.cpp",
	    "Src/Kernel/Driver/**.cpp",
	    "Src/Kernel/Hardware/**.cpp",
	    "Src/Kernel/Init/**.cpp",
	    "Src/Kernel/Memory/**.cpp",
	    "Src/Kernel/Posix/**.cpp",
	    "Src/Kernel/Scheduler/**.cpp",
    }
	

	add_cxflags(flags.general.cxx, { force = true })
	add_asflags(flags.general.asm, { force = true })
	add_ldflags(flags.general.ld, { force = true })

	if is_arch("x86_64", "x64") then
		add_cxflags(flags.x86_64.common)
		add_asflags(flags.x86_64.asm)
        add_files("Src/Kernel/Arch/x86_64/**.asm")
	    add_files("Src/Kernel/Arch/x86_64/**.cpp")
    end

	if is_mode("debug") then
		set_symbols("debug")
		set_optimize("none") -- aqui é vital
		add_defines("FKERNEL_DEBUG")
	end

	if is_mode("release") then
		set_symbols("hidden")
		set_optimize("faster")
		set_strip("all")
	end

    add_files("Src/LibC/**.c")
    add_files("Src/LibC/**.cpp")
    add_files("Src/LibFK/**.cpp")

    add_files(kernel_non_architecture_related)

	after_link(function ()
		os.execv("lua Meta/x86_64-tools/mount_mockos.lua")
	end)

	on_run(function ()
		os.execv("lua Meta/x86_64-tools/run_mockos.lua")
	end)
target_end()
