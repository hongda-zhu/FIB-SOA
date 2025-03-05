/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>
#include <utils.h>
#include <zeos_interrupt.h>

void keyboard_handler();
void clock_handler();
void page_fault2_handler();
void system_call_handler();
void syscall_handler_sysenter();
void writeMSR(int msr, unsigned long value);

Gate idt[IDT_ENTRIES];
Register    idtR;

char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','\xA1','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','\xF1',
  '\0','\xE7','\0','\xBA','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};


void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;

  writeMSR(0x174, __KERNEL_CS);
  writeMSR(0x175, INITIAL_ESP);
  writeMSR(0x176, (unsigned long)syscall_handler_sysenter);

  
  set_handlers();

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */
  setInterruptHandler(14, page_fault2_handler, 0);
  setInterruptHandler(33, keyboard_handler, 0);
  setInterruptHandler(32, clock_handler, 0);
  setTrapHandler(0x80, system_call_handler, 3);
  set_idt_reg(&idtR);
}

void keyboard_routine()
{
	char key = inb(0x60);
	int make = (key & 0x80) == 0;
	key = key&0x7F;
	
	if (make) {
		char c = char_map[(unsigned char) key];
		if (c == '\0') c = 'L';
		
		printc_xy(75, 20, c);
	}
}

void page_fault2_routine(int err, int p)
{
    unsigned int eip = p;  // Guarda el valor original de p
    printk("Process generates a PAGE FAULT exception at EIP: 0x");
    
    // Buffer para almacenar la representación hexadecimal (8 dígitos + terminador nulo)
    char hex_buffer[9];
    hex_buffer[8] = '\0';  // Asegura que la cadena termine correctamente
    
    // Convierte el valor a hexadecimal, dígito por dígito
    for (int i = 7; i >= 0; i--) {
        int digit = eip & 0xF;  // Obtiene el último dígito hexadecimal
        hex_buffer[i] = (digit < 10) ? ('0' + digit) : ('A' + digit - 10);
        eip >>= 4;  // Desplaza 4 bits a la derecha (un dígito hexadecimal)
    }
    
    printk(hex_buffer);
    printk("\n");
    while(1);
}

int zeos_ticks = 0;

void init_zeos_ticks()
{
	zeos_ticks = 0;
}

void clock_routine()
{
  zeos_show_clock();
  ++zeos_ticks;
}
