.PHONY: run clean test fast
GCC=/opt/homebrew/opt/llvm/bin/clang++ --std=c++2a -fopenmp
# GCC=g++ --std=c++2a
CC=$(GCC) -g -c

OFILES=out/world.o out/grid.o out/kernel.o out/vec2.o out/parse_input.o out/iisph.o out/physics.o
CFILES=world.cpp grid.cpp kernel.cpp vec2.cpp parse_input.cpp iisph.cpp physics.cpp main.cpp

out/main: out/main.o out/world.o out/grid.o out/kernel.o out/vec2.o out/parse_input.o out/iisph.o out/physics.o
	$(GCC) out/main.o $(OFILES) -o out/main


fast: $(CFILES)
	$(GCC) -O3 $(CFILES) -o out/main

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
	./out/main scenes/wells.txt --time 2

clean:
	rm -r out/
	mkdir out/
