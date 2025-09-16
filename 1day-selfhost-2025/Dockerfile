FROM gcc as builder
COPY ./main.c /app/main.c
WORKDIR /app
RUN gcc -o main main.c -static -Werror -Wall -Wextra -std=c11 -O2 -march=native

FROM scratch
COPY --from=builder /app/main /app/main
COPY ./test/03.txt /app/03.txt
CMD ["/app/main", "/app/03.txt"]