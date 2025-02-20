set -eo pipefail

# Define a local export to path identify this local as a path

PWD=$(pwd)
NEW_PATH="${PWD%/Lib}/bin:$PATH"

if [[ ":$PATH:" != *":$NEW_PATH:"* ]]; then
	echo "export PATH=\"$NEW_PATH:\$PATH\"" >>.profile
	echo "[INFO]: Caminho '$NEW_PATH' adicionado ao .profile"
else
	echo "[INFO]: Caminho '$NEW_PATH' já está presente no PATH"
fi

export PATH="$NEW_PATH:$PATH"
