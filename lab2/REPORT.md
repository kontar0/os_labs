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

Научиться создавать процессы, запускать в них другие программы, перенаправлять их ввод и вывод и передавать данные между процессами через канал (pipe).

## 2. Задание

Родитель читает из stdin первой строкой имя файла и открывает его на чтение. Затем создаёт дочерний процесс (это отдельная программа), у которого stdin подменён этим файлом, а stdout перенаправлен в pipe1. Ребёнок читает строки вида `число число число`, считает сумму (тип `int`, чисел в строке сколько угодно) и пишет её в stdout. Родитель читает pipe1 и печатает прочитанное в свой stdout.

Платформа: Linux (Ubuntu 24.04), gcc, только POSIX API.

## 3. Как устроено

```
 пользователь: "tests/data/basic.txt"
        │ stdin
        ▼
 ┌─────────────── parent ───────────────┐
 │ fgets -> имя файла                   │
 │ open(файл) = 3                       │
 │ pipe(pipe1): [0] чтение, [1] запись  │
 │ fork ───────────────┐                │
 │ close(3), close(pipe1[1])            │
 │ read(pipe1[0]) -> write(1) ◄──────┐  │ ──► stdout: суммы
 │ waitpid(child)                    │  │
 └───────────────────────────────────│──┘
                       │             │ pipe1
                       ▼             │
 ┌─────── ребёнок (до exec) ─────────│──┐
 │ dup2(3, 0)        stdin  = файл   │  │
 │ dup2(pipe1[1], 1) stdout = pipe1 ─┘  │
 │ close(лишние номера)                 │
 │ execl("./child")                     │
 ├─────── программа child ──────────────┤
 │ getline(stdin) -> сумма -> write(1)  │ ──► stderr: сообщения с PID
 └──────────────────────────────────────┘
```

| Файл | Что в нём |
|---|---|
| [parent.c](parent.c) | родитель: имя файла, `open`, `pipe`, чтение из трубы, код возврата |
| [process.c](process.c), [process.h](process.h) | `start_child`: `fork` + `dup2` + `execl`; `wait_child`: `waitpid` |
| [child.c](child.c) | ребёнок: читает строки из stdin, пишет суммы в stdout |
| [sum.c](sum.c), [sum.h](sum.h) | разбор строки через `strtol` и проверки на мусор и выход за `int` |
| [Makefile](Makefile) | `make`, `make test`, `make clean` |
| [tests/run_tests.sh](tests/run_tests.sh) | 5 тестов |

Ребёнок пишет по одной строке на каждую сумму. Сообщения с PID и ошибки оба процесса пишут в stderr, он не перенаправлен, поэтому они не смешиваются с суммами. Конец данных: ребёнок завершается, у трубы не остаётся концов записи, `read` у родителя возвращает 0.

Коды выхода у обеих программ: 0 всё хорошо, 1 была ошибка (плохие строки в файле, нет файла, обрыв трубы и т. п.). Родитель возвращает 1, если ребёнок завершился не с 0.

Путь к ребёнку фиксированный: `./child`. Поэтому `parent` запускается из каталога `lab2`, где `make` кладёт обе программы. Если запустить из другого места, `execl` вернёт ошибку `No such file or directory`, ребёнок напечатает её и выйдет с кодом 1.

## 4. Ключевые фрагменты кода

### 4.1. Создание ребёнка с перенаправлением ([process.c](process.c))

```c
pid_t start_child(const char *path, int in_fd, int out_fd, int unused_fd)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("parent: fork");
        return -1;
    }
    if (pid > 0)
        return pid;

    /* Дальше выполняется только ребёнок. Программа в нём пока ещё parent. */
    if (dup2(in_fd, STDIN_FILENO) == -1) {
        perror("child: dup2 stdin");
        _exit(1);
    }
    if (dup2(out_fd, STDOUT_FILENO) == -1) {
        perror("child: dup2 stdout");
        _exit(1);
    }

    close_or_report(in_fd);
    close_or_report(out_fd);
    close_or_report(unused_fd);

    execl(path, "child", (char *)NULL);

    perror("child: execl");
    _exit(1);
}
```

`dup2` делается между `fork` и `exec`: `exec` меняет программу, но таблица дескрипторов остаётся. Программа `child` просто читает 0 и пишет в 1 и не знает, что за ними файл и труба. Ребёнок закрывает конец чтения `pipe1[0]`, он ему не нужен.

### 4.2. Закрытие концов трубы и чтение ([parent.c](parent.c))

```c
    /*
     * Файл и конец записи нужны только ребёнку. read на трубе вернёт 0 (EOF)
     * только когда закрыты ВСЕ концы записи. Если не закрыть pipe1[1] здесь,
     * родитель сам держит конец записи открытым и навсегда зависнет в read.
     */
    close_or_report(file_fd);
    close_or_report(pipe1[1]);

    char buffer[256];
    ssize_t count;
    while ((count = read(pipe1[0], buffer, sizeof(buffer))) > 0) {
        if (write(STDOUT_FILENO, buffer, count) != count) {
            perror("parent: write");
            exit_code = 1;
            break;
        }
    }
    if (count == -1) {
        perror("parent: read");
        exit_code = 1;
    }

    close_or_report(pipe1[0]);
```

Проверено: если закомментировать `close_or_report(pipe1[1])`, родитель печатает все суммы и висит, `ps` показывает его в состоянии `S` с `wchan = anon_pipe_read`, а ребёнка как зомби (`Z`), потому что до `waitpid` дело не доходит.

### 4.3. Ожидание ребёнка ([process.c](process.c))

```c
int wait_child(pid_t pid)
{
    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("parent: waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        fprintf(stderr, "[parent PID=%d] ребёнок PID=%d завершился с кодом %d\n",
                getpid(), pid, WEXITSTATUS(status));
        return WEXITSTATUS(status) == 0 ? 0 : 1;
    }
    if (WIFSIGNALED(status))
        fprintf(stderr, "[parent PID=%d] ребёнок PID=%d убит сигналом %d\n",
                getpid(), pid, WTERMSIG(status));
    return 1;
}
```

### 4.4. Краевые случаи

- **Числа** ([sum.c](sum.c)): `strtol` + проверка, что после числа идёт пробел или конец строки (`abc`, `2x` это ошибка), и что число влезает в `int`. Сумма копится в `long long`, в конце сравнивается с `INT_MIN`/`INT_MAX`. Пустые строки пропускаются, плохие строки пропускаются с сообщением, остальные считаются.
- **Длина строки и память**: `getline` сам увеличивает буфер. Если памяти не хватит, `getline` вернёт -1 без признака конца файла, ребёнок напечатает ошибку и выйдет с кодом 1.
- **Обрыв связи**: оба процесса делают `signal(SIGPIPE, SIG_IGN)`. Тогда запись в трубу без читателя не убивает процесс, а `write` возвращает ошибку `EPIPE`. Пример: `./parent | head -n 1`. `head` выходит, `write` родителя даёт `EPIPE`, родитель закрывает `pipe1[0]`, `write` ребёнка тоже даёт `EPIPE`, ребёнок выходит с кодом 1, родитель его дожидается.
- **Имя файла**: пустая строка, конец ввода без строки, несуществующий файл дают сообщение и код 1.

## 5. Сборка и запуск

```sh
cd lab2
make                               # gcc -Wall -Wextra -g, без предупреждений
make test
echo tests/data/basic.txt | ./parent
./parent                           # или ввести имя файла руками
```

На Mac из корня репозитория: `./linux.sh make -C lab2 test` или `./linux.sh` и дальше команды внутри контейнера.

## 6. Результаты

### 6.1. Запуск ([docs/run.log](docs/run.log))

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
код возврата: 0
```

Файл `basic.txt`: `1 2 3`, `10 20 30 40`, `-5 5`, `42`. Порядок строк из stderr и stdout от запуска к запуску разный: процессы работают параллельно.

Файл с ошибками `edge.txt` (пустая строка, табуляции, `abc 1`, `1 2x`, `2147483647 1`, `2147483648`, `-2147483648 0`, последняя строка `100` без перевода строки):

```
6
child: не целое число: abc
[child PID=1539] строка 4 пропущена
child: не целое число: 2x
[child PID=1539] строка 5 пропущена
child: сумма не влезает в int: 2147483648
[child PID=1539] строка 6 пропущена
child: число не влезает в int: 2147483648
[child PID=1539] строка 7 пропущена
[child PID=1539] обработано строк: 9
24
-2147483648
100
[parent PID=1538] ребёнок PID=1539 завершился с кодом 1
```

### 6.2. Тесты (`make test`)

```
OK    обычный файл
OK    пустые строки, мусор, выход за int
OK    несуществующий файл
OK    пустое имя файла
OK    обрыв трубы: оба получили EPIPE, ребёнок дождан
ВСЕ ТЕСТЫ ПРОЙДЕНЫ
```

### 6.3. Трасса ([docs/strace.log](docs/strace.log))

```sh
echo tests/data/basic.txt | strace -f -o docs/strace.log \
    -e trace=openat,pipe2,clone,dup2,dup3,execve,read,write,close,wait4 ./parent
```

Главное из трассы (1659 родитель, 1660 ребёнок, загрузка libc и строки `unfinished/resumed` убраны):

```
1659 read(0, "tests/data/basic.txt\n", 4096) = 21
1659 openat(AT_FDCWD, "tests/data/basic.txt", O_RDONLY) = 3
1659 pipe2([4, 5], 0)                  = 0
1659 clone(...SIGCHLD...)              = 1660
1659 close(3) = 0;  close(5) = 0         родитель закрыл файл и конец записи
1660 dup2(3, 0) = 0;  dup2(5, 1) = 1     ребёнок: stdin = файл, stdout = труба
1660 close(3); close(5); close(4)
1660 execve("./child", ["child"], ...) = 0
1660 read(0, "1 2 3\n10 20 30 40\n-5 5\n42\n", 4096) = 26
1660 write(1, "6\n", 2)                = 2
1659 read(4, "6\n", 256)               = 2
1659 write(1, "6\n", 2)                = 2
...
1660 read(0, "", 4096)                 = 0   конец файла
1660 +++ exited with 0 +++
1659 read(4, "", 256)                  = 0   конец трубы
1659 close(4)                          = 0
1659 wait4(1660, [{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 1660
```

Трасса снята на x86-64, поэтому видно `dup2`; на aarch64 (Docker на Mac) вместо него будет `dup3`. `fork` виден как `clone`, `pipe` как `pipe2`, `open` как `openat`, `waitpid` как `wait4`. Вызовы `write` ребёнка и `read` родителя чередуются: процессы работают одновременно.

### 6.4. Память и дескрипторы ([docs/valgrind.log](docs/valgrind.log))

`echo tests/data/edge.txt | valgrind --leak-check=full --trace-children=yes --track-fds=yes ./parent`, у обоих процессов:

```
FILE DESCRIPTORS: 3 open (3 std) at exit.
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

После завершения `ps -eo pid,ppid,stat,comm | grep -E "parent|child"` ничего не показывает: зомби нет.

## 7. Выводы

1. Написаны две программы, `parent` и `child`, которые обмениваются данными через неименованный канал. stdin ребёнка подменён файлом, stdout каналом.
2. Перенаправление делается через `dup2` между `fork` и `exec`. Таблица дескрипторов переживает `exec`, поэтому `child` работает с обычными 0 и 1.
3. Лишние концы трубы нужно закрывать в обоих процессах. Если родитель не закроет `pipe1[1]`, `read` никогда не вернёт 0 и родитель зависнет; это проверено.
4. Каждый системный вызов проверяется, ошибка печатается через `perror`. Обработаны мусор в строках, выход за `int`, отсутствующий файл и обрыв трубы (`SIGPIPE` игнорируется, `write` даёт `EPIPE`).
5. Родитель всегда ждёт ребёнка через `waitpid`, valgrind не нашёл утечек памяти и незакрытых дескрипторов.
