#!/usr/bin/env lua

-- Function Utility: Run command safety

function RunCommand(cmd)
	local ok, why, code = os.execute(cmd)

	-- Lua 5.1: retorna apenas um número
	if type(ok) == "number" then
		return ok == 0
	end

	-- Lua 5.2+: retorna true/false + "exit" + código
	if ok == true and why == "exit" and code == 0 then
		return true
	end

	return false
end

return RunCommand
