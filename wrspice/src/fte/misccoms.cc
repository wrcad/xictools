
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
 $Id: misccoms.cc,v 2.101 2015/09/28 17:32:13 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "spglobal.h"
#include "input.h"
#include "frontend.h"
#include "ftedata.h"
#include "ftehelp.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "subexpand.h"
#include "random.h"
#include "pathlist.h"
#include "filestat.h"
#include "update_itf.h"
#ifdef WIN32
#include "msw.h"
#endif

#ifdef HAVE_LOCAL_ALLOCATOR
#include "local_malloc.h"
#endif

#define SYSTEM_MAIL       "mail -s \"%s (%s) Bug Report\" %s"


// Quick help, just list the command description string.
//
void
CommandTab::com_qhelp(wordlist *wl)
{
    bool allflag = false;
    if (wl && lstring::cieq(wl->wl_word, "all")) {
        allflag = true;
        wl = 0;
    }

    // We want to use more mode whether "moremode" is set or not.
    bool tmpmm = TTY.morestate();
    TTY.setmore(true);
    TTY.init_more();
    if (wl == 0) {

        // determine environment
        int env = 0;
        if (Sp.PlotList()->next_plot())
            env |= E_HASPLOTS;
        else
            env |= E_NOPLOTS;

        // determine level
        int level = E_INTERMED;
        VTvalue vv;
        if (Sp.GetVar(kw_level, VTYP_STRING, &vv) && vv.get_string()) {
            switch (*vv.get_string()) {
            case 'b':   level = E_BEGINNING;
                break;
            case 'i':   level = E_INTERMED;
                break;
            case 'a':   level = E_ADVANCED;
                break;
            }
        }

        // Sort the commands
        sCommand **cc;
        int numcoms = Cmds.Commands(&cc);
        TTY.send("\n");
        for (int i = 0; i < numcoms; i++) {
            if (allflag || ((cc[i]->co_env <= (unsigned)level) &&
                    (!(cc[i]->co_env & (E_BEGINNING-1)) ||
                    (env & cc[i]->co_env)))) {
                if (cc[i]->co_help == 0)
                    continue;
                TTY.printf("%s ", cc[i]->co_comname);
                TTY.printf(cc[i]->co_help, CP.Program());
                TTY.send("\n");
            }
        }
        delete [] cc;
    }
    else {
        for ( ; wl; wl = wl->wl_next) {
            sCommand *c = Cmds.FindCommand(wl->wl_word);
            if (c) {
                TTY.printf("%s ", c->co_comname);
                TTY.printf(c->co_help, CP.Program());
                TTY.send("\n");
            }
            else {
                // See if this is aliased
                sAlias *al = CP.GetAlias(wl->wl_word);
                if (al == 0)
                    TTY.printf("Sorry, no help for %s.\n", wl->wl_word);
                else {
                    TTY.printf("%s is aliased to ", wl->wl_word);
                    TTY.wlprint(al->text());
                    TTY.send("\n");
                }
            }
        }
    }
    TTY.send("\n");
    TTY.setmore(tmpmm);
}


// The main help command, accesses the help database.
//
void
CommandTab::com_help(wordlist *wl)
{
    VTvalue vv;
    if (Sp.GetVar(kw_helpinitxpos, VTYP_NUM, &vv))
        HLP()->set_init_x(vv.get_int());
    if (Sp.GetVar(kw_helpinitypos, VTYP_NUM, &vv))
        HLP()->set_init_y(vv.get_int());
    if (!wl || !wl->wl_word || !*wl->wl_word) {
        // top of help tree
        static wordlist *wdef;
        //
        // Have to be a little careful.  If we give "wrspice" as the
        // topic, and the help database isn't found, we could load the
        // wrspice executable into the browser if it happens to exist
        // in the cwd.  The "wrspice_top_topic" is aliased to the
        // wrspice topic in the help database.
        //
        if (!wdef)
            wdef = new wordlist("wrspice_top_topic", 0);
        wl = wdef;
    }
    else if (!wl->wl_next && lstring::eq(wl->wl_word, "-c")) {
        HLP()->rehash();
        return;
    }

    // If multiple words are given, the window shows the last word,
    // and the back button will cycle through the words in reverse
    // order.

    HLPwords *h0 = 0, *he = 0;
    for ( ; wl; wl = wl->wl_next) {
        if (!h0)
            h0 = he = new HLPwords(wl->wl_word, 0);
        else {
            he->next = new HLPwords(wl->wl_word, he);
            he = he->next;
        }
    }
    HLP()->list(h0);
    HLPwords::destroy(h0);

    const char *err = HLP()->error_msg();
    if (err)
        GRpkgIf()->ErrPrintf(ET_ERROR, err);
}


void
CommandTab::com_helpreset(wordlist*)
{
    HLP()->rehash();
}


void
CommandTab::com_quit(wordlist*)
{
    const char *zz = "Are you sure you want to quit [y]? ";
    bool noask = Sp.GetVar(kw_noaskquit, VTYP_BOOL, 0);
    
    // Confirm
    if (!noask && !Sp.GetFlag(FT_BATCHMODE) && !CP.GetFlag(CP_NOTTYIO)) {
        int ncc = 0;
        for (sFtCirc *cc = Sp.CircuitList(); cc; cc = cc->next())
            if (cc->inprogress())
                ncc++;
        int npl = 0;
        for (sPlot *pl = Sp.PlotList(); pl; pl = pl->next_plot())
            if (!pl->written() && pl->num_perm_vecs())
                npl++;
        if (ncc || npl) {
            TTY.init_more();
            TTY.printf("Warning: ");
            if (ncc) {
                TTY.printf( 
            "the following simulation%s still in progress:\n",
                        (ncc > 1) ? "s are" : " is");
                for (sFtCirc *cc = Sp.CircuitList(); cc; cc = cc->next())
                    if (cc->inprogress())
                        TTY.printf("\t%s\n", cc->name());
            }
            if (npl) {
                if (ncc)
                    TTY.printf("and ");
                TTY.printf("the following plot%s been saved:\n",
                    (npl > 1) ? "s haven't" : " hasn't");
                for (sPlot *pl = Sp.PlotList(); pl; pl = pl->next_plot())
                    if (!pl->written() && pl->num_perm_vecs())
                        TTY.printf("%s\t%s, %s\n",
                            pl->type_name(), pl->title(), pl->name());
            }
            if (!TTY.prompt_for_yn(true, zz))
                return;
        }
    }
    fatal(false);
}


void
CommandTab::com_bug(wordlist*)
{
    if (!Global.BugAddr() || !*Global.BugAddr()) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "no address to send bug reports to.\n");
            return;
    }
    const char *msg =
        "Please include the OS version number and machine architecture.\n"
        "If the problem is with a specific circuit, please include the\n"
        "input file.\n\n";
    char buf[BSIZE_SP];
    if (GRpkgIf()->CurDev()) {
        TTY.printf(msg);
        sprintf(buf, "WRspice %s bug report", Sp.Version());
        GRwbag *cx = GRpkgIf()->MainDev()->NewWbag("mail", 0);
        cx->SetCreateTopLevel();
        cx->PopUpMail(buf, Global.BugAddr());
    }
    else {
        TTY.printf("Calling the mail program . . .(sending to %s)\n\n",
            Global.BugAddr());
        TTY.printf(msg);
        TTY.printf(
    "You are in the mail program.  Type your message, end with \".\"\n"
    "on its own line.\n\n");

        sprintf(buf, SYSTEM_MAIL, Sp.Simulator(), Sp.Version(),
            Global.BugAddr());
        CP.System(buf);
        TTY.printf("Bug report sent.  Thank you.\n");
    }
}


void
CommandTab::com_version(wordlist *wl)
{
    TTY.init_more();
    if (!wl) {
        TTY.printf("Program: %s, version: %s\n", Sp.Simulator(),
            Sp.Version());
        if (Global.Notice() && *Global.Notice())
            TTY.printf("\t%s\n", Global.Notice());
        if (Global.BuildDate() && *Global.BuildDate())
            TTY.printf("Date built: %s\n", Global.BuildDate());
    }
    else {
        // assume version in form major.minor.rev
        char *s = wordlist::flatten(wl);
        int v1, v2, v3;
        int r1 = sscanf(s, "%d.%d.%d", &v1, &v2, &v3);
        int c1, c2, c3;
        int r2 = sscanf(Sp.Version(), "%d.%d.%d", &c1, &c2, &c3);
        if (r1 != r2 || (r1 >= 1 && v1 > c1) || (r1 >= 2 && v2 > c2) ||
                (r1 == 3 && v3 > c3)) {
            GRpkgIf()->ErrPrintf(ET_WARN,
                "rawfile is version %s (current version is %s).\n",
                wl->wl_word, Sp.Version());
        }
        delete [] s;
    }
}


void
CommandTab::com_seed(wordlist *wl)
{
    int seed;
    if (!wl || !wl->wl_word || sscanf(wl->wl_word, "%d", &seed) != 1)
#ifdef HAVE_GETPID
        seed = getpid();
#else
        seed = 17;
#endif
    Rnd.seed(seed);
}


void
CommandTab::com_passwd(wordlist*)
{
    const char *namsg = "passwd:  command not available.\n";
    if (CP.GetFlag(CP_NOTTYIO)) {
        GRpkgIf()->ErrPrintf(ET_WARN, namsg);
        return;
    }
    if (!Global.UpdateIf()) {
        GRpkgIf()->ErrPrintf(ET_WARN, namsg);
        return;
    }
    char buf[256];
    if (!TTY.prompt_for_input(buf, 256, "Enter user name: ") || !*buf)
        return;
    char *in = buf;
    char *user = lstring::gettok(&in);
    if (!user)
        return;
    GCarray<char*> gc_user(user);
            
    if (!TTY.prompt_for_input(buf, 256, "Enter password: ", true) || !*buf)
        return;

    in = buf;
    char *pw1 = lstring::gettok(&in);
    if (!pw1)
        return;
    GCarray<char*> gc_pw1(pw1);

    if (!TTY.prompt_for_input(buf, 256, "Reenter password: ", true) || !*buf)
        return;

    in = buf;
    char *pw2 = lstring::gettok(&in);
    if (!pw2)
        return;
    GCarray<char*> gc_pw2(pw2);

    if (!lstring::eq(pw1, pw2)) {
        TTY.printf("Try again, you mistyped the password.\n");
        return;
    }

    UpdIf udif(*Global.UpdateIf());
    const char *err = udif.update_pwfile(user, pw1);
    if (err) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "passwd: %s.\n", err);
        return;
    }
    TTY.printf(
    "The .wrpasswd file in your home directory was updated successfully.\n");
}


void
CommandTab::com_proxy(wordlist *wl)
{
    const char *namsg = "proxy:  command not available.\n";
    if (CP.GetFlag(CP_NOTTYIO)) {
        GRpkgIf()->ErrPrintf(ET_WARN, namsg);
        return;
    }
    if (!Global.UpdateIf()) {
        GRpkgIf()->ErrPrintf(ET_WARN, namsg);
        return;
    }
    char buf[256];
    const char *addr = wl ? wl->wl_word : 0;
    if (!addr) {
        if (!TTY.prompt_for_input(buf, 256,
                "Enter proxy internet address: ") || !*buf)
            return;
        addr = buf;
    }

    if (*addr == '-' || *addr == '+') {
        if (!UpdIf::move_proxy(addr))
            TTY.printf("Operation failed: %s.", filestat::error_msg());
        else {
            int c = *addr++;
            if (!*addr)
                addr = "bak";
            if (c == '-') {
                TTY.printf("Move .wrproxy file to .wrproxy.%s succeeded.\n",
                    addr);
            }
            else {
                TTY.printf("Move .wrproxy.%s to .wrproxy file succeeded.\n",
                    addr);
            }
        }
        return;
    }
    if (!lstring::prefix("http:", addr)) {
        TTY.printf("Error: \"http:\" prefix required in address.");
        return;
    }

    bool a_has_port = false;
    const char *e = strrchr(addr, ':');
    if (e) {
        e++;
        if (isdigit(*e)) {
            e++;
            while (isdigit(*e))
                e++;
        }
        if (!*e)
            a_has_port = true;
    }

    const char *port = wl->wl_next ? wl->wl_next->wl_word : 0;
    char pbuf[16];
    if (!a_has_port && !port) {

        if (!TTY.prompt_for_input(pbuf, 16, "Enter port number: "))
            return;
        if (*pbuf)
            port = pbuf;
    }
    if (port) {
        for (const char *c = port; *c; c++) {
            if (!isdigit(*c)) {
                TTY.printf("Error: port is not numeric.");
                return;
            }
        }
    }

    const char *err = UpdIf::set_proxy(addr, port);
    if (err)
        TTY.printf("Operation failed: %s.\n", err);
    else if (port)
        TTY.printf("Created .wrproxy file for %s:%s.\n", addr, port);
    else
        TTY.printf("Created .wrproxy file for %s.\n", addr);
}


// Name of update script file, found in library path.
#define DST_SCRIPT  "wr_install"


#ifdef WIN32
namespace {
    char *msw_exepath;

    // Exit procedure, create a new process to run the install program.
    //
    void msw_install()
    {
        if (!msw_exepath)
            return;
        PROCESS_INFORMATION *info = msw::NewProcess(msw_exepath,
            DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, false);
        (void)info;
    }
}
#endif


void
CommandTab::com_wrupdate(wordlist *wl)
{
    if (CP.GetFlag(CP_NOTTYIO)) {
        GRpkgIf()->ErrPrintf(ET_WARN, "command not available.\n");
        return;
    }
    if (!Global.UpdateIf()) {
        GRpkgIf()->ErrPrintf(ET_WARN, "wrupdate: command not available.\n");
        return;
    }
    UpdIf udif(*Global.UpdateIf());
    if (!udif.username() || !udif.password()) {
        TTY.printf(
            "First use the passwd command to create your .wrpasswd file.\n");
        return;
    }

    char *s = wordlist::flatten(wl);
    GCarray<char*> gc_s(s);
        
    bool os_given = false;
    bool force_dl = false;
    bool no_dl_existing = false;
    char *osname = 0;
    char *prefix = 0;  
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (lstring::eq(tok, "-f")) {
            delete [] tok;
            force_dl = true;
            continue;
        }
        if (lstring::eq(tok, "-fi")) {
            // Undocumented - this will skip the download if the file
            // exists.
            delete [] tok;
            force_dl = true;
            no_dl_existing = true;
            continue;
        }
        if (lstring::eq(tok, "-p")) {
            prefix = lstring::getqtok(&s);
            if (prefix && lstring::is_rooted(prefix)) {
                delete [] tok;
                continue;
            }
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "wrupdate: Bad or missing prefix.\n");
        }
        else if (lstring::eq(tok, "-o")) {
            osname = lstring::gettok(&s);
            if (osname && *osname != '-') {
                delete [] tok;
                os_given = true;
                continue;
            }
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "wrupdate: Bad or missing osname.\n");
        }
        else
            TTY.printf(
                "Syntax error.  Usage: wrupdate [-o osname] [-p prefix]\n");
        delete [] tok;
        delete [] osname;
        delete [] prefix;
        return;
    }

    if (!osname)
        osname = lstring::copy(Global.OSname());
    if (!prefix)
        prefix = lstring::copy(Global.Prefix());

    GCarray<char*> gc_osname(osname);
    GCarray<char*> gc_prefix(prefix);

    release_t my_rel = udif.my_version();
    if (my_rel == release_t(0)) {
        GRpkgIf()->ErrPrintf(ET_INTERR,
            "wrupdate: I can't find my version numbers!\n");
        return;
    }

    char *arch;
    char *suffix;
    char *subdir;
    char *errmsg;
    release_t new_rel = udif.distrib_version(osname, &arch, &suffix, &subdir,
        &errmsg);
    if (new_rel == release_t(0)) {
        GRpkgIf()->ErrPrintf(ET_ERROR,
            "wrupdate: Failed to obtain release number from server:\n%s.\n",
            errmsg);
        delete [] errmsg;
        return;
    }

    GCarray<char*> gc_arch(arch);
    GCarray<char*> gc_suffix(suffix);
    GCarray<char*> gc_subdir(subdir);

    if (!force_dl) {
        if (!os_given && !(my_rel < new_rel)) {
            TTY.printf("You are running the current release of %s.\n",
                Global.Product());
            return;
        }
    }

    // destination directory
    const char *tmpdir = getenv("SPICE_TMP_DIR");
    if (!tmpdir || !*tmpdir)
        tmpdir = getenv("TMPDIR");
    if (!tmpdir || !*tmpdir)
        tmpdir = "/tmp";

    char *dst_file = udif.distrib_filename(new_rel, osname, arch, suffix);
    char *dst_path = pathlist::mk_path(tmpdir, dst_file);

    GCarray<char*> gc_dst_file(dst_file);
    GCarray<char*> gc_dst_path(dst_path);

    bool need_dl = false;
    if (!force_dl || no_dl_existing) {
        FILE *fp = fopen(dst_path, "r");
        if (fp)
            fclose(fp);
        else
            need_dl = true;
    }
    else if (force_dl)
        need_dl = true;

    char buf[256];
    if (need_dl) {
        char *new_rel_str = new_rel.string();
        sprintf(buf, "Download %s distribution file [n]? ",
            os_given ? dst_file : new_rel_str);
        delete [] new_rel_str;

        if (!TTY.prompt_for_yn(false, buf))
            return;

        char *my_dst_file = udif.download(dst_file, osname, subdir, &errmsg);
        if (!my_dst_file) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "wrupdate: Download failed:\n%s.\n", errmsg);
            delete [] errmsg;
            return;
        }
        delete [] my_dst_file;  // same as dst_path
    }

#ifdef WIN32
    if (!atexit(msw_install)) {
        msw_exepath = lstring::copy(dst_path);
        TTY.printf(
            "Exit this program to install update (run %s).", dst_path);
    }
    else {
        TTY.printf(
            "Error: unknown error scheduling exit process.");
    }

#else
    sprintf(buf, "Install %s [n]? ", dst_path);
    if (!TTY.prompt_for_yn(false, buf))
        return;

    VTvalue vv;
    Sp.GetVar(kw_installcmdfmt, VTYP_STRING, &vv);
    const char *cmdfmt = vv.get_string();

    char *scriptfile = pathlist::mk_path(Global.StartupDir(), DST_SCRIPT);
    GCarray<char*> gc_scriptfile(scriptfile);
    FILE *fp = fopen(scriptfile, "r");
    if (fp)
        fclose(fp);
    else {
        GRpkgIf()->ErrPrintf(ET_ERROR, 
            "wrupdate: Can not open update script file.\n");
        return;
    }

    // This will likely fail if there is an osname mismatch to the
    // running operating system.

    int ret = udif.install(scriptfile, cmdfmt, dst_path, prefix, osname,
        &errmsg);
    if (ret != 0) {
        if (errmsg) {
            GRpkgIf()->ErrPrintf(ET_ERROR, "wrupdate: %s\n", errmsg);
            delete [] errmsg;
        }
        else
            GRpkgIf()->ErrPrintf(ET_WARN,
                "wrupdate: install process returned error code %d.\n", ret);
        return;
    }

    // Warning:  a 0 return doesn't necessarily mean that the install
    // was successful - just that there were no config errors and the
    // xterm popped up.

    TTY.printf(
        "Done.  "
        "If install succeeded, restart the program to run new release.\n");
#endif
}


namespace {
    void pr_usage()
    {
        TTY.printf("Usage:\n"
        "\tcache help\n"
        "\tcache l[ist]\n"
        "\tcache d[ump] [tags...]\n"
        "\tcache r[emove] [tags...]\n"
        "\tcache c[lear]\n");
    }

    void pr_tags()
    {
        wordlist *tl = SPcache.listCache();
        if (!tl)
            TTY.printf("Subcircuit/model cache is empty.\n");
        else {
            TTY.printf("Subcircuit/model cache tags:\n");
            for (wordlist *w = tl; w; w = w->wl_next)
                TTY.printf("\t%s\n", w->wl_word);
            TTY.printf("\n");
        }
        wordlist::destroy(tl);
    }
}


void
CommandTab::com_cache(wordlist *wl)
{
    // cache [help|list|dump|remove|clear] [name]

    if (!wl) {
        pr_tags();
        return;
    }

    const char *word = wl->wl_word;
    wl = wl->wl_next;

    if (lstring::cieq(word, "h") || lstring::cieq(word, "?") ||
            lstring::cieq(word, "help")) {
        pr_usage();
    }
    else if (lstring::cieq(word, "l") || lstring::cieq(word, "list")) {
        pr_tags();
    }
    else if (lstring::cieq(word, "d") || lstring::cieq(word, "dump")) {
        if (!wl) {
            wordlist *tl = SPcache.dumpCache(0);
            if (!tl)
                TTY.printf("Subcircuit/model cache is empty.\n");
            else {
                for (wordlist *w = tl; w; w = w->wl_next)
                    TTY.printf("%s\n", w->wl_word);
                TTY.printf("\n");
                wordlist::destroy(tl);
            }
        }
        else {
            for (wordlist *w = wl; w; w = w->wl_next) {
                if (!SPcache.inCache(w->wl_word))
                    TTY.printf("Unknown tag %s.\n", w->wl_word);
                else {
                    wordlist *tl = SPcache.dumpCache(w->wl_word);
                    for (wordlist *t = tl; t; t = t->wl_next)
                        TTY.printf("%s\n", t->wl_word);
                    wordlist::destroy(tl);
                }
            }
        }
    }
    else if (lstring::cieq(word, "r") || lstring::cieq(word, "remove")) {
        if (!wl)
            TTY.printf("No tags given to remove.\n");
        else {
            for (wordlist *w = wl; w; w = w->wl_next) {
                if (!SPcache.inCache(w->wl_word))
                    TTY.printf("Unknown tag %s.\n", w->wl_word);
                else {
                    SPcache.removeCache(w->wl_word);
                    TTY.printf("Entry %s deleted.\n", w->wl_word);
                }
            }
        }
    }
    else if (lstring::cieq(word, "c") || lstring::cieq(word, "clear")) {
        SPcache.clearCache();
        TTY.printf("Subcircuit/model cache cleared.\n");
    }
    else {
        TTY.printf("Unknown directive.\n");
        pr_usage();
    }
}


// Undocumented interface to the allocation monitor, for core-leak
// hunting.  This does not work in production releases, the monitor
// must be enabled in the memory system.
//
// mmom start [depth]
// mmon stop [filename]
// mmon [status | check]
//
void
CommandTab::com_mmon(wordlist *wl)
{
#ifdef HAVE_LOCAL_ALLOCATOR
    if (wl && wl->wl_word) {
        if (lstring::cieq(wl->wl_word, "start")) {
            int depth = 4;
            if (wl->wl_next)
                depth = atoi(wl->wl_next->wl_word);
            if (depth < 1 || depth > 15) {
                TTY.printf(
                    "Error: depth must be in range 1-15 (default 4).\n");
                return;
            }
            if (!Memory()->mon_start(depth)) {
                TTY.printf(
                    "Error: memory monitor failed to start.\n");
                return;
            }
            TTY.printf(
                "Memory monitor started.\n");
            return;
        }
        if (lstring::cieq(wl->wl_word, "stop")) {
            Sp.VecGc();
            const char *fname = "mon.out";
            if (wl->wl_next)
                fname = wl->wl_next->wl_word;
            if (!Memory()->mon_stop()) {
                TTY.printf(
                    "Memory monitor is inactive.\n");
                return;
            }
            if (!Memory()->mon_dump(fname)) {
                TTY.printf(
                    "Error: data dump to file failed, monitor inactive.\n");
                return;
            }
            TTY.printf(
                "Memory monitor stopped, data in file \"%s\".\n", fname);
            return;
        }
        if (!lstring::cieq(wl->wl_word, "status") &&
                !lstring::cieq(wl->wl_word, "check")) {
            return;
        }
    }
    TTY.printf(
        "Allocation table contains %d entries.\n", Memory()->mon_count());
#else
    (void)wl;
    TTY.printf(
        "Allocation monitor not available.\n");
#endif
}

