#include <libc.h>

char buff[24];

int pid;
char* screen_buffer;

int mobex;

void test(void* par) {
	SetPriority(2);
	while(1) {
		if (mobex == 1) {
			write(1, "xd", 2);
			mobex = 0;
		}
		else write(1, ",", 1);
	}
}

void draw_char(int x, int y, char c, char color) {
	x %= 80;
	y %= 25;
    int pos = (y * 80 + x) * 2;
    screen_buffer[pos] = c;
    screen_buffer[pos + 1] = color;
}

int __attribute__ ((__section__(".text.main")))
  main(void)
{
	mobex = 0;
	write(1, "USR CODE\n", 9);
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
	char keyboard_state[256];
	screen_buffer = (char*)StartScreen();
	
	if (pthread_create(test, (void*)0, 4096) != 0) {
		write(1, "CREATED_THREAD", 14);
		while(1) {
			if (mobex == 0) {
				write(1, "sus", 3);
				mobex = 1;
			}
			else write(1, ".", 1);
		}
	}
	
	if (fork() != 0) {
		write(1, "fork", 4);
		exit();
	}
	write(1, "boomer", 6);
	char buff[256];
	itoa(screen_buffer, buff);
	write(1, buff, strlen(buff));
	int x = 0;
  while(1) { 
	  //write(1, "x", 1);
	  //pause(1000);
	  //GetKeyboardState(keyboard_state);
	  //if (keyboard_state['c']) write(1, "c", 1);
	  if (x > 0) draw_char(x-1, 10, ' ', 0x07);

		draw_char(x, 10, '*', 0x0E);

		x++;
	  }
}
