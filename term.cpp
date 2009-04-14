#include "definition.h"
#include "term.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <termios.h>

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
        SHOW_CURSOR;
        NORMAL_VIDEO;
        ENDSEQ;
        fflush(stdout);
}

void set_input_mode (void)
{
       struct termios tattr;

       /* Save the terminal attributes so we can restore them later. */
       tcgetattr (STDIN_FILENO, &saved_attributes);
       //atexit (reset_input_mode);

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
       NORMAL_VIDEO;
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

int shift_escape_sequence() {
    uchar c;
    if (read(STDIN_FILENO, &c, 1) && c== ';')
    if (read(STDIN_FILENO, &c, 1) && c== '2')
    if (read(STDIN_FILENO, &c, 1)) {
        switch(c) {
            case 'A' : return PAGE_UP;
            case 'B' : return PAGE_DOWN;
            case 'C' : return KEY_FWORD;
            case 'D' : return KEY_BWORD;
            case 'H' : return KEY_BEGIN;
            case 'F' : return KEY_END;
        }
    }
    return KEY_ESC;
}

int  tilde_escape_sequence(char c)
{
        int a;

        switch (c) {
                case '2':
                        a = KEY_INSERT;
                        break;
                case '3':
                        a = KEY_DELETE;
                        break;
                case '5':
                        a = PAGE_UP;
                        break;
                case '6':
                        a = PAGE_DOWN;
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
                        return KEY_CUT;
        }
        return KEY_ESC;
}

// **********************
// mouse stuff
// **********************
vector<mevent> mevent_stack;

int mouse_sequence() {
    uchar button,x,y;
    if (!read(STDIN_FILENO, &button, 1)) return KEY_ESC;
    if (!read(STDIN_FILENO, &x, 1)) return KEY_ESC;
    if (!read(STDIN_FILENO, &y, 1)) return KEY_ESC;

    mevent event;

    // 0 based from top left corner
    event.x = x - 33;
    event.y = y - 33;

    // mouse button
    event.button = button - 32;

    mevent_stack.pb(event);
    return MOUSE_EVENT;
}

int function_keys() {
    uchar c;
    if (read (STDIN_FILENO, &c, 1)) {
        switch (c) {
            case 'H' : return KEY_BEGIN;
            case 'F' : return KEY_END;
            case 'P' : return KEY_F1;
            case 'Q' : return KEY_F2;
            case 'R' : return KEY_F3;
            case 'S' : return KEY_F4;
            default : return KEY_ESC;
        }
    }
    return KEY_ESC;
}

int escape_sequence()
{
        struct termios tattr;
        tcgetattr (STDIN_FILENO, &tattr);

        /* Set a timeout on the input */
        tattr.c_cc[VTIME] = 1;   /* wait 0.1 seconds */
        tattr.c_cc[VMIN]  = 0;
        tcsetattr (STDIN_FILENO, TCSANOW, &tattr);

        int a = KEY_ESC;
        uchar c;

        if (read (STDIN_FILENO, &c, 1)) {
                if (c == 'O') {
                    a = function_keys();
                } else
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

int term_getchar_internal()
{
    uchar c;

    if (stored_char) {
        stored_char = 0;
        c = input_char;
    } else {
        read (STDIN_FILENO, &c, 1);
    }

    if (c==KEY_ESC) return escape_sequence();
    if (c==127) return KEY_BACKSPACE;
    return c;
}

int term_getchar()
{
    int ch= term_getchar_internal();
    uchar c = ch & 0xFF;

    if ((c >> 7)&1) {
        /* compute utf8 encoding length */
        int l=0;
        while ((c >> (6-l))&1) l++;

        /* compute corresponding int */
        fi (l) {
            c = term_getchar_internal() & 0xFF;
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
int fg_color      = -1;
int bg_color      = -1;

void term_putchar(unsigned int c, int color)
{
    /* deal with control caracter
     * display a red capital letter instead
     */
    if (c<32) {
        c += 64;
        color = RED;
    }
    if (c>126 && c<256) {
        c = '@';
        color = BLUE;
    }

    int col = color & 0xFF;
    if (fg_color != col) {
        if (color == DEFAULT_COLOR)
            NORMAL_STYLE;
        else
            SET_FG_COLOR(col);
        fg_color = col;
    }

    if ((color & REVERSE) && !video_reverse) {
        video_reverse=1;
        REVERSE_VIDEO;
    }
    if ((color & REVERSE)==0 && video_reverse) {
        video_reverse=0;
        NORMAL_VIDEO;
        SET_FG_COLOR(col);
    }

    do {
        int t = c & 0xFF;
        putchar(t);
        c = c >> 8;
    } while(c);
}
