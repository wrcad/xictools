
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
 $Id: inpcom.cc,v 2.95 2015/09/30 04:37:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "outplot.h"
#include "spglobal.h"
#include "frontend.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "commands.h"
#include "inpline.h"
#include "lstring.h"
#include "pathlist.h"
#include "filestat.h"
#include "update_itf.h"
#include <sys/types.h>
#include <sys/stat.h>


//
// Listing and editing of input files.
//

// define this to enable Internet file access via http/ftp
#define USE_TRANSACT

#ifdef USE_TRANSACT
#ifndef WIN32
#define USE_GTK
#endif
#include "httpget/transact.h"
namespace { FILE *net_callback(const char*, char**); }
#endif

namespace {
    bool callback(const char*, void*, XEtype);
    int filedate(const char*);
}


// Edit and re-load the current input deck.  In this version, the previous
// circuit (before editing) is kept, but is no longer associated with the
// original file name, unless "-r" option is given. "-n" supresses source
// after edit;
//
void
CommandTab::com_edit(wordlist *wl)
{
    const char *filename = 0;
    bool permfile = false, nosource = false, reuse = false;
    while (wl) {
        // suppress sourcing if "-n" given
        // reuse ckt structure if "-r" given
        if (lstring::eq(wl->wl_word, "-n"))
            nosource = true;
        else if (lstring::eq(wl->wl_word, "-r"))
            reuse = true;
        else if (!filename) {
            filename = wl->wl_word;
            permfile = true;
        }
        wl = wl->wl_next;
    }

    char filebuf[64];
    if (!filename) {
        if (Sp.CurCircuit() && Sp.CurCircuit()->filename()) {
            // know the circuit and file
            filename = Sp.CurCircuit()->filename();
            permfile = true;
        }
        else {
            // work with temp file
            filename = filestat::make_temp("sp");
            filestat::queue_deletion(filename);
            strcpy(filebuf, filename);
            delete [] filename;
            filename = filebuf;
            permfile = false;
        }
        if (Sp.CurCircuit() && !Sp.CurCircuit()->filename()) {
            // Circuit came from file which was subsequently modified.
            // Dump circuit listing into temp file.
            //
            FILE *fp;
            if (!(fp = fopen(filename, "w"))) {
                GRpkgIf()->Perror(filename);
                return;
            }
            Sp.Listing(fp, Sp.CurCircuit()->origdeck() ?
                Sp.CurCircuit()->origdeck() : Sp.CurCircuit()->deck(),
                Sp.CurCircuit()->options(), LS_DECK);
            GRpkgIf()->ErrPrintf(ET_WARN,
                "editing a temporary file -- circuit not saved.\n");
            fclose(fp);
        }
        else if (!Sp.CurCircuit()) {
            // No current circuit, user must create (using temp file)
            FILE *fp;
            if (!(fp = fopen(filename, "w"))) {
                GRpkgIf()->Perror(filename);
                return;
            }
            fprintf(fp, "WRspice test deck\n");
            fclose(fp);
        }
    }

    // suppress source if file is not written
    int date = filedate(filename);
    bool usex;
    if (Sp.Edit(filename, (void(*)(char*, bool, int))callback, true,
            &usex) || usex)
        return;
    if (date == filedate(filename) && date) {
        // file was not modified
        for (sFtCirc *old = Sp.CircuitList(); old; old = old->next()) {
            if (old->filename() && lstring::eq(old->filename(), filename))
                // already sourced
                return;
        }
    }

    if (!nosource)
        Sp.EditSource(filename, permfile, reuse);
}


void
CommandTab::com_xeditor(wordlist *wl)
{
    if (!GRpkgIf()->CurDev()) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "internal editor not available without graphics.");
        return;
    }
    GRwbag *cx = GRpkgIf()->MainDev()->NewWbag("xeditor", 0);
    cx->SetCreateTopLevel();
    cx->PopUpTextEditor(wl ? wl->wl_word : 0, callback, (void*)1, true);
}


// Do a listing. Use is listing [expanded] [logical] [physical] [deck].
//
void
CommandTab::com_listing(wordlist *wl)
{
    int type = LS_LOGICAL;
    bool expand = false;
    bool no_cont = false;
    if (Sp.CurCircuit()) {
        while (wl) {
            char *s = wl->wl_word;
            while (isspace(*s) || *s == Global.OptChar())
                s++;
            switch (*s) {
            case 'l':
            case 'L':
                type = LS_LOGICAL;
                break;
            case 'p':
            case 'P':
                type = LS_PHYSICAL;
                break;
            case 'd':
            case 'D':
                type = LS_DECK;
                break;
            case 'e':
            case 'E':
                expand = true;
                break;
            case 'n':
            case 'N':
                no_cont = true;
                break;
            default:
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad listing type %s.\n", s);
            }
            wl = wl->wl_next;
        }
        if (type == LS_DECK && no_cont)
            type |= LS_NOCONT;
        if (expand)
            type |= LS_EXPAND;

        Sp.Listing(0,
            expand ? Sp.CurCircuit()->deck() : Sp.CurCircuit()->origdeck(),
            expand ? Sp.CurCircuit()->options() : 0, type);
    }
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "no circuit loaded.\n");
}
// End of CommandTab functions.


bool
IFsimulator::Edit(const char *filename, void (*callback)(char*, bool, int),
    bool havesource, bool *usex)
{
    const char *editor;
    VTvalue vv;
    if (Sp.GetVar(kw_editor, VTYP_STRING, &vv) &&
            vv.get_string() && *vv.get_string())
        editor = vv.get_string();
    else if (Global.Editor() && *Global.Editor())
        editor = Global.Editor();
    else
        editor = "vi";
    if (lstring::eq(editor, "xeditor")) {
        if (CP.Display() && GRpkgIf()->CurDev()) {
            *usex = true;
            GRwbag *cx = GRpkgIf()->MainDev()->NewWbag("xeditor", 0);
            cx->SetCreateTopLevel();
            cx->PopUpTextEditor(filename,
                (bool(*)(const char*, void*, XEtype))callback, (void*)1,
                havesource);
            return (false);
        }
        editor = "vi";
    }
    *usex = false;
    char buf[BSIZE_SP];
    sprintf(buf, "%s %s", editor, filename);
    if (!(CP.Display() && GRpkgIf()->CurDev()) ||
            Sp.GetVar("noeditwin", VTYP_BOOL, 0)) {
        int i = CP.System(buf);
        if (CP.GetFlag(CP_WAITING))
            CP.Prompt();
        return (i);
    }
    return (GP.PopUpXterm(buf));
}


namespace {
    // Break up long lines using SPICE '+' continuation.
    //
    wordlist *break_long_line(const char *line)
    {
        int width = TTY.getwidth();
        if (width < 40)
            width = DEF_WIDTH;
        width -= 4;

        wordlist *w0 = 0, *we = 0;
        const char *start = line;
        for (;;) {
            int cnt = 0;
            for (const char *s = start; ; s++) {
                cnt++;
                if (cnt == width) {
                    for (int i = 0; i < 16; i++) {
                        if (isspace(s[-i])) {
                            s -= i;
                            cnt -= i;
                            break;
                        }
                    }
                    if (!w0) {
                        char *t = new char[cnt+1];
                        memcpy(t, start, cnt);
                        t[cnt] = 0;
                        w0 = we = new wordlist(t, 0);
                    }
                    else {
                        char *t = new char[cnt+2];
                        t[0] = '+';
                        memcpy(t+1, start, cnt);
                        t[cnt+1] = 0;
                        we->wl_next = new wordlist(t, 0);
                        we = we->wl_next;
                    }
                    start = s;
                    break;
                }
                if (!*s) {
                    // Return null if line does not need breaking.
                    if (!w0)
                        return (0);
                    char *t = new char[cnt+2];
                    t[0] = '+';
                    strcpy(t+1, start);
                    we->wl_next = new wordlist(t, 0);
                    we = we->wl_next;
                    return (w0);
                }
            }
        }
        return (0);  // not reached.
    }
}


// Provide an input listing on the specified file of the given card
// deck.  The listing should be of either LS_PHYSICAL or LS_LOGICAL or
// LS_DECK lines as specified by the flag bits.
//
void
IFsimulator::Listing(FILE *file, sLine *deck, sLine *extras, int flags)
{
    if (!ft_curckt)
        return;
    LStype type = LS_TYPE(flags);
    bool expand = (flags & LS_EXPAND);
    bool no_cont = (flags & LS_NOCONT);

    bool useout = !file;
    if (useout)
        TTY.init_more();

    if (type != LS_DECK) {
        if (useout) {
            TTY.printf("\t%s\n", ft_curckt->descr());
            if (ft_curckt->execs()->name())
                TTY.printf("\tBound exec codeblock: %s\n",
                    ft_curckt->execs()->name());
            if (ft_curckt->controls()->name())
                TTY.printf("\tBound control codeblock: %s\n",
                    ft_curckt->controls()->name());
            TTY.send("\n");
        }
        else {
            fprintf(file, "\t%s\n", ft_curckt->descr());
            if (ft_curckt->execs()->name())
                fprintf(file, "\tBound exec codeblock: %s\n",
                    ft_curckt->execs()->name());
            if (ft_curckt->controls()->name())
                fprintf(file, "\tBound control codeblock: %s\n",
                    ft_curckt->controls()->name());
            fputc('\n', file);
        }
    }

    int i = 1;
    bool renumber = GetVar(spkw_renumber, VTYP_BOOL, 0);
    if (type == LS_LOGICAL) {
        sLine *endcard = 0;
        bool ending = false;
        for (;;) {
            for (sLine *here = deck; here; here = here->next()) {
                if (!ending && lstring::cieq(here->line(), ".end")) {
                    // Defer this till after extras.
                    endcard = here;
                    break;
                }
                if (renumber)
                    here->set_line_num(i);
                i++;
                if (*here->line() != '*') {
                    if (useout) {
                        TTY.printf("%6d : ", here->line_num());
                        TTY.send(here->line());
                        TTY.send("\n");
                    }
                    else {
                        fprintf(file, "%6d : ", here->line_num());
                        fputs(here->line(), file);
                        fputc('\n', file);
                    }
                    if (here->error()) {
                        if (useout) {
                            TTY.send(here->error());
                            TTY.send("\n");
                        }
                        else {
                            fputs(here->error(), file);
                            fputc('\n', file);
                        }
                    }
                }
            }
            if (extras) {
                deck = extras;
                extras = 0;
                continue;
            }
            if (endcard) {
                deck = endcard;
                endcard = 0;
                ending = true;
                continue;
            }
            break;
        }
    }
    else if ((type == LS_PHYSICAL) || (type == LS_DECK)) {
        sLine *endcard = 0;
        bool ending = false;
        for (;;) {
            for (sLine *here = deck; here; here = here->next()) {
                if (!ending && lstring::cieq(here->line(), ".end")) {
                    // Defer this till after extras.
                    endcard = here;
                    break;
                }
                if (expand || here->actual() == 0 || here == deck) {
                    if (renumber)
                        here->set_line_num(i);
                    i++;
                    if (type == LS_PHYSICAL) {
                        if (useout) {
                            TTY.printf("%6d : ", here->line_num());
                            TTY.send(here->line());
                            TTY.send("\n");
                        }
                        else {
                            fprintf(file, "%6d : ", here->line_num());
                            fputs(here->line(), file);
                            fputc('\n', file);
                        }
                    }
                    else {
                        wordlist *w0 =
                            no_cont ? 0 : break_long_line(here->line());
                        if (w0) {
                            for (wordlist *wl = w0; wl; wl = wl->wl_next) {
                                if (useout) {
                                    TTY.send(wl->wl_word);
                                    TTY.send("\n");
                                }
                                else {
                                    fputs(wl->wl_word, file);
                                    fputc('\n', file);
                                }
                            }
                            wordlist::destroy(w0);
                        }
                        else {
                            if (useout) {
                                TTY.send(here->line());
                                TTY.send("\n");
                            }
                            else {
                                fputs(here->line(), file);
                                fputc('\n', file);
                            }
                        }
                    }
                    if (here->error() && (type == LS_PHYSICAL)) {
                        if (useout) {
                            TTY.send(here->error());
                            TTY.send("\n");
                        }
                        else {
                            fputs(here->error(), file);
                            fputc('\n', file);
                        }
                    }
                }
                else {
                    for (sLine *there = here->actual(); there;
                            there = there->next()) {
                        there->set_line_num(i++);
                        if (type == LS_PHYSICAL) {
                            if (useout) {
                                TTY.printf("%6d : ", there->line_num());
                                TTY.send(there->line());
                                TTY.send("\n");
                            }
                            else {
                                fprintf(file, "%6d : ", there->line_num());
                                fputs(there->line(), file);
                                fputc('\n', file);
                            }
                        }
                        else {
                            wordlist *w0 =
                                no_cont ? 0 : break_long_line(there->line());
                            if (w0) {
                                for (wordlist *wl = w0; wl; wl = wl->wl_next) {
                                    if (useout) {
                                        TTY.send(wl->wl_word);
                                        TTY.send("\n");
                                    }
                                    else {
                                        fputs(wl->wl_word, file);
                                        fputc('\n', file);
                                    }
                                }
                            }
                            else {
                                if (useout) {
                                    TTY.send(there->line());
                                    TTY.send("\n");
                                }
                                else {
                                    fputs(there->line(), file);
                                    fputc('\n', file);
                                }
                            }
                        }
                        if (there->error() && (type == LS_PHYSICAL)) {
                            if (useout) {
                                TTY.send(there->error());
                                TTY.send("\n");
                            }
                            else {
                                fputs(there->error(), file);
                                fputc('\n', file);
                            }
                        }
                    }
                    here->set_line_num(i);
                }
            }
            if (extras) {
                deck = extras;
                extras = 0;
                continue;
            }
            if (endcard) {
                deck = endcard;
                endcard = 0;
                ending = true;
                continue;
            }
            break;
        }
    }
    else
        GRpkgIf()->ErrPrintf(ET_INTERR, "Listing: bad type %d.\n", type);
}


FILE *
IFsimulator::PathOpen(const char *namein, const char *mode, bool *no_rewind)
{
    if (no_rewind)
        *no_rewind = false;
#ifdef USE_TRANSACT
    if (lstring::ciprefix("http://", namein) ||
            lstring::ciprefix("ftp://", namein)) {
        if (strchr(mode, 'w') || strchr(mode, 'a') || strchr(mode, '+')) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "can't write file across network.\n");
            return (0);
        }
        return (net_callback(namein, 0));
    }
#endif

    char *name;
    if (*mode == 'w')
        name = pathlist::expand_path(namein, false, true);
    else
        name = FullPath(namein);
    if (name) {
        struct stat st;
        bool stval = (stat(name, &st) == 0);
        if (!stval) {
            if (strchr(mode, 'r')) {
                // reading, and file not found or unreadable
                delete [] name;
                return (0);
            }
        }
        else {
            // make sure this is not a directory
            if (S_ISDIR(st.st_mode)) {
                delete [] name;
                return (0);
            }
            if (S_ISFIFO(st.st_mode)) {
                if (no_rewind)
                    *no_rewind = true;
            }
#ifdef S_IFSOCK
            if (S_ISSOCK(st.st_mode)) {
                if (no_rewind)
                    *no_rewind = true;
            }
#endif
        }
        FILE *fp = fopen(name, mode);
        delete [] name;
        return (fp);
    }
    return (0);
}


#ifdef USE_TRANSACT

namespace {
    // This is called periodically during http/ftp transfers.  Print
    // the message on the prompt line and check for interrupts.  If
    // interrupted, abort download
    //
    bool http_puts(void*, const char *s)
    {
        if (!s)
            return (false);
        fputs(s, stderr);
        if (Sp.GetFlag(FT_INTERRUPT))
            return (true);
        return (false);
    }


    // Callback to download and open a file given as a URL 
    // 
    FILE *net_callback(const char *url, char **filename) 
    { 
        Transaction t; 
        char *u = lstring::gettok(&url);
        t.set_url(u);
        delete [] u;

        // Set proxy, this is handled by updater.
        char *pxy = UpdIf::get_proxy();
        t.set_proxy(pxy);
        delete [] pxy;

        // The url can contain this trailing option.
        while (*url) {
            char *tok = lstring::gettok(&url);
            if (lstring::eq(tok, "-o")) {
                delete [] tok;
                tok = lstring::gettok(&url);
                t.set_destination(tok);
                delete [] tok;
            }
        }
        t.set_puts(http_puts);
        bool tmp_file = false;
        if (!t.destination()) {
            if (!filename || !*filename || !**filename) {
                char *tt = filestat::make_temp("sp"); 
                t.set_destination(tt);
                delete [] tt;
                tmp_file = true;
            }
            else 
                t.set_destination(*filename); 
        }
        if (t.transact() == 0) { 
            FILE *fp = fopen(t.destination(), "r");  
            if (tmp_file)
                filestat::queue_deletion(t.destination()); 
            if (filename && (!*filename || !**filename)) 
                *filename = lstring::copy(t.destination());
            return (fp);
        }
        else {
            if (tmp_file)
                filestat::queue_deletion(t.destination()); 
        }
        return (0);
    }
}

#endif


// Look up the variable sourcepath and try everything in the list in order
// if the file isn't in . and it isn't an abs path name.  Return a copy of
// the full path.
//
char *
IFsimulator::FullPath(const char *name)
{
    // If this is an abs pathname, or there is no sourcepath var, just
    // copy name and return.
    //
    VTvalue vv;
    if (lstring::strdirsep(name) || !Sp.GetVar(kw_sourcepath, VTYP_LIST, &vv))
        return (pathlist::expand_path(name, false, true));

    variable *v = vv.get_list();
    while (v) {
        if (v->type() == VTYP_STRING) {
            char *t = lstring::copy(v->string());
            CP.Strip(t);
            char *path = pathlist::mk_path(t, name);
            delete [] t;
            t = pathlist::expand_path(path, false, true);
            delete [] path;
            path = t;
            if (!access(path, F_OK))
                return (path);
            delete [] path;
        }
        v = v->next();
    }
    return (0);
}


namespace {
    // This gets called when the 'source' button is pressed in the
    // xeditor window.
    //
    bool callback(const char *filename, void *arg, XEtype fromsource)
    {
        if (fromsource == XE_SOURCE) {
            TTY.monitor();
            Sp.EditSource(filename, arg ? true : false, false);
            CP.SetAltPrompt();
            if (TTY.wasoutput())
                CP.Prompt();
        }
        return (true);
    }


    int filedate(const char *filename)
    {
        struct stat st;
        int i = stat(filename, &st);
        if (i)
            st.st_mtime = 0;
        return (st.st_mtime);
    }
}

