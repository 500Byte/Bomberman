#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdlib.h>

#define MAX_FILAS 13
#define MAX_COLS 15
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 624
#define TILE_SIZE 48

static ALLEGRO_DISPLAY *pantalla = NULL;
static ALLEGRO_TIMER *temporizador = NULL;
static ALLEGRO_EVENT_QUEUE *cola_eventos = NULL;
static ALLEGRO_BITMAP *terreno[3] = { NULL };
static ALLEGRO_BITMAP *bomberizq[3] = { NULL };
static ALLEGRO_BITMAP *bomberar[3] = { NULL };
static ALLEGRO_BITMAP *bomberder[3] = { NULL };
static ALLEGRO_BITMAP *bomberab[3] = { NULL };

static int posX = TILE_SIZE;
static int posY = TILE_SIZE;
static int direccion = 0;
static bool teclas[ALLEGRO_KEY_MAX] = { false };
static bool redibujar = true;
static bool ejecutando = true;
static unsigned int milisegundos = 0;

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
    for (int row = 0; row < MAX_FILAS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            ALLEGRO_BITMAP *tile = terreno[0];
            if (mapa[row][col] == 'X') {
                tile = terreno[2];
            } else if (mapa[row][col] == 'O') {
                tile = terreno[1];
            }
            al_draw_bitmap(tile, col * TILE_SIZE, row * TILE_SIZE, 0);
        }
    }
}

static void limpiar(void) {
    for (int i = 0; i < 3; i++) {
        if (terreno[i]) {
            al_destroy_bitmap(terreno[i]);
            terreno[i] = NULL;
        }
        if (bomberizq[i]) {
            al_destroy_bitmap(bomberizq[i]);
            bomberizq[i] = NULL;
        }
        if (bomberar[i]) {
            al_destroy_bitmap(bomberar[i]);
            bomberar[i] = NULL;
        }
        if (bomberder[i]) {
            al_destroy_bitmap(bomberder[i]);
            bomberder[i] = NULL;
        }
        if (bomberab[i]) {
            al_destroy_bitmap(bomberab[i]);
            bomberab[i] = NULL;
        }
    }

    if (temporizador) {
        al_destroy_timer(temporizador);
        temporizador = NULL;
    }

    if (cola_eventos) {
        al_destroy_event_queue(cola_eventos);
        cola_eventos = NULL;
    }

    if (pantalla) {
        al_destroy_display(pantalla);
        pantalla = NULL;
    }

    al_shutdown_image_addon();
    al_shutdown_native_dialog_addon();
}

static ALLEGRO_BITMAP *cargar_sprite(const char *path) {
    ALLEGRO_BITMAP *bmp = al_load_bitmap(path);
    if (!bmp) {
        al_show_native_message_box(pantalla, "Error", "Error al cargar imagen", path, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        limpiar();
        exit(EXIT_FAILURE);
    }
    return bmp;
}

static void cargar_recursos(void) {
    terreno[0] = cargar_sprite("Sprites/Terreno/block00.bmp");
    terreno[1] = cargar_sprite("Sprites/Terreno/block01.bmp");
    terreno[2] = cargar_sprite("Sprites/Terreno/block02.bmp");

    bomberizq[0] = cargar_sprite("Sprites/Bomberman/izq00.bmp");
    bomberizq[1] = cargar_sprite("Sprites/Bomberman/izq01.bmp");
    bomberizq[2] = cargar_sprite("Sprites/Bomberman/izq02.bmp");

    bomberar[0] = cargar_sprite("Sprites/Bomberman/ar00.bmp");
    bomberar[1] = cargar_sprite("Sprites/Bomberman/ar01.bmp");
    bomberar[2] = cargar_sprite("Sprites/Bomberman/ar02.bmp");

    bomberder[0] = cargar_sprite("Sprites/Bomberman/der00.bmp");
    bomberder[1] = cargar_sprite("Sprites/Bomberman/der01.bmp");
    bomberder[2] = cargar_sprite("Sprites/Bomberman/der02.bmp");

    bomberab[0] = cargar_sprite("Sprites/Bomberman/ab00.bmp");
    bomberab[1] = cargar_sprite("Sprites/Bomberman/ab01.bmp");
    bomberab[2] = cargar_sprite("Sprites/Bomberman/ab02.bmp");
}

static void dibujar_personaje(void) {
    int frame = (milisegundos / 100) % 3;
    ALLEGRO_BITMAP *sprite = bomberab[1];

    switch (direccion) {
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

    al_draw_bitmap(sprite, posX, posY, 0);
}

int main(void) {
    if (!al_init()) {
        return EXIT_FAILURE;
    }

    if (!al_init_image_addon()) {
        return EXIT_FAILURE;
    }

    if (!al_init_native_dialog_addon()) {
        return EXIT_FAILURE;
    }

    if (!al_install_keyboard()) {
        return EXIT_FAILURE;
    }

    pantalla = al_create_display(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!pantalla) {
        limpiar();
        return EXIT_FAILURE;
    }

    temporizador = al_create_timer(1.0 / 60.0);
    if (!temporizador) {
        limpiar();
        return EXIT_FAILURE;
    }

    cola_eventos = al_create_event_queue();
    if (!cola_eventos) {
        limpiar();
        return EXIT_FAILURE;
    }

    al_register_event_source(cola_eventos, al_get_display_event_source(pantalla));
    al_register_event_source(cola_eventos, al_get_keyboard_event_source());
    al_register_event_source(cola_eventos, al_get_timer_event_source(temporizador));

    cargar_recursos();

    al_start_timer(temporizador);

    while (ejecutando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(cola_eventos, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            ejecutando = false;
        } else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[evento.keyboard.keycode] = true;
            if (evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                ejecutando = false;
            }
        } else if (evento.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[evento.keyboard.keycode] = false;
        } else if (evento.type == ALLEGRO_EVENT_TIMER) {
            milisegundos += 16;
            if (!teclas[ALLEGRO_KEY_LEFT] && !teclas[ALLEGRO_KEY_RIGHT] && !teclas[ALLEGRO_KEY_UP] && !teclas[ALLEGRO_KEY_DOWN]) {
                direccion = 0;
            }
            if (teclas[ALLEGRO_KEY_LEFT]) {
                direccion = 1;
                posX -= 2;
            }
            if (teclas[ALLEGRO_KEY_RIGHT]) {
                direccion = 2;
                posX += 2;
            }
            if (teclas[ALLEGRO_KEY_UP]) {
                direccion = 3;
                posY -= 2;
            }
            if (teclas[ALLEGRO_KEY_DOWN]) {
                direccion = 4;
                posY += 2;
            }
            redibujar = true;
        }

        if (redibujar && al_is_event_queue_empty(cola_eventos)) {
            redibujar = false;
            dibujar_mapa();
            dibujar_personaje();
            al_flip_display();
        }
    }

    limpiar();
    return EXIT_SUCCESS;
}
