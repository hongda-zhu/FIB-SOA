### **Clase 1: Estrategias de Asignación de Bloques de Datos de Ficheros**

El núcleo de esta clase es discutir cómo los sistemas operativos gestionan el espacio en disco, específicamente cómo se almacena el contenido de los datos de un fichero (organizado en bloques de datos) en el disco, y qué información necesita contener el i-nodo (o una estructura de metadatos similar) para encontrar estos bloques que pertenecen a un fichero específico. Un principio fundamental es que un bloque de datos, en un mismo instante, solo puede pertenecer a un fichero.

El profesor explicó varios métodos principales de asignación de bloques de datos:

1.  **Asignación Contigua**
2.  **Asignación Encadenada**
3.  **Asignación Encadenada con Tabla de Asignación de Ficheros (FAT)**
4.  **Asignación Indexada**
5.  **Asignación Indexada Multinivel**

Analicemos cada uno en detalle:

-----

### **1. Asignación Contigua**

* **Principio:**
    El sistema asigna a un fichero un conjunto de bloques de datos que son contiguos en la dirección física del disco. Todos los datos del fichero se almacenan en estos bloques adyacentes.

* **Información registrada en el i-nodo (o en la entrada de directorio):**
    * **Número de bloque inicial (Start_Block / Primer bloque):** La dirección/número en el disco del primer bloque de datos que ocupa el fichero.
    * **Longitud/Número de bloques (Length / Número de bloques):** El número total de bloques de datos que ocupa el fichero, o el tamaño total en bytes del fichero (que se puede convertir a número de bloques).

* **Diagrama:**
    ```
    I-nodo / Directorio:
    +-------------------------------------+
    | Nombre del fichero (Nombre)         |
    | Bloque inicial (Start_Block): 7     |
    | Longitud/Nº bloques (Length): 3 bloques |
    | Otros metadatos...                  |
    +-------------------------------------+

    Disco:
    ... [ B4 ] [ B5 ] [ B6 ] [ D1 (B7) ] [ D2 (B8) ] [ D3 (B9) ] [ B10 ] [ B11 ] ...
                           /           /           /
                          /           /           /
    Fichero "ejemplo.txt" -> Bloque 7    Bloque 8    Bloque 9
                             (Datos)      (Datos)      (Datos)
    ```
    * **Explicación:**
        * Se asume que los datos del fichero "ejemplo.txt" se almacenan en 3 bloques contiguos a partir del bloque de disco 7 (B7, B8, B9).
        * El i-nodo registra el bloque inicial 7 y la longitud 3.

* **Modo de acceso:**
    * **Acceso secuencial:** Muy eficiente. Una vez conocido el primer bloque, las direcciones de los bloques posteriores son consecutivas (número de bloque actual + 1).
    * **Acceso aleatorio:** Muy eficiente. Para acceder a datos en un desplazamiento de bytes específico dentro del fichero, se puede calcular directamente la dirección del bloque de datos objetivo mediante `Bloque_Inicial + (desplazamiento / tamaño_del_bloque)`, y luego sumar el desplazamiento dentro del bloque.

* **Ventajas:**
    * Velocidad de acceso rápida, tanto para lectura/escritura secuencial como aleatoria, ya que los bloques del fichero están físicamente concentrados, reduciendo el tiempo de búsqueda del disco (seek time).
    * Implementación sencilla.

* **Desventajas:**
    * **Dificultad para el crecimiento de ficheros:** Si un fichero necesita crecer después de su creación pero no hay suficiente espacio contiguo libre a continuación, es necesario mover el fichero completo a otra área contigua lo suficientemente grande, lo cual es muy costoso en tiempo.
    * **Fragmentación externa:** Se generan muchos pequeños bloques libres no contiguos en el disco, que pueden no ser suficientes para las necesidades de almacenamiento contiguo de ficheros más grandes, aunque el espacio libre total sea suficiente.
    * **Necesidad de conocer el tamaño del fichero de antemano:** Para evitar los problemas anteriores, normalmente es necesario asignar el espacio máximo que el fichero podría necesitar en el momento de su creación, lo que no es práctico en muchos casos.

* **Casos de uso:**
    * Se utiliza principalmente en escenarios donde el tamaño del fichero es fijo y no cambia después de la creación, como en sistemas de ficheros para CD-ROM, DVD o Blu-ray (por ejemplo, el estándar ISO 9660). En estos medios, los ficheros no se modifican una vez escritos.

-----

### **2. Asignación Encadenada**

* **Principio:**
    Los diversos bloques de datos de un fichero pueden almacenarse de forma dispersa en cualquier posición libre del disco. Cada bloque de datos, además de almacenar los datos del fichero, contiene un puntero a la dirección física del siguiente bloque de datos del mismo fichero. El puntero en el último bloque de datos del fichero suele apuntar a una marca especial (como NULL o -1) para indicar el final del fichero.

* **Información registrada en el i-nodo (o en la entrada de directorio):**
    * **Número de bloque inicial (Start_Block / Primer bloque):** La dirección/número en disco del primer bloque de datos del fichero.
    * **Tamaño del fichero (File_Size / Tamaño):** Se utiliza para determinar el final del fichero (aunque también se puede determinar por el puntero final).

* **Diagrama:**
    ```
    I-nodo / Directorio:
    +-------------------------------------+
    | Nombre del fichero (Nombre)         |
    | Bloque inicial (Start_Block): 9     |
    | Tamaño del fichero (File_Size)      |
    | Otros metadatos...                  |
    +-------------------------------------+

    Disco:
    ... [ B7 ] [ B8 ] [ D1 (B9) | Ptr -> B12 ] ... [ B11 ] [ D2 (B12) | Ptr -> B15 ] ...
                           ^                                  ^
                           |                                  |
    Fichero "ejemplo.txt" -----+                                  +----- (segundo bloque)

    ... [ B14 ] [ D3 (B15) | NULL/-1 ] ...
                             ^
                             |
                             +------------------------------------ (tercer y último bloque)
    ```
    * **Explicación:**
        * El primer bloque de datos del fichero "ejemplo.txt" es el bloque de disco 9 (D1).
        * D1 almacena datos y tiene un puntero al siguiente bloque de datos, que es el bloque 12 (D2).
        * D2 almacena datos y tiene un puntero al siguiente bloque de datos, el bloque 15 (D3).
        * D3 almacena datos y su puntero es NULL, indicando el final del fichero.
        * Los bloques de datos B9, B12 y B15 pueden estar dispersos en el disco.

* **Modo de acceso:**
    * **Acceso secuencial:** Bueno. Se encuentra el primer bloque a partir del i-nodo y luego se leen los bloques siguientes siguiendo los punteros de cada bloque.
    * **Acceso aleatorio:** Muy malo. Para acceder a un bloque de datos intermedio (por ejemplo, el bloque `k`), es necesario empezar desde el primer bloque y seguir la cadena de punteros, accediendo secuencialmente a los `k-1` bloques anteriores para encontrar el bloque `k`. Esto implica una gran cantidad de operaciones de búsqueda y lectura en el disco.

* **Ventajas:**
    * **Asignación dinámica:** Los ficheros pueden crecer (añadiendo nuevos bloques al final de la cadena) o reducirse (liberando bloques y modificando punteros) fácilmente.
    * **Sin fragmentación externa:** Se puede utilizar cualquier bloque de disco libre.

* **Desventajas:**
    * **Rendimiento del acceso aleatorio extremadamente pobre.**
    * **Espacio ocupado por los punteros:** Cada bloque de datos necesita una parte de su espacio para almacenar el puntero, lo que reduce el espacio real para almacenar datos.
    * **Problemas de fiabilidad:** Si algún puntero de la cadena se daña o se pierde, todos los bloques de datos posteriores se vuelven inaccesibles.

* **Aplicaciones:**
    * Utilizado en algunos sistemas de ficheros antiguos, como MS-DOS 2.0. Debido a los problemas de rendimiento del acceso aleatorio, ya no es común como método de asignación principal en los sistemas de ficheros de propósito general modernos.

-----

### **3. Asignación Encadenada con Tabla de Asignación de Ficheros (FAT)**

* **Principio:**
    Para resolver los problemas de lentitud en el acceso aleatorio y de espacio de punteros de la asignación encadenada normal, el sistema de ficheros FAT centraliza toda la información de enlace de los bloques de datos (es decir, cuál es el siguiente bloque de cada bloque) en un área especial al principio del disco, llamada **Tabla de Asignación de Ficheros (FAT)**. La tabla FAT funciona como un array, donde cada entrada corresponde a un bloque de datos en el disco. El contenido de la entrada indica el índice (número de bloque) del siguiente bloque de datos del fichero, o una marca de fin de fichero (EOF), o una marca de bloque libre o de bloque defectuoso.

* **Información registrada en la entrada de directorio (reemplaza parte de la funcionalidad del i-nodo tradicional):**
    * **Nombre del fichero (File_Name)**
    * **Número de bloque inicial (Start_Block / First Cluster Number):** El índice en la tabla FAT del primer bloque de datos del fichero (que también es su número de bloque correspondiente en el área de datos del disco).
    * **Tamaño del fichero (File_Size)**
    * Otros atributos (fecha, hora, solo lectura, etc.).

* **Diagrama:**
    ```
    Entrada de Directorio:
    +-------------------------------------+
    | Nombre fichero: "ejemplo.txt"       |
    | Bloque inicial (Start_Block): 3     |
    | Tamaño fichero (File_Size)          |
    | ...                                 |
    +-------------------------------------+

    Tabla de Asignación de Ficheros (FAT) - Normalmente cargada en memoria:
    +-------------+-------------------------------+
    | Índice (Bloque)| Contenido (Siguiente Bloque/EOF)|
    +-------------+-------------------------------+
    |      0      | (Reservado/No disponible)       |
    |      1      | (Reservado/No disponible)       |
    |      2      | FREE (Libre)                    |
    |      3      | 6                               | <--- Primer bloque de "ejemplo.txt"
    |      4      | FREE                            |
    |      5      | 8                               | <--- Bloque inicial de otro fichero
    |      6      | 10                              | <--- Segundo bloque de "ejemplo.txt"
    |      7      | FREE                            |
    |      8      | EOF                             | <--- Fin de otro fichero
    |      9      | FREE                            |
    |     10      | EOF                             | <--- Tercer y último bloque de "ejemplo.txt"
    |     ...     | ...                             |
    +-------------+-------------------------------+

    Área de Datos en Disco:
    ... [Bloque 2] [Bloque 3 (D1)] [Bloque 4] [Bloque 5] [Bloque 6 (D2)] ... [Bloque 10 (D3)] ...
                     ^                           ^                          ^
                     |                           |                          |
    "ejemplo.txt" -> Bloque de datos 1           Bloque de datos 2          Bloque de datos 3
    ```
    * **Explicación:**
        * La entrada de directorio indica que "ejemplo.txt" comienza en el bloque 3.
        * Al consultar la tabla FAT: el contenido del índice 3 es 6, lo que significa que el siguiente bloque del fichero es el bloque 6.
        * El contenido del índice 6 es 10, lo que indica que el siguiente bloque es el 10.
        * El contenido del índice 10 es EOF (End Of File), lo que significa que este es el último bloque del fichero.
        * Por lo tanto, la cadena de bloques del fichero es 3 -> 6 -> 10. Estos bloques pueden estar dispersos en el disco.

* **Optimización:**
    * Al arrancar el sistema o montar el sistema de ficheros, toda la tabla FAT (o su parte más utilizada) se carga en la memoria. De esta manera, el proceso de búsqueda de la cadena de bloques de un fichero se realiza principalmente en la memoria, lo que es muy rápido.
    * Las modificaciones en la tabla FAT se realizan primero en la copia en memoria, y el sistema operativo las escribe periódicamente o durante operaciones específicas (como desmontar o apagar) de nuevo en el disco. El profesor destacó la importancia de un **apagado ordenado (graceful shutdown)** para asegurar que la versión más reciente de la tabla FAT en memoria se guarde correctamente en el disco.

* **Modo de acceso:**
    * **Acceso secuencial:** Bueno. Se encuentra el siguiente bloque buscando en la tabla FAT en memoria.
    * **Acceso aleatorio:** Muy bueno. Para acceder al bloque `k` de un fichero, se puede empezar desde el bloque inicial y realizar `k-1` búsquedas en la tabla FAT en memoria para localizar rápidamente el bloque objetivo.

* **Ventajas:**
    * Asignación dinámica.
    * Sin fragmentación externa.
    * El rendimiento del acceso aleatorio es mucho mejor que en la asignación encadenada básica.
    * Implementación relativamente simple.

* **Desventajas:**
    * **Tamaño de la tabla FAT:** El tamaño de la tabla FAT es proporcional a la capacidad de la partición del disco. Para discos muy grandes, la tabla FAT en sí misma puede ocupar una gran cantidad de espacio en disco y también una cantidad considerable de memoria (si se carga por completo).
    * **Integridad de la tabla FAT:** La tabla FAT es el núcleo del sistema de ficheros. Si se corrompe, puede provocar la pérdida o inaccesibilidad de todos los datos de la partición (por ello, suelen existir copias de seguridad de la tabla FAT).

* **Aplicaciones:**
    * Ampliamente utilizado en MS-DOS y las primeras versiones de Windows (FAT12, FAT16, FAT32).
    * exFAT es su evolución, compatible con ficheros y particiones más grandes.
    * Las unidades USB, tarjetas SD y otros dispositivos de almacenamiento extraíbles todavía usan con frecuencia FAT32 o exFAT por su buena compatibilidad.

-----

### **4. Asignación Indexada**

* **Principio:**
    Se asigna a cada fichero uno o más **bloques de índices**. Estos bloques no almacenan datos del fichero en sí, sino una lista de punteros (direcciones/números de bloque en disco) a los bloques de datos reales del fichero. El i-nodo almacena un puntero a este (o al primer) bloque de índices.

* **Información registrada en el i-nodo:**
    * **Puntero al bloque de índices (Pointer to Index Block):** Apunta al bloque que almacena la lista de direcciones de los bloques de datos del fichero.
    * **Tamaño del fichero (File_Size)**
    * Otros metadatos.

* **Diagrama (índice de un solo nivel):**
    ```
    I-nodo:
    +-------------------------------------+
    | Nombre del fichero (Nombre)         |
    | Puntero al bloque de índices: 20    |
    | Tamaño del fichero (File_Size)      |
    | Otros metadatos...                  |
    +-------------------------------------+
             |
             |
             v
    Bloque de Disco 20 (Bloque de Índices):
    +-------------------------------------+
    | Ptr -> Bloque_Datos_1 (ej: B9)      |
    | Ptr -> Bloque_Datos_2 (ej: B12)     |
    | Ptr -> Bloque_Datos_3 (ej: B15)     |
    | Ptr -> Bloque_Datos_4 (ej: B2)      |
    | ... (hasta llenar el bloque)        |
    | Puntero_a_siguiente_bloque_de_índices (si es necesario y no es multinivel) |
    +-------------------------------------+

    Área de Datos en Disco:
    ... [B2(D4)] ... [B9(D1)] ... [B12(D2)] ... [B15(D3)] ...
    ```
    * **Explicación:**
        * El i-nodo apunta al bloque de disco 20, que es un bloque de índices.
        * El bloque de índices 20 no almacena datos de usuario, sino una serie de punteros a los bloques de datos reales. Por ejemplo, el primer puntero apunta a B9 (bloque de datos 1 del fichero), el segundo a B12 (bloque de datos 2), y así sucesivamente.
        * Los bloques de datos reales B9, B12, B15, B2, etc., pueden estar dispersos en el disco.
        * Si el fichero es muy grande y un solo bloque de índices no puede contener todos los punteros a los bloques de datos, hay dos formas de expandirlo:
            * **Bloques de índices encadenados:** El final del primer bloque de índices tiene un puntero al segundo bloque de índices, y así sucesivamente, formando una cadena de bloques de índices.
            * **Índice multinivel:** Es un método más avanzado, que se detalla en la siguiente sección.

* **Modo de acceso:**
    * **Acceso secuencial:** Aceptable. Se necesita leer primero el i-nodo, luego el bloque de índices, y después leer secuencialmente los bloques de datos según los punteros del bloque de índices. Es ligeramente peor que FAT porque requiere al menos un acceso a disco adicional para el bloque de índices (a menos que este también esté en caché).
    * **Acceso aleatorio:** Bueno. Para acceder al bloque de datos `k` de un fichero, primero se lee el i-nodo, luego el bloque de índices, se obtiene directamente la `k`-ésima entrada del bloque de índices (que es el puntero al bloque de datos `k`), y finalmente se accede a dicho bloque de datos.

* **Ventajas:**
    * Soporta asignación dinámica.
    * Sin fragmentación externa.
    * **Resuelve el problema del gran tamaño de la tabla FAT:** Cada fichero gestiona su propia información de índices, que está descentralizada en sus respectivos bloques de índices en lugar de en una tabla centralizada enorme.
    * Buen rendimiento en acceso aleatorio.

* **Desventajas:**
    * **Coste del bloque de índices:** Para ficheros muy pequeños (por ejemplo, que ocupan solo uno o dos bloques de datos), todavía es necesario asignar un bloque de índices completo, lo que puede causar un desperdicio de espacio (fragmentación interna, pero esta vez a nivel de bloque de índices).
    * Para ficheros grandes, si se utiliza el método de bloques de índices encadenados, acceder a los bloques de datos del final puede requerir la lectura de múltiples bloques de índices.

* **Aplicaciones:**
    * Muchos sistemas de ficheros modernos (como aspectos de NTFS y algunos sistemas de ficheros Unix antiguos) han adoptado la idea de la asignación indexada o sus variantes.

-----

### **5. Asignación Indexada Multinivel**

* **Principio:**
    Es una extensión de la asignación indexada, especialmente adecuada para soportar un acceso eficiente a ficheros muy grandes. El i-nodo no solo contiene punteros directos a bloques de datos, sino también punteros a bloques de índices indirectos. Estos bloques de índices indirectos pueden, a su vez, apuntar a bloques de índices de nivel inferior o directamente a bloques de datos.

* **Información registrada en el i-nodo (ejemplo de Linux Ext2/Ext3/Ext4):**
    * **Punteros a bloques directos (Direct Block Pointers):** Suelen ser varios (por ejemplo, 10-12) y apuntan directamente a los primeros bloques de datos del fichero.
    * **Puntero a bloque indirecto simple (Single Indirect Pointer):** Apunta a un bloque de índice de primer nivel. Este bloque de primer nivel está lleno de punteros a bloques de datos reales.
    * **Puntero a bloque doblemente indirecto (Double Indirect Pointer):** Apunta a un bloque de índice de segundo nivel. Este bloque de segundo nivel está lleno de punteros a bloques de índice de primer nivel. Cada uno de esos bloques de primer nivel, a su vez, apunta a bloques de datos.
    * **Puntero a bloque triplemente indirecto (Triple Indirect Pointer):** Añade otro nivel de indirección para soportar ficheros extremadamente grandes.
    * **Tamaño del fichero (File_Size)**
    * Otros metadatos.

* **Diagrama (ejemplo simplificado: 2 bloques directos, 1 bloque indirecto simple):**
    ```
    I-nodo:
    +-------------------------------------------------+
    | Nombre del fichero (Nombre)                     |
    | Direct_Ptr_1: B5                                | --> Bloque de Datos 1 (B5)
    | Direct_Ptr_2: B8                                | --> Bloque de Datos 2 (B8)
    | Single_Indirect_Ptr: B20                        | ---+
    | Double_Indirect_Ptr: (ej: NULL si no se usa)    |    |
    | Triple_Indirect_Ptr: (ej: NULL si no se usa)    |    |
    | Tamaño del fichero (File_Size)                  |    |
    | Otros metadatos...                              |    |
    +-------------------------------------------------+    |
                                                         |
                                                         v
                                            Bloque de Disco 20 (Bloque de Índices de 1er Nivel):
                                            +-------------------------------------+
                                            | Ptr -> Bloque_Datos_3 (ej: B2)      | --> BD3 (B2)
                                            | Ptr -> Bloque_Datos_4 (ej: B17)     | --> BD4 (B17)
                                            | Ptr -> Bloque_Datos_5 (ej: B1)      | --> BD5 (B1)
                                            | ... (muchos más punteros a datos)   |
                                            +-------------------------------------+
    ```
    * **Explicación:**
        * Los ficheros pequeños pueden necesitar solo los punteros a bloques directos. Por ejemplo, si un fichero tiene solo 2 bloques de datos, solo se usarían `Direct_Ptr_1` y `Direct_Ptr_2`, y los punteros indirectos serían NULL.
        * Cuando el fichero crece más allá del alcance de los bloques directos, el sistema asigna un bloque de índice de primer nivel (B20 en este ejemplo). El `Single_Indirect_Ptr` del i-nodo apunta a B20. B20 está lleno de punteros a bloques de datos reales. Si un bloque tiene un tamaño de 4 KB y un puntero ocupa 4 bytes, un bloque de índice de primer nivel puede apuntar a 1024 bloques de datos.
        * Si el fichero es aún más grande y agota todos los bloques de datos a los que puede apuntar el bloque indirecto simple, el sistema utiliza el puntero de bloque doblemente indirecto. Este puntero apunta a un bloque de segundo nivel, donde cada entrada apunta a un bloque de primer nivel, y cada bloque de primer nivel apunta a su vez a bloques de datos. Esto aumenta exponencialmente el número de bloques de datos direccionables. Lo mismo se aplica al de tercer nivel.
        * El profesor mencionó que con esta estructura (por ejemplo, 10 bloques directos, y un puntero a bloque indirecto simple, doble y triple, con un tamaño de bloque de 4KB y punteros de 4 bytes), un sistema de ficheros de Linux puede soportar un único fichero de hasta 4 PB (Petabytes).

* **Modo de acceso:**
    * El acceso a ficheros pequeños (que solo usan bloques directos) es muy rápido.
    * Al acceder a ficheros grandes, es necesario determinar si el bloque de datos se localiza a través de un puntero directo, de primer, segundo o tercer nivel, lo que implicará la lectura de los bloques de índices correspondientes.

* **Ventajas:**
    * **Excelente soporte para ficheros grandes:** Capaz de gestionar ficheros de tamaño muy grande.
    * **Amigable con ficheros pequeños:** El coste de acceso para ficheros pequeños es bajo.
    * **Asignación dinámica y coste de indexación bajo demanda:** Los bloques de índices indirectos solo se asignan cuando el fichero crece hasta necesitarlos.
    * Sin fragmentación externa.

* **Desventajas:**
    * Implementación relativamente compleja.
    * Para datos en la parte central o final de ficheros grandes, el acceso puede requerir la lectura de múltiples niveles de bloques de índices, lo que introduce cierta latencia (aunque estos bloques de índices suelen ser cacheados).

* **Aplicaciones:**
    * Los sistemas de ficheros modernos de Unix/Linux (como Ext2, Ext3, Ext4, UFS, etc.) utilizan ampliamente este esquema de asignación indexada multinivel o uno similar.