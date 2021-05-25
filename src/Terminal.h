#pragma once

#include "includes.h"
#include <curses.h>
#include <termios.h>


class Terminal {

private:
    static Terminal *term;
    WINDOW *mainwin;

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

    void output(word data);
    void flush();
    byte get_input();
};
