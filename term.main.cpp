#include "definition.h"
#include "term.h"

#include <stdlib.h>
#include <stdio.h>

int main (void)
{
        unsigned char c;
            
        term_init();
        SETMOUSE;

        while (1)
         {
           c = term_rawchar();
           if (c == CTRL('D'))     
             break;
           if (c == CTRL('Q'))
             break;
			if (c=='t') {
				TITLE("fred");
			} else;
		   if (c=='p') {GETSEL;} else
		   if (c=='a') {SETSEL("yo")} else
           term_putchar(c);
           fflush(stdout);
         }
        NOMOUSE;
        return EXIT_SUCCESS;
}
