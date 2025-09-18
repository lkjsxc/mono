/**
 * AI Content Processor using Google Gemini API
 * Handles processing of lecture materials with functional programming patterns
 */

import * as TE from 'fp-ts/TaskEither';
import { pipe } from 'fp-ts/function';
import * as path from 'path';
import { GoogleGenerativeAI } from '@google/generative-ai';
import * as pdf from 'pdf-parse';
import type { 
  AppError, 
  ProcessingResult, 
  ProcessingJobResult 
} from '../types';
import { 
  readTextFile,
  writeTextFile,
  exists,
  getFileExtension,
  getFilenameWithoutExt,
  isFile
} from '../utils/files';
import { logger } from '../utils/logger';
import { getAppConfig } from '../config';

/**
 * Processing options and configuration
 */
export interface ProcessingOptions {
  outputDir?: string;
  outputFormat?: 'markdown' | 'txt';
  includeOriginal?: boolean;
  customPrompt?: string;
  temperature?: number;
  maxTokens?: number;
  processingType?: 'summary' | 'notes' | 'qa' | 'custom';
}

/**
 * Supported file formats for processing
 */
const SUPPORTED_FORMATS = [
  '.txt', '.md', '.pdf', '.docx', '.doc', '.rtf', '.srt', '.vtt'
];

/**
 * Default processing prompts for different types
 */
const PROCESSING_PROMPTS = {
  summary: `Please analyze the following lecture material and create a comprehensive summary in clean Markdown format. 

Structure your response with:
- **Title**: A clear, descriptive title for the content
- **Overview**: Brief summary (2-3 sentences)
- **Key Concepts**: Main topics covered with bullet points
- **Important Details**: Specific facts, definitions, or examples
- **Takeaways**: 3-5 key learning points

Format the output as proper Markdown with headers, bullet points, and emphasis where appropriate.

Content to analyze:`,

  notes: `Please convert the following lecture material into well-structured study notes in Markdown format.

Create organized notes with:
- **Course/Lecture Title**
- **Main Topics** (with clear headers)
- **Definitions** (highlighted important terms)
- **Key Points** (bulleted lists)
- **Examples** (if provided)
- **Questions for Review** (generate 3-5 review questions)

Use proper Markdown formatting throughout.

Content to process:`,

  qa: `Please analyze the following lecture content and create a comprehensive Q&A study guide in Markdown format.

Generate:
- **Content Overview** (2-3 sentences)
- **Study Questions & Answers** (10-15 questions covering key concepts)
- **Key Terms & Definitions** (important vocabulary)
- **Practice Scenarios** (if applicable)

Format as clean Markdown with proper headers and organization.

Content to analyze:`,

  custom: `Please process the following content according to the specific instructions provided. Output should be in clean Markdown format.

Content to process:`,
} as const;

/**
 * Initialize Gemini AI client
 */
const initializeGeminiClient = (): TE.TaskEither<AppError, GoogleGenerativeAI> => {
  return TE.tryCatch(
    async () => {
      const config = getAppConfig();
      
      if (!config.gemini.apiKey) {
        throw new Error('Gemini API key not found. Please set GEMINI_API_KEY in your .env file.');
      }
      
      const genAI = new GoogleGenerativeAI(config.gemini.apiKey);
      logger.debug('✅ Gemini AI client initialized');
      return genAI;
    },
    (error): AppError => ({
      type: 'ConfigurationError',
      message: `Failed to initialize Gemini client: ${error instanceof Error ? error.message : 'Unknown error'}`,
      key: 'gemini',
    })
  );
};

/**
 * Validate file for processing
 */
const validateInputFile = (filePath: string): TE.TaskEither<AppError, string> => {
  return TE.tryCatch(
    async () => {
      if (!(await exists(filePath))) {
        throw new Error(`File not found: ${filePath}`);
      }
      
      if (!(await isFile(filePath))) {
        throw new Error(`Path is not a file: ${filePath}`);
      }
      
      const extension = getFileExtension(filePath).toLowerCase();
      if (!SUPPORTED_FORMATS.includes(extension)) {
        throw new Error(`Unsupported file format: ${extension}. Supported formats: ${SUPPORTED_FORMATS.join(', ')}`);
      }
      
      logger.info(`✅ Input file validated: ${path.basename(filePath)}`);
      return filePath;
    },
    (error): AppError => ({
      type: 'ValidationError',
      message: `File validation failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
      field: 'inputFile',
    })
  );
};

/**
 * Extract text content from various file formats
 */
const extractTextContent = (filePath: string): TE.TaskEither<AppError, string> => {
  const extension = getFileExtension(filePath).toLowerCase();
  
  return TE.tryCatch(
    async () => {
      switch (extension) {
        case '.txt':
        case '.md':
        case '.srt':
        case '.vtt':
          return await readTextFile(filePath);
          
        case '.pdf':
          const fs = await import('fs');
          const pdfBuffer = fs.readFileSync(filePath);
          const pdfData = await pdf.default(pdfBuffer);
          return pdfData.text;
          
        case '.docx':
        case '.doc':
          // For Word documents, we'd need additional libraries
          // For now, inform user that manual conversion is needed
          throw new Error('Word document processing requires manual conversion to PDF or TXT format');
          
        default:
          throw new Error(`Unsupported file type for text extraction: ${extension}`);
      }
    },
    (error): AppError => ({
      type: 'ProcessingError',
      message: `Text extraction failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
      stage: 'text-extraction',
    })
  );
};

/**
 * Process content with Gemini AI
 */
const processWithGemini = (
  content: string,
  options: ProcessingOptions
): TE.TaskEither<AppError, ProcessingResult> => {
  return pipe(
    initializeGeminiClient(),
    TE.chain((genAI) => 
      TE.tryCatch(
        async () => {
          const config = getAppConfig();
          const model = genAI.getGenerativeModel({ 
            model: config.gemini.model,
            generationConfig: {
              temperature: options.temperature || config.gemini.temperature,
              maxOutputTokens: options.maxTokens || config.gemini.maxTokens,
            },
          });
          
          // Build prompt
          const processingType = options.processingType || 'summary';
          const basePrompt = options.customPrompt || PROCESSING_PROMPTS[processingType];
          const fullPrompt = `${basePrompt}\n\n${content}`;
          
          logger.info(`🤖 Processing content with Gemini (${processingType})...`);
          logger.debug(`Content length: ${content.length} characters`);
          
          const result = await model.generateContent(fullPrompt);
          const response = await result.response;
          const processedContent = response.text();
          
          if (!processedContent) {
            throw new Error('No content generated by Gemini');
          }
          
          // Extract key points if it's a summary
          let keyPoints: string[] | undefined;
          if (processingType === 'summary') {
            keyPoints = extractKeyPoints(processedContent);
          }
          
          const processingResult: ProcessingResult = {
            originalContent: content,
            processedContent: processedContent.trim(),
            summary: processingType === 'summary' ? processedContent.trim() : undefined,
            keyPoints,
            processingModel: config.gemini.model,
            processedAt: new Date().toISOString(),
          };
          
          logger.success(`✅ Content processed successfully (${processedContent.length} characters generated)`);
          return processingResult;
        },
        (error): AppError => ({
          type: 'ProcessingError',
          message: `Gemini processing failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
          stage: 'ai-processing',
        })
      )
    )
  );
};

/**
 * Extract key points from markdown content
 */
const extractKeyPoints = (markdownContent: string): string[] => {
  const keyPoints: string[] = [];
  const lines = markdownContent.split('\n');
  
  let inKeyPointsSection = false;
  let inTakeawaysSection = false;
  
  for (const line of lines) {
    const trimmedLine = line.trim();
    
    // Detect key points sections
    if (trimmedLine.toLowerCase().includes('key concept') || 
        trimmedLine.toLowerCase().includes('key point') ||
        trimmedLine.toLowerCase().includes('important detail')) {
      inKeyPointsSection = true;
      continue;
    }
    
    if (trimmedLine.toLowerCase().includes('takeaway')) {
      inTakeawaysSection = true;
      continue;
    }
    
    // Stop at next major section
    if (trimmedLine.startsWith('##') && (inKeyPointsSection || inTakeawaysSection)) {
      inKeyPointsSection = false;
      inTakeawaysSection = false;
      continue;
    }
    
    // Extract bullet points
    if ((inKeyPointsSection || inTakeawaysSection) && trimmedLine.startsWith('- ')) {
      keyPoints.push(trimmedLine.substring(2).trim());
    }
  }
  
  return keyPoints;
};

/**
 * Generate output file path
 */
const generateOutputPath = (
  inputPath: string,
  options: ProcessingOptions
): string => {
  const inputName = getFilenameWithoutExt(inputPath);
  const inputDir = options.outputDir || path.dirname(inputPath);
  const extension = options.outputFormat || 'markdown';
  const fileExt = extension === 'markdown' ? 'md' : 'txt';
  
  const processingType = options.processingType || 'summary';
  return path.join(inputDir, `${inputName}_${processingType}.${fileExt}`);
};

/**
 * Save processed content to file
 */
const saveProcessedContent = (
  result: ProcessingResult,
  outputPath: string,
  options: ProcessingOptions
): TE.TaskEither<AppError, string> => {
  return TE.tryCatch(
    async () => {
      let contentToSave = result.processedContent;
      
      // Add metadata header
      const metadata = `<!-- Generated by zenbukko on ${result.processedAt} -->\n<!-- Model: ${result.processingModel} -->\n\n`;
      contentToSave = metadata + contentToSave;
      
      // Optionally include original content
      if (options.includeOriginal) {
        contentToSave += '\n\n---\n\n## Original Content\n\n';
        contentToSave += '```\n' + result.originalContent + '\n```';
      }
      
      await writeTextFile(outputPath, contentToSave);
      logger.success(`💾 Processed content saved to: ${path.basename(outputPath)}`);
      return outputPath;
    },
    (error): AppError => ({
      type: 'FileSystemError',
      message: `Failed to save processed content: ${error instanceof Error ? error.message : 'Unknown error'}`,
      path: outputPath,
    })
  );
};

/**
 * Main processing function
 */
export const processFile = (
  inputPath: string,
  options: ProcessingOptions = {}
): TE.TaskEither<AppError, ProcessingJobResult> => {
  const outputPath = generateOutputPath(inputPath, options);
  
  logger.info(`🧠 Starting AI processing: ${path.basename(inputPath)}`);
  
  return pipe(
    validateInputFile(inputPath),
    TE.chain(() => extractTextContent(inputPath)),
    TE.chain((content) => {
      if (content.length < 50) {
        return TE.left({
          type: 'ValidationError',
          message: 'Content too short for meaningful processing (minimum 50 characters)',
        });
      }
      
      if (content.length > 100000) {
        logger.warn('⚠️  Content is very long and may be truncated by AI model');
      }
      
      const startTime = Date.now();
      
      return pipe(
        processWithGemini(content, options),
        TE.chain((result) => 
          pipe(
            saveProcessedContent(result, outputPath, options),
            TE.map((): ProcessingJobResult => {
              const endTime = Date.now();
              const processingTime = endTime - startTime;
              
              logger.success(`✅ Processing job completed: ${path.basename(outputPath)}`);
              
              return {
                inputFile: inputPath,
                outputFile: outputPath,
                result,
                processingTime,
              };
            })
          )
        )
      );
    }),
    TE.mapLeft((error) => ({
      ...error,
      message: `File processing failed for ${path.basename(inputPath)}: ${error.message}`,
    }))
  );
};

/**
 * Batch process multiple files
 */
export const processFileBatch = (
  inputPaths: string[],
  options: ProcessingOptions = {}
): TE.TaskEither<AppError, ProcessingJobResult[]> => {
  logger.info(`🧠 Starting batch processing of ${inputPaths.length} files`);
  
  const processAll = inputPaths.map(inputPath => processFile(inputPath, options));
  
  return pipe(
    processAll,
    // Process sequentially to avoid API rate limits
    (tasks) => tasks.reduce(
      (acc, task) => pipe(
        acc,
        TE.chain((results) => pipe(
          task,
          TE.map((result) => [...results, result]),
          // Add delay between requests to respect rate limits
          TE.chainFirst(() => TE.tryCatch(
            () => new Promise(resolve => setTimeout(resolve, 1000)),
            (): AppError => ({ type: 'UnknownError', message: 'Delay failed' })
          ))
        ))
      ),
      TE.right<AppError, ProcessingJobResult[]>([])
    ),
    TE.map((results) => {
      const totalTime = results.reduce((sum, r) => sum + r.processingTime, 0);
      logger.success(`✅ Batch processing completed: ${results.length} files in ${(totalTime / 1000).toFixed(2)}s`);
      return results;
    })
  );
};

/**
 * Process transcription files (specialized function)
 */
export const processTranscriptionFile = (
  transcriptionPath: string,
  options: Omit<ProcessingOptions, 'processingType'> = {}
): TE.TaskEither<AppError, ProcessingJobResult> => {
  const processingOptions: ProcessingOptions = {
    ...options,
    processingType: 'notes',
    customPrompt: `Please convert this video transcription into well-structured lecture notes in Markdown format.

Create organized notes with:
- **Lecture Title** (infer from content)
- **Main Topics** (with clear section headers)
- **Key Concepts** (highlighted important terms)
- **Detailed Points** (organized bullet points)
- **Practical Examples** (if mentioned)
- **Summary** (key takeaways)

Clean up any transcription artifacts, improve grammar, and organize the content logically.

Transcription to process:`,
  };
  
  return processFile(transcriptionPath, processingOptions);
};
