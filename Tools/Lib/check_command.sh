set -eo pipefail

# define a function to check if a command exists in system
function check_command {
	if ! command -v "$1" >/dev/null; then
		printf "[ERROR]: Command %s not found\n" "$1"
		exit 1
	fi
}
