/*
 * io.c - 
 */

#include <io.h>

#include <types.h>

/**************/
/** Screen  ***/
/**************/

#define NUM_COLUMNS 80
#define NUM_ROWS    25

Byte x, y=19;

/* Read a byte from 'port' */
Byte inb (unsigned short port)
{
  Byte v;

  __asm__ __volatile__ ("inb %w1,%0":"=a" (v):"Nd" (port));
  return v;
}

void printc(char c)
{
     __asm__ __volatile__ ( "movb %0, %%al; outb $0xe9" ::"a"(c)); /* Magic BOCHS debug: writes 'c' to port 0xe9 */
  if (c=='\n')
  {
    x = 0;
    y=(y+1)%NUM_ROWS;
  }
  else
  {
    Word ch = (Word) (c & 0x00FF) | 0x0200;
	Word *screen = (Word *)0xb8000;
	screen[(y * NUM_COLUMNS + x)] = ch;
    if (++x >= NUM_COLUMNS)
    {
      x = 0;
      y=(y+1)%NUM_ROWS;
    }
  }
}

void printc_xy(Byte mx, Byte my, char c)
{
  Byte cx, cy;
  cx=x;
  cy=y;
  x=mx;
  y=my;
  printc(c);
  x=cx;
  y=cy;
}

void printk(char *string)
{
  int i;
  for (i = 0; string[i]; i++)
    printc(string[i]);
}

void print_number(int num)
{
  char buffer[12];  
  int pos = 0;      
  
  if (num < 0) {
    printc('-');
    num = -num;  
  }
  
  if (num == 0) {
    printc('0');
    return;
  }
  
  while (num > 0 && pos < sizeof(buffer) - 1) {
    buffer[pos++] = (num % 10) + '0';  
    num /= 10;
  }
  
  buffer[pos] = '\0';  
  
  while (--pos >= 0) {
    printc(buffer[pos]);
  }
}

void print_hex(int num)
{
	char hex_buffer[9];
    hex_buffer[8] = '\0';  // Asegura que la cadena termine correctamente
    
    // Convierte el valor a hexadecimal, dígito por dígito
    for (int i = 7; i >= 0; i--) {
        int digit = num & 0xF;  // Obtiene el último dígito hexadecimal
        hex_buffer[i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        num >>= 4;  // Desplaza 4 bits a la derecha (un dígito hexadecimal)
    }
    
    printk(hex_buffer);
    printk("\n");
}
