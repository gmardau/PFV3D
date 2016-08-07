CC = g++
-CC = -Wall -ansi -pedantic -lm -g -lpthread -std=c++11
-GL = -lGLEW -lGLU -lGL -lSDL2 -lX11

example1: example1.cpp
	$(CC) $(-CC) $(-GL) example1.cpp -o example1

main: example1

all: example1

clean:
	rm -f example1

.PHONY: example1
