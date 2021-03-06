#include "definition.h"
#include "term.h"

// original argv[1]
char    *file;

// different components of the edited file name
char    *file_name;
char    *file_dir;
char    *file_ext;

// used to compute internal name
char    *temp_name;

// selection
vector<int> selection;


// **************************************************************
// **************************************************************

// interface with screen
// All the code here could work withouth interacting with screen.cc
// screen.cc will read the values in some exported variable to be able
// to draw the text
int     search_highlight=0;
string  text_message;

// We can modify this to change the screen display
// It is just an hint, screen.cc will change it to the
// exact value after each refresh.
// So we can save this together with the cursor postion if
// we want to restore the screen later exactly as it was.
int first_screen_line = 0;

// Indicate if the text as changed and is saved.
int     text_saved = 1;
int     text_modif = 0;

// **************************************************************
// **************************************************************

// The main text class is implemented by a gap buffer
// plus a small cache for the positions of some EOL

// the data is in [text] with index in
// [0,text_gap)
// [text_restart, text_end)
vector<int>     text;
int             text_gap = 0;
int             text_restart = 0;
int             text_end = 0;

// We keep the total numbers of lines
// and the line number of the cursor.
int             text_lines = 0;
int             text_l = 0;


// **************************************************************
// **************************************************************

// We have two kind of text position:
// - a normal position which is an index in text and should skip the gap.
// - an absolute position, which is the n-th caracter of the data.

int text_index_from_absolute(int i)
{
    if (i<text_gap) return i;
    else return i + text_restart - text_gap;
}

int text_absolute_from_index(int i)
{
    if (i<text_gap) return i;
    else return i - text_restart + text_gap;
}

int text_compute_position(int begin, int size)
{
    int res = begin + size;
    if (begin < text_gap && res >= text_gap)
        res += text_restart - text_gap;
    return res;
}

// **************************************************************
// **************************************************************

// Cache EOL number i
// it goes into position i%cache_size of the cache.

const int cache_size = 50;
int cache_num[cache_size]= {0};
int cache_pos[cache_size]= {0};

void cache_init()
{
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
void cache_add(int num, int pos)
{
    int i = num % cache_size;
    cache_num[i] = num;
    cache_pos[i] = pos;
}

// Update the cache on EOL insert
void cache_line_insert(int line_num, int pos)
{
    int m = line_num % cache_size;
    int i = m;
    do {
      int old_index = i;
      i = (i + cache_size - 1) % cache_size;
      if (cache_num[i] >= line_num) {
        if (cache_num[old_index] == 0) {
          cache_num[old_index] = cache_num[i]+1;
          cache_pos[old_index] = cache_pos[i];
        }
        cache_num[i] = 0;
      }
    } while (i != m);
    cache_add(line_num, pos);
}

// Update the cache on EOL delete
void cache_line_delete(int line_num)
{
    int m = line_num % cache_size;
    if (cache_num[m] == line_num) {
        cache_num[m] = 0;
    }
    int i = m;
    do {
      int old_index = i;
      i = (i+1) % cache_size;
      if (cache_num[i] > line_num) {
        if (cache_num[old_index] == 0) {
          cache_num[old_index] = cache_num[i] - 1;
          cache_pos[old_index] = cache_pos[i];
        }
        cache_num[i]=0;
      }
    } while (i != m);
}

// **************************************************************
// **************************************************************
// Maintains a list of jump positions.

const int max_jump_size = 6;
int jump_size = 0;
int jump_pos[max_jump_size];
int jump_screen[max_jump_size];

// The jump position are maintained as exact position in the text.
// The only places we need to update them is on text_move
// We also erase there the position not corresponding to real one anymore.
// move is the new position of gap/restart in the text.
void update_jump_on_move(int move)
{
  fi (jump_size) {
    int p = jump_pos[i];
    if (p >= text_gap && p < text_restart) {
      jump_size --;
      swap(jump_pos[i], jump_pos[jump_size]);
      swap(jump_screen[i], jump_screen[jump_size]);
    } else if (p >= move && p < text_gap) {
      jump_pos[i] += text_restart - text_gap;
    } else if (p >= text_restart && p < move) {
      jump_pos[i] -= text_restart - text_gap;
    }
  }
}

void add_jump_pos()
{
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
// TODO : problem with undo ? no because only happen on add char.
void text_check(int l)
{
    int gapsize = text_restart - text_gap;
    if (gapsize >= l) return;

    int num=0;
    while (gapsize + num < l) {
        num += text_blocsize;
    }

    int oldsize = text_end;
    for (int i=0; i<num; i++) text.push_back('#');
    for (int i = oldsize - 1; i >= text_restart; i--) {
        text[i + num] = text[i];
        text[i] = '#';
    }
    text_restart += num;
    text_end = text.size();

    // Update cache.
    fi (cache_size) {
        if (cache_pos[i] >= text_gap)
            cache_pos[i] += num;
    }

    // Update jump.
    fi (jump_size) {
        if (jump_pos[i] >= text_gap)
            jump_pos[i] += num;
    }
}


// **************************************************************
// **************************************************************

// Only functions that modify the text !
// Not most efficient, but fast enough for normal use
// assertion are not needed

void text_add(int c)
{
    text_check(1);
    text[text_gap++] = c;

    if (c == EOL) {
        text_l++;
        text_lines++;
        cache_line_insert(text_l, text_gap-1);
    }
}

void text_del()
{
    if (text_restart>=text_end) return;

    if (text[text_restart]==EOL) {
        cache_line_delete(text_l+1);
        text_lines--;
    }

    text_restart++;
}

void text_back()
{
    if (text_gap==0) return;
    text_gap--;

    if (text[text_gap]==EOL) {
        cache_line_delete(text_l);
        text_l--;
        text_lines--;
    }
}

// **************************************************************
// **************************************************************

// Put the cursor at a given position
// [i] must design a caracter stored in text
// that is in [0,text_gap) or [text_restart,text_end)
// Everything else should not happen.
// TODO: We enforce here that text_restart != text_end, but I am not sure
// about this. This way we just ensure that we cannot get pass the
// last EOL... Need to enforce this properly or remove it.
void text_internal_move(int i) {

    if (i<0 || (i>=text_gap && i<text_restart) || i>=text_end) {
        text_message= "internal wrong move ";
        return;
    }

    update_jump_on_move(i);

    if (i >= 0 && i < text_gap) {
      while (text_gap != i) {
        text_restart--;
        text_gap--;
        text[text_restart]=text[text_gap];
        // text[text_gap] = '#';

        if (text[text_restart]==EOL) {
            cache_add(text_l, text_restart);
            text_l--;
        }
      }
    } else if (i > text_restart && i <= text_end) {
      while (text_restart != i) {
        if (text[text_restart]==EOL) {
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
// [mark] is where the cursor was when the operation started
// [del] is a flag to know if the [content] was deleted/inserted
struct s_undo {
    int pos;
    int mrk;
    int del;
    vector<int> content;
};

// apply an operation to the text.
void text_apply(struct s_undo op)
{
    text_internal_move(text_index_from_absolute(op.pos));
    if (op.del) {
        fi (op.content.sz)
            text_add(op.content[i]);
    } else {
        fi (op.content.sz)
            text_del();
    }

    if (op.mrk>=0)
        text_internal_move(text_index_from_absolute(op.mrk));
}

vector<struct s_undo>    undo_stack;
vector<struct s_undo>    redo_stack;
int undo_pos=-1;
int undo_mrk=-1;
int undo_last;

void undo_flush()
{
    // Check if there is any pending operation
    if (undo_pos==-1) return;
    if (text_gap == undo_pos) {
        undo_pos=-1;
        return;
    }

    // At this point we are going to store a new
    // operation to the undo_stack.
    redo_stack.clear();

    int b=min(text_gap,undo_pos);
    int e=max(text_gap,undo_pos);

    struct s_undo op;
    op.mrk = undo_mrk;
    op.pos = b;

    if (undo_pos<text_gap)
        op.del = 0;
    else
        op.del = 1;

    while (b<e) {
        op.content.pb(text[b]);
        b++;
    }

    undo_stack.pb(op);
    undo_pos=-1;
    undo_mrk=-1;
}

int text_undo()
{
    undo_flush();
    if (undo_stack.empty()) return 0;
    struct s_undo op;
    do {
        op=undo_stack.back();
        text_apply(op);
        op.del=1-op.del;
        redo_stack.pb(op);
        undo_stack.pop_back();
    } while (!undo_stack.empty() && op.mrk<0);
    return 1;
}

int text_redo()
{
    undo_flush();
    if (redo_stack.empty()) return 0;
    struct s_undo op;
    do {
        op=redo_stack.back();
        text_apply(op);
        op.del=1-op.del;
        undo_stack.pb(op);
        redo_stack.pop_back();
    } while (!redo_stack.empty() && op.mrk<0);
    return 1;
}

void undo_savepos() {
    undo_last = text_gap;
}

void undo_start() {
    undo_flush();
    undo_pos = text_gap;
    undo_mrk = undo_last;
    undo_last=-1;
}

// ******************************************************
// same as above but with undo support
// ******************************************************

void edit_text() {
    text_modif++;
    text_saved=0;
}

void text_putchar(int c)
{
    edit_text();

    if (undo_pos == -1 || undo_pos>text_gap) undo_start();
    text_add(c);
}

void text_backspace()
{
    if (text_gap==0) return;
    edit_text();

    if (undo_pos<0) undo_start();
    text_back();
}


void text_delete()
{
    if (text_restart>=text_end) return;
    edit_text();

    if (undo_pos<text_gap) undo_start();
    text[undo_pos]=text[text_restart];
    undo_pos++;
    text_del();
}


/******************************************************/
/******************************************************/

// Put the cursor at a given position
// [i] must design a caracter stored in text
// that is in [0,text_gap) or [text_restart,text_end)
// Everything else should not happen.
void text_move(int i)
{
    // assert that i is in range
    if (i<0 || (i>=text_gap && i<text_restart) || i>=text_end) {
        text_message= "wrong move ";
        return;
    }

    // we move so save current undo
    undo_flush();
    text_internal_move(i);
}

void text_move_to_absolute(int i)
{
    text_move(text_index_from_absolute(i));
}

// **********************************************************
// **********************************************************

// return the indice of the begining of the line l
// in [0,gap[ [restart,end[
// this is mainly used by the displaying part of the editor.
int text_line_begin(int l)
{
    // special case
    if (l>text_lines) l=text_lines;
    if (l<=0) return 0;

    // beginning cached ?
    if (cache_num[l % cache_size] == l) {
        int t = cache_pos[l % cache_size] + 1;
        if (t == text_gap) t = text_restart;
        return t;
    }

    // line num and line begin
    int n=0;
    int p=0;

    // are we looking before text_gap
    // or after text_restart ?
    if (l>text_l) {
        n = text_l;
        p = text_restart;

        // find the next EOL and cache it
        while (n<l && p<text_end) {
            while (p<text_end && text[p] != EOL) p++;
            cache_add(n+1,p);
            n++;
            p++;
        }
    } else {
        n = text_l+1;
        p = text_gap;

        // find the previous EOL and cache it
        while (n>l && p>=0) {
            do {
                p--;
            } while (p>=0 && text[p] !=EOL);
            n--;
            cache_add(n,p);
        }
        p++;
    }

    // return
    if (p==text_gap) p=text_restart;
    return p;
}

int line_begin()
{
    return text_line_begin(text_l);
}

int line_end()
{
    int i = (text_l+1) % cache_size;
    if (cache_num[i] == text_l+1)
        return cache_pos[i];

    i=text_restart;
    while (i<text_end && text[i]!=EOL)
        i++;

    cache_add(text_l+1,i);
    return i;
}

// **********************************************************

// after a change of line,
// base pos should be the original position the cursor goto
int base_pos;

// return pos in the current line
int compute_pos()
{
    int t=line_begin();
    if (t==text_restart) return 0;
    return text_gap - t;
}

// goto a given pos in the current line.
void line_goto(int pos)
{
    int b=line_begin();
    int e=line_end();
    int i = text_compute_position(b, pos);
    if (i > e) i = e;
    text_move(i);
}

/**************************************************/
// Here is where we get new char from screen and
// record them if we need to.
/**************************************************/

int     record=0;
vector<int>  record_data;

void start_record() {
    record_data.clear();
    record = 1;
}

void end_record() {
    record = 0;
}

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

// sould be with the other is_ function below.
int is_indent()
{
    int i=text_gap-1;
    while (i>=0 && text[i]==' ') i--;

    if (text[i]==EOL) return 1;
    else 0;
}


// if not -1, replay this char on the next getchar()
int replay = -1;

// buffer of char that will be used for processing
// should be a queue.
int pending = 0;
vector<int> play_macro;

// closure to call if non-null when play_macro is empty.
// This way we can read a file small part by small part.

int text_getchar()
{
    // if replay >= 0, just return it
    // We only replay command char normally
    if (replay >= 0) {
        int c = replay;
        replay = -1;
        return c;
    }

    // if we are playing a macro,
    // get the char from there.
    if (!play_macro.empty()) {
        int c = play_macro[0];
        play_macro.erase(play_macro.begin());
        return c;
    }

    // add (+) to the title to reflect a modified text
    static int oldstatus;
    if (oldstatus != text_saved) {
        string title(file);
        if (!text_saved) title += " (+)";
        term_set_title((uchar *) title.c_str());
        oldstatus=text_saved;
    }

    // read the next input char
    // As a side effect, this update the screen
    int res = screen_getchar();

    // clear text message
    text_message.clear();

    // to group operation in the undo struct corresponding to
    // only one keystroke and remember the position that started it all
    undo_savepos();

    // record char ?
    if (record) {
      // Hack for completion in a macro...
      if (res == KEY_TAB &&
          record_data.size() > 0 &&
          record_data.back() == KEY_TAB &&
          !is_indent()) {
        // do not add extra tab.
      } else {
         record_data.pb(res);
      }
    }

    // return char
    return res;
}

/*******************************************************************/
// useful functions
/*******************************************************************/

int is_begin()
{
    int i=text_gap;
    return (i==0 || text[i-1]==EOL);
}

int is_end()
{
    int i=text_restart;
    return (i==text_end || text[i]==EOL);
}

int is_space_before()
{
    int i=text_gap;
    return (i==0 || text[i-1]==EOL || text[i-1]==' ');
}

int is_letter_before()
{
    int i=text_gap-1;
    return (i>=0 && isletter(text[i]));
}

/*******************************************************************/
// implementation of the commands

void yank_line()
{
    selection.clear();
    int c;
    text_move(line_begin());
    do {
        int i=text_gap;
        text_move(line_end()+1);
        while (i<text_gap) {
            selection.pb(text[i]);
            i++;
        }
        c =text_getchar();
    } while (c==KEY_COPY);
    replay=c;
}

void del_line()
{
    selection.clear();

    // move to line begin
    text_move(line_begin());

    int c;
    do {
        // no more line ??
        if (text_lines==0) return;

        // save line
        int i=text_restart;
        do {
            selection.pb(text[i]);
            i++;
        } while (i<text.sz && selection[selection.sz-1]!=EOL);

        // delete it
        while (text_restart+1<text.sz && text[text_restart]!=EOL) {
            text_delete();
        }
        text_delete();

        c = text_getchar();
    } while (c==KEY_CUT);
    replay = c;
}

void text_print()
{
    fi (selection.sz)
        text_putchar(selection[i]);
}

void insert_indent()
{
    // move to first non blanc char
    while (text_restart<text_end && text[text_restart]==' ') {
        text_move(text_restart+1);
    }
    // add indent.
    int pos=compute_pos();
    do {
        text_putchar(' ');
        pos++;
    } while (pos % TABSTOP != 0);
}

void smart_backspace()
{
    int i=text_gap;

    // remove trailing char
    if (i>0 && text[i-1]==EOL) {
        do {
            text_backspace();
            i--;
        } while (i>0 && text[i-1]==' ');
        return;
    }

    // remove indent if necessary
    if (i>0 && is_indent()) {
//    if (i>0 && text[i-1]==' ') {
        int pos = compute_pos();
        do {
            text_backspace();
            pos--;
            i--;
        } while (i>0 && text[i-1]==' ' && pos % TABSTOP !=0);
        return;
    }

    // normal behavior
    text_backspace();
}

void smart_delete()
{
    int i=text_restart;

    // deal with indent
    if (i+1<text_end && text[i]==' ') {
        int pos=compute_pos();
        int pos2=0;
        while (i<text_end && text[i]==' ') {
            i++;
            pos2++;
        }
        do {
            text_delete();
            pos2--;
        } while (pos2>0 && (pos+pos2) % TABSTOP !=0);
        return;
    }

    // deal with end of line
    if (i+1<text_end && text[i]==EOL && text_gap>0 && text[text_gap-1]!=EOL) {
        do {
            text_delete();
            i++;
        } while (i+1<text_end && text[i+1]==' ');
        return;
    }

    // normal behavior
    text_delete();
}

int auto_indent=1;
void smart_enter()
{
    if (auto_indent==0) {
        text_putchar(EOL);
        return;
    }

    // compute line indent from current position
    int i=line_begin();
    int pos=0;
    while (i<text_gap && text[i]==' ') {
        i++;
        pos++;
    }

    // put EOL and correct indent
    text_putchar(EOL);
    fj(pos) text_putchar(' ');
}

void toggle_ai()
{
    if (auto_indent) {
        text_message="auto indent off";
        auto_indent=0;
    } else {
        text_message="auto indent on";
        auto_indent=1;
    };
}

void open_line_after()
{
    text_move(line_end());
    smart_enter();
}

void open_line_before()
{
    text_move(line_begin());

    // compute line indent
    int pos=0;
    int i=text_restart;
    while (text[i++]==' ') pos++;

    // insert it
    for (int j=0; j<pos; j++) text_putchar(' ');
    text_putchar(EOL);
    text_move(text_gap-1);
}

//*************************************************
//** Internal search functions
//*************************************************

// check if string [s] appears at position [i] in [text]
// return 0 if not or the position just after the end if yes
// treat whitespace at begining/end of [s] in a special way
int match(vector<int> &s, int i)
{
    int j=0;
    if (s.sz==0) return 0;

    // space at begining
    if (s[0]==' ') {
        int t=i;
        if (t>text_gap && t<=text_restart) t=text_gap;
        if (t>0 && isletter(text[t-1])) return 0;
        j=1;
    }

    // space at the end;
    int size = s.sz;
    if (s[size-1]==' ') size--;
    if (size==0) return 0;

    // match
    if (i<0) return 0;
    while(j<size && i<text_end) {
        if (i>=text_gap && i<text_restart) i=text_restart;
        if (s[j]==text[i]) {
            i++;
            j++;
        } else {
            return 0;
        }
    }
    if (i==text_end) return 0;

    // space at the end
    if (i>=text_gap && i<text_restart) i=text_restart;
    if (j<s.sz && isletter(text[i])) return 0;

    // return correct pos
    return i;
}

// search backward for [s] in [text], starting just before [i]
// return the position at the beginning of the match
// or -1 if no match.
int search_prev(vector<int> &s, int i, int limit=0)
{
    limit=max(0,limit);
    if (i>text_end) return -1;
    while (i>limit) {
        i--;
        if (i>=text_gap && i<text_restart) i=text_gap-1;
        if (match(s,i))
            return i;
    }
    return -1;
}

// search forward for [s] in [text], starting at [i]
// return the position just after the match
// or -1 if no match.
int search_next(vector<int> &s, int i, int limit=text_end)
{
    limit=min(text_end,limit);
    if (i<0) return -1;
    while (i<limit) {
        if (i>=text_gap && i<text_restart) i=text_restart;
        int t=match(s,i);
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
set< vector<int> > possibilities;

// first completion = last one with the same begin
// I think this is cool
map< vector<int> , vector<int> > last_completions;

// Interface function for the completion
void text_complete()
{
    undo_flush();
    possibilities.clear();
    vector<int> begin;
    vector<int> end;
    possibilities.insert(end);

//  find begining of current word
    int i=text_gap-1;
    while (i>=0 && !isalphanum(text[i])) i--;
    while (i>=0 && isalphanum(text[i])) i--;
    i++;

//  not after a word? return.
    if (i==text_gap) return;

//  [pos] always corresponds to the beginning of the last match
    int pos=i;

//  compute [begin] (that is the search pattern)
//  put white space first so we don't match partial word.
    begin.pb(' ');
    while (i<text_gap) {
        begin.pb(text[i]);
        i++;
    }

//  look first in the map
    int c = KEY_TAB;
    if (last_completions.find(begin)!=last_completions.end()) {
        end = last_completions[begin];
        possibilities.insert(end);
        fi (end.sz) text_putchar(end[i]);
        c = text_getchar();
    }

//  first search backward
    int backward=1;

//  No match or user not happy,
//  look in the text
    while (c==KEY_TAB)
    {
        // remove current completion proposal
        if (end.sz>0) {
            fi (end.sz) text_backspace();
        }

        // compute end
        // use possibilities to not propose the same thing twice
        do {
            end.clear();
            if (backward) {
                pos = search_prev(begin, pos);
                if (pos>=0) {
                    int i=pos + begin.sz-1;
                    while (isalphanum(text[i]) && i<text_gap) {
                        end.pb(text[i]);
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
                if (pos>=0) {
                    int i=pos;
                    while (isalphanum(text[i]) && i<text_end) {
                        end.pb(text[i]);
                        i++;
                    }
                } else {
                    // no more match,
                    // quit the completion functions
                    return;
                }
            }
        } while (possibilities.find(end)!=possibilities.end());

        // use current end,
        possibilities.insert(end);
        fi (end.sz) text_putchar(end[i]);

        c = text_getchar();
    }
    replay=c;

    // save completed text in map
    // except if it is empty
    if (!end.empty())
        last_completions[begin]=end;

    return;
}

// Almost same as previous one ...
void text_search_complete(vector<int> &str)
{
    possibilities.clear();
    vector<int> begin;
    vector<int> end;
    possibilities.insert(end);

//  do not complete if str is empty
    if (str.empty()) return;

//  set begin to str
    if (str[0]!=' ') begin.pb(' ');
    fi (str.sz)
        begin.pb(str[i]);

//  start wit cursor
    int pos=text_restart;

//  look first in the map
    int c = KEY_TAB;
    if (last_completions.find(begin)!=last_completions.end()) {
        end = last_completions[begin];
        possibilities.insert(end);
        fi (end.sz) text_putchar(end[i]);
        c = text_getchar();
    }

//  first search backward
//  then from start...
    int from_start=0;

//  No match or user not happy,
//  look in the text
    while (c==KEY_TAB)
    {
        // remove current completion proposal
        if (end.sz>0) {
            fi (end.sz) str.pop_back();
        }

        // compute end
        // use possibilities to not propose the same thing twice
        do {
            end.clear();

            pos = search_next(begin, pos);
            if (pos>=0) {
                int i=pos;
                while (isletter(text[i]) && i<text_end) {
                    end.pb(text[i]);
                    i++;
                }
            } else {
                if (from_start) return;
                from_start = 1;
                pos = 0;
            }
        } while (possibilities.find(end)!=possibilities.end());

        // use current end,
        possibilities.insert(end);
        fi (end.sz) str.push_back(end[i]);

        c = text_getchar();
    }
    replay=c;

    // save completed text in map
    // except if it is empty
    if (!end.empty())
        last_completions[begin]=end;

    return;
}

//******************************************
//** user search functions
//******************************************

// Pattern is the current search pattern

vector<int> pattern;
int display_pattern=0;

// get id at current cursor position
void get_id()
{
    vector<int> old_pattern = pattern;

    pattern.clear();
    pattern.pb(' ');
    int i=text_gap;
    while (i>0 && isletter(text[i-1])) i--;
    while (i<text_gap) {
        pattern.pb(text[i]);
        i++;
    }
    i = text_restart;
    while (i<text_end && isletter(text[i])) {
        pattern.pb(text[i]);
        i++;
    }
    pattern.pb(' ');

    if (pattern.sz==2) pattern=old_pattern;
}

// search next occurrence of [pattern] from text_restart
// [pattern] is the previous pattern if (search_highlight==1)
// else it is the one returned by get_id()
int text_search_next()
{
    if (search_highlight==0) {
        get_id();
        search_highlight=1;
        // move at end of id instead of looking
        // for the next one
        int i = text_restart;
        while (i<text_end && isletter(text[i])) {
            i++;
        }
        text_move(i);
        return 1;
    }

    int t = search_next(pattern,text_restart);
    if (t>0)  text_move(t);
    else {
        text_message = "search restarted on top";
        t = search_next(pattern,0);
        if (t>0) text_move(t);
        else {
            text_message = "word not found";
            return 0;
        }
    }
    return 1;
}

// search first occurrence of [pattern]
// [pattern] is the previous pattern if (search_highlight==1)
// else it is the one returned by get_id()
void search_first()
{
    if (search_highlight==0) get_id();
    search_highlight=1;

    // search for it
    // starting at the beginning
    text_move(0);
    text_search_next();
}

void search_again()
{
  search_highlight=1;
  text_search_next();
}

// search previous occurrence of [pattern] from text_gap
// [pattern] is the previous pattern if (search_highlight==1)
// else it is the one returned by get_id()
int text_search_prev()
{
    if (search_highlight==0) get_id();
    search_highlight=1;

    int size = pattern.sz;
    if (pattern.sz>0 && pattern[0]==' ') size--;
    if (pattern.sz>0 && pattern[pattern.sz-1]==' ') size--;

    int t = search_prev(pattern,text_gap-size);
    if (t>=0) {
        t = text_index_from_absolute(t+size);
        text_move(t);
    } else {
        text_message = "search restarted on bottom";
        t = search_prev(pattern,text_end-2);
        if (t>=0) {
            t = text_compute_position(t,size);
            text_move(t);
        } else {
            text_message = "word not found";
            return 0;
        }
    }
    return 1;
}

// Let the user enter a new pattern
// if empty use the old one...
void text_new_search()
{
    search_highlight=1;
    vector<int> old_pattern = pattern;
    pattern.clear();
    while (1) {
        if (pattern.empty()) {
            display_pattern=0;
            text_message="<search>";
        } else {
            display_pattern=1;
        }
        int c = text_getchar();
        if (isprint(c)) {
            pattern.pb(c);
            continue;
        }
        if (c==KEY_BACKSPACE && !pattern.empty()) {
            pattern.pop_back();
            continue;
        }
        if (c==KEY_TAB) {
            text_search_complete(pattern);
            continue;
        }
        if (c==KEY_ENTER) {
            if (pattern.empty())
                pattern = old_pattern;
            text_search_next();

            break;
        }
        replay = c;
        break;
    }
    display_pattern=0;
}

/******************************************************************/
/******************************************************************/

void text_kill_word()
{
    while (text_gap>0 && !isletter(text[text_gap-1])) text_backspace();
    while (text_gap>0 && isletter(text[text_gap-1])) text_backspace();
}

void text_delete_to_end()
{
    while (text[text_restart]!=EOL)
        text_delete();
}

// TODO: really bad undowise
void text_change_case()
{
    int i=text_restart;
    int c;
    if (text[i]>='a' && text[i]<='z') c = text[i]-'a'+'A';
    else if (text[i]>='A' && text[i]<='Z') c = text[i]-'A'+'a';
    else return;
    text_delete();
    text_putchar(c);
}

void text_tab()
{
    if (is_indent())
        insert_indent();
    else
        text_complete();
}

void space_tab()
{
    if (is_indent())
        insert_indent();
    else
        text_putchar(' ');
}

// This is what can be recorded in an
// automatic macro. Always stays
// on the same line.
// MAYBE : add some inline movements.
void insert()
{
    while (1)
    {
        int c = text_getchar();
//        if (c==' ') space_tab();else
        if (isprint(c)) text_putchar(c);
        else switch (c)
        {
            case KEY_INSERT :
                text_message="<insert>";
                c=text_getchar();
                if (c==EOL) {
                    replay=KEY_ENTER;
                    return;
                }
                text_putchar(c);
                break;
            case KEY_BACKSPACE :
                if (is_begin()) {
                    replay=c;
                    return;
                }
                smart_backspace();
                break;
            case KEY_DELETE :
                if (is_end()) {
                    replay=c;
                    return;
                }
                smart_delete();
                break;
            case KEY_KWORD: text_kill_word();break;
            case KEY_DEND: text_delete_to_end();break;
            case KEY_CASE: text_change_case();break;
            case KEY_TAB: text_tab();break;
            default:
                replay=c;
                return;
        }
    }
}

/******************************************************************/
/******************************************************************/

void text_up()
{
    text_move(line_begin());
    text_move(text_gap-1);
    line_goto(base_pos);
}

void text_down()
{
    text_move(line_end()+1);
    line_goto(base_pos);
}

void text_back_word()
{
    int i=text_gap;
//    while (i>0 && (!isletter(text[i-1]))) i--;
//    while (i>0 && isletter(text[i-1])) i--;
    while (i>0 && isletter(text[i-1])) i--;
    while (i>0 && (!isletter(text[i-1]))) i--;
    text_move(i);
}

void text_next_word()
{
    int i=text_restart;
    while (i<text_end && (!isletter(text[i]))) i++;
    while (i<text_end && isletter(text[i])) i++;
    text_move(i);
}

void text_next_letter()
{
    int i=text_restart;
    while (i<text_end && isletter(text[i])) i++;
    while (i<text_end && (!isletter(text[i]))) i++;
    text_move(i);
}

/******************************************************************************/
/******************************************************************************/

// Justify the whole paragraph where
// a paragraph is a set of line that start with a letter (no white)
// Why we need this ? it is because of our line wrap policy ...
// it is convenient to have it, like in nano. Also quite convenient
// to check if a line is > 80 char or not.

void justify()
{
    int i=text_gap;
    int last=-1;
    char c=text[text_restart];
    while (i>0 && (text[i-1]!=EOL || isletter(c))) {
        if (text[i-1]==EOL) last=i;
        i--;
        c=text[i];
    }
    if (last<0 || i==0) last=i;

    text_move(last);
    i=text_restart;
    while (i+1<text_end && (text[i]!=EOL || isletter(text[i+1]))) {
        if (text[i]==EOL) {
            text_move(i);
            text_delete();
            text_putchar(' ');
        }
        i++;
    }
    text_move(last);
    i=text_restart;
    int count=0;
    int b=-1;
    while (i<text_end && text[i]!=EOL) {
        if (text[i]==' ') b=i;
        count++;
        // Line length without EOL <= JUSTIFY.
        if (count > JUSTIFY) {
            if (b>0) {
              text_move(b);
              text_delete();
            } else {
              text_move(i);
            }
            text_putchar(EOL);
            count=0;
            b=-1;
            i = text_restart-1;
        }
        i++;
    }
}

/******************************************************************/
/******************************************************************/

// we store there the last automatic macro
vector<int>  macro_data;
// absolute position in the text of where the last
// operation was executed
int          macro_end=-1;

// debug
void macro_display() {
    SS yo;
    fi (macro_data.sz) yo << macro_data[i];
    yo << '#';
    yo << macro_end;
    text_message = yo.str();
}

// insert is called throught this
int macro_record()
{
    start_record();
    insert();
    end_record();

    if (record_data.sz > 1) {
        record_data.erase(record_data.end()-1);
        macro_data = record_data;
        // to be sure it will exit insert()
        // next time we execute it...
        macro_data.pb(KEY_NULL);
        macro_end = text_gap;
        return 1;
    }
    return 0;
}

void macro_exec()
{
    play_macro = macro_data;
    insert();

    macro_end=text_gap;
    play_macro.clear();
    replay=-1;
}

void replace()
{
    int i=0;
    int limit=text_gap;
    text_move_to_absolute(macro_end);

    int old=0;
    int loop=1;
    while (loop && text_search_next())
    {
        i++;
        if (text_gap>=limit && (macro_end<limit || macro_end>=text_gap))
            loop=0;
        old=text_gap;

        macro_exec();

        // Beware !!
        // limit change because we replace stuff...
        if (old<limit) {
            limit += text_gap-old;
        }
    }

    SS m;
    m << i << " operations";
    text_message = m.str();
}

void macro_till()
{
    if (search_highlight) return replace();

    int limit=text_l;
    text_move_to_absolute(macro_end);
    while (text_l!=limit)
    {
        if (text_l>limit) text_up();
        else text_down();
        macro_exec();
    };
}

/******************************************************************/
/******************************************************************/

int text_goto() {
    char c;
    int num=0;
    int count=0;
    string my_message="";
    while (1) {
        if (count)
            text_message = my_message;
        else
            text_message = "<goto>";
        c=text_getchar();
        if (isnum(c)) {
            num = 10*num + c-'0';
            count++;
            my_message.pb(c);
        } else if (c==KEY_BACKSPACE && count>0) {
            num /=10;
            my_message.erase(my_message.end()-1);
            count--;
        } else if (c==KEY_ENTER) {
            num = (num + text_lines - 1) % text_lines;
            int i=text_line_begin(num);
            text_move(i);
            line_goto(base_pos);
            break;
        } else {
            replay=c;
            break;
        }
    }
}

// ********************************************************
// ********************************************************

void jump_interface()
{
  // Remove bad positions.
  update_jump_on_move(text_restart);
  // Return if nothing to do.
  if (jump_size == 0) {
    return;
  }
  int i=0;
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

int move_command(uchar c)
{
    int search=0;
    int base=1;

    switch (c)
    {
        // Search move
        case KEY_AGAIN: search_again(); search=1; break;
        case KEY_FIND: text_new_search(); search=1; break;
        case KEY_FIRST: search_first(); search=1; break;
        case KEY_NEXT: text_search_next(); search=1; break;
        case KEY_PREV: text_search_prev(); search=1; break;

        // Move that do not reset base pos
        case KEY_UP: text_up();base=0;break;
        case KEY_DOWN: text_down();base=0;break;
        case KEY_GOTO: text_goto();base=0;break;

        case PAGE_UP: screen_ppage();line_goto(base_pos);base=0;break;
        case PAGE_DOWN: screen_npage();line_goto(base_pos);base=0;break;

        // Inline move that reset base pos
        case KEY_TAB: text_next_letter();break;
        case KEY_BWORD: text_back_word();break;
        case KEY_FWORD: text_next_word();break;
        case KEY_BEGIN: text_move(line_begin());break;
        case KEY_END: text_move(line_end());break;
        case KEY_LEFT: text_move(text_gap-1);break;
        case KEY_RIGHT: text_move(text_restart+1);break;

        default : return 0;
    }

    if (!search) search_highlight=0;
    if (base) base_pos = compute_pos();
    return 1;
}

// ********************************************************
// ********************************************************

//void yank_select(int mark) {
//    int b,e;
//    if (mark<text_gap) {
//        b=mark;
//        e=text_gap;
//    } else {
//        b=text_restart;
//        e=mark + text_restart-text_gap;
//    }
//    selection.clear();
//    while (b<e) {
//        selection.pb(text[b]);
//        b++;
//    }
//}
//
//void del_select(int mark) {
//    if (mark<text_gap) {
//        while (mark<text_gap)
//            text_backspace();
//    } else {
//        int limit = mark + text_restart - text_gap;
//        while (text_restart<limit)
//            text_delete();
//    }
//}
//
//void text_select()
//{
//    int mark = text_gap;
//    while (1) {
//        int c = text_getchar();
//        if (move_command(c)) continue;
//        switch (c) {
//            case KEY_COPY : yank_select(mark);
//                             return;
//            case KEY_CUT : yank_select(mark);
//                             del_select(mark);
//                             return;
//            case KEY_PRINT : del_select(mark);
//                             text_print();
//                             return;
//            case KEY_ESC : text_move_to_absolute(mark);
//                           base_pos=compute_pos();
//                           return;
//        }
//        replay = c;
//        return;
//    }
//}

// save pos on any modification...
int mainloop()
{
    int first_move=1;
    int c=0;
    while (c!=KEY_QUIT)
    {
        // insert? + record?
        if (macro_record()) {
            add_jump_pos();
        }

        // other command to process
        int b=1;
        c = text_getchar();

        if (first_move) add_jump_pos();
        if (move_command(c)) {
            first_move=0;
            continue;
        }
        first_move=1;

        switch (c) {
            case KEY_JUMP: jump_interface();break;
            case KEY_AI : toggle_ai();break;
//            case KEY_MARK: text_select();b=0;break;
            case KEY_UNDO: text_undo();break;
            case KEY_TILL: macro_till();b=0;break;
            case KEY_REDO:
                if (!text_redo()) {
                    macro_exec();
                    b=0;
                }
                break;
            case KEY_COPY: yank_line();break;
            case KEY_CUT: del_line();break;
            case KEY_PRINT: text_print();break;
            case KEY_OLINE: open_line_before();break;
            case KEY_JUSTIFY: justify();break;
            case KEY_SAVE: text_save();break;
            case KEY_ENTER: smart_enter();break;

            // here only on line boundary ...
            case KEY_BACKSPACE: smart_backspace();break;
            case KEY_DELETE: smart_delete();break;
            default: b=0;
        }
        if (b) {
            base_pos=compute_pos();
        }
    }
}

/******************************************************************/
/******************************************************************/

// Clean this : mouse control from screen.cpp
// we have to clean pgup/pgdown from screen.cpp too

const int X_buffer_size=1024;
uchar X_buffer[X_buffer_size];

void mouse_X_select(int b, int e)
{
    FILE *fd = popen("xclip 2>/dev/null", "w");

    if (fd==NULL) {
        // even if the command is not there, fd is valid !
        text_message = "Unable to copy to X selection. Is xclip installed?";
        return;
    }

    while (b<=e) {
        int n=0;
        while (b<=e && n+4<X_buffer_size) {
            if (b==text_gap) b=text_restart;
            int temp = text[b];
            b++;
            do {
                X_buffer[n++] = temp & 0xFF;
                temp >>=8;
            } while (temp);
        }

        fwrite(X_buffer, 1, n, fd);
    }
    pclose(fd);
}

void mouse_X_paste()
{
    undo_flush();
    undo_savepos();

    FILE *fd = popen("xclip -o 2>/dev/null","r");
    if (fd == NULL) {
        // even if the command is not there, fd is valid !
        text_message="Unable to get X selection. Is xclip installed?";
        return;
    }

    int n;
    int l=0;
    int p=0;
    int ch;
    do {
        n=fread(X_buffer,1,X_buffer_size,fd);

        fi (n) {
            uchar c=X_buffer[i];

            if (p<l) {
                ch ^= c << (8*(p+1));
                p++;
                if (p==l) {
                    text_putchar(ch);
                    p=l=0;
                }
                continue;
            }

            if ((c >> 7)&1) {
                /* compute utf8 encoding length */
                l=0;
                while ((c >> (6-l))&1) l++;
                ch = c;
                continue;
            }
            text_putchar(c);
        }
    } while(n);

    pclose(fd);
}

void mouse_select(int b, int e) {
    mouse_X_select(b,e);
    selection.clear();
    while (b<=e) {
        if (b==text_gap) b=text_restart;
        selection.pb(text[b]);
        b++;
    }
}

// not used ...
void mouse_delete(int b, int e)
{
    undo_flush();
    undo_savepos();

    // convert position to absolute
    b = text_absolute_from_index(b);
    e = text_absolute_from_index(e);

    int save = text_gap;
    if (save>b) {
        if (save<e) save = b;
        else save -= (e-b)+1;
    }

    text_move_to_absolute(b);
    while(b<=e) {
        b++;
        text_delete();
    }
    text_move_to_absolute(save);
}

void mouse_paste() {
    mouse_X_paste();
//    undo_flush();
//    undo_savepos();
//    text_print();
}

/******************************************************************/
/* file handling                                                  */
/******************************************************************/

// copy file to file_dir/.fe/file_name
int text_backup()
{
    if (access(file,F_OK)!=0) return 1;

    strcpy(temp_name,file_dir);
    strcat(temp_name,".fe/");
    strcat(temp_name,file_name);

    const int buf_size=1024;
    uchar buf[buf_size];

    int res=1;

    FILE *src = fopen(file,"rb");
    FILE *dst = fopen(temp_name,"wb");

    int num=0;
    if (src!=NULL && dst != NULL)
    do {
        num = fread(buf, sizeof(uchar), buf_size, src);
        if (num==0 && ferror(src)) {
            res = 0;
            break;
        }
        if (fwrite(buf, sizeof(uchar), num, dst) < num) {
            res = 0;
            break;
        }
    } while (num>0);

    if (src==NULL || fclose(src) == EOF) res=0;
    if (dst==NULL || fclose(dst) == EOF) res=0;

    return res;
}

// open file and write new content
int text_write(char* name)
{
    ofstream s(name,ios_base::trunc);
    if (!s) return 0;

    int trailing_space = 0;
    fi (text_end) {
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
            temp>>=8;
        } while (temp);
    }

    s.close();
    return 1;
}

int utf_isvalid(unsigned int c)
{
    unsigned int t = c & 0xFF;
    unsigned int l = 0;

    // over long encoding
    if (t==192 || t==193) return 0;
    // too high
    if (t>=245) return 0;

    while ((t >> (7-l)) & 1) l++;

    if (l==0 && (c>>8)) return 0;
    if (l==1) return 0;

    for (int i=1; i<l; i++) {
        c >>= 8;
        t = c & 0xFF;
        if (t>>6 != 2) return 0;
    }
    if (c>>8) return 0;

    return 1;
}

int open_file()
{
    /* Open file */
    ifstream inputStream(file);
    if (!inputStream) return 0;

    /* Put file in gap buffer text */
    char c;
    int temp=0;

    int l=0;
    unsigned int ch;
    unsigned int pending;

    while (1)
    {
        if (l) {
            ch = pending & 0xFF;
            pending >>= 8;
            l--;
        } else {
            if (!inputStream.get(c)) break;
            ch = uchar(c);

            if ((c >> 7)&1)
            {
                // compute utf8 encoding length
                l=0;
                while ((c >> (6-l))&1 && l<3) l++;

                // compute corresponding int
                fi (l) {
                    if (!inputStream.get(c)) break;
                    ch ^= uchar(c) << (8*(i+1));
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
            } while ((temp%TABSTOP)!=0);
        } else {
            if (ch == EOL) {
                // remove trailing
                while (text_gap>0 && text[text_gap-1]==' ')
                    text_back();
                text_add(EOL);
                temp=0;
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

int compute_name(char *argument)
{
    int n=strlen(argument);

    file = argument;
    file_name = (char *) malloc(n);
    file_dir = (char *) malloc(n);
    file_ext = (char *) malloc(n);

    // file directory
    strcpy(file_dir,file);
    int j=0;
    for (int i=n; i>=0; i--) {
        if (file[i]=='/') break;
        file_dir[i]=0;
        j++;
    }

    // file name
    for (int i=0; j>=0; j--,i++) {
        file_name[i] = file[n+1-j];
    }

    // file extension
    j=0;
    for (int i=n; i>=0; i--) {
        if (file[i]=='.') break;
        j++;
    }
    for (int i=0; j>=0; j--,i++) {
        file_ext[i] = file[n+1-j];
    }

    // allocate temp_name for various file we will use
    // our file should always have a name length < 20
    temp_name = (char *) malloc(n + 20);

//    printf("%s\n",file);
//    printf("%s\n",file_name);
//    printf("%s\n",file_dir);
//    printf("%s\n",file_ext);
}

// ********************************************************
// To remember last line on repoen.
// ********************************************************

struct s_info {
  int absolute_pos;
  int screen_first_line;
};

// map filename -> s_info
map<string, struct s_info> info;

int load_info()
{
    strcpy(temp_name,file_dir);
    strcat(temp_name,".fe/info");

    ifstream s;
    s.open(temp_name);
    if (!s) return 0;

    string name;
    int absolute_pos;
    int screen_first_line;
    while (s >> name >> absolute_pos >> screen_first_line) {
        info[name].absolute_pos = absolute_pos;
        info[name].screen_first_line = screen_first_line;
    }
    s.close();

    if (info.find(file_name)!=info.end()) {
      screen_set_first_line(info[file_name].screen_first_line);
      return info[file_name].absolute_pos;
    }
    return 0;
}

void save_info()
{
    // to deal with multiple instance running
    // reload the last info, update it and save it.
    //info.clear();
    //load_info(); //pb: load info set the first line ...
    strcpy(temp_name,file_dir);
    strcat(temp_name,".fe/info");

    ofstream s;
    s.open(temp_name);

    // text_gap is the same as text_absolute_from_index(text_restart)
    info[file_name].absolute_pos = text_gap;
    info[file_name].screen_first_line = screen_get_first_line();
    map<string, struct s_info>::iterator iter = info.begin();
    while (iter!=info.end()) {
        s << (iter->first) << " "
          << (iter->second).absolute_pos << " "
          << (iter->second).screen_first_line << endl;
        iter++;
    }
    s.close();
}

// ********************************************************
// to save selection
// ********************************************************

void load_selection()
{
    strcpy(temp_name,file_dir);
    strcat(temp_name,".fe/selection");
    ifstream s(temp_name);
    if (!s) return;

    selection.clear();
    int ch;
    char c;
    while (s.get(c)) {
        ch = uchar(c);
        if ((c >> 7)&1) {
            /* compute utf8 encoding length */
            int l=0;
            while ((c >> (6-l))&1) l++;

            /* compute corresponding int */
            fi (l) {
                s.get(c);
                ch ^= uchar(c) << (8*(i+1));
            }
        }
        selection.push_back(ch);
    }
    s.close();
}

void save_selection()
{
    if (selection.sz==0) return;

    strcpy(temp_name,file_dir);
    strcat(temp_name,".fe/selection");
    ofstream s(temp_name);
    if (!s) return;

    for (int i=0; i<selection.sz; i++) {
        // convert to utf-8
        unsigned int temp = selection[i];
        do {
            s.put(temp & 0xFF);
            temp>>=8;
        } while (temp);
    }
    s.close();
}

// ********************************************************

// save file, create backup file before
// we actually overwrite the file
int text_save()
{
    if (!text_backup()) {
        text_message="Could not create backup file, press ^S to confirm save";
        int c = text_getchar();
        if (c!=KEY_SAVE) {
            replay=c;
            return 0;
        }
    }

    if (text_write(file)) {
        text_message="File saved";
        text_saved=1;
    } else {
        text_message="Could not save file";
        return 0;
    }
    return 1;
}

int text_exit()
{
    int ok=0;
    if (text_saved==0) {
        strcpy(temp_name,file_dir);
        strcat(temp_name,".fe/");
        strcat(temp_name,file_name);

        ok = text_write(temp_name);
        if (!ok) {
            text_message = "Could not create backup file, press ^Q to confirm quit";
            int c = text_getchar();
            if (c!=KEY_QUIT) {
                replay=c;
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

void terminate(int param)
{
    // so we are sure to quit even if no backup
    // could be created.
    replay=KEY_QUIT;
    text_exit();
    exit(1);
}

int main(int argc, char **argv)
{
    cache_init();
    // exit if no file_name
    if (argc<2) {
        cout << "please provide a filename." << endl;
        exit(1);
    }

    // parse file name
    compute_name(argv[1]);

    // basic option
    int start_line = -1;
    if (argc>2) {
        int num=0;
        int i=0;
        while (argv[2][i] != 0) {
            num = num * 10 + argv[2][i]-'0';
            i++;
        }
        start_line=num;
    }

    // create .fe/ if it doesn't exist
    strcpy(temp_name,file_dir);
    strcat(temp_name,".fe/");
    mkdir(temp_name,S_IRWXU);

    // load selection
    load_selection();

    // load file
    open_file();

    // init
    // TODO : necessary ??
    if (text_gap==0 || text[text_gap-1]!=EOL) {
        text_add(EOL);
    }

    // catch deadly signal for proper terminaison
    signal(SIGTERM, terminate);

    // move to given line arg or to saved pos
    if (start_line >= 0) {
      text_move(text_line_begin(start_line - 1));
    } else {
      int pos = load_info();
      text_move_to_absolute(pos);
    }
    // Fix a bug when pos > text_len
    if (text_restart == text_end) {
      text_move(0);
    }
    base_pos = compute_pos();

    // init screen
    // switch to fullscreen mode
    screen_init();
    term_set_title((uchar *)file);

    // enter mainloop
    // backup unsaved text if necessary
    // and exit properly when mainloop is done
    do {
        mainloop();
    } while (!text_exit());

    //  Thanks for using this editor !
    return EXIT_SUCCESS;
}
