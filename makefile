CC = g++
-CC = -Wall -ansi -pedantic -lm -g -lpthread -std=c++11
-GL = -lGLEW -lGLU -lGL -lSDL2 -lX11

default: all

example1: example1.cpp
	$(CC) $(-CC) $(-GL) example1.cpp -o example1
example2: example2.cpp
	$(CC) $(-CC) $(-GL) example2.cpp -o example2
example3: example3.cpp
	$(CC) $(-CC) $(-GL) example3.cpp -o example3
example4: example4.cpp
	$(CC) $(-CC) $(-GL) example4.cpp -o example4
example5: example5.cpp
	$(CC) $(-CC) $(-GL) example5.cpp -o example5
example6: example6.cpp
	$(CC) $(-CC) $(-GL) example6.cpp -o example6

all: example1 example2 example3 example4 example5 example6

clean:
	rm -f example1 example2 example3 example4 example5 example6

.PHONY: example1 example2 example3 example4 example5 example6
