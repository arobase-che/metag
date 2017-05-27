
#ifndef REGEX_METAG_H
#define REGEX_METAG_H


#include <regex.h>
#include "main.h"

static int compile_regex (regex_t * r, const char * regex_text);
static int match_regex (const char* rS, const char * to_match, char* m2[], int nbR);


void regexXtract(itemC* it, const char* str);
void regexSelection(menuC* menu, const char* msg);

#endif
