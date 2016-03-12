
fterm: main.c fontstash.c shl-pty.c shl-ring.c
	gcc -Wall -g -o $@ $^ `sdl-config --cflags --libs` -lm -lGL -ltsm 

clean:
	rm fterm

.PHONY: clean
