FROM gcc as builder
COPY ./lkjscript.c /lkjscript.c
RUN gcc -o /lkjscript /lkjscript.c -static -march=native -std=c11 -Werror -Wall -Wextra -O2

FROM scratch
COPY --from=builder /lkjscript /lkjscript
CMD ["/lkjscript", "/data/main.lkjscript"]