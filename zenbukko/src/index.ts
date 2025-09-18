#!/usr/bin/env node
/**
 * Zenbukko CLI - NNN Lecture Downloader & Processor
 * Main entry point for the command-line interface
 */

import { Command } from 'commander';
import * as E from 'fp-ts/Either';
import { logger } from './utils/logger';
import { CONSTANTS, RUNTIME_INFO } from './config';
import { runAuthentication, validateSession } from './services/auth.service';
import { listCourses, getCompleteCourseStructure } from     // Add all commands
    program.addCommand(createAuthCommand());
    program.addCommand(createListCoursesCommand());
    program.addCommand(createDownloadCommand());
    program.addCommand(createTranscribeCommand());
    program.addCommand(createProcessCommand());
    program.addCommand(createDebugCommand());
    program.addCommand(createDebugApiCommand());ices/nnn.service';
import { downloadCourse } from './core/downloader';
import { transcribeVideo } from './core/transcriber';
import { processFile, processTranscriptionFile } from './core/processor';
import { DownloadArgsSchema, TranscribeArgsSchema } from './types/nnn.schema';

/**
 * Create the main CLI application
 */
const createCLI = (): Command => {
  const program = new Command();
  
  program
    .name('zenbukko')
    .description('NNN Lecture Downloader & Processor - Download and process educational content')
    .version(RUNTIME_INFO.projectVersion)
    .configureOutput({
      writeErr: (str) => logger.error(str.trim()),
      writeOut: (str) => logger.info(str.trim()),
    });
  
  return program;
};

/**
 * Authentication command
 */
const createAuthCommand = (): Command => {
  return new Command('auth')
    .description('Authenticate with NNN platform using browser login')
    .action(async () => {
      try {
        logger.info('🔐 Starting NNN authentication process...');
        
        // Check if already authenticated
        const sessionValidation = await validateSession()();
        if (E.isRight(sessionValidation) && sessionValidation.right) {
          logger.info('✅ Already authenticated with valid session');
          logger.info('💡 Use --force to re-authenticate anyway');
          return;
        }
        
        const result = await runAuthentication();
        if (result.success) {
          logger.success('🎉 Authentication successful!');
          logger.info('📝 Session data saved. You can now use other commands.');
        } else {
          logger.error(`❌ Authentication failed: ${result.error?.message}`);
          process.exit(1);
        }
        
      } catch (error) {
        logger.error('❌ Authentication error:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * List courses command
 */
const createListCoursesCommand = (): Command => {
  return new Command('list-courses')
    .description('List all available courses')
    .option('--format <format>', 'output format (table|json)', 'table')
    .action(async (options) => {
      try {
        logger.info('📚 Fetching course list...');
        
        const coursesResult = await listCourses()();
        
        if (E.isLeft(coursesResult)) {
          logger.error(`❌ Failed to fetch courses: ${coursesResult.left.message}`);
          process.exit(1);
        }
        
        const courses = coursesResult.right;
        
        if (courses.length === 0) {
          logger.warn('📭 No courses found');
          return;
        }
        
        if (options.format === 'json') {
          console.log(JSON.stringify(courses, null, 2));
        } else {
          // Table format
          console.log('\n📚 Available Courses:');
          console.log('─'.repeat(80));
          courses.forEach((course, index) => {
            console.log(`${index + 1}. ${course.title}`);
            console.log(`   ID: ${course.id} | B-Course ID: ${course.b_course_id}`);
            if (course.description) {
              console.log(`   Description: ${course.description.substring(0, 100)}${course.description.length > 100 ? '...' : ''}`);
            }
            console.log(`   Status: ${course.status} | Updated: ${new Date(course.updated_at).toLocaleDateString()}`);
            console.log('');
          });
          console.log(`Total: ${courses.length} courses found`);
        }
        
      } catch (error) {
        logger.error('❌ Error listing courses:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * Download command
 */
const createDownloadCommand = (): Command => {
  return new Command('download')
    .description('Download course content')
    .requiredOption('--courseId <id>', 'Course ID to download')
    .option('--outputDir <dir>', 'Output directory for downloads', CONSTANTS.DEFAULT_DOWNLOADS_DIR)
    .option('--overwrite', 'Overwrite existing files', false)
    .option('--concurrency <num>', 'Maximum concurrent downloads', '3')
    .option('--no-subdirs', 'Do not create subdirectories', false)
    .option('--transcribe', 'Automatically transcribe video files after download', false)
    .option('--process', 'Automatically process materials with AI after download', false)
    .action(async (options) => {
      try {
        // Validate arguments
        const validationResult = DownloadArgsSchema.safeParse({
          courseId: options.courseId,
          outputDir: options.outputDir,
          includeTranscription: options.transcribe,
          processWithAI: options.process,
        });
        
        if (!validationResult.success) {
          logger.error('❌ Invalid arguments:');
          validationResult.error.errors.forEach(err => {
            logger.error(`  - ${err.path.join('.')}: ${err.message}`);
          });
          process.exit(1);
        }
        
        const args = validationResult.data;
        logger.info(`🚀 Starting download for course: ${args.courseId}`);
        
        // Get course structure
        const courseStructureResult = await getCompleteCourseStructure(args.courseId)();
        
        if (E.isLeft(courseStructureResult)) {
          logger.error(`❌ Failed to fetch course structure: ${courseStructureResult.left.message}`);
          process.exit(1);
        }
        
        const courseStructure = courseStructureResult.right;
        logger.info(`📊 Course: ${courseStructure.course.title}`);
        logger.info(`📚 Chapters: ${courseStructure.chapters.length}`);
        
        const totalContent = courseStructure.chapters.reduce((sum, ch) => sum + ch.contents.length, 0);
        logger.info(`🎬 Total content items: ${totalContent}`);
        
        // Download course
        const downloadOptions = {
          outputDir: args.outputDir,
          overwrite: options.overwrite,
          createSubdirectories: !options.noSubdirs,
          maxConcurrency: parseInt(options.concurrency, 10),
          onProgress: (progress: any) => {
            logger.progress(
              `${progress.contentTitle}`,
              {
                current: Math.round(progress.percentage),
                total: 100,
              }
            );
          },
        };
        
        const downloadResult = await downloadCourse(courseStructure, downloadOptions)();
        
        if (E.isLeft(downloadResult)) {
          logger.error(`❌ Download failed: ${downloadResult.left.message}`);
          process.exit(1);
        }
        
        const result = downloadResult.right;
        logger.success('🎉 Download completed!');
        logger.info(`📁 Files downloaded: ${result.summary.successfulDownloads}/${result.summary.totalFiles}`);
        logger.info(`💾 Total size: ${(result.summary.totalSize / 1024 / 1024).toFixed(2)} MB`);
        logger.info(`⏱️  Time taken: ${(result.summary.totalTime / 1000).toFixed(2)} seconds`);
        
        // Post-processing steps
        if (args.includeTranscription) {
          logger.info('🎙️  Starting automatic transcription...');
          // TODO: Implement batch transcription of downloaded videos
        }
        
        if (args.processWithAI) {
          logger.info('🧠 Starting AI processing...');
          // TODO: Implement batch processing of downloaded materials
        }
        
      } catch (error) {
        logger.error('❌ Download error:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * Transcribe command
 */
const createTranscribeCommand = (): Command => {
  return new Command('transcribe')
    .description('Transcribe video file using whisper.cpp')
    .requiredOption('--file <path>', 'Path to video file')
    .option('--model <model>', 'Whisper model to use', CONSTANTS.DEFAULTS.WHISPER_MODEL)
    .option('--language <lang>', 'Language code (e.g., en, ja)')
    .option('--format <format>', 'Output format (txt|srt|vtt)', 'txt')
    .option('--output <dir>', 'Output directory (default: same as input file)')
    .action(async (options) => {
      try {
        // Validate arguments
        const validationResult = TranscribeArgsSchema.safeParse({
          file: options.file,
          model: options.model,
          outputFormat: options.format,
          language: options.language,
        });
        
        if (!validationResult.success) {
          logger.error('❌ Invalid arguments:');
          validationResult.error.errors.forEach(err => {
            logger.error(`  - ${err.path.join('.')}: ${err.message}`);
          });
          process.exit(1);
        }
        
        const args = validationResult.data;
        logger.info(`🎙️  Starting transcription: ${args.file}`);
        
        const transcriptionOptions = {
          model: args.model,
          ...(args.language && { language: args.language }),
          outputFormat: args.outputFormat as 'txt' | 'srt' | 'vtt',
          outputDir: options.output,
          verbose: logger.shouldLog('debug'),
        };
        
        const transcriptionResult = await transcribeVideo(args.file, transcriptionOptions)();
        
        if (E.isLeft(transcriptionResult)) {
          logger.error(`❌ Transcription failed: ${transcriptionResult.left.message}`);
          process.exit(1);
        }
        
        const result = transcriptionResult.right;
        logger.success('🎉 Transcription completed!');
        logger.info(`📝 Output file: ${result.outputFile}`);
        logger.info(`⏱️  Processing time: ${(result.processingTime / 1000).toFixed(2)} seconds`);
        logger.info(`📄 Text length: ${result.transcription.text.length} characters`);
        
        if (result.transcription.segments && result.transcription.segments.length > 0) {
          logger.info(`🎬 Segments: ${result.transcription.segments.length}`);
        }
        
      } catch (error) {
        logger.error('❌ Transcription error:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * Process command
 */
const createProcessCommand = (): Command => {
  return new Command('process')
    .description('Process documents/transcriptions with AI')
    .requiredOption('--file <path>', 'Path to file to process')
    .option('--type <type>', 'Processing type (summary|notes|qa|custom)', 'summary')
    .option('--output <dir>', 'Output directory (default: same as input file)')
    .option('--format <format>', 'Output format (markdown|txt)', 'markdown')
    .option('--prompt <text>', 'Custom processing prompt')
    .option('--include-original', 'Include original content in output', false)
    .action(async (options) => {
      try {
        logger.info(`🧠 Starting AI processing: ${options.file}`);
        
        const processingOptions = {
          processingType: options.type as 'summary' | 'notes' | 'qa' | 'custom',
          outputDir: options.output,
          outputFormat: options.format as 'markdown' | 'txt',
          customPrompt: options.prompt,
          includeOriginal: options.includeOriginal,
        };
        
        const isTranscription = options.file.includes('transcription') || 
                               options.file.endsWith('.srt') || 
                               options.file.endsWith('.vtt');
        
        const processingResult = isTranscription
          ? await processTranscriptionFile(options.file, processingOptions)()
          : await processFile(options.file, processingOptions)();
        
        if (E.isLeft(processingResult)) {
          logger.error(`❌ Processing failed: ${processingResult.left.message}`);
          process.exit(1);
        }
        
        const result = processingResult.right;
        logger.success('🎉 AI processing completed!');
        logger.info(`📝 Output file: ${result.outputFile}`);
        logger.info(`⏱️  Processing time: ${(result.processingTime / 1000).toFixed(2)} seconds`);
        logger.info(`📄 Generated content: ${result.result.processedContent.length} characters`);
        
      } catch (error) {
        logger.error('❌ Processing error:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * Setup Whisper command
 */
const createSetupWhisperCommand = (): Command => {
  return new Command('setup-whisper')
    .description('Set up whisper.cpp for transcription')
    .option('--force', 'Force reinstallation even if already exists', false)
    .action(async (options) => {
      try {
        logger.info('🔧 Setting up whisper.cpp...');
        
        const { setupWhisper } = await import('./scripts/setup-whisper');
        const success = await setupWhisper(options.force);
        
        if (success) {
          logger.success('✅ Whisper.cpp setup completed successfully!');
          logger.info('🎙️  You can now use the transcribe command');
        } else {
          logger.error('❌ Whisper.cpp setup failed');
          process.exit(1);
        }
        
      } catch (error) {
        logger.error('❌ Setup error:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * Debug command for examining course page structure
 */
const createDebugCommand = (): Command => {
  return new Command('debug-course-page')
    .description('Debug course page structure (development only)')
    .option('--puppeteer', 'Use Puppeteer to debug dynamic content', false)
    .action(async (options) => {
      try {
        const { debugCoursePage, debugCoursePageWithPuppeteer } = await import('./services/debug.service');
        
        if (options.puppeteer) {
          logger.info('🔍 Debugging with Puppeteer...');
          const result = await debugCoursePageWithPuppeteer()();
          
          if (result._tag === 'Left') {
            logger.error('❌ Puppeteer debug failed:', result.left.message);
          } else {
            logger.success('✅ Puppeteer debugging completed');
          }
        } else {
          logger.info('🔍 Debugging static HTML...');
          const result = await debugCoursePage()();
          
          if (result._tag === 'Left') {
            logger.error('❌ HTML debug failed:', result.left.message);
          } else {
            logger.success('✅ HTML debugging completed');
          }
        }
        
      } catch (error) {
        logger.error('❌ Debug error:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * Debug command for testing API endpoints
 */
const createDebugApiCommand = (): Command => {
  return new Command('debug-api')
    .description('Debug API endpoint responses (development only)')
    .requiredOption('--endpoint <endpoint>', 'API endpoint to test (e.g., /material/courses/{courseId})')
    .option('--course-id <courseId>', 'Course ID to use in endpoint (if endpoint contains {courseId})')
    .action(async (options) => {
      try {
        const { debugApiEndpoint } = await import('./services/debug.service');
        
        logger.info(`🔍 Debugging API endpoint: ${options.endpoint}`);
        const result = await debugApiEndpoint(options.endpoint, options.courseId)();
        
        if (result._tag === 'Left') {
          logger.error('❌ API debug failed:', result.left.message);
        } else {
          logger.success('✅ API debugging completed');
        }
        
      } catch (error) {
        logger.error('❌ Debug error:', error instanceof Error ? error.message : 'Unknown error');
        process.exit(1);
      }
    });
};

/**
 * Main CLI function
 */
const main = async (): Promise<void> => {
  try {
    const program = createCLI();
    
    // Add all commands
    program.addCommand(createAuthCommand());
    program.addCommand(createListCoursesCommand());
    program.addCommand(createDownloadCommand());
    program.addCommand(createTranscribeCommand());
    program.addCommand(createProcessCommand());
    program.addCommand(createSetupWhisperCommand());
    program.addCommand(createDebugCommand());
    program.addCommand(createDebugApiCommand());
    
    // Global error handler
    process.on('uncaughtException', (error) => {
      logger.error('💥 Uncaught exception:', error.message);
      if (logger.shouldLog('debug')) {
        console.error(error.stack);
      }
      process.exit(1);
    });
    
    process.on('unhandledRejection', (reason) => {
      logger.error('💥 Unhandled rejection:', reason instanceof Error ? reason.message : String(reason));
      if (logger.shouldLog('debug') && reason instanceof Error) {
        console.error(reason.stack);
      }
      process.exit(1);
    });
    
    // Parse command line arguments
    await program.parseAsync(process.argv);
    
  } catch (error) {
    logger.error('💥 Fatal error:', error instanceof Error ? error.message : 'Unknown error');
    process.exit(1);
  }
};

// Run the CLI if this file is executed directly
if (require.main === module) {
  main();
}

export { main };
