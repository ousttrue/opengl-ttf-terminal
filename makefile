

CFLAGS += -g -Wall
CFLAGS += -I./libtsm/src/tsm -I./libtsm/src/shared -I./libtsm
CFLAGS += `sdl-config --cflags`

LFLAGS += `sdl-config --libs`
LFLAGS += -lm -lGL

TSM = $(wildcard libtsm/src/tsm/*.c) $(wildcard libtsm/external/*.c) $(wildcard libtsm/src/shared/*.c)

fterm: main.c fontstash.c $(TSM)
	$(CC) $(CFLAGS) -o $@ $^ $(LFLAGS)

clean:
	rm -vf fterm

.PHONY: clean
