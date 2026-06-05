#!/usr/bin/env sh
set -eu

PRESET="debug"
SKIP_FORMAT=0
RUN_TIDY=0

usage() {
    cat <<'EOF'
Usage: scripts/check.sh [--preset <name>] [--skip-format] [--tidy]

Runs the standard local verification path:
  1. clang-format dry run when clang-format is available
  2. cmake configure with the selected preset
  3. cmake build with the selected preset
  4. ctest with the selected preset

Use --tidy to also run run-clang-tidy when it is available.
EOF
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --preset)
            if [ "$#" -lt 2 ]; then
                usage
                exit 2
            fi
            PRESET="$2"
            shift 2
            ;;
        --skip-format)
            SKIP_FORMAT=1
            shift
            ;;
        --tidy)
            RUN_TIDY=1
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            usage
            exit 2
            ;;
    esac
done

format_files() {
    if [ "$SKIP_FORMAT" -eq 1 ]; then
        echo "Skipping clang-format."
        return
    fi

    if ! command -v clang-format >/dev/null 2>&1; then
        echo "Skipping clang-format: command not found."
        return
    fi

    files=$(
        find include src tests \
            -type f \( -name '*.c' -o -name '*.h' \) \
            ! -path 'include/glad/*' \
            ! -path 'include/KHR/*' \
            ! -path 'include/nuklear/*' \
            ! -path 'src/glad.c' \
            | sort
    )

    if [ -z "$files" ]; then
        return
    fi

    echo "$files" | xargs clang-format --dry-run --Werror
}

run_tidy() {
    if [ "$RUN_TIDY" -eq 0 ]; then
        return
    fi

    if ! command -v run-clang-tidy >/dev/null 2>&1; then
        echo "Skipping clang-tidy: run-clang-tidy command not found."
        return
    fi

    run-clang-tidy -p "build/$PRESET"
}

format_files
cmake --preset "$PRESET"
cmake --build --preset "$PRESET" --parallel
ctest --preset "$PRESET" --output-on-failure
run_tidy
