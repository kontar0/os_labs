#include "sum.h"

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const char *const SEPARATORS = " \t\v\f\r";

sum_status_t sum_line(char *line, int *result, const char **bad_token)
{
    int sum = 0;
    int count = 0;
    char *save = NULL;

    for (char *tok = strtok_r(line, SEPARATORS, &save); tok != NULL;
         tok = strtok_r(NULL, SEPARATORS, &save)) {
        char *end = NULL;
        errno = 0;
        long v = strtol(tok, &end, 10);

        if (end == tok || *end != '\0') {
            *bad_token = tok;
            return SUM_BAD_TOKEN;
        }
        if (errno == ERANGE || v < INT_MIN || v > INT_MAX) {
            *bad_token = tok;
            return SUM_RANGE;
        }
        if (__builtin_add_overflow(sum, (int)v, &sum))
            return SUM_OVERFLOW;
        count++;
    }

    if (count == 0)
        return SUM_EMPTY;
    *result = sum;
    return SUM_OK;
}

const char *sum_strerror(sum_status_t st)
{
    switch (st) {
    case SUM_OK:        return "ok";
    case SUM_EMPTY:     return "no numbers in line";
    case SUM_BAD_TOKEN: return "not an integer";
    case SUM_RANGE:     return "number out of int range";
    case SUM_OVERFLOW:  return "sum overflows int";
    }
    return "unknown error";
}
