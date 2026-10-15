<div align="center">

**Отчёт по лабораторной работе №2**
по курсу "Операционные системы"

**Тема: Управление процессами. Обмен данными между процессами через каналы**

Группа 2, вариант 6

Выполнил: студент группы ______ ____________________
Проверил: ____________________

2026

</div>

---

## 1. Цель работы

Научиться создавать процессы, перенаправлять их ввод-вывод и передавать данные между процессами через pipe.

## 2. Задание

Родитель читает из stdin имя файла и открывает его. Создаёт ребёнка (отдельная программа), у которого stdin подменён этим файлом, а stdout перенаправлен в pipe1. Ребёнок читает строки `число число число`, считает сумму (`int`) и пишет её в stdout. Родитель читает pipe1 и печатает в свой stdout.

## 3. Архитектура

```
parent: имя файла -> open -> pipe(pipe1) -> fork -> read(pipe1[0]) -> stdout -> waitpid
                                              │                ▲
child:  dup2(файл, 0), dup2(pipe1[1], 1) -> execl("./child") -> сумма -> write(1)
```

| Файл | Что делает |
|---|---|
| `parent.c` | родитель: имя файла, `open`, `pipe`, чтение pipe1, ожидание ребёнка |
| `process.c/.h` | `start_child` (`fork` + `dup2` + `execl`), `wait_child` (`waitpid`) |
| `child.c` | ребёнок: читает строки из stdin, пишет суммы в stdout |
| `sum.c/.h` | разбор строки через `strtol`, проверка мусора и выхода за `int` |

Сборка `make`, тесты `make test`. Сообщения с PID оба процесса пишут в stderr.

## 4. Ключевые фрагменты кода

Создание ребёнка (`process.c`):

```c
pid_t pid = fork();
if (pid == -1) {
    perror("parent: fork");
    return -1;
}
if (pid > 0)
    return pid;

if (dup2(in_fd, STDIN_FILENO) == -1) { perror("child: dup2 stdin"); _exit(1); }
if (dup2(out_fd, STDOUT_FILENO) == -1) { perror("child: dup2 stdout"); _exit(1); }
close_or_report(in_fd);
close_or_report(out_fd);
close_or_report(unused_fd);

execl(path, "child", (char *)NULL);
perror("child: execl");
_exit(1);
```

Родитель закрывает лишнее и читает трубу (`parent.c`). Без `close(pipe1[1])` `read` никогда не вернёт 0 и родитель зависнет:

```c
close_or_report(file_fd);
close_or_report(pipe1[1]);

while ((count = read(pipe1[0], buffer, sizeof(buffer))) > 0) {
    if (write(STDOUT_FILENO, buffer, count) != count) {
        perror("parent: write");
        exit_code = 1;
        break;
    }
}
close_or_report(pipe1[0]);
if (wait_child(child_pid) != 0)
    exit_code = 1;
```

Краевые случаи: плохие числа и выход за `int` пропускаются с сообщением; несуществующий файл и пустое имя дают ошибку; `SIGPIPE` игнорируется, поэтому обрыв трубы даёт ошибку `EPIPE`, а не падение процесса.

## 5. Результаты

Запуск ([docs/run.log](docs/run.log)):

```
$ echo tests/data/basic.txt | ./parent
[parent PID=1534] запущен, жду имя файла
[parent PID=1534] создан ребёнок PID=1535
[child PID=1535] запущен, родитель PID=1534
[child PID=1535] обработано строк: 4
6
100
0
42
[parent PID=1534] ребёнок PID=1535 завершился с кодом 0
```

Тесты `make test`: обычный файл, мусор и выход за `int`, несуществующий файл, пустое имя, обрыв трубы: все 5 пройдены.

Трасса ([docs/strace.log](docs/strace.log)), 1659 родитель, 1660 ребёнок:

```
1659 openat(AT_FDCWD, "tests/data/basic.txt", O_RDONLY) = 3
1659 pipe2([4, 5], 0) = 0
1659 clone(...) = 1660
1660 dup2(3, 0) = 0
1660 dup2(5, 1) = 1
1660 execve("./child", ["child"], ...) = 0
1660 write(1, "6\n", 2) = 2
1659 read(4, "6\n", 256) = 2
1659 read(4, "", 256) = 0
1659 wait4(1660, [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 1660
```

valgrind ([docs/valgrind.log](docs/valgrind.log)): утечек памяти нет, у обоих процессов на выходе открыты только 0, 1, 2. Зомби после завершения нет.

## 6. Выводы

1. Написаны две программы, обменивающиеся данными через pipe: stdin ребёнка подменён файлом, stdout каналом.
2. Перенаправление делается через `dup2` между `fork` и `exec`: дескрипторы сохраняются после `exec`.
3. Неиспользуемые концы трубы нужно закрывать, иначе родитель не получит конец данных и зависнет.
4. Все системные вызовы проверяются, родитель ждёт ребёнка через `waitpid`.
