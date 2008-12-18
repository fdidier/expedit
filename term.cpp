#include "definition.h"
#include "term.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <termios.h>

//#define INISEQ    printf("\e[?1048h\e[?1047h\e[H");
//#define ENDSEQ    printf("\e[?1047l\e[?1048l");
#define INISEQ    printf("\e[?1049h");
#define ENDSEQ    printf("\e[?1049l");

/* To remember original terminal attributes. */
struct termios saved_attributes;
     
void term_get_size(uint &x, uint &y)
{
        struct winsize ws;

        if (ioctl(STDIN_FILENO,TIOCGWINSZ,&ws)!=0) {
                perror("ioctl(/dev/tty,TIOCGWINSZ)");
                exit (EXIT_FAILURE);
        }

        x = ws.ws_row;
        y = ws.ws_col;
//        y--;
}

void reset_input_mode (void)
{
        tcsetattr (STDIN_FILENO, TCSANOW, &saved_attributes);
        NOMOUSE;
        ENDSEQ; 
        fflush(stdout);
}
     
void set_input_mode (void)
{
       struct termios tattr;
     
       /* Save the terminal attributes so we can restore them later. */
       tcgetattr (STDIN_FILENO, &saved_attributes);
       atexit (reset_input_mode);
     
       /* Set the funny terminal modes. */
       tcgetattr (STDIN_FILENO, &tattr);

       tattr.c_lflag &= ~(ICANON|ECHO);     /* Non canonical input, No echo  */
       tattr.c_lflag &= ~(ISIG);            /* Ignore Signal  */
       tattr.c_oflag &= ~OPOST;             /* \n and \r unchanged on output */
       tattr.c_iflag &= ~(ICRNL|INLCR);     /* \n and \r unchanged on input  */
       tattr.c_iflag &= ~(INPCK|ISTRIP|IGNCR|IXON|PARMRK); 

       tattr.c_cc[VMIN] = 1;                /* One char at a time, no timeout */
       tattr.c_cc[VTIME] = 0;

       tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);

       INISEQ;
       SETMOUSE;
       fflush(stdout);
}

void term_init()
{
        /* Make sure stdin is a terminal. */
        if (!isatty (STDIN_FILENO)) {
           fprintf (stderr, "stdin is not a terminal.\n");
           exit (EXIT_FAILURE);
        }

        /* Set terminal modes */
        set_input_mode();

        /* Set output to stdout fully buffered */
        setvbuf(stdout, NULL, _IOFBF, BUFSIZ);
}

/****************************************************************/

int  stored_char = 0;
char input_char;

uchar shift_escape_sequence() {
    uchar c;
    if (read(STDIN_FILENO, &c, 1) && c== ';') 
    if (read(STDIN_FILENO, &c, 1) && c== '2') 
    if (read(STDIN_FILENO, &c, 1)) {
        switch(c) {
            case 'A' : return KEY_PPAGE;
            case 'B' : return KEY_NPAGE;
            case 'C' : return KEY_END;
            case 'D' : return KEY_BEGIN;
            case 'H' : return KEY_BEGIN;
            case 'F' : return KEY_END;
        }
    } 
    return KEY_ESC;
}

uchar tilde_escape_sequence(char c)
{
        uchar a;

        switch (c) {
                case '2':
                        a = KEY_INSERT;
                        break;
                case '3':
                        a = KEY_DELETE;
                        break;
                case '5':
                        a = KEY_PPAGE;
                        break;
                case '6':
                        a = KEY_NPAGE;
                        break;
                case '7':
                        a = KEY_BEGIN;
                        break;
                case '8':
                        a = KEY_END;
                        break;
                default:
                        return KEY_ESC;
        };

        if (read (STDIN_FILENO, &c, 1)) {
                if (c == '~') return a;
                if (c == ';' && a==KEY_DELETE)
                    if (read(STDIN_FILENO, &c, 1) && c=='2')
                    if (read(STDIN_FILENO, &c, 1) && c=='~')
                        return KEY_DLINE;
        }
        return KEY_ESC;
}

uchar mouse_sequence() {
    uchar button,x,y;
    if (!read(STDIN_FILENO, &button, 1)) return KEY_ESC;
    if (!read(STDIN_FILENO, &x, 1)) return KEY_ESC;
    if (!read(STDIN_FILENO, &y, 1)) return KEY_ESC;
    if (button=='`') return KEY_PPAGE; // wheel up
    if (button=='a') return KEY_NPAGE; // wheel down
    if (button=='!') { // mid
        GETSEL;
        return 'a';
    } else if (button=='\"') {
        SETSEL("fred");
        return 'a';
    }

    return KEY_ESC;
}

uchar escape_sequence()
{
        struct termios tattr;
        tcgetattr (STDIN_FILENO, &tattr);

        /* Set a timeout on the input */
        tattr.c_cc[VTIME] = 1;   /* wait 0.1 seconds */
        tattr.c_cc[VMIN]  = 0;
        tcsetattr (STDIN_FILENO, TCSANOW, &tattr);

        uchar a = KEY_ESC;
        uchar c;

        if (read (STDIN_FILENO, &c, 1)) {
                if (c == '[') {
                        if (read (STDIN_FILENO, &c, 1)) {
                                switch (c) {
                                        case 'M' :
                                            a=mouse_sequence();
                                            break;
                                        case 'A' : 
                                                a = KEY_UP;
                                                break;
                                        case 'B' :
                                                a = KEY_DOWN;
                                                break;
                                        case 'C' :
                                                a = KEY_RIGHT;
                                                break;
                                        case 'D' :
                                                a = KEY_LEFT;
                                                break;
                                        case 'H' :
                                                a = KEY_BEGIN;
                                                break;
                                        case 'F' :
                                                a = KEY_END;
                                                break;
                                        case '1' :
                                                a = shift_escape_sequence();
                                                break;
                                        default :
                                                a = tilde_escape_sequence(c); 
                                                break;
                                }
                        };
                } else {
                        stored_char = 1;
                        input_char = c;
                }
        };

        /* disable the timeout on input */
        tattr.c_cc[VTIME] = 0;   
        tattr.c_cc[VMIN]  = 1;
        tcsetattr (STDIN_FILENO, TCSANOW, &tattr);

        return a;
}

/****************************************************************/

char term_getchar_internal()
{
        uchar c;

        if (stored_char) {
                stored_char = 0;
                c = input_char; 
        } else {
                read (STDIN_FILENO, &c, 1);
        }

        if (c==KEY_ESC) 
                return escape_sequence();
        else    
                return c;
}

int term_getchar() {
    int ch;
    uchar c;
    c = term_getchar_internal();
    ch = c;
    if ((c >> 7)&1) {
        /* compute utf8 encoding length */
        int l=0;
        while ((c >> (6-l))&1) l++;
        
        /* compute corresponding int */
        fi (l) {
            c = term_getchar_internal();
            ch ^= c << (8*(i+1));
        }
    }
    return ch;
}

int term_rawchar() {
    char c;
    read (STDIN_FILENO, &c, 1);
    return c;
}

void term_pushback(uchar c)
{
        stored_char = 1;
        input_char = c;
}

/****************************************************************/

void term_set_title(uchar *title)
{
        TITLE(title);
        fflush(stdout);
}

void term_reset()
{
        stored_char = 0;
}

int video_reverse = 0;
int fg_color      = BLACK;
int bg_color      = WHITE;

void term_putchar(int c, int color)
{
    /* deal with control caracter 
     * display a red capital letter instead
     */
    if (c<32) {
        c += 64;
        color = RED;
    }

    if (fg_color != color) {
        SET_FG_COLOR(color);
        fg_color = color;
    }

    do {
        putchar(c & 0xFF);
        c = c >> 8;
    } while(c);
}
