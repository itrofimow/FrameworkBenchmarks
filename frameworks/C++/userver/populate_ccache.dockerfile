FROM ghcr.io/userver-framework/docker-userver-build-base:v1a AS ccache_holder
WORKDIR /src
RUN ccache -s
RUN git clone https://github.com/userver-framework/userver.git && \
    cd userver && git checkout b69a8db23844d3abbb68e40a502eae0ecd2e4b62
COPY userver_benchmark/ ./
RUN mkdir build && cd build && \
    cmake -DUSERVER_IS_THE_ROOT_PROJECT=0 -DUSERVER_OPEN_SOURCE_BUILD=1 -DUSERVER_FEATURE_CRYPTOPP_BLAKE2=0 \
          -DUSERVER_FEATURE_REDIS=0 -DUSERVER_FEATURE_CLICKHOUSE=0 -DUSERVER_FEATURE_MONGODB=0 -DUSERVER_FEATURE_RABBITMQ=0 -DUSERVER_FEATURE_GRPC=0 \
          -DUSERVER_FEATURE_UTEST=0 \
          -DUSERVER_FEATURE_POSTGRESQL=1 \
          -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && \
    make -j $(nproc)

RUN ls | grep -xv "userver" | xargs rm -r
RUN ccache -s

