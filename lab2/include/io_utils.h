#ifndef IO_UTILS_H
#define IO_UTILS_H

#include <stddef.h>
#include <sys/types.h>

/*
 * Буферизованное построчное чтение поверх read(2).
 * Строка может быть произвольной длины: буфер строки растёт через realloc.
 */
typedef struct {
    int fd;
    char buf[4096]; /* сырые данные, полученные одним вызовом read() */
    size_t pos;     /* позиция первого непрочитанного байта в buf */
    size_t len;     /* количество валидных байт в buf */
    char *line;     /* текущая строка (без '\n'), владеет reader */
    size_t line_cap;
} line_reader_t;

void lr_init(line_reader_t *lr, int fd);

/*
 * Читает следующую строку.
 * Возвращает длину строки (>= 0) и кладёт указатель на неё в *out,
 * -1 при EOF (данных больше нет), -2 при ошибке (errno сохранён).
 * Указатель действителен до следующего вызова lr_next/lr_free.
 */
ssize_t lr_next(line_reader_t *lr, char **out);

void lr_free(line_reader_t *lr);

/* Пишет ровно len байт, повторяя write(2) при частичной записи и EINTR. 0 — успех, -1 — ошибка. */
int write_all(int fd, const void *data, size_t len);

/* Удобная обёртка: write_all для C-строки. */
int write_str(int fd, const char *s);

/* Вывод сообщения "[role PID=..] msg" в stderr — для демонстрации конкурентного выполнения. */
void log_msg(const char *role, const char *fmt, ...);

#endif
