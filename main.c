#include <allegro.h>
#include <stdlib.h>

#define MAX_FILAS 13
#define MAX_COLS 15
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 624
#define TILE_SIZE 48

volatile int milisegundos;
volatile int quit = 0;

void msec_counter(void) {
    milisegundos++;
}
END_OF_FUNCTION(msec_counter)

void close_button(void) {
    quit = 1;
}
END_OF_FUNCTION(close_button)

BITMAP *buffer = NULL;
BITMAP *terreno[3] = { NULL };
BITMAP *bomberizq[3] = { NULL };
BITMAP *bomberar[3] = { NULL };
BITMAP *bomberder[3] = { NULL };
BITMAP *bomberab[3] = { NULL };

int posX = TILE_SIZE;
int posY = TILE_SIZE;
int dir = 0;

const char mapa[MAX_FILAS][MAX_COLS + 1] = {
    "XXXXXXXXXXXXXXX",
    "X             X",
    "X X X X X X X X",
    "X             X",
    "X X X X X X X X",
    "X             X",
    "X X X X X X X X",
    "X             X",
    "X X X X X X X X",
    "X             X",
    "X X X X X X X X",
    "X             X",
    "XXXXXXXXXXXXXXX"
};

static void dibujar_mapa(void) {
    int row, col;

    for (row = 0; row < MAX_FILAS; row++) {
        for (col = 0; col < MAX_COLS; col++) {
            BITMAP *tile = terreno[0];
            if (mapa[row][col] == 'X') {
                tile = terreno[2];
            } else if (mapa[row][col] == 'O') {
                tile = terreno[1];
            }
            draw_sprite(buffer, tile, col * TILE_SIZE, row * TILE_SIZE);
        }
    }
}

static void cleanup(void) {
    int i;

    destroy_bitmap(buffer);
    for (i = 0; i < 3; i++) {
        destroy_bitmap(terreno[i]);
        destroy_bitmap(bomberizq[i]);
        destroy_bitmap(bomberar[i]);
        destroy_bitmap(bomberder[i]);
        destroy_bitmap(bomberab[i]);
    }
}

static BITMAP *load_sprite(const char *path) {
    BITMAP *bmp = load_bitmap(path, NULL);
    if (!bmp) {
        allegro_message("No se pudo cargar %s", path);
        cleanup();
        exit(EXIT_FAILURE);
    }
    return bmp;
}

static void cargar_recursos(void) {
    terreno[0] = load_sprite("Sprites/Terreno/block00.bmp");
    terreno[1] = load_sprite("Sprites/Terreno/block01.bmp");
    terreno[2] = load_sprite("Sprites/Terreno/block02.bmp");

    bomberizq[0] = load_sprite("Sprites/Bomberman/izq00.bmp");
    bomberizq[1] = load_sprite("Sprites/Bomberman/izq01.bmp");
    bomberizq[2] = load_sprite("Sprites/Bomberman/izq02.bmp");

    bomberar[0] = load_sprite("Sprites/Bomberman/ar00.bmp");
    bomberar[1] = load_sprite("Sprites/Bomberman/ar01.bmp");
    bomberar[2] = load_sprite("Sprites/Bomberman/ar02.bmp");

    bomberder[0] = load_sprite("Sprites/Bomberman/der00.bmp");
    bomberder[1] = load_sprite("Sprites/Bomberman/der01.bmp");
    bomberder[2] = load_sprite("Sprites/Bomberman/der02.bmp");

    bomberab[0] = load_sprite("Sprites/Bomberman/ab00.bmp");
    bomberab[1] = load_sprite("Sprites/Bomberman/ab01.bmp");
    bomberab[2] = load_sprite("Sprites/Bomberman/ab02.bmp");
}

static void dibujar_personaje(void) {
    int frame = (milisegundos / 100) % 3;
    BITMAP *sprite = bomberab[1];

    switch (dir) {
    case 1:
        sprite = bomberizq[frame];
        break;
    case 2:
        sprite = bomberder[frame];
        break;
    case 3:
        sprite = bomberar[frame];
        break;
    case 4:
        sprite = bomberab[frame];
        break;
    default:
        sprite = bomberab[1];
        break;
    }

    draw_sprite(buffer, sprite, posX, posY);
}

/* void dibujar_bomba() {
	blit(BombSprite, Bomb, 20, 0, 0, 0, 20, 20);
	draw_sprite(buffer, Bomb, posX, posY);
}*/

int main(void) {
    allegro_init();

    if (install_keyboard() != 0) {
        allegro_message("No se pudo inicializar el teclado");
        return EXIT_FAILURE;
    }

    install_timer();

    set_color_depth(32);
    if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0) != 0) {
        allegro_message("Error al iniciar modo gráfico: %s", allegro_error);
        return EXIT_FAILURE;
    }

    LOCK_VARIABLE(milisegundos);
    LOCK_FUNCTION(msec_counter);
    install_int_ex(msec_counter, MSEC_TO_TIMER(1));

    LOCK_VARIABLE(quit);
    LOCK_FUNCTION(close_button);
    set_close_button_callback(close_button);

    buffer = create_bitmap(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!buffer) {
        allegro_message("No se pudo crear el buffer");
        return EXIT_FAILURE;
    }

    cargar_recursos();

	while(!quit && !key[KEY_ESC]) {
		if (!key[KEY_LEFT] && !key[KEY_RIGHT] && !key[KEY_UP] && !key[KEY_DOWN]) {
			dir = 0;
		}
		if (key[KEY_LEFT]) {
			dir = 1;
			posX -=2;
			//if (mapa[(posY+48)/48][posX/48] != 'X') posX -= 2;
		}
		if (key[KEY_RIGHT]) {
			dir = 2;
			posX +=2;
			//if (mapa[(posY-48)/48][posX/48] != 'X') posX += 2;
		}
		if (key[KEY_UP]) {
			dir = 3;
			posY -=2;
			//if (mapa[posY/48][(posX+48)/48] != 'X') posY -= 2;
		}
		if (key[KEY_DOWN]) {
			dir = 4;
			posY += 2;
			//if (mapa[posY/48][(posX-48)/48] != 'X') posY += 2;
		}
		if (key[KEY_SPACE]) {
			//dibujar_bomba();
		}
		clear_bitmap(buffer);
		dibujar_mapa();
		dibujar_personaje();
		blit(buffer, screen, 0, 0, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
		rest(10);
	}

	cleanup();
	return EXIT_SUCCESS;
}
END_OF_MAIN();
