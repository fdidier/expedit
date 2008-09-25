#include "definition.h"
#include "term.h"

/************************************************************************/
/* Screen Handling                                                      */
/************************************************************************/

/* screen dimension */
uint     screen_lines;
uint     screen_columns;
uint     screen_lsize;

/* screen currently displayed */
uint     **screen_real;
uint     screen_real_i;      /* cursor line in screen */
uint     screen_real_j;      /* cursor pos  in screen */

/* screen we want to display */
uint     **screen_wanted;
uint     screen_wanted_i;
uint     screen_wanted_j;

/* Other variables */
int screen_scroll_hint;
int screen_saved_num;

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
        }
                
        return array;
}

void    screen_alloc() 
{
        if (screen_wanted) {
                free(screen_wanted[0]);
                free(screen_wanted);
        }

        if (screen_real) {
                free(screen_real[0]);
                free(screen_real);
        }

        screen_wanted = screen_alloc_internal(screen_lines, screen_columns+1);
        screen_real   = screen_alloc_internal(screen_lines, screen_columns+1);
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
    if (screen_real_j == screen_columns) {
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
void screen_print(uint c) 
{
    term_putchar(c);
    if (isok(c & 0xFF)) {
        if (screen_real_j == screen_columns) {
            screen_real_j = 0;
            screen_real_i ++;
        }
        screen_real_j++;
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

                        screen_print(w);
                        j++;
                }
        }
}

/* Make the real screen look like the wanted one */
void screen_make_it_real() 
{
        uint i;
        uint poss;  // pos on screen
        uint posw;  // pos on wanted buffer
        uint posr;  // pos on real buffer
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
                poss=0;
                posw=0;
                posr=0;
                r=0;
                while (1) {
                        w = screen_wanted[i-off][posw]; 
                        if (r != '\n') r = screen_real[i-b][posr];

                        if (w == '\n') {
                                if (r != '\n') {
                                        screen_move_curs(i,poss);
                                        CLEAR_EOL;
                                        if (i != (screen_lines - 1)) 
                                            screen_newl();
                                }
                                break;
                        }

                        /* utf8 case !! */
                        if (w>=128) {
                            int diff=0;
                            int t=1;
                            if (w != r) diff=1;
                            while (!isok( screen_wanted[i-off][posw+t] & 0xFF)) {
                                if (!diff)
                                if (screen_wanted[i-off][posw+t] != screen_real[i-b][posr+t]) 
                                    diff = 1;
                                t++;
                            }
                            if (diff) {
                                screen_move_curs(i,poss);
                                fk(t) screen_print(screen_wanted[i-off][posw+k]);
                            }
                        } 
                        else if (w != r) {
                                screen_move_curs(i,poss);
                                screen_print(w);
                                if (screen_real_j == screen_columns) {
                                        poss=0;
                                }
                        }

                        poss++;
                        posw++; 
                        while (!isok(screen_wanted[i-off][posw] & 0xFF)) posw++;
                        posr++; 
                        while (!isok(screen_real[i-b][posr] & 0xFF)) posr++;
                }
        }

        if (off) {
                if (screen_real_i != screen_lines-1) 
                    screen_move_curs(screen_lines-1,0);
                screen_newl();
                screen_dump_wanted(screen_lines-off,screen_lines);
        }
}

/* Function to call when the real screen is equal to the wanted one */
void screen_done() 
{
        /* put the cursor as wanted */
        screen_move_curs(screen_wanted_i, screen_wanted_j);
        screen_scroll_hint = 0;

        /* swap the two screen array to update screen real */
        uint **temp = screen_real;
        screen_real = screen_wanted;
        screen_wanted = temp;

        /* make the change */
        fflush(stdout);
}


/*********************************************************************************/
/* Specific functions for displaying a text structure                            */
/*********************************************************************************/

/* Try to set the wanted curs position and the scroll_hint in a clever way */
/* (no wrap version) */
void screen_movement_hint() 
{
        /* screen cursor position */
        screen_wanted_j = screen_p;
        screen_wanted_j %= screen_columns;

        /* screen cursor line move according to line num */
        int temp = int(screen_l)-int(screen_saved_num);
        temp += int(screen_real_i);
        screen_saved_num = screen_l;

        /* make the screen wanted cursor line in range and set the scroll_hint */
        screen_scroll_hint = 0;

        if (temp<=0) {
                screen_scroll_hint = temp;
                screen_wanted_i = 1;
                if (text_l==0) screen_wanted_i=0;
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

void screen_compute_wanted() 
{
    fi (screen_lines) 
        screen_wanted[i][screen_columns]=EOL;

    // not enough line ?
    if (screen_wanted_i > text_l) 
        screen_wanted_i = text_l;

    // line numbers
    int opt_line=1;
    if (opt_line) {
            int num = text_l+screen_lines-screen_wanted_i;
            shift=1;
            while (num) {
                shift++;
                num/=10;
            }
            shift=max(4,shift);
    }
    // compute screen_wanted_j
    
    screen_lsize = screen_columns-shift;
    screen_wanted_j = shift + (screen_p % ( screen_columns-shift));

    // find the top left character position
    int i=text_gap;
    int l=screen_wanted_i;
    int n=text_l;
    while (1) {
        int s=0;
        while (i>0 && text[i-1]!=EOL) {
            i--;
            s++;
        }
        l -= s / (screen_columns - shift);
        if (l<=0) break;
        n--;
        i--;
        l--;
    }

    while (l<0) {
        i+= (screen_columns - shift);
        l++;
    }

    // fill screen_wanted

    // i = position in text
    // l = screen_line
    // j = screen_column
    // n = current_line number
    
    for (l=0; l<screen_lines; l++) 
    {
        int j=0;
        // display line number
        // first version : no empty number when swapped line 
        int num=n+1;
        while (j<shift) {
            if (num==0 || j==0) {
                screen_wanted[l][shift-1-j]= ' ';
            } else {
                screen_wanted[l][shift-1-j]= ((num%10) +'0') | (YELLOW <<8);
                num/=10;
            }
            j++;
        }

        if (i==text_gap) i=text_restart;
        while (i<text_end && text[i]!=EOL && j<screen_columns) {
            screen_wanted[l][j] = text[i];
            j++;
            i++;
            if (i==text_gap) i=text_restart;
        }
        
        if (text[i]==EOL) {
            n++;
            i++;
            // j can be equal to text_columns but it is fine.
            screen_wanted[l][j]=EOL;
        } 

        // end of text
        if (i==text_end) {
            l++;
            while (l<screen_lines) { 
                screen_wanted[l][0]=EOL;
                l++;
            }
            break;
        }
    }

    // message ?
    if (!text_message.empty()) {
        int i=0;
        while (text_message[i]!=EOL && i<screen_columns) {
            screen_wanted[screen_lines-1][i] = text_message[i];
            i++;
        }
        screen_wanted[screen_lines-1][i]='\n';
    }
}

map<string,int> keyword;
void screen_highlight() 
{
    /* comment */ 
    int bloc=0; 
    int line=0; 
    uint **yo=screen_wanted;
//  fi(screen_lines) fj(screen_columns) if (yo[i][j]!=EOL) yo[i][j]|=WHITE<<8; 
//  return;
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
        term_reset();

        screen_clear();
        
        screen_movement_hint();
        if (hint ==-1) {
            screen_wanted_i=screen_lines/2;
        } else if (hint>0 && hint<screen_lines-1) {
            screen_wanted_i=hint;
        }
        
        screen_compute_wanted(); 
        screen_highlight(); 
        screen_dump_wanted(0,screen_lines);
        screen_done();
}

void screen_refresh() 
{
//      screen_redraw();return;
        screen_movement_hint();
        screen_compute_wanted(); 
        screen_highlight(); 
        screen_make_it_real();
        screen_done();
}
