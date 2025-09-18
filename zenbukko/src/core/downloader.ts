/**
 * File Downloader Core Logic
 * Handles downloading files with functional programming patterns
 */

import * as TE from 'fp-ts/TaskEither';
import * as A from 'fp-ts/Array';
import { pipe } from 'fp-ts/function';
import * as path from 'path';
import type { 
  AppError, 
  DownloadResult, 
  Content, 
  Chapter, 
  Course
} from '../types';
import { apiDownloadFile } from '../services/api.client';
import { 
  ensureParentDir, 
  sanitizeFilename, 
  generateUniqueFilename, 
  getFileExtension,
  formatFileSize,
  exists
} from '../utils/files';
import { logger } from '../utils/logger';
import { CONSTANTS } from '../config';

/**
 * Download options and configuration
 */
export interface DownloadOptions {
  outputDir: string;
  overwrite?: boolean;
  createSubdirectories?: boolean;
  maxConcurrency?: number;
  retryAttempts?: number;
  onProgress?: (progress: DownloadProgress) => void;
}

export interface DownloadProgress {
  contentId: string;
  contentTitle: string;
  chapterTitle: string;
  courseTitle: string;
  bytesDownloaded: number;
  totalBytes?: number;
  percentage: number;
  speed?: number;
  estimatedTimeRemaining?: number;
  filePath: string;
}

/**
 * Generate safe file path for downloaded content
 */
const generateFilePath = (
  content: Content,
  chapter: Chapter,
  course: Course,
  outputDir: string,
  createSubdirs: boolean = true
): string => {
  // Sanitize all components
  const safeCourseTitle = sanitizeFilename(course.title);
  const safeChapterTitle = sanitizeFilename(chapter.title);
  const safeContentTitle = sanitizeFilename(content.title);
  
  // Determine file extension
  let extension = '';
  if (content.file_url || content.stream_url) {
    const url = content.file_url || content.stream_url!;
    extension = getFileExtension(url) || '';
    
    // If no extension from URL, try to infer from content type
    if (!extension && content.mime_type) {
      const mimeToExt: Record<string, string> = {
        'video/mp4': '.mp4',
        'video/avi': '.avi',
        'video/mov': '.mov',
        'video/wmv': '.wmv',
        'application/pdf': '.pdf',
        'text/plain': '.txt',
        'application/msword': '.doc',
        'application/vnd.openxmlformats-officedocument.wordprocessingml.document': '.docx',
        'application/vnd.ms-powerpoint': '.ppt',
        'application/vnd.openxmlformats-officedocument.presentationml.presentation': '.pptx',
      };
      extension = mimeToExt[content.mime_type] || '';
    }
  }
  
  // Build file path
  const filename = `${safeContentTitle}${extension}`;
  
  if (createSubdirs) {
    return path.join(outputDir, safeCourseTitle, safeChapterTitle, filename);
  } else {
    return path.join(outputDir, `${safeCourseTitle}_${safeChapterTitle}_${filename}`);
  }
};

/**
 * Download a single content item
 */
const downloadContent = (
  content: Content,
  chapter: Chapter,
  course: Course,
  options: DownloadOptions
): TE.TaskEither<AppError, DownloadResult> => {
  const downloadUrl = content.file_url || content.stream_url;
  
  if (!downloadUrl) {
    return TE.left({
      type: 'ValidationError',
      message: `No download URL found for content: ${content.title}`,
      field: 'file_url',
    });
  }
  
  const filePath = generateFilePath(
    content, 
    chapter, 
    course, 
    options.outputDir, 
    options.createSubdirectories ?? true
  );
  
  logger.info(`📥 Downloading: ${content.title}`);
  logger.debug(`    URL: ${downloadUrl}`);
  logger.debug(`    Path: ${filePath}`);
  
  return pipe(
    TE.tryCatch(
      async () => {
        // Check if file already exists
        if (!options.overwrite && await exists(filePath)) {
          const uniquePath = await generateUniqueFilename(filePath);
          logger.warn(`⚠️  File exists, using unique name: ${path.basename(uniquePath)}`);
          return uniquePath;
        }
        return filePath;
      },
      (error): AppError => ({
        type: 'FileSystemError',
        message: `Failed to prepare file path: ${error instanceof Error ? error.message : 'Unknown error'}`,
        path: filePath,
      })
    ),
    TE.chain((finalPath) => {
      const startTime = Date.now();
      let lastProgress = 0;
      let lastTime = startTime;
      
      const onProgress = (progressData: { loaded: number; total?: number }) => {
        const { loaded: downloaded, total } = progressData;
        const now = Date.now();
        const timeDiff = now - lastTime;
        const bytesDiff = downloaded - lastProgress;
        
        // Calculate speed (bytes per second)
        const speed = timeDiff > 0 ? (bytesDiff / timeDiff) * 1000 : 0;
        
        // Calculate ETA
        const estimatedTimeRemaining = total && speed > 0 
          ? ((total - downloaded) / speed) * 1000 
          : undefined;
        
        const percentage = total ? (downloaded / total) * 100 : 0;
        
        const progress: DownloadProgress = {
          contentId: content.content_id,
          contentTitle: content.title,
          chapterTitle: chapter.title,
          courseTitle: course.title,
          bytesDownloaded: downloaded,
          ...(total !== undefined && { totalBytes: total }),
          percentage,
          speed,
          ...(estimatedTimeRemaining !== undefined && { estimatedTimeRemaining }),
          filePath: finalPath,
        };
        
        options.onProgress?.(progress);
        
        lastProgress = downloaded;
        lastTime = now;
      };
      
      return pipe(
        apiDownloadFile(downloadUrl, onProgress),
        TE.chain(({ data }) =>
          TE.tryCatch(
            async () => {
              await ensureParentDir(finalPath);
              const fs = await import('fs/promises');
              await fs.writeFile(finalPath, data);
              
              const endTime = Date.now();
              const downloadTime = endTime - startTime;
              
              logger.success(`✅ Downloaded: ${content.title} (${formatFileSize(data.length)}) in ${downloadTime}ms`);
              
              return {
                contentId: content.content_id,
                filePath: finalPath,
                fileSize: data.length,
                downloadTime,
              } as DownloadResult;
            },
            (error): AppError => ({
              type: 'FileSystemError',
              message: `Failed to save file: ${error instanceof Error ? error.message : 'Unknown error'}`,
              path: finalPath,
            })
          )
        )
      );
    }),
    TE.mapLeft((error) => ({
      ...error,
      message: `Failed to download ${content.title}: ${error.message}`,
    }))
  );
};

/**
 * Download multiple content items with concurrency control
 */
export const downloadContentBatch = (
  downloadItems: Array<{
    content: Content;
    chapter: Chapter;
    course: Course;
  }>,
  options: DownloadOptions
): TE.TaskEither<AppError, DownloadResult[]> => {
  logger.info(`📦 Starting batch download of ${downloadItems.length} items`);
  
  const maxConcurrency = options.maxConcurrency || CONSTANTS.DEFAULTS.CONCURRENT_DOWNLOADS;
  
  // Split items into chunks based on concurrency limit
  const chunks: Array<typeof downloadItems> = [];
  for (let i = 0; i < downloadItems.length; i += maxConcurrency) {
    chunks.push(downloadItems.slice(i, i + maxConcurrency));
  }
  
  const downloadChunk = (chunk: typeof downloadItems): TE.TaskEither<AppError, DownloadResult[]> => {
    const downloads = chunk.map(({ content, chapter, course }) =>
      downloadContent(content, chapter, course, options)
    );
    
    return A.sequence(TE.ApplicativeSeq)(downloads);
  };
  
  // Process chunks sequentially to respect concurrency limits
  return pipe(
    chunks,
    A.reduce(
      TE.right<AppError, DownloadResult[]>([]),
      (acc, chunk) => pipe(
        acc,
        TE.chain((results) => pipe(
          downloadChunk(chunk),
          TE.map((chunkResults) => [...results, ...chunkResults])
        ))
      )
    ),
    TE.map((results) => {
      const totalSize = results.reduce((sum, r) => sum + r.fileSize, 0);
      const totalTime = results.reduce((max, r) => Math.max(max, r.downloadTime), 0);
      
      logger.success(`✅ Batch download completed: ${results.length} files, ${formatFileSize(totalSize)}, ${totalTime}ms`);
      return results;
    })
  );
};

/**
 * Download entire course with progress tracking
 */
export const downloadCourse = (
  courseStructure: {
    course: Course;
    chapters: Array<{
      chapter: Chapter;
      contents: Content[];
    }>;
  },
  options: DownloadOptions
): TE.TaskEither<AppError, {
  course: Course;
  results: DownloadResult[];
  summary: {
    totalFiles: number;
    successfulDownloads: number;
    failedDownloads: number;
    totalSize: number;
    totalTime: number;
  };
}> => {
  logger.info(`🚀 Starting course download: ${courseStructure.course.title}`);
  
  // Flatten course structure into downloadable items
  const downloadItems = courseStructure.chapters.flatMap(({ chapter, contents }) =>
    contents
      .filter(content => content.file_url || content.stream_url)
      .map(content => ({ content, chapter, course: courseStructure.course }))
  );
  
  if (downloadItems.length === 0) {
    return TE.left({
      type: 'ValidationError',
      message: 'No downloadable content found in course',
    });
  }
  
  logger.info(`📊 Found ${downloadItems.length} downloadable items across ${courseStructure.chapters.length} chapters`);
  
  const startTime = Date.now();
  
  return pipe(
    downloadContentBatch(downloadItems, options),
    TE.map((results) => {
      const endTime = Date.now();
      const totalTime = endTime - startTime;
      const totalSize = results.reduce((sum, r) => sum + r.fileSize, 0);
      
      const summary = {
        totalFiles: downloadItems.length,
        successfulDownloads: results.length,
        failedDownloads: downloadItems.length - results.length,
        totalSize,
        totalTime,
      };
      
      logger.success(`🎉 Course download completed!`);
      logger.success(`   Files: ${summary.successfulDownloads}/${summary.totalFiles}`);
      logger.success(`   Size: ${formatFileSize(summary.totalSize)}`);
      logger.success(`   Time: ${(summary.totalTime / 1000).toFixed(2)}s`);
      
      return {
        course: courseStructure.course,
        results,
        summary,
      };
    }),
    TE.mapLeft((error) => ({
      ...error,
      message: `Course download failed: ${error.message}`,
    }))
  );
};

/**
 * Create download report
 */
export const createDownloadReport = (
  results: DownloadResult[],
  outputDir: string
): TE.TaskEither<AppError, string> => {
  const reportPath = path.join(outputDir, 'download_report.json');
  
  const report = {
    timestamp: new Date().toISOString(),
    totalFiles: results.length,
    files: results.map(result => ({
      contentId: result.contentId,
      filePath: result.filePath,
      fileSize: result.fileSize,
      downloadTime: result.downloadTime,
      humanFileSize: formatFileSize(result.fileSize),
    })),
    summary: {
      totalSize: results.reduce((sum, r) => sum + r.fileSize, 0),
      averageDownloadTime: results.length > 0 
        ? results.reduce((sum, r) => sum + r.downloadTime, 0) / results.length 
        : 0,
    },
  };
  
  return pipe(
    TE.tryCatch(
      async () => {
        const { writeJsonFile } = await import('../utils/files');
        await writeJsonFile(reportPath, report);
        logger.info(`📋 Download report saved: ${reportPath}`);
        return reportPath;
      },
      (error): AppError => ({
        type: 'FileSystemError',
        message: `Failed to create download report: ${error instanceof Error ? error.message : 'Unknown error'}`,
        path: reportPath,
      })
    )
  );
};
