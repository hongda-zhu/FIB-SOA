### **Clase 3: Independencia de dispositivo y llamadas al sistema principales**

Esta clase tiene como objetivo explicar cómo los sistemas operativos modernos ocultan la complejidad y diversidad del hardware subyacente a los usuarios y aplicaciones, proporcionando así una interfaz unificada y abstracta para interactuar con diversos dispositivos de entrada/salida (E/S).

Los principales temas tratados en esta clase incluyen:

1.  **Concepto y objetivo de la independencia de dispositivo.**
2.  **Clasificación de dispositivos en Linux (y sistemas tipo Unix): dispositivos físicos, lógicos y virtuales.**
3.  **Mecanismos de asociación entre dispositivos y el proceso de identificación de hardware durante el arranque del sistema.**
4.  **El rol y la estructura de los controladores de dispositivo (Drivers).**
5.  **Llamada al sistema `mknod`: creación de ficheros de dispositivo.**
6.  **El flujo interno de la llamada al sistema `open` (para ficheros de dispositivo).**
7.  **El flujo interno de la llamada al sistema `read` (para ficheros de dispositivo).**
8.  **El problema del bloqueo causado por las operaciones de E/S síncronas y una reflexión inicial.**

-----

### **1. Concepto y objetivo de la independencia de dispositivo**

* **Objetivo principal:**
    Un objetivo de diseño clave de los sistemas operativos modernos es lograr la **independencia de dispositivo**. Esto significa que los usuarios y las aplicaciones pueden utilizar un conjunto de interfaces unificadas y de alto nivel (como las llamadas al sistema estándar `open`, `read`, `write`, `close`) para acceder a una gran variedad de dispositivos de hardware muy diferentes (como discos duros, SSD, teclados, ratones, impresoras, interfaces de red, tarjetas gráficas, etc.), sin necesidad de preocuparse por cómo funcionan específicamente estos dispositivos o qué diferencias existen entre ellos a nivel de hardware.
* **El papel del sistema operativo:**
    El sistema operativo se encarga internamente de gestionar todos estos complejos detalles y diferencias relacionados con el hardware, proporcionando una visión simple y consistente a las capas superiores. Esta capacidad de abstracción simplifica enormemente el desarrollo de aplicaciones y mejora la portabilidad y escalabilidad del sistema (por ejemplo, cambiar la marca de un disco duro generalmente no requiere modificar el código de la aplicación).

-----

### **2. Clasificación de dispositivos en Linux**

Para implementar eficazmente la independencia de dispositivo, Linux (y otros sistemas tipo Unix) abstraen y clasifican los dispositivos:

* **a. Dispositivos Físicos:**
    * Se refieren a los componentes de hardware reales y tangibles, como la propia unidad de disco duro, el ratón, el teclado, el chip de la tarjeta de red, etc.
    * El código que interactúa directamente con el dispositivo físico es altamente dependiente del hardware específico y suele ser proporcionado por el fabricante del hardware, formando el núcleo del **controlador de dispositivo (Device Driver)**.

* **b. Dispositivos Lógicos:**
    * Es una **abstracción** que el sistema operativo proporciona para el acceso a E/S. Son las interfaces que los usuarios y las aplicaciones suelen "ver" y con las que interactúan.
    * En el sistema de ficheros, los dispositivos lógicos suelen manifestarse como **ficheros de dispositivo** especiales, la mayoría ubicados en el directorio `/dev` (por ejemplo, `/dev/sda` representa el primer disco duro SATA, `/dev/tty1` la primera terminal de consola, `/dev/input/mouse0` el ratón).
    * **Funciones y tipos:**
        * Pueden mapear directamente a un dispositivo físico (como `/dev/sda1` que corresponde a la primera partición de un disco duro).
        * Pueden representar un servicio o una funcionalidad sin corresponder directamente a un hardware específico (por ejemplo, `/dev/null`, un dispositivo "agujero negro" donde todos los datos escritos se descartan; `/dev/random`, un generador de números aleatorios).
        * Pueden utilizarse para agregar o virtualizar dispositivos físicos (por ejemplo, un volumen lógico que abarca varios discos físicos).
    * **Identificadores clave:** Los dispositivos lógicos se identifican de forma única mediante un par de números que especifican su tipo y su instancia concreta:
        * **Número Mayor (Major Number):** Identifica el controlador de dispositivo asociado a este dispositivo lógico (es decir, le dice al núcleo qué controlador es responsable de gestionar las operaciones en este dispositivo). Los dispositivos del mismo tipo (como todos los discos duros SATA) suelen compartir el mismo número mayor.
        * **Número Menor (Minor Number):** Es utilizado por el controlador de dispositivo correspondiente para distinguir entre múltiples instancias de dispositivos físicos del mismo tipo (por ejemplo, si el sistema tiene varios discos duros SATA, pueden tener el mismo número mayor, pero diferentes números menores, como `/dev/sda` con número menor 0 y `/dev/sdb` con número menor 1).

* **c. Dispositivos Virtuales:**
    * Cuando un proceso **abre** un dispositivo lógico (es decir, abre un fichero de dispositivo), el sistema operativo crea una **instancia de dispositivo virtual** para este acceso del proceso. Esta instancia se manifiesta normalmente como un **descriptor de fichero (file descriptor)** que se devuelve al proceso.
    * El dispositivo virtual mantiene el estado de acceso específico de ese proceso al dispositivo lógico correspondiente, como la posición del puntero de lectura/escritura actual (offset), el modo de apertura (solo lectura, solo escritura, lectura/escritura), etc.
    * Esto permite que múltiples procesos abran y utilicen el mismo dispositivo lógico simultáneamente (o secuencialmente), mientras que sus estados de acceso respectivos pueden no interferir entre sí (o pueden compartirse o sincronizarse según las características del dispositivo). Por ejemplo, varios procesos pueden abrir el mismo dispositivo de terminal al mismo tiempo, pero sus entradas y salidas se distinguirán.

* **Diagrama: Jerarquía de abstracción de dispositivos**
    ```
    +---------------------+      +---------------------+      +---------------------+
    |      Aplicación     |----->|  Dispositivo Lógico |----->|  Dispositivo Físico |
    | (ej: editor texto)  |      | (ej: /dev/tty1)     |      | (ej: Teclado y      |
    +---------------------+      +---------------------+      |  Controlador Pantalla)|
            ^                           ^      |               +---------------------+
            | (usa file descriptor)     |      | (SO usa Mayor/Menor para)
            |                           |      v
    +---------------------+      +---------------------+
    | Dispositivo Virtual |<-----|  Controlador Disp.  |
    | (estado por proceso)|      | (código específico) |
    +---------------------+      +---------------------+
    ```

-----

### **3. Mecanismos de asociación entre dispositivos e identificación de hardware en el arranque**

* **¿Cómo sabe el sistema operativo qué dispositivos están disponibles?**
    * **a. Proceso de arranque e sondeo de hardware (Boot Process & Hardware Probing):**
        1.  **Inicialización de BIOS/UEFI:** Cuando el ordenador se enciende, la BIOS (Basic Input/Output System) o UEFI (Unified Extensible Firmware Interface) realiza primero una autocomprobación e inicialización básica del hardware (CPU, memoria, etc.).
        2.  **Selección del dispositivo de arranque:** La BIOS/UEFI busca un dispositivo de almacenamiento de arranque (disco duro, USB, red, etc.) según la configuración (como el orden de arranque).
        3.  **Carga del núcleo del sistema operativo:** Una vez encontrado el dispositivo de arranque, el cargador de arranque (bootloader) carga el núcleo del sistema operativo en la memoria y comienza su ejecución.
        4.  **Sondeo de hardware por parte del núcleo:** Una vez que el núcleo está en ejecución, sondea activamente los dispositivos de hardware conectados a los diversos buses del ordenador (como PCI, USB, SATA, etc.). Los dispositivos suelen informar al núcleo de su identidad.
        5.  **VID y PID:** Los dispositivos de hardware suelen proporcionar dos identificadores clave al sistema operativo:
            * **ID de Vendedor (VID):** El ID único del fabricante.
            * **ID de Producto (PID):** El ID único del producto de ese fabricante.
            El núcleo utiliza el VID y el PID para determinar el modelo específico del dispositivo y buscar el controlador adecuado.

    * **b. Asociación de dispositivos lógicos y físicos — `mknod` y ficheros de dispositivo:**
        * Como se mencionó anteriormente, los dispositivos lógicos se manifiestan en el sistema de ficheros como ficheros de dispositivo. Estos ficheros (principalmente en el directorio `/dev`) se crean mediante la llamada al sistema `mknod` (make node) (o automáticamente durante la inicialización del sistema por mecanismos como `udev` / `devtmpfs`).
        * El comando `mknod` o su mecanismo equivalente, al crear un fichero de dispositivo, almacena en el **i-nodo** de ese fichero (nodo de información, explicado en detalle en la Clase 2) su tipo (dispositivo de caracteres o de bloque) y los cruciales **número mayor** y **número menor**.
        * Este i-nodo es la "encarnación" del dispositivo lógico en el sistema de ficheros. Al leer el número mayor/menor del i-nodo del fichero de dispositivo, el sistema operativo sabe qué controlador debe invocar para operar con el hardware físico (o virtual) asociado.

-----

### **4. El rol y la estructura de los controladores de dispositivo (Device Drivers)**

* **¿Qué es un controlador?**
    * Un controlador de dispositivo es una porción de código dentro del núcleo del sistema operativo (generalmente un módulo del núcleo cargable) que actúa como una capa de traducción o interfaz entre el núcleo del sistema operativo y un dispositivo de hardware específico.
    * Cada tipo de dispositivo de hardware (o una clase de dispositivos similares) necesita su correspondiente controlador.

* **Funciones de un controlador:**
    * Entender los detalles de bajo nivel de un dispositivo de hardware específico, como su conjunto de comandos, registros, mecanismo de interrupciones, etc.
    * Proporcionar una interfaz estándar y abstracta al núcleo del sistema operativo, para que este pueda controlar el hardware a través de este conjunto de interfaces sin necesidad de conocer los detalles privados del hardware.
    * Gestionar las interrupciones del hardware y la transferencia de datos con el mismo (DMA, PIO, etc.).

* **Estructura de un controlador (conceptual):**
    * Cuando el núcleo identifica un dispositivo y carga el controlador adecuado para él, este controlador generalmente registra un **descriptor de dispositivo** o un conjunto de **funciones de operación** (la estructura `file_operations` en Linux) en el núcleo.
    * Esta estructura contiene una serie de punteros a funciones específicas dentro del controlador, que implementan la lógica para realizar operaciones estándar en ese dispositivo (como `open`, `read`, `write`, `close`, `ioctl`, etc.).
    * Cuando el núcleo del sistema operativo necesita realizar una operación en ese dispositivo, invoca el código correspondiente en el controlador a través de estos punteros de función.

* **Diagrama: Interacción del controlador con el núcleo del sistema operativo**
    ```
    +-------------------------+
    |   Sistema Operativo     |
    |   (Núcleo / Kernel)     |
    |                         |
    |   +-------------------+ |
    |   | Subsistema de E/S | |
    |   | (genérico)        | |
    |   +-------------------+ |
    |           |             | ---- Petición de E/S (ej: leer de /dev/sda)
    |           v             |
    |   +-------------------+ | ---- Sabiendo que /dev/sda usa el driver "XYZ"
    |   | Driver "XYZ"      | |      (gracias al Número Mayor)
    |   |  - open_xyz()     | |
    |   |  - read_xyz()     | | ---- Llama a read_xyz()
    |   |  - write_xyz()    | |
    |   +-------------------+ |
    |           |             |
    +-----------|-------------+
                v
    +-------------------------+
    |    Hardware (Disco SDA) |
    +-------------------------+
    ```

-----

### **5. Llamada al sistema `mknod`: crear ficheros de dispositivo**

* **Uso:**
    `mknod` (make node) es una llamada al sistema de Unix/Linux (y también una herramienta de línea de comandos correspondiente) que se utiliza para crear un fichero especial, generalmente un fichero de dispositivo (de caracteres o de bloque) o una tubería con nombre (FIFO).
* **Parámetros al crear un fichero de dispositivo:**
    * `pathname`: la ruta y el nombre del fichero de dispositivo a crear (p. ej., `/dev/mydevice`).
    * `mode`: contiene el tipo de fichero (dispositivo de caracteres `S_IFCHR` o de bloque `S_IFBLK`) y los bits de permiso.
    * `dev`: un valor que combina el **número mayor** y el **número menor**, utilizado para especificar a qué controlador y a qué instancia de dispositivo concreta está asociado este fichero de dispositivo.
* **Funcionamiento interno:**
    1.  Crea un nuevo i-nodo en la ruta especificada.
    2.  Marca el tipo de fichero en el i-nodo (dispositivo de caracteres/bloque).
    3.  Almacena el `dev` (número mayor/menor) pasado como argumento en ese i-nodo.
* **Punto clave:**
    `mknod` por sí mismo normalmente **no interactúa directamente con el hardware** ni verifica si el número mayor/menor corresponde realmente a un dispositivo físico actualmente disponible con un controlador cargado. Simplemente crea un "punto de entrada" o "etiqueta" en el sistema de ficheros. Este fichero de dispositivo, a través de su número mayor/menor en el i-nodo, declara el tipo de dispositivo lógico que representa. La vinculación real con el controlador y el acceso al hardware ocurren en la operación `open` posterior.

-----

### **6. Flujo interno de la llamada al sistema `open` (para ficheros de dispositivo)**

Cuando un proceso de usuario llama a `open("/dev/mydevice", O_RDWR)` para abrir un fichero de dispositivo, el sistema operativo realiza internamente una serie de operaciones complejas, que también se resumen bien en el fichero `.md`:

1.  **Resolución de la ruta (Pathname Resolution):**
    * La capa del Sistema de Ficheros Virtual (VFS) resuelve la ruta `/dev/mydevice` desde el directorio raíz, buscando en el árbol de directorios hasta encontrar la entrada de directorio llamada `mydevice` y obtener su **número de i-nodo** correspondiente.

2.  **Carga y comprobación del i-nodo:**
    * Utilizando el número de i-nodo, lee la información completa del i-nodo de `mydevice` desde la tabla de i-nodos en el disco (o desde la caché de i-nodos en memoria).
    * Comprueba el tipo del i-nodo. Descubre que es un fichero de dispositivo (de caracteres o de bloque).
    * Comprueba si el proceso tiene permisos suficientes (según `O_RDWR` y los bits de permiso en el i-nodo) para abrir el dispositivo.

3.  **Obtención del número mayor/menor y asociación con el controlador:**
    * Extrae el **número mayor (Major Number)** almacenado en el i-nodo.
    * El núcleo del sistema operativo mantiene una lista de controladores registrados (generalmente un array o una tabla hash indexada por el número mayor). Usando este número mayor, el núcleo puede encontrar el **controlador de dispositivo** responsable de este tipo de dispositivos.
    * Si el controlador aún no está cargado en el núcleo, el sistema podría intentar cargarlo dinámicamente (dependiendo de la configuración y capacidad del sistema operativo).

4.  **Invocación de la rutina `open` del controlador:**
    * Una vez que se encuentra el controlador correcto, el núcleo invoca la función `open` registrada por ese controlador (normalmente a través de un puntero de función en la estructura `file_operations` mencionada anteriormente).
    * Pasa a la función `open` del controlador la información del i-nodo (o su número menor, Minor Number) y el modo de apertura (`flags`), entre otros datos.
    * La función `open` del controlador ejecuta operaciones de inicialización específicas del dispositivo, como:
        * Comprobar el estado del dispositivo (si está listo, si hay errores).
        * Asignar los recursos internos que el dispositivo necesita (como búferes).
        * Si el dispositivo es de uso exclusivo, marcarlo como ocupado.
        * Realizar un reinicio de hardware o establecer un modo específico.

5.  **Creación de estructuras de datos del núcleo para representar el fichero/dispositivo abierto:**
    * Si la operación `open` del controlador tiene éxito, el núcleo crea (o se asocia a) varias estructuras de datos importantes:
        * **Entrada en la tabla de ficheros abiertos a nivel de sistema:** El núcleo tiene una tabla global que registra todos los ficheros y dispositivos abiertos actualmente. Se crea una nueva entrada para esta operación `open`. Esta entrada contiene:
            * Un puntero al **v-nodo** correspondiente a este fichero de dispositivo en la capa del VFS (el v-nodo es la representación en memoria del i-nodo).
            * El puntero/desplazamiento de lectura/escritura actual (offset), que generalmente se inicializa a 0 para un fichero recién abierto.
            * El modo de apertura (`flags`).
            * Un contador de referencias, que registra cuántos descriptores de fichero a nivel de proceso apuntan a esta entrada.
        * **Entrada en la tabla de descriptores de fichero del proceso (Tabla de Canales):** Cada proceso tiene su propia tabla de descriptores de fichero. El núcleo encuentra el entero más pequeño no utilizado en esta tabla como **descriptor de fichero (file descriptor - fd)** y hace que este `fd` apunte a la nueva entrada creada en la tabla de ficheros abiertos a nivel de sistema.

6.  **Devolución del descriptor de fichero al proceso de usuario:**
    * La llamada al sistema `open` finalmente devuelve este descriptor de fichero entero (`fd`) al proceso de usuario.
    * El proceso de usuario utilizará este `fd` en operaciones posteriores como `read`, `write`, `close`, `ioctl`, etc., para referirse a este dispositivo ya abierto.

* **Diagrama: Flujo simplificado de la llamada al sistema `open`**
    ```
    Usuario: fd = open("/dev/sda", O_RDONLY);
       |
       v syscall
    Núcleo:
    1. VFS: Resolver "/dev/sda" -> i-nodo de /dev/sda
    2. I-nodo: Tipo=DispositivoDeBloque, Permisos=OK, Mayor=X, Menor=Y
    3. Buscar Driver para Número Mayor X
       +-----------------------+
       | Controlador de Disco  |
       |   - open_disk()       |
       |   - read_disk()       |
       |   - ...               |
       +-----------------------+
    4. Llamar a open_disk(Menor Y, flags) -> Éxito
    5. Crear entrada en Tabla Global de Ficheros Abiertos (TGF)
       TGF[n]: { v-nodo_sda, offset=0, flags=O_RDONLY, contador_ref=1 }
    6. Encontrar un fd libre en la Tabla de Descriptores del Proceso (TDP)
       TDP[fd_libre] -> TGF[n]
       |
       v syscall_return
    Usuario: Recibe fd_libre
    ```

-----

### **7. Flujo interno de la llamada al sistema `read` (para ficheros de dispositivo)**

Cuando un proceso de usuario, después de obtener un descriptor de fichero `fd` a través de `open`, llama a `read(fd, buffer, count)` para leer datos del dispositivo:

1.  **Validación de parámetros y conversión de fd:**
    * El núcleo primero verifica que `fd` sea un descriptor de fichero válido, que la zona de memoria apuntada por `buffer` sea escribible por el proceso de usuario y que `count` sea un valor razonable.
    * Utiliza `fd` para encontrar la entrada correspondiente en la tabla de descriptores de fichero del proceso actual, la cual apunta a una entrada en la tabla de ficheros abiertos a nivel de sistema.

2.  **Obtención de información del dispositivo y del controlador:**
    * Desde la entrada de la tabla de ficheros abiertos a nivel de sistema, el núcleo puede obtener:
        * El desplazamiento de lectura/escritura actual (offset).
        * Un puntero al v-nodo del fichero de dispositivo, y a través de él, acceso al i-nodo.
        * Del i-nodo, obtiene los números mayor/menor.
    * A través del número mayor, vuelve a localizar el controlador responsable de este dispositivo y su tabla de funciones de operación.

3.  **Invocación de la rutina `read` del controlador:**
    * El núcleo invoca la función `read` registrada por el controlador, pasándole la siguiente información:
        * El número menor (para distinguir la instancia específica del dispositivo).
        * La dirección del búfer proporcionado por el usuario (`buffer`).
        * El número de bytes a leer (`count`).
        * El desplazamiento de lectura/escritura actual (offset).
        * Un puntero a la entrada de la tabla de ficheros abiertos a nivel de sistema (o una estructura relacionada), para que el controlador pueda actualizar el desplazamiento o el estado.

4.  **Interacción del controlador con el hardware:**
    * La función `read` del controlador ejecuta las operaciones específicas del hardware para leer los datos:
        * Puede que necesite enviar comandos al controlador del dispositivo.
        * Esperar a que el dispositivo tenga los datos listos (posiblemente mediante sondeo o interrupciones).
        * Transferir los datos desde el hardware del dispositivo (por ejemplo, el búfer del disco, la FIFO del adaptador de red) a un búfer temporal en el espacio del núcleo.
        * Luego, **copiar** los datos desde el búfer temporal del núcleo al `buffer` proporcionado por el proceso de usuario.

5.  **Actualización del estado y retorno:**
    * El controlador (o el núcleo) actualiza el desplazamiento de lectura/escritura en la entrada de la tabla de ficheros abiertos a nivel de sistema, incrementándolo por el número de bytes realmente leídos.
    * La llamada al sistema `read` devuelve al proceso de usuario el número de bytes realmente leídos. Si se llega al final del fichero (lo cual tiene sentido para algunos dispositivos), puede devolver 0. Si ocurre un error, devuelve -1 y establece `errno`.

-----

### **8. El problema del bloqueo causado por las operaciones de E/S síncronas y una reflexión inicial**

* **E/S Síncrona (Synchronous I/O):**
    * En el modelo básico de `read` o `write` descrito anteriormente, cuando un proceso de usuario inicia una solicitud de E/S, ese proceso generalmente se **bloquea**, es decir, suspende su ejecución hasta que la operación de E/S se completa físicamente (los datos se leen del disco a la memoria, o los datos se escriben de la memoria a la red, etc.).
* **El problema:**
    * Muchos dispositivos de E/S (especialmente los discos duros mecánicos, las comunicaciones de red) son extremadamente lentos en comparación con la velocidad de la CPU.
    * Cuando un proceso se bloquea esperando una E/S, si el sistema operativo no puede utilizar eficazmente ese tiempo, la CPU permanecerá inactiva, lo que reduce el rendimiento general del sistema y la utilización de recursos.
    * En la clase se mencionó que este bloqueo ocurre mientras el proceso está en **modo núcleo (kernel mode)** ejecutando la llamada al sistema.
* **Ideas iniciales para la solución (anticipando el contenido de clases posteriores):**
    * La clase insinuó que la clave para resolver este problema es implementar alguna forma de **concurrencia**, permitiendo que la CPU, mientras espera que se complete una operación de E/S lenta, cambie para ejecutar otros procesos o hilos que estén listos.
    * Esto introduce los mecanismos de gestión de E/S más complejos que se discutirán en cursos posteriores (como la Clase 4), por ejemplo:
        * **E/S síncrona con gestor de dispositivos:** Delegar la operación de E/S real a hilos o procesos del núcleo especializados (gestores). El proceso de usuario sigue esperando el resultado, pero el núcleo puede planificar de manera más flexible.
        * **E/S asíncrona (Asynchronous I/O):** El proceso de usuario inicia la solicitud de E/S y regresa inmediatamente, sin tener que esperar a que la operación se complete. Puede continuar realizando otras tareas mientras la E/S se lleva a cabo y verificar el resultado más tarde.
    * En el fichero `.md` también se mencionó que una razón importante para introducir **hilos (threads)** es resolver el coste de la colaboración entre procesos y lograr una concurrencia más eficiente, lo cual está en línea con la idea de optimizar el manejo de la E/S.
