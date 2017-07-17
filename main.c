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


void edit_rl(menuC* menu,void func(TagLib_Tag *, const char*), int c);

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
void edit_rl_int(menuC* menu,void func(TagLib_Tag *, unsigned int), int c) {
    status[1]=c;
    printStatus();
    refresh();
    curs_set(2);
    move(LINES-1, 0);
    readline_n();
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
            if( !strcmp(optlv[optIndex].name, "version") ) {
                puts( VERSION_METAG );
            }
            break;

            case 'h':
                puts( HELP_STRING_METAG );
                return EXIT_SUCCESS;

            break;
            case 'c':
                if( optarg ) {

                } else {
                    fprintf(stderr, "Veuillez indiquer un directory\n");
                }
            break;
            case '?':
            default:
                return EXIT_FAILURE;
        }
    }




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
                prepare("xs");
                regexSelection(&menu,msg_win_str);
                resizeMain(&menu);
                break;
            case 'x':
                prepare("x");
                regexXtract(&menu.list[menu.hl], msg_win_str);
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
                    edit_rl_int(&menu,taglib_tag_set_year,'b');
                    break;
                case 'n':
                    edit_rl_int(&menu,taglib_tag_set_track,'b');
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



