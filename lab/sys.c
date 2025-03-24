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

int BASE_PID = 2;

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
void mobe(){printc('a');}
extern struct list_head freequeue;
extern struct list_head readyqueue;
extern long* get_ebp();
int sys_fork2()
{
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
  
  /* KERNEL */
  for (pag=0;pag<NUM_PAG_KERNEL;pag++){
	set_ss_pag(childPT, pag, get_frame(parentPT, 0+pag));
  }

  /* CODE */
  for (pag=0;pag<NUM_PAG_CODE;pag++){
  	set_ss_pag(childPT, PAG_LOG_INIT_CODE+pag, get_frame(parentPT, PAG_LOG_INIT_CODE+pag));
  }

  /* DATA */ 
  for (pag=0;pag<NUM_PAG_DATA;pag++){
	childFrame = get_frame(childPT, PAG_LOG_INIT_DATA+pag); //fisica que correspon a la pagina logica DEL CHILD
	set_ss_pag(parentPT, 0x0F, childFrame); //assignem la pagina logica 0x0F del PARENT a la fisica que hem trobat

	copy_data((PAG_LOG_INIT_DATA+pag)*PAGE_SIZE, 0x0F*PAGE_SIZE, PAGE_SIZE); //copiem logica a logica, que correspon a copiar logica del parent a mateixa logica del child
	del_ss_pag(parentPT, 0x0F);
  }
  
  child_task->PID = BASE_PID++;
  mobe();
  child_task->k_esp = get_ebp();
  child_task->k_esp--;
   
  child_union->stack[(long)(child_task->k_esp) % 0x1000] = 0;
  child_union->stack[((long)(child_task->k_esp) % 0x1000) + 1] = ret_from_fork;
  
  child_task->current_state = ST_READY;
  list_add_tail(&(child_task->list), &readyqueue);
  printc('0' + child_task->PID);
  
  //flush TLB
  // set_cr3(get_DIR(current())); //WTFFF
  return child_task->PID;
}

int sys_fork() {
    
    // a) Get a free task_struct for the process. If there is no space for a new process, an error will be returned.

    if (list_empty(&freequeue)) return -ENOMEM;
    struct list_head *child_list_head = list_first(&freequeue);
    list_del(child_list_head);

    struct task_struct *child_task_struct = list_head_to_task_struct(child_list_head);
    
    union task_union *child_task_union = (union task_union *) child_task_struct;
    
    // b) Inherit system data: copy the parent’s task_union to the child. Determine whether it is necessary to modify the pag table of the parent to access the child’s system data. The copy_data function can be used to copy.

    copy_data(current(), child_task_union, TOTAL_PAGES);

    // c) Get a new pag directory to store the child’s address space and initialize the dir_pages_baseAddr field using the allocate_DIR routine.

    allocate_DIR(child_task_struct);

    /*
    d) Search frames (physical pages) in which to map logical pages for data+stack of the child process (using the alloc_frame function). If there are not enough free pages, an error will be returned.  

    e) Initialize child’s address space: Modify the pag table of the child process to map the logical addresses to the physical ones. This pag table is accessible through the directory field in the task_struct (get_PT routine can be used):

    // i) Page table entries for system code, system data and user code can have the same content as the parent’s pag table entries (they will be shared).
    // ii) Page table entries for the user data+stack have to point to the new allocated frames for this region.

    */

    int pag, pag2, new_frame;
    int PT_child = get_PT(child_task_struct);
    int PT_parent = get_PT(current());

    for (pag = 0; pag < NUM_PAG_DATA; pag++) {
        new_frame = alloc_frame();
        if (new_frame == -1) {
            // no hay frame libres
            free_user_pages(child_task_struct);
            /* 
            // Intento de free_frame
            for (pag2 = 0; pag2 < NUM_PAG_DATA; pag2++) {
                free_frame(get_frame(PT_child, pag + PAG_LOG_INIT_DATA)); 
                del_ss_pag(PT_child, pag + PAG_LOG_INIT_DATA);
            }
            */
            list_add(child_list_head, &freequeue);
            return -EAGAIN;
        } 
        // hay frame libre 
        set_ss_pag(PT_child, pag + PAG_LOG_INIT_DATA, new_frame);
    }

    // pasar los datos del padre al hijo

    //  Kernel: mismo sitio que del padre (apartado e. i)
    for (pag = 0; pag < NUM_PAG_KERNEL; pag++) {
        set_ss_pag(PT_child, pag, get_frame(PT_parent, pag));
    }

    // CODE: mismo sitio que del padre (apartado e. i)
    for (pag = 0; pag < NUM_PAG_CODE; pag++) {
        set_ss_pag(PT_child, pag + PAG_LOG_INIT_CODE, get_frame(PT_parent, pag + PAG_LOG_INIT_CODE));
    }

    // DATA: NO mismo sitio que del padre, debe estar estos datos en un nuevo lugar (apartado e) ii + apartado f)
    
    // Constantes para mejorar la legibilidad
    const int FIRST_DATA_PAGE = NUM_PAG_KERNEL + NUM_PAG_CODE;
    const int LAST_DATA_PAGE = FIRST_DATA_PAGE + NUM_PAG_DATA;
    
    // f: Para cada página de datos y pila del proceso
    for (int pag = FIRST_DATA_PAGE; pag < LAST_DATA_PAGE; pag++) {
        // Calcular direcciones base
        void *parent_addr = (void*)(pag << 12); // Dirección original en el padre
        
        // Calcular ubicación temporal para el mapeo
        int temp_page = pag + NUM_PAG_DATA;
        void *temp_addr = (void*)(temp_page << 12);
        
        // 1. Crear mapeo temporal: hacer accesible la página física del hijo 
        //    desde el espacio de direcciones del padre
        unsigned int child_frame = get_frame(PT_child, pag);
        set_ss_pag(PT_parent, temp_page, child_frame);
        
        // 2. Copiar los datos del padre al espacio del hijo
        copy_data(parent_addr, temp_addr, PAGE_SIZE);
        
        // 3. Eliminar el mapeo temporal
        del_ss_pag(PT_parent, temp_page);
    }
    // f.c) flush TLB
    set_cr3(get_DIR(child_task_struct));
    
    // g) + h) actualizar campos que tienen que ser diferente el hijo y el padre
    child_task_struct->PID = BASE_PID++;
    child_task_struct->current_state = ST_READY;

    // i)
    
   // Preparar el contexto de la pila para el retorno
    int parent_ebp = (int) get_ebp();
    int child_ebp = (parent_ebp - (int)current()) + (int)(child_task_union);
    
    // Guardar el valor de EBP que encontramos en la pila
    DWord saved_ebp_value = *(DWord*)child_ebp;
    
    // Configurar register_esp para apuntar justo después del EBP
    child_task_struct->k_esp = child_ebp + sizeof(DWord);
    
    // Añadir dirección de ret_from_fork en la pila
    child_task_struct->k_esp -= sizeof(DWord);
    *(DWord*)(child_task_struct->k_esp) = (DWord)&ret_from_fork;
    
    // Añadir valor de EBP en la pila
    child_task_struct->k_esp -= sizeof(DWord);
    *(DWord*)(child_task_struct->k_esp) = saved_ebp_value;
    
    // Añadir el hijo a la cola de procesos listos
    list_add_tail(&(child_task_struct->list), &readyqueue);
    
    // Retornar el PID del hijo
    return child_task_struct->PID;
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
