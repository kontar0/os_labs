#ifndef PROCESS_H
#define PROCESS_H

#include <sys/types.h>

/*
 * Создаёт ребёнка и запускает в нём программу path.
 * У ребёнка stdin подменяется на in_fd, stdout на out_fd,
 * а unused_fd (ненужный ребёнку конец трубы) закрывается.
 * Возвращает PID ребёнка или -1, если fork не удался.
 */
pid_t start_child(const char *path, int in_fd, int out_fd, int unused_fd);

/* Ждёт завершения ребёнка. Возвращает 0, если он вышел с кодом 0, иначе 1. */
int wait_child(pid_t pid);

#endif
