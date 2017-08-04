
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
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

/*************************************************************************
 MFB graphics and miscellaneous library
 Copyright (c) Stephen R. Whiteley 1995
 Author: Stephen R. Whiteley
 *************************************************************************/

#include "config.h"
#include "ttyio.h"
#include "wlist.h"
#include "outplot.h"
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <stdarg.h>
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

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

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#define DEF_SCRHEIGHT   24
#define DEF_SCRWIDTH    80


sTTYio TTY;

sTTYio::sTTYio()
{
    fp_in           = stdin;
    fp_out          = stdout;
    fp_err          = stderr;
    t_fp_curin      = stdin;
    t_fp_curout     = stdout;
    t_fp_curerr     = stderr;
    t_context       = 0;
    t_width         = 0;
    t_height        = 0;
    t_xsize         = 0;
    t_ysize         = 0;
    t_xpos          = 0;
    t_ypos          = 0;
    t_out_count     = 0;
    t_ref_out_count = 0;
    t_noprint       = false;
    t_nopause       = false;
    t_oneline       = false;
    t_moremode      = true;
    t_isatty        = isatty(fileno(stdout));
    t_hidechars     = false;
}


// IO context handling.  This is tricky, since if we are sourcing a
// command file, and io has been redirected from inside the file, we
// have to reset it back to what it was for the source, not for the
// top level.  That way if you type "foo > bar" where foo is a script,
// and it has redirections of its own inside of it, none of the output
// from foo will get sent to stdout...
//
// Resetting the file pointers for redirection is handled by the shell.
// The fp_xx pointers are always used for io, fp_curxx pointers are
// the non-redirected pointers.


// Redirect the given non-nil pointers
//
void
sTTYio::ioRedirect(FILE *in, FILE *out, FILE *err)
{
    if (in)
        fp_in = in;
    if (out) {
        fp_out = out;
        t_isatty = isatty(fileno(fp_out));
    }
    if (err)
        fp_err = err;
}


// This is called to clean up after redirection
//
void
sTTYio::ioReset()
{
    if (fp_in != t_fp_curin) {
        if (fp_in && fp_in != stdin)
            fclose(fp_in);
        fp_in = t_fp_curin;
    }
    // Careful not to double-close ">&" output.
    FILE *fo = 0;
    if (fp_out != t_fp_curout) {
        if (fp_out && fp_out != stdout) {
            fo = fp_out;
            fclose(fp_out);
        }
        fp_out = t_fp_curout;
        t_isatty = isatty(fileno(fp_out));
    }
    if (fp_err != t_fp_curerr) {
        if (fp_err && fp_err != stderr && fp_err != stdout && fp_err != fo)
            fclose(fp_err);
        fp_err = t_fp_curerr;
    }
}


// This is called before execution of a script or code block.  If a
// file pointer is passed, it will be the standard output default.
//
void
sTTYio::ioPush(FILE *out)
{
    t_context = new sTTYcx(t_fp_curin, t_fp_curout, t_fp_curerr, t_context);
    t_fp_curin = fp_in;
    t_fp_curout = out ? out : fp_out;
    t_fp_curerr = fp_err;
    if (out) {
        t_context->fp_old_out = fp_out;
        fp_out = out;
        t_isatty = isatty(fileno(fp_out));
    }
}


// This is called upon return from a script or code block
//
void
sTTYio::ioPop()
{
    if (t_context) {
        ioReset();
        t_fp_curin = t_context->fp_saved_in;
        t_fp_curout = t_context->fp_saved_out;
        t_fp_curerr = t_context->fp_saved_err;
        if (t_context->fp_old_out) {
            fp_out = t_context->fp_old_out;
            t_isatty = isatty(fileno(fp_out));
        }
        sTTYcx *cx = t_context;
        t_context = t_context->next;
        delete cx;
    }
}


// This is called on interrupt while executing a script or code block
//
void
sTTYio::ioTop()
{
    while (t_context)
        ioPop();
    ioReset();
    t_isatty = isatty(fileno(fp_out));
}


// Temporary override, use with care
//
void
sTTYio::ioOverride(FILE *in, FILE *out, FILE *err)
{
    if (in)
        fp_in = t_fp_curin = in;
    if (out) {
        fp_out = t_fp_curout = out;
        t_isatty = isatty(fileno(fp_out));
    }
    if (err)
        fp_err = t_fp_curerr = err;
}


// Do this only right before an exec, since we lose the old std*'s
//
void
sTTYio::fixDescriptors()
{
    if (fp_in != stdin)
        dup2(fileno(fp_in), fileno(stdin));
    if (fp_out != stdout)
        dup2(fileno(fp_out), fileno(stdout));
    if (fp_err != stderr)
        dup2(fileno(fp_err), fileno(stderr));
}


// Functions to handle "more"d output.  There are some serious system
// dependencies in here, and it isn't clear that versions of this stuff
// can be written for every possible machine...

#define bufputc(c)  ( --ourbuf.count >= 0 ? ((int) \
    (*ourbuf.ptr++ = (unsigned)(c))) : fbufputc((unsigned) (c)))

#define UseMore() (t_moremode && TTYif.isInteractive() && t_isatty)

namespace {
    // Putc may not be buffered, so we do it ourselves.
    //
    char staticbuf[BUFSIZ];
    struct bf
    {
        int count;
        char *ptr;
    } ourbuf = { BUFSIZ, staticbuf };

    // Send buffer out.
    void outbufputc()
    {
        if (ourbuf.count != BUFSIZ) {
            fputs(staticbuf, TTY.outfile());
            memset(staticbuf, 0, BUFSIZ-ourbuf.count);
            ourbuf.count = BUFSIZ;
            ourbuf.ptr = staticbuf;
        }
    }

    int fbufputc(unsigned c)
    {
        ourbuf.count = 0;
        outbufputc();
        ourbuf.count = BUFSIZ;
        ourbuf.ptr = staticbuf;
        bufputc(c);
        return (0);
    }
}


// Figure out the screen size.
//
void
sTTYio::winsize(int *cols, int *rows)
{
    // Handle file output, too
    //
    if (!t_isatty) {
        if (cols) {
            if (getwidth() > 0)
                *cols = getwidth();  // value from "set width"
            else
                *cols = DEF_WIDTH;
        }
        if (rows) {
            if (getheight() > 0)
                *rows = getheight();  // value from "set height"
            else
                *rows = DEF_HEIGHT;
        }
        return;
    }

    int x = 0, y = 0;
#ifdef TIOCGWINSZ
    if (!x || !y) {
        struct winsize ws;
        if (ioctl(fileno(fp_out), TIOCGWINSZ, (char *) &ws) >= 0) {
            x = ws.ws_col;
            y = ws.ws_row;
        }
    }
#endif
    char *s;
    if (!x) {
        if ((s = getenv("COLS"))) {
            x = atoi(s);
            if (x <= 0)
                x = 0;
        }
    }
    if (!x) {
        if ((s = getenv("COLUMNS"))) {
            x = atoi(s);
            if (x <= 0)
                x = 0;
        }
    }
    if (!y) {
        if ((s = getenv("LINES"))) {
            y = atoi(s);
            if (y <= 0)
                y = 0;
        }
    }

#if defined(HAVE_TERM_H) && defined(HAVE_TIGETSTR)
    if (!x || !y) {
        static int didsetup;
        if (!didsetup) {
            if ((s = getenv("TERM")) != 0 &&
                    setupterm(s, fileno(fp_out), 0) != ERR)
                didsetup = 1;
            else
                didsetup = 2;
        }
        if (didsetup == 1) {
            x = tigetnum((char*)"cols");
            y = tigetnum((char*)"lines");
            if ((x <= 0) || (y <= 0))
                x = y = 0;
        }
    }
#else
#ifdef HAVE_TGETENT
    if (!x || !y) {
        char tbuf[4096];
        if ((s = getenv("TERM")) != 0 && tgetent(tbuf, s) == 1) {
            x = tgetnum("co");
            y = tgetnum("li");
            if ((x <= 0) || (y <= 0))
                x = y = 0;
        }
    }
#endif
#endif

#ifdef WIN32
    if (!x || !y) {
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE) {
            CONSOLE_SCREEN_BUFFER_INFO sbi;
            if (GetConsoleScreenBufferInfo(h, &sbi)) {
                x = sbi.dwSize.X;
                y = sbi.dwSize.Y;
            }
        }
    }
#endif

    if (!x) {
        if (t_width > 0)
            x = t_width;
    }
    if (!y) {
        if (t_height > 0)
            y = t_height;
    }

    if (!x)
        x = DEF_SCRWIDTH;
    if (!y)
        y = DEF_SCRHEIGHT;

    if (cols != 0)
        *cols = x;
    if (rows != 0)
        *rows = y;
}


// Return the process group.
//
int
sTTYio::getpgrp()
{
#ifdef HAVE_TCGETPGRP
    return (tcgetpgrp(outfileno()));
#else
    return (-1);
#endif
}


// Start output...
//
void
sTTYio::init_more()
{
    t_noprint = t_nopause = false;
    monitor();
    if (!t_moremode || !TTYif.isInteractive())
        return;
    winsize(&t_xsize, &t_ysize);
    t_ysize -= 2;
    t_xpos = t_ypos = 0;
}


void
sTTYio::promptreturn()
{
    const char *xx = "-- More --  (? for help) ";
    const char *menu =
"\nPossible responses:\n\
    q      : Discard the rest of the output.\n\
    c      : Continuously print the rest of the output.\n\
    h, ?   : Print this help message.\n\
    Enter  : Print one more line.\n\
    other  : Print the next page of output.\n";

moe:
    fprintf(fp_out, "\n");
    fputs(xx, fp_out);
    fflush(fp_out);
    char buf[16];
    if (fp_in == stdin)
        *buf = TTYif.getchar();
    else if (!fgets(buf, 16, fp_in)) {
        clearerr(fp_in);
        *buf = 'q';
    }
    if (fp_out == stdout)
        fputs("\r                                  \r", fp_out);
    switch (*buf) {
    case 'q':
        t_noprint = true;
        break;
    case 'c':
        t_nopause = true;
        break;
    case '?':
    case 'h':
        out_printf("%s\n", menu);
        goto moe;
    case '\r':
    case '\n':
        t_oneline = true;
    }
}


// Print a string to the output.  If this would cause the screen to scroll, 
// print "more".
//
void
sTTYio::send(const char *string)
{
    if (t_noprint || !string)
        return;
    t_out_count++;
    if (t_isatty && TTYif.getWaiting()) {
        if (t_nopause)
            fputc('\n', fp_out);
        else
            bufputc('\n');
        TTYif.setWaiting(false);
    }
    if (!UseMore() || t_nopause || !t_xsize) {
        fputs(string, fp_out);
        return;
    }
    while (*string) {
        switch (*string) {
            case '\n':
                t_xpos = 0;
                t_ypos++;
                break;
            case '\f':
                t_ypos = t_ysize;
                t_xpos = 0;
                break;
            case '\t':
                t_xpos = t_xpos / 8 + 1;
                t_xpos *= 8;
                break;
            case '\b':
                if (t_xpos)
                    t_xpos--;
                break;
            default:
                t_xpos++;
                break;
        }
        while (t_xpos >= t_xsize) {
            t_xpos -= t_xsize;
            t_ypos++;
        }
        if (t_ypos >= t_ysize || (t_oneline && t_ypos)) {
            t_oneline = false;
            outbufputc();       // out goes buffer
            promptreturn();
            fflush(fp_out);
            t_ypos = t_xpos = 0;
            if (t_noprint)
                return;
            if (*string != '\n')
                bufputc(*string);
        }
        else
            bufputc(*string);
        string++;
    }
    outbufputc();
}


#define MAXLEN 4096

// Print to output channel using "more" mode.
//
void
sTTYio::printf(const char *fmt, ...)

{
    va_list args;
    char buf[MAXLEN];
    if (!fmt)
        return;
    va_start(args, fmt);
    vsnprintf(buf, MAXLEN, fmt, args);
    va_end(args);
    send(buf);
}


// Print to output channel bypassing "more" mode, deal with prompting.
//
void
sTTYio::printf_force(const char *fmt, ...)

{
    va_list args;
    char buf[MAXLEN];
    if (!fmt)
        return;
    va_start(args, fmt);
    vsnprintf(buf, MAXLEN, fmt, args);
    va_end(args);

    if (t_isatty && TTYif.getWaiting()) {
        fputs("\n", fp_out);
        TTYif.setWaiting(false);
    }
    fputs(buf, fp_out);
}


// Print arguments directly to output channel, bypass "more" mode and
// prompting.
//
void
sTTYio::out_printf(const char *fmt, ...)

{
    va_list args;
    char buf[MAXLEN];
    if (!fmt)
        return;
    va_start(args, fmt);
    vsnprintf(buf, MAXLEN, fmt, args);
    va_end(args);
    fputs(buf, fp_out);
}


// Print arguments to error channel.
//
void
sTTYio::err_printf(const char *fmt, ...)

{
    va_list args;
    char buf[MAXLEN];
    if (!fmt)
        return;
    va_start(args, fmt);
    vsnprintf(buf, MAXLEN, fmt, args);
    va_end(args);
    fputs(buf, fp_err);
}


void
sTTYio::outc(int c)
{
    if (t_hidechars && !isspace(c))
        fputc('*', fp_out);
    else
        fputc(c, fp_out);
}


// print a wordlist
//
void
sTTYio::wlprint(const wordlist *wl)
{
    if (wl) {
        wordlist *wlc = wordlist::copy(wl);
        TTYif.stripList(wlc);
        for (wl = wlc; wl; wl = wl->wl_next) {
            send(wl->wl_word);
            if (wl->wl_next)
                send(" ");
        }
        wordlist::destroy(wlc);
    }
}


void
sTTYio::flush()
{
    fflush(fp_out);
}


char *
sTTYio::prompt_for_input(char *s, int n, const char *prompt, bool hide)
{
    t_hidechars = hide;
    *s = '\0';
    if (prompt != 0) {
        fputs(prompt, fp_out);
        fflush(fp_out);
    }
    wordlist *wl = TTYif.getWordlist();
    t_hidechars = false;
    if (wl) {
        char *t = wordlist::flatten(wl);
        if (strlen(t) >= (unsigned)n)
            t[n-1] = '\0';
        strcpy(s, t);
        delete [] t;
        wordlist::destroy(wl);
        return (s);
    }
    return (0);
} 


bool
sTTYio::prompt_for_yn(bool defyes, const char *prompt)
{
    char buf[32];
    prompt_for_input(buf, 32, prompt);
    if (*buf == 'y' || *buf == 'Y' || *buf == '1')
        return (true);
    if (*buf == 'n' || *buf == 'N' || *buf == '0')
        return (false);
    return (defyes);
}

// end of sTTYio methods

