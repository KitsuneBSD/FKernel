-- Transversal build tools: checks, setup helpers, analysis.
-- These tasks span all modules so they live together, not per-module.

task("setup-hda")
    set_menu({
        usage = "xmake setup-hda",
        description = "Create a partitioned FAT32 disk image (build/FKernel-HDA.qcow2)",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/create_hda.lua" })
    end)
task_end()

task("build-initrd")
    set_menu({
        usage = "xmake build-initrd",
        description = "Build the initrd TAR (build/initrd.tar) without rebuilding the ISO",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/mount_mockos.lua", "--only-initrd" })
    end)
task_end()

task("config-initrd")
    set_menu({
        usage = "xmake config-initrd",
        description = "Interactively configure the initrd system style (busybox / openrc)",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/configure_initrd.lua" })
    end)
task_end()

task("analyze")
    set_menu({
        usage = "xmake analyze",
        description = "Analyze kernel runtime logs",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/analyze_kernel_runtime.lua" })
    end)
task_end()

task("check-layers")
    set_menu({
        usage = "xmake check-layers",
        description = "Verify LibC → LibFK → Kernel layer separation",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/check_layer_separation.lua" })
    end)
task_end()

task("check-structs")
    set_menu({
        usage = "xmake check-structs",
        description = "Verify one struct/class per file and no nested types (AGENTS.md Secret Rule)",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/check_one_struct_per_file.lua" })
    end)
task_end()

task("check-syscalls")
    set_menu({
        usage = "xmake check-syscalls",
        description = "Verify one syscall handler per file in SyscallList/ (AGENTS.md Secret Rule)",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/check_one_syscall_per_file.lua" })
    end)
task_end()

task("check-arch-asm")
    set_menu({
        usage = "xmake check-arch-asm",
        description = "Verify asm/inline-asm files live under Arch/ dirs, not in generic code (AGENTS.md arch_* policy)",
    })
    on_run(function()
        os.execv("lua", { "Meta/x86_64-tools/check_arch_asm.lua" })
    end)
task_end()

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
        local probe_flags = {
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
        local out = os.iorunv(cc, probe_flags)
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
