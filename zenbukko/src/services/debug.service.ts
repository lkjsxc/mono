/**
 * Debug utility to save HTML content and examine page structure
 */
import * as TE from 'fp-ts/TaskEither';
import { pipe } from 'fp-ts/function';
import * as cheerio from 'cheerio';
import * as fs from 'fs/promises';
import { getAuthenticatedClient } from './api.client';
import type { AppError } from '../types';
import { logger } from '../utils/logger';

/**
 * Debug function to fetch and save HTML page content
 */
export const debugCoursePage = (): TE.TaskEither<AppError, void> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) => TE.tryCatch(
      async () => {
        logger.info('🔍 Fetching course page for debugging...');
        
        // Get the HTML page
        const response = await client.get('https://www.nnn.ed.nico/my_course?tab=zen_univ');
        const htmlContent = response.data as string;
        
        // Save to file for inspection
        await fs.writeFile('./debug_course_page.html', htmlContent, 'utf8');
        logger.info('📝 Saved HTML content to debug_course_page.html');
        
        // Parse and analyze structure
        const $ = cheerio.load(htmlContent);
        
        // Look for any potential course-related content
        logger.debug('Page analysis:');
        logger.debug(`- Title: ${$('title').text()}`);
        logger.debug(`- Total elements: ${$('*').length}`);
        logger.debug(`- Scripts: ${$('script').length}`);
        logger.debug(`- Links: ${$('a').length}`);
        logger.debug(`- Images: ${$('img').length}`);
        logger.debug(`- Divs: ${$('div').length}`);
        
        // Look for any divs that might contain course data
        const divs = $('div');
        logger.debug(`Found ${divs.length} div elements`);
        
        // Check for data attributes or classes that might indicate dynamic content
        const elementsWithData = $('[data-*], [class*="react"], [class*="vue"], [class*="angular"], [id*="root"], [id*="app"]');
        logger.debug(`Elements suggesting SPA: ${elementsWithData.length}`);
        
        if (elementsWithData.length > 0) {
          elementsWithData.each((i, el) => {
            const $el = $(el);
            const element = el as any;
            logger.debug(`SPA element ${i}: ${element.tagName || element.name || 'unknown'} class="${$el.attr('class')}" id="${$el.attr('id')}"`);
          });
        }
        
        // Look for script tags that might load course data
        $('script').each((i, el) => {
          const $script = $(el);
          const src = $script.attr('src');
          if (src) {
            logger.debug(`Script ${i}: ${src}`);
          }
        });
        
        // Check if there are any forms or hidden inputs with course data
        const forms = $('form');
        const inputs = $('input');
        logger.debug(`Forms: ${forms.length}, Inputs: ${inputs.length}`);
        
        inputs.each((i, el) => {
          const $input = $(el);
          const type = $input.attr('type');
          const name = $input.attr('name');
          const value = $input.attr('value');
          
          if (name && (name.includes('course') || name.includes('token') || name.includes('csrf'))) {
            logger.debug(`Important input ${i}: type="${type}" name="${name}" value="${value}"`);
          }
        });
        
        logger.info('✅ Debug analysis complete');
      },
      (error): AppError => ({
        type: 'NetworkError',
        message: `Debug failed: ${error instanceof Error ? error.message : String(error)}`,
      })
    ))
  );
};

/**
 * Try to use Puppeteer to load the dynamic content
 */
export const debugCoursePageWithPuppeteer = (): TE.TaskEither<AppError, void> => {
  return TE.tryCatch(
    async () => {
      const puppeteer = await import('puppeteer');
      
      logger.info('🔍 Using Puppeteer to debug dynamic course page...');
      
      const browser = await puppeteer.launch({ headless: false });
      const page = await browser.newPage();
      
      // Set cookies from session
      const { loadSessionData } = await import('../config');
      const sessionData = loadSessionData();
      
      if (sessionData?.cookies) {
        // Parse cookies and set them
        const cookieStrings = sessionData.cookies.split(';');
        for (const cookieString of cookieStrings) {
          const [nameValue] = cookieString.trim().split('=');
          if (nameValue) {
            const [name, ...valueParts] = nameValue.split('=');
            const value = valueParts.join('=');
            if (name && value) {
              await page.setCookie({
                name: name.trim(),
                value: value.trim(),
                domain: '.nnn.ed.nico'
              });
            }
          }
        }
      }
      
      await page.goto('https://www.nnn.ed.nico/my_course?tab=zen_univ', {
        waitUntil: 'networkidle2',
        timeout: 30000
      });
      
      // Wait for dynamic content to load
      logger.info('⏳ Waiting for dynamic content...');
      await new Promise(resolve => setTimeout(resolve, 5000));
      
      // Get the final HTML after JavaScript execution
      const dynamicHtml = await page.content();
      await fs.writeFile('./debug_course_page_dynamic.html', dynamicHtml, 'utf8');
      logger.info('📝 Saved dynamic HTML content to debug_course_page_dynamic.html');
      
      // Analyze the dynamic content
      const $ = cheerio.load(dynamicHtml);
      logger.debug('Dynamic page analysis:');
      logger.debug(`- Title: ${$('title').text()}`);
      logger.debug(`- Total elements: ${$('*').length}`);
      logger.debug(`- Links: ${$('a').length}`);
      
      // Look for course-related content again
      const courseElements = $('[class*="course"], [data-*="course"], a[href*="course"]');
      logger.debug(`Found ${courseElements.length} potential course elements`);
      
      if (courseElements.length > 0) {
        courseElements.each((i, el) => {
          const $el = $(el);
          const element = el as any;
          logger.debug(`Course element ${i}: ${element.tagName || element.name || 'unknown'} class="${$el.attr('class')}" href="${$el.attr('href')}" text="${$el.text().trim().slice(0, 100)}"`);
        });
      }
      
      await browser.close();
      logger.info('✅ Dynamic debugging complete');
    },
    (error): AppError => ({
      type: 'ProcessingError',
      message: `Puppeteer debug failed: ${error instanceof Error ? error.message : String(error)}`,
    })
  );
};

/**
 * Debug function to test specific API endpoints and see their raw responses
 */
export const debugApiEndpoint = (endpoint: string, courseId?: string): TE.TaskEither<AppError, void> => {
  return pipe(
    getAuthenticatedClient(),
    TE.chain((client) => TE.tryCatch(
      async () => {
        const finalEndpoint = courseId ? endpoint.replace('{courseId}', courseId) : endpoint;
        logger.info(`🔍 Testing API endpoint: ${finalEndpoint}`);
        
        try {
          const response = await client.get(finalEndpoint);
          
          logger.info(`📊 Response Status: ${response.status}`);
          logger.info(`📊 Response Headers:`, JSON.stringify(response.headers, null, 2));
          
          // Check if response is JSON
          const contentType = response.headers['content-type'] || '';
          if (contentType.includes('application/json')) {
            logger.info(`📊 Response Body (JSON):`, JSON.stringify(response.data, null, 2));
          } else {
            logger.info(`📊 Response Body (Text/HTML):`, typeof response.data === 'string' ? response.data.substring(0, 1000) + '...' : String(response.data));
          }
          
          // Save response to file
          const filename = `debug_api_response_${courseId || 'test'}.json`;
          await fs.writeFile(`./${filename}`, JSON.stringify({
            status: response.status,
            headers: response.headers,
            data: response.data
          }, null, 2), 'utf8');
          
          logger.success(`✅ API response saved to ${filename}`);
          
        } catch (error: any) {
          logger.error(`❌ API request failed:`, error.response?.status, error.response?.statusText);
          logger.error(`❌ Error details:`, error.message);
          
          if (error.response?.data) {
            logger.error(`❌ Error response body:`, JSON.stringify(error.response.data, null, 2));
          }
        }
      },
      (error): AppError => ({
        type: 'ProcessingError',
        message: `Failed to debug API endpoint: ${error instanceof Error ? error.message : String(error)}`,
      })
    ))
  );
};
