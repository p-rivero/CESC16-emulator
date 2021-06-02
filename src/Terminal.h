#pragma once

#include "includes.h"
#include "Memory.h"

#include <curses.h>
#include <termios.h>
#include <signal.h>
#include <sys/ioctl.h>



class Terminal {

private:
    static Terminal *term;
    WINDOW *mainwin, *term_screen, *stat_screen;
    byte current_input = 0;

    static const int R_STATUS = 25;
    static const int C_STATUS = 7;

    Terminal();
    ~Terminal();
    // Utilities
    static void fatal_error(const char* msg);
    static void draw_rectangle(int y1, int x1, int y2, int x2, const char *text);
    static void sig_handler(int sig);
    static void size_check();

public:
    static const int ROWS = 25;
    static const int COLS = 40;

    static Terminal *initialize() {
        if (term == NULL) term = new Terminal;
        // Make sure window is big enough
        size_check();
        return term;
    }

    static void destroy() {
        term->flush(); // Discard any buffered outputs
        delete term;
        term = NULL;
    }

    // Output a char
    void output(word data);
    // Flush the output stream
    void flush();

    // Get the current input byte
    byte read_input() const;
    // Acknowledge the current input byte
    void ack_input();
    // Update the current input byte if needed. Returns true if a new input has been loaded
    bool update_input();
};


class Keyboard : public MemCell {
private:
    Terminal *term;

public:
    Keyboard();

    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
};


class Display : public MemCell {
private:
    Terminal *term;

public:
    Display();

    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
};
