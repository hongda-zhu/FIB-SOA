#include <libc.h>

#define X_size 26 //80px
#define Y_size 8  //25px
//each position is 3x3 pixels? ?? (have to determine later mapping from in game coords to pixels)

#define sprite_color 0x00
#define bg_color 0xFF

#define RIGHT 0
#define UP 1
#define LEFT 2
#define DOWN 3
#define NO_MOVEMENT 4
int x_dirs[4] = {1, 0, -1, 0, 0}; //must correspond with define numbers
int y_dirs[4] = {0, -1, 0, 1, 0};

void print(char* s) {
	write(1, s, strlen(s));
}

void print_n(int num) {
  char* buffer = "00";
  buffer[1] = '0' + num%10;
  buffer[0] = '0' + num/10;
  print(buffer);
}

struct sgs {
	int grid[X_size][Y_size]; //0 = empty, -1 = wall, -2 = fruit, 1,2,3... = snake (no need to save snake numbers but makes it easier to code)
	unsigned int length; //snake length
	unsigned int seed; //for rng
	int head_x; //current x position of snake head
	int head_y; //current y position of snake head
	char dir; //current direction
	char next_dir; //direction next tick
	char alive; //boolean
}; //snake game state

/* Helper functions */
unsigned int getNextSeed(unsigned int seed) {
	return (seed * 0x5DEECE66DL + 0xBL);
}

int isValidPos(int x, int y) {
	return x > 0 && x < X_size && y > 0 && y < Y_size;
}

/* Direction update */
char getNextDir(char key_just_pressed, char current_dir) {
	if (key_just_pressed == 'w' && current_dir != DOWN) return UP;
	if (key_just_pressed == 'a' && current_dir != RIGHT) return LEFT;
	if (key_just_pressed == 's' && current_dir != UP) return DOWN;
	if (key_just_pressed == 'd' && current_dir != LEFT) return RIGHT;
	return current_dir;
}

char keys_for_dirs[] = {'w','a','s','d'};
void updateSelectedDir(struct sgs* game, char* old_keyboard, char* new_keyboard) {
	int i;
	for (i = 0; i < 4; ++i)
		if (new_keyboard[keys_for_dirs[i]] && !old_keyboard[keys_for_dirs[i]])
			game->next_dir = getNextDir(keys_for_dirs[i], game->dir);
}

/* Game Logic */

/* Returns -1 if dead, 0 if moved, 1 if moved and eaten fruit */
int move(struct sgs* game) {
	int x, y, i, j, xx, yy;
	int fruit_eaten;
	char new_grid_value;

	if (!game->alive) return -1;

	x = game->head_x + x_dirs[game->next_dir];
	y = game->head_y + y_dirs[game->next_dir];

	game->dir = game->next_dir;

	//check if game ends
	if (!isValidPos(x, y)) { //outside of map?
		game->alive = 0;
		return -1;
	}
	new_grid_value = game->grid[x][y];
	if (new_grid_value == -1) { //collided with wall
		game->alive = 0;
		return -1;
	}
	if (new_grid_value > 0 && new_grid_value != game->length) { //collided with itself (excluding tail)
		game->alive = 0;
		return -1;
	}

	game->head_x = x;
	game->head_y = y;
	
	fruit_eaten = (new_grid_value == -2);

	//move snake
	for (i = 1; i <= game->length; ++i) {
		game->grid[x][y] = i;
		for (j = 0; j < 4; ++j) {
			xx = x + x_dirs[j];
			yy = y + y_dirs[j];
			if (isValidPos(xx, yy) && game->grid[x][y] == game->grid[xx][yy]) {
				x = xx;
				y = yy;
				break;
			}
		}
	}

	//determine what happens with tail
	if (fruit_eaten) {
		++game->length;
		game->grid[x][y] = game->length;
		return 1;
	}
	else {
		game->grid[x][y] = 0;
		return 0;
	}
}

void spawnFruit(struct sgs* game) {
	int x, y, randRes;
	int freeSpaces = 0;

	//count how many free spaces there are
	for (x = 0; x < X_size; ++x) {
		for (y = 0; y < Y_size; ++y) {
			if (game->grid[x][y] == 0) ++freeSpaces;
		}
	}

	//choose a random number out of all these free spaces, and spawn fruit there
	randRes = game->seed % freeSpaces;
	game->seed = getNextSeed(game->seed);
	for (x = 0; x < X_size; ++x) {
		for (y = 0; y < Y_size; ++y) {
			if (game->grid[x][y] == 0) --randRes;
			if (randRes == 0) {
				game->grid[x][y] = -2;
				return;
			}
		}
	}
}

void draw_char(char* screen_buffer, int x, int y, char c, char color) {
	x %= 80;
	y %= 25;
    int pos = (y * 80 + x) * 2;
    screen_buffer[pos] = c;
    screen_buffer[pos + 1] = color;
}

/* Rendering */
void drawSpecial(char* screen_buffer, int x, int y, int element_id, char color, char background_color) {
	int i, j;
	if (element_id == -1) {
		//wall
		for (i = x*3; i < x*3+3; ++i) {
			for (j = y*3; j < y*3+3; ++j) {
				draw_char(screen_buffer, i, j, 'X', color);
			}
		}
		return;
	}
	
	//draw emptyness
	for (i = x*3; i < x*3+3; ++i) {
		for (j = y*3; j < y*3+3; ++j) {
			draw_char(screen_buffer, i, j, ' ', background_color);
		}
	}
	if (element_id == -2) {
		//fruit
		draw_char(screen_buffer, x*3+1, y*3+1, 'O', color);
	}
}

void drawBody(char* screen_buffer, int x, int y, int prev, int next, char color, char background_color) {
	int i, j;
	//draw emptyness
	for (i = x*3; i < x*3+3; ++i) {
		for (j = y*3; j < y*3+3; ++j) {
			draw_char(screen_buffer, i, j, ' ', background_color);
		}
	}

	draw_char(screen_buffer, x*3+1, y*3+1, 'X', color);
	draw_char(screen_buffer, x*3+1 + x_dirs[prev], y*3+1 + y_dirs[prev], 'X', color);
	draw_char(screen_buffer, x*3+1 + x_dirs[next], y*3+1 + y_dirs[next], 'X', color);
}

void drawGame(int grid[X_size][Y_size], char* screen_buffer) {
	int x, y, xx, yy, i;
	char next, prev;
	for (x = 0; x < X_size; ++x) {
		for (y = 0; y < Y_size; ++y) {
			if (grid[x][y] <= 0) { //not snake
				drawSpecial(screen_buffer, x, y, grid[x][y], sprite_color, bg_color);
			}
			else { //snake body
				prev = 4;
				next = 4;
				for (i = 0; i < 4; ++i) {
					xx = x + x_dirs[i];
					yy = y + y_dirs[i];
					if (!isValidPos(xx, yy)) continue;
					if (grid[xx][yy] == grid[x][y] + 1) next = i;
					else if (grid[x][y] != 1 && grid[xx][yy] == grid[x][y] - 1) prev = i; // != 1 is exception for head
				}
				drawBody(screen_buffer, x, y, prev, next, sprite_color, bg_color);
			}
		}
	}
}

int update(struct sgs* game, char* screen_buffer, int do_movement) {
	int res;
	if (do_movement) {
		print("m(");
		print_n(game->head_x);
		print(",");
		print_n(game->head_y);
		print(")\n");
		res = move(game);
		if (res == -1) return -1;
		if (res == 1) {
			spawnFruit(game);
			print("s");
		}
	}
	//print("d");
	drawGame(game->grid, screen_buffer);
	return 0;
}

int poll_keyboard(struct sgs* game) {
	//SetPriority(2);
	
	char keyboard1[256];
	char keyboard2[256];
	int recent_keyboard = 1;
	int res;
	
	res = GetKeyboardState(keyboard1);
	res = GetKeyboardState(keyboard2);
	
	print("thread set up. entering thread loop\n");
	while (1) {
		print("polling keyboard\n");
		if (recent_keyboard == 1) {
			res = GetKeyboardState(keyboard2);
			updateSelectedDir(game, keyboard1, keyboard2);
			recent_keyboard = 2;
		}
		else {
			res = GetKeyboardState(keyboard1);
			updateSelectedDir(game, keyboard2, keyboard1);
			recent_keyboard = 1;
		}
		pause(100);
	}
}

char* screen_buffer;

int __attribute__ ((__section__(".text.main")))
main(void)
{
	int i, j;
    struct sgs game;
    for (i = 0; i < X_size; i++) {
		for (j = 0; j < Y_size; j++) {
			if (i == 0 || j == 0 || i == X_size - 1 || j == Y_size - 1)
				game.grid[i][j] = -1;
			else
				game.grid[i][j] = 0;
		}
	}
	game.length = 1;
	game.seed = 1;
	game.seed = getNextSeed(game.seed);
	
	game.head_x = 3;
	game.head_y = 3;
	game.dir = RIGHT;
	game.next_dir = RIGHT;
	
	game.alive = 1;
	
	print("initialized game\n");
	
	if (pthread_create(poll_keyboard, &game, 4096) == 0) {
		write(1, "error thread create\n", 20);
		exit();
	}
	
	screen_buffer = StartScreen();
	print("started screen. entering main loop\n");
	int cnt, do_move;
	cnt = 0;
	while(1) {
		++cnt;
		if (cnt == 100) {
			do_move = 1;
			cnt = 0;
		}
		else do_move = 0;
		if (update(&game, screen_buffer, do_move) == -1) {
			//dead
			break;
		}
	}
    return 0;
}
