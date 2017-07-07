
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
 $Id: history.cc,v 2.49 2015/06/20 01:58:11 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "cshell.h"
#include "commands.h"
#include "graphics.h"


//
// Do history substitutions.
//

namespace {
    wordlist *dohmod(char**, wordlist*);
    char *dohs(const char*, const char*);
}


// The "history" command. history [-r] [number]
//
void
CommandTab::com_history(wordlist *wl)
{
    bool rev = false;
    if (wl && lstring::eq(wl->wl_word, "-r")) {
        wl = wl->wl_next;
        rev = true;
    }
    if (wl == 0)
        CP.HistPrint(CP.Event() - 1, CP.Event() - CP.HistLength(), rev);
    else
        CP.HistPrint(CP.Event() - 1, CP.Event() - 1 - atoi(wl->wl_word),
            rev);
}
// End of CommandTab functions.


// First check for a ^ at the beginning
// of the line, and then search each word for !. Following this can be any
// of string, number, ?string, -number ; then there may be a word specifier, 
// the same as csh, and then the : modifiers. For the :s modifier, 
// the syntax is :sXoooXnnnX, where X is any character, and ooo and nnn are
// strings not containing X.
//
void
CshPar::HistSubst(wordlist **list)
{
    // Replace ^old^new with !:s^old^new
    if (list == 0)
        return;
    wordlist *wlist = *list;
    cp_didhsubst = false;
    char buf[BSIZE_SP];
    if (*wlist->wl_word == cp_hat) {
        sprintf(buf, "%c%c:s%s", cp_bang, cp_bang, wlist->wl_word);
        delete [] wlist->wl_word;
        wlist->wl_word = lstring::copy(buf);
    }
    for (wordlist *w = wlist; w; w = w->wl_next) {
        int k = 0;
        char *b = w->wl_word;
        for (char *s = b; *s; s++, k++) {
            if (*s == cp_bang) {
                cp_didhsubst = true;
                wordlist *n = dohsubst(s + 1);
                if (!n) {
                    wordlist::destroy(wlist);
                    *list = 0;
                    return;
                }
                if (k) {
                    sprintf(buf, "%.*s%s", k, b, n->wl_word);
                    delete [] n->wl_word;
                    n->wl_word = lstring::copy(buf);
                }
                wordlist *nwl = w->splice(n);
                if (wlist == w)
                    wlist = n;
                w = nwl;
                break;
            }
        }
    }
    *list = wlist;
}


// Add a wordlist to the history list. (Done after the first parse.) Note
// that if event numbers are given in a random order that's how they'll
// show up in the history list.
//
void
CshPar::AddHistEnt(int event, wordlist *wlist)
{
    if (cp_lastone && !cp_lastone->text())
        GRpkgIf()->ErrPrintf(ET_INTERR, "bad history list.\n");
    if (cp_lastone == 0)
        cp_lastone = new sHistEnt(event, wlist);
    else {
        cp_lastone->set_next(new sHistEnt(event, wlist));
        cp_lastone->next()->set_prev(cp_lastone);
        cp_lastone = cp_lastone->next();
    }
    freehist(cp_histlength - cp_maxhistlength);
    cp_histlength++;
}


// Print out history between eventhi and eventlo. 
// This doesn't remember quoting, so 'hodedo' prints as hodedo.
//
void
CshPar::HistPrint(int eventhi, int eventlo, bool rev)
{
    sHistEnt *hi;
    if (rev) {
        for (hi = cp_lastone; hi; hi = hi->prev()) {
            if ((hi->event() <= eventhi) && (hi->event() >= eventlo) &&
                    hi->text()) {
                TTY.printf("%d\t", hi->event());
                TTY.wlprint(hi->text());
                TTY.send("\n");
            }
        }
    }
    else if (cp_lastone) {
        for (hi = cp_lastone; hi->prev(); hi = hi->prev()) ;
        for (; hi; hi = hi->next()) {
            if ((hi->event() <= eventhi) && (hi->event() >= eventlo) &&
                    hi->text()) {
                TTY.printf("%d\t", hi->event());
                TTY.wlprint(hi->text());
                TTY.send("\n");
            }
        }
    }
}


// Do a history substitution on one word. Figure out which event is
// being referenced, then do word selections and modifications, and
// then stick anything left over on the end of the last word.
//
wordlist *
CshPar::dohsubst(char *string)
{
    char buf[BSIZE_SP], *r = 0;
    wordlist *wl;
    if (*string == cp_bang) {
        if (cp_lastone) {
            wl = cp_lastone->text();
            string++;
        }
        else {
            GRpkgIf()->ErrPrintf(ET_MSG, "0: event not found.\n");
            return (0);
        }
    }
    else {
        switch(*string) {
        case '-':
            wl = getevent(cp_event - lstring::scannum(++string));
            if (!wl)
                return (0);
            while (isdigit(*string))
                string++;
            break;

        case '?':
            strcpy(buf, string + 1);
            {
                char *s;
                if ((s = strchr(buf, '?')) != 0)
                    *s = '\0';
                wl = hpattern(buf);
                if (!wl)
                    return (0);
                if (s == 0) // No modifiers on this one.
                    return (wordlist::copy(wl));
            }
            break;

        case '\0':  // Maybe this should be cp_event
            wl = new wordlist("!", 0);
            cp_didhsubst = false;
            return (wl);

        default:
            if (isdigit(*string)) {
                wl = getevent(lstring::scannum(string));
                if (!wl)
                    return (0);
                while (isdigit(*string))
                    string++;
            }
            else {
                strcpy(buf, string);
                for (const char *s = ":^$*-%"; *s; s++) {
                    char *t = strchr(buf, *s);
                    if (t && ((t < r) || !r)) {
                        r = t;
                        string += r - buf;
                    }
                }
                if (r)
                    *r = '\0';
                else 
                    while (*string)
                        string++;
                if ((buf[0] == '\0') && cp_lastone)
                    wl = cp_lastone->text();
                else
                    wl = hprefix(buf);
                if (!wl)
                    return (0);
            }
        }
    }
    if (wl == 0) {   // Shouldn't happen
        GRpkgIf()->ErrPrintf(ET_MSG, "Event not found.\n");
        return (0);
    }
    wordlist *nwl = dohmod(&string, wordlist::copy(wl));
    if (!nwl)
        return (0);
    if (*string) {
        for (wl = nwl; wl->wl_next; wl = wl->wl_next) ;
        sprintf(buf, "%s%s", wl->wl_word, string);
        delete [] wl->wl_word;
        wl->wl_word = lstring::copy(buf);
    }
    return (nwl);
}


// Look for an event with a pattern in it...
//
wordlist *
CshPar::hpattern(const char *buf)
{
    if (*buf == '\0') {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad pattern specification.\n");
        return (0);
    }
    for (sHistEnt *hi = cp_lastone; hi; hi = hi->prev()) {
        for (wordlist *wl = hi->text(); wl; wl = wl->wl_next)
            if (lstring::substring(buf, wl->wl_word))
                return (hi->text());
    }
    GRpkgIf()->ErrPrintf(ET_MSG, "%s: event not found.\n", buf);
    return (0);
}


wordlist *
CshPar::hprefix(const char *buf)
{
    if (*buf == '\0') {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad pattern specification.\n");
        return (0);
    }
    for (sHistEnt *hi = cp_lastone; hi; hi = hi->prev()) {
        if (hi->text() && lstring::prefix(buf, hi->text()->wl_word))
            return (hi->text());
    }
    GRpkgIf()->ErrPrintf(ET_MSG, "%s: event not found.\n", buf);
    return (0);
}


// Get a copy of the wordlist associated with an event. Error if out 
// of range. 
//
wordlist *
CshPar::getevent(int num)
{
    sHistEnt *hi;
    for (hi = cp_lastone; hi; hi = hi->prev()) {
        if (hi->event() == num)
            break;
    }
    if (hi == 0) {
        GRpkgIf()->ErrPrintf(ET_MSG, "%d: event not found.\n", num);
        return (0);
    }
    return (hi->text());
}


// This just gets rid of the first num entries on the history list, and
// decrements cp_histlength.
//
void
CshPar::freehist(int num)
{
    if (num < 1 || !cp_lastone)
        return;
    cp_histlength -= num;
    sHistEnt *hi, *ht;
    if (cp_histlength <= 0) {
        cp_histlength = 0;
        for (ht = cp_lastone; ht; ht = hi) {
            hi = ht->prev();
            delete ht;
        }
        cp_lastone = 0;
        return;
    }

    num = cp_histlength;
    hi = cp_lastone;
    while (num-- && hi->prev())
        hi = hi->prev();
    if (hi->prev()) {
        ht = hi->prev();
        hi->set_prev(0);
        for (; ht; ht = hi) {
            hi = ht->prev();
            delete ht;
        }
    }
    else
        GRpkgIf()->ErrPrintf(ET_INTERR, "history list mangled.\n");
}


namespace {
    // Modify the wordlist according to string.  Null is returned if the
    // modifier fails, and the given wl is freed.
    //
    wordlist *dohmod(char **string, wordlist *wl)
    {
        do {
            int numwords = wordlist::length(wl);
            bool globalsubst = false;
            int eventlo = 0;
            int eventhi = numwords - 1;

            // Now we know what wordlist we want.  Take care of
            // modifiers now.
            char *r = 0;
            for (const char *s = ":^$*-%"; *s; s++) {
                char *t = strchr(*string, *s);
                if (t && ((t < r) || (r == 0)))
                    r = t;
            }
            if (!r)     // No more modifiers
                return (wl);

            *string = r;
            if (**string == ':')
                (*string)++;

            switch(**string) {
            case '$':   /* Last word. */
                eventhi = eventlo = numwords - 1;
                break;
            case '*':   // Words 1 through $
                if (numwords == 1)
                    return (0);
                eventlo = 1;
                eventhi = numwords - 1;
                break;
            case '-':   // Words 0 through ...
                eventlo = 0;
                if (*(*string + 1))
                    eventhi = lstring::scannum(*string + 1);
                else
                    eventhi = numwords - 1;
                if (eventhi > numwords - 1)
                    eventhi = numwords - 1;
                break;
            case 'p':
                // Print the command and don't execute it. 
                // This doesn't work quite like csh.
                //
                TTY.init_more();
                TTY.wlprint(wl);
                TTY.send("\n");
                wordlist::destroy(wl);
                return (0);
            case 's':   // Do a substitution
                {
                    bool didsub = false;
                    for (wordlist *w = wl; w; w = w->wl_next) {
                        char *s = dohs(*string + 1, w->wl_word);
                        if (s) {
                            delete [] w->wl_word;
                            w->wl_word = s;
                            didsub = true;
                            if (globalsubst == false) {
                                while (**string)
                                    (*string)++;
                                break;
                            }
                        }
                    }
                    if (!didsub) {
                        GRpkgIf()->ErrPrintf(ET_ERROR, "modifier failed.\n");
                        wordlist::destroy(wl);
                        return (0);
                    }
                }
                break;
            default:
                if (!isdigit(**string)) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "%s: bad modifier.\n", 
                        *string);
                    wordlist::destroy(wl);
                    return (0);
                }
                int i = lstring::scannum(*string);
                if (i > eventhi) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "bad event number %d\n", i);
                    wordlist::destroy(wl);
                    return (0);
                }
                eventhi = eventlo = i;
                while (isdigit(**string))
                    (*string)++;
                if (**string == '*')
                    eventhi = numwords - 1;
                if (**string == '-') {
                    if (!isdigit(*(*string + 1)))
                        eventhi = numwords - 1;
                    else {
                        eventhi = lstring::scannum(++*string);
                        while (isdigit(**string))
                            (*string)++;
                    }
                }
            }
            // Now change the word list accordingly and make another pass
            // if there is more of the substitute left.
            
            wl = wordlist::range(wl, eventlo, eventhi);
            numwords = wordlist::length(wl);
        } while (**string && *++*string);
        return (wl);
    }


    // Do a :s substitution.
    //
    char *dohs(const char *patrn, const char *str)
    {
        if (patrn == 0)
            return (0);
        char pbuf[BSIZE_SP];
        strcpy(pbuf, patrn);
        char *pat = pbuf;
        char schar = *pat++;
        char *s = strchr(pat, schar);
        if (s == 0) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "bad substitute.\n");
            return (0);
        }
        *s++ = '\0';
        char *p = strchr(s, schar);
        if (p)
            *p = '\0';
        int plen = strlen(pat) - 1;
        bool ok = false;
        char buf[BSIZE_SP];
        int i;
        for (i = 0; *str; str++) {
            if ((*str == *pat) &&
                    lstring::prefix(pat, str) && (ok == false)) {
                for (p = s; *p; p++)
                    buf[i++] = *p;
                str += plen;
                ok = true;
            }
            else
                buf[i++] = *str;
        }
        buf[i] = '\0';
        if (ok)
            return (lstring::copy(buf));
        return (0);
    }
}

