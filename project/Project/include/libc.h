/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

extern int errno;

int write(int fd, char *buffer, int size);

void itoa(int a, char *b);

int strlen(char *a);

void perror();

int getpid();

int fork();

int pthread_create(void *(*func)(void*), void *param, int stack_size);

int SetPriority(int priority);

void exit();

void pthread_exit();

int yield();

int get_stats(int pid, struct stats *st);

int get_keyboard_state(char* keyboard);

int pause(int milliseconds);

int semCreate(int value);

int semWait(int semid);

int semPost(int semid);

int semDestroy(int semid);

void* StartScreen(void);
#endif  /* __LIBC_H__ */

