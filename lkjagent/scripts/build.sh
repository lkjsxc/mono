#!/bin/bash

# LKJAgent Build Script

set -e

echo "🔨 LKJAgent Build Script"
echo "========================"

# Check if we're in the right directory
if [ ! -f "Makefile" ]; then
    echo "❌ Error: Makefile not found. Please run from project root."
    exit 1
fi

# Parse command line arguments
MODE="release"
SETUP=false
CLEAN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --debug)
            MODE="debug"
            shift
            ;;
        --setup)
            SETUP=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --debug   Build with debug symbols"
            echo "  --setup   Create data directory and default files"
            echo "  --clean   Clean build files before building"
            echo "  --help    Show this help message"
            exit 0
            ;;
        *)
            echo "❌ Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo "🧹 Cleaning build files..."
    make clean
fi

# Setup if requested
if [ "$SETUP" = true ]; then
    echo "⚙️  Setting up project..."
    make setup
fi

# Build the project
echo "🔨 Building LKJAgent ($MODE mode)..."
if [ "$MODE" = "debug" ]; then
    make debug
else
    make all
fi

# Check if build succeeded
if [ -f "build/lkjagent" ]; then
    echo "✅ Build successful!"
    echo "📁 Executable: build/lkjagent"
    
    # Show file size
    SIZE=$(ls -lh build/lkjagent | awk '{print $5}')
    echo "📊 Size: $SIZE"
    
    echo ""
    echo "🚀 Quick start:"
    echo "  ./build/lkjagent        # Run the agent"
    echo "  make install            # Install to /usr/local/bin"
    echo "  cat data/config.json    # View configuration"
    echo "  cat data/memory.json    # View memory state"
else
    echo "❌ Build failed!"
    exit 1
fi
