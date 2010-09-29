
/* terminal function */
void term_get_size(unsigned int &x, unsigned int &y);

void term_init();
void term_reset();
void term_pushback(uchar c);
void term_putchar(unsigned int c, int color=0);

/* STYLE STUFF */

#define FG_COLOR(a)        ((a>>8) & 0xF)
#define BG_COLOR(a)        ((a>>12) & 0xF)

#define COMMENTS_BIT       (1 << 16)
#define SELECTION_BIT      (1 << 17)

/* MOUSE stuff */

typedef struct _mevent {
    int x;
    int y;
    int button;
} mevent;

extern vector<mevent> mevent_stack;

/* Escape sequence for xterm */

#define SHOW_CURSOR     printf("\e[?25h")
#define HIDE_CURSOR     printf("\e[?25l")

#define PASTEMODE       printf("\e[?2004h")
#define NOPASTEMODE     printf("\e[?2004l")

#define SETMOUSE        printf("\e[?1002h")
#define NOMOUSE         printf("\e[?1002l")

#define GETSEL          printf("\e]52;p;?\007")
#define SETSEL(a)       printf("\e]52;p;%s\007",a);

#define TITLE(a)        printf("\e]2;%s\007",a)

#define HOME            printf("\e[H")
#define MOVE(a,b)       printf("\e[%i;%iH",a+1,b+1)
#define UP(a)           if (a) printf("\e[%iA",a); else printf("\e[A")
#define DOWN(a)         if (a) printf("\e[%iB",a); else printf("\e[B")
#define RIGHT(a)        printf("\e[%iC",a)
#define LEFT(a)         printf("\e[%iD",a)

#define DEL_LINE(a)     printf("\e[%iM",a)
#define INS_LINE(a)     printf("\e[%iL",a)
#define CLEAR_EOL       printf("\e[K")
#define CLEAR_ALL       printf("\e[2J")

#define DEL_CAR(a)      if (a) printf("\e[%iP",a); else printf("\e[P")
#define INS_CAR(a)      if (a) printf("\e[%i@",a); else printf("\e[@")

#define SCROLL_UP(a)    if (a) printf("\e[%iS",a); else printf("\e[S")
#define SCROLL_DOWN(a)  if (a) printf("\e[%iT",a); else printf("\e[T")

#define BLACK   0
#define RED     1
#define GREEN   2
#define YELLOW  3
#define BLUE    4
#define MAGENTA 5
#define CYAN    6
#define WHITE   7
#define DEFAULT_COLOR 9

#define REVERSE 1<<10

#define NORMAL_COLOR    0
#define COMMENTS_COLOR  4

#define NORMAL_STYLE    printf("\e[m")
#define REVERSE_VIDEO   printf("\e[7m")
#define NORMAL_VIDEO    printf("\e[0m")
#define SET_BG_COLOR(a) printf("\e[4%im",a)
#define SET_FG_COLOR(a) printf("\e[3%im",a)

// Commands definition

// Design :
// the editor interpret 32 bits integer

// 0->31        control commands
// 32->126      normal ascii char, interpreted as if
// 127          converted to ^H
// 128->255     extended commands

// higher       used for unicode char...
//              packed utf-8 encoding

// Control macro
#define CTRL(c)         ((c) & 0x1F)

// Keys we can't change :(
#define KEY_ESC         CTRL('[')
#define KEY_TAB         CTRL('I')
#define KEY_BACKSPACE   CTRL('H')
#define KEY_ENTER       CTRL('M')

#define KEY_QUIT        CTRL('Q')
#define KEY_SAVE        CTRL('S')

#define KEY_DELETE      CTRL('D')
#define KEY_INSERT      201//CTRL(']')
#define KEY_OLINE       CTRL('O')
#define KEY_JUSTIFY     CTRL('J')
#define KEY_CASE        CTRL(']')

#define KEY_GOTO        CTRL('L')

#define KEY_FIND        CTRL('F')
#define KEY_AGAIN       CTRL('G')
#define KEY_FIRST       CTRL('U')
#define KEY_NEXT        CTRL('N')
#define KEY_PREV        CTRL('P')

#define KEY_END         CTRL('E')
#define KEY_BEGIN       CTRL('A')

#define KEY_CUT         CTRL('X')
#define KEY_COPY        CTRL('C')
#define KEY_PRINT       CTRL('V')

#define KEY_UNDO        CTRL('Z')
#define KEY_REDO        CTRL('R')
#define KEY_TILL        CTRL('T')

#define KEY_KWORD       CTRL('W')
#define KEY_DEND        CTRL('K')

#define KEY_BWORD       CTRL('B')
#define KEY_FWORD       CTRL('Y')

#define KEY_JUMP        CTRL('\\')

// used for run length coding ??
#define KEY_NULL        200

// special commands
#define KEY_MARK        200//CTRL('@')

// arrow key
#define KEY_DOWN        128
#define KEY_UP          129
#define KEY_LEFT        130
#define KEY_RIGHT       131

// pgup/down
#define PAGE_UP         136
#define PAGE_DOWN       137

#define MOUSE_EVENT     132

// xterm button code
#define MOUSE_R         0
#define MOUSE_L         2
#define MOUSE_M         1
#define MOUSE_RELEASE   3
#define WHEEL_UP        64
#define WHEEL_DOWN      65

//Toggle Flag
#define KEY_DISP        140
#define KEY_AI          141

//Function keys
#define KEY_F1       KEY_DISP
#define KEY_F2       KEY_AI
#define KEY_F3       KEY_DISP
#define KEY_F4       KEY_ESC

