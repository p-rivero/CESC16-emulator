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
3. The emulator will start running at the default clock speed, make sure the size of the terminal window is big enough to fit all the elements.

    ![Demo](https://github.com/p-rivero/CESC16-emulator/blob/main/demo/demo.png?raw=true)


## Performance
The clock frequency of the emulated CPU can be chenged by using the `-f` option.

Example (set clock speed to 50 kHz):
```
./CESC_Emu -f 50000 my_ROM_file.hex
```

When compiled with `-O2`, the emulator was able to run at 50MHz without problems on my PC, so it's safe to assume that the emulator is able to run faster than the real CPU will ever do.
