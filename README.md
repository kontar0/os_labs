# Лабораторные работы по курсу "Операционные системы" (Linux, POSIX)

| ЛР | Тема | Отчёт |
|---|---|---|
| 1 | Профиль системных вызовов `bash` через `strace` + сквозной профиль следующих ЛР | [lab1/REPORT.md](lab1/REPORT.md) |
| 2 | Процессы и каналы, группа 2, вариант 6 | [lab2/REPORT.md](lab2/REPORT.md) |

Объяснение всего, что используется в работах, и вопросы к защите: [EXPLANATION.md](EXPLANATION.md).

## Структура

```
lab1/
  bash.log       трасса: strace -f -y -o bash.log bash -c 'echo hello > out.txt; cat out.txt | wc -l; rm out.txt'
  summary.txt    сводка той же команды с -c
  REPORT.md
lab2/
  parent.c child.c process.c process.h sum.c sum.h Makefile
  tests/run_tests.sh, tests/data/
  docs/          run.log, strace.log, strace_summary.txt, valgrind.log
  REPORT.md
Dockerfile, linux.sh   Ubuntu 24.04 с gcc, strace, valgrind для запуска на Mac
```

## Запуск

На Linux:

```sh
make -C lab2 test
cd lab2 && echo tests/data/basic.txt | ./parent
cd lab1 && strace -f -y -o bash.log bash -c 'echo hello > out.txt; cat out.txt | wc -l; rm out.txt'
```

На Mac через Docker (из корня репозитория):

```sh
./linux.sh make -C lab2 test     # одна команда в контейнере
./linux.sh                       # интерактивный bash внутри Ubuntu, каталог репозитория смонтирован в /work
```
