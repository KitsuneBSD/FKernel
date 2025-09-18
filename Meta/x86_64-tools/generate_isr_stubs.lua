#!/usr/bin/env lua

local filename = "isr_stubs.h"
local max_limit_isr = 31
local max_limit_irq = 15
local file = io.open(filename, "w")

file:write("#pragma once\n\n")
file:write('extern "C" {\n\n')

for j = 0, max_limit_isr do
	file:write(string.format("    void isr%d();\n", j))
end

file:write("\n")

for k = 0, max_limit_irq do
	file:write(string.format("    void irq%d();\n", k))
end

file:write("\n}\n\n")

file:write("static void (*g_isr_stubs[32])() = {\n")
for j = 0, max_limit_isr do
	local sep = (j < max_limit_isr) and "," or ""
	file:write(string.format("    isr%d%s\n", j, sep))
end
file:write("};\n\n")

file:write("static void (*g_irq_stubs[16])() = {\n")
for k = 0, max_limit_irq do
	local sep = (k < max_limit_irq) and "," or ""
	file:write(string.format("    irq%d%s\n", k, sep))
end
file:write("};\n")

file:close()
print("Gerado: " .. filename)
