# Desarrollo Detallado: Proyecto Q2 2024-2025 SOA

## 1. Objetivo General del Proyecto

El objetivo principal es **mejorar el sistema operativo ZeOS** para que sea capaz de **soportar la ejecución de un videojuego interactivo**. Actualmente, ZeOS carece de las funcionalidades necesarias para manejar eficientemente la entrada de teclado en tiempo real, la salida gráfica para los frames del juego, y la concurrencia requerida para la lógica del juego. El Pajuelo sugiere implementar un juego tipo Pac-Man, pero deja abierta la posibilidad de crear otros juegos como Snake o Arkanoid, o cualquier otro que demuestre las capacidades añadidas. Lo fundamental no es el juego en sí, sino **crear el soporte necesario dentro del sistema operativo** para que este tipo de aplicaciones sean posibles.

## 2. Arquitectura Requerida para el Videojuego

Independientemente del juego específico que se decida implementar, este **debe seguir una arquitectura basada en dos threads (hilos)** que trabajen de forma concurrente:

* **Thread 1: Gestión de Entrada y Estado del Jugador:**
    * Responsable de leer el estado del teclado para determinar las acciones del jugador.
    * Actualiza el estado del juego en base a la entrada del teclado (ej: mover a Pac-Man).
    * Debe usar la llamada `pause` para bloquearse durante un breve periodo después de cada lectura, evitando así consumir el 100% de la CPU mientras espera la siguiente interacción.
* **Thread 2: Lógica del Juego y Renderizado:**
    * Gestiona el movimiento de elementos autónomos (ej: fantasmas, pelota de Arkanoid) y otras lógicas del juego.
    * Calcula el estado final del frame a mostrar.
    * Dibuja el frame resultante en la pantalla.
* **Exclusión Mutua:** Dado que ambos threads probablemente necesitarán acceder y modificar datos compartidos (por ejemplo, la posición de Pac-Man, el estado del tablero), es **imprescindible** utilizar mecanismos de exclusión mutua (semáforos, que se implementarán en el Milestone 4) para garantizar la integridad de los datos y evitar condiciones de carrera.
* **Pantalla y Estadísticas:**
    * La pantalla en ZeOS se considera una matriz de 80 columnas por 25 filas. Se debe revisar cómo funciona `printc` para entender el acceso a la memoria de vídeo.
    * Es necesario mostrar estadísticas de rendimiento, concretamente los **frames por segundo (FPS)**, en la línea superior de la pantalla.

## 3. Milestone 1: Soporte de Teclado (2 puntos)

Este milestone se centra en permitir que los programas de usuario puedan leer el estado del teclado de forma eficiente.

* **Modificación de la Interrupción de Teclado:**
    * La rutina actual de interrupción de teclado simplemente muestra la tecla pulsada.
    * Se debe modificar esta rutina para que, en lugar de (o además de) imprimir, actualice un **buffer de estado del teclado mantenido por el kernel**. Este buffer (un array) debe tener una entrada por cada tecla posible (tantas como `charmap`), indicando si está pulsada (1) o no (0) en ese momento. La interrupción actualizará este buffer cada vez que se pulse o libere una tecla.
* **Syscall `GetKeyboardState(char *keyboard)`:**
    * **Propósito:** Permitir al proceso de usuario obtener el estado actual de todas las teclas.
    * **Funcionamiento:** Copia el contenido del buffer de estado del kernel (mantenido por la rutina de interrupción) al buffer `keyboard` proporcionado por el proceso de usuario. El buffer de usuario debe estar prealocado.
    * **Características:** Es una llamada **no bloqueante**. Devuelve 0 si la operación fue exitosa, o -1 (u otro código de error negativo) si hubo un problema (ej: buffer de usuario inválido).
* **Syscall `pause(int milliseconds)`:**
    * **Propósito:** Bloquear temporalmente el thread que la invoca. Esto es esencial para el thread que lee el teclado, para que no esté constantemente preguntando por el estado (`GetKeyboardState`) y consumiendo CPU.
    * **Funcionamiento:** El thread que llama a `pause` se pone en estado `BLOCKED` durante (aproximadamente) el número de milisegundos especificado. Una vez transcurrido el tiempo, el planificador lo devolverá al estado `READY`.
    * **Precisión:** El reloj del sistema en ZeOS genera interrupciones unas 18 veces por segundo (cada 5.56ms aprox.). La llamada `pause` no podrá ser más precisa que esta granularidad. Si se pide `pause(10)`, el bloqueo durará probablemente 2 ticks de reloj (≈11.1ms), pero esta aproximación es suficiente para el propósito del juego. Devuelve 0 si fue exitosa.

## 4. Milestone 2: Soporte de Pantalla (2 puntos)

El objetivo es proporcionar un mecanismo para que los procesos de usuario puedan dibujar gráficos (o caracteres en modo texto) en la pantalla de forma eficiente.

* **Memoria de Vídeo:**
    * La memoria física donde reside el contenido de la pantalla en modo texto comienza en la dirección `0xB8000`.
    * Está organizada como una matriz de 80 columnas x 25 filas.
    * Cada celda de la matriz ocupa 2 bytes: el primer byte es el código ASCII del carácter y el segundo byte contiene los atributos (4 bits color de fondo, 4 bits color de letra).
    * Normalmente, solo el kernel (código en nivel de privilegio 0) puede escribir directamente en esta zona de memoria. La función `printk` lo hace.
* **Syscall `StartScreen()`:**
    * **Propósito:** Establecer un mecanismo para que el proceso pueda dibujar.
    * **Funcionamiento:**
        1.  El kernel busca una página física libre.
        2.  Mapea esta página física a una dirección lógica dentro del espacio de direcciones del proceso que realizó la llamada. El algoritmo para elegir la página lógica es "first-free".
        3.  Devuelve al proceso la **dirección lógica** de esta página mapeada. Si no se puede asignar la página, devuelve un error (ej: `(void*)-1`).
        4.  El kernel también guarda una referencia a esta página física asociada al proceso. Un proceso solo puede tener una página de este tipo.
* **Mecanismo de Dibujado:**
    * El **proceso de usuario** ahora escribe el contenido del frame que quiere mostrar directamente en la memoria a partir de la **dirección lógica** que le devolvió `StartScreen`. Escribir en esta página lógica es, en efecto, escribir en la página física compartida.
    * La **rutina de interrupción del reloj del kernel** (`clock_routine`), que se ejecuta periódicamente (18 veces/seg), realiza la siguiente acción en cada tick:
        1.  Identifica qué proceso está actualmente en ejecución (`current()`).
        2.  Comprueba si este proceso ha llamado previamente a `StartScreen` (es decir, si tiene una página física compartida asociada).
        3.  Si la tiene, el kernel **copia el contenido completo** de esa página física compartida a la memoria de vídeo real (`0xB8000`).
    * **Resultado:** El contenido que el usuario escribe en su página lógica aparece en pantalla, actualizado a la frecuencia del reloj del sistema (≈18 FPS teóricos máximos debidos a este mecanismo).
    * **Concurrencia:** Si varios procesos llaman a `StartScreen`, cada uno tendrá su propia página compartida. Sin embargo, en cada tick de reloj, **solo se vuelca a la pantalla el contenido del proceso que esté en ejecución en ese instante**. Si hay un cambio de contexto, la pantalla cambiará para mostrar lo que el nuevo proceso tenga en su buffer.

## 5. Milestone 3: Soporte de Threads (2 puntos)

Este es un milestone crucial que introduce el concepto de threads en ZeOS, cambiando fundamentalmente el modelo de ejecución.

* **Motivación:**
    * Los procesos tienen espacios de memoria separados. La comunicación entre ellos (IPC) requiere pasar por el kernel (ej: pipes, sockets), lo cual es lento e introduce sobrecarga (`overhead`).
    * Para tareas cooperativas (como un videojuego donde diferentes componentes deben interactuar rápidamente), se necesita un paralelismo más eficiente.
    * Los **threads** ofrecen esta eficiencia. Múltiples threads pueden existir dentro de **un mismo proceso**, compartiendo sus recursos, especialmente la **memoria**. Esto permite una comunicación muy rápida (simplemente escribiendo/leyendo en memoria compartida) sin involucrar al kernel.
* **Nuevo Paradigma:**
    * El **Proceso** se convierte en un **contenedor de recursos** (memoria, descriptores de fichero, etc.). Ya no es la unidad que se ejecuta directamente.
    * El **Thread** se convierte en la **unidad mínima de planificación y ejecución**. El planificador (`scheduler`) ahora gestionará y alternará la ejecución de threads.
    * Cuando un thread pide un recurso (ej: memoria con `malloc`, un fichero con `open`), el recurso se asigna al **proceso** al que pertenece el thread.
* **Implementación (Estilo Linux):**
    * **Abstracción de Procesos:** ZeOS adoptará un enfoque similar a Linux donde **no existe una estructura de datos separada para el "proceso"**. La estructura que antes representaba un proceso (`task_union` conteniendo `task_struct`) ahora **representará un thread**. La noción de "proceso" emerge de un grupo de threads que **comparten ciertos recursos críticos**, como el directorio de páginas de memoria, logrado a través del mecanismo de creación.
    * **Información del Thread:** Cada `task_struct` (ahora de thread) necesitará:
        * Identificador único (TID, que en este modelo será global como el PID anterior).
        * Pila de sistema (ya contenida en `task_union`).
        * Pila de usuario (necesita ser gestionada).
        * Contexto de ejecución (registros, guardados en la pila de sistema durante cambios de contexto/interrupciones).
        * Puntero al directorio de páginas (compartido por threads del mismo "proceso").
        * Almacenamiento Local del Thread (TLS): Un espacio para variables propias del thread, como `errno`.
    * **Syscall `clone` (Reemplaza a `sys_fork`):**
        * Firma: `int clone(int what, void *(*func)(void*), void *param, int stack_size);`.
        * Parámetro `what`: Indica si se crea un "proceso" (`CLONE_PROCESS`) o un "thread" (`CLONE_THREAD`).
        * Parámetros `func`, `param`, `stack_size`: Solo relevantes para `CLONE_THREAD`. `func` es la dirección de la función que el nuevo thread comenzará a ejecutar. `param` es el argumento que se pasará a `func`. `stack_size` especifica el tamaño requerido para la pila de usuario del nuevo thread.
        * **Mecanismo Interno:**
            1.  **Obtener `task_union` libre:** Igual que en `fork`.
            2.  **Copiar `task_union` del creador:** `copy_data(current(), new_thread, ...)`. Esto es **clave**: si se crea un thread (`CLONE_THREAD`), esta copia hace que se comparta el puntero al directorio de páginas (`dir_pages_baseAddr`), la tabla de ficheros, etc., formando así parte del mismo "proceso". Si se crea un "proceso" (`CLONE_PROCESS`), la lógica sería similar a `fork`, creando copias separadas de ciertos recursos como la tabla de páginas.
            3.  **Asignar TID:** Dar un identificador único al nuevo thread.
            4.  **Preparar Contexto de Ejecución (para `CLONE_THREAD`):**
                * Asignar espacio para la **pila de usuario** del nuevo thread (basado en `stack_size`).
                * En la pila de usuario recién asignada, construir el **frame de activación** para la llamada `func(param)`. Esto implica empilar el parámetro `param` y una dirección de retorno (puede ser una función de salida del thread o nula si `func` no debe retornar).
                * Modificar la **pila de sistema** del nuevo thread (que es una copia de la del creador). Específicamente, se deben sobrescribir los valores guardados de `EIP` y `ESP` de usuario que `iret` restaurará. El `EIP` guardado (`@ handler` en la pila) debe apuntar ahora a la dirección de `func`. El `ESP` guardado debe apuntar a la cima del frame de activación recién creado en la pila de usuario.
                * Actualizar el puntero `register_esp` en el `task_struct` del nuevo thread para que apunte a la cima actual de su pila de sistema (modificada para el `ret_from_fork` o similar).
            5.  **Añadir a `readyqueue`:** Poner el nuevo thread en la cola de listos.
        * **Wrappers:** Se deben mantener las funciones de biblioteca `fork()` y una nueva `pthread_create()` (o nombre similar). `fork()` llamará a `sys_clone` con `CLONE_PROCESS`. `pthread_create()` llamará a `sys_clone` con `CLONE_THREAD` y los parámetros `func`, `param`, `stack_size` correspondientes.
    * **Planificación con Prioridades y Apropiación Inmediata:**
        * **Syscall `SetPriority(int priority)`:** Permite al thread actual establecer su propia prioridad. Un valor numérico más alto indica mayor prioridad. Se debe añadir un campo para almacenar la prioridad en `task_struct`.
        * **Modificación del Planificador:** El planificador debe implementar **apropiación inmediata** (preemptive scheduling based on priority).
            * **¿Cuándo verificar?:** La necesidad de replanificar por prioridad debe comprobarse cuando:
                1.  Un thread pasa de `BLOCKED` a `READY` (ej: al terminar `pause` o `sem_wait`).
                2.  Se crea un nuevo thread con `clone`.
                3.  El thread actual cambia su prioridad con `SetPriority`.
                4.  (Opcional, pero común) Al final de un tick de reloj si se implementa time-slicing basado en prioridad.
            * **Lógica:** Si un thread (`T_new`) que acaba de pasar a `READY` (o se acaba de crear) tiene una prioridad **estrictamente mayor** que la del thread actualmente en `RUN` (`T_run`), entonces:
                1.  `T_run` es interrumpido inmediatamente (se le quita la CPU).
                2.  Se cambia el estado de `T_run` a `READY` y se encola en `readyqueue`.
                3.  `T_new` pasa directamente al estado `RUN` y obtiene la CPU.
            * Si `T_new` no tiene mayor prioridad, simplemente se añade a `readyqueue` como antes.

## 6. Milestone 4: Soporte de Semáforos (2 puntos)

Introduce primitivas para la sincronización y exclusión mutua, esenciales para la programación concurrente con threads.

* **Necesidad: Condiciones de Carrera (`Race Conditions`)**
    * Cuando múltiples threads acceden y modifican datos compartidos sin coordinación, el resultado final puede depender del orden exacto (no determinista) en que se intercalan sus instrucciones.
    * **Ejemplo Clásico:** Dos threads (T1, T2) ejecutan `A++` sobre una variable compartida `A` (inicialmente 0). La operación `A++` se compila típicamente en varias instrucciones ensamblador (leer A en registro, incrementar registro, escribir registro en A). Si T1 lee A (0), y antes de que escriba el 1, el planificador cambia a T2, T2 también leerá A (0), incrementará a 1 y escribirá 1. Cuando T1 vuelva a ejecutarse, escribirá el 1 que tenía en su registro. Resultado final: A=1, en lugar del esperado A=2.
    * La solución es asegurar que la secuencia de instrucciones que accede/modifica el dato compartido (la **sección crítica**) se ejecute **atómicamente** respecto a otros threads que accedan al mismo dato, es decir, en **exclusión mutua**. El programador debe identificar estas secciones críticas.
* **Semáforos:**
    * Son un mecanismo ofrecido por el kernel para implementar la exclusión mutua.
    * **Estructura Interna:** Un semáforo gestionado por el kernel consiste fundamentalmente en:
        1.  Un **contador entero**.
        2.  Una **cola** donde pueden esperar los threads que intentan acceder al recurso protegido por el semáforo y lo encuentran ocupado.
* **Operaciones (Syscalls - Locales al Proceso):**
    * **`int sem_init(int value)`:**
        * Crea una nueva instancia de semáforo en el kernel, asociada al proceso actual.
        * Inicializa el **contador** interno con el `value` proporcionado. Para exclusión mutua simple (permitir solo un thread a la vez), `value` típicamente es **1**.
        * Inicializa la cola de bloqueados como vacía.
        * Devuelve un **identificador entero (`sem_id`)** único (dentro del proceso) que se usará para referirse a este semáforo en las otras llamadas.
    * **`int sem_wait(int sem_id)` (También conocido como P, down, acquire):**
        * Intenta entrar en la sección crítica protegida por el semáforo `sem_id`.
        * **Decrementa** el contador del semáforo.
        * **Comprueba** el nuevo valor del contador:
            * Si es **>= 0**: El recurso está disponible. El thread continúa y entra en la sección crítica.
            * Si es **< 0**: El recurso no está disponible (otro thread ya está en la sección crítica). El thread actual **se bloquea**. Para ello:
                1.  Se añade el `task_struct` del thread actual a la cola de bloqueados del semáforo.
                2.  Se cambia el estado del thread a `BLOCKED`.
                3.  Se invoca al planificador (`sched_next_rr()`) para ceder la CPU a otro thread. El thread no continuará hasta que sea despertado por un `sem_post`.
    * **`int sem_post(int sem_id)` (También conocido como V, up, signal, release):**
        * Señala que el thread ha salido de la sección crítica protegida por `sem_id`.
        * **Incrementa** el contador del semáforo.
        * **Comprueba** el nuevo valor del contador:
            * Si es **> 0**: No había nadie esperando, simplemente se ha incrementado la disponibilidad del recurso.
            * Si es **<= 0**: Había al menos un thread esperando en la cola del semáforo. El kernel debe despertar a uno de ellos:
                1.  Se saca el primer thread de la cola de bloqueados del semáforo.
                2.  Se cambia su estado a `READY`.
                3.  Se añade a la `readyqueue` para que el planificador lo considere.
    * **`int sem_destroy(int sem_id)`:**
        * Libera los recursos asociados al semáforo `sem_id` en el kernel. Se debe asegurar que no haya threads bloqueados en él cuando se destruye.
* **Uso Típico (Exclusión Mutua):**
    ```c
    // Inicialización (una sola vez)
    int mutex_sem = sem_init(1);

    // ... en el código de cada thread que necesita acceder al recurso ...
    sem_wait(mutex_sem); // Intentar entrar: bloquearse si está ocupado
    // --- Inicio de la Sección Crítica ---
    // Acceder/modificar datos compartidos aquí
    // v[pos] = dato; pos++;
    // --- Fin de la Sección Crítica ---
    sem_post(mutex_sem); // Salir y despertar a un posible thread en espera
    ```

## 7. Milestone 5: Videojuego Funcional (2 puntos)

Este es el milestone final donde se integran todas las piezas anteriores para crear la aplicación de usuario: el videojuego.

* **Implementación del Juego:**
    * Elegir un juego (Pac-Man, Snake, Arkanoid, u otro).
    * Implementar la lógica del juego elegida.
* **Utilización de las Nuevas Funcionalidades de ZeOS:**
    * **Arquitectura de 2 Threads:** El juego debe estructurarse obligatoriamente con los dos threads descritos: uno para entrada/estado y otro para lógica/renderizado.
    * **Entrada:** Usar `GetKeyboardState` para leer el teclado y `pause` para evitar el busy-waiting en el thread de entrada.
    * **Salida:** Usar `StartScreen` para obtener el buffer de pantalla y escribir los frames en él desde el thread de renderizado.
    * **Sincronización:** Usar `sem_init`, `sem_wait`, `sem_post` para proteger cualquier acceso a datos compartidos entre los dos threads (ej: posición del jugador, estado del tablero).
    * **Estadísticas:** Calcular y mostrar los FPS en la parte superior de la pantalla.
* **Pruebas:** Es fundamental probar exhaustivamente no solo el juego, sino también la correcta funcionalidad de todas las llamadas al sistema implementadas en los milestones anteriores.

## 8. Evaluación y Logística

* **Orden y Dependencia:** Los milestones deben realizarse y entregarse en orden (1 al 5). La evaluación de un milestone requiere que los anteriores sean correctos.
* **Feedback Continuo:** Se recomienda **enviar cada milestone por correo electrónico al Pajuelo** para validación (`ok` / `no ok` con explicación) antes de continuar con el siguiente. Esto asegura los puntos y evita problemas posteriores.
* **Entrega Final:** Es **obligatorio** realizar al menos una entrega final en la plataforma Racó. En esta entrega se debe indicar explícitamente hasta qué milestone se ha completado y validado.
* **Nota:** Cada milestone funcional aporta 2 puntos a la nota final del proyecto. No es necesario completar los 5 milestones para obtener una nota; cada estudiante decide hasta dónde llegar.
* **Autonomía y Decisiones:** El enunciado no cubre todos los detalles de implementación (ej: números exactos para las syscalls, gestión de errores específicos, qué hacer si `fork` es llamado por un proceso con múltiples threads). Los estudiantes deben **tomar decisiones de diseño razonables**, buscando información si es necesario (ej: comportamiento estándar en Linux) y justificándolas si se requiere.
