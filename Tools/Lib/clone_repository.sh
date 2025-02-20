set -eo pipefail

function clone_repository {
	local url=$1
	local repo_name
	repo_name=$(extract_repo_name "$1")

	if [ ! -d "$repo_name" ]; then
		git clone "$url"
	else
		rm -rf "$repo_name"
		git clone "$url"
	fi

	cd "$repo_name" || exit 1
}
