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
    static termios shell_settings; // Terminal settings received from shell
    termios curses_settings; // Terminal settings after setting up ncurses windows
    std::queue<byte> input_buffer; // Buffer for the received keystrokes
    std::ofstream output_file;  // If -o is used, all CPU outputs are stored in output_file
    
    static const int COLS_STATUS = 15;

    Terminal();
    ~Terminal();
    // Utilities
    static void fatal_error(const char* msg, ...);
    static void draw_rectangle(int y1, int x1, int y2, int x2, const char *text);
    static void sig_handler(int sig);
    static void size_check();
    static bool is_regular_char(int ch);
    void init_ncurses();
    void cleanup_ncurses();
    void stop();
    void resume();

public:
    static const int ROWS = 25;
    static const int COLS = 40;
    
    enum color { BLACK=1, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE };
    enum print_mode { BOTH, ONLY_SCREEN, ONLY_FILE };
    
    static Terminal *initialize();
    static void destroy();
    
    // Output a char
    void print(char c, print_mode mode = BOTH);
    // Output status info
    void display_status(word PC, bool user_mode, const StatusFlags& flg, Regfile& regs, double CPI);
    // Flush the output stream
    void flush();
    
    // Process ncurses key queue until it's empty (called periodically)
    void update_input();
    // Returns the first character in the input queue (and removes it from the queue)
    byte get_input();
    // Gets the current cursor coordinates (leaves them in row and col)
    void get_coords(int& row, int& col) const;
    // Sets the current cursor coordinates
    void set_coords(int row, int col);
    // If row>=0, move the cursor to a given row and erase it. Otherwise, clear current line from cursor onward
    void clear_line(int row);
    // Set color of cursor. If row>=0, also move the cursor to a given row and change its color
    void set_color(color c, int row);
    // Enable or disable cursor blinking
    void set_cursor_blink(bool blink);
};
