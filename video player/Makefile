

clean: 
	rm -rf Tam2

all: Tam2.c
	gcc -o Tam2 Tam2.c -lavformat -lavcodec -lswscale -lz -lm `sdl-config --cflags --libs` -lavutil 

