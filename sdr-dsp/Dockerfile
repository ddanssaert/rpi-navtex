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
    x86_64)  sdr_arch="x86_64" ;; \
    aarch64) sdr_arch="arm64" ;; \
    armv7l)  sdr_arch="armhf" ;; \
    *)       sdr_arch="x86_64" ;; \
    esac \
    && cp /tmp/sdrplay/$sdr_arch/libsdrplay_api.so* /usr/local/lib/ \
    && ldconfig \
    && rm -rf /tmp/sdrplay*

WORKDIR /build
COPY . .
RUN cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j$(nproc)

FROM debian:bookworm-slim
COPY --from=build /usr/local/lib/libsdrplay_api.so* /usr/local/lib/
RUN ldconfig
COPY --from=build /build/build/sdr-dsp /usr/local/bin/sdr-dsp
ENTRYPOINT ["/usr/local/bin/sdr-dsp"]
