#!/usr/bin/env lua

-- Function Utility: Print Colored Message
-- Arguments:
-- is_error_message: bool -> if false, print as sucess message, if true, print as error message
-- message, message to be printed

function PrintMessage(is_error_message, msg)
	if is_error_message then
		io.stderr:write("\27[41m\27[97mError: " .. msg .. "\27[0m\n")
	else
		io.stdout:write("\27[32mSuccess: " .. msg .. "\27[0m\n")
	end
end

return PrintMessage
