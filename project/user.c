#include <libc.h>

// Colores
#define sprite_color 0x04  // Serpiente
#define wall_color 0x07    // Paredes
#define bg_color 0x00      // Fondo
#define fruit_color 0x0A   // Fruta
#define head_color 0x0C    // Cabeza serpiente

// Dimensiones tablero (celdas)
#define X_size 26
#define Y_size 8

// Direcciones
#define RIGHT 0
#define UP 1
#define LEFT 2
#define DOWN 3
#define NO_MOVEMENT 4

// Vectores para calcular desplazamiento por dirección
int x_dirs[5] = {1, 0, -1, 0, 0};
int y_dirs[5] = {0, -1, 0, 1, 0};

// Semáforo para proteger el cambio de dirección
int dir_sem;

// Imprime string en consola (debug)
void print(char* s) {
    write(1, s, strlen(s));
}

void printc(char c) {
	char* s = ".";
	s[0] = c;
	print(s);
}

// Imprime número de dos dígitos en consola (debug)
void print_n(int num) {
    char* buffer = "00";
    buffer[1] = '0' + num%10;
    buffer[0] = '0' + num/10;
    print(buffer);
}

void debug_keyboard(char* k) {
	int i = 0;
	for (i = 0; i < 100; ++i) {
		printc(i);
	}
	printc('\n');
	for (i = 0; i < 100; ++i) {
		printc('0' + k[i]);
	}
	printc('\n');
}

// Estructura con el estado del juego
struct sgs {
    int grid[X_size][Y_size]; // Tablero: 0=vacío, -1=pared, -2=fruta, >0=serpiente
    unsigned int length;      // Longitud serpiente
    unsigned int seed;        // Semilla para RNG
    int head_x;               // Posición X cabeza
    int head_y;               // Posición Y cabeza
    char dir;                 // Dirección actual
    char next_dir;            // Próxima dirección (input)
    char alive;               // 1 si viva, 0 muerta
    int moves;                // Contador movimientos
    int fruits_eaten;         // Contador frutas
}; //snake game state

/* --- Helper functions --- */

// Genera siguiente número pseudoaleatorio
unsigned int getNextSeed(unsigned int seed) {
    return (seed * 0x5DEECE66DL + 0xBL);
}

// Comprueba si (x,y) está dentro del tablero jugable
int isValidPos(int x, int y) {
    return x > 0 && x < X_size-1 && y > 0 && y < Y_size-1;
}

/* --- Actualización de Dirección --- */

// Calcula la nueva dirección según tecla y dirección actual (evita giro 180º)
char getNextDir(char key_just_pressed, char current_dir) {
    if (key_just_pressed == 'w' && current_dir != DOWN) return UP;
    if (key_just_pressed == 'a' && current_dir != RIGHT) return LEFT;
    if (key_just_pressed == 's' && current_dir != UP) return DOWN;
    if (key_just_pressed == 'd' && current_dir != LEFT) return RIGHT;
    return current_dir;
}

// Teclas de dirección
char keys_for_dirs[] = {'w','a','s','d'};

// Actualiza game->next_dir si se pulsa una tecla de dirección nueva
void updateSelectedDir(struct sgs* game, char* old_keyboard, char* new_keyboard) {
    int i;
    for (i = 0; i < 4; ++i) {
        // Comprueba si la tecla se acaba de pulsar
        if (new_keyboard[(unsigned char)keys_for_dirs[i]] && !old_keyboard[(unsigned char)keys_for_dirs[i]]) {
            semWait(dir_sem); // Protege acceso a next_dir/dir
            char old_dir = game->next_dir;
            game->next_dir = getNextDir(keys_for_dirs[i], game->dir);
            if (old_dir != game->next_dir) {
                print("Dir changed from "); print_n(old_dir); print(" to "); print_n(game->next_dir); print("\n");
            }
            semPost(dir_sem);
        }
    }
}

/* --- Lógica del Juego --- */

/* Mueve la serpiente 1 paso. Devuelve:
 * -1 si muere
 *  0 si movimiento normal
 *  1 si come fruta
 */
int move(struct sgs* game) {
    int x, y, i, j;
    int fruit_eaten;
    char new_grid_value;
    char next_dir;

    if (!game->alive) return -1;

    semWait(dir_sem); // Lee dirección protegida
    next_dir = game->next_dir;
    semPost(dir_sem);

    // Calcula nueva posición cabeza
    x = game->head_x + x_dirs[(int)next_dir];
    y = game->head_y + y_dirs[(int)next_dir];

    game->dir = next_dir;
    game->moves++;

    // Comprueba colisión con bordes
    if (x <= 0 || x >= X_size-1 || y <= 0 || y >= Y_size-1) {
        print("Collision with border at ("); print_n(x); print(","); print_n(y); print(")\n");
        game->alive = 0;
        return -1;
    }

    new_grid_value = game->grid[x][y];

    // Comprueba colisión con pared o consigo misma
    if (new_grid_value == -1) {
        print("Collision with wall at ("); print_n(x); print(","); print_n(y); print(")\n");
        game->alive = 0;
        return -1;
    }
    if (new_grid_value > 0 && new_grid_value != game->length) {
        print("Collision with self at ("); print_n(x); print(","); print_n(y); print(")\n");
        game->alive = 0;
        return -1;
    }

    // Actualiza posición cabeza
    game->head_x = x;
    game->head_y = y;

    fruit_eaten = (new_grid_value == -2);

    // Mueve la cabeza en el grid
    game->grid[x][y] = 1;

    // Actualiza el cuerpo: incrementa valor de segmentos y borra la cola vieja
    for (i = 0; i < X_size; i++) {
        for (j = 0; j < Y_size; j++) {
            if (game->grid[i][j] > 0 && (i != x || j != y)) { // Si es cuerpo y no es la nueva cabeza
                game->grid[i][j]++;
                if (game->grid[i][j] > game->length) { // Si era la cola
                    game->grid[i][j] = 0; // Borra la cola
                }
            }
        }
    }

    if (fruit_eaten) {
        ++game->length;
        game->fruits_eaten++;
        return 1; // Comió
    }

    return 0; // Movimiento normal
}

// Coloca una fruta en una celda vacía aleatoria
void spawnFruit(struct sgs* game) {
    int x, y, randRes;
    int freeSpaces = 0;

    // Cuenta espacios libres
    for (x = 0; x < X_size; ++x) {
        for (y = 0; y < Y_size; ++y) {
            if (game->grid[x][y] == 0) ++freeSpaces;
        }
    }

    if (freeSpaces == 0) {
        print("No free spaces for fruit!\n");
        return;
    }

    // Elige una celda vacía aleatoria
    randRes = game->seed % freeSpaces;
    game->seed = getNextSeed(game->seed);

    // Busca la celda elegida y coloca la fruta (-2)
    for (x = 0; x < X_size; ++x) {
        for (y = 0; y < Y_size; ++y) {
            if (game->grid[x][y] == 0) --randRes;
            if (randRes < 0) {
                game->grid[x][y] = -2;
                print("Spawned fruit at ("); print_n(x); print(","); print_n(y); print(")\n");
                return;
            }
        }
    }
}

/* --- Funciones de Dibujo --- */

// Dibuja un carácter en el buffer de pantalla (coordenadas píxel)
void draw_char(char* screen_buffer, int x, int y, char c, char color) {
    if (x < 0 || x >= 80 || y < 0 || y >= 25) return; // Límites pantalla
    int pos = (y * 80 + x) * 2;
    if (pos < 0 || pos >= 80*25*2) return; // Límites buffer
    screen_buffer[pos] = c;
    screen_buffer[pos + 1] = color;
}

// Dibuja estadísticas (Score, Moves) en la fila 0
void drawStats(char* screen_buffer, struct sgs* game) {
    char* score_text = "Score: ";
    char* moves_text = " Moves: ";
    int i;
    // Dibuja "Score: X"
    for (i = 0; i < 7; i++) draw_char(screen_buffer, i, 0, score_text[i], 0x07);
    draw_char(screen_buffer, 7, 0, '0' + game->fruits_eaten, 0x07);
    // Dibuja " Moves: X"
    for (i = 0; i < 8; i++) draw_char(screen_buffer, i + 9, 0, moves_text[i], 0x07);
    if (game->moves < 10) {
        draw_char(screen_buffer, 17, 0, '0' + game->moves, 0x07);
    } else {
        draw_char(screen_buffer, 17, 0, '0' + game->moves / 10, 0x07);
        draw_char(screen_buffer, 18, 0, '0' + game->moves % 10, 0x07);
    }
}

// Imprime el grid en consola (debug)
void debugGrid(struct sgs* game) {
    int i, j;
    print("\nGrid state:\n");
    for (j = 0; j < Y_size; j++) {
        for (i = 0; i < X_size; i++) {
            if (game->grid[i][j] == -1) print("#");      // Pared
            else if (game->grid[i][j] == -2) print("F"); // Fruta
            else if (game->grid[i][j] == 1) print("H");  // Cabeza
            else if (game->grid[i][j] > 1) print("B");   // Cuerpo
            else print("."); // Vacío
        }
        print("\n");
    }
}

// Dibuja un bloque 3x3 para pared, fruta o vacío
void drawSpecial(char* screen_buffer, int x, int y, int element_id, char color, char background_color) {
    int i, j;
    // Limpia área 3x3
    for (i = x*3; i < x*3+3; ++i) {
        for (j = y*3; j < y*3+3; ++j) {
            draw_char(screen_buffer, i, j, ' ', bg_color);
        }
    }
    // Dibuja elemento
    if (element_id == -1) { // Pared
        for (i = x*3; i < x*3+3; ++i) {
            for (j = y*3; j < y*3+3; ++j) {
                draw_char(screen_buffer, i, j, '#', wall_color);
            }
        }
    } else if (element_id == -2) { // Fruta
        draw_char(screen_buffer, x*3,   y*3+1, '*', fruit_color);
        draw_char(screen_buffer, x*3+1, y*3,   '*', fruit_color);
        draw_char(screen_buffer, x*3+1, y*3+1, 'O', fruit_color);
        draw_char(screen_buffer, x*3+1, y*3+2, '*', fruit_color);
        draw_char(screen_buffer, x*3+2, y*3+1, '*', fruit_color);
    }
}

// Dibuja un bloque 3x3 para la cabeza o cuerpo de la serpiente
void drawSnake(char* screen_buffer, int x, int y, int value, int length) {
    int i, j;
    char symbol;
    int color;
    // Limpia área 3x3
    for (i = x*3; i < x*3+3; ++i) {
        for (j = y*3; j < y*3+3; ++j) {
            draw_char(screen_buffer, i, j, ' ', bg_color);
        }
    }
    // Elige símbolo y color
    if (value == 1) { symbol = '@'; color = head_color; } // Cabeza
    else { symbol = 'O'; color = sprite_color; } // Cuerpo
    // Dibuja cruz 3x3
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (i == 1 || j == 1) {
                draw_char(screen_buffer, x*3+i, y*3+j, symbol, color);
            }
        }
    }
}

// Dibuja el estado completo del juego recorriendo el grid
void drawGame(int grid[X_size][Y_size], char* screen_buffer, struct sgs* game) {
    int x, y;
    drawStats(screen_buffer, game); // Dibuja estadísticas
    // Recorre el grid y dibuja cada celda
    for (x = 0; x < X_size; ++x) {
        for (y = 0; y < Y_size; ++y) {
            int value = grid[x][y];
            if (value <= 0) drawSpecial(screen_buffer, x, y, value, sprite_color, bg_color); // Pared, vacío, fruta
            else drawSnake(screen_buffer, x, y, value, game->length); // Serpiente
        }
    }
}

/* --- Funciones de Actualización y Threads --- */

// Actualiza el juego (mueve serpiente si toca) y redibuja la pantalla
int update(struct sgs* game, char* screen_buffer, int do_movement) {
    int res = 0;
    if (do_movement) {
        print("m("); print_n(game->head_x); print(","); print_n(game->head_y); print(")\n");
        res = move(game);
        // Debug grid ocasional
        if (game->moves % 5 == 0) debugGrid(game);
        if (res == -1) return -1; // Game Over
        if (res == 1) { // Comió fruta
            spawnFruit(game);
            print("Ate fruit! Score: "); print_n(game->fruits_eaten); print("\n");
        }
    }
    drawGame(game->grid, screen_buffer, game); // Dibuja siempre
    return 0; // OK
}

// Thread secundario: lee el teclado y actualiza la dirección
void* poll_keyboard(void* param) {
    struct sgs* game = (struct sgs*)param;
    SetPriority(10); // Prioridad alta para buena respuesta

    char keyboard1[256]; // Buffer teclado anterior
    char keyboard2[256]; // Buffer teclado actual
    int recent_keyboard = 1; // Alterna buffers

    GetKeyboardState(keyboard1); // Lee estado inicial
    GetKeyboardState(keyboard2);

    print("thread set up. entering thread loop\n");
    while (1) {
        if (recent_keyboard == 1) {
            GetKeyboardState(keyboard2); // Lee teclado actual
            debug_keyboard(keyboard2);
            // Debug: imprime teclas WASD presionadas
            if (keyboard2[(unsigned char)'w'] || keyboard2[(unsigned char)'a'] ||
                keyboard2[(unsigned char)'s'] || keyboard2[(unsigned char)'d']) {
                print("Keys: ");
                if (keyboard2[(unsigned char)'w']) print("w");
                if (keyboard2[(unsigned char)'a']) print("a");
                if (keyboard2[(unsigned char)'s']) print("s");
                if (keyboard2[(unsigned char)'d']) print("d");
                print("\n");
            }
            updateSelectedDir(game, keyboard1, keyboard2); // Actualiza dirección si hubo cambio
            recent_keyboard = 2;
        }
        else { // Lo mismo pero usando keyboard1 como actual
            GetKeyboardState(keyboard1);
            debug_keyboard(keyboard1);
            if (keyboard1[(unsigned char)'w'] || keyboard1[(unsigned char)'a'] ||
                keyboard1[(unsigned char)'s'] || keyboard1[(unsigned char)'d']) {
                print("Keys: ");
                if (keyboard1[(unsigned char)'w']) print("w");
                if (keyboard1[(unsigned char)'a']) print("a");
                if (keyboard1[(unsigned char)'s']) print("s");
                if (keyboard1[(unsigned char)'d']) print("d");
                print("\n");
            }
            updateSelectedDir(game, keyboard2, keyboard1);
            recent_keyboard = 1;
        }
        pause(200); // Pausa para ceder CPU
    }
    return 0;
}

// Puntero global al buffer de pantalla
char* screen_buffer;

// Función principal del programa
int __attribute__ ((__section__(".text.main")))
main(void)
{
    int i, j;
    struct sgs game;

    // Inicializa contadores
    game.moves = 0;
    game.fruits_eaten = 0;

    // Inicializa grid con paredes y vacío
    for (i = 0; i < X_size; i++) {
        for (j = 0; j < Y_size; j++) {
            if (i == 0 || j == 0 || i == X_size - 1 || j == Y_size - 1)
                game.grid[i][j] = -1; // Pared
            else
                game.grid[i][j] = 0;   // Vacío
        }
    }

    // Inicializa serpiente
    game.length = 1;
    game.seed = 1; // Usar gettime() sería mejor para aleatoriedad
    game.seed = getNextSeed(game.seed);
    game.head_x = 3;
    game.head_y = 3;
    game.grid[3][3] = 1;
    game.dir = RIGHT;
    game.next_dir = RIGHT;
    game.alive = 1;

    // Inicializa semáforo
dir_sem = semCreate(1);
print("Semaphore created with ID: ");
print_n(dir_sem);
print("\n");
if (dir_sem < 0) {
    print("Error creating semaphore\n");
    exit();
}

    // Imprime grid inicial (debug)
    debugGrid(&game);

    // Coloca fruta inicial
    game.grid[10][3] = -2;
    print("Initial fruit at (10,3)\n");

    print("initialized game\n");

    // Crea thread para teclado
	if (pthread_create(poll_keyboard, (void*)&game, 4096) <= 0) {
		print("Thread creation error, code: ");
		print_n(dir_sem); // Para ver qué valor retorna
		write(1, "error thread create\n", 20);
		exit();
	}
	print("Thread created successfully\n");

    // Obtiene buffer de pantalla
    screen_buffer = StartScreen();
    if (screen_buffer == (void*)-1) {
        print("Failed to get screen buffer\n");
        exit();
    }

    print("started screen. entering main loop\n");

    int cnt = 0;
    int do_move; // 1 si toca mover, 0 si no

    // Bucle principal (controlado por el thread main)
    while(1) {
        ++cnt;
        // Control de velocidad: mueve cada 100 ciclos de este bucle
        if (cnt == 1000) {
            do_move = 1;
            cnt = 0;
        }
        else do_move = 0;

        // Actualiza estado y dibuja. Si devuelve -1, termina.
        if (update(&game, screen_buffer, do_move) == -1) {
            print("\n-----------------------\n");
            print("GAME OVER!\n");
            print("Final Score: "); print_n(game.fruits_eaten);
            print("\nMoves: "); print_n(game.moves);
            print("\n-----------------------\n");
            break; // Sale del bucle while
        }
    }

    // Limpieza al final
    semDestroy(dir_sem);

    return 0;
}
