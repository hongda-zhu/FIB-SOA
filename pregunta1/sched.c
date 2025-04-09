/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return (struct task_struct *)((unsigned long)l & 0xFFFFF000);
}

struct list_head freequeue;
struct list_head readyqueue;
struct list_head zombie_queue;
extern struct list_head blocked;

struct task_struct * idle_task;
struct task_struct * task1;

/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

void cpu_idle(void)
{
	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
		printk("idle");
	;
	}
}

void init_idle (void)
{
	struct list_head * idle_list_head = list_first( &freequeue );
	list_del( idle_list_head ); 
	idle_task = list_head_to_task_struct(idle_list_head);
	idle_task->PID = 0;
	idle_task->parent = 0; //nullptr
	idle_task->pending_unblocks = 0;
	idle_task->exit_error = 0;
    INIT_LIST_HEAD(&idle_task->waiters);
	INIT_LIST_HEAD(&idle_task->children);
	allocate_DIR(idle_task);
	union task_union * idle_task_union = (union task_union *) idle_task;
	idle_task_union->stack[KERNEL_STACK_SIZE-1] = cpu_idle;
	idle_task_union->stack[KERNEL_STACK_SIZE-2] = 0;
	idle_task->k_esp = &(idle_task_union->stack[KERNEL_STACK_SIZE-2]);
}

extern void writeMSR(int msr, unsigned long value);
void init_task1(void)
{
	struct list_head * task1_list_head = list_first( &freequeue );
	list_del( task1_list_head );
	task1 = list_head_to_task_struct(task1_list_head);
	task1->PID = 1;
	task1->parent = idle_task;
	task1->pending_unblocks = 0;
	list_add_tail(&(task1->proc_anchor), &(idle_task->children));
	INIT_LIST_HEAD(&task1->children);
	task1->exit_error = 0;
    INIT_LIST_HEAD(&task1->waiters);
	task1->quantum = 2;
	allocate_DIR(task1);
	set_user_pages(task1);
	union task_union * task1_union = (union task_union *) task1;
	
	tss.esp0 = &(task1_union->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175, &(task1_union->stack[KERNEL_STACK_SIZE]));
	
	set_cr3(task1->dir_pages_baseAddr);
}

void init_freequeue (void) {
	INIT_LIST_HEAD( &freequeue );
	for (int i = 0; i < NR_TASKS; i++) {
		list_add( &(task[i].task.list), &freequeue);
	};
}

void init_sched()
{
	init_freequeue();
	INIT_LIST_HEAD( &readyqueue );

	// Inicializar lista de zombies
    INIT_LIST_HEAD(&zombie_queue);
    
    // Inicializar los nuevos campos en todos los task_struct
    for (int i = 0; i < NR_TASKS; i++) {
        task[i].task.exit_error = 0;
        INIT_LIST_HEAD(&task[i].task.waiters);
    }
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

int get_quantum (struct task_struct *t) {
	return t->quantum;
}

void set_quantum (struct task_struct *t, int new_quantum) {
	t->quantum = new_quantum;
}

void update_sched_data_rr() {
	current()->stats_rr.elapsed_total_ticks++;
	
	if (current()->stats_rr.remaining_ticks > 0)
		current()->stats_rr.remaining_ticks--;
}

//returns: 1 if it is necessary to change the current process and 0 otherwise
int needs_sched_rr() {
	return (current()->stats_rr.remaining_ticks == 0) && !list_empty(&readyqueue);
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue) {
	if (dst_queue == 0) { //RUN
			list_del(&(t->list));
			t->stats_rr.remaining_ticks = get_quantum(t);
			t->stats_rr.total_trans++;
	}
	else {
		list_add_tail(&(t->list), dst_queue);
	}
}

void sched_next_rr () {
	struct list_head * lh;
	struct task_struct * ts;
	if (!list_empty(&readyqueue)) {
		lh = list_first(&readyqueue);
		ts = list_head_to_task_struct(lh);
	} else {
		ts = idle_task;
	}
	
	update_process_state_rr(ts, 0);
	task_switch(ts);
}
/*
void sched_next_rr() {
	struct task_struct * ready_task_struct;
	struct list_head * ready_proc = list_first(&readyqueue);
	if (ready_proc == 0) //no hi ha processos, posar idle
		ready_task_struct = idle_task;
	else
		ready_task_struct = list_head_to_task_struct(ready_proc);
		
	update_process_state_rr(ready_task_struct, 0);
	task_switch(ready_task_struct);
}
*/

void schedule() {
	update_sched_data_rr();
	if (needs_sched_rr()) {
		if (current()->PID != 0) { //no idle
			update_process_state_rr(current(), &readyqueue);
		}
		sched_next_rr();
	}
}

extern void task_switch43(void*, long*);
void inner_task_switch(union task_union*t) {
	//t = (union task_union*)idle_task;
	//long mobe = 1;
	tss.esp0 = &(t->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175, &(t->stack[KERNEL_STACK_SIZE]));
	set_cr3(t->task.dir_pages_baseAddr);
	//long* ebp = &mobe + 4;
	//current()->k_esp = ebp;
	task_switch43(&(current()->k_esp), t->task.k_esp);
}
