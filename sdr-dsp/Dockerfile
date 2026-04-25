FROM debian:bookworm-slim AS build

RUN apt-get update && apt-get install -y \
    cmake g++ libpthread-stubs0-dev \
    && rm -rf /var/lib/apt/lists/*

# Bake SDRPlay API into the image.
# The installer .run file must be placed in sdr-dsp/vendor/ before building.
# Download from: https://www.sdrplay.com/downloads/
COPY vendor/SDRplay_RSP_API-Linux-*.run /tmp/sdrplay_install.run
RUN chmod +x /tmp/sdrplay_install.run \
    && /tmp/sdrplay_install.run --noexec --target /tmp/sdrplay \
    && cp /tmp/sdrplay/inc/* /usr/local/include/ \
    && arch=$(uname -m) \
    && case "$arch" in \
    x86_64)  search="x86_64 x64 amd64" ;; \
    aarch64) search="arm64 aarch64" ;; \
    armv7l)  search="armhf armv7l" ;; \
    *)       search="$arch" ;; \
    esac \
    && for dir in $search; do \
    if [ -d "/tmp/sdrplay/$dir" ]; then \
    cp /tmp/sdrplay/$dir/libsdrplay_api.so* /usr/local/lib/; \
    if [ -f "/tmp/sdrplay/$dir/sdrplay_apiService" ]; then \
    cp "/tmp/sdrplay/$dir/sdrplay_apiService" /usr/local/bin/; \
    fi; \
    found=1; break; \
    fi; \
    done \
    && if [ -z "$found" ]; then \
    echo "Could not find SDRPlay libs for $arch in: $(ls /tmp/sdrplay)"; exit 1; \
    fi \
    && ldconfig \
    && ln -s /usr/local/lib/libsdrplay_api.so.3 /usr/local/lib/libsdrplay_api.so \
    && rm -rf /tmp/sdrplay*

WORKDIR /build
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

FROM debian:bookworm-slim
COPY --from=build /usr/local/lib/libsdrplay_api.so* /usr/local/lib/
COPY --from=build /usr/local/bin/sdrplay_apiService /usr/local/bin/sdrplay_apiService
RUN ldconfig
COPY --from=build /build/build/sdr-dsp /usr/local/bin/sdr-dsp
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
