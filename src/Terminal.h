#pragma once

#include "includes.h"
#include <curses.h>
#include <termios.h>


class Terminal {

private:
    static Terminal *term;
    WINDOW *mainwin;
    byte current_input = 0;

    Terminal();
    ~Terminal();

public:
    static Terminal *initialize() {
        if (term == NULL) term = new Terminal;
        return term;
    }

    static void destroy() {
        delete term;
        term = NULL;
    }

    // Output a char
    void output(word data);
    // Flush the output stream
    void flush();

    // Get the current input byte
    byte read_input() const;
    // Update the current input byte if needed. Returns true if a new input has been loaded
    bool update_input();
};
