-- Meta/wiggum-tools/reset-ralph.lua - Disaster Recovery for Ralph
local Git = require("Meta.Lib.git_utils")

print("🛑 Resetting Ralph Wiggum state...")
os.execute("git checkout .") -- Descarta mudanças locais
os.execute("git clean -fd")  -- Remove arquivos não rastreados
os.execute("rm -rf .ralph/*.json .ralph/logs/*.log")

local current = Git.get_current_branch()
if current:find("^wiggum/") then
    print("⚠️  Leaving wiggum branch: " .. current)
    -- Se precisar trocar para main, descomente a linha abaixo:
    -- os.execute("git checkout main")
end

print("✅ Clean slate. Run 'lua Meta/wiggum.lua import' to start over.")