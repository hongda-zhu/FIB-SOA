/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <p_stats.h>

#include <errno.h>

#define LECTURA 0
#define ESCRIPTURA 1

void * get_ebp();

int check_fd(int fd, int permissions)
{
  if (fd!=1) return -EBADF; 
  if (permissions!=ESCRIPTURA) return -EACCES; 
  return 0;
}

void user_to_system(void)
{
  update_stats(&(current()->p_stats.user_ticks), &(current()->p_stats.elapsed_total_ticks));
}

void system_to_user(void)
{
  update_stats(&(current()->p_stats.system_ticks), &(current()->p_stats.elapsed_total_ticks));
}

int sys_ni_syscall()
{
	return -ENOSYS; 
}

int sys_getpid()
{
	return current()->PID;
}

int global_PID=1000;
int global_TID=1000;

int ret_from_fork()
{
  return 0;
}

//what: 0 is CLONE_PROCESS, 1 is CLONE_THREAD
int sys_clone(int what, void *(*func)(void*), void *param, int stack_size) 
{	
  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  int data_pages_copy[NUM_PAG_DATA];
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  lhcurrent=list_first(&freequeue);
  
  list_del(lhcurrent);
  
  uchild=(union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  page_table_entry *parent_PT = get_PT(current());
  int new_ph_pag, pag, i, j;
  
  for (i=0; i<NUM_PAG_DATA; i++) {
	/* Map one child page to parent's address space. */
	pag=NUM_PAG_KERNEL+NUM_PAG_CODE+i;
	
	if (has_frame(parent_PT, pag+NUM_PAG_DATA))
		data_pages_copy[i] = get_frame(parent_PT, pag+NUM_PAG_DATA);
	else
		data_pages_copy[i] = -1;
  }
  
  if (what == 0) {
	  /* new pages dir */
	  allocate_DIR((struct task_struct*)uchild);
	  
	  page_table_entry *process_PT = get_PT(&uchild->task);
	  
	  /* Allocate pages for DATA+STACK */
	  for (pag=0; pag<NUM_PAG_DATA; pag++)
	  {
		new_ph_pag=alloc_frame();
		if (new_ph_pag!=-1) /* One page allocated */
		{
		  set_ss_pag(process_PT, PAG_LOG_INIT_DATA+pag, new_ph_pag);
		}
		else /* No more free pages left. Deallocate everything */
		{
		  /* Deallocate allocated pages. Up to pag. */
		  for (i=0; i<pag; i++)
		  {
			free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
			del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
		  }
		  /* Deallocate task_struct */
		  list_add_tail(lhcurrent, &freequeue);
		  
		  /* Return error */
		  return -EAGAIN; 
		}
	  }

	  /* Copy parent's SYSTEM and CODE to child. */
	  for (pag=0; pag<NUM_PAG_KERNEL; pag++)
	  {
		set_ss_pag(process_PT, pag, get_frame(parent_PT, pag));
	  }
	  for (pag=0; pag<NUM_PAG_CODE; pag++)
	  {
		set_ss_pag(process_PT, PAG_LOG_INIT_CODE+pag, get_frame(parent_PT, PAG_LOG_INIT_CODE+pag));
	  }
	  /* Copy parent's DATA to child. We will use TOTAL_PAGES-1 as a temp logical page to map to */
	  set_cr3(get_DIR(current()));
	  
	  for (i=0; i<NUM_PAG_DATA; i++)
	  {	
		pag=NUM_PAG_KERNEL+NUM_PAG_CODE+i;
		set_ss_pag(parent_PT, pag+NUM_PAG_DATA, get_frame(process_PT, pag));
		copy_data((void*)(pag<<12), (void*)((pag+NUM_PAG_DATA)<<12), PAGE_SIZE);
		del_ss_pag(parent_PT, pag+NUM_PAG_DATA);
	  }
	  
	  if (current()->screen_buffer != NULL) {
		new_ph_pag=alloc_frame();
		int alfredo = get_free_logical_pages(parent_PT, PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA, 1);
		if (new_ph_pag != -1 && alfredo != -1) /* One page allocated */
		{
		  set_ss_pag(process_PT, ((int)(current()->screen_buffer))>>12, new_ph_pag);
		  set_ss_pag(parent_PT, alfredo, new_ph_pag);
		  copy_data(current()->screen_buffer, (void*)((alfredo)<<12), 80*25*2);
		  del_ss_pag(parent_PT, alfredo);
		}
		else {
		  /* Deallocate allocated pages. Up to pag. */
		  for (i=0; i<pag; i++)
		  {
			free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
			del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
			if (data_pages_copy[i] != -1)
				set_ss_pag(parent_PT, NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA+i, data_pages_copy[i]);
		  }

		  /* Deallocate task_struct */
		  list_add_tail(lhcurrent, &freequeue);
		  
		  free_frame(new_ph_pag);
		  
		  /* Return error */
		  return -EAGAIN; 
		}
	  }

  }
  else {
	  /* copy pages dir */
	  uchild->task.dir_pages_baseAddr = current()->dir_pages_baseAddr;
	  
	  /* prepare child user stack with function context */
	  int required_pags = (stack_size - 1) / 4096 + 1;
	  
	  int first_usr_stack_page = get_free_logical_pages(parent_PT, NUM_PAG_KERNEL+NUM_PAG_CODE+2*NUM_PAG_DATA, required_pags);
	  if (first_usr_stack_page == -1) {
		  return -ENOMEM;
	  }
	  
	  for (i = 0; i < required_pags; i++) {
		  pag = first_usr_stack_page + i;
		  new_ph_pag = alloc_frame();
		  if (new_ph_pag == -1) {
			  for (j = 0; j < i; j++) {
				  free_frame(get_frame(parent_PT, pag));
				  del_ss_pag(parent_PT, pag);
			  }
			  /* Deallocate task_struct */
			  list_add_tail(lhcurrent, &freequeue);
			  return -EAGAIN;
		  }
		  set_ss_pag(parent_PT, pag, new_ph_pag);
	  }
	  
	  unsigned long* child_esp = (first_usr_stack_page + required_pags - 1)<<12 | 0xFF8;
	  
	  uchild->task.user_stack_page_start = first_usr_stack_page;
	  uchild->task.user_stack_page_end = first_usr_stack_page + required_pags - 1;
	  
	  *child_esp = 0; //@ret. podem canviar a apuntar a exit()?
	  ++child_esp;
	  *child_esp = param;
	  --child_esp;
	  
	  uchild->stack[KERNEL_STACK_SIZE-2] = child_esp; //esp
	  uchild->stack[KERNEL_STACK_SIZE-5] = func; //eip
  }
	
  /* Restore old logical pages after data section*/
  for (i=0; i<NUM_PAG_DATA; i++) {
	  if (data_pages_copy[i] != -1)
		set_ss_pag(parent_PT, NUM_PAG_KERNEL+NUM_PAG_CODE+NUM_PAG_DATA+i, data_pages_copy[i]);
  }
  
  /* Deny access to the child's memory space */
  set_cr3(get_DIR(current()));
  
  uchild->task.TID=++global_TID;
  
  int register_ebp;		/* frame pointer */
  /* Map Parent's ebp to child's stack */
  register_ebp = (int) get_ebp();
  register_ebp=(register_ebp - (int)current()) + (int)(uchild);

  uchild->task.register_esp=register_ebp + sizeof(DWord);

  DWord temp_ebp=*(DWord*)register_ebp;
  
  	  /* Prepare child stack for context switch */
	  uchild->task.register_esp-=sizeof(DWord);
	  *(DWord*)(uchild->task.register_esp)=(DWord)&ret_from_fork;
	  uchild->task.register_esp-=sizeof(DWord);
	  *(DWord*)(uchild->task.register_esp)=temp_ebp;

  if (what == 0) {
	  uchild->task.PID = ++global_PID;
	  uchild->task.master = &uchild->task;
	  uchild->task.num_threads = 1;
	  INIT_LIST_HEAD(&uchild->task.threads);
	  //list_add_tail(&uchild->task.threads_list, &uchild->threads); //?
  }
  else {
	  uchild->task.master = current();
	  current()->num_threads++;
	  list_add_tail(&uchild->task.threads_list, &current()->threads);
  }
  uchild->task.state=ST_READY;
  
  /* Set stats to 0 */
  init_stats(&(uchild->task.p_stats));

  /* Queue child process into readyqueue */
  uchild->task.state=ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);
  
  if (what == 0)
	return uchild->task.PID;
  else
	return uchild->task.TID;
}

int sys_SetPriority(int priority) {
	if (priority < 0) return -EINVAL;
	
	int boomer_prio = current()->priority;
	
	current()->priority = priority;
	
	if (!list_empty(&readyqueue)) {
		struct list_head *l;
		struct task_struct *t;
		int highest_prio = -1;
		
		list_for_each(l, &readyqueue) {
			t = list_head_to_task_struct(l);
			if (t->priority > highest_prio) {
				highest_prio = t->priority;
			}
		}
		
		if (highest_prio > current()->priority) {
			update_process_state_rr(current(), &readyqueue);
			sched_next_rr();
		}
	}
	
	return boomer_prio;
}

#define TAM_BUFFER 512

int sys_write(int fd, char *buffer, int nbytes) {
char localbuffer [TAM_BUFFER];
int bytes_left;
int ret;

	if ((ret = check_fd(fd, ESCRIPTURA)))
		return ret;
	if (nbytes < 0)
		return -EINVAL;
	if (!access_ok(VERIFY_READ, buffer, nbytes))
		return -EFAULT;
	
	bytes_left = nbytes;
	while (bytes_left > TAM_BUFFER) {
		copy_from_user(buffer, localbuffer, TAM_BUFFER);
		ret = sys_write_console(localbuffer, TAM_BUFFER);
		bytes_left-=ret;
		buffer+=ret;
	}
	if (bytes_left > 0) {
		copy_from_user(buffer, localbuffer,bytes_left);
		ret = sys_write_console(localbuffer, bytes_left);
		bytes_left-=ret;
	}
	return (nbytes-bytes_left);
}


extern int zeos_ticks;

int sys_gettime()
{
  return zeos_ticks;
}

void sys_exit()
{
  int i;
  struct task_struct* master = current()->master;
  page_table_entry *process_PT = get_PT(current());
  
  if (current() == master && master->num_threads > 1) {
	  struct list_head *l, *l_next;
	  list_for_each_safe(l, l_next, &master->threads) {
		  struct task_struct *t = list_head_to_task_struct(l);
		  for (i = t->user_stack_page_start; i <= t->user_stack_page_end; i++) {
			  int frame = get_frame(process_PT, i);
			  del_ss_pag(process_PT, i);
			  free_frame(frame);
		  }
		  
		  list_del(&t->threads_list);
		  list_del(&t->list);
		  t->TID = -1;
		  t->PID = -1;
		  list_add_tail(&t->list, &freequeue);
	  }
  }
	  
  // Deallocate all the propietary physical pages
  for (i=0; i<NUM_PAG_DATA; i++)
  {
	free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
	del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  
  if (current()->screen_buffer != NULL) {
	  page_table_entry* pt = get_PT(current());
	  int frame = get_frame(pt, (unsigned int)current()->screen_buffer >> 12);
	  del_ss_pag(process_PT, ((int)current()->screen_buffer)>>12);
	  current()->screen_buffer = NULL;
	  free_frame(frame);
  }
	  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID = -1;
  current()->TID = -1,
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

void sys_pthread_exit()
{
  int i;
  struct task_struct* master = current()->master;
  page_table_entry *process_PT = get_PT(current());
  
  if (current() == master) {
	  sys_exit();
	  return;
  }
  
  for (i = current()->user_stack_page_start; i <= current()->user_stack_page_end; i++) {
	  int frame = get_frame(process_PT, i);
	  del_ss_pag(process_PT, i);
	  free_frame(frame);
  }
  
  master->num_threads--;
  list_del(&current()->threads_list);
  
  /* Free task_struct */
  list_add_tail(&(current()->list), &freequeue);
  
  current()->PID = -1;
  current()->TID = -1,
  
  /* Restarts execution of the next process */
  sched_next_rr();
}

/* System call to force a task switch */
int sys_yield()
{
  force_task_switch();
  return 0;
}

extern int remaining_quantum;

int sys_get_stats(int pid, struct stats *st)
{
  int i;
  
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -EFAULT; 
  
  if (pid<0) return -EINVAL;
  for (i=0; i<NR_TASKS; i++)
  {
    if (task[i].task.PID==pid)
    {
      task[i].task.p_stats.remaining_ticks=remaining_quantum;
      copy_to_user(&(task[i].task.p_stats), st, sizeof(struct stats));
      return 0;
    }
  }
  return -ESRCH; /*ESRCH */
}

extern char* keyboard_state;
int sys_GetKeyboardState(char* keyboard)
{
	int res;
	if (!access_ok(VERIFY_WRITE, keyboard, 256)) return -EFAULT;
	res = copy_to_user(&keyboard_state, keyboard, 256);
	if (res < 0) return -EFAULT;
	return 0;
}

unsigned long get_ticks(void);
extern struct list_head blocked;
int sys_pause(int milliseconds)
{
	if (milliseconds < 0) return -EINVAL;
	
	int ticks = milliseconds*18/1000;
	if (ticks < 1) ticks = 1;
	
	current()->unpause_tick = get_ticks() + ticks;
	update_process_state_rr(current(), &blocked);
	sched_next_rr();
	return 0;
}

int alloc_frame();
void* sys_StartScreen(void)
{
	if (current()->screen_buffer != NULL) {
		return current()->screen_buffer;
	}
	
	int frame = alloc_frame();
	if (frame == -1) {
		return (void *)-1;
	}
	
	page_table_entry * pt = get_PT(current());
	
	int logic_page = get_free_logical_pages(pt, PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA, 1);
	if (logic_page == -1) {
		//couldn't find free logical page (should never happen)
		free_frame(frame);
		return (void *)-1;
	}
	set_ss_pag(pt, logic_page, frame);
	current()->screen_buffer = (void *) (logic_page << 12);
	return current()->screen_buffer;
}
