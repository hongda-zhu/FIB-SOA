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

extern void wrapper_func(void); // Declaration for assembly function

int sys_clone(int what, void *(*func)(void*), void *param, int stack_size); // Forward declaration

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

int sys_fork()
{
  return sys_clone(CLONE_PROCESS, NULL, NULL, 0);
}

int sys_clone(int what, void *(*func)(void*), void *param, int stack_size) 
{   
  // Validación de parámetros
  if (what != CLONE_PROCESS && what != CLONE_THREAD) return -EINVAL;
  if (what == CLONE_THREAD) {
    if (func == NULL) return -EINVAL;
    if (stack_size <= 0) return -EINVAL;
  }

  print_number(what);
  print_number(func);
  print_number(param);
  print_number(stack_size);

  struct list_head *lhcurrent = NULL;
  union task_union *uchild;
  
  /* Any free task_struct? */
  if (list_empty(&freequeue)) return -ENOMEM;

  // Obtenemos un task_struct libre
  lhcurrent = list_first(&freequeue);
  list_del(lhcurrent);
  uchild = (union task_union*)list_head_to_task_struct(lhcurrent);
  
  /* Copy the parent's task struct to child's */
  copy_data(current(), uchild, sizeof(union task_union));
  
  page_table_entry *process_PT = get_PT(&uchild->task);
  page_table_entry *parent_PT = get_PT(current());
  int new_ph_pag, pag, i;
  
  if (what == CLONE_PROCESS) { // CLONE_PROCESS (similar a fork)
    /* new pages dir */
    allocate_DIR((struct task_struct*)uchild);
    
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
    for (pag=0; pag<NUM_PAG_DATA; pag++)
    {
      /* Map one child page to parent's address space. */
      set_ss_pag(parent_PT, TOTAL_PAGES-1, get_frame(process_PT, PAG_LOG_INIT_DATA+pag));
      copy_data((void*)((PAG_LOG_INIT_DATA+pag)<<12), (void*)((TOTAL_PAGES-1)<<12), PAGE_SIZE);
      del_ss_pag(parent_PT, TOTAL_PAGES-1);
    }
    
    // Copiar pantalla compartida si existe
    if (current()->screen_buffer != NULL) {
      new_ph_pag = alloc_frame();
      if (new_ph_pag != -1) {
        int log_page = ((int)current()->screen_buffer) >> 12;
        set_ss_pag(process_PT, log_page, new_ph_pag);
        
        // Usar una página temporal para copiar
        int temp_page = TOTAL_PAGES-1;
        set_ss_pag(parent_PT, temp_page, new_ph_pag);
        copy_data(current()->screen_buffer, (void*)(temp_page<<12), 80*25*2);
        del_ss_pag(parent_PT, temp_page);
        
        uchild->task.screen_buffer = (void*)(log_page<<12);
      }
    }
    
    /* Deny access to the child's memory space */
    set_cr3(get_DIR(current()));
    
    // Asignar PID
    uchild->task.PID = ++global_PID;
    uchild->task.TID = ++global_TID;
    
    // Configurar como proceso independiente
    uchild->task.master = &uchild->task;
    uchild->task.num_threads = 1;
    INIT_LIST_HEAD(&uchild->task.threads);
    
    // Este es un proceso, por lo que preparamos el contexto como en fork
    int register_ebp = (int)get_ebp();
    register_ebp = (register_ebp - (int)current()) + (int)(uchild);
    
    uchild->task.register_esp = register_ebp + sizeof(DWord);
    
    DWord temp_ebp = *(DWord*)register_ebp;
    uchild->task.register_esp -= sizeof(DWord);
    *(DWord*)(uchild->task.register_esp) = (DWord)&ret_from_fork;
    uchild->task.register_esp -= sizeof(DWord);
    *(DWord*)(uchild->task.register_esp) = temp_ebp;
  }
  else { // CLONE_THREAD
    printk("NO PUEDE ENTRAR AQUI\n");
    // En caso de thread, compartimos el espacio de memoria
    uchild->task.dir_pages_baseAddr = current()->dir_pages_baseAddr;
    
    // Asignar un identificador único al thread
    uchild->task.TID = ++global_TID;
    uchild->task.PID = current()->PID; // El PID es el mismo que el del proceso padre
    
    // Vincular el thread al proceso padre
    uchild->task.master = current()->master;
    current()->master->num_threads++;
    list_add_tail(&uchild->task.threads_list, &current()->master->threads);
    
    // Calcular número de páginas necesarias (redondeando hacia arriba)
    int required_pages = (stack_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // Buscar un bloque de páginas lógicas contiguas
    int first_page = find_free_contiguous_pages(parent_PT, required_pages);
    
    if (first_page == -1) {
        // No hay suficientes páginas contiguas
        current()->master->num_threads--; // Revertir incremento
        list_del(&uchild->task.threads_list); // Quitar de la lista de threads
        list_add_tail(lhcurrent, &freequeue);
        return -ENOMEM;
    }
    
    // Alocar frames físicos y mapearlos a las páginas lógicas contiguas
    for (i = 0; i < required_pages; i++) {
        new_ph_pag = alloc_frame();
        if (new_ph_pag == -1) {
            // Error: no hay más frames libres
            // Liberar los frames ya asignados
            for (int j = 0; j < i; j++) {
                free_frame(get_frame(parent_PT, first_page + j));
                del_ss_pag(parent_PT, first_page + j);
            }
            
            current()->master->num_threads--; // Revertir incremento
            list_del(&uchild->task.threads_list); // Quitar de la lista de threads
            list_add_tail(lhcurrent, &freequeue);
            return -ENOMEM;
        }
        
        // Mapear la página lógica al frame físico
        set_ss_pag(parent_PT, first_page + i, new_ph_pag);
    }

    printk("clone 3 \n");
    
    // La pila de usuario crece hacia abajo, así que el puntero apunta al final+1 del espacio asignado
    uchild->task.user_stack_sp = (void*)((first_page + required_pages) << 12);
    
    // Inicializar la pila con el frame de activación
    // Asegurarnos de que la pila está correctamente alineada
    unsigned int stack_top = (unsigned int)(uchild->task.user_stack_sp);
    
    // Guardar parámetros en la pila de usuario
    *((unsigned int*)(stack_top - 4)) = (unsigned int)param;  // Parámetro para la función
    *((unsigned int*)(stack_top - 8)) = (unsigned int)func;   // Dirección de la función
    *((unsigned int*)(stack_top - 12)) = 0;                   // Dirección de retorno (0 para thread)
    
    // Modificar el contexto de ejecución para que el thread comience a ejecutarse en wrapper_func
    int register_ebp = (int)get_ebp();
    register_ebp = (register_ebp - (int)current()) + (int)(uchild);
    
    uchild->task.register_esp = register_ebp + sizeof(DWord);
    
    DWord temp_ebp = *(DWord*)register_ebp;
    uchild->task.register_esp -= sizeof(DWord);
    
    // Cuando el thread comience, lo hará en wrapper_func
    *(DWord*)(uchild->task.register_esp) = (DWord)&wrapper_func;
    uchild->task.register_esp -= sizeof(DWord);
    *(DWord*)(uchild->task.register_esp) = temp_ebp;
    
    // Establecer prioridad inicial (heredada del padre)
    uchild->task.priority = current()->priority;
  }


  
  // Inicializar estadísticas
  init_stats(&(uchild->task.p_stats));
  
  // Añadir a la cola de ready
  uchild->task.state = ST_READY;
  list_add_tail(&(uchild->task.list), &readyqueue);

  printk("clone 4 \n");
  
  // Retornar el PID para procesos o el TID para threads
  return (what == CLONE_PROCESS) ? uchild->task.PID : uchild->task.TID;
}

void sys_exit(void); // Forward declaration

void sys_threadExit(void)
{
  struct task_struct* master = current()->master;
  
  // Si es el último thread del proceso, finalizar el proceso
  if (master->num_threads == 1) {
    sys_exit();
    return;
  }
  
  // Liberar los recursos específicos del thread
  // Liberar la pila de usuario
  if (current()->user_stack_sp) {
    page_table_entry* pt = get_PT(current());
    
    // Comenzar desde la página que contiene la pila (user_stack_sp apunta al final)
    int page = ((unsigned int)current()->user_stack_sp >> 12) - 1;
    
    // Liberar las páginas contiguas de la pila mientras estén mapeadas
    while (page >= PAG_LOG_INIT_DATA + NUM_PAG_DATA && pt[page].bits.present) {
      int frame = get_frame(pt, page);
      del_ss_pag(pt, page);
      free_frame(frame);
      page--;
    }
  }
  
  // Actualizar el contador de threads del proceso
  master->num_threads--;
  
  // Quitar el thread de la lista de threads del proceso
  list_del(&current()->threads_list);
  
  // Añadir este task_struct a la freequeue
  current()->TID = -1;
  current()->PID = -1;
  list_add_tail(&(current()->list), &freequeue);
  
  // Planificar el siguiente thread/proceso
  sched_next_rr();
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

  // Comprobar si somos un thread dentro de un proceso con múltiples threads
  struct task_struct* master = current()->master;
  
  // Si no somos el master thread o el único thread, solo liberamos nuestros recursos
  if (current() != master && master->num_threads > 1) {
    // Somos un thread secundario, usar threadExit en su lugar
    sys_threadExit();
    return;
  }
  
  // Si llegamos aquí, es porque somos el master thread o el último thread del proceso
  // Procedemos a liberar todos los recursos del proceso
  
  int i;
  page_table_entry *process_PT = get_PT(current());
	printk("exit 1 \n");
  // Liberar todas las páginas de datos
  for (i=0; i<NUM_PAG_DATA; i++) {
    free_frame(get_frame(process_PT, PAG_LOG_INIT_DATA+i));
    del_ss_pag(process_PT, PAG_LOG_INIT_DATA+i);
  }
  	printk("exit 2 \n");
  // Si somos el master thread, liberar también todos los threads asociados
  if (current() == master && master->num_threads > 1) {
    struct list_head *l, *l_next;
    
    // Recorrer la lista de threads del proceso
    list_for_each_safe(l, l_next, &master->threads) {
      struct task_struct *t = list_head_to_task_struct(l);
      
      // Liberar la pila de usuario del thread
      if (t->user_stack_sp) {
        page_table_entry* pt = get_PT(t);
        
        int page = ((unsigned int)t->user_stack_sp >> 12) - 1;
        
        while (page >= PAG_LOG_INIT_DATA + NUM_PAG_DATA && pt[page].bits.present) {
          int frame = get_frame(pt, page);
          del_ss_pag(pt, page);
          free_frame(frame);
          page--;
        }
      }
      
      // Quitar el thread de la lista y liberarlo
      list_del(&t->threads_list);
      list_del(&t->list);
      t->TID = -1;
      t->PID = -1;
      list_add_tail(&t->list, &freequeue);
    }
  }
  
  // Liberar cualquier otro recurso específico del proceso
  // Por ejemplo, la pantalla compartida si existe
  if (current()->screen_buffer != NULL) {
    int frame = get_frame(process_PT, ((int)current()->screen_buffer) >> 12);
    del_ss_pag(process_PT, ((int)current()->screen_buffer) >> 12);
    free_frame(frame);
    current()->screen_buffer = NULL;
  }
  
  // Añadir el task_struct del proceso/thread principal a la freequeue
  current()->PID = -1;
  current()->TID = -1;
  list_add_tail(&(current()->list), &freequeue);
  
  // Planificar el siguiente proceso/thread
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
	
	int logic_page = get_free_logical_page(pt, PAG_LOG_INIT_DATA + 2*NUM_PAG_DATA);
	if (logic_page == -1) {
		//couldn't find free logical page (should never happen)
		free_frame(frame);
		return (void *)-1;
	}
	set_ss_pag(pt, logic_page, frame);
	current()->screen_buffer = (void *) (logic_page << 12);
	return current()->screen_buffer;
}

int sys_threadCreate(void (*function)(void* arg), void* parameter) {
  // Llamar a sys_clone con CLONE_THREAD y un tamaño de pila predeterminado
  return sys_clone(CLONE_THREAD, (void *(*)(void*))function, parameter, 4096);
}