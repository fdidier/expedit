#include "signal.h"
#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "sys/sendfile.h"
#include "sys/stat.h"
#include "sys/types.h"

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

using namespace std;

typedef unsigned char uchar;
typedef stringstream    SS;
typedef long long       LL;
typedef vector<int>     VI;
typedef vector<VI>      VVI;
typedef vector<LL>      VL;
typedef vector<double>  VD;
typedef vector<string>  VS;

#define fr(x,y)     for (int x=0; x<(y); x++)
#define fi(n)       fr(i,n)
#define fj(n)       fr(j,n)
#define fk(n)       fr(k,n)
#define fm(n)       fr(m,n)
#define fn(m)       fr(n,m)

#define all(c)      (c).begin(),(c).end()
#define pb          push_back
#define sz          size()

/* text macro */
#define isok(c)      ((uchar)(c)<0x80 || (uchar)(c)>0xBF)
#define isprint(c)   (((c)>=32 && (c)<128) || (c)>=256)
#define issmall(c)   ((c)>='a' && (c)<='z')
#define isbig(c)     ((c)>='A' && (c)<='Z')
#define isnum(c)     ((c)>='0' && (c)<='9')
#define isletter(c)  (issmall(c) || isbig(c) || (c)=='_' || (uchar)(c)>=128)
#define isspecial(c) (!isletter(c) && !isnum(c) && (uchar)(c)>32 && (c)!=' ')

#define EOL          '\n'

#define TABSTOP    4
#define JUSTIFY    70

/* function declaration */
extern vector<int>   text;
extern int      text_lines;
extern int      text_l;
extern int      text_gap;
extern int      undo_pos;
extern int      text_restart;
extern int      text_end;
extern void     text_move(int);
extern int      text_line_begin(int);
extern int      text_save();
extern void     line_goto(int);

extern void mouse_select(int b, int e);
extern void mouse_delete(int b, int e);
extern void mouse_paste();

extern int      match(vector<int> &s, int i);
extern int      text_real_position(int i);
extern int      search_highlight;
extern int      display_pattern;
extern vector<int> pattern;
extern string   text_message;

//extern void     screen_save();
//extern void     screen_restore();
//extern void     screen_npage();
//extern void     screen_ppage();

extern void     screen_init();
extern void     screen_redraw();
extern void     screen_refresh();
extern uint     screen_lsize;
extern void     screen_ol();
extern int      screen_getchar();

extern int      term_rawchar();
extern int      term_getchar();
extern void     term_set_title(uchar *);
extern void     reset_input_mode();
