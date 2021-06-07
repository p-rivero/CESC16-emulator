# CESC16 Emulator
An emulator for the homebrew CESC16 CPU architecture.

The main code of the emulator (`CPU.cpp`) is based on [Dave Poo's 6502 emulator](https://www.youtube.com/playlist?list=PLLwK93hM93Z13TRzPx9JqTIn33feefl37), plus some improvements for adding IO and adjustable clock speed.

## How to use
1. Build the emulator. Currently Linux is the only supported OS.
    ```
    make
    ```
2. Run the emulator. Specify as an argument which hex file to load into ROM.
    ```
    ./CESC_Emu my_ROM_file.hex
    ```
3. The emulator will start running, make sure the size of the terminal window is big enough to fit all the elements.

    ![Demo](https://github.com/p-rivero/CESC16-emulator/blob/main/demo/demo.png?raw=true)

