local RunCommand = require("Meta.Lib.run_command")
local OSInteract = require("Meta.Lib.os_interact")

local ArchiveUtils = {}

function ArchiveUtils.download(url, output)
    if OSInteract.FileExists(output) then return true end
    print("  [WGET] Downloading " .. url)
    return RunCommand(string.format("wget %s -O %s", url, output))
end

function ArchiveUtils.extract(archive, dest_dir)
    os.execute("mkdir -p " .. dest_dir)
    print("  [TAR]  Extracting " .. archive)
    return RunCommand(string.format("tar -xf %s -C %s", archive, dest_dir))
end

return ArchiveUtils
