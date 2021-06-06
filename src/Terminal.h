#pragma once

#include "includes.h"
#include "Memory.h"

#include <curses.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>



class Terminal {

private:
    static Terminal *term;
    WINDOW *mainwin, *term_screen, *stat_screen;
    byte current_input = 0;
    sighandler_t ncurses_stop_handler; // Current SIGTSTP handler, implemented by ncurses
    termios shell_settings; // Terminal settings received from shell
    termios curses_settings; // Terminal settings after setting up ncurses windows

    static const int COLS_STATUS = 15;

    Terminal();
    ~Terminal();
    // Utilities
    static void fatal_error(const char* msg);
    static void draw_rectangle(int y1, int x1, int y2, int x2, const char *text);
    static void sig_handler(int sig);
    static void size_check();
    void stop();
    void resume();

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
    // Output a char
    void display_status(word PC, bool user_mode, const StatusFlags& flg, Regfile& regs);
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
