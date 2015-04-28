#include <allegro.h>
#define MAXFILAS 13 /*Para el eje Y*/
#define MAXCOLS 15 /*Para el eje X*/

volatile int ticks;
void tick_counter() {
	ticks++;
}
END_OF_FUNCTION(tick_counter)

volatile int milisegundos;
void msec_counter() {
	milisegundos++;
}
END_OF_FUNCTION(msec_counter)

BITMAP *buffer;
int posX = 48, posY = 48;
int dir;

char mapa [MAXFILAS][MAXCOLS] = {
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

void dibujar_mapa() {
	int row, col;

	BITMAP *Terreno[3];
	Terreno[0] = load_bitmap("Sprites/Terreno/block00.bmp", NULL);
	Terreno[1] = load_bitmap("Sprites/Terreno/block01.bmp", NULL);
	Terreno[2] = load_bitmap("Sprites/Terreno/block02.bmp", NULL);

	for(row = 0; row < MAXFILAS; row++) {
		for(col = 0; col < MAXCOLS; col++) {
			if(mapa[row][col] == 'X') {
				draw_sprite(buffer, Terreno[2], col*48, row*48);
			}
			if(mapa[row][col] == ' ') {
				draw_sprite(buffer, Terreno[0], col*48, row*48);
			}
			if(mapa[row][col] == 'O') {
				draw_sprite(buffer, Terreno[1], col*48, row*48);
			}
		}
	}
}

void pantalla() {
	blit(buffer, screen , 0, 0, 0, 0, 720, 624);
}

void dibujar_personaje() {
	int num_frames = 3;
	int frame_atual;
	int tempo_troca = 100;

	frame_atual = (milisegundos / tempo_troca) % num_frames;

	BITMAP *Bomberizq[4];
	Bomberizq[0] = load_bitmap("Sprites/Bomberman/izq00.bmp", NULL);
	Bomberizq[1] = load_bitmap("Sprites/Bomberman/izq01.bmp", NULL);
	Bomberizq[2] = load_bitmap("Sprites/Bomberman/izq02.bmp", NULL);

	BITMAP *Bomberar[4];
	Bomberar[0] = load_bitmap("Sprites/Bomberman/ar00.bmp", NULL);
	Bomberar[1] = load_bitmap("Sprites/Bomberman/ar01.bmp", NULL);
	Bomberar[2] = load_bitmap("Sprites/Bomberman/ar02.bmp", NULL);

	BITMAP *Bomberder[4];
	Bomberder[0] = load_bitmap("Sprites/Bomberman/der00.bmp", NULL);
	Bomberder[1] = load_bitmap("Sprites/Bomberman/der01.bmp", NULL);
	Bomberder[2] = load_bitmap("Sprites/Bomberman/der02.bmp", NULL);

	BITMAP *Bomberab[4];
	Bomberab[0] = load_bitmap("Sprites/Bomberman/ab00.bmp", NULL);
	Bomberab[1] = load_bitmap("Sprites/Bomberman/ab01.bmp", NULL);
	Bomberab[2] = load_bitmap("Sprites/Bomberman/ab02.bmp", NULL);

	BITMAP *Bomb[4];
	Bomb[0] = load_bitmap("Sprites/Bomberman/bomb00.bmp", NULL);
	Bomb[1] = load_bitmap("Sprites/Bomberman/bomb01.bmp", NULL);
	Bomb[2] = load_bitmap("Sprites/Bomberman/bomb02.bmp", NULL);

	if (dir==0) {
		draw_sprite(buffer, Bomberab[1], posX, posY);
	}
	if (dir==1) {
		draw_sprite(buffer, Bomberizq[frame_atual], posX, posY);
	}
	if (dir==2) {
		draw_sprite(buffer, Bomberder[frame_atual], posX, posY);
	}
	if (dir==3) {
		draw_sprite(buffer, Bomberar[frame_atual], posX, posY);
	}
	if (dir==4) {
		draw_sprite(buffer, Bomberab[frame_atual], posX, posY);
	}

}

/* void dibujar_bomba() {
	blit(BombSprite, Bomb, 20, 0, 0, 0, 20, 20);
	draw_sprite(buffer, Bomb, posX, posY);
}*/

int main() {
	allegro_init();
	install_keyboard();
	install_timer();

	set_color_depth(32);
	set_gfx_mode(GFX_AUTODETECT_WINDOWED, 720, 624, 0, 0);

	ticks = 0;
	LOCK_FUNCTION(tick_counter);
	LOCK_VARIABLE(ticks);
	install_int_ex(tick_counter, BPS_TO_TIMER(30));

	milisegundos = 0;
	LOCK_FUNCTION(msec_counter);
	LOCK_VARIABLE(milisegundos);
	install_int_ex(msec_counter, MSEC_TO_TIMER(1));

	buffer = create_bitmap(768, 624);

	while(!key[KEY_ESC]) {
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
		clear(buffer);
		dibujar_mapa();
		dibujar_personaje();
		pantalla();
	}
}
END_OF_MAIN();
