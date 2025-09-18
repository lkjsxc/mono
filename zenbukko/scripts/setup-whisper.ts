/**
 * Whisper.cpp Setup Script
 * Automatically clones, builds, and configures whisper.cpp for transcription
 */

import * as path from 'path';
import * as fs from 'fs/promises';
import { spawn, exec } from 'child_process';
import { promisify } from 'util';
import { logger } from '../src/utils/logger';
import { CONSTANTS } from '../src/config';
import { exists, ensureDir } from '../src/utils/files';

const execAsync = promisify(exec);

/**
 * Configuration for whisper.cpp setup
 */
const WHISPER_CONFIG = {
  REPO_URL: 'https://github.com/ggerganov/whisper.cpp.git',
  MODELS_TO_DOWNLOAD: ['base.en', 'small.en', 'base'],
  BUILD_TARGETS: ['main'],
};

/**
 * Check if required tools are available
 */
const checkRequirements = async (): Promise<boolean> => {
  logger.info('🔍 Checking system requirements...');
  
  const requiredTools = [
    { name: 'git', command: 'git --version' },
    { name: 'make', command: 'make --version' },
    { name: 'gcc/clang', command: 'gcc --version || clang --version' },
  ];
  
  let allAvailable = true;
  
  for (const tool of requiredTools) {
    try {
      await execAsync(tool.command);
      logger.debug(`✅ ${tool.name} is available`);
    } catch (error) {
      logger.error(`❌ ${tool.name} is not available or not in PATH`);
      allAvailable = false;
    }
  }
  
  if (!allAvailable) {
    logger.error('❌ Missing required build tools. Please install:');
    logger.error('   - Git: https://git-scm.com/');
    logger.error('   - Make & GCC/Clang: Install Xcode Command Line Tools on macOS');
    logger.error('   - Run: xcode-select --install');
  }
  
  return allAvailable;
};

/**
 * Clone whisper.cpp repository
 */
const cloneWhisperRepo = async (): Promise<boolean> => {
  logger.info('📥 Cloning whisper.cpp repository...');
  
  try {
    if (await exists(CONSTANTS.WHISPER_DIR)) {
      logger.info('📁 Whisper.cpp directory already exists');
      return true;
    }
    
    await new Promise<void>((resolve, reject) => {
      const gitProcess = spawn('git', [
        'clone',
        WHISPER_CONFIG.REPO_URL,
        CONSTANTS.WHISPER_DIR
      ], {
        stdio: ['pipe', 'pipe', 'pipe'],
        cwd: CONSTANTS.PROJECT_ROOT,
      });
      
      let stdout = '';
      let stderr = '';
      
      gitProcess.stdout?.on('data', (data) => {
        stdout += data.toString();
        logger.debug(`Git stdout: ${data.toString().trim()}`);
      });
      
      gitProcess.stderr?.on('data', (data) => {
        stderr += data.toString();
        logger.debug(`Git stderr: ${data.toString().trim()}`);
      });
      
      gitProcess.on('close', (code) => {
        if (code === 0) {
          logger.success('✅ Repository cloned successfully');
          resolve();
        } else {
          logger.error(`❌ Git clone failed with code ${code}: ${stderr}`);
          reject(new Error(`Git clone failed: ${stderr}`));
        }
      });
      
      gitProcess.on('error', (error) => {
        logger.error(`❌ Failed to start git process: ${error.message}`);
        reject(error);
      });
    });
    
    return true;
    
  } catch (error) {
    logger.error(`❌ Clone failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    return false;
  }
};

/**
 * Build whisper.cpp executable
 */
const buildWhisper = async (): Promise<boolean> => {
  logger.info('🔨 Building whisper.cpp...');
  
  try {
    // Check if already built
    if (await exists(CONSTANTS.WHISPER_EXECUTABLE)) {
      logger.info('🔧 Executable already exists, checking if rebuild needed...');
    }
    
    await new Promise<void>((resolve, reject) => {
      const makeProcess = spawn('make', [], {
        stdio: ['pipe', 'pipe', 'pipe'],
        cwd: CONSTANTS.WHISPER_DIR,
      });
      
      let stdout = '';
      let stderr = '';
      
      makeProcess.stdout?.on('data', (data) => {
        const output = data.toString();
        stdout += output;
        
        // Log build progress
        if (output.includes('CC ') || output.includes('CXX ')) {
          const match = output.match(/(CC|CXX)\s+([^\s]+)/);
          if (match) {
            logger.progress(`Compiling ${path.basename(match[2])}...`);
          }
        }
      });
      
      makeProcess.stderr?.on('data', (data) => {
        stderr += data.toString();
        logger.debug(`Make stderr: ${data.toString().trim()}`);
      });
      
      makeProcess.on('close', (code) => {
        if (code === 0) {
          logger.success('✅ Build completed successfully');
          resolve();
        } else {
          logger.error(`❌ Build failed with code ${code}`);
          if (stderr) {
            logger.error(`Build error: ${stderr}`);
          }
          reject(new Error(`Make failed: ${stderr || 'Unknown build error'}`));
        }
      });
      
      makeProcess.on('error', (error) => {
        logger.error(`❌ Failed to start make process: ${error.message}`);
        reject(error);
      });
    });
    
    // Verify executable was created
    if (!(await exists(CONSTANTS.WHISPER_EXECUTABLE))) {
      throw new Error('Executable not found after build');
    }
    
    logger.success(`✅ Whisper executable built: ${CONSTANTS.WHISPER_EXECUTABLE}`);
    return true;
    
  } catch (error) {
    logger.error(`❌ Build failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    return false;
  }
};

/**
 * Download whisper models
 */
const downloadModels = async (): Promise<boolean> => {
  logger.info('📥 Downloading whisper models...');
  
  try {
    await ensureDir(CONSTANTS.WHISPER_MODELS_DIR);
    
    for (const model of WHISPER_CONFIG.MODELS_TO_DOWNLOAD) {
      const modelFile = path.join(CONSTANTS.WHISPER_MODELS_DIR, `ggml-${model}.bin`);
      
      if (await exists(modelFile)) {
        logger.info(`✅ Model ${model} already exists`);
        continue;
      }
      
      logger.info(`📥 Downloading model: ${model}`);
      
      await new Promise<void>((resolve, reject) => {
        const downloadScript = path.join(CONSTANTS.WHISPER_DIR, 'models', 'download-ggml-model.sh');
        const downloadProcess = spawn('bash', [downloadScript, model], {
          stdio: ['pipe', 'pipe', 'pipe'],
          cwd: CONSTANTS.WHISPER_MODELS_DIR,
        });
        
        let stdout = '';
        let stderr = '';
        
        downloadProcess.stdout?.on('data', (data) => {
          const output = data.toString();
          stdout += output;
          
          // Log download progress
          if (output.includes('%')) {
            const progressMatch = output.match(/(\d+)%/);
            if (progressMatch) {
              logger.progress(`Downloading ${model}`, {
                current: parseInt(progressMatch[1], 10),
                total: 100,
              });
            }
          }
        });
        
        downloadProcess.stderr?.on('data', (data) => {
          stderr += data.toString();
        });
        
        downloadProcess.on('close', (code) => {
          if (code === 0) {
            logger.success(`✅ Model ${model} downloaded successfully`);
            resolve();
          } else {
            logger.error(`❌ Failed to download model ${model}: ${stderr}`);
            reject(new Error(`Model download failed: ${stderr}`));
          }
        });
        
        downloadProcess.on('error', (error) => {
          logger.error(`❌ Failed to start download process: ${error.message}`);
          reject(error);
        });
      });
    }
    
    return true;
    
  } catch (error) {
    logger.error(`❌ Model download failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    return false;
  }
};

/**
 * Test whisper.cpp installation
 */
const testWhisperInstallation = async (): Promise<boolean> => {
  logger.info('🧪 Testing whisper.cpp installation...');
  
  try {
    // Test with --help flag
    await new Promise<void>((resolve, reject) => {
      const testProcess = spawn(CONSTANTS.WHISPER_EXECUTABLE, ['--help'], {
        stdio: ['pipe', 'pipe', 'pipe'],
      });
      
      let stdout = '';
      
      testProcess.stdout?.on('data', (data) => {
        stdout += data.toString();
      });
      
      testProcess.on('close', (code) => {
        if (code === 0 && stdout.includes('usage:')) {
          logger.success('✅ Whisper.cpp is working correctly');
          resolve();
        } else {
          reject(new Error(`Test failed with code ${code}`));
        }
      });
      
      testProcess.on('error', (error) => {
        reject(error);
      });
    });
    
    return true;
    
  } catch (error) {
    logger.error(`❌ Installation test failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    return false;
  }
};

/**
 * Clean up any partial/failed installations
 */
const cleanup = async (): Promise<void> => {
  logger.info('🧹 Cleaning up...');
  
  try {
    if (await exists(CONSTANTS.WHISPER_DIR)) {
      await fs.rm(CONSTANTS.WHISPER_DIR, { recursive: true, force: true });
      logger.info('🗑️  Removed whisper.cpp directory');
    }
  } catch (error) {
    logger.warn(`⚠️  Cleanup warning: ${error instanceof Error ? error.message : 'Unknown error'}`);
  }
};

/**
 * Main setup function
 */
export const setupWhisper = async (force: boolean = false): Promise<boolean> => {
  logger.info('🚀 Starting whisper.cpp setup...');
  
  try {
    // Check if already installed and not forcing
    if (!force && await exists(CONSTANTS.WHISPER_EXECUTABLE)) {
      const testPassed = await testWhisperInstallation();
      if (testPassed) {
        logger.info('✅ Whisper.cpp is already set up and working');
        return true;
      }
      logger.warn('⚠️  Existing installation has issues, proceeding with setup...');
    }
    
    // Force cleanup if requested
    if (force) {
      await cleanup();
    }
    
    // Step 1: Check requirements
    if (!(await checkRequirements())) {
      return false;
    }
    
    // Step 2: Clone repository
    if (!(await cloneWhisperRepo())) {
      return false;
    }
    
    // Step 3: Build whisper.cpp
    if (!(await buildWhisper())) {
      return false;
    }
    
    // Step 4: Download models
    if (!(await downloadModels())) {
      return false;
    }
    
    // Step 5: Test installation
    if (!(await testWhisperInstallation())) {
      return false;
    }
    
    logger.success('🎉 Whisper.cpp setup completed successfully!');
    logger.info('📝 Available models:');
    for (const model of WHISPER_CONFIG.MODELS_TO_DOWNLOAD) {
      logger.info(`   - ${model}`);
    }
    logger.info('🎙️  You can now use the transcribe command');
    
    return true;
    
  } catch (error) {
    logger.error(`❌ Setup failed: ${error instanceof Error ? error.message : 'Unknown error'}`);
    return false;
  }
};

/**
 * CLI entry point for the setup script
 */
const main = async (): Promise<void> => {
  const force = process.argv.includes('--force');
  const success = await setupWhisper(force);
  process.exit(success ? 0 : 1);
};

// Run setup if this file is executed directly
if (require.main === module) {
  main();
}
