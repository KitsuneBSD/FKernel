-- Root: global identity — modes, policies, dirs, languages, toolchain definition.
-- Targets, options, and tasks live in xmake/.

add_rules("mode.debug", "mode.release")
set_policy("check.auto_ignore_flags", false)
set_targetdir("build")
set_objectdir("build/objs")

set_languages("cxx20", "c17")
set_version("0.0.1")

-- Custom Toolchain for Kernel (freestanding x86_64)
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

includes("xmake/options.lua")
includes("xmake/targets.lua")
includes("xmake/tasks.lua")
