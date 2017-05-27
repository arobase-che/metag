
#ifndef READLINE_METAG_H
#define READLINE_METAG_H

#include "main.h"
#include <stdio.h>
#include <wchar.h>
#include <locale.h>
#include <wctype.h>
#include <unistd.h>
#include <readline/history.h>
#include <readline/readline.h>





void readline_n(void);

void init_readline(void);

void cmd_win_redisplay(bool for_resize);

void deinit_readline(void);

#endif

