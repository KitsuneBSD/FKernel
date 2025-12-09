#!/usr/bin/env lua

local LogParser = {}

function LogParser.extract_log_type(line)
	local log_type = line:match("%[(.-)%]")
	return log_type
end

function LogParser.detect_severity(line)
	if line:match("%[31m") then
		return "ERROR"
	elseif line:match("%[33m") then
		return "WARNING"
	elseif line:match("%[32m") then
		return "SUCCESS"
	elseif line:match("%[37m") then
		return "DEBUG"
	else
		return "INFO"
	end
end

function LogParser.strip_ansi(line)
	return line:gsub("\27%[[%d;]*m", "")
end

return LogParser
