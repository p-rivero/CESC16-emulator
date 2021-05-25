# todo: once finished this can be replaced by -O2
OPTIONS = -g


CESC_Emu: src/main.o src/CPU.o src/Terminal.o
	g++ $(OPTIONS) -o CESC_Emu src/main.o src/CPU.o src/Terminal.o -lncurses -pthread


src/main.o: src/main.cpp
	g++ $(OPTIONS) -c src/main.cpp -o src/main.o

src/CPU.o: src/CPU.cpp
	g++ $(OPTIONS) -c src/CPU.cpp -o src/CPU.o

src/Terminal.o: src/Terminal.cpp
	g++ $(OPTIONS) -c src/Terminal.cpp -o src/Terminal.o

clean:
	rm -f src/*.o
	rm -f CESC_Emu
