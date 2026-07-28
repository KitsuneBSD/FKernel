
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

print("---" .. " FKernel BusyBox Toolchain ---")

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
      inittab:write("tty0::respawn:-/bin/sh\n")
      inittab:close()
  end

  -- Create /etc/init.d/rcS
  local rcs = io.open(INITRD_DIR .. "/etc/init.d/rcS", "w")
  if rcs then
    rcs:write("#!/bin/sh\n")
    rcs:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
    rcs:write("\n")
    rcs:write("echo \"\"\n")
    rcs:write("echo \"FKernel booting...\"\n")
    rcs:write("echo \"\"\n")
    rcs:write("\n")
    rcs:write("if [ -x /bin/ktest ]; then\n")
    rcs:write("    /bin/ktest\n")
    rcs:write("    if [ $? -eq 0 ]; then\n")
    rcs:write("        echo \"[BOOT] All kernel tests passed.\"\n")
    rcs:write("    else\n")
    rcs:write("        echo \"[BOOT] Some kernel tests FAILED.\"\n")
    rcs:write("    fi\n")
    rcs:write("else\n")
    rcs:write("    echo \"[BOOT] ktest not found, skipping tests.\"\n")
    rcs:write("fi\n")
    rcs:write("\n")
    rcs:write("echo \"\"\n")
    rcs:write("echo \"FKernel ready.\"\n")
    rcs:close()
    os.execute("chmod +x " .. INITRD_DIR .. "/etc/init.d/rcS")
  end

  -- Create /etc/profile
  local profile = io.open(INITRD_DIR .. "/etc/profile", "w")
  if profile then
    profile:write("export PATH=/bin:/sbin:/usr/bin:/usr/sbin\n")
    profile:write("export PS1='# ' \n")
    profile:write("export TERM=vt100\n")
    profile:close()
  end

  -- Create /etc/passwd
  local passwd = io.open(INITRD_DIR .. "/etc/passwd", "w")
  if passwd then
    passwd:write("root:x:0:0:root:/root:/bin/sh\n")
    passwd:write("daemon:x:1:1:daemon:/usr/sbin:/bin/false\n")
    passwd:write("nobody:x:65534:65534:nobody:/nonexistent:/bin/false\n")
    passwd:close()
  end

  -- Create /etc/group
  local group = io.open(INITRD_DIR .. "/etc/group", "w")
  if group then
    group:write("root:x:0:\n")
    group:write("daemon:x:1:\n")
    group:write("nogroup:x:65534:\n")
    group:close()
  end

  -- Create /etc/shadow (locked by default except root with no password)
  local shadow = io.open(INITRD_DIR .. "/etc/shadow", "w")
  if shadow then
    shadow:write("root::0:0:99999:7:::\n")
    shadow:write("daemon:!:0:0:99999:7:::\n")
    shadow:write("nobody:!:0:0:99999:7:::\n")
    shadow:close()
  end
  os.execute("chmod 640 " .. INITRD_DIR .. "/etc/shadow")

  -- Create /etc/hostname
  local hostname = io.open(INITRD_DIR .. "/etc/hostname", "w")
  if hostname then
    hostname:write("fkernel\n")
    hostname:close()
  end

  -- Create /etc/hosts
  local hosts = io.open(INITRD_DIR .. "/etc/hosts", "w")
  if hosts then
    hosts:write("127.0.0.1  localhost\n")
    hosts:write("127.0.1.1  fkernel\n")
    hosts:close()
  end

  -- Create /etc/nsswitch.conf (BusyBox uses this for name resolution order)
  local nsswitch = io.open(INITRD_DIR .. "/etc/nsswitch.conf", "w")
  if nsswitch then
    nsswitch:write("passwd:   files\n")
    nsswitch:write("group:    files\n")
    nsswitch:write("hosts:    files\n")
    nsswitch:close()
  end

  -- OpenRC-compatible structure (Phase 15a prep)
  os.execute("mkdir -p " .. INITRD_DIR .. "/etc/conf.d")
  os.execute("mkdir -p " .. INITRD_DIR .. "/etc/runlevels/boot")
  os.execute("mkdir -p " .. INITRD_DIR .. "/etc/runlevels/default")
  os.execute("mkdir -p " .. INITRD_DIR .. "/var/cache")
  os.execute("mkdir -p " .. INITRD_DIR .. "/var/lock")
  os.execute("mkdir -p " .. INITRD_DIR .. "/var/empty")
end

local function create_all_links()
  print("Creating safe symlinks for initrd...")
  create_etc_configs()
  
  local src_dir = BUILD_DIR .. "/" .. BB_NAME
  -- Use the freshly built binary to list applets
  os.execute(src_dir .. "/busybox --list > /tmp/bb_applets.txt")
  
  local f = io.open("/tmp/bb_applets.txt", "r")
  if f then
    for line in f:lines() do
      if line ~= "busybox" then
        mlink("../usr/bin/busybox", "bin/" .. line)
      end
    end
    f:close()
  end
  
  -- Extra safety links
  mlink("../usr/bin/busybox", "bin/sh")
  mlink("../usr/bin/busybox", "sbin/init")
end

local src_dir = BUILD_DIR .. "/" .. BB_NAME
local lock_file = BUILD_DIR .. "/.build.lock"

-- Prevent simultaneous builds
if OSInteract.FileExists(lock_file) then
  PrintMessage(true, "BusyBox build is already in progress (lock found). Skipping.")
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
  local sed_cmd1 = string.format("sed -i 's/^# %s is not set/%s=y/' %s", key, key, config_file)
  os.execute(sed_cmd1)
  local sed_cmd2 = string.format("sed -i 's/^%s=n/%s=y/' %s", key, key, config_file)
  os.execute(sed_cmd2)
end

-- Start from allnoconfig
print("Generating minimal configuration...")
os.execute("make -C " .. src_dir .. " allnoconfig")

local config_file = src_dir .. "/.config"
print("Enabling essential applets and installer...")

local enable_list = {
  "CONFIG_BUSYBOX",
  "CONFIG_FEATURE_INSTALLER",
  "CONFIG_ASH",
  "CONFIG_SH_IS_ASH",
  "CONFIG_INIT",
  "CONFIG_FEATURE_USE_INITTAB",
  "CONFIG_CAT",
  "CONFIG_RM",
  "CONFIG_LS",
  "CONFIG_CP",
  "CONFIG_MV",
  "CONFIG_MKDIR",
  "CONFIG_CHMOD",
  "CONFIG_CHOWN",
  "CONFIG_LN",
  "CONFIG_SLEEP",
  "CONFIG_UNAME",
  "CONFIG_ID",
  "CONFIG_WHOAMI",
  "CONFIG_ENV",
  "CONFIG_TOUCH",
  "CONFIG_TAIL",
  "CONFIG_HEAD",
  "CONFIG_WC",
  "CONFIG_BASENAME",
  "CONFIG_DIRNAME",
  "CONFIG_TRUE",
  "CONFIG_FALSE",
  "CONFIG_ECHO",
  "CONFIG_CLEAR",
  "CONFIG_SYNC",
  "CONFIG_KILL",
  "CONFIG_PIE",
  -- Process info (backed by /proc)
  "CONFIG_PS",
  "CONFIG_FREE",
  "CONFIG_UPTIME",
  "CONFIG_TOP",
  -- Filesystem tools
  "CONFIG_DF",
  "CONFIG_DU",
  "CONFIG_FIND",
  "CONFIG_STAT",
  "CONFIG_MKTEMP",
  "CONFIG_REALPATH",
  -- Text processing
  "CONFIG_GREP",
  "CONFIG_SED",
  "CONFIG_CUT",
  "CONFIG_SORT",
  "CONFIG_UNIQ",
  "CONFIG_TR",
  "CONFIG_PRINTF",
  "CONFIG_EXPR",
  "CONFIG_TEST",
  -- System tools
  "CONFIG_DATE",
  "CONFIG_HOSTNAME",
  "CONFIG_WHICH",
  "CONFIG_MOUNT",
  "CONFIG_UMOUNT",
  "CONFIG_DMESG",
  "CONFIG_REBOOT",
  "CONFIG_HALT",
  "CONFIG_POWEROFF",
  -- Shell extras
  "CONFIG_LESS",
  "CONFIG_MORE",
  "CONFIG_XARGS",
  "CONFIG_TEE",
  "CONFIG_SPLIT",
  "CONFIG_STTY",
  "CONFIG_YES",
  -- Features
  "CONFIG_FEATURE_LS_FILETYPES",
  "CONFIG_FEATURE_LS_FOLLOWLINKS",
  "CONFIG_FEATURE_LS_RECURSIVE",
  "CONFIG_FEATURE_LS_WIDTH",
  "CONFIG_FEATURE_LS_SORTFILES",
  "CONFIG_FEATURE_LS_TIMESTAMPS",
  "CONFIG_FEATURE_LS_USERNAME",
  "CONFIG_FEATURE_LS_COLOR",
  "CONFIG_FEATURE_PS_WIDE",
  "CONFIG_FEATURE_FIND_EXEC",
  "CONFIG_FEATURE_FIND_TYPE",
  "CONFIG_FEATURE_FIND_PERM",
  "CONFIG_FEATURE_FIND_NEWER",
  "CONFIG_FEATURE_FIND_SIZE",
  "CONFIG_FEATURE_FIND_MTIME",
  "CONFIG_FEATURE_FIND_NAME",
  "CONFIG_FEATURE_GREP_EGREP_ALIAS",
  "CONFIG_FEATURE_GREP_FGREP_ALIAS",
  "CONFIG_FEATURE_SED_MULTILINE",
  "CONFIG_FEATURE_MOUNT_HELPERS",
  "CONFIG_SHOW_USAGE"
}

for _, cfg in ipairs(enable_list) do
  ensure_config_set(config_file, cfg)
end

os.execute("make -C " .. src_dir .. " oldconfig")

print("Compiling BusyBox...")
local CC = Toolchain.get_clang()
-- Copy Linux kernel ABI headers into the sysroot so BusyBox can include <linux/vt.h> etc.
-- These are kernel-ABI-only headers (no glibc), safe with musl.
os.execute("rsync -a /usr/include/linux " .. SYSROOT .. "/include/ 2>/dev/null || true")
os.execute("rsync -a /usr/include/asm " .. SYSROOT .. "/include/ 2>/dev/null || true")
os.execute("rsync -a /usr/include/asm-generic " .. SYSROOT .. "/include/ 2>/dev/null || true")

local cflags = string.format("--sysroot=%s -isystem %s/include --target=x86_64-linux-musl", SYSROOT, SYSROOT)
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
