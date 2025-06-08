
### **Clase 5: Arquitectura en Capas del Sistema de Ficheros**

Esta clase ofrece una **visión de la arquitectura en capas** de cómo funcionan los sistemas de ficheros modernos, integrando varios conceptos discutidos en clases anteriores, como la independencia de dispositivo, los i-nodos, el VFS (Sistema de Ficheros Virtual), el soporte para diferentes tipos de sistemas de ficheros y la optimización del rendimiento mediante mecanismos de caché. Esto coincide en gran medida con las secciones de "Implementación" y "Optimizaciones" del fichero `tema5.pdf`, y también se ve corroborado y complementado por las partes relevantes del fichero `SOA_Segundo_Parcial (1).md`.

El objetivo de esta clase es explicar cómo un sistema de ficheros moderno logra sus complejas funcionalidades a través de una ingeniosa estructura multicapa, incluyendo la abstracción de las diferencias de hardware, el soporte a múltiples formatos de sistemas de ficheros, la adición de funcionalidades lógicas como las tuberías (pipes) y la superación de la latencia de acceso a dispositivos físicos (como los discos).

Los principales temas discutidos en esta clase incluyen:

1.  **Los objetivos de diseño principales de los sistemas de ficheros modernos.**
2.  **La arquitectura en capas del sistema de ficheros:**
    * Capa del Sistema de Ficheros Virtual (VFS) / Sistema de Ficheros Lógico.
    * Capa de controladores de sistemas de ficheros específicos (Drivers de Sistemas de Ficheros Específicos - SFS).
    * Capa de caché de búfer (Buffer Cache / Page Cache).
    * Capa de controladores de dispositivo (Drivers de Dispositivos Físicos / Device Drivers).
3.  **El flujo y la transformación de los datos entre las distintas capas (por ejemplo, bloques de datos vs. sectores).**
4.  **La posición y el rol de los gestores de dispositivos (Gestores) en esta arquitectura.**

-----

### **1. Objetivos de diseño principales de los sistemas de ficheros modernos**

Antes de profundizar en la arquitectura, el profesor repasó los objetivos clave que un sistema de ficheros moderno debe cumplir, para entender por qué se adopta un diseño en capas:

* **Abstraer dispositivos:** Ocultar los detalles de implementación de los dispositivos físicos (como diferentes modelos de discos duros, SSD) y lógicos (como las particiones).
* **Proporcionar una interfaz unificada:** Los usuarios y las aplicaciones interactúan con el sistema de ficheros a través de un conjunto estándar de llamadas al sistema, sin necesidad de preocuparse por la implementación subyacente concreta.
* **Soportar múltiples formatos de sistemas de ficheros:** Ser capaz de montar y operar simultáneamente con diferentes tipos de sistemas de ficheros (como ext4 en Linux, NTFS de Windows, FAT32 en una unidad USB, ISO9660 en un CD, etc.).
* **Implementar funcionalidades especiales:** Soportar abstracciones que no son ficheros de disco tradicionales, como las tuberías (pipes), los ficheros de socket y los ficheros de dispositivo especiales (`/dev/null`, `/dev/random`).
* **Optimización del rendimiento:** Superar la latencia de E/S de los dispositivos de almacenamiento como los discos, mejorando la velocidad de acceso mediante técnicas como el almacenamiento en caché.
* **Fiabilidad y consistencia:** Asegurar el almacenamiento persistente de los datos y la capacidad de recuperación en caso de fallo (por ejemplo, mediante journaling).
* **Control de concurrencia y seguridad:** Gestionar el acceso concurrente de múltiples procesos al sistema de ficheros y aplicar las comprobaciones de permisos.

-----

### **2. La arquitectura en capas del sistema de ficheros**

Para implementar de forma modular los objetivos anteriores, el sistema operativo descompone la funcionalidad del sistema de ficheros en diferentes capas, cada una con responsabilidades claras y comunicándose con las capas superior e inferior a través de interfaces bien definidas. Este diseño mejora la escalabilidad (facilita la adición de nuevos tipos de sistemas de ficheros o controladores de dispositivo) y la mantenibilidad del sistema.

* **Diagrama: Modelo genérico en capas del sistema de ficheros**

    ```
    +-----------------------------------------------------+
    |             Aplicaciones de Usuario                 |
    |        (usando llamadas al sistema como open, read, write) |
    +-------------------------+---------------------------+
                              | Llamadas al Sistema (Syscalls)
                              v
    +-------------------------+---------------------------+
    | Capa 1:                 Virtual File System (VFS)   | Interfaz Unificada
    |                         (Sistema de Ficheros Lógico)| (v-nodes, d-cache, i-cache)
    +-------------------------+---------------------------+
                              | Interfaz del VFS (operaciones genéricas)
                              v
    +-------------------------+---------------------------+
    | Capa 2:                 Drivers de Sistemas de      | Implementaciones Específicas
    |                         Ficheros Específicos (SFS)  | (ext4, NTFS, FAT32, pipes, etc.)
    |                         (ej: módulo ext4.ko)        |
    +-------------------------+---------------------------+
                              | Peticiones de Bloques Lógicos
                              v
    +-------------------------+---------------------------+
    | Capa 3:                 Buffer Cache / Page Cache   | Optimización de Rendimiento
    |                         (Caché de Bloques/Páginas)  |
    +-------------------------+---------------------------+
                              | Peticiones de Bloques Físicos (Sectores)
                              v
    +-------------------------+---------------------------+
    | Capa 4:                 Drivers de Dispositivos     | Interacción con Hardware
    |                         Físicos (Device Drivers)    | (Controladores de Disco/SSD)
    |                         (ej: driver AHCI/NVMe)      |
    +-------------------------+---------------------------+
                              | Comandos de Hardware
                              v
    +-------------------------+---------------------------+
    |                Dispositivos Físicos de E/S          |
    |                (Disco Duro, SSD, Red, etc.)         |
    +-----------------------------------------------------+
    ```

Ahora analizamos cada capa en detalle:

* **a. Capa 1: Virtual File System (VFS) / Sistema de Ficheros Lógico**

    * **Rol y funcionalidad:**
        * Es la interfaz de más alto nivel con la que las aplicaciones de usuario interactúan directamente a través de las llamadas al sistema.
        * La tarea principal del VFS es proporcionar un **modelo de sistema de ficheros unificado y abstracto** a las aplicaciones de nivel superior, ocultando por completo las diferencias de los formatos de sistema de ficheros subyacentes. Por ejemplo, en Linux, independientemente de si estás operando con un fichero en una partición ext4, un fichero en una unidad USB FAT32 o un fichero en un sistema de ficheros de red (NFS), utilizas las mismas llamadas al sistema `open()`, `read()`, `write()`, etc., y ves una estructura de directorios, un modelo de permisos, etc., consistentes.
        * Es responsable de resolver las rutas, realizar las comprobaciones de permisos y traducir las operaciones de fichero genéricas (como "leer el N-ésimo byte de un fichero") en llamadas a los controladores de sistema de ficheros específicos de la capa inferior.
    * **Estructuras de datos clave (complementado según el fichero `.md`):**
        * **V-node (nodo virtual / i-nodo virtual):** El VFS mantiene un v-nodo para cada fichero o directorio activo en memoria (es decir, que ha sido abierto o accedido recientemente). El v-nodo es una representación genérica en memoria del i-nodo del disco (u otra estructura de metadatos específica del sistema de ficheros). Contiene información genérica e independiente del sistema de ficheros (como el tipo de fichero, un concepto aproximado del tamaño, contador de referencias, etc.) y un conjunto de punteros a las funciones de operación estándar proporcionadas por el sistema de ficheros específico (por ejemplo, para un fichero ext4, estos punteros apuntarían a las funciones `read`, `write`, etc., implementadas en el módulo del controlador ext4).
        * **Tabla de descriptores de fichero del proceso (Tabla de Canales):** Propia de cada proceso.
        * **Tabla de ficheros abiertos del sistema:** Global del sistema, registra todos los ficheros abiertos.
        * **D-cache (Directory Entry Cache / Caché de Entradas de Directorio):** Almacena en caché los resultados de la resolución de rutas a i-nodos/v-nodos para acelerar búsquedas posteriores de la misma ruta o sus componentes.
        * **I-cache (Inode Cache):** Almacena en caché los i-nodos activos (o su representación en el VFS, los v-nodos) para reducir el coste de leer repetidamente los i-nodos desde el disco.
    * **Interfaz:** El VFS define un conjunto de interfaces de operación de ficheros estándar (como `vfs_read`, `vfs_write`) que los controladores de sistemas de ficheros específicos deben implementar.

* **b. Capa 2: Controladores de Sistemas de Ficheros Específicos (SFS)**

    * **Rol y funcionalidad:**
        * Se encuentra debajo del VFS y contiene módulos de controladores independientes para **cada tipo de sistema de ficheros específico** (por ejemplo, en el núcleo de Linux habría módulos como `ext4.ko`, `ntfs3.ko`, `vfat.ko`, etc.).
        * También incluye módulos para manejar **dispositivos lógicos o ficheros especiales** que, aunque se presentan como objetos del sistema de ficheros, no se almacenan en particiones de disco tradicionales, como las **tuberías (pipes)**, los ficheros de socket y los ficheros de dispositivo especiales como `/dev/null` y `/dev/random`.
        * Estos controladores son el código que realmente "entiende" la **disposición física en disco y las estructuras de metadatos** de su sistema de ficheros correspondiente (como el superbloque, los descriptores de grupo de bloques, la tabla de i-nodos y el formato de las entradas de directorio de ext4; o el sector de arranque, la tabla FAT y la estructura de las entradas de directorio de FAT) y cómo ejecutar operaciones de fichero específicas (como buscar un nombre de fichero en un directorio, analizar un i-nodo para localizar bloques de datos, asignar nuevos bloques de datos, gestionar los mapas de bits de espacio libre, etc.).
    * **Interacción:** Cuando el VFS necesita realizar una operación en un fichero específico (por ejemplo, el usuario llama a `read` y el VFS determina que el fichero se encuentra en una partición ext4), el VFS, a través del puntero de función guardado en el v-nodo de ese fichero, invoca la función `read` correspondiente proporcionada por el módulo del controlador ext4, pasándole el v-nodo (o el puntero al i-nodo específico de ext4 que contiene) y los parámetros de la operación (como el desplazamiento y el número de bytes).

* **c. Capa 3: Caché de Búfer / Caché de Páginas (Buffer Cache / Page Cache)**

    * **Propósito y principio:**
        * Las operaciones de E/S de disco (especialmente la latencia de búsqueda y rotación de los discos duros mecánicos) son muy lentas en comparación con la velocidad de la CPU y la memoria. Para salvar esta brecha de velocidad y mejorar significativamente el rendimiento del sistema de ficheros, el sistema operativo introduce una **caché de búfer** entre la capa de controladores de sistemas de ficheros específicos y la capa de controladores de dispositivos de bloque de nivel inferior.
        * Mantiene en la memoria física los **bloques de disco (blocks)** leídos o escritos recientemente (o, en sistemas más modernos, se gestiona en unidades de **páginas de memoria (pages)**, llamándose Caché de Páginas, que puede almacenar tanto datos de ficheros como metadatos del sistema de ficheros).
    * **Flujo de operación:**
        * Cuando un controlador de sistema de ficheros específico (SFS) necesita leer un bloque de disco, primero solicita ese bloque a la caché de búfer.
            * **Acierto de caché (Cache Hit):** Si el bloque ya existe en la caché (es decir, fue leído anteriormente y aún no ha sido reemplazado), los datos se obtienen directamente de la rápida memoria física, evitando un lento acceso físico al disco.
            * **Fallo de caché (Cache Miss):** Si el bloque no está en la caché, la caché de búfer se encarga de leer ese bloque desde el dispositivo físico (disco) a través del controlador de dispositivo de la capa inferior, lo almacena en la caché (posiblemente reemplazando algún bloque antiguo y menos utilizado recientemente, según un algoritmo de reemplazo como LRU), y luego proporciona los datos al SFS que lo solicitó.
        * Las operaciones de escritura también pueden optimizarse a través de la caché:
            * **Escritura directa (Write-through):** Los datos se escriben simultáneamente en la caché y en el disco. Garantiza la persistencia inmediata de los datos, pero es más lento.
            * **Escritura diferida (Write-back):** Los datos se escriben primero solo en la caché (marcando el bloque/página como "sucio"). El mecanismo de gestión de la caché escribirá estos bloques/páginas sucios en el disco de forma asíncrona en un momento adecuado posterior (como cuando la caché está llena, en un refresco periódico, al cerrar un fichero o al desmontar el sistema). Esto mejora el rendimiento de la escritura, pero conlleva un riesgo de pérdida de datos (si el sistema falla antes de la escritura en el disco).
    * **Beneficios:** La caché de búfer aprovecha enormemente el **principio de localidad del acceso** (localidad temporal: los datos accedidos recientemente es probable que se vuelvan a acceder pronto; localidad espacial: después de acceder a un bloque de datos, es probable que se accedan los bloques cercanos). Incluso si el tamaño de la caché es mucho menor que la capacidad total del disco, puede alcanzar una tasa de aciertos muy alta (por ejemplo, el profesor mencionó que incluso una caché de unos pocos MB para un disco de TB podría alcanzar una tasa de aciertos del 98%), mejorando así significativamente el rendimiento general de la E/S.

* **d. Capa 4: Controladores de Dispositivos Físicos (Device Drivers / Controladores de Dispositivos de Bloque)**

    * **Rol y funcionalidad:**
        * Es el software de más bajo nivel que se comunica directamente con los **dispositivos de almacenamiento físico** (como los controladores de disco duro como AHCI SATA, los controladores de SSD como NVMe, los controladores de unidades USB, etc.).
        * La caché de búfer (en caso de un fallo de caché) o los controladores de sistemas de ficheros específicos de nivel superior (si necesitan omitir la caché o realizar operaciones de control de dispositivo específicas, como formatear) emiten los comandos reales de lectura/escritura al dispositivo físico a través de los controladores de esta capa.
    * **Interacción:**
        * Estos controladores son responsables de traducir las peticiones de alto nivel que reciben (generalmente dirigidas a una **Dirección de Bloque Lógico (LBA)** o a un sector específico) en comandos de hardware concretos que el dispositivo físico puede entender (por ejemplo, construyendo un paquete de comandos ATAPI para SATA, o una entrada en la cola de comandos de NVMe).
        * También se encargan de gestionar la transferencia de datos con el dispositivo (como configurar una transferencia DMA), de manejar las interrupciones de hardware del dispositivo (por ejemplo, cuando se completa una operación de lectura/escritura) y de informar y gestionar los errores del dispositivo.

-----

### **3. Flujo y transformación de datos entre las capas**

* **De la petición de usuario al VFS:** Un programa de usuario, a través de funciones de la biblioteca estándar C (como `fopen`, `fread`), invoca indirectamente las llamadas al sistema del núcleo, como `open()`, `read()`. Estas llamadas al sistema llegan primero al VFS.
* **Del VFS al SFS:** El VFS resuelve la ruta, encuentra el v-nodo correspondiente y, a través de los punteros de función de operación en el v-nodo, invoca la función correspondiente del controlador de sistema de ficheros específico (como `ext4_read_inode`, `ext4_file_read_iter` de ext4).
* **Del SFS a la Caché de Búfer (Unidad: Bloque de Datos):**
    * El SFS (como ext4) conoce la estructura lógica del fichero, por ejemplo, de qué **bloques de datos** se compone lógicamente un fichero (estos números de bloque son relativos a esa partición del sistema de ficheros y pueden localizarse a través de los punteros directos/indirectos en el i-nodo).
    * Cuando el SFS necesita leer una parte de un fichero, calcula qué bloques de datos lógicos necesita y solicita esos **bloques de datos** a la caché de búfer.
    * El fichero `.md` enfatiza: la unidad mínima de transferencia entre el sistema operativo/sistema de ficheros y el dispositivo (a través de la Caché de Búfer) es el **bloque de datos**. El i-nodo también apunta a bloques de datos.
* **De la Caché de Búfer al Controlador de Dispositivo (Unidad: Sectores):**
    * Si ocurre un fallo de caché, la Caché de Búfer necesita leer los datos desde el dispositivo físico.
    * El dispositivo físico (como un disco) opera en unidades de **sectores**.
    * Por lo tanto, dentro de la Caché de Búfer, debe realizarse una **transformación**: convertir el **número de bloque de datos** lógico solicitado por el SFS (y el desplazamiento dentro del bloque) en el **número de sector** (o LBA) específico y la cantidad en el dispositivo físico. Esta conversión requiere conocer el tamaño del bloque, el sector de inicio de la partición en el disco, etc.
    * Luego, la Caché de Búfer emite un comando de lectura para esos sectores al dispositivo físico a través del controlador de dispositivo.
* **Del Controlador de Dispositivo al Dispositivo Físico:** El controlador de dispositivo traduce la solicitud de lectura/escritura de sectores en comandos de hardware.

-----

### **4. Posición y rol de los Gestores de Dispositivos en esta arquitectura**

Como se discutió en la Clase 4, los gestores de dispositivos se utilizan para manejar la concurrencia de las solicitudes de E/S, su encolamiento y la sincronización con los hilos de usuario, para superar la alta latencia de la E/S física. En esta arquitectura en capas, la posición ideal de un gestor de dispositivos es:

* **Generalmente entre la Caché de Búfer y los controladores de dispositivos físicos de la capa inferior, o como parte de la gestión de solicitudes de disco dentro de la propia Caché de Búfer.**
* **Razón:**
    * Cuando ocurre un fallo de caché y es necesario realizar una E/S física real, esta operación consume tiempo.
    * Si la Caché de Búfer (o el hilo del núcleo que ejecuta el acceso al disco en su nombre) se bloqueara directamente esperando a que se complete la E/S física, otras solicitudes de caché que podrían ser procesadas (ya sea del mismo proceso o de procesos diferentes) u otras actividades del núcleo podrían retrasarse innecesariamente.
    * En el fichero `.md` se menciona: "Como la buffer cache puede recibir peticiones secuencialmente (...) mientras aún no ha terminado una operación de larga latencia, será necesario tener un gestor para administrar estas **peticiones pendientes**." Esto significa que, aunque en un momento dado solo haya un flujo de ejecución en modo núcleo operando en la Caché de Búfer, debido a la naturaleza asíncrona y la larga latencia de la E/S física, la Caché de Búfer puede acumular múltiples solicitudes de disco pendientes.
    * Por lo tanto, un **gestor de dispositivos** (o planificador de E/S) en este punto puede:
        * Gestionar una **cola de solicitudes de E/S físicas pendientes**.
        * Ordenar y optimizar estas solicitudes (por ejemplo, el algoritmo del ascensor para discos, para minimizar el movimiento del cabezal).
        * Enviar las solicitudes de forma asíncrona al controlador de dispositivo.
        * Manejar las interrupciones del controlador de dispositivo, marcar las solicitudes completadas y despertar a los procesos o a la lógica interna de la Caché de Búfer que pudieran estar esperando esos datos.
        * De esta manera, la Caché de Búfer puede "descargar" las operaciones de E/S físicas al gestor y, posiblemente, continuar procesando otras operaciones de caché que no dependan de ese acceso a disco específico (por ejemplo, atender otras solicitudes que resulten ser aciertos de caché).
