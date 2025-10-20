require("Meta.Lib.os_interact")
require("Meta.Lib.run_command")
require("Meta.Lib.print_message")

local kernel_bin = "build/FKernel.bin"

local function check_command(cmd)
    local ok, _, code = os.execute(cmd)
    return ok == true or code == 0
end

if not FileExists(kernel_bin) then
    PrintMessage(true, string.format("Can't find the binary %s", kernel_bin))
    os.exit(1)
end

if not check_command("grub-file --is-x86-multiboot " .. kernel_bin) then
    PrintMessage(true, string.format("%s is NOT a valid multiboot kernel", kernel_bin))
end

if not check_command("grub-file --is-x86-multiboot2 " .. kernel_bin) then
    PrintMessage(true, string.format("%s is NOT a valid multiboot2 kernel", kernel_bin))
end
