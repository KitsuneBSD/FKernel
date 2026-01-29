#!/usr/bin/env lua

-- EFI Header Patcher for FKernel (Compatibility Version)

local function write_u16(val)
    return string.char(val & 0xFF, (val >> 8) & 0xFF)
end

local function write_u32(val)
    return string.char(val & 0xFF, (val >> 8) & 0xFF, (val >> 16) & 0xFF, (val >> 24) & 0xFF)
end

local function write_u64(val)
    local low = val & 0xFFFFFFFF
    local high = (val >> 32) & 0xFFFFFFFF
    return write_u32(low) .. write_u32(high)
end

local function patch_efi(input_path, output_path)
    local f_in = io.open(input_path, "rb")
    if not f_in then return false, "Could not open input file" end
    local content = f_in:read("*all")
    f_in:close()

    local size = #content
    local entry_point = 0x1000 

    -- 1. DOS Header
    local dos = "MZ" .. string.rep("\0", 58) .. write_u32(0x80)
    dos = dos .. string.rep("\0", 0x80 - #dos)

    -- 2. PE Header
    local pe = "PE\0\0"
    pe = pe .. write_u16(0x8664) -- x86_64
    pe = pe .. write_u16(1)      -- 1 section
    pe = pe .. write_u32(os.time())
    pe = pe .. write_u32(0) .. write_u32(0) -- Symbols
    pe = pe .. write_u16(0xF0)   -- Size of optional header
    pe = pe .. write_u16(0x0202) -- Characteristics

    -- 3. Optional Header (PE32+)
    local opt = write_u16(0x020B) -- Magic PE32+
    opt = opt .. string.char(0x02, 0x1E) -- Linker version
    opt = opt .. write_u32(size)  -- Size of code
    opt = opt .. write_u32(0) .. write_u32(0) -- Data
    opt = opt .. write_u32(entry_point)
    opt = opt .. write_u32(0x1000) -- Base of code
    opt = opt .. write_u64(0x100000) -- Image Base
    opt = opt .. write_u32(0x1000) -- Section alignment
    opt = opt .. write_u32(0x200)  -- File alignment
    opt = opt .. write_u32(0) .. write_u32(0) -- Versions
    opt = opt .. write_u16(0) .. write_u16(0) -- Subsystem versions
    opt = opt .. write_u32(0)      -- Win32 version
    opt = opt .. write_u32(size + 0x2000) -- Size of image
    opt = opt .. write_u32(0x400)  -- Size of headers
    opt = opt .. write_u32(0)      -- Checksum
    opt = opt .. write_u16(10)     -- Subsystem (EFI Application)
    opt = opt .. write_u16(0)      -- DLL Characteristics
    
    opt = opt .. write_u64(0x100000) -- Stack reserve
    opt = opt .. write_u64(0x1000)   -- Stack commit
    opt = opt .. write_u64(0x100000) -- Heap reserve
    opt = opt .. write_u64(0x1000)   -- Heap commit
    opt = opt .. write_u32(0)        -- Loader flags
    opt = opt .. write_u32(16)       -- Data directories
    opt = opt .. string.rep("\0", 128)

    -- 4. Section Table
    local sec = ".text" .. string.rep("\0", 3)
    sec = sec .. write_u32(size)   -- Virtual size
    sec = sec .. write_u32(0x1000) -- RVA
    sec = sec .. write_u32(size)   -- Raw size
    sec = sec .. write_u32(0x400)  -- Raw pointer
    sec = sec .. write_u32(0) .. write_u32(0) .. write_u16(0) .. write_u16(0) -- Relocs
    sec = sec .. write_u32(0x60000020) -- Code, Execute, Read

    local f_out = io.open(output_path, "wb")
    f_out:write(dos)
    f_out:write(pe)
    f_out:write(opt)
    f_out:write(sec)
    
    local current = f_out:seek()
    f_out:write(string.rep("\0", 0x400 - current))
    f_out:write(content)
    f_out:close()

    return true
end

local input, output = arg[1], arg[2]
if not input or not output then os.exit(1) end
patch_efi(input, output)