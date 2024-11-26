.PHONY: run clean test
CC=g++ -g --std=c++20 -c

OFILES=out/world.o out/grid.o out/kernel.o out/vec2.o out/parse_input.o out/iisph.o out/physics.o
out/main: out/main.o out/world.o out/grid.o out/kernel.o out/vec2.o out/parse_input.o out/iisph.o out/physics.o
	g++ --std=c++20 out/main.o $(OFILES) -o out/main

test: out/world.o out/grid.o out/kernel.o out/vec2.o out/parse_input.o out/test.o
	g++ out/test.o $(OFILES) -o out/test
	out/test

out/test.o: test.cpp
	$(CC) test.cpp -o out/test.o

out/main.o: main.cpp
	$(CC) main.cpp -o out/main.o

out/world.o: world.cpp
	$(CC) world.cpp -o out/world.o

out/physics.o: physics.cpp
	$(CC) physics.cpp -o out/physics.o

out/iisph.o: iisph.cpp
	$(CC) iisph.cpp -o out/iisph.o

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
