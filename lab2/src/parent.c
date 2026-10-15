/*
 * Родительский процесс (вариант 6, группа 2).
 *  1. Читает из stdin первую строку — имя файла с командами.
 *  2. Открывает файл на чтение, создаёт pipe1.
 *  3. Порождает child: stdin <- файл, stdout -> pipe1.
 *  4. Читает из pipe1 и печатает прочитанное в свой stdout.
 *  5. Дожидается завершения child (нет зомби).
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "io_utils.h"
#include "process.h"

/* Убирает пробельные символы по краям строки (in-place). */
static char *trim(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == ' ' || s[n - 1] == '\t'))
        s[--n] = '\0';
    return s;
}

/* Читает имя файла первой строкой stdin. Возвращает копию (malloc) или NULL. */
static char *read_filename(void)
{
    line_reader_t lr;
    lr_init(&lr, STDIN_FILENO);

    char *line;
    ssize_t n = lr_next(&lr, &line);
    if (n == -2) {
        perror("parent: read(stdin)");
        lr_free(&lr);
        return NULL;
    }
    if (n == -1) {
        fprintf(stderr, "parent: no file name given (EOF on stdin)\n");
        lr_free(&lr);
        return NULL;
    }

    char *name = trim(line);
    if (*name == '\0') {
        fprintf(stderr, "parent: empty file name\n");
        lr_free(&lr);
        return NULL;
    }
    char *copy = strdup(name);
    if (copy == NULL)
        perror("parent: strdup");
    lr_free(&lr);
    return copy;
}

/* Перекачивает всё из fd в stdout, пока писатель не закроет канал (read вернёт 0). */
static int relay_pipe(int fd)
{
    char buf[4096];
    for (;;) {
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR)
                continue;
            perror("parent: read(pipe1)");
            return -1;
        }
        if (r == 0)
            return 0; /* EOF: все писатели закрыли pipe1 */
        if (write_all(STDOUT_FILENO, buf, (size_t)r) < 0) {
            perror("parent: write(stdout)");
            return -1;
        }
    }
}

int main(void)
{
    /* Запись в закрытый stdout (например, `./parent | head -1`) должна давать EPIPE,
     * а не убивать родителя до того, как он дождётся ребёнка. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if (sigemptyset(&sa.sa_mask) < 0 || sigaction(SIGPIPE, &sa, NULL) < 0) {
        perror("parent: sigaction(SIGPIPE)");
        return EXIT_FAILURE;
    }

    char child_path[PATH_MAX];
    if (resolve_child_path(child_path, sizeof(child_path)) < 0) {
        perror("parent: cannot locate child executable");
        return EXIT_FAILURE;
    }

    char *filename = read_filename();
    if (filename == NULL)
        return EXIT_FAILURE;

    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) {
        fprintf(stderr, "parent: open(%s): %s\n", filename, strerror(errno));
        free(filename);
        return EXIT_FAILURE;
    }
    free(filename);

    int pipe1[2];
    if (pipe(pipe1) < 0) {
        perror("parent: pipe");
        close(file_fd);
        return EXIT_FAILURE;
    }

    pid_t pid = spawn_redirected(child_path, file_fd, pipe1[1], pipe1[0]);
    if (pid < 0) {
        perror("parent: fork");
        close(file_fd);
        close(pipe1[0]);
        close(pipe1[1]);
        return EXIT_FAILURE;
    }
    log_msg("parent", "created child PID=%d", (int)pid);

    /* Родителю не нужны файл и пишущий конец канала. Если не закрыть pipe1[1],
     * read() в relay_pipe никогда не вернёт 0 — родитель зависнет. */
    if (close(file_fd) < 0)
        perror("parent: close(file)");
    if (close(pipe1[1]) < 0)
        perror("parent: close(pipe1[1])");

    int relay_rc = relay_pipe(pipe1[0]);

    /* Закрытие читающего конца: если мы прервались раньше EOF, ребёнок получит EPIPE и завершится. */
    if (close(pipe1[0]) < 0)
        perror("parent: close(pipe1[0])");

    int child_rc = wait_child(pid);

    if (relay_rc < 0 || child_rc != 0)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
