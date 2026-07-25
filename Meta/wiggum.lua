--[[
    Meta/wiggum.lua - Ralph Wiggum Autonomous Coding Agent for FKernel
    (Modular, Hardened, Agentic, Marathon & Full-Delegation version)
--]]

require("Meta.Lib.os_interact")
require("Meta.Lib.run_command")
require("Meta.Lib.print_message")
local Json = require("Meta.Lib.json_utils")
local Git = require("Meta.Lib.git_utils")
local LLM = require("Meta.Lib.llm_client")

local RALPH_DIR = ".ralph"
local STATE_PATH = RALPH_DIR .. "/state.json"
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
    if OSInteract.FileExists(STATE_PATH) then
        local f = io.open(STATE_PATH, "r")
        local content = f:read("*a")
        f:close()
        local parsed = Json.parse(content)
        if parsed then 
            self.state = parsed 
            return true
        end
    end
    
    if OSInteract.FileExists("extracted_tasks.json") then
        self:log("Found extracted_tasks.json, merging into state...", "INFO")
        local f = io.open("extracted_tasks.json", "r")
        local content = f:read("*a")
        f:close()
        local data = Json.parse(content)
        if data and data.tasks then
            self.state.tasks = {}
            for i, t in ipairs(data.tasks) do
                t.id = t.id or "task_" .. string.format("%04d", i)
                t.status = "pending"
                t.attempts = 0
                table.insert(self.state.tasks, t)
            end
            self:save_state()
            os.remove("extracted_tasks.json")
            return true
        end
    end
    return false
end

function Wiggum:save_state()
    os.execute("mkdir -p " .. RALPH_DIR)
    self.state.last_updated = os.date("%Y-%m-%dT%H:%M:%SZ")
    self.state.model_usage = LLM.usage 
    local f = io.open(STATE_PATH, "w")
    f:write(Json.encode(self.state))
    f:close()
end

function Wiggum:import_tasks()
    self:log("Delegating task import to opencode agent...", "INFO")
    local model = LLM.get_next_model()
    local prompt_file = "/tmp/wiggum_import_prompt.txt"
    local f = io.open(prompt_file, "w")
    f:write(string.format([[
MISSION: Read TODO.md and populate %s.
If the file doesn't exist, create it.
Maintain existing tasks and add new ones from TODO.md.
JSON Structure: {"tasks": [{"id": "task_0001", "title": "...", "priority": "...", "description": "...", "status": "pending"}]}
]], STATE_PATH))
    f:close()

    local cmd = 'opencode run "Import missions from TODO.md" -m ' .. model .. ' -f "' .. prompt_file .. '" -f "TODO.md" --log-level WARN'
    os.execute(cmd)
    os.remove(prompt_file)
    
    if self:load_state() then
        self:log("Import complete. Tasks: " .. #self.state.tasks)
    else
        self:log("Import failed to update state.", "ERROR")
    end
end

function Wiggum:run_build(max_iters, concurrency)
    local status_handle = io.popen("git status --porcelain")
    local status = status_handle:read("*a")
    status_handle:close()
    
    if status ~= "" then
        self:log("Working directory is dirty! Commit changes first.", "ERROR")
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
        self:load_state() 
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
        task.subtasks = task.subtasks or {}
        task.start_hash = Git.get_last_hash()
        self:save_state()

        -- Phase 1: Planning (if no subtasks)
        if #task.subtasks == 0 then
            self:log("📝 Planning subtasks for: " .. task.title)
            local model = LLM.get_next_model()
            local plan_prompt = string.format([[
MISSION: Break this task into small, verifiable subtasks for FKernel.
TASK: %s
DESCRIPTION: %s
Format: JSON only. {"subtasks": [{"title": "...", "description": "..."}]}
]], task.title, task.description or "")
            
            -- This is a simplified call, we assume the LLM agent handles the file creation/parsing
            -- In a real scenario, we'd use a specific tool to parse this.
            -- For now, we'll delegate to opencode to update the state.json directly or via a temp file.
            local plan_cmd = string.format('opencode run "Break task %s into subtasks and update the subtasks array in %s. Return ONLY the updated JSON for the task object or the whole file." -m %s -f %s --log-level WARN', task.id, STATE_PATH, model, STATE_PATH)
            os.execute(plan_cmd)
            self:load_state()
            -- Re-fetch task after state update
            for _, t in ipairs(self.state.tasks) do if t.id == task.id then task = t; break end end
        end

        -- Phase 2: Execution of Subtasks
        local all_subtasks_ok = true
        local models_to_try = {
            "opencode/trinity-large-preview-free",
            "opencode/kimi-k2.5-free",
            "opencode/minimax-m2.1-free",
            "opencode/glm-4.7-free"
        }

        for i, sub in ipairs(task.subtasks or {}) do
            if sub.status ~= "completed" then
                local sub_success = false
                
                for _, model in ipairs(models_to_try) do
                    self.state.current_model = model
                    self.state.current_subtask = sub.title
                    self:save_state()
                    LLM.wait_if_needed(model)

                    self:log(string.format("🔧 Subtask %d/%d: %s (Trying: %s)", i, #task.subtasks, sub.title, model))
                    
                    local prompt = string.format([[
MISSION: Implement FKernel SUBTASK: %s
Context: Part of task "%s"
Description: %s
CONSTRAINTS (STRICT ENFORCEMENT):
- 🔑 SECRET RULE: ONE STRUCT/CLASS PER FILE.
- Naming: Directories in PascalCase, Files in snake_case (mandatory).
- Deep development: use domain-based directory structure.
- Follow ALL instructions in AGENTS.md and GEMINI.md.
- Object Calisthenics (No else, wrap primitives, one dot per line).
- Entity size: classes ≤200 lines, methods ≤20 lines.
- No getters/setters: use rich domain model methods.
- Return Result<T, Error> for all fallible operations.
]], sub.title, task.title, sub.description or "")

                    LLM.call(model, prompt)
                    
                    self:log("Validating subtask: " .. sub.title)
                    local build_ok = RunCommand("xmake -bv 2>&1")
                    local test_ok = build_ok and RunCommand("xmake run Test 2>&1")

                    if build_ok and test_ok then
                        sub.status = "completed"
                        Git.commit(string.format("ralph - feat(%s): %s", task.id, sub.title))
                        self:log("✅ Subtask SUCCESS with " .. model, "INFO")
                        sub_success = true
                        break
                    else
                        self:log("❌ Subtask FAILED with " .. model .. ". Swapping model...", "WARN")
                        os.execute("git checkout .") 
                        os.execute("git clean -fd")
                    end
                end

                if not sub_success then
                    all_subtasks_ok = false
                    sub.status = "failed"
                    self:log("🛑 Subtask could not be completed with any available model.", "ERROR")
                    break
                end
                self:save_state()
            end
        end

        if all_subtasks_ok and #task.subtasks > 0 then
            task.status = "completed"
            self:log("🎉 Task FULLY COMPLETED: " .. task.title, "INFO")
            
            -- Final task cleanup and commit (without ralph- prefix)
            if task.start_hash then
                self:log("📦 Finalizing task with stash and clean commit...")
                os.execute("git reset --soft " .. task.start_hash)
                os.execute("git stash")
                os.execute("git stash pop")
                Git.commit(string.format("feat(%s): %s", task.id, task.title))
            end
        else
            if (task.attempts or 0) >= 3 then
                task.status = "blocked"
                self:log("🛑 Task BLOCKED after retries", "ERROR")
            else
                task.status = "pending"
                -- Reset subtasks status for retry if needed or keep progress? 
                -- Usually keep progress if atomic.
            end
        end
        
        self.state.iteration = marathon_count
        self.state.current_subtask = nil
        self:save_state()
        
        local _, reason = os.execute("sleep 1")
        if reason == "signal" then break end
    end
    
    self.state.phase = "IDLE"
    self:save_state()
end

function Wiggum:analyze()
    self.state.phase = "ANALYZING"
    self:log("Delegating full analysis to opencode...", "INFO")
    local model = LLM.get_next_model()
    local prompt = string.format("Audit FKernel code and update TODO.md and %s summary.", STATE_PATH)
    LLM.call(model, prompt)
    self:load_state()
    self:log("Analysis and sync complete.")
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
    print("Ralph Wiggum v3.5 (Verified Syntax)")
    print("Phase: " .. Wiggum.state.phase)
    print("Tasks: " .. #Wiggum.state.tasks)
    for _, t in ipairs(Wiggum.state.tasks) do
        print(string.format("  [%s] %s", t.status, t.title))
    end
else
    print("Usage: lua Meta/wiggum.lua [import|build|analyze|status]")
end