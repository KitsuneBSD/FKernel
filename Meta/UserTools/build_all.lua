
local PrintMessage = require("Meta.Lib.print_message")

print(">>> Starting FKernel Userland Build <<<")

print("\n[1/3] Building Musl LibC...")
if not os.execute("lua Meta/UserTools/musl/build.lua") then
  PrintMessage(true, "Failed to build Musl.")
  os.exit(1)
end

print("\n[1.5/4] Building LibFKUser...")
if not os.execute("lua Meta/UserTools/libfk_user/build.lua") then
  PrintMessage(true, "Failed to build LibFKUser.")
  os.exit(1)
end

print("\n[2/4] Building Official LibBSD (and LibMD)...")
if not os.execute("lua Meta/UserTools/libbsd/build.lua") then
  PrintMessage(true, "Failed to build LibBSD.")
  os.exit(1)
end

print("\n[3/4] Building BusyBox...")
if not os.execute("lua Meta/UserTools/busybox/build.lua") then
  PrintMessage(true, "Failed to build BusyBox.")
  os.exit(1)
end

print("\n[4/4] OpenRC (Optional/Manual)...")
os.execute("lua Meta/UserTools/openrc/build.lua")

print("\n>>> Userland ready! <<<")
print("Binaries are in build/initrd_root/")
