set -eo pipefail

function clone_repository {
	local url=$1
	local repo_name
	repo_name=$(extract_repo_name "$1")

	if [ ! -d "$repo_name" ]; then
		git clone "$url" "$(dirname "$0")/$repo_name"
	else
		rm -rf "$(dirname "$0")/$repo_name"
		git clone "$url" "$(dirname "$0")/$repo_name"
	fi

	cd "$(dirname "$0")/$repo_name" || exit 1
}
