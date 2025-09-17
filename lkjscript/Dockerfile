FROM gcc:12 as builder
COPY ./lkjscript.c /app/lkjscript.c

WORKDIR /app
RUN gcc -o lkjscript lkjscript.c -march=native -Werror -Wall -Wextra -std=c11 -O2 -g

FROM scratch
COPY --from=builder /app/lkjscript /app/lkjscript
CMD ["/app/lkjscript"]