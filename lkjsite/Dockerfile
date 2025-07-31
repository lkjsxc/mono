FROM gcc:12 as builder
COPY ./src/ /app/src
COPY ./Makefile /app

WORKDIR /app
RUN make

FROM scratch
COPY --from=builder /app/build/lkjsite /app/build/lkjsite
CMD ["/app/build/lkjsite"]