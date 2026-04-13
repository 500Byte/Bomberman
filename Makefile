CC = gcc
CFLAGS = -Wall -Wextra -std=c99
ALLEGRO_CFLAGS := $(shell allegro-config --cflags)
ALLEGRO_LIBS := $(shell allegro-config --libs)
LDFLAGS = $(ALLEGRO_LIBS) -lm -ldl -lpthread
SRC = main.c
OBJ = $(SRC:.c=.o)
BIN = bomberman

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(ALLEGRO_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)
