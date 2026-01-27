#!/usr/bin/env lua

-- Script de construção do BusyBox para FKernel
local ArchiveUtils = require("Meta.Lib.archive_utils")
local BuildUtils = require("Meta.Lib.build_utils")
local Toolchain = require("Meta.Lib.toolchain")
local OSInteract = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")

local BB_VERSION = "1.36.1"
local BB_NAME = "busybox-" .. BB_VERSION
local BB_URL = "https://busybox.net/downloads/" .. BB_NAME .. ".tar.bz2"
local ROOT_DIR = os.getenv("PWD")
local BUILD_DIR = ROOT_DIR .. "/build/userland/busybox"
local SYSROOT = ROOT_DIR .. "/build/sysroot"
local INITRD_DIR = ROOT_DIR .. "/build/initrd_root"

print("--- FKernel BusyBox Toolchain ---")

local function mlink(target, link)
  os.execute(string.format("ln -sf %s %s/%s", target, INITRD_DIR, link))
end

local function create_etc_configs()
  print("Creating basic /etc configurations...")
  os.execute("mkdir -p " .. INITRD_DIR .. "/etc/init.d")

  -- Create /etc/inittab
  -- local inittab = io.open(INITRD_DIR .. "/etc/inittab", "w")
  -- if inittab then
  --     inittab:write("::sysinit:/etc/init.d/rcS\n")
  --     inittab:write("::askfirst:-/bin/sh\n")
  --     inittab:write("::ctrlaltdel:/sbin/reboot\n")
  --     inittab:write("::shutdown:/bin/umount -a -r\n")
  --     inittab:close()
  -- end

  -- Create /etc/init.d/rcS
  local rcs = io.open(INITRD_DIR .. "/etc/init.d/rcS", "w")
  if rcs then
    rcs:write("#!/bin/sh\n")
    rcs:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
    rcs:write("echo \"FKernel standard system booted successfully.\"\n")
    rcs:close()
    os.execute("chmod +x " .. INITRD_DIR .. "/etc/init.d/rcS")
  end

  -- Create /etc/profile
  local profile = io.open(INITRD_DIR .. "/etc/profile", "w")
  if profile then
    profile:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
    profile:write("export PS1='# '\n")
    profile:close()
  end
end

local function create_all_links()
  print("Creating safe symlinks for initrd...")
  create_etc_configs()
  mlink("../usr/bin/busybox", "bin/sh")
  mlink("../usr/bin/busybox", "sbin/init")
  
  -- Requested tools
  mlink("../usr/bin/busybox", "bin/cat")
  mlink("../usr/bin/busybox", "bin/rm")
  mlink("../usr/bin/busybox", "bin/rmdir")
  mlink("../usr/bin/busybox", "bin/ls")
  mlink("../usr/bin/busybox", "bin/vi")
  
  -- Essentials
  mlink("../usr/bin/busybox", "bin/echo")
end

-- Force rebuild to apply new config
-- local src_dir = BUILD_DIR .. "/" .. BB_NAME
-- if OSInteract.FileExists(src_dir .. "/busybox") then ... end

local src_dir = BUILD_DIR .. "/" .. BB_NAME
os.execute("mkdir -p " .. BUILD_DIR)
os.execute("mkdir -p " .. INITRD_DIR .. "/usr/bin")
os.execute("mkdir -p " .. INITRD_DIR .. "/bin")
os.execute("mkdir -p " .. INITRD_DIR .. "/sbin")

local tarball = BUILD_DIR .. "/busybox.tar.bz2"
ArchiveUtils.download(BB_URL, tarball)
ArchiveUtils.extract(tarball, BUILD_DIR)

local function ensure_config_set(config_file, key)
  local grep_cmd = string.format("grep -q '^%s=y' %s", key, config_file)
  if not os.execute(grep_cmd) then
    local sed_cmd1 = string.format("sed -i 's/^# %s is not set/%s=y/' %s", key, key, config_file)
    os.execute(sed_cmd1)
    local sed_cmd2 = string.format("sed -i 's/^%s=n/%s=y/' %s", key, key, config_file)
    os.execute(sed_cmd2)
    if not os.execute(grep_cmd) then
       local f = io.open(config_file, "a")
       if f then 
         f:write(key .. "=y\n")
         f:close()
       end
    end
  end
end

local function ensure_config_unset(config_file, key)
  local sed_cmd = string.format("sed -i 's/^%s=y/# %s is not set/' %s", key, key, config_file)
  os.execute(sed_cmd)
end

if not OSInteract.FileExists(src_dir .. "/.config") then
  print("Generating minimal configuration...")
  os.execute("make -C " .. src_dir .. " allnoconfig")
end

-- Force ensure our desired config options are enabled
local config_file = src_dir .. "/.config"
print("Ensuring BusyBox configuration...")
local desired_configs = {
  -- Core & Init
  "CONFIG_STATIC", "CONFIG_ASH", "CONFIG_SH_IS_ASH", "CONFIG_INIT", 
  "CONFIG_FEATURE_USE_INITTAB", "CONFIG_FEATURE_INIT_SCTTY",
  
  -- Requested Tools
  "CONFIG_CAT", "CONFIG_RM", "CONFIG_RMDIR", "CONFIG_LS",
  "CONFIG_VI", 
  
  -- Essentials (implicit shell support)
  "CONFIG_ECHO",
  
  -- LS features
  "CONFIG_FEATURE_LS_FILETYPES", "CONFIG_FEATURE_LS_FOLLOWLINKS",
  "CONFIG_FEATURE_LS_RECURSIVE", "CONFIG_FEATURE_LS_WIDTH",
  "CONFIG_FEATURE_LS_SORTFILES", "CONFIG_FEATURE_LS_TIMESTAMPS",
  "CONFIG_FEATURE_LS_USERNAME", "CONFIG_FEATURE_LS_COLOR"
}

for _, cfg in ipairs(desired_configs) do
  ensure_config_set(config_file, cfg)
end

-- Force disable Linux-specific configs to maintain POSIX compatibility
local linux_configs = {
  "CONFIG_PLATFORM_LINUX",
  "CONFIG_LINUXRC",
  "CONFIG_FREE",
  "CONFIG_FEATURE_MOUNT_LOOP",
  "CONFIG_FEATURE_MOUNT_LABEL",
  "CONFIG_FEATURE_MOUNT_NFS",
  "CONFIG_FEATURE_MOUNT_CIFS",
  "CONFIG_FEATURE_INIT_MODIFY_CMDLINE",
  "CONFIG_FEATURE_INIT_COREDUMPS",
  "CONFIG_RTCWAKE",
  "CONFIG_UEVENT",
  "CONFIG_HWCLOCK",
  "CONFIG_UBIATTACH", "CONFIG_UBIDETACH", "CONFIG_UBIMKVOL", "CONFIG_UBIRMVOL", "CONFIG_UBIRSVOL", "CONFIG_UBIUPDATEVOL",
  "CONFIG_VCONFIG",
  "CONFIG_KLOGD",
  "CONFIG_LOGGER",
  "CONFIG_LOGREAD",
  "CONFIG_SYSLOGD",
  "CONFIG_ACPID",
  "CONFIG_BLKID",
  "CONFIG_FDISK",
  "CONFIG_FREERAMDISK",
  "CONFIG_FSCK_MINIX",
  "CONFIG_MKFS_MINIX",
  "CONFIG_MKFS_REISER",
  "CONFIG_FLOCK",
  "CONFIG_MKSWAP",
  "CONFIG_SWAPON",
  "CONFIG_SWAPOFF",
  "CONFIG_LOSETUP",
  "CONFIG_LSPCI",
  "CONFIG_LSUSB",
  "CONFIG_REV",
  "CONFIG_WATCHDOG"
}

for _, cfg in ipairs(linux_configs) do
  ensure_config_unset(config_file, cfg)
end

os.execute("make -C " .. src_dir .. " oldconfig")

print("Compiling BusyBox...")
local CC = Toolchain.get_clang()
local cflags = string.format("--sysroot=%s -isystem %s/include --target=%s", SYSROOT, SYSROOT,
  Toolchain.TRIPLE)
local ldflags = string.format("-static --sysroot=%s -L%s/lib", SYSROOT, SYSROOT)

local make_args = {
  string.format("CC='%s'", CC),
  string.format("EXTRA_CFLAGS='%s'", cflags),
  string.format("EXTRA_LDFLAGS='%s'", ldflags)
}

if BuildUtils.make(src_dir, make_args) then
  os.execute("cp " .. src_dir .. "/busybox " .. INITRD_DIR .. "/usr/bin/")
  create_all_links()
  PrintMessage(false, "BusyBox installed safely.")
else
  os.exit(1)
end
