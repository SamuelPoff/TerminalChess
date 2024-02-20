#ifndef H_TERMINAL_CONTROL
#define H_TERMINAL_CONTROL

#include "ansi_colors.h"

#define tc_clear_screen() printf("\033[2J\033[3J")
#define tc_cursor_to_home() printf("\033[H")

#define tc_set_text_color(color) printf("\x1B[38;5;%dm", color)
#define tc_set_bg_color(color) printf("\x1B[48;5;%dm", color)
#define tc_set_text_color_rgb(r,g,b) printf("\x1B[38;2;%d;%d;%dm", r, g, b);
#define tc_set_bg_color_rgb(r,g,b) printf("\x1B[48;2;%d;%d;%dm", r, g, b);

#define tc_hide_cursor() printf("\x1B[?25l")
#define tc_reveal_cursor() printf("\x1B[?225h");
#define tc_reset_style() printf(ANSI_RESET_ALL)

void tc_set_cursor_position(int x, int y);

void tc_set_cursor_position(int x, int y){ 
    printf("\x1B[%d;%df", y, x);
    }

void tc_set_cursor_column(int column){
    printf("\x1B%dG", column);
}


#endif