/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>

union task_union task[NR_TASKS]
  __attribute__((__section__(".data.task")));

#if 1 //=?=?===????
struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}
#endif

struct list_head freequeue;
struct list_head readyqueue;
extern struct list_head blocked;

struct task_struct * idle_task;

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
	;
	}
}

void init_idle (void)
{
	struct list_head * idle_list_head = list_first( &freequeue );
	list_del( idle_list_head ); 
	idle_task = list_head_to_task_struct(idle_list_head);
	idle_task->PID = 0;
	allocate_DIR(idle_task);
	union task_union * idle_task_union = (union task_union *) &idle_task;
	idle_task_union->stack[KERNEL_STACK_SIZE-1] = cpu_idle;
	idle_task_union->stack[KERNEL_STACK_SIZE-2] = 0;
	idle_task->k_esp = &(idle_task_union->stack[KERNEL_STACK_SIZE-2]);
}

void init_task1(void)
{
	struct list_head * task1_list_head = list_first( &freequeue );
	list_del( task1_list_head );
	struct task_struct * task1 = list_head_to_task_struct(task1_list_head);
	task1->PID = 1;
	allocate_DIR(task1);
	set_user_pages(task1);
	union task_union * task1_union = (union task_union *) &task1;
	
	tss.esp0 = &(task1_union->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175, &(task1_union->stack[KERNEL_STACK_SIZE]));
	
	set_cr3(task1->dir_pages_baseAddr);
}

void init_freequeue (void) {
	INIT_LIST_HEAD( &freequeue );
	for (int i = 0; i < NR_TASKS; ++i) {
		list_add( &(task[i].task.list), &freequeue);
	};
}

void init_sched()
{
	init_freequeue();
	INIT_LIST_HEAD( &readyqueue );
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

void inner_task_switch(union task_union*t) {
	//long mobe = 1;
	tss.esp0 = &(t->stack[KERNEL_STACK_SIZE]);
	writeMSR(0x175, &(t->stack[KERNEL_STACK_SIZE]));
	set_cr3(t->task.dir_pages_baseAddr);
	//long* ebp = &mobe + 4;
	//current()->k_esp = ebp;
	task_switch43(&(current()->k_esp), t->task.k_esp);
}
