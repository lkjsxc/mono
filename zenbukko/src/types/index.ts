import { z } from 'zod';
import * as Schema from './nnn.schema';

/**
 * Inferred TypeScript types from Zod schemas
 */

export type Course = z.infer<typeof Schema.CourseSchema>;
export type Chapter = z.infer<typeof Schema.ChapterSchema>;
export type Content = z.infer<typeof Schema.ContentSchema>;

export type CourseResponse = z.infer<typeof Schema.CourseResponseSchema>;
export type ChapterResponse = z.infer<typeof Schema.ChapterResponseSchema>;
export type ContentResponse = z.infer<typeof Schema.ContentResponseSchema>;
export type CourseListResponse = z.infer<typeof Schema.CourseListResponseSchema>;

export type SessionData = z.infer<typeof Schema.SessionDataSchema>;
export type DownloadArgs = z.infer<typeof Schema.DownloadArgsSchema>;
export type TranscribeArgs = z.infer<typeof Schema.TranscribeArgsSchema>;

export type DownloadProgress = z.infer<typeof Schema.DownloadProgressSchema>;
export type ErrorResponse = z.infer<typeof Schema.ErrorResponseSchema>;
export type TranscriptionResult = z.infer<typeof Schema.TranscriptionResultSchema>;
export type ProcessingResult = z.infer<typeof Schema.ProcessingResultSchema>;

export type NNNApiResponse = z.infer<typeof Schema.NNNApiResponseSchema>;

/**
 * Additional utility types for the application
 */

export interface DownloadableContent extends Content {
  downloadPath?: string;
  downloadStatus?: 'pending' | 'downloading' | 'completed' | 'error';
  downloadError?: string;
}

export interface ProcessableContent extends DownloadableContent {
  transcriptionPath?: string;
  processedPath?: string;
  processingStatus?: 'pending' | 'processing' | 'completed' | 'error';
  processingError?: string;
}

export interface AuthConfig {
  sessionFile: string;
  cookieString: string;
  token: string | undefined;
  isValid: boolean;
}

export interface WhisperConfig {
  executablePath: string;
  modelPath: string;
  model: string;
  language?: string;
  outputFormat: 'txt' | 'srt' | 'vtt';
}

export interface GeminiConfig {
  apiKey: string;
  model: string;
  temperature: number;
  maxTokens: number;
}

export interface AppConfig {
  defaultOutputDir: string;
  whisper: WhisperConfig;
  gemini: GeminiConfig;
  auth: AuthConfig;
  logLevel: 'debug' | 'info' | 'warn' | 'error';
}

/**
 * Error types for functional programming with fp-ts
 */
export type AppError = 
  | { type: 'NetworkError'; message: string; statusCode?: number }
  | { type: 'AuthenticationError'; message: string }
  | { type: 'ValidationError'; message: string; field?: string }
  | { type: 'FileSystemError'; message: string; path?: string }
  | { type: 'ProcessingError'; message: string; stage?: string }
  | { type: 'ConfigurationError'; message: string; key?: string }
  | { type: 'UnknownError'; message: string };

/**
 * Result types for API operations
 */
export type AuthResult = {
  success: boolean;
  sessionData?: SessionData;
  error?: AppError;
};

export type DownloadResult = {
  contentId: string;
  filePath: string;
  fileSize: number;
  downloadTime: number;
};

export type TranscriptionJobResult = {
  inputFile: string;
  outputFile: string;
  transcription: TranscriptionResult;
  processingTime: number;
};

export type ProcessingJobResult = {
  inputFile: string;
  outputFile: string;
  result: ProcessingResult;
  processingTime: number;
};

/**
 * Re-export all schemas for validation
 */
export * as Schema from './nnn.schema';
