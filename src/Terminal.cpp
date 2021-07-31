#include "Terminal.h"

void destroy_terminal() { Terminal::destroy(); }

Terminal *Terminal::term = NULL;
int Globals::terminal_delay = 32; // 32 microseconds

void Terminal::fatal_error(const char* msg, ...) {
    _KILL_GUARD
    destroy(); // Destroy terminal
    // Print error message
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end (args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
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
    else if (sig == SIGCONT) term->resume();
    else if (sig == SIGTSTP) term->stop();
    else fatal_error("Error: Unknown signal received");
}

void Terminal::size_check() {
    winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    
    if (w.ws_row <= ROWS+4) fatal_error("ERROR - Terminal height too small");
    if (w.ws_col <= COLS+COLS_STATUS+4) fatal_error("ERROR - Terminal width too small");
}

void Terminal::stop() {
    // Save updated settings for ncurses
    tcgetattr(0, &curses_settings);
    // Restore correct settings for shell
    tcsetattr(0, TCSANOW, &shell_settings);
    
    // Call the predefined SIGTSTP handler
    (ncurses_stop_handler)(SIGTSTP);
}
void Terminal::resume() {
    // Restore correct settings for ncurses
    tcsetattr(0, TCSANOW, &curses_settings);
    redrawwin(term_screen);
    tcsetattr(0, TCSADRAIN, &curses_settings);
}


Terminal::Terminal(){
    // Save terminal settings
    tcgetattr(0, &shell_settings);

    // Initialize main window
    mainwin = initscr();
    if (mainwin == NULL)
        fatal_error("Error initializing main window!\r");

    // Initialize subwindow (terminal output)
    term_screen = newwin(ROWS, COLS, 1, 1);
    if (term_screen == NULL)
        fatal_error("Error initializing subwindow!");

    // Initialize subwindow (status)
    stat_screen = newwin(ROWS, COLS_STATUS, 1, COLS+4);
    if (stat_screen == NULL)
        fatal_error("Error initializing subwindow!");
        
    // Initialize subwindow (metrics)
    perf_screen = newwin(1, COLS+COLS_STATUS+3, ROWS+3, 1);
    if (perf_screen == NULL)
        fatal_error("Error initializing subwindow!");

    noecho(); // Turn off key echoing
    keypad(mainwin, TRUE); // Enable the keypad for non-char keys
    nodelay(mainwin, TRUE); // Enable non-blocking input
    scrollok(term_screen, true); // Enable scrolling for terminal subwindow

    // SIGWINCH is triggered whenever the user resizes the window
    if (signal(SIGWINCH, sig_handler) == SIG_ERR)
        fatal_error("Error: Couldn't catch SIGWINCH!");

    ncurses_stop_handler = signal(SIGTSTP, sig_handler);
    if (ncurses_stop_handler == SIG_ERR)
        fatal_error("Error: Couldn't catch SIGTSTP!");
    
    if (signal(SIGCONT, sig_handler) == SIG_ERR)
        fatal_error("Error: Couldn't catch SIGCONT!");

    // Draw frames around subwindows
    draw_rectangle(0, 0, ROWS+1, COLS+1, "Terminal output");
    draw_rectangle(0, COLS+3, ROWS+1, COLS+COLS_STATUS+4, "Status");
    draw_rectangle(ROWS+2, 0, ROWS+4, COLS+COLS_STATUS+4, "Performance");
    refresh();

    // If -o is used, all CPU outputs are stored in output_file
    if (Globals::out_file) {
        output_file = std::ofstream(Globals::out_file, std::fstream::out);
        if (not output_file) fatal_error("Error: Output file [%s] could not be opened", Globals::out_file);
    }
}

Terminal::~Terminal(){
    // Clean up
    delwin(term_screen);
    delwin(mainwin);
    endwin();
    refresh();

    // Restore correct settings for shell
    tcsetattr(0, TCSANOW, &shell_settings);

    if (Globals::out_file) output_file.close();
}


// Output a char
void Terminal::output(word data) {
    wprintw(term_screen, "%c", char(data));
    if (Globals::out_file) output_file << char(data);
}

void Terminal::display_status(word PC, bool user_mode, const StatusFlags& flg, Regfile& regs, double CPI) {
    curs_set(0);
    wmove(stat_screen, 0, 0); // Set cursor to beginning of window

    wprintw(stat_screen, " PC=0x%04X", PC);
    if (user_mode) wprintw(stat_screen, " [U]");
    wprintw(stat_screen, "\n Mode: %s\n", user_mode ? "RAM" : "ROM");
    wprintw(stat_screen, " Flags: %c%c%c%c\n\n", flg.Z?'Z':'.', flg.C?'C':'.', flg.V?'V':'.', flg.S?'S':'.');

    for (uint i = 1; i < 16; i++)
        wprintw(stat_screen, " %s = 0x%04X\n", regs.ABI_names[i], word(regs[i]));

    if (Globals::is_paused) wprintw(stat_screen, "\n [PAUSED]\n F5: Resume\n F6: Step\n F7: Cycle = 0\n");
    else wprintw(stat_screen, "\n\n\n\n\n", CPI);
    
    wmove(perf_screen, 0, 0); // Set cursor to beginning of window
    wprintw(perf_screen, " CPI: %.4lf\tElapsed cycles: %llu\n", CPI, Globals::elapsed_cycles);
}

// Flush the output stream
void Terminal::flush() {
    wrefresh(perf_screen);
    wrefresh(stat_screen);
    wrefresh(term_screen);
    curs_set(1);
    // Output file doesn't need to be flushed
}

// Get the current input byte
byte Terminal::read_input() const {
    if (CPU_input_busy) return 0;
    return current_input;
}

// Acknowledge the current input byte
void Terminal::ack_input() {
    current_input = 0;
    CPU_input_busy = true;
}

// CPU is ready to be interrupted again
void Terminal::ready_input() {
    current_input = 0;
    CPU_input_busy = false;
}

// Update the current input byte if needed. Returns true if a new input has been loaded
bool Terminal::update_input() {
    int ch;
    // First, empty the ncurses buffer and store on local input queue (this way function keys get processed immediately)
    while ((ch = getch()) != ERR) {
        switch (ch) {
            // The END key pauses execution
            case KEY_HOME: input_buffer.push(23); break;        // From PS2Keyboard.h (Arduino PS/2 controller)
            case KEY_BACKSPACE: input_buffer.push('\b'); break; // Backspace: Send \b
            // Todo: add the rest of special keys

            case KEY_F(5):
                // Pause/unpause emulator
                Globals::is_paused = not Globals::is_paused;
                Globals::single_step = false;
                break;
            case KEY_F(6):
                // Execute 1 instruction
                if (not Globals::is_paused) break;  // F6 only works when paused
                Globals::single_step = true;
                Globals::is_paused = false;
                break;
            case KEY_F(7):
                // Reset cycle counter
                if (not Globals::is_paused) break;  // F7 only works when paused
                Globals::elapsed_cycles = 0;
                break;
            
            // Else, check that all special keys have been catched and send regular input
            default: if (is_regular_char(ch)) input_buffer.push(ch); break;
        }
    }
    
    // Then, get new input only if needed
    if (current_input == 0 and not CPU_input_busy and not input_buffer.empty()) {
        // Load new input and trigger IRQ
        current_input = input_buffer.front();
        input_buffer.pop();
        return true;
    }
    // No new input, don't trigger IRQ
    return false;
}

// Returns true if ch is a regular ascii character
bool Terminal::is_regular_char(int ch) {
    // Make sure all ncurses special keys have been catched
    if (ch > 0xFF) {
        _KILL_GUARD
        destroy(); // Destroy terminal
        fprintf(stderr, "ERROR - Uncatched key: %s (0x%X)\n", keyname(ch), ch);
        exit(EXIT_FAILURE);
    }

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
    if (ch > 0x7F) {
        _KILL_GUARD
        destroy();
        fprintf(stderr, "UNREACHABLE INPUT: %s (0x%X)\n", keyname(ch), ch);
        exit(EXIT_FAILURE);
    }
    return true;
}



Keyboard::Keyboard() { term = Terminal::initialize(); }

// WRITE
MemCell& Keyboard::operator=(word rhs) {
    byte val = rhs & 0x7F;  // Only lower 7 bits are used
    if (val == ACK) term->ack_input();
    else if (val == RDY) term->ready_input();
    else throw "Invalid keyboard command";
    
    return *this;
}
// READ
Keyboard::operator int() const {
    return term->read_input();
}



Display::Display() { term = Terminal::initialize(); }

// WRITE
MemCell& Display::operator=(word rhs) {
    if (busy_flag != 0 and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw "Terminal: attempting to output while the controller was busy";
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
    
    term->output(rhs);
    return *this;
}
// READ
Display::operator int() const {
    return busy_flag;
}
