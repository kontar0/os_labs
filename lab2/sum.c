#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sum.h"

int sum_line(const char *line, int *result)
{
    const char *position = line;
    long long sum = 0;
    int count = 0;

    while (1) {
        while (isspace((unsigned char)*position))
            position++;
        if (*position == '\0')
            break;

        char *end;
        errno = 0;
        long value = strtol(position, &end, 10);

        /* end == position: число не началось; после числа должен быть пробел или конец строки. */
        if (end == position || (*end != '\0' && !isspace((unsigned char)*end))) {
            fprintf(stderr, "child: не целое число: %.*s\n", (int)strcspn(position, " \t\r\n"), position);
            return SUM_ERROR;
        }
        if (errno == ERANGE || value < INT_MIN || value > INT_MAX) {
            fprintf(stderr, "child: число не влезает в int: %.*s\n", (int)(end - position), position);
            return SUM_ERROR;
        }

        sum += value;
        count++;
        position = end;
    }

    if (count == 0)
        return SUM_EMPTY;

    /* Сумма считается в long long, поэтому переполнение int можно просто проверить сравнением. */
    if (sum < INT_MIN || sum > INT_MAX) {
        fprintf(stderr, "child: сумма не влезает в int: %lld\n", sum);
        return SUM_ERROR;
    }

    *result = (int)sum;
    return SUM_OK;
}
