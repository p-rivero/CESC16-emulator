#include "Display.h"

int Globals::terminal_delay = 0; // In real hardware this would be 32 microseconds

Display::Display() {
    term = Terminal::initialize();
}

void Display::set_color(byte row, byte color) {
    const Terminal::color COLORS[8] = {
        Terminal::color::BLACK,
        Terminal::color::BLUE,
        Terminal::color::GREEN,
        Terminal::color::CYAN,
        Terminal::color::RED,
        Terminal::color::MAGENTA,
        Terminal::color::YELLOW,
        Terminal::color::WHITE
    };
    
    byte reduced_color = 0; // 3-bit representation of the 6-bit color
    // If red is 0b10 or 0b11, red bit is set
    if ((color & 0b110000) >= 0b100000) reduced_color |= 0b100;
    // If green is 0b10 or 0b11, green bit is set
    if ((color & 0b001100) >= 0b001000) reduced_color |= 0b010;
    // If blue is 0b10 or 0b11, blue bit is set
    if ((color & 0b000011) >= 0b000010) reduced_color |= 0b001;
    
    term->set_color(COLORS[reduced_color], row);
}

bool Display::is_bit_set(byte data, byte bit_num) {
    return (data & (1 << bit_num));
}

// processes a character (see VGA terminal docs)
void Display::process_char(byte inbyte) {
    int mRow, mCol;
    bool update_coords = true;
    term->get_coords(mRow, mCol);   // Read the cursor coordinates
    
    // Second byte of a 2-byte sequence
    if (next_byte != FIRST_BYTE) {
        if (next_byte == SET_COLOR_LINE) {
            set_color(mRow, inbyte & 0x3F);
        }
        else if (next_byte == SET_COLOR_SCREEN) {
            for (int i = 0; i < ROWS; i++)
                set_color(i, inbyte & 0x3F);
        }
        next_byte = FIRST_BYTE;
        return;
    }
    
    if (is_bit_set(inbyte, 7)) {    // COMMAND
        if (is_bit_set(inbyte, 6)) {
            // Move to column
            byte new_col = inbyte & 0x3F;
            if (new_col < COLS) mCol = new_col;
        }
        else if (is_bit_set(inbyte, 5)) {
            // Move to line/row
            byte new_row = inbyte & 0x1F;
            if (new_row < ROWS) mRow = new_row;
        }
        // 0b1001XXXX (bit 4) -> Unused
        else if (is_bit_set(inbyte, 3)) {
            if (is_bit_set(inbyte, 0)) {
                // Set color for line
                next_byte = SET_COLOR_LINE;
            }
            else {
                // Set color for screen
                next_byte = SET_COLOR_SCREEN;
            }
        }
        else if (is_bit_set(inbyte, 2)) {
            if (is_bit_set(inbyte, 1)) {
                if (is_bit_set(inbyte, 0)) {
                    // Clear line
                    term->clear_line(mRow);
                }
                else {
                    // Clear screen
                    for (int i = 0; i < ROWS; i++) term->clear_line(i);
                }
            }
            else {
                if (is_bit_set(inbyte, 0)) {
                    // Restore cursor pos
                    mRow = old_mRow;
                    mCol = old_mCol;
                }
                else {
                    // Save cursor pos
                    old_mRow = mRow;
                    old_mCol = mCol;
                }
            }
        }
        else if (is_bit_set(inbyte, 1)) {
            // Turn on/off cursor blink
            throw "Emulator does not allow turning on/off cursor blink";
        }
        else if (is_bit_set(inbyte, 0)) {
            // Reset (set cursor to top-left, clear screen, set color of screen to white)
            mRow = mCol = 0;
            for (int i = 0; i < ROWS; i++) term->clear_line(i);
            for (int i = 0; i < ROWS; i++) set_color(i, WHITE);
        }
    }
    else {  // ASCII CHAR
        switch (inbyte) {
            case '\b':  // Backspace: Remove 1 character (move cursor left)
                if (mCol > 0) {
                    // Remove from the same line
                    term->set_coords(mRow, --mCol);
                    term->print(' ');
                }
                else if (mRow > 0) {
                    // If not on the first line, remove from previous line
                    mRow--;
                    term->set_coords(mRow, COLS-1);
                    term->print(' ');
                    mCol = COLS-1;
                }
                break;
            
            case 0x7F:  // Delete: Remove 1 character (move cursor right)
                term->print(' ');
                update_coords = false;
                break;
            
            case '\t':  // Tab: move to next multiple of 4
                mCol &= 0xFC;
                if (mCol < COLS-5) mCol += 4;
                else mCol = COLS-1;
                break;
            
            case '\n':  // Move to a new line
                mCol = 0;
                // Also do '\v'
            
            case '\v':  // Vertical tab: LF without CR
                if (mRow < ROWS-1) mRow++;
                else term->print('\n'); // Scroll screen
                break;
                
            case '\f':  // Form feed: insert a page break
                mRow = ROWS-1;
                mCol = 0;
                for (int i = 0; i < ROWS; i++) term->clear_line(i);
                set_color(ROWS-1, WHITE);
                break;
            
            case '\r':  // Carriage return: move to the beginning of line
                mCol = 0;
                break;
                
            case 0x1C:  // Move cursor left
                if (mCol > 0) mCol--;
                break;
            
            case 0x1D:  // Move cursor right
                if (mCol < COLS-1) mCol++;
                break;
            
            case 0x1E:  // Move cursor down
                if (mRow < ROWS-1) mRow++;
                break;
            
            case 0x1F:  // Move cursor up
                if (mRow > 0) mRow--;
                break;
            
            default: // No special character detected.
                if (inbyte < ' ') return; // Check if it's printable
                term->print(inbyte);  // Just print the character
                update_coords = false;
                break;
        }
    }
    if (update_coords) term->set_coords(mRow, mCol);   // Write the new cursor coordinates
}

// WRITE
MemCell& Display::operator=(word rhs) {
    if (busy_flag != 0 and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw "Terminal: attempting to output while the controller was busy";
    }
    if (rhs > 0xFF and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 8-bit long
        throw "Terminal: Value written is bigger than 8 bit and will be truncated";
    }
    if (Globals::terminal_delay > 0) {
        busy_flag = rhs;
        // VGA terminal can process 1 input every 32 microseconds, wait and clear the flag
        std::thread([this]() {
            _KILL_GUARD
            std::this_thread::sleep_for(std::chrono::microseconds(Globals::terminal_delay));
            busy_flag = 0;
        }).detach();
    }
    
    // Output char or process command
    process_char(rhs);
    return *this;
}

// READ
Display::operator int() const {
    return busy_flag;
}
