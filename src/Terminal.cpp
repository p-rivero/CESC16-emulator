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

void Terminal::sig_handler(int sig) { size_check(); }

void Terminal::size_check() {
    winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    
    if (w.ws_row <= ROWS+R_STATUS+3) fatal_error("ERROR - Terminal height too small");
    if (w.ws_col <= COLS+C_STATUS+3) fatal_error("ERROR - Terminal width too small");
}


Terminal::Terminal(){
    // Initialize main window
    mainwin = initscr();
    if (mainwin == NULL)
        fatal_error("Error initializing main window!\r");

    // Change line endings from CRLF to LF
    struct termios settings;
    tcgetattr(0, &settings);
    settings.c_oflag |= ONLCR; 
    tcsetattr(0, TCSANOW, &settings);

    // Initialize subwindow (terminal output)
    term_screen = newwin(ROWS, COLS, 1, 1);
    if (term_screen == NULL)
        fatal_error("Error initializing subwindow!");

    // Initialize subwindow (status)
    stat_screen = newwin(R_STATUS, C_STATUS, ROWS+2, COLS+2);
    if (stat_screen == NULL)
        fatal_error("Error initializing subwindow!");

    noecho(); // Turn off key echoing
    keypad(mainwin, TRUE); // Enable the keypad for non-char keys
    nodelay(mainwin, TRUE); // Enable non-blocking input
    scrollok(term_screen, true); // Enable scrolling for terminal subwindow

    // SIGWINCH is triggered whenever the user resizes the window
    if (signal(SIGWINCH, sig_handler) == SIG_ERR)
        fatal_error("Error: Couldn't catch SIGWINCH!");

    // Draw frames around subwindows
    draw_rectangle(0, 0, ROWS+1, COLS+1, "Terminal output");
    draw_rectangle(ROWS+2, COLS+2, ROWS+R_STATUS+3, COLS+C_STATUS+3, "Status");
    refresh();
    wrefresh(term_screen);
}

Terminal::~Terminal(){
    // Clean up
    delwin(term_screen);
    delwin(mainwin);
    endwin();
    refresh();
}


// Output a char
void Terminal::output(word data) {
    wprintw(term_screen, "%c", char(data));
}

// Flush the output stream
void Terminal::flush() {
    curs_set(0);
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
            // UTF-8 encoded characters, ignore them by default
            if (ch == 0xC2 or ch == 0xC3) {
                assert(getch());
                return false;
            }
            // Make sure all special keys have been catched
            if (ch > 0x7F) {
                destroy(); // Destroy terminal
                fprintf(stderr, "ERROR - Uncatched key: %s (0x%X)\n", keyname(ch), ch);
                exit(EXIT_FAILURE);
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
