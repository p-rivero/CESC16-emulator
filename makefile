# todo: once finished this can be replaced by -O2
OPTIONS = -g

BIN_NAME = CESC_Emu

# $@ = Name of the rule target
# $< = Name of all the first prerequisite

$(BIN_NAME): src/main.o src/CpuController.o src/CPU.o src/Memory.o src/Terminal.o src/Timer.o src/Disk.o
	g++ $(OPTIONS) $^ -o $@ -lncurses -pthread


src/main.o: src/main.cpp
	g++ $(OPTIONS) -c $< -o $@

src/CpuController.o: src/CpuController.cpp src/CpuController.h src/CPU.h
	g++ $(OPTIONS) -c $< -o $@

src/CPU.o: src/CPU.cpp src/CPU.h src/Memory.h src/Terminal.h src/Timer.h src/Disk.h src/ArithmeticMean.h
	g++ $(OPTIONS) -c $< -o $@

src/Memory.o: src/Memory.cpp src/Memory.h
	g++ $(OPTIONS) -c $< -o $@

src/Terminal.o: src/Terminal.cpp src/Terminal.h src/Memory.h
	g++ $(OPTIONS) -c $< -o $@

src/Timer.o: src/Timer.cpp src/Timer.h src/Memory.h
	g++ $(OPTIONS) -c $< -o $@

src/Disk.o: src/Disk.cpp src/Disk.h src/Memory.h
	g++ $(OPTIONS) -c $< -o $@

clean:
	rm -f src/*.o
	rm -f $(BIN_NAME)
