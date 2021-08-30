#pragma once

#include "Terminal.h"
#include <thread>

class Display : public MemCell {
private:
    Terminal *term;
    word busy_flag = 0;
    // Some commands are sent in 2 bytes, store the command state
    enum {FIRST_BYTE = 0, SET_COLOR_LINE, SET_COLOR_SCREEN};
    byte next_byte = FIRST_BYTE;
    // Store the state of the cursor, so it can be restored later
    byte old_mRow, old_mCol;
    
    static const int ROWS = Terminal::ROWS;
    static const int COLS = Terminal::COLS;
    static const int WHITE = 0b111111;
    
    static inline bool is_bit_set(byte data, byte bit_num);
    void process_char(byte inbyte);
    void set_color(byte row, byte color);

public:
    Display();

    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
};
