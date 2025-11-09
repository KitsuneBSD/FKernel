#!/usr/bin/env lua

-- Function Utility: Check if a file exists
-- Arguments:
-- path: file path
function FileExists(path)
	local f = io.open(path, "r")
	if f then
		f:close()
		return true
	end
	return false
end

-- Function Utility: Check if a directory exists
-- Arguments:
-- path : directory path
function DirExists(path)
	local ok = os.execute("[ -d " .. path .. " ]")
	return ok == true or ok == 0
end

return FileExists, DirExists
