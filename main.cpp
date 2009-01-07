#include "definition.h"
#include "term.h"

char        *filename;
char        *swapname;
char        *savename;

int          text_saved;

string          text_highlight;
string          text_message;
vector<int>     text;             // gap_buffer   
int             text_gap=0;       // cursor/gap absolute position
int             text_restart=0;   // end of the text until text.sz
int             text_end=0;       // number of char in the gap buffer
int             text_lines=0;     // total number of line in text
int             text_l=0;         // current line number
int             text_modif=1;     // indicateur de modif...
int             text_blocsize = 1024;

// selection
vector<int> selection;
        
void dump_error(char *err) {
    fprintf(stderr,"err: ");
    fprintf(stderr,err);
    fprintf(stderr,"\n");
}

// check text size and realloc if needed
// TODO : use realloc ?
void text_check(int l) 
{
    int gapsize=text_restart - text_gap;
    if (gapsize>=l) return;
    int num=gapsize;
    while (num<l) num += text_blocsize;
    int oldsize=text.sz;
    for (int i=0; i<num; i++) text.pb('#');
    for (int i=oldsize-1; i>=text_restart; i--) {
        text[i+num]=text[i];
        text[i]='#';
    }
    text_restart += num;
    text_end = text.sz;
}

int undo_pos=-1;
int undo_mrk=-1;
int undo_last;

void edit_text() {
    text_modif++;
    text_saved=0;
}

// ***************************
// undo stuff.
// ***************************

struct s_undo {
    int pos;
    int num;
    int del;
    vector<int> content;
};

vector<struct s_undo>    undo_stack;
vector<struct s_undo>    redo_stack;

void undo_flush() 
{
    if (undo_pos==-1) return; 
    if (text_gap == undo_pos) {
        undo_pos=-1;
        return;
    }
    
    redo_stack.clear();

    int b=min(text_gap,undo_pos);
    int e=max(text_gap,undo_pos);

    struct s_undo op;
    op.num = undo_mrk;
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

void undo_savepos() {
    undo_last = text_gap;
}

void undo_start() {
    undo_flush();
    undo_pos = text_gap;
    undo_mrk = undo_last;
    undo_last=-1;
}

void text_add(int c) 
{
    text_check(1);
    edit_text();
    if (c==EOL) {
        text_l++;
        text_lines++;
    }
    text[text_gap++]=c;
}

void text_sup() 
{
    if (text_restart+1>=text.sz) return;
    edit_text();

    if (text[text_restart]==EOL) 
        text_lines--;
    text_restart++;
}

void text_putchar(int c) 
{
    if (undo_pos == -1 || undo_pos>text_gap) {
        undo_start();
    }

    text_add(c);
}

void text_backspace() 
{
    if (text_gap==0) return;
    
    edit_text();
    if (undo_pos<0) {
        undo_start();
    }

    text_gap--;

    if (text[text_gap]==EOL) {
        text_l--;
        text_lines--;
    } 
}


void text_delete() 
{
    if (text_restart+1>=text.sz) return;

    edit_text();
    if (undo_pos<text_gap) {
        undo_start();
    }

    text[undo_pos]=text[text_restart];
    text_restart++;
    undo_pos++;

    if (text[text_restart-1]==EOL) 
        text_lines--;
}


/******************************************************/
/******************************************************/

/* text is 0...[gap ... [restart ... [end
 * put the cursor on the argument (text_restart=i or text_gap=i)
 * An argument in the middle do nothing.
 *
 * Never go behond the last EOL (gap<text.sz).
 *
 */
void text_move(int i) 
{
    /* we can't go behond the last EOL */
    if (i<0 || (i>=text_gap && i<text_restart) || i>=text.sz) {
        return;
    }

    // we move so save current undo
    undo_flush();

    if (i<text_gap) {
        while (text_gap !=i) {
            if (text[text_gap-1]==EOL) text_l--;
            text_restart--;
            text_gap--;
            text[text_restart]=text[text_gap];
            text[text_gap]='#';
        }
    } else {
        while (text_restart!=i) {
            if (text[text_restart]==EOL) text_l++;
            text[text_gap] = text[text_restart];
            text[text_restart]='#';
            text_gap++;
            text_restart++;
        }
    }
}

int  text_real_position(int i) {
    if (i<text_gap) return i;
    else return i + text_restart-text_gap;
}

void text_absolute_move(int i) {
    if (i<text_gap) text_move(i);
    else text_move(i + text_restart-text_gap);
}

void text_apply(struct s_undo op) 
{
    text_absolute_move(op.pos);
    if (op.del) {
        text_check(op.content.sz);
        fi (op.content.sz)
            text_add(op.content[i]);
    } else {
        fi (op.content.sz)
            text_sup();
    }

    if (op.num>=0)
        text_absolute_move(op.num);
}

void text_undo() {
    undo_flush();
    if (undo_stack.empty()) return;
    
    struct s_undo op;
    do {
        op=undo_stack.back();
        text_apply(op);
        op.del=1-op.del;
        redo_stack.pb(op);
        undo_stack.pop_back();
    } while (!undo_stack.empty() && op.num<0);
}

int text_redo() {
    undo_flush();
    if (redo_stack.empty()) return 0;
    
    struct s_undo op;
    op=redo_stack.back();
    do {
        text_apply(op);
        op.del=1-op.del;
        undo_stack.pb(op);
        redo_stack.pop_back();
        if (redo_stack.empty()) break;
        op=redo_stack.back();
    } while (op.num<0);
    return 1;
}

// **********************************************************
// **********************************************************

// next_eol/prev_eol/ i-th eol
// TODO:
// Cache values to be efficient in case of big lines...
// Problem is to change them in case of deletion/insertion modif
// But it should be easy ...

// return index of the next EOL
int next_eol() 
{
    int i=text_restart;
    while (i<text_end && text[i]!=EOL) 
        i++;
    return i;
}

// return index just after the previous EOL
int prev_eol() 
{
    int i=text_gap;
    while (i>0 && text[i-1]!=EOL) 
        i--;
    return i;
}

/* Not used exept for goto and pgup/down ... */
/* return the indice of the begining of the line l 
 * in [0,gap[ [restart,end[*/
int text_line_begin(int l) 
{
    /* text modif */
    static int previous_l=0;
    static int previous_p=0;
    static int previous_mod=0;

    /* keep l in range */
    l = l % text_lines;
    if (l<0) l+= text_lines;

    /* deal with special case*/
    if (l==text_l) {
        int i=text_gap;
        while ((i>0)&&(text[i-1]!=EOL)) i--;
        if (i<text_gap) return i;
        return text_restart;
    }

    int i,n;
    if (previous_mod == text_modif && 
        (l>text_l && previous_l>text_l || 
         l<text_l && previous_l<text_l) 
       ) {
        /* no modif and same side of the text_gap: 
         * restart from previous pos 
         */
        i=previous_p;
        n=previous_l;
    } else {
        /* restart from text_gap */
        if (l>text_l) i=text_restart;
        else {
            i=text_gap;
            while ((i>0)&&(text[i-1]!=EOL)) i--;
        }
        n=text_l;
    }

    /* find the line searching from the latest position */
    if (n<=l) {
        while (n<l) {
            while (text[i] != EOL) i++;
            n++;
            i++;
        }
    } else {
        do {
            i--;
            while ((i>0)&&(text[i-1]!=EOL)) i--;
            n--;
        } while(l<n);
    }
    
    previous_p=i;
    previous_l=l;
    previous_mod = text_modif;
    
    return i;
}

// **********************************************************

// after a change of line,
// base pos should be the original position the cursor goto
int base_pos;

// return pos in the current line
int compute_pos() 
{
    return text_gap - prev_eol();
}

// goto a given pos in the current line.
void line_goto(int pos) {
    int i=prev_eol();
    if (i==text_gap) i=text_restart;
    int p=0;
    while (p<pos && i<text.sz && text[i]!=EOL) {
        i++;
        if (i==text_gap) i=text_restart;
        p++;
    }
    text_move(i);
}

/**************************************************/
/* Remember what was done on the last edited_line */
/**************************************************/

int     record=0;
vector<int>  record_data;
vector<int>  play_macro;

void start_record() {
    record_data.clear();
    record=1;
}

void end_record() {
    record=0;
}

/* Swap file implementation in case of a crash:
 *
 * Just dump every keystroke in the swap file.
 * The behaviour is perfectly reproducible.
 * Wrong: some function depends on screen size !! -> fix this
 *
 * At some point I wanted to use it to undo stuff, but seems 
 * like too much trouble. However, if we keep the same file
 * from one edit to another, we can reconstruct the undo struct,
 * jump between save and even go to some points just before an 
 * undo that are lost in the undo struct ..
 *
 * timestamp and show the date of the save ?
 * 
 * TODO : catch deadly signal and write then ...
 */
 
string   swap_filename;
ofstream swap_stream;
int      swap_index;

/* cool:
 * We refresh the screen each time the program is waiting for a
 * char from the keyboard. And only then.
 *
 * So everything here can work without any display.
 * like in redo mode. or macro.
 *
 */

int replay=0;
int pending=0;
int text_getchar() 
{
    if (replay!=0) {
        int c = replay;
        replay = 0;
        return c;
    }

    if (!play_macro.empty()) {
        int c=play_macro[0];
        play_macro.erase(play_macro.begin());
        return c;
    }

    // force output every 10 chars
    // not sure about that, is the buffer in the process or in the system ? 
    if (swap_index  == 10) {
        swap_stream.flush();
        swap_index=0;
    }

    // add (+) to the title to reflect a modified text ?
    static int oldstatus;
    if (oldstatus!=text_saved) {
        string title(filename);
        if (!text_saved) 
            title += " (+)";
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
    if (record) record_data.pb(res);

    // TODO: run-length coding because many movement commands ??
    // or just @ + position with 4 char...
    // put a file limit < 4 GB...

    // reconvert in utf-8 before writing in swap file
    int temp = (uint) res;
    do {
        swap_stream.put( temp & 0xFF);
        swap_index++;
        temp >>= 8;
    } while (temp);
        
    // return char
    return res;
}

/*******************************************************************/
// useful functions 
 
int is_begin() {
    int i=text_gap;
    return (i==0 || text[i-1]==EOL);
}

int is_end() {
    int i=text_restart;
    return (i==text_end || text[i]==EOL);
}

int is_space_before() {
    int i=text_gap;
    return (i==0 || text[i-1]==EOL || text[i-1]==' ');
}

int capitalise(int c) {
    if ((c>='a' && c<='z')) return c-'a'+'A';
}


/*******************************************************************/
// implementation of the commands

vector<int> inserted;

void yank_line() {
    selection.clear();
    char c;
    do {
        int i=prev_eol();
        text_move(next_eol()+1);
        while (i<text_gap) {
            selection.pb(text[i]);
            i++;
        }
        c =text_getchar();
    } while (c==KEY_YLINE);
    replay=c;
}

void del_line() {
    selection.clear();

    // move to line begin
    text_move(prev_eol());
    char c;
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
        while (text_restart<text.sz && text[text_restart]!=EOL) {
            text_delete();
        }
        text_delete();

        c = text_getchar();
    } while (c==KEY_DLINE);
    replay = c;
}

void text_print() {
    fi (selection.sz) 
        text_putchar(selection[i]);
}

void insert_indent() 
{
    int pos=compute_pos();
    do {
        text_putchar(' ');
        pos++;
    } while (pos % TABSTOP != 0);
}

void smart_backspace() 
{
    int i=text_gap;

    // remove indent if necessary
    if (i>0 && text[i-1]==' ') {
        int pos = compute_pos();
        do {
            text_backspace();
            pos--;
            i--;
        } while (i>0 && text[i-1]==' ' && pos % TABSTOP !=0);
        return;
    }

    // remove trailing char
    if (i>0 && text[i-1]==EOL) {
        do {
            text_backspace();
            i--;
        } while (i>0 && text[i-1]==' ');
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
    if (i+1<text_end && text[i]==EOL) {
        do {
            text_delete();
            i++;
        } while (i+1<text_end && text[i]==' ');
        return;
    }

    // normal behavior
    text_delete();
}

void smart_enter() 
{
    // compute line indent
    // from current position
    int i=prev_eol();
    int pos=0;
    while (i<text_gap && text[i]==' ') {
        i++;
        pos++;
    }

    // put EOL and correct indent
    text_putchar(EOL);
    fj(pos) text_putchar(' ');
}

void open_line_after() 
{
    text_move(next_eol());
    smart_enter();
}

void open_line_before() 
{
    text_move(prev_eol());
    text_putchar(EOL);

    // compute line indent
    int pos=0;
    int i=text_restart;
    while (text[i++]==' ') pos++;

    // insert it
    text_move(text_gap-1);
    for (int j=0; j<pos; j++) text_putchar(' ');
}

/******************************************************************/
// Completion
// todo : search in other direction when no match
/******************************************************************/

// dictionnary
// so we do not propose twice the same completion !
set< vector<int> > possibilities;

// first completion = last one with the same begin
// I think this is cool
map< vector<int> , vector<int> > last_completions;

// for completion ...
vector<int> search_comp(vector<int> c, int &i) 
{
    vector<int> res;
    i--;
    do {
        while (i>=0 && isletter(text[i])) 
            i--;
        int a = i+1;
        int b = 0;
        while (text[a] == c[b]) { 
            a++;
            b++;
            if (b>= c.sz) { 
                if (c.sz>0 && !isletter(c[c.sz-1])) {
                    while (text[a]==' ') {
                        res.pb(text[a]);
                        a++;
                    }
                }
                while (isletter(text[a])) {
                    res.pb(text[a]);
                    a++;
                }
                if (possibilities.find(res)!=possibilities.end()) {
                    res.clear();
                }
                if (!res.empty()) 
                    return res;
                break;
            }
        }
        i--;
    } while (i>=0);
    return res;
}

vector<int> text_complete() 
{
    undo_flush();
    possibilities.clear();
    int i=text_gap-1;
    vector<int> begin;
    vector<int> end;
    
    int white=0;
//    if (i>=0 && !isletter(text[i])) white=1;

//  if previous white, do intelligent stuff
//    while (i>=0 && !isletter(text[i]) && text[i]!=EOL) {
//        begin = text[i] + begin;
//        i--;
//    }
    while (i>=0 && isletter(text[i])) i--;
    
    int temp=i+1;
    while (temp<text_gap) {
        begin.pb(text[temp]);
        temp++;
    }
    
//  if (i>=0 && text[i]==EOL) begin = EOL+begin;
    int pos=i;
    if (i<0) pos=i+1;

    char c=KEY_TAB;    
//  look first in the map
    if (last_completions.find(begin)!=last_completions.end()) {
        end = last_completions[begin];
        possibilities.insert(end);
        fi (end.sz) text_putchar(end[i]);
        c=text_getchar();
    }
        
//  No match or user not happy,
//  look in the text
    while (c==KEY_TAB) {
        if (end.sz>0) {
            fi (end.sz) 
                if (isok(end[i])) 
                    text_backspace();
        }
        if (c==KEY_TAB) {
            end=search_comp(begin,pos);
            if (end.sz==0) return end;
            possibilities.insert(end);
            fi (end.sz) text_putchar(end[i]);
            c=text_getchar();
        }
    }
    replay=c;
    
//  Save completion in map only if normal one
    if (!end.empty() && !white) 
        last_completions[begin]=end;
    
    return end;
}

int insert() {
    inserted.clear();
    while (1) {
        int c = text_getchar();
        if (c==KEY_NULL) break;
        if (c==KEY_INSERT) {
            text_message="<insert>";
            c=text_getchar();
            if (c!=EOL) {
                text_putchar(c);
                inserted.pb(c);
                continue;
            }
        }
        if (isprint(c)) {
            text_putchar(c);
            inserted.pb(c);
            continue;
        }
        if (c==KEY_BACKSPACE && !is_begin()) {
                smart_backspace();
                if (!inserted.empty()) 
                    inserted.erase(inserted.end()-1);
                continue;
        }
        if (c==KEY_DELETE && !is_end()) {
                smart_delete();
                continue;
        }
        if (c == KEY_TAB) {
            if (is_space_before()) {
                insert_indent();
                continue;
            } else {
                vector<int> temp = text_complete();
                fi (temp.sz) inserted.pb(temp[i]);
                continue;
            }
        }
        inserted.pb(EOL);
        replay = c;
        break;
    };
}

/*********************/
/* textcommand stuff */
/*********************/

string cmd;

int search_prev(string c, int i, int limit=0) 
{
    int j=c.sz-1;
    limit=max(0,limit);
    if (c.sz == 0) return -1; 
    if (i>text_end) return -1;
    if (i==(text_end-1) && c[j]==' ') j--; 
    while (i>limit) {
        if (i>=text_gap && i<text_restart) 
            i=text_gap;
        if (text[i] == c[j] || (c[j]==' ' && !isletter(text[i]))) { 
            j--;
            if (j<0) 
                return i;
        } else {
            i = i+(c.sz-1-j);
            j = c.sz-1;
        }
        i--;
    }
    return -1;
}

int search_next(string c, int i, int limit=text_end) 
{
    int j=0;
    limit=min(text_end,limit);
    if (c.sz ==0) return -1;
    if (i<0) return -1;
    if (i==0 && c[0]==' ') j=1;
    while (i<limit) {
        if (i>=text_gap && i<text_restart) 
            i=text_restart;
        if (text[i] == c[j] || (c[j]==' ' && !isletter(text[i]))) { 
            j++;
            if (j>= c.sz) 
                return i+1;
        } else {
            i = i-j;
            j = 0;
        }
        i++;
    }
    return -1;
}

int text_search(string s) 
{
    int t = search_next(s,text_restart);
    if (t>0)  text_move(t);
    else {
        text_message = "search restarted on top";
        t = search_next(s,0);
        if (t>0) text_move(t);
        else {
            text_message = "word not found";
            return 0;
        }
    }
    return 1;
}

int text_search_back(string s) 
{
    int t = search_prev(s,text_gap);
    if (t>=0)  text_move(t);
    else {
        text_message = "search restarted on bottom";
        t = search_prev(s,text_end-2);
        if (t>=0) text_move(t);
        else {
            text_message = "word not found";
            return 0;
        }
    }
    return 1;
}

int search_highlight=0;

void inline_search() {
    cmd.clear();
    int begin = text_gap;
    while (1) {
        if (cmd.empty()) {
            text_message="<find>";
            text_absolute_move(begin);
        } else {
            text_message=cmd;
            int t=search_next(cmd,text_real_position(begin));
            if (t>0)  
                text_move(t);
            else {
                text_message = cmd + " (restarted on top)";
                t = search_next(cmd,0);
                if (t>0) 
                    text_move(t);
                else {
                    text_message = cmd + " (not found)";
                    cmd.erase(cmd.end()-1);
                }
            }
        }
        search_highlight = cmd.sz;
        int c = text_getchar();
        if (isprint(c)) {
            cmd.pb(uchar(c));
            continue;
        }
        if (c==KEY_BACKSPACE && !cmd.empty()) {
            cmd.erase(cmd.end()-1);
            continue;
        }
        if (c==KEY_TAB) {
            while (isletter(text[text_restart])) {
                cmd.pb(text[text_restart]);
                text_move(text_restart+1);
            }
            continue;
        }
        if (c==KEY_ENTER) {
            break;
        }
        replay = c;
        break;
    }
    search_highlight=0;
}

vector<string> history;
void text_command(string prompt) {
    cmd.clear();
    int c;
    string current;
    int size=history.size();
    int modulo=size+1;
    int index=size;
    while (1) {
        if (cmd.empty()) {
            text_message=prompt;
        } else {
            text_message=cmd;
        }
        c=text_getchar();
        if (isprint(c)) {
            cmd.pb(uchar(c));
            current=cmd;
            continue;
        }
        if (c==KEY_BACKSPACE) {
            while (!cmd.empty() && !isok(cmd[cmd.sz-1]))
                cmd.erase(cmd.end()-1);
            if (cmd.empty()) return;
            cmd.erase(cmd.end()-1);
            current=cmd;
            continue;
        }
        if (c==KEY_UP) {
            index = (index+modulo-1) % modulo;
            if (index<size)
                cmd = history[index];
            else cmd=current;
            continue;
        }
        if (c==KEY_DOWN) {
            index = (index + 1) % modulo;
            if (index<size) 
                cmd = history[index];
            else cmd=current;
            continue;
        }
        if (c==KEY_ENTER) {
            // save in history.
            // no duplicate.
            int i=0;
            for (;i<history.sz; i++) 
                if (history[i]==cmd) break;
            if (i<history.sz) {
                for (;i+1<history.sz; i++)
                    history[i]=history[i+1];    
                history[history.sz-1]=cmd;
            } else 
                history.pb(cmd);
            return;
        }
        replay = c;
        break;
    }
}

void search_id() {
    cmd.clear();
    cmd.pb(' ');
    int i=text_gap;
    while (i>0 && isletter(text[i-1])) i--;
    while (i<text_gap) {
        cmd.pb(text[i]);
        i++;
    }
    i = text_restart;
    while (i<text_end && isletter(text[i])) {
        cmd.pb(text[i]);
        i++;
    }
    cmd.pb(' ');
    if (text_search(cmd)) text_move(text_gap-1);
}


/******************************************************************/
/******************************************************************/

// Justify the whole paragraph where
// a paragraph is a set of line that start with a letter (no white)
// Why we need this ? it is because of our line wrap policy ...
// it is convenient to have it, like in nano.

void justify() {
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
        if (isok(text[i])) count++;
        if (count>=75) {
            if (b>0) text_move(b);
            else text_move(i);
            text_delete();
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

vector<int>  macro_data;
char    macro_next=KEY_NEXT;
int     macro_end=-1;
int     macro_begin=-1;

void macro_display() {
    SS yo;
    fi (macro_data.sz) yo << macro_data[i];
    yo << '#';
    yo << macro_next << '#';
    yo << macro_end;
    text_message = yo.str();
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
            if (num>0) num = (num-1) % text_lines;
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

void text_up() {
    text_move(prev_eol()-1);
    line_goto(base_pos);
    macro_next=KEY_UP;
}
void text_down() {
    text_move(next_eol()+1);
    line_goto(base_pos);
    macro_next=KEY_DOWN;
}

int macro_record() {
    start_record();
    insert();
    end_record();
    if (record_data.sz>1) {
        record_data.erase(record_data.end()-1);
        macro_data = record_data;
        macro_data.pb(KEY_NULL);
        macro_end = text_gap;
        return 1;
    }
    return 0;
}

int macro_change_line() {
    switch (macro_next) {
        case KEY_UP: text_up();break;
        case KEY_DOWN: text_down();break;
        case KEY_NEXT: if (!text_search(cmd)) return 0;break;
        case KEY_PREV: if (!text_search_back(cmd)) return 0;break;
    }
    return 1;
}

int macro_exec() {
    play_macro = macro_data;
    // un truc qui fait rien;
    // Il faut changer ca ...
    if (text_gap == macro_end) {
        if (macro_change_line()) {
            insert();
            macro_end=text_gap;
        } else {
            play_macro.clear();
        }
    } else {
        insert();
        macro_end=text_gap;
        macro_change_line();
    }
}

void replace() {
    int i=0;
    vector<int> yop=macro_data;
    // PB : this change when we replace stuff !!
    int limit=text_gap;
    int old=0;
    while (text_search(cmd)) {
        i++;
        if (text_gap>=limit && (macro_end<limit || macro_end>=text_gap)) break;
        old=text_gap;
        play_macro = yop;
        insert();
        macro_end=text_gap;
        if (old<limit) {
            limit += text_gap-old;
        }
    }
    SS m;
    m << i << " operations";
    text_message = m.str();
}

void macro_till() {
    if (text_gap==macro_end && (macro_next==KEY_NEXT || macro_next==KEY_PREV)) 
        return replace();
    vector<int> yop = macro_data;
    yop.pb(KEY_NULL);
    int limit=text_l;
    text_absolute_move(macro_end);
    if (text_l==limit) return;
    if (text_l>limit) macro_next=KEY_UP;
    else macro_next=KEY_DOWN;
    do {
        if (!macro_change_line()) break;       
        play_macro = yop;
        insert();
        macro_end=text_gap;
    } while (text_l!=limit);
}

// ********************************************************

// Save position and return to it
// save as well screen state 
// Not clean when redoing stuff without screen ...

int  saved_gap=-1;
void save_pos() {
    saved_gap = text_gap;
    //screen_save();
}

void restore_pos() {
    if (saved_gap<0) return;
    text_absolute_move(saved_gap);
   // screen_restore();
}

// ********************************************************

void yank_select(int mark) {
    int b,e;
    if (mark<text_gap) {
        b=mark;
        e=text_gap;
    } else {
        b=text_restart;
        e=mark + text_restart-text_gap;
    }
    selection.clear();
    while (b<e) {
        selection.pb(text[b]);
        b++;
    }
}

void del_select(int mark) {
    if (mark<text_gap) {
        while (mark<text_gap) 
            text_backspace();
    } else {
        int limit = mark + text_restart - text_gap; 
        while (text_restart<limit) 
            text_delete();
    }
}

int move_command(char c) 
{
    switch (c) {
        case KEY_UP: text_up();return 1;
        case KEY_DOWN: text_down();return 1;
        case KEY_GOTO: text_goto();return 1;
    }
    switch (c) {
        case KEY_BEGIN: text_move(prev_eol());break;
        case KEY_END: text_move(next_eol());break;
        case KEY_LEFT: text_move(text_gap-1);break;
        case KEY_RIGHT: text_move(text_restart+1);break;
//        case KEY_FIND: text_command("<find>");text_search(cmd);macro_next=KEY_NEXT;break;
        case KEY_FIND: inline_search();macro_next=KEY_NEXT;break;
        case KEY_WORD: search_id();macro_next=KEY_NEXT;break;
        case KEY_NEXT: text_search(cmd);macro_next=KEY_NEXT;break;
        case KEY_PREV: text_search_back(cmd);macro_next=KEY_PREV;break;
        default : return 0;
    }
    base_pos = compute_pos();
    //save_pos();
    return 1;
}

void text_select() {
    int mark = text_gap;
    while (1) {
        int c = text_getchar();
        if (move_command(c)) continue;
        switch (c) {
            case KEY_YLINE : yank_select(mark);return;
            case KEY_DLINE : yank_select(mark);
                             del_select(mark);
                             return;
            case KEY_PRINT : del_select(mark);
                             text_print();
                             return;
            case KEY_ESC : text_absolute_move(mark);
                           base_pos=compute_pos();
                           return;
        }
        replay = c;
        return;
    }   
}

void text_back_word() {
    int i=text_gap;
    while (i>0 && text[i-1]!=EOL && text[i-1]!=' ') i--;
    while (i>0 && text[i-1]==' ') i--;
    text_move(i);
}

void text_kill_word() {
    while (text_gap>0 && text[text_gap-1]==' ') 
        text_backspace();
    while (text_gap>0 && text[text_gap-1]!=' ' && text[text_gap-1]!=EOL)
        text_backspace();
}

// save pos on any modification...
int mainloop() {
    int c=0;
    while (c!=KEY_QUIT) {
        // insert? + record?
        if (macro_record()) { 
            save_pos();
        }
        // other command to process
        int b=1;
        c = text_getchar();
        if (move_command(c)) continue;
        switch (c) {
            case KEY_KWORD : text_kill_word();break;
            case KEY_BWORD : text_back_word();break;
            case KEY_ESC : restore_pos();break;
            case KEY_EOL : text_putchar(EOL);
            case KEY_MARK: text_select();b=0;break;
            case KEY_UNDO: text_undo();break;
            case KEY_TILL: macro_till();b=0;save_pos();break;
            case KEY_REDO: 
                if (!text_redo()) {
                    macro_exec();
                    save_pos();
                    b=0;
                }
                break;
            case KEY_YLINE: yank_line();break;
            case KEY_DLINE: del_line();break;
            case KEY_OLINE: open_line_before();break;
            case KEY_JUSTIFY: justify();break;
            case KEY_PRINT: text_print();break;
            case KEY_SAVE: text_save();break;
            case KEY_ENTER: smart_enter();break;
            // here only on line boundary ...
            case KEY_BACKSPACE: smart_backspace();break;
            case KEY_DELETE: smart_delete();break;
            case KEY_DISP: // already processed in screen
                           // used internally to save pos
            default: b=0;
        }
        if (b) {
            base_pos=compute_pos();
            save_pos();
        }
    }
}

// ********************************************************

void terminate(int param) {
    reset_input_mode();
    exit(1);
}

/******************************************************************/
/******************************************************************/

void text_backup() 
{
    // Open the input file
    int read = open(filename, O_RDONLY);
    
    // Stat the input file to obtain its size
    struct stat s;
    fstat(read, &s);
    
    // Open the output file for writing, 
    // with the same permissions as the source file
    
    int write = open(savename, O_WRONLY | O_CREAT, s.st_mode);
    
    // Blast the bytes from one file to the other
    off_t off = 0;
    sendfile(write, read, &off, s.st_size);
    
    // Close up
    close(read);
    close(write);    
}

int text_save() 
{
    // flush swap so we are safe if anything happen
    swap_stream.flush();

    // open file and write new content
    ofstream s;
    s.open(filename,ios_base::trunc);
    fi (text_end) {
        // jump gap if needed
        if (i==text_gap) i=text_restart;

        // convert to utf-8
        int temp = text[i];
        do {
            s.put(temp);
            temp>>=8;
        } while (temp);
    }
    s.close();

    text_message="File saved";
    text_saved=1;
}

int open_file() {
    /* Open file */
    ifstream inputStream;
    inputStream.open(filename);
    if (!inputStream) {
        return 0;
    }

    /* Put file in gap buffer text */
    char c;
    int ch;
    int last=0;
    int temp=0;
    while (inputStream.get(c)) {
        ch = uchar(c);
        if ((c >> 7)&1) {
            /* compute utf8 encoding length */
            int l=0;
            while ((c >> (6-l))&1) l++;
            
            /* compute corresponding int */
            fi (l) {
                inputStream.get(c);
                ch ^= uchar(c) << (8*(i+1));
            }
        }
        
        // convert tab in space
        last=ch;
        if (ch == '\t') {
            do {
                text_add(' ');
                temp++;
            } while ((temp%TABSTOP)!=0);
        } else if (ch == EOL) {
            text_add(EOL);
            temp=0;
        } else {
            text_add(ch);
            temp++;
        }
    }
    if (last!=EOL) text_add(EOL);
    text_saved=1;

    /* close file */
    inputStream.close();
    
    return 1;
}

int main(int argc, char **argv) 
{
    signal(SIGTERM,terminate);

    if (argc<2) exit(1);
    filename = argv[1];

    /* basic option */
    int start_line=-1;
    if (argc>2) {
        int num=0;
        int i=0;
        while (argv[2][i]!=0) {
            num = num * 10 + argv[2][i]-'0';
            i++;
        }
        start_line=num;
    }

//  todo:
//  hash/crc ??
//  get the file timestamp and make good use of it
//  resume editing to the current pos.
//  +-num line command option

    // test for existance
    if (access(filename,F_OK)) {
//        printf("File do not exist\n");
//        exit(1);
    } else {
        if (!open_file()) {
            printf("Error openning file %s\n", argv[1]);
            exit(1);
        }
    }
    
//  TODO : do it only on the first save ...
//  Do a backup of the original file
//  not really useful if we keep the same swap file for all the editing session
//  and we use a versionning system
//  But safer for all the development phase, and we need a starting point for the 
//  swap reconstruction to work ...
    swap_filename = "." + string(argv[1]) + string(".old");
    swap_stream.open(swap_filename.c_str());
    if (!swap_stream) {
        printf("Error openning backup file %s\n", swap_filename.c_str());
        exit(0);
    }
    
//  the text_gap is at the end of the file
//  after the loading process ...
    fi (text_gap) swap_stream.put(text[i]);
    swap_stream.flush();
    swap_stream.close();

//  Swap file name 
//  problem with directory ...
    swap_index = 0;
    swap_filename = "." + string(argv[1]) + string(".swp"); 

//  Read old pos 
//  pretty ugly but other sol 
//  difficult with c++ file handling
    int oldpos=0;
    ifstream test;
    test.open(swap_filename.c_str());
    if (test) {
        test >> oldpos;
    }
    test.close();
    
// Open swap file
    swap_stream.open(swap_filename.c_str());
    if (!swap_stream) {
        printf("Error openning swap file %s\n", swap_filename.c_str());
        exit(0);
    }
//  to be able to write the position 
//  without destroing swap at the end...
    fi (16) swap_stream << ' ';
    swap_stream << EOL;

//  Set position in the text
//  TODO: write corresponding instructions in swapfile. 
    text_move(oldpos);
    if (start_line>=0) text_move(text_line_begin(start_line-1));
    base_pos=compute_pos();
    
    screen_init();
    screen_redraw();
    term_set_title((uchar *)argv[1]);

    mainloop();

//  output current position to be able
//  to restore it the next time we open the file
//  erase the beginning of the swap file
//  wanted to use the end but then difficult to read !!
    swap_stream.seekp(0);
    swap_stream << text_gap;
    swap_stream.put(EOL);
    swap_stream.flush();
    swap_stream.close();

//  leave the terminal correctly
    reset_input_mode();
    
// don't work all the time ??
    if (text_saved==0)
        cout << "The file was not saved" << endl;
    cout.flush();

    exit(0);
}
