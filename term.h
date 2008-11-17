/* TODO */
// fonction pour changer les attributs
// modif get_char : parser les caractère speciaux et retourner les keysym...

/* terminal function */
void term_get_size(unsigned int &x, unsigned int &y);

void term_init();
void term_reset();
void term_pushback(uchar c);
void term_putchar(int c);

/* STYLE STUFF */

#define FG_COLOR(a)        ((a>>8) & 0xF)
#define BG_COLOR(a)        ((a>>12) & 0xF)

#define COMMENTS_BIT       (1 << 16)
#define SELECTION_BIT      (1 << 17)


/* Escape sequence for xterm */

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

#define NORMAL_COLOR    0
#define COMMENTS_COLOR  4

#define REVERSE_VIDEO   printf("\e[7m");
#define NORMAL_VIDEO    printf("\e[0m");
#define SET_BG_COLOR(a) printf("\e[4%im",a);
#define SET_FG_COLOR(a) printf("\e[3%im",a);

/* Key translation */

#define CTRL(c)         ((c) & 0x1F)

#define KEY_NULL        CTRL('@')

// Key we can't change :( 
#define KEY_ESC         CTRL('[')
#define KEY_TAB         CTRL('I')
#define KEY_BACKSPACE   CTRL('H')
#define KEY_ENTER       CTRL('M')

// Key that the user can acces without ctrl
#define KEY_PPAGE       CTRL('^')
#define KEY_NPAGE       CTRL('_')
#define KEY_DELETE      CTRL('D')
#define KEY_INSERT      CTRL('\\') 

// same here but no room ...
#define KEY_DOWN        CTRL('J')
#define KEY_UP          CTRL('K')

#define KEY_WORD        CTRL('Y')
#define KEY_FIND        CTRL('F')
#define KEY_GOTO        CTRL('G')

#define KEY_OLINE       CTRL('O')
#define KEY_L           CTRL('L') 

#define KEY_LEFT        CTRL('[')
#define KEY_RIGHT       CTRL(']')

#define KEY_END         CTRL('E')
#define KEY_BEGIN       CTRL('B')

#define KEY_NEXT        CTRL('N')
#define KEY_PREV        CTRL('P')

#define KEY_DLINE       CTRL('X')
#define KEY_YLINE       CTRL('C') 
#define KEY_PRINT       CTRL('V') 

#define KEY_QUIT        CTRL('Q')
#define KEY_SAVE        CTRL('S')

#define KEY_UNDO        CTRL('U')
#define KEY_REDO        CTRL('R') 
#define KEY_TILL        CTRL('T')

#define KEY_DISP        CTRL('Z')
#define KEY_JUSTIFY     CTRL('W')
#define KEY_A           CTRL('A') 
