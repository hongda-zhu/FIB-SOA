# 0 "wrappers.S"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "wrappers.S"
# 1 "include/asm.h" 1
# 2 "wrappers.S" 2
# 1 "include/segment.h" 1
# 3 "wrappers.S" 2

.globl write; .type write, @function; .align 0; write:
    pushl %ebp
    movl %esp, %ebp

    pushl %edi
    pushl %esi
    pushl %ebx

    pushl %ecx
    pushl %edx

    movl 8(%ebp), %edx
    movl 12(%ebp), %ecx
    movl 16(%ebp), %ebx

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

.globl gettime; .type gettime, @function; .align 0; gettime:
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
