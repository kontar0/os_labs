#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

/*
 * Создаёт дочерний процесс, исполняющий программу path, у которого
 * stdin перенаправлен на in_fd, а stdout — на out_fd.
 * close_fd — дескриптор, который нужно закрыть в ребёнке (второй конец канала), или -1.
 * Возвращает PID ребёнка или -1 при ошибке fork (errno сохранён).
 */
pid_t spawn_redirected(const char *path, int in_fd, int out_fd, int close_fd);

/*
 * Дожидается завершения процесса pid (waitpid с повтором при EINTR),
 * печатает, как он завершился. Возвращает код выхода ребёнка,
 * 128+сигнал при завершении сигналом, -1 при ошибке waitpid.
 */
int wait_child(pid_t pid);

/* Путь к программе child, лежащей в том же каталоге, что и текущий исполняемый файл. */
int resolve_child_path(char *buf, size_t size);

#endif
