FROM gcc:latest

COPY . /app

WORKDIR /app

RUN gcc -static -O2 -o main main.c

FROM gcr.io/distroless/static-debian12

COPY --from=0 /app /app

WORKDIR /app

CMD ["./main"]