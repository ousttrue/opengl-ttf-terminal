

PLATFORM = $(shell uname)
REV = $(shell git rev-parse HEAD)

ifeq ($(findstring Linux,$(PLATFORM)),Linux)
  $(info Building revision $(REV) for $(PLATFORM))
else
  $(error $(PLATFORM) is not supported)
  $(shell exit 2)
endif

CFLAGS += -g -Wall
CFLAGS += -I./libtsm/src/tsm -I./libtsm/src/shared -I./libtsm
CFLAGS += `sdl-config --cflags`
CFLAGS += -pg 

LFLAGS += `sdl-config --libs`
LFLAGS += -lm -lGL

TSM = $(wildcard libtsm/src/tsm/*.c) $(wildcard libtsm/external/*.c) $(wildcard libtsm/src/shared/*.c)

SRC = $(TSM) fontstash.c main.c
OBJ = $(SRC:.c=.o)
BIN = togl

.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

$(BIN): $(OBJ)
	$(CC) -o $@ $^  $(LFLAGS)

clean:
	rm -vf $(BIN) $(OBJ)

.PHONY: clean
