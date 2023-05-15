
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1986 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#ifdef HAVE_MOZY
#include "cshell.h"
#include "help/help_defs.h"
#include "help/help_topic.h"
#include "text_help.h"
#include <stdio.h>
#include <unistd.h>


bool
text_help::display(HLPtopic *t)
{
    if (!t)
        return (false);
    show(t);
    for (;;) {
        HLPtopic *parent;
        HLPtopList *res = handle(curtop, &parent);
        if (!res && !parent)
            // No more windows
            break;
        if (res) {
            // Create a new window...
            HLPtopic *newtop;
            if (!(newtop = HLP()->read(res->keyword()))) {
                GRpkg::self()->ErrPrintf(ET_INTERR, "tdisplay: bad link.\n");
                continue;
            }
            newtop->set_sibling(parent->lastborn());
            parent->set_lastborn(newtop);
            newtop->set_parent(parent);
            show(newtop);
        }
        else {
            // Blow this one and its descendants away
            HLPtopic *last = parent->parent();
            parent->unlink();
            HLPtopic::destroy(parent);
            if (!last)
                break;
            show(last);
        }
    }
    return (true);
}


void
text_help::show(HLPtopic *t)
{
    curtop = t;
    TTY.init_more();
    TTY.send("\n");
    TTY.printf("%s\n\n", t->title());

    int cols, rows;
    TTY.winsize(&cols, &rows);
    char *s = t->strip_html(cols - 6, 0);
    TTY.send(s);
    TTY.send("\n");
    delete [] s;
    
    int i = 0;
    if (t->subtopics()) {
        const char *xx = " Sub-Topics:";
        TTY.printf("%s\n", xx);
        i = putlist(t, t->subtopics(), 0);
        TTY.send("\n");
    }
    if (t->seealso()) {
        const char *xx = " See Also:";
        TTY.printf("%s\n", xx);
        putlist(t, t->seealso(), i);
        TTY.send("\n");
    }
    if (!t->subtopics() && !t->seealso())
        TTY.send("\n");
}


HLPtopList *
text_help::handle(HLPtopic *t, HLPtopic **parent)
{
    quitflag = false;
    if (!t) {
        *parent = 0;
        return (0);
    }
    for (;;) {
        char buf[512];
        if (!TTY.prompt_for_input(buf, 512, "Selection (`?' for help): ")) {
            clearerr(TTY.infile());
            *parent = t;
            return (0);
        }

        char *s;
        for (s = buf; *s && isspace(*s); s++) ;
        switch (*s) {
        case '?':
            TTY.out_printf(
                "\nType the number of a sub-topic or see also, or one of:\n"
                "\tr\tReprint the current topic\n"
                "\tp or CR\tReturn to the previous topic\n"
                "\tq\tQuit help\n"
                "\t?\tPrint this message\n\n");
            continue;

        case 'r':
            {
                char *ss;
                if ((ss = strchr(s, '\n')) != 0)
                    *ss = '\0';

                if ((ss = strchr(s, '>')) == 0) {
                    display(t);
                    continue;
                }    
                ss++;
                FILE *fp;
                if (*ss == '>') {
                    ss++;
                    fp = fopen(ss, "a");
                }
                else
                    fp = fopen(ss, "w");
                if (!fp)
                    perror(ss);
                else {
                    FILE *tmp = TTY.outfile();
                    TTY.ioRedirect(0, fp, 0);
                    display(t);
                    TTY.ioRedirect(0, tmp, 0);
                    fclose(fp);
                }
                continue;
            }

        case 'q':
            quitflag = true;
            *parent = 0;
            return (0);

        case 'p':
        case '\0':
        case '\n':
        case '\r':
            *parent = t;
            return (0);
        }
        if (!isdigit(*s)) {
            TTY.err_printf("Invalid command\n");
            continue;
        }
        int num = atoi(s);
        if (num <= 0) {
            TTY.err_printf("Bad choice.\n");
            continue;
        }
        HLPtopList *tl;
        for (tl = t->subtopics(); tl; tl = tl->next())
            if (--num == 0)
                break;
        if (num) {
            for (tl = t->seealso(); tl; tl = tl->next())
                if (--num == 0)
                    break;
        }
        if (num) {
            TTY.err_printf("Bad choice.\n");
            continue;
        }
        *parent = t;
        return (tl);
    }
}


// Figure out the number of columns we can use.  Assume an entry like
// nn) word -- add 5 characters to the width...
//
int
text_help::putlist(HLPtopic *t, HLPtopList *tl, int base)
{
    int width;
    TTY.winsize(&width, 0);
    width -= 4;
    int maxwidth = 0;
    int nbuts = 0;
    stringlist *s0 = 0, *se = 0;
    for (HLPtopList *tt = tl; tt; tt = tt->next()) {
        stringlist *s =
            new stringlist(t->strip_html(80, tt->description()), 0);
        if ((int)strlen(s->string) + 5 > maxwidth)
            maxwidth = strlen(s->string) + 5;
        nbuts++;
        if (!s0)
            s0 = s;
        else
            se->next = s;
        se = s;
    }
    int ncols = width / maxwidth;
    if (!ncols) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "selection title too long.\n");
        return (0);
    }
    if (ncols > nbuts)
        ncols = nbuts;
    maxwidth = width / ncols;
    int nrows = nbuts / ncols;
    if (nrows * ncols < nbuts)
        nrows++;

    for (int i = 0; i < nrows; i++) {
        stringlist *sl = s0;
        for (int j = 0; j < i; j++, sl = sl->next) ;
        for (int j = 0; j < ncols; j++) {
            if (sl)
                TTY.printf("%2d) %-*s ", base + j * nrows + i + 1,
                    maxwidth - 5, sl->string);
            for (int k = 0; k < nrows; k++)
                if (sl)
                    sl = sl->next;
            
        }
        TTY.send("\n");
    }
    stringlist::destroy(s0);
    return (nbuts);
}

#endif  //HAVE_MOZY

