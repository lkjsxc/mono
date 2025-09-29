const TAG_SEPARATOR = ",";

const trim = (value: string): string => value.trim();

const unique = <T>(values: readonly T[]): T[] => {
  const seen = new Set<T>();
  const result: T[] = [];
  for (const value of values) {
    if (!seen.has(value)) {
      seen.add(value);
      result.push(value);
    }
  }
  return result;
};

export const toTagArray = (tags: string | readonly string[] | undefined | null): string[] => {
  if (!tags) return [];
  const array = Array.isArray(tags)
    ? Array.from(tags)
    : String(tags)
        .split(TAG_SEPARATOR)
        .map(trim);
  const filtered = array.filter((value): value is string => value.length > 0);
  return unique(filtered).sort((a, b) => a.localeCompare(b));
};

export const normalizeTags = (tags: string | readonly string[] | undefined | null): string =>
  toTagArray(tags).join(TAG_SEPARATOR);

export const tagsMatchPrefix = (search: string, entry: string): boolean => {
  if (!search) return true;
  if (!entry) return false;
  if (entry.startsWith(search)) {
    return entry.length === search.length || entry.charAt(search.length) === TAG_SEPARATOR;
  }
  return false;
};

export const searchTagsInEntry = (search: string, entry: string): boolean => {
  if (!search) return true;
  const searchTags = toTagArray(search);
  const entryTags = new Set(toTagArray(entry));
  if (searchTags.length === 0) return true;
  if (entryTags.size === 0 && searchTags.length > 0) return false;
  return searchTags.every((tag) => entryTags.has(tag));
};
