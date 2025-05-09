#include <libc.h>

// Variable compartida
int shared_var = 0;
int sem_id;
int mobex = 1;

void test(void* par) {
	SetPriority(2);
	while(1) {
		if (mobex == 1) {
			write(1, "xd", 2);
			mobex = 0;
		}
		else write(1, ",", 1);
	}
}

void test2(void* par) {
  int i;
  for (i = 0; i < 1000; i++) {
    semWait(sem_id);
    // Sección crítica
    shared_var++;
    write(1, ".", 1);
    semPost(sem_id);
  }
  // Salir del thread
  pthread_exit();
}

int __attribute__ ((__section__(".text.main")))
main(void) {
  // Crear un semáforo con valor inicial 1 (mutex)
  sem_id = semCreate(1);
  int sem_wait_id = semWait(sem_id);
  int sem_post_id = semPost(sem_id);
  semDestroy(sem_id);

  sem_id = semCreate(1);
  if (sem_id < 0) {
    write(1, "Error creating semaphore\n", 25);
    exit();
  }
  
  // Crear 2 threads
  if (pthread_create(test2, (void*)0, 4096) == 0) {
    write(1, "Error creating thread 1\n", 24);
    exit();
  }
  
  if (pthread_create(test2, (void*)0, 4096) == 0) {
    write(1, "Error creating thread 2\n", 24);
    exit();
  }
  
  // Esperar un tiempo para que los threads terminen
  pause(1000);
  //int i;
  //for (i = 0; i < 10000000; i++);
  
  // Mostrar el valor final
  char buffer[12];
  write(1, "\nFinal value: ", 14);
  itoa(shared_var, buffer);
  write(1, buffer, strlen(buffer));
  write(1, "\n", 1);
  
  // Destruir el semáforo
  semDestroy(sem_id);
  
  while(1);
}
