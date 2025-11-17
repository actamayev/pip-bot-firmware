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
        
        # Safety check: only allow fixing files in src/ directory
        if [[ ! "$2" =~ ^src/ ]]; then
            echo "Error: Can only fix files in src/ directory"
            echo "Attempted to fix: $2"
            exit 1
        fi
        
        pio run -t compiledb -e local > /dev/null 2>&1
        /opt/homebrew/opt/llvm/bin/clang-tidy \
            --config-file=.clang-tidy \
            --header-filter='^src/.*' \
            --fix \
            --fix-errors \
            --extra-arg=-Wno-unknown-attributes \
            --line-filter="[{\"name\":\"$2\"}]" \
            -p . \
            "$2"
        ;;
    "fix-dir")
        # Fix all files in a specific directory
        if [ -z "$2" ]; then
            echo "Usage: ./scripts/lint.sh fix-dir <directory>"
            echo "Example: ./scripts/lint.sh fix-dir src/utils"
            exit 1
        fi
        
        # Safety check: only allow fixing directories under src/
        if [[ ! "$2" =~ ^src/ ]]; then
            echo "Error: Can only fix directories under src/"
            echo "Attempted to fix: $2"
            exit 1
        fi
        
        echo "Running clang-tidy fix on directory: $2"
        pio run -t compiledb -e local > /dev/null 2>&1
        
        find "$2" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) | while read -r file; do
            echo "Fixing: $file"
            /opt/homebrew/opt/llvm/bin/clang-tidy \
                --config-file=.clang-tidy \
                --header-filter='^src/.*' \
                --fix \
                --fix-errors \
                --extra-arg=-Wno-unknown-attributes \
                --line-filter="[{\"name\":\"$file\"}]" \
                -p . \
                "$file" 2>&1 | grep -v "warnings generated" || true
        done
        echo "Done!"
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
        echo "Usage: ./scripts/lint.sh {check|fix|fix-all|fix-dir|format} [file]"
        echo ""
        echo "Commands:"
        echo "  check              - Run all static analysis checks"
        echo "  fix <file>         - Auto-fix issues in a single file"
        echo "  fix-all            - Auto-fix issues in all source files (src/ only)"
        echo "  fix-dir <dir>      - Auto-fix issues in a directory"
        echo "  format <path>      - Format file or directory"
        echo ""
        echo "Examples:"
        echo "  ./scripts/lint.sh check"
        echo "  ./scripts/lint.sh fix src/utils/utils.cpp"
        echo "  ./scripts/lint.sh fix-all"
        echo "  ./scripts/lint.sh fix-dir src/utils"
        echo "  ./scripts/lint.sh format src/utils"
        exit 1
        ;;
esac
