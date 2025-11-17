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
    "fix-all")
        # Fix all files in src/ directory
        echo "Running clang-tidy fix on all source files..."
        pio run -t compiledb -e local > /dev/null 2>&1
        
        # Find all .cpp and .h files in src/ and lib/
        find src lib -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) | while read -r file; do
            echo "Fixing: $file"
            /opt/homebrew/opt/llvm/bin/clang-tidy \
                --config-file=.clang-tidy \
                --header-filter='^src/.*' \
                --fix \
                --fix-errors \
                --extra-arg=-Wno-unknown-attributes \
                -p . \
                "$file" 2>&1 | grep -v "warnings generated" || true
        done
        echo "Done!"
        ;;
    "fix-dir")
        # Fix all files in a specific directory
        if [ -z "$2" ]; then
            echo "Usage: ./scripts/lint.sh fix-dir <directory>"
            echo "Example: ./scripts/lint.sh fix-dir src/utils"
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
        echo "  fix-all            - Auto-fix issues in all source files"
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
