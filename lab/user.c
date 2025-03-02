#include <libc.h>

/* Función sencilla que realiza la suma de dos números */
int add(int a, int b) {
    return a + b;
}

int addASM(int par1,int par2);
int gettime();

int __attribute__ ((__section__(".text.main")))
main(void)
{
    int ticks;

    /*------------------------------------------------------------
     * Sección 3: Llamada al sistema gettime
     * La función gettime() retorna el número de ticks transcurridos desde el arranque del sistema
     *------------------------------------------------------------*/
    ticks = gettime();

    /*------------------------------------------------------------
     * Sección 2: Llamada al sistema write (sys_write)
     * Envía un mensaje a la pantalla, comprobando que la llamada al sistema write funciona correctamente
     *------------------------------------------------------------*/
     

    /*------------------------------------------------------------
     * Sección 1: Gestión de errores de página (Page Fault)
     * Se intenta acceder a una dirección inválida (0), lo que desencadena una excepción de fallo de página.
     * El manejador de excepción imprimirá la dirección EIP inadecuada y se detendrá el sistema.
     *------------------------------------------------------------*/
    char *p = 0;
    *p = 'x';  /* Esto provoca una exception Page Fault */

    while(1){
        ;  /* Se permanece en bucle infinito tras la excepción */
    }

        /*------------------------------------------------------------
     * Sección 0: Llamada a función normal (no es una llamada al sistema)
     * Llamada a la función add() para probar la mecánica básica de llamadas a función
     *------------------------------------------------------------*/
    add(42, 66);

    return 0;
}
