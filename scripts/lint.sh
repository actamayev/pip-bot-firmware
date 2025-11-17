#!/bin/bash

case "$1" in
    "check")
        # Check everything
        pio check -e local
        ;;
    "fix")
        # Fix specific file
        if [ -z "$2" ]; then
            echo "Usage: ./scripts/lint.sh fix <file>"
            echo "Example: ./scripts/lint.sh fix src/utils/utils.cpp"
            exit 1
        fi
        pio run -t compiledb -e local > /dev/null 2>&1
        /opt/homebrew/opt/llvm/bin/clang-tidy \
            --config-file=.clang-tidy \
            --header-filter='^src/.*' \
            --fix \
            --fix-errors \
            --extra-arg=-Wno-unknown-attributes \
            -p . \
            "$2"
        ;;
    "format")
        # Format specific file or directory
        if [ -z "$2" ]; then
            echo "Usage: ./scripts/lint.sh format <file-or-dir>"
            echo "Example: ./scripts/lint.sh format src/utils"
            exit 1
        fi
        if [ -d "$2" ]; then
            find "$2" \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) -exec clang-format -i {} +
        else
            clang-format -i "$2"
        fi
        ;;
    *)
        echo "Usage: ./scripts/lint.sh {check|fix|format} [file]"
        echo ""
        echo "Commands:"
        echo "  check              - Run all static analysis checks"
        echo "  fix <file>         - Auto-fix issues in a file"
        echo "  format <path>      - Format file or directory"
        echo ""
        echo "Examples:"
        echo "  ./scripts/lint.sh check"
        echo "  ./scripts/lint.sh fix src/utils/utils.cpp"
        echo "  ./scripts/lint.sh format src/utils"
        exit 1
        ;;
esac
