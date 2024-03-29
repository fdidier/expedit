#include "signal.h"
#include "unistd.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "sys/types.h"

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

using namespace std;

typedef unsigned char uchar;

/* text macro */
#define isprint(c) (((c) >= 32 && (c) < 128) || (c) >= 256)
#define issmall(c) ((c) >= 'a' && (c) <= 'z')
#define isbig(c) ((c) >= 'A' && (c) <= 'Z')
#define isnum(c) ((c) >= '0' && (c) <= '9')
#define isletter(c) (issmall(c) || isbig(c) || (c) == '_' || (uchar)(c) >= 128)
#define isalphanum(c) (isletter(c) || isnum(c))
#define isspecial(c) (!isalphanum(c) && (uchar)(c) > 32 && (c) != ' ')

#define EOL '\n'

#define TABSTOP 2
#define JUSTIFY 80  // Justification will cut line <= JUSTIFY (without eol).

/* function declaration */
extern vector<int> text;
extern int text_lines;
extern int text_l;
extern int text_gap;
extern int undo_pos;
extern int text_restart;
extern int text_end;
extern int text_compute_position(int begin, int size);
extern int text_line_begin(int);
extern int text_save();

extern void mouse_select(int b, int e);
extern void mouse_word_select(int* left, int* right);
extern void mouse_delete(int b, int e);
extern void mouse_paste();
extern void text_move_from_screen(int);
extern void text_line_goto(int);

extern int match(vector<int>& s, int i);
extern int text_real_position(int i);
extern int search_highlight;
extern int display_pattern;
extern vector<int> pattern;
extern string text_message;

extern int screen_get_first_line();
extern void screen_set_first_line(int);
extern void screen_npage();
extern void screen_ppage();

extern void screen_init();
extern void screen_redraw();
extern void screen_refresh();
extern uint screen_lsize;
extern void screen_ol();
extern int screen_getchar();

extern int term_rawchar();
extern int term_getchar();
extern void term_set_title(uchar*);
extern void reset_input_mode();
