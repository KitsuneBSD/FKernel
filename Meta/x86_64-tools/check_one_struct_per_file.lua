-- Meta/x86_64-tools/check_one_struct_per_file.lua
-- Verifies AGENTS.md Secret Rule:
--   "One struct/class per file. File name matches class name."
--   "Nested classes, structs, and enums are FORBIDDEN."
--
-- How it works:
--   Uses a scope stack to distinguish namespace {} from type {}:
--   - "namespace" scope: introduced by namespace {} and extern "C" {}
--   - "type" scope: introduced by struct/class/enum definition {}
--   - "other" scope: all other {} (functions, control flow, initializers)
--   A type is "nested" only when a "type" entry exists in the scope stack.
--   When a struct/class/enum definition is confirmed by finding { before ;,
--   the scanner jumps past that { (consuming it) and pushes "type" directly.
--   This prevents initializer lists inside the type body from being mistaken
--   for additional type scopes.
--
-- Usage: lua Meta/x86_64-tools/check_one_struct_per_file.lua [--full]
--        xmake check-structs         (kernel only)
--        xmake check-structs --full  (libc + libfk + kernel)
--
-- Exits with code:
--   0 = no violations (or only known tech debt)
--   1 = violations found

local script_name = "check-one-struct-per-file"

-- ─── Exemption files ────────────────────────────────────────────────────────

local EXEMPTED = {}
for _, path in ipairs {
  -- ACPI specification structures
  "Include/Kernel/Hardware/Acpi/srat.h",
  "Include/Kernel/Hardware/Acpi/dmar.h",
  "Include/Kernel/Hardware/Acpi/mcfg.h",
  "Include/Kernel/Hardware/Acpi/topology_manager.h",
  "Include/Kernel/Hardware/Acpi/acpi.h",
  -- On-disk filesystem format structures
  "Include/Kernel/Fs/Disk/Ext2/ext2_super.h",
  "Include/Kernel/Fs/Disk/Ext3/ext3_super.h",
  "Include/Kernel/Fs/Disk/Ext4/ext4_super.h",
  "Include/Kernel/Fs/Disk/Exfat/exfat_bpb.h",
  "Include/Kernel/Fs/Disk/MinixFs/minix_super.h",
  "Include/Kernel/Fs/Disk/Ufs/ufs_super.h",
  "Include/Kernel/Fs/Disk/HfsPlus/hfsplus_vh.h",
  "Include/Kernel/Fs/Disk/HfsPlus/hfsplus_btree.h",
  "Include/Kernel/Fs/Disk/RamDisk/ram_disk.h",
  "Include/Kernel/Fs/Disk/Iso9660/iso9660_vd.h",
  -- Storage hardware spec/impl structures
  "Include/Kernel/Driver/Storage/storage_cache.h",
  "Include/Kernel/Driver/Storage/Nvme/nvme_command.h",
  "Include/Kernel/Driver/Storage/Nvme/nvme_queue_manager.h",
  "Include/Kernel/Driver/Storage/Nvme/nvme_controller.h",
  "Include/Kernel/Driver/Storage/Ata/dma_strategy.h",
  "Include/Kernel/Driver/Storage/Ahci/ahci_controller.h",
  "Include/Kernel/Driver/Storage/Ahci/interrupt_driven_ahci.h",
  -- Loader ABI/result type groupings
  "Include/Kernel/Loader/Types/elf64_dynamic.h",
  "Include/Kernel/Loader/Types/elf_results.h",
  -- PCI device + ioctl param
  "Include/Kernel/Hardware/Pci/pci_node.h",
  -- VFS types
  "Include/Kernel/Fs/Vfs/file_lock.h",
  "Include/Kernel/Fs/Disk/Fat/fat_common.h",
  -- Signal definitions
  "Include/Kernel/Posix/signal_defs.h",
  -- Volume layer
  "Include/Kernel/Driver/Device/BlockDevice/stackable_block_device.h",
  -- SMP
  "Include/Kernel/Smp/smp.h",
  -- Debug
  "Include/Kernel/Debug/debug_commands.h",
  -- VBE BIOS specification structures
  "Include/Kernel/Arch/x86_64/Driver/Vga/vbe_types.h",
  -- x86_64 IDT hardware specification
  "Include/Kernel/Arch/x86_64/Interrupt/interrupt_types.h",
  -- Multiboot2 specification
  "Include/Kernel/Boot/Multiboot/multiboot2.h",
  -- POSIX compatibility types
  "Include/Kernel/Posix/sys/time.h",
} do
  EXEMPTED[path] = true
end

-- ─── CLI arguments ──────────────────────────────────────────────────────────

local FULL_SCAN = false
local args = {...}
for _, arg in ipairs(args) do
  if arg == "--full" or arg == "-f" then FULL_SCAN = true end
end

local scope_label = FULL_SCAN and "ALL" or "KERNEL"

-- Known tech debt (documented in TODO.md, reported as warnings not errors)
local TECH_DEBT = {
  ["Include/Kernel/Scheduler/Task/task.h"] =
    "8+ top-level structs + nested Control/Resources (TODO.md: AGENTS.md P1 Secret Rule #1)",
  ["Include/Kernel/Boot/boot_info.h"] =
    "7 top-level types (TODO.md: AGENTS.md P1 Secret Rule #2)",
  ["Include/Kernel/Boot/boot_timer.h"] =
    "nested Mark struct (TODO.md: AGENTS.md P1 Secret Rule #10)",
  ["Include/Kernel/Loader/Domains/dynamic_domain.h"] =
    "nested RelaTable/SymbolContext (TODO.md: AGENTS.md P1 Secret Rule #9)",
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

-- ─── Scanner ─────────────────────────────────────────────────────────────────

-- Returns: top_types, nested_types, err
-- Each type entry: { name, line, kind }
local function scan_file(filepath)
  local f = io.open(filepath, "r")
  if not f then return nil, nil, "Cannot open file" end
  local content = f:read("*a")
  f:close()

  -- ALSO keep raw content for extern "C" detection
  local raw_content = content
  content = strip_comments_and_strings(content)

  local top_types    = {}
  local nested_types = {}
  local line         = 1
  local n            = #content

  -- Scope stack: each entry is "type", "namespace", or "other"
  local scope_stack = {}

  -- Helper: test if a keyword starts at position pos
  local function at_kw(kw, pos)
    if pos + #kw - 1 > n then return false end
    if content:sub(pos, pos + #kw - 1) ~= kw then return false end
    local prev = content:sub(pos - 1, pos - 1)
    if prev ~= "" and prev:match("%w") then return false end
    local after = content:sub(pos + #kw, pos + #kw)
    if after ~= "" and after:match("%w") then return false end
    return true
  end

  -- Helper: skip whitespace from position
  local function skip_ws(pos)
    while pos <= n and content:sub(pos, pos):match("%s") do
      if content:sub(pos, pos) == '\n' then line = line + 1 end
      pos = pos + 1
    end
    return pos
  end

  -- Helper: extract a C++ identifier starting at pos
  local function extract_name(pos)
    local start = pos
    while pos <= n and content:sub(pos, pos):match("[%w_~]") do
      pos = pos + 1
    end
    if pos == start then return "", pos end
    return content:sub(start, pos - 1), pos
  end

  -- Helper: check if we are nested inside a "type" scope
  local function is_type_nested()
    for _, s in ipairs(scope_stack) do
      if s == "type" then return true end
    end
    return false
  end

  -- Helper: find the closest enclosing type name for error reporting
  local function find_enclosing_type()
    -- Search backwards through top_types for the most recent one that is
    -- still "open" (hasn't been closed yet). We use a simple heuristic:
    -- the last top-level type added that was a struct/class (not enum).
    for i = #top_types, 1, -1 do
      local t = top_types[i]
      if t.kind == "struct" or t.kind == "class" then
        return t.name
      end
    end
    return "unknown"
  end

  -- Helper: find the position of a matching } for extern "C" in raw content
  -- Used to skip extern blocks as namespace scope
  local function find_extern_block_braces(raw)
    local extern_fn = raw:match('extern%s+"[^"]*"%s*({)')
    if not extern_fn then return nil, nil end
    -- Find the { position and its matching }
    local open_pos = raw:find('extern%s+"[^"]*"%s*{')
    if not open_pos then return nil, nil end
    -- Find the { character
    local _, brace_pos = raw:find('extern%s+"[^"]*"%s*{', open_pos)
    local depth = 1
    local pos = brace_pos + 1
    while pos <= #raw and depth > 0 do
      local c = raw:sub(pos, pos)
      if c == '{' then depth = depth + 1
      elseif c == '}' then depth = depth - 1
      end
      pos = pos + 1
    end
    return brace_pos, pos - 1  -- positions of { and }
  end

  -- Pre-scan: find extern "C" { ... } blocks in RAW content and record
  -- their { } positions for namespace scope handling
  local extern_opens = {}  -- position -> true for { that starts extern block
  local extern_closes = {} -- position -> true for } that ends extern block
  do
    local pos = 1
    while pos <= #raw_content do
      local s = raw_content:find('extern%s+"[^"]*"%s*{', pos)
      if not s then break end
      -- Find the { character
      local open_brace = raw_content:find('{', s)
      if not open_brace then break end
      -- Find matching }
      local depth = 1
      local p = open_brace + 1
      while p <= #raw_content and depth > 0 do
        local c = raw_content:sub(p, p)
        if c == '{' then depth = depth + 1
        elseif c == '}' then depth = depth - 1
        end
        p = p + 1
      end
      extern_opens[open_brace] = true
      extern_closes[p - 1] = true
      pos = p
    end
  end

  -- ─── Main scan loop ─────────────────────────────────────────────────────

  local i = 1
  while i <= n do
    local ch = content:sub(i, i)

    -- Track line number
    if ch == '\n' then
      line = line + 1
      i = i + 1
      goto continue
    end

    -- ── Keyword: namespace ──
    -- Sets next { to be "namespace" scope
    if at_kw("namespace", i) then
      local after_ns = skip_ws(i + 9)
      if after_ns <= n then
        local ns_char = content:sub(after_ns, after_ns)
        -- Could be "namespace name {", "namespace {", or "namespace name = ..."
        if ns_char == '{' then
          -- anonymous namespace
          -- DON'T consume the { here - let the main loop handle it
          -- But we need to tell the main loop that the next { is namespace
          -- We do this by setting a flag in the scope that the main { handler checks
          -- Actually, simpler: just handle it inline
          table.insert(scope_stack, "namespace")
          i = after_ns + 1  -- skip past {
          goto continue
        elseif ns_char:match("[%w_~]") then
          -- named namespace - could be "namespace X { }" or "namespace X::Y { }"
          -- Skip the name(s)
          local _, name_end = extract_name(after_ns)
          -- Handle nested namespaces like A::B::C
          local scan = name_end
          while scan <= n and content:sub(scan, scan) == ':' do
            if content:sub(scan, scan + 1) == '::' then
              scan = scan + 2
              local _, next_end = extract_name(scan)
              scan = next_end
            else
              break
            end
          end
          local after_name = skip_ws(scan)
          if after_name <= n and content:sub(after_name, after_name) == '{' then
            table.insert(scope_stack, "namespace")
            i = after_name + 1
            goto continue
          end
        end
      end
      i = i + 9
      goto continue
    end

    -- ── Keyword: extern (handled via the pre-scanned extern_opens) ──
    -- The main { } handler (below) checks extern_opens/extern_closes tables
    -- to push "namespace" instead of "other"

    -- ── Keyword: template ──
    -- Skip template <...> parameters to avoid matching "class" inside them
    if at_kw("template", i) then
      local after_tmpl = skip_ws(i + 8)
      if after_tmpl <= n and content:sub(after_tmpl, after_tmpl) == '<' then
        local tdepth = 1
        local pos = after_tmpl + 1
        while pos <= n and tdepth > 0 do
          local tc = content:sub(pos, pos)
          if tc == '\n' then line = line + 1 end
          if tc == '<' then tdepth = tdepth + 1
          elseif tc == '>' then tdepth = tdepth - 1
          end
          pos = pos + 1
        end
        i = pos
        goto continue
      end
      i = i + 8
      goto continue
    end

    -- ── Braces ──

    if ch == '{' then
      if extern_opens[i] then
        table.insert(scope_stack, "namespace")
      else
        table.insert(scope_stack, "other")
      end
      i = i + 1
      goto continue
    end

    if ch == '}' then
      if #scope_stack > 0 then
        table.remove(scope_stack)
      end
      i = i + 1
      goto continue
    end

    -- ── Keyword: struct / class / enum ──

    local kw = nil
    local kw_len = 0

    if at_kw("struct", i) then kw = "struct"; kw_len = 6
    elseif at_kw("class", i) then kw = "class"; kw_len = 5
    elseif at_kw("enum", i) then kw = "enum"; kw_len = 4
    end

    if kw then
      local pos = i + kw_len
      local type_kind = kw

      if kw == "enum" then
        -- Check for "enum class" or "enum struct"
        local after_enum = skip_ws(pos)
        if after_enum <= n then
          local sub4 = content:sub(after_enum, after_enum + 4)
          if sub4:match("^class") then
            type_kind = "enum class"; pos = after_enum + 5
          elseif sub4:match("^struct") then
            type_kind = "enum struct"; pos = after_enum + 6
          end
        end
      end

      pos = skip_ws(pos)

      -- Extract type name
      local name, name_end = extract_name(pos)

      -- Skip anonymous types and numeric names
      if name == "" or name:match("^%d") then
        i = i + kw_len
        goto continue
      end

      -- Skip friend declarations
      local prev_ctx = content:sub(math.max(1, i - 20), i - 1)
      if prev_ctx:match("friend%s*$") then
        i = i + kw_len
        goto continue
      end

      -- Determine if this is a definition or a forward declaration
      -- Skip whitespace, check for inheritance, then look for { or ;
      -- ALSO detect variable declarations like "struct Foo x = { ... }"
      local is_definition = false
      local brace_pos = nil  -- position of { that starts the definition body

      local scan = skip_ws(name_end)
      local paren_depth = 0
      local scan_limit = math.min(scan + 1000, n)

      -- Check for inheritance
      if scan <= n and content:sub(scan, scan) == ':' then
        -- Skip past inheritance specifiers
        scan = scan + 1
        while scan <= scan_limit do
          local sc = content:sub(scan, scan)
          if sc == '\n' then line = line + 1 end
          if sc == '{' and paren_depth == 0 then
            is_definition = true
            brace_pos = scan
            break
          elseif sc == ';' and paren_depth == 0 then
            break  -- forward declaration
          elseif sc == '(' then paren_depth = paren_depth + 1
          elseif sc == ')' then paren_depth = math.max(0, paren_depth - 1)
          end
          scan = scan + 1
        end
      else
        -- No inheritance. Skip whitespace.
        -- If the next non-whitespace char is {, it's a definition.
        -- If it's an identifier, =, or ; it's a variable decl or forward decl.
        scan = skip_ws(name_end)
        if scan <= n then
          local next_c = content:sub(scan, scan)
          if next_c == '{' then
            is_definition = true
            brace_pos = scan
          elseif next_c == ';' then
            -- forward declaration
          elseif next_c:match("[%w_~]") then
            -- Variable declaration: struct Foo x = ... or struct Foo x;
            -- The identifier after Foo is a variable name, not the body {
            -- Skip until { or ;
            while scan <= scan_limit do
              local sc = content:sub(scan, scan)
              if sc == '\n' then line = line + 1 end
              if sc == '{' and paren_depth == 0 then
                -- This is the = { ... } initializer, NOT the type body
                -- Not a type definition, it's a variable with struct init
                break
              elseif sc == ';' and paren_depth == 0 then
                break  -- struct Foo x; or struct Foo x = init;
              elseif sc == '(' then paren_depth = paren_depth + 1
              elseif sc == ')' then paren_depth = math.max(0, paren_depth - 1)
              end
              scan = scan + 1
            end
          end
        end
      end

      if not is_definition then
        i = i + kw_len
        goto continue
      end

      -- ── This IS a type definition ──

      local display_name = type_kind .. " " .. name
      local saved_line = line

      if is_type_nested() then
        table.insert(nested_types, {
          name = display_name,
          line = saved_line,
          kind = type_kind,
          parent = find_enclosing_type(),
        })
      else
        table.insert(top_types, { name = display_name, line = saved_line, kind = type_kind })
      end

      -- Push "type" onto the scope stack and consume the { that starts the body
      table.insert(scope_stack, "type")

      -- Advance main position past the { that opens the definition body
      i = brace_pos + 1
      goto continue
    end

    i = i + 1
    ::continue::
  end

  return top_types, nested_types, nil
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
  -- Try common locations
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

-- ─── Main ───────────────────────────────────────────────────────────────────

local function main()
  local all_errors     = {}
  local all_warnings   = {}
  local exempt_found   = {}

  local headers, sources
  if FULL_SCAN then
    headers = find_files(ROOT .. "/Include", "*.h")
    sources = find_files(ROOT .. "/Src", "*.cpp")
  else
    headers = find_files(ROOT .. "/Include/Kernel", "*.h")
    sources = find_files(ROOT .. "/Src/Kernel", "*.cpp")
  end

  local function process_file(file, is_source)
    local rf = relpath(file)
    local top_types, nested_types, err = scan_file(file)

    if err then
      table.insert(all_errors, { file = rf, msg = "Parse error: " .. err })
      return
    end

    local top_count    = #top_types
    local nested_count = #nested_types

    -- For source files (.cpp), only flag if there are NESTED types
    -- Top-level types in .cpp are often file-local helpers in anonymous namespaces
    -- (which look like top-level to our scanner)
    if is_source then
      if nested_count == 0 then return end
      top_count = 0  -- don't report top-level for source files
    end

    if top_count > 1 or nested_count > 0 then
      local msg_parts = {}
      if top_count > 1 then
        table.insert(msg_parts, top_count .. " top-level types")
      end
      if nested_count > 0 then
        table.insert(msg_parts, nested_count .. " nested types")
      end
      local msg = table.concat(msg_parts, ", ")

      local entry = {
        file = rf,
        msg = msg,
        top_types = top_types,
        nested_types = nested_types,
      }

      if EXEMPTED[rf] then
        table.insert(exempt_found, entry)
      elseif TECH_DEBT[rf] then
        entry.tech_debt_msg = TECH_DEBT[rf]
        table.insert(all_warnings, entry)
      else
        table.insert(all_errors, entry)
      end
    end
  end

  for _, file in ipairs(headers) do process_file(file, false) end
  for _, file in ipairs(sources) do process_file(file, true) end

  -- ─── Report ──────────────────────────────────────────────────────────

  print(string.format("\n🔍 [%s] Checking one struct/class per file rule...\n", script_name))
  print(string.format("   Scanned [%s]:  %d headers, %d sources", scope_label, #headers, #sources))
  print(string.format("   Exempt:   %d files (containers, spec structs, formats)", #exempt_found))
  print("")

  local total_errors   = #all_errors
  local total_warnings = #all_warnings

  if total_errors == 0 and total_warnings == 0 then
    print("✅ [PASS] All files conform to one-struct-per-file rule.")
    os.exit(0)
  end

  if total_warnings > 0 then
    print(string.format("⚠️  KNOWN TECH DEBT (%d files):", total_warnings))
    print("   " .. string.rep("─", 65))
    for _, w in ipairs(all_warnings) do
      print(string.format("   📄 %s", w.file))
      print(string.format("      └─ %s", w.msg))
      if w.tech_debt_msg then
        print(string.format("         ⓘ  %s", w.tech_debt_msg))
      end
      if w.top_types and #w.top_types > 1 then
        for _, t in ipairs(w.top_types) do
          print(string.format("         • %s (line %d)", t.name, t.line))
        end
      end
      if w.nested_types and #w.nested_types > 0 then
        for _, t in ipairs(w.nested_types) do
          print(string.format("         • %s → nested inside %s (line %d)", t.name, t.parent, t.line))
        end
      end
      print("")
    end
  end

  if total_errors > 0 then
    print(string.format("❌ VIOLATIONS (%d files):", total_errors))
    print("   " .. string.rep("─", 65))
    for _, e in ipairs(all_errors) do
      print(string.format("   📄 %s", e.file))
      print(string.format("      └─ %s", e.msg))
      if e.top_types and #e.top_types > 0 then
        for _, t in ipairs(e.top_types) do
          print(string.format("         • %s (line %d)", t.name, t.line))
        end
      end
      if e.nested_types and #e.nested_types > 0 then
        for _, t in ipairs(e.nested_types) do
          print(string.format("         • %s → nested inside %s (line %d)", t.name, t.parent, t.line))
        end
      end
      print("")
    end
    print(string.format("   Fix: Extract each type to its own file."))
    os.exit(1)
  end

  print("✅ [PASS] No hard violations found (only known tech debt warnings above).")
  os.exit(0)
end

main()
