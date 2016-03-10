
fontstash: main.c fontstash.c
	gcc -Wall -g -o fontstash main.c fontstash.c `sdl-config --cflags --libs` -lm -lGL

