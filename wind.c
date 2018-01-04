/*
 * Ache - 2017-08-14 - GPLv3
 */

#include "wind.h"

/* Managed windows */



static short my_fg = COLOR_WHITE;
static short my_bg = COLOR_BLACK;
extern char status[10];
extern WINDOW *cmd_win;
extern WINDOW *sep_win;


void prepare(char* s) {
    refresh();
    strcpy(status, s);
    printStatus();
    readline_n();
    clear();
    curs_set(0);
}

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
void printHelp() {
    attrset( COLOR_PAIR( COLOR_RED + 1) | A_BOLD);
    mvprintc(COLS/2+2, 10, "j : Down",20);
    mvprintc(COLS/2+2, 11, "k : Up",20);
    mvprintc(COLS/2+2, 12, "et: Edit title",20);
    mvprintc(COLS/2+2, 13, "ea: Edit artist",20);
    mvprintc(COLS/2+2, 14, "eb: Edit album",20);
    mvprintc(COLS/2+2, 15, "ey: Edit year",20);
    mvprintc(COLS/2+2+24, 10, "en: Edit track N°",20);
    mvprintc(COLS/2+2+24, 11, "et: Edit genre",20);
    mvprintc(COLS/2+2+24, 12, "s : Regex selection",20);
    mvprintc(COLS/2+2+24, 13, "x : Regex edition",20);
    mvprintc(COLS/2+2+24, 14, "q : Quit",20);
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
        menu->firstElem-=menu->h/2;
        if( menu->firstElem < 0 )
            menu->firstElem = 0;

        cleanmenu(menu);
    }

    if( menu->hl >= (menu->firstElem+menu->h ) ) {
        attrset(0 | A_NORMAL );
        menu->firstElem+=menu->h/2;
        if( menu-> firstElem > menu->nbElem )
            menu-> firstElem = menu->nbElem;
        cleanmenu(menu);
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


void resize(void)
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

void resizeMain(menuC* menu) {
    clear();
    menu->h = LINES-2;
    menu->w =  COLS/2-1;
    printmenu(menu);
    printTagInfoHeader();
    printHelp();
    printTagInfo(menu);
    printStatus();
    move(1,COLS/2);
    vline( ACS_VLINE, LINES-2)  ;
}

