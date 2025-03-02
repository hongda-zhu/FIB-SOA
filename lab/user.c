#include <libc.h>

char buff[24];

int pid;

int add(int par1,int par2) {
	return par1 + par2;
}

int addASM(int par1,int par2);
int gettime();

int __attribute__ ((__section__(".text.main")))
  main(void)
{
	int res;
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	int t = gettime();
    res = add(0x42, 0x666);
  
  /*
  // Error EIP
  char* p = 0;
	*p = 'x';
	
  */
  // Error EIP
  char* p = 0;
	*p = 'x';
	
	
  while(1) { 
	  
	  }
}
