/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>
#include <errno.h>
#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <interrupt.h>

#define LECTURA 0
#define ESCRIPTURA 1

int check_fd(int fd, int operation)
{
    if (fd != 1) return -EBADF;  
    if (operation != ESCRIPTURA) return -EACCES; 
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

int sys_write(int fd, char *buffer, int size) {

    int fd_error = check_fd(fd, ESCRIPTURA);
    if (fd_error) return fd_error;
    
    if (buffer == NULL) return -EFAULT;
    
    if (size < 0) return -EINVAL;

    if (!access_ok(VERIFY_READ, buffer, size)) return -EFAULT;
    
    char local_buffer[256];
    int bytes_written = 0;
    
    while (bytes_written < size) {
        int chunk = ((size - bytes_written) > 256) ? 256 : (size - bytes_written);
        
        if (copy_from_user(buffer + bytes_written, local_buffer, chunk) < 0)
            return -EFAULT;
        
        int ret = sys_write_console(local_buffer, chunk);
        if (ret < 0) return ret;
        
        bytes_written += ret;
        
        if (ret < chunk) break;
    }
    
    return bytes_written;
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

