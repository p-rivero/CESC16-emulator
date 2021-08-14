#pragma once

#include "includes.h"
#include "Memory.h"

#include <curses.h>
#include <termios.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <queue>
#include <fstream>



class Terminal {

private:
    static Terminal *term;
    WINDOW *mainwin, *term_screen, *stat_screen, *perf_screen;
    byte current_input = 0;
    sighandler_t ncurses_stop_handler; // Current SIGTSTP handler, implemented by ncurses
    termios shell_settings; // Terminal settings received from shell
    termios curses_settings; // Terminal settings after setting up ncurses windows
    std::queue<byte> input_buffer; // Buffer for the received keystrokes
    std::ofstream output_file;  // If -o is used, all CPU outputs are stored in output_file
    bool CPU_input_busy = false;

    static const int COLS_STATUS = 15;

    Terminal();
    ~Terminal();
    // Utilities
    static void fatal_error(const char* msg, ...);
    static void draw_rectangle(int y1, int x1, int y2, int x2, const char *text);
    static void sig_handler(int sig);
    static void size_check();
    static bool is_regular_char(int ch);
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
        if (term != NULL) delete term;
        term = NULL;
    }

    // Output a char
    void output(word data);
    // Output status info
    void display_status(word PC, bool user_mode, const StatusFlags& flg, Regfile& regs, double CPI);
    // Flush the output stream
    void flush();

    // Get the current input byte
    byte read_input() const;
    // Acknowledge the current input byte
    void ack_input();
    // CPU is ready to be interrupted again
    void ready_input();
    // Update the current input byte if needed. Returns true if a new input has been loaded
    bool update_input();
};
