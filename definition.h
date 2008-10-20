#include "signal.h"

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

#define all(c)      (c).begin(),(c).end()
#define pb          push_back
#define sz          size()

/* text macro */
#define isok(c)    ((c)<0x80 || (c)>0xBF)
#define isprint(c) ((c)>=32)
#define issmall(c) ((c)>='a' && (c)<='z')
#define isbig(c)   ((c)>='A' && (c)<='Z')
#define EOL        '\n'
#define TABSTOP    4

/* function declaration */
extern vector<char>   text;
extern int     text_lines;
extern int     text_l;
extern int     text_gap;
extern int     text_restart;
extern int     text_end;    
extern void	   text_move(int);
extern int	   text_line_begin(int);

extern string  text_highlight;
extern string  text_message;

extern void	   screen_save();
extern void	   screen_restore();
extern void    screen_npage();
extern void    screen_ppage();
//extern void    screen_lineup();
//extern void    screen_linedown();

extern void    screen_init();
extern void    screen_redraw(int hint=-1);
extern void    screen_refresh();
extern uint    screen_lsize;
   
extern uchar   term_getchar();
extern void    term_set_title(uchar *);
extern void    reset_input_mode();
