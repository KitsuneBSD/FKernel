require("Meta.Lib.os_interact")
require("Meta.Lib.run_command")
require("Meta.Lib.print_message")

local kernel_bin = "build/FKernel.bin"

local function check_command(cmd)
	local ok, _, _ = os.execute(cmd)
	-- Em Lua 5.1, `os.execute` retorna:
	--  * code (numérico) em POSIX
	--  * boolean/nil em Windows
	-- Então normalizamos:
	if type(ok) == "number" then
		return ok == 0
	elseif type(ok) == "boolean" then
		return ok
	else
		return false
	end
end

if not FileExists(kernel_bin) then
	PrintMessage(true, string.format("Can't find the binary %s", kernel_bin))
	os.exit(1)
end
local is_mb = check_command("grub-file --is-x86-multiboot " .. kernel_bin)
local is_mb2 = check_command("grub-file --is-x86-multiboot2 " .. kernel_bin)

if is_mb or is_mb2 then
	-- Print an informational message; prefer multiboot2 when available.
	if is_mb2 then
		PrintMessage(false, string.format("%s is a valid multiboot2 kernel", kernel_bin))
	else
		PrintMessage(false, string.format("%s is a valid multiboot (legacy) kernel", kernel_bin))
	end
else
	-- Neither multiboot nor multiboot2 detected: emit actionable diagnostics
	PrintMessage(true, string.format("%s is NOT a valid multiboot or multiboot2 kernel", kernel_bin))
	PrintMessage(
		true,
		"Hint: Ensure `.multiboot_header` is present within the first 32KiB of the binary and is part of a loadable PT_LOAD segment."
	)
	PrintMessage(
		true,
		"Suggestion: run `readelf -l` and inspect the program headers and `readelf -S` for the .multiboot_header placement."
	)
	-- TODO: Consider automatically dumping the first 32KiB or running `readelf`
	--       to help diagnose header placement issues.
	os.exit(1)
end
