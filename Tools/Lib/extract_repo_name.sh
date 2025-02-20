set -eo pipefail

function extract_repo_name {
	local url="$1"
	local repo_name="${url##*/}"
	echo "$repo_name"
}
