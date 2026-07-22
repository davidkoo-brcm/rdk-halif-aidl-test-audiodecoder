#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ "$1" == "help" || "$1" == "-h" || "$1" == "--help" ]]; then
	exec "${SCRIPT_DIR}/build.sh" help
fi

exec "${SCRIPT_DIR}/build.sh" mock "$@"
