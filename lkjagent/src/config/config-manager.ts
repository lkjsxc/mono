import * as fs from 'fs/promises';
import * as path from 'path';

export interface MemoryConfig {
  ramCharacterLimit: number;
  systemPromptDirectory: string;
  criticalPaths: string[];
  backupInterval: number;
}

export interface Config {
  memory: MemoryConfig;
}

export class ConfigManager {
  private static instance: ConfigManager;
  private config: Config = {} as Config; // Initialize with a default empty object

  private constructor() { }

  public static async getInstance(): Promise<ConfigManager> {
    if (!ConfigManager.instance) {
      ConfigManager.instance = new ConfigManager();
      await ConfigManager.instance.loadConfig();
    }
    return ConfigManager.instance;
  }

  private async loadConfig(): Promise<void> {
    const configPath = path.join(__dirname, '..', '..', 'data', 'config.json');
    const configData = await fs.readFile(configPath, 'utf-8');
    this.config = JSON.parse(configData);
  }

  public getMemoryConfig(): MemoryConfig {
    return this.config.memory;
  }

  public async updateConfig(newConfig: Partial<Config>): Promise<void> {
    this.config = { ...this.config, ...newConfig };
    const configPath = path.join(__dirname, '..', '..', 'data', 'config.json');
    await fs.writeFile(configPath, JSON.stringify(this.config, null, 2), 'utf-8');
  }
}
