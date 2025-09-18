/**
 * Authentication service for NNN platform using Puppeteer
 * Handles login flow and session management with functional programming principles
 */

import puppeteer, { Browser, Page } from 'puppeteer';
import * as E from 'fp-ts/Either';
import * as TE from 'fp-ts/TaskEither';
import { pipe } from 'fp-ts/function';
import type { AppError, SessionData, AuthResult } from '../types';
import { CONSTANTS, saveSessionData } from '../config';
import { logger } from '../utils/logger';

/**
 * Browser configuration for Puppeteer
 */
const BROWSER_CONFIG = {
  headless: false, // User needs to log in manually
  defaultViewport: { width: 1280, height: 720 },
  args: [
    '--no-sandbox',
    '--disable-setuid-sandbox',
    '--disable-dev-shm-usage',
    '--disable-accelerated-2d-canvas',
    '--no-first-run',
    '--no-zygote',
    '--disable-gpu',
  ] as string[],
};

/**
 * Wait timeouts in milliseconds
 */
const TIMEOUTS = {
  pageLoad: 30000,
  userAction: 300000, // 5 minutes for user to log in
  navigation: 10000,
  elementVisible: 5000,
} as const;

/**
 * Launch browser with appropriate configuration
 */
const launchBrowser = (): TE.TaskEither<AppError, Browser> => {
  return TE.tryCatch(
    async () => {
      logger.info('🚀 Launching browser for authentication...');
      const browser = await puppeteer.launch(BROWSER_CONFIG);
      return browser;
    },
    (error): AppError => ({
      type: 'ProcessingError',
      message: `Failed to launch browser: ${error instanceof Error ? error.message : 'Unknown error'}`,
      stage: 'browser-launch',
    })
  );
};

/**
 * Navigate to login page
 */
const navigateToLogin = (page: Page): TE.TaskEither<AppError, void> => {
  return TE.tryCatch(
    async () => {
      logger.info(`📍 Navigating to ${CONSTANTS.NNN_LOGIN_URL}`);
      await page.goto(CONSTANTS.NNN_LOGIN_URL, {
        waitUntil: 'networkidle2',
        timeout: TIMEOUTS.pageLoad,
      });
      
      const currentUrl = page.url();
      logger.info(`✅ Login page loaded: ${currentUrl}`);
      logger.info('🔍 Browser window is now ready for manual login');
      
      // Give a moment for the page to fully render
      await new Promise(resolve => setTimeout(resolve, 2000));
    },
    (error): AppError => ({
      type: 'NetworkError',
      message: `Failed to navigate to login page: ${error instanceof Error ? error.message : 'Unknown error'}`,
    })
  );
};

/**
 * Wait for user to complete login manually with terminal confirmation
 */
const waitForUserLogin = (page: Page): TE.TaskEither<AppError, void> => {
  return TE.tryCatch(
    async () => {
      logger.info('👤 Please log in to your NNN account in the browser window...');
      logger.info('🔍 Browser window is now open - please complete your login process');
      logger.info('⏳ After you have successfully logged in, press ENTER in this terminal to continue...');
      
      // Record initial URL for debugging
      const initialUrl = page.url();
      logger.debug(`Initial URL: ${initialUrl}`);
      
      // Wait for user to press Enter in terminal
      await new Promise<void>((resolve) => {
        const readline = require('readline');
        const rl = readline.createInterface({
          input: process.stdin,
          output: process.stdout
        });
        
        rl.question('✅ Press ENTER after you have completed login in the browser: ', () => {
          rl.close();
          resolve();
        });
      });
      
      // Now verify that login was successful
      logger.info('🔄 Verifying authentication status...');
      
      const finalUrl = page.url();
      logger.debug(`Current URL after confirmation: ${finalUrl}`);
      
      // Check if we're still on a login page
      if (finalUrl.includes('login') || finalUrl.includes('signin')) {
        logger.warn('⚠️  Warning: Still appears to be on login page. Please ensure you completed the login process.');
        
        // Ask user to confirm they want to proceed anyway
        const shouldContinue = await new Promise<boolean>((resolve) => {
          const readline = require('readline');
          const rl = readline.createInterface({
            input: process.stdin,
            output: process.stdout
          });
          
          rl.question('🤔 Do you want to proceed anyway? (y/N): ', (answer: string) => {
            rl.close();
            resolve(answer.toLowerCase() === 'y' || answer.toLowerCase() === 'yes');
          });
        });
        
        if (!shouldContinue) {
          throw new Error('Authentication cancelled by user');
        }
      }
      
      // Additional verification by checking for authenticated elements
      let hasAuthElements = false;
      try {
        hasAuthElements = await page.evaluate(`() => {
          const userElements = [
            '[class*="profile"]', '[class*="user-"]', '.user-menu', '.profile-menu',
            '[data-testid*="user"]', '[data-testid*="profile"]',
            'a[href*="logout"]', 'a[href*="signout"]', 'button[class*="logout"]',
            '[class*="avatar"]', '.user-avatar', '.profile-avatar',
            'nav [class*="user"]', 'nav [class*="profile"]',
            '.username', '.user-name', '#username'
          ];
          
          return userElements.some(selector => document.querySelector(selector) !== null);
        }`) as boolean;
      } catch (error) {
        logger.debug('Could not evaluate page elements, skipping DOM check');
        hasAuthElements = false;
      }
      
      if (!hasAuthElements && !finalUrl.includes('course') && !finalUrl.includes('dashboard') && !finalUrl.includes('member')) {
        logger.warn('⚠️  Warning: Could not detect typical authenticated user elements. Authentication may not be complete.');
      } else {
        logger.info('✅ Authentication verification passed');
      }
      
      logger.success('🎉 Authentication process completed!');
      logger.info(`📍 Final URL: ${finalUrl}`);
    },
    (error): AppError => ({
      type: 'AuthenticationError',
      message: `Login timeout or failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
    })
  );
};

/**
 * Extract authentication tokens and cookies from browser
 */
const extractAuthData = (page: Page): TE.TaskEither<AppError, SessionData> => {
  return TE.tryCatch(
    async () => {
      logger.info('🔐 Extracting authentication data...');
      
      // Get all cookies
      const cookies = await page.cookies();
      const cookieString = cookies
        .map(cookie => `${cookie.name}=${cookie.value}`)
        .join('; ');
      
      // Try to extract tokens from localStorage/sessionStorage
      const storageData = await page.evaluate(() => {
        const localStorage = (globalThis as any).window?.localStorage;
        const sessionStorage = (globalThis as any).window?.sessionStorage;
        
        // Common token keys to look for
        const tokenKeys = [
          'token', 'auth_token', 'access_token', 'session_token',
          'authToken', 'accessToken', 'sessionToken',
          'nnn_token', 'nnn_auth', 'user_token'
        ];
        
        let foundToken = '';
        
        // Check localStorage first
        for (const key of tokenKeys) {
          const value = localStorage.getItem(key);
          if (value) {
            foundToken = value;
            break;
          }
        }
        
        // Check sessionStorage if not found in localStorage
        if (!foundToken) {
          for (const key of tokenKeys) {
            const value = sessionStorage.getItem(key);
            if (value) {
              foundToken = value;
              break;
            }
          }
        }
        
        return {
          token: foundToken,
          userId: localStorage.getItem('user_id') || sessionStorage.getItem('user_id'),
          expiresAt: localStorage.getItem('expires_at') || sessionStorage.getItem('expires_at'),
        };
      });
      
      // Create session data
      const sessionData: SessionData = {
        cookies: cookieString,
        token: storageData.token || undefined,
        user_id: storageData.userId || undefined,
        expires_at: storageData.expiresAt || undefined,
        created_at: new Date().toISOString(),
      };
      
      // Validate that we have at least cookies
      if (!sessionData.cookies) {
        throw new Error('No authentication data found - cookies are empty');
      }
      
      logger.success('✅ Authentication data extracted successfully');
      logger.debug(`Extracted ${cookies.length} cookies and token: ${sessionData.token ? '✓' : '✗'}`);
      
      return sessionData;
    },
    (error): AppError => ({
      type: 'ProcessingError',
      message: `Failed to extract auth data: ${error instanceof Error ? error.message : 'Unknown error'}`,
      stage: 'auth-extraction',
    })
  );
};

/**
 * Save session data to file
 */
const saveSession = (sessionData: SessionData): TE.TaskEither<AppError, SessionData> => {
  return TE.tryCatch(
    async () => {
      logger.info('💾 Saving session data...');
      saveSessionData(sessionData);
      logger.success(`✅ Session data saved to ${CONSTANTS.SESSION_FILE}`);
      return sessionData;
    },
    (error): AppError => ({
      type: 'FileSystemError',
      message: `Failed to save session: ${error instanceof Error ? error.message : 'Unknown error'}`,
      path: CONSTANTS.SESSION_FILE,
    })
  );
};

/**
 * Clean up browser instance
 */
const closeBrowser = (browser: Browser): TE.TaskEither<AppError, void> => {
  return TE.tryCatch(
    async () => {
      logger.info('🔒 Closing browser...');
      await browser.close();
      logger.info('✅ Browser closed');
    },
    (error): AppError => ({
      type: 'ProcessingError',
      message: `Failed to close browser: ${error instanceof Error ? error.message : 'Unknown error'}`,
      stage: 'cleanup',
    })
  );
};

/**
 * Complete authentication flow using functional composition
 */
export const authenticateWithNNN = (): TE.TaskEither<AppError, AuthResult> => {
  return pipe(
    launchBrowser(),
    TE.chain((browser) => 
      pipe(
        TE.Do,
        TE.bind('page', () => TE.tryCatch(
          () => browser.newPage(),
          (error): AppError => ({
            type: 'ProcessingError',
            message: `Failed to create page: ${error instanceof Error ? error.message : 'Unknown error'}`,
            stage: 'page-creation',
          })
        )),
        TE.bind('navigation', ({ page }) => navigateToLogin(page)),
        TE.bind('login', ({ page }) => waitForUserLogin(page)),
        TE.bind('sessionData', ({ page }) => extractAuthData(page)),
        TE.bind('savedSession', ({ sessionData }) => saveSession(sessionData)),
        TE.chainFirst(() => closeBrowser(browser)),
        TE.map(({ sessionData }) => ({
          success: true,
          sessionData,
        } as AuthResult)),
        TE.orElse((error) =>
          pipe(
            closeBrowser(browser),
            TE.chain(() => TE.left(error))
          )
        )
      )
    )
  );
};

/**
 * Validate existing session by making a test request
 */
export const validateSession = (): TE.TaskEither<AppError, boolean> => {
  return pipe(
    TE.tryCatch(
      async () => {
        const { loadSessionData } = await import('../config');
        const sessionData = loadSessionData();
        
        if (!sessionData) {
          return false;
        }
        
        // Try to make a simple request to the course list page
        const axios = (await import('axios')).default;
        
        const response = await axios.get(CONSTANTS.NNN_COURSE_LIST_URL, {
          headers: {
            'Cookie': sessionData.cookies,
            'User-Agent': 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36',
          },
          timeout: 10000,
        });
        
        // Check if response looks like a valid course page (not redirected to login)
        const isValid = !response.data.includes('login') && 
                       !response.data.includes('signin') &&
                       response.status === 200;
        
        if (isValid) {
          logger.info('✅ Session is still valid');
        } else {
          logger.warn('⚠️  Session appears to be expired');
        }
        
        return isValid;
      },
      (error): AppError => ({
        type: 'NetworkError',
        message: `Session validation failed: ${error instanceof Error ? error.message : 'Unknown error'}`,
      })
    )
  );
};

/**
 * Helper function to run authentication with proper error handling
 */
export const runAuthentication = async (): Promise<AuthResult> => {
  logger.info('🔐 Starting authentication process...');
  
  const result = await authenticateWithNNN()();
  
  return pipe(
    result,
    E.fold(
      (error: AppError): AuthResult => {
        logger.error(`Authentication failed: ${error.message}`);
        return { success: false, error };
      },
      (authResult: AuthResult): AuthResult => {
        logger.success('🎉 Authentication completed successfully!');
        return authResult;
      }
    )
  );
};
