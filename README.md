# Лабораторные работы по курсу «Операционные системы» (Linux, POSIX)

| ЛР | Тема | Отчёт |
|---|---|---|
| 1 | Профиль системных вызовов (`bash` под `strace`) + сквозной профиль ЛР | [lab1/REPORT.md](lab1/REPORT.md) |
| 2 | Процессы и каналы, вариант 6 | [lab2/REPORT.md](lab2/REPORT.md) |

## Окружение

Работы рассчитаны на Ubuntu 24.04 LTS (gcc, make, strace). На Linux всё запускается напрямую:

```sh
make -C lab2 test
cd lab1 && ./profile.sh bash bash scenario.sh
```

На macOS/Windows используется Docker-контейнер с чистой Ubuntu ([Dockerfile](Dockerfile)):

```sh
./linux.sh make -C lab2 test     # одна команда
./linux.sh                       # интерактивный shell внутри Ubuntu
```
