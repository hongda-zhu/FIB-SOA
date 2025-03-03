#include <asm.h>
#include <segment.h>

ENTRY(write)
    pushl %ebp
    movl %esp, %ebp
    
    pushl %edx
    pushl %ecx
    
    movl 8(%ebp), %ebx    /* fd */   
    movl 12(%ebp), %ecx   /* buffer */
    movl 16(%ebp), %edx   /* size */
    

    movl $4, %eax        
    
    pushl $write_return
    
    pushl %ebp
    movl %esp, %ebp
    
    sysenter

write_return:
    popl %ebp
    addl $4, %esp
    
    popl %ecx
    popl %edx
    
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

    pushl $gettime_return

    pushl %ebp
    movl %esp, %ebp

    sysenter

gettime_return:
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge gettime_exit

    negl %eax
    movl %eax, errno
    movl $-1, %eax

gettime_exit:
    movl %ebp, %esp
    popl %ebp
    ret

