#include <asm.h>
#include <segment.h>

ENTRY(write)
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %edx   /* fd */   
    movl 12(%ebp), %ecx  /* buffer */
    movl 16(%ebp), %ebx  /* size */

    movl $4, %eax       

    pushl %ecx           
    pushl %edx

    pushl $write_return

    pushl %ebp            
    movl %esp, %ebp

    sysenter

write_return:
    popl %ebp            
    addl $4, %esp        
    popl %edx            
    popl %ecx            
    
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

    /* 设置系统调用号 */
    movl $10, %eax

    /* 保存 ECX 和 EDX，因为 sysexit 会修改它们 */
    pushl %ecx
    pushl %edx

    /* 保存返回地址 */
    pushl $gettime_return

    /* 创建假动态链接 */
    pushl %ebp
    movl %esp, %ebp

    /* 进入内核 */
    sysenter

gettime_return:
    /* 恢复寄存器 */
    popl %ebp
    addl $4, %esp
    popl %edx
    popl %ecx

    /* 检查返回值是否为错误 */
    cmpl $0, %eax
    jge gettime_exit

    /* 处理错误：设置 errno 并返回 -1 */
    negl %eax
    movl %eax, errno
    movl $-1, %eax

gettime_exit:
    movl %ebp, %esp
    popl %ebp
    ret

