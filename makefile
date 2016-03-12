
CFLAGS = -I./libtsm/src/tsm -I./libtsm/src/shared -I./libtsm
CFLAGS += `sdl-config --cflags`
LFLAGS = `sdl-config --libs`

TSM = $(wildcard libtsm/src/tsm/*.c) $(wildcard libtsm/external/*.c) $(wildcard libtsm/src/shared/*.c)

fterm: main.c fontstash.c $(TSM)
	$(CC) -Wall -g -o $@ $^ $(CFLAGS) $(LFLAGS) -lm -lGL

clean:
	rm fterm

.PHONY: clean
