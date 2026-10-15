#include "process.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "io_utils.h"

pid_t spawn_redirected(const char *path, int in_fd, int out_fd, int close_fd)
{
    pid_t pid = fork();
    if (pid != 0)
        return pid; /* родитель: PID ребёнка или -1 */

    /* --- дочерний процесс (до exec) --- */
    if (dup2(in_fd, STDIN_FILENO) < 0) {
        perror("child: dup2(stdin)");
        _exit(127);
    }
    if (dup2(out_fd, STDOUT_FILENO) < 0) {
        perror("child: dup2(stdout)");
        _exit(127);
    }
    /* Оригиналы больше не нужны: после dup2 они доступны как 0 и 1.
     * Особенно важно закрыть читающий конец канала, иначе он будет «висеть» в ребёнке. */
    if (in_fd != STDIN_FILENO && close(in_fd) < 0)
        perror("child: close(in_fd)");
    if (out_fd != STDOUT_FILENO && close(out_fd) < 0)
        perror("child: close(out_fd)");
    if (close_fd >= 0 && close(close_fd) < 0)
        perror("child: close(close_fd)");

    char *const argv[] = {(char *)path, NULL};
    execv(path, argv);
    /* сюда попадаем только если exec не удался */
    fprintf(stderr, "child: execv(%s): %s\n", path, strerror(errno));
    _exit(127); /* _exit, а не exit: не сбрасываем унаследованные stdio-буферы родителя */
}

int wait_child(pid_t pid)
{
    int wstatus;
    pid_t r;

    do {
        r = waitpid(pid, &wstatus, 0);
    } while (r < 0 && errno == EINTR);

    if (r < 0) {
        perror("parent: waitpid");
        return -1;
    }
    if (WIFEXITED(wstatus)) {
        log_msg("parent", "child PID=%d exited with code %d", (int)pid, WEXITSTATUS(wstatus));
        return WEXITSTATUS(wstatus);
    }
    if (WIFSIGNALED(wstatus)) {
        log_msg("parent", "child PID=%d killed by signal %d (%s)", (int)pid,
                WTERMSIG(wstatus), strsignal(WTERMSIG(wstatus)));
        return 128 + WTERMSIG(wstatus);
    }
    return -1;
}

int resolve_child_path(char *buf, size_t size)
{
    ssize_t n = readlink("/proc/self/exe", buf, size - 1);
    if (n < 0)
        return -1;
    buf[n] = '\0';

    char *slash = strrchr(buf, '/');
    size_t dir_len = slash ? (size_t)(slash - buf) + 1 : 0;
    const char *name = "child";
    if (dir_len + strlen(name) + 1 > size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    strcpy(buf + dir_len, name);
    return 0;
}
