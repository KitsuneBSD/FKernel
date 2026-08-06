-- Product targets: FKernel binary, LibC_Testing static lib, Test host binary.
-- Paths are relative to this file's directory (xmake/), so root-level paths use "../".

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
    "../Src/Kernel/Boot/**.cpp",
    "../Src/Kernel/Clock/**.cpp",
    "../Src/Kernel/Driver/**.cpp",
    "../Src/Kernel/Hardware/**.cpp",
    "../Src/Kernel/Init/**.cpp",
    "../Src/Kernel/Memory/**.cpp",
    "../Src/Kernel/Fs/**.cpp",
    "../Src/Kernel/Ipc/**.cpp",
    "../Src/Kernel/Posix/**.cpp",
    "../Src/Kernel/Loader/**.cpp",
    "../Src/Kernel/Scheduler/**.cpp",
    "../Src/Kernel/Syscall/**.cpp",
    "../Src/Kernel/Net/**.cpp",
    "../Src/Kernel/Io/**.cpp",
}

-- TARGET: FKernel
target("FKernel")
    set_kind("binary")
    set_toolchains("FKernel_Compiling")
    set_default(true)
    set_filename("FKernel.bin")
    set_license("BSD-3-Clause")
    set_warnings("allextra", "error")
    add_includedirs("../Include")

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
        add_defines("FKERNEL_LOG_LEVEL=2")
    end

    add_files("../Src/LibC/**.c")
    add_files("../Src/LibC/**.cpp")
    add_files("../Src/LibFK/**.cpp")

    if is_arch("x86_64", "x64") then
        add_cxflags(flags.x86_64.cxx)
        add_asflags(flags.x86_64.asm)
        add_files("../Src/Kernel/Arch/x86_64/**.asm")
        add_files("../Src/Kernel/Arch/x86_64/**.cpp")
    end

    add_files(kernel_non_architecture_related)

    after_link(function(target)
        os.execv("lua", { "Meta/x86_64-tools/mount_mockos.lua" })
    end)

    on_run(function(target)
        os.execv("lua", { "Meta/x86_64-tools/run_mockos.lua" })
    end)
target_end()

-- TARGET: LibC_Testing (Static library with renamed symbols for host test harness)
target("LibC_Testing")
    set_kind("static")
    set_toolchains("clang")
    add_includedirs("../Include")
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
        "vsnprintf=kernel_vsnprintf",
        "sscanf=kernel_sscanf",
        "vsscanf=kernel_vsscanf"
    )
    add_files("../Src/LibC/string/*.c")
    add_files("../Src/LibC/stdio/vsnprintf.c")
    add_files("../Src/LibC/stdio/snprintf.c")
    add_files("../Src/LibC/stdio/sscanf.c")
    add_files("../Src/LibC/ctype.c")
target_end()

-- TARGET: Test (Host executable — runs against LibC_Testing + mocked kernel stubs)
target("Test")
    set_kind("binary")
    set_default(false)
    set_toolchains("clang")
    add_deps("LibC_Testing")
    add_includedirs("../Include", "..")
    set_languages("cxx20")
    add_defines("FKERNEL_TEST")
    add_cxflags("-g")

    add_files("../tests/main.cpp")
    add_files("../tests/LibC/test_string_memory_comprehensive.cpp")
    add_files("../tests/LibC/test_stdio_comprehensive.cpp")
    add_files("../tests/LibFK/test_traits.cpp")
    add_files("../tests/LibFK/test_circular_buffer.cpp")
    add_files("../tests/LibFK/test_containers.cpp")
    add_files("../tests/LibFK/test_smart_pointers.cpp")
    add_files("../tests/LibFK/test_text.cpp")
    add_files("../tests/LibFK/test_multi_containers.cpp")
    add_files("../tests/LibFK/test_tuple.cpp")
    add_files("../tests/LibFK/test_stack_queue_staticvec.cpp")
    add_files("../tests/LibFK/test_nonnull_weak_bump.cpp")
    add_files("../tests/LibFK/test_lock_rank_format.cpp")
    add_files("../tests/LibFK/test_string_builder.cpp")
    add_files("../tests/LibFK/test_bitmap_unordered_set.cpp")
    add_files("../tests/LibFK/test_algorithms.cpp")
    add_files("../tests/LibFK/test_string_view.cpp")
    add_files("../tests/LibFK/test_fixed_string.cpp")
    add_files("../tests/LibFK/test_optional.cpp")
    add_files("../tests/LibFK/test_byte_checksum.cpp")
    add_files("../tests/LibFK/test_byte_order.cpp")
    add_files("../tests/LibFK/test_crc32.cpp")
    add_files("../tests/LibFK/test_scatter_io.cpp")
    add_files("../tests/LibFK/test_indirect_blocks.cpp")
    add_files("../tests/LibFK/test_bitmap.cpp")
    add_files("../tests/LibFK/test_path.cpp")
    add_files("../tests/LibFK/test_time_math.cpp")
    add_files("../tests/LibFK/test_id_generator.cpp")

    add_files("../tests/Kernel/test_file_lock.cpp")
    add_files("../tests/Kernel/test_cspace.cpp")
    add_files("../tests/Kernel/test_elf_header.cpp")
    add_files("../tests/Kernel/test_slab_free_list.cpp")
    add_files("../tests/Kernel/test_buddy_state.cpp")
    add_files("../tests/Kernel/test_zone_allocator.cpp")
    add_files("../tests/Kernel/test_qos.cpp")
    add_files("../tests/Kernel/test_dentry.cpp")
    add_files("../tests/Kernel/test_buddy_allocator.cpp")
    add_files("../tests/Kernel/test_slab_allocator.cpp")
    add_files("../tests/Kernel/test_errno_abi.cpp")
    add_files("../tests/Kernel/test_turnstile.cpp")
    add_files("../tests/Kernel/test_mlfq_queue.cpp")
    add_files("../tests/Kernel/test_tcp_connection.cpp")
    add_files("../tests/Kernel/test_path_resolver.cpp")
    add_files("../tests/Kernel/test_file_description.cpp")
    add_files("../tests/Driver/Storage/Nvme/test_nvme_refactoring.cpp")
    add_files("../tests/Kernel/stubs/memory_stubs.cpp")
    add_files("../Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_state.cpp")
    add_files("../Src/Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.cpp")
    add_files("../Src/Kernel/Memory/ObjectMemory/slab_allocator.cpp")
    add_files("../Src/Kernel/Memory/ObjectMemory/Zone/zone_allocator.cpp")
    add_files("../Src/Kernel/Fs/Vfs/FileLock/file_lock_list.cpp")
    add_files("../Src/Kernel/Scheduler/Qos/qos.cpp")
    add_files("../Src/Kernel/Fs/Vfs/Core/dentry.cpp")
    add_files("../Src/Kernel/Fs/Vfs/Core/dentry_node_stack.cpp")
    add_files("../Src/Kernel/Fs/Vfs/Core/node.cpp")
    add_files("../Src/Kernel/Fs/Vfs/Core/path_resolver.cpp")
    add_files("../Src/Kernel/Fs/Vfs/Core/file_description.cpp")
    add_files("../Src/Kernel/Scheduler/Sync/turnstile.cpp")
    add_files("../Src/Kernel/Net/Tcp/tcp_connection.cpp")
    add_files("../tests/Kernel/stubs/vfs_stubs.cpp")
    add_files("../tests/Kernel/stubs/vfs_resolver_stubs.cpp")
    add_files("../tests/Kernel/stubs/scheduler_stubs.cpp")

    add_files("../Src/LibFK/Text/string.cpp")
    add_files("../Src/LibFK/Text/string_builder.cpp")
    add_files("../Src/LibFK/Memory/Allocators/new.cpp")
    add_files("../Src/LibFK/Algorithms/Crypto/crc32.cpp")
    add_files("../Src/LibFK/Algorithms/Crypto/djb2.cpp")
    add_files("../Src/LibFK/Algorithms/Logging/log_targets.cpp")
    add_files("../tests/test_mock.c")
    add_files("../tests/test_mock.cpp")

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
