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

function CopyFile(src, dst)
	local code = os.execute(string.format("cp -r %s %s", src, dst))
	if code == 0 then
		PrintMessage(false, "Copied " .. src)
	else
		PrintMessage(true, "Failed to copy " .. src)
	end
	return code == 0
end

function CommandExists(cmd)
	return os.execute("command -v " .. cmd .. " >/dev/null 2>&1") == 0
end

function CaptureCommand(cmd)
	local f = io.popen(cmd .. " 2>&1")
	local out = f:read("*a")
	f:close()
	return out
end

return FileExists, DirExists, CopyFile, CommandExists, CaptureCommand
