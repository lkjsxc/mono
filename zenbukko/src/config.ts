import { config } from 'dotenv';
import * as path from 'path';
import * as fs from 'fs';
import { z } from 'zod';
import type { AppConfig, SessionData } from './types';
import { SessionDataSchema } from './types/nnn.schema';

// Load environment variables
config();

/**
 * Schema for environment variables validation
 */
const EnvSchema = z.object({
  GEMINI_API_KEY: z.string().min(1, 'GEMINI_API_KEY is required'),
  NNN_SESSION_TOKEN: z.string().optional(),
  NNN_COOKIES: z.string().optional(),
  DEFAULT_OUTPUT_DIR: z.string().default('./downloads'),
  WHISPER_MODEL: z.string().default('base.en'),
  LOG_LEVEL: z.enum(['debug', 'info', 'warn', 'error']).default('info'),
  NODE_ENV: z.string().default('development'),
});

/**
 * Parse and validate environment variables
 */
const parseEnv = () => {
  try {
    return EnvSchema.parse(process.env);
  } catch (error) {
    console.error('❌ Environment validation failed:');
    if (error instanceof z.ZodError) {
      error.errors.forEach(err => {
        console.error(`  - ${err.path.join('.')}: ${err.message}`);
      });
    }
    process.exit(1);
  }
};

const env = parseEnv();

/**
 * Application constants and paths
 */
export const CONSTANTS = {
  // NNN Platform URLs
  NNN_LOGIN_URL: 'https://www.nnn.ed.nico/home',
  NNN_COURSE_LIST_URL: 'https://www.nnn.ed.nico/my_course?tab=zen_univ',
  NNN_API_BASE_URL: 'https://api.nnn.ed.nico/v2',
  
  // File paths
  PROJECT_ROOT: process.cwd(),
  SESSION_FILE: path.join(process.cwd(), 'session.json'),
  WHISPER_DIR: path.join(process.cwd(), 'whisper.cpp'),
  DEFAULT_DOWNLOADS_DIR: path.resolve(env.DEFAULT_OUTPUT_DIR),
  
  // Whisper configuration
  WHISPER_EXECUTABLE: path.join(process.cwd(), 'whisper.cpp', 'main'),
  WHISPER_MODELS_DIR: path.join(process.cwd(), 'whisper.cpp', 'models'),
  
  // API endpoints patterns
  API_ENDPOINTS: {
    COURSE_DETAILS: (bCourseId: string) => `/material/courses/${bCourseId}?revision=1`,
    CHAPTER_DETAILS: (bCourseId: string, chapterId: string) => 
      `/material/courses/${bCourseId}/chapters/${chapterId}?revision=1`,
    CONTENT_DETAILS: (courseId: string, chapterId: string, contentId: string) => 
      `/material/courses/${courseId}/chapters/${chapterId}/movies/${contentId}?revision=1`,
  },
  
  // Default settings
  DEFAULTS: {
    WHISPER_MODEL: env.WHISPER_MODEL,
    OUTPUT_FORMAT: 'txt' as const,
    GEMINI_MODEL: 'gemini-1.5-flash',
    GEMINI_TEMPERATURE: 0.3,
    GEMINI_MAX_TOKENS: 8192,
    DOWNLOAD_TIMEOUT: 30000,
    RETRY_ATTEMPTS: 3,
    CONCURRENT_DOWNLOADS: 3,
  }
} as const;

/**
 * Load session data from file
 * @returns Session data or null if not found/invalid
 */
export const loadSessionData = (): SessionData | null => {
  try {
    if (!fs.existsSync(CONSTANTS.SESSION_FILE)) {
      return null;
    }
    
    const sessionRaw = fs.readFileSync(CONSTANTS.SESSION_FILE, 'utf-8');
    const sessionData = JSON.parse(sessionRaw);
    
    // Validate session data structure
    const validatedSession = SessionDataSchema.parse(sessionData);
    
    // Check if session is expired
    if (validatedSession.expires_at) {
      const expiryDate = new Date(validatedSession.expires_at);
      if (expiryDate <= new Date()) {
        console.warn('⚠️  Session has expired, please re-authenticate');
        return null;
      }
    }
    
    return validatedSession;
  } catch (error) {
    console.warn('⚠️  Failed to load session data:', error instanceof Error ? error.message : 'Unknown error');
    return null;
  }
};

/**
 * Save session data to file
 * @param sessionData - Session data to save
 */
export const saveSessionData = (sessionData: SessionData): void => {
  try {
    // Validate before saving
    const validatedSession = SessionDataSchema.parse(sessionData);
    fs.writeFileSync(CONSTANTS.SESSION_FILE, JSON.stringify(validatedSession, null, 2));
  } catch (error) {
    throw new Error(`Failed to save session data: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Check if authentication is valid
 */
export const isAuthValid = (): boolean => {
  const session = loadSessionData();
  return session !== null;
};

/**
 * Get complete application configuration
 */
export const getAppConfig = (): AppConfig => {
  const session = loadSessionData();
  
  return {
    defaultOutputDir: CONSTANTS.DEFAULT_DOWNLOADS_DIR,
    whisper: {
      executablePath: CONSTANTS.WHISPER_EXECUTABLE,
      modelPath: path.join(CONSTANTS.WHISPER_MODELS_DIR, `ggml-${CONSTANTS.DEFAULTS.WHISPER_MODEL}.bin`),
      model: CONSTANTS.DEFAULTS.WHISPER_MODEL,
      outputFormat: CONSTANTS.DEFAULTS.OUTPUT_FORMAT,
    },
    gemini: {
      apiKey: env.GEMINI_API_KEY,
      model: CONSTANTS.DEFAULTS.GEMINI_MODEL,
      temperature: CONSTANTS.DEFAULTS.GEMINI_TEMPERATURE,
      maxTokens: CONSTANTS.DEFAULTS.GEMINI_MAX_TOKENS,
    },
    auth: {
      sessionFile: CONSTANTS.SESSION_FILE,
      cookieString: session?.cookies || env.NNN_COOKIES || '',
      token: session?.token || env.NNN_SESSION_TOKEN || undefined,
      isValid: isAuthValid(),
    },
    logLevel: env.LOG_LEVEL,
  };
};

/**
 * Ensure required directories exist
 */
export const ensureDirectories = (): void => {
  const dirs = [
    CONSTANTS.DEFAULT_DOWNLOADS_DIR,
    path.dirname(CONSTANTS.SESSION_FILE),
  ];
  
  dirs.forEach(dir => {
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
  });
};

/**
 * Check if Whisper.cpp is set up correctly
 */
export const isWhisperSetup = (): boolean => {
  return fs.existsSync(CONSTANTS.WHISPER_EXECUTABLE) && 
         fs.existsSync(CONSTANTS.WHISPER_MODELS_DIR);
};

/**
 * Environment and runtime information
 */
export const RUNTIME_INFO = {
  nodeVersion: process.version,
  platform: process.platform,
  arch: process.arch,
  isProduction: env.NODE_ENV === 'production',
  isDevelopment: env.NODE_ENV === 'development',
  projectVersion: require('../package.json').version,
} as const;

// Initialize directories on module load
ensureDirectories();
