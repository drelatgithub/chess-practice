#!/bin/sh -e

# System requirements for this script:
#   - git >= 2.7.0
#   - cmake >= 3.13.0
#   - Latest C++ compiler

# Set directories and files
export chess_root_dir=$(X= cd -- "$(dirname -- "$0")" && pwd -P)

# Run bootstrapping
echo "Bootstrapping..."
$chess_root_dir/scripts/bootstrap.sh
