NODE_BIN ?= npx

.PHONY: install build dev start test lint clean

install:
	npm install

build:
	npm run build

dev:
	$(NODE_BIN) tsx watch src/index.ts

start:
	npm start

test:
	npm test

clean:
	rm -rf dist node_modules
