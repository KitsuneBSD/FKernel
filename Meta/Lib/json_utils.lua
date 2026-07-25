-- Meta/Lib/json_utils.lua - Robust JSON handling for FKernel Tools

local JsonUtils = {}

-- Utility to load Lua code across different versions
local function load_chunk(code)
    if loadstring then
        return loadstring(code)
    end
    return load(code)
end

function JsonUtils.parse(content)
    if not content or content == "" then return nil end
    
    local res = {}
    local in_string = false
    local escaped = false
    local i = 1
    
    while i <= #content do
        local c = content:sub(i, i)
        
        if c == '"' and not escaped then
            in_string = not in_string
            table.insert(res, c)
        elseif in_string then
            if c == '\\' then
                escaped = not escaped
            else
                escaped = false
            end
            table.insert(res, c)
        else
            -- Structural JSON to Lua conversion
            if c == '[' then 
                table.insert(res, '{')
            elseif c == ']' then 
                table.insert(res, '}')
            elseif c == ':' then 
                table.insert(res, '=')
            elseif c == 'n' and content:sub(i, i+3) == "null" then
                table.insert(res, "nil")
                i = i + 3
            elseif c == 't' and content:sub(i, i+3) == "true" then
                table.insert(res, "true")
                i = i + 3
            elseif c == 'f' and content:sub(i, i+4) == "false" then
                table.insert(res, "false")
                i = i + 4
            else
                table.insert(res, c)
            end
        end
        i = i + 1
    end
    
    local lua_content = table.concat(res)
    
    -- Final pass to wrap keys in brackets: "key"= value -> ["key"]= value
    lua_content = lua_content:gsub('("[^"]-")%s*=', '[%1]=')
    
    local ok, result = pcall(function()
        local chunk = load_chunk("return " .. lua_content)
        if not chunk then return nil end
        return chunk()
    end)
    
    return ok and result or nil
end

function JsonUtils.encode(data, indent)
    indent = indent or 0
    local indent_str = string.rep("  ", indent)

    if type(data) == "nil" then return "null"
    elseif type(data) == "boolean" then return tostring(data)
    elseif type(data) == "number" then return tostring(data)
    elseif type(data) == "string" then
        local escaped = data:gsub("\\", "\\\\"):gsub("\"", "\\\""):gsub("\n", "\\n")
        return "\"" .. escaped .. "\""
    elseif type(data) == "table" then
        -- Robust array detection
        local is_array = true
        local count = 0
        for k, v in pairs(data) do
            count = count + 1
            if type(k) ~= "number" or k <= 0 or math.floor(k) ~= k then
                is_array = false
                break
            end
        end
        -- Lua-specific: empty table is ambiguous, but we'll treat as array [] 
        -- if it has no string keys, as is common in JSON tasks.
        if count == 0 then is_array = true end
        
        -- Check for gaps in array
        if is_array and count > 0 then
            for i = 1, count do
                if data[i] == nil then
                    is_array = false
                    break
                end
            end
        end

        local items = {}
        if is_array then
            for i = 1, count do
                table.insert(items, indent_str .. "  " .. JsonUtils.encode(data[i], indent + 1))
            end
            return "[\n" .. table.concat(items, ",\n") .. "\n" .. indent_str .. "]"
        else
            local keys = {}
            for k in pairs(data) do 
                if type(k) == "string" then table.insert(keys, k) end
            end
            table.sort(keys)
            for _, k in ipairs(keys) do
                table.insert(items, indent_str .. "  \"" .. k .. "\": " .. JsonUtils.encode(data[k], indent + 1))
            end
            return "{\n" .. table.concat(items, ",\n") .. "\n" .. indent_str .. "}"
        end
    end
    return "null"
end

return JsonUtils
