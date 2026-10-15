#include "io_utils.h"

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void lr_init(line_reader_t *lr, int fd)
{
    lr->fd = fd;
    lr->pos = 0;
    lr->len = 0;
    lr->line = NULL;
    lr->line_cap = 0;
}

void lr_free(line_reader_t *lr)
{
    free(lr->line);
    lr->line = NULL;
    lr->line_cap = 0;
}

/* Гарантирует, что в lr->line помещается need байт. */
static int ensure_cap(line_reader_t *lr, size_t need)
{
    if (need <= lr->line_cap)
        return 0;
    size_t cap = lr->line_cap ? lr->line_cap : 128;
    while (cap < need) {
        if (cap > SIZE_MAX / 2) {
            errno = ENOMEM;
            return -1;
        }
        cap *= 2;
    }
    char *p = realloc(lr->line, cap);
    if (p == NULL)
        return -1; /* errno = ENOMEM выставлен realloc */
    lr->line = p;
    lr->line_cap = cap;
    return 0;
}

ssize_t lr_next(line_reader_t *lr, char **out)
{
    size_t n = 0;
    int got_any = 0;

    for (;;) {
        if (lr->pos == lr->len) {
            ssize_t r = read(lr->fd, lr->buf, sizeof(lr->buf));
            if (r < 0) {
                if (errno == EINTR)
                    continue;
                return -2;
            }
            if (r == 0) { /* EOF: отдаём последнюю строку без '\n', если она есть */
                if (!got_any)
                    return -1;
                break;
            }
            lr->pos = 0;
            lr->len = (size_t)r;
        }

        got_any = 1;
        char *start = lr->buf + lr->pos;
        size_t avail = lr->len - lr->pos;
        char *nl = memchr(start, '\n', avail);
        size_t chunk = nl ? (size_t)(nl - start) : avail;

        if (ensure_cap(lr, n + chunk + 1) < 0)
            return -2;
        memcpy(lr->line + n, start, chunk);
        n += chunk;
        lr->pos += chunk;

        if (nl) {
            lr->pos++; /* пропускаем сам '\n' */
            break;
        }
    }

    if (ensure_cap(lr, n + 1) < 0)
        return -2;
    if (n > 0 && lr->line[n - 1] == '\r') /* файлы, сохранённые в Windows */
        n--;
    lr->line[n] = '\0';
    *out = lr->line;
    return (ssize_t)n;
}

int write_all(int fd, const void *data, size_t len)
{
    const char *p = data;
    while (len > 0) {
        ssize_t w = write(fd, p, len);
        if (w < 0) {
            if (errno == EINTR)
                continue;
            return -1;
        }
        p += w;
        len -= (size_t)w;
    }
    return 0;
}

int write_str(int fd, const char *s)
{
    return write_all(fd, s, strlen(s));
}

void log_msg(const char *role, const char *fmt, ...)
{
    char msg[512];
    char line[640];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    int n = snprintf(line, sizeof(line), "[%s PID=%d] %s\n", role, (int)getpid(), msg);
    if (n < 0)
        return;
    if ((size_t)n >= sizeof(line))
        n = (int)sizeof(line) - 1;
    /* Одним write(), чтобы строки разных процессов не перемешивались. Ошибку записи
     * в stderr сообщить некуда, поэтому её игнорируем осознанно. */
    (void)write_all(STDERR_FILENO, line, (size_t)n);
}
