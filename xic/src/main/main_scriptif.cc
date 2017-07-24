
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: main_scriptif.cc,v 5.68 2017/03/17 04:35:00 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "editif.h"
#include "scedif.h"
#include "extif.h"
#include "fio.h"
#include "pcell.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "si_macro.h"
#include "main_scriptif.h"
#include "keymacro.h"
#include "layertab.h"
#include "promptline.h"
#include "errorlog.h"
#include "tech.h"
#include "select.h"
#include "ghost.h"
#include "timer.h"
#include "filestat.h"
#include "pathlist.h"
#include "reltag.h"

#include <algorithm>
#include <sys/types.h>
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#ifndef direct
#define direct dirent
#endif
#else
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#endif


//-----------------------------------------------------------------------------
// Implement the application-specific callbacks.

namespace {
    // Return the current display mode.
    //
    DisplayMode
    get_cur_mode()
    {
        return (DSP()->CurMode());
    }

    // Return the current layer.
    //
    CDl *
    get_cur_layer()
    {
        return (LT()->CurLayer());
    }

    // Return the current cell, used to determine the effective
    // field size for geometric operations if not otherwise known.
    //
    CDs *
    get_cur_phys_cell()
    {
        return (CDcdb()->findCell(DSP()->CurCellName(), Physical));
    }

    // Return the name of the current cell.
    //
    const char *
    get_cur_cell_name()
    {
        return (Tstring(DSP()->CurCellName()));
    }

    // Resolve the cell desc from the passed GroupDesc, which is an opaque
    // type within the interpreter core.  If extraction is not used, this
    // can return null (it won't be called anyway).
    //
    CDs *
    get_cell_from_group_desc(cGroupDesc *g)
    {
        return (ExtIf()->cellDesc(g));
    }

    // Print a progress message emitted from the evaluator.
    //
    void
    send_message(const char *fmt, va_list args)
    {
        char buf[256];
        vsnprintf(buf, 256, fmt, args);
        PL()->ShowPrompt(buf);
    }

    unsigned long check_time;

    // Return true if the current operation should be aborted,
    // called periodically.
    //
    bool
    check_interrupt(const char *msg)
    {
        if (Timer()->check_interval(check_time)) {
            if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
                dspPkgIf()->CheckForInterrupt();
            return (XM()->ConfirmAbort(msg));
        }
        return (false);
    }

    GRlistPopup *funcList;

    void
    fl_cb(const char *item, void*)
    {
        if (item == 0)
            funcList = 0;
    }

    // Pop up or update a list of functions in memory.  If the argument is
    // null, an existing pop-up will be updated.  The list must be passed
    // to pop-up the window initially.
    //
    void
    show_function_list(stringlist *sl)
    {
        if (funcList) {
            if (sl)
                funcList->update(sl, "Functions in memory", 0);
            else {
                sl = SI()->GetSubfuncList();
                funcList->update(sl, "Functions in memory", 0);
                stringlist::destroy(sl);
            }
        }
        else if (sl) {
            funcList = DSP()->MainWdesc()->Wbag()->PopUpList(sl,
                "Functions in memory", 0, fl_cb, 0, false, false);
            if (funcList)
                funcList->register_usrptr((void**)&funcList);
        }
    }

    // Parse the "#macro" direective.
    //
    void
    macro_parse(SIfile *sfp, stringlist **wl, const char **line, int *lcnt)
    {
        KbMac()->MacroParse(sfp, wl, line, lcnt);
    }

    // Create a Zlist from the selection queue, clipped to zref.
    //
    Zlist *
    get_sq_zlist(const CDl *ld, const Zlist *zref)
    {
        return (Selections.getZlist(CurCell(Physical), ld, zref));
    }

    // Function to set up predefined macros.  These are applied when
    // SI()->NewMacroHandler is called.
    //
    void
    macro_predefs(MacroHandler *mh)
    {
        char buf[256];
        sprintf(buf, "RELEASE %d", XIC_RELEASE_NUM);
        mh->parse_macro(buf, true);

        // XIC_RELEASE_NUM is gxyy0
        int gen = XIC_RELEASE_NUM/10000;
        int n = XIC_RELEASE_NUM - gen*10000;
        int major = n/1000;
        n -= major*1000;
        n /= 10;
        int minor = n;

        sprintf(buf, "GENERATION %d", gen);
        mh->parse_macro(buf, true);
        sprintf(buf, "MAJOR %d", major);
        mh->parse_macro(buf, true);
        sprintf(buf, "MINOR %d", minor);
        mh->parse_macro(buf, true);

#ifdef WIN32
        sprintf(buf, "OSTYPE \"%s\"", "Windows");
#else
#ifdef __linux
        sprintf(buf, "OSTYPE \"%s\"", "Linux");
#else
#ifdef __APPLE__
        sprintf(buf, "OSTYPE \"%s\"", "OSX");
#else
#ifdef __FreeBSD__
        sprintf(buf, "OSTYPE \"%s\"", "UNIX");
#else
        sprintf(buf, "OSTYPE \"%s\"", "UNKNOWN");
#endif
#endif
#endif
#endif
        mh->parse_macro(buf, true);

        sprintf(buf, "OSNAME \"%s\"", XM()->OSname());
        mh->parse_macro(buf, true);

        if (sizeof(void*) == 8)
            sprintf(buf, "OSBITS %d", 64);
        else
            sprintf(buf, "OSBITS %d", 32);
        mh->parse_macro(buf, true);

        sprintf(buf, "XTROOT \"%s/%s\"", XM()->Prefix(), XM()->ToolsRoot());
        mh->parse_macro(buf, true);

        sprintf(buf, "PROGROOT \"%s\"", XM()->ProgramRoot());
        mh->parse_macro(buf, true);

        const char *progname = XM()->Product();
        if (progname && *progname)
            mh->parse_macro(progname, true, true);   // No substitution.

        if (ExtIf()->hasExtract()) {
            sprintf(buf, "FEATURESET \"%s\"", "FULL");
            mh->parse_macro(buf, true);
        }
        else if (EditIf()->hasEdit()) {
            sprintf(buf, "FEATURESET \"%s\"", "EDITOR");
            mh->parse_macro(buf, true);
        }
        else {
            sprintf(buf, "FEATURESET \"%s\"", "VIEWER");
            mh->parse_macro(buf, true);
        }
        const char *techname = Tech()->TechnologyName();
        if (techname && *techname) 
            mh->parse_macro(techname, true, true);   // No subdtitution.
    }
}


// Load the script libraries, called from constructor.
//
void
cMain::LoadScriptFuncs()
{
    // setup callbacks
    SIparse()->RegisterIfGetCurMode(get_cur_mode);
    SIparse()->RegisterIfGetCurLayer(get_cur_layer);
    SIparse()->RegisterIfGetCurPhysCell(get_cur_phys_cell);
    SIparse()->RegisterIfGetCurCellName(get_cur_cell_name);
    SIparse()->RegisterIfGetCellFromGroupDesc(get_cell_from_group_desc);
    SIparse()->RegisterIfSendMessage(send_message);
    SIparse()->RegisterIfCheckInterrupt(check_interrupt);
    SIparse()->RegisterIfShowFunctionList(show_function_list);
    SIparse()->RegisterIfMacroParse(macro_parse);
    SIparse()->RegisterIfGetSqZlist(get_sq_zlist);

    // setup local context
    SImacroHandler::register_predef_func(&macro_predefs);
    SI()->RegisterLocalContext(new SIlocalContext);

    // load script libraries
    load_funcs_misc1();
    load_funcs_misc2();
    load_funcs_misc3();
}


//-----------------------------------------------------------------------------
// Implement the application-specific execution context.  This is used by
// the application's script function libraries.

SIlocalContext *SIlocalContext::instancePtr;

// initial print max counts
#define DEF_MAX_ARRAY_PTS   100
#define DEF_MAX_ZOIDS       20

SIlocalContext::SIlocalContext()
{
    if (instancePtr) {
        fprintf(stderr,
            "Singleton class SIlocalContext already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    lc_pushvarTab       = 0;

    lc_ghostList        = 0;
    lc_ghostCount       = 0;
    lc_curGhost         = 0;
    lc_lastX            = 0;
    lc_lastY            = 0;

    lc_indentLevel      = 0;
    lc_maxArrayPts      = DEF_MAX_ARRAY_PTS;
    lc_maxZoids         = DEF_MAX_ZOIDS;

    lc_transformX       = 0;
    lc_transformY       = 0;
    lc_applyTransform   = false;

    lc_noRedisplayBak   = false;
    lc_frozen           = false;
    lc_doingPCell       = false;
}


// Private static error exit.
//
void
SIlocalContext::on_null_ptr()
{
    fprintf(stderr,
        "Singleton class SIlocalContext used before instantiated.\n");
    exit(1);
}


void
SIlocalContext::clear()
{
    if (lc_ghostCount > 0) {
        while (lc_ghostCount--)
            Gst()->SetGhost(GFnone);
    }

    lc_ghostCount = 0;
    PolyList::destroy(lc_ghostList);
    lc_ghostList = 0;

    lc_applyTransform = false;
    lc_indentLevel = 0;
    lc_maxArrayPts = DEF_MAX_ARRAY_PTS;
    lc_maxZoids = DEF_MAX_ZOIDS;

    if (lc_pushvarTab) {
        // Revert variables set with PushVar/PopVar.
        SymTabGen gen(lc_pushvarTab, true);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            stringlist *s0 = (stringlist*)h->stData;
            if (s0) {
                stringlist *s = s0;
                while (s->next)
                    s = s->next;
                if (s->string)
                    CDvdb()->setVariable(h->stTag, s->string);
                else
                    CDvdb()->clearVariable(h->stTag);
                stringlist::destroy(s0);
            }
            delete [] h->stTag;
            delete h;
        }
        delete lc_pushvarTab;
        lc_pushvarTab = 0;
    }

    if (lc_frozen) {
        lc_frozen = false;
        DSP()->SetNoRedisplay(lc_noRedisplayBak);
        EditIf()->ulCommitChanges();
        DSP()->RedisplayAll(Physical);
        DSP()->RedisplayAll(Electrical);
    }
    else {
        EditIf()->ulCommitChanges();
        DSP()->RedisplayAll();
    }

    PC()->revertPCinstParams();
    LT()->ShowLayerTable();
    if (DSP()->CurMode() == Electrical)
        ScedIf()->assertSymbolic(true);
}


void
SIlocalContext::setFrozen(bool freeze)
{
    if (freeze) {
        if (!lc_frozen) {
            lc_frozen = true;
            lc_noRedisplayBak = DSP()->NoRedisplay();
            DSP()->SetNoRedisplay(true);
        }
    }
    else {  
        if (lc_frozen) { 
            lc_frozen = false;
            DSP()->SetNoRedisplay(lc_noRedisplayBak);
            if (!DSP()->NoRedisplay()) {
                DSP()->RedisplayAll(Physical);
                DSP()->RedisplayAll(Electrical);
            }
        }
    }
}


void
SIlocalContext::pushVar(const char *name, const char *string)
{
    if (!lc_pushvarTab)
        lc_pushvarTab = new SymTab(true, false);
    stringlist *sl =
        new stringlist(lstring::copy(CDvdb()->getVariable(name)), 0);

    SymTabEnt *h = SymTab::get_ent(lc_pushvarTab, name);
    if (!h)
        lc_pushvarTab->add(lstring::copy(name), sl, false);
    else {
        sl->next = (stringlist*)h->stData;
        h->stData = sl;
    }
    if (string)
        CDvdb()->setVariable(name, string);
    else
        CDvdb()->clearVariable(name);
}


void
SIlocalContext::popVar(const char *name)
{
    if (!lc_pushvarTab)
        return;

    SymTabEnt *h = SymTab::get_ent(lc_pushvarTab, name);
    if (!h)
        return;

    stringlist *sl = (stringlist*)h->stData;
    if (!sl) {
        lc_pushvarTab->remove(name);
        return;
    }
    h->stData = sl->next;
    if (!sl->next)
        lc_pushvarTab->remove(name);

    if (sl->string)
        CDvdb()->setVariable(name, sl->string);
    else
        CDvdb()->clearVariable(name);
    sl->next = 0;
    stringlist::destroy(sl);
}
// End of SIlocalContext functions


//-----------------------------------------------------------------------------
// Files with the following name are expected to contain function
// definitions only, and reside in the script search path.  When the
// User menu is built, these files are read, and the functions will
// be saved in the symbol table.
//
#define LIBRARY "library"

// Xic has no native netlist format.  Instead, netlists are generated
// by a formatting script, zero or more of which are kept in this
// library, as function definitions.
//
#define EX_FORMAT_LIB "xic_format_lib"

// Look for this token following '#' in script files to get menu text.
//
#define MENU_LABEL "menulabel "

namespace {
    // Temp struct for sorting
    struct ut { char *n; char *r; umenu *m; };

    // Menu text fixup, remap problem characters.
    //
    void fixup(char *string)
    {
        if (string) {
            while (*string) {
                if (*string == '/')     // path separator
                    *string = '-';
                string++;
            }
        }
    }

    // alpha sort function for umenu
    //
    inline bool
    comp(const ut &a, const ut &b)
    {
        const char *s1 = a.n ? a.n : a.r;
        const char *s2 = b.n ? b.n : b.r;
        if (s1 && s2)
            return (strcmp(s1, s2) < 0);
        return (s1 < s2);
    }
}


// Static function.
// umenu sort function
//
void
umenu::sort(umenu *thisu)
{
    int cnt = 0;
    for (umenu *u = thisu; u; u = u->u_next, cnt++) ;
    if (cnt < 2)
        return;
    ut *aa = new ut[cnt];
    cnt = 0;
    for (umenu *u = thisu; u; u = u->u_next, cnt++) {
        aa[cnt].n = u->u_name;
        aa[cnt].r = u->u_realname;
        aa[cnt].m = u->u_menu;
    }
    std::sort(aa, aa + cnt, comp);
    cnt = 0;
    for (umenu *u = thisu; u; u = u->u_next, cnt++) {
        u->u_name = aa[cnt].n;
        u->u_realname = aa[cnt].r;
        u->u_menu = aa[cnt].m;
    }
    delete [] aa;
}


// return the number of entries in the menu tree
//
int
umenu::numitems() const
{
    int cnt = 0;
    for (const umenu *u = this; u; u = u->u_next) {
        cnt++;
        if (u->u_menu)
            cnt += u->u_menu->numitems();
    }
    return (cnt);
}
// End of umenu functions


//-----------------------------------------------------------------------------
// Application-specific global stuff.
//
namespace {
    namespace main_script {
        struct SIlocal
        {
            // This class lists the scripts included in the technology file,
            // or otherwise registered.  This does not include the scripts
            // from the search path.
            //
            // If path and script are both set, path has precedence.  This
            // enables redirecting techfile scripts.
            //
            struct sScript
            {
                sScript(char *n)
                    {
                        name = n;
                        script = 0;
                        path = 0;
                        next = 0;
                    }

                ~sScript()
                    {
                        delete [] name;
                        delete [] path;
                        stringlist::destroy(script);
                    }

                char *name;         // name of script
                stringlist *script; // script text if from tech file
                char *path;         // path to script file
                sScript *next;
            };

            SymTab *GetFormatFuncTab(int ix)
                {
                    return (formatFuncTab[ix]);
                }

            siVariable *GetFormatVars(int ix)
                {
                    return (formatVars[ix]);
                }

            void RegisterScript(FILE*);
            void RegisterScript(const char*, const char*);
            void DumpScripts(FILE*, char*, bool);
            umenu *AddScript(umenu*, SymTab*);
            void OpenScript(const char*, SIfile**, stringlist**, bool);
            char *FindScript(const char*);
            void OpenFormatLib(int);
            void RunExecs();

            static char *grab_script(FILE*, char*);
            static umenu *add_dir(const char*, SymTab*, umenu*);
            static umenu *get_lib(const char*);
            static char *get_lib_name(const char*);
            static char *get_libref(const char*, const char*);
            static char *get_script_label(const char*);
            static FILE *findopen(const char*, const char*, char**);

        private:
            void InsertScript(sScript*, bool);
            sScript *GetScript(const char*);

            siVariable *formatVars[EX_NUM_FORMATS]; // for netlist formatting
            SymTab *formatFuncTab[EX_NUM_FORMATS];  // saved netlist format
                                                    //  funcs
            sScript *Scripts;
            sScript *ScriptsToExec;
        };

        SIlocal SIxic;
    }
}

using namespace main_script;


// Maintain a list of scripts yanked from the technology file.  Used
// when reading the tech file, but should be operable stand-alone as
// well.
//
void
SIlocal::RegisterScript(FILE *fp)
{
    const char *inbuf = Tech()->InBuf();;
    const char *s = inbuf;
    char *tok = lstring::getqtok(&s);
    if (!tok)
        return;
    bool exec = false;
    sScript *scr = new sScript(tok);
    stringlist *end = 0;

    Tech()->BeginParse();
    while (Tech()->GetKeywordLine(fp, tNoKeyword) != 0) {
        s = inbuf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;
        if (lstring::cimatch("RunScript", s)) {
            exec = true;
            continue;
        }
        if (lstring::cimatch("EndScript", s)) {
            InsertScript(scr, exec);
            return;
        }
        if (!scr->script)
            end = scr->script = new stringlist(lstring::copy(inbuf), 0);
        else {
            end->next = new stringlist(lstring::copy(inbuf), 0);
            end = end->next;
        }
    }
    Tech()->EndParse();
}


// Add a script, with the full path to the script file, to the list.
// The name is the invoking name of the script, without the extension.
// the path is the full path including the file with extension.  If
// the path is null, remove an existing entry with a path, with a
// matching name.
//
void
SIlocal::RegisterScript(const char *name, const char *fullpath)
{
    if (fullpath && *fullpath) {
        sScript *scr = new sScript(lstring::copy(name));
        scr->path = lstring::copy(fullpath);
        InsertScript(scr, false);
    }
    else {
        sScript *sp = 0;
        for (sScript *s = Scripts; s; s = s->next) {
            if (!strcmp(s->name, name)) {
                delete s->path;
                s->path = 0;
                if (!s->script) {
                    // remove from list
                    if (!sp)
                        Scripts = s->next;
                    else
                        sp->next = s->next;
                    delete s;
                }
                break;
            }
            sp = s;
        }
    }
}


// Print the contents of the scripts from the technology file to fp,
// in the same format as used in the technology file.  If name is 0,
// all such scripts are dumped, otherwise only the named script is
// dumped.  This applies only to scripts with a commands text list,
// which is true for scripts read from the technology file.
//
void
SIlocal::DumpScripts(FILE *fp, char *name, bool head)
{
    const char *sep =
        "#-------------------------------------------------------------"
        "-----------------\n";
    const char *top =
        "# Script Definitions\n"
        "# In Xic, these appear in the User Menu.  They are still available "
        "in XicII\n"
        "# through the !exec command, for example type \"!exec xstr\" ("
        "without quotes)\n"
        "# to run the xstr script.\n";

    bool haveone = false;
    if (!name && head) {
        for (sScript *a = Scripts; a; a = a->next) {
            if (a->script) {
                haveone = true;
                break;
            }
        }
    }
    if (haveone) {
        fputs(sep, fp);
        fputs(top, fp);
        fputs("\n", fp);
    }
    for (sScript *a = Scripts; a; a = a->next) {
        if (!a->script)
            continue;
        if (!name || !strcmp(name, a->name)) {
            if (head)
                fprintf(fp, "Script %s\n", a->name);
            for (stringlist *l = a->script; l; l = l->next)
                fprintf(fp, "%s\n", l->string);
            if (head)
                fprintf(fp, "EndScript\n\n");
        }
    }
    if (haveone) {
        fputs(sep, fp);
        fputs("\n", fp);
    }
}


// Function to add the names of listed scripts to the passed word
// list.  The updated list head is returned.
//
umenu *
SIlocal::AddScript(umenu *ul, SymTab *nametab)
{
    for (sScript *a = Scripts; a; a = a->next) {
        if (SymTab::get(nametab, a->name) == ST_NIL) {
            char *s = lstring::copy(a->name);
            ul = new umenu(s, 0, ul);
            nametab->add(s, 0, false);
        }
    }
    return (ul);
}


// Return a file pointer to the file containing the named script.  The
// name can be a (directory) path to a script file, the name of a
// script to open in the script search path, or a library path to open
// through a script library.  The ".scr" suffix may or may not appear
// in the name, and similar for library names.
//
// The return is either an SIfile open for reading or a stringlist
// containing the script text.
//
void
SIlocal::OpenScript(const char *namein, SIfile **pfp, stringlist **pwl,
    bool checkcwd)
{
    *pfp = 0;
    *pwl = 0;

    char *name = pathlist::expand_path(namein, false, true);
    if (!name)
        return;
    GCarray<char*> gc_name(name);

    char *ext = strrchr(name, '.');
    if (lstring::strdirsep(name) && !lstring::prefix(SCR_LIBCODE, name)) {
        // file path, open the file
        if (!ext || !lstring::cieq(ext, SCR_SUFFIX)) {
            char *cp = new char[strlen(name) + strlen(SCR_SUFFIX) + 1];
            strcpy(cp, name);
            strcat(cp, SCR_SUFFIX);
            name = cp;
        }
        *pfp = SIfile::create(name, 0, 0);
        return;
    }
    if (ext && lstring::cieq(ext, SCR_SUFFIX))
        *ext = 0;

    // For script references, search for listed scripts with a path first.
    // These override tech file and search path scripts.
    if (!lstring::prefix(SCR_LIBCODE, name)) {
        // not a library reference, so check stored scripts
        sScript *a = GetScript(name);
        if (a) {
            if (a->path) {
                char *p = pathlist::expand_path(a->path, false, true);
                *pfp = SIfile::create(p, 0, 0);
                delete [] p;
                return;
            }
        }
    }

    // Check the search path, recursively descend libraries.
    const char *path = CDvdb()->getVariable(VA_ScriptPath);
    if (path) {
        pathgen pg(path);
        char *p;
        while ((p = pg.nextpath(false)) != 0) {
            FILE *fp = findopen(p, name, 0);
            delete [] p;
            if (fp) {
                *pfp = SIfile::create(name, fp, 0);
                return;
            }
        }
    }

    // Check for a tech file script.
    if (!lstring::prefix(SCR_LIBCODE, name)) {
        // not a library reference, so check stored scripts
        sScript *a = GetScript(name);
        if (a) {
            if (!a->path && a->script) {
                *pwl = a->script;
                return;
            }
        }
    }

    // Optionally, just look in the current directory for the file.
    if (checkcwd) {
        FILE *fp = findopen(".", name, 0);
        if (fp) {
            *pfp = SIfile::create(name, fp, 0);
            return;
        }
    }

    // Unresolved.
}


// Return the full path to the file containing the named script.  The
// name can be a (directory) path to a script file, the name of a
// script to open in the script search path, or a library path to open
// through a script library.  The ".scr" suffix may or may not appear
// in the name, and similar for library names.
//
// If no file can be opened with the given name, return 0.
//
char *
SIlocal::FindScript(const char *namein)
{
    char *name = pathlist::expand_path(namein, false, true);
    if (!name)
        return (0);
    char *ext = strrchr(name, '.');
    if (lstring::strdirsep(name) && !lstring::prefix(SCR_LIBCODE, name)) {
        // file path, open the file
        if (!ext || !lstring::cieq(ext, SCR_SUFFIX)) {
            char *cp = new char[strlen(name) + strlen(SCR_SUFFIX) + 1];
            strcpy(cp, name);
            strcat(cp, SCR_SUFFIX);
            delete [] name;
            name = cp;
        }
        if (access(name, R_OK) == 0)
            return (name);
        delete [] name;
        return (0);
    }
    if (ext && lstring::cieq(ext, SCR_SUFFIX))
        *ext = 0;

    // For script references, search for listed scripts with a path first.
    // These override tech file and search path scripts.
    if (!lstring::prefix(SCR_LIBCODE, name)) {
        // not a library reference, so check stored scripts
        sScript *a = GetScript(name);
        if (a) {
            if (a->path) {
                delete [] name;
                char *p = pathlist::expand_path(a->path, false, true);
                if (access(p, R_OK) == 0)
                    return (p);
                delete [] p;
                return (0);
            }
        }
    }

    // Check the search path, recursively descend libraries.
    const char *path = CDvdb()->getVariable(VA_ScriptPath);
    if (path) {
        pathgen pg(path);
        char *p;
        while ((p = pg.nextpath(false)) != 0) {
            char *fullpath;
            FILE *fp = findopen(p, name, &fullpath);
            delete [] p;
            if (fp) {
                fclose(fp);
                if (fullpath) {
                    delete [] name;
                    return (fullpath);
                }
                break;
            }
        }
    }

    // Tech file scripts have no path se we're done.
    delete [] name;
    return (0);
}


// Open the format library file and parse the format function
// definitions.
//
void
SIlocal::OpenFormatLib(int ix)
{
    if (ix < 0 || ix >= EX_NUM_FORMATS)
        return;
    if (formatFuncTab[ix])
        // already loaded
        return;
    const char *libpath = CDvdb()->getVariable(VA_LibPath);
    FILE *fp = pathlist::open_path_file(EX_FORMAT_LIB, libpath, "r", 0, true);
    if (!fp) {
        /*
        Log()->WarningLog(mh::Initialization,
            "Format library file \"%s\" not found.\n", fname);
        */
        return;
    }

    siVariable *ftvars = ExtIf()->createFormatVars();
    formatFuncTab[ix] = new SymTab(false, false);

    char buf[256];
    while (fgets(buf, 256, fp) != 0) {
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;
        char *tok = lstring::gettok(&s);
        char *script = 0, *name = 0;;
        if (lstring::cieq(tok, "PhysFormat")) {
            name = lstring::getqtok(&s);
            script = grab_script(fp, buf);
            if (ix != EX_PNET_FORMAT) {
                delete [] script;
                script = 0;
            }
        }
        else if (lstring::cieq(tok, "ElecFormat")) {
            name = lstring::getqtok(&s);
            script = grab_script(fp, buf);
            if (ix != EX_ENET_FORMAT) {
                delete [] script;
                script = 0;
            }
        }
        else if (lstring::cieq(tok, "LvsFormat")) {
            name = lstring::getqtok(&s);
            script = grab_script(fp, buf);
            if (ix != EX_LVS_FORMAT) {
                delete [] script;
                script = 0;
            }
        }
        else {
            delete [] tok;
            Log()->ErrorLogV("format library file",
                "Unknown keyword or junk found in %s file, aborting read.",
                EX_PNET_FORMAT);
            break;
        }
        delete [] tok;
        if (script) {
            if (!name)
                Log()->ErrorLogV("format library file",
                    "Unnamed block found in %s file, ignoring.",
                    EX_PNET_FORMAT);
            else {
                const char *stmp = script;
                SIfunc *sf = SI()->GetBlock(0, 0, &stmp, &ftvars);
                formatFuncTab[ix]->add(name, sf, false);
            }
            delete [] script;
        }
        else
            delete [] name;
    }
    formatVars[ix] = ftvars;
}


void
SIlocal::RunExecs()
{
    while (ScriptsToExec) {
        sScript *s = ScriptsToExec;
        ScriptsToExec = ScriptsToExec->next;
        SI()->Interpret(0, s->script, 0, 0);
        delete s;
        EditIf()->ulCommitChanges(true);
    }
}


// Method to add a script element to the list.  If a script of the
// same name already exists, the entry is updated, and the passed
// script is freed.
//
// If exec is true, add it to the list of scripts to execute after the
// technology file is read.
//
void
SIlocal::InsertScript(sScript *s, bool exec)
{
    if (exec) {
        if (!ScriptsToExec)
            ScriptsToExec = s;
        else {
            for (sScript *a = ScriptsToExec; a; a = a->next) {
                if (!strcmp(a->name, s->name)) {
                    // No script entries with paths here.

                    stringlist::destroy(a->script);
                    a->script = s->script;
                    s->script = 0;
                    delete s;
                    return;
                }
                if (!a->next) {
                    a->next = s;
                    s->next = 0;
                    break;
                }
            }
        }
    }
    else {
        if (!Scripts)
            Scripts = s;
        else {
            for (sScript *a = Scripts; a; a = a->next) {
                if (!strcmp(a->name, s->name)) {

                    // The path overrides the script, so update the path
                    // if present.
                    if (s->path) {
                        delete a->path;
                        a->path = s->path;
                        s->path = 0;
                    }
                    else if (!a->path) {
                        stringlist::destroy(a->script);
                        a->script = s->script;
                        s->script = 0;
                    }
                    delete s;
                    return;
                }
                if (!a->next) {
                    a->next = s;
                    s->next = 0;
                    break;
                }
            }
        }
    }
}


// Return the script list element corresponding to the passed name.
//
SIlocal::sScript *
SIlocal::GetScript(const char *name)
{
    for (sScript *a = Scripts; a; a = a->next)
        if (!strcmp(name, a->name))
            return (a);
    return (0);
}


// Static function.
char *
SIlocal::grab_script(FILE *fp, char *buf)
{
    SImacroHandler M;
    sLstr lstr;
    while (fgets(buf, 256, fp) != 0) {
        char *s = buf;
        char *tok = lstring::gettok(&s);
        if (!tok)
            continue;
        if (lstring::cieq(tok, "EndScript")) {
            delete [] tok;
            break;
        }
        delete [] tok;
        s = M.macro_expand(buf);
        lstr.add(s);
        delete [] s;
    }
    return (lstr.string_trim());
}


// Static function.
// Add to the list the files found in dir with .scr suffix, and
// submenu libraries in dir with .scm suffix.
//
umenu *
SIlocal::add_dir(const char *dir, SymTab *nametab, umenu *ul)
{
    // Open the "library" file and parse the function definitions.  Do
    // this before reading the scripts and menus, so that library
    // definitions are available.
    //
    char *lfile = pathlist::mk_path(dir, LIBRARY);
    SIfile *sfp = SIfile::create(lfile, 0, 0);
    delete [] lfile;
    if (sfp)
        SI()->Interpret(sfp, 0, 0, 0);
    delete sfp;

    DIR *wdir;
    if (!(wdir = opendir(dir)))
        return (ul);

    char *path = new char[strlen(dir) + 256];
    strcpy(path, dir);
    char *end = path + strlen(path) - 1;
    if (!lstring::is_dirsep(*end)) {
        end++;
        *end++ = '/';
        *end = 0;
    }
    else
        end++;

    struct direct *de;
    while ((de = readdir(wdir)) != 0) {
        char *s = strrchr(de->d_name, '.');
        if (!s)
            continue;
        if (lstring::cieq(s, SCR_SUFFIX) || lstring::cieq(s, LIB_SUFFIX)) {
            strcpy(end, de->d_name);
            if (filestat::is_readable(path)) {
                if (lstring::cieq(s, SCR_SUFFIX)) {
                    char buf[256];
                    strcpy(buf, de->d_name);
                    s = strrchr(buf, '.');
                    if (s)
                        *s = '\0';

                    char *label = get_script_label(path);
                    if (label) {
                        if (SymTab::get(nametab, label) == ST_NIL) {
                            ul = new umenu(label, lstring::copy(buf), ul);
                            nametab->add(label, 0, false);
                        }
                        else
                            delete [] label;
                    }
                    else if (SymTab::get(nametab, buf) == ST_NIL) {
                        char *stmp = lstring::copy(buf);
                        ul = new umenu(0, stmp, ul);
                        nametab->add(stmp, 0, false);
                    }
                }
                else {
                    umenu *nl = get_lib(path);
                    if (nl) {
                        umenu *ne = nl;
                        while (ne->next())
                            ne = ne->next();
                        ne->set_next(ul);
                        ul = nl;
                    }
                }
            }
        }
    }
    delete [] path;
    closedir(wdir);

    return (ul);
}


namespace {
    // Return true and advance the pointer if the string is in the
    // form "([<space>]library<space>", where "library" is
    // case-insensitive and literal.  The trailing white space is
    // stripped.
    //
    bool check_lib(char **ps)
    {
        char *s = *ps;
        if (*s++ != '(')
            return (false);
        while (isspace(*s))
            s++;
        if (*s != 'l' && *s != 'L')
            return (false);
        s++;
        if (*s != 'i' && *s != 'I')
            return (false);
        s++;
        if (*s != 'b' && *s != 'B')
            return (false);
        s++;
        if (*s != 'r' && *s != 'R')
            return (false);
        s++;
        if (*s != 'a' && *s != 'A')
            return (false);
        s++;
        if (*s != 'r' && *s != 'R')
            return (false);
        s++;
        if (*s != 'y' && *s != 'Y')
            return (false);
        s++;
        if (!isspace(*s))
            return (false);
        while (isspace(*s))
            s++;
        *ps = s;
        return (true);
    }
}


// Static function.
// Return a sorted library listing.
//
umenu *
SIlocal::get_lib(const char *lpath)
{
    FILE *fp = fopen(lpath, "r");
    if (!fp)
        return (0);

    // prevent duplicate entries
    SymTab *nametab = new SymTab(false, false);

    bool nosort = false;
    char buf[MACRO_BUFSIZE];
    SImacroHandler macros;

    umenu *u0 = 0, *u1 = 0;
    while (fgets(buf, MACRO_BUFSIZE, fp) != 0) {
        NTstrip(buf);
        macros.inc_line_number();
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;
        if (!u0) {
            if (!check_lib(&s)) {
                delete nametab;
                fclose(fp);
                return (0);
            }

            // Extract the library name, this is the button text.
            char c = 0;
            if (*s == '"' || *s == '\'')
                c = *s++;
            char *t = s + strlen(s) - 1;
            while (t >= s && (*t == ';' || *t == ')' || isspace(*t)))
                t--;
            if (c && t >= s && *t == c)
                t--;
            t++;
            char *name = 0;
            int len = t - s;
            if (len > 0) {
                name = new char[len + 1];
                strncpy(name, s, len);
                name[len] = 0;
                fixup(name);
            }

            // Save the file name, need this re resolve this lib later.
            char *lname = lstring::copy(lstring::strip_path(lpath));
            char *stmp = strrchr(lname, '.');
            if (stmp)
                *stmp = 0;
            u0 = new umenu(name, lname, 0);
            continue;
        }

        char *emsg;
        if (macros.handle_keyword(fp, s, 0, lpath, &emsg)) {
            if (emsg) {
                Log()->WarningLogV(mh::Initialization, "%s\n", emsg);
                delete [] emsg;
            }
            continue;
        }

        char *name = lstring::getqtok(&s);
        if (!name)
            continue;

        char *val = lstring::getqtok(&s);
        if (!val && lstring::cieq(name, "nosort")) {
            nosort = true;
            delete [] name;
            continue;
        }

        if (val) {
            char *cp = macros.macro_expand(val);
            delete [] val;
            val = cp;
        }

        // The name token can be omitted for a reference to a .scm file.
        // This just means that the referenced library name label appears
        // on the button, not overridden by a given name.

        if (!val) {
            val = name;
            name = 0;
        }
        else
            fixup(name);
        if (name && SymTab::get(nametab, name) != ST_NIL) {
            delete [] name;
            delete [] val;
            continue;
        }

        char *t = strrchr(val, '.');
        if (t && lstring::cieq(t, LIB_SUFFIX)) {
            umenu *ux = get_lib(val);
            if (ux) {
                if (name) {
                    ux->set_name(name);
                    nametab->add(name, 0, false);
                }
                else {
                    if (SymTab::get(nametab, ux->label()) != ST_NIL) {
                        umenu::destroy(ux);
                        delete [] val;
                        continue;
                    }
                    nametab->add(ux->label(), 0, false);
                }

                ux->set_next(u1);
                u1 = ux;
                delete [] val;
            }
            continue;
        }
        delete [] val;
        if (name) {
            u1 = new umenu(name, 0, u1);
            nametab->add(name, 0, false);
        }
    }
    delete nametab;
    fclose (fp);

    if (u0) {
        if (!u1) {
            umenu::destroy(u0);
            return (0);
        }
        if (nosort) {
            // have to reverse list to regain file order
            umenu *ur = 0;
            while (u1) {
                umenu *u = u1;
                u1 = u1->next();
                u->set_next(ur);
                ur = u;
            }
            u1 = ur;
        }
        else
            umenu::sort(u1);
        u0->set_menu(u1);
        return (u0);
    }
    return (0);
}


// Return the library name string.
//
char *
SIlocal::get_lib_name(const char *lpath)
{
    FILE *fp = fopen(lpath, "r");
    if (!fp)
        return (0);
    char buf[1024];
    while (fgets(buf, 1024, fp) != 0) {
        NTstrip(buf);
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;
        if (!check_lib(&s)) {
            fclose(fp);
            return (0);
        }

        // Extract the library name, this is the button text.
        char c = 0;
        if (*s == '"' || *s == '\'')
            c = *s++;
        char *t = s + strlen(s) - 1;
        while (t >= s && (*t == ';' || *t == ')' || isspace(*t)))
            t--;
        if (c && t >= s && *t == c)
            t--;
        t++;
        int len = t - s;
        if (len > 0) {
            char *name = new char[len + 1];
            strncpy(name, s, len);
            name[len] = 0;
            fixup(name);
            fclose(fp);
            return (name);
        }
        char *name = lstring::copy(lstring::strip_path(lpath));
        t = strrchr(name, '.');
        if (t && lstring::cieq(t, ".scm"))
            *t = 0;
        return (name);
    }
    return (0);
}


// Static function.
// Open the library in path, and return the path referenced by
// name, or 0 if not found.
//
char *
SIlocal::get_libref(const char *path, const char *name)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return (0);
    char buf[MACRO_BUFSIZE];
    if (!fgets(buf, MACRO_BUFSIZE, fp)) {
        fclose (fp);
        return (0);
    }
    char *s = buf;
    if (!check_lib(&s)) {
        // not a library
        fclose (fp);
        return (0);
    }

    SImacroHandler macros;
    while (fgets(buf, MACRO_BUFSIZE, fp) != 0) {
        NTstrip(buf);
        macros.inc_line_number();
        s = buf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;

        char *emsg;
        if (macros.handle_keyword(fp, s, 0, path, &emsg)) {
            if (emsg) {
                Log()->WarningLogV(mh::Initialization, "%s\n", emsg);
                delete [] emsg;
            }
            continue;
        }

        char *nm = lstring::getqtok(&s);
        if (!nm)
            continue;

        char *val = lstring::getqtok(&s);
        if (!val && lstring::cieq(nm, "nosort")) {
            delete [] nm;
            continue;
        }

        if (val) {
            char *cp = macros.macro_expand(val);
            delete [] val;
            val = cp;
        }

        if (!val) {
            val = nm;
            nm = 0;
            char *m = get_lib_name(val);
            if (m && !strcmp(name, m)) {
                delete [] m;
                fclose (fp);
                return (val);
            }
            delete [] m;
        }
        else
            fixup(nm);
        if (nm && !strcmp(name, nm)) {
            // success
            delete [] nm;
            fclose (fp);
            return (val);
        }
        delete [] nm;
        delete [] val;
    }
    fclose (fp);
    return (0);
}


// Return the menu label text from a script file.  This is obtained
// from a line of the form
//   #menulabel "label text"
// which must appear at the top of the file.
//
char *
SIlocal::get_script_label(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp)
        return (0);
    char buf[1024];
    while (fgets(buf, 1024, fp) != 0) {
        NTstrip(buf);
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s)
            continue;
        if (*s != '#')
            break;
        s++;
        while (isspace(*s))
            s++;
        if (lstring::ciprefix(MENU_LABEL, s)) {
            lstring::advtok(&s);
            char *label = lstring::getqtok(&s);
            fixup(label);
            fclose(fp);
            return (label);
        }
        break;
    }
    fclose(fp);
    return (0);
}


namespace {
    // Return the next component of the path p, advancing p.  The '/'
    // is stripped.
    //
    char *
    ltok(char **p)
    {
        char *s = *p;
        char *t = lstring::strdirsep(s);
        if (t) {
            char *tok = new char[t-s + 1];
            strncpy(tok, s, t-s);
            tok[t-s] = 0;
            *p = t+1;
            return (tok);
        }
        char *tok = lstring::copy(s);
        *p = s + strlen(s);
        return (tok);
    }
}


// Static function.
// Look for a match in dir to name.  If found, open for reading and
// return the FILE pointer.   The name has the suffix stripped.
//
FILE *
SIlocal::findopen(const char *dir, const char *name, char **fullpath)
{
    if (fullpath)
        *fullpath = 0;
    if (lstring::prefix(SCR_LIBCODE, name)) {
        // This is a library path.  The library should be found in dir,
        // so follow the references and open the final one.
        char *path = lstring::copy(name + strlen(SCR_LIBCODE));
        char *p = path;
        char *libtok = ltok(&p);
        char *pl = pathlist::mk_path(dir, libtok);
        delete [] libtok;
        char *ptmp = pathlist::expand_path(pl, false, true);
        delete [] pl;
        pl = ptmp;
        char *t = strrchr(pl, '.');
        if (!t || !lstring::cieq(t, LIB_SUFFIX)) {
            ptmp = new char[strlen(pl) + strlen(LIB_SUFFIX) + 1];
            strcpy(ptmp, pl);
            strcat(ptmp, LIB_SUFFIX);
            delete [] pl;
            pl = ptmp;
        }
        do {
            char *nm = ltok(&p);
            if (!nm) {
                delete [] pl;
                delete [] path;
                return (0);
            }
            libtok = get_libref(pl, nm);
            delete [] nm;
            if (!libtok) {
                delete [] pl;
                delete [] path;
                return (0);
            }
            delete [] pl;
            pl = libtok;

        } while (*p);

        FILE *fp = fopen(pl, "rb");
        if (fullpath && fp)
            *fullpath = pl;
        else
            delete [] pl;
        delete [] path;
        return (fp);
    }

    char *p = pathlist::mk_path(dir, name);
    char *ptmp = pathlist::expand_path(p, false, true);
    delete [] p;
    p = ptmp;
    char *t = strrchr(p, '.');
    if (!t || !lstring::cieq(t, SCR_SUFFIX)) {
        ptmp = new char[strlen(p) + strlen(SCR_SUFFIX) + 1];
        strcpy(ptmp, p);
        strcat(ptmp, SCR_SUFFIX);
        delete [] p;
        p = ptmp;
    }
    FILE *fp = fopen(p, "rb");
    if (fullpath && fp)
        *fullpath = p;
    else
        delete [] p;
    return (fp);
}
// End of SIlocal functions.


//-----------------------------------------------------------------------------

void
cMain::SaveScript(FILE *fp)
{
    SIxic.RegisterScript(fp);
}


void
cMain::DumpScripts(FILE *fp)
{
    SIxic.DumpScripts(fp, 0, true);
}


SymTab *
cMain::GetFormatFuncTab(int ix)
{
    return (SIxic.GetFormatFuncTab(ix));
}


siVariable *
cMain::GetFormatVars(int ix)
{
    return (SIxic.GetFormatVars(ix));
}


void
cMain::RegisterScript(const char *name, const char *fullpath)
{
    SIxic.RegisterScript(name, fullpath);
}


// Return a sorted tree of the user menu items.
//
umenu *
cMain::GetFunctionList()
{
    // We choose not to support identical button labels Use a hash
    // table to eliminate duplicate names.

    SymTab *nametab = new SymTab(false, false);
    umenu *ul = 0;
    const char *path = CDvdb()->getVariable(VA_ScriptPath);
    if (path) {
        pathgen pg(path);
        char *p;
        while ((p = pg.nextpath(false)) != 0) {
            ul = SIlocal::add_dir(p, nametab, ul);
            delete [] p;
        }
    }
    ul = SIxic.AddScript(ul, nametab);
    delete nametab;
    umenu::sort(ul);
    return (ul);
}


void
cMain::OpenScript(const char *namein, SIfile **pfp, stringlist **pwl,
    bool checkcwd)
{
    SIxic.OpenScript(namein, pfp, pwl, checkcwd);
}


char *
cMain::FindScript(const char *namein)
{
    return (SIxic.FindScript(namein));
}


void
cMain::OpenFormatLib(int ix)
{
    SIxic.OpenFormatLib(ix);
}


void
cMain::RunTechScripts()
{
    SIxic.RunExecs();
}
// End of cMain functions


// Function to draw the ghost_list.
//
void
cGhost::ShowGhostScript(int xp, int yp, int,  int)
{
    CDtf tfold, tfnew;
    DSP()->TCurrent(&tfold);
    DSP()->TPush();
    if (SIlcx()->applyTransform())
        GEO()->applyCurTransform(DSP(), 0, 0, xp, yp);
    else
        DSP()->TTranslate(xp, yp);
    DSP()->TCurrent(&tfnew);
    DSP()->TPop();
    for (PolyList *p = SIlcx()->ghostList(); p; p = p->next) {
        Poly poly = p->po;
        DSP()->TLoadCurrent(&tfnew);
        poly.points = Point::dup_with_xform(poly.points, DSP(), poly.numpts);
        DSP()->TLoadCurrent(&tfold);
        if (SIlcx()->applyTransform()) {
            if (GEO()->curTx()->magset())
                Point::scale(poly.points, poly.numpts, GEO()->curTx()->magn(),
                    xp, yp);
        }
        ShowGhostPath(poly.points, poly.numpts);
        delete [] poly.points;
    }
}

