-- Meta/Lib/llm_client.lua

local LLM = {
    _failures = 0,
    _idx      = 0,
}

local MODELS = {
    "claude-sonnet-4-6",
    "claude-haiku-4-5-20251001",
    "claude-opus-4-8",
}

local MISSION_FILE      = "/tmp/wiggum/mission.txt"
local EXHAUSTION_LIMIT  = 5   -- consecutive full-model-cycle failures before stopping

-- Round-robin model selection.
function LLM.next()
    LLM._idx = (LLM._idx % #MODELS) + 1
    return MODELS[LLM._idx]
end

-- Write mission to file, ask claude to read and execute it.
-- Returns true on exit-0, false otherwise.
function LLM.call(model, mission)
    os.execute("mkdir -p /tmp/wiggum")
    local f = io.open(MISSION_FILE, "w")
    if not f then
        io.stderr:write("[LLM] Cannot write mission file\n")
        return false
    end
    f:write(mission)
    f:close()

    local cmd = string.format(
        "claude --model %s --dangerously-skip-permissions -p %q",
        model,
        "Read /tmp/wiggum/mission.txt then execute every step described there."
        .. " Use AGENTS.md as your coding style guide."
    )

    io.write(string.format("[LLM] %s ...\n", model))
    io.flush()
    local ok, why, code = os.execute(cmd)
    local success = (ok == true) or (ok == 0) or (why == "exit" and code == 0)

    if success then
        LLM._failures = 0
    else
        LLM._failures = LLM._failures + 1
    end

    return success
end

-- True when too many consecutive failures have been seen across all models.
function LLM.exhausted()
    return LLM._failures >= EXHAUSTION_LIMIT
end

-- Reset the failure counter (call after a cooldown period).
function LLM.reset()
    LLM._failures = 0
end

return LLM
