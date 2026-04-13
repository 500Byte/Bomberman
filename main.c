#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX_FILAS 13
#define MAX_COLS 15
#define WINDOW_WIDTH 720
#define WINDOW_HEIGHT 624
#define TILE_SIZE 48
#define TILESET_SIZE 16
#define TILE_GAP 1
#define VELOCIDAD 2
#define MAX_BOMBAS 5
#define TIEMPO_EXPLOSION 2000
#define RADIO_EXPLOSION 2
#define TIEMPO_EXPLOSION_VISIBLE 500

typedef struct { int x; int y; } CoordTile;

#define MURO_PIEDRA    (CoordTile){33, 0}
#define PISO_METAL     (CoordTile){33, 1}
#define PASTO          (CoordTile){9, 0}
#define AGUA           (CoordTile){7, 1}
#define CAMINO_TIERRA  (CoordTile){4, 0}
#define LADRILLO_NARANJA (CoordTile){0, 9}
#define LADRILLO_GRIS  (CoordTile){1, 9}
#define ROCA_AGRIETADA (CoordTile){6, 0}

#define BOMBA_NORMAL   (CoordTile){0, 6}
#define BOMBA_PARP_1   (CoordTile){1, 6}
#define BOMBA_PARP_2   (CoordTile){2, 6}
#define BOMBA_ROJA     (CoordTile){1, 6}

#define EXP_CENTRO     (CoordTile){1, 8}
#define EXP_HORIZ      (CoordTile){2, 8}
#define EXP_VERT       (CoordTile){3, 8}
#define EXP_PUNTA_ARR  (CoordTile){3, 5}
#define EXP_PUNTA_ABJ  (CoordTile){3, 9}
#define EXP_PUNTA_IZQ  (CoordTile){1, 6}
#define EXP_PUNTA_DER  (CoordTile){5, 6}

typedef struct {
    bool activa;
    int x, y;
    unsigned int tiempo_inicio;
    bool explotada;
    int radio;
} Bomba;

typedef struct {
    int posX, posY;
    int direccion;
    int frame_animacion;
    bool moviendose;
    int max_bombas;
    int bombas_colocadas;
} Jugador;

typedef struct {
    int x, y;
    unsigned int tiempo_inicio;
    int radio;
} Explosion;

static ALLEGRO_DISPLAY *pantalla = NULL;
static ALLEGRO_TIMER *temporizador = NULL;
static ALLEGRO_EVENT_QUEUE *cola_eventos = NULL;
static ALLEGRO_BITMAP *tileset = NULL;
static ALLEGRO_BITMAP *bomberizq[3] = { NULL };
static ALLEGRO_BITMAP *bomberar[3] = { NULL };
static ALLEGRO_BITMAP *bomberder[3] = { NULL };
static ALLEGRO_BITMAP *bomberab[3] = { NULL };

static Jugador jugador;
static Bomba bombas[MAX_BOMBAS];
static Explosion explosiones[MAX_BOMBAS];
static bool teclas[ALLEGRO_KEY_MAX] = { false };
static bool redibujar = true;
static bool ejecutando = true;
static unsigned int milisegundos = 0;

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

static void dibujar_tile(CoordTile coords, int pantalla_x, int pantalla_y) {
    if (!tileset) return;
    
    al_draw_scaled_bitmap(tileset,
        coords.x * (TILESET_SIZE + TILE_GAP), coords.y * (TILESET_SIZE + TILE_GAP),
        TILESET_SIZE, TILESET_SIZE,
        pantalla_x, pantalla_y,
        TILE_SIZE, TILE_SIZE,
        0);
}

static void dibujar_tile_alpha(CoordTile coords, int x, int y, float alpha) {
    ALLEGRO_COLOR tinte = al_map_rgba_f(alpha, alpha, alpha, alpha);
    al_draw_tinted_scaled_bitmap(tileset, tinte,
        coords.x * (TILESET_SIZE + TILE_GAP), coords.y * (TILESET_SIZE + TILE_GAP),
        TILESET_SIZE, TILESET_SIZE,
        x, y, TILE_SIZE, TILE_SIZE,
        0);
}

static void inicializar_jugador(void) {
    jugador.posX = TILE_SIZE;
    jugador.posY = TILE_SIZE;
    jugador.direccion = 4;
    jugador.frame_animacion = 1;
    jugador.moviendose = false;
    jugador.max_bombas = 2;
    jugador.bombas_colocadas = 0;
}

static void inicializar_bombas(void) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        bombas[i].activa = false;
        explosiones[i].tiempo_inicio = 0;
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
            int jugador_col = (x + TILE_SIZE/2) / TILE_SIZE;
            int jugador_row = (y + TILE_SIZE/2) / TILE_SIZE;
            
            if (jugador_col == bomba_col && jugador_row == bomba_row) {
                int prev_col = (jugador.posX + TILE_SIZE/2) / TILE_SIZE;
                int prev_row = (jugador.posY + TILE_SIZE/2) / TILE_SIZE;
                if (prev_col != bomba_col || prev_row != bomba_row) {
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
            bombas[i].radio = RADIO_EXPLOSION;
            jugador.bombas_colocadas++;
            break;
        }
    }
}

static void crear_explosion(int centro_x, int centro_y, int radio) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (explosiones[i].tiempo_inicio == 0 || 
            milisegundos - explosiones[i].tiempo_inicio > TIEMPO_EXPLOSION_VISIBLE) {
            explosiones[i].x = centro_x;
            explosiones[i].y = centro_y;
            explosiones[i].radio = radio;
            explosiones[i].tiempo_inicio = milisegundos;
            break;
        }
    }
}

static void destruir_bloque(int row, int col) {
    if (row < 0 || row >= MAX_FILAS || col < 0 || col >= MAX_COLS) return;
    if (mapa[row][col] == 'O') {
        mapa[row][col] = ' ';
    }
}

static void explotar_bomba(int indice) {
    Bomba *b = &bombas[indice];
    if (b->explotada) return;
    
    b->explotada = true;
    jugador.bombas_colocadas--;
    
    int centro_col = b->x / TILE_SIZE;
    int centro_row = b->y / TILE_SIZE;
    int radio = b->radio;
    
    crear_explosion(b->x, b->y, radio);
    destruir_bloque(centro_row, centro_col);
    
    for (int r = 1; r <= radio; r++) {
        if (centro_row - r >= 0) {
            if (mapa[centro_row - r][centro_col] == 'X') break;
            destruir_bloque(centro_row - r, centro_col);
        }
    }
    for (int r = 1; r <= radio; r++) {
        if (centro_row + r < MAX_FILAS) {
            if (mapa[centro_row + r][centro_col] == 'X') break;
            destruir_bloque(centro_row + r, centro_col);
        }
    }
    for (int r = 1; r <= radio; r++) {
        if (centro_col - r >= 0) {
            if (mapa[centro_row][centro_col - r] == 'X') break;
            destruir_bloque(centro_row, centro_col - r);
        }
    }
    for (int r = 1; r <= radio; r++) {
        if (centro_col + r < MAX_COLS) {
            if (mapa[centro_row][centro_col + r] == 'X') break;
            destruir_bloque(centro_row, centro_col + r);
        }
    }
    
    b->activa = false;
}

static void actualizar_bombas(void) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (bombas[i].activa && !bombas[i].explotada) {
            if (milisegundos - bombas[i].tiempo_inicio >= TIEMPO_EXPLOSION) {
                explotar_bomba(i);
            }
        }
    }
}

static void dibujar_mapa(void) {
    for (int row = 0; row < MAX_FILAS; row++) {
        for (int col = 0; col < MAX_COLS; col++) {
            int x = col * TILE_SIZE;
            int y = row * TILE_SIZE;
            
            dibujar_tile(CAMINO_TIERRA, x, y);
            
            switch (mapa[row][col]) {
                case 'X':
                    dibujar_tile(MURO_PIEDRA, x, y);
                    break;
                case 'O':
                    dibujar_tile(LADRILLO_NARANJA, x, y);
                    break;
            }
        }
    }
}

static void dibujar_bombas(void) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (bombas[i].activa && !bombas[i].explotada) {
            unsigned int tiempo_restante = TIEMPO_EXPLOSION - (milisegundos - bombas[i].tiempo_inicio);
            CoordTile frame = BOMBA_NORMAL;
            
            if (tiempo_restante < 400) {
                frame = ((milisegundos / 50) % 2 == 0) ? BOMBA_ROJA : BOMBA_PARP_2;
            } else if (tiempo_restante < 800) {
                frame = ((milisegundos / 100) % 2 == 0) ? BOMBA_PARP_1 : BOMBA_NORMAL;
            } else if (tiempo_restante < 1200) {
                frame = BOMBA_PARP_1;
            }
            
            dibujar_tile(frame, bombas[i].x, bombas[i].y);
        }
    }
}

static void dibujar_explosiones(void) {
    for (int i = 0; i < MAX_BOMBAS; i++) {
        if (explosiones[i].tiempo_inicio == 0) continue;
        
        unsigned int tiempo_pasado = milisegundos - explosiones[i].tiempo_inicio;
        if (tiempo_pasado > TIEMPO_EXPLOSION_VISIBLE) {
            explosiones[i].tiempo_inicio = 0;
            continue;
        }
        
        int x = explosiones[i].x;
        int y = explosiones[i].y;
        int radio = explosiones[i].radio;
        
        float alpha = 1.0f - ((float)tiempo_pasado / TIEMPO_EXPLOSION_VISIBLE);
        
        dibujar_tile_alpha(EXP_CENTRO, x, y, alpha);
        
        for (int r = 1; r <= radio; r++) {
            int offset = r * TILE_SIZE;
            int col = x / TILE_SIZE;
            int row = y / TILE_SIZE;
            
            if (row - r >= 0 && mapa[row - r][col] != 'X') {
                CoordTile tile = (r == radio) ? EXP_PUNTA_ARR : EXP_VERT;
                dibujar_tile_alpha(tile, x, y - offset, alpha);
            }
            if (row + r < MAX_FILAS && mapa[row + r][col] != 'X') {
                CoordTile tile = (r == radio) ? EXP_PUNTA_ABJ : EXP_VERT;
                dibujar_tile_alpha(tile, x, y + offset, alpha);
            }
            if (col - r >= 0 && mapa[row][col - r] != 'X') {
                CoordTile tile = (r == radio) ? EXP_PUNTA_IZQ : EXP_HORIZ;
                dibujar_tile_alpha(tile, x - offset, y, alpha);
            }
            if (col + r < MAX_COLS && mapa[row][col + r] != 'X') {
                CoordTile tile = (r == radio) ? EXP_PUNTA_DER : EXP_HORIZ;
                dibujar_tile_alpha(tile, x + offset, y, alpha);
            }
        }
    }
}

static void dibujar_personaje(void) {
    if (jugador.moviendose) {
        jugador.frame_animacion = (milisegundos / 100) % 3;
    } else {
        jugador.frame_animacion = 1;
    }
    
    ALLEGRO_BITMAP *sprite = bomberab[1];
    switch (jugador.direccion) {
        case 1: sprite = bomberizq[jugador.frame_animacion]; break;
        case 2: sprite = bomberder[jugador.frame_animacion]; break;
        case 3: sprite = bomberar[jugador.frame_animacion]; break;
        case 4: sprite = bomberab[jugador.frame_animacion]; break;
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
        if (hay_colision(jugador.posX, jugador.posY)) jugador.posX = prev_x;
    }
    else if (teclas[ALLEGRO_KEY_RIGHT]) {
        jugador.direccion = 2;
        jugador.moviendose = true;
        jugador.posX += VELOCIDAD;
        if (hay_colision(jugador.posX, jugador.posY)) jugador.posX = prev_x;
    }
    
    if (teclas[ALLEGRO_KEY_UP]) {
        jugador.direccion = 3;
        jugador.moviendose = true;
        jugador.posY -= VELOCIDAD;
        if (hay_colision(jugador.posX, jugador.posY)) jugador.posY = prev_y;
    }
    else if (teclas[ALLEGRO_KEY_DOWN]) {
        jugador.direccion = 4;
        jugador.moviendose = true;
        jugador.posY += VELOCIDAD;
        if (hay_colision(jugador.posX, jugador.posY)) jugador.posY = prev_y;
    }
}

static void limpiar(void) {
    if (tileset) al_destroy_bitmap(tileset);
    for (int i = 0; i < 3; i++) {
        if (bomberizq[i]) al_destroy_bitmap(bomberizq[i]);
        if (bomberar[i]) al_destroy_bitmap(bomberar[i]);
        if (bomberder[i]) al_destroy_bitmap(bomberder[i]);
        if (bomberab[i]) al_destroy_bitmap(bomberab[i]);
    }
    if (temporizador) al_destroy_timer(temporizador);
    if (cola_eventos) al_destroy_event_queue(cola_eventos);
    if (pantalla) al_destroy_display(pantalla);
    al_shutdown_image_addon();
    al_shutdown_native_dialog_addon();
}

static ALLEGRO_BITMAP *cargar_sprite(const char *path) {
    al_set_new_bitmap_flags(ALLEGRO_VIDEO_BITMAP);
    
    ALLEGRO_BITMAP *bmp = al_load_bitmap(path);
    if (!bmp) {
        fprintf(stderr, "Error fatal: No se pudo cargar la imagen '%s'\n", path);
        if (pantalla && al_is_native_dialog_addon_initialized()) {
            al_show_native_message_box(pantalla, "Error", "Error al cargar", path, NULL, ALLEGRO_MESSAGEBOX_ERROR);
        }
        limpiar();
        exit(EXIT_FAILURE);
    }
    
    al_convert_mask_to_alpha(bmp, al_map_rgb(255, 0, 255));
    
    return bmp;
}

static void cargar_recursos(void) {
    tileset = cargar_sprite("Sprites/tileset.bmp");
    
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

int main(void) {
    if (!al_init()) {
        fprintf(stderr, "Error: No se pudo inicializar Allegro.\n");
        return EXIT_FAILURE;
    }
    if (!al_init_image_addon()) {
        fprintf(stderr, "Error: No se pudo inicializar el addon de imagenes.\n");
        return EXIT_FAILURE;
    }
    if (!al_install_keyboard()) {
        fprintf(stderr, "Error: No se pudo instalar el teclado.\n");
        return EXIT_FAILURE;
    }

    pantalla = al_create_display(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!pantalla) {
        fprintf(stderr, "Error: No se pudo crear la pantalla de %dx%d.\n", WINDOW_WIDTH, WINDOW_HEIGHT);
        limpiar(); return EXIT_FAILURE; 
    }

    if (!al_init_native_dialog_addon()) {
        fprintf(stderr, "Nota: El soporte de diálogos nativos no está disponible. Se usará la consola para errores.\n");
    }

    temporizador = al_create_timer(1.0 / 60.0);
    if (!temporizador) {
        fprintf(stderr, "Error: No se pudo crear el temporizador.\n");
        limpiar(); return EXIT_FAILURE; 
    }

    cola_eventos = al_create_event_queue();
    if (!cola_eventos) {
        fprintf(stderr, "Error: No se pudo crear la cola de eventos.\n");
        limpiar(); return EXIT_FAILURE; 
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
            if (evento.keyboard.keycode == ALLEGRO_KEY_ESCAPE) ejecutando = false;
            else if (evento.keyboard.keycode == ALLEGRO_KEY_SPACE) colocar_bomba();
        }
        else if (evento.type == ALLEGRO_EVENT_KEY_UP) {
            teclas[evento.keyboard.keycode] = false;
        }
        else if (evento.type == ALLEGRO_EVENT_TIMER) {
            milisegundos += 16;
            actualizar_movimiento();
            actualizar_bombas();
            redibujar = true;
        }

        if (redibujar && al_is_event_queue_empty(cola_eventos)) {
            redibujar = false;
            al_clear_to_color(al_map_rgb(0, 0, 0));
            
            dibujar_mapa();
            dibujar_bombas();
            dibujar_explosiones();
            dibujar_personaje();
            
            al_flip_display();
        }
    }

    limpiar();
    return EXIT_SUCCESS;
}