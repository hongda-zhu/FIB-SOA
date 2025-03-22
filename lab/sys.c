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

int sys_getpid()
{
	return current()->PID;
}

int sys_fork()
{
  // creates the child process
  struct list_head * child_list_head = list_first( &freequeue );
  if (child_list_head == 0) return -1;
  
  list_del( e ); 
  
  struct task_struct * child_task = list_head_to_task_struct(child_list_head);
  union task_union * child_union = (union task_union *) child_task;
  
  copy_data(current(), child_union, KERNEL_STACK_SIZE);
  
  set_user_pages(child_task);
  allocate_DIR(child_task);
  
  int frame = alloc_frame();
  if (frame == -1) {
	  free_user_pages(child_task);
	  list_add( child_list_head, &freequeue );
	  return -1;
  }
  
  page_table_entry * childPT = get_PT(child_task);
  page_table_entry * parentPT = get_PT(current());
  
  /*
   * mirar be mm.c (funcions de alloc_frame, fisica a logica i viceversa, etc)
   * a mm_address.h hi ha com esta distribuida la taula de pagines (per saber quines estan buides per fer servir per copiar de parent a child)
   * 
   * 
   * */

  /* CODE */
  for (pag=0;pag<NUM_PAG_CODE;pag++){
	childPT[PAG_LOG_INIT_CODE+pag].entry = 0;
  	childPT[PAG_LOG_INIT_CODE+pag].bits.pbase_addr = parentPT[PAG_LOG_INIT_CODE+pag].bits.pbase_addr;
  	childPT[PAG_LOG_INIT_CODE+pag].bits.user = 1;
  	childPT[PAG_LOG_INIT_CODE+pag].bits.present = 1;
  }
  
  /* DATA */ 
  for (pag=0;pag<NUM_PAG_DATA;pag++){
  	childPT[PAG_LOG_INIT_DATA+pag].entry = 0;
  	childPT[PAG_LOG_INIT_DATA+pag].bits.pbase_addr = new_ph_pag;
  	childPT[PAG_LOG_INIT_DATA+pag].bits.user = 1;
  	childPT[PAG_LOG_INIT_DATA+pag].bits.rw = 1;
  	childPT[PAG_LOG_INIT_DATA+pag].bits.present = 1;
  }
  
  
  //...
  child_task->PID = 0;
  return child_task->PID;
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
