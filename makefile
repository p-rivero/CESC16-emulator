# todo: once finished this can be replaced by -O2
OPTIONS = -g

CESC_Emu: src/main.cpp src/CPU.cpp
	g++ $(OPTIONS) -o CESC_Emu src/main.cpp src/CPU.cpp
