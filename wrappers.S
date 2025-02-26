#include <asm.h>
#include <segment.h>


ENTRY(gettime)
	movl $10, %eax
	int $0x80
	ret
