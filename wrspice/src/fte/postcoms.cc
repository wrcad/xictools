
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
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "parser.h"
#include "rawfile.h"
#include "csdffile.h"
#include "output.h"
#include "psffile.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "toolbar.h"
#include "circuit.h"
#include "spnumber/hash.h"
#include "spnumber/spnumber.h"
#include "miscutil/filestat.h"
#include <algorithm>


//
// Various post-processor commands having to do with vectors.
//

// Load in a file
//
void
CommandTab::com_load(wordlist *wl)
{
    if (!wl) {
        const char *stmp = OP.getOutDesc()->outFile();
        // Can't read PSF data yet.
        if (cPSFout::is_psf(stmp))
            stmp = 0;
        OP.loadFile(&stmp, true);
    }
    else {
        ToolBar()->UpdatePlots(2);
        char *str = wordlist::flatten(wl);
        const char *s = str;
        while (*s) {
            OP.loadFile(&s, true);
        }
        delete [] str;
        ToolBar()->UpdatePlots(2);
    }
}


namespace {
    const char *numprint(double num, const char *format, char *buf)
    {
        if (!format || !*format)
            return (SPnum.printnum(num));
        snprintf(buf, 64, format, num);
        return (buf);
    }


    const char *numprint2(double re, double im, const char *format, char *buf)
    {
        if (!format || !*format) {
            char *e = lstring::stpcpy(buf, SPnum.printnum(re));
            *e++ = ',';
            *e++ = ' ';
            strcpy(e, SPnum.printnum(im));
        }
        else {
            snprintf(buf, 64, format, re);
            char *e = buf + strlen(buf);
            *e++ = ',';
            *e++ = ' ';
            snprintf(e, 64 - (e-buf), format, im);
        }
        return (buf);
    }


    void print_line(sDvList *dl0, const char *fmtstr, bool printall)
    {
        // See whether we have to prepend plot names to vector names,
        // This is done if the vector list references more than one
        // plot.
        //
        sPlot *pl = dl0->dl_dvec->plot();
        bool plotnames = false;
        for (sDvList *dl = dl0->dl_next; dl; dl = dl->dl_next) {
            if (pl != dl->dl_dvec->plot()) {
                plotnames = true;
                break;
            }
        }
        sDataVec *bv = dl0->dl_dvec;
        sDvList *bl = dl0;

        // Use the vector's scale if possible.
        sDataVec *scale = bv->scale();
        if (scale) {
            for (sDvList *dl = bl->dl_next; dl; dl = dl->dl_next) {
                if (scale != dl->dl_dvec->scale()) {
                    scale = 0;
                    break;
                }
            }
        }

        // No common scale, use the plot's scale.
        if (!scale)
            scale = bv->plot()->scale();
        if (printall && scale) {
            // The scale is not returned with "all", unless it is the
            // only vector or has length 1.

            sDvList *dl;
            for (dl = dl0; dl; dl = dl->dl_next) {
                if (IFoutput::vecEq(dl->dl_dvec, scale))
                    break;
            }
            if (!dl) {
                dl = new sDvList;
                dl->dl_dvec = scale;
                dl->dl_next = dl0;
                dl0 = dl;
            }
        }

        const char *format = 0;
        char fmt_buf[32];
        if (fmtstr) {
            int n = 0;
            char fc = 0;
            for (const char *s = fmtstr; *s; s++) {
                if (isdigit(*s)) {
                    n = atoi(s);
                    while (isdigit(*s))
                        s++;
                    s--;
                }
                else if (*s == 'f' || *s == 'F')
                    fc = 'f';
                else if (*s == 'e' || *s == 'E')
                    fc = 'e';
            }
            if (fc || n > 0) {
                if (n <= 0 || n > 16)
                    n = CP.NumDigits() > 0 ? CP.NumDigits() : 6;
                if (!fc)
                    fc = 'e';
                // Width must be less than 2 tabs!
                snprintf(fmt_buf, sizeof(fmt_buf), "%%-15.%d%c", n, fc);
                format = fmt_buf;
            }
        }

        int width;
        TTY.winsize(&width, 0);
        if (width < 40)
            width = 40;

        char buf[BSIZE_SP];
        char buf2[64];
        for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            char *s = v->basename();
            if (plotnames)
                snprintf(buf, sizeof(buf), "%s.%s", v->plot()->type_name(), s);
            else
                strcpy(buf, s);
            delete [] s;
            for (s = buf; *s; s++) ;
            s--;
            while (isspace(*s)) {
                *s = '\0';
                s--;
            }
            if (v->length() == 1) {
                char *b = lstring::copy(buf);
                if (v->isreal()) {
                    const char *bb = b;
                    double *d = SPnum.parse(&bb, false);
                    snprintf(buf, sizeof(buf), "%s", 
                        format ? numprint(v->realval(0), format, buf2) :
                        SPnum.printnum(v->realval(0), v->units(), false));
                    if (strcmp(b, buf) && (!d || *d != v->realval(0)))
                        TTY.printf("%s = %s\n", b, buf);
                    else
                        TTY.printf("%s\n", buf);
                }
                else {
                    snprintf(buf, sizeof(buf), "%s,%s", 
                        format ? numprint(v->realval(0), format, buf2) :
                        SPnum.printnum(v->realval(0), "", false),
                        format ? numprint(v->imagval(0), format, buf2) :
                        SPnum.printnum(v->imagval(0), v->units(), false));
                    if (strcmp(b, buf))
                        TTY.printf("%s = %s\n", b, buf);
                    else
                        TTY.printf("%s\n", buf);
                }
                delete [] b;
            }
            else {
                int ll = strlen(buf) + 5;
                TTY.printf("%s = ( ", buf);
                for (int i = 0; i < v->length(); i++) {
                    if (v->isreal())
                        strcpy(buf, numprint(v->realval(i), format, buf2));
                    else {
                        strcpy(buf, numprint2(v->realval(i), v->imagval(i),
                            format, buf2));
                    }
                    int n = strlen(buf);
                    if (ll + n >= width-2) {
                        TTY.send("\n  ");
                        ll = 2;
                    }
                    TTY.send(buf);
                    TTY.send(" ");
                    ll += strlen(buf) + 1;
                }
                TTY.send(")\n");
            }
        }
    }


    // Field width for numbers.
    int colwid(bool isscale, bool isfixed, int ndgt)
    {
        if (isscale) {
            int n = CP.NumDigits() > 0 ? CP.NumDigits() : 6;
            return (n + 8);
        }
        if (isfixed)
            return (ndgt + 3);
        return (ndgt + 8);
    }


    void print_col(sDvList *dl0, const char *fmtstr, bool printall)
    {
        sDataVec *bv = dl0->dl_dvec;
        sDvList *bl = dl0;

        // Use the vector's scale if possible.
        sDataVec *scale = bl->dl_dvec->scale();
        if (scale) {
            for (sDvList *dl = bl->dl_next; dl; dl = dl->dl_next) {
                if (scale != dl->dl_dvec->scale()) {
                    scale = 0;
                    break;
                }
            }
        }

        // The print command in responds to the following variables, which
        // can also be set from the options word in the command line.  Most
        // of these apply only to col mode.
        //
        //   height             shell variable
        //   width              shell variable
        // b nopage             shell variable
        //   noprintscale       alias for printnoscale (deprecated)
        // a printautowidth     set width according to columns up to maximum
        // h printnoheader      suppress top header
        // i printnoindex       suppress index
        // p printnopageheader  suppress page header
        // s printnoscale       skip printing scale as first column

        int width, height;
        TTY.winsize(&width, &height);
        if (width < 40)
            width = 40;
        if (height < 20)
            height = 20;

        // Maximum output line width not counting newline character,
        // when autowidth is set.
        int max_width = 2047;

        bool nopage = Sp.GetFlag(FT_NOPAGE) || TTY.is_tty();
        bool noscale = Sp.GetVar(kw_printnoscale, VTYP_BOOL, 0);
        if (!noscale) {
            // Backwards compatibility.
            noscale = Sp.GetVar("noprintscale", VTYP_BOOL, 0);
        }
        bool noheader = Sp.GetVar(kw_printnoheader, VTYP_BOOL, 0);
        bool nopgheader = Sp.GetVar(kw_printnopageheader, VTYP_BOOL, 0);
        bool noindex = Sp.GetVar(kw_printnoindex, VTYP_BOOL, 0);
        bool autowidth = Sp.GetVar(kw_printautowidth, VTYP_BOOL, 0);

        // Process options, if any.  The options token starts with a
        // '/' and contains the following characters, with no space.
        //
        // [number][f]
        //   The number is the significant figures to print, which takes
        //   a default if not given.  If 'f' follows, a fixed-point
        //   format is used, otherwise an exponential format is used.
        // +   Don't negate effect of options that follow.
        // -   Negate the effect of options that follow.
        // a   Take printautowidth as if set, or not set if negated.
        // b   Take nopage as if set, or not set if negated.
        // h   Take printnoheader as if set, or not set if negated.
        // i   Take printnoindex as if set, or not set if negated.
        // p   Take printnopageheader as if set, or not set if negated.
        // s   Take print no scale as if set, or not set if negated.
        // n   Alias for "abhips".

        int ndgt = CP.NumDigits() > 0 ? CP.NumDigits() : 6;
        char fc = 0;
        const char *format = 0;
        char fmt_buf[32];
        if (fmtstr) {
            bool neg = false;
            int n = 0;
            for (const char *s = fmtstr; *s; s++) {
                if (isdigit(*s)) {
                    n = atoi(s);
                    while (isdigit(*s))
                        s++;
                    s--;
                }
                else if (*s == 'f' || *s == 'F')
                    fc = 'f';
                else if (*s == 'e' || *s == 'E')
                    fc = 'e';
                else if (*s == '+')
                    neg = false;
                else if (*s == '-')
                    neg = true;
                else if (*s == 'a' || *s == 'A')
                    autowidth = !neg;
                else if (*s == 'b' || *s == 'B')
                    nopage = !neg;
                else if (*s == 'h' || *s == 'H')
                    noheader = !neg;
                else if (*s == 'i' || *s == 'I')
                    noindex = !neg;
                else if (*s == 'p' || *s == 'P')
                    nopgheader = !neg;
                else if (*s == 's' || *s == 'S')
                    noscale = !neg;
                else if (*s == 'n' || *s == 'N') {
                    autowidth = !neg;
                    nopage = !neg;
                    noheader = !neg;
                    noindex = !neg;
                    nopgheader = !neg;
                    noscale = !neg;
                }
            }
            if (fc || n > 0) {
                if (n > 0 && n <= 16)
                    ndgt = n;
                if (!fc)
                    fc = 'e';
                // Width must be less than 2 tabs!
                snprintf(fmt_buf, sizeof(fmt_buf), "%%.%d%c", ndgt, fc);
                format = fmt_buf;
            }
        }

        // Don't print a scalar scale.
        if (scale && scale->length() == 1)
            scale = 0;
        else if (noscale)
            scale = 0;
        else if (!scale && bv->plot()->num_dimensions()) {
            // No common scale, use the plot's scale.
            scale = bv->plot()->scale();
        }
        if (printall && !scale && !bv->plot()->num_dimensions() &&
                bv->plot()->scale()) {
            // The scale is not returned with "all".
            sDvList *dl;
            for (dl = dl0; dl; dl = dl->dl_next) {
                if (dl->dl_dvec == bv->plot()->scale())
                    break;
            }
            if (!dl) {
                dl = new sDvList;
                dl->dl_dvec = bv->plot()->scale();
                dl->dl_next = dl0;
                dl0 = dl;
                bv = dl0->dl_dvec;
                bl = dl0;
            }
        }

        for (;;) {
            // Make the first vector of every page be the scale...

            sDvList xl;
            if (!noscale) {
                if (scale && !IFoutput::vecEq(bv, scale)) {
                    xl.dl_dvec = scale;
                    xl.dl_next = bl;
                    bl = &xl;
                    bv = xl.dl_dvec;
                }
            }

            int ll = 0;
            if (!noindex)
                ll += 8;  // One tab for index.
            sDvList *dl;
            for (dl = bl; dl; dl = dl->dl_next) {
                int fw = colwid((xl.dl_dvec == dl->dl_dvec), fc == 'f', ndgt);
                if (dl->dl_dvec->isreal())
                    ll += fw+2;
                else
                    ll += fw+fw+4;
                // Make sure we have at least 2 vectors per page...
                int w = autowidth ? max_width : width;
                if ((ll > w) && (dl != bl) && (dl != bl->dl_next))
                    break;
            }
            int linewidth = width;
            if (autowidth) {
                int w = SPMIN(ll, max_width);
                if (w > linewidth)
                    linewidth = w;
            }

            // Print the header on the first page only.
            sPlot *p = bv->plot();
            char buf[BSIZE_SP];
            char buf2[BSIZE_SP];
            int lineno = 0;
            if (!noheader) {
                TTY.send(p->title());
                TTY.send("\n");
                int j = linewidth - strlen(p->name()) - strlen(p->date());
                if (j > 3) {
                    TTY.send(p->name());
                    j -= 2;
                    while (j--)
                        TTY.send(" ");
                    TTY.send(p->date());
                    TTY.send("\n");
                }
                else {
                    TTY.send(p->name());
                    TTY.send("\n");
                    TTY.send(p->date());
                    TTY.send("\n");
                }
                memset(buf2, '-', linewidth);
                buf2[linewidth] = '\n';
                buf2[linewidth+1] = '\0';
                TTY.send(buf2);

                noheader = true;
                lineno = 3;
            }
            if (!nopgheader) {
                // Compose the page header line in buf.

                buf[0] = 0;
                char *e = buf;
                if (!noindex)
                    e = lstring::stpcpy(e, "Index   ");
                for (sDvList *tl = bl; tl && tl != dl; tl = tl->dl_next) {
                    sDataVec *v = tl->dl_dvec;
                    int fw = colwid((xl.dl_dvec == v), fc == 'f', ndgt);
                    if (v->isreal())
                        snprintf(buf2, sizeof(buf2), "%-*s", fw+2, v->name());
                    else {
                        snprintf(buf2, sizeof(buf2), "%-*s", fw+fw+4,
                            v->name());
                    }
                    e = lstring::stpcpy(e, buf2);   
                }
                *e++ = '\n';
                *e = 0;
            }

            int j = 0;          // point index
            int npoints = 0;    // max points
            for (sDvList *tl = bl; tl && tl != dl; tl = tl->dl_next) {
                sDataVec *v = tl->dl_dvec;
                if (v->length() > npoints)
                    npoints = v->length();
            }

            bool newpage = !nopgheader;
            sDvList *xs = scale ? bl : 0;
            for (;;) {

                // New page
                if (newpage) {
                    // Emit the page header.
                    TTY.send(buf);
                    memset(buf2, '-', linewidth);
                    buf2[linewidth] = '\n';
                    buf2[linewidth+1] = '\0';
                    TTY.send(buf2);
                    lineno += 2;
                }

                while ((j < npoints) && (lineno < height)) {
                    if (!noindex)
                        TTY.printf("%-8d", j);
                    bool isfld = false;
                    for (sDvList *tl = bl; tl && tl != dl; tl = tl->dl_next) {
                        sDataVec *v = tl->dl_dvec;
                        int fw = colwid((xl.dl_dvec == v), fc == 'f', ndgt);
                        if (v->length() <= j) {
                            if (v->isreal()) {
                                memset(buf2, ' ', fw+2);
                                buf2[fw+2] = 0;
                            }
                            else {
                                memset(buf2, ' ', fw+fw+4);
                                buf2[fw+fw+4] = 0;
                            }
                            TTY.send(buf2);
                        }
                        else {
                            if (v->isreal()) {
                                TTY.printf("%-*s", fw+2,
                                    numprint(v->realval(j),
                                        xl.dl_dvec == v ? 0 : format, buf2));
                            }
                            else {
                                TTY.printf("%-*s", fw+fw+4,
                                    numprint2(v->realval(j), v->imagval(j),
                                        xl.dl_dvec == v ? 0 : format, buf2));
                            }
                            if (tl != xs)
                                isfld = true;
                        }
                    }
                    TTY.send("\n");
                    if (!isfld && !dl && bl->dl_next) {
                        // This will stop the print when all data points are
                        // printed, for example if the data is length 1,
                        // and the scale is length 1000, we just print the one
                        // point
                        return;
                    }
                    j++;
                    lineno++;
                }
                if (j == npoints) {
                    if (!dl || (!noscale && !dl->dl_next &&
                            bl->dl_dvec == dl->dl_dvec)) {
                        // No more to print.
                        // Don't print just the scale if printed along
                        // side of something before.
                        //
                        return;
                    }

                    // More vectors to print.
                    bl = dl;
                    bv = bl->dl_dvec;
                    TTY.send("\f\n");   // Form feed
                    break;
                }

                // Otherwise go to a new page
                lineno = 0;
                if (nopage)
                    newpage = false;
                else
                    TTY.send("\f\n");   // Form feed
            }
        }
    }
}


// Print out the values of an expression list.
//
void
CommandTab::com_print(wordlist *wl)
{
    static wordlist *wlast;

    if (!wl) {
        if (!wlast) {
            GRpkg::self()->ErrPrintf(ET_ERRORS, "no vectors given.\n");
            return;
        }
        wl = wlast;
    }
    else if (!wl->wl_next && *wl->wl_word == '/') {
        // Nothing but a format.  Get the vectors from wlast.
        if (!wlast) {
            GRpkg::self()->ErrPrintf(ET_ERRORS, "no vectors given.\n");
            return;
        }
        if (*wlast->wl_word == '/') {
            delete [] wlast->wl_word;
            wlast->wl_word = lstring::copy(wl->wl_word);
            wl = wlast;
        }
        else {
            wl = wordlist::copy(wl);
            wl->wl_next = wlast;
            wlast = wl;
        }
    }
    else {
        wordlist::destroy(wlast);
        wlast = wordlist::copy(wl);
        wl = wlast;
        for (wordlist *ww = wl; ww; ww = ww->wl_next) {
            if (lstring::eq(ww->wl_word, ".")) {
                wordlist *wx = Sp.ExtractPrintCmd(0);
                if (!wx) {
                    GRpkg::self()->ErrPrintf(ET_ERRORS,
                        "no vectors found for '.'.\n");
                    return;
                }
                if (ww == wl) {
                    wlast = wx;
                    wl = wx;
                }
                ww = ww->splice(wx);
                continue;
            }
            if (ww->wl_word[0] == '.' && ww->wl_word[1] == '@' &&
                    isdigit(ww->wl_word[2])) {
                int n = atoi(ww->wl_word + 2);
                wordlist *wx = Sp.ExtractPrintCmd(n ? n-1 : n);
                if (!wx) {
                    GRpkg::self()->ErrPrintf(ET_ERRORS,
                        "no vectors found for '.@%d'.\n", n);
                    return;
                }
                if (ww == wl) {
                    wlast = wx;
                    wl = wx;
                }
                ww = ww->splice(wx);
                continue;
            }
        }
    }

    // Grab the format string for later (strip '/').
    const char *format_str = 0;
    if (*wl->wl_word == '/') {
        format_str = wl->wl_word+1;
        wl = wl->wl_next;
    }

    bool col = true;
    bool optgiven = false;
    if (lstring::eq(wl->wl_word, "col")) {
        col = true;
        optgiven = true;
        wl = wl->wl_next;
    }
    else if (lstring::eq(wl->wl_word, "line")) {
        col = false;
        optgiven = true;
        wl = wl->wl_next;
    }

    // See if we are printing "all".
    bool printall = false;
    for (wordlist *wx = wl; wx; wx = wx->wl_next) {
        if (lstring::eq(wx->wl_word, "all")) {
            printall = true;
            break;
        }
    }

    pnlist *n0 = Sp.GetPtree(wl, true);
    if (!n0)
        return;
    sDvList *dl0 = Sp.DvList(n0);
    if (!dl0)
        return;

    if (!optgiven) {
        // Figure out whether col or line should be used...
        col = false;
        for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
            if (dl->dl_dvec->length() > 1) {
                col = true;
                break;
            }
        }
    }

    if (TTY.is_tty())
        TTY.send("\n");

    if (col) {
        // Print in columns.
        print_col(dl0, format_str, printall);
    }
    else {
        // Print in line.
        print_line(dl0, format_str, printall);
    }

    if (TTY.is_tty())
        TTY.send("\n");
    sDvList::destroy(dl0);

    if (OP.curPlot()->notes()) {
        TTY.send("Notes:\n");
        int ncnt = 1;
        for (wordlist *w = OP.curPlot()->notes(); w; w = w->wl_next) {
            TTY.printf("%d. %s\n", ncnt, w->wl_word);
            ncnt++;
        }
        TTY.send("\n");
    }
}


namespace {
    void do_write(const char *file, sPlot *p, bool appendwrite)
    {
        if (!file)
            return;
        const char *dname;
        if ((dname = cPSFout::is_psf(file)) != 0) {
            cPSFout psf(p);
            psf.file_write(dname, appendwrite);
        }
        else if (cCSDFout::is_csdf_ext(file)) {
            cCSDFout csdf(p);
            csdf.file_write(file, appendwrite);
        }
        else {
            cRawOut raw(p);
            raw.file_write(file, appendwrite);
        }
    }
}


// Write out some data. write filename expr ... Some cleverness here is
// required.  If the user mentions a few vectors from various plots,
// probably he means for them to be written out seperate plots.  In any
// case, we have to be sure to write out the scales for everything we
// write...
//
void
CommandTab::com_write(wordlist *wl)
{
    bool appendwrite = Sp.GetVar(kw_appendwrite, VTYP_BOOL, 0);

    const char *file;
    if (wl) {
        file = wl->wl_word;
        wl = wl->wl_next;
    }
    else
        file = OP.getOutDesc()->outFile();
    if (!wl) {
        // just dump the current plot
        if (OP.curPlot()->num_perm_vecs() == 0) {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "plot is empty, nothing written.\n");
            return;
        }
        do_write(file, OP.curPlot(), appendwrite);
        OP.curPlot()->set_written(true);
        return;
    }

    // There may be a "vs" in the arg list.  If so, just ignore it.
    pnlist *names = Sp.GetPtree(wl, false);
    if (names == 0)
        return;
    sDvList *dl0 = Sp.DvList(names);
    if (dl0 == 0)
        return;
    sDvList *dp = 0, *dn;
    for (sDvList *dl = dl0; dl; dl = dn) {
        dn = dl->dl_next;
        if (!dl->dl_dvec) {
            // corresponds to "vs"
            if (!dp)
                // shouldn't happen
                dl0 = dn;
            else
                dp->dl_next = dn;
            delete dl;
            continue;
        }
        dp = dl;
    }

    // Now we have to write them out plot by plot.
    //
    while (dl0) {

        sPlot *tpl = dl0->dl_dvec->plot();
        if (!tpl)
           continue;
        dl0 = tpl->write(dl0, appendwrite, file);

        // If there are more plots we want them appended...
        appendwrite = true;
    }
}


namespace {
    // Copy the struct, but not the data (saves time and space).
    //
    sDataVec *copyvec(sDataVec *d)
    {
        if (!d)
            return (0);
        int ltmp = d->length();
        d->set_length(0);
        sDataVec *v = d->copy();
        d->set_length(ltmp);
        v->set_length(ltmp);
        if (d->isreal())
            v->set_realvec(d->realvec());
        else
            v->set_compvec(d->compvec());
        return (v);
    }
}


sDvList *
sPlot::write(sDvList *dl0, bool appendwrite, const char *file)
{
    set_written(true);
    sPlot newplot(*this);
    newplot.pl_dvecs = 0;
    newplot.pl_hashtab = 0;
    bool scalefound = false;

    // Figure out how many vectors are in this plot. Also look
    // for the scale, or a copy of it, which may have a different
    // name.
    //
    for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
        sDataVec *d = dl->dl_dvec;
        if (d->plot() == this) {
            sDataVec *vv = copyvec(d);
            char *tmpname = d->basename();
            vv->set_name(tmpname);
            delete [] tmpname;
            if (newplot.pl_hashtab == 0)
                newplot.pl_hashtab = new sHtab(sHtab::get_ciflag(CSE_VEC));
            newplot.pl_hashtab->add(vv->name(), vv);
            if (IFoutput::vecEq(d, scale())) {
                newplot.pl_scale = vv;
                scalefound = true;
            }
        }
    }

    // Maybe we shouldn't make sure that the default scale is
    // present if nobody uses it.
    //
    if (!scalefound) {
        sDataVec *vv = copyvec(scale());
        char *tmpname = scale()->basename();
        vv->set_name(tmpname);
        delete [] tmpname; 
        newplot.pl_scale = vv;
        newplot.pl_hashtab->add(vv->name(), vv);
    }

    // Now let's go through and make sure that everything that 
    // has its own scale has it in the plot.
    //
    for (;;) {
        scalefound = false;
        wordlist *wl0 = newplot.list_perm_vecs();
        for (wordlist *tl = wl0; tl; tl = tl->wl_next) {
            sDataVec *d = newplot.get_perm_vec(tl->wl_word);
            if (d->scale()) {
                wordlist *tll;
                for (tll = wl0; tll; tll = tll->wl_next) {
                    sDataVec *vv = newplot.get_perm_vec(tll->wl_word);
                    if (IFoutput::vecEq(vv, d->scale())) {
                        d->set_scale(vv);
                        break;
                    }
                }
                if (!tll) {
                    // We have to grab it...
                    sDataVec *vv = copyvec(d->scale());
                    char *tmpname = d->scale()->basename();
                    vv->set_name(tmpname);
                    delete [] tmpname; 
                    newplot.pl_hashtab->add(vv->name(), vv);
                    d->set_scale(vv);
                    scalefound = true;
                }
            }
        }
        wordlist::destroy(wl0);
        if (!scalefound)
            break;
        // Otherwise loop through again...
    }

    if (sHtab::empty(newplot.pl_hashtab))
        GRpkg::self()->ErrPrintf(ET_WARN, "plot is empty, nothing written.\n");
    else
        do_write(file, &newplot, appendwrite);

    // Now throw out the vectors we have written already...
    sDvList *dp = 0, *dn;
    for (sDvList *dl = dl0; dl; dl = dn) {
        dn = dl->dl_next;
        if (dl->dl_dvec->plot() == this) {
            if (dp == 0)
                dl0 = dn;
            else
                dp->dl_next = dl->dl_next;
            delete dl;
        }
        else
            dp = dl;
    }

    sHgen gen(newplot.pl_hashtab, true);
    sHent *h;
    while ((h = gen.next()) != 0) {
        sDataVec *vv = (sDataVec *)h->data();
        vv->set_realvec(0); // since data is not copied!
        delete vv;
        delete h;
    }
    delete newplot.pl_hashtab;

    // for destructor
    memset((void*)&newplot, 0, sizeof(sPlot));

    return (dl0);
}


namespace {
    inline bool isnum(const char *s)
    {
        while (*s) {
            if (!isdigit(*s))
                return (false);
            s++;
        }
        return (true);
    }

    inline bool nv_cmp(const sCKTnodeVal &n1, const sCKTnodeVal &n2)
    {
        if (isnum(n1.name) && isnum(n2.name))
            return (atoi(n1.name) < atoi(n2.name));
        return (strcmp(n1.name, n2.name) < 0);
    }
}


// Print a sorted listing of node names with the current solution vector
// value.
//
void
CommandTab::com_dumpnodes(wordlist*)
{
    if (!Sp.CurCircuit() || !Sp.CurCircuit()->runckt()) {
        GRpkg::self()->ErrPrintf(ET_ERRORS, "no current circuit nodes.\n");
        return;
    }
    sCKTnodeVal *nvs;
    unsigned int nv = Sp.CurCircuit()->runckt()->nodeVals(&nvs);
    std::sort(nvs, nvs+nv-1, nv_cmp);

    TTY.send("\nNode voltages and branch currents;\n");
    unsigned int nv2 = (nv+1)/2;
    unsigned int cnt = 0;
    for (unsigned int i = 0; i < nv2; i++) {
        TTY.printf("%-24s=%14.6e", nvs[cnt].name, nvs[cnt].value);
        cnt++;
        if (cnt < nv)
            TTY.printf(" %-24s=%14.6e\n", nvs[cnt].name, nvs[cnt].value);
        else
            TTY.send("\n");
        cnt++;
    }
    TTY.send("\n");
    delete [] nvs;
}


void
CommandTab::com_dumpopts(wordlist*)
{
    if (!Sp.CurCircuit()) {
        GRpkg::self()->ErrPrintf(ET_ERRORS, "no current circuit.\n");
        return;
    }
    if (Sp.CurCircuit()->runckt())
        Sp.CurCircuit()->runckt()->CKTcurTask->TSKopts.dump();
    else
        Sp.CurCircuit()->defOpt()->dump();
    TTY.send("\n");
}
