
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

#ifndef TTYIO_H
#define TTYIO_H

#include <stdio.h>


//
// Application Input/Output control.
//

struct wordlist;

// The application must implement this interface:
//
struct sTTYioIF
{
    bool isInteractive();
        // Return true if application is interactive.

    bool getWaiting();
        // Return true is appplication is prompting for input.

    void setWaiting(bool);
        // Set the waiting flag.

    int getchar();
        // Get one character from terminal input.

    wordlist *getWordlist();
        // Get a string as a wordlist from standard input.

    void stripList(wordlist*);
        // Strip the shell's quote chars from words.
};

extern sTTYioIF TTYif;


// Save IO context.
//
struct sTTYcx
{
    sTTYcx(FILE *in, FILE *out, FILE *err, sTTYcx *cx)
        {
            fp_saved_in = in;
            fp_saved_out = out;
            fp_saved_err = err;
            fp_old_out = 0;
            next = cx;
        }

    FILE *fp_saved_in;
    FILE *fp_saved_out;
    FILE *fp_saved_err;
    FILE *fp_old_out;
    sTTYcx *next;
};


// Main struct.
//
struct sTTYio
{
    sTTYio();

    void ioRedirect(FILE*, FILE*, FILE*);
    void ioReset();
    void ioPush(FILE* = 0);
    void ioPop();
    void ioTop();
    void ioOverride(FILE*, FILE*, FILE*);
    void fixDescriptors();
    void winsize(int*, int*);
    int  getpgrp();
    void init_more();
    void promptreturn();
    void send(const char*);
    void printf(const char*, ...);
    void printf_force(const char*, ...);
    void out_printf(const char*, ...);
    void err_printf(const char*, ...);
    void outc(int);
    void wlprint(const wordlist*);
    void flush();
    char *prompt_for_input(char*, int, const char*, bool = false);
    bool prompt_for_yn(bool, const char*);

    int  outfileno()        { return (fileno(fp_out)); }
    int  infileno()         { return (fileno(fp_in)); }
    FILE *outfile()         { return (fp_out); }
    FILE *infile()          { return (fp_in); }
    bool is_tty()           { return (t_isatty); }
    void set_tty(bool i)    { t_isatty = i; }
    void setmore(bool on)   { t_moremode = on; };
    bool morestate()        { return (t_moremode); };
    void setwidth(int w)    { t_width = w; };
    int getwidth()          { return (t_width); };
    void setheight(int h)   { t_height = h; };
    int getheight()         { return (t_height); };
    void monitor()          { t_ref_out_count = t_out_count; }
    bool wasoutput()        { return (t_out_count != t_ref_out_count); }

private:
    FILE *fp_in;
    FILE *fp_out;
    FILE *fp_err;
    FILE *t_fp_curin;
    FILE *t_fp_curout;
    FILE *t_fp_curerr;
    sTTYcx *t_context;
    int t_width;            // value set with "width" variable
    int t_height;           // value set with "height" variable
    int t_xsize;
    int t_ysize;
    int t_xpos;
    int t_ypos;
    int t_out_count;
    int t_ref_out_count;
    bool t_noprint;
    bool t_nopause;
    bool t_oneline;
    bool t_moremode;
    bool t_isatty;
    bool t_hidechars;
};

extern sTTYio TTY;

#endif // TTYIO_H

