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

