
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "editif.h"
#include "scedif.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "cd_memmgr.h"
#include "cd_sdb.h"
#include "cd_netname.h"
#include "cd_propnum.h"
#include "geo_memmgr.h"
#include "dsp_tkif.h"
#include "dsp_color.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "fio_library.h"
#include "fio_compare.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_lisp.h"
#include "pcell.h"
#include "layertab.h"
#include "keymap.h"
#include "menu.h"
#include "ghost.h"
#include "file_menu.h"
#include "promptline.h"
#include "select.h"
#include "tech.h"
#include "tech_cds_in.h"
#include "tech_cds_out.h"
#include "errorlog.h"
#include "cvrt.h"
#include "ginterf/grfont.h"
#include "ginterf/xdraw.h"
#include "miscutil/timer.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/timedbg.h"
#include "miscutil/miscutil.h"
#include "miscutil/childproc.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#include "upd/update_itf.h"
#endif

#ifdef WIN32
#include "miscutil/msw.h"
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dlfcn.h>
#endif

#include <errno.h>

#ifdef HAVE_REGEX_H
#include <regex.h>
#else
#include "libregex/regex.h"
#endif

#include <signal.h>
#include <time.h>


//=========================================================================
//
// Main '!' Command Interface
//
//=========================================================================

#ifndef M_PI
#define M_PI  3.14159265358979323846  // pi
#endif

// Help database keywords used in this file
#define HELP_VAR_LIST "!set:variables"


namespace main_txtcmds {
    // Hash table element for "bang" command.
    //
    struct bcel_t
    {
        bcel_t(const char *n, BangCmdFunc *f)
            {
                cmdname = lstring::copy(n);
                func = f;
                next = 0;
            }

        ~bcel_t() { delete [] cmdname; }

        const char *tab_name()          { return (cmdname); }
        bcel_t *tab_next()              { return (next); }
        bcel_t *tgen_next(bool)         { return (next); }
        void set_tab_next(bcel_t *n)    { next = n; }
        BangCmdFunc *tab_func()         { return (func); }
        void set_tab_func(BangCmdFunc *f) { func = f; }

    private:
        char *cmdname;
        BangCmdFunc *func;
        bcel_t *next;
    };
}

using namespace main_txtcmds;

// Register a "bang" command.
//
void
cMain::RegisterBangCmd(const char *name, BangCmdFunc *f)
{
    if (!xm_bang_cmd_tab)
        xm_bang_cmd_tab = new table_t<bcel_t>;
    bcel_t *el = xm_bang_cmd_tab->find(name);
    if (el) {
        el->set_tab_func(f);
        return;
    }
    el = new bcel_t(name, f);
    xm_bang_cmd_tab->link(el);
    xm_bang_cmd_tab = xm_bang_cmd_tab->check_rehash();
}


namespace {
    enum LStype { LSbang, LSscript, LSvars };
    namespace bangcmds {
        // dummy
        void pop_up_list(LStype);
        void bangbangcmd(const char*);

        // Compression
        void gzip(const char*);
        void gunzip(const char*);
        void md5(const char*);

        // Create Output
        void sa(const char*);
        void sqdump(const char*);
        void assemble(const char*);
        void splwrite(const char*);

        // Current Directory
        void cd(const char*);
        void pwd(const char*);

        // Diagnostics
        void time(const char*);
        void timedbg(const char*);
        void xdepth(const char*);
        void bincnt(const char*);
        void netxp(const char*);
        void pcdump(const char*);
        // --- below are undocumented
        void debug(const char*);
        void checkrefs(const char*);
        void segfault(const char*);
        void memerr(const char*);
        void checkalloc(const char*);
        void ipt(const char*);
        void rdsp(const char*);
        void reconnect(const char*);

        // DRC (drc_txtcmds.cc)
        // showz
        // errs
        // errlayer

        // Electrical (sced_txtcmds.cc)
        // check
        // regen
        // devkeys
        // sced2xic

        // Extract (ext_txtcmds.cc)
        // antenna
        // netext
        // source
        // exset
        // addcells
        // find
        // ptrms
        // ushow
        // fc
        // fh
        // updlabel
        // updmeas

        // Graphics
        void setcolor(const char*);
        void display(const char*);

        // Grid
        void sg(const char*);
        void rg(const char*);

        // Help
        void help(const char*);
        void helpfont(const char*);
        void helpfixed(const char*);
        void helpreset(const char*);

        // Keyboard
        void kmap(const char*);

        // Layers
        void ltab(const char*);
        void ltsort(const char*);
        void exlayers(const char*);

        // Layout Editing (edit_txtcmds.cc)
        // array
        // layer
        // mo
        // co
        // spin
        // rename
        // svq
        // rcq
        // box2poly
        // path2poly
        // poly2path
        // bloat
        // join
        // split
        // manh
        // polyfix
        // polyrev
        // noacute
        // togrid
        // tospot
        // origin
        // import

        // Layout Information
        void fileinfo(const char*);
        void summary(const char*);
        void compare(const char*);
        void diffcells(const char*);
        void empties(const char*);
        void area(const char*);
        void perim(const char*);
        void bb(const char*);
        void checkgrid(const char*);
        void checkover(const char*);
        void check45(const char*);
        void dups(const char*);
        void wirecheck(const char*);
        void polycheck(const char*);
        void polymanh(const char*);
        void poly45(const char*);
        void polynum(const char*);
        void setflag(const char*);

        // Libraries and Databases
        void mklib(const char*);
        void lsdb(const char*);

        // Marks
        void mark(const char*);

        // Memory Management
        void clearall(const char*);
        void mmstats(const char*);
        void mmclear(const char*);
        //--- undocumented
        void attrhash(const char*);
        void attrclear(const char*);
        void stclear(const char*);
        void gentest(const char*);
        // memory.cc
        //   vmem       (WIN32 only)
        //   memfault
        //   oom
        //   oom1       (HAVE_LOCAL_ALLOCATOR)
        //   monstart   (HAVE_LOCAL_ALLOCATOR)
        //   monstop    (HAVE_LOCAL_ALLOCATOR)
        //   monstatus  (HAVE_LOCAL_ALLOCATOR)

        // PCells
        void rmpcprops(const char*);

        // Rulers
        void dr(const char*);

        // Scripts
        void script(const char*);
        void rehash(const char*);
        void exec(const char*);
        void lisp(const char*);
        // py
        // tk
        void listfuncs(const char*);
        void rmfunc(const char*);
        void mkscript(const char*);
        void ldshared(const char*);

        // Selections
        void select(const char*);
        void desel(const char*);
        void zs(const char*);

        // Shell
        void shell(const char*);
        void ssh(const char*);

        // Technology File
        void attrvars(const char*);
        void dumpcds(const char*);

        // Update Release
        void update(const char*);
        void passwd(const char*);
        void proxy(const char*);

        // Variables
        void set(const char*);
        void unset(const char*);
        void setdump(const char*);

        // WRspice Interface (sced_txtcmds.cc)
        // spcmd
    }

    const char *undoc[] = {
        // Diagnostics
        "debug",
        "checkrefs",
        "segfault",
        "memerr",
        "checkalloc",
        "ipt",
        "rdsp",
        "reconnect",

        // Memory Management
        "attrhash",
        "attrclear",
        "stclear",
        "gentest",
        "vmem",
        "memfault",
        "oom",
        "oom1",
        "monstart",
        "monstop",
        "monstatus",

        // Extract
        "updlabels",
        "updmeas",

        // hackery
        "testfunc",
        0 };
}


// Return a sorted list of registered '!' commands.  Undocumented and
// dummy commands are not included.
//
stringlist *
cMain::ListBangCmds()
{
    tgen_t<bcel_t> tgen(xm_bang_cmd_tab);
    bcel_t *el;
    stringlist *s0 = 0;
    while ((el = tgen.next()) != 0) {
        if (!isalpha(*el->tab_name()))
            continue;
        bool skip = false;
        for (const char **s = undoc; *s; s++) {
            if (!strcmp(*s, el->tab_name())) {
                skip = true;
                break;
            }
        }
        if (!skip)
            s0 = new stringlist(lstring::copy(el->tab_name()), s0);
    }
    stringlist::sort(s0);
    return (s0);
}


// Register the main "bang" commands, called from the constructor.
//
void
cMain::setupBangCmds()
{
    // !! dummy
    RegisterBangCmd(BANG_BANG_NAME, &bangcmds::bangbangcmd);

    // Compression
    RegisterBangCmd("gzip", &bangcmds::gzip);
    RegisterBangCmd("gunzip", &bangcmds::gunzip);
    RegisterBangCmd("md5", &bangcmds::md5);

    // Create Output
    RegisterBangCmd("sa", &bangcmds::sa);
    RegisterBangCmd("sqdump", &bangcmds::sqdump);
    RegisterBangCmd("assemble", &bangcmds::assemble);
    RegisterBangCmd("splwrite", &bangcmds::splwrite);

    // Current Directory
    RegisterBangCmd("cd", &bangcmds::cd);
    RegisterBangCmd("pwd", &bangcmds::pwd);

    // Diagnostics
    RegisterBangCmd("time", &bangcmds::time);
    RegisterBangCmd("timedbg", &bangcmds::timedbg);
    RegisterBangCmd("xdepth", &bangcmds::xdepth);
    RegisterBangCmd("bincnt", &bangcmds::bincnt);
    RegisterBangCmd("netxp", &bangcmds::netxp);
    RegisterBangCmd("pcdump", &bangcmds::pcdump);
    RegisterBangCmd("debug", &bangcmds::debug);
    RegisterBangCmd("checkrefs", &bangcmds::checkrefs);
    RegisterBangCmd("segfault", &bangcmds::segfault);
    RegisterBangCmd("memerr", &bangcmds::memerr);
    RegisterBangCmd("checkalloc", &bangcmds::checkalloc);
    RegisterBangCmd("ipt", &bangcmds::ipt);
    RegisterBangCmd("rdsp", &bangcmds::rdsp);
    RegisterBangCmd("reconnect", &bangcmds::reconnect);

    // Graphics
    RegisterBangCmd("setcolor", &bangcmds::setcolor);
    RegisterBangCmd("display", &bangcmds::display);

    // Grid
    RegisterBangCmd("sg", &bangcmds::sg);
    RegisterBangCmd("rg", &bangcmds::rg);

    // Help
    RegisterBangCmd("help", &bangcmds::help);
    RegisterBangCmd("helpfont", &bangcmds::helpfont);
    RegisterBangCmd("helpfixed", &bangcmds::helpfixed);
    RegisterBangCmd("helpreset", &bangcmds::helpreset);

    // Keyboard
    RegisterBangCmd("kmap", &bangcmds::kmap);

    // Layers
    RegisterBangCmd("ltab", &bangcmds::ltab);
    RegisterBangCmd("ltsort", &bangcmds::ltsort);
    RegisterBangCmd("exlayers", &bangcmds::exlayers);

    // Layout Information
    RegisterBangCmd("fileinfo", &bangcmds::fileinfo);
    RegisterBangCmd("summary", &bangcmds::summary);
    RegisterBangCmd("compare", &bangcmds::compare);
    RegisterBangCmd("diffcells", &bangcmds::diffcells);
    RegisterBangCmd("empties", &bangcmds::empties);
    RegisterBangCmd("area", &bangcmds::area);
    RegisterBangCmd("perim", &bangcmds::perim);
    RegisterBangCmd("bb", &bangcmds::bb);
    RegisterBangCmd("checkgrid", &bangcmds::checkgrid);
    RegisterBangCmd("checkover", &bangcmds::checkover);
    RegisterBangCmd("check45", &bangcmds::check45);
    RegisterBangCmd("dups", &bangcmds::dups);
    RegisterBangCmd("wirecheck", &bangcmds::wirecheck);
    RegisterBangCmd("polycheck", &bangcmds::polycheck);
    RegisterBangCmd("polymanh", &bangcmds::polymanh);
    RegisterBangCmd("poly45", &bangcmds::poly45);
    RegisterBangCmd("polynum", &bangcmds::polynum);
    RegisterBangCmd("setflag", &bangcmds::setflag);

    // Libraries and Databases
    RegisterBangCmd("mklib", &bangcmds::mklib);
    RegisterBangCmd("lsdb", &bangcmds::lsdb);

    // Marks
    RegisterBangCmd("mark", &bangcmds::mark);

    // Memory Management
    RegisterBangCmd("clearall", &bangcmds::clearall);
    RegisterBangCmd("mmstats", &bangcmds::mmstats);
    RegisterBangCmd("mmclear", &bangcmds::mmclear);
    RegisterBangCmd("attrhash", &bangcmds::attrhash);
    RegisterBangCmd("attrclear", &bangcmds::attrclear);
    RegisterBangCmd("stclear", &bangcmds::stclear);
    RegisterBangCmd("gentest", &bangcmds::gentest);
    setupMemoryBangCmds();

    // PCells
    RegisterBangCmd("rmpcprops", &bangcmds::rmpcprops);

    // Rulers
    RegisterBangCmd("dr", &bangcmds::dr);

    // Scripts
    RegisterBangCmd("script", &bangcmds::script);
    RegisterBangCmd("rehash", &bangcmds::rehash);
    RegisterBangCmd("exec", &bangcmds::exec);
    RegisterBangCmd("lisp", &bangcmds::lisp);
    RegisterBangCmd("listfuncs", &bangcmds::listfuncs);
    RegisterBangCmd("rmfunc", &bangcmds::rmfunc);
    RegisterBangCmd("mkscript", &bangcmds::mkscript);
    RegisterBangCmd("ldshared", &bangcmds::ldshared);

    // Selections
    RegisterBangCmd("select", &bangcmds::select);
    RegisterBangCmd("desel", &bangcmds::desel);
    RegisterBangCmd("zs", &bangcmds::zs);

    // Shell
    RegisterBangCmd("shell", &bangcmds::shell);
    RegisterBangCmd("ssh", &bangcmds::ssh);

    // Technology File
    RegisterBangCmd("attrvars", &bangcmds::attrvars);
    RegisterBangCmd("dumpcds", &bangcmds::dumpcds);

    // Update Release
    RegisterBangCmd("update", &bangcmds::update);
    RegisterBangCmd("passwd", &bangcmds::passwd);
    RegisterBangCmd("proxy", &bangcmds::proxy);

    // Variables
    RegisterBangCmd("set", &bangcmds::set);
    RegisterBangCmd("unset", &bangcmds::unset);
    RegisterBangCmd("setdump", &bangcmds::setdump);
}


// Save the last few command lines
//
#define HISTORY_LEN 6

namespace {
#ifdef WIN32
    char *find_cygwin_path(const char *prog)
    {
        const char *p = getenv("CYGWIN_BIN");
        if (p) {
            char *fp = pathlist::mk_path(p, prog);
            lstring::dos_path(fp);
            if (!access(fp, X_OK))
                return (fp);
            delete [] fp;
            return (0);
        }
        char *fp = new char[strlen(prog) + 20];
        sprintf(fp, "\\cygwin\\bin\\%s", prog);
        if (!access(fp, X_OK))
            return (fp);
        sprintf(fp, "\\bin\\%s", prog);
        if (!access(fp, X_OK))
            return (fp);
        delete [] fp;
        return (0);
    }
#endif


    // Return the path to use for a shell.
    //
    char *
    get_shell()
    {
#ifdef WIN32
        const char *shellpath = CDvdb()->getVariable(VA_Shell);
        if (!shellpath) {
            shellpath = getenv("SHELL");
            // Be careful with this.  If set in a Cygwin shell, it
            // represents a Cygwin path, not a Windows path.  If we
            // find a forward slash, ignore this.
            if (shellpath && strchr(shellpath, '/'))
                shellpath = 0;
        }
        if (!shellpath) {
            char *s = find_cygwin_path("bash.exe");
            if (s) {
                char *e = strrchr(s, '.');
                if (e && lstring::cieq(e, ".exe"))
                    *e = 0;
                e = lstring::copy(s);
                delete [] s;
                return (e);
            }
        }
        if (!shellpath)
            shellpath = getenv("COMSPEC");
        if (!shellpath)
            shellpath = "c:\\Windows\\system32\\cmd.exe";
        char *path = lstring::copy(shellpath);
        lstring::dos_path(path);
        return (path);
#else
        const char *shellpath = CDvdb()->getVariable(VA_Shell);
        if (!shellpath)
            shellpath = getenv("SHELL");
        if (!shellpath)
            shellpath = "/bin/sh";
        return (lstring::copy(shellpath));
#endif
    }


    struct History
    {
        void push(const char *str);

        static void down_cb();
        static void up_cb();

        char *h_array[HISTORY_LEN + 1];
        int index;
    };
    History history;

    void
    History::push(const char *str)
    {
        if (!str || (h_array[0] && !strcmp(str, h_array[0])))
            return;
        delete [] h_array[HISTORY_LEN];
        for (int i = HISTORY_LEN; i > 0; i--)
            h_array[i] = h_array[i-1];
        h_array[0] = lstring::copy(str);
    }

    void
    History::down_cb()
    {
        int n = history.index - 1;
        if (n >= 0) {
            history.index = n;
            const char *ctmp = history.h_array[n];
            PL()->EditPrompt("! ", ctmp, PLedUpdate);
        }
    }

    void
    History::up_cb()
    {
        int n = history.index + 1;
        if (n < HISTORY_LEN && history.h_array[n]) {
            history.index = n;
            const char *ctmp = history.h_array[n];
            PL()->EditPrompt("! ", ctmp, PLedUpdate);
        }
    }
}




// Execute the commands that are keyed with '!'.  Always returns true.
//
bool
cMain::TextCmd(const char *ss, bool helpmode)
{
    EV()->MotionClear();

    // Copy the passed string, remove leading and trailing white space.
    char *cmd = lstring::copy(ss);
    GCarray<char*> gc_cmd(cmd);

    if (cmd) {
        while (isspace(*cmd))
            cmd++;
        char *etmp = cmd + strlen(cmd) - 1;
        while (etmp >= cmd && isspace(*etmp))
            *etmp-- = 0;
        if (!*cmd)
            cmd = 0;
    }
    if (!cmd) {
        PL()->ErasePrompt();
        if (helpmode) {
            // Look for a help keyword in the current command state. 
            // If we don't have one, or it is "idle", use the
            // top-level topic.

            CmdState *cs = EV()->CurCmd();
            const char *kw = cs ? cs->HelpKword() : 0;
            if (!kw || !strcasecmp(kw, "idle"))
                kw = "xicinfo";
            DSPmainWbag(PopUpHelp(kw))
        }
        else
            bangcmds::shell("");
        return (true);
    }

    if (helpmode) {
        char *tok = lstring::gettok(&cmd);
        if (tok) {
            if (!tok[1]) {
                switch (tok[0]) {
                case '!':
                case 'b':
                case 'B':
                    bangcmds::pop_up_list(LSbang);
                    break;
                case 'v':
                case 'V':
                    bangcmds::pop_up_list(LSvars);
                    break;
                case 's':
                case 'S':
                    XM()->PopUpVariables(true);
                    break;
                case 'f':
                case 'F':
                    bangcmds::pop_up_list(LSscript);
                    break;
                default:
                    PL()->ShowPrompt("Unknown request.");
                    break;
                }
                delete [] tok;
                return (true);
            }
            else
                DSPmainWbag(PopUpHelp(tok))
            delete [] tok;
        }
        return (true);
    }

    // History request?
    if (*cmd == '#') {
        // User wants history.  syntax:  !#[n]
        int n = 0;
        if (isdigit(cmd[1]))
            n = cmd[1] - '0';
        if (n < HISTORY_LEN && history.h_array[n]) {
            const char *ctmp = history.h_array[n];
            // Let user confirm/edit.  The up/down arrow keys cycle
            // through history list.
            history.index = n;

            PL()->RegisterArrowKeyCallbacks(History::down_cb, History::up_cb);
            char *in = PL()->EditPrompt("! ", ctmp);
            PL()->RegisterArrowKeyCallbacks(0, 0);

            if (!in) {
                PL()->ErasePrompt();
                return (true);
            }
            while (isspace(*in))
                in++;
            gc_cmd.flush();
            cmd = lstring::copy(in);
            gc_cmd.set(cmd);
            char *itmp = cmd + strlen(cmd) - 1;
            while (itmp >= cmd && isspace(*itmp))
                *itmp-- = 0;
        }
        else {
            PL()->ErasePrompt();
            return (true);
        }
    }

    // Help request?
    if (*cmd == '?') {
        PL()->ErasePrompt();
        cmd++;
        if (*cmd == '?') {
            // "!??" pop up a list of registered '!' commands.
            bangcmds::pop_up_list(LSbang);
            return (true);
        }

        char *tok = lstring::gettok(&cmd);
        if (tok) {
            // "!?xxx" pop up help for xxx
            DSPmainWbag(PopUpHelp(tok))
            delete [] tok;
        }
        else
            DSPmainWbag(PopUpHelp("keybang"))
        return (true);
    }

    history.push(cmd);

    // Exec as script line?
    if (*cmd == '!') {
        cmd++;
        while (isspace(*cmd))
            cmd++;
        bcel_t *el = xm_bang_cmd_tab->find(BANG_BANG_NAME);
        if (el) {
            if (el->tab_func())
                (*el->tab_func())(cmd);
        }
        return (true);
    }

    // Look up the command by name, exec and return if found.
    if (xm_bang_cmd_tab) {
        char *ctmp = cmd;
        char *tok = lstring::gettok(&ctmp);
        if (tok) {
            bcel_t *el = xm_bang_cmd_tab->find(tok);
            delete [] tok;
            if (el) {
                if (el->tab_func())
                    (*el->tab_func())(ctmp);
                return (true);
            }
        }
    }

    // Not a known function, assume that it is an operating system
    // command.
    bangcmds::shell(cmd);

    return (true);
}


// Static function.
// Export this.  Pop up a window running cmd.  If cmd is null or
// empty, run the shell.
//
void
cMain::ShellWindowCmd(const char *cmd)
{
    bangcmds::shell(cmd);
}


//-----------------------------------------------------------------------------
// dummy
//

void
bangcmds::pop_up_list(LStype type)
{
    char buf[256];
    if (type == LSbang) {
        // Pop up a list of registered '!' commands.
        stringlist *s0 = XM()->ListBangCmds();
        sLstr lstr;
        lstr.add(
            "<h2>Available '!' commands</h2>\n<p>Below is a list of "
            "the <a href=\"keybang\">'!' commands</a> that have been "
            "registered in this program.\n\n<p>\n");

        if (!s0)
            lstr.add("No commands found.\n");
        else {
            const int cols = 4;
            int len = stringlist::length(s0);
            int nc = len/cols + (len%cols != 0);
            stringlist *s = s0;
            lstr.add("<table border=0 cellspacing=4><tr>\n");
            for (int i = 0; i < cols; i++) {
                lstr.add("<td valign=top>");
                for (int j = 0; j < nc; j++) {
                    if (s) {
                        sprintf(buf, "<a href=\"!%s\"><b>!%s</b></a><br>\n",
                            s->string, s->string);
                        s = s->next;
                        lstr.add(buf);
                    }
                }
                lstr.add("</td>\n");
            }
            lstr.add("</tr></table>\n");
        }
        stringlist::destroy(s0);
        DSPmainWbagRet(PopUpHTMLinfo(MODE_ON, lstr.string()));
    }
    else if (type == LSscript) {
        // Pop up a list of registered script functions.
        stringlist *s0 = SIparse()->funcList();
        sLstr lstr;
        lstr.add(
            "<h2>Available script functions</h2>\n<p>Below is a list of "
            "the <a href=\"scr:iffuncs\">script functions</a> that have been "
            "registered in this program.\n\n<p>\n");

        if (!s0)
            lstr.add("No functions found.\n");
        else {
            const int cols = 3;
            int len = stringlist::length(s0);
            int nc = len/cols + (len%cols != 0);
            stringlist *s = s0;
            lstr.add("<table border=0 cellspacing=4><tr>\n");
            for (int i = 0; i < cols; i++) {
                lstr.add("<td valign=top>");
                for (int j = 0; j < nc; j++) {
                    if (s) {
                        sprintf(buf, "<a href=\"%s\"><b>%s</b></a><br>\n",
                            s->string, s->string);
                        s = s->next;
                        lstr.add(buf);
                    }
                }
                lstr.add("</td>\n");
            }
            lstr.add("</tr></table>\n");
        }
        stringlist::destroy(s0);
        s0 = SIparse()->altFuncList();
        if (s0) {
            lstr.add(
                "<br clear=all><p>These functions apply in layer "
                "expressions.\n\n<p>\n");
            const int cols = 3;
            int len = stringlist::length(s0);
            int nc = len/cols + (len%cols != 0);
            stringlist *s = s0;
            lstr.add("<table border=0 cellspacing=4><tr>\n");
            for (int i = 0; i < cols; i++) {
                lstr.add("<td valign=top>");
                for (int j = 0; j < nc; j++) {
                    if (s) {
                        sprintf(buf, "<a href=\"%s\"><b>%s</b></a><br>\n",
                            s->string, s->string);
                        s = s->next;
                        lstr.add(buf);
                    }
                }
                lstr.add("</td>\n");
            }
            lstr.add("</tr></table>\n");
        }
        DSPmainWbagRet(PopUpHTMLinfo(MODE_ON, lstr.string()));
    }
    else if (type == LSvars) {
        // Pop up a list of registered variable names.
        stringlist *s0 = CDvdb()->listInternal();
        sLstr lstr;
        lstr.add(
            "<h2>Internally recognized variable names</h2>\n<p>Below is a "
            "list of the internal <a href=\"!set:variables\">variables</a> "
            "that have been registered in this program.\n\n<p>\n");

        if (!s0)
            lstr.add("No variables found.\n");
        else {
            const int cols = 3;
            int len = stringlist::length(s0);
            int nc = len/cols + (len%cols != 0);
            stringlist *s = s0;
            lstr.add("<table border=0 cellspacing=4><tr>\n");
            for (int i = 0; i < cols; i++) {
                lstr.add("<td valign=top>");
                for (int j = 0; j < nc; j++) {
                    if (s) {
                        sprintf(buf, "<a href=\"%s\"><b>%s</b></a><br>\n",
                            s->string, s->string);
                        s = s->next;
                        lstr.add(buf);
                    }
                }
                lstr.add("</td>\n");
            }
            lstr.add("</tr></table>\n");
        }
        stringlist::destroy(s0);
        DSPmainWbagRet(PopUpHTMLinfo(MODE_ON, lstr.string()));
    }
}


// Dummy function, for the !! script text execution festure.
//
void
bangcmds::bangbangcmd(const char *s)
{
    // Process the passed text as a script line.
    if (s && *s) {
        PL()->ErasePrompt();
        // execute line as if script
        char *tbuf = lstring::copy(s); // scInterpret takes non-const
        const char *t = tbuf;
        SI()->Interpret(0, 0, &t, 0);
        delete [] tbuf;
    }
}


//-----------------------------------------------------------------------------
// Compression
//

void
bangcmds::gzip(const char *s)
{
    char *infile = lstring::getqtok(&s);
    char *outfile = lstring::getqtok(&s);
    if (!infile) {
        PL()->ShowPrompt("Usage:  !gzip infile [outfile]");
        return;
    }
    char *t = strrchr(infile, '.');
    if (t && lstring::cieq(t, ".gz")) {
        PL()->ShowPrompt("Error: input file has .gz suffix.");
        delete [] infile;
        delete [] outfile;
        return;
    }
    if (!outfile) {
        outfile = new char[strlen(infile) + 4];
        strcpy(outfile, infile);
        strcat(outfile, ".gz");
    }
    else {
        t = strrchr(outfile, '.');
        if (!t || !lstring::cieq(t, ".gz")) {
            PL()->ShowPrompt("Error: output file missing .gz suffix.");
            delete [] infile;
            delete [] outfile;
            return;
        }
    }
    PL()->ShowPrompt("Working...");
    char *err = gzip(infile, outfile, GZIPcompress);
    if (err) {
        PL()->ShowPromptV("Failed: %s", err);
        delete [] err;
    }
    else
        PL()->ShowPrompt("Done, succeeded.");
    delete [] infile;
    delete [] outfile;
}


void
bangcmds::gunzip(const char *s)
{
    char *infile = lstring::getqtok(&s);
    char *outfile = lstring::getqtok(&s);
    if (!infile) {
        PL()->ShowPrompt("Usage:  !gunzip infile [outfile]");
        return;
    }
    char *t = strrchr(infile, '.');
    if (!t || !lstring::cieq(t, ".gz")) {
        PL()->ShowPrompt("Error: input file missing .gz suffix.");
        delete [] infile;
        delete [] outfile;
        return;
    }
    if (!outfile) {
        outfile = lstring::copy(infile);
        t = strrchr(outfile, '.');
        *t = 0;
    }
    else {
        t = strrchr(outfile, '.');
        if (t && lstring::cieq(t, ".gz")) {
            PL()->ShowPrompt("Error: output file has .gz suffix.");
            delete [] infile;
            delete [] outfile;
            return;
        }
    }
    PL()->ShowPrompt("Working...");
    char *err = gzip(infile, outfile, GZIPuncompress);
    if (err) {
        PL()->ShowPromptV("Failed: %s", err);
        delete [] err;
    }
    else
        PL()->ShowPrompt("Done, succeeded.");
    delete [] infile;
    delete [] outfile;
}


void
bangcmds::md5(const char *s)
{
    char *path = lstring::getqtok(&s);
    if (!path) {
        PL()->ShowPrompt("Usage:  !md5 file");
        return;
    }
    char *digest = PC()->md5Digest(path);
    if (!digest) {
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());
        PL()->ShowPrompt("Command failed.");
        delete [] path;
        return;
    }
    PL()->ShowPrompt(digest);
    delete [] digest;
}


//-----------------------------------------------------------------------------
// Create Output
//

void
bangcmds::sa(const char*)
{
    PL()->ErasePrompt();
    XM()->Save();
}


void
bangcmds::sqdump(const char *s)
{
    char *fn = lstring::getqtok(&s);
    if (!fn)
        PL()->ShowPrompt("Usage:  !sqdump cellpath");
    else {
        CDol *ol0 = 0, *oe = 0;
        sSelGen sg(Selections, CurCell());
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (!ol0)
                ol0 = oe = new CDol(od, 0);
            else {
                oe->next = new CDol(od, 0);
                oe = oe->next;
            }
        }
        if (ol0) {
            if (FIO()->ListToNative(ol0, fn))
                PL()->ShowPromptV("Selections saved in %s.", fn);
            else
                PL()->ShowPromptV("Save FAILED: %s", Errs()->get_error());
            CDol::destroy(ol0);
        }
        else
            PL()->ShowPrompt("No selections, nothing to save.");
        delete [] fn;
    }
}


void
bangcmds::assemble(const char *s)
{
    while (isspace(*s))
       s++;
    if (!*s) {
        PL()->ShowPrompt("Usage:  !assemble configfile | argument_list");
        return;
    }
    dspPkgIf()->SetWorking(true);
    Errs()->init_error();
    PL()->ShowPrompt("Working...");
    bool ret = FIO()->AssembleArchive(s);
    if (ret)
        PL()->ShowPrompt("Done, success.");
    else {
        PL()->ShowPrompt("Done, FAILED.");
        if (Errs()->has_error())
            Log()->ErrorLog("!assemble", Errs()->get_error());
    }
    dspPkgIf()->SetWorking(false);
}


void
bangcmds::splwrite(const char *s)
{

    while (isspace(*s))
       s++;
    if (!*s) {
        PL()->ShowPrompt("Usage:  splwrite -i fmame -o bname.ext [-c cname] "
            "-g grid | -r l,b,r,t[,l,b,r,t]... [-b bloat] [-w l,b,r,t] [-f] "
            "[-cl] [-e]");
        return;
    }
    dspPkgIf()->SetWorking(true);
    Errs()->init_error();
    PL()->ShowPrompt("Working...");
    bool ret = FIO()->SplitArchive(s);
    if (ret)
        PL()->ShowPrompt("Done, success.");
    else {
        PL()->ShowPrompt("Done, FAILED.");
        if (Errs()->has_error())
            Log()->ErrorLog("!splwrite", Errs()->get_error());
    }
    dspPkgIf()->SetWorking(false);
}


//-----------------------------------------------------------------------------
// Current Directory
//

void
bangcmds::cd(const char *s)
{
    if (!s || !*s)
        s = "~";
    s = pathlist::expand_path(s, true, true);

    if (!chdir(s))
        PL()->ShowPromptV("Working directory: %s", s);
    else
        PL()->ShowPromptV("Directory change failed: %s.", strerror(errno));
    delete [] s;
}


void
bangcmds::pwd(const char*)
{
    char buf[256];
    if (FIO()->PGetCWD(buf, 256))
        PL()->ShowPrompt(buf);
    else
        PL()->ShowPrompt("Current directory unknown!");
}


//-----------------------------------------------------------------------------
// Diagnostics
//

void
bangcmds::time(const char*)
{
    printf("%.3f\n", Timer()->elapsed_msec()/1000.0);
}


void
bangcmds::timedbg(const char *s)
{
    const char *msg = "Execution time diagnostic is %s.";
    const char *active = "active";
    const char *na = "not active";
    char *tok = lstring::gettok(&s);
    if (!tok) {
        PL()->ShowPromptV(msg, Tdbg()->is_active() ? active : na);
        return;
    }

    char *t = tok;
    if (*t == '-')
        t++;
    if (lstring::ciprefix("n", t) || lstring::ciprefix("of", t)) {
        Tdbg()->set_active(false);
        PL()->ShowPromptV(msg, Tdbg()->is_active() ? active : na);
        delete [] tok;
        return;
    }
    if (lstring::ciprefix("y", t) || lstring::ciprefix("on", t)) {
        delete [] tok;
        tok = lstring::getqtok(&s);
        int lev = -1;
        if (tok && tok[0] == '-') {
            if (isdigit(tok[1])) {
                lev = atoi(tok + 1);
                delete [] tok;
                tok = lstring::getqtok(&s);
            }
            else {
                lev = -2;
                delete [] tok;
            }
        }
        if (lev != -2) {
            Tdbg()->set_active(true);
            Tdbg()->set_max_level(lev);
            Tdbg()->set_logfile(tok);
            delete [] tok;
            PL()->ShowPromptV(msg, Tdbg()->is_active() ? active : na);
            return;
        }
    }
    PL()->ShowPrompt(
        "!timedbg: syntax error, usage: !timedbg [y|n [-maxlev] [filename]]");
}


void
bangcmds::xdepth(const char*)
{
    printf("%d %d\n", DSP()->TDepth(), DSP()->THighWater());
}


void
bangcmds::bincnt(const char *s)
{
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("No current cell!");
        return;
    }

    char *lname = lstring::gettok(&s);
    char *levstr = lstring::gettok(&s);
    int lev = levstr ? atoi(levstr) : -1;
    delete [] levstr;

    CDl *ld;
    if (!lname)
        ld = CellLayer();
    else {
        ld = CDldb()->findLayer(lname, DSP()->CurMode());
        delete [] lname;
    }
    if (!ld)
        PL()->ShowPrompt("Bad layer name.");
    else {
        CDs *cursd = CurCell();
        if (cursd) {
            cursd->bincnt(ld, lev);
            PL()->ErasePrompt();
        }
    }
}


void
bangcmds::netxp(const char *s)
{
    if (!s || !*s) {
        printf("Error: null or empty string\n");
        return;
    }
    printf("Orig: %s\n", s);
    CDnetex *netex;
    if (!CDnetex::parse(s, &netex)) {
        printf("Error, parse failed: %s\n", Errs()->get_error());
        return;
    }
    sLstr lstr;
    CDnetex::print_all(netex, &lstr);
    printf("Back: %s\n", lstr.string());
    CDnetName nm;
    int n;
    CDnetexGen ngen(netex);
    while (ngen.next(&nm, &n)) {
        if (n < 0)
            printf(" %s\n", Tstring(nm));
        else
            printf(" %s<%d>\n", Tstring(nm), n);
    }
    printf("\n");
    CDnetex::destroy(netex);
}


void
bangcmds::pcdump(const char *s)
{
    FILE *fp = 0;
    if (s && *s && strcmp(s, "stdout")) {
        fp = fopen(s, "w");
        if (!fp) {
            PL()->ShowPromptV("Failed to open file %s.", s);
            return;
        }
    }
    PC()->dump(fp);
    if (fp)
        fclose(fp);
}


void
bangcmds::debug(const char *s)
{
    char *tok1 = lstring::gettok(&s);
    if (!tok1) {
        XM()->PopUpDebugFlags(0, MODE_ON);
        PL()->ErasePrompt();
        return;
    }

    unsigned int flgs = 0;
    bool bad = false;
    if (tok1[0] == '0') {
        if (tok1[1] == 'x' || tok1[1] == 'X') {
            if (sscanf(tok1+2, "%x", &flgs) != 1)
                bad = true;
        }
        else if (sscanf(tok1, "%o", &flgs) != 1)
            bad = true;
    }
    else if (sscanf(tok1, "%u", &flgs) != 1)
        bad = true;
    delete [] tok1;
    if (bad) {
        PL()->ShowPrompt("Usage:  !debug [flags [filename]]");
        return;
    }

    XM()->SetDebugFlags(flgs);
    if (!flgs) {
        if (XM()->DebugFp()) {
            fclose(XM()->DebugFp());
            XM()->SetDebugFp(0);
        }
        delete [] XM()->DebugFile();
        XM()->SetDebugFile(0);
        XM()->PopUpDebugFlags(0, MODE_UPD);
        PL()->ErasePrompt();
        return;
    }

    char *tok2 = lstring::getqtok(&s);
    if (tok2) {
        if (strcmp(tok2, "stdout")) {
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(0);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(tok2);
        }
        else if (strcmp(tok2, "stderr")) {
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(0);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(0);
            delete [] tok2;
        }
        else {
            if (XM()->DebugFile() && !strcmp(XM()->DebugFile(), tok2)) {
                XM()->PopUpDebugFlags(0, MODE_UPD);
                PL()->ErasePrompt();
                return;
            }
            FILE *fp = fopen(tok2, "w");
            if (!fp) {
                PL()->ShowPromptV("Error: can't open %s.", tok2);
                delete [] tok2;
                XM()->PopUpDebugFlags(0, MODE_UPD);
                PL()->ErasePrompt();
                return;
            }
            if (XM()->DebugFp()) {
                fclose(XM()->DebugFp());
                XM()->SetDebugFp(fp);
            }
            delete [] XM()->DebugFile();
            XM()->SetDebugFile(tok2);
        }
    }
    XM()->PopUpDebugFlags(0, MODE_UPD);
    PL()->ErasePrompt();
}


void
bangcmds::checkrefs(const char*)
{
    CDcellTab::do_test_ptrs();
}


void
bangcmds::segfault(const char*)
{
    *(char*)-1 = 0;
}


void
bangcmds::memerr(const char*)
{
    char *t = (char*)malloc(100);
    free(t);
    free(t);
}


void
bangcmds::checkalloc(const char*)
{
    CD()->CheckAlloc();
}


void
bangcmds::ipt(const char*)
{
    bool b = ScedIf()->doingIplot();
    char *c = ScedIf()->getIplotCmd(false);
    if (c) {
        printf("iplot %s: %s\n", b ? "on" : "off", c);
        delete [] c;
    }
    else
        printf("iplot %s: %s\n", b ? "on" : "off", "(null)");
}


// Redisplay diagnostic, check that area redisplay doesn't leave
// artifacts.
//
void
bangcmds::rdsp(const char *s)
{
    BBox BB;
    if (sscanf(s, "%d %d %d %d",
            &BB.left, &BB.bottom, &BB.right, &BB.top) != 4) {
        PL()->ShowPrompt("Usage: rdsp l b r t");
        return;
    }
    BB.fix();
    DSP()->MainWdesc()->Redisplay(&BB);
}


void
bangcmds::reconnect(const char*)
{
    CDs *sd = CurCell(Electrical);
    if (sd) {
        sd->unsetConnected();
        ScedIf()->connectAll(false);
    }
}


//-----------------------------------------------------------------------------
// Graphics
//

void
bangcmds::setcolor(const char *s)
{
    const char *msg = "Usage:  !setcolor colorname value";
    char *cname = lstring::gettok(&s);
    if (!cname) {
        PL()->ShowPrompt(msg);
        return;
    }

    char *cval = lstring::copy(s);
    char *e = cval + strlen(cval);
    while (isspace(*e) && e >= cval)
        *e-- = 0;
    if (!*cval) {
        delete [] cname;
        PL()->ShowPrompt(msg);
        return;
    }

    if (!DSP()->ColorTab()->is_colorname(cname))
        PL()->ShowPromptV("Unknown color name %s.", cname);
    else if (!DSP()->ColorTab()->set_color(cname, cval))
        PL()->ShowPromptV("Operation failed, bad color value %s.", cval);
    else
        PL()->ErasePrompt();

    delete [] cname;
    delete [] cval;
}


void
bangcmds::display(const char *s)
{
#ifdef WIN32
    (void)s;
    PL()->ShowPrompt("Command not available under Windows.");
#else
    unsigned long w = 0;
    char *t1 = lstring::gettok(&s);
    char *t2 = lstring::gettok(&s);
    if (t1 && t2) {
        if (*t2 == '0' && (*(t2+1) == 'x' || *(t2+1) == 'X'))
            sscanf(t2+2, "%lx", &w);
        else if (*t2 == '0')
            sscanf(t2+2, "%lo", &w);
        else
            sscanf(t2, "%ld", &w);
        if (w != 0) {
            Xdraw *xd = new Xdraw(t1, w);
            if (!xd->check_error())
                xd->draw(DSP()->MainWdesc()->Window()->left,
                    DSP()->MainWdesc()->Window()->bottom,
                    DSP()->MainWdesc()->Window()->right,
                    DSP()->MainWdesc()->Window()->top);
            delete xd;
            PL()->ErasePrompt();
        }
        else
            PL()->ShowPrompt("Bad win_id given.");
    }
    else
        PL()->ShowPrompt("Usage:  !display display_string win_id");
    delete [] t1;
    delete [] t2;
#endif
}


//-----------------------------------------------------------------------------
// Grid
//

void
bangcmds::sg(const char *s)
{
    int indx = 0;
    if (*s) {
        if (isdigit(*s) && (indx = atoi(s)) >= 0 && indx < TECH_NUM_GRIDS) ;
        else {
            char buf[256];
            sprintf(buf, "Bad grid register, 0-%d are valid.",
                TECH_NUM_GRIDS-1);
            PL()->ShowPrompt(buf);
            return;
        }
    }
    WindowDesc *wdesc =
        EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
    Tech()->SetGridReg(indx, *wdesc->Attrib()->grid(wdesc->Mode()),
        wdesc->Mode());
    PL()->ErasePrompt();
}


void
bangcmds::rg(const char *s)
{
    int indx = 0;
    if (*s) {
        if (isdigit(*s) && (indx = atoi(s)) >= 0 && indx < TECH_NUM_GRIDS) ;
        else {
            char buf[256];
            sprintf(buf, "Bad grid register, 0-%d are valid.",
                TECH_NUM_GRIDS-1);
            PL()->ShowPrompt(buf);
            return;
        }
    }
    WindowDesc *wdesc =
        EV()->CurrentWin() ? EV()->CurrentWin() : DSP()->MainWdesc();
    GridDesc lastgrid = *wdesc->Attrib()->grid(wdesc->Mode());
    wdesc->Attrib()->grid(wdesc->Mode())->set(
        *Tech()->GridReg(indx, wdesc->Mode()));
    Tech()->SetGridReg(0, lastgrid, wdesc->Mode());
    if (lastgrid.visually_differ(wdesc->Attrib()->grid(wdesc->Mode())))
        wdesc->Redisplay(0);
    if (wdesc == DSP()->MainWdesc())
        XM()->ShowParameters();
    PL()->ErasePrompt();
}


//-----------------------------------------------------------------------------
// Help
//

void
bangcmds::help(const char *s)
{
#ifdef HAVE_MOZY
    PL()->ErasePrompt();
    if (!*s)
        s = "xicinfo";
    DSPmainWbag(PopUpHelp(s))
#else
    (void)s;
    PL()->ShowPrompt("Help system not available.");
#endif
}


void
bangcmds::helpfont(const char *s)
{
#ifdef HAVE_MOZY
    HLP()->set_font_family(s);
    PL()->ErasePrompt();
#else
    (void)s;
    PL()->ShowPrompt("Help system not available.");
#endif
}


void
bangcmds::helpfixed(const char *s)
{
#ifdef HAVE_MOZY
    HLP()->set_fixed_family(s);
    PL()->ErasePrompt();
#else
    (void)s;
    PL()->ShowPrompt("Help system not available.");
#endif
}


void
bangcmds::helpreset(const char*)
{
#ifdef HAVE_MOZY
    HLP()->rehash();
    PL()->ErasePrompt();
#else
    PL()->ShowPrompt("Help system not available.");
#endif
}


//-----------------------------------------------------------------------------
// Keyboard
//

void
bangcmds::kmap(const char *s)
{
    const char *usage = "Usage:  !kmap key_map_file";
    char *tok = lstring::getqtok(&s);
    if (!tok) {
        PL()->ShowPrompt(usage);
        return;
    }
    char *estr = Kmap()->ReadMapFile(tok);
    if (!estr)
        Log()->ErrorLog("!kmap", "Key map file not found.");
    else if (strcmp(estr, "ok"))
        Log()->ErrorLogV("!kmap", "Key map file read failed:\n%s", estr);
    else
        PL()->ShowPrompt("Key map file read.");
    delete [] estr;
    delete [] tok;
}


//-----------------------------------------------------------------------------
// Layers
//

void
bangcmds::ltab(const char *s)
{
    const char *usage =
        "Usage:  !ltab a[dd]|i[nsert]|rem[ove]|ren[ame] (arguments)";
    char *tok = lstring::gettok(&s);
    if (!tok) {
        PL()->ShowPrompt(usage);
        return;
    }
    if (lstring::ciprefix("rem", tok)) {
        delete [] tok;
        const char *t = LT()->RemoveLayer(s, DSP()->CurMode());
        if (t)
            PL()->ShowPrompt(t);
        else
            PL()->ErasePrompt();
        return;
    }
    if (lstring::ciprefix("ren", tok)) {
        delete [] tok;
        char *oldname = lstring::gettok(&s);
        char *newname = lstring::gettok(&s);
        if (!LT()->RenameLayer(oldname, newname))
            PL()->ShowPrompt(Errs()->get_error());
        else
            PL()->ErasePrompt();
        delete [] oldname;
        delete [] newname;
        return;
    }
    if (lstring::ciprefix("a", tok)) {
        delete [] tok;
        char *t;
        bool err = false;
        while ((t = lstring::gettok(&s)) != 0) {
            if (!CDldb()->newLayer(t, DSP()->CurMode())) {
                PL()->ShowPromptV("Failed to add layer %s.", t);
                Log()->ErrorLogV("!ltab", "Failed to add layer %s:\n%s",
                    t, Errs()->get_error());
                delete [] t;
                err = true;
                break;
            }
            delete [] t;
        }
        LT()->InitLayerTable();
        LT()->ShowLayerTable();
        if (!err)
            PL()->ErasePrompt();
        return;
    }
    if (lstring::ciprefix("i", tok)) {
        delete [] tok;
        char *nm = lstring::gettok(&s);
        if (!nm) {
            PL()->ShowPrompt("Missing layer name.");
            return;
        }
        char *ival = lstring::gettok(&s);
        int v, ix = -1;
        if (ival && sscanf(ival, "%d", &v) == 1)
            ix = v;
        delete [] ival;
        if (!LT()->AddLayer(nm, ix)) {
            PL()->ShowPromptV("Failed to add layer %s.", nm);
            Log()->ErrorLogV("!ltab", "Failed to add layer %s:\n%s",
                nm, Errs()->get_error());
        }
        else
            PL()->ErasePrompt();
        delete [] nm;
        return;
    }
    PL()->ShowPrompt(usage);
    delete [] tok;
}


void
bangcmds::ltsort(const char*)
{
    CDldb()->sort(Physical);
    LT()->InitLayerTable();
    LT()->ShowLayerTable();
    XM()->PopUpLayerPalette(0, MODE_UPD, false, 0);
}


void
bangcmds::exlayers(const char*)
{
    CDl *ld;
    {
        bool first = true;
        CDextLgen gen(CDL_CONDUCTOR);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("Conductor:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
    {
        bool first = true;
        CDextLgen gen(CDL_ROUTING);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("Routing:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
    {
        bool first = true;
        CDextLgen gen(CDL_GROUNDPLANE);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("GroundPlane:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
    {
        bool first = true;
        CDextLgen gen(CDL_IN_CONTACT);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("Contact:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
    {
        bool first = true;
        CDextLgen gen(CDL_VIA);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("Via:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
    {
        bool first = true;
        CDextLgen gen(CDL_DIELECTRIC);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("Dielectric:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
    {
        bool first = true;
        CDextLgen gen(CDL_PLANARIZE);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("Planarize:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
    {
        bool first = true;
        CDextLgen gen(CDL_DARKFIELD);
        while ((ld = gen.next()) != 0) {
            if (first) {
                printf("DarkField:\n");
                first = false;
            }
            printf("  %s\n", ld->name());
        }
    }
}


//-----------------------------------------------------------------------------
// Layout Information
//

void
bangcmds::fileinfo(const char *s)
{
    char *fname = lstring::getqtok(&s);
    if (!fname) {
        PL()->ShowPrompt(
            "Usage:  !fileinfo filename [flags] [outfilename]");
        return;
    }
    char *fstr = lstring::getqtok(&s);
    int flags = cCHD::infoFlags(fstr);
    delete [] fstr;
    char *outname = lstring::getqtok(&s);
    if (!outname)
        outname = lstring::copy("xic_fileinfo.log");

    if (!filestat::create_bak(outname)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        PL()->ShowPrompt("Error: file write error.");
        delete [] fname;
        delete [] outname;
        return;
    }
    FILE *op = fopen(outname, "w");
    if (!op) {
        PL()->ShowPromptV("Error: failed to open %s.", outname);
        delete [] fname;
        delete [] outname;
        return;
    }

    char *realname;
    FILE *fp = FIO()->POpen(fname, "r", &realname);
    if (fp) {
        FileType ft = FIO()->GetFileType(fp);
        fclose(fp);
        if (FIO()->IsSupportedArchiveFormat(ft)) {
            cCHD *chd = FIO()->NewCHD(realname, ft, Physical, 0, cvINFOplpc);
            if (chd) {
                chd->prInfo(op, Physical, flags);
                delete chd;
                PL()->ErasePrompt();
            }
        }
        else {
            PL()->ShowPrompt("Error: incorrect file type.");
            delete [] fname;
            delete [] outname;
            delete [] realname;
            fclose(op);
            return;
        }
    }
    else {
        PL()->ShowPrompt("Error: file not found.");
        delete [] fname;
        delete [] outname;
        fclose(op);
        return;
    }

    fclose(op);
    delete [] fname;
    delete [] realname;

    char buf[256];
    sprintf(buf, "Done, info written to file \"%s\", view file? [n] ",
        outname);
    char *in = PL()->EditPrompt(buf, "n");
    in = lstring::strip_space(in);
    if (in && (*in == 'y' || *in == 'Y'))
        DSPmainWbag(PopUpFileBrowser(outname))
    PL()->ErasePrompt();
    delete [] outname;
}


void
bangcmds::summary(const char *s)
{
    CDs *cursd = CurCell(true);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    char *tok, *fname = 0;
    int lev = 0;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (!strcmp(tok, "-v")) {
            lev = 1;
            delete [] tok;
            continue;
        }
        if (fname) {
            delete [] fname;
            PL()->ShowPrompt("Usage:  !summary [-v] [filename]");
            return;
        }
        fname = pathlist::expand_path(tok, true, true);
        delete [] tok;
    }
    if (!fname)
        fname = lstring::copy("xic_summary.log");
    if (!filestat::create_bak(fname)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        PL()->ShowPrompt("Error: file write error.");
        delete [] fname;
        return;
    }
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        PL()->ShowPromptV("Error: failed to open %s.", fname);
        delete [] fname;
        return;
    }
    const char *sep = "---------------------------------------------\n";
    fprintf(fp, "Summary of %s cells rooted in %s\n",
        cursd->isElectrical() ? "electrical" : "physical",
        Tstring(cursd->cellname()));
    fprintf(fp, "Created with %s.\n", XM()->IdString());

    stringlist *s0 = 0;
    CDgenHierDn_s gen(cursd);
    CDs *sd;
    bool err;
    while ((sd = gen.next(&err)) != 0)
        s0 = new stringlist(lstring::copy(Tstring(sd->cellname())), s0);
    stringlist::sort(s0);
    while (s0) {
        CDcbin cbin;
        CDcdb()->findSymbol(s0->string, &cbin);
        sd = cursd->isElectrical() ? cbin.elec() : cbin.phys();
        if (sd) {
            fputs(sep, fp);
            char *str = XM()->Info(sd, lev);
            fputs(str, fp);
            delete [] str;
        }
        stringlist *sx = s0;
        s0 = s0->next;
        delete [] sx->string;
        delete sx;
    }
    fclose(fp);

    char buf[256];
    sprintf(buf, "Done, summary written to file \"%s\", view file? [n] ",
        fname);
    char *in = PL()->EditPrompt(buf, "n");
    in = lstring::strip_space(in);
    if (in && (*in == 'y' || *in == 'Y'))
        DSPmainWbag(PopUpFileBrowser(fname))
    PL()->ErasePrompt();

    delete [] fname;
}


void
bangcmds::compare(const char *string)
{
    cCompare cmp;
    if (!cmp.parse(string)) {
        PL()->ShowPromptV("Error: %s", Errs()->get_error());
        return;
    }
    if (!cmp.setup()) {
        PL()->ShowPromptV("Error: %s", Errs()->get_error());
        return;
    }
    dspPkgIf()->SetWorking(true);
    DFtype df = cmp.compare();
    dspPkgIf()->SetWorking(false);

    if (df == DFabort)
        PL()->ShowPrompt("Comparison aborted.");
    else if (df == DFerror)
        PL()->ShowPromptV("Comparison failed: %s.", Errs()->get_error());
    else {
        char buf[256];
        sprintf(buf,
            "Comparison data written to file \"%s\", view file? [n] ",
            DIFF_LOG_FILE);
        char *in = PL()->EditPrompt(buf, "n");
        in = lstring::strip_space(in);
        if (in && (*in == 'y' || *in == 'Y'))
            DSPmainWbag(PopUpFileBrowser(DIFF_LOG_FILE))
        PL()->ErasePrompt();
    }
}


void
bangcmds::diffcells(const char *string)
{
    diff_parser dp;
    char *fname = lstring::getqtok(&string);
    bool ret = dp.diff2cells(fname);
    delete [] fname;
    if (!ret) {
        Log()->ErrorLog("!diffcells", Errs()->get_error());
        PL()->ShowPrompt("Operation returned error.");
    }
    else
        PL()->ShowPrompt("Operation completed successfully.");
}


void
bangcmds::empties(const char *s)
{
    char *tok = lstring::gettok(&s);
    if (tok) {
        if (!strcmp(tok, "force_delete_all")) {
            Cvt()->CheckEmpties(true);
            PL()->ShowPrompt(
                "All empty cells/instances have been deleted when possible");
        }
        else
            PL()->ShowPrompt("Usage: !empties [force_delete_all]");
        delete tok;
        return;
    }
    Cvt()->CheckEmpties(false);
    PL()->ErasePrompt();
}


void
bangcmds::area(const char *s)
{
    if (DSP()->CurCellName()) {
        if (DSP()->CurMode() != Physical) {
            PL()->ShowPrompt(
                "The !area command applies to physical mode only.");
            return;
        }
        CDs *cursd = CurCell(Physical);
        if (!cursd)
            return;
        if (Selections.hasTypes(cursd, "bpwc")) {
            double g_area = 0.0;
            double c_area = 0.0;
            sSelGen sg(Selections, cursd, "bpwc");
            CDo *od;
            while ((od = sg.next()) != 0) {
                if (od->type() == CDINSTANCE)
                    c_area += od->area();
                else
                    g_area += od->area();
            }
            PL()->ShowPromptV(
                "Total area of selected geometry: %.6f, subcells: %.6f",
                g_area, c_area);
        }
        else {
            char *t = lstring::gettok(&s);
            CDl *ld = 0;
            if (!t && DSP()->CurMode() == Physical)
                ld = LT()->CurLayer();
            else if (t)
                ld = CDldb()->findLayer(t, Physical);
            if (!ld) {
                if (t)
                    PL()->ShowPromptV("Can't find layer %s.", t);
                else
                    PL()->ShowPrompt("Can't use current layer.");
                return;
            }
            delete [] t;
            double d = cursd->area(ld, true);
            double dc = cursd->BB()->area();
            PL()->ShowPromptV("%s: area = %.6f, coverage = %g%%.",
                ld->name(), d, 100*d/dc);
        }
    }
    else
        PL()->ErasePrompt();
}


void
bangcmds::perim(const char*)
{
    if (DSP()->CurCellName()) {
        if (DSP()->CurMode() != Physical) {
            PL()->ShowPrompt(
                "The !perim command applies to physical mode only.");
            return;
        }
        double g_perim = 0.0;
        double c_perim = 0.0;
        sSelGen sg(Selections, CurCell(Physical), "bpwc");
        CDo *od;
        while ((od = sg.next()) != 0) {
            if (od->type() == CDINSTANCE)
                c_perim += od->perim();
            else
                g_perim += od->perim();
        }
        int ndgt = CD()->numDigits();
        PL()->ShowPromptV("Total perimeter in microns of selected geometry:"
            " %.*f, subcells: %.*f",
            ndgt, g_perim, ndgt, c_perim);
    }
    else
        PL()->ErasePrompt();
}


void
bangcmds::bb(const char*)
{
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("This command for physical mode only!");
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("Physical part of current cell is empty.");
        return;
    }
    const BBox *BB = cursdp->BB();
    int ndgt = CD()->numDigits();
    PL()->ShowPromptV("L,B R,T: %.*f,%.*f %.*f,%.*f",
        ndgt, MICRONS(BB->left), ndgt, MICRONS(BB->bottom),
        ndgt, MICRONS(BB->right), ndgt, MICRONS(BB->top));
}


namespace {
    // This is not a command (though it could be), called from
    // bang_checkgrid.
    //
    void checkgrid1(const char *s)
    {
        // checkgrid1 -l layer_list -s -g spac -b l,b,r,t -t bpw -d depth
        // -f ofile

        /* already checked in caller
        if (DSP()->CurMode() != Physical) {
            PL()->ShowPrompt("Switch to physical mode to run !checkgrid1.");
            return;
        }
        */

        CDs *cursd = CurCell(Physical);
        /* already checked in caller
        if (!cursd) {
            PL()->ShowPrompt("No current cell!");
            return;
        }
        */

        const char *msg1 = "Error: missing token after \"%s\".";
        const char *msg2 = "Error: syntax after \"%s\".";

        const char *layer_list = 0;
        int spacing = INTERNAL_UNITS(
            DSP()->MainWdesc()->Attrib()->grid(Physical)->spacing(Physical));
        BBox BB;
        const char *types = 0;
        int depth = CDMAXCALLDEPTH;
        const char *fname = 0;
        bool use_bb = false;
        bool skip = false;
        bool ret = false;

        if (*s) {
            char *tok;
            while ((tok = lstring::gettok(&s)) != 0) {
                if (!strcmp(tok, "-l")) {
                    char *str = lstring::getqtok(&s);
                    if (!str) {
                        PL()->ShowPromptV(msg1, "-l");
                        delete [] tok;
                        goto bad;
                    }
                    layer_list = str;
                }
                else if (!strcmp(tok, "-s"))
                    skip = true;
                else if (!strcmp(tok, "-g")) {
                    char *str = lstring::gettok(&s);
                    if (!str) {
                        PL()->ShowPromptV(msg1, "-g");
                        delete [] tok;
                        goto bad;
                    }
                    double sp;
                    if (sscanf(str, "%lf", &sp) != 1) {
                        PL()->ShowPromptV(msg2, "-g");
                        delete [] str;
                        delete [] tok;
                        goto bad;
                    }
                    spacing = INTERNAL_UNITS(sp);
                }
                else if (!strcmp(tok, "-b")) {
                    char *str = lstring::gettok(&s);
                    if (!str) {
                        PL()->ShowPromptV(msg1, "-b");
                        delete [] tok;
                        goto bad;
                    }
                    double l, b, r, t;
                    if (sscanf(str, "%lf,%lf,%lf,%lf", &l, &b, &r, &t) != 4) {
                        PL()->ShowPromptV(msg2, "-b");
                        delete [] str;
                        delete [] tok;
                        goto bad;
                    }
                    BB.left = INTERNAL_UNITS(l);
                    BB.bottom = INTERNAL_UNITS(b);
                    BB.right = INTERNAL_UNITS(r);
                    BB.top = INTERNAL_UNITS(t);
                    if (BB.left > BB.right)
                        mmSwapInts(BB.left, BB.right);
                    if (BB.bottom > BB.top)
                        mmSwapInts(BB.bottom, BB.top);
                    use_bb = true;
                }
                else if (!strcmp(tok, "-t")) {
                    char *str = lstring::gettok(&s);
                    if (!str) {
                        PL()->ShowPromptV(msg1, "-t");
                        delete [] tok;
                        goto bad;
                    }
                    types = str;
                }
                else if (!strcmp(tok, "-d")) {
                    char *str = lstring::gettok(&s);
                    if (!str) {
                        PL()->ShowPromptV(msg1, "-d");
                        delete [] tok;
                        goto bad;
                    }
                    if (sscanf(str, "%d", &depth) != 1) {
                        PL()->ShowPromptV(msg2, "-d");
                        delete [] str;
                        delete [] tok;
                        goto bad;
                    }
                    delete [] str;
                }
                else if (!strcmp(tok, "-f")) {
                    fname = lstring::getqtok(&s);
                    if (!fname) {
                        PL()->ShowPromptV(msg1, "-f");
                        delete [] tok;
                        goto bad;
                    }
                }
                else if (!strcmp(tok, "-")) {
                    // ignore this;
                    ;
                }
                else {
                    PL()->ShowPromptV("Error: unrecognized argument %s.", tok);
                    delete [] tok;
                    goto bad;
                }
            }
        }
        if (!fname) {
            char buf[256];
            sprintf(buf, "%s_vertices.log", Tstring(cursd->cellname()));
            fname = lstring::copy(buf);
        }
        FILE *fp;
        if (!strcmp(fname, "stdout"))
            fp = stdout;
        else {
            if (!filestat::create_bak(fname)) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
                PL()->ShowPromptV("Can't move \"%s\" to \"%s.bak\".", fname,
                    fname);
                goto bad;
            }
            fp = fopen(fname, "w");
        }
        if (!fp) {
            PL()->ShowPromptV("Can't open \"%s\" file for output.", fname);
            goto bad;
        }

        dspPkgIf()->SetWorking(true);
        PL()->ShowPrompt("Working...");
        ret = CD()->CheckGrid(cursd, spacing, use_bb ? &BB : 0, layer_list,
            skip, types, depth, fp);
        if (fp != stdout)
            fclose(fp);
        dspPkgIf()->SetWorking(false);

        if (ret)
            PL()->ShowPromptV("Done, results in file \"%s\".", fname);
        else
            PL()->ShowPrompt("Done, terminated with error.");

    bad:
        delete [] layer_list;
        delete [] types;
        delete [] fname;
    }
}


void
bangcmds::checkgrid(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("Switch to physical mode to run !checkgrid.");
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No physical current cell!");
        return;
    }
    PL()->ErasePrompt();
    if (*s == 'o' || *s == 'n' || *s == '0') {
        DSP()->EraseMarks(MARK_BOX);
        return;
    }
    if (*s == '-') {
        checkgrid1(s);
        return;
    }

    bool docells = false;
    if (*s == 'c')
        docells = true;

    bool issel = Selections.hasTypes(CurCell(Physical), "bpwc");

    int vcnt = 0;
    int ocnt = 0;
    if (issel) {
        sSelGen sg(Selections, CurCell(Physical));
        CDo *od;
        while ((od = sg.next()) != 0) {
            int v = DSP()->MainWdesc()->CheckGrid(od, true);
            if (v) {
                ocnt++;
                vcnt += v;
            }
            else
                Selections.removeObject(CurCell(Physical), od);
        }
    }
    else {
        Selections.deselectTypes(CurCell(Physical), 0);
        CDl *ld;
        CDlgen lgen(Physical,
            docells ? CDlgen::BotToTopWithCells : CDlgen::BotToTopNoCells);
        while ((ld = lgen.next()) != 0) {
            // Check only visible, selectable layers.  Note that the
            // cell layer is always checked.

            if (ld->isInvisible())
                continue;
            if (ld->isNoSelect())
                continue;
            CDg gdesc;
            gdesc.init_gen(cursdp, ld);
            CDo *odesc;
            while ((odesc = gdesc.next()) != 0) {
                int v = DSP()->MainWdesc()->CheckGrid(odesc, true);
                if (v) {
                    odesc->set_state(CDVanilla);
                    Selections.insertObject(CurCell(Physical), odesc);
                    ocnt++;
                    vcnt += v;
                }
            }
        }
    }
    if (vcnt)
        PL()->ShowPromptV("Found %d off-grid vertices in %d objects.",
            vcnt, ocnt);
    else
        PL()->ShowPrompt("No off-grid vertices found.");
}


void
bangcmds::checkover(const char *s)
{
    if (!DSP()->CurCellName()) {
        PL()->ErasePrompt();
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    bool tmpfile = false;
    char *fname = lstring::getqtok(&s);
    if (!fname) {
        fname = filestat::make_temp("or");
        tmpfile = true;
    }
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        PL()->ShowPromptV("Error: can't open file %s.", fname);
        delete [] fname;
        return;
    }
    int ndgt = CD()->numDigits();
    fprintf(fp, "Overlapping subcells of %s.\n\n",
        Tstring(DSP()->CurCellName()));
    bool found = false;
    CDm_gen mgen(cursdp, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
        CDc_gen cgen(m);
        for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
            CDg gdesc;
            gdesc.init_gen(cursdp, CellLayer(), &c->oBB());
            bool hdr = false;
            CDc *cdesc;
            while ((cdesc = (CDc*)gdesc.next()) != 0) {
                if (cdesc == c)
                    continue;
                if (cdesc->oBB().right <= c->oBB().left ||
                        cdesc->oBB().left >= c->oBB().right ||
                        cdesc->oBB().top <= c->oBB().bottom ||
                        cdesc->oBB().bottom >= c->oBB().top)
                    continue;
                if (!hdr) {
                    fprintf(fp, "Instance of %s, %.*f,%.*f %.*f,%.*f\n",
                        Tstring(m->cellname()),
                        ndgt, MICRONS(c->oBB().left),
                        ndgt, MICRONS(c->oBB().bottom),
                        ndgt, MICRONS(c->oBB().right),
                        ndgt, MICRONS(c->oBB().top));
                    hdr = true;
                }
                fprintf(fp, "    Instance of %s, %.*f,%.*f %.*f,%.*f\n",
                    Tstring(cdesc->cellname()),
                    ndgt, MICRONS(cdesc->oBB().left),
                    ndgt, MICRONS(cdesc->oBB().bottom),
                    ndgt, MICRONS(cdesc->oBB().right),
                    ndgt, MICRONS(cdesc->oBB().top));
                found = true;
            }
        }
    }
    if (!found)
        fprintf(fp, "No overlapping subcells found.\n");
    fclose(fp);
    if (!tmpfile) {
        char *in = PL()->EditPrompt(
            "Overlap report written to %s.  View file? ", "n");
        in = lstring::strip_space(in);
        if (in && (*in == 'y' || *in == 'Y'))
            DSPmainWbag(PopUpFileBrowser(fname))
    }
    else {
        DSPmainWbag(PopUpFileBrowser(fname))
        unlink(fname);
    }
    delete [] fname;
}


void
bangcmds::check45(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("switch to physical mode for !check45.");
        return;
    }

    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No current cell!");
        return;
    }

    bool ponly = false;
    bool wonly = false;
    if (s && (*s == 'p' || *s == 'P'))
        ponly = true;
    else if (s && (*s == 'w' || *s == 'W'))
        wonly = true;

    const char *types = "pw";
    if (ponly)
        types = "p";
    else if (wonly)
        types = "w";
    Selections.deselectTypes(cursdp, types);
    int pcount = 0, wcount = 0;
    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        // Check only visible, selectable layers.

        if (ld->isInvisible())
            continue;
        if (ld->isNoSelect())
            continue;
        CDg gdesc;
        gdesc.init_gen(cursdp, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() == CDPOLYGON) {
                if (wonly)
                    continue;
                if (((const CDpo*)odesc)->po_has_non45()) {
                    odesc->set_state(CDVanilla);
                    Selections.insertObject(cursdp, odesc);
                    pcount++;
                }
            }
            if (odesc->type() == CDWIRE) {
                if (ponly)
                    continue;
                if (((const CDw*)odesc)->w_has_non45()) {
                    odesc->set_state(CDVanilla);
                    Selections.insertObject(cursdp, odesc);
                    wcount++;
                }
            }
        }
    }
    if (ponly)
        PL()->ShowPromptV("Selected %d polygons with non-45 angle.", pcount);
    else if (wonly)
        PL()->ShowPromptV("Selected %d wires with non-45 angle.", wcount);
    else
        PL()->ShowPromptV("Selected %d polygons %d wires with non-45 angle.",
            pcount, wcount);
}


void
bangcmds::dups(const char*)
{
    Selections.deselectTypes(CurCell(), 0);
    int dupcnt = 0;

    CDs *cursd = CurCell();
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (cursd->owner()) {
        PL()->ShowPrompt("Current cell is symbolic, display schematic first.");
        return;
    }
    CDl *ld;
    CDlgen lgen(DSP()->CurMode(), CDlgen::BotToTopWithCells);
    while ((ld = lgen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(cursd, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->state() == CDSelected)
                continue;

            CDol *o0 = cursd->db_list_coinc(odesc);
            for (CDol *o = o0; o; o = o->next) {
                o->odesc->set_state(CDVanilla);
                Selections.insertObject(CurCell(true), o->odesc);
                dupcnt++;
            }
            CDol::destroy(o0);
        }
    }
    if (dupcnt == 0)
        PL()->ShowPrompt("No coincident duplicates found.");
    else if (dupcnt == 1)
        PL()->ShowPrompt("One coincident duplicate found, selected.");
    else
        PL()->ShowPromptV("Found %d coincident duplicates, all selected.",
            dupcnt);
}

void
bangcmds::wirecheck(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("Switch to physical mode for !wirecheck.");
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No current cell!");
        return;
    }

    CDol *wirelist = 0;
    sSelGen sg(Selections, CurCell(Physical), "w");
    CDo *od;
    while ((od = sg.next()) != 0)
        wirelist = new CDol(od, wirelist);

    Selections.deselectTypes(CurCell(Physical), "w");
    int count = 0;
    if (!*s) {
        CDl *ld;
        CDlgen lgen(Physical);
        while ((ld = lgen.next()) != 0) {
            if (wirelist) {
                for (CDol *o = wirelist; o; o = o->next) {
                    if (o->odesc->ldesc() != ld)
                        continue;
                    Wire w(((CDw*)o->odesc)->w_wire());
                    if (!w.checkWire()) {
                        o->odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, o->odesc);
                        count++;
                    }
                }
            }
            else {
                CDg gdesc;
                gdesc.init_gen(cursdp, ld);
                CDo *odesc;
                while ((odesc = gdesc.next()) != 0) {
                    if (odesc->type() != CDWIRE)
                        continue;
                    Wire w(((CDw*)odesc)->w_wire());
                    if (!w.checkWire()) {
                        odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, odesc);
                        count++;
                    }
                }
            }
        }
    }
    else {
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            CDl *ld = CDldb()->findLayer(tok, Physical);
            delete [] tok;
            if (!ld)
                continue;
            if (wirelist) {
                for (CDol *o = wirelist; o; o = o->next) {
                    if (o->odesc->ldesc() != ld)
                        continue;
                    Wire w(((CDw*)o->odesc)->w_wire());
                    if (!w.checkWire()) {
                        o->odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, o->odesc);
                        count++;
                    }
                }
            }
            else {
                CDg gdesc;
                gdesc.init_gen(cursdp, ld);
                CDo *odesc;
                while ((odesc = gdesc.next()) != 0) {
                    if (odesc->type() != CDWIRE)
                        continue;
                    Wire w(((CDw*)odesc)->w_wire());
                    if (!w.checkWire()) {
                        odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, odesc);
                        count++;
                    }
                }
            }
        }
    }
    CDol::destroy(wirelist);
    if (count) {
        XM()->ShowParameters();
        PL()->ShowPromptV("Selected wires are questionable, found %d.",
            count);
    }
    else
        PL()->ShowPrompt("No questionable wires found.");
}


void
bangcmds::polycheck(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("switch to physical mode for !polycheck.");
        return;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No current cell!");
        return;
    }

    CDol *polylist = 0;
    sSelGen sg(Selections, CurCell(Physical), "p");
    CDo *od;
    while ((od = sg.next()) != 0)
        polylist = new CDol(od, polylist);

    Selections.deselectTypes(CurCell(Physical), "p");
    int count = 0;
    if (!*s) {
        CDl *ld;
        CDlgen lgen(Physical);
        while ((ld = lgen.next()) != 0) {
            if (polylist) {
                for (CDol *o = polylist; o; o = o->next) {
                    if (o->odesc->ldesc() != ld)
                        continue;
                    int ret = ((CDpo*)o->odesc)->po_check_poly(PCHK_REENT,
                        true);
                    if (ret) {
                        // Select if any flag set.
                        o->odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, o->odesc);
                        count++;
                    }
                }
            }
            else {
                CDg gdesc;
                gdesc.init_gen(cursdp, ld);
                CDo *odesc;
                while ((odesc = gdesc.next()) != 0) {
                    if (odesc->type() != CDPOLYGON)
                        continue;
                    int ret = ((CDpo*)odesc)->po_check_poly(PCHK_REENT, true);
                    if (ret) {
                        // Select if any flag set.
                        odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, odesc);
                        count++;
                    }
                }
            }
        }
    }
    else {
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            CDl *ld = CDldb()->findLayer(tok, Physical);
            delete [] tok;
            if (!ld)
                continue;
            if (polylist) {
                for (CDol *o = polylist; o; o = o->next) {
                    if (o->odesc->ldesc() != ld)
                        continue;
                    int ret = ((CDpo*)o->odesc)->po_check_poly(PCHK_REENT,
                        true);
                    if (ret) {
                        // Select if any flag set.
                        o->odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, o->odesc);
                        count++;
                    }
                }
            }
            else {
                CDg gdesc;
                gdesc.init_gen(cursdp, ld);
                CDo *odesc;
                while ((odesc = gdesc.next()) != 0) {
                    if (odesc->type() != CDPOLYGON)
                        continue;
                    int ret = ((CDpo*)odesc)->po_check_poly(PCHK_REENT, true);
                    if (ret) {
                        // Select if any flag set.
                        odesc->set_state(CDVanilla);
                        Selections.insertObject(cursdp, odesc);
                        count++;
                    }
                }
            }
        }
    }
    CDol::destroy(polylist);
    if (count) {
        XM()->ShowParameters();
        PL()->ShowPromptV("Selected polys are badly formed, found %d.",
            count);
    }
    else
        PL()->ShowPrompt("No badly formed polys found.");
}


void
bangcmds::polymanh(const char *s)
{
    bool shownon = false;
    if (s && *s)
        shownon = true;

    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("switch to physical mode for !polymanh.");
        return;
    }

    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    Selections.deselectTypes(cursdp, "p");
    int count = 0;
    CDl *ld;
    CDlgen lgen(Physical);
    while ((ld = lgen.next()) != 0) {
        // Check only visible, selectable layers.

        if (ld->isInvisible())
            continue;
        if (ld->isNoSelect())
            continue;
        CDg gdesc;
        gdesc.init_gen(cursdp, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() == CDPOLYGON) {
                bool ismanh = ((const CDpo*)odesc)->po_is_manhattan();
                if ((ismanh && !shownon) || (!ismanh && shownon)) {
                    odesc->set_state(CDVanilla);
                    Selections.insertObject(cursdp, odesc);
                    count++;
                }
            }
        }
    }
    PL()->ShowPromptV("Selected %d %s polygons.", count,
        shownon ? "non-Manhattan" : "Manhattan");
}


void
bangcmds::poly45(const char*)
{
    // This command is redundant.
    check45("p");
}


void
bangcmds::polynum(const char *s)
{
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("switch to physical mode for !polynum.");
        return;
    }
    CDs *cursd = CurCell(Physical);
    if (!cursd) {
        PL()->ShowPrompt("No current cell!");
        return;
    }

    bool ostate = DSP()->NumberVertices();
    if (!s || !*s)
        DSP()->SetNumberVertices(!DSP()->NumberVertices());
    else if (*s == 'n' || *s == 'N' || *s == '0' || lstring::ciprefix("of", s))
        DSP()->SetNumberVertices(false);
    else if (*s == 'y' || *s == 'Y' || *s == '1' || lstring::ciprefix("on", s))
        DSP()->SetNumberVertices(true);
    else
        DSP()->SetNumberVertices(!DSP()->NumberVertices());

    if (ostate != DSP()->NumberVertices()) {
        sSelGen sg(Selections, cursd, "p", false);
        CDo *od;
        while ((od = sg.next()) != 0) {
            Selections.showUnselected(cursd, od);
            Selections.showSelected(cursd, od);
        }
    }
    PL()->ErasePrompt();
}


void
bangcmds::setflag(const char *s)
{
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    while (isspace(*s))
        s++;
    if (!*s || *s == '?') {
        sLstr lstr;
        char buf[256];
        CDs *cursd = CurCell(true);
        if (cursd) {
            sprintf(buf, "Flag status in current cell, %s mode\n",
                DisplayModeNameLC(DSP()->CurMode()));
            lstr.add(buf);
            lstr.add("* --> user settable, name value description:\n");
            const char *format = "%c %-12s%c  %s\n";
            for (FlagDef *f = SdescFlags; f->name; f++) {
                sprintf(buf, format, f->user_settable ? '*' : ' ',
                    f->name, (cursd->getFlags() & f->value) ? '1' : '0',
                    f->desc);
                lstr.add(buf);
            }
        }

        DSPmainWbag(PopUpInfo(MODE_ON, lstr.string(), STY_FIXED))
        PL()->ErasePrompt();
        return;
    }
    const char *msg =
    "Syntax error:  \"setflag name 1/0\" or \"setflag ?\" for flag list.";
    char *name = lstring::gettok(&s);
    char *val = lstring::gettok(&s);
    if (!name || !val) {
        PL()->ShowPrompt(msg);
        delete [] name;
        return;
    }
    bool assert;
    if (*val == '1' || *val == 'y' || *val == 'Y' || *val == 't' ||
            *val == 'T')
        assert = true;
    else if (*val == '0' || *val == 'n' || *val == 'N' || *val == 'f' ||
            *val == 'F')
        assert = false;
    else {
        PL()->ShowPrompt(msg);
        delete [] name;
        delete [] val;
        return;
    }
    delete [] val;
    CDs *cursd = CurCell(true);
    if (cursd) {
        for (FlagDef *f = SdescFlags; f->name; f++) {
            if (!strcasecmp(name, f->name)) {
                if (!f->user_settable) {
                    PL()->ShowPromptV("Flag %s is not user settable.",
                        name);
                    delete [] name;
                    return;
                }
                if (assert) {
                    unsigned fg = cursd->getFlags();
                    fg |= f->value;
                    cursd->setFlags(fg);
                }
                else {
                    unsigned fg = cursd->getFlags();
                    fg &= ~f->value;
                    cursd->setFlags(fg);
                }
                PL()->ShowPromptV("%s is now %s", f->name,
                    assert ? "true" : "false");
                delete [] name;
                if (f->value == CDs_IMMUTABLE)
                    EditIf()->setEditingMode(assert);
                return;
            }
        }
    }
    PL()->ShowPromptV("Flag %s not found.", name);
    delete [] name;
}


//-----------------------------------------------------------------------------
// Libraries
//

namespace {
    void
    list_subcells(CDs *sdesc, SymTab *tab)
    {
        if (sdesc) {
            CDm_gen mgen(sdesc, GEN_MASTERS);
            for (CDm *m = mgen.m_first(); m; m = mgen.m_next())
                list_subcells(m->celldesc(), tab);
            tab->add(Tstring(sdesc->cellname()), 0, true);
        }
    }
}


void
bangcmds::mklib(const char *s)
{
    // mklib [arcfile] [-a] [-l]|[-u]
    // If arcfile is given, references are to cells in arcfile.
    // Otherwise, references are to cells in current hierarchy.  If
    // the current hierarchy is an archive, save references as if in
    // an archive file (can be overridden).
    // -l    reference name is lower-cased cellname
    // -u    reference name is upper-cased cellname
    // -a    append to existing library
    //
    char *arcfile = 0;
    bool append = false;
    bool dolc = false;
    bool douc = false;
    char buf[256];
    char *tok;
    if (!DSP()->CurCellName()) {
        PL()->ErasePrompt();
        return;
    }
    CDcbin cbin(DSP()->CurCellName());
    while ((tok = lstring::gettok(&s)) != 0) {
        if (!strcmp(tok, "-a"))
            append = true;
        else if (!strcmp(tok, "-l"))
            dolc = true;
        else if (!strcmp(tok, "-u"))
            douc = true;
        else if (*tok != '-' && !arcfile)
            arcfile = lstring::copy(tok);
        else {
            PL()->ShowPrompt("Usage:  !mklib [arcfile] [-a] [-l]|[-u]");
            delete [] tok;
            return;
        }
        delete [] tok;
    }
    if (dolc && douc)
        dolc = douc = false;
    char *arcname = 0;

    if (!arcfile && FIO()->IsSupportedArchiveFormat(cbin.fileType())) {
        sprintf(buf, "%s%s", Tstring(DSP()->CurCellName()),
            FIO()->GetTypeExt(cbin.fileType()));
        char mbuf[256];
        const char *msg =
    "Enter reference %s file name, or blank for native cell references: ";
        sprintf(mbuf, msg,
            FIO()->TypeName(cbin.fileType()));
        char *in = PL()->EditPrompt(mbuf, buf);
        in = lstring::strip_space(in);
        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        if (*in)
            arcname = lstring::copy(in);
    }

    char *refpath = 0;
    if (!(arcfile && lstring::is_rooted(arcfile))) {
        const char *in = PL()->EditPrompt(
            "Enter directory path for references: ", ".");
        in = lstring::strip_space(in);
        if (!in) {
            delete [] arcfile;
            delete [] arcname;
            PL()->ErasePrompt();
            return;
        }
        if (!*in)
            in = ".";
        refpath = pathlist::expand_path(in, true, true);
        if (!lstring::is_rooted(refpath)) {
            char *t = new char[strlen(refpath) + 3];
            t[0] = '.';
            t[1] = '/';
            strcpy(t+2, refpath);
            delete [] refpath;
            refpath = pathlist::expand_path(t, true, true);
            delete [] t;
        }
        char *e = refpath + strlen(refpath) - 1;
        if (lstring::is_dirsep(*e))
            *e = 0;
        for (e = refpath; *e; e++) {
            if (isspace(*e) || *e == PATH_SEP) {
                char *t = new char[strlen(refpath) + 3];
                t[0] = '"';
                strcpy(t+1, refpath);
                strcat(t, "\"");
                delete [] refpath;
                refpath = t;
                break;
            }
        }
    }

    if (arcfile)
        strcpy(buf, lstring::strip_path(arcfile));
    else if (arcname)
        strcpy(buf, arcname);
    else
        strcpy(buf, Tstring(DSP()->CurCellName()));
    char *t = strrchr(buf, '.');
    if (t) {
        *t++ = 0;
        if (!strcmp(t, "gz")) {
            char *stmp = strrchr(buf, '.');
            if (stmp)
                *stmp = 0;
        }
    }
    strcat(buf, ".lib");
    char *in = PL()->EditPrompt("Enter name for library file: ", buf);
    in = lstring::strip_space(in);
    if (!in) {
        delete [] arcfile;
        delete [] arcname;
        delete [] refpath;
        PL()->ErasePrompt();
        return;
    }
    if (!*in) {
        PL()->ShowPrompt("Error: no name given for library file.");
        delete [] arcfile;
        delete [] arcname;
        delete [] refpath;
        return;
    }
    char *libfile = lstring::copy(in);

    // put all cell names in tab
    SymTab *tab = new SymTab(false, false);
    if (arcfile) {
        FILE *fp = fopen(arcfile, "rb");
        if (!fp) {
            PL()->ShowPromptV("Error: can't open %s.", arcfile);
            delete [] arcfile;
            delete [] arcname;
            delete [] refpath;
            delete [] libfile;
            delete tab;
            return;
        }
        Errs()->init_error();
        FileType ft = FIO()->GetFileType(fp);
        fclose(fp);
        cCHD *chd = 0;
        if (!FIO()->IsSupportedArchiveFormat(ft))
            Errs()->add_error("Error: %s format not supported.",
                arcfile);
        else
            // no aliasing, except will apply l/u options
            chd = FIO()->NewCHD(arcfile, ft, Electrical, 0);
        if (!chd) {
            PL()->ShowPrompt(Errs()->get_error());
            delete [] arcfile;
            delete [] arcname;
            delete [] refpath;
            delete [] libfile;
            delete tab;
            return;
        }
        if (chd->nameTab(Physical)) {
            namegen_t gen(chd->nameTab(Physical));
            symref_t *p;
            while ((p = gen.next()) != 0) {
                if (FIO()->LookupLibCell(0, Tstring(p->get_name()),
                        LIBdevice, 0))
                    continue;
                tab->add(Tstring(p->get_name()), 0, true);
            }
        }
        if (chd->nameTab(Electrical)) {
            namegen_t gen(chd->nameTab(Electrical));
            symref_t *p;
            while ((p = gen.next()) != 0) {
                if (FIO()->LookupLibCell(0, Tstring(p->get_name()),
                        LIBdevice, 0))
                    continue;
                tab->add(Tstring(p->get_name()), 0, true);
            }
        }
        delete chd;
    }
    else {
        list_subcells(cbin.phys(), tab);
        list_subcells(cbin.elec(), tab);
    }

    // create a stringlist of the cell names
    stringlist *s0 = SymTab::names(tab);
    delete tab;

    int len = stringlist::length(s0);
    if (len <= 0) {
        PL()->ShowPrompt("No cells found!  Library creation aborted.");
        delete [] arcfile;
        delete [] arcname;
        delete [] refpath;
        delete [] libfile;
        return;
    }

    // sort the cells names
    stringlist::sort(s0);

    if (append) {
        FILE *fp = fopen(libfile, "r");
        if (fp) {
            if (!FIO()->IsLibrary(fp)) {
                PL()->ShowPrompt(
                    "Error: File %s is not a library, can't append.");
                fclose(fp);
                stringlist::destroy(s0);
                delete [] arcfile;
                delete [] arcname;
                delete [] refpath;
                delete [] libfile;
                return;
            }
            fclose(fp);
        }
        else
            append = false;
    }

    if (!append) {
        if (!filestat::create_bak(libfile))
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
    }
    FILE *fp = fopen(libfile, append ? "a" : "w");
    if (fp) {
        char *arcpath = 0;
        if (!append) {
            fprintf(fp, "(Library %s);\n", libfile);
            if (arcfile) {
                if (refpath)
                    arcpath = pathlist::mk_path(refpath, arcfile);
                else
                    arcpath = lstring::copy(arcfile);
            }
            else if (arcname)
                arcpath = pathlist::mk_path(refpath, arcname);
            if (arcpath)
                fprintf(fp, "Define ARCH_PATH %s\n", arcpath);
        }
        for (stringlist *sl = s0; sl; sl = sl->next) {
            strcpy(buf, sl->string);
            if (dolc)
                lstring::strtolower(buf);
            else if (douc)
                lstring::strtoupper(buf);
            if (arcfile) {
                if (arcpath)
                    fprintf(fp, "Reference %-20s ARCH_PATH %s\n", buf,
                        sl->string);
                else if (refpath)
                    fprintf(fp, "Reference %s %s/%s %s\n", buf, refpath,
                        arcfile, sl->string);
                else
                    fprintf(fp, "Reference %s %s %s\n", buf,
                        arcfile, sl->string);
            }
            else if (arcname) {
                if (arcpath)
                    fprintf(fp, "Reference %-20s ARCH_PATH %s\n", buf,
                        sl->string);
                else
                    fprintf(fp, "Reference %s %s/%s %s\n", buf, refpath,
                        arcname, sl->string);
            }
            else
                fprintf(fp, "Reference %s %s/%s\n", buf, refpath,
                    sl->string);
        }
        PL()->ShowPromptV("Library file %s %s.", libfile,
            append ? "appended" : "created");
        delete [] arcpath;
    }
    else
        PL()->ShowPromptV("Error: can't open %s.", libfile);
    fclose(fp);
    stringlist::destroy(s0);
    delete [] arcfile;
    delete [] arcname;
    delete [] refpath;
    delete [] libfile;
}


void
bangcmds::lsdb(const char*)
{
    stringlist *s0 = CDsdb()->listDB();
    if (!s0) {
        PL()->ShowPrompt("No \"special\" databases currently saved.");
        return;
    }
    sLstr lstr;
    for (stringlist *sl = s0; sl; sl = sl->next) {
        lstr.add(sl->string);
        lstr.add_c('\n');
    }
    DSPmainWbag(PopUpInfo(MODE_ON, lstr.string(), STY_FIXED))
    PL()->ErasePrompt();
    stringlist::destroy(s0);
    return;
}


//-----------------------------------------------------------------------------
// Marks
//

namespace {
    namespace bangcmds {
        struct MarkState : public CmdState
        {
            MarkState(const char*, const char*);
            virtual ~MarkState();

            void setup(GRobject c, int m, int a)
                {
                    Caller = c;
                    Mode = m;
                    Attri = a;
                }

            void b1down();
            void b1up();
            void esc();
            bool key(int, const char*, int);
            void undo();
            void redo();
            bool action(int, int);
            void message();

        private:
            void SetLevel1(bool show)
                { Level = 1; First = true; if (show) message(); }
            void SetLevel2() { Level = 2; First = false; message(); }

            GRobject Caller;  // calling button
            int State;        // internal state
            bool First;       // true before first point accepted
            int Lastx;        // reference point
            int Lasty;
            int Mode;         // mark mode
            int Attri;        // mark presentation attributes
        };

        MarkState *MarkCmd;
    }

    void save_marks(bool flash)
    {
        CDs *sd = CurCell(true);
        if (!sd)
            return;  // can't happen
        char buf[256];
        sprintf(buf, "%s.%s.marks", Tstring(sd->cellname()),
            sd->isElectrical() ? "elec" : "phys");
        const char *in = PL()->EditPrompt("Filename for saved marks? ", buf);
        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        char *fn = lstring::getqtok(&in);
        int mcnt = DSP()->DumpUserMarks(fn, sd);
        if (mcnt < 0) {
            Log()->ErrorLog("!mark", Errs()->get_error());
            if (flash)
                PL()->FlashMessage("Operation failed.");
            else
                PL()->ShowPrompt("Operation failed.");
        }
        else if (mcnt == 0) {
            if (flash)
                PL()->FlashMessage("No marks found.");
            else
                PL()->ShowPrompt("No marks found.");
        }
        else {
            if (flash)
                PL()->FlashMessageV("Wrote %d marks to %s.", mcnt,
                    fn && *fn ? fn : buf);
            else
                PL()->ShowPromptV("Wrote %d marks to %s.", mcnt,
                    fn && *fn ? fn : buf);
        }
    }

    void recall_marks(bool flash)
    {
        CDs *sd = CurCell(true);
        if (!sd)
            return;  // can't happen
        char buf[256];
        sprintf(buf, "%s.%s.marks", Tstring(sd->cellname()),
            sd->isElectrical() ? "elec" : "phys");
        const char *in = PL()->EditPrompt("Marks file to read? ", buf);
        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        char *fn = lstring::getqtok(&in);
        int mcnt = DSP()->ReadUserMarks(fn);
        if (mcnt < 0) {
            Log()->ErrorLog("!mark", Errs()->get_error());
            if (flash)
                PL()->FlashMessage("Operation failed.");
            else
                PL()->ShowPrompt("Operation failed.");
        }
        else if (mcnt == 0) {
            if (flash)
                PL()->FlashMessage("No marks found.");
            else
                PL()->ShowPrompt("No marks found.");
        }
        else {
            if (flash)
                PL()->FlashMessageV("Read %d marks from %s.", mcnt,
                    fn && *fn ? fn : buf);
            else
                PL()->ShowPromptV("Read %d marks from %s.", mcnt,
                    fn && *fn ? fn : buf);
        }
    }
}

using namespace bangcmds;


void
bangcmds::mark(const char *s)
{
    const char *usage = "Usage: !mark l|b|t|u|c|e|d|w|r [attr_flags]";
    if (MarkCmd)
        MarkCmd->esc();
    if (DSP()->MainWdesc()->DbType() != WDcddb) {
        PL()->ShowPrompt("Marks not available in CHD display mode.");
        return;
    }
    if (!XM()->CheckCurCell(true, true, DSP()->CurMode()))
        return;
    if (CurCell()->owner()) {
        PL()->ShowPrompt("Current cell is symbolic, display schematic first.");
        return;
    }
    char *tok = lstring::gettok(&s);
    if (!tok) {
        PL()->ShowPrompt(usage);
        return;
    }

    int attr = *s ? atoi(s) : 0;

    if (*tok == 'r' || *tok == 'R') {
        recall_marks(false);
        return;
    }
    if (*tok == 'd' || *tok == 'D' || *tok == 'w' || *tok == 'W') {
        save_marks(false);
        return;
    }

    MarkCmd = new MarkState("MARK", "!mark"); 
    if (*tok == 'l' || *tok == 'L')
        MarkCmd->setup(0, 'l', attr);
    else if (*tok == 'b' || *tok == 'B')
        MarkCmd->setup(0, 'b', attr);
    else if (*tok == 't' || *tok == 'T')
        MarkCmd->setup(0, 't', attr);
    else if (*tok == 'u' || *tok == 'U')
        MarkCmd->setup(0, 'u', attr);
    else if (*tok == 'c' || *tok == 'C')
        MarkCmd->setup(0, 'c', attr);
    else if (*tok == 'e' || *tok == 'E')
        MarkCmd->setup(0, 'e', attr);
    else {
        PL()->ShowPrompt(usage);
        delete MarkCmd;
    }
      
    delete [] tok;

    if (MarkCmd) {
        if (!EV()->PushCallback(MarkCmd)) {
            delete MarkCmd;
            return;
        }
        MarkCmd->message();
    }
}


MarkState::MarkState(const char *nm, const char *hk) : CmdState(nm, hk)
{
    Caller = 0;
    State = 0;
    Lastx = Lasty = 0;
    Mode = 0;
    Attri = 0;

    SetLevel1(false);
}


MarkState::~MarkState()
{
    MarkCmd = 0;
}


// Button 1 press handler.
//
void
MarkState::b1down()
{
    if (Level == 1) {
        EV()->Cursor().get_raw(&Lastx, &Lasty);

        State = 1;
        XM()->SetCoordMode(CO_RELATIVE, Lastx, Lasty);
        Gst()->SetGhostAt(GFbox_ns, Lastx, Lasty);
        EV()->DownTimer(GFbox_ns);
    }
    else {
        // Create the box.
        //
        int xc, yc;
        EV()->Cursor().get_raw(&xc, &yc);
        if (action(xc, yc))
            State = 3;
    }
}


// Button 1 release handler.
//
void
MarkState::b1up()
{
    if (Level == 1) {
        if (!State)
            return;

        if (EV()->Cursor().get_downstate() & GR_SHIFT_MASK) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            BBox BB(x, y, Lastx, Lasty);
            BB.fix();
            int bv = 1 + (int)(
                DSP()->PixelDelta()/EV()->CurrentWin()->Ratio());
            BB.bloat(bv);
            DSP()->RemoveUserMarkAt(&BB);
            return;
        }

        if (!EV()->UpTimer() && EV()->Cursor().is_release_ok() &&
                EV()->CurrentWin()) {
            int x, y;
            EV()->Cursor().get_release(&x, &y);
            if (Lastx != x && Lasty != y) {
                if (action(x, y)) {
                    State = 2;
                    return;
                }
            }
        }
        SetLevel2();
    }
    else {
        if (State == 3) {
            State = 2;
            SetLevel1(true);
        }
    }
}


// Esc entered, clean up and abort.
//
void
MarkState::esc()
{
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    if (Caller)
        Menu()->Deselect(Caller);
    delete this;
}


// Change mode or attribute.
//
bool
MarkState::key(int, const char *text, int)
{
    if (text) {
        if (*text == 'r') {
            recall_marks(true);
            message();
            return (true);
        }
        if (*text == 'd' || *text == 'w') {
            save_marks(true);
            message();
            return (true);
        }

        if (*text == 'l') {
            Mode = 'l';
            message();
            return (true);
        }
        if (*text == 'b') {
            Mode = 'b';
            message();
            return (true);
        }
        if (*text == 't') {
            Mode = 't';
            message();
            return (true);
        }
        if (*text == 'u') {
            Mode = 'u';
            message();
            return (true);
        }
        if (*text == 'c') {
            Mode = 'c';
            message();
            return (true);
        }
        if (*text == 'e') {
            Mode = 'e';
            message();
            return (true);
        }
        if (*text >= '0' && *text <= '9') {
            Attri = *text - '0';
            message();
            return (true);
        }
    }
    return (false);
}


// Undo handler.
//
void
MarkState::undo()
{
    if (Level == 1)
        EditIf()->ulUndoOperation();
    else {
        // Undo the corner anchor.
        //
        Gst()->SetGhost(GFnone);
        XM()->SetCoordMode(CO_ABSOLUTE);
        if (State == 2)
            State = 3;
        SetLevel1(true);
    }
}


// Redo handler.
//
void
MarkState::redo()
{
    if (Level == 1) {
        // Redo undone corner anchor or operation.
        //
        if ((State == 1 && !EditIf()->ulHasRedo())) {
            XM()->SetCoordMode(CO_RELATIVE, Lastx, Lasty);
            Gst()->SetGhostAt(GFbox_ns, Lastx, Lasty);
            if (State == 3)
                State = 2;
            SetLevel2();
        }
        else
            EditIf()->ulRedoOperation();
    }
    else {
        if (State == 2) {
            Gst()->SetGhost(GFnone);
            XM()->SetCoordMode(CO_ABSOLUTE);
            EditIf()->ulRedoOperation();
            SetLevel1(true);
        }
    }
}


bool
MarkState::action(int x, int y)
{
    Gst()->SetGhost(GFnone);
    XM()->SetCoordMode(CO_ABSOLUTE);

    CDs *cursd = CurCell(true);
    if (!cursd)
        return (false);
    bool ret = false;
    if (Mode == 'l')
        ret = DSP()->AddUserMark(hlLine, Lastx, Lasty, x, y, Attri);
    else if (Mode == 'b') {
        BBox BB(x, y, Lastx, Lasty);
        BB.fix();
        ret = DSP()->AddUserMark(hlBox, BB.left, BB.bottom, BB.right, BB.top,
            Attri);
    }
    else if (Mode == 't') {
        int yl = mmMin(Lasty, y);
        int yu = mmMax(Lasty, y);
        ret = DSP()->AddUserMark(hlHtriang, yl, yu, Lastx, x, Attri);
    }
    else if (Mode == 'u') {
        int xl = mmMin(Lastx, x);
        int xr = mmMax(Lastx, x);
        ret = DSP()->AddUserMark(hlVtriang, xl, xr, Lasty, y, Attri);
    }
    else if (Mode == 'c') {
        int rad = (int)sqrt((x - Lastx)*(double)(x - Lastx) +
            (y - Lasty)*(double)(y - Lasty));
        ret = DSP()->AddUserMark(hlCircle, Lastx, Lasty, rad, Attri);
    }
    else if (Mode == 'e') {
        BBox BB(x, y, Lastx, Lasty);
        BB.fix();
        int xc = (BB.left + BB.right)/2;
        int yc = (BB.top + BB.bottom)/2;
        int rx = (BB.right - BB.left)/2;
        int ry = (BB.top - BB.bottom)/2;

        ret = DSP()->AddUserMark(hlEllipse, xc, yc, rx, ry, Attri);
    }

    return (ret);
}


void
MarkState::message()
{
    if (Level == 1) {
        const char *which = "figure";
        if (Mode == 'l')
            which = "line";
        else if (Mode == 'b')
            which = "box";
        else if (Mode == 't')
            which = "horizontal triangle";
        else if (Mode == 'u')
            which = "vertical triangle";
        else if (Mode == 'c')
            which = "circle";
        else if (Mode == 'e')
            which = "ellipse";
        PL()->ShowPromptV(
            "Click twice or drag to define %s with attribute %d.",
            which, Attri);
    }
    else
        PL()->ShowPrompt("Click on second point.");
}
// End of MarkState functions.


//-----------------------------------------------------------------------------
// Memory Management
//

void
bangcmds::clearall(const char *s)
{
    char *tok = lstring::gettok(&s);
    bool clear_tech = tok && lstring::cieq(tok, "tech");
    delete [] tok;
    XM()->ClearAll(clear_tech);
}


void
bangcmds::mmstats(const char*)
{
    int inuse = 0;
    int not_inuse = 0;
    CDmmgr()->stats(&inuse, &not_inuse);
    GEOmmgr()->stats(&inuse, &not_inuse);
    printf("Total bytes in use: %d\n", inuse);
    printf("Total bytes not in use: %d\n", not_inuse);
}


void
bangcmds::mmclear(const char*)
{
    CDmmgr()->collectTrash();
    GEOmmgr()->collectTrash();
}


void
bangcmds::attrhash(const char*)
{
    CD()->PrintAttrStats();
}


void
bangcmds::attrclear(const char*)
{
    CD()->ClearAttrDB();
}


void
bangcmds::stclear(const char*)
{
    CD()->ClearStringTables();
}


void
bangcmds::gentest(const char*)
{
    CDg::print_gen_allocation();
    sPF::print_gen_allocation();
}


//-----------------------------------------------------------------------------
// PCells
//

void
bangcmds::rmpcprops(const char *s)
{
    bool doall = false;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        char c = tok[0];
        if (c == '-')
            c = tok[1];
        if (c == 'a' || c == 'A')
            doall = true;
        delete [] tok;
    }

    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    CDgenHierDn_s gen(cursdp);
    CDs *sd;
    bool err;
    while ((sd = gen.next(&err)) != 0) {
        if (sd->isPCellSubMaster()) {
            if (doall || sd->pcType() == CDpcOA) {
                sd->prptyRemove(XICP_PC);
                sd->prptyRemove(XICP_PC_PARAMS);
                sd->setPCell(false, false, false);
                sd->setPCellReadFromFile(false);
            }
        }
    }
    if (err) {
        Log()->WarningLogV("!rmpcprops",
            "Cell hierarchy too deep, recursive?");
        return;
    }
    gen.init(cursdp);
    while ((sd = gen.next(&err)) != 0) {
        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
            CDs *msd = m->celldesc();
            if (!msd->prpty(XICP_PC)) {
                CDc_gen cgen(m);
                for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                    c->prptyRemove(XICP_PC);
                    c->prptyRemove(XICP_PC_PARAMS);
                }
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Rulers
//

void
bangcmds::dr(const char *s)
{
    int n;
    if (isdigit(*s))
        n = atoi(s);
    else if (*s == 'a' || *s == 'A')
        n = -1;
    else
        n = 0;
    XM()->EraseRulers(0, 0, n);
}


//-----------------------------------------------------------------------------
// Scripts
//

void
bangcmds::script(const char *s)
{
    char *name = lstring::getqtok(&s);
    char *path = lstring::getqtok(&s);
    if (!name) {
        PL()->ShowPrompt("Usage:  !script <name> [ <path> ]");
        return;
    }
    XM()->RegisterScript(name, path);
    Menu()->UpdateUserMenu();
    delete [] name;
    delete [] path;
    PL()->ErasePrompt();
}


void
bangcmds::rehash(const char*)
{
    XM()->Rehash();
}


void
bangcmds::exec(const char *s)
{
    if (!*s) {
        PL()->ShowPrompt("Usage:  !exec script_name_or_path");
        return;
    }
    SIfile *sfp;
    stringlist *sl;
    XM()->OpenScript(s, &sfp, &sl, true);
    if (sfp || sl) {
        EditIf()->ulListCheck("script", CurCell(), false);
        SI()->Interpret(sfp, sl, 0, 0);
        if (sfp)
            delete sfp;
        EditIf()->ulCommitChanges(true);
    }
    else
        PL()->ShowPrompt("Script not found.");
}


void
bangcmds::lisp(const char *s)
{
    if (!*s) {
        PL()->ShowPrompt("Usage:  !lisp filename [args ... ]");
        return;
    }
    if (!CdsIn())
        new cTechCdsIn;
    char *err;
    if (!CdsIn()->readEvalLisp(s, CDvdb()->getVariable(VA_ScriptPath),
            false, &err)) {
        PL()->ShowPromptV("Failed: %s.", err ? err : "unknown error");
        delete [] err;
        return;
    }
    PL()->ShowPrompt("Done.");
}


void
bangcmds::listfuncs(const char*)
{
    if (SI()->ShowSubfuncs())
        PL()->ErasePrompt();
    else
        PL()->ShowPrompt("No functions in memory.");
}


void
bangcmds::rmfunc(const char *s)
{
    if (!*s) {
        PL()->ShowPrompt("Usage:  !rmfunc function_name_regex");
        return;
    }

    regex_t preg;
    if (regcomp(&preg, s, REG_EXTENDED | REG_NOSUB)) {
        PL()->ShowPrompt("Regular expression syntax error.");
        return;
    }

    stringlist *sl = SI()->GetSubfuncList();
    for (stringlist *sx = sl; sx; sx = sx->next) {
        char *t = strchr(sx->string, '(');
        if (t)
            *t = 0;

        if (!regexec(&preg, sx->string, 0, 0, 0))
            SI()->FreeSubfunc(sx->string);
    }

    regfree(&preg);
}


void
bangcmds::mkscript(const char *s)
{
    CDs *sdesc = CurCell();
    if (!sdesc) {
        PL()->ShowPrompt("No current cell!");
        return;
    }
    if (sdesc->isElectrical() && sdesc->owner())
        sdesc = sdesc->owner();
    int depth = 0;
    char *fname = 0;
    char *tok;
    while ((tok = lstring::getqtok(&s)) != 0) {
        if (!strcmp(tok, "-d")) {
            delete [] tok;
            tok = lstring::getqtok(&s);
            if (!tok) {
                PL()->ShowPrompt("Missing depth value, command aborted.");
                return;
            }
            if (*tok == 'a' || *tok == 'A') {
                delete [] tok;
                depth = CDMAXCALLDEPTH;
                continue;
            }
            char *e;
            depth = strtol(tok, &e, 10);
            if (e == tok) {
                delete [] tok;
                PL()->ShowPrompt("Bad depth value, command aborted.");
                return;
            }
            if (depth < 0 || depth > CDMAXCALLDEPTH)
                depth = CDMAXCALLDEPTH;
            delete [] tok;
            continue;
        }
        if (fname) {
            delete [] fname;
            delete [] tok;
            PL()->ShowPrompt("Unknown junk in command line, command aborted.");
            return;
        }
        fname = tok;
    }
    if (!fname)
        fname = lstring::copy("mkscript.scr");

    if (!filestat::create_bak(fname)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        PL()->ShowPrompt("Error: file write error.");
        delete [] fname;
        return;
    }
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        PL()->ShowPromptV("Error: failed to open %s.", fname);
        delete [] fname;
        return;
    }
    fprintf(fp, "# Generated by %s\n", XM()->IdString());
    fprintf(fp, "Mode(0, \"%s\")\n", DisplayModeName(DSP()->CurMode()));
    fprintf(fp, "pfx = \"mkscr_\"\n");
    CDgenHierDn_s gen(sdesc, depth);
    CDs *sd;
    while ((sd = gen.next()) != 0) {
        if (sd->owner())
            sd = sd->owner();
        if (sd->isDevice() && sd->isLibrary())
            continue;
        fprintf(fp, "TouchCell(pfx + \"%s\", TRUE)\n",
            Tstring(sd->cellname()));
        fprintf(fp, "ClearCell(FALSE, 0)\n");
        if (!sd->writeScript(fp, "pfx")) {
            fclose(fp);
            delete [] fname;
            Log()->PopUpErr(Errs()->get_error());
            PL()->ShowPrompt("Script creation failed.");
            return;
        }
    }
    fprintf(fp, "Window(0, 0, -1, 0)\n");

    fclose(fp);
    PL()->ShowPromptV("New script saved in %s.", fname);
    delete [] fname;
}


void
bangcmds::ldshared(const char *s)
{
    typedef void(*initfunc)(const char*);
    typedef void(*uninitfunc)();

    char *path = lstring::getqtok(&s);
    if (!path) {
        PL()->ShowPrompt("Usage: ldshared /path/to/shared/lib [args...]");
        return;
    }
#ifdef WIN32
    // Windows code.

    // Use a full path.
    if (!lstring::is_rooted(path)) {
        const char *p = path;
        path = pathlist::mk_path(".", p);
        delete [] p;
        p = path;
        path = pathlist::expand_path(p, true, true);
        delete [] p;
    }
    // Need a table for open handles.
    static SymTab *winldtab;

    bool uicalled = false;
    HINSTANCE handle = 0;
    if (!winldtab)
        winldtab = new SymTab(true, false);
    else {
        handle = (HINSTANCE)SymTab::get(winldtab, path);
        if (handle == (HINSTANCE)ST_NIL)
            handle = 0;
        else {
            uninitfunc uninit = (uninitfunc)GetProcAddress(handle, "uninit");
            if (uninit) {
                (*uninit)();
                uicalled = true;
            }
            FreeLibrary(handle);
            winldtab->remove(path);
        }
    }

    handle = LoadLibrary(path);
    if ((unsigned long)handle <= HINSTANCE_ERROR) {
        int err = GetLastError();
        PL()->ShowPromptV("LoadLibrary failed, error code %d", err);
        delete [] path;
        return;
    }
    initfunc init = (initfunc)GetProcAddress(handle, "init");
    if (!init) {
        FreeLibrary(handle);
        PL()->ShowPrompt("Could not find init function in library.");
        delete [] path;
        return;  
    }
    winldtab->add(path, handle, false);

#else
    if (!lstring::strdirsep(path)) {
        // The path needs a directory separator or dlopen will not
        // search the cwd.

        char *xpath = path;
        path = pathlist::mk_path(".", path);
        delete [] xpath;
    }
    // See if the library is loaded already, if so unload.
    bool uicalled = false;
    void *handle = dlopen(path, RTLD_LAZY | RTLD_NOLOAD);
    if (handle) {
        uninitfunc uninit = (uninitfunc)dlsym(handle, "uninit");
        if (uninit) {
            (*uninit)();
            uicalled = true;
        }
        dlclose(handle);
    }

    handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {   
        PL()->ShowPromptV("dlopen failed: %s", dlerror());
        delete [] path;
        return;
    }
    delete [] path;

    initfunc init = (initfunc)dlsym(handle, "init");
    if (!init) {
        PL()->ShowPromptV("dlsym failed: %s\n", dlerror());
        return;  
    }
#endif
    (*init)(s);

    // If the function pointers change, then existing parse trees that
    // call these functions will have a bad pointer and will probably
    // fault if run.  Call Rehash to rebuild the script libraries and
    // functions found in the search path.

    if (uicalled)
        XM()->Rehash();

    PL()->ShowPrompt("Shared object loaded and initialized.");
}


//-----------------------------------------------------------------------------
// Selections
//

void
bangcmds::select(const char *s)
{
    PL()->ErasePrompt();
    if (DSP()->CurCellName()) {
        EV()->InitCallback();
        Selections.parseSelections(CurCell(), s, true);
        XM()->ShowParameters();
    }
}


void
bangcmds::desel(const char *s)
{
    PL()->ErasePrompt();
    if (DSP()->CurCellName()) {
        EV()->InitCallback();
        Selections.parseSelections(CurCell(), s, false);
        XM()->ShowParameters();
    }
}


void
bangcmds::zs(const char*)
{
    if (EV()->CurrentWin()) {
        BBox BB;
        if (!Selections.computeBB(CurCell(), &BB, false)) {
            PL()->ShowPrompt("No selections!");
            return;
        }
        EV()->CurrentWin()->CenterFullView(&BB);
        XM()->ShowParameters();
        EV()->CurrentWin()->Redisplay(0);
    }
    PL()->ErasePrompt();
}


//-----------------------------------------------------------------------------
// Shell
//

void
bangcmds::shell(const char *cmd)
{
    PL()->ErasePrompt();
    char *shellpath = get_shell();
    if (!cmd || !*cmd) {
        miscutil::fork_terminal(shellpath);
        delete [] shellpath;
        return;
    }

    char *shell = lstring::strip_path(shellpath);
    char tbuf[64];
    strcpy(tbuf, shell);
    shell = tbuf;
    char *tdot = strrchr(shell, '.');
    if (tdot)
        *tdot = 0;

    int shtype = 0;
    if (!strcmp(shell, "csh") || !strcmp(shell, "tcsh"))
        shtype = 1;
#ifdef WIN32
    else if (!strcmp(shell, "cmd"))
        shtype = 2;
#endif

    if (shtype == 0 || shtype == 1) {
        // sh or csh
        char *tf = filestat::make_temp("xx");
        FILE *fp = fopen(tf, "wb");
        if (fp) {
            filestat::queue_deletion(tf);
            fprintf(fp, shtype == 0 ?
                "#! %s\n%s\necho press Enter to exit\nread xx\nrm -f %s" :
                "#! %s\n%s\necho press Enter to exit\n$<\nrm -f %s",
                shellpath, cmd, tf);
            fclose(fp);
            char *t = new char[strlen(shellpath) + strlen(tf) + 2];
            sprintf(t, "%s %s", shellpath, tf);
            miscutil::fork_terminal(t);
            delete [] t;
        }
        delete [] tf;
    }
#ifdef WIN32
    else {
        // DOS box (cmd.exe)
        char *tf = filestat::make_temp("xx");
        char *tt = new char[strlen(tf) + 5];
        strcpy(tt, tf);
        strcat(tt, ".bat");
        delete [] tf;
        tf = tt;
        FILE *fp = fopen(tf, "wb");
        if (fp) {
            filestat::queue_deletion(tf);
            tt = tf;
            while (*tt) {
                if (*tt == '/')
                    *tt = '\\';
                tt++;
            }
            fprintf(fp, "echo off\n%s\npause\ndel %s", cmd, tf);
            fclose(fp);
            miscutil::fork_terminal(tf);
        }
        delete [] tf;
    }
#endif
    delete [] shellpath;
}


#define TIMEOUT_SECS 30
#ifdef __linux
#define sig_t sighandler_t
#endif

namespace {
    // Struct passted to idle proc.
    //
    struct ssh_pill_t
    {
        ssh_pill_t(time_t t0, int s)
            {
                start = t0;
                ioskt = -1;
                comskt = s;
                idle_id = 0;
            }

        ~ssh_pill_t()
            {
                reset();
            }

        void reset()
            {
                if (ioskt > 0) {
                    close(ioskt);
                    ioskt = -1;
                }
                if (comskt > 0) {
                    close(comskt);
                    comskt = -1;
                }
                idle_id = 0;
            }

        time_t start;
        int ioskt;
        int comskt;
        int idle_id;
    };


    /*** Unused presently.
    // Handle shell termination, this will shut down the idle proc if
    // active, e.g., user immediately closed the window before giving
    // a password.
    //
    void ssh_child_proc(int, int, void *arg)
    {
        ssh_pill_t *p = (ssh_pill_t*)arg;
        if (p->idle_id) {
            dspPkgIf()->RemoveIdleProc(p->idle_id);
            p->reset();
        }
        delete p;
    }
    ***/


    // Idle proc, check for timeout, data availability.  If the
    // DISPLAY name is read, set the SpiceHostDisplay variable.
    //
    int ssh_idle_proc(void *arg)
    {
        const char *emsg = "Could not obtain DISPLAY from remote host.\n%s";
        ssh_pill_t *p = (ssh_pill_t*)arg;
        if (!p->idle_id)
            return (0);
        if (::time(0) - p->start > TIMEOUT_SECS) {
            Log()->WarningLogV("!ssh", emsg, "Connection timed out.");
            p->reset();
            return (0);
        }

        fd_set ready;
        timeval to;
        to.tv_sec = 0;
        to.tv_usec = 0;
        if (p->ioskt < 0) {
            FD_ZERO(&ready);
            FD_SET(p->comskt, &ready);
            int i = select(p->comskt+1, &ready, 0, 0, &to);
            if (i < 0) {
                if (errno == EINTR)
                    return (1);
                Errs()->sys_error("select");
                Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
                p->reset();
                return (0);
            }
            if (i && FD_ISSET(p->comskt, &ready)) {
                sockaddr_in from;
                socklen_t len = sizeof(sockaddr_in);
                p->ioskt = accept(p->comskt, (sockaddr*)&from, &len);
                if (p->ioskt < 0) {
                    if (errno == EINTR)
                        return (1);
                    Errs()->sys_error("accept");
                    Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
                    p->reset();
                    return (0);
                }
            }
            return (1);
        }

        FD_ZERO(&ready);
        FD_SET(p->ioskt, &ready);
        int i = select(p->ioskt+1, &ready, 0, 0, &to);
        if (i < 0) {
            if (errno == EINTR)
                return (1);
            Errs()->sys_error("select");
            Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
            p->reset();
            return (0);
        }
        if (i && FD_ISSET(p->ioskt, &ready)) {
            // Read the DISPLAY value.
            char buf[256];
            i = recv(p->ioskt, buf, 256, 0);
            if (i < 0) {
                if (errno == EINTR)
                    return (1);
                Errs()->sys_error("recv");
                Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
                p->reset();
                return (0);
            }
            buf[i] = 0;
            // Seems to arrive with trailing white space, strip it.
            char *t = buf + strlen(buf) - 1;
            while (t >= buf && isspace(*t))
                *t-- = 0;
            if (*buf) {
                CDvdb()->setVariable(VA_SpiceHostDisplay, buf);
                PL()->ShowPromptV(
                    "SpiceHostDisplay variable set to \"%s\".", buf);
            }
            p->reset();
            return (0);
        }
        return (1);
    }
}


void
bangcmds::ssh(const char *s)
{
#ifdef WIN32
    char *ssh_path = find_cygwin_path("ssh.exe");
    if (ssh_path) {
        char *e = strrchr(ssh_path, '.');
        if (e && lstring::cieq(e, ".exe"))
            *e = 0;
    }
    else {
        Log()->ErrorLogV("!ssh",
            "Error: can't find ssh.  Set CYGWIN_BIN in environment to\n"
            "Windows path to Cygwin bin and/or install ssh.");
        PL()->ErasePrompt();
        return;
    }
    GCarray<char*> gc_ssh_path(ssh_path);
#else
    const char *ssh_path = "ssh";
#endif
    while (isspace(*s))
        s++;
    char *host = 0;
    if (*s)
        host = lstring::copy(s);
    else {
        const char *in = PL()->EditPrompt(
            "Enter any ssh arguments and host name: ", 0);
        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        in = lstring::strip_space(in);
        if (!*in) {
            PL()->ErasePrompt();
            return;
        }
        host = lstring::copy(in);
    }

    // Create a socket on a temporary port, and start listening.  If
    // all goes well, we'll get the display name back from the remote
    // host.
    //
    const char *emsg = "Could not obtain DISPLAY from remote host.\n%s";
    int port = 0;
    int skt = 0;
    char hostname[256];
    if (gethostname(hostname, 256) == 0) {

// Comment this to disable the automatic callback to set the
// SpiceHostDisplay variable.
#define SSH_CALLBACK

#ifdef SSH_CALLBACK
        skt = socket(AF_INET, SOCK_STREAM, 0);
        if (skt > 0) {
            sockaddr_in sin;
            sin.sin_port = 0;
            sin.sin_family = AF_INET;
            sin.sin_addr.s_addr = INADDR_ANY;
            if (bind(skt, (sockaddr*)&sin, sizeof(sockaddr_in)) == 0) {
                socklen_t len = sizeof(sockaddr_in);
                if (getsockname(skt, (struct sockaddr*)&sin, &len) == 0) {
                    port = ntohs(sin.sin_port);
                    listen(skt, 5);
                }
                else {
                    Errs()->sys_error("getsockname");
                    Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
                }
            }
            else {
                Errs()->sys_error("bind");
                Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
            }
        }
        else {
            Errs()->sys_error("socket");
            Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
        }
#endif
    }
    else {
        Errs()->sys_error("gethostname");
        Log()->WarningLogV("!ssh", emsg, Errs()->get_error());
    }

    char *cmd = new char[strlen(ssh_path) + 1024];
    if (port > 0) {
        // For ssh:
        // -X  Turns on X forwarding.
        // -Y  Turns on trusted Y forwarding.
        // -R  Sets up reverse forwarding of our transient port for DISPLAY
        //     name transfer.
        // -t  Forces allocation of a pseudo-terminal, required for bash
        //     to have job control.
        /******
        // Note the /dev/tcp/host/port construct, this is a bash feature,
        // /dev/tcp is not a real device.
        sprintf(cmd, "%s -Y -R %d:%s:%d -t %s "
            "'echo \"$DISPLAY\" > /dev/tcp/localhost/%d; bash;'",
            ssh_path, port, hostname, port, host, port);
        *******/
        // Here's a better one: uses any shell user has set.
        // It seems that only Cygwin really needs -Y, -X appears to work
        // fine for Linux/FreeBSD/OS X.  RHEL3 ssh doesn't have -Y, no
        // good way to test for lack of -Y in ssh.
#ifdef WIN32
        sprintf(cmd, "%s -Y -R %d:%s:%d -t %s "
            "'echo \"$DISPLAY\" | nc -w 1 localhost %d; $SHELL;'",
            ssh_path, port, hostname, port, host, port);
#else
        sprintf(cmd, "%s -X -R %d:%s:%d -t %s "
            "'echo \"$DISPLAY\" | nc -w 1 localhost %d; $SHELL;'",
            ssh_path, port, hostname, port, host, port);
#endif
    }
    else
        sprintf(cmd, "%s -Y %s", ssh_path, host);
    delete [] host;

    // Now fork a terminal with the ssh shell.  The user can enter a
    // password if necessary.  The shell must remain active while the
    // DISPLAY is being used for remote WRspice.
    //
    int pid = miscutil::fork_terminal(cmd);
    delete [] cmd;
    if (pid <= 0) {
        PL()->ShowPrompt("Error: fork terminal failed!");
        return;
    }
    PL()->ErasePrompt();

    // Now set up the idle proc to read the return from the remote
    // host.  This will timeout if it takes too long, and if
    // successful will set the SpiceHostDisplay variable.

    if (port > 0) {
        ssh_pill_t *p = new ssh_pill_t(::time(0), skt);
        int id = dspPkgIf()->RegisterIdleProc(&ssh_idle_proc, p);
        p->idle_id = id;

        // We can't do this with gnome-terminal and possibly others. 
        // The pid process apparently sub-forks and exits immediately,
        // calling the handler prematurely and breaking everything. 
        // The time-out should provide adequate clean-up.
        //
        // Proc()->RegisterChildHandler(pid, &ssh_child_proc, p);
    }
    else
        PL()->ShowPrompt("Can't get remote DISPLAY, SpiceHostDisplay not set.");
}


//-----------------------------------------------------------------------------
// Technology File
//

void
bangcmds::attrvars(const char *s)
{
    if (!s || !*s)
        s = "attrvars.txt";
    FILE *fp = fopen(s, "w");
    if (!fp) {
        PL()->ShowPromptV("Error, can't open %s for writing.", s);
        return;
    }
    Tech()->PrintAttrVars(fp);
    PL()->ShowPromptV("Tech attribute variable names listed in %s.", s);
    fclose(fp);
}


void
bangcmds::dumpcds(const char *s)
{
    cTechCdsOut out;
    if (!out.write_tech(s)) {
        PL()->ShowPrompt("Error writing Cadence ASCII technology file.");
        Log()->PopUpErr(Errs()->get_error());
        return;
    }
    if (!out.write_drf(s)) {
        PL()->ShowPrompt("Error writing Cadence DRF file.");
        Log()->PopUpErr(Errs()->get_error());
        return;
    }
    if (!out.write_lmap(s)) {
        PL()->ShowPrompt("Error writing Cadence GDSII layer map file.");
        Log()->PopUpErr(Errs()->get_error());
        return;
    }
    PL()->ShowPrompt(
        "Cadence tech, DRF, and GDSII map files written, no errors.");
}


//-----------------------------------------------------------------------------
// Update Release
//

// Name of update script file, found in library path.
#define DST_SCRIPT  "wr_install"

#ifdef HAVE_MOZY

namespace {
    // Show progress when downloading.
    //
    bool http_puts(void*, const char *msg)
    {
        dspPkgIf()->CheckForInterrupt();
        if (XM()->ConfirmAbort("Interrupted.  Abort download? "))
            return (true);
        if (!msg)
            return (false);
        const char *m = msg;
        while (isspace(*m))
            m++;
        if (!*m)
            return (false);
        PL()->ShowPrompt(msg);
        return (false);
    }


#ifdef WIN32
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
#endif
}

#endif


void
bangcmds::update(const char *s)
{
#ifdef HAVE_MOZY
    if (!XM()->UpdateIf()) {
        PL()->ShowPrompt("Command not available.");
        return;
    }
    UpdIf udif(*XM()->UpdateIf());
    if (!udif.username() || !udif.password()) {
        PL()->ShowPrompt("Use !passwd to create your .wrpasswd file.");
        return;
    }

    bool os_given = false;
    bool force_dl = false;
    bool no_dl_existing = false;
    char *osname = 0;
    char *prefix = 0;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (!strcmp(tok, "-f")) {
            delete [] tok;
            force_dl = true;
            continue;
        }
        if (!strcmp(tok, "-fi")) {
            // Undocumented - this will skip the download if the file
            // exists.
            delete [] tok;
            force_dl = true;
            no_dl_existing = true;
            continue;
        }
        if (!strcmp(tok, "-p")) {
            prefix = lstring::getqtok(&s);
            if (prefix && lstring::is_rooted(prefix)) {
                delete [] tok;
                continue;
            }
            PL()->ShowPrompt("Bad or missing prefix.");
        }
        else if (!strcmp(tok, "-o")) {
            osname = lstring::gettok(&s);
            if (osname && *osname != '-') {
                delete [] tok;
                os_given = true;
                continue;
            }
            PL()->ShowPrompt("Bad or missing osname.");
        }
        else
            PL()->ShowPrompt(
                "Syntax error.  Usage: !update [-f] [-o osname] [-p prefix]");
        delete [] tok;
        delete [] osname;
        delete [] prefix;
        return;
    }

    if (!osname)
        osname = lstring::copy(XM()->OSname());
    if (!prefix)
        prefix = lstring::copy(XM()->Prefix());

    GCarray<char*> gc_osname(osname);
    GCarray<char*> gc_prefix(prefix);

    release_t my_rel = udif.my_version();
    if (my_rel == release_t(0)) {
        PL()->ShowPrompt("Internal error: I can't find my version numbers!");
        return;
    }

    char *arch;
    char *suffix;
    char *subdir;
    char *errmsg;
    release_t new_rel = udif.distrib_version(osname, &arch, &suffix, &subdir,
        &errmsg);
    if (new_rel == release_t(0)) {
        PL()->ShowPromptV(
            "Failed to obtain current release number from server: %s.",
            errmsg);
        delete [] errmsg;
        return;
    }

    GCarray<char*> gc_arch(arch);
    GCarray<char*> gc_suffix(suffix);
    GCarray<char*> gc_subdir(subdir);

    if (!force_dl) {
        if (!os_given && !(my_rel < new_rel)) {
            PL()->ShowPromptV("You are running the current release of %s.",
                XM()->Product());
            return;
        }
    }

    // destination directory
    const char *tmpdir = getenv("XIC_TMP_DIR");
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
        sprintf(buf, "Download %s distribution file? ",
            os_given ? dst_file : new_rel_str);
        delete [] new_rel_str;

        const char *in = PL()->EditPrompt(buf, "n");
        in = lstring::strip_space(in);
        if (!in || (*in != 'y' && *in != 'Y')) {
            PL()->ErasePrompt();
            return;
        }

        dspPkgIf()->SetWorking(true);
        char *my_dst_file = udif.download(dst_file, osname, subdir, &errmsg,
            http_puts);
        dspPkgIf()->SetWorking(false);
        if (!my_dst_file) {
            PL()->ShowPromptV("Download failed: %s.", errmsg);
            delete [] errmsg;
            return;
        }
        delete [] my_dst_file;  // same as dst_path
    }

    sprintf(buf, "Install %s? ", dst_path);
    const char *in = PL()->EditPrompt(buf, "n");
    in = lstring::strip_space(in);
    if (!in || (*in != 'y' && *in != 'Y')) {
        PL()->ErasePrompt();
        return;
    }

#ifdef WIN32
    if (!atexit(msw_install)) {
        msw_exepath = lstring::copy(dst_path);
        PL()->ShowPrompt("Update will be installed on program exit.");
    }
    else {
        PL()->ShowPrompt(
            "Error: unknown error scheduling exit process.");
    }

#else
    const char *cmdfmt = CDvdb()->getVariable(VA_InstallCmdFormat);

    const char *libpath = CDvdb()->getVariable(VA_LibPath);
    char *scriptfile;
    FILE *fp = (pathlist::open_path_file(DST_SCRIPT, libpath, "r",
        &scriptfile, true));
    GCarray<char*> gc_scriptfile(scriptfile);
    if (fp)
        fclose(fp);
    else {
        PL()->ShowPrompt("Can not open update script file.");
        return;
    }

    // This will likely fail if there is an osname mismatch to the
    // running operating system.
    // Also likely to fail is the user isn't root.

    int ret = udif.install(scriptfile, cmdfmt, dst_path, prefix, osname,
        &errmsg);
    if (ret != 0) {
        if (errmsg) {
            PL()->ShowPromptV("Error: %s", errmsg);
            delete [] errmsg;
        }
        else
            PL()->ShowPromptV(
                "Warning: install process returned error code %d.", ret);
        return;
    }

    // Warning:  a 0 return doesn't necessarily mean that the install
    // was successful - just that there were no config errors and the
    // terminal popped up.

    PL()->ShowPrompt(
        "Done.  If install succeeded, restart the program to run new release.");
#endif

#else
    (void)s;
    PL()->ShowPrompt("Command not available.");
#endif  // HAVE_MOZY
}


void
bangcmds::passwd(const char*)
{
#ifdef HAVE_MOZY
    if (!XM()->UpdateIf()) {
        PL()->ShowPrompt("Command not available.");
        return;
    }
    char *in = PL()->EditPrompt("Enter user name: ", 0);
    if (!in) {
        PL()->ErasePrompt();
        return;
    }
    char *user = lstring::gettok(&in);
    if (!user) {
        PL()->ErasePrompt();
        return;
    }
    GCarray<char*> gc_user(user);

    in = PL()->EditPrompt("Enter password: ", 0, PLedStart, PLedNormal, true);
    if (!in) {
        PL()->ErasePrompt();
        return;
    }
    char *pw1 = lstring::gettok(&in);
    if (!pw1) {
        PL()->ErasePrompt();
        return;
    }
    GCarray<char*> gc_pw1(pw1);

    in = PL()->EditPrompt("Reenter password: ", 0, PLedStart, PLedNormal, true);
    if (!in) {
        PL()->ErasePrompt();
        return;
    }
    char *pw2 = lstring::gettok(&in);
    if (!pw2) {
        PL()->ErasePrompt();
        return;
    }
    GCarray<char*> gc_pw2(pw2);

    if (strcmp(pw1, pw2)) {
        PL()->ShowPrompt("Try again, you mistyped the password.");
        return;
    }

    UpdIf udif(*XM()->UpdateIf());
    const char *err = udif.update_pwfile(user, pw1);
    if (err) {
        PL()->ShowPromptV("Error: %s", err);
        return;
    }
    PL()->ShowPrompt(
        "The .wrpasswd file in your home directory was updated successfully.");

#else
    PL()->ShowPrompt("Command not available.");
#endif  // HAVE_MOZY
}


void
bangcmds::proxy(const char *s)
{
#ifdef HAVE_MOZY
    char *addr = lstring::gettok(&s);
    char *port = lstring::gettok(&s);

    if (!addr) {
        char *in = PL()->EditPrompt("Enter proxy internet address: ", 0);
        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        addr = lstring::gettok(&in);
        if (!addr) {
            PL()->ErasePrompt();
            return;
        }
    }
    GCarray<char*> gc_addr(addr);

    if (*addr == '-' || *addr == '+') {
        if (!UpdIf::move_proxy(addr))
            PL()->ShowPromptV("Operation failed: %s.", filestat::error_msg());
        else {
            const char *a = addr;
            int c = *a++;
            if (!*a)
                a = "bak";
            if (c == '-') {
                PL()->ShowPromptV(
                    "Move .wrproxy file to .wrproxy.%s succeeded.", a);
            }
            else {
                PL()->ShowPromptV(
                    "Move .wrproxy.%s to .wrproxy file succeeded.", a);
            }
        }
        return;
    }
    if (!lstring::prefix("http:", addr)) {
        PL()->ShowPrompt("Error: \"http:\" prefix required in address.");
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

    if (!a_has_port && !port) {
        const char *in = PL()->EditPrompt("Enter proxy port number: ", 0);
        if (!in) {
            PL()->ErasePrompt();
            return;
        }
        port = lstring::gettok(&in);
    }
    GCarray<char*> gc_port(port);
    if (port) {
        for (const char *c = port; *c; c++) {
            if (!isdigit(*c)) {
                PL()->ShowPrompt("Error: port is not numeric.");
                return;
            }
        }
    }

    const char *err = UpdIf::set_proxy(addr, port);
    if (err)
        PL()->ShowPromptV("Operation failed: %s.", err);
    else if (port)
        PL()->ShowPromptV("Created .wrproxy file for %s:%s", addr, port);
    else
        PL()->ShowPromptV("Created .wrproxy file for %s", addr);

#else
    (void)s;
    PL()->ShowPrompt("Command not available.");
#endif  // HAVE_MOZY
}


//-----------------------------------------------------------------------------
// Variables
//

// Parse the "!set" command line.  Only one variable can be set per
// line.  The first token is the name, the rest of the line is the
// string the variable is set to.  If there is no name, print the
// currently set variables.  If the name is "?", print the variables
// which can be set.
//
void
bangcmds::set(const char *s)
{
    PL()->ErasePrompt();
    if (!s || !*s) {
        // show the variables currently set
        XM()->PopUpVariables(true);
        return;
    }
    if (*s == '?' && (!*(s+1) || isspace(*(s+1)))) {
        // list the known variables
        DSPmainWbag(PopUpHelp(HELP_VAR_LIST))
        return;
    }
    char buf[128];
    char *t = buf;
    while (*s && !isspace(*s))
        *t++ = *s++;
    *t = '\0';
    CDvdb()->setVariable(buf, s);
}


void
bangcmds::unset(const char *s)
{
    if (*s) {
        char *t = lstring::gettok(&s);
        if (t)
            CDvdb()->clearVariable(t);
        delete [] t;
    }
    PL()->ErasePrompt();
}


void
bangcmds::setdump(const char *s)
{
    char *fname = lstring::gettok(&s);
    stringlist *sl0 = CDvdb()->listVariables();

    FILE *fp;
    if (fname) {
        fp = fopen(fname, "w");
        if (!fp) {
            PL()->ShowPromptV("Error: can't open %s for writing.", fname);
            delete [] fname;
            return;
        }
        delete [] fname;
    }
    else
        fp = stdout;

    for (stringlist *sl = sl0; sl; sl = sl->next) {
        char *t = sl->string;
        char *name = lstring::gettok(&t);
        fprintf(fp, "Set(\"%s\", \"", name);
        fputs(t, fp);
        fputs("\")\n", fp);
        delete [] name;
    }
    if (fp != stdout)
        fclose(fp);
    PL()->ErasePrompt();
}

