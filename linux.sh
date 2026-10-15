#!/bin/sh
# Запуск команды (или интерактивного shell) в Linux-окружении курса.
# SYS_PTRACE и seccomp=unconfined нужны, чтобы внутри контейнера работал strace.
# Пример: ./linux.sh make -C lab2 test
set -e
cd "$(dirname "$0")"
docker build -q -t oslab . >/dev/null
TTY=""
[ -t 0 ] && TTY="-it"
exec docker run --rm $TTY --cap-add=SYS_PTRACE --security-opt seccomp=unconfined \
    -v "$PWD":/work -w /work oslab "${@:-bash}"
