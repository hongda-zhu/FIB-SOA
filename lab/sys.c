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

int PID_cnt = 2;

int check_fd(int fd, int operation)
{
    if (fd != 1) return -EBADF;  
    if (operation != ESCRIPTURA) return -EACCES; 
    return 0;
}

int sys_ni_syscall()
{
	printk("no hauria d'apareixer aquest missatge xd");
	return -ENOSYS;
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork() {
	return 0;
}

extern struct list_head freequeue;
extern struct list_head readyqueue;
extern long* get_ebp();
int sys_fork()
{
	printk("ola");
  struct list_head * child_list_head;
  page_table_entry * childPT;
  page_table_entry * parentPT;
  struct task_struct * child_task;
  union task_union * child_union;
  int childFrame, pag;
  
  // creates the child process
  child_list_head = list_first( &freequeue );
  if (child_list_head == 0) return -1;
  
  list_del( child_list_head ); 
  
  child_task = list_head_to_task_struct(child_list_head);
  child_union = (union task_union *) child_task;
  
  copy_data(current(), child_union, KERNEL_STACK_SIZE);
  
  set_user_pages(child_task);
  
  childPT = get_PT(child_task);
  parentPT = get_PT(current());
  
  //check if all frames were given
  for (pag=0;pag<NUM_PAG_CODE;pag++){
	if (childPT[PAG_LOG_INIT_CODE+pag].bits.pbase_addr == -1) {
	  free_user_pages(child_task);
	  list_add( child_list_head, &freequeue );
	  return -1;
	}
  }
  for (pag=0;pag<NUM_PAG_DATA;pag++){ 
    if (childPT[PAG_LOG_INIT_CODE+pag].bits.pbase_addr == -1) {
	  free_user_pages(child_task);
	  list_add( child_list_head, &freequeue );
	  return -1;
	}
  }
  
  allocate_DIR(child_task);
  
  int frame = alloc_frame();
  if (frame == -1) {
	  free_user_pages(child_task);
	  list_add( child_list_head, &freequeue );
	  return -1;
  }

  /* CODE */
  for (pag=0;pag<NUM_PAG_CODE;pag++){
  	childPT[PAG_LOG_INIT_CODE+pag].bits.pbase_addr = parentPT[PAG_LOG_INIT_CODE+pag].bits.pbase_addr;
  }
  
  /* DATA */ 
  for (pag=0;pag<NUM_PAG_DATA;pag++){ //0x0102
	childFrame = get_frame(childPT, PAG_LOG_INIT_DATA+pag); //fisica que correspon a l'adreça logica DEL CHILD 0x0102
	set_ss_pag(parentPT, 0x0F, childFrame); //assignem l'adreca logica 0x0F del PARENT a la fisica que hem trobat
	copy_data(PAG_LOG_INIT_DATA+pag, 0x0F, PAGE_SIZE); //copiem logica a logica, que correspon a copiar logica del parent a mateixa logica del child
  }
  del_ss_pag(parentPT, 0x0F);
  
  //flush TLB
  set_cr3(get_DIR(current()));
  
  child_task->PID = PID_cnt++;
  
  child_task->k_esp = get_ebp();
  child_task->k_esp--;
  
  *(child_task->k_esp + (child_task - current())) = 0;
  *(child_task->k_esp + (child_task - current()) + 1) = ret_from_fork; //potser falta multiplicar per 4
  
  child_task->current_state = ST_READY;
  list_add_tail(&(child_task->list), &readyqueue);
    
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
