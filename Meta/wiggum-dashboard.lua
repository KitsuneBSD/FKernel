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
    table.insert(lines, "--- Model Usage (RPM) ---")
    local sorted_models = {}
    for name, data in pairs(usage or {}) do
        table.insert(sorted_models, {name = name, data = data})
    end
    table.sort(sorted_models, function(a, b) return a.name < b.name end)
    for _, m in ipairs(sorted_models) do
        local short_name = m.name:gsub("opencode/", "")
        table.insert(lines, string.format("%-15s: %d RPM | Total: %d", 
            short_name, m.data.count or 0, m.data.total or m.data.count or 0))
    end
    return table.concat(lines, "\n")
end

local function format_tasks(tasks)
    local stats = { pending = 0, completed = 0, failed = 0, blocked = 0, in_progress = 0 }
    local blockers = {}
    for _, t in ipairs(tasks or {}) do
        stats[t.status] = (stats[t.status] or 0) + 1
        if t.status == "blocked" then
            table.insert(blockers, "! " .. t.id .. ": " .. (t.feedback or "No info"):sub(1, 40))
        end
    end
    
    local line1 = string.format("Tasks: %d Done | %d Work | %d Pend | %d Block", 
        stats.completed, stats.in_progress, stats.pending, stats.blocked)
    
    if #blockers > 0 then
        line1 = line1 .. "\n\nBlockers:\n" .. table.concat(blockers, "\n")
    end
    return line1
end

while true do
    local state = Json.parse(read_file(STATE_PATH) or "{}") or {}
    local report = Json.parse(read_file(REPORT_PATH) or "{}") or {}
    
    local view = {
        "=== RALPH WIGGUM COMMAND CENTER ===",
        "Status: " .. (state.phase or "IDLE"):upper(),
        "Iters:  " .. (state.iteration or 0),
        "Model:  " .. (state.current_model or "None"):gsub("opencode/", ""),
        "",
        format_tasks(state.tasks),
        "",
        "--- Analysis ---"
    }
    
    for _, sec in ipairs(report.sections or {}) do
        table.insert(view, string.format("[%s] %-12s: %s", sec.status or "?", sec.name, sec.details))
    end
    
    table.insert(view, "")
    table.insert(view, format_usage(state.model_usage))
    
    local cmd = string.format('dialog --backtitle "FKernel - Ralph Wiggum Dashboard" ' ..
                              '--title "Live Monitor" ' ..
                              '--infobox "%s" 24 78', table.concat(view, "\n"))
    os.execute(cmd)
    os.execute("sleep 2")
end
