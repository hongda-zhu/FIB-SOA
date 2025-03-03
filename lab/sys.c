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

int sys_write(int fd, char *buffer, int size)
{
    // Check if file descriptor is valid (only 1 for stdout is supported)
    if (fd != 1) return -EBADF;
    
    // Check buffer pointer
    if (buffer == NULL) return -EFAULT;
    
    // Check size parameter
    if (size < 0) return -EINVAL;
    
    // Check if buffer is in user memory space
    if (!access_ok(VERIFY_READ, buffer, size)) return -EFAULT;
    
    // Local buffer to copy user data
    char local_buffer[256];
    int bytes_left = size;
    int bytes_written = 0;
    
    while (bytes_left > 0) {
        // Copy data in chunks to avoid large stack allocations
        int chunk = (bytes_left > 256) ? 256 : bytes_left;
        
        // Copy from user space to kernel space
        if (copy_from_user(buffer + bytes_written, local_buffer, chunk) < 0)
            return -EFAULT;
        
        // Write to console (device-dependent part)
        bytes_written += sys_write_console(local_buffer, chunk);
        bytes_left -= chunk;
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

