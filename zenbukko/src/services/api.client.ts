/**
 * API Client service for NNN platform
 * Provides authenticated HTTP client with functional programming patterns
 */

import axios, { AxiosInstance, AxiosRequestConfig, AxiosResponse } from 'axios';
import * as TE from 'fp-ts/TaskEither';
import { pipe } from 'fp-ts/function';
import type { AppError, SessionData } from '../types';
import { CONSTANTS, loadSessionData } from '../config';
import { logger } from '../utils/logger';

/**
 * Default request configuration
 */
const DEFAULT_CONFIG: AxiosRequestConfig = {
  timeout: 30000,
  headers: {
    'Accept': 'application/json, text/plain, */*',
    'Accept-Language': 'ja,en;q=0.9',
    'Cache-Control': 'no-cache',
    'Pragma': 'no-cache',
    'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36',
  },
};

/**
 * Response validation helper
 */
const isSuccessResponse = (response: AxiosResponse): boolean => {
  return response.status >= 200 && response.status < 300;
};

/**
 * Convert axios errors to AppError
 */
const mapAxiosError = (error: any, context?: string): AppError => {
  const prefix = context ? `[${context}] ` : '';
  
  if (axios.isAxiosError(error)) {
    if (error.response) {
      // Server responded with error status
      const statusCode = error.response.status;
      const statusText = error.response.statusText;
      
      if (statusCode === 401) {
        return {
          type: 'AuthenticationError',
          message: `${prefix}Authentication failed - session may be expired`,
        };
      } else if (statusCode === 403) {
        return {
          type: 'AuthenticationError',
          message: `${prefix}Access denied - insufficient permissions`,
        };
      } else if (statusCode >= 400 && statusCode < 500) {
        return {
          type: 'ValidationError',
          message: `${prefix}Client error (${statusCode}): ${statusText}`,
        };
      } else {
        return {
          type: 'NetworkError',
          message: `${prefix}Server error (${statusCode}): ${statusText}`,
          statusCode,
        };
      }
    } else if (error.request) {
      // Network error - no response received
      return {
        type: 'NetworkError',
        message: `${prefix}Network error - no response received`,
      };
    } else {
      // Request setup error
      return {
        type: 'ConfigurationError',
        message: `${prefix}Request configuration error: ${error.message}`,
      };
    }
  }
  
  return {
    type: 'UnknownError',
    message: `${prefix}Unknown HTTP error: ${error instanceof Error ? error.message : 'Unknown error'}`,
  };
};

/**
 * Create authenticated axios instance
 */
const createAuthenticatedClient = (sessionData: SessionData): AxiosInstance => {
  const client = axios.create({
    ...DEFAULT_CONFIG,
    baseURL: CONSTANTS.NNN_API_BASE_URL,
    headers: {
      ...DEFAULT_CONFIG.headers,
      'Cookie': sessionData.cookies,
      ...(sessionData.token && { 'Authorization': `Bearer ${sessionData.token}` }),
    },
  });

  // Add request interceptor for logging
  client.interceptors.request.use(
    (config) => {
      logger.debug(`📤 API Request: ${config.method?.toUpperCase()} ${config.url}`);
      return config;
    },
    (error) => {
      logger.error('📤 Request interceptor error:', error);
      return Promise.reject(error);
    }
  );

  // Add response interceptor for logging and error handling
  client.interceptors.response.use(
    (response) => {
      logger.debug(`📥 API Response: ${response.status} ${response.config.method?.toUpperCase()} ${response.config.url}`);
      return response;
    },
    (error) => {
      if (axios.isAxiosError(error) && error.response) {
        logger.warn(`📥 API Error Response: ${error.response.status} ${error.response.statusText}`);
      } else {
        logger.error('📥 Response interceptor error:', error.message);
      }
      return Promise.reject(error);
    }
  );

  return client;
};

/**
 * Get authenticated client instance
 */
export const getAuthenticatedClient = (): TE.TaskEither<AppError, AxiosInstance> => {
  return TE.tryCatch(
    async () => {
      const sessionData = loadSessionData();
      
      if (!sessionData) {
        throw new Error('No valid session found - please authenticate first');
      }
      
      return createAuthenticatedClient(sessionData);
    },
    (error): AppError => ({
      type: 'AuthenticationError',
      message: `Failed to create authenticated client: ${error instanceof Error ? error.message : 'Unknown error'}`,
    })
  );
};

/**
 * Generic HTTP GET request
 */
export const apiGet = <T = any>(url: string, config?: AxiosRequestConfig): TE.TaskEither<AppError, T> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) =>
      TE.tryCatch(
        async () => {
          const response = await client.get<T>(url, config);
          
          if (!isSuccessResponse(response)) {
            throw new Error(`Unexpected response status: ${response.status}`);
          }
          
          return response.data;
        },
        (error) => mapAxiosError(error, `GET ${url}`)
      )
    )
  );
};

/**
 * Generic HTTP POST request
 */
export const apiPost = <T = any, D = any>(
  url: string, 
  data?: D, 
  config?: AxiosRequestConfig
): TE.TaskEither<AppError, T> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) =>
      TE.tryCatch(
        async () => {
          const response = await client.post<T>(url, data, config);
          
          if (!isSuccessResponse(response)) {
            throw new Error(`Unexpected response status: ${response.status}`);
          }
          
          return response.data;
        },
        (error) => mapAxiosError(error, `POST ${url}`)
      )
    )
  );
};

/**
 * Generic HTTP PUT request
 */
export const apiPut = <T = any, D = any>(
  url: string, 
  data?: D, 
  config?: AxiosRequestConfig
): TE.TaskEither<AppError, T> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) =>
      TE.tryCatch(
        async () => {
          const response = await client.put<T>(url, data, config);
          
          if (!isSuccessResponse(response)) {
            throw new Error(`Unexpected response status: ${response.status}`);
          }
          
          return response.data;
        },
        (error) => mapAxiosError(error, `PUT ${url}`)
      )
    )
  );
};

/**
 * Generic HTTP DELETE request
 */
export const apiDelete = <T = any>(url: string, config?: AxiosRequestConfig): TE.TaskEither<AppError, T> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) =>
      TE.tryCatch(
        async () => {
          const response = await client.delete<T>(url, config);
          
          if (!isSuccessResponse(response)) {
            throw new Error(`Unexpected response status: ${response.status}`);
          }
          
          return response.data;
        },
        (error) => mapAxiosError(error, `DELETE ${url}`)
      )
    )
  );
};

/**
 * Download file from authenticated endpoint
 */
export const apiDownloadFile = (
  url: string, 
  onProgress?: (progress: { loaded: number; total?: number }) => void
): TE.TaskEither<AppError, { data: Buffer; headers: Record<string, any> }> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) =>
      TE.tryCatch(
        async () => {
          const response = await client.get(url, {
            responseType: 'arraybuffer',
            onDownloadProgress: (progressEvent) => {
              if (onProgress && progressEvent.total) {
                onProgress({
                  loaded: progressEvent.loaded,
                  total: progressEvent.total,
                });
              } else if (onProgress) {
                onProgress({
                  loaded: progressEvent.loaded,
                });
              }
            },
          });
          
          if (!isSuccessResponse(response)) {
            throw new Error(`Download failed with status: ${response.status}`);
          }
          
          return {
            data: Buffer.from(response.data),
            headers: response.headers,
          };
        },
        (error) => mapAxiosError(error, `DOWNLOAD ${url}`)
      )
    )
  );
};

/**
 * Stream file download with better memory management for large files
 */
export const apiDownloadStream = (
  url: string,
  onProgress?: (progress: { loaded: number; total?: number }) => void
): TE.TaskEither<AppError, NodeJS.ReadableStream> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) =>
      TE.tryCatch(
        async () => {
          const response = await client.get(url, {
            responseType: 'stream',
            onDownloadProgress: (progressEvent) => {
              if (onProgress && progressEvent.total) {
                onProgress({
                  loaded: progressEvent.loaded,
                  total: progressEvent.total,
                });
              } else if (onProgress) {
                onProgress({
                  loaded: progressEvent.loaded,
                });
              }
            },
          });
          
          if (!isSuccessResponse(response)) {
            throw new Error(`Stream download failed with status: ${response.status}`);
          }
          
          return response.data as NodeJS.ReadableStream;
        },
        (error) => mapAxiosError(error, `STREAM ${url}`)
      )
    )
  );
};

/**
 * Test API connection and authentication
 */
export const testApiConnection = (): TE.TaskEither<AppError, boolean> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) =>
      TE.tryCatch(
        async () => {
          // Try to make a simple request to test authentication
          const response = await client.get('/', {
            timeout: 10000,
            validateStatus: (status) => status < 500, // Don't throw on 4xx errors
          });
          
          // Consider it successful if we don't get auth errors
          const isValid = response.status !== 401 && response.status !== 403;
          
          if (isValid) {
            logger.info('✅ API connection test successful');
          } else {
            logger.warn('⚠️  API connection test failed - authentication issue');
          }
          
          return isValid;
        },
        (error) => mapAxiosError(error, 'CONNECTION_TEST')
      )
    )
  );
};

/**
 * Retry wrapper for API calls with exponential backoff
 */
export const withRetry = <T>(
  operation: TE.TaskEither<AppError, T>,
  maxRetries: number = 3,
  baseDelay: number = 1000
): TE.TaskEither<AppError, T> => {
  const retry = (attempt: number): TE.TaskEither<AppError, T> => {
    return pipe(
      operation,
      TE.orElse((error) => {
        if (attempt >= maxRetries) {
          logger.error(`❌ Operation failed after ${maxRetries} attempts: ${error.message}`);
          return TE.left(error);
        }
        
        // Don't retry authentication errors
        if (error.type === 'AuthenticationError') {
          return TE.left(error);
        }
        
        const delay = baseDelay * Math.pow(2, attempt - 1);
        logger.warn(`⚠️  Attempt ${attempt} failed, retrying in ${delay}ms: ${error.message}`);
        
        return pipe(
          TE.tryCatch(
            () => new Promise(resolve => setTimeout(resolve, delay)),
            (): AppError => ({ type: 'UnknownError', message: 'Delay failed' })
          ),
          TE.chain(() => retry(attempt + 1))
        );
      })
    );
  };
  
  return retry(1);
};
