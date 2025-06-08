### **Clase 2: La evolución del sistema de ficheros – Directorios e I-nodos**

Esta clase traza la evolución de la forma en que los sistemas operativos organizan los ficheros. Desde las listas de ficheros muy simples de los primeros tiempos, hasta la necesidad de estructuras más complejas (como los directorios), y finalmente la introducción del concepto central del i-nodo para gestionar de manera eficiente grandes cantidades de ficheros y datos.

Los principales temas tratados en esta clase incluyen:

1.  Los primeros sistemas de ficheros y la gestión de metadatos.
2.  La necesidad de introducir directorios y su organización jerárquica.
3.  La evolución de la estructura de directorios: de una estructura de árbol a una estructura de grafo más flexible.
4.  Los problemas en la gestión de metadatos cuando múltiples enlaces apuntan al mismo fichero.
5.  La introducción del i-nodo (nodo de información) como solución y sus funciones.
6.  Fundamentos de la gestión de bloques de disco: la diferencia entre sectores (Sector) y bloques (Block).

-----

### **1. Primeros sistemas de ficheros y gestión de metadatos**

* **Contexto histórico:**
    En los primeros medios de almacenamiento (como los disquetes), la capacidad era muy limitada (generalmente unos pocos cientos de KB). [cite_start]Por lo tanto, el número de ficheros que se podían almacenar también era relativamente pequeño (por ejemplo, de 8 a 16 ficheros)[cite: 151].

* **Estructura simple:**
    En esa época, el sistema de ficheros solía ser una simple lista o una tabla plana. [cite_start]Para cada fichero en la lista, el sistema almacenaba la siguiente información básica[cite: 151]:

    * **Nombre de fichero:** Generalmente se usaban formatos de nombre de fichero cortos, como el clásico formato 8.3 (8 caracteres para el nombre principal y 3 para la extensión).
    * **Metadatos básicos:** Era la información esencial necesaria para acceder y gestionar el fichero, como:
        * Tipo de fichero (por ejemplo, fichero de texto, programa ejecutable).
        * Tamaño del fichero.
        * Fecha de creación o de última modificación del fichero.
        * Un puntero o dirección al primer bloque de datos del fichero en el disco.

* **Diagrama: Área de directorio/metadatos en un disquete antiguo**
    ```
    Área de directorio/metadatos en un disquete (lista simple):
    +-----------------+--------------------------------+---------------------+
    | Nombre.extensión| Información de metadatos       | Puntero inicio datos|
    +-----------------+--------------------------------+---------------------+
    | REPORT.TXT      | (Tipo:TXT, Tamaño:1KB,         | Bloque de disco 7   |
    |                 |  Fecha:10/01/1985)             |                     |
    +-----------------+--------------------------------+---------------------+
    | GAME.EXE        | (Tipo:EXE, Tamaño:50KB,        | Bloque de disco 15  |
    |                 |  Fecha:12/03/1985)             |                     |
    +-----------------+--------------------------------+---------------------+
    | NOTES.DOC       | (Tipo:DOC, Tamaño:5KB,         | Bloque de disco 66  |
    |                 |  Fecha:11/05/1985)             |                     |
    +-----------------+--------------------------------+---------------------+
    | ... (el número de entradas está limitado por el espacio disponible)     |
    +------------------------------------------------------------------------+
    ```
    * **Explicación del diagrama:**
        * Cada fila de esta "tabla" primitiva representa un fichero.
        * Contiene el nombre del fichero, algunos atributos clave (metadatos) y la ubicación inicial de los datos del fichero en el disco.
        * El espacio para almacenar estos metadatos era muy limitado.

-----

### **2. La necesidad de introducir directorios y su organización jerárquica**

* **El problema:**
    Con la aparición de los discos duros, la capacidad de almacenamiento aumentó drásticamente (pasando del nivel de KB al de MB). Esto permitió al sistema almacenar cientos o miles de ficheros. [cite_start]En esta situación, una simple lista plana de ficheros se volvió difícil de gestionar, tanto para el usuario (dificultad para encontrar ficheros) como para el sistema operativo (baja eficiencia de búsqueda)[cite: 151].

* **La solución: La aparición de los directorios (carpetas):**
    Para resolver este problema, se introdujo el concepto de **directorios** (también conocidos comúnmente como carpetas). [cite_start]Un directorio es en sí mismo un tipo especial de fichero, cuyo contenido es una lista de otros ficheros y/o directorios (llamados subdirectorios)[cite: 151].

* **Estructura inicial: Estructura jerárquica de árbol:**
    Los directorios se organizaron en una estructura jerárquica, similar a un árbol genealógico. [cite_start]Esta estructura comienza desde un **directorio raíz** de nivel superior (representado como `/` en sistemas Unix/Linux, y normalmente como `C:\` en sistemas DOS/Windows), y los demás directorios y ficheros "cuelgan" de este directorio raíz, formando múltiples niveles de anidamiento[cite: 151].

* **Diagrama: Estructura de directorios en árbol**
    ```
    Representación lógica del árbol de directorios:
        / (directorio raíz)
        |-- USERS/
        |   |-- ANNA/
        |   |   |-- REPORT.TXT (fichero)
        |   |   `-- PHOTO.JPG (fichero)
        |   `-- JOHN/
        |       `-- PROJECT.DOC (fichero)
        `-- SYSTEM/
            |-- KERNEL.BIN (fichero)
            `-- DRIVERS/
                `-- NET.SYS (fichero)

    Representación conceptual interna (simplificada y en evolución):

    Contenido del bloque de datos del directorio raíz ("/"):
    +--------------+------+-----------------------------------+
    | Nombre       | Tipo | Puntero a bloque de datos / Metadatos |
    +--------------+------+-----------------------------------+
    | "USERS"      | DIR  | Puntero al bloque de datos del dir. "USERS" |
    | "SYSTEM"     | DIR  | Puntero al bloque de datos del dir. "SYSTEM" |
    +--------------+------+-----------------------------------+

    Contenido del bloque de datos del directorio "USERS/" (apuntado desde el nivel superior):
    +--------------+------+-----------------------------------+
    | Nombre       | Tipo | Puntero a bloque de datos / Metadatos |
    +--------------+------+-----------------------------------+
    | "ANNA"       | DIR  | Puntero al bloque de datos del dir. "ANNA" |
    | "JOHN"       | DIR  | Puntero al bloque de datos del dir. "JOHN" |
    +--------------+------+-----------------------------------+

    Contenido del bloque de datos del directorio "ANNA/" (apuntado desde el nivel superior):
    +--------------+------+-----------------------------------+
    | Nombre       | Tipo | Puntero a bloque de datos / Metadatos |
    +--------------+------+-----------------------------------+
    | "REPORT.TXT" | FILE | (Contiene metadatos y puntero a datos) |
    | "PHOTO.JPG"  | FILE | (Contiene metadatos y puntero a datos) |
    +--------------+------+-----------------------------------+
    ```
    * **Explicación del diagrama:**
        * Cada directorio (que es en sí mismo un fichero especial) contiene varias entradas.
        * Si una entrada es un subdirectorio, su puntero apunta al bloque de disco que contiene las entradas de ese subdirectorio.
        * Si una entrada es un fichero normal, (en esta fase evolutiva) la entrada de directorio almacenaría los metadatos de ese fichero y un puntero a sus datos.

-----

### **3. La evolución de la estructura de directorios: De árbol a grafo**

* **Limitaciones de la estructura de árbol estricta:**
    Una estructura puramente de árbol implica que cada fichero o directorio (excepto el raíz) tiene un único directorio "padre". [cite_start]Esto dificulta compartir ficheros o subdirectorios entre diferentes ramas del sistema de ficheros, a menos que se copien (lo que conduce a la redundancia de datos)[cite: 152].
* **La necesidad de enlaces (Links):**
    Para organizar los ficheros de manera más flexible, surgió la necesidad de que un mismo fichero pudiera ser accedido a través de múltiples nombres o desde múltiples ubicaciones (por ejemplo, **enlaces duros**). [cite_start]Esta necesidad transformó la estructura de directorios de un simple árbol a un **grafo acíclico dirigido (DAG)** más general, e incluso puede contener ciclos con la permisión de mecanismos como los enlaces simbólicos[cite: 152].
* **Rutas absolutas demasiado largas:**
    Con el crecimiento exponencial de la capacidad de los discos (de GB a TB), las rutas absolutas de los ficheros (la ruta completa desde el directorio raíz) se volvieron muy largas, lo que incomodaba la gestión para los usuarios y el sistema. [cite_start]Aunque esto no está causado directamente por la diferencia entre estructuras de árbol y de grafo, una estructura de grafo (especialmente con la ayuda de i-nodos, como se verá más adelante) puede manejar las referencias de manera más eficiente[cite: 152].

-----

### **4. Problemas en la gestión de metadatos cuando múltiples enlaces apuntan al mismo fichero**

* **El dilema central:**
    [cite_start]Si el sistema de ficheros permite que múltiples nombres de fichero (posiblemente en diferentes directorios) apunten a los *exactamente mismos datos físicos* en el disco, ¿dónde deberían almacenarse los metadatos de esos datos (como el tamaño, los permisos, el propietario, la ubicación de los bloques de datos, etc.)? [cite: 153]

    * **Opción A (copiar los metadatos):** Si cada entrada de directorio que apunta a esos datos copia un conjunto completo de metadatos, surgirían problemas graves:
        * **Redundancia de datos:** Almacenar la misma información varias veces desperdiciaría espacio en disco.
        * **Inconsistencia de datos:** Si se modifican los metadatos a través de un nombre de fichero (por ejemplo, cambiando los permisos), al acceder a través de otros nombres de fichero se podrían ver los metadatos antiguos y no actualizados, lo que llevaría a un estado inconsistente del sistema.
        * **Gestión compleja de la eliminación:** Si se elimina un nombre de fichero, ¿deberían eliminarse los datos físicos? Si todavía hay otros nombres de fichero que apuntan a esos datos, no deberían eliminarse. ¿Cómo puede el sistema saberlo?

* **Diagrama: Escenario potencial de problemas de metadatos con enlaces duros sin i-nodos**
    ```
    Datos físicos del fichero en el disco: [ Bloque_de_datos_X ] [ Bloque_de_datos_Y ] ...

    Contenido del directorio /docs:
    +--------------+------------------------------------------------+
    | Nombre fichero | Metadatos (podrían duplicarse si hay enlaces)  |
    +--------------+------------------------------------------------+
    | "report.txt" | (Tamaño:10KB, Permisos:RW-, Punteros:[Bloque_X,Bloque_Y]) |
    +--------------+------------------------------------------------+

    Contenido del directorio /projects/current:
    +--------------+------------------------------------------------+
    | Nombre fichero | Metadatos (podrían duplicarse si hay enlaces)  |
    +--------------+------------------------------------------------+
    | "final.txt"  | (Tamaño:10KB, Permisos:RW-, Punteros:[Bloque_X,Bloque_Y]) | <-- Apunta a los mismos datos,
    +--------------+------------------------------------------------+       pero los metadatos están duplicados.
                                                                        Si los permisos se cambian a través de "report.txt",
                                                                        ¿se actualizarán aquí también?
    ```

-----

### **5. La introducción del i-nodo (nodo de información) como solución y sus funciones**

* **Idea central: Desacoplamiento (Decoupling)**
    [cite_start]La clave está en separar el **nombre del fichero** (que reside en la entrada de directorio) de la **información intrínseca del fichero** (es decir, sus metadatos y la ubicación de sus datos)[cite: 154].

* **Definición de i-nodo (nodo de información):**
    Para ello, se creó una nueva estructura de datos en el disco llamada **i-nodo**. [cite_start]Cada fichero (o directorio, ya que los directorios también se consideran ficheros especiales) en el sistema de ficheros tiene un i-nodo único correspondiente[cite: 155].

    * **Contenido almacenado en un i-nodo:** El i-nodo es responsable de almacenar todos los metadatos esenciales del fichero:
        * **Modo/Tipo de fichero:** Fichero normal, directorio, enlace simbólico, fichero de dispositivo de bloque, fichero de dispositivo de caracteres, etc.
        * **Permisos de acceso:** Permisos de lectura, escritura y ejecución para el propietario del fichero, el grupo y otros usuarios.
        * **Identificador de propietario (UID) e identificador de grupo (GID).**
        * **Tamaño del fichero (en bytes).**
        * **Marcas de tiempo (timestamps):** Hora del último acceso, hora de la última modificación, hora del último cambio del i-nodo.
        * **Contador de enlaces (Link Count):** El número de nombres de fichero (entradas de directorio) que apuntan a este i-nodo. Este contador es crucial para la implementación de enlaces duros y se utiliza para determinar cuándo se pueden liberar de forma segura los bloques de datos asociados a un fichero.
        * [cite_start]**Punteros a bloques de datos:** Información que describe la ubicación específica de los datos del fichero en el disco (como se discutió en la Clase 1: asignación contigua, asignación indexada, asignación indexada multinivel, etc.)[cite: 158].
    * **Ubicación de almacenamiento de los i-nodos:** Los i-nodos suelen almacenarse en un área específica del disco, a menudo llamada tabla de i-nodos. Cada i-nodo tiene un número único (número de i-nodo) que lo identifica.

* **La evolución de las entradas de directorio:**
    Con la introducción de los i-nodos, el contenido de una entrada de directorio se simplificó enormemente. Ahora, una entrada de directorio contiene principalmente:

    * **El nombre del fichero.**
    * [cite_start]**El número de i-nodo asociado a ese nombre de fichero**[cite: 154].

* **Diagrama: Cómo los i-nodos resuelven el problema de los enlaces**
    ```
    Área de i-nodos en el disco:
    +--------------------------------------------------------------------+
    | i-nodo número 75:                                                  |
    |   Tipo: Fichero normal, Permisos: RW-R--R--, Propietario: Anna, Grupo: Users |
    |   Tamaño: 2048 bytes, Contador de enlaces: 2                       |
    |   Timestamps: ..., Punteros a bloques de datos: [Bloque 100, Bloque 101, Bloque 102] |
    +--------------------------------------------------------------------+
    | i-nodo número 76: ...                                              |
    | ...                                                                |
    +--------------------------------------------------------------------+

    Contenido del directorio /home/anna:
    +-------------------+--------------+
    | Nombre de fichero   | Número de i-nodo |
    +-------------------+--------------+
    | "my_document.txt" |     75       |  <--- Apunta al i-nodo 75
    +-------------------+--------------+
    | "photo.png"       |     88       |
    +-------------------+--------------+

    Contenido del directorio /backups:
    +-------------------+--------------+
    | Nombre de fichero   | Número de i-nodo |
    +-------------------+--------------+
    | "important_doc"   |     75       |  <--- También apunta al i-nodo 75 (esto es un enlace duro)
    +-------------------+--------------+
    ```
    * **Explicación del diagrama:**
        * Los nombres de fichero "my_document.txt" e "important_doc" apuntan al mismo i-nodo (número 75).
        * Todos los metadatos están centralizados en el i-nodo 75. Por lo tanto, si se modifican los permisos a través de un nombre de fichero, el cambio será visible al acceder a través del otro nombre, ya que comparten el mismo i-nodo.
        * El "contador de enlaces" del i-nodo 75 será 2. Si se elimina "my_document.txt", el contador de enlaces se reducirá a 1, pero el i-nodo y sus bloques de datos no se eliminarán. Solo cuando "important_doc" también se elimine y el contador de enlaces llegue a 0, el sistema operativo sabrá que puede recuperar de forma segura ese i-nodo y los bloques de datos que ocupa.

-----

### **6. Fundamentos de la gestión de bloques de disco: La diferencia entre sectores (Sector) y bloques (Block)**

* **La unidad física del disco:**

    * **Sector:** La unidad de datos más pequeña que el hardware del disco puede leer o escribir. Tradicionalmente era de 512 bytes, aunque los discos modernos también utilizan sectores de 4 KB (lo que se conoce como Formato Avanzado). [cite_start]El sistema operativo no puede leer solo medio sector[cite: 156].
    * **Bloque:** La unidad lógica más pequeña que el sistema operativo utiliza para asignar espacio de almacenamiento para los ficheros. Un bloque consta de uno o más sectores físicos contiguos. [cite_start]Los tamaños de bloque comunes son 1 KB, 2 KB, 4 KB, 8 KB, etc.[cite: 156].

* **¿Por qué usar bloques en lugar de directamente sectores?**

    * **Reducir la sobrecarga de gestión:** Gestionar el espacio en disco en unidades más grandes (bloques) reduce la cantidad de punteros y metadatos que el sistema operativo necesita rastrear y gestionar.
    * **Eficiencia de E/S:** Leer/escribir un bloque grande de una vez puede ser más eficiente que leer/escribir múltiples sectores más pequeños, aunque esto depende de la carga de trabajo específica.

* **La elección del tamaño del bloque y la fragmentación interna:**

    * En clase se mencionó que la práctica más común es usar **bloques de tamaño fijo que no se comparten entre sectores** (es decir, un bloque lógico está compuesto completamente por N sectores físicos, y diferentes bloques lógicos no comparten el mismo sector físico).
    * **Fragmentación interna:** Si el tamaño de un fichero no es un múltiplo entero del tamaño del bloque, se desperdiciará una parte del espacio en el último bloque asignado a ese fichero. [cite_start]Por ejemplo, si el tamaño del bloque es de 4 KB y un fichero es de 5 KB, necesitará dos bloques (un total de 8 KB asignados), y en el segundo bloque habrá 3 KB de espacio no utilizado por ese fichero pero que ya está ocupado (esto es la fragmentación interna)[cite: 157]. En clase se señaló que, dada la simplicidad y el rendimiento de este método, este nivel de fragmentación interna suele ser aceptable.

* **Diagrama: Relación entre sectores y bloques**
    ```
    Disco físico (el hardware ve sectores):
    |Sector0|Sector1|Sector2|Sector3|Sector4|Sector5|Sector6|Sector7|Sector8|Sector9|Sec10|Sec11| ... (suponiendo 512 bytes por sector)
    +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+----+----+

    Sistema operativo (gestiona bloques lógicos, suponiendo: 1 bloque lógico = 4 sectores físicos = 2 KB):
    +-----------------------+-----------------------+-----------------------+
    |      Bloque lógico 0    |      Bloque lógico 1    |      Bloque lógico 2    | ...
    | (Sec0+Sec1+Sec2+Sec3) | (Sec4+Sec5+Sec6+Sec7) | (Sec8+Sec9+Sec10+Sec11) |
    +-----------------------+-----------------------+-----------------------+

    Fichero "data.txt" (tamaño 5 KB):
    Si el tamaño del bloque es 2 KB, necesitará 3 bloques lógicos.
    Bloque lógico 0: Ocupado completamente por "data.txt" (almacena 2 KB de datos)
    Bloque lógico 1: Ocupado completamente por "data.txt" (almacena 2 KB de datos)
    Bloque lógico 2: Almacena los 1 KB restantes de "data.txt" + 1 KB de espacio no utilizado (esto es la fragmentación interna).
    ```

