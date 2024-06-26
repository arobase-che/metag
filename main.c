/*
 * Ache - 2018-10-21 - GPLv3 or later
 */

#include "main.h"
#include "regex.h"
#include "readline.h"

#include <dirent.h>
#include <stdnoreturn.h>
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>

#include "item.h"
#include "wind.h"

/*
 * Entry point of metag and event loop
 */


#ifdef DEBUG

#include <assert.h>

#endif

const int color[] = {
    COLOR_BLACK, COLOR_RED,
    COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
    COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE};





#define MAX_PATH 1024



extern char* msg_win_str;

static bool visual_mode = false;
WINDOW *cmd_win;
WINDOW *sep_win;
char status[10];


void fail_exit(const char *msg) {
    if (visual_mode)
        endwin();
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}


void edit_rl(menuC* menu,void func(TagLib_Tag *, const char*), int c, const char* str);

void edit_rl(menuC* menu,void func(TagLib_Tag *, const char*), int c, const char* str) {
    status[1]=c;
    printStatus();
    refresh();
    curs_set(2);
    move(LINES-1, 0);
    readline_n(str);
    clear();
    curs_set(0);
    itemC* it = menu->list+menu->hl;
    if( msg_win_str) {
        int selected = 0;
        for(int i = 0 ; i < menu->nbElem ; i++)
            if( menu->list[i].selected )
                selected = 1;
        if( ! selected && ! it->opt ) {
#ifdef DEBUG
            mvprintc(20, 20, "H$1e$2l$3l$4o", 10);
#endif
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
void edit_rl_int(menuC* menu,void func(TagLib_Tag *, unsigned int), int c, int oldOne_int) {
    status[1]=c;
    printStatus();
    refresh();
    curs_set(2);
    move(LINES-1, 0);
    char oldOne[5];
    sprintf(oldOne, "%d", oldOne_int);
    readline_n(oldOne);
    clear();
    curs_set(0);
    itemC* it = menu->list+menu->hl;

    int res = -1;
    {
        int t = 0;
        int r = sscanf(msg_win_str, "%u", &t);
        if( r == 1 )
            res = t;
        else {
            return;
        }



    }

    if( msg_win_str) {
        int selected = 0;
        for(int i = 0 ; i < menu->nbElem ; i++)
            if( menu->list[i].selected )
                selected = 1;
        if( ! selected && ! it->opt ) {
#ifdef DEBUG
            mvprintc(20, 20, "H$1e$2l$3l$4o", 10);
#endif
            func(it->info.tag, res);
            taglib_file_save(it->info.file);
        }else{
            for(int i = 0 ; i < menu->nbElem ; i++)
                if( menu->list[i].selected && !menu->list[i].opt ) {
                    func(menu->list[i].info.tag, res);
                    taglib_file_save(menu->list[i].info.file);
                }
        }
    }
}

int main(int argc, char* argv[]){

    /* Gestion des paramètres */
    while (1) {
        int c;
        int optIndex = 0;
        static struct option optlv[] = {
            {"help",     no_argument,           0,  'h' },
            {"version",  no_argument,           0,  'v'  },
            {"dir",    required_argument,       0,  'd' },
            {NULL,       0,                     0,   0  }
        };


        c = getopt_long(argc, argv, "hvd:", optlv, &optIndex);
        if (c == -1)
            break;

        switch (c) {
            case 'v':
                puts( VERSION_METAG );
                return EXIT_SUCCESS;
            break;

            case 'h':
                puts( HELP_STRING_METAG );
                return EXIT_SUCCESS;

            break;
            case 'd':
                if( optarg ) {
                    chdir(optarg);
                } else {
                    fprintf(stderr, "Veuillez indiquer un directory\n");
                }
            break;
            case '?':
            default:
                return EXIT_FAILURE;
        }
    }




    /* Initialisation */
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

    listdir(0,  &menuL, &size);
    qsort(menuL, size, sizeof *menuL, sort_i);

    menuC menu = (menuC) { menuL, size, 0, 0, 0, NULL, NULL, 1, 1, COLS/2-1, LINES-2};


    /* Premier affichage */

    printmenu(&menu);
    printTagInfoHeader();
    printHelp();
    move(1,COLS/2);
    vline( ACS_VLINE, LINES-2)  ;


    /* Event loop */
    while( c = getch() ) {
        if (c == KEY_RESIZE) {
            resizeMain(&menu);
            continue;
        }
        switch( comp ) {
            case 0:
        switch (c) {
            case 'B':
            case 'j': // Up
                if( menu.hl < menu.nbElem-1 )
                    menu.hl++;
                printTagInfo(&menu);
                break;
            case 'A':
            case 'k': // Down
                if( menu.hl > 0)
                    menu.hl--;
                printTagInfo(&menu);
                break;
            case KEY_SLEFT:
                break;
            case KEY_SRIGHT:
                break;
            case 'e': // Edit mode
                if( !status[0] ) {
                    status[0] = 'e';
                    comp = 'e';
                }else{
                    status[0] = 0;
                }
                break;
            case ' ': // Select under cursor mode
                menu.list[menu.hl].selected = !menu.list[menu.hl].selected;
                break;
            case 's': // Select by regex mode
                prepare("xs", "");
                regexSelection(&menu,msg_win_str);
                resizeMain(&menu);
                break;
            case '/': // Search mode
                prepare("/", "");
                regexSearch(&menu, msg_win_str);
                resizeMain(&menu);
                break;
            case 'x': // Regex mode
                prepare("x", "");
                regexXtracts(&menu, msg_win_str);
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

                    freelitem(menuL, size);
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
            case 'e': {
                echo();
                move(LINES-1,1);
                itemC* it = menu.list+menu.hl;
                switch(c) {
                    case 't':
                        edit_rl(&menu,taglib_tag_set_title,'t', taglib_tag_title(it->info.tag));
                        break;
                    case 'a':
                        edit_rl(&menu,taglib_tag_set_artist,'a', taglib_tag_artist(it->info.tag));
                        break;
                    case 'b':
                        edit_rl(&menu,taglib_tag_set_album,'b', taglib_tag_album(it->info.tag));
                        break;
                    case 'y':
                        edit_rl_int(&menu,taglib_tag_set_year,'y', taglib_tag_year(it->info.tag));
                        break;
                    case 'n':
                        edit_rl_int(&menu,taglib_tag_set_track,'n', taglib_tag_track(it->info.tag));
                        break;
                    case 'g':
                        edit_rl(&menu,taglib_tag_set_genre,'g', taglib_tag_genre(it->info.tag));
                        break;
                    case 'c':
                        edit_rl(&menu,taglib_tag_set_comment,'c', taglib_tag_comment(it->info.tag));
                        break;
                    default:
                        ;
                }
                comp = 0;
                status[0] = status[1] = 0;

                printmenu(&menu);
                printTagInfoHeader();
                printTagInfo(&menu);
            }
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



