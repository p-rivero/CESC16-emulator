#include "Display.h"
#include "Exceptions/EmulatorException.h"

int Globals::terminal_delay = 0; // In real hardware this would be 32 microseconds

Display::Display() {
    term = Terminal::initialize();
    // Initialize color lines to all white
    for (Terminal::color& c : cram) c = Terminal::color::WHITE;
}

void Display::set_color(byte color, byte row) {
    assert(row < ROWS);
    typedef Terminal::color col;
    const col COLORS[8] = {
        col::BLACK,  col::BLUE,     col::GREEN,  col::CYAN,
        col::RED,    col::MAGENTA,  col::YELLOW, col::WHITE
    };
    // Colors have 2 bits per channel (64 colors), but most terminals
    // support only 8 colors. Convert them to 1 bit per channel.
    
    byte reduced_color = 0; // 3-bit representation of the 6-bit color
    // If red is 0b10 or 0b11, red bit is set
    if ((color & 0b110000) >= 0b100000) reduced_color |= 0b100;
    // If green is 0b10 or 0b11, green bit is set
    if ((color & 0b001100) >= 0b001000) reduced_color |= 0b010;
    // If blue is 0b10 or 0b11, blue bit is set
    if ((color & 0b000011) >= 0b000010) reduced_color |= 0b001;
    
    cram[row] = COLORS[reduced_color];
    term->set_color(COLORS[reduced_color], row);
}

bool Display::is_bit_set(byte data, byte bit_num) {
    return (data & (1 << bit_num));
}

// Set the color of the line row+1 to the color of the line row
void Display::propagate_color(int row) {
    assert(row < ROWS-1);
    Terminal::color col = cram[row];
    cram[row+1] = col;
    term->set_color(col, row+1);
}

// Sets color for new chars to the color of a certain row
void Display::update_cursor_color(int row) {
    term->set_color(cram[row], -1);
}

// processes a character (see VGA terminal docs)
void Display::process_char(byte inbyte) {
    int mRow, mCol;
    bool update_coords = true;
    term->get_coords(mRow, mCol);   // Read the cursor coordinates
    
    // Second byte of a 2-byte sequence
    if (next_byte != FIRST_BYTE) {
        if (next_byte == SET_COLOR_LINE) {
            set_color(inbyte & 0x3F, mRow);
        }
        else if (next_byte == SET_COLOR_SCREEN) {
            for (int i = 0; i < ROWS; i++)
                set_color(inbyte & 0x3F, i);
        }
        next_byte = FIRST_BYTE;
        // Write the new cursor coordinates
        if (update_coords) term->set_coords(mRow, mCol);
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
            if (new_row < ROWS) {
                mRow = new_row;
                // New chars are printed in the color of the new line
                update_cursor_color(mRow);
            }
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
                    // New chars are printed in the color of the new line
                    update_cursor_color(mRow);
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
            term->set_cursor_blink(is_bit_set(inbyte, 0));
        }
        else if (is_bit_set(inbyte, 0)) {
            // Reset (set cursor to top-left, clear screen, set color of screen to white)
            mRow = mCol = 0;
            set_color(WHITE, 0);
            for (int i = 0; i < ROWS; i++) {
                term->clear_line(i);
                cram[i] = Terminal::color::WHITE;
            }
        }
    }
    else {  // ASCII CHAR
        term->print(inbyte, Terminal::print_mode::ONLY_FILE);
        switch (inbyte) {
            case '\b':  // Backspace: Remove 1 character (move cursor left)
                if (mCol > 0) { // Backspace doesn't change line
                    term->set_coords(mRow, --mCol);
                    term->print(' ', Terminal::print_mode::ONLY_SCREEN);
                }
                break;
            
            case 0x7F:  // Delete: Remove 1 character (move cursor right)
                if (mCol < COLS-1) term->print(' ', Terminal::print_mode::ONLY_SCREEN);
                else term->clear_line(-1);
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
                if (mRow < ROWS-1) {
                    // Propagate color to the next line
                    propagate_color(mRow++);
                }
                else {
                    // Scroll screen (color propagation is implicit)
                    term->print('\n', Terminal::print_mode::ONLY_SCREEN);
                    for (int i = 0; i < ROWS-1; i++)
                        cram[i] = cram[i+1];
                }
                break;
                
            case '\f':  // Form feed: insert a page break
                mRow = ROWS-1;
                mCol = 0;
                for (int i = 0; i < ROWS; i++) {
                    term->clear_line(i);
                    cram[i] = Terminal::color::WHITE;
                } 
                set_color(WHITE, ROWS-1);
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
                if (mRow < ROWS-1) update_cursor_color(++mRow);
                break;
            
            case 0x1F:  // Move cursor up
                if (mRow > 0) update_cursor_color(--mRow);
                break;
            
            default: // No special character detected.
                if (inbyte < ' ') return; // Check if it's printable
                int mRow_old = mRow;
                // Print the character, line may overflow
                term->print(inbyte, Terminal::print_mode::ONLY_SCREEN);
                term->get_coords(mRow, mCol);   // Read the new coordinates
                update_coords = false;
                
                if (mRow > mRow_old) { // Line overflowed, no scroll
                    // Set the color of the new line (mRow) to the color of the old line (mRow-1)
                    propagate_color(mRow-1);
                }
                else if (mCol == 0) { // Line overflowed with scroll, scroll the cram
                    for (int i = 0; i < ROWS-1; i++)
                        cram[i] = cram[i+1];
                }
                break;
        }
    }
    if (update_coords) term->set_coords(mRow, mCol);   // Write the new cursor coordinates
}

// WRITE
MemCell& Display::operator=(word rhs) {
    if (busy_flag != 0 && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw EmulatorException("Terminal: attempting to output while the controller was busy");
    }
    if (rhs > 0xFF && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 8-bit long
        throw EmulatorException("Terminal: Value written is bigger than 8 bit and will be truncated");
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
