# Sistema Operativo 2 (SO2) - Proyecto Final
**Facultat d'Informàtica de Barcelona (FIB)**  
**Universitat Politècnica de Catalunya (UPC)**  
**BarcelonaTech**  
**Curso Académico: 2024-2025 Q2**

## 👥 Equipo de Desarrollo

| Estudiante | Calificación SOA |
|------------|------------------|
| **Pau Martí Biosca** | Matrícula de Honor (MH) |
| **Hongda Zhu** | 8.0 |


## 🎯 Descripción del Proyecto

Este proyecto consiste en la **creación de un sistema operativo funcional** que incluye:

- ✅ Mecanismos de entrada al sistema
- ✅ Gestión básica de procesos
- ✅ Características adicionales
- ✅ Hilos y sincronización
- ✅ Gestión de E/S y memoria dinámica

---

## 📁 Estructura del Repositorio

### `/lab/` - Código para Lab 1 / Lab 2
Contiene los archivos base del sistema operativo desarrollado durante las prácticas:

**Archivos principales:**
- `interrupt.c` - Gestión de interrupciones
- `io.c` - Operaciones de entrada/salida
- `mm.c` - Gestión de memoria
- `sched.c` - Planificador de procesos
- `sys.c` - Llamadas al sistema
- `user.c` - Código de usuario
- `system.c` - Inicialización del sistema

**Archivos de configuración:**
- `Makefile` - Script de compilación
- `system.lds` / `user.lds` - Scripts del enlazador
- `.bochsrc` - Configuración del emulador Bochs

**Ensamblador:**
- `bootsect.S` - Sector de arranque
- `entry.S` - Punto de entrada del kernel
- `sys_call_table.S` - Tabla de llamadas al sistema
- `wrappers.S` - Wrappers de sistema

### `/project/` - Entregas del Proyecto final
Contiene las entregas finales y documentación:

- `Project/` - Código fuente final del proyecto
- `Q2 2024-2025 SOA Project.pdf` - Especificación del proyecto
- `part1.tar.gz` - Entrega Parte 1
- `part2.tar.gz` - Entrega Parte 2  
- `part3.tar.gz` - Entrega Parte 3
- `part4.tar.gz` - Entrega Parte 4
- `readme.md` - Documentación detallada del proyecto

### `/teoria-p2/` - Material Teórico
Documentación y material de estudio:
- `tema1.pdf` - `tema5.pdf` - Temas teóricos del curso
- `resumen p2/` - Resúmenes de la segunda parte

### En la rama milestone5x2 está nuestro entrega final del proyecto


## 🛠️ Compilación y Ejecución

### Prerrequisitos

Usar el imagen de https://softdocencia.fib.upc.edu/software/Ubuntu22v3r1.ova;

### Compilación
```bash
cd lab/
make clean
make
```

### Ejecución
```bash
# Ejecutar con Bochs
bochs -f .bochsrc

# Debug con GDB
make gdb
```

---

## 📊 Sistema de Evaluación

### **Competencia Técnica (CTc) - 100%**

| Componente | Peso | Descripción |
|------------|------|-------------|
| **E1** | 2.5% | Entrega 1: Estado del proyecto |
| **E2** | 2.5% | Entrega 2: Proyecto final |
| **T1** | 25% | Examen parcial 1 (teoría) |
| **T2** | 25% | Examen parcial 2 (teoría) |
| **L1** | 20% | Examen de laboratorio |
| **Milestones** | 25% | Hitos del proyecto |

### **Distribución General**
- **50% Teoría** (T1 + T2)
- **50% Laboratorio** (L1 + Seguimiento + Proyecto)

