#include "Terminal.h"

Terminal *Terminal::term = NULL;

void Terminal::fatal_error(const char* msg) {
    destroy(); // Destroy terminal
    fprintf(stderr, "%s\n", msg);
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
    
    if (w.ws_row <= ROWS+1) fatal_error("ERROR - Terminal height too small");
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
    refresh();
}

Terminal::~Terminal(){
    // Clean up
    delwin(term_screen);
    delwin(mainwin);
    endwin();
    refresh();

    // Restore correct settings for shell
    tcsetattr(0, TCSANOW, &shell_settings);
}


// Output a char
void Terminal::output(word data) {
    wprintw(term_screen, "%c", char(data));
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

    wprintw(stat_screen, "\n CPI: %.4lf\n", CPI);
}

// Flush the output stream
void Terminal::flush() {
    wrefresh(stat_screen);
    wrefresh(term_screen);
    curs_set(1);
}

// Get the current input byte
byte Terminal::read_input() const {
    return current_input;
}

// Acknowledge the current input byte
void Terminal::ack_input() {
    current_input = 0;
}

// Update the current input byte if needed. Returns true if a new input has been loaded
bool Terminal::update_input() {
    // Only get new input if needed
    if (current_input != 0) return false;

    int ch = getch();
    if (ch == ERR) return false;

    switch (ch) {
        case KEY_END: // END: Terminate execution
            Terminal::destroy();
            exit(EXIT_SUCCESS);
            break;

        case KEY_HOME: current_input = 23; break;          // From PS2Keyboard.h (Arduino PS/2 controller)
        case KEY_BACKSPACE: current_input = '\b'; break;   // Backspace: Send \b
        // Todo: add the rest of special keys
        
        default:
            // Make sure all special keys have been catched
            if (ch > 0xFF) {
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
            current_input = ch;  // Else send regular input
            break;
    }
    
    return true;
}



Keyboard::Keyboard() { term = Terminal::initialize(); }

// WRITE
MemCell& Keyboard::operator=(word rhs) {
    // Todo: check if rhs=ACK
    term->ack_input();
    return *this;
}
// READ
Keyboard::operator int() const {
    return term->read_input();
}



Display::Display() { term = Terminal::initialize(); }

// WRITE
MemCell& Display::operator=(word rhs) {
    term->output(rhs);
    return *this;
}
// READ
Display::operator int() const {
    // Todo: wait for some milliseconds before clearing ready flag
    return 0;
}
