CC = g++
-CC = -Wall -ansi -pedantic -lm -g -lpthread -std=c++14
-GL = -lGLEW -lGLU -lGL -lSDL2 -lX11

default: example

example: example.cpp
	$(CC) $(-CC) $(-GL) example.cpp -o example

all: example

clean:
	rm -f example

.PHONY: example
