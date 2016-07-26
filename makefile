CC = g++
-CC = -Wall -ansi -pedantic -lm -g -lpthread -std=c++11
-GL = -lGLEW -lGLU -lGL -lSDL2 -lX11

DEP = main.cpp pfv3d.h
MAIN = main.cpp

main: $(DEP)
	$(CC) $(-CC) $(-GL) $(MAIN) -o main

all: main

clean:
	rm -f main
