# todo: once finished this can be replaced by -O2
OPTIONS = -g


CESC_Emu: src/main.o src/CpuController.o src/CPU.o src/Memory.o src/Terminal.o src/Timer.o
	g++ $(OPTIONS) -o CESC_Emu src/main.o src/CpuController.o src/CPU.o src/Memory.o src/Terminal.o src/Timer.o -lncurses -pthread


src/main.o: src/main.cpp
	g++ $(OPTIONS) -c src/main.cpp -o src/main.o

src/CpuController.o: src/CpuController.cpp
	g++ $(OPTIONS) -c src/CpuController.cpp -o src/CpuController.o

src/CPU.o: src/CPU.cpp
	g++ $(OPTIONS) -c src/CPU.cpp -o src/CPU.o

src/Memory.o: src/Memory.cpp
	g++ $(OPTIONS) -c src/Memory.cpp -o src/Memory.o

src/Terminal.o: src/Terminal.cpp
	g++ $(OPTIONS) -c src/Terminal.cpp -o src/Terminal.o

src/Timer.o: src/Timer.cpp
	g++ $(OPTIONS) -c src/Timer.cpp -o src/Timer.o

clean:
	rm -f src/*.o
	rm -f CESC_Emu
