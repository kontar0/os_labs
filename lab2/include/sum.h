#ifndef SUM_H
#define SUM_H

typedef enum {
    SUM_OK = 0,
    SUM_EMPTY,     /* в строке нет ни одного числа */
    SUM_BAD_TOKEN, /* токен не является целым числом */
    SUM_RANGE,     /* число не помещается в int */
    SUM_OVERFLOW   /* сумма не помещается в int */
} sum_status_t;

/*
 * Разбирает строку вида "число число ... число" (разделители — пробелы/табуляции)
 * и считает сумму в типе int.
 * При ошибке SUM_BAD_TOKEN/SUM_RANGE в *bad_token кладётся указатель на проблемный токен
 * внутри line (строка модифицируется: токен завершается '\0').
 */
sum_status_t sum_line(char *line, int *result, const char **bad_token);

const char *sum_strerror(sum_status_t st);

#endif
