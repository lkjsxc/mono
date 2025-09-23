FROM gcc as builder
WORKDIR /app
COPY ./lkjscript.c /app/lkjscript.c
RUN gcc -o lkjscript lkjscript.c -static -march=native -std=c11 -Werror -Wall -Wextra -O2

FROM scratch
COPY --from=builder /app/lkjscript /app/lkjscript
CMD ["/app/lkjscript"]