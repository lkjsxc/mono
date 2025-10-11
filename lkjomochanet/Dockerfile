FROM gcc:latest

WORKDIR /app

COPY main.c index.html .

RUN gcc -o social-network main.c

# Default command (can be overridden)
CMD ["./social-network", "server"]
