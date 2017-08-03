
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

//
// Initial lexer.
//

#include "config.h"
#include "cshell.h"
#include "commands.h"
#include "frontend.h"
#include "kwords_fte.h"
#include "pathlist.h"
#include "spglobal.h"
#include "outplot.h"
#include "graphics.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef WIN32
#include <winsock2.h>
#include <process.h>
#else
#include <sys/socket.h>
#include <sys/ioctl.h>
#endif
#include <sys/stat.h>
#include <fcntl.h>


// If defined, a line editing option similar to the GNU readline() will
// be included.
//
#define USE_RDLINE

#ifdef HAVE_TERM_H
#include <curses.h>
#include <term.h>
#undef TTY
#else
#ifdef HAVE_TGETENT
#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#endif
#endif
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#else
#ifdef HAVE_TERMIO_H
#include <termio.h>
#else
#ifdef HAVE_SGTTY_H
#include <sgtty.h>
#endif
#endif
#endif

#ifndef TAB3
#define TAB3 0
#endif
#ifndef ECHOCTL
#define ECHOCTL 0
#endif

namespace { const char *eofmsg = "%s: received EOF, connection closed\n"; }

struct lterm
{
    lterm(int li, int bc, int ec, lterm *n)
        {
            lineno = li;
            begincol = bc;
            endcol = ec;
            next = n;
        }

    int lineno;
    int begincol;
    int endcol;
    lterm *next;
};

#define LEX_MAXWORD 16536 // Max input word size
#define LEX_MAXLINE 16536 // Max input line length

struct sLx
{
    const char *initialize();
    void terminate();
    char *rdline(int);
    void addchars(const char*);
    int  stuffchar(int);
    void ttyecho(int);
    int  colcnt(int);
    int  fcpos(int);
    void cur_adv(int);
    void cur_rev(int);
    void cte(int);
    void cpos_adv(bool);
    void cpos_rev();
    void mv_l();
    void mv_d();
    void mv_r();
    void mv_u();

    // methods:
    // rdline():     Obtain a line of input (main routine)
    // addchars();   Add char string to input buffer at current position
    // stuffchar():  Process returned character
    // ttyecho():    Echo input to screen
    // colcnt():     Return screen width of char at given index
    // fcpos():      Return column position at end of given char count
    // cpos_adv():   Advance cursor explicitly (right arrow)
    // cpos_rev():   Back cursor explicitly (left arrow)
    // cur_adv():    Advance cursor properly for given char
    // cur_rev():    Back cursor properly for given char
    // cte():        Clear to end of line

    const char *Terminal;         // terminal name
    const char *Tle;
    const char *Tdo;
    const char *Tnd;
    const char *Tup;              // cursor motion excapes
    int Cend;                     // end column of text, logical
    int Cpos;                     // cursor position, logical
    int Ccpos;                    // cursor position, character
    int PromptLen;                // length of prompt string
    int ScrCols;                  // number of screen colums per line
    sHistEnt *Histent;            // history list entry in buffer
    const char *LastLine;         // last text line before history switch
    lterm *LtBase;                // base for line table
    char *LineBuf;                // character buffer used for input
    bool QuoteNext;               // the next char is taken literally
};

namespace { sLx Lx; }


// Assumed max length of an escape sequence returned from key press
#define MAX_RSP 5


#define KC_BASE 0x10000
enum KCode
{
    KC_CA = KC_BASE,
    KC_CD,
    KC_CE,
    KC_CK,
    KC_CU,
    KC_CV,
    KC_TAB,
    KC_BSP,
    KC_DEL,
    KC_L,
    KC_R,
    KC_U,
    KC_D,
    KC_foo
};

#ifndef WIN32

struct sKmap
{
    struct kmap
    {
        kmap(const char *f, KCode c) { from = f; to = c; }

        const char *from;
        KCode to;
    };

    const char *km_string(KCode c) { return (maps[c - KC_BASE].from); }
    void km_set(const char *f, KCode c) { maps[c - KC_BASE] = kmap(f, c); }

    void km_mapkey(wordlist*);
    kmap *km_match(const char*);
    int km_possible(const char*);
    void dump_kmaps(FILE*);
    void read_kmaps(FILE*);

private:
    static const char *km_word(KCode);
    static int km_code(const char*);

    static kmap maps[];
};

namespace { sKmap Kmap; }


// Order must match KCode enum.
sKmap::kmap sKmap::maps[] = 
{
    sKmap::kmap( "\x01",    KC_CA ),
    sKmap::kmap( "\x04",    KC_CD ),
    sKmap::kmap( "\x05",    KC_CE ),
    sKmap::kmap( "\x0b",    KC_CK ),
    sKmap::kmap( "\x15",    KC_CU ),
    sKmap::kmap( "\x16",    KC_CV ),
    sKmap::kmap( "\t",      KC_TAB ),
    sKmap::kmap( "\x08",    KC_BSP ),
    sKmap::kmap( "\x7f",    KC_DEL ),
    sKmap::kmap( "\x1b""[D", KC_L ),
    sKmap::kmap( "\x1b""[C", KC_R ),
    sKmap::kmap( "\x1b""[A", KC_U ),
    sKmap::kmap( "\x1b""[B", KC_D ),
    sKmap::kmap( 0,           KC_foo )
};

void
sKmap::km_mapkey(wordlist *wl)
{
    if (!wl || !wl->wl_word) {
        // No args, prompt for chars and create new map.
        for (kmap *map = maps; map->from; map++) {
            printf("Please press %s: ", km_word(map->to));
            fflush(stdout);
            char cbuf[MAX_RSP + 1];
            memset(cbuf, 0, sizeof(cbuf));
            cbuf[0] = CP.Getchar(fileno(stdin), true, false);
            int i = 1, c;
            while ((c = CP.Getchar(fileno(stdin), true, true)) != 0) {
                if (i < MAX_RSP)
                    cbuf[i] = c;
                i++;
            }
            printf("\n");
            if (i >= MAX_RSP)
                GRpkgIf()->ErrPrintf(ET_WARN, "%s map too long, truncated\n",
                    km_word(map->to));
            km_set(lstring::copy(cbuf), map->to);
        }
        return;
    }
    if (lstring::eq(wl->wl_word, "-w")) {
        // Dump internal map to file.
        const char *fname = wl->wl_next ? wl->wl_next->wl_word : "wrs_keymap";
        char *path = pathlist::expand_path(fname, true, true);
        FILE *fp = fopen(path, "w");
        delete [] path;
        if (!fp) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "can't open %s.\n", fname);
            return;
        }
        dump_kmaps(fp);
        fclose(fp);
        return;
    }
    if (lstring::eq(wl->wl_word, "-r")) {
        // Read map file, set internal map.
        const char *fname = wl->wl_next ? wl->wl_next->wl_word : "wrs_keymap";
        char *path = pathlist::expand_path(fname, true, true);
        FILE *fp = fopen(path, "r");
        delete [] path;
        if (!fp) {
            if (!lstring::is_rooted(fname)) {
                path = pathlist::mk_path(Global.StartupDir(), fname);
                fp = fopen(path, "r");
                delete [] path;
            }
        }
        if (!fp) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "can't open %s.\n", fname);
            return;
        }
        read_kmaps(fp);
        fclose(fp);
        return;
    }

    // Map one key.
    int code = km_code(wl->wl_word);
    if (!code) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "unknown key %s.\n", wl->wl_word);
        return;
    }
    if (!wl->wl_next) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no mapping given.\n");
        return;
    }
    char cbuf[MAX_RSP + 1];
    int n = 0;
    for (wl = wl->wl_next; wl; wl = wl->wl_next) {
        int x;
        if (sscanf(wl->wl_word, "%x", &x) != 1 || x < 0 || x > 255) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad mapping given.\n");
            return;
        }
        cbuf[n++] = x;
        if (n == MAX_RSP)
            break;
    }
    cbuf[n] = 0;
    km_set(lstring::copy(cbuf), (KCode)code);
}


sKmap::kmap *
sKmap::km_match(const char *str)
{
    kmap *match = 0;
    int plen = 0;
    for (kmap *k = maps; k->from; k++) {
        if (lstring::prefix(k->from, str)) {
            int l = strlen(k->from);
            if (l > plen) {
                plen = l;
                match = k;
            }
        }
    }

#ifndef WIN32
    // For xterms and gnome-terminals, sometimes the Bsp key sends ^H
    // (on FreeBSD) and sometimes 0x7f (RHEL5).  The keycodes are set
    // on the system running WRspice, which might not match if running
    // remotely using X.  Here, if either key code is unmapped, map it
    // to the backspace operation, which should fix this issue when
    // running on an xterm.
    //
    // Note that this should be safe, since if 0x7f is used for DEL, it
    // will be mapped and therefor not re-mapped.
    //
    if (!match && *str == 0x8)
        match = &maps[KC_BSP - KC_BASE];
    if (!match && *str == 0x7f)
        match = &maps[KC_BSP - KC_BASE];
#endif
    return (match);
}


int
sKmap::km_possible(const char *str)
{
    int poss = 0;
    for (kmap *k = maps; k->from; k++) {
        if (lstring::prefix(str, k->from))
            poss++;
    }
    return (poss);
}


void
sKmap::dump_kmaps(FILE *fp)
{
    fprintf(fp, "#WRspice Key Map File\n");
    for (kmap *k = maps; k->from; k++) {
        fprintf(fp, "%s", km_word(k->to));
        for (const char *s = k->from; *s; s++)
            fprintf(fp, " %x", *s);
        fprintf(fp, "\n");
    }
}


void
sKmap::read_kmaps(FILE *fp)
{
    char buf[256];
    while (fgets(buf, 256, fp) != 0) {
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!isalpha(*s))
            continue;
        int code = km_code(s);
        if (!code)
            continue;
        lstring::advtok(&s);
        char cbuf[MAX_RSP + 1];
        cbuf[0] = 0;
        int n = 0;
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0 && n < MAX_RSP) {
            int x;
            if (sscanf(tok, "%x", &x) != 1 || x < 0 || x > 255) {
                cbuf[0] = 0;
                delete [] tok;
                break;
            }
            cbuf[n++] = x;
            delete [] tok;
        }
        cbuf[n] = 0;
        if (!*cbuf)
            continue;
        km_set(lstring::copy(cbuf), (KCode)code);
    }
}


const char *
sKmap::km_word(KCode c)
{
    switch (c) {
    case KC_CA:     return ("Ctrl-a");
    case KC_CD:     return ("Ctrl-d");
    case KC_CE:     return ("Ctrl-e");
    case KC_CK:     return ("Ctrl-k");
    case KC_CU:     return ("Ctrl-u");
    case KC_CV:     return ("Ctrl-v");
    case KC_BSP:    return ("Backspace");
    case KC_DEL:    return ("Delete");
    case KC_TAB:    return ("Tab");
    case KC_L:      return ("LeftArrow");
    case KC_R:      return ("RightArrow");
    case KC_U:      return ("UpArrow");
    case KC_D:      return ("DownArrow");
    default:       break;   
    }
    return (0);
}


int
sKmap::km_code(const char *str)
{
    if (lstring::ciprefix("ctrl-a", str))
        return (KC_CA);
    if (lstring::ciprefix("ctrl-d", str))
        return (KC_CD);
    if (lstring::ciprefix("ctrl-e", str))
        return (KC_CE);
    if (lstring::ciprefix("ctrl-k", str))
        return (KC_CK);
    if (lstring::ciprefix("ctrl-u", str))
        return (KC_CU);
    if (lstring::ciprefix("ctrl-v", str))
        return (KC_CV);
    if (lstring::ciprefix("back", str) || lstring::ciprefix("bsp", str))
        return (KC_BSP);
    if (lstring::ciprefix("del", str))
        return (KC_DEL);
    if (lstring::ciprefix("tab", str))
        return (KC_TAB);
    if (lstring::ciprefix("left", str))
        return (KC_L);
    if (lstring::ciprefix("right", str))
        return (KC_R);
    if (lstring::ciprefix("up", str))
        return (KC_U);
    if (lstring::ciprefix("down", str))
        return (KC_D);
    return (0);
}

#endif // WIN32


namespace {
    void set_raw_mode(int, bool);
    void set_noedit_mode(int, bool);
}

#define ESCAPE  '\033'


// Wait for keypress (useful in scripts)
//
void
CommandTab::com_pause(wordlist*)
{
    if (!Sp.GetFlag(FT_SERVERMODE)) {
        FILE *fp;
        if (isatty(fileno(stdin)))
            fp = stdin;
        else
            fp = fopen("/dev/tty", "r");
        if (!fp)
            return;
        CP.RawGetc(fileno(fp));
        if (fp != stdin)
            fclose (fp);
        TTY.send("\n");
    }
}


// mapkey [ -r [filename] | -w [filename] | chname map... ]
void
CommandTab::com_mapkey(wordlist *wl)
{
#ifdef WIN32
    (void)wl;
    GRpkgIf()->ErrPrintf(ET_ERROR,
        "The mapkey commamd is not available under Windows.\n");
#else
    Kmap.km_mapkey(wl);
#endif
}
// End of CommandTab functions.


wordlist *
CshPar::PromptUser(const char *string)
{
    if (cp_flags[CP_NOTTYIO])
        return (0);
    char buf1[128];
    sprintf(buf1,  "%s: ",  string);
    char buf[LEX_MAXLINE];
    if (TTY.prompt_for_input(buf, sizeof(buf), buf1) == 0)
        return (0);
    char *c;
    if ((c = strchr(buf, '\n')) != 0)
        *c = '\0';
    wordlist *wl = new wordlist(buf, 0);
    CP.VarSubst(&wl);
    CP.BackQuote(&wl);
    CP.DoGlob(&wl);
    return (wl);
}


// Return a list of words, with backslash quoting and '' quoting done.
// Strings enclosed in "" or `` are made single words and returned, 
// but with the "" or `` still present. For the \ and '' cases, the
// 8th bit is turned on (as in csh) to prevent them from being recogized, 
// and stripped off once all processing is done. We also have to deal with
// command, filename, and keyword completion here.
// If string is non-0, then use it instead of the fp. Escape and EOF
// have no business being in the string.


namespace {
    inline wordlist *newword(wordlist *cw, char *buf)
    {
        cw->wl_word = lstring::copy(buf);
        cw->wl_next = new wordlist(0, cw);
        cw = cw->wl_next;
        memset(buf, 0, LEX_MAXWORD + 10);
        return (cw);
    }


    // Return true if c is one of the special characters included, which
    // break words
    //
    inline bool breakword(char c)
    {
        return (c == '<' || c == '>' || c == ';' || c == '&');
    }
}


//
// NOTE: in Win32, cp_noedit is always false.
//

#define FILENO(fp) ((fp) ? fileno(fp) : -1)

wordlist *
CshPar::Lexer(const char *string)
{
    if (cp_input == 0 && cp_flags[CP_INTERACTIVE])
        cp_input = TTY.infile();

    char linebuf[LEX_MAXLINE + 10];
    if (!string && cp_flags[CP_INTERACTIVE]) {
        if (!cp_flags[CP_NOEDIT]) {
            string = Lx.rdline(FILENO(cp_input));
            if (!string)
                return (0);
            wordlist *wlist = Lexer(string);
            delete [] string;
            return (wlist);
        }
        else
            Lx.LineBuf = linebuf;
    }

    wordlist *wlist = 0;
    char buf[LEX_MAXWORD + 10];
    bool nloop = true;
    while (nloop) {
        nloop = false;
        int i = 0;
        int j = 0;
        memset(linebuf, 0, LEX_MAXLINE+10);
        memset(buf, 0, LEX_MAXWORD+10);
        wordlist *cw = new wordlist;
        wlist = cw;
        bool done = false;
        while (!done) {
            int c;
            if (string) {
                c = *string++;
                if (c == '\0')
                    c = '\n';
                if (c == ESCAPE)
                    c = '[';
            }
            else
                c = Getchar(FILENO(cp_input), true);
            bool gotchar = true;
            while (gotchar) {
                gotchar = false;
                if ((c != EOF) && (c != ESCAPE) && (c != 4))
                    linebuf[j++] = c;
                if (i >= LEX_MAXWORD) {
                    GRpkgIf()->ErrPrintf(ET_WARN, "word too long.\n");
                    c = ' ';
                }
                if (j >= LEX_MAXLINE) {
                    GRpkgIf()->ErrPrintf(ET_WARN, "line too long.\n");
                    if (cp_bqflag)
                        c = EOF;
                    else
                        c = '\n';
                }
                if (c != EOF)
                    c = STRIP(c);   // Don't need to do this really

                // "backslash" quoting
                if (c == '\\' || c == '\026') { // ^V
                    bool foo = false;
                    if (string) {
                        c = *string++;
                        if (c == '\0') {
                            c = '\n';  // don't quote this
                            foo = true;
                        }
                    }
                    else
                        c = Getchar(FILENO(cp_input), true);
                    linebuf[j++] = c;
                    if (!foo)
                        c = QUOTE(c);
                }

                if ((c == '\n') && cp_bqflag)
                    c = ' ';
                if ((c == EOF || c == 4) && cp_bqflag)
                    c = '\n';
                if ((c == cp_hash) && !cp_flags[CP_INTERACTIVE] && (j == 1)) {
                    if (string) {
                        wordlist::destroy(wlist);
                        return (0);
                    }
                    while (((c = Getchar(FILENO(cp_input), true))
                            != '\n') && (c != EOF)) ;
                    nloop = true;
                    done = true;
                    break;
                }

                char d;
                switch (c) {

                case ' ':
                case '\t':
                    if (i > 0) {
                        cw = newword(cw, buf);
                        i = 0;
                    }
                    break;

                case '\n':
                    if (i) {
                        buf[i] = '\0';
                        cw->wl_word = lstring::copy(buf);
                    }
                    else if (cw->wl_prev) {
                        cw->wl_prev->wl_next = 0;
                        delete cw;
                        cw = 0;
                    }
                    else
                        cw->wl_word = lstring::copy("");
                    if (!string)
                        cp_flags[CP_WAITING] = false;
                    //done
                    done = true;
                    break;

                case '\'':
                    while (((c = (string ? *string++ :
                            Getchar(FILENO(cp_input), true)))
                            != '\'') && c && i < LEX_MAXWORD &&
                            j < LEX_MAXLINE) {
                        if (c == '\n' || c == EOF || c == ESCAPE || c == 0) {
                            if (c == 0 && string)
                                string--;
                            gotchar = true;
                            break;
                        }
                        else {
                            buf[i++] = QUOTE(c);
                            linebuf[j++] = c;
                        }
                    }
                    if (c == 0 && string)
                        string--;
                    if (!gotchar)
                        linebuf[j++] = '\'';
                    break;

                case '"':
                case '`':
                    d = c;
                    buf[i++] = d;
                    while (((c = (string ? *string++ :
                            Getchar(FILENO(cp_input), true))) != d)
                            && i < LEX_MAXWORD && j < LEX_MAXLINE) {
                        if (c == '\n' || c == EOF || c == ESCAPE || c == 0) {
                            if (c == 0 && string)
                                string--;
                            gotchar = true;
                            break;
                        }
                        else if (c == '\\') {
                            linebuf[j++] = c;
                            c = (string ? *string++ :
                                Getchar(FILENO(cp_input), true));
                            buf[i++] = QUOTE(c);
                            linebuf[j++] = c;
                        }
                        else {
                            buf[i++] = c;
                            linebuf[j++] = c;
                        }
                    }
                    if (!gotchar) {
                        buf[i++] = d;
                        linebuf[j++] = d;
                    }
                    break;

                case EOF:
                case 4: // ^D
                    if (cp_flags[CP_INTERACTIVE] && !cp_flags[CP_NOCC] &&
                            string == 0) {
                        Complete(wlist, buf, false);
                        wordlist::destroy(wlist);
                        nloop = true;
                        done = true;
                        break;
                    }
                    // EOF during a source
                    if (cp_flags[CP_INTERACTIVE]) {
                        TTY.out_printf("quit\n");
                        CommandTab::com_quit(0);
                        // done
                        done = true;
                        break;
                    }
                    wordlist::destroy(wlist);
                    return (0);

                case ESCAPE:
                    if (cp_flags[CP_INTERACTIVE] && !cp_flags[CP_NOCC]) {
                        Complete(wlist, buf, true);
                        wordlist::destroy(wlist);
                        nloop = true;
                        done = true;
                        break;
                    } // Else fall through

                default:
                    // We have to remember the special cases $<, $&, $#&, $?&
                    if (i > 0 && breakword(c)) {
                        switch (c) {
                        case '<':
                            if (buf[i-1] == '$')
                                break;
                            cw = newword(cw, buf);
                            i = 0;
                            break;

                        case '&':
                            if (buf[i-1] == '$')
                                break;
                            if (i > 1 && (buf[i-1] == '#' || buf[i-1] == '?')
                                    && buf[i-2] == '$')
                                break;
                            cw = newword(cw, buf);
                            i = 0;
                            break;

                        default:
                            cw = newword(cw, buf);
                            i = 0;
                        }
                    }
                    buf[i++] = c;

                    // have to also catch <=, >=, and <>
                    if (breakword(c)) {
                        switch (c) {
                        case '<':
                            if (i > 1 && buf[i-2] == '$')
                                break;
                            // fallthrough

                        case '>':
                            d = c;
                            if (string) {
                                c = *string++;
                                if (c == '\0')
                                    c = '\n';
                                if (c == ESCAPE)
                                    c = '[';
                            }
                            else
                                c = Getchar(FILENO(cp_input), true);

                            if (c == '=' || (d == '<' && c == '>')) {
                                buf[i++] = c;
                                linebuf[j++] = c;
                                cw = newword(cw, buf);
                                i = 0;
                                break;
                            }
                            cw = newword(cw, buf);
                            i = 0;
                            gotchar = true;
                            break;

                        case '&':
                            if (i > 1 && buf[i-2] == '$')
                                break;
                            if (i > 2 && (buf[i-2] == '#' || buf[i-2] == '?')
                                    && buf[i-3] == '$')
                                break;
                            cw = newword(cw, buf);
                            i = 0;
                            break;

                        default:
                            cw = newword(cw, buf);
                            i = 0;
                        }
                    }
                }
            }
        }
    }
    if (!string && cp_flags[CP_INTERACTIVE])
        Lx.LineBuf = 0;
    return (wlist);
}


void
CshPar::Prompt()
{
    if (cp_flags[CP_INTERACTIVE] == false)
        return;
    if (cp_flags[CP_NOTTYIO])
        return;
    const char *ps;
    if (cp_promptstring == 0)
        ps = "-> ";
    else
        ps = cp_promptstring;
    if (cp_altprompt)
        ps = cp_altprompt;

    TTY.init_more();
    char buf[BSIZE_SP];
    *buf = '\0';
    while (*ps) {
        switch (STRIP(*ps)) {
        case '!':
            sprintf(buf + strlen(buf), "%d", cp_event);
            break;
        case '\\':
            if (*(ps + 1)) {
                char *t = buf + strlen(buf);
                *t++ = STRIP(*++ps);
                *t = '\0';
            }
            // fallthrough

        // -p prints cwd
        case '-':
            if (*(ps+1) == 'p') {
                char *c = getcwd(0, 128);
                if (c) {
                    strcpy(buf + strlen(buf), c);
                    delete [] c;
                }
                ps++;
                break;
            }
            // fallthrough
        default:
            char *t = buf + strlen(buf);
            *t++ = *ps;
            *t = '\0';
            break;
        }
        ps++;
    }
    TTY.out_printf("%s", buf);
    TTY.flush();
    
    Lx.PromptLen = strlen(buf);
    if (!cp_flags[CP_NOEDIT]) {
        if (Lx.LineBuf) {
            for (int i = 0; i < Lx.Cend; i++)
                 Lx.ttyecho(Lx.LineBuf[i]);
            for (int i = Lx.Cend-1; i >= Lx.Cpos; i--)
                Lx.cur_rev(i);
            TTY.flush();
        }
    }
    else {
        // The following takes care of reprinting characters in the keyboard
        // buffer after the prompt.  If ESC or ^D was typed, LineBuf will
        // contain characters.
        //
        if (Lx.LineBuf) {
            // True in interactive mode while taking keyboard input, when
            // called (directly or indirectly) from Lexer().
            //
            char *s = buf;
#ifdef TIOCSTI
            char c, end = '\004';
            // Terminate input with ^D, and erase "^D" from the display
            ioctl((int)fileno(cp_input), TIOCSTI, &end);
            TTY.out_printf("\b\b  \b\b");
            TTY.flush();

            // Read the characters into buf
            for (;;) {
                if (read((int)fileno(cp_input), &c, 1) < 1)
                    break;
                if (c == '\004')
                    break;
                *s++ = c;
            }
#endif

            // Now cat the characters from Lexer's linebuf
            for (char *t = Lx.LineBuf; *t; )
                *s++ = *t++;
            *s = '\0';

            // Push the characters back into the keyboard buffer
            for (s = buf; *s; s++)
#ifdef TIOCSTI
                ioctl(fileno(cp_input), TIOCSTI, s);
#else
                TTY.outc(*s);    // But you can't edit
#endif
        }
    }
    cp_flags[CP_WAITING] = true;
}


// Called on application startup with clear=false.  Call before
// application terminates with clear=true.
//
void
CshPar::SetupTty(int fd, bool clear)
{
    if (cp_flags[CP_NOTTYIO])
        return;
    if (!clear && !cp_flags[CP_NOEDIT]) {
        const char *err = Lx.initialize();
        if (err) {
            fprintf(stderr, "Warning: %s.\n", err);
            fprintf(stderr, "Command line editing disabled.\n");
            cp_flags[CP_NOEDIT] = true;
            cp_flags[CP_LOCK_NOEDIT] = true;
        }
    }
    if (!cp_flags[CP_NOEDIT]) {
        set_raw_mode(fd, clear);
        if (clear)
            Lx.terminate();
    }
    else
        set_noedit_mode(fd, clear);
}


namespace {
    inline void shiftdown(char *buf)
    {
        for (int i = 0; i < MAX_RSP; i++)
            buf[i] = buf[i+1];
    }
}


int
CshPar::Getchar(int fd, bool literal, bool drain)
{
#ifdef WIN32
    if (drain)
        return (0);
    if (literal && !cp_flags[CP_RAWMODE] && fd >= 0)
        return (GRpkgIf()->GetChar(fd));
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    int c = 0;
    for (;;) {
        if (cp_srcfiles) {
            // These are temp files from the thread that reads the named
            // pipe.
            CommandTab::com_source(cp_srcfiles);
            for (wordlist *wl = cp_srcfiles; wl; wl = wl->wl_next)
                unlink(wl->wl_word);
            lstring::destroy(cp_srcfiles);
            cp_srcfiles = 0;
            Prompt();
        }
        if (cp_mesg_sock > 0 || cp_acct_sock > 0) {
            fd_set readfds;
            FD_ZERO(&readfds);
            int nfds = 0;
            if (cp_mesg_sock >= 0) {
                FD_SET(cp_mesg_sock, &readfds);
                nfds = cp_mesg_sock;
            }
            else if (cp_acct_sock >= 0) {
                FD_SET(cp_acct_sock, &readfds);
                nfds = cp_acct_sock;
            }

            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;

            int i = select(nfds + 1, &readfds, 0, 0, &timeout);

            if (i > 0) {
                if (cp_mesg_sock >= 0 && FD_ISSET(cp_mesg_sock, &readfds)) {
                    // got a message
                    char xx;
                    i = recv(cp_mesg_sock, &xx, 1, 0);
                    if (i == 1) {
                        if (!MessageHandler(xx)) {
                            fprintf(stderr, eofmsg, cp_program);
                            closesocket(cp_mesg_sock);
                            cp_mesg_sock = -1;
                        }
                    }
                    else if (i == 0) {
                        fprintf(stderr, eofmsg, cp_program);
                        closesocket(cp_mesg_sock);
                        cp_mesg_sock = -1;
                    }
                }
                else if (cp_acct_sock >= 0 &&
                        FD_ISSET(cp_acct_sock, &readfds)) {
                    cp_mesg_sock = accept(cp_acct_sock, 0, 0);
                    if (cp_mesg_sock < 0)
                        GRpkgIf()->Perror("accept");
                }
            }
        }

        INPUT_RECORD rec;
        DWORD bt;
        GP.Checkup();
        if (WaitForSingleObject(h, 100) == WAIT_OBJECT_0) {
            ReadConsoleInput(h, &rec, 1, &bt);
            if (rec.EventType == KEY_EVENT &&
                    rec.Event.KeyEvent.bKeyDown) {

                if (!literal) {
                    if (rec.Event.KeyEvent.wVirtualKeyCode == VK_LEFT)
                        return (KC_L);
                    if (rec.Event.KeyEvent.wVirtualKeyCode == VK_DOWN)
                        return (KC_D);
                    if (rec.Event.KeyEvent.wVirtualKeyCode == VK_RIGHT)
                        return (KC_R);
                    if (rec.Event.KeyEvent.wVirtualKeyCode == VK_UP)
                        return (KC_U);
                    if (rec.Event.KeyEvent.wVirtualKeyCode == VK_DELETE)
                        return (KC_DEL);
                    if (rec.Event.KeyEvent.wVirtualKeyCode == VK_BACK)
                        return (KC_BSP);
                    if (rec.Event.KeyEvent.wVirtualKeyCode == VK_TAB)
                        return (KC_TAB);
                }
                if (rec.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
                    return ('\n');
                if (rec.Event.KeyEvent.uChar.AsciiChar &&
                        isascii(rec.Event.KeyEvent.uChar.AsciiChar)) {
                    c = rec.Event.KeyEvent.uChar.AsciiChar;
                    if (!literal) {
                        if (c == 0x1)
                            return (KC_CA);
                        if (c == 0x4)
                            return (KC_CD);
                        if (c == 0x5)
                            return (KC_CE);
                        if (c == 0x8)
                            return (KC_BSP);
                        if (c == 0xb)
                            return (KC_CK);
                        if (c == 0x15)
                            return (KC_CU);
                        if (c == 0x16)
                            return (KC_CV);
                    }
                    break;
                }
            }
        }
    }

#else  // !Win32
    static int fifo_fd = -1;

    bool found_eof = false;
    static char cbuf[MAX_RSP+1];
    int ofs = 0;
    for (ofs = 0; ofs < MAX_RSP; ofs++) {
        if (!cbuf[ofs])
            break;
    }
    if (!drain) {
        int esc_cnt = 0;
        for (;;) {
            if (fifo_fd < 0 && Global.FifoName())
                fifo_fd = open(Global.FifoName(), O_RDONLY | O_NONBLOCK);
            if (ofs >= MAX_RSP)
                break;
            int nfds = fd > cp_mesg_sock ? fd : cp_mesg_sock;
            if (nfds < 0) {
                if (FILENO(cp_input) < 0)
                    // lost connection to Xic, exit
                    fatal(false);
                break;
            }
            fd_set readfds;
            FD_ZERO(&readfds);
            if (fd >= 0)
                FD_SET(fd, &readfds);
            if (fifo_fd >= 0) {
                FD_SET(fifo_fd, &readfds);
                if (fifo_fd > nfds)
                    nfds = fifo_fd;
            }
            if (cp_mesg_sock >= 0)
                FD_SET(cp_mesg_sock, &readfds);
            else if (cp_acct_sock >= 0) {
                FD_SET(cp_acct_sock, &readfds);
                if (cp_acct_sock > nfds)
                    nfds = cp_acct_sock;
            }
            timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 500;
#ifdef SELECT_TAKES_INTP
            // this stupidity from HPUX
            int i = select(nfds + 1, (int*)&readfds, 0, 0, &timeout);
#else
            int i = select(nfds + 1, &readfds, 0, 0, &timeout);
#endif
            if (i == -1)
                // interrupts do this
                continue;
            if (i == 0) {
                // timeout
                GP.Checkup();
                if (ofs) {
                    if (cbuf[0] == ESCAPE) {
                        if (literal || Kmap.km_possible(cbuf) > 1) {
                            if (esc_cnt < MAX_RSP) {
                                esc_cnt++;
                                continue;
                            }
                        }
                    }
                    break;
                }
                continue;
            }
            esc_cnt = 0;
            if (fifo_fd >= 0 && FD_ISSET(fifo_fd, &readfds)) {
                FILE *fp = fdopen(fifo_fd, "r");
                if (fp) {
                    Sp.SpSource(fp, false, false, 0);
                    fclose(fp);
                    fifo_fd = -1;
                    Prompt();
                }
                else {
                    close(fifo_fd);
                    fifo_fd = -1;
                }
                continue;
            }
            if (cp_mesg_sock >= 0 && FD_ISSET(cp_mesg_sock, &readfds)) {
                // got a message
                char xx;
                i = read(cp_mesg_sock, &xx, 1);
                if (i == 1) {
                    if (!MessageHandler(xx)) {
                        fprintf(stderr, eofmsg, cp_program);
                        close(cp_mesg_sock);
                        cp_mesg_sock = -1;
                    }
                }
                else if (i == 0) {
                    fprintf(stderr, eofmsg, cp_program);
                    close(cp_mesg_sock);
                    cp_mesg_sock = -1;
                }
                continue;
            }
            else if (cp_acct_sock >= 0 && FD_ISSET(cp_acct_sock,
                    &readfds)) {
                cp_mesg_sock = accept(cp_acct_sock, 0, 0);
                if (cp_mesg_sock < 0)
                    GRpkgIf()->Perror("accept");
            }
            if (fd >= 0 && FD_ISSET(fd, &readfds)) {
                i = read(fd, cbuf + ofs, MAX_RSP - ofs);
                if (i > 0) {
                    ofs += i;
                    cbuf[ofs] = 0;
                    if (cbuf[0] != ESCAPE)
                        break;
                }
                else {
                    found_eof = true;
                    break;
                }
            }
        }
    }

    // mapping
    int c = 0;
    if (!literal) {
        sKmap::kmap *k = Kmap.km_match(cbuf);
        if (k) {
            int n = strlen(k->from);
            while (n--)
                shiftdown(cbuf);
            c = k->to;
        }
    }
    if (!c && *cbuf) {
        c = *cbuf;
        shiftdown(cbuf);
    }
    if (!c && found_eof)
        c = EOF;
#endif

    return (c);
}


int
CshPar::RawGetc(int fd)
{
    bool inraw = cp_flags[CP_RAWMODE];
    if (!inraw)
        set_raw_mode(fd, false);
    // This will return only the first byte of a multi-byte response,
    // the rest are thrown away
    int c = Getchar(fd, true, false);
    while (Getchar(fd, true, true) != 0) ;
    if (!inraw)
        set_raw_mode(fd, true);
    return (c);
}


void
CshPar::StuffChars(const char *string)
{
    if (!string)
        return;
    if (!cp_flags[CP_NOEDIT]) {
        Lx.addchars(string);
        return;
    }
#ifdef TIOCSTI
    while (*string) {
        ioctl(TTY.infileno(), TIOCSTI, string);
        string++;
    }
#endif
}
// End of CshPar functions


#ifdef DEBUG_TERM

namespace {
    void
    xxxpr(const char *s, const char *t)
    {
        printf(s);
        if (!t)
            printf("(null)");
        else {
            while (*t) {
                printf(" %x", *t);
                t++;
            }
        }
        printf("\n");
    }
}

#endif


// Find the terminal motion strings, return 0 on success.  If error, return
// an error message.  Unless the function returns 0, don't use rdline.
// Note rdline is always used in Windows.
//
const char*
sLx::initialize()
{
#ifdef WIN32
    return (0);
#endif
    const char *term = 0;
    VTvalue vv;
    if (Sp.GetVar(kw_term, VTYP_STRING, &vv))
        term = vv.get_string();
    else
        term = getenv("TERM");
    if (!term)
        return ("TERM not set in environment and \"term\" variable not set");
    if (Terminal && !lstring::cieq(Terminal, term))
        return (0);

#if defined(__linux) || defined(sun) || defined(__APPLE__)
    if (!getenv("TERMINFO")) {
        if (!access("/usr/share/terminfo", F_OK))
            putenv((char*)"TERMINFO=/usr/share/terminfo");
        else if (!access("/usr/share/lib/terminfo", F_OK))
            putenv((char*)"TERMINFO=/usr/share/lib/terminfo");
        else if (!access("/usr/lib/terminfo", F_OK))
            putenv((char*)"TERMINFO=/usr/lib/terminfo");
    }
#endif

#if defined(HAVE_TIGETSTR) || defined(HAVE_TGETENT)
    // arrow key responses
    char *tle = 0;
    char *tdo = 0;
    char *tnd = 0;

    // arrow key values received
    char *tup = 0;
    char *tlek = 0;
    char *tdok = 0;
    char *tndk = 0;
    char *tupk = 0;

    // Bsp and Del received
    char *tbs = 0;
    char *tdl = 0;

    bool did_smkx = false;
#endif

#if defined(HAVE_TERM_H) && defined(HAVE_TIGETSTR)
    int errret;
    if (setupterm((char*)term, TTY.outfileno(), &errret) != ERR) {

        // This is arcane.  The termcap/terminfo for xterm/gnome
        // assumes that the terminal is in "application mode", which
        // has to be set explicitly by sending the "smkx (ks)" value. 
        // This has to be done before asking for the arrow key values. 
        // The "rmkx (ke)" value should be sent when we end the
        // session.
        //
        // Here is demonstration to run in an xterm or gnome terminal:
        // Run the command:  tput smkx; od -t ax1
        // Then hit left-arrow, etc.
        // Next run the command:  tput rmkx; od -t ax1
        // Then hit left-arrow, etc.
        //
        char *s = tigetstr((char*)"smkx");
        if (s && s != (char*)-1) {
            putp(s);
            did_smkx = true;
        }

        s = tigetstr((char*)"cub1");
        if (s && s != (char*)-1)
            tle = lstring::copy(s);
        s = tigetstr((char*)"cud1");
        if (s && s != (char*)-1)
            tdo = lstring::copy(s);
        s = tigetstr((char*)"cuf1");
        if (s && s != (char*)-1)
            tnd = lstring::copy(s);
        s = tigetstr((char*)"cuu1");
        if (s && s != (char*)-1)
            tup = lstring::copy(s);

        s = tigetstr((char*)"kcub1");
        if (s && s != (char*)-1)
            tlek = lstring::copy(s);
        s = tigetstr((char*)"kcud1");
        if (s && s != (char*)-1)
            tdok = lstring::copy(s);
        s = tigetstr((char*)"kcuf1");
        if (s && s != (char*)-1)
            tndk = lstring::copy(s);
        s = tigetstr((char*)"kcuu1");
        if (s && s != (char*)-1)
            tupk = lstring::copy(s);

        s = tigetstr((char*)"kbs");
        if (s && s != (char*)-1)
            tbs = lstring::copy(s);
        s = tigetstr((char*)"kdch1");
        if (s && s != (char*)-1)
            tdl = lstring::copy(s);
    }

#ifdef DEBUG_TERM
    printf("tigetstr\n");
    xxxpr("tle=", tle);
    xxxpr("tdo=", tdo);
    xxxpr("tnd=", tnd);
    xxxpr("tup=", tup);
    xxxpr("tlek=", tlek);
    xxxpr("tdok=", tdok);
    xxxpr("tndk=", tndk);
    xxxpr("tupk=", tupk);
    xxxpr("tbs=", tbs);
    xxxpr("tdl=", tdl);
#ifdef HAVE_TGETENT
    tle = 0;
    tdo = 0;
    tnd = 0;
    tup = 0;
    tlek = 0;
    tdok = 0;
    tndk = 0;
    tupk = 0;
    tbs = 0;
    tdl = 0;
#endif
#endif
#endif

#ifdef HAVE_TGETENT
    if (tgetent(0, (char*)term) == 1) {
        char abuf[64];

        // This is arcane.  The termcap/terminfo for xterm/gnome
        // assumes that the terminal is in "application mode", which
        // has to be set explicitly by sending the "smkx (ks)" value. 
        // This has to be done before asking for the arrow key values. 
        // The "rmkx (ke)" value should be sent when we end the
        // session.
        //
        if (!did_smkx) {
            char *s = abuf;
            char *t = tgetstr((char*)"ks", &s);
            if (t && t != (char*)-1)
                putp(t);
        }

        if (!tle) {
            char *s = abuf;
            char *t = tgetstr((char*)"le", &s);
            if (t && t != (char*)-1)
                tle = lstring::copy(t);
        }
        if (!tdo) {
            char *s = abuf;
            char *t = tgetstr((char*)"do", &s);
            if (t && t != (char*)-1)
                tdo = lstring::copy(t);
        }
        if (!tnd) {
            char *s = abuf;
            char *t = tgetstr((char*)"nd", &s);
            if (t && t != (char*)-1)
                tnd = lstring::copy(t);
        }
        if (!tup) {
            char *s = abuf;
            char *t = tgetstr((char*)"up", &s);
            if (t && t != (char*)-1)
                tup = lstring::copy(t);
        }

        if (!tlek) {
            char *s = abuf;
            char *t = tgetstr((char*)"kl", &s);
            if (t && t != (char*)-1)
                tlek = lstring::copy(t);
        }
        if (!tdok) {
            char *s = abuf;
            char *t = tgetstr((char*)"kd", &s);
            if (t && t != (char*)-1)
                tdok = lstring::copy(t);
        }
        if (!tndk) {
            char *s = abuf;
            char *t = tgetstr((char*)"kr", &s);
            if (t && t != (char*)-1)
                tndk = lstring::copy(t);
        }
        if (!tupk) {
            char *s = abuf;
            char *t = tgetstr((char*)"ku", &s);
            if (t && t != (char*)-1)
                tupk = lstring::copy(t);
        }

        if (!tbs) {
            char *s = abuf;
            char *t = tgetstr((char*)"kb", &s);
            if (t && t != (char*)-1)
                tbs = lstring::copy(t);
        }
        if (!tdl) {
            char *s = abuf;
            char *t = tgetstr((char*)"kD", &s);
            if (t && t != (char*)-1)
                tdl = lstring::copy(t);
        }
    }

#ifdef DEBUG_TERM
    printf("tgetent\n");
    xxxpr("tle=", tle);
    xxxpr("tdo=", tdo);
    xxxpr("tnd=", tnd);
    xxxpr("tup=", tup);
    xxxpr("tlek=", tlek);
    xxxpr("tdok=", tdok);
    xxxpr("tndk=", tndk);
    xxxpr("tupk=", tupk);
    xxxpr("tbs=", tbs);
    xxxpr("tdl=", tdl);
#endif
#endif

#if defined(HAVE_TIGETSTR) || defined(HAVE_TGETENT)
    // For some reason, (old) FreeBSD termcap for xterm doesn't define le
    if (!tle)
        tle = lstring::copy("\b");
    if (!tdo)
        tdo = lstring::copy("\n");

    if (tle && tdo && tnd && tup) {
        Lx.Tle = tle;
        Lx.Tdo = tdo;
        Lx.Tnd = tnd;
        Lx.Tup = tup;
        delete [] Terminal;
        Terminal = lstring::copy(term);
        Sp.SetVar(kw_term, Terminal);

        if (tle)
            Kmap.km_set(tlek, KC_L);
        if (tdo)
            Kmap.km_set(tdok, KC_D);
        if (tnd)
            Kmap.km_set(tndk, KC_R);
        if (tup)
            Kmap.km_set(tupk, KC_U);
        if (tbs)
            Kmap.km_set(tbs, KC_BSP);
        if (tdl)
            Kmap.km_set(tdl, KC_DEL);
        return (0);
    }
#endif

    return ("Can't find TERMINFO or TERMCAP cursor motion codes for terminal");
}


void
sLx::terminate()
{
#ifdef HAVE_TIGETSTR
    char *s = tigetstr((char*)"rmkx");
    if (s && s != (char*)-1) {
        putp(s);
        return;
    }
#else
#ifdef HAVE_TGETENT
    char *s = tgetstr((char*)"ke", 0);
    if (s && s != (char*)-1) {
        putp(s);
        return;
    }
#endif
#endif
}


char *
sLx::rdline(int fd)
{
    static char buf[512];
    if (!LineBuf) {
        // ^C will reenter
        TTY.winsize(&ScrCols, 0);
        LtBase = new lterm(1, PromptLen, ScrCols - PromptLen - 1, 0);
    }

    LineBuf = buf;
    Histent = 0;

    int c;
    do {
        c = CP.Getchar(fd, QuoteNext);
    } while (stuffchar(c) >= 0);

    TTY.flush();
    buf[Cend] = '\0';
    Cend = Cpos = Ccpos = 0;
    LineBuf = 0;
    delete [] LastLine;
    LastLine = 0;
    CP.SetFlag(CP_WAITING, false);
    lterm *lt;
    for ( ; LtBase; LtBase = lt) {
        lt = LtBase->next;
        delete [] LtBase;
    }
    if (*buf) {
        char *str = lstring::copy(buf);
        return (str);
    }
    return (0);
}


void
sLx::addchars(const char *string)
{
    while (*string) {
        QuoteNext = false;
        ttyecho(*string);
        int i;
        for (i = Cend; i > Cpos; i--)
            LineBuf[i] = LineBuf[i-1];
        LineBuf[Cpos] = *string;
        Cend++;
        Cpos++;
        if (Cpos < Cend) {
            for (i = Cpos; i < Cend; i++)
                ttyecho(LineBuf[i]);
            for (i = Cend-1; i >= Cpos; i--)
                cur_rev(i);
        }
        string++;
    }
}


int
sLx::stuffchar(int c)
{
    if (!LineBuf)
        return (0);
    int i;

    switch (c) {
    case '\n':
    case '\r':
        // finished; cursor to end of line, return
        if (QuoteNext)
            stuffchar(20); // add the ^V
        while (Cpos < Cend) {
            cur_adv(Cpos);
            Cpos++;
        }
        TTY.outc('\n');
        QuoteNext = false;
        return (-1);

    case KC_CA:
        // cursor to start of line
        while (Cpos) {
            Cpos--;
            cur_rev(Cpos);
        }
        return (1);

    case KC_CD:
        // list possible completion matches
        if (CP.GetFlag(CP_INTERACTIVE) && !CP.GetFlag(CP_NOCC) &&
                Cpos == Cend) {
            LineBuf[Cend] = '\0';
            i = Cpos;
            while (i && !isspace(LineBuf[i]))
                i--;
            wordlist *wlist;
            if (i || isspace(LineBuf[0])) {
                wlist = CP.Lexer(LineBuf);
                if (wordlist::length(wlist) == 1 && isspace(LineBuf[Cpos-1])) {
                    wlist->wl_next = new wordlist;
                    wlist->wl_next->wl_prev = wlist;
                    // This indicates to Complete() that the first arg
                    // is complete
                }
                CP.Complete(wlist, LineBuf + i + 1, false);
            }
            else {
                wlist = new wordlist;
                CP.Complete(wlist, LineBuf, false);
            }
            wordlist::destroy(wlist);
        }
        return (1);

    case KC_CE:
        // cursor to end of line
        while (Cpos < Cend) {
            cur_adv(Cpos);
            Cpos++;
        }
        return (1);

    case KC_BSP:
        // erase char to left of cursor
        if (Cpos == 0)
            return (1);
        Cpos--;
        cur_rev(Cpos);
        cte(Cpos);
        for (i = Cend - 1; i >= Cpos; i--) {
            cur_rev(i);
        }
        for (i = Cpos; i+1 < Cend; i++)
            LineBuf[i] = LineBuf[i+1];
        Cend--;
        for (i = Cpos; i < Cend; i++)
            ttyecho(LineBuf[i]);
        for (i = Cend-1; i >= Cpos; i--) {
            cur_rev(i);
        }
        return (1);

    case KC_TAB:
        // insert completion match, if any
        if (CP.GetFlag(CP_INTERACTIVE) && !CP.GetFlag(CP_NOCC) &&
                Cpos == Cend) {
            LineBuf[Cend] = '\0';
            i = Cpos;
            while (i && !isspace(LineBuf[i]))
                i--;
            wordlist *wlist;
            if (i) {
                wlist = CP.Lexer(LineBuf);
                CP.Complete(wlist, LineBuf + i + 1, true);
            }
            else {
                wlist = new wordlist;
                CP.Complete(wlist, LineBuf, true);
            }
            wordlist::destroy(wlist);
        }
        return (1);

    case KC_CK:
        // erase to end of line
        if (Cpos == Cend)
            return (1);
        cte(Cpos);
        for (i = Cend - 1; i >= Cpos; i--)
            cur_rev(i);
        Cend = Cpos;
        return (1);

    case KC_CU:
        // erase line
        while (Cpos) {
            Cpos--;
            cur_rev(Cpos);
        }
        cte(0);
        for (i = Cend-1; i >= 0; i--)
            cur_rev(i);
        Cend = 0;
        return (1);

    case KC_CV:
        // quote character
        QuoteNext = true;
        return (1);

    case KC_DEL:
        // delete char under cursor
        if (Cpos == Cend)
            return (1);
        cte(Cpos);
        for (i = Cend - 1; i >= Cpos; i--) {
            cur_rev(i);
        }
        for (i = Cpos; i+1 < Cend; i++)
            LineBuf[i] = LineBuf[i+1];
        Cend--;
        for (i = Cpos; i < Cend; i++)
            ttyecho(LineBuf[i]);
        for (i = Cend-1; i >= Cpos; i--)
            cur_rev(i);
        return (1);

    case KC_U:
        if (!Histent)
            Histent = CP.cp_lastone;
        else if (Histent->prev())
            Histent = Histent->prev();
        if (Histent) {
            if (Histent == CP.cp_lastone) {
                delete [] LastLine;
                LineBuf[Cend] = '\0';
                LastLine = lstring::copy(LineBuf);
            }
            while (Cpos) {
                Cpos--;
                cur_rev(Cpos);
            }
            cte(0);
            for (i = 0; i < Cend; i++)
                cur_rev(i);
            char *s = wordlist::flatten(Histent->text());
            for (Cend = 0; s[Cend]; Cend++) {
                LineBuf[Cend] = s[Cend];
                ttyecho(s[Cend]);
            }
            delete [] s;
            Cpos = Cend;
        }
        return (1);

    case KC_D:
        if (Histent) {
            Histent = Histent->next();
            if (Histent) {
                while (Cpos) {
                    Cpos--;
                    cur_rev(Cpos);
                }
                cte(0);
                for (i = 0; i < Cend; i++)
                    cur_rev(i);
                char *s = wordlist::flatten(Histent->text());
                for (Cend = 0; s[Cend]; Cend++) {
                    LineBuf[Cend] = s[Cend];
                    ttyecho(s[Cend]);
                }
                delete [] s;
                Cpos = Cend;
            }
            else if (LastLine) {
                while (Cpos) {
                    Cpos--;
                    cur_rev(Cpos);
                }
                cte(0);
                for (i = 0; i < Cend; i++)
                    cur_rev(i);
                const char *s = LastLine;
                for (Cend = 0; s[Cend]; Cend++) {
                    LineBuf[Cend] = s[Cend];
                    ttyecho(s[Cend]);
                }
                Cpos = Cend;
            }
        }
        return (1);

    case KC_R:
        if (Cpos < Cend) {
            cur_adv(Cpos);
            Cpos++;
        }
        return (1);

    case KC_L:
        if (Cpos) {
            Cpos--;
            cur_rev(Cpos);
        }
        return (1);
    }

    QuoteNext = false;
    ttyecho(c);
    for (i = Cend; i > Cpos; i--)
        LineBuf[i] = LineBuf[i-1];
    LineBuf[Cpos] = c;
    Cend++;
    Cpos++;
    if (Cpos < Cend) {
        for (i = Cpos; i < Cend; i++)
            ttyecho(LineBuf[i]);
        for (i = Cend-1; i >= Cpos; i--)
            cur_rev(i);
    }
    return (0);
}


void
sLx::ttyecho(int c)
{
    if ((c < 037 && c != '\t' && c != '\n') || c == 0177) {
        TTY.outc('^');
        cpos_adv(false);
        if (c == 0177)
            c = '?';
        else
            c += 'A' - 1;
    }
    if (c == '\t') {
        int i = 8 - ((Ccpos + LtBase->begincol) % 8);
        while (i--) {
            TTY.outc(' ');
            cpos_adv(false);
        }
    }
    else {
        TTY.outc(c);
        cpos_adv(false);
    }
    TTY.flush();
}


// Return the screen width used in rendering c
//
int
sLx::colcnt(int ind)
{
    int c = LineBuf[ind];
    if ((c < 037 && c != '\t' && c != '\n') || c == 0177)
        return (2);
    if (c == '\t') {
        int j = fcpos(ind);
        return (8 - j%8);
    }
    return (1);
}


// Return the column position after the first pos characters in screen
// rendering columns.
//
int
sLx::fcpos(int pos)
{
    // find the origin column
    int count = 0;
    lterm *lt = LtBase;
    if (lt) {
        while (lt->next)
            lt = lt->next;
        count = lt->begincol;
    }

    if (pos > Cend)
        pos = Cend;

    // count columns
    for (int i = 0; i < pos; i++) {
        if ((LineBuf[i] < 037 && LineBuf[i] != '\t' &&
                LineBuf[i] != '\n') || LineBuf[i] == 0177)
            count += 2;
        else if (LineBuf[i] == '\t')
            count += (8 - count % 8);
        else
            count++;
    }
    return (count);
}


void
sLx::cur_adv(int c)
{
    int i = colcnt(c);
    while (i--) {
        mv_r();
        cpos_adv(true);
    }
};


void
sLx::cur_rev(int c)
{
    int i = colcnt(c);
    while (i--) {
        mv_l();
        cpos_rev();
    }
};


void
sLx::cte(int c)
{
    for ( ; c < Cend; c++) {
        int j = colcnt(c);
        while (j--) {
            TTY.outc(' ');
            cpos_adv(false);
        }
    }
};


// If move is true, advance the cursor explicitly (response to a right
// arrow key).  Otherwise, the cursor is automatically advanced with the
// printing character.  There is a problem:  at the end of a line, the
// cursor is not advanced, but is advanced to column two of the following
// line on the next character.  Have to track this.
//
void
sLx::cpos_adv(bool move)
{
    Ccpos++;
    if (Ccpos > LtBase->endcol) {
        LtBase = new lterm(LtBase->lineno + 1, LtBase->endcol + 1,
            LtBase->endcol + ScrCols, LtBase);
        if (move) {
            // move cursor down
            mv_d();
            // move cursor to start of line
            for (int i = 0; i < ScrCols-1; i++)
                mv_l();
        }
        else {
            TTY.outc(' ');
            mv_l();
        }
    }
}


void
sLx::cpos_rev()
{
    Ccpos--;
    if (!LtBase->next)
        return;
    if (Ccpos <= LtBase->next->endcol) {
        lterm *lt = LtBase->next;
        delete [] LtBase;
        LtBase = lt;
        // move cursor up
        mv_u();
        // move cursor to end of line
        for (int i = 0; i < ScrCols-1; i++)
            mv_r();
    }
}


// The following functions move the cursor.  In Win32, there is no ANSI
// responsiveness since all MicroShit processing has been bypassed.

void
sLx::mv_l()
{
#ifdef WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(h, &info);
    if (info.dwCursorPosition.X) {
        info.dwCursorPosition.X--;
        SetConsoleCursorPosition(h, info.dwCursorPosition);
    }
#else
    TTY.out_printf("%s", Tle);
    TTY.flush();
#endif
}


void
sLx::mv_d()
{
#ifdef WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(h, &info);
    info.dwCursorPosition.Y++;
    SetConsoleCursorPosition(h, info.dwCursorPosition);
#else
    TTY.out_printf("%s", Tdo);
    TTY.flush();
#endif
}


void
sLx::mv_r()
{
#ifdef WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(h, &info);
    info.dwCursorPosition.X++;
    SetConsoleCursorPosition(h, info.dwCursorPosition);
#else
    TTY.out_printf("%s", Tnd);
    TTY.flush();
#endif
}


void
sLx::mv_u()
{
#ifdef WIN32
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(h, &info);
    if (info.dwCursorPosition.Y)
        info.dwCursorPosition.Y--;
    SetConsoleCursorPosition(h, info.dwCursorPosition);
#else
    TTY.out_printf("%s", Tup);
    TTY.flush();
#endif
}
// End of sLx functions.


namespace {
    void set_raw_mode(int fd, bool clear)
    {
        static bool tty_set = true;
        if (clear == tty_set || !isatty(fd))
            return;

#if HAVE_TERMIOS_H
        static struct termios saveterm;
        if (!clear) {
            struct termios s;
            tcgetattr(fd, &s);
            saveterm = s;
            s.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ECHOCTL|IEXTEN);
            s.c_oflag |=  (OPOST|ONLCR|TAB3);
            s.c_cc[VMIN] = 1;
            s.c_cc[VTIME] = 0;
            tcsetattr(fd, TCSADRAIN, &s);
            CP.SetFlag(CP_RAWMODE, true);
        }
        else {
            tcsetattr(fd, TCSADRAIN, &saveterm);
            CP.SetFlag(CP_RAWMODE, false);
        }
        tty_set = clear;
#else // HAVE_TERMIOS_H

#if HAVE_TERMIO_H
        static struct termio saveterm;
        if (!clear) {
            struct termio s;
            ioctl(fd, TCGETA, &s);
            saveterm = s;
            s.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ECHOCTL|IEXTEN);
            s.c_oflag |=  (OPOST|ONLCR|TAB3);
            s.c_oflag &= ~(OCRNL|ONOCR|ONLRET);
            s.c_cc[VMIN] = MAX_RSP;
            s.c_cc[VTIME] = 1;
            ioctl(fd, TCSETAW, &s);
            CP.cp_rawmode = true;
        }
        else {
            ioctl(fd, TCSETAW, &saveterm);
            CP.cp_rawmode = false;
        }
        tty_set = clear;
#else // HAVE_TERMIO_H

#ifdef HAVE_SGTTY_H
        static struct sgttyb saveterm;
        if (!clear) {
            struct sgttyb s;
            ioctl(fd, TIOCGETP, &s);
            struct ltchars l;
            ioctl(fd, TIOCGLTC, &l);
            saveterm = s;
            s.sg_flags |= CBREAK;
            s.sg_flags &= ~(ECHO|XTABS);
            ioctl(fd, TIOCSETN, &s);
            CP.cp_rawmode = true;
        }
        else {
            ioctl(fd, TIOCSETN, &saveterm);
            CP.cp_rawmode = false;
        }
        tty_set = clear;
#else

#ifdef WIN32
        if (clear) {
            HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
            SetConsoleMode(h,
                ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
            CP.SetFlag(CP_RAWMODE, false);
        }
        else {
            HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
            SetConsoleMode(h, ENABLE_PROCESSED_INPUT);
            CP.SetFlag(CP_RAWMODE, true);
        }
        tty_set = clear;
#else
        Bogus;
#endif

#endif // HAVE_SGTTY_H
#endif // HAVE_TERMIO_H
#endif // HAVE_TERMIOS_H
    }


    void set_noedit_mode(int fd, bool clear)
    {
        static bool tty_set = true;
        if (clear == tty_set || !isatty(fd))
            return;

#if HAVE_TERMIOS_H
        static struct termios saveterm;
        if (!clear) {
            struct termios s;
            tcgetattr(fd, &s);
            saveterm = s;
            s.c_cc[VEOF] = 0;
            s.c_cc[VEOL] = ESCAPE;
            s.c_cc[VEOL2] = 4; // ^D
            tcsetattr(fd, TCSADRAIN, &s);
        }
        else
            tcsetattr(fd, TCSADRAIN, &saveterm);
        tty_set = clear;
#else // HAVE_TERMIOS_H

#if HAVE_TERMIO_H
        static struct termio saveterm;
        if (!clear) {
            struct termio s;
            ioctl(fd, TCGETA, &s);
            saveterm = s;
            s.c_cc[VEOF] = 0;
            s.c_cc[VEOL] = ESCAPE;
            s.c_cc[VEOL2] = 4; // ^D
            ioctl(fd, TCSETAW, &s);
        }
        else
            ioctl(fd, TCSETAW, &saveterm);
        tty_set = clear;
#else // HAVE_TERMIO_H

#ifdef HAVE_SGTTY_H
        if (CP.cp_nocc || !CP.cp_interactive || ttyset == clear)
            return;

        // Set the terminal up -- make escape the break character, and
        // make sure we aren't in raw or cbreak mode.  Hope the ioctl's
        // won't fail.
        //
        struct tchars tbuf;
        ioctl(fd, TIOCGETC, &tbuf);
        if (!clear)
            tbuf.t_brkc = ESCAPE;
        else
            tbuf.t_brkc = '\0';
        ioctl(fd, TIOCSETC, &tbuf);
        struct sgttyb sbuf;
        ioctl(fd, TIOCGETP, &sbuf);
        sbuf.sg_flags &= ~(RAW|CBREAK);
        ioctl(fd, TIOCSETP, &sbuf);
        tty_set = clear;
#else

#ifdef WIN32
        tty_set = clear;
#else
        Bogus;
#endif

#endif // HAVE_SGTTY_H
#endif // HAVE_TERMIO_H
#endif // HAVE_TERMIOS_H
    }
}


