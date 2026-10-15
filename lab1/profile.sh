#!/bin/sh
# Снятие профиля системных вызовов программы под strace и группировка по назначению.
# Использование: [STDIN_FILE=файл] ./profile.sh <имя_профиля> <команда...>
#   STDIN_FILE — файл, подаваемый программе на стандартный ввод (по умолчанию /dev/null).
# Результаты: traces/<имя>.log (полная трасса), traces/<имя>.summary (strace -c),
#             traces/<имя>.groups (вызовы, сгруппированные по функционалу).
set -e
cd "$(dirname "$0")"
name=$1; shift
mkdir -p traces
in=${STDIN_FILE:-/dev/null}

strace -f -tt -s 64 -o "traces/$name.log" "$@" < "$in" > /dev/null
strace -f -c -o "traces/$name.summary" "$@" < "$in" > /dev/null

# Колонки strace -c: % time, seconds, usecs/call, calls, [errors], syscall
awk '
function group(s) {
    if (s ~ /^(clone|clone3|fork|vfork|execve|wait4|waitid|exit|exit_group|getpid|getppid|gettid|set_tid_address|set_robust_list|rseq|prlimit64|getrlimit|setpgid|getpgid|getpgrp|setsid|getsid|uname)$/) return "1. Управление процессами";
    if (s ~ /^(openat|open|close|read|write|pread64|pwrite64|readv|writev|lseek|fstat|newfstatat|statx|faccessat|faccessat2|access|readlinkat|getdents64|unlinkat|fcntl|ioctl|getcwd|chdir|fchdir|umask|dup|dup2|dup3|utimensat|fadvise64|fstatfs|statfs|fgetxattr|flistxattr|lgetxattr|getxattr|listxattr|llistxattr)$/) return "2. Файлы и дескрипторы";
    if (s ~ /^(pipe|pipe2|mkfifoat|mknodat|socket|connect|sendto|recvfrom)$/) return "3. Межпроцессное взаимодействие";
    if (s ~ /^(rt_sigaction|rt_sigprocmask|rt_sigreturn|rt_sigsuspend|kill|tgkill|sigaltstack|pause)$/) return "4. Сигналы";
    if (s ~ /^(mmap|munmap|mprotect|brk|madvise|mremap)$/) return "5. Управление памятью";
    if (s ~ /^(getuid|geteuid|getgid|getegid|getgroups|getrandom|sysinfo|clock_gettime|clock_nanosleep|nanosleep|gettimeofday|times)$/) return "6. Окружение, время, идентификация";
    return "7. Прочее";
}
/^-|% time|total/ { next }
NF >= 5 {
    sys = $NF; calls = $4; err = (NF == 6) ? $5 : 0;
    g = group(sys); cnt[g] += calls; list[g] = list[g] sprintf(" %s(%d%s)", sys, calls, err ? ", err " err : "");
}
END {
    for (g in cnt) printf "%s — %d вызовов:\t%s\n", g, cnt[g], list[g];
}' "traces/$name.summary" | sort | awk -F'\t' '{ printf "%s\n   %s\n\n", $1, $2 }' > "traces/$name.groups"

cat "traces/$name.groups"
