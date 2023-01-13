FROM userver_ccache AS builder
WORKDIR /src
RUN ccache -s
RUN ls -a
COPY userver_benchmark/ ./
RUN mkdir build && cd build && \
    cmake -DUSERVER_IS_THE_ROOT_PROJECT=0 -DUSERVER_OPEN_SOURCE_BUILD=1 -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 \
          -DUSERVER_FEATURE_REDIS=0 -DUSERVER_FEATURE_CLICKHOUSE=0 -DUSERVER_FEATURE_MONGODB=0 -DUSERVER_FEATURE_RABBITMQ=0 -DUSERVER_FEATURE_GRPC=0 \
          -DUSERVER_FEATURE_UTEST=0 \
          -DUSERVER_FEATURE_POSTGRESQL=1 \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && \
    make -j $(nproc)

FROM builder AS runner
RUN ccache -s
WORKDIR /app
COPY userver_configs/* ./
COPY --from=builder /userver_build/build/userver_techempower ./

EXPOSE 8080
CMD ./userver_techempower -c ./static_config.yaml

