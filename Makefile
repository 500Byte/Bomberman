CC = gcc
PKG_CONFIG = pkg-config
ALLEGRO_CFLAGS := $(shell $(PKG_CONFIG) --cflags allegro-5 allegro_image-5 allegro_dialog-5)
ALLEGRO_LIBS := $(shell $(PKG_CONFIG) --libs allegro-5 allegro_image-5 allegro_dialog-5)
CFLAGS = -Wall -Wextra -std=c99 $(ALLEGRO_CFLAGS)
LDFLAGS = $(ALLEGRO_LIBS) -lm
SRC = main.c
OBJ = $(SRC:.c=.o)
BIN = bomberman

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)
