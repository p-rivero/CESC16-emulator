#include "Terminal.h"

Terminal *Terminal::term = NULL;

Terminal::Terminal(){
    mainwin = initscr();
    if (mainwin == NULL) {
        fprintf(stderr, "Error initializing screen!\n");
        exit(127);
    }

    noecho();   // Turn off key echoing
    keypad(mainwin, TRUE);  // Enable the keypad for non-char keys
    nodelay(mainwin, TRUE); // Enable non-blocking input

    // Change line endings from CRLF to LF
    struct termios settings;
    tcgetattr(0, &settings);
    settings.c_oflag |= ONLCR; 
    tcsetattr(0, TCSANOW, &settings);
}

Terminal::~Terminal(){
    // Clean up
    delwin(mainwin);
    endwin();
    refresh();
}


// Output a char
void Terminal::output(word data) {
    printf("%c", char(data));
}

// Flush the output stream
void Terminal::flush() {
    fflush(stdout);
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
            if (ch & 0xFFFFFF00) {
                destroy(); // Destroy terminal
                fprintf(stderr, "ERROR - Uncatched key: %s (0x%8X)\n", keyname(ch), ch);
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
