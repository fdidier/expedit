#include "definition.h"
#include "term.h"

/************************************************************************/
/* Screen Handling                                                      */
/************************************************************************/

/* screen dimension */
uint     screen_lines;
uint     screen_columns;

/* screen currently displayed */
uint     **screen_real;
uint     **color_real;
uint     screen_real_i;      /* cursor line in screen */
uint     screen_real_j;      /* cursor pos  in screen */

int      cursor_hiden=0;

/* screen we want to display */
uint     **screen_wanted;
uint     **color_wanted;
uint     screen_wanted_i;
uint     screen_wanted_j;

uint     first_line;

/* Other variables */
int screen_scroll_hint;

/************************************************************************/
/* screen_real and screen_wanted allocation functions                   */
/************************************************************************/

uint     **screen_alloc_internal(int i, int j)
{
        uint *temp;
        uint **array;
        int k;

        temp  = (uint *)  malloc(i*j*sizeof(int));
        array = (uint **) malloc(i*sizeof(int *));

        for (k=0; k<i; k++) {
                array[k]= &temp[k*j];
                for (int l=0; l<j; l++) array[k][l]=0;
        }

        return array;
}

void    screen_alloc()
{
        if (screen_wanted) {
                free(screen_wanted[0]);
                free(screen_wanted);
        }
        if (color_wanted) {
                free(color_wanted[0]);
                free(color_wanted);
        }

        if (screen_real) {
                free(screen_real[0]);
                free(screen_real);
        }
        if (color_real) {
                free(color_real[0]);
                free(color_real);
        }

        screen_wanted = screen_alloc_internal(screen_lines, screen_columns+1);
        color_wanted = screen_alloc_internal(screen_lines, screen_columns+1);
        screen_real = screen_alloc_internal(screen_lines, screen_columns+1);
        color_real = screen_alloc_internal(screen_lines, screen_columns+1);
}

/************************************************************************/
/* Function that change the real screen                                 */
/************************************************************************/

/* Clear the real screen */
void screen_clear()
{
        CLEAR_ALL;

        for (uint i=0; i<screen_lines; i++) {
                screen_real[i][0]='\n';
        }

        HOME;
        screen_real_i=0;
        screen_real_j=0;
}

/* Change the real cursor position if necessary */
void screen_move_curs(int i, int j)
{
    if (screen_real_j == screen_columns+1) {
        screen_real_j = 0;
        screen_real_i ++;
    }

    if (i<0 || i>=screen_lines) {
        if (!cursor_hiden) {
            HIDE_CURSOR;
            cursor_hiden=1;
        }
    } else {
        if (cursor_hiden) {
            SHOW_CURSOR;
            cursor_hiden=0;
        }
    }


    if ((screen_real_i != i) || (screen_real_j != j)) {
        MOVE(i, j);
        screen_real_i = i;
        screen_real_j = j;
    }
}

/* Print new line */
void screen_newl()
{
    printf("\n\r");
    screen_real_i ++;
    if (screen_real_i == screen_lines)
        screen_real_i--;
    screen_real_j=0;
}

/* Print a char */
void screen_print(int c, int color=0)
{
    term_putchar(c, color);
    screen_real_j++;
    if (screen_real_j == screen_columns+1) {
        screen_real_j = 0;
        screen_real_i ++;
    }
}

/* Dump a part of screen wanted on a cleared real screen */
void screen_dump_wanted(int a, int b)
{
        int w,i,j;

        screen_move_curs(screen_real_i, 0);

        for (i=a; i<b; i++) {
                j=0;
                while (1) {
                        w = screen_wanted[i][j];
                        if (w == '\n') {
                                if (i != (b-1)) screen_newl();
                                break;
                        }

                        screen_print(w,color_wanted[i][j]);
                        j++;
                }
        }
}

/* Make the real screen look like the wanted one */
void screen_make_it_real()
{
        uint i;
        uint w,r;

        /* Use the scroll hint */
        uint a   = 0;
        uint b   = 0;
        uint off = 0;

        if (screen_scroll_hint>0) {
                if (screen_scroll_hint>= (int) screen_lines) {
                        screen_clear();
                        screen_dump_wanted(0,screen_lines);
                        return;
                }
                a   =(uint) screen_scroll_hint;
                off =(uint) screen_scroll_hint;
        }

        if (screen_scroll_hint<0) {
                a = b = (uint) - screen_scroll_hint;
                if (a >=  screen_lines) {
                        screen_clear();
                        screen_dump_wanted(0,screen_lines);
                        return;
                }

                screen_move_curs(0,0);
                SCROLL_DOWN(a);
                screen_move_curs(0,0);
                screen_dump_wanted(0,a);
        }

        for (i=a; i<screen_lines; i++) {
                r=0;
                fj (screen_columns) {
                        w = screen_wanted[i-off][j];
                        if (r != '\n') r = screen_real[i-b][j];

                        if (w == '\n') {
                                if (r != '\n') {
                                        screen_move_curs(i,j);
                                        CLEAR_EOL;
                                        if (i != (screen_lines - 1))
                                            screen_newl();
                                }
                                break;
                        }

                        if ( (w!=r) || (color_wanted[i-off][j] != color_real[i-b][j])) {
                            screen_move_curs(i,j);
                            screen_print(w,color_wanted[i-off][j]);
                        }
                }
        }

        if (off) {
                if (screen_real_i != screen_lines-1)
                    screen_move_curs(screen_lines-1,0);
                screen_newl();
                screen_dump_wanted(screen_lines-off,screen_lines);
        }
}

// Function to call when the real screen is equal to the wanted one
void screen_done()
{
    // put the cursor as wanted
    screen_move_curs(screen_wanted_i, screen_wanted_j);
    screen_scroll_hint = 0;

    // swap the two screen array to update screen real
    uint **temp = screen_real;
    screen_real = screen_wanted;
    screen_wanted = temp;

    // swap the two color arrays
    temp = color_real;
    color_real = color_wanted;
    color_wanted = temp;

    // make the change
    fflush(stdout);
}


/*********************************************************************************/
/* Specific functions for displaying a text structure                            */
/*********************************************************************************/

// if there is line number
int shift=0;
int opt_line=1;

int saved_first_line;

void screen_set_first_line(int hint)
{
    if (hint<0) hint=0;
    if (hint >= text_lines) hint=text_lines-1;
    first_line = hint;
}

// Compute the first line displayed according to
// cursor movement since last time
void screen_movement_hint()
{
    // current line if we use the same first line
    int temp = int(text_l)-int(first_line);

    if (temp<=0) {
        if (text_l==0)
            first_line=0;
        else
            first_line=text_l-1;
    }

    int top = screen_lines-2;
    if (temp>top) {
        first_line = int(text_l) - top;
    }
}

// Compute scroll hint to speed up display
void compute_scroll_hint()
{
    screen_scroll_hint = first_line - saved_first_line;
    saved_first_line = first_line;
}


// compute cursor position according to the first line
void compute_cursor_pos()
{
    // compute screen_wanted_i
    screen_wanted_i = text_l - first_line;

    // compute screen_wanted_j
    int l = text_gap - text_line_begin(text_l);
    screen_wanted_j = shift + min(l, int(screen_columns-shift-1));
}

// compute the wanted view of the screen
void screen_compute_wanted()
{
    // line numbers
    if (opt_line) {
            int num = text_lines;
            shift=1;
            while (num) {
                shift++;
                num/=10;
            }
            shift=max(4,shift);
    } else {
        shift=0;
    }

    compute_cursor_pos();

    // fill the line one by one
    fn (screen_lines)
    {
        // pos on screen
        int j=0;

        // start by the line number
        int num = first_line + n + 1;

        // display only if lines exist
        if (num <= text_lines)
        {
            while (j<shift) {
                if (num==0 || j==0) {
                    screen_wanted[n][shift-1-j]= ' ';
                } else {
                    screen_wanted[n][shift-1-j]= (num%10) +'0';
                    num/=10;
                }
                j++;
            }

            // beginning of the current line
            int i = text_line_begin(first_line + n);
            if (i==text_gap) i=text_restart;

            // special case if line is bigger than screen width
            // only for text_l
            if (first_line + n == text_l && text_gap>i ) {
                int pos = text_gap - i;
                if (pos  >= screen_columns - shift )
                    i = text_gap - (screen_columns - shift-1);
            }

            // display the line
            while (i<text_end && text[i]!=EOL && j<screen_columns) {
                screen_wanted[n][j] = text[i];
                j++;
                i++;
                if (i==text_gap) i=text_restart;
            }
        }

        // fill the end with white
        screen_wanted[n][j]=EOL;
        while (j+1<screen_columns) {
            screen_wanted[n][j+1]=' ';
            j++;
        }
    }
}

// **************************************************************************
// **************************************************************************

void display_message()
{
    if (display_pattern) {
        int j=0;
        while (j<pattern.sz && j<screen_columns) {
            if (pattern[j]==EOL) pattern[j]='J';
            screen_wanted[screen_lines-1][j] = pattern[j];
            color_wanted[screen_lines-1][j] = DEFAULT_COLOR | REVERSE;
            j++;
        }
        screen_wanted[screen_lines-1][j]=EOL;
        color_wanted[screen_lines-1][j]=0;
    }

    if (text_message.empty()) return;

//    // clear end of line
//    int j=0;
//    while (j<screen_columns && screen_wanted[screen_lines-1][j]!=EOL) j++;
//    while (j<screen_columns) {
//        screen_wanted[screen_lines-1][j]=' ';
//        color_wanted[screen_lines-1][j]=0;
//        j++;
//    }
//
//    // replace by message
//    j=0;
//    int n=text_message.sz-1;
//    screen_wanted[screen_lines-1][screen_columns]=EOL;
//    color_wanted[screen_lines-1][screen_columns]=0;
//    while (j<text_message.sz && j<screen_columns) {
//        if (text_message[n-j]==EOL) text_message[n-j]='J';
//        screen_wanted[screen_lines-1][screen_columns-1-j] = text_message[n-j];
//        color_wanted[screen_lines-1][screen_columns-1-j] = BLACK | REVERSE;
//        j++;
//    }
//    if (j<screen_columns) {
//        screen_wanted[screen_lines-1][screen_columns-1-j]=' ';
//        color_wanted[screen_lines-1][screen_columns-1-j]=0;
//    }

    // begin of line version
    int j=0;
    while (j<text_message.sz && j<screen_columns) {
        if (text_message[j]==EOL) text_message[j]='J';
        screen_wanted[screen_lines-1][j] = text_message[j];
        color_wanted[screen_lines-1][j] = DEFAULT_COLOR | REVERSE;
        j++;
    }
    screen_wanted[screen_lines-1][j]=EOL;
    color_wanted[screen_lines-1][j]=0;
}

// ***********************************************************
// ***********************************************************
// ***********************************************************


void highlight_clear()
{
    fi (screen_lines)
        fj (screen_columns)
            if (j<shift) {
                color_wanted[i][j]=YELLOW;
            } else {
                color_wanted[i][j]=DEFAULT_COLOR;
            }
}

void highlight_comment()
{
    // comment
    int bloc=0;
    int line=0;
    fi(screen_lines) {
        int color=0;
        int first=1;
        for (int j=shift; j<screen_columns; j++)
        {
            if (screen_wanted[i][j]==EOL) break;

            char c=screen_wanted[i][j];
            if (first) {
                if (c==' ') continue;
                if (c=='/') color = BLUE;
                if (c=='%') color = BLUE;
                if (c=='#') color = MAGENTA;
                first=0;
            }
            if (color) color_wanted[i][j] = color;
        }
    }
}

void highlight_header()
{
    fi(screen_lines) {
        if (isletter(screen_wanted[i][shift]))
        fj(screen_columns) {
            if (j<shift) {
                continue;
            }
            if (screen_wanted[i][j]==EOL) break;
            color_wanted[i][j] = RED;
        }
    }
}

void highlight_maj()
{
    fi(screen_lines) {
        for (int j=shift; j<screen_columns; j++) {
            if (isbig(screen_wanted[i][j]) &&
               (j==shift || !isletter(screen_wanted[i][j-1]))) {
                    do {
                        color_wanted[i][j] = MAGENTA;
                        j++;
                    } while (j<screen_columns && isletter(screen_wanted[i][j]));
            }
        }
    }
}

void highlight_number()
{
    fi(screen_lines)
        for (int j=shift; j<screen_columns; j++)
            if (isnum(screen_wanted[i][j]))
                color_wanted[i][j] = RED;
}

void highlight_bracket()
{
    if (screen_wanted_i<0 || screen_wanted_i>=screen_lines) return;
    if (screen_wanted_j<0 || screen_wanted_j>=screen_columns) return;

    int c;
    int temp=screen_wanted_j;

    do {
        c = screen_wanted[screen_wanted_i][temp];
        if (c=='(' || c==')' || c=='{' || c=='}' || c=='[' || c==']') break;
        temp--;
    } while (temp>=0);

    if (temp>=0) {
        if (temp!=screen_wanted_j)
            color_wanted[screen_wanted_i][temp]= RED;

        if (c=='{' || c=='[' || c=='(') {
            char b=c;
            char e;
            if (b=='{') e='}';
            if (b=='[') e=']';
            if (b=='(') e=')';
            int count=0;
            int i=screen_wanted_i;
            int j=temp;
            for(;i<screen_lines;i++) {
                for(;j<screen_columns;j++) {
                    if (screen_wanted[i][j]==b) count++;
                    if (screen_wanted[i][j]==e) count--;
                    if (count == 0) {
                        color_wanted[i][j] = RED;
                        i=screen_lines;
                        j=screen_columns;
                    }
                }
                j=0;
            }
        }
        if (c=='}' || c==']' || c==')') {
            char b=c;
            char e;
            if (b=='}') e='{';
            if (b==']') e='[';
            if (b==')') e='(';
            int count=0;
            int i=screen_wanted_i;
            int j=temp;
            for(;i>=0;i--) {
                for(;j>=0;j--) {
                    if (screen_wanted[i][j]==b) count++;
                    if (screen_wanted[i][j]==e) count--;
                    if (count == 0) {
                        color_wanted[i][j] = RED;
                        i=-1;
                        j=-1;
                    }
                }
                j=screen_columns-1;
            }
        }
    }
}

map<string,int> keyword;
void highlight_keywords()
{
    fi (screen_lines) {
        int j=0;
        int s=0;
        while (j<screen_columns) {
            if (screen_wanted[i][j]=='\n') break;
            while (j<screen_columns && !isletter(screen_wanted[i][j])) j++;

            string word;
            s=j;
            while (j<screen_columns && isletter(screen_wanted[i][j])) {
                word.pb(screen_wanted[i][j]);
                j++;
            }
            if (keyword.find(word) != keyword.end()) {
                int color = keyword[word];
                while (s<j) {
                    if (color_wanted[i][s] != MAGENTA)
                        color_wanted[i][s] = color;
                    s++;
                }
            }
        }
    }
}

void highlight_search()
{
    if (search_highlight==0)
        return;

    int size = pattern.sz;
    if (size==0) return;

    if (pattern[0]==' ') size--;
    if (pattern[pattern.sz-1]==' ') size--;

    fi (screen_lines) {
        int begin = text_line_begin(first_line+i);
        if (begin==text_gap) begin = text_restart;

        for (int j=shift; j<screen_columns; j++) {

            int pos = begin + j-shift;
            if (begin<=text_gap) {
                if (pos>=text_gap) {
                    pos += text_restart-text_gap;
                }
            }

            if (match(pattern,pos)) {
                fk (size) {
                    color_wanted[i][j]|=REVERSE;
                    j++;
                    if (j>= screen_columns) break;
                }
            }

        }
    }
}

void screen_highlight()
{
    highlight_clear();
    highlight_bracket();
//    highlight_maj();
    highlight_number();
//    highlight_header();
    highlight_keywords();
    highlight_search();
    highlight_comment();
}


/********************************************************************************/
/* Function called on a screen resize event                                     */
/********************************************************************************/

void screen_sigwinch_handler(int sig)
{
        term_get_size(screen_lines, screen_columns);
        screen_alloc();
        screen_redraw();
}

/********************************************************************************/
/* Interface Functions                                                          */
/********************************************************************************/

void screen_init()
{
        keyword["if"]=YELLOW;
        keyword["fi"]=YELLOW;
        keyword["fj"]=YELLOW;
        keyword["else"]=YELLOW;
        keyword["return"]=YELLOW;
        keyword["break"]=YELLOW;
        keyword["continue"]=YELLOW;
        keyword["switch"]=YELLOW;
        keyword["default"]=YELLOW;
        keyword["case"]=YELLOW;
        keyword["while"]=YELLOW;
        keyword["do"]=YELLOW;
        keyword["for"]=YELLOW;
        keyword["struct"]=YELLOW;
        keyword["sizeof"]=YELLOW;
        keyword["new"]=YELLOW;
        keyword["delete"]=YELLOW;

        keyword["unsigned"]=GREEN;
        keyword["short"]=GREEN;
        keyword["int"]=GREEN;
        keyword["uint"]=GREEN;
        keyword["char"]=GREEN;
        keyword["string"]=GREEN;
        keyword["double"]=GREEN;
        keyword["void"]=GREEN;

        /* Initialize the terminal */
        term_init();

        /* SIGWINCH signal handling */
        (void) signal(SIGWINCH, screen_sigwinch_handler);

        /* Get terminal dimension */
        term_get_size(screen_lines, screen_columns);

        /* Allocating the screen array */
        screen_alloc();

        /* clear real screen */
        screen_clear();
}

void screen_redraw()
{
        screen_compute_wanted();
        screen_highlight();
        display_message();
        highlight_search();

        term_reset();
        screen_clear();
        screen_dump_wanted(0,screen_lines);
        screen_done();
}

void screen_refresh()
{
        compute_scroll_hint();
        screen_compute_wanted();
        screen_highlight();
        display_message();
        highlight_search();
        screen_make_it_real();
        screen_done();
}

int saved_line;

void screen_save()
{
    saved_line=first_line;
}

void screen_restore()
{
    screen_set_first_line(saved_line);
    screen_redraw();
}


void screen_ppage() {
    int dest = text_l - screen_lines/2;
    if (dest < 0) return;
    text_move(text_line_begin(dest));
    screen_set_first_line(dest - screen_lines/2);
    screen_refresh();
}

void screen_npage() {
    int dest = text_l + screen_lines/2;
    if (dest >= text_lines) return;
    text_move(text_line_begin(dest));
    screen_set_first_line(dest - screen_lines/2);
    screen_refresh();
}

void screen_ol() {
    if (opt_line) opt_line=0;
    else opt_line=1;
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
// on selection
// - left click inside, paste selection at cursor
// - right click inside, del selection

int mouse_highlight(int l1, int p1, int l2, int p2, int mode) {
    if (l1>l2) {
        swap(l1,l2);
        swap(p1,p2);
    }
    if (l1==l2 && p1>p2) {
        swap(p1,p2);
    }

    int i=l1-first_line;
    if (i<0) {
        i=0;
        p1=0;
    }

    int imax = l2-first_line;
    if (imax>= int(screen_lines))
        imax = screen_lines-1;

    int j= max(p1 + shift,shift);
    int jmax = p2 + shift;
    int end=0;

    if (mode) {
        j = shift;
        jmax = screen_columns;
    }

    while (i<imax || (i==imax && j <= jmax) ) {
        if (screen_wanted[i][j]==EOL) {
            end=1;
            if (i==imax) jmax = screen_columns;
            screen_wanted[i][screen_columns]=EOL;
        }
        color_wanted[i][j] = DEFAULT_COLOR | REVERSE;
        if (end) {
            screen_wanted[i][j]=' ';
        }
        j++;
        if (j==screen_columns) {
            j=shift;
            end=0;
            i++;
        }
    }
}

int inside(int mi_l,int mi_p,int ma_l,int ma_p,int l,int p) {
    if (l<mi_l) return 0;
    if (l>ma_l) return 0;
    if (p<0) return 1;
    if (l==mi_l && p<mi_p) return 0;
    if (l==ma_l && p>ma_p) return 0;
    return 1;
}

void get_pos(int f_line, int f_pos, int l_line, int l_pos, int &b, int &e)
{
    b=text_line_begin(f_line);
    e=text_line_begin(l_line);
    if (b==text_gap) b=text_restart;
    if (e==text_gap) e=text_restart;

    int i=0;
    while (i<f_pos && text[b]!=EOL) {
        b++;
        i++;
        if (b==text_gap) b=text_restart;
    }
    if (i!=f_pos) b=text_line_begin(f_line+1);

    i=0;
    while (i<l_pos && text[e]!=EOL) {
        e++;
        i++;
        if (e==text_gap) e=text_restart;
    }
}

int mouse_handling()
{
    int c;
    int select=0;
    int pending=0;
    int move=0;
    int mode=0;
    int f_pos;
    int f_line;
    int l_pos;
    int l_line;

    while (1) {
        mevent e = mevent_stack[mevent_stack.sz-1];
        mevent_stack.erase(mevent_stack.end()-1);

        int line = first_line + e.y;
        int pos  = e.x - shift;

        switch (e.button) {
            case MOUSE_M :
                // paste
                select = 0;
                mode = 0;
                move =0;
                mouse_paste();
                break;
            case MOUSE_L :
                if (select && inside(f_line,f_pos,l_line,l_pos,line,pos))
                {
                    // del
                    int b,e;
                    get_pos(f_line,f_pos,l_line,l_pos,b,e);
                    mouse_delete(b,e);
                } else {
                    // move
                    text_move(text_line_begin(line));
                    line_goto(pos);
                    if (e.y==0)
                        screen_set_first_line(first_line-1);
                    if (e.y==screen_lines-1)
                        screen_set_first_line(first_line+1);
                }
                select = 0;
                mode = 0;
                move =0;
                break;
            case MOUSE_R :
                if (select && inside(f_line,f_pos,l_line,l_pos,line,pos))
                {
                    // paste
                    mouse_paste();
                    select = 0;
                    mode = 0;
                    move =0;
                } else {
                    // select
                    select = 1;
                    pending = 1;
                    f_line = line;
                    f_pos = pos;
                    mode =0;
                    if (f_pos<0) {
                        mode =1;
                        move =1;
                    }
                }
                break;
            case WHEEL_UP :
                screen_set_first_line(first_line-4);
                break;
            case WHEEL_DOWN :
                screen_set_first_line(first_line+4);
                break;
            case MOUSE_RELEASE :
                pending = 0;
                if (select) {
                    // reorder points
                    if (l_line < f_line || (l_line == f_line && l_pos <= f_pos)) {
                        swap(l_line, f_line);
                        swap(l_pos, f_pos);
                    }

                    if (mode) {
                        f_pos = 0;
                        l_pos = screen_columns;
                    }

                    int b,e;
                    get_pos(f_line,f_pos,l_line,l_pos,b,e);
                    mouse_select(b,e);
                }
        }

        if (pending) {
            l_pos = pos;
            l_line = line;
            if (l_pos != f_pos || l_line != f_line) move = 1;
        }
        else if (select) {
        }

        compute_scroll_hint();
        screen_compute_wanted();
        screen_highlight();
        display_message();
        if (select)
           mouse_highlight(f_line,f_pos,l_line,l_pos,mode);
        screen_make_it_real();
        screen_done();

        c = term_getchar();
        if (c!=MOUSE_EVENT) break;
    }

    return c;
}

int mouse = 0;
vector<int> buffer;
int screen_getchar() {
    int c;
    if (!buffer.empty()) {
        c = buffer[buffer.sz-1];
        buffer.erase(buffer.end()-1);
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

        if (mouse) {
            c = mouse_handling();
            mouse =0;
        } else {
            c = term_getchar();
            if (c==MOUSE_EVENT) {
                mouse = 1;
                c = KEY_ESC;
            }
        }

        if (c==KEY_DISP) {
            screen_ol();
            screen_redraw();
        } else if (c==PAGE_UP) {
            screen_ppage();
            continue;
        } else if (c==PAGE_DOWN) {
            screen_npage();
            continue;
        } else {
            break;
        }
    }
    return c;
}
