#include "readline.h"


/* 
 * Sets of functions to read a line with readline library and ncurses together
 */


static bool input_avail = false;
static unsigned char input;
static bool should_exit = false;
char *msg_win_str = NULL;
extern WINDOW *cmd_win;


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

void cmd_win_redisplay(bool for_resize)
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


void init_readline(void)
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


void readline_n(const char* str) {
    curs_set(2);
    resize();
    for( ; *str ; str++ ) {
        if( *str != '\n' && *str != '\f' && *str != KEY_RESIZE ) {
            forward_to_readline(*str);
        }
    }
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

