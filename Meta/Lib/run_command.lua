#!/usr/bin/env lua

-- Function Utility: Run command safety

function RunCommand(command)
	local _, _, code = os.execute(command)
	return code == 0
end

return RunCommand
