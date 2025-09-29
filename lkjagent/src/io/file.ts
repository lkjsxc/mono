import { promises as fs } from "node:fs";
import path from "node:path";

export const appendToFile = async (filePath: string, content: string): Promise<void> => {
  const directory = path.dirname(filePath);
  await fs.mkdir(directory, { recursive: true });
  await fs.appendFile(filePath, content, "utf8");
};
