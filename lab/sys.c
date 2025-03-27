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

int sys_fork() {
    
    // a) Get a free task_struct for the process. If there is no space for a new process, an error will be returned.

    if (list_empty(&freequeue)) return -ENOMEM;
    struct list_head *child_list_head = list_first(&freequeue);
    list_del(child_list_head);

    struct task_struct *child_task_struct = list_head_to_task_struct(child_list_head);
    
    union task_union *child_task_union = (union task_union *) child_task_struct;
    
    // b) Inherit system data: copy the parent’s task_union to the child. Determine whether it is necessary to modify the pag table of the parent to access the child’s system data. The copy_data function can be used to copy.

    copy_data(current(), child_task_union, PAGE_SIZE);

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
            //free_user_pages(child_task_struct);
            
            // Intento de free_frame fins pag
            for (pag2 = 0; pag2 < pag; pag2++) {
                free_frame(get_frame(PT_child, pag2 + PAG_LOG_INIT_DATA));
                del_ss_pag(PT_child, pag2 + PAG_LOG_INIT_DATA);
            }
            
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

    // DATA: NO mismo sitio que del padre, debe estar estos datos en un nuevo lugar (apartado e) ii + apartado f)
    
    // f: Para cada página de datos y pila del proceso
    for (pag = PAG_LOG_INIT_DATA; pag < PAG_LOG_INIT_DATA+NUM_PAG_DATA; pag++) {
        // Calcular direcciones base
        void *parent_addr = (void*)(pag << 12); // Dirección original en el padre
        
        // Calcular ubicación temporal para el mapeo
        int temp_page = pag + NUM_PAG_DATA + NUM_PAG_CODE;
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

    // CODE: mismo sitio que del padre (apartado e. i)
    for (pag = 0; pag < NUM_PAG_CODE; pag++) {
        set_ss_pag(PT_child, pag + PAG_LOG_INIT_CODE, get_frame(PT_parent, pag + PAG_LOG_INIT_CODE));
    }

    // f.c) flush TLB
    set_cr3(get_DIR(current()));
    
    // g) + h) actualizar campos que tienen que ser diferente el hijo y el padre
	child_task_struct->PID = BASE_PID++;
	child_task_struct->parent = current();
	
	INIT_LIST_HEAD(&child_task_struct->children);
	
	list_add_tail(&(child_task_struct->proc_anchor), &(current()->children));
	
    // Calcular el contexto del hijo
    // Calcular la posición en la pila del hijo que corresponde al EBP del padre
    unsigned long parent_stack_pos = (unsigned long)get_ebp();
    unsigned long offset = parent_stack_pos - (unsigned long)current();
    unsigned long child_stack_pos = (unsigned long)child_task_union + offset;
    
    // Guardar el valor original que está en esa posición
    unsigned long original_value = *((unsigned long*)child_stack_pos);
    
    // Preparar la nueva estructura de pila para el retorno
    unsigned long *stack_ptr = (unsigned long*)child_stack_pos;
    
    // En lugar de mover ESP y luego escribir, modificamos directamente
    // la pila en las posiciones necesarias
    *stack_ptr = (unsigned long)&ret_from_fork;  // Sobreescribir con dirección de retorno
    
    // Colocar el valor original en la posición anterior
    child_task_struct->k_esp = (unsigned long)&stack_ptr[-1];
    stack_ptr[-1] = original_value;  // El valor del EBP salvado
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
    
    while (bytes_left > 0) {
        int chunk_size = (bytes_left > BUFFER_SIZE - 1) ? BUFFER_SIZE - 1 : bytes_left;
        
        if (copy_from_user(buffer + bytes_written, localbuffer, chunk_size) != 0)
            return -EFAULT;
        
        ret = sys_write_console(localbuffer, chunk_size);
        if (ret < 0)
            return ret;
        
        bytes_left -= ret;
        bytes_written += ret;
        
        if (ret < chunk_size)
            break;
    }
    
    return bytes_written;
}

extern int zeos_ticks;

int sys_gettime()
{
    return zeos_ticks;
}

extern struct list_head blocked;
extern struct task_struct * idle_task;
void sys_block() {
	struct task_struct * curr = current();
	if (curr->pending_unblocks != 0) {
		curr->pending_unblocks--;
		return;
	}
	curr->pending_unblocks = -1;
	list_add_tail(&(curr->list), &blocked);
	sched_next_rr();
}

int sys_unblock(int pid) {
	struct list_head * e;
	struct task_struct * child;
	int found = 0;
	list_for_each( e, &(current()->children) ) {
		child = list_head_to_task_struct(e);
		if (child->PID == pid) {
			found = 1;
			break;
		}
	}
	if (!found) return -1;
	
	if (child->pending_unblocks < 0) { //blocked
		child->pending_unblocks = 0;
		update_process_state_rr(child, &readyqueue);
	}
	else {
		child->pending_unblocks++;
	}
	return 0;
}

void sys_exit() {
	struct task_struct * curr = current();
	int pt = get_PT(current());
	int pag;
	
	//remove child from parent
	list_del(&curr->proc_anchor);
	
	//make children parents be idle process
	struct list_head * e;
	printk("x1");
	list_for_each( e, &(curr->children) ) {
		struct task_struct * child = list_head_to_task_struct(e);
		child->parent = idle_task;
		list_add_tail(&(idle_task->children), &(child->proc_anchor));
	}
	printk("x2");
	//free data section
	for (pag = 0; pag < NUM_PAG_DATA; pag++) {
		free_frame(get_frame(pt, pag + PAG_LOG_INIT_DATA));
		del_ss_pag(pt, pag + PAG_LOG_INIT_DATA);
	}printk("x3");
	list_add_tail(&(curr->list), &freequeue);printk("x4");
	sched_next_rr();
}
