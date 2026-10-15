#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include "process.h"

static void close_or_report(int fd)
{
    if (close(fd) == -1)
        perror("child: close");
}

pid_t start_child(const char *path, int in_fd, int out_fd, int unused_fd)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("parent: fork");
        return -1;
    }
    if (pid > 0)
        return pid;

    /* Дальше выполняется только ребёнок. Программа в нём пока ещё parent. */
    if (dup2(in_fd, STDIN_FILENO) == -1) {
        perror("child: dup2 stdin");
        _exit(1);
    }
    if (dup2(out_fd, STDOUT_FILENO) == -1) {
        perror("child: dup2 stdout");
        _exit(1);
    }

    /* Файл и труба теперь доступны как 0 и 1, старые номера больше не нужны. */
    close_or_report(in_fd);
    close_or_report(out_fd);
    close_or_report(unused_fd);

    /* exec заменяет программу, но таблица дескрипторов остаётся той же. */
    execl(path, "child", (char *)NULL);

    /* Сюда попадаем, только если exec не удался. */
    perror("child: execl");
    _exit(1);
}

int wait_child(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("parent: waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        fprintf(stderr, "[parent PID=%d] ребёнок PID=%d завершился с кодом %d\n",
                getpid(), pid, WEXITSTATUS(status));
        return WEXITSTATUS(status) == 0 ? 0 : 1;
    }
    if (WIFSIGNALED(status))
        fprintf(stderr, "[parent PID=%d] ребёнок PID=%d убит сигналом %d\n",
                getpid(), pid, WTERMSIG(status));
    return 1;
}
