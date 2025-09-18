import { z } from 'zod';

/**
 * Schema for NNN Course data structure
 */
export const CourseSchema = z.object({
  id: z.string(),
  b_course_id: z.string(),
  title: z.string(),
  description: z.string().optional(),
  thumbnail: z.string().optional(),
  created_at: z.string(),
  updated_at: z.string(),
  status: z.enum(['active', 'inactive', 'draft']),
  chapters: z.array(z.string()).optional(),
});

/**
 * Schema for NNN Chapter data structure
 */
export const ChapterSchema = z.object({
  id: z.string(),
  chapter_id: z.string(),
  title: z.string(),
  description: z.string().optional(),
  order: z.number(),
  course_id: z.string(),
  contents: z.array(z.string()).optional(),
  created_at: z.string(),
  updated_at: z.string(),
});

/**
 * Schema for NNN Content item (movie/material)
 */
export const ContentSchema = z.object({
  id: z.string(),
  content_id: z.string(),
  title: z.string(),
  description: z.string().optional(),
  type: z.enum(['movie', 'material', 'text', 'pdf', 'document']),
  file_url: z.string().optional(),
  stream_url: z.string().optional(),
  duration: z.number().optional(),
  file_size: z.number().optional(),
  mime_type: z.string().optional(),
  chapter_id: z.string(),
  course_id: z.string(),
  order: z.number(),
  created_at: z.string(),
  updated_at: z.string(),
});

/**
 * Schema for NNN API Course response
 */
export const CourseResponseSchema = z.object({
  success: z.boolean(),
  data: CourseSchema,
  chapters: z.array(ChapterSchema).optional(),
  message: z.string().optional(),
});

/**
 * Schema for NNN API Chapter response
 */
export const ChapterResponseSchema = z.object({
  success: z.boolean(),
  data: ChapterSchema,
  contents: z.array(ContentSchema).optional(),
  message: z.string().optional(),
});

/**
 * Schema for NNN API Content response
 */
export const ContentResponseSchema = z.object({
  success: z.boolean(),
  data: ContentSchema,
  message: z.string().optional(),
});

/**
 * Schema for NNN Course list response
 */
export const CourseListResponseSchema = z.object({
  success: z.boolean(),
  data: z.array(CourseSchema),
  total: z.number().optional(),
  page: z.number().optional(),
  limit: z.number().optional(),
  message: z.string().optional(),
});

/**
 * Schema for authentication session data
 */
export const SessionDataSchema = z.object({
  cookies: z.string(),
  token: z.string().optional(),
  user_id: z.string().optional(),
  expires_at: z.string().optional(),
  created_at: z.string(),
});

/**
 * Schema for CLI command arguments - download command
 */
export const DownloadArgsSchema = z.object({
  courseId: z.string().min(1, 'Course ID is required'),
  outputDir: z.string().default('./downloads'),
  includeTranscription: z.boolean().default(false),
  processWithAI: z.boolean().default(false),
});

/**
 * Schema for CLI command arguments - transcribe command
 */
export const TranscribeArgsSchema = z.object({
  file: z.string().min(1, 'Video file path is required'),
  model: z.string().default('base.en'),
  outputFormat: z.enum(['txt', 'srt', 'vtt']).default('txt'),
  language: z.string().optional(),
});

/**
 * Schema for download progress tracking
 */
export const DownloadProgressSchema = z.object({
  courseId: z.string(),
  courseName: z.string(),
  totalChapters: z.number(),
  completedChapters: z.number(),
  totalContent: z.number(),
  downloadedContent: z.number(),
  currentChapter: z.string().optional(),
  currentContent: z.string().optional(),
  startedAt: z.string(),
  estimatedTimeRemaining: z.number().optional(),
});

/**
 * Schema for error responses from NNN API
 */
export const ErrorResponseSchema = z.object({
  success: z.literal(false),
  error: z.object({
    code: z.string(),
    message: z.string(),
    details: z.any().optional(),
  }),
  message: z.string().optional(),
});

/**
 * Schema for Whisper.cpp transcription result
 */
export const TranscriptionResultSchema = z.object({
  text: z.string(),
  segments: z.array(z.object({
    start: z.number(),
    end: z.number(),
    text: z.string(),
  })).optional(),
  language: z.string().optional(),
  duration: z.number().optional(),
});

/**
 * Schema for Gemini API processing result
 */
export const ProcessingResultSchema = z.object({
  originalContent: z.string(),
  processedContent: z.string(),
  summary: z.string().optional(),
  keyPoints: z.array(z.string()).optional(),
  processingModel: z.string(),
  processedAt: z.string(),
});

/**
 * Union schema for all possible NNN API responses
 */
export const NNNApiResponseSchema = z.union([
  CourseResponseSchema,
  ChapterResponseSchema,
  ContentResponseSchema,
  CourseListResponseSchema,
  ErrorResponseSchema,
]);
