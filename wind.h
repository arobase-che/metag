
#ifndef WIND_METAG_H
#define WIND_METAG_H

#include "main.h"
#include "readline.h"
#include "basic_curses.h"


void printTagInfoHeader();

void printTagInfo(menuC* menu);

void cleanmenu(menuC* menu);

void printmenu(menuC* menu);

void printStatus(void);

void prepare(char* s);

void resize(void);
void resizeMain(menuC* menu);
#endif
