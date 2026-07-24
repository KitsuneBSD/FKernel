-- Meta/x86_64-tools/check_layer_separation.lua
-- Verifies LibC → LibFK → Kernel layer separation
-- Kernel code MUST NOT include LibC headers directly

local function scan_file(filepath)
  local violations = {}
  local f = io.open(filepath, "r")
  if not f then return violations end

  local line_num = 0
  for line in f:lines() do
    line_num = line_num + 1
    -- Match #include <LibC/...> or #include "LibC/..."
    if line:match('#%s*include%s*[<"]LibC/') then
      table.insert(violations, { file = filepath, line = line_num, content = line })
    end
  end
  f:close()
  return violations
end

local function find_files(root, pattern)
  local files = {}
  local cmd = string.format('find "%s" -name "%s" -type f 2>/dev/null', root, pattern)
  local handle = io.popen(cmd)
  if handle then
    for file in handle:lines() do
      table.insert(files, file)
    end
    handle:close()
  end
  return files
end

-- Documented exceptions (files allowed to include LibC)
local exceptions = {
  ["Src/Kernel/Arch/x86_64/Panic/panic.cpp"] = true,
  ["Src/Kernel/Io/kernel_puts.cpp"] = true,
}

local all_violations = {}

-- Scan Kernel source files
local kernel_cpp = find_files("Src/Kernel", "*.cpp")
for _, file in ipairs(kernel_cpp) do
  if not exceptions[file] then
    local viols = scan_file(file)
    for _, v in ipairs(viols) do
      table.insert(all_violations, v)
    end
  end
end

-- Scan Kernel header files
local kernel_h = find_files("Include/Kernel", "*.h")
for _, file in ipairs(kernel_h) do
  local viols = scan_file(file)
  for _, v in ipairs(viols) do
    table.insert(all_violations, v)
  end
end

-- Scan LibFK source files for Kernel includes (reverse violation)
local libfk_cpp = find_files("Src/LibFK", "*.cpp")
local libfk_h = find_files("Include/LibFK", "*.h")
for _, file in ipairs(libfk_cpp) do
  local f = io.open(file, "r")
  if f then
    local line_num = 0
    for line in f:lines() do
      line_num = line_num + 1
      if line:match('#%s*include%s*[<"]Kernel/') then
        table.insert(all_violations, {
          file = file,
          line = line_num,
          content = line,
          reverse = true
        })
      end
    end
    f:close()
  end
end

for _, file in ipairs(libfk_h) do
  local f = io.open(file, "r")
  if f then
    local line_num = 0
    for line in f:lines() do
      line_num = line_num + 1
      if line:match('#%s*include%s*[<"]Kernel/') then
        table.insert(all_violations, {
          file = file,
          line = line_num,
          content = line,
          reverse = true
        })
      end
    end
    f:close()
  end
end

-- Report
if #all_violations == 0 then
  print("✅ Layer separation check passed. No violations found.")
  os.exit(0)
else
  print(string.format("\n❌ Layer separation violations found: %d\n", #all_violations))
  for _, v in ipairs(all_violations) do
    local tag = v.reverse and "REVERSE (LibFK→Kernel)" or "Kernel→LibC"
    print(string.format("  [%s] %s:%d: %s", tag, v.file, v.line, v.content:match("^%s*(.-)%s*$")))
  end
  print(string.format("\nFix: Replace LibC includes with LibFK equivalents."))
  print(string.format("Exceptions: panic.cpp, kernel_puts.cpp (early boot/panic context)."))
  os.exit(1)
end
