
#ifndef ITEM_METAG_H
#define ITEM_METAG_H

#include "main.h"
#include <dirent.h>


int sort_i(const void* A, const void* B);
void freelitem( itemC* m, int s);
void listdir(int option, itemC** m, int* s);

#endif
