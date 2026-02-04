--[[
    Meta/wiggum.lua - Ralph Wiggum Autonomous Coding Agent for FKernel
    (Modular, Hardened, Agentic & Marathon version)
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
            self:log("Failed to parse state.json", "WARNING")
        end
    end
end

function Wiggum:save_state()
    os.execute("mkdir -p " .. RALPH_DIR)
    self.state.last_updated = os.date("%Y-%m-%dT%H:%M:%SZ")
    self.state.model_usage = LLM.usage 
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
    local status_handle = io.popen("git status --porcelain")
    local status = status_handle:read("*a")
    status_handle:close()
    
    if status ~= "" then
        self:log("Working directory is dirty! Commit changes before marathon.", "ERROR")
        return
    end

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

    local marathon_count = 0
    while true do
        if max_iters and marathon_count >= max_iters then break end

        local task = nil
        for _, t in ipairs(self.state.tasks) do
            if t.status == "pending" then task = t; break end
        end

        if not task then 
            self:log("🏁 MARATHON COMPLETE", "INFO")
            break 
        end

        marathon_count = marathon_count + 1
        task.status = "in_progress"
        task.attempts = (task.attempts or 0) + 1
        self:save_state()

        local model = LLM.get_next_model()
        self.state.current_model = model
        LLM.wait_if_needed(model)

        self:log("🏃 Step " .. marathon_count .. " | Task: " .. task.title .. " (" .. model .. ")")
        
        local prompt = string.format([[
Project: FKernel
Task: %s
Rules: Object Calisthenics, Result<T, Error>, One class per file.
Validation: Must pass 'xmake -bv' and 'xmake run Test'.
]], task.title)

        LLM.call(model, prompt)
        
        self:log("Validating " .. task.id)
        local build_ok = RunCommand("xmake -bv 2>&1")
        local test_ok = build_ok and RunCommand("xmake run Test 2>&1")

        if build_ok and test_ok then
            task.status = "completed"
            task.feedback = nil
            Git.commit("feat: " .. task.title)
            self:log("✅ Task SUCCESS", "INFO")
        else
            if (task.attempts or 0) >= 3 then
                task.status = "blocked"
                task.feedback = "Failed 3 models."
                self:log("🛑 Task BLOCKED", "ERROR")
            else
                task.status = "pending"
                os.execute("git checkout .") 
                self:log("🔄 Task FAILED. Cleaning up...", "WARNING")
            end
        end
        
        self.state.iteration = marathon_count
        self:save_state()
        
        -- Pequena pausa para permitir interrupção via Ctrl+C entre tarefas
        local _, reason = os.execute("sleep 1")
        if reason == "signal" then
            self:log("Marathon interrupted by user.", "INFO")
            break
        end
    end
    
    self.state.phase = "IDLE"
    self:save_state()
end

function Wiggum:analyze()
    self.state.phase = "ANALYZING"
    self:log("Analyzing codebase...")
    local build_status = RunCommand("xmake build -q") and "✅" or "❌"
    local report = {
        timestamp = os.date("%Y-%m-%dT%H:%M:%SZ"),
        sections = {
            { name = "Build", status = build_status, details = "Kernel compilation" },
            { name = "Test", status = "📊", details = "Live test results" }
        }
    }
    
    -- Sync TODO.md
    local f = io.open("TODO.md", "r")
    if f then
        local lines = {}
        for line in f:lines() do
            local updated = line
            for _, t in ipairs(self.state.tasks) do
                if t.status == "completed" then
                    local esc = t.title:gsub("([%^%$%(%)%%%.%[%]%*%+%-%?])", "%%%1")
                    updated = updated:gsub("%%- %[ %] " .. esc, "- [x] " .. t.title)
                end
            end
            table.insert(lines, updated)
        end
        f:close()
        local fw = io.open("TODO.md", "w")
        fw:write(table.concat(lines, "\n"))
        fw:close()
    end
    
    local fr = io.open(RALPH_DIR .. "/analysis_report.json", "w")
    fr:write(Json.encode(report))
    fr:close()
    
    self.state.phase = "IDLE"
    self:save_state()
end

local args = {...}
local cmd = args[1] or "status"
Wiggum:load_state()

if cmd == "import" then
    Wiggum:import_tasks()
elseif cmd == "build" then
    local iters = tonumber(args[2]) or 999
    local concurrency = 1
    if args[2] == "--parallel" then
        concurrency = tonumber(args[3]) or 2
        iters = 1
    end
    Wiggum:run_build(iters, concurrency)
elseif cmd == "analyze" then
    Wiggum:analyze()
elseif cmd == "status" then
    print("Ralph Wiggum v3.0 (Marathon Mode)")
    print("Phase: " .. Wiggum.state.phase)
    print("Tasks: " .. #Wiggum.state.tasks)
else
    print("Usage: lua Meta/wiggum.lua [import|build|analyze|status]")
end