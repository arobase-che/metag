
#ifndef BASIC_CURSES_H
#define BASIC_CURSES_H

#include "main.h"

void printfc(char* fstrc, int max_length, ...);

void printc(char* fstrc, int max_length);
void mvprintc(int x, int y, char* fstrc, int max_length);


#define mvprintfc( x, y, fstrc, max, ...) (move((y),(x)),printfc(fstrc, max, __VA_ARGS__ ))

#endif
