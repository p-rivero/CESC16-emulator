#pragma once

#include "includes.h"
#include "Memory.h"

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
