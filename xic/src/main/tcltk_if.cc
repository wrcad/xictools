
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
 $Id: tcltk_if.cc,v 5.18 2015/07/04 22:19:51 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "main.h"
#include "promptline.h"
#include "errorlog.h"
#include "dsp_inlines.h"
#include "tcltk_if.h"
#include "tcl_base.h"
#include "reltag.h"

#ifdef HAVE_TCL
#include <dlfcn.h>
#endif


//-----------------------------------------------------------------------------
// Interface to Tcl/Tk interpreter.
//

namespace {
    const char *notavail = "Tcl/Tk interface not available.";
}

class cTcl_nogo : public cTcl_base
{
    const char *id_string() { return (notavail); }
    bool init(SymTab*)      { Errs()->add_error(notavail); return (false); }
    bool run(const char*, bool)
                            { Errs()->add_error(notavail); return (false); }
    bool runString(const char*, const char*)
                            { Errs()->add_error(notavail); return (false); }
    void reset()            { }
    int wrapper(const char*, Tcl_Interp*, Tcl_Obj* const*, int, int,
                            SIscriptFunc)
                            { return (0); }
    bool hasTk()            { return (false); }
};


#ifdef HAVE_TCL
namespace {
    // Dynamically open our helper lib, and return a pointer to the
    // interface if successful.
    //
    cTcl_base *find_tcl(bool with_tk, char **lname)
    {
        bool verbose = (getenv("XIC_PLUGIN_DBG") != 0);
        sLstr lstr;
        const char *tclso_path = getenv("XIC_TCLSO_PATH");
        if (tclso_path) {
            // User told us where to look.
            lstr.add(tclso_path);
        }
        else {
            // Look in the plugins directory.
            lstr.add(XM()->ProgramRoot());
            lstr.add("/plugins/");
            lstr.add(with_tk ? "tcltk." : "tcl.");
#ifdef __APPLE__
            lstr.add("dylib");
#else
            lstr.add("so");
#endif
        }

        void *handle = dlopen(lstr.string(), RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            if (verbose)
                printf("dlopen failed: %s\n", dlerror());
            return (0);
        }

        cTcl_base*(*tclptr)() = (cTcl_base*(*)())dlsym(handle, "tclptr");
        if (!tclptr) {
            if (verbose)
                printf("dlsym failed: %s\n", dlerror());
            return (0);
        }

        char idstr[64];
        sprintf(idstr, "%s %s", XM()->OSname(), CVS_RELEASE_TAG);
        cTcl_base *tcl = (*tclptr)();
        if (tcl && (!tcl->id_string() || strcmp(idstr, tcl->id_string()))) {
            if (verbose) {
                printf ("%s plug-in version mismatch:\n"
                    "Xic is \"%s\", plug-in is \"%s\"\n",
                    with_tk ? "Tcl/Tk" : "Tcl", idstr,
                    tcl->id_string() ? tcl->id_string() : "");
            }
            tcl = 0;
        }
        if (!tcl) {
            if (verbose)
                printf("%s interface returned null pointer\n",
                    with_tk ? "Tcl/Tk" : "Tcl");
            return (0);
        }
        if (lname)
            *lname = lstring::copy(lstring::strip_path(lstr.string()));
        return (tcl);
    }
}
#endif


SymTab *cTclIf::tcl_functions = 0;
cTclIf *cTclIf::instancePtr = 0;

cTclIf::cTclIf()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cTclIf already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

#ifdef HAVE_TCL
    char *lname = 0;
    tclPtr = find_tcl(true, &lname);  // First try with Tk.
    if (!tclPtr)
        tclPtr = find_tcl(false, &lname);  // Try again without Tk.
    if (tclPtr) {
        has_tcl = true;
        has_tk = tclPtr->hasTk();
        XM()->RegisterBangCmd("tcl", &bang_tcl);
        if (has_tk) {
            XM()->RegisterBangCmd("tk", &bang_tcl);
            printf("Using Tcl/Tk (%s).\n", lname);
        }
        else
            printf("Using Tcl (%s).\n", lname);
        delete [] lname;
        return;
    }
#endif

    tclPtr = new cTcl_nogo;
    has_tcl = false;
}


// Private static error exit.
//
void
cTclIf::on_null_ptr()
{
    fprintf(stderr, "Singleton class cTclIf used before instantiated.\n");
    exit(1);
}


bool
cTclIf::run(const char *s)
{
    Errs()->init_error();
    const char *cmdstr = s;
    char *cmdfile = lstring::getqtok(&s);
    if (!cmdfile) {
        Errs()->add_error("run: no command file given!");
        return (false);
    }
    GCarray<char*> gc_cmdfile(cmdfile);
    bool is_tk = false;
    if (has_tk) {
        // Tk is available.  The file must have a .tcl or .tk extension.
        char *ext = strrchr(cmdfile, '.');
        if (!ext || (!lstring::cieq(ext, ".tcl") &&
                !lstring::cieq(ext, ".tk"))) {
            Errs()->add_error(
                "run: command file must have .tcl or .tk extension.");
            return (false);
        }
        is_tk = lstring::cieq(ext, ".tk");

    }
    else {
        // We allow any extension but .tk.
        char *ext = strrchr(cmdfile, '.');
        if (ext && lstring::cieq(ext, ".tk")) {
            Errs()->add_error("run: .tk extension, Tk is unavailable.");
            return (false);
        }
    }
    if (tcl_functions) {
        bool ret = tclPtr->init(tcl_functions);
        tcl_functions = 0;
        if (!ret)
            return (false);
    }
    return (tclPtr->run(cmdstr, is_tk));
}


bool
cTclIf::runString(const char *prmstr, const char *cmds)
{
    Errs()->init_error();
    if (tcl_functions) {
        bool ret = tclPtr->init(tcl_functions);
        tcl_functions = 0;
        if (!ret)
            return (false);
    }
    return (tclPtr->runString(prmstr, cmds));
}


void
cTclIf::reset()
{
    tclPtr->reset();
}


// Static function.
// This puts a Tcl/Tk function implementation into the table.  The table
// must be filled before Tcl/Tk is run, at which time the table will be
// loaded into the Tcl/Tk interpreter and destroyed.
//
// The fname MUST be a persistent string!
//
void
cTclIf::register_func(const char *fname,
    int(*func)(ClientData, Tcl_Interp*, int, Tcl_Obj* const*))
{
    if (!fname || !func)
        return;
    if (!tcl_functions)
        tcl_functions = new SymTab(false, false);
    SymTabEnt *ent = tcl_functions->get_ent(fname);
    if (!ent)
        tcl_functions->add(fname, (void*)func, false);
    else
        ent->stData = (void*)func;
}


// Static function
//
void
cTclIf::bang_tcl(const char *s)
{
    if (TclIf()->run(s))
        PL()->ShowPrompt("Tcl/Tk done.");
    else {
        Log()->ErrorLog("Tck/Tk Interface", Errs()->get_error());
        PL()->ShowPrompt("Tcl/Tk execution failed.");
    }
}

