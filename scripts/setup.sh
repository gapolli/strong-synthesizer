#!/usr/bin/env bash

set -euo pipefail

show_help() {
    cat << 'EOF'
NAME
    setup.sh - Initialize the Strong Synthesizer development environment

SYNOPSIS
    setup.sh [OPTIONS]

DESCRIPTION
    Prepares the repository workspace by creating the necessary directory
    structures, configuring the Python virtual environment, and installing 
    required packages. The C++ dependencies (miniaudio) are managed natively 
    by CMake.

OPTIONS
    -h, --help
        Display this manual-style help message and exit.

    -f, --force
        Force re-initialization of python environments and build folders,
        overwriting existing localized configuration binaries.

    --skip-python
        Isolate building configurations strictly to C++ Core components,
        skipping virtualenv creation and package manager execution.

EXAMPLES
    ./scripts/setup.sh
    ./scripts/setup.sh --force
EOF
}

FORCE_INIT=false
SKIP_PYTHON=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -f|--force)
            FORCE_INIT=true
            shift
            ;;
        --skip-python)
            SKIP_PYTHON=true
            shift
            ;;
        *)
            echo "Error: Unknown option $1" >&2
            show_help >&2
            exit 1
            ;;
    esac
done

echo "=== Initializing Workspace Structure ==="
mkdir -p config
mkdir -p src/core/include
mkdir -p src/core/source
mkdir -p src/bridge
mkdir -p tests/cpp
mkdir -p tests/python
mkdir -p build

if [ "$SKIP_PYTHON" = false ]; then
    echo "=== Setting Up Python Environment ==="
    if [ "$FORCE_INIT" = true ] && [ -d ".venv" ]; then
        rm -rf .venv
    fi

    if [ ! -d ".venv" ]; then
        python3 -m venv .venv
    fi

    source .venv/bin/activate
    pip install --upgrade pip
    
    if [ -f "pyproject.toml" ]; then
        pip install -e .[dev]
    fi
fi

if [ ! -f "src/core/source/main.cpp" ]; then
    cat << 'EOF' > src/core/source/main.cpp
int main() {
    return 0;
}
EOF
fi

echo "=== Project Setup Successfully Completed ==="
