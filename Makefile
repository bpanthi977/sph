CC=g++ --std=c++11 -c

out/main: out/main.o out/world.o out/grid.o out/kernel.o out/vec2.o
	g++ out/main.o out/world.o out/grid.o out/kernel.o out/vec2.o -o out/main

out/main.o: main.cpp
	$(CC) main.cpp -o out/main.o

out/world.o: world.cpp
	$(CC) world.cpp -o out/world.o

out/grid.o: grid.cpp
	$(CC) grid.cpp -o out/grid.o

out/kernel.o: kernel.cpp
	$(CC) kernel.cpp -o out/kernel.o

out/vec2.o: vec2.cpp
	$(CC) vec2.cpp -o out/vec2.o

.PHONY clean:
	rm -r out/
	mkdir out/
