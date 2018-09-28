/*
 * Ache - 2017-08-14 - GPLv3
 */

#include "item.h"
#include<errno.h>

/*
 * Set of functions to manage a list of items (One file or directory is
 * an item).
 */



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
void freelitem( itemC* m, int s) {
    for(int i = 0 ; i < s ; i++) {
        free(m[i].cstr);
        if( !m[i].opt) {
                taglib_file_free(m[i].info.file);
        }
    }

    taglib_tag_free_strings();
    free(m);
}
void listdir(int option, itemC** m, int* s) {
    DIR *dir;
    int nbitem = 0;
    itemC* menu = NULL;
    struct dirent *entry;


    if (!(dir = opendir("."))) {
        perror("opendir failled");
        return;
    }
    errno = 0;

    if (!(entry = readdir(dir)))
        return;



    do {
        if(errno) {
            break;
        }

        if( entry->d_name[0] == '.' && !(option & HIDDEN) && strcmp(entry->d_name, "..") )
            continue;

// QUICK FIX OF TAGLIB
        if( strchr(entry->d_name, '#') )
            continue;
#ifdef DEBUG
        mvprintc(1,1,entry->d_name,COLS/2-5);
#endif



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
                menu[nbitem-1].opt  = 2;
            }else{
                menu[nbitem-1].opt  = 0;
            }
            menu[nbitem-1].cstr = strdup(entry->d_name);
            menu[nbitem-1].id   = nbitem-1;
            menu[nbitem-1].info = (tagInfo){file, tag, properties};
            menu[nbitem-1].selected   = 0;

        }
    } while (entry = readdir(dir));
end:

    closedir(dir);
    *m = menu;
    *s = nbitem;
    refresh();
}


