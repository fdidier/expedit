#include "definition.h"
#include "term.h"

string       filename;
int          text_saved;

string       text_highlight;
string       text_message;
vector<char> text;          // gap_buffer   
int          text_gap;      // cursor/gap absolute position
int          text_restart;  // end of the text until text.sz
int          text_end;      // number of char in the gap buffer
int          text_lines;    // total number of line in text
int          text_l;        // current line number
int          text_modif;    // indicateur de modif...
int          text_blocsize = 1024;

int          screen_p;  // position in the line
int          screen_l;  // line position on screen

        
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

int undo_number;
int record_undo;
int saved_pos;

void text_init() 
{
    saved_pos=-1;

    screen_p=0;
    screen_l=0;
    screen_lsize=10;

    text_gap=0;
    text_l=0;
    text_restart=0;
    text_modif=1;
    text_end=0;
    text_lines=0;
    record_undo=0;
    text_check(text_blocsize);
}   

void compute_screen_p() 
{
    screen_p = 0;
    int i=text_gap;
    while (i>0 && text[i-1] !=EOL ) {
        if (isok((uchar) text[i-1])) screen_p++;
        i--;
    }
}

void inc_screen_p(uchar c) 
{
    if (c==EOL) {
        screen_p=0;
        screen_l++;
    } else if (isok(c)) {
        screen_p++;
        if ((screen_p % screen_lsize) == 0) 
            screen_l++;
    }
}

void dec_screen_p(uchar c) 
{
    if (c==EOL) {
        compute_screen_p();
        screen_l--;
    } else if (isok(c)) {
        if ((screen_p % screen_lsize) == 0) 
            screen_l--;
        screen_p--;
    }
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
    string content;
};

vector<struct s_undo>    undo_stack;

void undo_flush() 
{
    if (saved_pos==-1) return;
    if (record_undo==1) 
    {
        if (text_gap == saved_pos) return;

        struct s_undo op;
        op.num = undo_number;
        if (saved_pos<text_gap) 
            op.pos = + text_gap;
        else 
            op.pos = - text_gap;

        int b=min(text_gap,saved_pos);
        int e=max(text_gap,saved_pos);

        while (b<e) {
            op.content.pb(text[b]);
            b++;
        }

        undo_stack.pb(op);
    }
    saved_pos=-1;
}

void text_putchar(int c) 
{
    edit_text();
    if (saved_pos>=text_gap || saved_pos==-1) {
        undo_flush();
        saved_pos=text_gap;
    }

    text_check(1);
    if (c==EOL) {
        text_l++;
        text_lines++;
    }
    text[text_gap++]=c;
    inc_screen_p(c);
}

// tab control n'est pas nécessairement
// un problème pour undo si tout se passe
// pareil quand on refait le truc ??
void text_backspace(int flag=1) 
{
    if (text_gap==0) return;
    
    edit_text();
    if (saved_pos<0) {
        saved_pos=text_gap;
    }

    do {
        text_gap--;
    } while (flag && text_gap>0 && !isok((uchar) text[text_gap])); 

    dec_screen_p(text[text_gap]);
    if (text[text_gap]==EOL) {
        text_l--;
        text_lines--;
    } 

    // respect indent on the rest of the line if necessary
//  if (flag)
//  if (text_restart+1<text.sz && 
//      text[text_restart]==' ' &&
//      text[text_restart+1]==' ') {
//          text_restart--;
//          text[text_restart]=' ';
//  }
}

void text_delete(int flag=1) 
{
    if (text_restart+1>=text.sz) return;

    edit_text();
    if (saved_pos<text_gap) {
        undo_flush();
        saved_pos=text_gap;
    }

    do {
        text[saved_pos]=text[text_restart];
        text_restart++;
        saved_pos++;
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
            dec_screen_p(text[text_restart]);
        }
    } else {
        /* utf8 correction up */
        while (i<text.sz && !isok((uchar) text[i])) i++;
        while (text_restart!=i) {
            inc_screen_p(text[text_restart]);
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

// pas beau + un bug somewhere...
void text_undo() {
    undo_flush();
    if (undo_stack.empty()) return;
    record_undo=0;
    struct s_undo op=undo_stack.back();
    int pos = abs(op.pos);
    if (pos<text_gap) {
        text_move(pos);
    } else {
        text_move(text_restart + pos-text_gap); 
    }   

    if (op.pos >0) {
        fi (op.content.sz) text_backspace(0);
    } else {
        fi (op.content.sz) text_putchar(op.content[i]);
    }
    undo_stack.pop_back();
    undo_flush();
    record_undo=1;
}

void text_typechar(int c) 
{
    text_putchar(c);
    
    // deal with indent
//  if (isok((uchar) c))
//  if (text_restart+1<text.sz && text[text_restart]==' ' && text[text_restart+1]==' ') 
//      text_delete();
}

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
//int mod_base_pos=0;

// goto a given pos in the current line.
void line_goto(int pos) {
    int i=text_line_begin(text_l);
    int p=0;
    while (p<pos && i<text.sz && text[i]!=EOL) {
        i++;
        if (i==text_gap) i=text_restart;
        // utf-8
        if (isok((uchar) text[i])) p++;
    }
    text_move(i);
}
void compute_enterpos() {
    //line_goto(base_pos);
}

/* Remember what was done on the last edited_line */

string  record_cmd;
int     record_pos;

string current_cmd;
int    current_pos;
int    current_line=-1;

void record_clear_trailing() {
    while (!current_cmd.empty()) {
        char c=current_cmd[current_cmd.sz-1];
        if  (c==KEY_LEFT || c==KEY_RIGHT)
            current_cmd.erase(current_cmd.end()-1);
        else 
            break;
    }
}

int clear_trailing() {
    int temp = current_cmd.sz;
    while (!current_cmd.empty()) {
        char c=current_cmd[current_cmd.sz-1];
        if  (c==KEY_LEFT || c==KEY_RIGHT)
            current_cmd.erase(current_cmd.end()-1);
        else 
            break;
    }
    if (current_cmd.sz == temp) return 0;
    return 1;
}

void record_end() {
    if (!current_cmd.empty()) {
        record_cmd = current_cmd;
        record_pos = current_pos;
    }
    current_cmd.clear();
    current_line=-1;
}

void record_display() {
    text_message = record_cmd;
    text_message.pb('#');
    text_message += current_cmd;
    text_message.pb('#');
    text_message.pb(EOL);
}

void record(char c) 
{
    if (current_line != text_l) {
        if (!current_cmd.empty()) {
            if (!clear_trailing()) 
                current_cmd.pb(c);
            record_end();
        }
        current_line = text_l;
        current_pos = screen_p;
        return;
    }
    if (c==KEY_TAB || 
       (current_cmd.empty() && (c == KEY_LEFT || c==KEY_RIGHT ))) {
        return;
    }
    current_cmd.pb(c);
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
 * We take care of utf-8 for the actual cursor position.
 * This is the only place after witch screen_p has is true meaning.
 */
void text_refresh() 
{
//    if (mod_base_pos) base_pos = screen_p;
    screen_refresh();
    text_message.clear();
}

string play_macro;
int replay=0;
int pending=0;
int last_ch=0;
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
    // only one keystroke 
    undo_number++;
    
    // save last_char
    record(last_ch);

    /* save the typed char and return it */
    last_ch=res;
    swap_stream.put(res);
    swap_index++;
    return res;
}

/*******************************************************************/
/* implementation of the commands 
 */
 
#define CMD_FALLBACK    0
#define CMD_EDIT        1
#define CMD_LINE        2
#define CMD_SEARCH      3
#define CMD_NEWLINE     4

int    last_command;
int    current_command;

string inserted;
string saved_lines;

string last_line_commands;
// tab on the same line, strip las command group
// otherwise execute all.

void yank_line() {
    saved_lines.clear();
    current_command=CMD_LINE;
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
    current_command=CMD_LINE;
    while (1) {
        int c = text_getchar();
        // delete whole line
        if (c==KEY_DLINE) {
            // no more line ??
            if (text_lines==0) return;
            
            // move to line begin
            text_move(text_line_begin(text_l));

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

            line_goto(base_pos);
            continue;
        }
        // small redo
//      if (c==KEY_BACKSPACE && !saved_lines.empty()) {
//          text_move(text_line_begin(text_l));
//          int begin=saved_lines.sz-1;
//          while (begin>0 && saved_lines[begin-1]!=EOL) begin--;
//          for (int i=begin; i<saved_lines.sz; i++) 
//              text_putchar(saved_lines[i]);
//          saved_lines.erase(saved_lines.begin()+begin,saved_lines.end());
//          
//          text_move(text_line_begin(text_l-1));
//          compute_enterpos();
//          continue;
//      }
        replay = c;
        break;
    }
}

int put_line_after() {
    // pb on last line !!
    text_move(text_line_begin(text_l+1));
    fi (saved_lines.sz) text_putchar(saved_lines[i]);
    text_move(text_line_begin(text_l-1));
    compute_enterpos();
}

int put_line_before() {
    text_move(text_line_begin(text_l));
    //int saved=text_gap;
    fi (saved_lines.sz) text_putchar(saved_lines[i]);
    //text_move(saved);
    compute_enterpos();
}

int isletter(int c) {
    return ((c>='a'&&c<='z') || (c>='A'&& c<='Z'));
}

int capitalise(int c) {
    if ((c>='a' && c<='z')) return c-'a'+'A';
}

// smart space :
// return 1 and insert an indent if
// begin of line or whitespace before
int text_spacetab() {
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
        text_move(i);
        do {
            text_putchar(' ');
            pos++;
        } while (pos % TABSTOP != 0);
        return 1;
    } 
    return 0;
}

// remove full indent when necessary
// todo: smart if lot of space in the middle ??
void smart_backspace() {
    int i=text_gap;
    int pos=0;
    while (i>0 && text[i-1] ==' ') {
        i--;
        pos++;
    }
    if (pos>0) if (i==0 || text[i-1]==EOL) {
        do {
            text_backspace();
            pos--;
        } while (pos % TABSTOP !=0);
        return;
    } 
    text_backspace();
}

// remove indent when necessary
void smart_delete() {
}

void smart_enter() {
    current_command=CMD_NEWLINE;
    int i=text_line_begin(text_l);
    int pos=0;
    while (i<text_gap && text[i]==' ') {
        i++;
        pos++;
    }
    text_putchar(EOL);
    fj(pos) text_putchar(' ');
}

void text_beginline() {
    int begin=text_line_begin(text_l);
    text_move(begin);
//    int i=begin;
//    while (i<text_gap && text[i]==' ') i++;
//    if (i>=text_gap) {
//        i = text_restart;
//        while (i<text.sz && text[i]==' ') i++;
//    }
//    text_move(i);
//
//    int c = text_getchar();
//    if (c==KEY_BEGIN) text_move(begin);
//    else {
//        replay = c;
//        return;
//    }
//
//    c = text_getchar();
//    if (c==KEY_BEGIN) text_move(0);
//    else replay=c;
}
 
void text_endline() {
    int i=text_restart;
    while (i<text.sz && text[i]!=EOL) i++;
    text_move(i);
//    int c = text_getchar();
//    if (c==KEY_END) text_move(text.sz-1);
//    else replay=c;
}

void smart_enter2() {
    current_command=CMD_NEWLINE;
    int i=text_line_begin(text_l);
    int pos=0;
    while (i<text_gap && text[i]==' ') {
        i++;
        pos++;
    }
    if (i>=text_gap) {
        i = text_restart;
        while (i<text.sz && text[i]==' ') {
            i++;
            pos++;
        }
    }
    
    i=text_restart;
    while (i<text.sz && text[i]!=EOL) i++;
    text_move(i);
    text_putchar(EOL);
    fj(pos) text_putchar(' ');
}

// blank line at the end, open before.
void open_line() {
    current_command=CMD_NEWLINE;
    text_move(text_line_begin(text_l));
    text_putchar(EOL);
    int pos=0;
    int i=text_restart;
    while (text[i++]==' ') pos++;
    text_move(text_line_begin(text_l-1));
    for (int j=0; j<pos; j++) text_putchar(' ');
}


/******************************************************************/
/******************************************************************/

int text_save() {
    /* flush swap file */
    swap_stream.flush();

    ofstream s;
    s.open(filename.c_str(),ios_base::trunc);
    fi (text_gap) s.put(text[i]);
    for (int i=text_restart; i<text_end; i++) s.put(text[i]);
    s.close();

    text_message="File saved.\n";
    text_saved=1;
}

int text_exit() {
    if (text_saved) return 1;
    text_message="The file is not saved. q:quit  s:save and quit\n";
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

int isletter(char c) {
    if (c>='a' && c<='z') return 1;
    if (c>='A' && c<='Z') return 1;
    if (c=='_') return 1;
    if (c<0) return 1;
    return 0;
}

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
    possibilities.clear();
    int i=text_gap-1;
    string begin;
    string end;

    // if previous white, do intelligent stuff
    while (i>0 && !isletter(text[i]) && text[i]!=EOL) {
        begin = text[i] + begin;
        i--;
    }
    while (i>0 && isletter(text[i])) {
        begin = text[i] + begin;
        i--;
    }
    int  pos=i;
    char c=KEY_TAB;
    
    // look first in the map
    if (last_completions.find(begin)!=last_completions.end()) {
        end = last_completions[begin];
        possibilities.insert(end);
        fi (end.sz) text_typechar(end[i]);
        c=text_getchar();
    }
    
    // No match or user not happy,
    // look in the text
    while (c==KEY_TAB) {
        if (end.sz>0) {
            fi (end.sz) 
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
    
    if (c==KEY_ESC) {
        text_gap-=end.sz;
    } else {
        replay=c;
    }
    
    if (!end.empty()) 
        last_completions[begin]=end;
    
    return end;
}

int insert() {
   // mod_base_pos=0;
    inserted.clear();
    while (1) {
        int c = text_getchar();
        if (isprint(c)) {
            if (c==' ' && text_spacetab()) {
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
//              if (inserted.empty()) {
//                  last_command=CMD_FALLBACK;
//                  return 1;
//              }
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

// no debug yet
int search_previous(string c, int i) 
{
    int j=c.sz-1;
    if (c.sz == 0) return -1; 
    if (i>text_end) return -1;
    if (i==(text_end-1) && c[j]==' ') j--; 
    while (i>0) {
        if (i>=text_gap && i<text_restart) 
            i=text_gap;
        if (text[i] == c[j] || (c[j]==' ' && !isletter(text[i]))) { 
            j--;
            if (j<0) 
                return i+1;
        } else {
            i = i+(c.sz-1-j);
            j = c.sz-1;
        }
        i--;
    }
    return -1;
}

int search_next(string c, int i) 
{
    int j=0;
    if (c.sz ==0) return -1;
    if (i<0) return -1;
    if (i==0 && c[0]==' ') j=1;
    while (i<text_end) {
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
    compute_enterpos();
    return 1;
}

int text_search(string s) 
{
    current_command=CMD_SEARCH;
    int t = search_next(s,text_restart);
    if (t>0)  text_move(t);
    else {
        text_message = "search restarted on top.\n";
        t = search_next(s,0);
        if (t>0) text_move(t);
        else text_message = "word not found.\n";
    }
    return 1;
}

void text_command() {
    cmd.clear();
    char c;
    while (1) {
        text_message=':'+cmd+'\n';
        c=text_getchar();
        if (isprint(c)) {
            cmd.pb(c);
            continue;
        }
        if (c==KEY_BACKSPACE) {
            if (cmd.empty()) return;
            cmd.erase(cmd.end()-1);
            continue;
        }
        if (c==KEY_ENTER) {
            if (cmd.empty()) {
                // search word under cursor
                return;
            }
            if (text_jump(cmd)) return;
            if (text_search(cmd)) return;
            return;
        }
        replay = c;
        break;
    }
}

/******************************************************************/
/******************************************************************/

void smart_op() 
{
    current_command=last_command;
    if (last_command==CMD_NEWLINE) {
        text_spacetab();
        return;
    }
    if (last_command==CMD_LINE) {
        put_line_before();
        return;
    }
    if (last_command==CMD_SEARCH) {
        text_search(cmd);
        // Redo whole line op
        return;
    }

    // Redo whole line op
    // line_goto(record_pos); mieux sans si goto line implicite
    if (!current_cmd.empty()) {
        record_clear_trailing();
        record_end();
    }
    play_macro=record_cmd;
}

/******************************************************************/
/******************************************************************/

// if yank/del lines/line op
// repeat unitl current line
// if still on the same line and line del : repeat del until
// current position...
int smart_repeat() {
}

/******************************************************************/
/******************************************************************/

void keyup() {
    if(text_l>0) 
        text_move(text_line_begin(text_l-1));
    line_goto(base_pos);
}

void keydown() {
    if(text_l+1<text_lines) 
        text_move(text_line_begin(text_l+1));
    line_goto(base_pos);
}

void ppage() 
{
    int temp=0;
    if (screen_real_i==1) {
        if (text_l<=1) return;
        temp=text_l - int(screen_lines)+1;
        temp=max(temp,1);
    } else {
        temp=text_l-int(screen_real_i)+1;
    }
    text_move(text_line_begin(temp));
    compute_enterpos();
}

void npage() 
{
    int temp=0;
    if (screen_real_i==screen_lines-2) {
        if (text_l>=text_end-1) return;
        temp=text_l+int(screen_lines)-1;
        temp=min(temp,text_lines-1);
    } else {
        temp=text_l-int(screen_real_i)+int(screen_lines)-2;
    }
    text_move(text_line_begin(temp));
    compute_enterpos();
}

int move(char c) {
    switch(c) {
        case KEY_END: text_endline();base_pos=screen_p;break;
        case KEY_BEGIN: text_beginline();base_pos=screen_p;break;
        case KEY_LEFT: text_move(text_gap-1);base_pos=screen_p;break;
        case KEY_RIGHT: text_move(text_restart+1);base_pos=screen_p;break;
        case KEY_UP: keyup();break;
        case KEY_DOWN: keydown();break;
        case KEY_PPAGE: ppage();break; 
        case KEY_NPAGE: npage();break;
        default:
            return 0;
    }
    return 1;
}

int mainloop() {
    int movement=0;
    int saved_mark=-1;
    last_command = CMD_FALLBACK;
    int c='q';
    while (1) {
        current_command = CMD_FALLBACK;
        c = text_getchar();
        if (move(c)) {
            movement = 1;
            current_command = last_command;
        } else {
            switch (c) {
                case ':' : text_command();break;
                case KEY_ESC :
                case '`' :  if (movement && saved_mark>=0) {
                                text_absolute_move(saved_mark);
                                screen_restore();
                                movement = 0;
                            } else {
                                text_undo();
                            }
                            break;
                case '1' : record_display();break;
                case KEY_UNDO: text_undo();break;
                case KEY_OLINE: open_line();break;
                case KEY_SLINE: smart_enter2();break;
                case KEY_DLINE: replay=c;del_line(); record_end();break;
                case KEY_YLINE: yank_line();break;
                case KEY_INSERT: put_line_before();break;
                case KEY_PLINE: put_line_after();break;
                case KEY_TAB: smart_op();break;
                case KEY_QUIT  : if (text_exit()) return 0; record_end();break;
                case KEY_SAVE  : text_save();record_end();break;
                case KEY_ENTER: smart_enter();break;
                case KEY_BACKSPACE: smart_backspace();break;
                case KEY_DELETE: text_delete();break;
                default:  
                    if (isprint(c)) {
                        if (c==' ' && text_spacetab()) break;
                        replay=c;
                        insert();
                        break;
                    }
            }        
        
            movement=0;
            saved_mark=text_gap;
            screen_save();
        }
        last_command = current_command;
    }
}

int main(int argc, char **argv) 
{
    if (argc<2) exit(1);
    filename = argv[1];

    /* basic init */
    text_init();

    // todo:
    // get the file timestamp and make good use of it
    // resume editing to the current pos.
    // +-num line command option
    
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
                text_putchar(' ');
                temp++;
            } while ((temp%TABSTOP)!=0);
        } else if (c == EOL) {
            text_putchar(EOL);
            temp=0;
        } else {
            text_putchar(c);
            temp++;
        }
    }
    if (last!=EOL) text_putchar(EOL);
    text_saved=1;

    /* close file */
    inputStream.close();

    // Do a backup of the original file
    // not really useful if we keep the same swap file for all the editing session
    // and we use a versionning system
    // But safer for all the development phase, and we need a starting point for the 
    // swap reconstruction to work ...
    swap_filename = "." + string(argv[1]) + string(".old");
    swap_stream.open(swap_filename.c_str());
    if (!swap_stream) {
        printf("Error openning backup file %s\n", swap_filename.c_str());
        exit(0);
    }
    
    // the text_gap is at the end of the file
    // after the loading process ...
    fi (text_gap) swap_stream.put(text[i]);
    swap_stream.flush();
    swap_stream.close();

    // Open swap file 
    // problem with directory ...
    swap_index = 0;
    swap_filename = "." + string(argv[1]) + string(".swp"); 
    swap_stream.open(swap_filename.c_str());
    if (!swap_stream) {
        printf("Error openning swap file %s\n", swap_filename.c_str());
        exit(0);
    }

    /* Set position in the text */
    text_move(0);
    screen_init();
    screen_redraw();
    term_set_title((uchar *)argv[1]);

    record_undo=1;
    mainloop();

    // output current position to be able
    // to restore it the next time we open the file
    swap_stream.put('\t');
    swap_stream << text_gap;
    swap_stream.put(EOL);
    swap_stream.flush();
    swap_stream.close();


    undo_flush();
    fi (undo_stack.sz) {
        cout << undo_stack[i].num << " " << undo_stack[i].pos << endl;
        cout << undo_stack[i].content << EOL;
    }
    exit(0);
}
