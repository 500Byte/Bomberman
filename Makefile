CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags allegro-5 allegro_image-5 allegro_dialog-5 allegro_main-5)
LIBS = $(shell pkg-config --libs allegro-5 allegro_image-5 allegro_dialog-5 allegro_main-5)

TARGET = bomberman

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c $(LIBS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)