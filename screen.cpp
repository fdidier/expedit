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

/* screen we want to display */
uint     **screen_wanted;
uint     **color_wanted;
uint     screen_wanted_i;
uint     screen_wanted_j;

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
void screen_move_curs(uint i, uint j) 
{
    if (screen_real_j == screen_columns+1) {
        screen_real_j = 0;
        screen_real_i ++;
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
int saved_num;

/* Try to set the wanted curs position and the scroll_hint in a clever way */
void screen_movement_hint() 
{
    /* screen line position */
    int temp = int(text_l)-int(saved_num);
    temp += int(screen_real_i);
    saved_num = text_l;
    
    /* make the screen wanted cursor line in range and set the scroll_hint */
    screen_scroll_hint = 0;
    if (temp<=0) {
        if (text_l==0) {
            screen_scroll_hint = temp;
            screen_wanted_i=0;
        } else {
            screen_scroll_hint = temp-1;
            screen_wanted_i=1;
        }
    } else {
        screen_wanted_i = temp;
        int top = screen_lines-2;
        if (screen_wanted_i>top) {
                screen_scroll_hint = screen_wanted_i - top;
                screen_wanted_i = top;
        }
    }
}

/* compute the wanted view of the screen */

// if there is line number
int shift;

int opt_line=1;

void screen_compute_wanted() 
{
    // not enough line ?
    if (screen_wanted_i > text_l) 
        screen_wanted_i = text_l;

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
    
    // compute screen_wanted_j
    int i = text_gap;
    int l = 0;
    while (i>0 && text[i-1]!=EOL) { 
        l++;   
        i--;
    }
    screen_wanted_j = shift + min(l, int(screen_columns-shift-1));

    // find the beginning of the first line.
    fn (screen_wanted_i) {
        i--;
        while (i>0 && text[i-1]!=EOL) i--;
    }

    // correct if i==0
    if (i==text_gap) i=text_restart;

    // fill the line one by one
    fn (screen_lines) {
        int j=0;
        int num = text_l - screen_wanted_i + n +1;
        if (num > text_lines) num=0;
        while (j<shift) {
            if (num==0 || j==0) {
                screen_wanted[n][shift-1-j]= ' ';
            } else {
                screen_wanted[n][shift-1-j]= (num%10) +'0';
                num/=10;
            }
            j++;
        }
        while (i<text_end && text[i]!=EOL) {
            if (n==screen_wanted_i) {
                int pos = i - text_gap;
                if (pos>0) pos = i - text_restart;
                pos += screen_wanted_j;
                if (pos>=shift && pos < screen_columns ) {
                   screen_wanted[n][j] = text[i];
                   j++;
                }
            } else {
                if (j<screen_columns) {
                    screen_wanted[n][j] = text[i];
                    j++;
                }
            }
            i++;
            if (i==text_gap) i=text_restart;
        };
        i++;
        if (i==text_gap) i=text_restart;
        screen_wanted[n][j]=EOL;
        while (j+1<screen_columns) {
            screen_wanted[n][j+1]=' ';
            j++;
        }
    }
}

void display_message() 
{
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
        color_wanted[screen_lines-1][j] = BLACK | REVERSE;
        j++;
    }
    screen_wanted[screen_lines-1][j]=EOL;
    color_wanted[screen_lines-1][j]=0;
}

void highlight_search() {
    if (search_highlight==0) return;
    int j=screen_wanted_j-1;
    int n=0;
    while (j>=shift && n < search_highlight) {
        color_wanted[screen_wanted_i][j] |= REVERSE;
        j--;
        n++;
    }
}

map<string,int> keyword;
void screen_highlight() 
{
    // clear
    fi (screen_lines)
    fj (screen_columns)
        color_wanted[i][j]=0;

    // comment 
    int bloc=0; 
    int line=0; 
    uint **yo=screen_wanted;
    fi(screen_lines) {
        int blue=0;
        int first=1;
        fj(screen_columns) {
      //      color_wanted[i][j]=0;
            if (j<shift) {
                color_wanted[i][j]=YELLOW;
                continue;
            }
            if (yo[i][j]==EOL) break;
            
            char c=yo[i][j];
            if (first) {
                if (c==' ') continue;
                if (isspecial(c) && c!='{' && c!='}' && c!='[' 
                                 && c!=']' && c!='(' && c!=')') 
                    blue=1;
                first=0;    
            }
            if (blue)
                color_wanted[i][j] = MAGENTA; 
//            else color_wanted[i][j] = WHITE;
        }
    }
    
    /* bracket matching */
    int c;
    int temp=screen_wanted_j;
    do {
        c = yo[screen_wanted_i][temp];
        if (c=='(' || c==')' || c=='{' || c=='}' || c=='[' || c==']') break;
        temp--;
    } while (temp>=0);
    if (temp<0) return;
    if (temp!=screen_wanted_j)
        color_wanted[screen_wanted_i][temp]=RED;
    
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
                if (yo[i][j]==b) count++;
                if (yo[i][j]==e) count--;
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
                if (yo[i][j]==b) count++;
                if (yo[i][j]==e) count--;
                if (count == 0) {
                    color_wanted[i][j] = RED;
                    i=-1;
                    j=-1;
                }
            }
            j=screen_columns-1;
        }
    }    
    
    return;
    
    fi (screen_lines) {
        line = 0;
        for (int j=shift; j<screen_columns; j++) {
            if (yo[i][j]=='\n') break;
            if (yo[i][j]=='/' && yo[i][j+1]=='*') bloc=1;
            if (yo[i][j]=='/' && yo[i][j+1]=='/') line=1;
            if (bloc || line) {
                yo[i][j] &= 0xFF;
                yo[i][j] |= BLUE << 8;
            }
            if (j>0 && (yo[i][j-1]&0xFF)=='*' && (yo[i][j]&0xFF)=='/') {
                if (bloc==0) {
                    /* opposite direction */
                    for (int a=0; a<=i; a++)
                    for (int b=shift;(a==i?b<=j:b<screen_columns);b++) {
                        if (yo[a][b]==EOL) break;
                        yo[a][b] &= 0xFF;
                        yo[a][b] |= BLUE << 8;
                    }
                }
                bloc=0;
            }
        }
    }
    return;
    /* c keywords */
    fi (screen_lines) {
        int j=0;
        int s=0;
        while (j<screen_columns) {
            if (screen_wanted[i][j]=='\n') break;
            while (j<screen_columns && !issmall(screen_wanted[i][j])) j++;

            string word;
            s=j;
            while (j<screen_columns && issmall(screen_wanted[i][j])) {
                word.pb(screen_wanted[i][j]);
                j++;
            }
            if (s>0 && screen_wanted[i][s-1]=='#') {
                /* preprocessor */
                s--;
                int color = MAGENTA;
                while (s<j) {
                    screen_wanted[i][s] |= color << 8;
                    s++;
                }
            } else if (keyword.find(word) != keyword.end()) {
                int color = keyword[word];
                while (s<j) {
                    screen_wanted[i][s] |= color << 8;
                    s++;
                }
            }
        }
    }
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

        keyword["int"]=GREEN;
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
}

void screen_redraw(int hint) 
{    
        screen_movement_hint();
        if (hint>=0 && hint<=screen_lines-1) {
            screen_wanted_i = hint;
        };
        
        screen_compute_wanted(); 
        screen_highlight(); 
        display_message();
        highlight_search();

        term_reset();
        screen_clear();
        screen_dump_wanted(0,screen_lines);
        screen_done();
}

int saved_line;
void screen_save() {
    saved_line=screen_real_i;
}
void screen_restore() {
    screen_redraw(saved_line);
}

void screen_refresh() 
{
//      screen_redraw();return;
        screen_movement_hint();
        screen_compute_wanted(); 
        screen_highlight(); 
        display_message();
        highlight_search();
//        fi (screen_lines) fj (screen_columns) color_real[i][j]=RED;
        screen_make_it_real();
        screen_done();
}

// mouvement bizarre des fois !!
void screen_ppage() {
    if (screen_real_i==text_l) return;
    if (screen_real_i>=screen_lines-2) {
        text_move(text_line_begin(text_l -int(screen_lines)+1));
    }
    screen_redraw(screen_lines-2);
}

void screen_npage() {
    if (int(screen_real_i) + int(text_lines) - int(text_l) <= int(screen_lines)) 
        return;
    if (screen_real_i<=1) {
        text_move(text_line_begin(text_l + int(screen_lines)-1));
    }
    screen_redraw(1);
}

void screen_ol() {
    if (opt_line) opt_line=0;
    else opt_line=1;
}

string debug;

int force = -1;
vector<int> buffer;
int screen_getchar() {
    int c;
    if (!buffer.empty()) {
        c = buffer[buffer.sz-1];
        buffer.erase(buffer.end()-1);
        return c;
    }

    if (!debug.empty()) {
        text_message = debug;
        debug.clear();
    }
    // refresh screen
    if (force>=0) {
        screen_redraw(force);
        force=-1;
    } else {
        screen_refresh();
    }

    while (1) {
        c = term_getchar();
        if (c==KEY_DISP) {
            screen_ol();
            screen_redraw();
        } else if (c==MOUSE_EVENT) {
            mevent e = mevent_stack[mevent_stack.sz-1];
            mevent_stack.erase(mevent_stack.end()-1);
            SS yo;
            yo << e.x << " " << e.y << " " << e.button;
            debug=yo.str();
            if (e.button==WHEEL_UP) {
                fi(4) buffer.pb(KEY_UP);
                c = KEY_UP;
                break;
            }
            if (e.button==WHEEL_DOWN) {
                fi(4) buffer.pb(KEY_DOWN);
                c = KEY_DOWN;
                break;
            }
            return KEY_ESC;
        } else if (c==PAGE_UP) {
            if (screen_real_i==text_l) continue;
            if (screen_real_i != screen_lines-2) {
                screen_redraw(screen_lines-2);
                continue;
            }
            force = screen_lines-2;
            fi (screen_lines-4) buffer.pb(KEY_UP);
            c = KEY_UP;
            break;
        } else if (c==PAGE_DOWN) {
            if (int(screen_real_i) + int(text_lines) - int(text_l) <= int(screen_lines)) continue; 
            if (screen_real_i > 1) {
                screen_redraw(1);
                continue;
            }
            // problem when we are on line 0...
            // do one more down ?
            force = 1;
            fi (screen_lines-4) buffer.pb(KEY_DOWN);
            c = KEY_DOWN;
            break;
        } else {
            break;
        }
    }
    return c;
}
