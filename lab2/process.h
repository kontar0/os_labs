#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

/*
 * Создаёт ребёнка и запускает в нём программу path под именем name.
 * У ребёнка stdin подменяется на in_fd, stdout на out_fd,
 * после чего все номерки из fds_to_close закрываются (нужные уже лежат в 0 и 1).
 * Возвращает PID ребёнка или -1, если fork не удался.
 */
pid_t start_child(const char *path, const char *name, int in_fd, int out_fd,
                  const int *fds_to_close, int count);

/* Ждёт завершения ребёнка. Возвращает 0, если он вышел с кодом 0, иначе 1. */
int wait_child(pid_t pid);

#endif
