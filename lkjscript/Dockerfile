FROM gcc:12 as builder
COPY ./src/lkjscript.c /lkjscript.c
RUN gcc -o lkjscript -static -O2 -march=native lkjscript.c

FROM scratch
COPY --from=builder /lkjscript /lkjscript
CMD ["/lkjscript"]