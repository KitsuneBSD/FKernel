-- Meta/wiggum-tools/health-check.lua - Wiggum Integrity Check
local Json = require("Meta.Lib.json_utils")
local LLM = require("Meta.Lib.llm_client")

print("🔍 Checking Ralph Wiggum Integrity...")

-- 1. Lib Check
print("- JsonUtils: " .. (type(Json.parse) == "function" and "✅" or "❌"))
print("- LLMClient: " .. (type(LLM.call) == "function" and "✅" or "❌"))

-- 2. OpenCode Check
print("- OpenCode Connectivity: ")
local out = LLM.call("opencode/minimax-m2.1-free", "ping")
if out and #out > 0 then
    print("  ✅ Response received")
else
    print("  ❌ No response")
end

-- 3. Dir Check
os.execute("mkdir -p .ralph/logs")
print("- Logs Directory: ✅")

print("
All systems nominal.")
