--[[
    Meta/wiggum.lua - Ralph Wiggum Autonomous Coding Agent for FKernel
    (Modular, Hardened & Agentic version)
--]]

require("Meta.Lib.os_interact")
require("Meta.Lib.run_command")
require("Meta.Lib.print_message")
local Json = require("Meta.Lib.json_utils")
local Git = require("Meta.Lib.git_utils")
local LLM = require("Meta.Lib.llm_client")

local RALPH_DIR = ".ralph"
local RALPH_LOGS_DIR = ".ralph/logs"

local Wiggum = {
    state = {
        tasks = {},
        iteration = 0,
        phase = "IDLE",
        current_model = nil,
        model_usage = {}
    }
}

function Wiggum:log(msg, level)
    local timestamp = os.date("%Y-%m-%d %H:%M:%S")
    print(string.format("[%s] [%s] %s", timestamp, level or "INFO", msg))
    os.execute("mkdir -p " .. RALPH_LOGS_DIR)
    local f = io.open(RALPH_LOGS_DIR .. "/wiggum.log", "a")
    if f then
        f:write(string.format("[%s] [%s] %s\n", timestamp, level or "INFO", msg))
        f:close()
    end
end

function Wiggum:load_state()
    local path = RALPH_DIR .. "/state.json"
    if OSInteract.FileExists(path) then
        local f = io.open(path, "r")
        local content = f:read("*a")
        f:close()
        local parsed = Json.parse(content)
        if parsed then 
            self.state = parsed 
        else
            self:log("Failed to parse state.json, using defaults", "WARNING")
        end
    end
end

function Wiggum:save_state()
    os.execute("mkdir -p " .. RALPH_DIR)
    self.state.last_updated = os.date("%Y-%m-%dT%H:%M:%SZ")
    self.state.model_usage = LLM.usage -- Sincroniza uso de tokens
    local f = io.open(RALPH_DIR .. "/state.json", "w")
    f:write(Json.encode(self.state))
    f:close()
end

function Wiggum:import_tasks()
    self:log("Importing tasks from TODO.md...")
    local f = io.open("TODO.md", "r")
    if not f then return end
    local content = f:read("*a")
    f:close()
    
    local tasks = {}
    for line in content:gmatch("[^\n]+") do
        if line:find("^%s*-%s*%[%s*%]") then
            local title = line:gsub("^%s*-%s*%[%s*%]%s*", ""):gsub("%s+", " ")
            table.insert(tasks, {
                id = "task_" .. string.format("%04d", #tasks + 1),
                title = title,
                status = "pending",
                priority = (line:find("🚨") or line:find("critical")) and "critical" or "medium",
                attempts = 0
            })
        end
    end
    self.state.tasks = tasks
    self.state.phase = "PLANNING"
    self:save_state()
    self:log("Imported " .. #tasks .. " tasks.")
end

function Wiggum:run_build(max_iters, concurrency)
    self.state.phase = "BUILDING"
    local base_branch = Git.get_current_branch()
    local wiggum_branch = Git.get_wiggum_branch_name(base_branch)
    Git.ensure_branch(wiggum_branch)

    if concurrency > 1 then
        self:log("Spawning " .. concurrency .. " workers...", "INFO")
        for i = 1, concurrency do
            os.execute("lua Meta/wiggum.lua build 1 &")
        end
        return
    end

    for i = 1, max_iters do
        local task = nil
        for _, t in ipairs(self.state.tasks) do
            if t.status == "pending" then task = t; break end
        end

        if not task then break end

        task.status = "in_progress"
        task.attempts = (task.attempts or 0) + 1
        self:save_state()

        local model = LLM.get_next_model()
        self.state.current_model = model
        LLM.wait_if_needed(model)

        self:log("Starting mission: " .. task.title .. " with " .. model)
        
        local prompt = string.format([[
Mission: %s
Rules: Object Calisthenics, Result<T, Error>, One class per file.
Objective: Implement and ensure 'xmake -bv' and 'xmake run Test' pass.
]], task.title)

        local output = LLM.call(model, prompt)
        
        self:log("Validation phase for " .. task.id)
        if RunCommand("xmake -bv 2>&1") and RunCommand("xmake run Test 2>&1") then
            task.status = "completed"
            Git.commit("feat: " .. task.title)
            self:log("Task " .. task.id .. " SUCCESS", "INFO")
        else
            if task.attempts >= 3 then
                task.status = "blocked"
                task.feedback = "Failed after 3 attempts with different models."
            else
                task.status = "pending"
                os.execute("git checkout .") -- Cleanup
            end
            self:log("Task " .. task.id .. " FAILED/RETRY", "WARNING")
        end
        self.state.iteration = self.state.iteration + 1
        self:save_state()
    end
    self.state.phase = "IDLE"
    self:save_state()
end

function Wiggum:analyze()
    self.state.phase = "ANALYZING"
    self:log("Performing full codebase analysis...")
    
    local report = {
        timestamp = os.date("%Y-%m-%dT%H:%M:%SZ"),
        sections = {
            { name = "Build", status = RunCommand("xmake build -q") and "✅" or "❌", details = "Kernel compilation" },
            { name = "Tests", status = "📊", details = "Check dashboard for coverage" }
        }
    }
    
    -- Sync TODO.md
    local f = io.open("TODO.md", "r")
    local lines = {}
    for line in f:lines() do
        local updated = line
        for _, t in ipairs(self.state.tasks) do
            if t.status == "completed" then
                local escaped = t.title:gsub("([%^%$%(%)%%%.%[%]%*%+%-%?])", "%%%1")
                updated = updated:gsub("%%- %[ %] " .. escaped, "- [x] " .. t.title)
            end
        end
        table.insert(lines, updated)
    end
    f:close()
    
    local f2 = io.open("TODO.md", "w")
    f2:write(table.concat(lines, "\n"))
    f2:close()
    
    local f3 = io.open(RALPH_DIR .. "/analysis_report.json", "w")
    f3:write(Json.encode(report))
    f3:close()
    
    self.state.phase = "IDLE"
    self:save_state()
    self:log("Analysis complete and TODO.md synced.")
end

-- Entry Point
local args = {...}
local cmd = args[1] or "status"

Wiggum:load_state()

if cmd == "import" then
    Wiggum:import_tasks()
elseif cmd == "build" then
    local iters = tonumber(args[2]) or 1
    local concurrency = 1
    if args[2] == "--parallel" then
        concurrency = tonumber(args[3]) or 2
        iters = 1
    end
    Wiggum:run_build(iters, concurrency)
elseif cmd == "analyze" then
    Wiggum:analyze()
elseif cmd == "status" then
    print("Ralph Wiggum Status:")
    print("Phase: " .. Wiggum.state.phase)
    print("Tasks: " .. #Wiggum.state.tasks)
else
    print("Usage: lua Meta/wiggum.lua [import|build|analyze|status]")
end