#!/usr/bin/env lua

require("Meta.Lib.print_message")

local LogFileIO = {}

function LogFileIO.ensure_directory(path)
	os.execute("mkdir -p " .. path)
end

function LogFileIO.write_file(filename, content)
	local file = io.open(filename, "w")
	if not file then
		PrintMessage(true, "ERROR: Could not write to " .. filename)
		return false
	end
	file:write(content)
	file:close()
	return true
end

function LogFileIO.read_log(filename)
	local lines = {}
	local file = io.open(filename, "r")

	if not file then
		PrintMessage(true, "ERROR: Could not open file " .. filename)
		return nil
	end

	for line in file:lines() do
		table.insert(lines, line)
	end

	file:close()
	PrintMessage(false, "✓ File loaded: " .. #lines .. " lines")
	return lines
end

return LogFileIO
