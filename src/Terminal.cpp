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

void Terminal::output(word data) {
    printf("%c", char(data));
}

void Terminal::flush() {
    fflush(stdout);
}

byte Terminal::get_input() {
    int ch = getch();
    if (ch == ERR) return 0; 

    byte result = 0;

    switch (ch) {
        case KEY_END: // END: Terminate execution
            Terminal::destroy();
            exit(EXIT_SUCCESS);
            break;

        case KEY_HOME: result = 23; break;          // From PS2Keyboard.h (Arduino PS/2 controller)
        case KEY_BACKSPACE: result = '\b'; break;   // Backspace: Send \b
        // Todo: add the rest of special keys
        
        default:
            assert((ch & 0xFFFFFF00) == 0); // Make sure all special keys have been catched
            result = ch;  // Else send regular input
    }
    
    return result;
}
