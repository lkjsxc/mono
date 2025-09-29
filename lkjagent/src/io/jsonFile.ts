import { promises as fs } from "node:fs";
import path from "node:path";

export const readJsonFile = async <T>(filePath: string): Promise<T | undefined> => {
  try {
    const raw = await fs.readFile(filePath, "utf8");
    return JSON.parse(raw) as T;
  } catch (error) {
    if ((error as NodeJS.ErrnoException).code === "ENOENT") {
      return undefined;
    }
    throw error;
  }
};

export const writeJsonFile = async (filePath: string, value: unknown): Promise<void> => {
  const directory = path.dirname(filePath);
  await fs.mkdir(directory, { recursive: true });
  const payload = JSON.stringify(value, null, 2);
  await fs.writeFile(filePath, payload, "utf8");
};
