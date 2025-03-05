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
#define BUFFER_SIZE 256

char localbuffer[BUFFER_SIZE];

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
    int bytes_left;
    int bytes_written = 0;
    int ret;

    if ((ret = check_fd(fd, ESCRIPTURA)))
        return ret;
    
    if (buffer == NULL)
        return -EFAULT;
    
    if (size < 0)
        return -EINVAL;
    
    if (!access_ok(VERIFY_READ, buffer, size))
        return -EFAULT;
    
    bytes_left = size;
    
    if (bytes_left <= BUFFER_SIZE - 1) {
        if (copy_from_user(buffer, localbuffer, bytes_left) != 0)
            return -EFAULT;
        
        localbuffer[bytes_left] = '\n';
        
        ret = sys_write_console(localbuffer, bytes_left + 1);
        if (ret < 0)
            return ret;
        
        return (ret > 0) ? size : ret - 1;
    }
    
    while (bytes_left > 0) {
        int chunk_size = (bytes_left > BUFFER_SIZE - 1) ? BUFFER_SIZE - 1 : bytes_left;
        int is_last_chunk = (bytes_left <= BUFFER_SIZE - 1);
        
        if (copy_from_user(buffer + bytes_written, localbuffer, chunk_size) != 0)
            return -EFAULT;
        
        if (is_last_chunk) {
            localbuffer[chunk_size] = '\n';
            chunk_size++;
        }
        
        ret = sys_write_console(localbuffer, chunk_size);
        if (ret < 0)
            return ret;
        
        int actual_written = is_last_chunk ? ret - 1 : ret;
        bytes_left -= actual_written;
        bytes_written += actual_written;
        
        if (actual_written < chunk_size - (is_last_chunk ? 1 : 0))
            break;
    }
    
    return bytes_written;
}

extern int zeos_ticks;

int sys_gettime()
{
    return zeos_ticks;
}

void sys_exit()
{  
}
