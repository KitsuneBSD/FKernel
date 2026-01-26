#!/usr/bin/env lua

local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local RunCommand = require("Meta.Lib.run_command")

local OPENRC_VERSION = "0.52.1"
local OPENRC_NAME = "openrc-" .. OPENRC_VERSION
local OPENRC_URL = "https://github.com/OpenRC/openrc/archive/refs/tags/" .. OPENRC_VERSION .. ".tar.gz"

-- Infer project root directory
local ROOT_DIR = os.getenv("PWD") or "."
local BUILD_DIR = ROOT_DIR .. "/build/userland/openrc"
local SYSROOT = ROOT_DIR .. "/build/sysroot"
local INITRD_DIR = ROOT_DIR .. "/build/initrd_root"

print("--- FKernel OpenRC Toolchain ---")

-- Previous checks
if not OSInteract.CommandExists("meson") then
    PrintMessage(true, "Meson not found. OpenRC requires meson.")
    os.exit(1)
end

if not OSInteract.FileExists(SYSROOT .. "/lib/libc.a") then
    PrintMessage(true, "Musl libc.a not found in sysroot. Build musl first.")
    os.exit(1)
end

RunCommand("mkdir -p " .. BUILD_DIR)

local tarball = BUILD_DIR .. "/openrc.tar.gz"
if not OSInteract.FileExists(tarball) then
    ArchiveUtils.download(OPENRC_URL, tarball)
end

local src_dir = BUILD_DIR .. "/" .. OPENRC_NAME
if not OSInteract.DirExists(src_dir) then
    ArchiveUtils.extract(tarball, BUILD_DIR)
end

local build_openrc_dir = BUILD_DIR .. "/build"
local openrc_bin = build_openrc_dir .. "/src/openrc/openrc"

if OSInteract.FileExists(openrc_bin) and OSInteract.FileExists(INITRD_DIR .. "/usr/sbin/openrc") then
    PrintMessage(false, "OpenRC is already compiled and installed. Skipping.")
    os.exit(0)
end

-- Generate Meson Cross-File
local cross_file = BUILD_DIR .. "/cross_file.txt"
local f = io.open(cross_file, "w")
f:write("[binaries]\n")
f:write("c = '" .. Toolchain.get_clang() .. "'\n")
f:write("cpp = '" .. Toolchain.get_clang() .. "++'\n")
f:write("ar = '" .. Toolchain.get_tool("llvm-ar", "ar") .. "'\n")
f:write("strip = '" .. Toolchain.get_tool("llvm-strip", "strip") .. "'\n")
f:write("pkgconfig = 'false'\n")
f:write("\n")
f:write("[properties]\n")
f:write("c_args = ['--target=", Toolchain.TRIPLE, "', '-isystem', '" .. SYSROOT .. "/include', '-D__fkernel__', '-D__linux__']\n")
f:write("cpp_args = ['--target=", Toolchain.TRIPLE, "', '-isystem', '" .. SYSROOT .. "/include', '-D__fkernel__', '-D__linux__']\n")
f:write("c_link_args = ['-L" .. SYSROOT .. "/lib', '-static']\n")
f:write("cpp_link_args = ['-L" .. SYSROOT .. "/lib', '-static']\n")
f:write("\n")
f:write("[host_machine]\n")
f:write("system = 'linux'\n")
f:write("cpu_family = 'x86_64'\n")
f:write("cpu = 'x86_64'\n")
f:write("endian = 'little'\n")
f:close()

print("Configuring OpenRC with Meson...")
-- OpenRC options for minimal freestanding system
local cmd_setup = string.format(
    "cd %s && meson setup %s --cross-file %s --prefix=/ --libdir=lib --sysconfdir=/etc " ..
    "-Ddefault_library=static -Dshell=/bin/sh -Dpam=false -Dselinux=disabled -Daudit=disabled " ..
    "-Dbranding='\"FKernel\"' -Dsysvinit=false -Dnewnet=true -Dtermcap='' -Dos=Linux " ..
    "-Dpkgconfig=false -Dbash-completions=false -Dzsh-completions=false",
    src_dir, build_openrc_dir, cross_file
)

if not OSInteract.DirExists(build_openrc_dir) then
    if not RunCommand(cmd_setup) then
        PrintMessage(true, "Failed to configure OpenRC")
        os.exit(1)
    end
end

print("Compiling OpenRC...")
if RunCommand("cd " .. build_openrc_dir .. " && ninja") then
    print("Installing OpenRC to initrd staging via DESTDIR...")
    -- Use ninja install with DESTDIR to get everything (binaries + scripts + config)
    if RunCommand(string.format("cd %s && DESTDIR=%s ninja install", build_openrc_dir, INITRD_DIR)) then
        -- Post-install adjustments
        RunCommand("mkdir -p " .. INITRD_DIR .. "/sbin")
        RunCommand("mv " .. INITRD_DIR .. "/usr/sbin/openrc-init " .. INITRD_DIR .. "/sbin/init.openrc")
        
        -- Setup OpenRC environment for FKernel
        local function setup_fkernel_runlevels()
            local runlevel_services = {
                boot = {"bootmisc", "procps"},
                default = {"local", "network"}
            }
            
            for runlevel, services in pairs(runlevel_services) do
                for _, service in ipairs(services) do
                    local service_path = INITRD_DIR .. "/etc/init.d/" .. service
                    local runlevel_path = INITRD_DIR .. "/etc/runlevels/" .. runlevel .. "/" .. service
                    if OSInteract.FileExists(service_path) then
                        RunCommand("ln -sf /etc/init.d/" .. service .. " " .. runlevel_path)
                    end
                end
            end
        end

        local function setup_openrc_fkernel()
            -- Create OpenRC directory structure
            RunCommand("mkdir -p " .. INITRD_DIR .. "/etc/init.d")
            RunCommand("mkdir -p " .. INITRD_DIR .. "/etc/conf.d") 
            RunCommand("mkdir -p " .. INITRD_DIR .. "/etc/runlevels/boot")
            RunCommand("mkdir -p " .. INITRD_DIR .. "/etc/runlevels/default")
            RunCommand("mkdir -p " .. INITRD_DIR .. "/var/log")
            
            -- Create FKernel-specific rc.conf
            local rc_conf = io.open(INITRD_DIR .. "/etc/rc.conf", "w")
            rc_conf:write("# OpenRC configuration for FKernel\n")
            rc_conf:write("rc_sys=\"FKernel\"\n")
            rc_conf:write("rc_logger=\"YES\"\n")
            rc_conf:write("rc_parallel=\"NO\"\n")  -- Single-core safe
            rc_conf:write("rc_runlevel_lock_timeout=\"60\"\n")
            rc_conf:write("rc_hotplug=\"NO\"\n")  -- No hotplug support
            rc_conf:close()
            
            -- Copy FKernel-specific services
            local services_dir = ROOT_DIR .. "/Meta/UserTools/openrc/fkernel-services"
            if OSInteract.DirExists(services_dir) then
                RunCommand("cp " .. services_dir .. "/* " .. INITRD_DIR .. "/etc/init.d/")
                RunCommand("chmod +x " .. INITRD_DIR .. "/etc/init.d/*")
            end
            
            -- Setup default runlevels
            setup_fkernel_runlevels()
        end

        setup_openrc_fkernel()
        PrintMessage(false, "OpenRC successfully installed to initrd.")
    else
        PrintMessage(true, "Failed to install OpenRC")
        os.exit(1)
    end
else
    PrintMessage(true, "Failed to compile OpenRC")
    os.exit(1)
end