-- Meta/x86_64-tools/check_one_syscall_per_file.lua
-- Verifies AGENTS.md Secret Rule:
--   "One syscall handler per file. Each Src/Kernel/Syscall/SyscallList/
--    file defines at most one sys_* handler. File name matches the handler
--    name minus the `sys_` prefix."
--
-- Files with zero handlers are allowed (shared support files, e.g.
-- Time/posix_timer.cpp). Files with MORE THAN ONE handler definition are
-- violations. Files with EXACTLY ONE handler are also checked for the
-- name-match rule: the file basename (without .cpp) must equal the handler
-- name minus the `sys_` prefix (e.g. sys_prlimit64 -> prlimit64.cpp).
--
-- Detection:
--   A handler definition is a function named `sys_<name>` declared as
--   `uint64_t sys_<name>(...)` (with optional `extern "C"` / `extern "C" {}`
--   wrapping) whose parameter list is followed by `{` rather than `;`.
--   Multi-line signatures are handled by scanning to the matching `)`.
--   Prototypes ending in `;` are not counted.
--
-- Usage: lua Meta/x86_64-tools/check_one_syscall_per_file.lua
--        xmake check-syscalls
--
-- Exits with code:
--   0 = no violations
--   1 = violations found

local script_name = "check-one-syscall-per-file"

-- ─── C++ comment/string stripper ────────────────────────────────────────────

local function strip_comments_and_strings(text)
  local out = {}
  local i = 1
  local n = #text

  local in_line   = false
  local in_block  = false
  local in_str    = false
  local str_char  = nil
  local escaped   = false

  while i <= n do
    local ch = text:sub(i, i)
    local n2 = text:sub(i, i + 1)

    if in_line then
      if ch == '\n' then
        in_line = false
        out[#out + 1] = ch
      else
        out[#out + 1] = ' '
      end
    elseif in_block then
      if n2 == '*/' then
        in_block = false
        out[#out + 1] = ' '
        out[#out + 1] = ' '
        i = i + 1
      else
        out[#out + 1] = (ch == '\n') and ch or ' '
      end
    elseif in_str then
      if escaped then
        escaped = false
      elseif ch == '\\' then
        escaped = true
      elseif ch == str_char then
        in_str = false
      end
      out[#out + 1] = (ch == '\n') and ch or ' '
    else
      if n2 == '//' then
        in_line = true
        out[#out + 1] = ' '
        out[#out + 1] = ' '
        i = i + 1
      elseif n2 == '/*' then
        in_block = true
        out[#out + 1] = ' '
        out[#out + 1] = ' '
        i = i + 1
      elseif ch == '"' or ch == "'" then
        in_str = true
        str_char = ch
        out[#out + 1] = ' '
      else
        out[#out + 1] = ch
      end
    end
    i = i + 1
  end

  return table.concat(out)
end

-- ─── File finding ──────────────────────────────────────────────────────────

local function find_files(root, pattern)
  local files = {}
  local cmd = string.format('find "%s" -name "%s" -type f 2>/dev/null | sort', root, pattern)
  local handle = io.popen(cmd)
  if handle then
    for file in handle:lines() do
      table.insert(files, file)
    end
    handle:close()
  end
  return files
end

-- Resolve project root
local function find_project_root()
  local candidates = {
    ".",
    "..",
    "../..",
    "/home/kitsune/Projects/FKernel",
  }
  for _, dir in ipairs(candidates) do
    local abs = dir
    if abs:sub(1, 1) ~= '/' then
      local script_dir = (debug.getinfo(1, "S").source or ""):match("^@?(.*/)") or ""
      abs = script_dir .. dir
    end
    local f = io.open(abs .. "/xmake.lua", "r")
    if f then f:close(); return abs end
  end
  return "."
end

local ROOT = find_project_root():gsub("/+$", "")
local SYSCALL_DIR = ROOT .. "/Src/Kernel/Syscall/SyscallList"

local function relpath(abs)
  if abs:sub(1, #ROOT) == ROOT then
    local r = abs:sub(#ROOT + 2)
    return r
  end
  return abs
end

-- ─── Scanner ────────────────────────────────────────────────────────────────

-- Returns: handlers, err
-- Each handler entry: { name, line }
local function scan_file(filepath)
  local f = io.open(filepath, "r")
  if not f then return nil, "Cannot open file" end
  local content = strip_comments_and_strings(f:read("*a"))
  f:close()

  local handlers = {}
  local n = #content
  local line = 1
  local i = 1
  local pat = "uint64_t%s+sys_[%w_]+%s*%("

  while true do
    local s, e = content:find(pat, i)
    if not s then break end

    -- Advance line counter to the match start
    while i < s do
      if content:sub(i, i) == '\n' then line = line + 1 end
      i = i + 1
    end

    -- Extract the handler name (position right after "uint64_t")
    local name = content:match("sys_[%w_]+", s + 8)
    if not name then break end

    -- Scan to the matching close paren of the parameter list
    local depth = 1
    local p = e + 1
    while p <= n and depth > 0 do
      local c = content:sub(p, p)
      if c == '\n' then line = line + 1 end
      if c == '(' then depth = depth + 1
      elseif c == ')' then depth = depth - 1
      end
      p = p + 1
    end

    -- Skip whitespace after the close paren
    while p <= n do
      local c = content:sub(p, p)
      if c == '\n' then
        line = line + 1
        p = p + 1
      elseif c:match("%s") then
        p = p + 1
      else
        break
      end
    end

    -- `{` → definition; `;` → prototype (not counted)
    if content:sub(p, p) == '{' then
      table.insert(handlers, { name = name, line = line })
    end

    i = p
  end

  return handlers, nil
end

-- ─── Main ───────────────────────────────────────────────────────────────────

local function main()
  local files = find_files(SYSCALL_DIR, "*.cpp")
  local violations = {}

  for _, file in ipairs(files) do
    local handlers, err = scan_file(file)

    if err then
      table.insert(violations, {
        file = relpath(file),
        msg = "Parse error: " .. err,
        handlers = {},
      })
    elseif #handlers > 1 then
      table.insert(violations, {
        file = relpath(file),
        msg = #handlers .. " syscall handler definitions (max 1)",
        handlers = handlers,
      })
    elseif #handlers == 1 then
      local basename = file:match("([^/]+)%.cpp$")
      local expected = handlers[1].name:gsub("^sys_", "")
      if basename and basename ~= expected then
        table.insert(violations, {
          file = relpath(file),
          msg = string.format(
            "file name '%s' does not match handler '%s' (expected '%s.cpp')",
            basename, handlers[1].name, expected),
          handlers = handlers,
        })
      end
    end
  end

  print(string.format("\n🔍 [%s] Checking one syscall handler per file rule...\n", script_name))
  print(string.format("   Scanned: %d files in Src/Kernel/Syscall/SyscallList/", #files))
  print("   Rules:  at most 1 handler/file AND file name = handler minus `sys_`")
  print("")

  if #violations == 0 then
    print(string.format("✅ [PASS] check-one-syscall-per-file: %d files checked, 0 violations", #files))
    print("   OK")
    os.exit(0)
  end

  print(string.format("❌ VIOLATIONS (%d files):", #violations))
  print("   " .. string.rep("─", 65))
  for _, v in ipairs(violations) do
    print(string.format("   📄 %s", v.file))
    print(string.format("      └─ %s", v.msg))
    for _, h in ipairs(v.handlers) do
      print(string.format("         • %s (body starts line %d)", h.name, h.line))
    end
    print("")
  end
  print("   Fix: Split handlers into separate files (file name = handler name minus `sys_`); rename files whose basename does not match the single handler.")
  print(string.format("   check-one-syscall-per-file: %d files checked, %d violations", #files, #violations))
  os.exit(1)
end

main()
