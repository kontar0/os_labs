/*
 * Дочерний процесс (вариант 6).
 * stdin  — открытый родителем файл с командами (подменён через dup2),
 * stdout — пишущий конец pipe1 (подменён через dup2).
 * Для каждой строки «число число ... число» выводит сумму в stdout.
 * Диагностика и ошибки — в stderr (он не перенаправлен и идёт сразу в терминал).
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "io_utils.h"
#include "sum.h"

enum {
    CHILD_OK = 0,
    CHILD_BAD_LINES = 1, /* были некорректные строки, остальные обработаны */
    CHILD_IO_ERROR = 2   /* ошибка чтения/записи или обрыв канала */
};

int main(void)
{
    /* Если родитель закроет pipe раньше времени, write() вернёт EPIPE вместо
     * завершения процесса сигналом SIGPIPE — так мы сможем корректно закончить работу. */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if (sigemptyset(&sa.sa_mask) < 0 || sigaction(SIGPIPE, &sa, NULL) < 0) {
        perror("child: sigaction(SIGPIPE)");
        return CHILD_IO_ERROR;
    }

    log_msg("child", "started, PPID=%d", (int)getppid());

    line_reader_t lr;
    lr_init(&lr, STDIN_FILENO);

    int status = CHILD_OK;
    long line_no = 0;
    char *line;
    ssize_t n;

    while ((n = lr_next(&lr, &line)) >= 0) {
        line_no++;

        int sum;
        const char *bad = NULL;
        sum_status_t st = sum_line(line, &sum, &bad);

        if (st == SUM_EMPTY)
            continue; /* пустые строки просто пропускаем */
        if (st != SUM_OK) {
            if (bad)
                log_msg("child", "line %ld: %s: '%s'", line_no, sum_strerror(st), bad);
            else
                log_msg("child", "line %ld: %s", line_no, sum_strerror(st));
            status = CHILD_BAD_LINES;
            continue;
        }

        char out[32];
        int len = snprintf(out, sizeof(out), "%d\n", sum);
        if (write_all(STDOUT_FILENO, out, (size_t)len) < 0) {
            log_msg("child", "write to pipe failed: %s", strerror(errno));
            lr_free(&lr);
            return CHILD_IO_ERROR;
        }
    }

    if (n == -2) {
        log_msg("child", "read from stdin failed: %s", strerror(errno));
        status = CHILD_IO_ERROR;
    }

    lr_free(&lr);
    log_msg("child", "finished, %ld line(s) processed", line_no);
    return status;
}
