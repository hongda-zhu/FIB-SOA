/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <interrupt.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  
  return PID;
}

int sys_write()
{
	return 0;
}

int sys_gettime()
{
	int p = gettime_routine();
	char* s = "0x00000";
	int desp = 0;
	while (p > 0) {
		s[6-desp] = '0' + p%16;
		p /= 16;
		++desp;
	}
	printk(s);
	return p;
}

void sys_exit()
{  
}

