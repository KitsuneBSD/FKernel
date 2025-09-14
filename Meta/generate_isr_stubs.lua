#!/usr/bin/env lua

local filename = "isr_stubs.h"
local file = io.open(filename, "w")

file:write("#pragma once\n\n")
file:write('extern "C" {\n')

for i = 0, 255 do
	file:write(string.format("    void isr%d();\n", i))
end

file:write("}\n")

file:close()
print("Gerado: " .. filename)
