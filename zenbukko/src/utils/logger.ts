/**
 * Simple and structured logging utility for zenbukko
 * Supports different log levels and formatted output
 */

export type LogLevel = 'debug' | 'info' | 'warn' | 'error';

export interface LoggerConfig {
  level: LogLevel;
  enableColors: boolean;
  enableTimestamps: boolean;
}

/**
 * ANSI color codes for terminal output
 */
const COLORS = {
  reset: '\x1b[0m',
  bright: '\x1b[1m',
  dim: '\x1b[2m',
  red: '\x1b[31m',
  green: '\x1b[32m',
  yellow: '\x1b[33m',
  blue: '\x1b[34m',
  magenta: '\x1b[35m',
  cyan: '\x1b[36m',
  white: '\x1b[37m',
} as const;

/**
 * Log level priorities (higher number = higher priority)
 */
const LOG_LEVELS: Record<LogLevel, number> = {
  debug: 0,
  info: 1,
  warn: 2,
  error: 3,
} as const;

/**
 * Log level styling configuration
 */
const LOG_STYLES: Record<LogLevel, { color: string; prefix: string }> = {
  debug: { color: COLORS.dim, prefix: '🐛 DEBUG' },
  info: { color: COLORS.blue, prefix: 'ℹ️  INFO' },
  warn: { color: COLORS.yellow, prefix: '⚠️  WARN' },
  error: { color: COLORS.red, prefix: '❌ ERROR' },
} as const;

/**
 * Logger class with configurable levels and formatting
 */
export class Logger {
  private config: LoggerConfig;

  constructor(config: Partial<LoggerConfig> = {}) {
    this.config = {
      level: config.level || 'info',
      enableColors: config.enableColors ?? true,
      enableTimestamps: config.enableTimestamps ?? true,
    };
  }

  /**
   * Check if a log level should be output
   */
  public shouldLog(level: LogLevel): boolean {
    return LOG_LEVELS[level] >= LOG_LEVELS[this.config.level];
  }

  /**
   * Format timestamp for log output
   */
  private formatTimestamp(): string {
    if (!this.config.enableTimestamps) return '';
    
    const now = new Date();
    const timestamp = now.toISOString();
    return this.config.enableColors 
      ? `${COLORS.dim}[${timestamp}]${COLORS.reset} `
      : `[${timestamp}] `;
  }

  /**
   * Format log message with level styling
   */
  private formatMessage(level: LogLevel, message: string): string {
    const timestamp = this.formatTimestamp();
    const style = LOG_STYLES[level];
    
    if (!this.config.enableColors) {
      return `${timestamp}${style.prefix}: ${message}`;
    }
    
    return `${timestamp}${style.color}${style.prefix}${COLORS.reset}: ${message}`;
  }

  /**
   * Generic log method
   */
  private log(level: LogLevel, message: string, ...args: any[]): void {
    if (!this.shouldLog(level)) return;
    
    const formattedMessage = this.formatMessage(level, message);
    const logMethod = level === 'error' ? console.error : console.log;
    
    if (args.length > 0) {
      logMethod(formattedMessage, ...args);
    } else {
      logMethod(formattedMessage);
    }
  }

  /**
   * Debug level logging
   */
  debug(message: string, ...args: any[]): void {
    this.log('debug', message, ...args);
  }

  /**
   * Info level logging
   */
  info(message: string, ...args: any[]): void {
    this.log('info', message, ...args);
  }

  /**
   * Warning level logging
   */
  warn(message: string, ...args: any[]): void {
    this.log('warn', message, ...args);
  }

  /**
   * Error level logging
   */
  error(message: string, ...args: any[]): void {
    this.log('error', message, ...args);
  }

  /**
   * Log success messages
   */
  success(message: string, ...args: any[]): void {
    const timestamp = this.formatTimestamp();
    const successMessage = this.config.enableColors
      ? `${timestamp}${COLORS.green}✅ SUCCESS${COLORS.reset}: ${message}`
      : `${timestamp}✅ SUCCESS: ${message}`;
    
    if (this.shouldLog('info')) {
      if (args.length > 0) {
        console.log(successMessage, ...args);
      } else {
        console.log(successMessage);
      }
    }
  }

  /**
   * Log progress updates
   */
  progress(message: string, progress?: { current: number; total: number }): void {
    if (!this.shouldLog('info')) return;
    
    const timestamp = this.formatTimestamp();
    let progressText = '';
    
    if (progress) {
      const percentage = Math.round((progress.current / progress.total) * 100);
      progressText = ` (${progress.current}/${progress.total} - ${percentage}%)`;
    }
    
    const progressMessage = this.config.enableColors
      ? `${timestamp}${COLORS.cyan}⏳ PROGRESS${COLORS.reset}: ${message}${progressText}`
      : `${timestamp}⏳ PROGRESS: ${message}${progressText}`;
    
    console.log(progressMessage);
  }

  /**
   * Update log level dynamically
   */
  setLevel(level: LogLevel): void {
    this.config.level = level;
  }

  /**
   * Enable or disable colors
   */
  setColors(enabled: boolean): void {
    this.config.enableColors = enabled;
  }

  /**
   * Enable or disable timestamps
   */
  setTimestamps(enabled: boolean): void {
    this.config.enableTimestamps = enabled;
  }
}

/**
 * Create a logger instance with environment-based configuration
 */
export const createLogger = (level?: LogLevel): Logger => {
  const logLevel = level || (process.env['LOG_LEVEL'] as LogLevel) || 'info';
  const isCI = process.env['CI'] === 'true';
  const isTTY = process.stdout.isTTY;
  
  return new Logger({
    level: logLevel,
    enableColors: !isCI && isTTY,
    enableTimestamps: true,
  });
};

/**
 * Default logger instance
 */
export const logger = createLogger();

/**
 * Utility functions for structured logging
 */
export const logUtils = {
  /**
   * Log object as formatted JSON
   */
  logObject: (level: LogLevel, label: string, obj: any) => {
    if (!logger.shouldLog(level)) return;
    
    logger[level](`${label}:`);
    console.log(JSON.stringify(obj, null, 2));
  },

  /**
   * Log error with stack trace
   */
  logError: (error: Error | unknown, context?: string) => {
    const contextMsg = context ? `[${context}] ` : '';
    
    if (error instanceof Error) {
      logger.error(`${contextMsg}${error.message}`);
      if (error.stack) {
        logger.debug(`Stack trace:\n${error.stack}`);
      }
    } else {
      logger.error(`${contextMsg}Unknown error:`, error);
    }
  },

  /**
   * Log performance timing
   */
  logTiming: (operation: string, startTime: number, endTime: number = Date.now()) => {
    const duration = endTime - startTime;
    const durationStr = duration < 1000 
      ? `${duration}ms` 
      : `${(duration / 1000).toFixed(2)}s`;
    
    logger.info(`⏱️  ${operation} completed in ${durationStr}`);
  },
};
