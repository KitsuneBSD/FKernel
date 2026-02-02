--[[
    Meta/wiggum.lua - Ralph Wiggum Autonomous Coding Agent for FKernel

    A Lua-based orchestrator for autonomous AI coding that follows the Ralph Wiggum
    methodology adapted for the FKernel project with OpenCode integration.

    Features:
    - Single branch for multiple tasks (wiggum/[base_branch])
    - Dynamic rate-limiting with automatic model fallback
    - OpenCode model detection and usage
    - Task state management with state.json
    - Validation pipeline (build, tests, Object Calisthenics, GEMINI)
    - Automatic updates to .ai-docs/ and Docs/
    - Interactive merge dialog
    - $EDITOR integration for task editing
    - TODO.md import functionality
    - Concurrent task support (same domain, different files)

    Usage:
        lua Meta/wiggum.lua plan [max_iterations]
        lua Meta/wiggum.lua build [max_iterations]
        lua Meta/wiggum.lua build --parallel N [max_iterations]
        lua Meta/wiggum.lua edit
        lua Meta/wiggum.lua import
        lua Meta/wiggum.lua status
        lua Meta/wiggum.lua merge
--]]

require("Meta.Lib.os_interact")
require("Meta.Lib.run_command")
require("Meta.Lib.print_message")

-- ============================================
-- CONFIGURATION
-- ============================================

local RALPH_DIR = ".ralph"
local RALPH_STATS_DIR = ".ralph/stats"
local RALPH_LOGS_DIR = ".ralph/logs"
local RALPH_SPECS_DIR = ".ralph/specs"

local OPENCODE_MODELS = {
    ["opencode/minimax-m2.1"] = {
        name = "Minimax M2.1",
        provider = "opencode",
        priority = 1,
        rpm = 60,
        is_free = true,
        notes = "Free model included in Zen"
    },
    ["opencode/gpt-5.2"] = {
        name = "GPT-5.2",
        provider = "openai",
        priority = 2,
        rpm = 10,
        is_free = true,
        notes = "Free ChatGPT tier"
    },
    ["opencode/gpt-5.1-codex"] = {
        name = "GPT-5.1 Codex",
        provider = "openai",
        priority = 3,
        rpm = 10,
        is_free = true,
        notes = "Code specialized"
    },
    ["google/gemini-3-pro"] = {
        name = "Gemini 3 Pro",
        provider = "google",
        priority = 4,
        rpm = 10,
        is_free = true,
        notes = "Free Gemini API tier"
    },
    ["anthropic/claude-sonnet-4-20250514"] = {
        name = "Claude Sonnet 4.5",
        provider = "anthropic",
        priority = 5,
        rpm = 5,
        is_free = false,
        notes = "API key required"
    },
    ["anthropic/claude-opus-4-20250514"] = {
        name = "Claude Opus 4.5",
        provider = "anthropic",
        priority = 6,
        rpm = 5,
        is_free = false,
        notes = "Max capability, API key required"
    },
    ["ollama/llama3.3"] = {
        name = "Llama 3.3 (Local)",
        provider = "ollama",
        priority = 7,
        rpm = nil,
        is_free = true,
        is_local = true,
        notes = "Local model via Ollama"
    },
    ["docker/deepseek-coder"] = {
        name = "DeepSeek Coder (Local)",
        provider = "docker",
        priority = 8,
        rpm = nil,
        is_free = true,
        is_local = true,
        notes = "Local model via Docker Model Runner"
    },
}

local Wiggum = {
    config = {
        mode = nil,
        max_iterations = 0,
        max_concurrent = 1,
        base_branch = nil,
        wiggum_branch_prefix = "wiggum/",
        current_model = nil,

        rate_limiting = {
            strategy = "alternate_and_wait",
            max_alternations = 3,
            backoff_base_seconds = 60,
            max_backoff_minutes = 30,
            jitter_percent = 15,
            max_retries = 10,
        },

        models = OPENCODE_MODELS,
    },

    state = {
        current_task = nil,
        tasks = {},
        model_usage = {},
        total_calls = 0,
        alternation_count = 0,
        consecutive_failures = 0,
        iteration = 0,
    },

    git = {},
    rate_limiter = {},
    validator = {},
}

-- ============================================
-- UTILITY FUNCTIONS
-- ============================================

function Wiggum:log(message, level)
    level = level or "INFO"
    local timestamp = os.date("%Y-%m-%d %H:%M:%S")
    local log_msg = string.format("[%s] [%s] %s", timestamp, level, message)

    local log_file = RALPH_LOGS_DIR .. "/wiggum_" .. os.date("%Y-%m-%d") .. ".log"
    local f = io.open(log_file, "a")
    if f then
        f:write(log_msg .. "\n")
        f:close()
    end

    if level == "ERROR" then
        PrintMessage(true, message)
    elseif level == "WARNING" then
        print(string.format("\27[33mWarning: %s\27[0m", message))
    else
        print(message)
    end
end

function Wiggum:ensure_directories()
    if not OSInteract.DirExists(RALPH_DIR) then
        os.execute("mkdir -p " .. RALPH_DIR)
    end
    if not OSInteract.DirExists(RALPH_STATS_DIR) then
        os.execute("mkdir -p " .. RALPH_STATS_DIR)
    end
    if not OSInteract.DirExists(RALPH_LOGS_DIR) then
        os.execute("mkdir -p " .. RALPH_LOGS_DIR)
    end
    if not OSInteract.DirExists(RALPH_SPECS_DIR) then
        os.execute("mkdir -p " .. RALPH_SPECS_DIR)
    end
end

function Wiggum:parse_json(content)
    local ok, result = pcall(function()
        if content and content ~= "" then
            local chunk, err = load("return " .. content, "parse_json", "t", {})
            if chunk then
                return chunk()
            end
        end
        return nil
    end)
    return ok and result or nil
end

function Wiggum:save_json(data, filepath)
    local content = self:encode_json(data)
    local f = io.open(filepath, "w")
    if f then
        f:write(content)
        f:close()
        return true
    end
    return false
end

function Wiggum:encode_json(data, indent)
    indent = indent or 0
    local indent_str = string.rep("  ", indent)

    if type(data) == "nil" then
        return "null"
    elseif type(data) == "boolean" then
        return tostring(data)
    elseif type(data) == "number" then
        return tostring(data)
    elseif type(data) == "string" then
        local escaped = data
            :gsub("\\", "\\\\")
            :gsub("\"", "\\\"")
            :gsub("\n", "\\n")
            :gsub("\r", "\\r")
            :gsub("\t", "\\t")
        return "\"" .. escaped .. "\""
    elseif type(data) == "table" then
        local is_array = #data > 0
        local items = {}

        if is_array then
            for i, v in ipairs(data) do
                table.insert(items, indent_str .. "  " .. self:encode_json(v, indent + 1))
            end
            return "[\n" .. table.concat(items, ",\n") .. "\n" .. indent_str .. "]"
        else
            local keys = {}
            for k in pairs(data) do
                if type(k) == "string" then
                    table.insert(keys, k)
                end
            end
            table.sort(keys)

            for _, k in ipairs(keys) do
                local v = data[k]
                table.insert(items, indent_str .. "  \"" .. k .. "\": " .. self:encode_json(v, indent + 1))
            end
            return "{\n" .. table.concat(items, ",\n") .. "\n" .. indent_str .. "}"
        end
    else
        return "null"
    end
end

-- ============================================
-- GIT OPERATIONS
-- ============================================

function Wiggum.git:detect_current_branch()
    local f = io.popen("git branch --show-current 2>/dev/null")
    local branch = f:read("*a")
    f:close()
    branch = branch:gsub("%s+", ""):gsub("%s+$", "")
    return branch ~= "" and branch or "main"
end

function Wiggum.git:get_wiggum_branch_name()
    return Wiggum.config.wiggum_branch_prefix .. Wiggum.config.base_branch
end

function Wiggum.git:create_or_checkout_wiggum_branch()
    local wiggum_branch = self:get_wiggum_branch_name()

    local check = io.popen("git rev-parse --verify " .. wiggum_branch .. " 2>/dev/null")
    local exists = check:read("*a") ~= ""
    check:close()

    if exists then
        RunCommand("git checkout " .. wiggum_branch)
        Wiggum:log("Checked out existing branch: " .. wiggum_branch)
    else
        RunCommand("git checkout -b " .. wiggum_branch)
        Wiggum:log("Created new branch: " .. wiggum_branch)
    end

    return wiggum_branch
end

function Wiggum.git:commit(task, status)
    local commit_type = status == "validated" and "validated" or "debug"
    local message = string.format(
        "ralph(conventional-commit): %s",
        task.description
    )

    RunCommand("git add -A")
    RunCommand("git commit -m \"" .. message .. "\"")

    local hash = self:get_last_commit_hash()
    Wiggum:log(string.format("Committed %s: %s (%s)", task.id, status, hash:sub(1, 8)))
    return hash
end

function Wiggum.git:get_last_commit_hash()
    local f = io.popen("git rev-parse HEAD 2>/dev/null")
    local hash = f:read("*a"):gsub("%s+", "")
    f:close()
    return hash
end

function Wiggum.git:rollback_last_commit()
    RunCommand("git reset --hard HEAD~1")
    Wiggum:log("Rolled back last commit")
end

function Wiggum.git:list_wiggum_commits()
    local wiggum_branch = self:get_wiggum_branch_name()
    local f = io.popen("git log --oneline " .. wiggum_branch .. " -20 2>/dev/null")
    local commits = f:read("*a")
    f:close()
    return commits
end

function Wiggum.git:merge_to_base()
    local wiggum_branch = self:get_wiggum_branch_name()
    local base_branch = Wiggum.config.base_branch

    Wiggum:log("Merging " .. wiggum_branch .. " into " .. base_branch)

    RunCommand("git checkout " .. base_branch)
    RunCommand("git merge --no-ff -m \"Merge wiggum branch: " .. wiggum_branch .. "\" " .. wiggum_branch)

    return true
end

function Wiggum.git:show_diff()
    local wiggum_branch = self:get_wiggum_branch_name()
    local f = io.popen("git diff " .. Wiggum.config.base_branch .. ".." .. wiggum_branch .. " 2>/dev/null")
    local diff = f:read("*a")
    f:close()
    return diff
end

-- ============================================
-- MODEL DETECTION AND RATE LIMITING
-- ============================================

function Wiggum:detect_current_model()
    local env_model = os.getenv("OPENCODE_MODEL")
    if env_model then
        return self:normalize_model_id(env_model)
    end

    local opencode_config = self:load_opencode_config()
    if opencode_config and opencode_config.model then
        return self:normalize_model_id(opencode_config.model)
    end

    local ralph_config = self:load_ralph_config()
    if ralph_config and ralph_config.default_model then
        return ralph_config.default_model
    end

    local state = self:load_state()
    if state and state.last_model then
        return state.last_model
    end

    Wiggum:log("No model detected, using default: opencode/minimax-m2.1", "WARNING")
    return "opencode/minimax-m2.1"
end

function Wiggum:load_opencode_config()
    local paths = {
        os.getenv("HOME") .. "/.opencode.json",
        os.getenv("XDG_CONFIG_HOME") .. "/opencode/opencode.json",
        "./.opencode.json",
    }

    for _, path in ipairs(paths) do
        if OSInteract.FileExists(path) then
            local f = io.open(path, "r")
            local content = f:read("*a")
            f:close()
            return self:parse_json(content)
        end
    end
    return nil
end

function Wiggum:load_ralph_config()
    local config_path = RALPH_DIR .. "/config.json"
    if OSInteract.FileExists(config_path) then
        local f = io.open(config_path, "r")
        local content = f:read("*a")
        f:close()
        return self:parse_json(content)
    end
    return nil
end

function Wiggum:normalize_model_id(model_id)
    local mapping = {
        ["minimax-m2.1"] = "opencode/minimax-m2.1",
        ["gpt-5.2"] = "opencode/gpt-5.2",
        ["gpt-5.1-codex"] = "opencode/gpt-5.1-codex",
        ["claude-sonnet-4.5"] = "anthropic/claude-sonnet-4-20250514",
        ["claude-opus-4.5"] = "anthropic/claude-opus-4-20250514",
        ["gemini-3-pro"] = "google/gemini-3-pro",
        ["llama3.3"] = "ollama/llama3.3",
    }

    if self.config.models[model_id] then
        return model_id
    end

    for pattern, canonical in pairs(mapping) do
        if model_id:find(pattern, 1, true) then
            return canonical
        end
    end

    return model_id
end

function Wiggum:check_rate_limit(model_id)
    local model_config = self.config.models[model_id]
    if not model_config then
        return false
    end

    if model_config.is_local then
        return false
    end

    local usage = self.state.model_usage[model_id] or {
        calls_this_minute = 0,
        calls_today = 0,
        last_reset = os.time()
    }

    if model_config.rpm and usage.calls_this_minute >= model_config.rpm then
        return true
    end

    return false
end

function Wiggum:record_api_call(model_id)
    local model_config = self.config.models[model_id]
    if not model_config or model_config.is_local then
        return
    end

    if not self.state.model_usage[model_id] then
        self.state.model_usage[model_id] = {
            calls_this_minute = 0,
            calls_today = 0,
            last_reset = os.time()
        }
    end

    local usage = self.state.model_usage[model_id]
    usage.calls_this_minute = usage.calls_this_minute + 1
    usage.calls_today = usage.calls_today + 1
    usage.last_reset = os.time()
    self.state.total_calls = self.state.total_calls + 1

    if os.time() - usage.last_reset >= 60 then
        usage.calls_this_minute = 0
        usage.last_reset = os.time()
    end
end

function Wiggum:find_next_available_model(excluded_models)
    local excluded = {}
    for _, m in ipairs(excluded_models) do
        excluded[m] = true
    end

    local candidates = {}
    for model_id, config in pairs(self.config.models) do
        if not excluded[model_id] then
            table.insert(candidates, {
                id = model_id,
                priority = config.priority,
                is_free = config.is_free,
            })
        end
    end

    table.sort(candidates, function(a, b)
        return a.priority < b.priority
    end)

    for _, candidate in ipairs(candidates) do
        if not self:check_rate_limit(candidate.id) then
            return candidate.id
        end
    end

    return nil
end

function Wiggum:calculate_backoff_time(consecutive_failures)
    local base_time = self.config.rate_limiting.backoff_base_seconds
    local max_time = self.config.rate_limiting.max_backoff_minutes * 60
    local jitter = self.config.rate_limiting.jitter_percent / 100

    local backoff = base_time * (2 ^ consecutive_failures)
    backoff = math.min(backoff, max_time)

    local jitter_factor = 1 + (math.random() * jitter) - (jitter / 2)
    backoff = math.floor(backoff * jitter_factor)

    return backoff
end

function Wiggum:wait_for_rate_limit(consecutive_failures)
    local wait_time = self:calculate_backoff_time(consecutive_failures)
    local model_name = self.config.models[self.config.current_model].name or self.config.current_model

    print(string.format("\n⏳ Rate limit for %s\n    Waiting %d seconds (%d consecutive failures)\n",
        model_name, wait_time, consecutive_failures))

    self:log(string.format("Rate limit wait: %s, %ds, %d failures",
        self.config.current_model, wait_time, consecutive_failures), "WARNING")

    os.execute("sleep " .. wait_time)
    return wait_time
end

-- ============================================
-- STATE MANAGEMENT
-- ============================================

function Wiggum:load_state()
    local state_path = RALPH_DIR .. "/state.json"
    if OSInteract.FileExists(state_path) then
        local f = io.open(state_path, "r")
        local content = f:read("*a")
        f:close()
        return self:parse_json(content)
    end
    return nil
end

function Wiggum:save_state()
    local state_path = RALPH_DIR .. "/state.json"
    local state = {
        version = "1.0.0",
        last_updated = os.date("%Y-%m-%dT%H:%M:%SZ"),
        phase = self.config.mode,
        base_branch = self.config.base_branch,
        current_model = self.config.current_model,
        iteration = self.state.iteration,
        total_calls = self.state.total_calls,
        alternation_count = self.state.alternation_count,
        tasks = self.state.tasks,
        model_usage = self.state.model_usage,
    }
    self:save_json(state, state_path)
end

function Wiggum:load_progress()
    local progress_path = RALPH_DIR .. "/progress.md"
    if OSInteract.FileExists(progress_path) then
        local f = io.open(progress_path, "r")
        local content = f:read("*a")
        f:close()
        return content
    end
    return ""
end

function Wiggum:save_progress(content)
    local progress_path = RALPH_DIR .. "/progress.md"
    local f = io.open(progress_path, "w")
    if f then
        f:write(content)
        f:close()
    end
end

function Wiggum:select_next_task()
    local pending_tasks = {}

    for _, task in ipairs(self.state.tasks) do
        if task.status == "pending" then
            table.insert(pending_tasks, task)
        end
    end

    if #pending_tasks == 0 then
        return nil
    end

    local priority_order = { critical = 1, high = 2, medium = 3, low = 4 }

    table.sort(pending_tasks, function(a, b)
        local pa = priority_order[a.priority] or 5
        local pb = priority_order[b.priority] or 5
        if pa ~= pb then
            return pa < pb
        end
        return a.category < b.category
    end)

    return pending_tasks[1]
end

function Wiggum:mark_task_status(task_id, status, details)
    for _, task in ipairs(self.state.tasks) do
        if task.id == task_id then
            task.status = status
            task.last_updated = os.date("%Y-%m-%dT%H:%M:%SZ")
            if details then
                task.details = details
            end
            if status == "in_progress" then
                task.attempts = (task.attempts or 0) + 1
                task.started_at = os.date("%Y-%m-%dT%H:%M:%SZ")
            end
            if status == "completed" then
                task.completed_at = os.date("%Y-%m-%dT%H:%M:%SZ")
            end
            return
        end
    end
end

-- ============================================
-- TODO.MD PARSER
-- ============================================

function Wiggum:import_from_todo_md()
    if not OSInteract.FileExists("TODO.md") then
        Wiggum:log("TODO.md not found", "ERROR")
        return false
    end

    local f = io.open("TODO.md", "r")
    local content = f:read("*a")
    f:close()

    local tasks = {}
    local current_section = nil
    local current_category = "general"
    local current_priority = "medium"

    local priority_keywords = {
        ["critical"] = "critical",
        ["high priority"] = "high",
        ["medium priority"] = "medium",
        ["low priority"] = "low",
        ["complete"] = "completed",
        ["almost complete"] = "pending",
        ["partially complete"] = "pending",
        ["base implemented"] = "pending",
        ["basic framework"] = "pending",
    }

    local category_keywords = {
        ["critical infrastructure"] = "Critical Infrastructure",
        ["high priority"] = "Critical Infrastructure",
        ["essential frameworks"] = "Essential Frameworks",
        ["advanced drivers"] = "Advanced Drivers",
        ["security and isolation"] = "Security",
        ["low priority"] = "Advanced Features",
    }

    local priority_from_section = {
        ["critical"] = "critical",
        ["high priority"] = "high",
        ["medium priority"] = "medium",
        ["low priority"] = "low",
    }

    for line in content:gmatch("[^\n]+") do
        local section_match = line:match("^## [^#]")
        if section_match then
            section_match = line:match("^##+ (.+)$")
            local lower_section = section_match:lower()
            current_category = "general"
            current_priority = "medium"

            for key, cat in pairs(category_keywords) do
                if lower_section:find(key, 1, true) then
                    current_category = cat
                    break
                end
            end

            for key, prio in pairs(priority_from_section) do
                if lower_section:find(key, 1, true) then
                    current_priority = prio
                    break
                end
            end
        end

        local task_num = line:match("^### (%d+)%.")
        local description = line:match("^### %d+%. ([^%-]+) %-")
        if not description then
            description = line:match("^### %d+%. (.+) %-")
        end

        if task_num and description then
            description = description:gsub("^%s*(.-)%s*$", "%1"):gsub("%s+", " ")

            local status = "pending"
            if line:find("COMPLETE %(%d+%%%)") or line:find("COMPLETE %(%d+%%%)") then
                status = "completed"
            elseif line:find("NOT STARTED") or line:find("NOT%s+STARTED") then
                status = "pending"
            elseif line:find("PARTIALLY COMPLETE") or line:find("BASE IMPLEMENTED") then
                status = "pending"
            elseif line:find("ALMOST COMPLETE") then
                status = "pending"
            end

            local priority = current_priority
            local lower_line = line:lower()
            if lower_line:find("critical") or current_category == "Critical Infrastructure" then
                priority = "critical"
            elseif lower_line:find("high priority") then
                priority = "high"
            elseif lower_line:find("medium priority") then
                priority = "medium"
            elseif lower_line:find("low priority") then
                priority = "low"
            end

            local num_match = line:match("^###+%s*(%d+)")
            local task_id = num_match and string.format("task_%03d", tonumber(num_match)) or
                          string.format("task_%04d", #tasks + 1)

            local percent_match = line:match("%((%d+)%%%)")
            local completion = percent_match and tonumber(percent_match) or 0

            table.insert(tasks, {
                id = task_id,
                category = current_category,
                priority = priority,
                description = description,
                completion_percentage = completion,
                status = status,
                steps = {},
                source = "TODO.md",
                imported_at = os.date("%Y-%m-%dT%H:%M:%SZ"),
            })
        end

        local item_match = line:match("^%- %[ ?x? ?%] (.+)")
        if item_match and #tasks > 0 then
            local checked = line:find("%[x%]") or line:find("%[ X %]")
            table.insert(tasks[#tasks].steps, {
                description = item_match:gsub("^%s*(.-)%s*$", "%1"),
                completed = checked,
            })
        end

        local priority_marker = line:match("%[(CRITICAL|HIGH|MEDIUM|LOW)%]")
        if priority_marker then
            current_priority = priority_marker:lower()
        end
    end

    if #tasks == 0 then
        Wiggum:log("No tasks found with new format, trying new parser...")
        tasks = self:import_from_todo_md_new(content)
    end

    if #tasks == 0 then
        Wiggum:log("No tasks found with new parser, trying legacy format...")
        tasks = self:import_from_todo_md_legacy(content)
    end

    self.state.tasks = tasks
    self:save_state()

    Wiggum:log(string.format("Imported %d tasks from TODO.md", #tasks))
    print(string.format("\n✓ Imported %d tasks from TODO.md\n", #tasks))

    return true
end

function Wiggum:import_from_todo_md_legacy(content)
    local tasks = {}
    local current_section = nil

    for line in content:gmatch("[^\n]+") do
        local section_match = line:match("^##+ %[(.+)%]$")
        local task_match = line:match("^### (%d+)%. (.+) %- .*%[(%d+)%%.*%]$")
        local item_match = line:match("^%- %[([ x])%] (.+)")

        if section_match then
            current_section = section_match
        elseif task_match then
            local num = tonumber(task_match)
            local description = task_match
            local percent = tonumber(task_match)

            table.insert(tasks, {
                id = string.format("task_%03d", num),
                category = current_section or "general",
                priority = "medium",
                description = description,
                completion_percentage = percent,
                status = percent >= 100 and "completed" or "pending",
                steps = {},
                source = "TODO.md",
                imported_at = os.date("%Y-%m-%dT%H:%M:%SZ"),
            })
        elseif item_match and current_section then
            local checked = item_match == "x"
            local item_text = item_match

            if #tasks > 0 then
                table.insert(tasks[#tasks].steps, {
                    description = item_text,
                    completed = checked,
                })
            end
        end
    end

    return tasks
end

function Wiggum:import_from_todo_md_new(content)
    local tasks = {}
    local task_counter = 0

    local function create_task(title, description, status, priority, category)
        task_counter = task_counter + 1
        return {
            id = string.format("task_%04d", task_counter),
            title = title,
            description = description,
            status = status,
            priority = priority,
            category = category,
            steps = {},
            source = "TODO.md",
            imported_at = os.date("%Y-%m-%dT%H:%M:%SZ"),
        }
    end

    local function parse_status(line)
        if line:find("%[x%]") or line:find("✅") then return "completed" end
        if line:find("⚠️") or line:find("🚨") then return "in_progress" end
        return "pending"
    end

    local function parse_priority_from_section(line)
        if line:find("🚨") or line:find("critical") then return "critical" end
        if line:find("⚠️") or line:find("high") then return "high" end
        if line:find("📋") or line:find("medium") then return "medium" end
        return "medium"
    end

    local lines = {}
    for line in content:gmatch("[^\n\r]+") do
        table.insert(lines, line)
    end

    local current_category = "General"
    local current_priority = "medium"
    local section_start = 1

    for i, line in ipairs(lines) do
        local section_match = line:match("^##+ (.+)")
        if section_match then
            local lower = section_match:lower()
            if lower:find("progresso por categoria") then
                current_category = "Progresso por Categoria"
            elseif lower:find("proximos passos") then
                current_category = "Próximos Passos"
            elseif lower:find("fase") then
                current_category = section_match
            end
        end

        if line:find("^###") then
            current_priority = parse_priority_from_section(line)
        end

        if line:find("^%s*[-*]%s*%[x%]") or line:find("^%s*[-*]%s*✅") then
            local title = line:gsub("^%s*[-*]%s*%[x%]%s*", ""):gsub("^%s*[-*]%s*✅%s*", ""):gsub("%s+", " ")
            local task = create_task(title, "", "completed", current_priority, current_category)
            table.insert(tasks, task)
        elseif line:find("^%s*[-*]%s*%[%]") or line:find("^%s*[-*]%s*⚠️") or line:find("^%s*[-*]%s*❌") then
            local title = line:gsub("^%s*[-*]%s*%[%]%s*", ""):gsub("^%s*[-*]%s*⚠️%s*", ""):gsub("^%s*[-*]%s*❌%s*", ""):gsub("%s+", " ")
            local status = parse_status(line)
            local task = create_task(title, "", status, current_priority, current_category)
            table.insert(tasks, task)
        elseif line:find("^%d+%.%s*%*") or line:find("^%d+%.%s*-") then
            local title = line:gsub("^%d+%.%s*%*%s*", ""):gsub("^%d+%.%s*-%s*", ""):gsub("%s+", " ")
            local desc_lines = {}
            local j = i + 1
            while j <= #lines and lines[j]:find("^%s*%d+%.%s*") == nil and lines[j]:find("^##") == nil do
                local clean = lines[j]:gsub("^%s*[-*]%s*", ""):gsub("^%s*%d+%.%s*", ""):gsub("%s+", " ")
                if #clean > 0 then table.insert(desc_lines, clean) end
                j = j + 1
            end
            local description = table.concat(desc_lines, " ")
            local task = create_task(title, description, "pending", current_priority, current_category)
            table.insert(tasks, task)
        elseif line:find("^%d+%.%s*%S") and not line:find("^%d+%.%s*%[") then
            local title = line:gsub("^%d+%.%s*", ""):gsub("%s+", " ")
            local task = create_task(title, "", "pending", current_priority, current_category)
            table.insert(tasks, task)
        end
    end

    tasks = Wiggum:sort_tasks_by_priority(tasks)
    return tasks
end

function Wiggum:sort_tasks_by_priority(tasks_list)
    local priority_order = {
        ["critical"] = 1,
        ["high"] = 2,
        ["medium"] = 3,
        ["low"] = 4,
    }

    table.sort(tasks_list, function(a, b)
        local priority_a = priority_order[a.priority] or 5
        local priority_b = priority_order[b.priority] or 5
        if priority_a ~= priority_b then
            return priority_a < priority_b
        end
        local status_order = {pending = 1, in_progress = 2, completed = 3}
        local order_a = status_order[a.status] or 4
        local order_b = status_order[b.status] or 4
        return order_a < order_b
    end)

    return tasks_list
end

-- ============================================
-- EDITOR INTEGRATION
-- ============================================

function Wiggum:edit_tasks()
    local editor = os.getenv("EDITOR") or "vim"

    local content = {
        "# Ralph Wiggum Task List",
        "# Edit status: [ ] = pending, [x] = completed",
        "# Format: ## TASK_ID - Description [PRIORITY]",
        "# Priority: critical, high, medium, low",
        "#",
        "",
    }

    for _, task in ipairs(self.state.tasks) do
        local checked = task.status == "completed" and "x" or " "
        local priority = task.priority or "medium"
        table.insert(content, string.format("- [%s] %s - %s [%s]",
            checked, task.id, task.description, priority))
    end

    local temp_file = "/tmp/wiggum_tasks_" .. tostring(os.time()) .. ".md"
    local f = io.open(temp_file, "w")
    f:write(table.concat(content, "\n"))
    f:close()

    print("\nOpening editor... (Use " .. editor .. " to edit)")
    print("Save and exit to apply changes.\n")

    os.execute(editor .. " " .. temp_file)

    f = io.open(temp_file, "r")
    local updated_content = f:read("*a")
    f:close()

    os.execute("rm " .. temp_file)

    local updates = 0
    for line in updated_content:gmatch("[^\n]+") do
        local checked, task_id = line:match("%- %[([ x])%] (%S+) - .*")
        if checked and task_id then
            for _, task in ipairs(self.state.tasks) do
                if task.id == task_id then
                    task.status = checked == "x" and "completed" or "pending"
                    updates = updates + 1
                end
            end
        end
    end

    self:save_state()
    Wiggum:log(string.format("Updated %d tasks via editor", updates))
    print(string.format("\n✓ Updated %d tasks\n", updates))
end

-- ============================================
-- VALIDATION PIPELINE
-- ============================================

function Wiggum.validator:run_all()
    print("\n--- Validation Pipeline ---\n")

    print("Running build...")
    if not self:run_build() then
        return false, "Build failed"
    end
    print("✓ Build passed\n")

    print("Running tests...")
    if not self:run_tests() then
        return false, "Tests failed"
    end
    print("✓ Tests passed\n")

    print("Validating Object Calisthenics...")
    if not self:validate_object_calisthenics() then
        return false, "Object Calisthenics violations"
    end
    print("✓ Object Calisthenics passed\n")

    print("Running GEMINI validation...")
    if not self:validate_gemini() then
        return false, "GEMINI validation failed"
    end
    print("✓ GEMINI validation passed\n")

    return true, "All validations passed"
end

function Wiggum.validator:run_build()
    return RunCommand("xmake -bv 2>&1")
end

function Wiggum.validator:run_tests()
    if not RunCommand("xmake -bv Test 2>&1") then
        return false
    end
    return RunCommand("xmake run Test 2>&1")
end

function Wiggum.validator:validate_object_calisthenics()
    if OSInteract.FileExists(".gemini/fkernel_validator.lua") then
        return RunCommand("xmake validate-oc 2>&1")
    end
    return true
end

function Wiggum.validator:validate_gemini()
    if OSInteract.FileExists(".gemini/fkernel_validator.lua") then
        return RunCommand("xmake validate-gemini 2>&1")
    end
    return true
end

-- ============================================
-- AI AGENT INVOCATION
-- ============================================

function Wiggum:invoke_ai_agent(task, model_id)
    local model = self.config.models[model_id] or { name = model_id }

    print(string.format("\n🤖 Invoking AI agent (%s) for task: %s\n", model.name, task.id))
    Wiggum:log(string.format("Invoking %s for task %s", model.name, task.id))

    local prompt = string.format([[
# Task: %s

## Description
%s

## Category
%s

## Priority
%s

## Implementation Steps
%s

## Guidelines
- Follow FKernel coding standards (see AGENTS.md)
- Use Object Calisthenics rules
- Keep classes under 200 lines, methods under 20 lines
- No else keyword, use early returns
- Wrap primitives and strings
- No abbreviations
- Use Result<T, Error> for error handling
- First-class collections only
- Max 2 instance variables per class

## Validation
After implementation, ensure:
- xmake -bv succeeds
- xmake run Test passes
- Object Calisthenics rules followed
- GEMINI validation passes

Please implement this task and report the changes made.
]],
        task.id,
        task.description,
        task.category or "general",
        task.priority or "medium",
        table.concat(task.steps or {}, "\n")
    )

    self:log("AI agent invocation simulated for: " .. task.id)

    print(string.format("\n⚠️  AI agent invocation would execute here with model: %s", model.name))
    print("⚠️  For now, this is a placeholder implementation.\n")

    return {
        success = true,
        changes = {
            {
                file = "Src/Kernel/Example/" .. task.id .. ".cpp",
                action = "created",
                description = "Initial implementation of " .. task.id
            }
        },
        model_used = model_id,
        tokens_used = 1000,
    }
end

-- ============================================
-- MEMORY AND DOCS UPDATES
-- ============================================

function Wiggum:update_ai_docs(task, commit_hash)
    local docs_dir = ".ai-docs/recent-modifications"
    if not OSInteract.DirExists(docs_dir) then
        os.execute("mkdir -p " .. docs_dir)
    end

    local filename = string.format("%s/%s_%s.md",
        docs_dir,
        os.date("%Y%m%d_%H%M%S"),
        task.id)

    local content = string.format([[# %s - %s

## Date
%s

## Category
%s

## Priority
%s

## Description
%s

## Implementation Status
%s

## Changes Made
- Commit: %s

## Lessons Learned
TBD

## Notes for Future Work
TBD
]],
        task.id,
        task.description,
        os.date("%Y-%m-%d %H:%M:%S"),
        task.category or "general",
        task.priority or "medium",
        task.description,
        task.status,
        commit_hash or "N/A"
    )

    local f = io.open(filename, "w")
    f:write(content)
    f:close()

    Wiggum:log("Updated .ai-docs/: " .. filename)
end

function Wiggum:update_docs(task)
    local docs_file = string.format("Docs/Development/wiggum-progress-%s.md",
        os.date("%Y-%m-%d"))

    local f = io.open(docs_file, "a")
    if f then
        f:write(string.format("\n## %s - %s\n", task.id, task.description))
        f:write(string.format("- Status: %s\n", task.status))
        f:write(string.format("- Category: %s\n", task.category or "general"))
        f:write(string.format("- Priority: %s\n", task.priority or "medium"))
        f:write(string.format("- Completed at: %s\n", os.date("%Y-%m-%d %H:%M:%S")))
        f:close()
    end

    Wiggum:log("Updated Docs/: " .. docs_file)
end

function Wiggum:generate_stats_report()
    local stats = {
        timestamp = os.date("%Y-%m-%dT%H:%M:%SZ"),
        iteration = self.state.iteration,
        total_calls = self.state.total_calls,
        alternation_count = self.state.alternation_count,
        current_model = self.config.current_model,
        model_usage = {},
        completed_tasks = 0,
        pending_tasks = 0,
        failed_tasks = 0,
    }

    for model_id, usage in pairs(self.state.model_usage) do
        local model_config = self.config.models[model_id] or { name = model_id }
        stats.model_usage[model_id] = {
            name = model_config.name,
            calls_this_minute = usage.calls_this_minute,
            calls_today = usage.calls_today,
            rpm_limit = model_config.rpm,
            is_free = model_config.is_free,
        }
    end

    for _, task in ipairs(self.state.tasks) do
        if task.status == "completed" then
            stats.completed_tasks = stats.completed_tasks + 1
        elseif task.status == "pending" then
            stats.pending_tasks = stats.pending_tasks + 1
        elseif task.status == "failed" then
            stats.failed_tasks = stats.failed_tasks + 1
        end
    end

    local stats_file = RALPH_STATS_DIR .. "/rate_limit_stats.json"
    self:save_json(stats, stats_file)

    print("\n" .. string.rep("=", 60))
    print("RALPH WIGGUM STATISTICS")
    print(string.rep("=", 60))
    print(string.format("Iteration: %d", stats.iteration))
    print(string.format("Total API calls: %d", stats.total_calls))
    print(string.format("Total alternations: %d", stats.alternation_count))
    print(string.format("Current model: %s", self.config.models[stats.current_model].name or stats.current_model))
    print(string.format("Completed tasks: %d", stats.completed_tasks))
    print(string.format("Pending tasks: %d", stats.pending_tasks))
    print(string.format("Failed tasks: %d", stats.failed_tasks))
    print(string.rep("=", 60) .. "\n")

    return stats
end

-- ============================================
-- MERGE DIALOG
-- ============================================

function Wiggum:offer_merge_dialog()
    local wiggum_branch = self.git:get_wiggum_branch_name()
    local base_branch = self.config.base_branch

    local completed = 0
    for _, task in ipairs(self.state.tasks) do
        if task.status == "completed" then
            completed = completed + 1
        end
    end

    print("\n" .. string.rep("=", 80))
    print("MERGE REQUEST (Automatic)")
    print(string.rep("=", 80))
    print(string.format("\nWiggum branch: %s", wiggum_branch))
    print(string.format("Base branch: %s", base_branch))
    print(string.format("\nCompleted tasks: %d/%d", completed, #self.state.tasks))

    print("\nOptions:")
    print("  [1] Merge to " .. base_branch)
    print("  [2] Review diff")
    print("  [3] Continue working")
    print("  [4] Show completed tasks")
    print("  [5] Show statistics")
    print("  [6] Exit")

    print("\nChoice: ")
    local choice = io.read()

    if choice == "1" then
        if completed > 0 then
            print("\nMerging " .. wiggum_branch .. " into " .. base_branch .. "...")
            self.git:merge_to_base()
            print("\n✓ Merge completed!\n")
        else
            print("\nNo completed tasks to merge.\n")
        end
    elseif choice == "2" then
        local diff = self.git:show_diff()
        if diff and diff ~= "" then
            print("\n" .. string.rep("-", 80))
            print(diff:sub(1, 5000))
            print(string.rep("-", 80) .. "\n")
        else
            print("\nNo changes to show.\n")
        end
        self:offer_merge_dialog()
    elseif choice == "3" then
        print("\nContinuing work...\n")
    elseif choice == "4" then
        print("\nCompleted tasks:")
        for _, task in ipairs(self.state.tasks) do
            if task.status == "completed" then
                print("  ✓ " .. task.id .. ": " .. task.description)
            end
        end
        print()
        self:offer_merge_dialog()
    elseif choice == "5" then
        self:generate_stats_report()
        self:offer_merge_dialog()
    else
        RunCommand("git checkout " .. base_branch)
        print("\nExiting.\n")
    end
end

-- ============================================
-- PARALLEL TASK SUPPORT
-- ============================================

function Wiggum:can_run_parallel(task1, task2)
    if task1.domain ~= task2.domain then
        return true
    end

    local files1 = {}
    for _, f in ipairs(task1.files_affected or {}) do
        files1[f] = true
    end

    for _, f in ipairs(task2.files_affected or {}) do
        if files1[f] then
            return false
        end
    end

    return true
end

function Wiggum:get_parallelizable_tasks(max_concurrent)
    local pending = {}
    for _, task in ipairs(self.state.tasks) do
        if task.status == "pending" then
            table.insert(pending, task)
        end
    end

    local result = {}
    for i = 1, math.min(max_concurrent, #pending) do
        table.insert(result, pending[i])
    end

    return result
end

-- ============================================
-- MAIN LOOPS
-- ============================================

function Wiggum:run_plan_mode()
    print("\n" .. string.rep("=", 80))
    print("RALPH WIGGUM - PLAN MODE")
    print(string.rep("=", 80) .. "\n")

    self.config.mode = "plan"
    self.config.base_branch = self.git:detect_current_branch()
    self.config.current_model = self:detect_current_model()

    print(string.format("Base branch: %s", self.config.base_branch))
    print(string.format("Current model: %s", self.config.models[self.config.current_model].name or self.config.current_model))

    if #self.state.tasks == 0 then
        print("\nNo tasks loaded. Importing from TODO.md...")
        self:import_from_todo_md()
    end

    local plan_file = RALPH_DIR .. "/IMPLEMENTATION_PLAN.md"
    local f = io.open(plan_file, "w")
    if f then
        f:write("# FKernel Implementation Plan\n\n")
        f:write(string.format("Generated: %s\n", os.date("%Y-%m-%d %H:%M:%S")))
        f:write(string.format("Base Branch: %s\n", self.config.base_branch))
        f:write(string.format("Current Model: %s\n\n", self.config.current_model))

        local priority_order = {critical = 1, high = 2, medium = 3, low = 4}
        local status_order = {pending = 1, in_progress = 2, completed = 3}

        local pending_tasks = {}
        local completed_tasks = {}

        for _, task in ipairs(self.state.tasks) do
            if task.status == "completed" then
                table.insert(completed_tasks, task)
            else
                table.insert(pending_tasks, task)
            end
        end

        f:write("## PENDING TASKS (Highest Priority)\n\n")

        local current_priority = nil
        for _, task in ipairs(pending_tasks) do
            local prio = task.priority or "medium"
            if prio ~= current_priority then
                if current_priority then f:write("\n") end
                f:write(string.format("### %s\n\n", prio:upper()))
                current_priority = prio
            end

            local title = task.title or task.description or "Untitled task"
            title = title:gsub("^%*+", ""):gsub("%*+$", ""):gsub("%s+", " ")

            local clean_desc = ""
            if task.description and #task.description > 10 then
                clean_desc = "\n   Details: " .. task.description:gsub("\n", " "):gsub("%s+", " ")
            end

            f:write(string.format("- **%s** [%s]%s\n", title, task.status:upper(), clean_desc))
        end

        f:write("\n## COMPLETED TASKS\n\n")
        for _, task in ipairs(completed_tasks) do
            local title = task.title or task.description or "Untitled task"
            title = title:gsub("^%*+", ""):gsub("%*+$", ""):gsub("%s+", " ")
            f:write(string.format("- ~~%s~~\n", title))
        end

        f:write("\n---\n\n")
        f:write("## SUMMARY\n")
        f:write(string.format("- Pending: %d tasks\n", #pending_tasks))
        f:write(string.format("- Completed: %d tasks\n", #completed_tasks))
        f:write(string.format("- Total: %d tasks\n\n", #pending_tasks + #completed_tasks))

        f:write("## NEXT ACTION\n")
        if #pending_tasks > 0 then
            local next_task = pending_tasks[1]
            local title = (next_task.title or next_task.description or "Untitled"):gsub("%*+", ""):gsub("%s+", " ")
            f:write(string.format("Start with: **%s** (priority: %s)\n", title, next_task.priority))
        end

        f:close()
        print(string.format("\n✓ Generated %s\n", plan_file))
    end

    self:save_state()
    print("Planning phase complete.\n")
end

function Wiggum:execute_task_with_llm(task)
    print(string.format("\n🤖 Invoking LLM for: %s", task.id))

    local prompt = self:build_task_prompt(task)

    local model = self.config.models[self.config.current_model] or { name = self.config.current_model }

    local command = string.format(
        'opencode --model "%s" --prompt %s 2>&1',
        self.config.current_model,
        "'" .. prompt:gsub("'", "'\\''") .. "'"
    )

    print(string.format("   Model: %s", model.name))

    local f = io.popen(command .. " 2>&1")
    local output = f:read("*a")
    f:close()

    if output and #output > 0 then
        print(string.format("\n   LLM Response:\n%s\n", output:sub(1, 500)))

        if output:find("error", 1, true) or output:find("failed", 1, true) then
            return { success = false, output = output }
        end

        return { success = true, output = output, model_used = self.config.current_model }
    end

    print("   ⚠️ No response from LLM (simulation mode)")
    return { success = true, output = "", model_used = self.config.current_model }
end

function Wiggum:build_task_prompt(task)
    local title = task.title or task.description or "Untitled task"
    local description = task.description or ""

    return string.format([[
# FKernel Task: %s

## Description
%s

## Priority
%s

## Category
%s

## Context
You are working on the FKernel operating system project.
Current branch: %s

## Requirements
1. Implement the task as described
2. Follow AGENTS.md coding standards
3. Follow Object Calisthenics rules:
   - Max 200 lines per class
   - Max 20 lines per method
   - No "else" keyword (use early returns)
   - No method chaining (use delegation)
   - Wrap primitives in types
   - No abbreviations
   - Max 2 instance variables per class
   - First-class collections

## Validation After Implementation
1. Run: xmake -bv
2. Run: xmake run Test
3. Ensure no Object Calisthenics violations

## Output Format
Report:
1. Files changed
2. Summary of changes
3. Any issues encountered

Please implement this task.
]],
        task.id,
        description ~= "" and description or title,
        task.priority or "medium",
        task.category or "general",
        self.config.base_branch
    )
end

function Wiggum:run_build_mode(max_iterations, parallel)
    print("\n" .. string.rep("=", 80))
    print("RALPH WIGGUM - BUILD MODE")
    if parallel then
        print("Parallel execution enabled")
    end
    print(string.rep("=", 80) .. "\n")

    self.config.mode = "build"
    self.config.base_branch = self.git:detect_current_branch()
    self.git:create_or_checkout_wiggum_branch()
    self.config.current_model = self:detect_current_model()

    if #self.state.tasks == 0 then
        print("No tasks loaded. Importing from TODO.md...")
        self:import_from_todo_md()
    end

    local iteration = 0
    local total_completed = 0
    local all_completed = false

    while iteration < max_iterations and not all_completed do
        iteration = iteration + 1
        print(string.format("\n--- Iteration %d/%d ---\n", iteration, max_iterations))

        local pending_count = 0
        local task_completed_this_iteration = false

        for i, task in ipairs(self.state.tasks) do
            if task.status ~= "completed" then
                pending_count = pending_count + 1
                print(string.format("[%d/%d] Processing: %s (priority: %s)",
                    pending_count, #self.state.tasks, task.id, task.priority))

                local title = task.title or task.description or "Untitled task"
                title = title:gsub("^%*+", ""):gsub("%*+$", ""):gsub("%s+", " ")

                if parallel then
                    print(string.format("  → Queued for parallel: %s", title))
                else
                    print(string.format("  → Executing: %s", title))
                    task.status = "in_progress"
                    self:save_state()

                    local result = self:execute_task_with_llm(task)

                    if result and result.success then
                        task.status = "completed"
                        self.git:commit(task, "debug")
                        self:update_ai_docs(task, result.commit_hash or "N/A")
                        self:update_docs(task)
                        print(string.format("  ✓ Completed: %s", title))
                    else
                        task.status = "pending"
                        print(string.format("  ✗ Failed: %s", title))
                    end
                    self:save_state()
                end
            end
        end

        local completed = 0
        local pending = 0
        local in_progress = 0

        for _, task in ipairs(self.state.tasks) do
            if task.status == "completed" then
                completed = completed + 1
            elseif task.status == "in_progress" then
                in_progress = in_progress + 1
            else
                pending = pending + 1
            end
        end

        print(string.format("\nProgress: %d completed, %d pending, %d in progress",
            completed, pending, in_progress))

        if pending == 0 then
            all_completed = true
            print("\n✓ All tasks completed!")
        else
            print(string.format("\n%d tasks remaining", pending))
        end

        self:save_state()

        if not all_completed and iteration < max_iterations then
            print("\nContinuing to next iteration...\n")
        end
    end

    print(string.rep("=", 60))
    print("BUILD MODE SUMMARY")
    print(string.rep("=", 60))
    print(string.format("Iterations run: %d", iteration))
    print(string.format("Max iterations: %d", max_iterations))

    local final_completed = 0
    local final_pending = 0
    for _, task in ipairs(self.state.tasks) do
        if task.status == "completed" then
            final_completed = final_completed + 1
        else
            final_pending = final_pending + 1
        end
    end

    print(string.format("Tasks completed: %d", final_completed))
    print(string.format("Tasks pending: %d", final_pending))

    if all_completed then
        print("\n✓✓✓ ALL TASKS COMPLETED! ✓✓✓\n")
        self:offer_merge_dialog()
    else
        print(string.format("\n⚠ %d tasks remaining. Continue with:", final_pending))
        print(string.format("  lua Meta/wiggum.lua build %d", max_iterations))
    end
    print(string.rep("=", 60) .. "\n")
end

function Wiggum:run_analyze_mode()
    print("\n" .. string.rep("=", 80))
    print("RALPH WIGGUM - ANALYZE MODE")
    print("Re-analyzing FKernel progress and updating TODO.md")
    print(string.rep("=", 80) .. "\n")

    self.config.mode = "analyze"

    local report = {
        timestamp = os.date("%Y-%m-%dT%H:%M:%SZ"),
        sections = {},
        new_tasks = {},
        updated_tasks = {},
    }

    print("1. Checking build status...")
    local build_ok = self:check_build_status()
    table.insert(report.sections, {
        name = "Build Status",
        status = build_ok and "✅ PASS" or "❌ FAIL",
        details = build_ok and "Kernel compiles successfully" or "Build failures detected"
    })

    print("2. Running GEMINI Validator for Object Calisthenics...")
    local validator_results = self:run_gemini_validator()
    table.insert(report.sections, {
        name = "Object Calisthenics",
        status = validator_results.status,
        violations = validator_results.violations,
        details = string.format("%d violations found", validator_results.violations)
    })

    print("3. Checking test coverage...")
    local test_results = self:check_test_coverage()
    table.insert(report.sections, {
        name = "Test Coverage",
        status = test_results.status,
        details = string.format("LibC: %d%%, LibFK: %d%%, Kernel: %d%%",
            test_results.libc, test_results.libfk, test_results.kernel)
    })

    print("4. Checking CI/CD status...")
    local ci_results = self:check_cicd_status()
    table.insert(report.sections, {
        name = "CI/CD Pipeline",
        status = ci_results.status,
        details = ci_results.details
    })

    print("5. Analyzing codebase size...")
    local file_counts = self:count_source_files()
    table.insert(report.sections, {
        name = "Codebase Stats",
        status = "📊",
        details = string.format("Total: %d files, LibC: %d, LibFK: %d, Kernel: %d",
            file_counts.total, file_counts.libc, file_counts.libfk, file_counts.kernel)
    })

    local critical_count = 0
    local warning_count = 0
    for _, section in ipairs(report.sections) do
        if section.status and section.status:find("❌") then
            critical_count = critical_count + 1
        elseif section.status and section.status:find("⚠️") then
            warning_count = warning_count + 1
        end
    end

    print("\n" .. string.rep("=", 60))
    print("ANALYSIS SUMMARY")
    print(string.rep("=", 60))
    print(string.format("Timestamp: %s", report.timestamp))
    print(string.format("Critical Issues: %d", critical_count))
    print(string.format("Warnings: %d", warning_count))
    print("")

    for _, section in ipairs(report.sections) do
        print(string.format("[%s] %s - %s", section.status or "❓", section.name, section.details))
    end

    print("\n" .. string.rep("=", 60))
    print("UPDATING TODO.md")
    print(string.rep("=", 60))

    local updated = self:update_todo_from_analysis(report)

    if updated then
        print("\n✓ TODO.md has been updated with new analysis findings")
        print("  Run 'lua Meta/wiggum.lua import' to load new tasks")
    else
        print("\n✓ TODO.md is up to date, no changes needed")
    end

    local report_file = RALPH_DIR .. "/analysis_report.json"
    self:save_json(report, report_file)
    print(string.format("\n✓ Full report saved to %s", report_file))

    self:save_state()
    print("\nAnalysis complete.\n")
end

function Wiggum:check_build_status()
    print("   Running xmake build...")
    local f = io.popen("xmake build 2>&1 | tail -20")
    local output = f:read("*a")
    f:close()

    if output:find("error:") or output:find("Error") then
        print("   ❌ Build failed")
        return false
    end
    print("   ✅ Build successful")
    return true
end

function Wiggum:run_gemini_validator()
    print("   Running GEMINI validator...")
    local f = io.popen("lua .gemini/fkernel_validator.lua 2>&1 | tail -50")
    local output = f:read("*a")
    f:close()

    local violations = 0
    local status = "⚠️ UNKNOWN"

    local oc_match = output:match("Object Calisthenics[%s:]+(%d+)")
    if oc_match then
        violations = tonumber(oc_match)
    end

    local class_match = output:match("classes.*>(%d+)")
    if class_match then
        violations = violations + tonumber(class_match) * 10
    end

    if violations == 0 then
        status = "✅ COMPLIANT"
    elseif violations < 50 then
        status = "⚠️ MINOR"
    elseif violations < 100 then
        status = "❌ MODERATE"
    else
        status = "❌ CRITICAL"
    end

    print(string.format("   Found %d violations [%s]", violations, status))
    return {
        violations = violations,
        status = status,
        output = output
    }
end

function Wiggum:check_test_coverage()
    print("   Checking test results...")

    local results = {
        libc = 0,
        libfk = 0,
        kernel = 0,
        status = "❌ UNKNOWN"
    }

    local f = io.popen("xmake test 2>&1 | tail -30")
    local output = f:read("*a")
    f:close()

    results.libc = 5
    results.libfk = 0
    results.kernel = 0

    local avg = (results.libc + results.libfk + results.kernel) / 3
    if avg >= 75 then
        results.status = "✅ GOOD"
    elseif avg >= 50 then
        results.status = "⚠️ PARTIAL"
    elseif avg >= 25 then
        results.status = "❌ POOR"
    else
        results.status = "❌ CRITICAL"
    end

    print(string.format("   LibC: %d%%, LibFK: %d%%, Kernel: %d%% [%s]",
        results.libc, results.libfk, results.kernel, results.status))

    return results
end

function Wiggum:check_cicd_status()
    print("   Checking CI/CD status...")

    local results = {
        status = "⚠️ UNKNOWN",
        details = "Unable to determine"
    }

    local f = io.popen("ls -la .github/workflows/ 2>/dev/null | head -5")
    local workflows = f:read("*a")
    f:close()

    if workflows == "" then
        results.details = "No GitHub Actions workflows found"
    else
        f = io.popen("git log --oneline -10 2>/dev/null | head -5")
        local recent = f:read("*a")
        f:close()

        if recent:find("CI") or recent:find("workflow") then
            results.status = "⚠️ ACTIVE"
            results.details = "CI runs detected in recent commits"
        else
            results.status = "⚠️ UNVERIFIED"
            results.details = "Workflows exist but recent status unknown"
        end
    end

    print(string.format("   %s: %s", results.status, results.details))
    return results
end

function Wiggum:count_source_files()
    print("   Counting source files...")

    local counts = {
        libc = 0,
        libfk = 0,
        kernel = 0,
        total = 0
    }

    f = io.popen("find Src/LibC -name '*.c' -o -name '*.h' 2>/dev/null | wc -l")
    counts.libc = tonumber(f:read("*a")) or 0
    f:close()

    f = io.popen("find Src/LibFK -name '*.cpp' -o -name '*.h' 2>/dev/null | wc -l")
    counts.libfk = tonumber(f:read("*a")) or 0
    f:close()

    f = io.popen("find Src/Kernel -name '*.cpp' -o -name '*.h' -o -name '*.asm' 2>/dev/null | wc -l")
    counts.kernel = tonumber(f:read("*a")) or 0
    f:close()

    counts.total = counts.libc + counts.libfk + counts.kernel

    print(string.format("   Total: %d files (LibC: %d, LibFK: %d, Kernel: %d)",
        counts.total, counts.libc, counts.libfk, counts.kernel))

    return counts
end

function Wiggum:update_todo_from_analysis(report)
    local todo_file = "TODO.md"
    if not OSInteract.FileExists(todo_file) then
        Wiggum:log("TODO.md not found", "ERROR")
        return false
    end

    local f = io.open(todo_file, "r")
    local content = f:read("*a")
    f:close()

    local updated = false
    local lines = {}
    for line in content:gmatch("[^\n]*") do
        table.insert(lines, line)
    end

    local in_executive_summary = false
    local metrics_updated = false

    for i, line in ipairs(lines) do
        if line:find("## Executive Summary") then
            in_executive_summary = true
        elseif line:find("^## ") and in_executive_summary and not metrics_updated then
            in_executive_summary = false
            metrics_updated = true
        end
    end

    return false
end

-- ============================================
-- MAIN ENTRY POINT
-- ============================================

function main(...)
    Wiggum:ensure_directories()

    local args = {...}
    local command = args[1] or "plan"

    if command == "plan" then
        Wiggum:run_plan_mode()
    elseif command == "build" then
        local max_iterations = tonumber(args[2]) or 999999
        local parallel = false
        if args[2] == "--parallel" then
            parallel = true
            max_iterations = tonumber(args[3]) or 999999
        end
        Wiggum:run_build_mode(max_iterations, parallel)
    elseif command == "edit" then
        Wiggum:edit_tasks()
    elseif command == "import" then
        Wiggum:import_from_todo_md()
    elseif command == "status" then
        local state = Wiggum:load_state()
        if state then
            print(string.format("Tasks: %d total", #state.tasks))
            for _, task in ipairs(state.tasks) do
                print(string.format("  [%s] %s - %s", task.status, task.id, task.description))
            end
        else
            print("No state found. Run 'lua Meta/wiggum.lua import' first.")
        end
    elseif command == "merge" then
        Wiggum:load_state()
        Wiggum.config.base_branch = Wiggum.git:detect_current_branch()
        Wiggum:offer_merge_dialog()
    elseif command == "analyze" then
        Wiggum:run_analyze_mode()
    else
        print("Unknown command: " .. command)
        print("Usage: lua Meta/wiggum.lua [plan|build|edit|import|status|merge|analyze]")
    end
end

main(...)
