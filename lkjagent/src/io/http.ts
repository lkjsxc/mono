export interface HttpOptions {
  readonly retries?: number;
  readonly timeoutMs?: number;
  readonly headers?: Record<string, string>;
}

class HttpError extends Error {
  constructor(message: string, readonly status: number, readonly body: string) {
    super(message);
    this.name = "HttpError";
  }
}

const DEFAULT_TIMEOUT = 60_000;

const sleep = (ms: number) => new Promise((resolve) => setTimeout(resolve, ms));

const performRequest = async (
  url: string,
  payload: string,
  options: Required<HttpOptions>,
): Promise<Response> => {
  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), options.timeoutMs);

  try {
    const response = await fetch(url, {
      method: "POST",
      headers: {
        "content-type": "application/json",
        ...options.headers,
      },
      body: payload,
      signal: controller.signal,
    });
    return response;
  } finally {
    clearTimeout(timeout);
  }
};

export const postJson = async <T>(url: string, body: unknown, options: HttpOptions = {}): Promise<T> => {
  const merged: Required<HttpOptions> = {
    retries: options.retries ?? 2,
    timeoutMs: options.timeoutMs ?? DEFAULT_TIMEOUT,
    headers: options.headers ?? {},
  };

  const payload = typeof body === "string" ? body : JSON.stringify(body);

  let attempt = 0;
  // exponential backoff
  while (true) {
    try {
      const response = await performRequest(url, payload, merged);
      if (!response.ok) {
        const text = await response.text();
        throw new HttpError(`Request failed with status ${response.status}`, response.status, text);
      }
      const text = await response.text();
      return JSON.parse(text) as T;
    } catch (error) {
      attempt += 1;
      if (attempt > merged.retries) {
        throw error;
      }
      const backoff = Math.min(2 ** attempt * 250, 5_000);
      await sleep(backoff);
    }
  }
};
