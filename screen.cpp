#include "definition.h"
#include "term.h"

/*****************************************************************************/
/* Screen Handling                                                           */
/*****************************************************************************/

/* screen dimension */
uint screen_lines;
uint screen_columns;

/* screen currently displayed */
uint **screen_real;
uint **color_real;
uint screen_real_i; /* cursor line in screen */
uint screen_real_j; /* cursor pos  in screen */

int cursor_hiden = 0;

/* screen we want to display */
uint **screen_wanted;
uint **color_wanted;
uint screen_wanted_i;
uint screen_wanted_j;

// Line number of the first line of the screen that we want.
uint first_line;

/* Other variables */
int screen_scroll_hint;

/*****************************************************************************/
/* screen allocation functions (2 dim array)                                 */
/*****************************************************************************/

uint **screen_alloc_internal(int i, int j) {
  uint *temp;
  uint **array;
  int k;

  temp = (uint *)malloc(i * j * sizeof(int));
  array = (uint **)malloc(i * sizeof(int *));

  for (k = 0; k < i; k++) {
    array[k] = &temp[k * j];
    for (int l = 0; l < j; l++) {
      array[k][l] = 0;
    }
  }

  return array;
}

void screen_free_internal(uint **a) {
  if (a) {
    free(a[0]);
    free(a);
  }
}

void screen_alloc() {
  screen_free_internal(screen_wanted);
  screen_free_internal(color_wanted);
  screen_free_internal(screen_real);
  screen_free_internal(color_real);
  screen_wanted = screen_alloc_internal(screen_lines, screen_columns + 1);
  color_wanted = screen_alloc_internal(screen_lines, screen_columns + 1);
  screen_real = screen_alloc_internal(screen_lines, screen_columns + 1);
  color_real = screen_alloc_internal(screen_lines, screen_columns + 1);
}

/************************************************************************/
/* Function that change the real screen                                 */
/************************************************************************/

/* Clear the real screen */
void screen_clear() {
  term_normal_style();
  CLEAR_ALL;
  for (uint i = 0; i < screen_lines; i++) {
    for (uint j = 0; j < screen_columns; j++) {
      screen_real[i][j] = EOL;
      color_real[i][j] = 0;
    }
  }
  HOME;
  screen_real_i = 0;
  screen_real_j = 0;
}

/* Change the real cursor position if necessary */
void screen_move_curs(int i, int j) {
  if (screen_real_j == screen_columns + 1) {
    screen_real_j = 0;
    screen_real_i++;
  }

  if (i < 0 || i >= screen_lines) {
    if (!cursor_hiden) {
      HIDE_CURSOR;
      cursor_hiden = 1;
    }
    return;
  } else {
    if (cursor_hiden) {
      SHOW_CURSOR;
      cursor_hiden = 0;
    }
  }

  if ((screen_real_i != i) || (screen_real_j != j)) {
    MOVE(i, j);
    screen_real_i = i;
    screen_real_j = j;
  }
}

/* Print new line */
void screen_newl() {
  term_newl();
  screen_real_i++;
  if (screen_real_i == screen_lines) screen_real_i--;
  screen_real_j = 0;
}

/* Print a char */
void screen_print(int c, int color = 0) {
  term_putchar(c, color);
  screen_real_j++;
  if (screen_real_j == screen_columns + 1) {
    screen_real_j = 0;
    screen_real_i++;
  }
}

/* Dump a part of screen wanted on a cleared real screen */
void screen_dump_wanted(int a, int b) {
  int w, i, j;

  screen_move_curs(screen_real_i, 0);

  for (i = a; i < b; i++) {
    j = 0;
    while (j < screen_columns) {
      w = screen_wanted[i][j];
      if (w == EOL) {
        if (i != (b - 1)) screen_newl();
        break;
      }

      screen_print(w, color_wanted[i][j]);
      j++;
    }
  }
}

/* Make the real screen look like the wanted one */
void screen_make_it_real() {
  uint i;
  uint w, r;

  /* Use the scroll hint */
  uint a = 0;
  uint b = 0;
  uint off = 0;

  if (screen_scroll_hint > 0) {
    if (screen_scroll_hint >= (int)screen_lines) {
      screen_clear();
      screen_dump_wanted(0, screen_lines);
      return;
    }
    a = (uint)screen_scroll_hint;
    off = (uint)screen_scroll_hint;
  }

  if (screen_scroll_hint < 0) {
    a = b = (uint) - screen_scroll_hint;
    if (a >= screen_lines) {
      screen_clear();
      screen_dump_wanted(0, screen_lines);
      return;
    }

    screen_move_curs(0, 0);
    term_normal_style();
    SCROLL_DOWN(a);
    screen_move_curs(0, 0);
    screen_dump_wanted(0, a);
  }

  for (i = a; i < screen_lines; i++) {
    r = 0;
    for (int j = 0; j < screen_columns; ++j) {
      w = screen_wanted[i - off][j];
      if (r != '\n') r = screen_real[i - b][j];

      if (w == '\n') {
        if (r != '\n') {
          screen_move_curs(i, j);
          CLEAR_EOL;
          if (i != (screen_lines - 1)) screen_newl();
        }
        break;
      }

      if ((w != r) || (color_wanted[i - off][j] != color_real[i - b][j])) {
        screen_move_curs(i, j);
        screen_print(w, color_wanted[i - off][j]);
      }
    }
  }

  if (off) {
    if (screen_real_i != screen_lines - 1)
      screen_move_curs(screen_lines - 1, 0);
    screen_newl();
    screen_dump_wanted(screen_lines - off, screen_lines);
  }
}

// Function to call when the real screen is equal to the wanted one
void screen_done() {
  // Put the cursor as wanted.
  screen_move_curs(screen_wanted_i, screen_wanted_j);
  screen_scroll_hint = 0;

  // Swap the two screen array to update screen real.
  uint **temp = screen_real;
  screen_real = screen_wanted;
  screen_wanted = temp;

  // Swap the two color arrays.
  temp = color_real;
  color_real = color_wanted;
  color_wanted = temp;

  // Flush all the chars.
  fflush(stdout);
}

/*****************************************************************************/
/* Specific functions for displaying a text structure                        */
/*****************************************************************************/

// if there is line number
int shift = 0;
int opt_line = 1;

// set first_line using the hint
void screen_set_first_line(int hint) {
  if (hint < 0) hint = 0;
  if (text_lines != 0 && hint >= text_lines) hint = text_lines - 1;
  first_line = hint;
}

// Compute the first line displayed according to
// cursor movement since last time
void screen_movement_hint() {
  // current line if we use the same first line
  // or the wanted one.
  int temp = int(text_l) - int(first_line);

  if (temp <= 0) {
    if (text_l == 0)
      first_line = 0;
    else
      first_line = text_l - 1;
  }

  int top = screen_lines - 2;
  if (temp > top) {
    first_line = int(text_l) - top;
  }
}

// Compute scroll hint to speed up display
int saved_first_line;
void compute_scroll_hint() {
  screen_scroll_hint = first_line - saved_first_line;
  saved_first_line = first_line;
}

// compute cursor position according to the first line
void compute_cursor_pos() {
  // compute screen_wanted_i
  screen_wanted_i = text_l - first_line;

  // compute screen_wanted_j
  int l = text_gap - text_line_begin(text_l);
  if (l < 0) l = 0;
  screen_wanted_j = shift + min(l, int(screen_columns - shift - 1));
}

// compute the wanted view of the screen
void screen_compute_wanted() {
  // line numbers
  if (opt_line) {
    int num = text_lines;
    shift = 1;
    while (num) {
      shift++;
      num /= 10;
    }
    shift = max(4, shift);
  } else {
    shift = 0;
  }

  compute_cursor_pos();

  // fill the line one by one
  for (int n = 0; n < screen_lines; ++n) {
    // pos on screen
    int j = 0;

    // start by the line number
    int num = first_line + n + 1;

    // line numbers,
    // special case for the last line
    if (num > text_lines) {
      while (j + 1 < shift) {
        screen_wanted[n][j] = '-';
        j++;
      }
      screen_wanted[n][j] = ' ';
      j++;
    } else {
      while (j < shift) {
        if (num == 0 || j == 0) {
          screen_wanted[n][shift - 1 - j] = ' ';
        } else {
          screen_wanted[n][shift - 1 - j] = (num % 10) + '0';
          num /= 10;
        }
        j++;
      }
    }

    // display only if lines exist
    if (num <= text_lines + 1) {
      // beginning of the current line
      int i = text_line_begin(first_line + n);
      if (i == text_gap) i = text_restart;

      // special case if line is bigger than screen width
      // only for text_l
      if (first_line + n == text_l && text_gap > i) {
        int pos = text_gap - i;
        if (pos >= screen_columns - shift)
          i = text_gap - (screen_columns - shift - 1);
      }

      // display the line
      while (i < text_end && text[i] != EOL && j < screen_columns) {
        screen_wanted[n][j] = text[i];
        j++;
        i++;
        if (i == text_gap) i = text_restart;
      }

      // display indicator for long line.
      if (shift > 0 && j == screen_columns && i < text_end && text[i] != EOL) {
        screen_wanted[n][shift - 1] = '>';
        color_wanted[n][shift - 1] = YELLOW;
      }
    }

    // fill the end with white
    screen_wanted[n][j] = EOL;
    while (j + 1 < screen_columns) {
      screen_wanted[n][j + 1] = ' ';
      j++;
    }
  }
}

// ****************************************************************************
// ****************************************************************************

void display_message() {
  int j = 0;
  if (display_pattern) {
    int i = 0;
    while (i < pattern.size() && j < screen_columns) {
      if (pattern[i] == EOL) pattern[i] = 'J';
      screen_wanted[screen_lines - 1][j] = pattern[i];
      color_wanted[screen_lines - 1][j] = NO_STYLE | REVERSE;
      i++;
      j++;
    }
    screen_wanted[screen_lines - 1][j] = EOL;
    color_wanted[screen_lines - 1][j] = 0;
  }

  if (text_message.empty()) return;

  // begin of line version
  int i = 0;
  while (i < text_message.size() && j < screen_columns) {
    if (text_message[i] == EOL) text_message[i] = 'J';
    screen_wanted[screen_lines - 1][j] = text_message[i];
    color_wanted[screen_lines - 1][j] = NO_STYLE | REVERSE;
    i++;
    j++;
  }
  screen_wanted[screen_lines - 1][j] = EOL;
  color_wanted[screen_lines - 1][j] = 0;
}

// ***********************************************************
// * Style ***************************************************
// ***********************************************************

// Most highlights are just line based.
// TODO: Remenber if line didn't change ?

void add_limit_bar() {
  int limit = shift + 81;
  if (limit > screen_columns) return;
  for (int i = 0; i < screen_lines; ++i) {
    bool eol = false;
    for (int j = 0; j < limit; j++) {
      if (eol || screen_wanted[i][j] == EOL) {
        eol = true;
        screen_wanted[i][j] = ' ';
        color_wanted[i][j] = NO_STYLE;
      }
    }
    if (eol) {
      screen_wanted[i][limit] = EOL;
      screen_wanted[i][limit - 1] = '|';
      color_wanted[i][limit - 1] = YELLOW;
    }
  }
}

void highlight_clear() {
  for (int i = 0; i < screen_lines; ++i)
  for (int j = 0; j < screen_columns; ++j) {
    if (j < shift) {
      color_wanted[i][j] = YELLOW;
    } else {
      color_wanted[i][j] = NO_STYLE;
    }
  }
}

// Just a line by line highlight. Also highlight includes.
void highlight_comment() {
  for (int i = 0; i < screen_lines; ++i) {
    int color = 0;
    for (int j = shift; j < screen_columns; j++) {
      if (screen_wanted[i][j] == EOL) break;
      char c = screen_wanted[i][j];
      char next = EOL;
      if (j + 1 < screen_columns) {
        next = screen_wanted[i][j + 1];
      }
      bool no_color = (color_wanted[i][j] == 0);
      if (!color) {
        if (c == '#' && no_color) color = MAGENTA;
        if (j == shift + 1 && screen_wanted[i][j] == '*' && screen_wanted[i][j-1] == ' ') color = RED;
        if (c == '/' && no_color && (next == '/' || next == '*')) color = RED;
      } else {
        if (j > 2 && screen_wanted[i][j-1] == '/' && screen_wanted[i][j-2] == '*') color = 0;
      }
      if (color) color_wanted[i][j] = color;
    }
  }
}

void highlight_string() {
  const int color = GREEN;
  for (int i = 0; i < screen_lines; ++i) {
    int in = 0;
    int start = 0;
    for (int j = 0; j < screen_columns; ++j) {
      int c = screen_wanted[i][j];
      if (in) {
        color_wanted[i][j] = color;
        // Skip escaped char inside quote.
        if (c == '\\') {
          j++;
          if (j < screen_columns) color_wanted[i][j] = color;
          continue;
        }
        if (c == start) in = 0;
      } else {
        if (c == '\'' || c == '"') {
          in = 1;
          start = c;
          color_wanted[i][j] = color;
        }
      }
    }
    // Undo if comment didn't finish.
    if (in) {
      int j = screen_columns - 1;
      while (j > 0 && color_wanted[i][j] == color) {
        color_wanted[i][j] = NO_STYLE;
        j--;
        int c = screen_wanted[i][j];
        if (c == start) break;
      }
    }
  }
}

void highlight_maj() {
  for (int i = 0; i < screen_lines; ++i) {
    for (int j = shift; j < screen_columns; j++) {
      if (isbig(screen_wanted[i][j]) &&
          (j == shift || !isletter(screen_wanted[i][j - 1]))) {
        do {
          color_wanted[i][j] = BOLD;
          j++;
        } while (j < screen_columns && isalphanum(screen_wanted[i][j]));
      }
    }
  }
}

void highlight_definition() {
  for (int i = 0; i < screen_lines; ++i) {
    for (int j = screen_columns - 1; j >= shift;) {
      if (screen_wanted[i][j] != '('
          && screen_wanted[i][j] != ')'
          && screen_wanted[i][j] != ';'
          && screen_wanted[i][j] != ':'
          && screen_wanted[i][j] != '='
          && screen_wanted[i][j] != ',') {
        --j;
        continue;
      }
      --j;
      for (;j >= shift && screen_wanted[i][j] == ' '; --j);
      const int last = j;
      for (;j >= shift && isalphanum(screen_wanted[i][j]); --j);
      if (j == last) continue;
      for (;j >= shift && screen_wanted[i][j] == ' '; --j);
      if (j == shift) break;
      if (color_wanted[i][j] != 0) continue;
      const int previous = screen_wanted[i][j - 1];
      const int current = screen_wanted[i][j];
      if ((current == '&' && previous != '&')
          || current == '*'
          // This one is broken because of a > b, but we need something like
          // this for template type :(
          || (current == '>' && previous != ' ' && previous != '-')
          || isalphanum(current)) {
        for (int k = j + 1; k <= last; ++k) {
          color_wanted[i][k] = YELLOW;
        }
        // TODO: color multiple def  def, word, word, word ...
      }
    }
  }
}

void highlight_number() {
  for (int i = 0; i < screen_lines; ++i) {
    for (int j = shift; j < screen_columns; j++) {
      if (isnum(screen_wanted[i][j])) {
        int previous = (j == shift) ? EOL : screen_wanted[i][j-1];
        const bool highlight = !isletter(previous);
        while (j < screen_columns && isnum(screen_wanted[i][j])) {
          if (highlight) color_wanted[i][j] = CYAN;
          ++j;
        }
      }
    }
  }
}

// Highlight the two enclosing {} [] or () of the cursor.
// Text already colored (!= NO_STYLE) is skipped so string and comment
// are ignored.
void highlight_bracket_before() {
  if (screen_wanted_i < 0 || screen_wanted_i >= screen_lines) return;
  if (screen_wanted_j < 0 || screen_wanted_j >= screen_columns) return;

  int inside = 0;
  int count = 0;
  int b = '?';
  int e = '?';

  int j = screen_wanted_j;
  for (int i = screen_wanted_i; i >= 0; i--) {
    for (; j >= 0; j--) {
      if (color_wanted[i][j] != NO_STYLE) continue;
      int c = screen_wanted[i][j];
      if (inside) {
        if (c == b) count++;
        if (c == e) count--;
        if (count < 0) inside = 0;
      } else {
        if (c == '(' || c == '{' || c == '[') {
          if (i != screen_wanted_i || j != screen_wanted_j) {
            color_wanted[i][j] = BG_COLOR(YELLOW);
          }
          return;
        }
        if (i != screen_wanted_i || j != screen_wanted_j) {
          if (c == ')' || c == '}' || c == ']') {
            inside = 1;
            b = c;
            count = 0;
            if (b == ')') e = '(';
            if (b == '}') e = '{';
            if (b == ']') e = '[';
          }
        }
      }
    }
    j = screen_columns - 1;
  }
}

void highlight_bracket_after() {
  if (screen_wanted_i < 0 || screen_wanted_i >= screen_lines) return;
  if (screen_wanted_j < 0 || screen_wanted_j >= screen_columns) return;

  int inside = 0;
  int count = 0;
  int b = '?';
  int e = '?';

  int j = screen_wanted_j;
  for (int i = screen_wanted_i; i < screen_lines; i++) {
    for (; j < screen_columns; j++) {
      if (color_wanted[i][j] != NO_STYLE) continue;
      int c = screen_wanted[i][j];
      if (inside) {
        if (c == b) count++;
        if (c == e) count--;
        if (count < 0) inside = 0;
      } else {
        if (c == ')' || c == '}' || c == ']') {
          if (i != screen_wanted_i || j != screen_wanted_j) {
            color_wanted[i][j] = BG_COLOR(YELLOW);
          }
          return;
        }
        if (i != screen_wanted_i || j != screen_wanted_j) {
          if (c == '(' || c == '{' || c == '[') {
            inside = 1;
            b = c;
            count = 0;
            if (b == '(') e = ')';
            if (b == '{') e = '}';
            if (b == '[') e = ']';
          }
        }
      }
    }
    j = 0;
  }
}

map<string, int> keyword;
void init_keywords() {
  int color = BLUE;
  keyword["if"]=color;
  keyword["else"]=color;
  keyword["for"]=color;
  keyword["while"]=color;
  keyword["do"]=color;
  keyword["return"]=color;
  keyword["break"]=color;
  keyword["continue"]=color;
  keyword["switch"]=color;
  keyword["case"]=color;
  keyword["default"]=color;

  keyword["this"]=color;
  keyword["const"]=color;

  keyword["struct"]=color;
  keyword["class"]=color;
  keyword["typedef"]=color;
  keyword["using"]=color;
  keyword["template"]=color;

  keyword["private"]=color;
  keyword["public"]=color;
  keyword["mutable"]=color;
  keyword["inline"]=color;
  keyword["namespace"]=color;
}

void highlight_keywords() {
  for (int i = 0; i < screen_lines; ++i) {
    int j = 0;
    int s = 0;
    while (j < screen_columns) {
      if (screen_wanted[i][j] == '\n') break;
      while (j < screen_columns && !isletter(screen_wanted[i][j])) j++;

      string word;
      s = j;
      while (j < screen_columns && isletter(screen_wanted[i][j])) {
        word.push_back(screen_wanted[i][j]);
        j++;
      }
      if (keyword.find(word) != keyword.end()) {
        int color = keyword[word];
        while (s < j) {
          if (color_wanted[i][s] != MAGENTA) color_wanted[i][s] = color;
          s++;
        }
      }
    }
  }
}

void highlight_search() {
  if (search_highlight == 0) return;

  int size = pattern.size();
  if (size == 0) return;

  if (pattern[0] == ' ') size--;
  if (pattern[pattern.size() - 1] == ' ') size--;

  for (int i = 0; i < screen_lines; ++i) {
    int begin = text_line_begin(first_line + i);

    for (int j = shift; j < screen_columns; j++) {
      // Find the real begin in the text.
      int pos = text_compute_position(begin, j - shift);
      if (match(pattern, pos)) {
        for (int k = 0; k < size; ++k) {
          color_wanted[i][j] = BG_COLOR(RED);  // |= REVERSE;
          j++;
          if (j >= screen_columns) break;
        }
      }
    }
  }
}

void screen_highlight() {
  highlight_clear();
  // highlight_maj();
  highlight_number();
  highlight_keywords();
  highlight_string();
  highlight_definition();
  highlight_comment();
  highlight_bracket_before();
  highlight_bracket_after();
  highlight_search();
  add_limit_bar();
}

/*****************************************************************************/
/* Function called on a screen resize event                                  */
/*****************************************************************************/

void screen_sigwinch_handler(int sig) {
  term_get_size(screen_lines, screen_columns);
  screen_alloc();
  screen_redraw();
}

/*****************************************************************************/
/* Interface Functions                                                       */
/*****************************************************************************/

void screen_init() {
  init_keywords();

  /* Initialize the terminal */
  term_init();

  /* SIGWINCH signal handling */
  (void)signal(SIGWINCH, screen_sigwinch_handler);

  /* Get terminal dimension */
  term_get_size(screen_lines, screen_columns);

  /* Allocating the screen array */
  screen_alloc();

  /* clear real screen */
  screen_clear();
}

void screen_redraw() {
  screen_compute_wanted();
  screen_highlight();
  display_message();

  term_reset();
  screen_clear();
  screen_dump_wanted(0, screen_lines);
  screen_done();
}

void screen_refresh() {
  compute_scroll_hint();
  screen_compute_wanted();
  screen_highlight();
  display_message();
  screen_make_it_real();
  screen_done();
}

int saved_line;

int screen_get_first_line() { return first_line; }

void screen_ppage() {
  int dest = text_l - screen_lines / 2;
  if (dest < 0) dest = text_l;
  text_move_from_screen(text_line_begin(dest));
  screen_set_first_line(dest - screen_lines / 2);
}

void screen_npage() {
  int dest = text_l + screen_lines / 2;
  if (dest >= text_lines) dest = text_l;
  text_move_from_screen(text_line_begin(dest));
  screen_set_first_line(dest - screen_lines / 2);
}

void screen_ol() {
  if (opt_line)
    opt_line = 0;
  else
    opt_line = 1;
}

string debug;

//***********************************************************
//** MOUSE HANDLING
//***********************************************************

// different from usual bindings :
//
// - whell scroll but do not change cursor pos.
//
// - simple right click move the cursor
// - left click (w/wo move) start selection
// - left click on number : select whole line.
//
// on selection [NOT THE CASE ANYMORE]
// - left click inside, paste selection at cursor
// - right click inside, del selection

void mouse_highlight(int l1, int p1, int l2, int p2, int line_mode) {
  if (l1 > l2) {
    swap(l1, l2);
    swap(p1, p2);
  }
  if (l1 == l2 && p1 > p2) {
    swap(p1, p2);
  }

  int i = l1 - first_line;
  if (i < 0) {
    i = 0;
    p1 = 0;
  }

  int imax = l2 - first_line;
  if (imax >= int(screen_lines)) imax = screen_lines - 1;

  int j = max(p1 + shift, shift);
  int jmax = p2 + shift;
  int end = 0;

  if (line_mode) {
    j = shift;
    jmax = screen_columns;
  }

  while (i < imax || (i == imax && j <= jmax)) {
    if (screen_wanted[i][j] == EOL) {
      end = 1;
      if (i == imax) jmax = screen_columns;
      screen_wanted[i][screen_columns] = EOL;
    }
    color_wanted[i][j] = NO_STYLE | REVERSE;
    if (end) {
      screen_wanted[i][j] = ' ';
    }
    j++;
    if (j == screen_columns) {
      j = shift;
      end = 0;
      i++;
    }
  }
}

int inside(int mi_l, int mi_p, int ma_l, int ma_p, int l, int p) {
  if (l < mi_l) return 0;
  if (l > ma_l) return 0;
  if (p < 0) return 1;
  if (l == mi_l && p < mi_p) return 0;
  if (l == ma_l && p > ma_p) return 0;
  return 1;
}

void get_pos(int f_line, int f_pos, int l_line, int l_pos, int &b, int &e) {
  b = text_line_begin(f_line);
  e = text_line_begin(l_line);
  if (b == text_gap) b = text_restart;
  if (e == text_gap) e = text_restart;

  int i = 0;
  while (i < f_pos && text[b] != EOL) {
    b++;
    i++;
    if (b == text_gap) b = text_restart;
  }
  if (i != f_pos) b = text_line_begin(f_line + 1);

  i = 0;
  while (i < l_pos && text[e] != EOL) {
    e++;
    i++;
    if (e == text_gap) e = text_restart;
  }
}

int mouse_handling() {
  int c;
  int select = 0;
  int pending = 0;
  int move = 0;
  int line_mode = 0;
  int f_pos;
  int f_line;
  int l_pos;
  int l_line;
  int fixed = 0;

  while (1) {
    mevent e = mevent_stack[mevent_stack.size() - 1];
    mevent_stack.erase(mevent_stack.end() - 1);

    int line = first_line + e.y;
    int pos = e.x - shift;

    switch (e.button) {
      case MOUSE_M:
        // paste.
        mouse_paste();
        return MOUSE_EVENT;
      case MOUSE_R:
        pending = 1;
        fixed = 0;
        select = 0;
        move = 1;

        f_line = line;
        f_pos = pos;

        // select line.
        line_mode = 0;
        if (f_pos < 0) {
          select = 1;
          move = 0;
          line_mode = 1;
        }
        break;
      case MOUSE_L:
        // TODO select id?
        pending = 1;
        select = 1;
        move = 0;
        fixed = 1;
        f_line = line;
        l_line = line;
        f_pos = pos;
        l_pos = pos;
        line_mode = 0;
        break;
      case WHEEL_UP:
        screen_set_first_line(first_line - 4);
        break;
      case WHEEL_DOWN:
        screen_set_first_line(first_line + 4);
        break;
      case MOUSE_RELEASE:
        pending = 0;
        if (move) {
          if (text_line_begin(line) + pos == text_gap) {
            int left;
            int right;
            mouse_word_select(&left, &right);
            f_line = line;
            l_line = line;
            f_pos = pos - left;
            l_pos = pos + right;
          } else {
            text_move_from_screen(text_line_begin(line));
            text_line_goto(pos);
            if (e.y == 0) screen_set_first_line(first_line - 1);
            if (e.y == screen_lines - 1) screen_set_first_line(first_line + 1);
          }
        }
        if (select) {
          // reorder points
          if (l_line < f_line || (l_line == f_line && l_pos <= f_pos)) {
            swap(l_line, f_line);
            swap(l_pos, f_pos);
          }

          if (line_mode) {
            f_pos = 0;
            l_pos = screen_columns;
          }

          int b, e;
          get_pos(f_line, f_pos, l_line, l_pos, b, e);
          mouse_select(b, e);
        }
    }

    if (pending && !fixed) {
      l_pos = pos;
      l_line = line;
    }
    if (move && (l_pos != f_pos || l_line != f_line)) {
      move = 0;
      select = 1;
    }

    compute_scroll_hint();
    screen_compute_wanted();
    screen_highlight();

    if (select) mouse_highlight(f_line, f_pos, l_line, l_pos, line_mode);
    display_message();
    screen_make_it_real();
    screen_done();

    c = term_getchar();
    if (c != MOUSE_EVENT) break;
  }

  return c;
}

vector<int> buffer;

// Refresh the screen as a side effect.
int screen_getchar() {
  int c;
  if (!buffer.empty()) {
    c = buffer[buffer.size() - 1];
    buffer.erase(buffer.end() - 1);
    return c;
  }

  while (1) {
    if (!debug.empty()) {
      text_message = debug;
      debug.clear();
    }

    // refresh screen
    screen_movement_hint();
    screen_refresh();

    c = term_getchar();
    if (c == MOUSE_EVENT) {
      buffer.push_back(mouse_handling());
    }

    if (c == KEY_DISP) {
      screen_ol();
      screen_redraw();
    } else {
      break;
    }
  }
  return c;
}
