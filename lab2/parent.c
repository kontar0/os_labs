/*
 * Родительская программа (группа 5, вариант 19).
 * Первые две строки stdin: имена файлов для child1 и child2.
 * Открывает оба файла на запись, создаёт pipe1 и pipe2, запускает двух детей
 * (child1: stdin = pipe1, stdout = файл 1; child2: stdin = pipe2, stdout = файл 2).
 * Дальше читает строки и с вероятностью 80% отправляет строку в pipe1, иначе в pipe2.
 */
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "process.h"

/* Путь относительный: parent нужно запускать из каталога lab2, где лежит child. */
#define CHILD_PATH "./child"

static void close_or_report(int fd)
{
    if (close(fd) == -1)
        perror("parent: close");
}

/* Читает имя файла из stdin. 0 — прочитано, 1 — ошибка (сообщение уже напечатано). */
static int read_filename(char *buffer, int size, const char *for_whom)
{
    if (fgets(buffer, size, stdin) == NULL) {
        fprintf(stderr, "parent: не введено имя файла для %s\n", for_whom);
        return 1;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
    if (buffer[0] == '\0') {
        fprintf(stderr, "parent: пустое имя файла для %s\n", for_whom);
        return 1;
    }
    return 0;
}

int main(void)
{
    char filename1[4096];
    char filename2[4096];
    int pipe1[2];
    int pipe2[2];
    int exit_code = 0;

    /* Если ребёнок умрёт, write в его трубу вернёт ошибку EPIPE, а не убьёт родителя. */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("parent: signal");
        return 1;
    }

    fprintf(stderr, "[parent PID=%d] запущен, жду имена файлов для child1 и child2\n", getpid());
    if (read_filename(filename1, sizeof(filename1), "child1") != 0)
        return 1;
    if (read_filename(filename2, sizeof(filename2), "child2") != 0)
        return 1;

    /* Файлы открываем на запись: создать, если нет, и очистить, если есть. */
    int file1 = open(filename1, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file1 == -1) {
        perror("parent: open файла для child1");
        return 1;
    }
    int file2 = open(filename2, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file2 == -1) {
        perror("parent: open файла для child2");
        close_or_report(file1);
        return 1;
    }

    /* Трубы создаём до fork: дети получат копии их концов. */
    if (pipe(pipe1) == -1) {
        perror("parent: pipe1");
        close_or_report(file1);
        close_or_report(file2);
        return 1;
    }
    if (pipe(pipe2) == -1) {
        perror("parent: pipe2");
        close_or_report(file1);
        close_or_report(file2);
        close_or_report(pipe1[0]);
        close_or_report(pipe1[1]);
        return 1;
    }

    /* Все номерки, которые сейчас открыты у родителя. Каждый ребёнок после dup2 закроет их все. */
    int all_fds[6] = {file1, file2, pipe1[0], pipe1[1], pipe2[0], pipe2[1]};

    pid_t child1 = start_child(CHILD_PATH, "child1", pipe1[0], file1, all_fds, 6);
    if (child1 == -1) {
        for (int i = 0; i < 6; i++)
            close_or_report(all_fds[i]);
        return 1;
    }
    pid_t child2 = start_child(CHILD_PATH, "child2", pipe2[0], file2, all_fds, 6);
    if (child2 == -1) {
        /* child1 уже работает: закрываем всё, он получит EOF и завершится. */
        for (int i = 0; i < 6; i++)
            close_or_report(all_fds[i]);
        wait_child(child1);
        return 1;
    }
    fprintf(stderr, "[parent PID=%d] созданы child1 PID=%d и child2 PID=%d\n",
            getpid(), child1, child2);

    /* Родителю нужны только концы записи. Файлы и концы чтения нужны детям. */
    close_or_report(file1);
    close_or_report(file2);
    close_or_report(pipe1[0]);
    close_or_report(pipe2[0]);

    srand(time(NULL) ^ getpid());

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    int line_number = 0;

    while ((length = getline(&line, &capacity, stdin)) != -1) {
        line_number++;

        /* Правило фильтрации: с вероятностью 80% в pipe1, иначе в pipe2. */
        int to_pipe1 = rand() % 100 < 80;
        int fd = to_pipe1 ? pipe1[1] : pipe2[1];

        if (write(fd, line, length) != length) {
            perror("parent: write в трубу");
            exit_code = 1;
            break;
        }
        fprintf(stderr, "[parent PID=%d] строка %d -> %s\n",
                getpid(), line_number, to_pipe1 ? "pipe1" : "pipe2");
    }
    if (ferror(stdin)) {
        perror("parent: getline");
        exit_code = 1;
    }
    free(line);

    /*
     * Закрываем концы записи. read у детей вернёт 0 (EOF) только когда закрыты
     * ВСЕ концы записи их трубы. Без этих close дети ждали бы вечно, а родитель
     * вечно ждал бы их в waitpid.
     */
    close_or_report(pipe1[1]);
    close_or_report(pipe2[1]);

    /* Без waitpid дети остались бы зомби до конца работы родителя. */
    if (wait_child(child1) != 0)
        exit_code = 1;
    if (wait_child(child2) != 0)
        exit_code = 1;
    return exit_code;
}
