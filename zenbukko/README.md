# Zenbukko - NNN Education Platform CLI Tool

A comprehensive command-line interface (CLI) tool for downloading, transcribing, and processing educational content from the NNN platform using Node.js v22 with TypeScript and functional programming patterns.

## Features

- 🔐 **Browser-based Authentication** - Secure login through Puppeteer automation
- 📚 **Course Management** - List and browse available courses
- ⬇️  **Content Download** - Batch download of videos, PDFs, and materials
- 🎤 **Video Transcription** - Local transcription using whisper.cpp
- 🤖 **AI Processing** - Content processing and analysis using Google Gemini API
- 🧩 **Functional Programming** - Built with fp-ts for robust error handling
- 📝 **TypeScript Strict Mode** - Type-safe with comprehensive validation

## Prerequisites

- Node.js v22 or higher
- macOS (M1/M2 supported)
- Google Gemini API key
- Git (for whisper.cpp setup)

## Installation

### Local Installation

1. Clone and navigate to the project:
```bash
cd /path/to/zenbukko
npm install
npm run build
```

2. Link globally (optional):
```bash
npm link
```

### Environment Setup

Create a `.env` file in the project root:

```env
# Required: Google Gemini API Key
GEMINI_API_KEY=your_gemini_api_key_here

# Optional: Custom configuration
OUTPUT_DIR=./downloads
LOG_LEVEL=info
GEMINI_MODEL=gemini-1.5-flash
GEMINI_TEMPERATURE=0.1
WHISPER_MODEL=base
```

## Usage

### Basic Commands

```bash
# Show help
zenbukko --help

# Show version
zenbukko --version
```

### Authentication

Before using the tool, authenticate with the NNN platform:

```bash
# Launch browser for manual login
zenbukko auth
```

This will:
- Open a Puppeteer browser window
- Navigate to NNN login page
- Wait for you to manually log in
- Capture and save session data
- Close browser automatically

### Course Management

```bash
# List all available courses (table format)
zenbukko list-courses

# List courses in JSON format
zenbukko list-courses --format json
```

### Content Download

```bash
# Download all content from a course
zenbukko download --course-id "course123"

# Download specific chapters
zenbukko download --course-id "course123" --chapters "1,2,3"

# Download to custom directory
zenbukko download --course-id "course123" --output ./my-downloads

# Download with concurrent limit
zenbukko download --course-id "course123" --concurrent 3
```

### Video Transcription

First, set up whisper.cpp (one-time setup):

```bash
# Install whisper.cpp and download models
zenbukko setup-whisper

# Force reinstallation
zenbukko setup-whisper --force
```

Then transcribe videos:

```bash
# Transcribe a single video file
zenbukko transcribe --input ./video.mp4

# Transcribe with custom model
zenbukko transcribe --input ./video.mp4 --model large

# Batch transcribe all videos in directory
zenbukko transcribe --input ./downloads --batch
```

### AI Content Processing

```bash
# Process a single document
zenbukko process --input ./document.pdf --type pdf

# Process transcript with custom prompt
zenbukko process --input ./transcript.txt --type transcript --prompt "Summarize key concepts"

# Batch process multiple files
zenbukko process --input ./downloads --batch

# Process with custom output format
zenbukko process --input ./content.txt --output-format markdown
```

## Command Reference

### `zenbukko auth`
Authenticate with NNN platform using browser automation.

**Options:** None

### `zenbukko list-courses [options]`
List all available courses from your account.

**Options:**
- `--format <format>` - Output format: `table` (default) or `json`

### `zenbukko download [options]`
Download course content including videos, PDFs, and materials.

**Options:**
- `--course-id <id>` - Course ID to download (required)
- `--chapters <chapters>` - Comma-separated chapter numbers (optional)
- `--output <path>` - Output directory (default: `./downloads`)
- `--concurrent <number>` - Concurrent downloads (default: 2)
- `--skip-videos` - Skip video downloads
- `--skip-pdfs` - Skip PDF downloads

### `zenbukko transcribe [options]`
Transcribe video files using whisper.cpp.

**Options:**
- `--input <path>` - Input video file or directory (required)
- `--model <model>` - Whisper model: `tiny`, `base`, `small`, `medium`, `large` (default: `base`)
- `--batch` - Process all videos in directory
- `--output <path>` - Output directory for transcripts

### `zenbukko process [options]`
Process documents and transcripts using Google Gemini AI.

**Options:**
- `--input <path>` - Input file or directory (required)
- `--type <type>` - Content type: `pdf`, `transcript`, `text`, `auto` (default: `auto`)
- `--prompt <prompt>` - Custom processing prompt
- `--output-format <format>` - Output format: `text`, `markdown`, `json` (default: `markdown`)
- `--batch` - Process all files in directory

### `zenbukko setup-whisper [options]`
Set up whisper.cpp for local transcription.

**Options:**
- `--force` - Force reinstallation even if already exists

## Configuration

The tool uses a hierarchical configuration system:

1. **Environment variables** (`.env` file)
2. **Session data** (stored in `~/.zenbukko/session.json`)
3. **Default values** (built-in)

### Configuration Files

- `~/.zenbukko/session.json` - Authentication session data
- `~/.zenbukko/whisper/` - Whisper.cpp installation directory
- `./downloads/` - Default download directory

## Architecture

### Core Principles

- **Functional Programming**: Built with fp-ts for immutable data and robust error handling
- **Type Safety**: Strict TypeScript with comprehensive Zod schema validation
- **Error Handling**: TaskEither patterns instead of try-catch blocks
- **Modular Design**: Clean separation of concerns with service layers

### Project Structure

```
src/
├── types/           # Type definitions and Zod schemas
├── config.ts        # Environment and session management
├── utils/          # Utility functions (logger, files)
├── services/       # External service integrations
│   ├── auth.service.ts     # NNN authentication
│   ├── api.client.ts       # HTTP client
│   └── nnn.service.ts      # NNN API wrapper
├── core/           # Core business logic
│   ├── downloader.ts       # Content download
│   ├── transcriber.ts      # Video transcription
│   └── processor.ts        # AI content processing
├── scripts/        # Setup and utility scripts
└── index.ts        # CLI entry point
```

## Development

### Building

```bash
# Clean build
npm run clean && npm run build

# Development with watch mode
npm run dev

# Type checking only
npm run type-check
```

### Testing

```bash
# Run linting
npm run lint

# Test CLI commands
node dist/index.js --help
```

## Error Handling

The tool uses functional programming patterns for robust error handling:

- **TaskEither** for asynchronous operations that may fail
- **Either** for synchronous operations with error states
- **Comprehensive logging** with structured error messages
- **Graceful degradation** with informative error messages

## Troubleshooting

### Common Issues

**1. API Key Error**
```
❌ Environment validation failed: GEMINI_API_KEY: Required
```
**Solution**: Add your Gemini API key to `.env` file

**2. Authentication Failed**
```
❌ Authentication failed: Session expired
```
**Solution**: Run `zenbukko auth` to re-authenticate

**3. Whisper Setup Failed**
```
❌ Failed to setup whisper.cpp: Build failed
```
**Solution**: Ensure you have build tools installed (`xcode-select --install` on macOS)

**4. Download Permission Error**
```
❌ Download failed: Permission denied
```
**Solution**: Check directory permissions and disk space

### Debug Mode

Enable debug logging by setting:
```env
LOG_LEVEL=debug
```

## Contributing

This project follows functional programming principles with fp-ts. When contributing:

1. Use TaskEither for async operations
2. Use Either for sync operations that may fail
3. Avoid try-catch blocks in favor of fp-ts patterns
4. Add comprehensive type definitions
5. Include proper error handling

## License

MIT License - see LICENSE file for details.

---

## Quick Start Example

```bash
# 1. Install and configure
npm install
echo "GEMINI_API_KEY=your_key_here" > .env
npm run build
npm link

# 2. Authenticate
zenbukko auth

# 3. List available courses
zenbukko list-courses

# 4. Download a course
zenbukko download --course-id "your-course-id"

# 5. Set up transcription (one-time)
zenbukko setup-whisper

# 6. Transcribe downloaded videos
zenbukko transcribe --input ./downloads --batch

# 7. Process content with AI
zenbukko process --input ./downloads --batch
```

For detailed documentation and advanced usage, please refer to the individual command help pages using `zenbukko [command] --help`.
