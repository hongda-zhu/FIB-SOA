# 0 "user-utils.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "user-utils.S"
# 1 "include/asm.h" 1
# 2 "user-utils.S" 2

.globl syscall_sysenter; .type syscall_sysenter, @function; .align 0; syscall_sysenter:
 push %ecx
 push %edx
 push $SYSENTER_RETURN
 push %ebp
 mov %esp, %ebp
 sysenter
.globl SYSENTER_RETURN; .type SYSENTER_RETURN, @function; .align 0; SYSENTER_RETURN:
 pop %ebp
 pop %edx
 pop %edx
 pop %ecx
 ret


.globl write; .type write, @function; .align 0; write:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $4, %eax
 movl 0x8(%ebp), %ebx;
 movl 0xC(%ebp), %ecx;
 movl 0x10(%ebp), %edx;
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


nok:
 neg %eax
 mov %eax, errno
 mov $-1, %eax
 popl %ebp
 ret


.globl gettime; .type gettime, @function; .align 0; gettime:
 pushl %ebp
 movl %esp, %ebp
 movl $10, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl getpid; .type getpid, @function; .align 0; getpid:
 pushl %ebp
 movl %esp, %ebp
 movl $20, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl fork; .type fork, @function; .align 0; fork:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 pushl %esi
 movl $7, %eax
 movl $0, %ebx;
 movl -4(%ebp), %ecx;
 movl $0, %edx;
 movl $4096, %esi ;
 call syscall_sysenter
 popl %esi
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


.globl pthread_create; .type pthread_create, @function; .align 0; pthread_create:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 pushl %esi
 movl $7, %eax
 movl $1, %ebx;
 movl 0x8(%ebp), %ecx;
 movl 0xC(%ebp), %edx;
 movl 0x10(%ebp), %esi;
 call syscall_sysenter
 popl %esi
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


.globl exit; .type exit, @function; .align 0; exit:
 pushl %ebp
 movl %esp, %ebp
 movl $1, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl pthread_exit; .type pthread_exit, @function; .align 0; pthread_exit:
 pushl %ebp
 movl %esp, %ebp
 movl $8, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl yield; .type yield, @function; .align 0; yield:
 pushl %ebp
 movl %esp, %ebp
 movl $13, %eax
 call syscall_sysenter
 popl %ebp
 ret


.globl get_stats; .type get_stats, @function; .align 0; get_stats:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $35, %eax
 movl 0x8(%ebp), %ebx;
 movl 0xC(%ebp), %ecx;
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


.globl GetKeyboardState; .type GetKeyboardState, @function; .align 0; GetKeyboardState:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $3, %eax
 movl 0x8(%ebp), %ebx;
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


.globl SetPriority; .type SetPriority, @function; .align 0; SetPriority:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $2, %eax
 movl 0x8(%ebp), %ebx;
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


.globl pause; .type pause, @function; .align 0; pause:
 pushl %ebp
 movl %esp, %ebp
 pushl %ebx;
 movl $5, %eax
 movl 0x8(%ebp), %ebx;
 call syscall_sysenter
 popl %ebx
 test %eax, %eax
 js nok
 popl %ebp
 ret


.globl StartScreen; .type StartScreen, @function; .align 0; StartScreen:
 pushl %ebp
 movl %esp, %ebp
 movl $6, %eax
 call syscall_sysenter
 popl %ebp
 ret



.globl semCreate; .type semCreate, @function; .align 0; semCreate:
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $23, %eax
  movl 0x8(%ebp), %ebx
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok
  popl %ebp
  ret


.globl semWait; .type semWait, @function; .align 0; semWait:
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $24, %eax
  movl 0x8(%ebp), %ebx
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok
  popl %ebp
  ret


.globl semPost; .type semPost, @function; .align 0; semPost:
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $25, %eax
  movl 0x8(%ebp), %ebx
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok
  popl %ebp
  ret


.globl semDestroy; .type semDestroy, @function; .align 0; semDestroy:
  pushl %ebp
  movl %esp, %ebp
  pushl %ebx
  movl $26, %eax
  movl 0x8(%ebp), %ebx
  call syscall_sysenter
  popl %ebx
  test %eax, %eax
  js nok
  popl %ebp
  ret
