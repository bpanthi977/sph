.PHONY: run clean
CC=g++ -g --std=c++20 -c

out/main: out/main.o out/world.o out/grid.o out/kernel.o out/vec2.o out/parse_input.o
	g++ out/main.o out/world.o out/grid.o out/kernel.o out/vec2.o out/parse_input.o -o out/main

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

out/parse_input.o: parse_input.cpp
	$(CC) parse_input.cpp -o out/parse_input.o

run:
	./out/main

clean:
	rm -r out/
	mkdir out/
