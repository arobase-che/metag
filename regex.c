#include "regex.h"

/* Manage regex */


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

    if( nbR != r->re_nsub && nbR > 0)
        return 0;
    regmatch_t m[r->re_nsub+1];

    /* "P" is a pointer into the string which points to the end of the
       previous match. */
    /* "N_matches" is the maximum number of matches allowed. */
    /* "M" contains the matches found. */
    int rs = regexec (r, to_match /* if only 1 match, set it to to_match */,
                    nbR > 0 ? r->re_nsub+1: 0 /* nbMatch Max */, m /*  res */, 0);
    
    // regexec(&re, line, 2, rm, 0)
    if( !rs )  {
        fprintf(stderr,"<<%s>>\n", to_match);
//        fprintf(stderr,"Line: <<%.*s>>\n", (int)(m[0].rm_eo - m[0].rm_so), to_match + m[0].rm_so);
    }

    if( m2 )
        for(int i = 1 ; i < (r->re_nsub +1) ; i++) {
            m2[i-1] = malloc( (unsigned int)(m[i].rm_eo - m[i].rm_so) + 1);
            m2[i-1][m[i].rm_eo - m[i].rm_so] = 0;
            strncpy(m2[i-1], to_match+m[i].rm_so, m[i].rm_eo - m[i].rm_so);
            fprintf(stderr,"Text: <<%.*s>>\n", (int)(m[i].rm_eo - m[i].rm_so), to_match + m[i].rm_so);
        }
    return !rs;

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
void regexSelection(menuC* menu, const char* msg) {
    for(int i = 0 ; i < menu->nbElem ; i++ ) {
        if( match_regex(msg, menu->list[i].cstr, NULL, -1 ) ) {
            menu->list[i].selected = 1;
        }
    }
}
void regexXtracts(menuC* menu, const char* msg) {
    int hasSelect = 0;
    for(int i = 0 ; i < menu->nbElem ; i++ ) {
        if( menu->list[i].selected == 1 ) {
            regexXtract(&(menu->list[i]), msg);
            hasSelect = 1;
        }
    }
    if( !hasSelect ) {
        regexXtract(&menu->list[menu->hl], msg);
    }

}
/*
 * Example regex : 
 *
 *       ^([a-Z0-9 ]+) - ([^\[]+)\[([0-9]+)\]/atn
 *       (Daft Punk) - Something about us[11].mp3

 */
void regexXtract(itemC* it, const char* msg) {
    if( !msg || !*msg) {
        return;
    }
    char* tmp = strrchr( msg , '/' );
    if( !tmp ) return;

    char *regexS = malloc(tmp-msg+1);

    strncpy( regexS, msg, tmp-msg);
    regexS[tmp - msg] = 0;
    char* const line = it->cstr;

    char** tab = NULL;
    tmp++;
    int nbR = strlen(tmp);
    tab = malloc( sizeof *tab * (nbR+1));
    if( match_regex(regexS, line, tab, nbR ) ) {
#ifdef DEBUG
        fprintf(stderr, "Hello");
#endif
        for(int i = 0 ; i < nbR ; i++) {
#ifdef DEBUG
            fprintf(stderr, "<<%s>>\n", tab[i]);
#endif
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
                    {
                        unsigned int t = 0;
                        int r = sscanf(tab[i], "%u", &t);
                        if( r == 1)
                            taglib_tag_set_year(it->info.tag, t);
                    break;
                    }
                case 'n':
                    {
                        unsigned int t = 0;
                        int r = sscanf(tab[i], "%u", &t);
                        if( r == 1)
                            taglib_tag_set_track(it->info.tag, t);
                    }
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

