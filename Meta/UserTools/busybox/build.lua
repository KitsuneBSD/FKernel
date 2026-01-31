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
  local inittab = io.open(INITRD_DIR .. "/etc/inittab", "w")
  if inittab then
      inittab:write("::sysinit:/etc/init.d/rcS\n")
      inittab:write("::respawn:-/bin/sh\n")
      inittab:write("::ctrlaltdel:/sbin/reboot\n")
      inittab:write("::shutdown:/bin/umount -a -r\n")
      inittab:close()
  end

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
    profile:write("export TERM=vt100\n")
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
  mlink("../usr/bin/busybox", "bin/cp")
  mlink("../usr/bin/busybox", "bin/mv")
  mlink("../usr/bin/busybox", "bin/mkdir")
  mlink("../usr/bin/busybox", "bin/grep")
  mlink("../usr/bin/busybox", "bin/sed")
  mlink("../usr/bin/busybox", "bin/awk")
  mlink("../usr/bin/busybox", "bin/find")
  mlink("../usr/bin/busybox", "bin/chmod")
  mlink("../usr/bin/busybox", "bin/chown")
  mlink("../usr/bin/busybox", "bin/ln")
  mlink("../usr/bin/busybox", "bin/sleep")
  mlink("../usr/bin/busybox", "bin/uname")
  mlink("../usr/bin/busybox", "bin/id")
  mlink("../usr/bin/busybox", "bin/whoami")
  mlink("../usr/bin/busybox", "bin/env")
  mlink("../usr/bin/busybox", "bin/touch")
  mlink("../usr/bin/busybox", "bin/tail")
  mlink("../usr/bin/busybox", "bin/head")
  mlink("../usr/bin/busybox", "bin/wc")
  mlink("../usr/bin/busybox", "bin/sort")
  mlink("../usr/bin/busybox", "bin/uniq")
  mlink("../usr/bin/busybox", "bin/basename")
  mlink("../usr/bin/busybox", "bin/dirname")
  mlink("../usr/bin/busybox", "bin/du")
  mlink("../usr/bin/busybox", "bin/df")
  mlink("../usr/bin/busybox", "bin/true")
  mlink("../usr/bin/busybox", "bin/false")
  
  mlink("../usr/bin/busybox", "bin/echo")
  mlink("../usr/bin/busybox", "bin/top")
  mlink("../usr/bin/busybox", "bin/ping")
end

local src_dir = BUILD_DIR .. "/" .. BB_NAME
local lock_file = BUILD_DIR .. "/.build.lock"

-- Prevent simultaneous builds
if OSInteract.FileExists(lock_file) then
  PrintMessage(true, "BusyBox build is already in progress (lock found). Skipping.")
  os.exit(0)
end

-- If binary exists and is recent enough, skip build but ensure links
if OSInteract.FileExists(src_dir .. "/busybox") then
  PrintMessage(false, "BusyBox already built. Ensuring symlinks...")
  os.execute("mkdir -p " .. INITRD_DIR .. "/usr/bin")
  os.execute("cp " .. src_dir .. "/busybox " .. INITRD_DIR .. "/usr/bin/")
  create_all_links()
  os.exit(0)
end

-- Create lock
os.execute("mkdir -p " .. BUILD_DIR)
os.execute("touch " .. lock_file)

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
  "CONFIG_VI", "CONFIG_CP", "CONFIG_MV", "CONFIG_MKDIR",
  "CONFIG_GREP", "CONFIG_SED", "CONFIG_AWK", "CONFIG_FIND",
  "CONFIG_CHMOD", "CONFIG_CHOWN", "CONFIG_LN", "CONFIG_SLEEP",
  "CONFIG_UNAME", "CONFIG_ID", "CONFIG_WHOAMI", "CONFIG_ENV",
  "CONFIG_TOUCH", "CONFIG_TAIL", "CONFIG_HEAD", "CONFIG_WC",
  "CONFIG_SORT", "CONFIG_UNIQ", "CONFIG_BASENAME", "CONFIG_DIRNAME",
  "CONFIG_DU", "CONFIG_DF", "CONFIG_TRUE", "CONFIG_FALSE",
  
  -- Top and Ping support
  "CONFIG_TOP", "CONFIG_FEATURE_TOP_CPU_USAGE_PERCENTAGE", 
  "CONFIG_FEATURE_TOP_CPU_GLOBAL_PERCENTS", "CONFIG_FEATURE_TOPMEM",
  "CONFIG_PING", "CONFIG_FEATURE_FANCY_PING",
  
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
  os.execute("rm -f " .. lock_file)
  PrintMessage(false, "BusyBox installed safely.")
else
  os.execute("rm -f " .. lock_file)
  os.exit(1)
end
