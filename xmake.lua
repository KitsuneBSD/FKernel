add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)
set_targetdir("build")
set_objectdir("build/objs")

set_languages("cxx20", "c17")
set_version("0.0.1")

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
	"Src/Kernel/Io/**.cpp",
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

add_defines("__FKERNEL_FREESTANDING__")
add_cxflags(flags.general.cxx, { force = true })
add_asflags(flags.general.asm, { force = true })
add_ldflags(flags.general.ld, { force = true })

if is_mode("debug") then
	set_symbols("debug")
	set_optimize("fast")
	add_defines("FKERNEL_DEBUG", "FKERNEL_LOG_LEVEL=3")
	if is_arch("x86_64", "x64") then
		add_cxflags(flags.x86_64.cxx)
	end
elseif is_mode("release") then
	add_defines("FKERNEL_LOG_LEVEL=2") -- strip debug; keep warn/error/fatal
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

after_link(function(target)
	os.execv("lua", { "Meta/x86_64-tools/mount_mockos.lua" })
end)

on_run(function(target)
	os.execv("lua", { "Meta/x86_64-tools/run_mockos.lua" })
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
add_files("tests/LibFK/test_traits.cpp")
add_files("tests/LibFK/test_circular_buffer.cpp")
add_files("tests/LibFK/test_containers.cpp")
add_files("tests/LibFK/test_smart_pointers.cpp")
add_files("tests/LibFK/test_text.cpp")
add_files("tests/LibFK/test_multi_containers.cpp")
add_files("tests/LibFK/test_tuple.cpp")
add_files("tests/LibFK/test_stack_queue_staticvec.cpp")
add_files("tests/LibFK/test_nonnull_weak_bump.cpp")
add_files("tests/LibFK/test_lock_rank_format.cpp")
add_files("tests/LibFK/test_string_builder.cpp")
add_files("tests/LibFK/test_bitmap_unordered_set.cpp")
add_files("tests/LibFK/test_algorithms.cpp")
add_files("tests/LibFK/test_string_view.cpp")
add_files("tests/LibFK/test_fixed_string.cpp")
add_files("tests/LibFK/test_byte_checksum.cpp")
add_files("tests/LibFK/test_byte_order.cpp")
add_files("tests/LibFK/test_crc32.cpp")

-- Kernel unit tests (host-side with mocked hardware)
add_files("tests/Kernel/test_file_lock.cpp")
add_files("tests/Kernel/test_cspace.cpp")
add_files("tests/Kernel/test_elf_header.cpp")
add_files("tests/Kernel/test_slab_free_list.cpp")
add_files("tests/Kernel/test_buddy_state.cpp")
add_files("tests/Kernel/test_zone_allocator.cpp")
add_files("tests/Kernel/test_qos.cpp")
add_files("tests/Kernel/test_dentry.cpp")
add_files("tests/Kernel/test_buddy_allocator.cpp")
add_files("tests/Kernel/test_slab_allocator.cpp")
add_files("tests/Kernel/stubs/memory_stubs.cpp")
add_files("Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_state.cpp")
add_files("Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.cpp")
add_files("Src/Kernel/Memory/ObjectMemory/slab_allocator.cpp")
add_files("Src/Kernel/Memory/ObjectMemory/Zone/zone_allocator.cpp")
add_files("Src/Kernel/Fs/Vfs/FileLock/file_lock_list.cpp")
add_files("Src/Kernel/Scheduler/Qos/qos.cpp")
add_files("Src/Kernel/Fs/Vfs/Core/dentry.cpp")
add_files("Src/Kernel/Fs/Vfs/Core/dentry_node_stack.cpp")
add_files("Src/Kernel/Fs/Vfs/Core/node.cpp")
add_files("tests/Kernel/stubs/vfs_stubs.cpp")

add_files("Src/LibFK/Text/string.cpp")
add_files("Src/LibFK/Text/string_builder.cpp")
add_files("Src/LibFK/Memory/Allocators/new.cpp")
add_files("Src/LibFK/Algorithms/Crypto/crc32.cpp")
add_files("Src/LibFK/Algorithms/Crypto/djb2.cpp")
add_files("Src/LibFK/Algorithms/Logging/log_targets.cpp")
add_files("tests/test_mock.c")
add_files("tests/test_mock.cpp")

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

-- TASK: setup-hda — Create and partition the disk image
task("setup-hda")
set_menu({
	usage = "xmake setup-hda",
	description = "Create a partitioned FAT32 disk image (build/FKernel-HDA.qcow2)",
})
on_run(function()
	os.execv("lua", { "Meta/x86_64-tools/create_hda.lua" })
end)
task_end()

-- TASK: build-initrd — Build initrd only (no ISO)
task("build-initrd")
set_menu({
	usage = "xmake build-initrd",
	description = "Build the initrd TAR (build/initrd.tar) without rebuilding the ISO",
})
on_run(function()
	os.execv("lua", { "Meta/x86_64-tools/mount_mockos.lua", "--only-initrd" })
end)
task_end()

-- TASK: config-initrd — Configure initrd system style
task("config-initrd")
set_menu({
	usage = "xmake config-initrd",
	description = "Interactively configure the initrd system style (busybox / openrc)",
})
on_run(function()
	os.execv("lua", { "Meta/x86_64-tools/configure_initrd.lua" })
end)
task_end()

-- TASK: analyze — Analyze kernel runtime logs
task("analyze")
set_menu({
	usage = "xmake analyze",
	description = "Analyze kernel runtime logs",
})
on_run(function()
	os.execv("lua", { "Meta/x86_64-tools/analyze_kernel_runtime.lua" })
end)
task_end()

-- TASK: check-layers — Verify LibC → LibFK → Kernel layer separation
task("check-layers")
set_menu({
	usage = "xmake check-layers",
	description = "Verify LibC → LibFK → Kernel layer separation",
})
on_run(function()
	os.execv("lua", { "Meta/x86_64-tools/check_layer_separation.lua" })
end)
task_end()

-- TASK: check-structs — Verify one struct/class per file rule
task("check-structs")
set_menu({
	usage = "xmake check-structs",
	description = "Verify one struct/class per file and no nested types (AGENTS.md Secret Rule)",
})
on_run(function()
	os.execv("lua", { "Meta/x86_64-tools/check_one_struct_per_file.lua" })
end)
task_end()

-- TASK: check-syscalls — Verify one syscall handler per file rule
task("check-syscalls")
set_menu({
	usage = "xmake check-syscalls",
	description = "Verify one syscall handler per file in syscall_list/ (AGENTS.md Secret Rule)",
})
on_run(function()
	os.execv("lua", { "Meta/x86_64-tools/check_one_syscall_per_file.lua" })
end)
task_end()

-- TASK: check-fp — Verify -fno-omit-frame-pointer is actually applied
task("check-fp")
set_menu({
	usage = "xmake check-fp",
	description = "Verify -fno-omit-frame-pointer is enforced (frame pointers kept for backtraces)",
})
on_run(function()
	local toolchain = os.getenv("HOME") .. "/.fkernel/toolchain/bin/"
	local cc = toolchain .. "x86_64-fkernel-clang"
	if not os.exists(cc) then
		cc = "clang++"
	end
	local dir = os.tmpdir() .. "/fkernel_fp_probe"
	os.mkdir(dir)
	local probe = dir .. "/fp_probe.cpp"
	io.writefile(probe, "int probe(int a) { return a + 1; }\n")
	local flags = {
		"--target=x86_64-fkernel",
		"-ffreestanding",
		"-fno-exceptions",
		"-fno-rtti",
		"-mcmodel=kernel",
		"-mno-sse",
		"-mno-avx",
		"-fno-omit-frame-pointer",
		"-S",
		"-o",
		"-",
		probe,
	}
	local out = os.iorunv(cc, flags)
	os.rm(probe)
	if not out or out == "" then
		os.raise("check-fp: compiler probe failed (is the FKernel toolchain available?)")
	end
	if out:find("rbp") then
		print("check-fp: OK - -fno-omit-frame-pointer is active")
	else
		os.raise("check-fp: FAILED - no rbp frame pointer emitted; -fno-omit-frame-pointer missing?")
	end
end)
task_end()
