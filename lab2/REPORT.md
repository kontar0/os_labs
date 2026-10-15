<div align="center">

**Отчёт по лабораторной работе №2**
по курсу «Операционные системы»

**Тема: Управление процессами. Обмен данными между процессами через каналы**

Вариант 6

Выполнил: студент группы ______ ____________________
Проверил: ____________________

2026

</div>

---

## 1. Цель работы

Получить практические навыки управления процессами в ОС и обмена данными между процессами через каналы (pipe).

## 2. Задание (группа 2, вариант 6)

Родительский процесс создаёт дочерний. Пользователь вводит в консоль родителя первой строкой имя файла. Этот файл открывается на чтение и **подменяет стандартный ввод дочернего процесса**, а **стандартный вывод дочернего процесса перенаправляется в pipe1**. Родитель читает из pipe1 и выводит прочитанное в свой stdout. Родитель и ребёнок — разные программы.

В файле записаны команды вида `число число число<endline>`. Дочерний процесс считает сумму чисел и выводит её в stdout. Числа имеют тип `int`, их количество в строке может быть любым.

**Платформа:** Linux (Ubuntu 24.04 LTS, gcc 13.3), POSIX API.

## 3. Архитектура решения

```
                    stdin (пользователь: имя файла)
                             │
                             ▼
 ┌──────────────────────────────────────────────┐
 │ parent                                       │
 │  read(0) → имя файла                         │
 │  open(file, O_RDONLY) → fd_file              │
 │  pipe(pipe1)                                 │
 │  fork() ─────────────────┐                   │
 │  close(fd_file), close(pipe1[1])             │
 │  read(pipe1[0]) → write(1)  ◄──────┐         │──► stdout (суммы)
 │  waitpid(child)                    │         │
 └────────────────────────────────────│─────────┘
                            │         │ pipe1
                            ▼         │
 ┌────────────────────────────────────│─────────┐
 │ child (после fork, до exec):       │         │
 │  dup2(fd_file, 0)   stdin  ← File  │         │
 │  dup2(pipe1[1], 1)  stdout → pipe1─┘         │
 │  close(...) лишних дескрипторов              │
 │  execv("child")                              │
 │                                              │
 │ child (программа): читает stdin построчно,   │
 │  считает сумму, write(1, "сумма\n")          │──► stderr (диагностика)
 └──────────────────────────────────────────────┘
```

**Состав проекта:**

| Файл | Назначение |
|---|---|
| [src/parent.c](src/parent.c) | Родитель: ввод имени файла, `open`, `pipe`, пересылка данных из канала в stdout, код возврата |
| [src/process.c](src/process.c), [include/process.h](include/process.h) | Создание процесса с перенаправлением (`fork`/`dup2`/`execv`), ожидание (`waitpid`), поиск исполняемого файла `child` |
| [src/child.c](src/child.c) | Дочерняя программа: читает команды из stdin, пишет суммы в stdout |
| [src/sum.c](src/sum.c), [include/sum.h](include/sum.h) | Разбор строки и сумма в `int` с проверкой диапазона и переполнения |
| [src/io_utils.c](src/io_utils.c), [include/io_utils.h](include/io_utils.h) | Чтение строк любой длины поверх `read()`, `write_all()`, журнал с PID |
| [Makefile](Makefile) | Сборка (`make`), тесты (`make test`), трассировка (`make trace`) |
| [tests/run_tests.sh](tests/run_tests.sh) | Автотесты, включая краевые случаи |

**Протокол обмена:** ребёнок пишет в pipe1 по одной строке на каждую команду: `"<сумма>\n"`. Сообщения об ошибках во входных данных идут в stderr ребёнка. stderr не перенаправлен, поэтому они сразу выводятся в терминал и не смешиваются с результатами. Конец передачи — закрытие канала: ребёнок завершается, `read()` у родителя возвращает 0.

**Коды завершения child:** 0 — всё обработано; 1 — были некорректные строки (остальные строки обработаны); 2 — ошибка ввода-вывода или обрыв канала. Код 127 означает, что не удался `dup2`/`execv`. Родитель возвращает 0, только если ребёнок завершился с кодом 0.

## 4. Ключевые фрагменты кода

### 4.1. Создание процесса с перенаправлением потоков (`src/process.c`)

```c
pid_t spawn_redirected(const char *path, int in_fd, int out_fd, int close_fd)
{
    pid_t pid = fork();
    if (pid != 0)
        return pid; /* родитель: PID ребёнка или -1 */

    /* --- дочерний процесс (до exec) --- */
    if (dup2(in_fd, STDIN_FILENO) < 0) {
        perror("child: dup2(stdin)");
        _exit(127);
    }
    if (dup2(out_fd, STDOUT_FILENO) < 0) {
        perror("child: dup2(stdout)");
        _exit(127);
    }
    if (in_fd != STDIN_FILENO && close(in_fd) < 0)
        perror("child: close(in_fd)");
    if (out_fd != STDOUT_FILENO && close(out_fd) < 0)
        perror("child: close(out_fd)");
    if (close_fd >= 0 && close(close_fd) < 0)
        perror("child: close(close_fd)");

    char *const argv[] = {(char *)path, NULL};
    execv(path, argv);
    fprintf(stderr, "child: execv(%s): %s\n", path, strerror(errno));
    _exit(127); /* _exit, а не exit: не сбрасываем унаследованные stdio-буферы родителя */
}
```

`dup2` выполняется **между `fork` и `exec`**: дескрипторы наследуются через `exec`, и программа `child` получает готовые stdin/stdout. Сама она ничего не знает ни о файле, ни о канале. Ребёнок закрывает читающий конец `pipe1[0]`, иначе в нём остался бы лишний дескриптор канала.

### 4.2. Родитель: закрытие лишних концов и пересылка данных (`src/parent.c`)

```c
    pid_t pid = spawn_redirected(child_path, file_fd, pipe1[1], pipe1[0]);
    if (pid < 0) {
        perror("parent: fork");
        close(file_fd); close(pipe1[0]); close(pipe1[1]);
        return EXIT_FAILURE;
    }
    log_msg("parent", "created child PID=%d", (int)pid);

    /* Если не закрыть pipe1[1], read() никогда не вернёт 0 — родитель зависнет. */
    if (close(file_fd) < 0)
        perror("parent: close(file)");
    if (close(pipe1[1]) < 0)
        perror("parent: close(pipe1[1])");

    int relay_rc = relay_pipe(pipe1[0]);
    if (close(pipe1[0]) < 0)
        perror("parent: close(pipe1[0])");

    int child_rc = wait_child(pid);
```

```c
static int relay_pipe(int fd)
{
    char buf[4096];
    for (;;) {
        ssize_t r = read(fd, buf, sizeof(buf));
        if (r < 0) {
            if (errno == EINTR)
                continue;
            perror("parent: read(pipe1)");
            return -1;
        }
        if (r == 0)
            return 0; /* EOF: все писатели закрыли pipe1 */
        if (write_all(STDOUT_FILENO, buf, (size_t)r) < 0) {
            perror("parent: write(stdout)");
            return -1;
        }
    }
}
```

### 4.3. Ожидание ребёнка — защита от зомби (`src/process.c`)

```c
int wait_child(pid_t pid)
{
    int wstatus;
    pid_t r;
    do {
        r = waitpid(pid, &wstatus, 0);
    } while (r < 0 && errno == EINTR);

    if (r < 0) { perror("parent: waitpid"); return -1; }
    if (WIFEXITED(wstatus)) { ... return WEXITSTATUS(wstatus); }
    if (WIFSIGNALED(wstatus)) { ... return 128 + WTERMSIG(wstatus); }
    return -1;
}
```

### 4.4. Обработка обрыва канала (SIGPIPE)

```c
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if (sigemptyset(&sa.sa_mask) < 0 || sigaction(SIGPIPE, &sa, NULL) < 0) {
        perror("child: sigaction(SIGPIPE)");
        return CHILD_IO_ERROR;
    }
```

Пусть читатель канала исчез (например, родитель завершил чтение досрочно). По умолчанию `write()` в такой канал убивает процесс сигналом SIGPIPE. Мы игнорируем сигнал, поэтому `write()` возвращает `-1/EPIPE`, а ребёнок пишет диагностику и завершается с кодом 2.

### 4.5. Подсчёт суммы с проверкой краевых случаев (`src/sum.c`)

```c
        errno = 0;
        long v = strtol(tok, &end, 10);
        if (end == tok || *end != '\0') { *bad_token = tok; return SUM_BAD_TOKEN; }
        if (errno == ERANGE || v < INT_MIN || v > INT_MAX) { *bad_token = tok; return SUM_RANGE; }
        if (__builtin_add_overflow(sum, (int)v, &sum))
            return SUM_OVERFLOW;
```

## 5. Сборка и запуск

```sh
make            # build/parent, build/child (флаги -Wall -Wextra -Wpedantic -Werror)
make test       # автотесты
make trace      # трасса strace -f в docs/strace.log
./build/parent  # первой строкой ввести имя файла
```

На macOS/Windows всё запускается в контейнере Ubuntu из корня репозитория: `./linux.sh make -C lab2 test`.

## 6. Результаты работы

### 6.1. Демонстрация (PID родителя и ребёнка видны в выводе) — [docs/run.log](docs/run.log)

```
$ cat tests/data/basic.txt
1 2 3
10 20 30 40
-5 5
42

$ ./build/parent
tests/data/basic.txt   <- ввод пользователя
[parent PID=9] created child PID=10
[child PID=10] started, PPID=9
[child PID=10] finished, 4 line(s) processed
6
100
0
42
[parent PID=9] child PID=10 exited with code 0
exit code: 0
```

Строки `[... PID=...]` выводятся в stderr, суммы — в stdout родителя. Порядок строк из разных процессов меняется от запуска к запуску, потому что процессы работают параллельно.

### 6.2. Краевые случаи

```
$ cat tests/data/edge.txt
1 2 3
                                  <- пустая строка
   7    8	9                     <- лишние пробелы и табуляция
abc 1
1 2x
2147483647 1
2147483648
-2147483648 0
100

[parent PID=13] created child PID=14
[child PID=14] started, PPID=13
[child PID=14] line 4: not an integer: 'abc'
[child PID=14] line 5: not an integer: '2x'
6
24
[child PID=14] line 6: sum overflows int
[child PID=14] line 7: number out of int range: '2147483648'
[child PID=14] finished, 9 line(s) processed
-2147483648
100
[parent PID=13] child PID=14 exited with code 1

$ ./build/parent  (ввод: no_such_file.txt)
parent: open(no_such_file.txt): No such file or directory
exit code: 1
```

### 6.3. Автотесты — `make test`

```
PASS  basic sums
PASS  edge cases
PASS  empty file
PASS  no trailing newline
PASS  file name with spaces
PASS  missing file
PASS  empty file name
PASS  200k numbers in a line
PASS  broken pipe handled (child exits with code 2, no crash)
ALL TESTS PASSED
```

Тест «broken pipe» запускает `./build/parent | head -n 1`, подавая файл из 300 000 строк. После первой строки `head` закрывает канал, родитель получает `EPIPE` и закрывает pipe1. Затем ребёнок тоже получает `EPIPE` и завершается с кодом 2. Ни один процесс не падает и не зависает, родитель дожидается ребёнка.

### 6.4. Утечки памяти и дескрипторов — valgrind ([docs/valgrind.log](docs/valgrind.log))

`valgrind --leak-check=full --trace-children=yes --track-fds=yes` на `edge.txt`, для обоих процессов:

```
==19== FILE DESCRIPTORS: 3 open (3 std) at exit.
==19== All heap blocks were freed -- no leaks are possible
==19== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
==20== FILE DESCRIPTORS: 3 open (3 std) at exit.
==20== All heap blocks were freed -- no leaks are possible
==20== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

После завершения в системе нет зомби-процессов (`ps -eo stat | grep Z` — пусто).

### 6.5. Трасса системных вызовов — `make trace` ([docs/strace.log](docs/strace.log))

Фрагмент без загрузки библиотек (13 — родитель, 14 — ребёнок):

```
13 execve("./build/parent", ["./build/parent"], ...)              = 0
13 rt_sigaction(SIGPIPE, {sa_handler=SIG_IGN, ...}, NULL, 8)       = 0
13 read(0, "tests/data/basic.txt\n", 4096)                         = 21     # имя файла
13 openat(AT_FDCWD, "tests/data/basic.txt", O_RDONLY)              = 3      # open()
13 pipe2([4, 5], 0)                                                = 0      # pipe()
13 clone(child_stack=NULL, flags=...|SIGCHLD, ...)                 = 14     # fork()
14 dup3(3, 0, 0)                                                   = 0      # dup2(file, stdin)
13 close(5)                                                        = 0      # родитель закрыл запись
14 dup3(5, 1, 0)                                                   = 1      # dup2(pipe1[1], stdout)
14 close(5) / close(4)                                             = 0
14 execve("/work/lab2/build/child", ["/work/lab2/build/child"], ...) = 0
14 rt_sigaction(SIGPIPE, {sa_handler=SIG_IGN, ...}, NULL, 8)       = 0
14 read(0, "1 2 3\n10 20 30 40\n-5 5\n42\n", 4096)                  = 26     # stdin = файл
14 write(1, "6\n", 2)                                              = 2      # stdout = pipe1
13 read(4, "6\n", 4096)                                            = 2      # родитель читает pipe1
13 write(1, "6\n", 2)                                              = 2      # ... и печатает
   ...
14 read(0, "", 4096)                                               = 0      # EOF файла
14 exit_group(0)                                                   = ?
13 read(4, "", 4096)                                               = 0      # EOF канала
13 close(4)                                                        = 0
13 --- SIGCHLD {si_pid=14, si_status=0, ...} ---
13 wait4(14, [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL)    = 14     # waitpid()
13 exit_group(0)                                                   = ?
```

Ядро aarch64 не имеет отдельных `dup2`, `pipe`, `fork`, `open`, поэтому glibc вызывает `dup3`, `pipe2`, `clone`, `openat`. На x86-64 в трассе будут `dup2` и `pipe`. Видно, что вызовы `write` ребёнка и `read` родителя чередуются: процессы работают одновременно, а канал служит буфером между ними.

## 7. Выводы

1. Написаны две программы, `parent` и `child`. Они обмениваются данными через неименованный канал, как требует вариант 6: stdin ребёнка подменён файлом, stdout ребёнка — каналом.
2. Перенаправление потоков сделано через `dup2()` между `fork()` и `execv()`. Дескрипторы наследуются через `exec`, поэтому дочерняя программа работает с обычными stdin/stdout.
3. Для корректного EOF нужно закрывать неиспользуемые концы канала в обоих процессах. Если родитель не закроет `pipe1[1]`, он навсегда заблокируется в `read()`.
4. Все системные вызовы проверяются на ошибки (`perror`/`strerror`). Отдельно обработаны `EINTR`, частичная запись, обрыв канала (`SIGPIPE`/`EPIPE`), отсутствующий файл, некорректные числа, выход за диапазон `int` и переполнение суммы.
5. Ресурсы освобождаются: valgrind не находит утечек памяти и открытых дескрипторов, ребёнок всегда дожидается через `waitpid`, зомби не остаётся.
