/*
 * Дочерняя программа (одна и та же для child1 и child2).
 * stdin уже подменён родителем на конец чтения своей трубы,
 * stdout на открытый на запись файл.
 * Каждую строку из stdin пишет в stdout без гласных.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vowels.h"

int main(int argc, char *argv[])
{
    /* Имя (child1 или child2) родитель передал через execl как argv[0]. */
    const char *name = argc > 0 ? argv[0] : "child";
    char *line = NULL;
    size_t capacity = 0;
    int line_count = 0;
    int exit_code = 0;

    fprintf(stderr, "[%s PID=%d] запущен, родитель PID=%d\n", name, getpid(), getppid());

    /* getline сам выделяет и увеличивает буфер, поэтому длина строки любая. */
    while (getline(&line, &capacity, stdin) != -1) {
        line_count++;
        remove_vowels(line);

        ssize_t length = strlen(line);
        if (write(STDOUT_FILENO, line, length) != length) {
            perror("child: write");
            free(line);
            return 1;
        }
    }

    /* getline вернул -1: либо родитель закрыл трубу (EOF), либо ошибка. */
    if (!feof(stdin)) {
        perror("child: getline");
        exit_code = 1;
    }

    free(line);
    fprintf(stderr, "[%s PID=%d] обработано строк: %d\n", name, getpid(), line_count);
    return exit_code;
}
