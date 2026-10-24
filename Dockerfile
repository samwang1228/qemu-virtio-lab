FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /workspace

RUN apt-get update && apt-get install -y --no-install-recommends \
    bc bison build-essential ca-certificates cpio flex file \
    gcc-aarch64-linux-gnu git libelf-dev libssl-dev \
    libncurses-dev python3 rsync unzip wget xz-utils \
    && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/workspace/scripts/build-guest.sh"]
