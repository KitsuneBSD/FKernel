-- Meta/wiggum-tools/reset-ralph.lua - Disaster Recovery for Ralph
local Git = require("Meta.Lib.git_utils")

print("🛑 Resetting Ralph Wiggum state...")
os.execute("rm -rf .ralph/*.json .ralph/logs/*.log")
os.execute("git checkout main")
os.execute("git branch -D wiggum/feature/early-init") -- Exemplo, remove o branch de trabalho
print("✅ Clean slate. Run 'lua Meta/wiggum.lua import' to start over.")
