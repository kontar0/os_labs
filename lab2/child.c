/*
 * Дочерняя программа.
 * stdin уже подменён родителем на файл, stdout на запись в pipe1.
 * Для каждой строки "число число ... число" пишет сумму в stdout.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "sum.h"

int main(void)
{
    char *line = NULL;
    size_t capacity = 0;
    int line_number = 0;
    int exit_code = 0;

    /* Если родитель закроет трубу, write вернёт ошибку EPIPE, а не убьёт процесс сигналом. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("child: signal");
        return 1;
    }

    fprintf(stderr, "[child PID=%d] запущен, родитель PID=%d\n", getpid(), getppid());

    /* getline сам выделяет и увеличивает буфер, поэтому длина строки любая. */
    while (getline(&line, &capacity, stdin) != -1) {
        line_number++;

        int sum;
        int status = sum_line(line, &sum);
        if (status == SUM_EMPTY)
            continue;
        if (status == SUM_ERROR) {
            fprintf(stderr, "[child PID=%d] строка %d пропущена\n", getpid(), line_number);
            exit_code = 1;
            continue;
        }

        char text[16];
        int length = snprintf(text, sizeof(text), "%d\n", sum);
        if (write(STDOUT_FILENO, text, length) != length) {
            perror("child: write");
            free(line);
            return 1;
        }
    }

    /* getline вернул -1: либо конец файла, либо ошибка чтения или нехватка памяти. */
    if (!feof(stdin)) {
        perror("child: getline");
        exit_code = 1;
    }

    free(line);
    fprintf(stderr, "[child PID=%d] обработано строк: %d\n", getpid(), line_number);
    return exit_code;
}
