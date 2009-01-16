
/* terminal function */
void term_get_size(unsigned int &x, unsigned int &y);

void term_init();
void term_reset();
void term_pushback(uchar c);
void term_putchar(int c, int color=0);

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

#define REVERSE 1<<10

#define NORMAL_COLOR    0
#define COMMENTS_COLOR  4

#define REVERSE_VIDEO   printf("\e[7m");
#define NORMAL_VIDEO    printf("\e[0m");
#define SET_BG_COLOR(a) printf("\e[4%im",a);
#define SET_FG_COLOR(a) printf("\e[3%im",a);

// Commands definition

// Design :
// the editor interpret a string of int
// 0->31        control commands
// 32->127      normal ascii char, interpreted as if
// 128->255     extended commands
// higher       used for unicode char...
//

// Control macro
#define CTRL(c)         ((c) & 0x1F)

// Keys we can't change :( 
#define KEY_ESC         CTRL('[')
#define KEY_TAB         CTRL('I')
#define KEY_BACKSPACE   CTRL('H')
#define KEY_ENTER       CTRL('M')
#define KEY_EOL         CTRL('J') // We can use this one

// What would be nice to use
// and their replacement for now

#define CONTROL_I       CTRL('_')
#define CONTROL_J       CTRL('^')
#define CONTROL_M       CTRL(']')

#define KEY_DELETE      140         //CTRL('D')
#define KEY_INSERT      141         //CTRL('\\') 

// used for run length coding ??
#define KEY_NULL        CTRL('@')

// to screen (used internaly to make a pos
#define KEY_DISP        CTRL('\\')

// not implemented yet
#define KEY_MACRO       CTRL('Z')

// goto, 
// TODO: add flavour to go to begin/end of text
// to given char ?
#define KEY_GOTO        CTRL('G')

// search stuff
#define KEY_WORD        CTRL('W')
#define KEY_FIND        CTRL('F')

#define KEY_NEXT        CTRL('N')
#define KEY_PREV        CTRL('P')

#define KEY_END         CTRL('E')
#define KEY_BEGIN       CTRL('A')

#define KEY_OLINE       CTRL('O')

#define KEY_MARK        CTRL('Y')

#define KEY_DLINE       CTRL('X')
#define KEY_YLINE       CTRL('C') 
#define KEY_PRINT       CTRL('V') 

#define KEY_QUIT        CTRL('Q')
#define KEY_SAVE        CTRL('S')

#define KEY_UNDO        CTRL('U')
#define KEY_REDO        CTRL('R') 
#define KEY_TILL        CTRL('T')

#define KEY_KWORD       CTRL('K')
#define KEY_DEND        CTRL('D')
#define KEY_BWORD       CTRL('B')

#define KEY_JUSTIFY     CTRL('L')


// special commands

// arrow key
#define KEY_DOWN        128
#define KEY_UP          129
#define KEY_LEFT        130
#define KEY_RIGHT       131

// pgup/down
#define PAGE_UP         136
#define PAGE_DOWN       137

#define KEY_AI          CONTROL_M

#define MOUSE_EVENT     132

// xterm button code
#define MOUSE_R         0
#define MOUSE_L         1
#define MOUSE_M         2
#define MOUSE_RELEASE   3
#define WHEEL_UP        64
#define WHEEL_DOWN      65

//Function keys
#define KEY_F1       KEY_DISP
#define KEY_F2       KEY_AI
#define KEY_F3       KEY_ESC
#define KEY_F4       KEY_ESC

