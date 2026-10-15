#ifndef SUM_H
#define SUM_H

#define SUM_OK 0
#define SUM_EMPTY 1
#define SUM_ERROR 2

/*
 * Считает сумму целых чисел в строке вида "число число ... число".
 * SUM_OK: сумма записана в *result.
 * SUM_EMPTY: в строке нет чисел.
 * SUM_ERROR: в строке мусор или результат не влезает в int (причина печатается в stderr).
 */
int sum_line(const char *line, int *result);

#endif
