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


/* Escape sequence for terminal command */

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

// Key we can't change :( 
#define KEY_ESC         CTRL('[')
#define KEY_TAB         CTRL('I')
#define KEY_BACKSPACE   CTRL('H')
#define KEY_ENTER       CTRL('M')

// Key that the user can acces without ctrl
#define KEY_PPAGE       CTRL('^')
#define KEY_NPAGE       CTRL('_')
#define KEY_DELETE      CTRL(']')
#define KEY_INSERT      CTRL('\\') 

// same here but no room ...
#define KEY_DOWN        CTRL('J')
#define KEY_UP          CTRL('K')

#define KEY_LEFT        CTRL('W')
#define KEY_RIGHT       CTRL('A')

#define KEY_END         CTRL('L')
#define KEY_BEGIN       CTRL('Y')

#define KEY_PREV        CTRL('P')
#define KEY_NEXT        CTRL('N')

#define KEY_FIND        CTRL('F')
#define KEY_GOTO        CTRL('G')
#define KEY_BACK        CTRL('B')
#define KEY_TILL        CTRL('T')

#define KEY_DISP        CTRL('Z')

#define KEY_CUT        CTRL('X')
#define KEY_COPY       CTRL('C')
#define KEY_PASTE      CTRL('V')
#define KEY_OLINE      CTRL('O')

#define KEY_QUIT        CTRL('Q')
#define KEY_SAVE        CTRL('S')

#define KEY_UNDO        CTRL('U')
#define KEY_REDO        CTRL('R')

/* Test with capital Letter 
 * Use tab to change capitalisation */

/* !! Warning with backup method */
//#define KEY_ESC         CTRL('[')
//#define KEY_TAB           CTRL('I')
//#define KEY_ENTER       CTRL('M')
//#define KEY_INSERT      CTRL('R')
//#define KEY_BACKSPACE   CTRL('H')
//#define KEY_DELETE        'X'

//#define KEY_BEGIN     'B'
//#define KEY_END           'E'

//#define KEY_OLINE     'O'
/*open line before*/
//#define KEY_DLINE     'D'
//#define KEY_PLINE     'P'
/* print line/bloc after
 * Go at end of block,
 * esc cancel, bacspace/tab chg indent?
 */

//#define KEY_YLINE       'Y'
//#define KEY_MLINE       'M'

//#define KEY_RLINE     'R'
/* 
 * Esc To cancel an editig and restore previous line.
 *
 * some idea on repeat :
 * Once we enter a line, command on it are [L/R moves] + [rest].
 * [L/R moves] are ignored, the rest is saved when we leave the line.
 * only [L/R moves] don't erase the saved operation.
 * repeat perform the [rest] on the current line.
 */

/* Issue with block and multiple repeat a la delete
 * Current idea:
 * If one of the R/D/M line command is directly preceded by a Mark command,
 * apply it on the block. (ignore move)
 *
 * We loose MMM to mark 3 lines though.
 * And problem with unwanted yanking I guess (because Mark yank as well).
 * Current sol:2 commands.
 */

//#define KEY_JUMP      'G'
/*can be used as a numerical flag too ?*/

//#define KEY_QUIT      'Q'
//#define KEY_SAVE      'S'
//#define KEY_CHANGE        'C'
/* change name of file / open new file, existing file */

//#define KEY_UP            'I'
//#define KEY_DOWN      'K'
//#define KEY_LEFT      'J'
//#define KEY_RIGHT     'L'

//#define KEY_REPLACE       'H'
/* ask if want to work on selected block if M before */
//#define KEY_SEARCH        'F'
//#define KEY_UNDOMODE  'Z'
/* Weird undo but simple to implement and most useful than many.
 * navigate between saved session,
 * detail of one, navigate between change,
 * done.
 * In all operation you can scroll.
 *
 * In bacup file, print the position used to resume ...
 * Suspend backup in this mode.
 */

/* Esc to end search session and retreive pup/pdown
 * Esc to go back at the beginning of a scroll*/
//#define KEY_PPAGE       'U'
//#define KEY_NPAGE       'N'

/* inline operation, 
 * We can use the same mark I guess */
//#define KEY_APPEND        'A'
//#define KEY_FIND      'T'
//#define KEY_FINDBACK  'W'
//#define KEY_CHAR      'V'


