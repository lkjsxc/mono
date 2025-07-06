FROM gcc:latest

WORKDIR /app

COPY . .

RUN gcc -static -O2 -o main main.c

FROM gcr.io/distroless/static-debian12

WORKDIR /app

COPY --from=0 /app/main /app/main

CMD ["./main"]