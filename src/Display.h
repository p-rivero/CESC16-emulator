#pragma once

#include "Terminal.h"

#include <array>

class Display : public MemCell {
private:
    static const int ROWS = Terminal::ROWS;
    static const int COLS = Terminal::COLS;
    static const int WHITE = 0b111111;
    
    Terminal *term;
    word busy_flag = 0;
    // Some commands are sent in 2 bytes, store the command state
    enum {FIRST_BYTE = 0, SET_COLOR_LINE, SET_COLOR_SCREEN};
    byte next_byte = FIRST_BYTE;
    // Store the state of the cursor, so it can be restored later
    int stored_mRow;
    int stored_mCol;
    // Store the color of each row. Initialize all to white
    std::array<Terminal::color,ROWS> cram;
    
    static inline bool is_bit_set(byte data, byte bit_num);
    void process_char(byte inbyte);
    void set_color(byte color, int row);
    inline void propagate_color(int row);
    inline void update_cursor_color(int row);

public:
    Display();

    // WRITE
    MemCell& operator=(word rhs) override;
    // READ
    operator word() const override;
};
