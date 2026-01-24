#!/usr/bin/env lua

local userland_dir = "Src/Userland"
local config_file = "build/initrd_config.txt"
os.execute("mkdir -p build")

local function run_dialog(cmd)
  local handle = io.popen(cmd .. " 2>&1")
  local result = handle:read("*a")
  handle:close()
  return result
end

local categories = {
    Core = {"init", "shell"},
    Utilities = {"cat", "ls", "echo", "pwd", "touch", "clear"},
    System = {"mount", "umount", "uname", "id", "kill", "sleep", "date"}
}

-- Detect any other components not in categories
local all_components = {}
local p = io.popen("ls -d " .. userland_dir .. "/*/")
for line in p:lines() do
  local name = line:match("Userland/([^/]+)/$")
  if name and name ~= "lib" and name ~= "libbsd" then
    all_components[name] = true
  end
end
p:close()

-- Add uncategorized to Utilities
for name, _ in pairs(all_components) do
    local found = false
    for _, list in pairs(categories) do
        for _, comp in ipairs(list) do
            if comp == name then found = true break end
        end
        if found then break end
    end
    if not found then
        table.insert(categories.Utilities, name)
    end
end

local sys_type = "minimal"
local selected = {}
for name, _ in pairs(all_components) do
    selected[name] = true
end

while true do
    local menu_cmd = "dialog --stdout --title \"FKernel InitRD Configuration\" --menu \"Main Menu\" 18 60 8 " ..
        "type \"Base System: [" .. sys_type .. "]\" " ..
        "core \"Select Core Components\" " ..
        "utils \"Select Utility Components\" " ..
        "sys \"Select System Components\" " ..
        "save \"Save and Exit\" " ..
        "exit \"Exit without saving\""
    
    local choice = run_dialog(menu_cmd)

    if choice == "type" then
        sys_type = run_dialog("dialog --stdout --title \"System Architecture\" --menu \"Choose Base System:\" 15 60 5 " ..
            "minimal \"Native Shell (Minimalist)\" " ..
            "standard \"Standard (Musl + BusyBox)\" " ..
            "advanced \"Advanced (OpenRC + Full Suite)\" ")
        if sys_type == "" then sys_type = "minimal" end
    elseif choice == "core" or choice == "utils" or choice == "sys" then
        local cat_name = choice == "core" and "Core" or (choice == "utils" and "Utilities" or "System")
        local checklist_cmd = "dialog --stdout --separate-output --checklist \"Select " .. cat_name .. " Components:\" 20 60 10 "
        for _, comp in ipairs(categories[cat_name]) do
            local status = selected[comp] and "on" or "off"
            checklist_cmd = checklist_cmd .. string.format("%s \"%s\" %s ", comp, comp:upper(), status) .. " "
        end
        local result = run_dialog(checklist_cmd)
        -- Clear current category selections
        for _, comp in ipairs(categories[cat_name]) do
            selected[comp] = false
        end
        -- Set new selections
        for line in result:gmatch("[^\n]+") do
            selected[line] = true
        end
    elseif choice == "save" then
        local f = io.open(config_file, "w")
        if f then
            f:write("SYSTEM_TYPE=" .. sys_type .. "\n")
            local comps = {}
            for name, is_selected in pairs(selected) do
                if is_selected then table.insert(comps, name) end
            end
            f:write("COMPONENTS=" .. table.concat(comps, ",") .. "\n")
            f:close()
            local PrintMessage = require("Meta.Lib.print_message")
            PrintMessage(false, "Deep configuration saved to " .. config_file)
            break
        else
            local PrintMessage = require("Meta.Lib.print_message")
            PrintMessage(true, "Failed to save configuration.")
        end
    elseif choice == "exit" or choice == "" then
        os.exit(0)
    end
end