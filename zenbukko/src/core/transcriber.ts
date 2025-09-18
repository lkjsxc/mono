/**
 * Video Transcription Core Logic using whisper.cpp
 * Handles video transcription with functional programming patterns
 */

import * as TE from 'fp-ts/TaskEither';
import { pipe } from 'fp-ts/function';
import * as path from 'path';
import { spawn } from 'child_process';
import type { 
  AppError, 
  TranscriptionResult, 
  TranscriptionJobResult 
} from '../types';
import { 
  exists, 
  getFileExtension, 
  getFilenameWithoutExt,
  ensureDir,
  isFile 
} from '../utils/files';
import { logger } from '../utils/logger';
import { CONSTANTS, isWhisperSetup } from '../config';

/**
 * Transcription options and configuration
 */
export interface TranscriptionOptions {
  model?: string;
  language?: string;
  outputFormat?: 'txt' | 'srt' | 'vtt';
  outputDir?: string;
  threads?: number;
  verbose?: boolean;
  wordTimestamps?: boolean;
}

/**
 * Supported video formats for transcription
 */
const SUPPORTED_VIDEO_FORMATS = [
  '.mp4', '.avi', '.mov', '.mkv', '.wmv', '.flv', 
  '.webm', '.m4v', '.3gp', '.mp3', '.wav', '.m4a'
];

/**
 * Validate video file for transcription
 */
const validateVideoFile = (filePath: string): TE.TaskEither<AppError, string> => {
  return TE.tryCatch(
    async () => {
      // Check if file exists
      if (!(await exists(filePath))) {
        throw new Error(`Video file not found: ${filePath}`);
      }
      
      // Check if it's a file (not directory)
      if (!(await isFile(filePath))) {
        throw new Error(`Path is not a file: ${filePath}`);
      }
      
      // Check file extension
      const extension = getFileExtension(filePath).toLowerCase();
      if (!SUPPORTED_VIDEO_FORMATS.includes(extension)) {
        throw new Error(`Unsupported video format: ${extension}. Supported formats: ${SUPPORTED_VIDEO_FORMATS.join(', ')}`);
      }
      
      logger.info(`✅ Video file validated: ${path.basename(filePath)}`);
      return filePath;
    },
    (error): AppError => ({
      type: 'ValidationError',
      message: `Video validation failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
      field: 'videoFile',
    })
  );
};

/**
 * Check if whisper.cpp is properly installed
 */
const validateWhisperSetup = (): TE.TaskEither<AppError, void> => {
  return TE.tryCatch(
    async () => {
      if (!isWhisperSetup()) {
        throw new Error('Whisper.cpp is not set up. Please run "npm run setup:whisper" first.');
      }
      
      // Check if executable is actually executable
      if (!(await exists(CONSTANTS.WHISPER_EXECUTABLE))) {
        throw new Error(`Whisper executable not found at: ${CONSTANTS.WHISPER_EXECUTABLE}`);
      }
      
      logger.debug('✅ Whisper.cpp setup validated');
    },
    (error): AppError => ({
      type: 'ConfigurationError',
      message: `Whisper setup validation failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
      key: 'whisper',
    })
  );
};

/**
 * Generate output file path for transcription
 */
const generateOutputPath = (
  inputPath: string, 
  options: TranscriptionOptions
): string => {
  const inputName = getFilenameWithoutExt(inputPath);
  const inputDir = options.outputDir || path.dirname(inputPath);
  const extension = options.outputFormat || 'txt';
  
  return path.join(inputDir, `${inputName}_transcription.${extension}`);
};

/**
 * Build whisper.cpp command arguments
 */
const buildWhisperArgs = (
  inputPath: string,
  outputPath: string,
  options: TranscriptionOptions
): string[] => {
  const args = [
    '-m', path.join(CONSTANTS.WHISPER_MODELS_DIR, `ggml-${options.model || CONSTANTS.DEFAULTS.WHISPER_MODEL}.bin`),
    '-f', inputPath,
    '-of', outputPath,
  ];
  
  // Add language if specified
  if (options.language) {
    args.push('-l', options.language);
  }
  
  // Add output format
  const format = options.outputFormat || 'txt';
  if (format === 'srt') {
    args.push('--output-srt');
  } else if (format === 'vtt') {
    args.push('--output-vtt');
  } else {
    args.push('--output-txt');
  }
  
  // Add threading
  if (options.threads) {
    args.push('-t', options.threads.toString());
  }
  
  // Add word timestamps if requested
  if (options.wordTimestamps) {
    args.push('--max-len', '0');
  }
  
  // Reduce verbosity unless explicitly requested
  if (!options.verbose) {
    args.push('--print-progress', 'false');
  }
  
  return args;
};

/**
 * Execute whisper.cpp transcription
 */
const runWhisperTranscription = (
  inputPath: string,
  outputPath: string,
  options: TranscriptionOptions
): TE.TaskEither<AppError, TranscriptionResult> => {
  return TE.tryCatch(
    async () => {
      const startTime = Date.now();
      const args = buildWhisperArgs(inputPath, outputPath, options);
      
      logger.info(`🎙️  Starting transcription: ${path.basename(inputPath)}`);
      logger.debug(`Command: ${CONSTANTS.WHISPER_EXECUTABLE} ${args.join(' ')}`);
      
      // Ensure output directory exists
      await ensureDir(path.dirname(outputPath));
      
      return new Promise<TranscriptionResult>((resolve, reject) => {
        const process = spawn(CONSTANTS.WHISPER_EXECUTABLE, args, {
          stdio: ['pipe', 'pipe', 'pipe'],
          cwd: CONSTANTS.WHISPER_DIR,
        });
        
        let stdout = '';
        let stderr = '';
        
        process.stdout?.on('data', (data) => {
          stdout += data.toString();
          if (options.verbose) {
            logger.debug(`Whisper stdout: ${data.toString().trim()}`);
          }
        });
        
        process.stderr?.on('data', (data) => {
          stderr += data.toString();
          if (options.verbose) {
            logger.debug(`Whisper stderr: ${data.toString().trim()}`);
          }
        });
        
        process.on('close', async (code) => {
          const endTime = Date.now();
          const duration = endTime - startTime;
          
          if (code !== 0) {
            reject(new Error(`Whisper process failed with code ${code}: ${stderr}`));
            return;
          }
          
          try {
            // Read the generated transcription file
            const { readTextFile } = await import('../utils/files');
            const transcriptionText = await readTextFile(outputPath);
            
            // Parse segments if available (for SRT/VTT formats)
            const segments = parseTranscriptionSegments(transcriptionText, options.outputFormat || 'txt');
            
            const result: TranscriptionResult = {
              text: transcriptionText,
              segments,
              language: options.language,
              duration: duration / 1000, // Convert to seconds
            };
            
            logger.success(`✅ Transcription completed in ${(duration / 1000).toFixed(2)}s`);
            resolve(result);
          } catch (error) {
            reject(new Error(`Failed to read transcription output: ${error instanceof Error ? error.message : 'Unknown error'}`));
          }
        });
        
        process.on('error', (error) => {
          reject(new Error(`Failed to start whisper process: ${error.message}`));
        });
      });
    },
    (error): AppError => ({
      type: 'ProcessingError',
      message: `Transcription failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
      stage: 'whisper-execution',
    })
  );
};

/**
 * Parse transcription segments from different output formats
 */
const parseTranscriptionSegments = (
  text: string, 
  format: string
): Array<{ start: number; end: number; text: string }> => {
  const segments: Array<{ start: number; end: number; text: string }> = [];
  
  if (format === 'srt') {
    // Parse SRT format
    const srtBlocks = text.split('\n\n').filter(block => block.trim());
    
    for (const block of srtBlocks) {
      const lines = block.split('\n');
      if (lines.length >= 3 && lines[1]) {
        const timeMatch = lines[1].match(/(\d+):(\d+):(\d+),(\d+) --> (\d+):(\d+):(\d+),(\d+)/);
        if (timeMatch) {
          const start = parseTimeToSeconds(timeMatch.slice(1, 5));
          const end = parseTimeToSeconds(timeMatch.slice(5, 9));
          const segmentText = lines.slice(2).join(' ');
          
          segments.push({ start, end, text: segmentText });
        }
      }
    }
  } else if (format === 'vtt') {
    // Parse WebVTT format
    const lines = text.split('\n');
    let currentSegment: { start: number; end: number; text: string } | null = null;
    
    for (const line of lines) {
      const timeMatch = line.match(/(\d+):(\d+):(\d+)\.(\d+) --> (\d+):(\d+):(\d+)\.(\d+)/);
      if (timeMatch) {
        const start = parseTimeToSeconds(timeMatch.slice(1, 5));
        const end = parseTimeToSeconds(timeMatch.slice(5, 9));
        currentSegment = { start, end, text: '' };
      } else if (currentSegment && line.trim() && !line.startsWith('WEBVTT')) {
        currentSegment.text += line + ' ';
        segments.push({
          ...currentSegment,
          text: currentSegment.text.trim(),
        });
        currentSegment = null;
      }
    }
  }
  
  return segments;
};

/**
 * Helper function to parse time string to seconds
 */
const parseTimeToSeconds = (timeParts: string[]): number => {
  const [hoursStr = '0', minutesStr = '0', secondsStr = '0', millisecondsStr = '0'] = timeParts;
  const hours = parseInt(hoursStr, 10) || 0;
  const minutes = parseInt(minutesStr, 10) || 0;
  const seconds = parseInt(secondsStr, 10) || 0;
  const milliseconds = parseInt(millisecondsStr, 10) || 0;
  return hours * 3600 + minutes * 60 + seconds + milliseconds / 1000;
};

/**
 * Main transcription function
 */
export const transcribeVideo = (
  inputPath: string,
  options: TranscriptionOptions = {}
): TE.TaskEither<AppError, TranscriptionJobResult> => {
  const outputPath = generateOutputPath(inputPath, options);
  
  logger.info(`🎬 Starting video transcription: ${path.basename(inputPath)}`);
  
  return pipe(
    validateWhisperSetup(),
    TE.chain(() => validateVideoFile(inputPath)),
    TE.chain(() => {
      const startTime = Date.now();
      
      return pipe(
        runWhisperTranscription(inputPath, outputPath, options),
        TE.map((result): TranscriptionJobResult => {
          const endTime = Date.now();
          const processingTime = endTime - startTime;
          
          logger.success(`✅ Transcription job completed: ${path.basename(outputPath)}`);
          
          return {
            inputFile: inputPath,
            outputFile: outputPath,
            transcription: result,
            processingTime,
          };
        })
      );
    }),
    TE.mapLeft((error) => ({
      ...error,
      message: `Video transcription failed for ${path.basename(inputPath)}: ${error.message}`,
    }))
  );
};

/**
 * Batch transcribe multiple videos
 */
export const transcribeVideoBatch = (
  inputPaths: string[],
  options: TranscriptionOptions = {}
): TE.TaskEither<AppError, TranscriptionJobResult[]> => {
  logger.info(`🎬 Starting batch transcription of ${inputPaths.length} videos`);
  
  const transcribeAll = inputPaths.map(inputPath => transcribeVideo(inputPath, options));
  
  return pipe(
    transcribeAll,
    // Process sequentially to avoid overwhelming the system
    (tasks) => tasks.reduce(
      (acc, task) => pipe(
        acc,
        TE.chain((results) => pipe(
          task,
          TE.map((result) => [...results, result])
        ))
      ),
      TE.right<AppError, TranscriptionJobResult[]>([])
    ),
    TE.map((results) => {
      const totalTime = results.reduce((sum, r) => sum + r.processingTime, 0);
      logger.success(`✅ Batch transcription completed: ${results.length} files in ${(totalTime / 1000).toFixed(2)}s`);
      return results;
    })
  );
};

/**
 * Clean up old transcription files
 */
export const cleanupTranscriptions = (directory: string, olderThanDays: number = 30): TE.TaskEither<AppError, number> => {
  return TE.tryCatch(
    async () => {
      const { listDirectory, getFileStats } = await import('../utils/files');
      const fs = await import('fs/promises');
      
      const files = await listDirectory(directory);
      const transcriptionFiles = files.filter(file => 
        file.includes('_transcription.') && 
        (file.endsWith('.txt') || file.endsWith('.srt') || file.endsWith('.vtt'))
      );
      
      const cutoffDate = new Date(Date.now() - olderThanDays * 24 * 60 * 60 * 1000);
      let deletedCount = 0;
      
      for (const file of transcriptionFiles) {
        const filePath = path.join(directory, file);
        const stats = await getFileStats(filePath);
        
        if (stats && stats.mtime < cutoffDate) {
          await fs.unlink(filePath);
          deletedCount++;
          logger.debug(`🗑️  Deleted old transcription: ${file}`);
        }
      }
      
      if (deletedCount > 0) {
        logger.info(`🗑️  Cleaned up ${deletedCount} old transcription files`);
      }
      
      return deletedCount;
    },
    (error): AppError => ({
      type: 'FileSystemError',
      message: `Cleanup failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
      path: directory,
    })
  );
};
