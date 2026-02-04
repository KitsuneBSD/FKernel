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
    self:log("Using opencode to import tasks intelligently...", "INFO")
    self.state.phase = "PLANNING"
    
    local model = LLM.get_next_model()
    local prompt = [[
Read the TODO.md file and create a JSON list of pending missions.
For each mission, provide:
- title: clear mission name
- priority: critical, high, or medium
- description: short detail

Return ONLY the JSON in this format:
{
  "tasks": [
    {"title": "...", "priority": "...", "description": "..."}
  ]
}
]]
    
    -- Usamos call sem yolo para capturar o output JSON
    local prompt_file = "/tmp/wiggum_import.txt"
    local f = io.open(prompt_file, "w")
    f:write(prompt)
    f:close()
    
    local cmd = 'opencode run --model "' .. model .. '" --file "' .. prompt_file .. '" --file "TODO.md"'
    local handle = io.popen(cmd)
    local output = handle:read("*a")
    handle:close()
    
    local json_str = output:match("({.*})")
    if json_str then
        local data = Json.parse(json_str)
        if data and data.tasks then
            self.state.tasks = {}
            for i, t in ipairs(data.tasks) do
                t.id = "task_" .. string.format("%04d", i)
                t.status = "pending"
                t.attempts = 0
                table.insert(self.state.tasks, t)
            end
            self:save_state()
            self:log("Imported " .. #self.state.tasks .. " missions via opencode.")
        end
    else
        self:log("Failed to parse missions from opencode. Check TODO.md manually.", "ERROR")
    end
    os.remove(prompt_file)
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
MISSION:
Implement the following feature in FKernel: %s
Description: %s

CONSTRAINTS:
1. One class per file. Deep domain structure (e.g. Src/Kernel/Net/Ipv4/...).
2. Object Calisthenics.
3. No 'else', use early returns.
4. Use Result<T, Error> for all fallible functions.

INSTRUCTIONS:
- You MUST create or update the necessary files.
- You MUST use the available tools to verify that the kernel still compiles.
- When finished, ensure 'xmake -bv' succeeds.
]], task.title, task.description or "")

        -- O LLM.call agora executa o agente e retorna se o processo terminou ok
        LLM.call(model, prompt)
        
        self:log("Validating current repository state for " .. task.id)
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
    self:log("Using opencode to analyze codebase and sync TODO.md...", "INFO")
    
    local model = LLM.get_next_model()
    local prompt = [[
Analyze the current state of the FKernel project and the pending tasks in TODO.md.
Check which tasks are actually implemented and which are missing.
Update the TODO.md file by marking completed tasks with [x] and adding any necessary sub-tasks or quality warnings based on the current code.
Finally, provide a short summary of the project health.
]]
    
    -- Aqui usamos o LLM.call em modo YOLO para que o agente possa editar o TODO.md e analisar arquivos
    LLM.call(model, prompt)
    
    self.state.phase = "IDLE"
    self:save_state()
    self:log("Analysis and sync complete.")
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
    print("Ralph Wiggum v3.2 (Super-Agent Mode)")
    print("Phase: " .. Wiggum.state.phase)
    print("Tasks: " .. #Wiggum.state.tasks)
else
    print("Usage: lua Meta/wiggum.lua [import|build|analyze|status]")
end
