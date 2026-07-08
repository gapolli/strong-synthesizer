#!/usr/bin/env bash

set -euo pipefail

show_help() {
    cat << 'EOF'
NAME
    validate_build.sh - Automate compiler pass tracking and testing for project stack

SYNOPSIS
    validate_build.sh [OPTIONS]

DESCRIPTION
    Runs clean compiler routines across the target C++ repository footprint, outputs
    configured executables and shared library binaries, and tests the runtime stability.

OPTIONS
    -h, --help
        Display this manual-style help message and exit.

    -c, --clean
        Wipe previous compilation artifact tracks entirely prior to running verification.

    --run-python
        Trigger the Python integration bridge runtime tracking script post successful build.
EOF
}

CLEAN_BUILD=false
RUN_PYTHON=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -c|--clean)
            CLEAN_BUILD=true
            shift
            ;;
        --run-python)
            RUN_PYTHON=true
            shift
            ;;
        *)
            echo "Error: Unknown option $1" >&2
            show_help >&2
            exit 1
            ;;
    esac
done

if [ "$CLEAN_BUILD" = true ] && [ -d "build" ]; then
    echo "=== Purging Build Artifact Tree ==="
    rm -rf build
fi

mkdir -p build

echo "=== Running CMake Configuration ==="
cmake -B build -S .

echo "=== Compiling Targets ==="
cmake --build build --config Release

echo "=== Executing Native Binary Verification ==="
./build/synth_cli

if [ "$RUN_PYTHON" = true ]; then
    echo "=== Executing Python Integration Tests ==="
    if [ -d ".venv" ]; then
        source .venv/bin/activate
    fi
    python3 tests/python/test_bridge.py
fi

echo "=== Automated Build Validation Complete ==="
