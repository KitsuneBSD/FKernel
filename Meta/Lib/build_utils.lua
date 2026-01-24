local RunCommand = require("Meta.Lib.run_command")

local BuildUtils = {}

function BuildUtils.get_cpu_count()
    local f = io.popen("nproc 2>/dev/null || echo 4")
    local count = f:read("*a"):gsub("%s+", "")
    f:close()
    return tonumber(count) or 4
end

function BuildUtils.get_memory_gb()
    local f = io.popen("free -g | grep Mem: | awk '{print $2}'")
    local mem = f:read("*a"):gsub("%s+", "")
    f:close()
    return tonumber(mem) or 4
end

function BuildUtils.get_polite_cpu_count()
    local cpu_count = BuildUtils.get_cpu_count()
    local mem_gb = BuildUtils.get_memory_gb()
    
    -- LLVM/Clang compilation usually needs ~1.5GB to 2GB per job to avoid swap
    local safe_mem_jobs = math.floor(mem_gb / 1.5)
    if safe_mem_jobs < 1 then safe_mem_jobs = 1 end
    
    local polite_cpu = cpu_count - 2
    if polite_cpu < 1 then polite_cpu = 1 end
    
    -- Pick the minimum of both constraints
    local final_count = math.min(polite_cpu, safe_mem_jobs)
    
    return final_count
end

function BuildUtils.make(dir, targets, env_vars)
    local env = ""
    if env_vars then
        for k, v in pairs(env_vars) do
            env = env .. string.format("%s='%s' ", k, v)
        end
    end
    local target_str = targets and table.concat(targets, " ") or ""
    local cmd = string.format("cd %s && %s make -j%d %s", dir, env, BuildUtils.get_cpu_count(), target_str)
    return RunCommand(cmd)
end

function BuildUtils.ninja(dir, targets)
    local target_str = targets and table.concat(targets, " ") or ""
    local cmd = string.format("cd %s && ninja -j%d %s", dir, BuildUtils.get_cpu_count(), target_str)
    return RunCommand(cmd)
end

return BuildUtils
