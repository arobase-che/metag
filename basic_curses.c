/*
 * Ache - 2017-08-14 - GPLv3 or later
 */

#include "basic_curses.h"

/*
 * Set of functions to easy print on the ncurse interface
 */



void printfc(char* fstrc, int max_length, ...) {
    int i = 0;
    int nbC = 0;
    char* tmp = NULL;
    int toP = 0;


    va_list ap;
    va_start(ap, max_length);


    while(fstrc[i] && nbC < max_length) {
        if( strchr( "${<%>}#", fstrc[i]) ) {
            if( tmp ) {
                printw("%.*s", toP, tmp);
                tmp=NULL;
                toP=0;
            }
            switch( fstrc[i] ) {
                case '$':
                    if( fstrc[i+1] ) {
                        if( isdigit(fstrc[i+1]) ) {
                            if( fstrc[i+1] == '9') {
                                ;
                            }else
                                attrset( COLOR_PAIR(fstrc[i+1]-'0') | A_NORMAL);
                        }else{
#ifdef DEBUG
                            fprintf(stderr,"%c isn't a color, skipping", fstrc[i+1]);
#endif
                        }
                    }else{
#ifdef DEBUG
                        fprintf(stderr,"end of line by '$', expect a color after '$'");
#endif
                        return;
                    }
                    i+=2;
                    break;
                case '%':
                    {
                        int j = 1;
                        char format[10] = "%";
                        while(j < 9 && !strchr("diouxXeEfFgGaAcsp%", format[j] = fstrc[i+j]) );
                        if( j == 9 ) {
#ifdef DEBUG
                            fprintf(stderr,"format too long %s...",format);
#endif
                            return;
                        }
                        if( strchr("di", format[j] ) ) {
                            printw(format, va_arg(ap, int));
                        }else if( strchr("ouxX", format[j]) ) {
                            printw(format, va_arg(ap, unsigned int));
                        }else if( strchr("eE", format[j]) ) {
                            printw(format, va_arg(ap, double));
                        }else if( strchr("fF", format[j]) ) {
                            printw(format, va_arg(ap, double));
                        }else if( strchr("gG", format[j]) ) {
                            printw(format, va_arg(ap, double));
                        }else if( strchr("aA", format[j]) ) {
                            printw(format, va_arg(ap, double));
                        }else if( strchr("c", format[j]) ) {
                            printw(format, va_arg(ap, int));
                        }else if( strchr("s", format[j]) ) {
                            printw(format, va_arg(ap, const char*));
                        }else if( strchr("p", format[j]) ) {
                            printw(format, va_arg(ap, void*));
                        }else if( strchr("%", format[j]) ) {
                            printw("%%");
                        }
//                        nbC+=

                        i+=j;
                    }
            }
        }else{
            if( ! tmp )
                tmp = fstrc+i;
            toP += 1;
            nbC++;
            i++;
        }
    }
    if(tmp)
        printw("%.*s", toP, tmp);
    fflush(NULL);
}

void printc(char* fstrc, int max_length) {
    int i = 0;
    int nbC = 0;
    char* tmp = NULL;
    int toP = 0;
    while(fstrc[i] && nbC < max_length) {
        if( strchr( "${<>}#", fstrc[i]) ) {
            if( tmp ) {
                printw("%.*s", toP, tmp);
                tmp=NULL;
                toP=0;
                break;
            }
            switch( fstrc[i] ) {
                case '$':
                    if( fstrc[i+1] ) {
                        if( isdigit(fstrc[i+1]) ) {
                            if( fstrc[i+1] == '9') {
                                ;
                            }else
                                attrset( COLOR_PAIR(fstrc[i+1]-'0') | A_NORMAL);
                        }else{
#ifdef DEBUG
                            fprintf(stderr,"%c isn't a color, skipping", fstrc[i+1]);
#endif                            
                        }
                    }else{
#ifdef DEBUG
                        fprintf(stderr,"end of line by '$', expect a color after '$'");
#endif
                        return;
                    }
                    i+=2;
            }
        }else{
            if( ! tmp )
                tmp = fstrc+i;
            toP += 1;
            nbC++;
            i++;
        }
    }
    if(tmp)
        printw("%.*s", toP, tmp);
    fflush(NULL);
}
void mvprintc(int x, int y, char* fstrc, int max_length) {
    move(y, x);
    printc(fstrc, max_length);
}


