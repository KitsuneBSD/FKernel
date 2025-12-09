#!/usr/bin/env lua

local OSInteract = {}

-- Function Utility: Check if a file exists
-- Arguments:
-- path: file path
function OSInteract.FileExists(path)
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
function OSInteract.DirExists(path)
	local ok = os.execute("[ -d " .. path .. " ]")
	return ok == true or ok == 0
end

function OSInteract.CopyFile(src, dst)
	local code = os.execute(string.format("cp -r %s %s", src, dst))
	if code == 0 then
		PrintMessage(false, "Copied " .. src)
	else
		PrintMessage(true, "Failed to copy " .. src)
	end
	return code == 0
end

function OSInteract.CommandExists(cmd)
	return os.execute("command -v " .. cmd .. " >/dev/null 2>&1") == 0
end

function OSInteract.CaptureCommand(cmd)
	local f = io.popen(cmd .. " 2>&1")
	local out = f:read("*a")
	f:close()
	return out
end

_G.OSInteract = OSInteract
return OSInteract
