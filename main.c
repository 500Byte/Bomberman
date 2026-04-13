#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_FILAS 13
#define MAX_COLS 15
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 624
#define TILE_SIZE 48
#define VELOCIDAD 2
#define MAX_BOMBAS 5
#define TIEMPO_EXPLOSION 2000
#define RADIO_EXPLOSION 1

typedef struct {
    bool activa;
    int x;
    int y;
    unsigned int tiempo_inicio;
    bool explotada;
} Bomba;

typedef struct {
    int posX;
    int posY;
    int direccion;  // 0=quieto, 1=izq, 2=der, 3=arriba, 4=abajo
    int frame_animacion;
    bool moviendose;
    int radio_bomba;
    int max_bombas;
    int bombas_colocadas;
} Jugador;

static ALLEGRO_DISPLAY *pantalla = NULL;
static ALLEGRO_TIMER *temporizador = NULL;
static ALLEGRO_EVENT_QUEUE *cola_eventos = NULL;
static ALLEGRO_BITMAP *terreno[3] = { NULL };
static ALLEGRO_BITMAP *bomberizq[3] = { NULL };
static ALLEGRO_BITMAP *bomberar[3] = { NULL };
static ALLEGRO_BITMAP *bomberder[3] = { NULL };
static ALLEGRO_BITMAP *bomberab[3] = { NULL };
static ALLEGRO_BITMAP *sprite_bomba = NULL;
static ALLEGRO_BITMAP *sprite_explosion = NULL;

static Jugador jugador;
static Bomba bombas[MAX_BOMBAS];
static bool teclas[ALLEGRO_KEY_MAX] = { false };
static bool redibujar = true;
static bool ejecutando = true;
static unsigned int milisegundos = 0;

// Mapa con destructibles ('O')
char mapa[MAX_FILAS][MAX_COLS + 1] = {
    "XXXXXXXXXXXXXXX",
    "XO O O O O O OX",
    "XOXOXOXOXOXOXOX",
    "X O O O O O O X",
    "XOXOXOXOXOXOXOX",
    "X O O O O O O X",
    "XOXOXOXOXOXOXOX",
    "X O O O O O O X",
    "XOXOXOXOXOXOXOX",
    "X O O O O O O X",
    "XOXOXOXOXOXOXOX",
    "XO O O O O O OX",
    "XXXXXXXXXXXXXXX"
};

static void inicializar_jugador(void) {
    jugador.posX = TILE_SIZE;
    jugador.posY = TILE_SIZE;
    jugador.direccion = 4;
    jugador.frame_animacion = 1;
    jugador.moviendose = false;
    jugador.radio_bomba = RADIO_EXPLOSION;
    jugador.max_bombas = 1;
    jugador.bombas_colocadas = 0;
}

static void inicializar_bombas(void) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        bombas[i].activa = false;
        bombas[i].x = 0;
        bombas[i].y = 0;
        bombas[i].tiempo_inicio = 0;
        bombas[i].explotada = false;
    }
}

static bool hay_colision(int x, int y) {
    if (x < 0 || x > WINDOW_WIDTH - TILE_SIZE || 
        y < 0 || y > WINDOW_HEIGHT - TILE_SIZE) {
        return true;
    }
    
    int col_left = x / TILE_SIZE;
    int col_right = (x + TILE_SIZE - 1) / TILE_SIZE;
    int row_top = y / TILE_SIZE;
    int row_bottom = (y + TILE_SIZE - 1) / TILE_SIZE;
    
    if (mapa[row_top][col_left] == 'X' || mapa[row_top][col_left] == 'O' ||
        mapa[row_top][col_right] == 'X' || mapa[row_top][col_right] == 'O' ||
        mapa[row_bottom][col_left] == 'X' || mapa[row_bottom][col_left] == 'O' ||
        mapa[row_bottom][col_right] == 'X' || mapa[row_bottom][col_right] == 'O') {
        return true;
    }
    
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (bombas[i].activa && !bombas[i].explotada) {
            int bomba_col = bombas[i].x / TILE_SIZE;
            int bomba_row = bombas[i].y / TILE_SIZE;
            
            int jugador_col_central = (x + TILE_SIZE/2) / TILE_SIZE;
            int jugador_row_central = (y + TILE_SIZE/2) / TILE_SIZE;
            
            if (jugador_col_central == bomba_col && jugador_row_central == bomba_row) {
                if (jugador_col_central != (jugador.posX + TILE_SIZE/2) / TILE_SIZE ||
                    jugador_row_central != (jugador.posY + TILE_SIZE/2) / TILE_SIZE) {
                    return true;
                }
            }
        }
    }
    
    return false;
}

static void colocar_bomba(void) {
    if (jugador.bombas_colocadas >= jugador.max_bombas) return;
    
    int col = (jugador.posX + TILE_SIZE/2) / TILE_SIZE;
    int row = (jugador.posY + TILE_SIZE/2) / TILE_SIZE;
    
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (bombas[i].activa && !bombas[i].explotada) {
            if (bombas[i].x / TILE_SIZE == col && bombas[i].y / TILE_SIZE == row) {
                return;
            }
        }
    }
    
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (!bombas[i].activa) {
            bombas[i].activa = true;
            bombas[i].x = col * TILE_SIZE;
            bombas[i].y = row * TILE_SIZE;
            bombas[i].tiempo_inicio = milisegundos;
            bombas[i].explotada = false;
            jugador.bombas_colocadas++;
            break;
        }
    }
}

static void actualizar_bombas(void) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (bombas[i].activa && !bombas[i].explotada) {
            if (milisegundos - bombas[i].tiempo_inicio >= TIEMPO_EXPLOSION) {
                bombas[i].explotada = true;
                
                int centro_col = bombas[i].x / TILE_SIZE;
                int centro_row = bombas[i].y / TILE_SIZE;
                
                for (int r = 0; r <= jugador.radio_bomba; r++) {
                    if (centro_row - r >= 0) {
                        if (mapa[centro_row - r][centro_col] == 'O') {
                            mapa[centro_row - r][centro_col] = ' ';
                        }
                    }
                    if (centro_row + r < MAX_FILAS) {
                        if (mapa[centro_row + r][centro_col] == 'O') {
                            mapa[centro_row + r][centro_col] = ' ';
                        }
                    }
                    if (centro_col - r >= 0) {
                        if (mapa[centro_row][centro_col - r] == 'O') {
                            mapa[centro_row][centro_col - r] = ' ';
                        }
                    }
                    if (centro_col + r < MAX_COLS) {
                        if (mapa[centro_row][centro_col + r] == 'O') {
                            mapa[centro_row][centro_col + r] = ' ';
                        }
                    }
                }
                
                bombas[i].activa = false;
                jugador.bombas_colocadas--;
            }
        }
    }
}

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

static void dibujar_bombas(void) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (bombas[i].activa) {
            unsigned int tiempo_restante = TIEMPO_EXPLOSION - (milisegundos - bombas[i].tiempo_inicio);
            bool visible = true;
            
            if (tiempo_restante < 500) {
                visible = (milisegundos / 50) % 2 == 0;
            } else if (tiempo_restante < 1000) {
                visible = (milisegundos / 100) % 2 == 0;
            }
            
            if (visible && sprite_bomba) {
                al_draw_bitmap(sprite_bomba, bombas[i].x, bombas[i].y, 0);
            } else if (visible) {
                // Dibujar un círculo temporal si no hay sprite
                al_draw_filled_circle(bombas[i].x + TILE_SIZE/2, bombas[i].y + TILE_SIZE/2, 
                                    TILE_SIZE/3, al_map_rgb(0, 0, 0));
            }
        }
    }
}

static void dibujar_personaje(void) {
    if (jugador.moviendose) {
        jugador.frame_animacion = (milisegundos / 100) % 3;
    } else {
        jugador.frame_animacion = 1;  // Frame idle
    }
    
    ALLEGRO_BITMAP *sprite = bomberab[1];  // Default mirando abajo idle

    switch (jugador.direccion) {
    case 1:
        sprite = bomberizq[jugador.frame_animacion];
        break;
    case 2:
        sprite = bomberder[jugador.frame_animacion];
        break;
    case 3:
        sprite = bomberar[jugador.frame_animacion];
        break;
    case 4:
        sprite = bomberab[jugador.frame_animacion];
        break;
    default:
        switch (jugador.direccion) {
        case 1: sprite = bomberizq[1]; break;
        case 2: sprite = bomberder[1]; break;
        case 3: sprite = bomberar[1]; break;
        case 4: sprite = bomberab[1]; break;
        }
        break;
    }

    al_draw_bitmap(sprite, jugador.posX, jugador.posY, 0);
}

static void actualizar_movimiento(void) {
    jugador.moviendose = false;
    
    int prev_x = jugador.posX;
    int prev_y = jugador.posY;
    
    if (teclas[ALLEGRO_KEY_LEFT]) {
        jugador.direccion = 1;
        jugador.moviendose = true;
        jugador.posX -= VELOCIDAD;
        if (hay_colision(jugador.posX, jugador.posY)) {
            jugador.posX = prev_x;
        }
    }
    else if (teclas[ALLEGRO_KEY_RIGHT]) {
        jugador.direccion = 2;
        jugador.moviendose = true;
        jugador.posX += VELOCIDAD;
        if (hay_colision(jugador.posX, jugador.posY)) {
            jugador.posX = prev_x;
        }
    }
    
    if (teclas[ALLEGRO_KEY_UP]) {
        jugador.direccion = 3;
        jugador.moviendose = true;
        jugador.posY -= VELOCIDAD;
        if (hay_colision(jugador.posX, jugador.posY)) {
            jugador.posY = prev_y;
        }
    }
    else if (teclas[ALLEGRO_KEY_DOWN]) {
        jugador.direccion = 4;
        jugador.moviendose = true;
        jugador.posY += VELOCIDAD;
        if (hay_colision(jugador.posX, jugador.posY)) {
            jugador.posY = prev_y;
        }
    }
    
    if (!jugador.moviendose) {
        jugador.direccion = 0;
    }
}

static void limpiar(void) {
    for (int i = 0; i < 3; i++) {
        if (terreno[i]) al_destroy_bitmap(terreno[i]);
        if (bomberizq[i]) al_destroy_bitmap(bomberizq[i]);
        if (bomberar[i]) al_destroy_bitmap(bomberar[i]);
        if (bomberder[i]) al_destroy_bitmap(bomberder[i]);
        if (bomberab[i]) al_destroy_bitmap(bomberab[i]);
    }
    
    if (sprite_bomba) al_destroy_bitmap(sprite_bomba);
    if (sprite_explosion) al_destroy_bitmap(sprite_explosion);

    if (temporizador) al_destroy_timer(temporizador);
    if (cola_eventos) al_destroy_event_queue(cola_eventos);
    if (pantalla) al_destroy_display(pantalla);

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
    terreno[0] = cargar_sprite("Sprites/Terreno/block00.bmp");  // Suelo
    terreno[1] = cargar_sprite("Sprites/Terreno/block01.bmp");  // Destruible
    terreno[2] = cargar_sprite("Sprites/Terreno/block02.bmp");  // Pared
    
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
    
    sprite_bomba = al_load_bitmap("Sprites/Terreno/bomba.bmp");
}

int main(void) {
    if (!al_init()) {
        return EXIT_FAILURE;
    }

    if (!al_init_image_addon()) {
        return EXIT_FAILURE;
    }

    if (!al_init_primitives_addon()) {
        fprintf(stderr, "Error al inicializar allegro_primitives\n");
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

    inicializar_jugador();
    inicializar_bombas();
    cargar_recursos();

    al_start_timer(temporizador);

    while (ejecutando) {
        ALLEGRO_EVENT evento;
        al_wait_for_event(cola_eventos, &evento);

        if (evento.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            ejecutando = false;
        } 
        else if (evento.type == ALLEGRO_EVENT_KEY_DOWN) {
            teclas[evento.keyboard.keycode] = true;
            
            if (evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                ejecutando = false;
            }
            else if (evento.keyboard.keycode == ALLEGRO_KEY_SPACE) {
                colocar_bomba();
            }
        } 
        else if (evento.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[evento.keyboard.keycode] = false;
        } 
        else if (evento.type == ALLEGRO_EVENT_TIMER) {
            milisegundos += 16;  // Aproximadamente 16ms por frame a 60fps
            
            actualizar_movimiento();
            actualizar_bombas();
            
            redibujar = true;
        }

        if (redibujar && al_is_event_queue_empty(cola_eventos)) {
            redibujar = false;
            
            al_clear_to_color(al_map_rgb(0, 0, 0));
            
            dibujar_mapa();
            dibujar_bombas();
            dibujar_personaje();
            
            al_flip_display();
        }
    }

    limpiar();
    return EXIT_SUCCESS;
}