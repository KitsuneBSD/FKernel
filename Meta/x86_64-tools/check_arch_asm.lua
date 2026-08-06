-- Meta/x86_64-tools/check_arch_asm.lua
-- Verifies AGENTS.md Architecture Portability policy:
--   "Generic code MUST call arch_*() functions, NOT inline asm()."
--   "New arch-specific primitives MUST follow arch_ + extern 'C' pattern."
--   "Implementations live in Src/Kernel/Arch/<arch>/<subsystem>/."
--
-- Rule enforced here:
--   Any file that deals with assembly — an .asm/.S/.s source file, or a
--   .cpp/.h file containing inline asm (asm / __asm__ / _asm tokens) —
--   MUST live under an `Arch/` path component (e.g. Src/Kernel/Arch/x86_64/,
--   Include/LibFK/Arch/x86_64/). Assembly loose in generic code is a
--   portability violation.
--
-- Scope: Src/Kernel, Include/Kernel, Src/LibFK, Include/LibFK.
--   Src/Userland is excluded (crt0.asm / syscalls.asm are inherent
--   userspace assembly).
--
-- Usage: lua Meta/x86_64-tools/check_arch_asm.lua
--        xmake check-arch-asm
--
-- Exits with code:
--   0 = no violations
--   1 = violations found

local script_name = "check-arch-asm"

local SCAN_ROOTS = {
  "Src/Kernel",
  "Include/Kernel",
  "Src/LibFK",
  "Include/LibFK",
}

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

-- ─── Helpers ────────────────────────────────────────────────────────────────

-- A file "deals with assembly" if it lives under an `Arch/` path component.
-- Lua patterns have no alternation, so test both anchors separately.
local function is_under_arch(path)
  return path:find("/Arch/") ~= nil or path:find("^Arch/") ~= nil
end

-- Inline asm detection: after stripping comments/strings, flag identifier
-- tokens `asm`, `__asm__`, `_asm`. `asm` is a C++ keyword, so a standalone
-- token is always inline assembly (with or without volatile/goto).
local ASM_TOKENS = { ["asm"] = true, ["__asm__"] = true, ["_asm"] = true }

local function is_asm_token(token)
  return ASM_TOKENS[token] == true
end

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

-- Resolve project root (same strategy as the other checkers)
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

local function relpath(abs)
  if abs:sub(1, #ROOT) == ROOT then
    local r = abs:sub(#ROOT + 2)
    return r
  end
  return abs
end

-- ─── Scanners ───────────────────────────────────────────────────────────────

-- Inline asm in a C/C++ file: returns list of { line, content }
local function scan_inline_asm(filepath)
  local f = io.open(filepath, "r")
  if not f then return nil, "Cannot open file" end
  local stripped = strip_comments_and_strings(f:read("*a"))
  f:close()

  local hits = {}
  local line = 1
  for text_line in stripped:gmatch("([^\n]*)\n?") do
    for token in text_line:gmatch("[%a_][%w_]*") do
      if is_asm_token(token) then
        table.insert(hits, { line = line, content = text_line:gsub("^%s+", "") })
        break
      end
    end
    line = line + 1
  end
  return hits, nil
end

-- ─── Main ───────────────────────────────────────────────────────────────────

local function main()
  local violations = {}
  local scanned = { cpp = 0, asm = 0 }

  for _, root in ipairs(SCAN_ROOTS) do
    local full_root = ROOT .. "/" .. root

    for _, file in ipairs(find_files(full_root, "*.cpp")) do
      scanned.cpp = scanned.cpp + 1
      if not is_under_arch(relpath(file)) then
        local hits, err = scan_inline_asm(file)
        if err then
          table.insert(violations, {
            file = relpath(file),
            msg = "Parse error: " .. err,
            hits = {},
          })
        elseif #hits > 0 then
          table.insert(violations, {
            file = relpath(file),
            msg = "inline asm in generic (non-Arch) code",
            hits = hits,
          })
        end
      end
    end

    for _, file in ipairs(find_files(full_root, "*.h")) do
      scanned.cpp = scanned.cpp + 1
      if not is_under_arch(relpath(file)) then
        local hits, err = scan_inline_asm(file)
        if err then
          table.insert(violations, {
            file = relpath(file),
            msg = "Parse error: " .. err,
            hits = {},
          })
        elseif #hits > 0 then
          table.insert(violations, {
            file = relpath(file),
            msg = "inline asm in generic (non-Arch) code",
            hits = hits,
          })
        end
      end
    end

    for _, ext in ipairs({ "*.asm", "*.S", "*.s" }) do
      for _, file in ipairs(find_files(full_root, ext)) do
        scanned.asm = scanned.asm + 1
        if not is_under_arch(relpath(file)) then
          table.insert(violations, {
            file = relpath(file),
            msg = "assembly source outside Arch/ directory",
            hits = {},
          })
        end
      end
    end
  end

  print(string.format("\n🔍 [%s] Checking asm/inline-asm stays under Arch/ dirs...\n", script_name))
  print(string.format("   Scanned: %d C/C++ files, %d assembly files (Kernel + LibFK)", scanned.cpp, scanned.asm))
  print("   Rule:  asm belongs in an Arch/ directory, never in generic code")
  print("")

  if #violations == 0 then
    print(string.format("✅ [PASS] check-arch-asm: all asm/inline-asm is under Arch/ directories"))
    print("   OK")
    os.exit(0)
  end

  print(string.format("❌ VIOLATIONS (%d files):", #violations))
  print("   " .. string.rep("─", 65))
  for _, v in ipairs(violations) do
    print(string.format("   📄 %s", v.file))
    print(string.format("      └─ %s", v.msg))
    for _, h in ipairs(v.hits) do
      print(string.format("         • line %d: %s", h.line, h.content))
    end
    print("")
  end
  print("   Fix: move assembly into an Arch/ directory (Src/Kernel/Arch/<arch>/... or")
  print("        Include/LibFK/Arch/<arch>/...) and route generic code through arch_*() calls.")
  print(string.format("   check-arch-asm: %d files scanned, %d violations", scanned.cpp + scanned.asm, #violations))
  os.exit(1)
end

main()
