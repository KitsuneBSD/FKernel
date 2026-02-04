#!/usr/bin/env lua
-- Meta/wiggum-dashboard.lua - Ralph Wiggum TUI Dashboard

local Json = require("Meta.Lib.json_utils")
local OS = require("Meta.Lib.os_interact")

local RALPH_DIR = ".ralph"
local STATE_PATH = RALPH_DIR .. "/state.json"
local REPORT_PATH = RALPH_DIR .. "/analysis_report.json"

local function read_file(path)
    local f = io.open(path, "r")
    if not f then return nil end
    local content = f:read("*a")
    f:close()
    return content
end

local function format_usage(usage)
    local lines = {}
    table.insert(lines, "--- Model Usage (RPM/Total) ---")
    
    local models = {}
    for name, data in pairs(usage or {}) do
        table.insert(models, {name = name, data = data})
    end
    table.sort(models, function(a, b) return a.name < b.name end)

    for _, m in ipairs(models) do
        local name = m.name:gsub("opencode/", "")
        table.insert(lines, string.format("%-20s : %d RPM | Total: %d", 
            name, m.data.calls_this_minute or 0, m.data.calls_today or 0))
    end
    return table.concat(lines, "\n")
end

local function format_tasks(tasks)
    local completed = 0
    local pending = 0
    local failed = 0
    local blocked = 0
    local feedback = ""
    
    for _, t in ipairs(tasks or {}) do
        if t.status == "completed" then completed = completed + 1
        elseif t.status == "failed" then failed = failed + 1
        elseif t.status == "blocked" then 
            blocked = blocked + 1
            if #feedback < 300 then -- Mostra apenas o primeiro feedback encontrado para não poluir
                feedback = feedback .. "\n ! " .. t.title .. ": " .. (t.feedback or "No explanation"):sub(1, 100) .. "..."
            end
        else pending = pending + 1 end
    end
    
    local res = string.format("Tasks: %d Done | %d Pend | %d Block | %d Fail", 
        completed, pending, blocked, failed)
    
    if blocked > 0 then
        res = res .. "\n\nBlocker Details:" .. feedback
    end
    return res
end

local function get_summary_text()
    local state_raw = read_file(STATE_PATH)
    local report_raw = read_file(REPORT_PATH)
    
    local state = Json.parse(state_raw) or {}
    local report = Json.parse(report_raw) or {}
    
    local lines = {}
    table.insert(lines, "=== RALPH WIGGUM COMMAND CENTER ===")
    table.insert(lines, "Status: " .. (state.phase or "IDLE"):upper())
    table.insert(lines, "Iteration: " .. (state.iteration or 0))
    table.insert(lines, "Last Update: " .. (state.last_updated or "N/A"))
    table.insert(lines, "")
    
    table.insert(lines, format_tasks(state.tasks))
    table.insert(lines, "")
    
    if report.sections then
        table.insert(lines, "--- Analysis Report ---")
        for _, sec in ipairs(report.sections) do
            table.insert(lines, string.format("[%s] %-18s: %s", 
                sec.status or "?", sec.name, sec.details))
        end
        table.insert(lines, "")
    end
    
    table.insert(lines, format_usage(state.model_usage))
    
    return table.concat(lines, "\n")
end

-- Main Loop
while true do
    local text = get_summary_text()
    -- Use dialog to show the status
    -- infobox doesn't wait for user, perfect for live updates
    local cmd = string.format('dialog --backtitle "FKernel - Ralph Wiggum Dashboard" ' ..
                              '--title "System Status" ' ..
                              '--infobox "%s" 22 75', text)
    os.execute(cmd)
    
    -- Check if user wants to quit (hard to do with infobox, so we use a small sleep)
    os.execute("sleep 2")
end