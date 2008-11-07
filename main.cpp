#include "definition.h"
#include "term.h"

string       filename;
int          text_saved;

string          text_highlight;
string          text_message;
vector<char>    text;           // gap_buffer   
int             text_gap;       // cursor/gap absolute position
int             text_restart;   // end of the text until text.sz
int             text_end;       // number of char in the gap buffer
int             text_lines;     // total number of line in text
int             text_l;         // current line number
int             text_modif;     // indicateur de modif...
int             text_blocsize = 1024;

string saved_lines("");
        
void dump_error(char *err) {
    fprintf(stderr,"err: ");
    fprintf(stderr,err);
    fprintf(stderr,"\n");
}

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

int undo_pos;
int undo_mrk;
int undo_last;

void text_init() 
{
    undo_pos=-1;

    text_gap=0;
    text_l=0;
    text_restart=0;
    text_modif=1;
    text_end=0;
    text_lines=0;
    text_check(text_blocsize);
    undo_mrk = -1;
}   

void edit_text() {
    text_modif++;
    text_saved=0;
}

// ***************************
// undo stuff.

struct s_undo {
    int pos;
    int num;
    int del;
    string content;
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
//  not sure ... 
    if (op.del) {
        saved_lines=op.content;
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

void text_backspace(int flag=1) 
{
    if (text_gap==0) return;
    
    edit_text();
    if (undo_pos<0) {
        undo_start();
    }

    do {
        text_gap--;
    } while (flag && text_gap>0 && !isok((uchar) text[text_gap])); 

    if (text[text_gap]==EOL) {
        text_l--;
        text_lines--;
    } 
}


void text_delete(int flag=1) 
{
    if (text_restart+1>=text.sz) return;

    edit_text();
    if (undo_pos<text_gap) {
        undo_start();
    }

    do {
        text[undo_pos]=text[text_restart];
        text_restart++;
        undo_pos++;
    } while (flag && text_restart<text.sz && !isok((uchar) text[text_restart])); 

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
 * Utf-8 :
 * Never stop in the middle of a sequence !!
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
        /* utf8 correction down */
        while (i>0 && !isok((uchar) text[i])) i--;
        while (text_gap !=i) {
            if (text[text_gap-1]==EOL) text_l--;
            text_restart--;
            text_gap--;
            text[text_restart]=text[text_gap];
            text[text_gap]='#';
        }
    } else {
        /* utf8 correction up */
        while (i<text.sz && !isok((uchar) text[i])) i++;
        while (text_restart!=i) {
            if (text[text_restart]==EOL) text_l++;
            text[text_gap] = text[text_restart];
            text[text_restart]='#';
            text_gap++;
            text_restart++;
        }
    }
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

void text_typechar(int c) 
{
    text_putchar(c);
    
    // deal with indent
//  if (isok((uchar) c))
//  if (text_restart+1<text.sz && text[text_restart]==' ' && text[text_restart+1]==' ') 
//      text_delete();
}

// return index off the next EOL
int next_eol() {
    int i=text_restart;
    while (i<text_end && text[i]!=EOL) 
        i++;
    return i;
}

// return index just after the previous EOL
int prev_eol() {
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

// after a change of line,
// base pos should be the original position the cursor goto
int base_pos;

int compute_pos() 
{
    int res=0;
    int i=text_gap;
    while (i>0 && text[i-1] !=EOL ) {
        if (isok((uchar) text[i-1])) res++;
        i--;
    }
    return res;
}

// goto a given pos in the current line.
void line_goto(int pos) {
    int i=prev_eol();
    if (i==text_gap) i=text_restart;
    int p=0;
    while (p<pos && i<text.sz && text[i]!=EOL) {
        i++;
        if (i==text_gap) i=text_restart;
        // utf-8
        if (isok((uchar) text[i])) p++;
    }
    text_move(i);
}

/**************************************************/
/* Remember what was done on the last edited_line */
/**************************************************/

int     record=0;
string  record_data;
string  play_macro;

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
void text_refresh() 
{
    screen_refresh();
    text_message.clear();
}

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

    /* utf8 encoding, do not refresh before we have the full char */
    if (pending==0) {
        static int oldstatus;
        text_refresh();
        if (oldstatus!=text_saved) {
            string title=filename;
            if (!text_saved) 
                title += " (+)";
            term_set_title((uchar *) title.c_str());
            oldstatus=text_saved;
        }
    } else {
        pending --;
    }

    /* read the next input char */
    int res = term_getchar();

    /* compute utf8 encoding length */
    if (res>=0xC0) {
        int t=6;
        pending=0;
        while ((res & (1<<t)) !=0) {
            t--;
            pending++;
        }
    }

    // to group operation in the undo struct corresponding to
    // only one keystroke and remember the position that started it all
    undo_savepos();
    
    // record char ?
    if (record) record_data+=res;

    swap_stream.put(res);
    swap_index++;
    return res;
}

/*******************************************************************/
/* implementation of the commands 
 */
string inserted("");

string last_line_commands;
// tab on the same line, strip last command group
// otherwise execute all.

void yank_line() {
    saved_lines.clear();
    int i = text_gap;
    while (i>0 && text[i-1]!=EOL) i--;
    while (i<text_gap) {
        saved_lines += text[i];
        i++;
    }
    i=text_restart;
    while (i<text_end && text[i]!=EOL) {
        saved_lines += text[i];
        i++;
    }
    saved_lines +=EOL;
}

void del_line() {
    saved_lines.clear();

    // move to line begin
    text_move(prev_eol());

    // Clean this,  mecanism not pretty since c==KEY_DLINE
    while (1) {
        int c = text_getchar();
        // delete whole line
        if (c==KEY_DLINE) {
            // no more line ??
            if (text_lines==0) return;
            
            // save line 
            int i=text_restart;
            do {
                saved_lines.pb(text[i]);
                i++;
            } while (i<text.sz && saved_lines[saved_lines.sz-1]!=EOL);
            // delete it
            while (text_restart<text.sz && text[text_restart]!=EOL) {
                text_delete();
            }
            text_delete();
            continue;
        }
        // small redo
        // Beware directly modify text !!
        // clean this
  /*      if ((c==KEY_UNDO || c=='`') && !saved_lines.empty()) {
            int i=saved_lines.sz-1;
            do {
                text[--text_restart]=saved_lines[i];
                text[--undo_pos]='A';
                i--;
            } while (i>=0 && saved_lines[i]!=EOL); 
            SS test;
            test << text_gap << " " << undo_pos << " " << text_restart;
            text_message=test.str();
            text_lines++;
            saved_lines.erase(saved_lines.begin()+i+1,saved_lines.end());
            continue;
        } */
        replay = c;
        break;
    }
}

void text_print() {
    fi (saved_lines.sz) 
        text_putchar(saved_lines[i]);
}

int capitalise(int c) {
    if ((c>='a' && c<='z')) return c-'a'+'A';
}

// return 1 and insert an indent if
// begin of line or whitespace before
// Or jump to current indent if in the middle of whites
int smart_space() {
    int i=text_gap;
    int pos=0;
    if (i==0 || text[i-1]==EOL || text[i-1]==' ') {
        while (i>0 && text[i-1] != EOL) {
            i--;
            pos++;
        }
        i = text_restart;
        while (i<text_end && text[i]==' ') {
            i++;
            pos++;
        }
        if (text_restart!=i)
            text_move(i);
        else do {
            text_putchar(' ');
            pos++;
        } while (pos % TABSTOP != 0);
        return 1;
    } 
    return 0;
}

void smart_backspace() {
    int i=text_gap;
    if (i>0 && text[i-1]==' ') {
        int pos = compute_pos();
        do {
            text_backspace();
            pos--;
            i--;
        } while (i>0 && text[i-1]==' ' && pos % TABSTOP !=0);
        return;
    } 
    if (i>0 && text[i-1]==EOL) {
        do {
            text_backspace();
            i--;
        } while (i>0 && text[i-1]==' ');
        return;
    }
    text_backspace();
}

void smart_delete() {
    int i=text_restart;
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
    if (i+1<text_end && text[i]==EOL) {
        do {
            text_delete();
            i++;
        } while (i+1<text_end && text[i]==' ');
        return;
    }
    text_delete();
}

void smart_enter() {
    int i=prev_eol();
    int pos=0;
    while (i<text_gap && text[i]==' ') {
        i++;
        pos++;
    }
    text_putchar(EOL);
    fj(pos) text_putchar(' ');
}

void open_line_after() {
    text_move(next_eol());
    smart_enter();
}

void open_line_before() {
    text_move(prev_eol());
    text_putchar(EOL);
    int pos=0;
    int i=text_restart;
    while (text[i++]==' ') pos++;
    text_move(text_gap-1);
    for (int j=0; j<pos; j++) text_putchar(' ');
}


/******************************************************************/
/******************************************************************/

int text_save() {
    swap_stream.flush();

    ofstream s;
    s.open(filename.c_str(),ios_base::trunc);
    fi (text_gap) s.put(text[i]);
    for (int i=text_restart; i<text_end; i++) s.put(text[i]);
    s.close();

    text_message="File saved.";
    text_saved=1;
}

int text_exit() {
    if (text_saved) return 1;
    text_message="The file is not saved. q:quit  s:save and quit";
    int c=text_getchar();
    if (c=='q'||c=='Q') return 1;
    if (c=='s'||c=='S') {
        text_save();
        return 1;
    }
    replay = c;
    return 0;
}

/******************************************************************/
// Completion
// todo : search in other direction when no match
/******************************************************************/

// dictionnary
// so we do not propose twice the same completion !
set<string> possibilities;

// first completion = last one with the same begin
// I think this is cool
map<string,string> last_completions;

// for completion ...
string search_comp(string c, int &i) 
{
    string res;
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

string text_complete() 
{
    undo_flush();
    possibilities.clear();
    int i=text_gap-1;
    string begin;
    string end;
    
    int white=0;
    if (i>=0 && !isletter(text[i])) white=1;

//  if previous white, do intelligent stuff
    while (i>=0 && !isletter(text[i]) && text[i]!=EOL) {
        begin = text[i] + begin;
        i--;
    }
    while (i>=0 && isletter(text[i])) {
        begin = text[i] + begin;
        i--;
    }
    
    if (i>=0 && text[i]==EOL) begin = EOL+begin;
    int pos=i;
    if (i<0) pos=i+1;

    char c=KEY_TAB;    
//  look first in the map
    if (last_completions.find(begin)!=last_completions.end()) {
        end = last_completions[begin];
        possibilities.insert(end);
        fi (end.sz) text_typechar(end[i]);
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
            if (end.sz==0) return "";
            possibilities.insert(end);
            fi (end.sz) text_typechar(end[i]);
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
        if (c==KEY_INSERT) {
            int c=text_getchar();
            if (c!=EOL) {
                text_typechar(c);
                inserted.pb(c);
                continue;
            }
        }
        if (isprint(c)) {
            if (c==' ' && smart_space()) {
                inserted.pb(EOL);
                break;
            }
            text_typechar(c);
            inserted.pb(c);
            continue;
        }
        if (c == KEY_BACKSPACE && !inserted.empty()) {
                text_backspace();
                inserted.erase(inserted.end()-1);
                continue;
        }
        if (c == KEY_TAB) {
            inserted+=text_complete();
            continue;
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

int text_jump(string s) {
    int num=0;
    fi (s.sz) {
        char c=s[i];
        if (c>='0' && c<='9') {
            num*=10;
            num+=c-'0';
        } else 
            return 0;
    }
    
    if (num>0) num = (num-1) % text_lines;
    int i=text_line_begin(num);
    text_move(i);
    return 1;
}

int text_search(string s) 
{
    int t = search_next(s,text_restart);
    if (t>0)  text_move(t);
    else {
        text_message = "search restarted on top.";
        t = search_next(s,0);
        if (t>0) text_move(t);
        else {
            text_message = "word not found.";
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
        text_message = "search restarted on bottom.";
        t = search_prev(s,text_end-2);
        if (t>=0) text_move(t);
        else {
            text_message = "word not found.";
            return 0;
        }
    }
    return 1;
}

void text_command(string prompt) {
    cmd.clear();
    char c;
    while (1) {
        text_message=prompt+cmd;
        c=text_getchar();
        if (isprint(c)) {
            cmd.pb(c);
            continue;
        }
        if (c==KEY_BACKSPACE) {
            while (!cmd.empty() && !isok(cmd[cmd.sz-1]))
                cmd.erase(cmd.end()-1);
            if (cmd.empty()) return;
            cmd.erase(cmd.end()-1);
            continue;
        }
        if (c==KEY_ENTER) {
//            if (cmd.empty()) {
//                // search word under cursor
//                return;
//            }
//            if (text_jump(cmd)) return;
//            if (text_search(cmd)) return;
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

string  macro_data="";
char    macro_next=KEY_NEXT;
int     macro_line=-1;

void macro_display() {
    SS yo;
    yo << macro_data << '#';
    yo << macro_next << '#';
    yo << macro_line;
    text_message = yo.str();
}

/******************************************************************/
/******************************************************************/

// pb: how to track the column ??
// we can do it in screen, if only screen commands, otherwise chg.
// But we need it as well for macro it seems ! or ^J & ^K just go 
// at the line begin ?? not conviced.
int screen_cmd() {
    char c;
    int num=0;
    while (1) {
        c=text_getchar();
        if (isnum(c)) {
            num = 10*num + c-'0';
        } else {
            break;
        }
    }
    switch(c) {
        case 'l' : text_move(text_gap-num);break;
        case 'r' : text_move(text_restart+num);break;
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
void text_ppage() {
    screen_ppage();
    line_goto(base_pos);
    macro_next=KEY_UP;
}
void text_npage() {
    screen_npage();
    line_goto(base_pos);
    macro_next=KEY_DOWN;
}

// only stuff that can be in an automatic macro.
// macro record everything, but cleared often...
int macro_loop() {
    start_record();
    int i=0;
    while (1) {
        i++;
        char c = text_getchar();
        if (isprint(c) || c==KEY_INSERT) 
            if (c!=' ' || !smart_space()) {
                replay=c;
                insert();
                continue;
            }
        if (c==KEY_BACKSPACE && text_gap>0 && text[text_gap-1]!=EOL) {
                smart_backspace();
                continue;
        }
        if (c==KEY_DELETE && text_restart<text_end && text[text_restart]!=EOL) {
                smart_delete();
                continue;
        }
        replay = c;
        break;
    }
    end_record();
    if (record_data.sz>1) {
        record_data.erase(record_data.sz-1);
        macro_data=record_data;
    }
    if (i>1) {
        macro_line=text_l;
        return 1;
    }
    return 0;
}

int macro_exec() {
    if (text_l == macro_line) {
        play_macro = macro_next;
        play_macro+= macro_data;
        play_macro+= KEY_SELECT; // un truc qui fait rien
    } else {
        play_macro  = macro_data;
        play_macro += macro_next;
    }
    macro_loop();
}

// REMOVED:
// jump to last pos w screen_save/screen_restore ...
// pb (nothing to do with main ...)
int mainloop() {
    char small=KEY_DOWN;
    int c=0;
    while (c!=KEY_QUIT) {
        int b=1;
        if (macro_loop()) {
            small=KEY_TILL;
            b=0;
        }
        c = text_getchar();
        switch (c) {
            case KEY_NULL: screen_cmd();break;
            case KEY_END: text_move(next_eol());break;
            case KEY_BEGIN: text_move(prev_eol());break;
            case KEY_LEFT: text_move(text_gap-1);break;
            case KEY_RIGHT: text_move(text_restart+1);break;
            case KEY_UP: text_up();b=0;break;
            case KEY_DOWN: text_down();b=0;break;
            case KEY_PPAGE: text_ppage();b=0;break;
            case KEY_NPAGE: text_npage();b=0;break;
            case KEY_GOTO: text_command(">");text_jump(cmd);break;
            case KEY_FIND: text_command("/");text_search(cmd);macro_next=KEY_NEXT;break;
            case KEY_WORD: search_id();macro_next=KEY_NEXT;break;
            case KEY_UNDO: text_undo();break;
            case KEY_SELECT: break;
            case KEY_DISP: macro_display();break;
            case KEY_NEXT: text_search(cmd);macro_next=KEY_NEXT;break;
            case KEY_PREV: text_search_back(cmd);macro_next=KEY_PREV;break;
            case KEY_TILL: macro_exec();b=0;break;
            case KEY_REDO: 
                if (!text_redo()) {
                    macro_exec();b=0;
                }
                break;
            case KEY_YLINE: yank_line();text_move(prev_eol());small=KEY_PRINT;break;
            case KEY_DLINE: replay=c;del_line();small=KEY_PRINT;break;
            case KEY_OLINE: open_line_before();small=' ';break;
            case KEY_JUSTIFY: justify();break;
            case KEY_PRINT: text_print();break;
            case KEY_TAB: replay=small;break;
            case KEY_SAVE: text_save();break;
            case KEY_ENTER: smart_enter();small=' ';break;
            case KEY_BACKSPACE: smart_backspace();break;
            case KEY_DELETE: smart_delete();break;
        }
        if (b) base_pos=compute_pos();
    }
}

// TODO
int macro_till() {
}

int main(int argc, char **argv) 
{
    if (argc<2) exit(1);
    filename = argv[1];

    /* basic init */
    text_init();

//  todo:
//  hash ??
//  get the file timestamp and make good use of it
//  resume editing to the current pos.
//  +-num line command option
    
    /* Open file */
    ifstream inputStream;
    inputStream.open(filename.c_str());
    if (!inputStream) {
        printf("Error openning file %s\n", argv[1]);
        exit(0);
    }

    /* Put file in gap buffer text */
    char c;
    int last=0;
    int temp=0;
    while (inputStream.get(c)) {
        last=c;
        if (c == '\t') {
            do {
                text_add(' ');
                temp++;
            } while ((temp%TABSTOP)!=0);
        } else if (c == EOL) {
            text_add(EOL);
            temp=0;
        } else {
            text_add(c);
            temp++;
        }
    }
    if (last!=EOL) text_add(EOL);
    text_saved=1;

    /* close file */
    inputStream.close();

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

    /* Set position in the text */
    text_move(oldpos);
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
        cout << "The file was not saved." << endl;
    cout.flush();

//    Debug info ...
//    undo_flush();
//    fi (undo_stack.sz) {
//        cout << undo_stack[i].num << " " << undo_stack[i].pos << endl;
//        cout << undo_stack[i].content << endl;
//    }
    exit(0);
}
