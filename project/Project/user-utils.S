#include <asm.h>

ENTRY(syscall_sysenter)
	push %ecx
	push %edx
	push $SYSENTER_RETURN
	push %ebp
	mov %esp, %ebp
	sysenter
ENTRY(SYSENTER_RETURN)
	pop %ebp
	pop %edx
	pop %edx
	pop %ecx
	ret

/* int write(int fd, char *buffer, int size) */
ENTRY(write)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $4, %eax
	movl 0x8(%ebp), %ebx;	//fd
	movl 0xC(%ebp), %ecx;	//buffer
	movl 0x10(%ebp), %edx;	//size
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -> 
	popl %ebp
	ret

/* Common code for negative return */
nok:
	neg %eax
	mov %eax, errno
	mov $-1, %eax
	popl %ebp
	ret

/* int gettime() */
ENTRY(gettime)
	pushl %ebp
	movl %esp, %ebp
	movl $10, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int getpid() */
ENTRY(getpid)
	pushl %ebp
	movl %esp, %ebp
	movl $20, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int fork() */
ENTRY(fork)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	pushl %esi
	movl $7, %eax
	movl $0, %ebx;	//what = 0
	movl -4(%ebp), %ecx; //*func = @ret = %ebp+4
	movl $0, %edx;	//*param = nothing
	movl $4096, %esi ;  //stack_size = 1 pag?
	call syscall_sysenter
	popl %esi
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* int pthread_create(void *(*func)(void*), void *param, int stack_size) */
ENTRY(pthread_create)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	pushl %esi
	movl $7, %eax
	movl $1, %ebx;	//what
	movl 0x8(%ebp), %ecx; //*func
	movl 0xC(%ebp), %edx; //*param
	movl 0x10(%ebp), %esi; //stack_size
	call syscall_sysenter
	call syscall_sysenter
	popl %esi
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -> 
	popl %ebp
	ret

/* void exit() */
ENTRY(exit)
	pushl %ebp
	movl %esp, %ebp
	movl $1, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* void pthread_exit() */
ENTRY(pthread_exit)
	pushl %ebp
	movl %esp, %ebp
	movl $8, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int yield() */
ENTRY(yield)
	pushl %ebp
	movl %esp, %ebp
	movl $13, %eax
	call syscall_sysenter
	popl %ebp
	ret

/* int get_stats(int pid, struct stats *st) */
ENTRY(get_stats)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $35, %eax
	movl 0x8(%ebp), %ebx;	//pid
	movl 0xC(%ebp), %ecx;	//st
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* int get_keyboard_state(char *keyboard) */
ENTRY(GetKeyboardState)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $3, %eax
	movl 0x8(%ebp), %ebx;	//*keyboard
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* int SetPriority(int priority) */
ENTRY(SetPriority)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $2, %eax
	movl 0x8(%ebp), %ebx;	//priority
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* int pause(int milliseconds) */
ENTRY(pause)
	pushl %ebp
	movl %esp, %ebp
	pushl %ebx;  // Save EBX, ESI and EDI if modified
	movl $5, %eax
	movl 0x8(%ebp), %ebx;	//milliseconds
	call syscall_sysenter
	popl %ebx
	test %eax, %eax
	js nok	// if (eax < 0) -->
	popl %ebp
	ret

/* void* StartScreen() */
ENTRY(StartScreen)
	pushl %ebp
	movl %esp, %ebp
	movl $6, %eax
	call syscall_sysenter
	popl %ebp
	ret


/* int semCreate(int value) */
ENTRY(semCreate)
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $23, %eax        // Número de syscall
  movl 0x8(%ebp), %ebx  // value
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok                // Si hay error
  popl %ebp
  ret

/* int semWait(int semid) */
ENTRY(semWait)
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $24, %eax        // Número de syscall
  movl 0x8(%ebp), %ebx  // semid
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok                // Si hay error
  popl %ebp
  ret

/* int semPost(int semid) */
ENTRY(semPost)
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $25, %eax        // Número de syscall
  movl 0x8(%ebp), %ebx  // semid
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok                // Si hay error
  popl %ebp
  ret

/* int semDestroy(int semid) */
ENTRY(semDestroy)
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $26, %eax        // Número de syscall
  movl 0x8(%ebp), %ebx  // semid
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok                // Si hay error
  popl %ebp
  ret