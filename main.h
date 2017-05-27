
#ifndef MAIN_METAG_H
#define MAIN_METAG_H

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef  _XOPEN_SOURCE
#define _XOPEN_SOURCE 700 // For strnlen()
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>
#include <taglib/tag_c.h>



typedef struct tagInfo {
  TagLib_File *file;
  TagLib_Tag *tag;
  const TagLib_AudioProperties *properties;
} tagInfo;

typedef struct itemC {
    char* cstr;
    int opt; 
    char* suffix;
    char* prefix;
    tagInfo info;
    int selected;
    int id;
} itemC;

typedef struct menuC {
    itemC* list;
    int nbElem;
    int hl;
    int firstElem;
    int opt; // SCROLL | HL_HIDE | HL_HIDE | HL_BOLD | HL_CLIGN | HL_INV [ | BORDER ]
    char* suffix;
    char* prefix;
    int x,y,w,h;
} menuC;


#define HIDDEN 1

void printStatus(void);

void resize(void);


void fail_exit(const char *msg);



// Checks errors for (most) ncurses functions. CHECK(fn, x, y, z) is a checked
// version of fn(x, y, z).
#define CHECK(fn, ...)               \
  do                                 \
      if (fn(__VA_ARGS__) == ERR)    \
          fail_exit(#fn"() failed"); \
  while (false)


#define max(a, b)         \
  ({ typeof(a) _a = a;    \
     typeof(b) _b = b;    \
     _a > _b ? _a : _b; })




#endif 
