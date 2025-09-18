/**
 * NNN Platform Service
 * Handles all interactions with NNN API endpoints using functional programming patterns
 */

import * as TE from 'fp-ts/TaskEither';
import * as A from 'fp-ts/Array';
import { pipe } from 'fp-ts/function';
import { z } from 'zod';
import * as cheerio from 'cheerio';
import type { 
  AppError, 
  Course, 
  Chapter, 
  Content,
  CourseResponse,
  ChapterResponse,
  ContentResponse 
} from '../types';
import { 
  CourseResponseSchema,
  ChapterResponseSchema,
  ContentResponseSchema 
} from '../types/nnn.schema';
import { CONSTANTS } from '../config';
import { apiGet, withRetry } from './api.client';
import { logger } from '../utils/logger';

/**
 * Validate API response using Zod schema
 */
const validateResponse = <T>(
  schema: z.ZodSchema<T>,
  data: any,
  endpoint: string
): TE.TaskEither<AppError, T> => {
  return TE.tryCatch(
    async () => {
      try {
        return schema.parse(data);
      } catch (error) {
        if (error instanceof z.ZodError) {
          logger.error(`Validation failed for ${endpoint}:`, error.errors);
          throw new Error(`API response validation failed: ${error.errors.map(e => `${e.path.join('.')}: ${e.message}`).join(', ')}`);
        }
        throw error;
      }
    },
    (error): AppError => ({
      type: 'ValidationError',
      message: `Response validation failed for ${endpoint}: ${error instanceof Error ? error.message : 'Unknown error'}`,
    })
  );
};

/**
 * Fetch list of courses by parsing dynamic HTML from course list page using Puppeteer
 */
export const listCourses = (): TE.TaskEither<AppError, Course[]> => {
  logger.info('📚 Fetching course list from dynamic HTML page...');
  
  return pipe(
    TE.tryCatch(
      async () => {
        const puppeteer = await import('puppeteer');
        const { loadSessionData } = await import('../config');
        
        logger.debug('🔍 Launching Puppeteer to get dynamic course content...');
        
        const browser = await puppeteer.launch({ headless: true });
        const page = await browser.newPage();
        
        // Set cookies from session
        const sessionData = loadSessionData();
        
        if (sessionData?.cookies) {
          // Parse cookies and set them
          const cookieStrings = sessionData.cookies.split(';');
          for (const cookieString of cookieStrings) {
            const parts = cookieString.trim().split('=');
            if (parts.length >= 2 && parts[0]) {
              const name = parts[0].trim();
              const value = parts.slice(1).join('=').trim();
              if (name && value) {
                try {
                  await page.setCookie({
                    name,
                    value,
                    domain: '.nnn.ed.nico'
                  });
                } catch (error) {
                  logger.debug(`Failed to set cookie ${name}:`, error);
                }
              }
            }
          }
        }
        
        await page.goto('https://www.nnn.ed.nico/my_course?tab=zen_univ', {
          waitUntil: 'networkidle2',
          timeout: 30000
        });
        
        // Wait for dynamic content to load
        logger.debug('⏳ Waiting for dynamic content to load...');
        await new Promise(resolve => setTimeout(resolve, 3000));
        
        // Get the final HTML after JavaScript execution
        const dynamicHtml = await page.content();
        await browser.close();
        
        return dynamicHtml;
      },
      (error): AppError => ({
        type: 'ProcessingError',
        message: `Failed to fetch dynamic course page: ${error instanceof Error ? error.message : String(error)}`,
      })
    ),
    TE.chainW((htmlContent: string) => TE.tryCatch(
      async () => {
        logger.debug('Received dynamic HTML content length:', htmlContent.length);
        
        const $ = cheerio.load(htmlContent);
        const courses: Course[] = [];
        
        // Look for course links with the pattern /courses/{id}
        const courseLinks = $('a[href*="/courses/"]');
        logger.debug(`Found ${courseLinks.length} course links`);
        
        courseLinks.each((_index, element) => {
          const $link = $(element);
          const href = $link.attr('href') || '';
          const ariaLabel = $link.attr('aria-label') || '';
          
          // Extract course ID from URL
          const courseIdMatch = href.match(/\/courses\/(\d+)/);
          if (!courseIdMatch) return;
          
          const courseId = courseIdMatch[1];
          
          // Get title from aria-label or h4 element
          let title = ariaLabel;
          if (!title) {
            const h4 = $link.find('h4').first();
            title = h4.text().trim();
          }
          
          if (!title) {
            title = $link.text().trim();
          }
          
          if (title && courseId) {
            // Check if we already have this course (avoid duplicates)
            const existingCourse = courses.find(c => c.b_course_id === courseId);
            if (!existingCourse) {
              courses.push({
                id: `course_${courseId}`,
                b_course_id: courseId,
                title: title.trim(),
                description: `Course: ${title.trim()}`,
                thumbnail: '',
                created_at: new Date().toISOString(),
                updated_at: new Date().toISOString(),
                status: 'active' as const,
                chapters: [],
              });
              
              logger.debug(`Found course: ${title} (ID: ${courseId})`);
            }
          }
        });
        
        logger.info(`✅ Parsed ${courses.length} courses from dynamic HTML`);
        return courses;
      },
      (error): AppError => ({
        type: 'ProcessingError',
        message: `Failed to parse dynamic course list HTML: ${error instanceof Error ? error.message : String(error)}`,
        stage: 'html-parsing',
      })
    ))
  );
};

/**
 * Fetch detailed information about a specific course
 */
export const getCourseDetails = (bCourseId: string): TE.TaskEither<AppError, CourseResponse> => {
  logger.info(`📖 Fetching course details for: ${bCourseId}`);
  
  const endpoint = CONSTANTS.API_ENDPOINTS.COURSE_DETAILS(bCourseId);
  
  return pipe(
    withRetry(apiGet<any>(endpoint), CONSTANTS.DEFAULTS.RETRY_ATTEMPTS),
    TE.chainW((response) => validateResponse(CourseResponseSchema, response, `getCourseDetails(${bCourseId})`)),
    TE.map((validatedResponse) => {
      logger.success(`✅ Course details fetched: ${validatedResponse.data.title}`);
      return validatedResponse;
    }),
    TE.mapLeft((error) => ({
      ...error,
      message: `Failed to fetch course ${bCourseId}: ${error.message}`,
    }))
  );
};

/**
 * Fetch detailed information about a specific chapter
 */
export const getChapterDetails = (bCourseId: string, chapterId: string): TE.TaskEither<AppError, ChapterResponse> => {
  logger.info(`📑 Fetching chapter details: ${chapterId} from course ${bCourseId}`);
  
  const endpoint = CONSTANTS.API_ENDPOINTS.CHAPTER_DETAILS(bCourseId, chapterId);
  
  return pipe(
    withRetry(apiGet<any>(endpoint), CONSTANTS.DEFAULTS.RETRY_ATTEMPTS),
    TE.chainW((response) => validateResponse(ChapterResponseSchema, response, `getChapterDetails(${bCourseId}, ${chapterId})`)),
    TE.map((validatedResponse) => {
      logger.success(`✅ Chapter details fetched: ${validatedResponse.data.title}`);
      return validatedResponse;
    }),
    TE.mapLeft((error) => ({
      ...error,
      message: `Failed to fetch chapter ${chapterId} from course ${bCourseId}: ${error.message}`,
    }))
  );
};

/**
 * Fetch detailed information about a specific content item
 */
export const getContentDetails = (
  courseId: string, 
  chapterId: string, 
  contentId: string
): TE.TaskEither<AppError, ContentResponse> => {
  logger.info(`🎬 Fetching content details: ${contentId} from chapter ${chapterId}`);
  
  const endpoint = CONSTANTS.API_ENDPOINTS.CONTENT_DETAILS(courseId, chapterId, contentId);
  
  return pipe(
    withRetry(apiGet<any>(endpoint), CONSTANTS.DEFAULTS.RETRY_ATTEMPTS),
    TE.chainW((response) => validateResponse(ContentResponseSchema, response, `getContentDetails(${courseId}, ${chapterId}, ${contentId})`)),
    TE.map((validatedResponse) => {
      logger.success(`✅ Content details fetched: ${validatedResponse.data.title}`);
      return validatedResponse;
    }),
    TE.mapLeft((error) => ({
      ...error,
      message: `Failed to fetch content ${contentId} from chapter ${chapterId}: ${error.message}`,
    }))
  );
};

/**
 * Get all chapters for a course
 */
export const getAllChapters = (bCourseId: string): TE.TaskEither<AppError, Chapter[]> => {
  logger.info(`📖 Fetching all chapters for course: ${bCourseId}`);
  
  return pipe(
    getCourseDetails(bCourseId),
    TE.map((courseResponse) => courseResponse.chapters || []),
    TE.map((chapters) => {
      logger.success(`✅ Found ${chapters.length} chapters in course`);
      return chapters;
    })
  );
};

/**
 * Get all content items for a chapter
 */
export const getAllContentForChapter = (
  bCourseId: string, 
  chapterId: string
): TE.TaskEither<AppError, Content[]> => {
  logger.info(`📑 Fetching all content for chapter: ${chapterId}`);
  
  return pipe(
    getChapterDetails(bCourseId, chapterId),
    TE.map((chapterResponse) => chapterResponse.contents || []),
    TE.map((contents) => {
      logger.success(`✅ Found ${contents.length} content items in chapter`);
      return contents;
    })
  );
};

/**
 * Get complete course structure with all chapters and content
 */
export const getCompleteCourseStructure = (bCourseId: string): TE.TaskEither<AppError, {
  course: Course;
  chapters: Array<{
    chapter: Chapter;
    contents: Content[];
  }>;
}> => {
  logger.info(`🏗️  Building complete course structure for: ${bCourseId}`);
  
  return pipe(
    getCourseDetails(bCourseId),
    TE.chain((courseResponse) => {
      const course = courseResponse.data;
      const chapters = courseResponse.chapters || [];
      
      // Fetch all content for each chapter
      const fetchAllContent = pipe(
        chapters,
        A.map((chapter) => 
          pipe(
            getAllContentForChapter(bCourseId, chapter.chapter_id),
            TE.map((contents) => ({ chapter, contents }))
          )
        ),
        A.sequence(TE.ApplicativeSeq)
      );
      
      return pipe(
        fetchAllContent,
        TE.map((chaptersWithContent) => ({
          course,
          chapters: chaptersWithContent,
        }))
      );
    }),
    TE.map((result) => {
      const totalContent = result.chapters.reduce((sum, ch) => sum + ch.contents.length, 0);
      logger.success(`✅ Complete course structure built: ${result.chapters.length} chapters, ${totalContent} content items`);
      return result;
    })
  );
};

/**
 * Get downloadable content URLs from course structure
 */
export const getDownloadableUrls = (bCourseId: string): TE.TaskEither<AppError, Array<{
  content: Content;
  chapter: Chapter;
  course: Course;
  downloadUrl: string;
}>> => {
  logger.info(`🔗 Extracting downloadable URLs for course: ${bCourseId}`);
  
  return pipe(
    getCompleteCourseStructure(bCourseId),
    TE.map(({ course, chapters }) => {
      const downloadableItems: Array<{
        content: Content;
        chapter: Chapter;
        course: Course;
        downloadUrl: string;
      }> = [];
      
      for (const { chapter, contents } of chapters) {
        for (const content of contents) {
          // Check if content has a downloadable URL
          if (content.file_url || content.stream_url) {
            downloadableItems.push({
              content,
              chapter,
              course,
              downloadUrl: content.file_url || content.stream_url!,
            });
          }
        }
      }
      
      logger.success(`✅ Found ${downloadableItems.length} downloadable items`);
      return downloadableItems;
    })
  );
};

/**
 * Search for content by title or description
 */
export const searchContent = (
  bCourseId: string, 
  searchTerm: string
): TE.TaskEither<AppError, Array<{
  content: Content;
  chapter: Chapter;
  relevanceScore: number;
}>> => {
  logger.info(`🔍 Searching content in course ${bCourseId} for: "${searchTerm}"`);
  
  const calculateRelevance = (content: Content, chapter: Chapter, term: string): number => {
    const lowerTerm = term.toLowerCase();
    let score = 0;
    
    // Title match (highest priority)
    if (content.title.toLowerCase().includes(lowerTerm)) score += 10;
    if (chapter.title.toLowerCase().includes(lowerTerm)) score += 5;
    
    // Description match
    if (content.description?.toLowerCase().includes(lowerTerm)) score += 3;
    if (chapter.description?.toLowerCase().includes(lowerTerm)) score += 2;
    
    // Exact match bonus
    if (content.title.toLowerCase() === lowerTerm) score += 20;
    
    return score;
  };
  
  return pipe(
    getCompleteCourseStructure(bCourseId),
    TE.map(({ chapters }) => {
      const results: Array<{
        content: Content;
        chapter: Chapter;
        relevanceScore: number;
      }> = [];
      
      for (const { chapter, contents } of chapters) {
        for (const content of contents) {
          const score = calculateRelevance(content, chapter, searchTerm);
          if (score > 0) {
            results.push({ content, chapter, relevanceScore: score });
          }
        }
      }
      
      // Sort by relevance score (highest first)
      results.sort((a, b) => b.relevanceScore - a.relevanceScore);
      
      logger.success(`✅ Found ${results.length} matching content items`);
      return results;
    })
  );
};

/**
 * Check if API endpoints are accessible (health check)
 */
export const checkApiHealth = (): TE.TaskEither<AppError, {
  isHealthy: boolean;
  availableEndpoints: string[];
  errors: string[];
}> => {
  logger.info('🏥 Checking NNN API health...');
  
  const testEndpoints = [
    { name: 'base', url: '/' },
    { name: 'courses', url: '/courses' },
    { name: 'material', url: '/material' },
  ];
  
  type EndpointResult = {
    name: string;
    url: string;
    success: boolean;
    error: string | null;
  };
  
  return pipe(
    testEndpoints,
    A.map(({ name, url }): TE.TaskEither<never, EndpointResult> => 
      pipe(
        apiGet(url, { timeout: 5000 }),
        TE.map((): EndpointResult => ({ name, url, success: true, error: null })),
        TE.orElse((error): TE.TaskEither<never, EndpointResult> => 
          TE.right({ name, url, success: false, error: error.message })
        )
      )
    ),
    A.sequence(TE.ApplicativeSeq),
    TE.map((results) => {
      const successful = results.filter(r => r.success);
      const failed = results.filter(r => !r.success);
      
      const healthStatus = {
        isHealthy: successful.length > 0,
        availableEndpoints: successful.map(r => r.url),
        errors: failed.map(r => `${r.name}: ${r.error}`),
      };
      
      if (healthStatus.isHealthy) {
        logger.success(`✅ API health check passed: ${successful.length}/${results.length} endpoints available`);
      } else {
        logger.error(`❌ API health check failed: all endpoints unavailable`);
      }
      
      return healthStatus;
    })
  );
};
