all: virtual-memory.o

#All error and warnings: -Wall -Werror -Wpedantic	Debug: -g
virtual-memory.o: virtual-memory.c
	gcc -pthread -o build/virtual-memory virtual-memory.c -g

clean:
	rm -r build/*