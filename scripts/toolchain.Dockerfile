FROM debian@sha256:88200866dfff7ea7f5cbcb6ec7c8a701889efe6fe859fe64d6990e4b07ea4171
RUN dpkg --add-architecture arm64 && apt-get update && apt-get install -y --no-install-recommends g++-aarch64-linux-gnu cmake make libsdl2-dev:arm64 libgles-dev:arm64 && rm -rf /var/lib/apt/lists/*
WORKDIR /work
