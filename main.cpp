#include "definition.h"
#include "term.h"

string fullpath;

// Selection.
vector<int> selection;

// **************************************************************
// **************************************************************

// interface with screen
// All the code here could work without interacting with screen.cc
// screen.cc will read the values in some exported variable to be able
// to draw the text
int search_highlight = 0;
string text_message;

// We can modify this to change the screen display
// It is just an hint, screen.cc will change it to the
// exact value after each refresh.
// So we can save this together with the cursor postion if
// we want to restore the screen later exactly as it was.
// TODO: Not used.
int first_screen_line = 0;

// Indicate if the text has changed and is saved.
int text_saved = 1;
int text_modif = 0;

// **************************************************************
// **************************************************************

// The main text class is implemented by a gap buffer plus a small cache for
// the positions of some EOL.

// The data is in [text] with index in
// [0,text_gap) union [text_restart, text_end).
vector<int> text;
int text_gap = 0;
int text_restart = 0;
int text_end = 0;

// We keep the total numbers of lines
// and the line number of the cursor.
int text_lines = 0;
int text_l = 0;

// **************************************************************
// **************************************************************

// We have two kind of text position:
// - a normal position which is an index in text and should skip the gap.
// - an absolute position, which is the n-th caracter in the vector [text].

int text_index_from_absolute(int i) {
  if (i < text_gap) {
    return i;
  } else {
    return i + text_restart - text_gap;
  }
}

int text_absolute_from_index(int i) {
  if (i < text_gap) {
    return i;
  } else {
    return i - text_restart + text_gap;
  }
}

int text_compute_position(int begin, int size) {
  int res = begin + size;
  if (begin < text_gap && res >= text_gap) {
    res += text_restart - text_gap;
  }
  return res;
}

// **************************************************************
// Cache EOL number i, it goes into position i % cache_size of the cache.
// **************************************************************

const int cache_size = 50;
int cache_num[cache_size] = {0};
int cache_pos[cache_size] = {0};

// I wasn't sure about the initialisation syntax. Shouldn't be needed.
void cache_init() {
  for (int i = 0; i < cache_size; ++i) {
    if (cache_num[i] != 0) {
      printf("error !!!!!!\n");
      exit(0);
    }
    if (cache_pos[i] != 0) {
      printf("error !!!!!!\n");
      exit(0);
    }
    cache_num[i] = 0;
    cache_pos[i] = 0;
  }
}

// Add an EOL to the cache
void cache_add(int num, int pos) {
  int i = num % cache_size;
  cache_num[i] = num;
  cache_pos[i] = pos;
}

// Update the cache on EOL insert
void cache_line_insert(int line_num, int pos) {
  int m = line_num % cache_size;
  int i = m;
  do {
    int old_index = i;
    i = (i + cache_size - 1) % cache_size;
    if (cache_num[i] >= line_num) {
      if (cache_num[old_index] == 0) {
        cache_num[old_index] = cache_num[i] + 1;
        cache_pos[old_index] = cache_pos[i];
      }
      cache_num[i] = 0;
    }
  } while (i != m);
  cache_add(line_num, pos);
}

// Update the cache on EOL delete
void cache_line_delete(int line_num) {
  int m = line_num % cache_size;
  if (cache_num[m] == line_num) {
    cache_num[m] = 0;
  }
  int i = m;
  do {
    int old_index = i;
    i = (i + 1) % cache_size;
    if (cache_num[i] > line_num) {
      if (cache_num[old_index] == 0) {
        cache_num[old_index] = cache_num[i] - 1;
        cache_pos[old_index] = cache_pos[i];
      }
      cache_num[i] = 0;
    }
  } while (i != m);
}

// **************************************************************
// Maintains a list of jump positions.
// **************************************************************

const int max_jump_size = 6;
int jump_size = 0;
int jump_pos[max_jump_size];
int jump_screen[max_jump_size];

// The jump position are maintained as exact position in the text.
// The only places we need to update them is on text_move
// We also erase there the position not corresponding to real one anymore.
// move is the new position of gap/restart in the text.
void update_jump_on_move(int move) {
  for (int i = 0; i < jump_size; ++i) {
    int p = jump_pos[i];
    if (p >= text_gap && p < text_restart) {
      jump_size--;
      swap(jump_pos[i], jump_pos[jump_size]);
      swap(jump_screen[i], jump_screen[jump_size]);
    } else if (p >= move && p < text_gap) {
      jump_pos[i] += text_restart - text_gap;
    } else if (p >= text_restart && p < move) {
      jump_pos[i] -= text_restart - text_gap;
    }
  }
}

void add_jump_pos() {
  // Don't add the same pos twice.
  // TODO: search for dup in the whole vector ?
  if (text_restart == jump_pos[0]) return;

  // Moves everything by one.
  jump_size++;
  if (jump_size > max_jump_size) {
    jump_size = max_jump_size;
  }
  for (int i = jump_size - 1; i > 0; i--) {
    jump_pos[i] = jump_pos[i - 1];
    jump_screen[i] = jump_screen[i - 1];
  }
  jump_pos[0] = text_restart;
  jump_screen[0] = screen_get_first_line();
}

// **************************************************************
// **************************************************************

const int text_blocsize = 1024;

// check text size and realloc if needed
// TODO : use realloc ?
void text_check(int l) {
  int gapsize = text_restart - text_gap;
  if (gapsize >= l) return;

  int num = 0;
  while (gapsize + num < l) {
    num += text_blocsize;
  }

  int oldsize = text_end;
  for (int i = 0; i < num; i++) text.push_back('#');
  for (int i = oldsize - 1; i >= text_restart; i--) {
    text[i + num] = text[i];
    text[i] = '#';
  }
  text_restart += num;
  text_end = text.size();

  // Update cache.
  for (int i = 0; i < cache_size; ++i) {
    if (cache_pos[i] >= text_gap) cache_pos[i] += num;
  }

  // Update jump.
  for (int i = 0; i < jump_size; ++i) {
    if (jump_pos[i] >= text_gap) jump_pos[i] += num;
  }
}

// **************************************************************
// **************************************************************

// Only functions that modify the text !
// Not most efficient, but fast enough for normal use
// assertion are not needed

void text_add(int c) {
  text_check(1);
  text[text_gap++] = c;

  if (c == EOL) {
    text_l++;
    text_lines++;
    cache_line_insert(text_l, text_gap - 1);
  }
}

void text_del() {
  if (text_restart >= text_end) return;

  if (text[text_restart] == EOL) {
    cache_line_delete(text_l + 1);
    text_lines--;
  }

  text_restart++;
}

void text_back() {
  if (text_gap == 0) return;
  text_gap--;

  if (text[text_gap] == EOL) {
    cache_line_delete(text_l);
    text_l--;
    text_lines--;
  }
}

// **************************************************************
// **************************************************************

// Put the cursor at a given position.
// [i] must design a caracter stored in text or be equal to text_end.
// that is in [0,text_gap) or [text_restart,text_end].
// Everything else should not happen.
void text_internal_move(int i) {

  if (i < 0 || (i >= text_gap && i < text_restart) || i > text_end) {
    text_message = "internal wrong move ";
    return;
  }

  update_jump_on_move(i);

  if (i >= 0 && i < text_gap) {
    while (text_gap != i) {
      text_restart--;
      text_gap--;
      text[text_restart] = text[text_gap];
      // text[text_gap] = '#';

      if (text[text_restart] == EOL) {
        cache_add(text_l, text_restart);
        text_l--;
      }
    }
  } else if (i > text_restart && i <= text_end) {
    while (text_restart != i) {
      if (text[text_restart] == EOL) {
        text_l++;
        cache_add(text_l, text_gap);
      }
      text[text_gap] = text[text_restart];
      // This do not work if text_gap == text_restart.
      // Another solution would be to never allow equality.
      // text[text_restart] = '#';
      text_gap++;
      text_restart++;
    }
  }
}

// **************************************************************
// **************************************************************
// undo stuff.

// [pos] is the begining of the inserted/deleted text
// [mrk] is where the cursor was when the operation started
// [del] is a flag to know if the [content] was deleted/inserted
// [first_line] to display the text at the same pos on undo.
struct s_undo {
  int pos;
  int mrk;
  int del;
  int first_line;
  vector<int> content;
};

// apply an operation to the text.
void text_apply(struct s_undo op) {
  text_internal_move(text_index_from_absolute(op.pos));
  if (op.del) {
    for (int i = 0; i < op.content.size(); ++i) text_add(op.content[i]);
  } else {
    for (int i = 0; i < op.content.size(); ++i) text_del();
  }

  if (op.mrk >= 0) text_internal_move(text_index_from_absolute(op.mrk));
}

vector<struct s_undo> undo_stack;
vector<struct s_undo> redo_stack;
int undo_pos = -1;
int undo_mrk = -1;
int undo_last;

void undo_flush() {
  // Check if there is any pending operation
  if (undo_pos == -1) return;
  if (text_gap == undo_pos) {
    undo_pos = -1;
    return;
  }

  // At this point we are going to store a new
  // operation to the undo_stack.
  redo_stack.clear();

  int b = min(text_gap, undo_pos);
  int e = max(text_gap, undo_pos);

  struct s_undo op;
  op.first_line = screen_get_first_line();
  op.mrk = undo_mrk;
  op.pos = b;

  if (undo_pos < text_gap) {
    op.del = 0;
  } else {
    op.del = 1;
  }

  while (b < e) {
    op.content.push_back(text[b]);
    b++;
  }

  undo_stack.push_back(op);
  undo_pos = -1;
  undo_mrk = -1;
}

int undo_internal(vector<struct s_undo> &u_stack,
                  vector<struct s_undo> &r_stack) {
  undo_flush();
  if (u_stack.empty()) return 0;
  struct s_undo op;
  int temp = text_gap;
  int first_line = screen_get_first_line();
  int stop = 0;
  do {
    op = u_stack.back();
    text_apply(op);
    if (op.mrk >= 0) {
      stop = 1;
    }
    op.del = 1 - op.del;
    op.mrk = temp;
    screen_set_first_line(op.first_line);
    op.first_line = first_line;
    if (temp >= 0) {
      temp = -1;
    }
    r_stack.push_back(op);
    u_stack.pop_back();
  } while (!u_stack.empty() && !stop);
  return 1;
}

int text_undo() { return undo_internal(undo_stack, redo_stack); }

int text_redo() { return undo_internal(redo_stack, undo_stack); }

void undo_savepos() { undo_last = text_gap; }

void undo_start() {
  undo_flush();
  undo_pos = text_gap;
  undo_mrk = undo_last;
  undo_last = -1;
}

// ******************************************************
// same as above but with undo support
// ******************************************************

void edit_text() {
  text_modif++;
  text_saved = 0;
}

void text_putchar(int c) {
  edit_text();

  if (undo_pos == -1 || undo_pos > text_gap) undo_start();
  text_add(c);
}

void text_backspace() {
  if (text_gap == 0) return;
  edit_text();

  if (undo_pos < 0) undo_start();
  text_back();
}

void text_delete() {
  if (text_restart >= text_end) return;
  edit_text();

  if (undo_pos < text_gap) undo_start();
  text[undo_pos] = text[text_restart];
  undo_pos++;
  text_del();
}

/******************************************************/
/******************************************************/

// Put the cursor at a given position.
// [i] must design a caracter stored in text or be equal to text_end.
// that is in [0, text_gap) or [text_restart, text_end].
// Everything else should not happen.
void text_move(int i) {
  // Assert that i is in range.
  if (i < 0 || (i >= text_gap && i < text_restart) || i > text_end) {
    text_message = "wrong move ";
    return;
  }

  // We move so save current undo.
  undo_flush();
  text_internal_move(i);
}

void text_move_to_absolute(int i) { text_move(text_index_from_absolute(i)); }

// **********************************************************
// **********************************************************

// Return the indice of the begining of the line l in [0,gap[ [restart,end[
// this is mainly used by the displaying part of the editor.
int text_line_begin(int l) {
  // special case
  if (l > text_lines) l = text_lines;
  if (l <= 0) {
    if (text_gap == 0) {
      return text_restart;
    }
    return 0;
  }

  // beginning cached ?
  if (cache_num[l % cache_size] == l) {
    int t = cache_pos[l % cache_size] + 1;
    if (t == text_gap) t = text_restart;
    return t;
  }

  // line num and line begin
  int n = 0;
  int p = 0;

  // are we looking before text_gap
  // or after text_restart ?
  if (l > text_l) {
    n = text_l;
    p = text_restart;

    // find the next EOL and cache it
    while (n < l && p < text_end) {
      while (p < text_end && text[p] != EOL) p++;
      cache_add(n + 1, p);
      n++;
      p++;
    }
  } else {
    n = text_l + 1;
    p = text_gap;

    // find the previous EOL and cache it
    while (n > l && p >= 0) {
      do {
        p--;
      } while (p >= 0 && text[p] != EOL);
      n--;
      cache_add(n, p);
    }
    p++;
  }

  // return
  if (p == text_gap) p = text_restart;
  return p;
}

int line_begin() { return text_line_begin(text_l); }

int line_end() {
  int i = (text_l + 1) % cache_size;
  if (cache_num[i] == text_l + 1) return cache_pos[i];

  i = text_restart;
  while (i < text_end && text[i] != EOL) i++;

  cache_add(text_l + 1, i);
  return i;
}

// **********************************************************

// After a change of line,
// base pos should be the original position the cursor goto.
int base_pos;

// return pos in the current line
int compute_pos() {
  int t = line_begin();
  if (t == text_restart) return 0;
  return text_gap - t;
}

// Goto a given pos in the current line.
void line_goto(int pos) {
  int b = line_begin();
  int e = line_end();
  int i = text_compute_position(b, pos);
  if (i > e) i = e;
  text_move(i);
}

/*******************************************************************/
// useful 'is' functions
/*******************************************************************/

int is_begin() {
  int i = text_gap;
  return (i == 0 || text[i - 1] == EOL);
}

int is_end() {
  int i = text_restart;
  return (i == text_end || text[i] == EOL);
}

int is_space_before() {
  int i = text_gap;
  return (i == 0 || text[i - 1] == EOL || text[i - 1] == ' ');
}

int is_letter_before() {
  int i = text_gap - 1;
  return (i >= 0 && isletter(text[i]));
}

int is_indent() {
  int i = text_gap - 1;
  while (i >= 0 && text[i] == ' ') i--;
  if (i <= 0 || text[i] == EOL) {
    return 1;
  }
  return 0;
}

/**************************************************/
// Here is where we get new char from screen and
// record them if we need to.
/**************************************************/

int record = 0;
vector<int> record_data;

// This is used while recording a macro to deal properly with recording
// completion.
vector<int> last_completion_for_macro;

void start_record() {
  record_data.clear();
  record = 1;
}

void end_record() { record = 0; }

/* Swap file implementation in case of a crash:
 *
 * Just dump every keystroke in the swap file.
 * The behaviour is perfectly reproducible.
 * TODO : for this to be true, we need to convert
 *        screen commands to char ...
 *
 * At some point I wanted to use it to undo stuff, but seems
 * like too much trouble. However, if we keep the same file
 * from one edit to another, we can reconstruct the undo struct,
 * jump between save and even go to some points just before an
 * undo that are lost in the undo struct ..
 *
 * timestamp and show the date of the save ?
 *
 */

/* cool:
 * We refresh the screen each time the program is waiting for a
 * char from the keyboard. And only then.
 *
 * So everything here can work without any display.
 * like in redo mode. or macro.
 */

// If not -1, replay this char on the next getchar().
int replay = -1;

// If not empty, read chars from there and not from the terminal.
// Warning: make sure to not end up in an infinite loop.
vector<int> play_macro;

// This should be the only way to add stuff to play_macro.
void text_add_macro(const vector<int> &input) {
  for (int i = 0; i < input.size(); ++i) {
    play_macro.push_back(input[i]);
  }
}

int text_getchar() {
  // If replay >= 0, just return it.
  // We only replay command char normally.
  if (replay >= 0) {
    int c = replay;
    replay = -1;
    return c;
  }

  // Add (+) to the title to reflect a modified text.
  // TODO: This should be in screen...
  static int oldstatus;
  if (oldstatus != text_saved) {
    string title(fullpath);
    if (!text_saved) title += " (+)";
    term_set_title((uchar *)title.c_str());
    oldstatus = text_saved;
  }

  // Loop as long as screen_get_char() return mouse event to us, this is
  // needed to do something on mouse paste (which will fill play_macro) and
  // then wait for the next char.
  int res = MOUSE_EVENT;
  while (res == MOUSE_EVENT) {
    // if we are playing a macro, get the char from there.
    if (!play_macro.empty()) {
      int c = play_macro[0];
      play_macro.erase(play_macro.begin());
      text_message.clear();
      if (record) {
        record_data.push_back(c);
      }
      return c;
    }

    // Read the next input char.
    // As a side effect, this update the screen!
    res = screen_getchar();
  }

  // Clear text message.
  text_message.clear();

  // to group operation in the undo struct corresponding to
  // only one keystroke and remember the position that started it all
  undo_savepos();

  // record char ?
  if (record) {
    // Hack for completion in a macro.
    // We do not record tab, but we push last_completion_for_macro.
    if (res != KEY_TAB || is_indent()) {
      if (!last_completion_for_macro.empty()) {
        record_data.insert(record_data.end(), last_completion_for_macro.begin(),
                           last_completion_for_macro.end());
        last_completion_for_macro.clear();
      }
      record_data.push_back(res);
    }
  }

  // return char
  return res;
}

int insert_mode = 0;
int reset_insert_mode = 0;

int text_getchar_insert() {
  if (reset_insert_mode) {
    insert_mode = 0;
    reset_insert_mode = 0;
  }
  int c = text_getchar();
  while (c == INSERT_MODE) {
    insert_mode = 1 - insert_mode;
    if (insert_mode) text_message = "<insert mode>";
    c = text_getchar();
  }
  if (insert_mode) {
    text_message = "<insert mode>";
  } else if (c == KEY_INSERT) {
    insert_mode = 1;
    reset_insert_mode = 1;
    text_message = "<insert>";
    c = text_getchar();
  }
  return c;
}

/*******************************************************************/
// implementation of the commands

void yank_line() {
  selection.clear();
  int c;
  text_move(line_begin());
  do {
    int i = text_gap;
    text_move(line_end() + 1);
    while (i < text_gap) {
      selection.push_back(text[i]);
      i++;
    }
    c = text_getchar();
  } while (c == KEY_COPY);
  replay = c;
}

void del_line() {
  vector<int> old_selection = selection;
  selection.clear();

  // move to line begin
  text_move(line_begin());

  int c;
  do {
    // no more line ??
    if (text_lines == 0) return;

    // save line
    int i = text_restart;
    while (i < text.size()) {
      selection.push_back(text[i]);
      i++;
      if (selection[selection.size() - 1] == EOL) break;
    }

    // delete it
    while (text_restart + 1 < text.size() && text[text_restart] != EOL) {
      text_delete();
    }
    text_delete();

    c = text_getchar();
  } while (c == KEY_CUT);

  // If we just print after a cut, use the previous selection.
  // This is super useful to replace lines.
  if (c == KEY_PRINT) {
    selection = old_selection;
  }
  replay = c;
}

void del_line_no_selection() {
  text_move(line_begin());
  while (text_restart + 1 < text.size() && text[text_restart] != EOL) {
    text_delete();
  }
  text_delete();
}

void text_print() {
  for (int i = 0; i < selection.size(); ++i) text_putchar(selection[i]);
}

void insert_indent() {
  // move to first non blanc char
  while (text_restart < text_end && text[text_restart] == ' ') {
    text_move(text_restart + 1);
  }
  // add indent.
  int pos = compute_pos();
  do {
    text_putchar(' ');
    pos++;
  } while (pos % TABSTOP != 0);
}

void smart_backspace() {
  int i = text_gap;

  // Remove trailing char after deleting EOL.
  if (i > 0 && text[i - 1] == EOL) {
    int num_deleted = 0;
    do {
      text_backspace();
      i--;
      ++num_deleted;
    } while (i > 0 && text[i - 1] == ' ');
    // Insert space only if there was trailing spaces.
    if (num_deleted > 1) text_putchar(' ');
    return;
  }

  // Remove indent if necessary.
  if (i > 0 && is_indent()) {
    int pos = compute_pos();
    int j = text_restart;
    while (j < text_end && text[j] == ' ') {
      j++;
      pos++;
    }
    do {
      text_backspace();
      pos--;
      i--;
    } while (i > 0 && text[i - 1] == ' ' && pos % TABSTOP != 0);
    return;
  }

  // Normal behavior.
  text_backspace();
}

void smart_delete() {
  int i = text_restart;

  // deal with indent
  if (i + 1 < text_end && text[i] == ' ') {
    int pos = compute_pos();
    int pos2 = 0;
    while (i < text_end && text[i] == ' ') {
      i++;
      pos2++;
    }
    do {
      text_delete();
      pos2--;
    } while (pos2 > 0 && (pos + pos2) % TABSTOP != 0);
    return;
  }

  // deal with end of line
  if (i + 1 < text_end && text[i] == EOL && text_gap > 0 &&
      text[text_gap - 1] != EOL) {
    do {
      text_delete();
      i++;
    } while (i + 1 < text_end && text[i + 1] == ' ');
    return;
  }

  // normal behavior
  text_delete();
}

int auto_indent = 1;
void smart_enter() {
  if (auto_indent == 0) {
    text_putchar(EOL);
    return;
  }

  // compute line indent from current position
  int i = line_begin();
  int pos = 0;
  while (i < text_gap && text[i] == ' ') {
    i++;
    pos++;
  }

  // put EOL and correct indent
  text_putchar(EOL);
  for (int j = 0; j < pos; ++j) text_putchar(' ');
}

void toggle_ai() {
  if (auto_indent) {
    text_message = "auto indent off";
    auto_indent = 0;
  } else {
    text_message = "auto indent on";
    auto_indent = 1;
  };
}

void open_line_after() {
  text_move(line_end());
  smart_enter();
}

void open_line_before() {
  text_move(line_begin());

  // compute line indent
  int pos = 0;
  int i = text_restart;
  while (text[i++] == ' ') pos++;

  // insert it
  for (int j = 0; j < pos; j++) text_putchar(' ');
  text_putchar(EOL);
  text_move(text_gap - 1);
}

//*************************************************
//** Internal search functions
//*************************************************

// check if string [s] appears at position [i] in [text]
// return 0 if not or the position just after the end if yes
// treat whitespace at begining/end of [s] in a special way
int match(vector<int> &s, int i) {
  int j = 0;
  if (s.size() == 0) return 0;

  // space at begining
  if (s[0] == ' ') {
    int t = i;
    if (t > text_gap && t <= text_restart) t = text_gap;
    if (t > 0 && isalphanum(text[t - 1])) return 0;
    j = 1;
  }

  // space at the end;
  int size = s.size();
  if (s[size - 1] == ' ') size--;
  if (size == 0) return 0;

  // match
  if (i < 0) return 0;
  while (j < size && i < text_end) {
    if (i >= text_gap && i < text_restart) i = text_restart;
    if (s[j] == text[i]) {
      i++;
      j++;
    } else {
      return 0;
    }
  }
  if (i == text_end) return 0;

  // space at the end
  if (i >= text_gap && i < text_restart) i = text_restart;
  if (j < s.size() && isalphanum(text[i])) return 0;

  // return correct pos
  return i;
}

// search backward for [s] in [text], starting just before [i]
// return the position at the beginning of the match
// or -1 if no match.
int search_prev(vector<int> &s, int i, int limit = 0) {
  limit = max(0, limit);
  if (i > text_end) return -1;
  while (i > limit) {
    i--;
    if (i >= text_gap && i < text_restart) i = text_gap - 1;
    if (match(s, i)) return i;
  }
  return -1;
}

// search forward for [s] in [text], starting at [i]
// return the position just after the match
// or -1 if no match.
int search_next(vector<int> &s, int i, int limit = text_end) {
  limit = min(text_end, limit);
  if (i < 0) return -1;
  while (i < limit) {
    if (i >= text_gap && i < text_restart) i = text_restart;
    int t = match(s, i);
    if (t) return t;
    i++;
  }
  return -1;
}

//******************************************************************
//** Completion
//******************************************************************

// dictionnary
// so we do not propose twice the same completion !
set<vector<int> > possibilities;

// first completion = last one with the same begin
// I think this is cool.
map<vector<int>, vector<int> > last_completions;

// Interface function for the completion
void text_complete() {
  undo_flush();
  possibilities.clear();
  vector<int> begin;
  vector<int> end;
  possibilities.insert(end);

  //  find begining of current word
  int i = text_gap - 1;
  while (i >= 0 && !isalphanum(text[i])) i--;
  while (i >= 0 && isalphanum(text[i])) i--;
  i++;

  //  not after a word? return.
  if (i == text_gap) return;

  //  [pos] always corresponds to the beginning of the last match
  int pos = i;

  //  compute [begin] (that is the search pattern)
  //  put white space first so we don't match partial word.
  begin.push_back(' ');
  while (i < text_gap) {
    begin.push_back(text[i]);
    i++;
  }

  //  look first in the map
  int c = KEY_TAB;
  if (last_completions.find(begin) != last_completions.end()) {
    end = last_completions[begin];
    possibilities.insert(end);
    last_completion_for_macro = end;
    for (int i = 0; i < end.size(); ++i) text_putchar(end[i]);
    c = text_getchar();
  }

  //  first search backward
  int backward = 1;

  //  No match or user not happy,
  //  look in the text
  while (c == KEY_TAB) {
    // remove current completion proposal
    if (end.size() > 0) {
      for (int i = 0; i < end.size(); ++i) text_backspace();
    }

    // compute end
    // use possibilities to not propose the same thing twice
    do {
      end.clear();
      if (backward) {
        pos = search_prev(begin, pos);
        if (pos >= 0) {
          int i = pos + begin.size() - 1;
          while (isalphanum(text[i]) && i < text_gap) {
            end.push_back(text[i]);
            i++;
          }
        } else {
          backward = 0;
          pos = text_restart;
        }
      } else {
        // pos is not at the beginning anymore,
        // but no problem
        pos = search_next(begin, pos);
        if (pos >= 0) {
          int i = pos;
          while (isalphanum(text[i]) && i < text_end) {
            end.push_back(text[i]);
            i++;
          }
        } else {
          // no more match,
          // quit the completion functions
          return;
        }
      }
    } while (possibilities.find(end) != possibilities.end());

    // use current end.
    possibilities.insert(end);
    last_completion_for_macro = end;
    for (int i = 0; i < end.size(); ++i) text_putchar(end[i]);

    c = text_getchar();
  }
  replay = c;

  // save completed text in map and also in last_completion_for_macro.
  // except if it is empty
  if (!end.empty()) {
    last_completions[begin] = end;
  }
  return;
}

// Almost same as previous one ...
// TODO: remove or merge.
void text_search_complete(vector<int> &str) {
  possibilities.clear();
  vector<int> begin;
  vector<int> end;
  possibilities.insert(end);

  //  do not complete if str is empty
  if (str.empty()) return;

  //  set begin to str
  if (str[0] != ' ') begin.push_back(' ');
  for (int i = 0; i < str.size(); ++i) begin.push_back(str[i]);

  //  start wit cursor
  int pos = text_restart;

  //  look first in the map
  int c = KEY_TAB;
  if (last_completions.find(begin) != last_completions.end()) {
    end = last_completions[begin];
    possibilities.insert(end);
    for (int i = 0; i < end.size(); ++i) text_putchar(end[i]);
    c = text_getchar();
  }

  //  first search backward
  //  then from start...
  int from_start = 0;

  //  No match or user not happy,
  //  look in the text
  while (c == KEY_TAB) {
    // remove current completion proposal
    if (end.size() > 0) {
      for (int i = 0; i < end.size(); ++i) str.pop_back();
    }

    // compute end
    // use possibilities to not propose the same thing twice
    do {
      end.clear();

      pos = search_next(begin, pos);
      if (pos >= 0) {
        int i = pos;
        while (isletter(text[i]) && i < text_end) {
          end.push_back(text[i]);
          i++;
        }
      } else {
        if (from_start) return;
        from_start = 1;
        pos = 0;
      }
    } while (possibilities.find(end) != possibilities.end());

    // use current end,
    possibilities.insert(end);
    for (int i = 0; i < end.size(); ++i) str.push_back(end[i]);

    c = text_getchar();
  }
  replay = c;

  // save completed text in map
  // except if it is empty
  if (!end.empty()) last_completions[begin] = end;

  return;
}

//******************************************
//** user search functions
//******************************************

// Pattern is the current search pattern.
vector<int> pattern;
int display_pattern = 0;

// Get id at current cursor position.
// Move cursor to id end and turn on search highlight.
void get_id() {
  vector<int> old_pattern = pattern;

  pattern.clear();
  pattern.push_back(' ');
  int i = text_gap;
  while (i > 0 && isalphanum(text[i - 1])) i--;
  while (i < text_gap) {
    pattern.push_back(text[i]);
    i++;
  }
  i = text_restart;
  while (i < text_end && isalphanum(text[i])) {
    pattern.push_back(text[i]);
    i++;
  }
  pattern.push_back(' ');

  if (pattern.size() == 2) {
    text_message = "No id at current position.";
    pattern = old_pattern;
  } else {
    text_move(i);
    search_highlight = 1;
  }
}

int text_search_prev() {
  if (search_highlight == 0) get_id();
  if (search_highlight == 0) return 1;

  int size = pattern.size();
  if (pattern.size() > 0 && pattern[0] == ' ') size--;
  if (pattern.size() > 0 && pattern[pattern.size() - 1] == ' ') size--;

  int t = search_prev(pattern, text_gap - size);
  if (t >= 0) {
    t = text_index_from_absolute(t + size);
    text_move(t);
  } else {
    text_message = "search restarted on bottom";
    t = search_prev(pattern, text_end - 2);
    if (t >= 0) {
      t = text_compute_position(t, size);
      text_move(t);
    } else {
      text_message = "pattern not found";
      search_highlight = 0;
      return 0;
    }
  }
  return 1;
}

int text_search_next() {
  if (search_highlight == 0) {
    get_id();
    return 1;
  }

  int t = search_next(pattern, text_restart);
  if (t > 0)
    text_move(t);
  else {
    text_message = "search restarted on top";
    t = search_next(pattern, 0);
    if (t > 0)
      text_move(t);
    else {
      text_message = "pattern not found";
      search_highlight = 0;
      return 0;
    }
  }
  return 1;
}

void text_search_again() {
  search_highlight = 1;
  text_search_next();
}

// search first occurrence of [pattern]
void text_search_first() {
  if (search_highlight == 0) get_id();
  if (search_highlight == 0) return;

  // search starting at the beginning.
  int saved_pos = text_restart;
  text_move(0);
  if (!text_search_next()) {
    text_move(saved_pos);
  }
}

// Let the user enter a new pattern before performing a search_next().
void text_new_search() {
  search_highlight = 1;
  vector<int> old_pattern = pattern;
  pattern.clear();
  while (1) {
    if (pattern.empty()) {
      display_pattern = 0;
      text_message = "<search>";
    } else {
      display_pattern = 1;
    }
    int c = text_getchar_insert();
    if (insert_mode || isprint(c)) {
      pattern.push_back(c);
      continue;
    }
    if (c == KEY_NULL) {
      continue;
    }
    if (c == KEY_BACKSPACE && !pattern.empty()) {
      pattern.pop_back();
      continue;
    }
    if (c == KEY_TAB) {
      text_search_complete(pattern);
      continue;
    }
    if (c == KEY_ENTER) {
      if (pattern.empty()) pattern = old_pattern;
      text_search_next();

      break;
    }
    replay = c;
    break;
  }
  display_pattern = 0;
}

/******************************************************************/
/******************************************************************/

void text_kill_word() {
  while (text_gap > 0 && !isletter(text[text_gap - 1]) &&
         text[text_gap - 1] != EOL) {
    text_backspace();
  }
  while (text_gap > 0 && isletter(text[text_gap - 1])) text_backspace();
}

void text_delete_word() {
  while (text_restart < text_end && !isletter(text[text_restart]) &&
         text[text_restart] != EOL) {
    text_delete();
  }
  while (text_restart < text_end && isletter(text[text_restart])) {
    text_delete();
  }
}

void text_delete_to_end() {
  while (text[text_restart] != EOL) {
    text_delete();
  }
}

void text_delete_to_begin() {
  while (text_gap > 0 && text[text_gap - 1] != EOL) {
    text_backspace();
  }
}

// TODO: really bad undowise. Needed?
void text_change_case() {
  int i = text_restart;
  int c;
  if (text[i] >= 'a' && text[i] <= 'z')
    c = text[i] - 'a' + 'A';
  else if (text[i] >= 'A' && text[i] <= 'Z')
    c = text[i] - 'A' + 'a';
  else
    return;
  text_delete();
  text_putchar(c);
}

void text_tab() {
  if (is_indent())
    insert_indent();
  else
    text_complete();
}

void space_tab() {
  if (is_indent())
    insert_indent();
  else
    text_putchar(' ');
}

// This is what can be recorded in an automatic macro. Always stays on the same
// line except in insert mode. Note that the command parsing start here.
//
// TODO: add some inline movements?
void insert() {
  while (1) {
    int c = text_getchar_insert();
    if (insert_mode || isprint(c))
      text_putchar(c);
    else
      switch (c) {
        case KEY_BACKSPACE:
          if (is_begin()) {
            replay = c;
            return;
          }
          smart_backspace();
          break;
        case KEY_DELETE:
          if (is_end()) {
            replay = c;
            return;
          }
          smart_delete();
          break;
        case KEY_KWORD:
          text_kill_word();
          break;
        case KEY_DWORD:
          text_delete_word();
          break;
        case KEY_DEND:
          text_delete_to_end();
          break;
        case KEY_DBEGIN:
          text_delete_to_begin();
          break;
        case KEY_CASE:
          text_change_case();
          break;
        case KEY_TAB:
          text_tab();
          break;
        default:
          replay = c;
          return;
      }
  }
}

/******************************************************************/
/******************************************************************/

void text_up() {
  text_move(line_begin());
  text_move(text_gap - 1);
  line_goto(base_pos);
}

void text_down() {
  text_move(line_end() + 1);
  line_goto(base_pos);
}

void text_back_word() {
  int i = text_gap;
  while (i > 0 && isletter(text[i - 1])) i--;
  while (i > 0 && (!isletter(text[i - 1]))) i--;
  text_move(i);
}

void text_next_word() {
  int i = text_restart;
  while (i < text_end && (!isletter(text[i]))) i++;
  while (i < text_end && isletter(text[i])) i++;
  text_move(i);
}

void text_next_letter() {
  int i = text_restart;
  while (i < text_end && isletter(text[i])) i++;
  while (i < text_end && (!isletter(text[i]))) i++;
  text_move(i);
}

/******************************************************************/
/******************************************************************/

// Behavior is a bit tricky:
// 1/ go to beginning of current line.
// 2/ extract prefix (no alphanum). Remove '-' for special item list behavior.
// 3/ merge together all lines with this prefix (also remove duplicate spaces).
// 4/ justify (and append the prefix to all lines).
void justify() {
  text_move(line_begin());
  int save = text_gap;
  int i = text_restart;
  string prefix;
  while (i < text_end && text[i] != EOL && !isalphanum(text[i])) {
    prefix.push_back(text[i] == '-' ? ' ' : text[i]);
    ++i;
  }

  // Make one line and dedup spaces.
  while (i < text_end) {
    if (text[i] == ' ' && i > 0 && text[i - 1] == ' ') {
      text_move(i);
      text_delete();
      i = text_restart;
      continue;
    }
    if (text[i] == EOL) {
      int begin = i;
      ++i;
      if (i == text_end || text[i] == EOL) break;
      int j = 0;
      for (; j < prefix.size(); ++j, ++i) {
        if (text[i] != prefix[j]) break;
      }
      if (j != prefix.size()) break;
      if (i == text_end || !isalphanum(text[i])) break;
      text_move(begin);
      for (int j = begin; j < i; ++j) {
        text_delete();
        ++begin;
      }
      if (text[text_gap - 1] != ' ') {
        text_putchar(' ');
      }
      i = begin;
    }
    ++i;
  }
  text_move_to_absolute(save);

  // justifies it.
  i = text_restart;
  int count = 0;
  int b = -1;
  while (i < text_end && text[i] != EOL) {
    if (text[i] == ' ') b = i;
    count++;
    // Line length without EOL <= JUSTIFY.
    if (count > JUSTIFY) {
      if (b > 0) {
        text_move(b);
        text_delete();
      } else {
        text_move(i);
      }
      text_putchar(EOL);
      for (int j = 0; j < prefix.size(); ++j) {
        text_putchar(prefix[j]);
      }
      count = prefix.size();
      b = -1;
      i = text_restart - 1;
    }
    i++;
  }

  // move to end.
  text_move(line_end());
}

/******************************************************************/
/******************************************************************/

// we store there the last automatic macro
vector<int> macro_data;
// absolute position in the text of where the last
// operation was executed
int macro_end = -1;

// debug
void macro_display() {
  stringstream yo;
  for (int i = 0; i < macro_data.size(); ++i) yo << macro_data[i];
  yo << '#';
  yo << macro_end;
  text_message = yo.str();
}

// insert is called throught this
int macro_record() {
  start_record();
  insert();
  end_record();

  if (record_data.size() > 1) {
    macro_data = record_data;
    macro_end = text_gap;
    return 1;
  }
  return 0;
}

void macro_exec() {
  text_add_macro(macro_data);
  insert();
  macro_end = text_gap;
  // TODO: this shouldn't be needed but is there because insert() exit when
  // we move out of the line...
  play_macro.clear();
  // On macro execution, we do not replay the char that caused insert to exit.
  replay = -1;
}

// TODO(debug):
void replace() {
  int i = 0;
  int limit = text_gap;
  text_move_to_absolute(macro_end);

  int old = 0;
  int loop = 1;
  while (loop && text_search_next()) {
    i++;
    if (text_gap >= limit && (macro_end < limit || macro_end >= text_gap))
      loop = 0;
    old = text_gap;

    macro_exec();

    // Beware !!
    // limit change because we replace stuff...
    if (old < limit) {
      limit += text_gap - old;
    }
  }

  stringstream m;
  m << i << " operations";
  text_message = m.str();
}

// TODO: this will not work if the macro add/modify line.
// What to do if the macro insert some EOL? take that into account.
void macro_till() {
  if (search_highlight) return replace();

  int limit = text_l;
  text_move_to_absolute(macro_end);
  while (text_l != limit) {
    if (text_l > limit)
      text_up();
    else
      text_down();
    macro_exec();
  };
}

/******************************************************************/
/******************************************************************/

void text_goto() {
  uint count = 0;
  uint line_number = text_lines - 1;
  uint line_pos = base_pos;
  bool after_colon = false;
  bool from_end = false;
  string typed_chars = "";

  char *test = new char[100];
  int position = text_gap <= line_begin() ? 1 : text_gap - line_begin() + 1;
  snprintf(test, 100, "line %d/%d (%d%%) pos %d/%d", text_l + 1, text_lines,
          (int)(100.0 * (text_l + 1) / text_lines), position,
          position + line_end() - text_restart - 1);
  string position_info(test);

  while (true) {
    if (typed_chars.empty()) {
      text_message = position_info;
    } else {
      text_message = typed_chars;
    }
    int c = text_getchar();
    if (isnum(c)) {
      count = 10 * count + c - '0';
      typed_chars.push_back(c);
    } else if (c=='-' && typed_chars.empty()) {
      typed_chars.push_back(c);
      from_end = true;
    } else if (c == ':' && !after_colon) {
      after_colon = true;
      line_number = count;
      count = 0;
      typed_chars.push_back(c);
    } else if (c == KEY_BACKSPACE && !typed_chars.empty()) {
      if (typed_chars.at(typed_chars.size() - 1) == ':') {
        after_colon = false;
        count = line_number;
      } else if (typed_chars.at(typed_chars.size() - 1) == '-') {
        from_end = false;
      } else {
        count /= 10;
      }
      typed_chars.erase(typed_chars.end() - 1);
    } else if (c == KEY_ENTER) {
      if (after_colon) {
        if (typed_chars.at(0) == ':') {
          line_number = text_l + 1;
        }
        line_pos = count;
      } else {
        line_number = count;
      }
      line_number = (line_number + text_lines - 1) % text_lines;
      if (from_end) line_number = text_lines - 1 - line_number;
      text_move(text_line_begin(line_number));
      line_goto(line_pos);
      break;
    } else if (c == KEY_GOTO && typed_chars.empty()) {
      text_move(0);
      line_goto(base_pos);
      break;
    } else {
      replay = c;
      break;
    }
  }
}

// ********************************************************
// ********************************************************

void jump_interface() {
  // Remove bad positions.
  update_jump_on_move(text_restart);
  // Return if nothing to do.
  if (jump_size == 0) {
    return;
  }
  int i = 0;
  int c = KEY_JUMP;
  int saved_pos = text_restart;
  int saved_screen = screen_get_first_line();
  while (c == KEY_JUMP) {
    if (i == jump_size) {
      i = 0;
      text_move(saved_pos);
      screen_set_first_line(saved_screen);
    } else {
      text_move(jump_pos[i]);
      screen_set_first_line(jump_screen[i]);
      i++;
    }
    c = text_getchar();
  }
  jump_pos[i] = saved_pos;
  jump_screen[i] = saved_screen;
  replay = c;
}

void search_move() {
  if (search_highlight == 0) {
    add_jump_pos();
  }
}

int move_command(uchar c) {
  int search = 0;
  int base = 1;

  switch (c) {
    // Search move.
    case KEY_AGAIN:
      search_move();
      text_search_again();
      search = 1;
      break;
    case KEY_FIND:
      search_move();
      text_new_search();
      search = 1;
      break;
    case KEY_FIRST:
      search_move();
      text_search_first();
      search = 1;
      break;
    case KEY_NEXT:
      search_move();
      text_search_next();
      search = 1;
      break;
    case KEY_PREV:
      search_move();
      text_search_prev();
      search = 1;
      break;

    // Move that do not reset base pos
    case KEY_UP:
      text_up();
      base = 0;
      break;
    case KEY_DOWN:
      text_down();
      base = 0;
      break;
    case KEY_GOTO:
      text_goto();
      base = 0;
      break;
    case PAGE_UP:
      screen_ppage();
      line_goto(base_pos);
      base = 0;
      break;
    case PAGE_DOWN:
      screen_npage();
      line_goto(base_pos);
      base = 0;
      break;

    // Inline move that reset base pos
    case KEY_TAB:
      text_next_letter();
      break;
    case KEY_BWORD:
      text_back_word();
      break;
    case KEY_FWORD:
      text_next_word();
      break;
    case KEY_BEGIN:
      text_move(line_begin());
      break;
    case KEY_END:
      text_move(line_end());
      break;
    case KEY_LEFT:
      text_move(text_gap - 1);
      break;
    case KEY_RIGHT:
      text_move(text_restart + 1);
      break;

    default:
      return 0;
  }

  if (!search) search_highlight = 0;
  if (base) base_pos = compute_pos();
  return 1;
}

// ********************************************************
// ********************************************************

// save pos on any modification...
void mainloop() {
  int first_move = 1;
  int c = 0;
  while (c != KEY_QUIT) {
    // insert? + record?
    if (macro_record()) {
      add_jump_pos();
    }

    // other command to process
    int b = 1;
    c = text_getchar();

    if (first_move) add_jump_pos();
    if (move_command(c)) {
      first_move = 0;
      continue;
    }
    first_move = 1;

    switch (c) {
      case KEY_JUMP:
        jump_interface();
        break;
      case KEY_AI:
        toggle_ai();
        break;
      case KEY_UNDO:
        text_undo();
        break;
      case KEY_REDO:
        text_redo();
        break;
      case KEY_TILL:
        macro_till();
        b = 0;
        break;
      case KEY_REPEAT:
        macro_exec();
        b = 0;
        break;
      case KEY_COPY:
        yank_line();
        break;
      case KEY_CUT:
        del_line();
        break;
      case KEY_PRINT:
        text_print();
        break;
      case KEY_OLINE:
        open_line_before();
        break;
      case KEY_JUSTIFY:
        justify();
        break;
      case KEY_SAVE:
        text_save();
        break;
      case KEY_ENTER:
        smart_enter();
        break;

      // here only on line boundary ...
      case KEY_BACKSPACE:
        smart_backspace();
        break;
      case KEY_DELETE:
        smart_delete();
        break;
      default:
        b = 0;
    }
    if (b) {
      base_pos = compute_pos();
    }
  }
}

/******************************************************************/
/******************************************************************/

// Clean this : mouse control from screen.cpp
// we have to clean pgup/pgdown from screen.cpp too

const int X_buffer_size = 1024;
uchar X_buffer[X_buffer_size];

void mouse_X_select(int b, int e) {
  FILE *fd = popen("xclip 2>/dev/null", "w");

  if (fd == NULL) {
    // even if the command is not there, fd is valid !
    text_message = "Unable to copy to X selection. Is xclip installed?";
    return;
  }

  while (b <= e) {
    int n = 0;
    while (b <= e && n + 4 < X_buffer_size) {
      if (b == text_gap) b = text_restart;
      int temp = text[b];
      b++;
      do {
        X_buffer[n++] = temp & 0xFF;
        temp >>= 8;
      } while (temp);
    }

    fwrite(X_buffer, 1, n, fd);
  }
  pclose(fd);
}

void mouse_X_paste() {
  FILE *fd = popen("xclip -o 2>/dev/null", "r");
  if (fd == NULL) {
    // even if the command is not there, fd is valid !
    text_message = "Unable to get X selection. Is xclip installed?";
    return;
  }
  play_macro.clear();
  play_macro.push_back(INSERT_MODE);

  int n;
  int l = 0;
  int p = 0;
  int ch;
  do {
    n = fread(X_buffer, 1, X_buffer_size, fd);

    for (int i = 0; i < n; ++i) {
      uchar c = X_buffer[i];

      if (p < l) {
        ch ^= c << (8 * (p + 1));
        p++;
        if (p == l) {
          play_macro.push_back(ch);
          p = l = 0;
        }
        continue;
      }

      if ((c >> 7) & 1) {
        /* compute utf8 encoding length */
        l = 0;
        while ((c >> (6 - l)) & 1) l++;
        ch = c;
        continue;
      }
      play_macro.push_back(c);
    }
  } while (n);
  play_macro.push_back(INSERT_MODE);
  pclose(fd);
}

void text_move_from_screen(int i) {
  search_highlight = 0;
  text_move(i);
}

void text_line_goto(int pos) {
  line_goto(pos);
  base_pos = pos;
}

void mouse_select(int b, int e) {
  mouse_X_select(b, e);
  selection.clear();
  while (b <= e) {
    if (b == text_gap) b = text_restart;
    selection.push_back(text[b]);
    b++;
  }
}

void mouse_word_select(int *left, int *right) {
  int b = text_gap;
  while (b > 0 && isletter(text[b - 1])) b--;
  *left = text_gap - b;

  // text_restart should always be a valid char in this case (the one clicked
  // on).
  int e = text_restart;
  while (e < text_end && isletter(text[e])) e++;
  if (e == text_restart) e++;
  *right = e - text_restart - 1;

  mouse_select(b, e - 1);
}

void mouse_paste() { mouse_X_paste(); }

/******************************************************************/
/* file handling                                                  */
/******************************************************************/

string ExtractDirname(const string& path) {
  const int pos = path.find_last_of('/');
  return (pos == string::npos) ? "" : path.substr(0, pos + 1);
}

string ExtractBasename(const string& path) {
  return path.substr(path.find_last_of('/') + 1);
}

// Copies a file from input_name to output_name. Returns true on success.
bool file_copy(string input_name, string output_name) {
  if (access(input_name.c_str(), F_OK) != 0) return false;

  FILE *src = fopen(input_name.c_str(), "rb");
  FILE *dst = fopen(output_name.c_str(), "wb");

  const int buf_size = 1024;
  uchar buf[buf_size];

  int num = 0;
  bool res = true;
  if (src != NULL && dst != NULL) do {
    num = fread(buf, sizeof(uchar), buf_size, src);
    if (num == 0 && ferror(src)) {
      res = false;
      break;
    }
    if (fwrite(buf, sizeof(uchar), num, dst) < num) {
      res = false;
      break;
    }
  } while (num > 0);
  if (src == NULL || fclose(src) == EOF) res = false;
  if (dst == NULL || fclose(dst) == EOF) res = false;
  return res;
}

// open file and write new content
int text_write(const string& file_name) {
  ofstream s(file_name.c_str(), ios_base::trunc);
  if (!s) return 0;

  int trailing_space = 0;
  for (int i = 0; i < text_end; ++i) {
    // jump gap if needed
    if (i == text_gap) i = text_restart;
    if (i == text_end) {
      break;
    }

    // Remove trailing space at write time.
    if (text[i] == ' ') {
      trailing_space++;
      continue;
    } else {
      if (text[i] != EOL) {
        while (trailing_space > 0) {
          s.put(' ');
          trailing_space--;
        }
      }
      trailing_space = 0;
    }

    // convert to utf-8
    unsigned int temp = text[i];
    do {
      s.put(temp & 0xFF);
      temp >>= 8;
    } while (temp);
  }

  // Add last EOL if necessary.
  if ((text_restart < text_end && text[text_end - 1] != EOL) ||
      (text_restart == text_end && text_gap > 0 && text[text_gap - 1] != EOL)) {
    s.put(EOL);
  }

  s.close();
  return 1;
}

int utf_isvalid(unsigned int c) {
  unsigned int t = c & 0xFF;
  unsigned int l = 0;

  // over long encoding
  if (t == 192 || t == 193) return 0;
  // too high
  if (t >= 245) return 0;

  while ((t >> (7 - l)) & 1) l++;

  if (l == 0 && (c >> 8)) return 0;
  if (l == 1) return 0;

  for (int i = 1; i < l; i++) {
    c >>= 8;
    t = c & 0xFF;
    if (t >> 6 != 2) return 0;
  }
  if (c >> 8) return 0;

  return 1;
}

int open_file() {
  /* Open file */
  ifstream inputStream(fullpath.c_str());
  if (!inputStream) return 0;

  /* Put file in gap buffer text */
  char c;
  int temp = 0;

  int l = 0;
  unsigned int ch;
  unsigned int pending;

  while (1) {
    if (l) {
      ch = pending & 0xFF;
      pending >>= 8;
      l--;
    } else {
      if (!inputStream.get(c)) break;
      ch = uchar(c);

      if ((c >> 7) & 1) {
        // compute utf8 encoding length
        l = 0;
        while ((c >> (6 - l)) & 1 && l < 3) l++;

        // compute corresponding int
        for (int i = 0; i < l; ++i) {
          if (!inputStream.get(c)) break;
          ch ^= uchar(c) << (8 * (i + 1));
        }

        // if not valid, just use individual char
        if (utf_isvalid(ch)) {
          l = 0;
        } else {
          pending = ch >> 8;
          ch &= 0xFF;
        }
      }
    }

    // convert tab in space
    if (ch == '\t') {
      do {
        text_add(' ');
        temp++;
      } while ((temp % TABSTOP) != 0);
    } else {
      if (ch == EOL) {
        // remove trailing
        while (text_gap > 0 && text[text_gap - 1] == ' ') text_back();
        text_add(EOL);
        temp = 0;
      } else {
        // normal char
        text_add(ch);
        temp++;
      }
    }
  }

  // close file
  inputStream.close();

  return 1;
}


// ********************************************************
// To remember last line on repoen.
//
// TODO save full jump pos?
// ********************************************************

struct s_info {
  int absolute_pos;
  int screen_first_line;
};

// map filename -> s_info
map<string, struct s_info> info;

void load_info() {
  const string temp_name = ExtractDirname(fullpath) + ".fe/info";
  ifstream s;
  s.open(temp_name.c_str());
  if (!s) return;

  string name;
  int absolute_pos;
  int screen_first_line;
  while (s >> name >> absolute_pos >> screen_first_line) {
    info[name].absolute_pos = absolute_pos;
    info[name].screen_first_line = screen_first_line;
  }
  s.close();
}

void use_info() {
  const string file_name = ExtractBasename(fullpath);
  if (info.find(file_name) == info.end()) return;
  screen_set_first_line(info[file_name].screen_first_line);
  text_move_to_absolute(info[file_name].absolute_pos);
}

void save_info() {
  // to deal with multiple instance running
  // reload the last info, update it and save it.
  info.clear();
  load_info();

  const string temp_name = ExtractDirname(fullpath) + ".fe/info";
  ofstream s;
  s.open(temp_name.c_str());
  if (!s) return;

  // text_gap is the same as text_absolute_from_index(text_restart)
  const string file_name = ExtractBasename(fullpath);
  info[file_name].absolute_pos = text_gap;
  info[file_name].screen_first_line = screen_get_first_line();
  map<string, struct s_info>::iterator iter = info.begin();
  while (iter != info.end()) {
    s << (iter->first) << " " << (iter->second).absolute_pos << " "
      << (iter->second).screen_first_line << endl;
    iter++;
  }
  s.close();
}

// ********************************************************
// to save selection
// ********************************************************

void load_selection() {
  ifstream s((string(getenv("HOME")) + "/.fe/selection").c_str());
  if (!s) return;
  selection.clear();
  int ch;
  char c;
  while (s.get(c)) {
    ch = uchar(c);
    if ((c >> 7) & 1) {
      /* compute utf8 encoding length */
      int l = 0;
      while ((c >> (6 - l)) & 1) l++;

      /* compute corresponding int */
      for (int i = 0; i < l; ++i) {
        s.get(c);
        ch ^= uchar(c) << (8 * (i + 1));
      }
    }
    selection.push_back(ch);
  }
  s.close();
}

void save_selection() {
  if (selection.size() == 0) return;
  ofstream s((string(getenv("HOME")) + "/.fe/selection").c_str());
  if (!s) return;
  for (int i = 0; i < selection.size(); i++) {
    // convert to utf-8
    unsigned int temp = selection[i];
    do {
      s.put(temp & 0xFF);
      temp >>= 8;
    } while (temp);
  }
  s.close();
}

// ********************************************************

// save file, create backup file before
// we actually overwrite the file
int text_save() {
  if (!file_copy(fullpath, ExtractDirname(fullpath) + ".fe/" + ExtractBasename(fullpath))) {
    text_message = "Could not create backup file, press ^S to confirm save";
    int c = text_getchar();
    if (c != KEY_SAVE) {
      replay = c;
      return 0;
    }
  }
  if (text_write(fullpath)) {
    text_message = ExtractBasename(fullpath) + " saved";
    text_saved = 1;
  } else {
    text_message = "Could not save " + ExtractBasename(fullpath);
    return 0;
  }
  return 1;
}

int text_exit() {
  string temp_name = ExtractDirname(fullpath) + ".fe/" + ExtractBasename(fullpath);
  int ok = 0;
  if (text_saved == 0) {
    ok = text_write(temp_name);
    if (!ok) {
      text_message = "Could not create backup file, press ^Q to confirm quit";
      int c = text_getchar();
      if (c != KEY_QUIT) {
        replay = c;
        return 0;
      }
    }
  }

  reset_input_mode();

  if (ok) {
    cout << "Last version not saved ";
    cout << "(see " << temp_name << " if needed)." << endl;
    cout.flush();
  };

  save_selection();
  save_info();
  return 1;
}

void terminate(int param) {
  // so we are sure to quit even if no backup
  // could be created.
  replay = KEY_QUIT;
  text_exit();
  exit(1);
}

// Split using : for easier gcc debuging.
// Issue: what if the filename do contains : ...
void split_file_name(const string &input, string *full_path, string *line_num,
                     string *line_pos) {
  string *out = full_path;
  int output_num = 0;
  for (int i = 0; i < input.size(); ++i) {
    if (input[i] == ':') {
      ++output_num;
      if (output_num == 1)
        out = line_num;
      else if (output_num == 2)
        out = line_pos;
      else
        return;
    } else {
      out->push_back(input[i]);
    }
  }
}

int main(int argc, char **argv) {
  cache_init();

  // If no argument is provided, we use this file name.
  fullpath = "fe_scratch";
  string start_line_string;
  string inline_pos_string;

  // parse file name and initial line number.
  if (argc >= 2) {
    fullpath.clear();
    split_file_name(argv[1], &fullpath, &start_line_string,
                    &inline_pos_string);
  }

  int start_line = -1;
  if (!start_line_string.empty()) {
    start_line = atoi(start_line_string.c_str());
  } else if (argc >= 3) {
    start_line = atoi(argv[2]);
  }

  int inline_pos = 0;
  if (!inline_pos_string.empty()) {
    inline_pos = atoi(inline_pos_string.c_str()) - 1;
  }

  // Create .fe/ if it doesn't exist.
  string temp_name = ExtractDirname(fullpath) + ".fe/";
  mkdir(temp_name.c_str(), S_IRWXU);

  // Create $home/.fe/ if it doesn't exist.
  mkdir((string(getenv("HOME")) + "/.fe/").c_str(), S_IRWXU);

  // load selection
  load_selection();

  // load file
  open_file();

  // catch deadly signal for proper terminaison
  signal(SIGTERM, terminate);

  // move to given line arg or to saved pos
  if (start_line >= 0) {
    text_move(text_line_begin(start_line - 1));
    line_goto(inline_pos);
  } else {
    load_info();
    use_info();
  }

  // Fix a bug when pos > text_len
  if (text_restart == text_end) {
    text_move(0);
  }
  base_pos = compute_pos();

  // init screen
  // switch to fullscreen mode
  screen_init();
  term_set_title((uchar *)ExtractBasename(fullpath).c_str());

  // enter mainloop
  // backup unsaved text if necessary
  // and exit properly when mainloop is done
  do {
    mainloop();
  } while (!text_exit());

  //  Thanks for using this editor !
  return EXIT_SUCCESS;
}
