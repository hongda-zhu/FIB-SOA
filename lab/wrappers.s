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

    pushl %ecx
    pushl %edx
    pushl %edi
    pushl %esi
    pushl %ebx

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

    popl %ebx
    popl %esi
    popl %edi
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

    popl %ebx
    popl %esi
    popl %edi
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


.globl getpid; .type getpid, @function; .align 0; getpid:
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

    popl %ebx
    popl %esi
    popl %edi
    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge getpid_exit

    negl %eax
    movl %eax, errno
    movl $-1, %eax

getpid_exit:
    movl %ebp, %esp
    popl %ebp
    ret


.globl fork; .type fork, @function; .align 0; fork:
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

    popl %ebx
    popl %esi
    popl %edi
    popl %edx
    popl %ecx

    cmpl $0, %eax
    jge fork_exit

    negl %eax
    movl %eax, errno
    movl $-1, %eax

fork_exit:
    movl %ebp, %esp
    popl %ebp
    ret
