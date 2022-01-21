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

    ![Demo](https://github.com/p-rivero/CESC16-emulator/blob/main/demo/demo.gif?raw=true)


## Performance
The clock frequency of the emulated CPU can be changed by using the `-f` option.

Example (set clock speed to 50 kHz):
```
./CESC_Emu -f 50000 my_ROM_file.hex
```

When compiled with `-O2`, the emulator was able to run at 50MHz without problems on my PC, so it's safe to assume that the emulator is able to run faster than the real CPU will ever do.

## Breakpoints
You can pause the emulator at any time by pressing the `F5` key.

You can also set breakpoints by using the `-b` option followed by an address **in hex format** (without the `0x`). The emulator will be paused *before* running the instruction at the specified position (PC).
The emulator supports an arbitrary number of breakpoints, just use `-b` multiple times.

Example (set breakpoints at `PC=0x0000` and `PC=0x1234`):
```
./CESC_Emu my_ROM_file.hex -b 0 -b 1234
```

Once the emulator is paused, you can single-step by pressing the `F6` key (each time it's pressed, exactly 1 instruction is executed).

## Exit points
Exit points work in exactly the same way as breakpoints, but they cause the emulator to exit instead of pausing execution. The exit code of the program (returned to the OS) is the value that was stored in register `a0`.

You can set an arbitrary number of exit points, using `-x` multiple times.

Example (jump to `PC=0xFFFF` in order to exit the emulator):
```
./CESC_Emu my_ROM_file.hex -x ffff
```
