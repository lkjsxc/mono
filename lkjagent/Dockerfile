FROM node:20-bookworm-slim AS base
WORKDIR /app
ENV NODE_ENV=production

FROM base AS deps
COPY package.json package-lock.json* tsconfig.json vitest.config.ts ./
RUN npm install --include=dev

FROM deps AS build
COPY src ./src
COPY tests ./tests
RUN npm run build

FROM base AS runtime
COPY --from=build /app/dist ./dist
COPY package.json package-lock.json* ./
RUN npm install --omit=dev && npm cache clean --force
COPY data ./data
CMD ["node", "dist/index.js"]