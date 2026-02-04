-- Meta/Lib/llm_client.lua - LLM interaction and Rate Limiting for Wiggum

local LLMClient = {
    models = {
        ["opencode/minimax-m2.1-free"] = { rpm = 2, priority = 1 },
        ["opencode/trinity-large-preview-free"] = { rpm = 2, priority = 2 },
        ["opencode/kimi-k2.5-free"] = { rpm = 2, priority = 3 },
        ["opencode/glm-4.7-free"] = { rpm = 2, priority = 4 },
    },
    usage = {},
    last_model_index = 0
}

local MODEL_LIST = {
    "opencode/minimax-m2.1-free",
    "opencode/trinity-large-preview-free",
    "opencode/kimi-k2.5-free",
    "opencode/glm-4.7-free"
}

function LLMClient.get_next_model()
    LLMClient.last_model_index = (LLMClient.last_model_index % #MODEL_LIST) + 1
    return MODEL_LIST[LLMClient.last_model_index]
end

function LLMClient.call(model_id, prompt)
    local prompt_file = "/tmp/wiggum_prompt.txt"
    local f = io.open(prompt_file, "w")
    if f then
        f:write(prompt)
        f:close()
    end

    -- Mudança crucial: usando 'opencode run' para ativar o modo agente
    local cmd = 'opencode run --model "' .. model_id .. '" --file "' .. prompt_file .. '" 2>&1'
    local proc = io.popen(cmd)
    local output = proc:read("*a")
    proc:close()
    
    os.remove(prompt_file)
    return output
end

function LLMClient.wait_if_needed(model_id)
    local now = os.time()
    LLMClient.usage[model_id] = LLMClient.usage[model_id] or { count = 0, last_reset = now }
    
    if now - LLMClient.usage[model_id].last_reset >= 60 then
        LLMClient.usage[model_id].count = 0
        LLMClient.usage[model_id].last_reset = now
    end
    
    local config = LLMClient.models[model_id]
    if config and LLMClient.usage[model_id].count >= config.rpm then
        local wait_time = 65 - (now - LLMClient.usage[model_id].last_reset)
        print(string.format("⏳ Rate limit for %s. Waiting %ds...", model_id, wait_time))
        os.execute("sleep " .. wait_time)
        LLMClient.usage[model_id].count = 0
        LLMClient.usage[model_id].last_reset = os.time()
    end
    
    LLMClient.usage[model_id].count = LLMClient.usage[model_id].count + 1
end

return LLMClient
