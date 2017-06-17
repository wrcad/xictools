
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: quote.cc,v 2.39 2010/03/10 06:15:03 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "cshell.h"


// Various things for quoting words. If this is not ascii, quote and
// strip are no-ops, so '' and \ quoting won't work.  To fix this, sell
// your IBM machine.


// Strip all the 8th bits from a string (destructively).
//
void
CshPar::Strip(char *str)
{
    if (str) {
        while (*str) {
            *str = STRIP(*str);
            str++;
        }
    }
}


// Quote all characters in a word.
//
void
CshPar::QuoteWord(char *str)
{
    if (str) {
        while (*str) {
            *str = QUOTE(*str);
            str++;
        }
    }
}


// Print a word (strip the word first).
//
void
CshPar::PrintWord(const char *string, FILE *fp)
{
    if (string) {
        for (const char *s = string; *s; s++)
            putc((STRIP(*s)), fp);
    }
}


// (Destructively) strip all the words in a wlist.
//
void
CshPar::StripList(wordlist *wlist)
{
    for (wordlist *wl = wlist; wl; wl = wl->wl_next)
        Strip(wl->wl_word);
}


// Remove the "" from a string.
//
void
CshPar::Unquote(char *string)
{
    if (string == 0)
        return;
    int l = strlen(string) - 1;
    if (string[l] == '"' && *string == '"') {
        string[l] = '\0';
        char *s = string;
        while (*s) {
            *s = *(s+1);
            s++;
        }
    }
}

