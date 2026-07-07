#!/bin/bash

# Recursively dump all file contents from a directory into a markdown file
# Includes hidden directories but skips problematic ones
# Usage: ./dump_to_markdown.sh [directory] [output_file]

set -e

# Default values
SEARCH_DIR="${1:-.}"
OUTPUT_FILE="${2:-context_dump.md}"

# Directories to skip (common VCS, build, and cache directories)
# Note: We skip these even if they're hidden
SKIP_DIRS="\.git|\.svn|\.hg|__pycache__|\.pytest_cache|\.mypy_cache|\.ruff_cache|node_modules|bower_components|venv|env|\.venv|\.env|\.idea|\.vscode|dist|build|target|\.next|\.nuxt|vendor|\.terraform|\.serverless|\.cache|\.parcel-cache"

# File extensions to skip (binary and non-text files)
SKIP_EXTS="\.(pyc|pyo|pyd|class|jar|war|ear|zip|gz|tar|rar|7z|so|dll|dylib|exe|msi|deb|rpm|png|jpg|jpeg|gif|bmp|svg|ico|mp4|avi|mov|mkv|mp3|wav|flac|ogg|pdf|doc|docx|xls|xlsx|ppt|pptx|iso|img|bin|dat|db|sqlite|log|lock|o|a|lib|obj)$"

# File extensions that are definitely text
TEXT_EXTS="\.(txt|md|markdown|rst|adoc|org|tex|json|yaml|yml|toml|xml|html|htm|xhtml|css|scss|sass|less|js|jsx|ts|tsx|vue|svelte|py|rb|go|rs|c|cpp|h|hpp|java|kt|scala|clj|cljs|ex|exs|erl|hrl|lua|r|sh|bash|zsh|fish|ps1|bat|cmd|pl|pm|t|pod|swift|m|mm|cs|fs|fsx|vb|vbs|sql|prisma|graphql|gql|proto|thrift|dockerfile|makefile|cmake|nim|zig|v|dart|groovy|gradle|properties|conf|config|ini|cfg|env|sample|example|template|tf|tfvars|hcl|sqlite|sqlite3)$"

# Start the markdown file
cat > "$OUTPUT_FILE" << 'EOF'
# Directory Content Dump

This file contains the contents of all text files from the directory tree (including hidden directories).

---

EOF

# Function to check if a file should be skipped based on extension
should_skip_file() {
    local file="$1"
    local filename=$(basename "$file")
    if [[ "$filename" =~ $OUTPUT_FILE ]]; then
        return 1
    fi
    
    # Always include .env files even though they're hidden
    if [[ "$filename" == ".env"* ]]; then
        return 1
    fi
    
    # Check if it matches any skip extension
    if [[ "$file" =~ $SKIP_EXTS ]]; then
        return 0
    fi
    
    # If it's a text extension, don't skip
    if [[ "$file" =~ $TEXT_EXTS ]]; then
        return 1
    fi
    
    # For unknown extensions, check if it's a text file using file command
    if command -v file &> /dev/null; then
        local file_type=$(file -b --mime-type "$file" 2>/dev/null)
        if [[ "$file_type" == text/* ]] || [[ "$file_type" == application/json ]] || [[ "$file_type" == application/xml ]] || [[ "$file_type" == inode/x-empty ]]; then
            return 1
        fi
    fi
    
    # Default: skip if we can't determine it's text
    return 0
}

# Function to process a directory recursively
process_directory() {
    local dir="$1"
    
    # Use find to get all files, including hidden ones
    # -not -path '*/\.*' removed to include hidden files/dirs
    find "$dir" -type f | sort | while read -r file; do
        # Check if file path contains any skipped directories
        # This will skip .git, node_modules, etc. even when hidden
        local should_skip=false
        IFS='|' read -ra SKIP_ARRAY <<< "$SKIP_DIRS"
        for skip_pattern in "${SKIP_ARRAY[@]}"; do
            if [[ "$file" =~ /$skip_pattern/ ]] || [[ "$file" =~ ^$skip_pattern/ ]]; then
                should_skip=true
                break
            fi
        done
        if [[ "$should_skip" == true ]]; then
            continue
        fi
        
        # Check if file should be skipped based on extension
        if should_skip_file "$file"; then
            continue
        fi
        
        # Get relative path for display
        local rel_path="${file#$SEARCH_DIR/}"
        if [[ "$rel_path" == "$file" ]]; then
            rel_path="$file"
        fi
        
        # Get file extension for syntax highlighting
        local ext="${file##*.}"
        local lang=""
        case "$ext" in
            py|python) lang="python" ;;
            js|javascript) lang="javascript" ;;
            ts|typescript) lang="typescript" ;;
            jsx) lang="jsx" ;;
            tsx) lang="tsx" ;;
            vue) lang="vue" ;;
            svelte) lang="svelte" ;;
            rb|ruby) lang="ruby" ;;
            go) lang="go" ;;
            rs|rust) lang="rust" ;;
            c) lang="c" ;;
            cpp|cxx|cc) lang="cpp" ;;
            h|hpp) lang="cpp" ;;
            java) lang="java" ;;
            kt|kts|ktm) lang="kotlin" ;;
            scala) lang="scala" ;;
            clj|cljs|cljc|cljx) lang="clojure" ;;
            ex|exs) lang="elixir" ;;
            erl|hrl) lang="erlang" ;;
            lua) lang="lua" ;;
            r) lang="r" ;;
            sh|bash|zsh|fish) lang="bash" ;;
            ps1) lang="powershell" ;;
            bat|cmd) lang="batch" ;;
            pl|pm) lang="perl" ;;
            swift) lang="swift" ;;
            m|mm) lang="objectivec" ;;
            cs) lang="csharp" ;;
            fs|fsx) lang="fsharp" ;;
            sql) lang="sql" ;;
            json) lang="json" ;;
            yaml|yml) lang="yaml" ;;
            toml) lang="toml" ;;
            xml) lang="xml" ;;
            html|htm|xhtml) lang="html" ;;
            css|scss|sass|less) lang="css" ;;
            md|markdown) lang="markdown" ;;
            rst) lang="rst" ;;
            tex) lang="latex" ;;
            dockerfile) lang="dockerfile" ;;
            makefile) lang="makefile" ;;
            cmake) lang="cmake" ;;
            proto) lang="protobuf" ;;
            graphql|gql) lang="graphql" ;;
            dart) lang="dart" ;;
            groovy) lang="groovy" ;;
            nim) lang="nim" ;;
            zig) lang="zig" ;;
            v) lang="v" ;;
            tf|tfvars) lang="terraform" ;;
            hcl) lang="hcl" ;;
            *) lang="" ;;
        esac
        
        # Write file header and content to markdown
        {
            echo "## 📄 \`$rel_path\`"
            echo ""
            echo "\`\`\`$lang"
            cat "$file" 2>/dev/null || echo "[ERROR: Could not read file]"
            echo "\`\`\`"
            echo ""
            echo "---"
            echo ""
        } >> "$OUTPUT_FILE"
        
        echo "Processed: $rel_path"
    done
}

# Main execution
echo "Starting dump from: $SEARCH_DIR"
echo "Output file: $OUTPUT_FILE"
echo ""

# Check if directory exists
if [[ ! -d "$SEARCH_DIR" ]]; then
    echo "Error: Directory '$SEARCH_DIR' does not exist"
    exit 1
fi

# Process the directory
process_directory "$SEARCH_DIR"

echo ""
echo "✅ Complete! Content dumped to: $OUTPUT_FILE"
echo "Total size: $(du -h "$OUTPUT_FILE" | cut -f1)"
echo "Total lines: $(wc -l < "$OUTPUT_FILE")"
wl-copy < $OUTPUT_FILE

