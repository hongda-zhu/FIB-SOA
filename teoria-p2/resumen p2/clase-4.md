### **Clase 4: E/S Síncrona/Asíncrona y Gestores de Dispositivos**

En la clase anterior, aprendimos que cuando un proceso de usuario inicia una solicitud de E/S (como `read` o `write`), si se utiliza un modelo síncrono simple, el proceso se bloquea en modo núcleo hasta que la operación de E/S física se completa. Para dispositivos lentos, esto significa que la CPU puede permanecer inactiva durante mucho tiempo. La Clase 4 se centra en resolver este cuello de botella.

Los principales temas discutidos en esta clase incluyen:

1.  **Repaso del problema del bloqueo en la E/S síncrona y su impacto en la eficiencia de la CPU.**
2.  **Introducción de los gestores/manejadores de dispositivos como componente central de la solución.**
3.  **Mecanismo de implementación de la E/S síncrona con gestor:**
    * El rol del Bloque de Solicitud de E/S (IORB - Input/Output Request Block).
    * Colas de peticiones y notificación de resultados.
    * Uso de semáforos para la sincronización entre el hilo de usuario y el gestor.
4.  **Concepto e implementación de la E/S asíncrona:**
    * Cómo permitir que un hilo de usuario continúe su ejecución mientras se realiza una operación de E/S.
    * Modelos de interfaz típicos de E/S asíncrona.

-----

### **1. Repaso del problema del bloqueo en la E/S síncrona**

* **Núcleo del problema:**
    Cuando un hilo de usuario solicita una E/S a través de una llamada al sistema (como `read` o `write`), si el núcleo interactúa directamente con el dispositivo físico y espera a que termine (por ejemplo, esperando la búsqueda en disco, la transferencia de datos), ese hilo de usuario (incluso si está ejecutando la llamada al sistema en modo núcleo) se suspende, es decir, se **bloquea**.
* **Impacto en la eficiencia de la CPU:**
    * La velocidad de las operaciones de E/S físicas (especialmente en discos duros mecánicos o redes) es mucho menor que la velocidad de procesamiento de la CPU.
    * Mientras un hilo está bloqueado esperando la E/S, si la CPU no tiene otras tareas que realizar, permanecerá inactiva. Esto provoca un gran desperdicio de recursos computacionales, reduciendo el rendimiento general del sistema y su capacidad de respuesta.
    * En el fichero `.md` también se subraya que la raíz de este problema es la **larga latencia** de las operaciones de E/S.

-----

### **2. Introducción de los Gestores/Manejadores de Dispositivos**

* **Idea central:**
    Para evitar que un hilo de usuario se bloquee durante mucho tiempo esperando por el hardware y para permitir que la CPU realice otras tareas durante ese tiempo, el sistema operativo introduce los **gestores de dispositivos** (también conocidos como manejadores de dispositivos o planificadores de E/S).
* **Rol del gestor de dispositivos:**
    * Generalmente son uno o más **hilos del núcleo** (o en algunos diseños, procesos del núcleo) que se especializan en realizar las operaciones de E/S reales con uno o un grupo de dispositivos físicos específicos.
    * La solicitud de E/S de un hilo de usuario ya no impulsa directamente el hardware, sino que la solicitud se **envía** al gestor de dispositivos correspondiente.
    * El gestor de dispositivos mantiene una o más **colas de peticiones** de E/S para los dispositivos que gestiona.
    * Cuando un dispositivo físico completa una operación, lo notifica a la CPU mediante una **interrupción de hardware**. La rutina de servicio de interrupción (ISR) a su vez notifica o despierta al gestor de dispositivos que estaba esperando la interrupción de ese dispositivo.

* **Diagrama: Arquitectura básica**
    ```
    +-------------------+      Petición de E/S      +----------------------+      Operación      +-----------------+
    |  Hilo de Usuario  | ------------------------> |  Gestor de Dispositivo| -----------------> | Dispositivo     |
    | (ej: en proceso A)|      (Crea IORB,          |  (Hilo/Proceso del   |      (A través      | Físico          |
    |                   |       encola IORB)        |   Núcleo)            |       del Driver)    | (ej: Disco Duro)|
    +-------------------+                           +----------------------+ <----------------- +-----------------+
            ^                                               ^      |           Interrupción (E/S completada)
            | Bloqueado esperando                           |      |
            | resultado (en E/S síncrona)                   |      | Desencola IORB,
            |                                               |      | procesa
            +-----------------------------------------------+      |
                          Notificación de finalización               v
                                                             Cola de Peticiones (IORBs)
    ```

-----

### **3. Mecanismo de implementación de la E/S Síncrona con gestor**

Aunque se denomina "E/S síncrona", lo que significa que desde la perspectiva del hilo de usuario la llamada sigue esperando a que el resultado esté disponible para continuar, su implementación interna optimiza el uso de la CPU a través de un gestor.

* **a. Bloque de Solicitud de E/S (IORB - Input/Output Request Block):**
    * Cuando un hilo de usuario inicia una solicitud de E/S (por ejemplo, a través de la capa VFS hacia el subsistema de E/S de bajo nivel), el núcleo crea un bloque de datos estructurado llamado **IORB**.
    * **Información contenida en el IORB:**
        * Tipo de operación (lectura, escritura, etc.).
        * Información del dispositivo lógico/físico de destino (por ejemplo, especificado indirectamente a través de un descriptor de dispositivo o número mayor/menor).
        * Dirección y tamaño del búfer en el espacio de usuario.
        * Desplazamiento dentro del fichero (si aplica).
        * Un puntero/identificador al hilo de usuario que realiza la solicitud (o a su primitiva de sincronización asociada).
        * Un **semáforo (semáforo del IORB)** para la sincronización de la finalización de este IORB específico.

* **b. Cola de peticiones y notificación de resultados:**
    1.  **Encolar:** El IORB creado se introduce en la **cola de peticiones** del gestor de dispositivos correspondiente.
    2.  **Bloqueo del hilo de usuario:** Después de encolar el IORB, el hilo de usuario que ha realizado la petición ejecuta una operación `wait` sobre el **semáforo específico del IORB** asociado a él. Dado que este semáforo se inicializa normalmente a 0 (indicando que el recurso no está disponible), el hilo de usuario se bloquea inmediatamente, esperando a que se complete el procesamiento de ese IORB.
    3.  **Procesamiento por parte del gestor:** El gestor de dispositivos toma un IORB de su cola de peticiones (posiblemente siguiendo alguna estrategia de planificación, como FCFS, algoritmo del ascensor, etc.) y luego interactúa con el dispositivo físico a través del controlador de dispositivo correspondiente para ejecutar la operación descrita en el IORB.
    4.  **Finalización de la operación y despertar:** Cuando la operación de E/S física se completa (el dispositivo notifica al gestor mediante una interrupción), el gestor marca el IORB como completado y realiza una operación `signal` sobre el **semáforo específico del IORB** asociado.
    5.  **Continuación del hilo de usuario:** La operación `signal` despierta al hilo de usuario que previamente estaba en `wait` sobre ese semáforo. El hilo de usuario reanuda su ejecución desde el estado de bloqueo, y su llamada al sistema de E/S (como `read`) puede ahora obtener el resultado y retornar.

* **c. Uso de Semáforos para la sincronización (detallado según el fichero `.md`):**
    Para coordinar las actividades entre los hilos de usuario y el gestor de dispositivos, normalmente se necesitan varios tipos de semáforos:

    1.  **Semáforo del IORB (o de Operación):**
        * **Propósito:** Bloquear el hilo de usuario que inicia la solicitud de E/S hasta que su petición específica se complete.
        * **Inicialización:** `sem_init(&iorb->s, 0, 0)` (el segundo 0 indica que el semáforo se comparte entre hilos del mismo proceso, el tercer 0 es el valor inicial).
        * **Hilo de usuario:** `encolar_peticion(iorb); sem_wait(&iorb->s); obtener_resultado(iorb);`
        * **Hilo del gestor:** `procesar_io(iorb); sem_post(&iorb->s);`

    2.  **Semáforo de trabajo del Gestor:**
        * **Propósito:** Permitir que el gestor de dispositivos se duerma cuando la cola de peticiones está vacía, para evitar la espera activa (sondear constantemente la cola consumiendo CPU).
        * **Inicialización:** `sem_init(&gestor->s_trabajo, 0, 0)`.
        * **Hilo del gestor:** `while(true) { sem_wait(&gestor->s_trabajo); iorb = desencolar_peticion_protegida(); ... }`
        * **Hilo de usuario (o lógica de encolado):** `encolar_peticion_protegida(iorb); sem_post(&gestor->s_trabajo);` (despierta al gestor, que podría estar dormido, después de encolar un IORB).

    3.  **Semáforo de exclusión mutua para la Cola de Peticiones (Mutex):**
        * **Propósito:** Proteger el acceso concurrente a la cola de peticiones compartida (varios hilos de usuario pueden intentar encolar al mismo tiempo, y el hilo gestor desencola).
        * **Inicialización:** `sem_init(&cola_iorbs->mutex, 0, 1)` (valor inicial 1, para implementar un cerrojo de exclusión mutua).
        * **Antes de acceder a la cola:** `sem_wait(&cola_iorbs->mutex);`
        * **Después de acceder a la cola:** `sem_post(&cola_iorbs->mutex);`

* **Diagrama: Flujo de E/S síncrona con gestor y semáforos**
    ```
    Hilo de Usuario:                                Gestor de Dispositivo (Hilo del Núcleo):
    1. Preparar IORB_A
       (sem_init(&IORB_A.sem_op, 0, 0))
    2. sem_wait(&cola.mutex)
    3. Encolar IORB_A en cola_peticiones
    4. sem_post(&cola.mutex)
    5. sem_post(&gestor.sem_trabajo) // Avisar al gestor
    6. sem_wait(&IORB_A.sem_op) <------ Bloqueado esperando -----+
    7. IORB_A completado, obtener resultado                     |
                                                                |
                                       A. bucle:                |
                                       B. sem_wait(&gestor.sem_trabajo) <--- Si la cola está vacía, se duerme
                                       C. sem_wait(&cola.mutex)
                                       D. Desencolar IORB_A de cola_peticiones
                                       E. sem_post(&cola.mutex)
                                       F. Procesar IORB_A (comunicarse con el driver, E/S física)
                                       G. sem_post(&IORB_A.sem_op) // Despertar hilo de usuario ---+
    ```

-----

### **4. Concepto e implementación de la E/S Asíncrona**

* **Objetivo principal:**
    Permitir que un hilo de usuario **no se bloquee** después de iniciar una solicitud de E/S, sino que pueda retornar inmediatamente y continuar ejecutando otras tareas computacionales. La operación de E/S real se procesa en segundo plano por el sistema operativo (generalmente, por el gestor de dispositivos y los hilos del núcleo) en paralelo. El hilo de usuario puede comprobar el estado y el resultado de la operación de E/S en un momento posterior.
* **Diferencia con la E/S síncrona:**
    * **Síncrona:** "Iniciar petición -> Esperar -> Operación completada -> Obtener resultado -> Continuar".
    * **Asíncrona:** "Iniciar petición -> Retorno inmediato -> (El hilo continúa haciendo otras cosas) ... -> Más tarde, comprobar estado/obtener resultado".
* **Modelos de interfaz típicos de E/S asíncrona (según el fichero `.md`, similar a POSIX AIO):**
    Normalmente implica un conjunto de llamadas al sistema o funciones de biblioteca:
    1.  **Iniciar la solicitud de E/S asíncrona:**
        * Por ejemplo, `aio_read(&aiocb)` o una función similar. `aiocb` es una estructura de bloque de control (Asynchronous I/O Control Block), similar a un IORB, que contiene el tipo de operación, el descriptor de fichero, el búfer, el tamaño, el desplazamiento y cómo notificar la finalización, etc.
        * Esta llamada envía la solicitud de E/S al núcleo (por ejemplo, construyendo un IORB a partir del contenido de `aiocb` y entregándolo al gestor de dispositivos) y luego **retorna inmediatamente** al llamador, normalmente sin esperar a que la E/S se complete realmente.
    2.  **Consultar el estado de la operación:**
        * Por ejemplo, `aio_error(&aiocb)`.
        * Se utiliza para comprobar el estado actual de la operación asíncrona especificada (identificada por `aiocb`):
            * `EINPROGRESS`: La operación todavía está en curso.
            * `0`: La operación se ha completado con éxito.
            * Otro código de error: La operación ha fallado.
        * Esta llamada suele ser no bloqueante.
    3.  **Obtener el resultado de la operación:**
        * Por ejemplo, `aio_return(&aiocb)`.
        * Si la operación asíncrona se ha completado con éxito (`aio_error` devuelve 0), esta función devuelve el resultado de esa operación (por ejemplo, para `aio_read`, devuelve el número de bytes realmente leídos).
        * Si la operación no se ha completado o ha fallado, el comportamiento puede depender de la implementación específica (podría bloquearse, o devolver -1 y establecer `errno`).
    4.  **Punto de espera/sincronización (opcional):**
        * Por ejemplo, `aio_suspend()`: Bloquea el hilo llamador hasta que una o más operaciones de E/S asíncronas especificadas se completen.
    5.  **Cancelar la operación (opcional):**
        * Por ejemplo, `aio_cancel()`: Intenta cancelar una solicitud de E/S asíncrona que se ha enviado pero que aún no se ha completado.
* **Mecanismo de implementación:**
    * Internamente en el núcleo, las solicitudes de E/S asíncronas suelen ser gestionadas por gestores de dispositivos y **hilos de trabajo del núcleo (kernel worker threads)** especializados, y su flujo es similar a la segunda parte de la E/S síncrona con gestor (el gestor procesando el IORB).
    * La diferencia clave es que el hilo de usuario no espera mediante su semáforo privado después de enviar la solicitud.
    * **Mecanismo de notificación:** Cuando una operación de E/S asíncrona se completa, el núcleo puede notificar al proceso de usuario de varias maneras:
        * **Sondeo (Polling):** El proceso de usuario llama periódicamente a `aio_error`.
        * **Señal (Signal):** El núcleo envía una señal específica al proceso de usuario (por ejemplo, configurando `sigev_notify = SIGEV_SIGNAL` en el `aiocb`).
        * **Función de devolución de llamada (Callback - no común en POSIX AIO tradicional, pero presente en otros modelos asíncronos):** El núcleo invoca una función de devolución de llamada previamente registrada por el usuario.
* **Mención en el fichero `.md`:** "Kernel threads can do the work" (Los hilos del núcleo pueden hacer el trabajo), que es precisamente el núcleo de la implementación de la E/S asíncrona, es decir, descargar las operaciones de E/S que consumen mucho tiempo a entidades concurrentes dentro del núcleo.
* **Ventajas:**
    * **Mejora la capacidad de respuesta del programa:** La interfaz de usuario o la lógica principal no se congela esperando la E/S.
    * **Mejora la utilización de los recursos:** Permite que un programa realice otros cálculos o inicie otras E/S mientras espera que se complete una operación de E/S.
    * **Potencial para una alta concurrencia de E/S:** Se pueden iniciar múltiples solicitudes de E/S asíncronas simultáneamente, permitiendo que el sistema operativo y el hardware las procesen en paralelo.
