#include <libc.h>

/* Función sencilla que realiza la suma de dos números */
int add(int a, int b) {
    return a + b;
}

int addASM(int par1,int par2);
int gettime();
void perror();

int __attribute__ ((__section__(".text.main")))
main(void)
{
    int pid, result, x, len;
    char buffer[128], time_str[12], *message, *error_msg, *blanc;
    /*------------------------------------------------------------
     * Sección 5: Llamada al sistema fork (sys_fork)
     *------------------------------------------------------------*/

    
    // Realizar el fork
    pid = fork();
    
    if (pid < 0) {
        // Error en fork
        write(1, "Error en fork\n", 14);
    }
    else if (pid == 0) {
        // Código del hijo
        write(1, "Soy el proceso hijo\n", 20);
        itoa(getpid(), buffer);
        write(1, "Mi PID es: ", 11);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
    }
    else {
        // Código del padre
        write(1, "Soy el proceso padre\n", 21);
        itoa(getpid(), buffer);
        write(1, "Mi PID es: ", 11);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
        write(1, "PID de mi hijo: ", 16);
        itoa(pid, buffer);
        write(1, buffer, strlen(buffer));
        write(1, "\n", 1);
    }

    /*------------------------------------------------------------
     * Sección 4: Llamada al sistema write (sys_write)
     * Envía un mensaje a la pantalla, comprobando que la llamada al sistema write funciona correctamente
     *------------------------------------------------------------*/

	// test getpid
	itoa(getpid(), message);
    result = write(1, message, strlen(message));
    
    itoa(pid, message);
    result = write(1, "forkRes:", 8);
    result = write(1, message, strlen(message));
    

    /*------------------------------------------------------------
     * Sección 3: Llamada al sistema gettime
     * La función gettime() retorna el número de ticks transcurridos desde el arranque del sistema
     *------------------------------------------------------------*/
    write(1, "\n", 1); 
    x= gettime();

    time_str[12];
    itoa(x, time_str); 
    len = strlen(time_str);
    write(1, time_str, len);

    /*------------------------------------------------------------
     * Sección 2: Llamada al sistema write (sys_write)
     * Envía un mensaje a la pantalla, comprobando que la llamada al sistema write funciona correctamente
     *------------------------------------------------------------*/
    
    // test write
    *message = "Hello, optimized write syscall!";
    *blanc ;
    result = write(1, message, strlen(message));
    // result = write(0, message, strlen(message));  // error case while fd = 0
    // result = write(1, blanc, strlen(blanc));  // EFAULT
    
    // if error
    if (result < 0) {
        char *error_msg = "Error in write syscall\n";
        write(1, error_msg, strlen(error_msg));
        perror();
    }
    

    /*------------------------------------------------------------
     * Sección 1: Gestión de errores de página (Page Fault)
     * Se intenta acceder a una dirección inválida (0), lo que desencadena una excepción de fallo de página.
     * El manejador de excepción imprimirá la dirección EIP inadecuada y se detendrá el sistema.
     *------------------------------------------------------------*/
    
    // char *p = 0;
    // *p = 'x'; 

    

    while(1){
		//write(1, "init", 4);
        ;  /* Se permanece en bucle infinito tras la excepción */
    }

        /*------------------------------------------------------------
     * Sección 0: Llamada a función normal (no es una llamada al sistema)
     * Llamada a la función add() para probar la mecánica básica de llamadas a función
     *------------------------------------------------------------*/
    add(42, 66);

    return 0;
}
