/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>


#define CLONE_PROCESS 0
#define CLONE_THREAD 1

extern int errno;

int write(int fd, char *buffer, int size);

void itoa(int a, char *b);

int strlen(char *a);

void perror();

int getpid();

int fork();  // Mantener para compatibilidad

int clone(int what, void *(*func)(void*), void *param, int stack_size);  // Syscall base

int threadCreate(void(*function)(void* arg), void* parameter);  // Wrapper para threads

void threadExit();  // Para finalizar threads

void* wrapper_func(void * (*func) (void *param), void * param);  // Función auxilia

void exit();

int yield();

int get_stats(int pid, struct stats *st);

int get_keyboard_state(char* keyboard);

int pause(int milliseconds);

void* StartScreen(void);
#endif  /* __LIBC_H__ */
