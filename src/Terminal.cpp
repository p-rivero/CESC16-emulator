#include "Terminal.h"
#include "Utilities/Assert.h"
#include "Utilities/ExitHelper.h"

#include <sys/ioctl.h>

Terminal Terminal::instance = Terminal();

Terminal *Terminal::get_instance() {
    return &instance;
}

void Terminal::draw_rectangle(int y1, int x1, int y2, int x2, const char *title) {
    mvhline(y1, x1, 0, x2-x1);
    mvhline(y2, x1, 0, x2-x1);
    mvvline(y1, x1, 0, y2-y1);
    mvvline(y1, x2, 0, y2-y1);
    mvaddch(y1, x1, ACS_ULCORNER);
    mvaddch(y2, x1, ACS_LLCORNER);
    mvaddch(y1, x2, ACS_URCORNER);
    mvaddch(y2, x2, ACS_LRCORNER);
    mvprintw(y1, x1+2, " %s ", title);
}

void Terminal::sig_handler(int sig) { 
    if (sig == SIGWINCH) size_check();
    else if (sig == SIGCONT) instance.resume();
    else if (sig == SIGTSTP) instance.stop();
    else ExitHelper::error("Error: Unknown signal received\n");
}

void Terminal::size_check() {
    winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    
    if (w.ws_row <= ROWS+4) ExitHelper::error("ERROR - Terminal height too small\n");
    if (w.ws_col <= COLS+COLS_STATUS+4) ExitHelper::error("ERROR - Terminal width too small\n");
}

void Terminal::stop() {
    // Save updated settings for ncurses
    tcgetattr(0, &curses_settings);
    // Restore correct settings for shell
    tcsetattr(0, TCSANOW, &shell_settings);
    
    // Call the predefined SIGTSTP handler
    ncurses_stop_handler(SIGTSTP);
}
void Terminal::resume() {
    // TODO: This remains broken, try to fix it
    // Restore correct settings for ncurses
    tcsetattr(0, TCSANOW, &curses_settings);
    redrawwin(mainwin);
    tcsetattr(0, TCSADRAIN, &curses_settings);
}


Terminal::Terminal(){
    // If -s is not used, initialize the ncurses windows
    if (!Globals::silent_flg) {
        init_ncurses();
    }
    // If -o is used, all CPU outputs are stored in output_file
    if (Globals::out_file) {
        output_file = std::ofstream(Globals::out_file, std::fstream::out);
        if (!output_file) ExitHelper::error("Error: Output file [%s] could not be opened\n", Globals::out_file);
    }
}

void Terminal::destroy() {
    // Discard any buffered outputs
    flush();
    
    if (!Globals::silent_flg) {
        cleanup_ncurses();
    }
    if (Globals::out_file) {
        output_file.close();
    }
    
    // Restore correct settings for shell
    tcsetattr(0, TCSANOW, &shell_settings);
}

void Terminal::init_ncurses() {
    // Save terminal settings
    tcgetattr(0, &shell_settings);

    // Initialize main window
    mainwin = initscr();
    if (mainwin == nullptr)
        ExitHelper::error("Error initializing main window!\r\n");

    // Initialize subwindow (terminal output)
    term_screen = newwin(ROWS, COLS, 1, 1);
    if (term_screen == nullptr)
        ExitHelper::error("Error initializing subwindow!\n");

    // Initialize subwindow (status)
    stat_screen = newwin(ROWS, COLS_STATUS, 1, COLS+4);
    if (stat_screen == nullptr)
        ExitHelper::error("Error initializing subwindow!\n");
        
    // Initialize subwindow (metrics)
    perf_screen = newwin(1, COLS+COLS_STATUS+3, ROWS+3, 1);
    if (perf_screen == nullptr)
        ExitHelper::error("Error initializing subwindow!\n");

    noecho(); // Turn off key echoing
    keypad(mainwin, true);  // Enable the keypad for non-char keys
    nodelay(mainwin, true); // Enable non-blocking input
    ESCDELAY = 0;   // Don't freeze emulator every time ESC is pressed
    scrollok(term_screen, true); // Enable scrolling for terminal subwindow
    TABSIZE = 4;

    // SIGWINCH is triggered whenever the user resizes the window
    if (signal(SIGWINCH, sig_handler) == SIG_ERR)
        ExitHelper::error("Error: Couldn't catch SIGWINCH!\n");

    ncurses_stop_handler = signal(SIGTSTP, sig_handler);
    if (ncurses_stop_handler == SIG_ERR)
        ExitHelper::error("Error: Couldn't catch SIGTSTP!\n");
    
    if (signal(SIGCONT, sig_handler) == SIG_ERR)
        ExitHelper::error("Error: Couldn't catch SIGCONT!\n");
        
    // Make sure terminal supports color
    if (!has_colors())
        ExitHelper::error("ERROR - Your terminal does not support color\n");
    
    start_color();
    use_default_colors();   // Allow terminal to keep default background using -1
    init_pair(color::BLACK, COLOR_BLACK, -1);
    init_pair(color::RED, COLOR_RED, -1);
    init_pair(color::GREEN, COLOR_GREEN, -1);
    init_pair(color::YELLOW, COLOR_YELLOW, -1);
    init_pair(color::BLUE, COLOR_BLUE, -1);
    init_pair(color::MAGENTA, COLOR_MAGENTA, -1);
    init_pair(color::CYAN, COLOR_CYAN, -1);
    init_pair(color::WHITE, COLOR_WHITE, -1);

    // Draw frames around subwindows
    draw_rectangle(0, 0, ROWS+1, COLS+1, "Terminal output");
    draw_rectangle(0, COLS+3, ROWS+1, COLS+COLS_STATUS+4, "Status");
    draw_rectangle(ROWS+2, 0, ROWS+4, COLS+COLS_STATUS+4, "Performance");
    refresh();
}

void Terminal::cleanup_ncurses() {
    delwin(term_screen);
    delwin(mainwin);
    endwin();
    refresh();
}


// Output a char
void Terminal::print(char c, print_mode mode) {
    switch (mode)
    {
    case ONLY_SCREEN:
        // Print to ncurses terminal screen
        if (!Globals::silent_flg) wprintw(term_screen, "%c", c);
        break;
        
    case ONLY_FILE:
        // Write to file
        if (Globals::out_file) output_file << c;
        // If in silent mode, print directly to terminal
        if (Globals::silent_flg) putc(c, stdout);
        break;
    
    case BOTH:
        // Write to file
        if (Globals::out_file) output_file << c;
        // Print to ncurses terminal screen
        if (Globals::silent_flg) putc(c, stdout);
        else wprintw(term_screen, "%c", c);
        break;
    
    default:
        ExitHelper::error("Invalid print mode: %d\n", int(mode));
        break;
    }
}

void Terminal::display_status(word PC, bool user_mode, const StatusFlags& flg, Regfile& regs, double CPI) {
    wmove(stat_screen, 0, 0); // Set cursor to beginning of window

    wprintw(stat_screen, " PC=0x%04X", PC);
    if (user_mode) wprintw(stat_screen, " [U]");
    wprintw(stat_screen, "\n Mode: %s\n", user_mode ? "RAM" : "ROM");
    wprintw(stat_screen, " Flags: %c%c%c%c\n\n", flg.Z?'Z':'.', flg.C?'C':'.', flg.V?'V':'.', flg.S?'S':'.');

    for (byte i = 1; i < 16; i++)
        wprintw(stat_screen, " %s = 0x%04X\n", regs.ABI_names[i].c_str(), word(regs[i]));

    if (Globals::is_paused) wprintw(stat_screen, "\n [PAUSED]\n F5: Resume\n F6: Step\n F7: Cycle = 0\n");
    else wprintw(stat_screen, "\n\n\n\n\n");
    
    wmove(perf_screen, 0, 0); // Set cursor to beginning of window
    wprintw(perf_screen, " CPI: %.4lf\tElapsed cycles: %llu\n", CPI, (unsigned long long)Globals::elapsed_cycles);
}

// Flush the output stream
void Terminal::flush() {
    wrefresh(perf_screen);
    wrefresh(stat_screen);
    wrefresh(term_screen);
    // Output file doesn't need to be flushed
}

// Process ncurses key queue until it's empty (called periodically)
void Terminal::update_input() {
    int ch;
    // Empty the ncurses buffer and store on local input queue (this way function keys get processed immediately)
    while ((ch = getch()) != ERR) {
        switch (ch) {
            // The END key pauses execution
            case KEY_BACKSPACE: input_buffer.push('\b'); break;
            case KEY_PPAGE:     input_buffer.push(0x0B); break;
            case KEY_NPAGE:     input_buffer.push(0x0C); break;
            case KEY_HOME:      input_buffer.push('\r'); break;
            case KEY_IC:        input_buffer.push(0x0E); break;
            case KEY_END:       input_buffer.push(0x1B); break;
            case KEY_LEFT:      input_buffer.push(0x1C); break;
            case KEY_RIGHT:     input_buffer.push(0x1D); break;
            case KEY_DOWN:      input_buffer.push(0x1E); break;
            case KEY_UP:        input_buffer.push(0x1F); break;
            case KEY_DC:        input_buffer.push(0x7F); break;

            case KEY_F(1): input_buffer.push(0x0F); break;
            case KEY_F(2): input_buffer.push(0x10); break;
            case KEY_F(3): input_buffer.push(0x11); break;
            case KEY_F(4): input_buffer.push(0x12); break;
            
            case KEY_F(5):
                // Pause/unpause emulator
                Globals::is_paused = !Globals::is_paused;
                Globals::single_step = false;
                break;
            case KEY_F(6):
                // Execute 1 instruction
                if (!Globals::is_paused) break;  // F6 only works when paused
                Globals::single_step = true;
                Globals::is_paused = false;
                break;
            case KEY_F(7):
                // Reset cycle counter
                if (!Globals::is_paused) break;  // F7 only works when paused
                Globals::elapsed_cycles = 0;
                break;
                
            case KEY_F(8):  input_buffer.push(0x16); break;
            case KEY_F(9):  input_buffer.push(0x17); break;
            case KEY_F(10): input_buffer.push(0x18); break;
            case KEY_F(11): input_buffer.push(0x19); break;
            case KEY_F(12): input_buffer.push(0x1A); break;
            
            // Else, check that all special keys have been catched and send regular input
            default:
                if (is_regular_char(ch)) input_buffer.push(byte(ch));
                break;
        }
    }
}

// Returns the first character in the input queue (and removes it from the queue)
byte Terminal::get_input() {
    update_input();
    
    // If the queue is not empty, remove and return the first element
    if (input_buffer.empty()) return 0;
    
    byte input = input_buffer.front();
    input_buffer.pop();
    return input;
}

// Returns true if ch is a regular ascii character
bool Terminal::is_regular_char(int ch) {
    // Make sure all ncurses special keys have been catched
    if (ch > 0xFF)
        ExitHelper::error("ERROR - Uncatched key: %s (0x%X)\n", keyname(ch), ch);

    // 110xxxxx => 2-byte UTF-8 encoded characters, ignore by default
    if ((ch & 0b11100000) == 0b11000000) {
        assert(getch() != ERR);
        return false;
    }
    // 1110xxxx => 3-byte UTF-8 encoded characters, ignore by default
    if ((ch & 0b11110000) == 0b11100000) {
        assert(getch() != ERR);
        assert(getch() != ERR);
        return false;
    }
    // 11110xxx => 4-byte UTF-8 encoded characters, ignore by default
    if ((ch & 0b11111000) == 0b11110000) {
        assert(getch() != ERR);
        assert(getch() != ERR);
        assert(getch() != ERR);
        return false;
    }
    // The 10xxxxxx range *should* be unused
    if (ch > 0x7F)
        ExitHelper::error("UNREACHABLE INPUT: %s (0x%X)\n", keyname(ch), ch);
    return true;
}

// Gets the current cursor coordinates (leaves them in row and col)
void Terminal::get_coords(int& row, int& col) const {
    getyx(term_screen, row, col);
}

// Sets the current cursor coordinates
void Terminal::set_coords(int row, int col) {
    assert(row < ROWS);
    assert(col < COLS);
    wmove(term_screen, row, col);
}

void Terminal::clear_line(int row) {
    assert(row < ROWS);
    if (row > 0) wmove(term_screen, row, 0);
    wclrtoeol(term_screen);
}

void Terminal::set_color(color c, int row) {
    assert(row < ROWS);
    // Set color for future sent chars
    wattron(term_screen, COLOR_PAIR(c));
    if (row >= 0) {
        // Set entire line (n=-1) to NORMAL with color c
        mvwchgat(term_screen, row, 0, -1, A_NORMAL, c, nullptr);
    }
}

void Terminal::set_cursor_blink(bool blink) const {
    curs_set(blink);
}
