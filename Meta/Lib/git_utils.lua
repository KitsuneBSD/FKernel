-- Meta/Lib/git_utils.lua - Git operations for Ralph Wiggum

local GitUtils = {}

function GitUtils.get_current_branch()
    local f = io.popen("git branch --show-current 2>/dev/null")
    local branch = f:read("*a"):gsub("%s+", "")
    f:close()
    return branch ~= "" and branch or "main"
end

function GitUtils.get_wiggum_branch_name(base_branch)
    if base_branch:find("^wiggum/") then
        return base_branch
    end
    return "wiggum/" .. base_branch
end

function GitUtils.ensure_branch(name)
    -- Check if already on the target branch to avoid unnecessary checkout noise
    if GitUtils.get_current_branch() == name then
        return true
    end

    local check = io.popen("git rev-parse --verify " .. name .. " 2>/dev/null")
    local exists = check:read("*a") ~= ""
    check:close()
    
    if exists then
        -- Use -f to force if necessary, but carefully. 
        -- For now, just checkout and let the user see the output.
        return os.execute("git checkout " .. name .. " --quiet")
    else
        return os.execute("git checkout -b " .. name .. " --quiet")
    end
end

function GitUtils.commit(message)
    os.execute("git add -A")
    local cmd = 'git commit -m "' .. message .. '" --no-verify'
    return os.execute(cmd)
end

function GitUtils.get_last_hash()
    local f = io.popen("git rev-parse HEAD 2>/dev/null")
    local hash = f:read("*a"):gsub("%s+", "")
    f:close()
    return hash
end

return GitUtils