#include <asm.h>
#include <segment.h>

ENTRY(write)
    pushl %ebp
    movl %esp, %ebp
    
    pushl %edi
    pushl %esi
    pushl %ebx

    pushl %ecx
    pushl %edx
    
    movl 8(%ebp), %edx    /* fd */   
    movl 12(%ebp), %ecx   /* buffer */
    movl 16(%ebp), %ebx   /* size */
    
    movl $4, %eax        
    
    pushl $write_return
    
    pushl %ebp
    movl %esp, %ebp
    
    
    sysenter

write_return:
    popl %ebp
    addl $4, %esp
    
    popl %edx
    popl %ecx

    popl %ebx
    popl %esi
    popl %edi
    
    cmpl $0, %eax
    jge write_exit
    
    negl %eax
    movl %eax, errno
    movl $-1, %eax
    
write_exit:
    movl %ebp, %esp
    popl %ebp
    ret

ENTRY(gettime)
    pushl %ebp
    movl %esp, %ebp

    movl $10, %eax

    pushl %ecx
    pushl %edx

    pushl %edi
    pushl %esi
    pushl %ebx

    pushl $gettime_return

    pushl %ebp
    movl %esp, %ebp

    sysenter

gettime_return:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx
    
    popl %ebx
    popl %esi
    popl %edi

    cmpl $0, %eax
    jge gettime_exit

    negl %eax
    movl %eax, errno
    movl $-1, %eax

gettime_exit:
    movl %ebp, %esp
    popl %ebp
    ret


ENTRY(getpid)
    pushl %ebp
    movl %esp, %ebp

    movl $20, %eax

    pushl %ecx
    pushl %edx

    pushl %edi
    pushl %esi
    pushl %ebx

    pushl $getpid_return

    pushl %ebp
    movl %esp, %ebp

    sysenter

getpid_return:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx
    
    popl %ebx
    popl %esi
    popl %edi

    cmpl $0, %eax
    jge getpid_exit

    negl %eax
    movl %eax, errno
    movl $-1, %eax

getpid_exit:
    movl %ebp, %esp
    popl %ebp
    ret


ENTRY(fork)
    pushl %ebp
    movl %esp, %ebp

    movl $2, %eax

    pushl %ecx
    pushl %edx

    pushl %edi
    pushl %esi
    pushl %ebx

    pushl $fork_return

    pushl %ebp
    movl %esp, %ebp

    sysenter

fork_return:
    popl %ebp
    addl $4, %esp

    popl %edx
    popl %ecx
    
    popl %ebx
    popl %esi
    popl %edi

    cmpl $0, %eax
    jge fork_exit

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fork_exit:
    movl %ebp, %esp
    popl %ebp
    ret

