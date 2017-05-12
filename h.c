#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 700 // For strnlen()
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include <ncurses.h>
#include <dirent.h>
#include <locale.h>
#include <taglib/tag_c.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdnoreturn.h>
#include <wchar.h>
#include <wctype.h>
#include <regex.h>
#include <unistd.h>


#ifdef DEBUG

#include <assert.h>

#endif





#define MAX_PATH 1024


#define max(a, b)         \
  ({ typeof(a) _a = a;    \
     typeof(b) _b = b;    \
     _a > _b ? _a : _b; })




static short my_fg = COLOR_WHITE;
static short my_bg = COLOR_BLACK;
static bool visual_mode = false;
static bool input_avail = false;
static unsigned char input;
static char *msg_win_str = NULL;
static bool should_exit = false;
static WINDOW *cmd_win;
static WINDOW *sep_win;

char status[10];

int color[] = {
    COLOR_BLACK, COLOR_RED,
    COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
    COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE};



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


// Calculates the cursor column for the readline window in a way that supports
// multibyte, multi-column and combining characters. readline itself calculates
// this as part of its default redisplay function and does not export the
// cursor column.
//
// Returns the total width (in columns) of the characters in the 'n'-byte
// prefix of the null-terminated multibyte string 's'. If 'n' is larger than
// 's', returns the total width of the string. Tries to emulate how readline
// prints some special characters.
//
// 'offset' is the current horizontal offset within the line. This is used to
// get tabstops right.
//
// Makes a guess for malformed strings.
static size_t strnwidth(const char *s, size_t n, size_t offset)
{
    mbstate_t shift_state;
    wchar_t wc;
    size_t wc_len;
    size_t width = 0;

    // Start in the initial shift state
    memset(&shift_state, '\0', sizeof shift_state);

    for (size_t i = 0; i < n; i += wc_len) {
        // Extract the next multibyte character
        wc_len = mbrtowc(&wc, s + i, MB_CUR_MAX, &shift_state);
        switch (wc_len) {
        case 0:
            // Reached the end of the string
            goto done;

        case (size_t)-1: case (size_t)-2:
            // Failed to extract character. Guess that the remaining characters
            // are one byte/column wide each.
            width += strnlen(s, n - i);
            goto done;
        }

        if (wc == '\t')
            width = ((width + offset + 8) & ~7) - offset;
        else
            // TODO: readline also outputs ~<letter> and the like for some
            // non-printable characters
            width += iswcntrl(wc) ? 2 : max(0, wcwidth(wc));
    }

done:
    return width;
}

// Like strnwidth, but calculates the width of the entire string
static size_t strwidth(const char *s, size_t offset)
{
    return strnwidth(s, SIZE_MAX, offset);
}



static noreturn void fail_exit(const char *msg)
{
    // Make sure endwin() is only called in visual mode. As a note, calling it
    // twice does not seem to be supported and messed with the cursor position.
    if (visual_mode)
        endwin();
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

// Checks errors for (most) ncurses functions. CHECK(fn, x, y, z) is a checked
// version of fn(x, y, z).
#define CHECK(fn, ...)               \
  do                                 \
      if (fn(__VA_ARGS__) == ERR)    \
          fail_exit(#fn"() failed"); \
  while (false)


// Not bothering with 'input_avail' and just returning 0 here seems to do the
// right thing too, but this might be safer across readline versions
static int readline_input_avail(void)
{
    return input_avail;
}

static int readline_getc(FILE *dummy)
{
    input_avail = false;
    return input;
}
static void forward_to_readline(char c)
{
    input = c;
    input_avail = true;
    rl_callback_read_char();
}
static void msg_win_redisplay(bool for_resize)
{
    CHECK(mvaddstr, 0, 0, msg_win_str ? msg_win_str : "");

    // We batch window updates when resizing
/*
    if (for_resize)
        CHECK(wnoutrefresh, msg_win);
    else
        CHECK(wrefresh, msg_win);
*/
}
static void got_command(char *line)
{
    if( line )
        if (*line != '\0')
            add_history(line);

    free(msg_win_str);
    msg_win_str = line;
    //msg_win_redisplay(false);
    should_exit = true;
}

static void cmd_win_redisplay(bool for_resize)
{
    size_t prompt_width = strwidth(rl_display_prompt, 0);
    size_t cursor_col = prompt_width +
                        strnwidth(rl_line_buffer, rl_point, prompt_width);

    CHECK(werase, cmd_win);
    // This might write a string wider than the terminal currently, so don't
    // check for errors
    mvwprintw(cmd_win, 0, 0, "%s%s", rl_display_prompt, rl_line_buffer);
    if (cursor_col >= COLS)
        // Hide the cursor if it lies outside the window. Otherwise it'll
        // appear on the very right.
        curs_set(0);
    else {
        CHECK(wmove, cmd_win, 0, cursor_col);
        //curs_set(2);
    }
    // We batch window updates when resizing
    if (for_resize)
        CHECK(wnoutrefresh, cmd_win);
    else
        CHECK(wrefresh, cmd_win);
}

static void readline_redisplay(void)
{
    cmd_win_redisplay(false);
}



void rintfc(char* fstrc, int max_length);


int sort_i(const void* A, const void* B) {
    itemC* a = (itemC*)A;
    itemC* b = (itemC*)B;

    if(a->opt > b->opt) 
        return -1;
    else if(a->opt < b->opt)
        return 1;
    else {
        return strcmp(a->cstr,b->cstr);
    }
}
void listdir(int option, itemC** m, int* s) {
    DIR *dir;
    int nbitem = 0;
    itemC* menu = NULL;
    struct dirent *entry;

    if (!(dir = opendir(".")))
        return;

    if (!(entry = readdir(dir)))
        return;

    do {
        if( entry->d_name[0] == '.' && !(option & HIDDEN) && strcmp(entry->d_name, "..") )
            continue;

        menu = realloc(menu, ++nbitem * sizeof *menu);

        if (entry->d_type == DT_DIR) {
            char* tmp = malloc( strlen(entry->d_name)+3);
            sprintf(tmp, "[%s]", entry->d_name);
            menu[nbitem-1].cstr = tmp;
            menu[nbitem-1].opt  = 1;
            menu[nbitem-1].id   = nbitem-1;
            menu[nbitem-1].selected   = 0;
        }
        else {
            TagLib_File *file;
            TagLib_Tag *tag;
            const TagLib_AudioProperties *properties;

            file = taglib_file_new(entry->d_name); 

            if(file == NULL) {
                menu = realloc(menu, --nbitem * sizeof *menu);
                continue;
            }

            tag = taglib_file_tag(file);
            properties = taglib_file_audioproperties(file);

            if( ! properties )  {
                fprintf(stderr, "ID3 file but not audio");
                taglib_tag_free_strings();
                taglib_file_free(file);
            }
            menu[nbitem-1].cstr = strdup(entry->d_name);
            menu[nbitem-1].opt  = 0;
            menu[nbitem-1].id   = nbitem-1;
            menu[nbitem-1].info = (tagInfo){file, tag, properties};
            menu[nbitem-1].selected   = 0;

        }
    } while (entry = readdir(dir));
end:

    closedir(dir);
    *m = menu;
    *s = nbitem;
}

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
                            fprintf(stderr,"%c isn't a color, skipping", fstrc[i+1]);
                        }
                    }else{
                        fprintf(stderr,"end of line by '$', expect a color after '$'");
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
                            fprintf(stderr,"format too long %s...",format);
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
                            fprintf(stderr,"%c isn't a color, skipping", fstrc[i+1]);
                        }
                    }else{
                        fprintf(stderr,"end of line by '$', expect a color after '$'");
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
#define mvprintfc( x, y, fstrc, max, ...) (move((y),(x)),printfc(fstrc, max, __VA_ARGS__ ))
void printTagInfoHeader() {
    attrset( COLOR_PAIR( COLOR_BLUE + 1) | A_BOLD);
    mvprintc(COLS/2+2, 1, "Title    :",15);
    mvprintc(COLS/2+2, 2, "Artist   :",15);
    mvprintc(COLS/2+2, 3, "Album    :",15);
    mvprintc(COLS/2+2, 4, "Year     :",15);
    mvprintc(COLS/2+2, 5, "Track    :",15);
    mvprintc(COLS/2+2, 6, "Genre    :",15);
    mvprintc(COLS/2+2, 7, "Comment  :",15);
    attrset( COLOR_PAIR( 0 ) | A_NORMAL);
}
void printTagInfo(menuC* menu) {
    static int mustClear = 0;
    if( mustClear == 1 ) {
        int x = menu->x;
        int y = menu->y;
        int h = menu->h;
        int w = menu->w;

        for(int i = 0 ; i < 8 ; i++) {
            move(1+i,COLS/2+14);
            printw("%*s", COLS/2-14, " ");
        }
        mustClear = 0;
    }
    itemC* it = menu->list+menu->hl;
    if( !it->opt ) {
        char inT[6] = "";
        mvprintc( COLS/2+14, 1, taglib_tag_title(it->info.tag), COLS/2-14);
        mvprintc( COLS/2+14, 2, taglib_tag_artist(it->info.tag), COLS/2-14);
        mvprintc( COLS/2+14, 3, taglib_tag_album (it->info.tag), COLS/2-14);
        sprintf(inT, "%d", taglib_tag_year  (it->info.tag));
        mvprintc( COLS/2+14, 4, inT, COLS/2-14);
        sprintf(inT, "%d", taglib_tag_track(it->info.tag));
        mvprintc( COLS/2+14, 5, inT, COLS/2-14);
        mvprintc( COLS/2+14, 6, taglib_tag_genre(it->info.tag), COLS/2-14);
        mvprintc( COLS/2+14, 7, taglib_tag_comment(it->info.tag), COLS/2-14);
        mustClear = 1;
    }
    
    
}
void cleanmenu(menuC* menu) {
    for(int i = menu->firstElem ; i < menu->h && i < menu->nbElem; i++) {
        move(menu->y+i,menu->x);
        printw("%*s", menu->w, " ");
    }
    refresh();
}
void printmenu(menuC* menu) {

    int x = menu->x;
    int y = menu->y;
    int h = menu->h;
    int w = menu->w;
    itemC* it = menu->list;
    int s = menu->nbElem;

    if( menu->hl < (menu->firstElem) ) {
        cleanmenu(menu);
    }

    if( menu->hl >= (menu->firstElem+menu->h ) ) {
        attrset(0 | A_NORMAL );
        menu->firstElem+=menu->h/2;
        for(int i = 0 ; i < h ; i++) {
            move(y+i,x);
            printw("%*s", w, " ");
        }
        refresh();
    }


    for(int i = menu->firstElem ;  i < (h+menu->firstElem) && i < s ; i++) {
        int color = 0, attr = A_NORMAL;
        if( it[i].opt == 1 )
            color = COLOR_BLUE+1;
        if( i == menu->hl )
            attr = A_REVERSE;
        if( it[i].selected ) {
            attr |= A_BOLD;
        }

        attrset( COLOR_PAIR(color) | attr);
        move(y,x);
        printw("%*s", w, " ");
        mvprintc(x,y++,it[i].cstr, w);
        attrset(0 | A_NORMAL);
    }
}
void printStatus(void) {
    mvprintc(COLS-5,LINES-3,"    ",4);
    mvprintc(COLS-5,LINES-3,status,4);
}


static void init_readline(void)
{
    // Disable completion. TODO: Is there a more robust way to do this?
    if (rl_bind_key('\t', rl_insert) != 0)
        fail_exit("Invalid key passed to rl_bind_key()");

    // Let ncurses do all terminal and signal handling
    rl_catch_signals = 0;
    rl_catch_sigwinch = 0;
    rl_deprep_term_function = NULL;
    rl_prep_term_function = NULL;

    // Prevent readline from setting the LINES and COLUMNS environment
    // variables, which override dynamic size adjustments in ncurses. When
    // using the alternate readline interface (as we do here), LINES and
    // COLUMNS are not updated if the terminal is resized between two calls to
    // rl_callback_read_char() (which is almost always the case).
    rl_change_environment = 0;

    // Handle input by manually feeding characters to readline
    rl_getc_function = readline_getc;
    rl_input_available_hook = readline_input_avail;
    rl_redisplay_function = readline_redisplay;

    rl_callback_handler_install("> ", got_command);
}
static void deinit_readline(void)
{
    rl_callback_handler_remove();
}
static void resizeMain(menuC* menu) {
    clear();
    menu->h = LINES-2;
    menu->w =  COLS/2-1;
    printmenu(menu);
    printTagInfoHeader();
    printTagInfo(menu);
    printStatus();
    move(1,COLS/2);
    vline( ACS_VLINE, LINES-2)  ;
}
static void resize(void)
{
    if (LINES >= 3) {
        CHECK(wresize, sep_win, 1, COLS);
        CHECK(wresize, cmd_win, 1, COLS);

        CHECK(mvwin, sep_win, LINES - 2, 0);
        CHECK(mvwin, cmd_win, LINES - 1, 0);
    }

    // Batch refreshes and commit them with doupdate()
    CHECK(wnoutrefresh, sep_win);
    cmd_win_redisplay(true);
    CHECK(doupdate);
}
void readline_n(void) {
    curs_set(2);
    resize();
    while (!should_exit) {
        // Using getch() here instead would refresh stdscr, overwriting the
        // initial contents of the other windows on startup
        int c = wgetch(cmd_win);

        if( c == '\n')
            should_exit = 1;
        if (c == KEY_RESIZE)
            resize();
        else if (c == '\f') { // Ctrl-L -- redraw screen.
            // Makes the next refresh repaint the screen from scratch
            CHECK(clearok, curscr, TRUE);
            // Resize and reposition windows in case that got messed up
            // somehow
            resize();
        }
        else
            forward_to_readline(c);
    }
    should_exit = 0;
}
void edit_rl(menuC* menu,void func(TagLib_Tag *, const char*), int c) {
    status[1]=c;
    printStatus();
    refresh();
    curs_set(2);
    move(LINES-1, 0);
    readline_n();
    clear();
    curs_set(0);
    itemC* it = menu->list+menu->hl;
    if( msg_win_str) {
        int selected = 0;
        for(int i = 0 ; i < menu->nbElem ; i++)
            if( menu->list[i].selected )
                selected = 1;
        if( ! selected && ! it->opt ) {
            mvprintc(20, 20, "H$1e$2l$3l$4o", 10);
            func(it->info.tag, msg_win_str);
            taglib_file_save(it->info.file);
        }else{
            for(int i = 0 ; i < menu->nbElem ; i++)
                if( menu->list[i].selected && !menu->list[i].opt ) {
                    func(menu->list[i].info.tag, msg_win_str);
                    taglib_file_save(menu->list[i].info.file);
                }
        }
    }
}

#define MAX_ERROR_MSG 0x1000

/* Compile the regular expression described by "regex_text" into
   "r". */

static int compile_regex (regex_t * r, const char * regex_text)
{
    int status = regcomp (r, regex_text, REG_EXTENDED|REG_NEWLINE);
    if (status != 0) {
	char error_message[MAX_ERROR_MSG];
	regerror (status, r, error_message, MAX_ERROR_MSG);
        printf ("Regex error compiling '%s': %s\n",
                 regex_text, error_message);
        return 1;
    }
    return 0;
}

/*
  Match the string in "to_match" against the compiled regular
  expression in "r".
 */
static int match_regex (const char* rS, const char * to_match, char* m2[], int nbR)
{
    regex_t* r = malloc(sizeof *r);
    compile_regex(r, rS);

    fprintf(stderr, "%d - %d\n", nbR, r->re_nsub);

    if( nbR != r->re_nsub )
        return 0;
    regmatch_t m[r->re_nsub];

    /* "P" is a pointer into the string which points to the end of the
       previous match. */
    /* "N_matches" is the maximum number of matches allowed. */
    /* "M" contains the matches found. */
    int rs = regexec (r, to_match /* if only 1 match, set it to to_match */,
                    r->re_nsub+1 /* nbMatch Max */, m /*  res */, 0);
    
    // regexec(&re, line, 2, rm, 0)
    fprintf(stderr,"<<%s>>\n", to_match);
    fprintf(stderr,"Line: <<%.*s>>\n", (int)(m[0].rm_eo - m[0].rm_so), to_match + m[0].rm_so);
    for(int i = 1 ; i < (r->re_nsub +1) ; i++) {
        m2[i-1] = malloc( (unsigned int)(m[i].rm_eo - m[i].rm_so) + 1);
        m2[i-1][m[i].rm_eo - m[i].rm_so] = 0;
        strncpy(m2[i-1], to_match+m[i].rm_so, m[i].rm_eo - m[i].rm_so);
        fprintf(stderr,"Text: <<%.*s>>\n", (int)(m[i].rm_eo - m[i].rm_so), to_match + m[i].rm_so);
    }

        /*
    {
    if( !rs ) 
        free(r);
    }
    */
    return 1;

    /*
    while (1) {
        int i = 0;
        int nomatch = regexec (r, p, n_matches, m, 0);
        if (nomatch) {
            printf ("No more matches.\n");
            return nomatch;
        }
        for (i = 0; i < n_matches; i++) {
            int start;
            int finish;
            if (m[i].rm_so == -1) {
                break;
            }
            start = m[i].rm_so + (p - to_match);
            finish = m[i].rm_eo + (p - to_match);
            if (i == 0) {
                printf ("$& is ");
            }
            else {
                printf ("$%d is ", i);
            }
            printf ("'%.*s' (bytes %d:%d)\n", (finish - start),
                    to_match + start, start, finish);
        }
        p += m[0].rm_eo;
    }
    return 0;
    */
}
void regexSelection(menuC* menu) {
    status[1]='s';
}
void regexXtract(itemC* it) {
    status[1]='x';

    readline_n();
    clear();
    curs_set(0);
    char* tmp = strrchr( msg_win_str , '/' );
    if( !msg_win_str || !*msg_win_str || !tmp) {
        return;
    }

    char *regexS = malloc(tmp-msg_win_str+1);

    strncpy( regexS, msg_win_str, tmp-msg_win_str);
    regexS[tmp - msg_win_str] = 0;
    char* const line = it->cstr;

    char** tab = NULL;
    tmp++;
    int nbR = strlen(tmp);
    tab = malloc( sizeof *tab * (nbR+1));
    if( match_regex(regexS, line, tab, nbR ) ) {
        fprintf(stderr, "Hello");
        for(int i = 0 ; i < nbR ; i++) {
            fprintf(stderr, "<<%s>>\n", tab[i]);
            switch(tmp[i]) {
                case 't':
                    taglib_tag_set_title(it->info.tag, tab[i]);
                    break;
                case 'a':
                    taglib_tag_set_artist(it->info.tag, tab[i]);
                    break;
                case 'b':
                    taglib_tag_set_album(it->info.tag, tab[i]);
                    break;
                case 'y':
                    break;
                case 'n':
                    break;
                case 'g':
                    taglib_tag_set_genre(it->info.tag, tab[i]);
                    break;
                case 'c':
                    taglib_tag_set_comment(it->info.tag, tab[i]);
                    break;
                default:
                    ;
            }
            taglib_file_save(it->info.file);
        }
    }
}
void loadMenu(itemC** menuL, int* size) {

}
int main(int argc, char* argv[]){
    itemC* menuL = NULL;
    int size = 0;
    int c;
    int comp = 0;

    setlocale(LC_ALL, "");
    initscr();
    if (has_colors()) {
        CHECK(start_color);
    }
    CHECK(cbreak);
    CHECK(noecho);
    CHECK(nonl);
    CHECK(intrflush, NULL, FALSE);
    visual_mode = true;
    taglib_set_strings_unicode(1);
    curs_set(0);

	cmd_win = newwin(1, COLS, LINES - 20, 0);
	sep_win = newwin(1, COLS, LINES - 2, 0);

    init_pair( 12, COLOR_WHITE, COLOR_BLUE);
    CHECK(wbkgd, sep_win, COLOR_PAIR(12));
    CHECK(wrefresh, sep_win);
    init_readline();

    for(int i = 0 ; i < 9 ; i++) {
        init_pair( i+1, i, COLOR_BLACK);
    }

    if( !argv[1] ) {
        listdir(0,  &menuL, &size);
    } else {
        chdir(argv[1]);
        listdir(0,&menuL, &size);
        puts(argv[1]);
    }
    qsort(menuL, size, sizeof *menuL, sort_i);

    menuC menu = (menuC) { menuL, size, 0, 0, 0, NULL, NULL, 1, 1, COLS/2-1, LINES-2};



    printmenu(&menu);
    printTagInfoHeader();
    move(1,COLS/2);
    vline( ACS_VLINE, LINES-2)  ;


    while( c = getch() ) {
        if (c == KEY_RESIZE) {
            resizeMain(&menu);
            continue;
        }
        switch( comp ) {
            case 0:
        switch (c) {
            case KEY_SF:
            case 'j':
                if( menu.hl < menu.nbElem-1 )
                    menu.hl++;
                printTagInfo(&menu);
                break;
            case KEY_SR:
            case 'k':
                if( menu.hl > 0)
                    menu.hl--;
                printTagInfo(&menu);
                break;
            case KEY_SLEFT:
                break;
            case KEY_SRIGHT:
                break;
            case 'e':
                if( !status[0] ) {
                    status[0] = 'e';
                    comp = 'e';
                }else{
                    status[0] = 0;
                }
                break;
            case ' ':
                menu.list[menu.hl].selected = !menu.list[menu.hl].selected;
                break;
            case 's':
                regexSelection(&menu);
                break;
            case 'x':
                regexXtract(&menu.list[menu.hl]);
                resizeMain(&menu);
                break;
            case 'q':
                goto end;
            case '\r':
                if( menu.list[menu.hl].opt ) {
                    int n = strlen(menu.list[menu.hl].cstr+1);
                    char* s = menu.list[menu.hl].cstr+1;
                    s[n-1] = 0;
                    chdir(s);
                    s[n-1] = ']';
                    cleanmenu(&menu);

                    listdir(0,&menuL, &size);
                    qsort(menuL, size, sizeof *menuL, sort_i);
                    menu.list = menuL;
                    menu.nbElem = size;
                    menu.hl = 0;
                }
            default:;
//                fprintf(stderr, "[%c]", c);
        }
        break;
            case 'e':
                echo();
                move(LINES-1,1);
            switch(c) {
                case 't':
                    edit_rl(&menu,taglib_tag_set_title,'t');
                    break;
                case 'a':
                    edit_rl(&menu,taglib_tag_set_artist,'a');
                    break;
                case 'b':
                    edit_rl(&menu,taglib_tag_set_album,'b');
                    break;
                case 'y':
                    break;
                case 'n':
                    break;
                case 'g':
                    edit_rl(&menu,taglib_tag_set_genre,'g');
                    break;
                case 'c':
                    edit_rl(&menu,taglib_tag_set_comment,'c');
                    break;
                default:
                    ;
            }
            comp = 0;
            status[0] = status[1] = 0;

            printmenu(&menu);
            printTagInfoHeader();
            printTagInfo(&menu);
        break;
        }
        printStatus();
        printmenu(&menu);
        move(1,COLS/2);
        vline( ACS_VLINE, LINES-2)  ;
        refresh();
    } 

    refresh();
    getch();
end:
    endwin();

    return 0;
}



