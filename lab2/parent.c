/*
 * Родительская программа (вариант 6).
 * Читает первой строкой stdin имя файла, открывает его, создаёт pipe1,
 * запускает child (stdin = файл, stdout = pipe1), печатает всё из pipe1
 * в свой stdout и ждёт завершения ребёнка.
 */
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "process.h"

/* Путь относительный: parent нужно запускать из каталога lab2, где лежит child. */
#define CHILD_PATH "./child"

static void close_or_report(int fd)
{
    if (close(fd) == -1)
        perror("parent: close");
}

int main(void)
{
    char filename[4096];
    int pipe1[2];
    int exit_code = 0;

    /*
     * Если наш stdout закроют (например, ./parent | head -1), write вернёт
     * ошибку EPIPE, а не убьёт процесс, и мы успеем дождаться ребёнка.
     */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("parent: signal");
        return 1;
    }

    fprintf(stderr, "[parent PID=%d] запущен, жду имя файла\n", getpid());
    if (fgets(filename, sizeof(filename), stdin) == NULL) {
        fprintf(stderr, "parent: имя файла не введено\n");
        return 1;
    }
    filename[strcspn(filename, "\n")] = '\0';
    if (filename[0] == '\0') {
        fprintf(stderr, "parent: пустое имя файла\n");
        return 1;
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        perror("parent: open");
        return 1;
    }

    /* Трубу создаём до fork: ребёнок получит копии обоих её концов. */
    if (pipe(pipe1) == -1) {
        perror("parent: pipe");
        close_or_report(file_fd);
        return 1;
    }

    pid_t child_pid = start_child(CHILD_PATH, file_fd, pipe1[1], pipe1[0]);
    if (child_pid == -1) {
        close_or_report(file_fd);
        close_or_report(pipe1[0]);
        close_or_report(pipe1[1]);
        return 1;
    }
    fprintf(stderr, "[parent PID=%d] создан ребёнок PID=%d\n", getpid(), child_pid);

    /*
     * Файл и конец записи нужны только ребёнку. read на трубе вернёт 0 (EOF)
     * только когда закрыты ВСЕ концы записи. Если не закрыть pipe1[1] здесь,
     * родитель сам держит конец записи открытым и навсегда зависнет в read.
     */
    close_or_report(file_fd);
    close_or_report(pipe1[1]);

    char buffer[256];
    ssize_t count;
    while ((count = read(pipe1[0], buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, count) != count) {
            perror("parent: write");
            exit_code = 1;
            break;
        }
    }
    if (count == -1) {
        perror("parent: read");
        exit_code = 1;
    }

    /* Если мы вышли из цикла раньше EOF, ребёнок после этого получит EPIPE и завершится. */
    close_or_report(pipe1[0]);

    /* Без waitpid ребёнок остался бы зомби до конца работы родителя. */
    if (wait_child(child_pid) != 0)
        exit_code = 1;
    return exit_code;
}
