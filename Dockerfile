# Чистая Ubuntu LTS со стандартным тулчейном для сборки и трассировки лабораторных
FROM ubuntu:24.04
RUN apt-get update \
 && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
      build-essential strace valgrind procps \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /work
