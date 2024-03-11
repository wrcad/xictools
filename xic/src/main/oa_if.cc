
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
#include "promptline.h"
#include "errorlog.h"
#include "pcell_params.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "oa_if.h"
#include "python_if.h"
#include "tcltk_if.h"
#include "si_parsenode.h"
#include "si_args.h"
#include "si_handle.h"
#include "si_parser.h"
#include "tech.h"
#include "reltag.h"
#ifdef HAVE_SECURE
#include "secure.h"
#endif
#include "miscutil/filestat.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif

#ifdef HAVE_OA
#include <dlfcn.h>
//#include <pwd.h>
#endif


//-----------------------------------------------------------------------------
// Interface to OpenAccess database.
//

namespace {
#ifdef HAVE_OA
    // Dynamically open our helper lib, and return a pointer to the
    // OA interface if successful.
    //
    cOA_base *find_oa(char **lname)
    {
        bool verbose = (getenv("XIC_PLUGIN_DBG") != 0);
#ifdef HAVE_SECURE
        // Use requires a license.
        int code = XM()->Auth()->validate(OA_CODE,
            CDvdb()->getVariable(VA_LibPath));
        if (code != OA_CODE) {
            if (verbose)
                printf("The OpenAccess plug-in is not licensed, "
                "contact\nWhiteley Research for product and licensing "
                "information.\n");
            return (0);
        }
#endif

        sLstr lstr;
        const char *oaso_path = getenv("XIC_OASO_PATH");
        if (oaso_path) {
            // User told us where to look.
            lstr.add(oaso_path);
        }
        else {
            // Look in the plugins directory.
            lstr.add(XM()->ProgramRoot());
            lstr.add("/plugins/");
            lstr.add("oa.");
#ifdef __APPLE__
            lstr.add("dylib");
#else
            lstr.add("so");
#endif
        }
        if (verbose) {
            const char *lp = getenv("LD_LIBRARY_PATH");
            if (lp && *lp)
                printf("lib path: %s\n", lp);
            else
                printf("lib path: %s\n", "null");
        }

        void *handle = dlopen(lstr.string(), RTLD_LAZY | RTLD_GLOBAL);
        if (!handle) {
            if (verbose) {
                printf("dlopen failed: %s\n", dlerror());
                printf("plugin: %s\n", lstr.string());
            }
            return (0);
        }

        cOA_base*(*oaptr)() = (cOA_base*(*)())dlsym(handle, "oaptr");
        if (!oaptr) {
            if (verbose) {
                printf("dlsym failed: %s\n", dlerror());
                printf("plugin: %s\n", lstr.string());
            }
            return (0);
        }

        char idstr[64];
        snprintf(idstr, sizeof(idstr), "%s %s", XM()->OSname(),
            XIC_RELEASE_TAG);
        cOA_base *oa = (*oaptr)();
        if (oa && (!oa->id_string() || strcmp(idstr, oa->id_string()))) {
            if (verbose) {
                printf ("OpenAccess plug-in version mismatch:\n"
                    "Xic is \"%s\", plug-in is \"%s\"\n", idstr,
                    oa->id_string() ? oa->id_string() : "");
                printf("plugin: %s\n", lstr.string());
            }
            return (0);
        }
        if (!oa) {
            if (verbose)
                printf ("oa interface returned null pointer\n");
            return (0);
        }
        if (lname)
            *lname = lstring::copy(lstring::strip_path(lstr.string()));
        return (oa);
    }
#endif

    // Phony class when OA is not available.
    class cOA_stubs : public cOA_base
    {
        const char *id_string()     { return (""); }
        bool initialize()           { return (false); }
        const char *version()       { return (0); }
        const char *set_debug_flags(const char*, const char*)
            { return ("unavailable"); }

        bool is_library(const char*, bool*) { return (false); }
        bool list_libraries(stringlist**) { return (0); }
        bool list_lib_cells(const char*, stringlist**) { return (0); }
        bool list_cell_views(const char*, const char*, stringlist**)
            { return (0); }
        bool set_lib_open(const char*, bool) { return (false); }
        bool is_lib_open(const char*, bool*) { return (false); }
        bool is_oa_cell(const char*, bool, bool*) { return (false); }
        bool is_cell_in_lib(const char*, const char*, bool*)
            { return (false); }
        bool is_oa_cellview(const char*, const char*, bool, bool*)
            { return (false); }
        bool is_cellview_in_lib(const char*, const char*, const char*, bool*)
            { return (false); }
        bool create_lib(const char*, const char*) { return (false); }
        bool brand_lib(const char*, bool) { return (false); }
        bool is_lib_branded(const char*, bool*) { return (false); }
        bool destroy(const char*, const char*, const char*) { return (false); }

        bool load_library(const char*) { return (false); }
        bool load_cell(const char*, const char*, const char*, int, bool,
                const char** = 0,
                PCellParam** = 0) { return (false); }
        OItype open_lib_cell(const char*, CDcbin*) { return (OIerror); }
        void clear_name_table() { }

        bool save(const CDcbin*, const char*, bool = false, const char * = 0)
            { return (false); }

        bool attach_tech(const char*, const char*) { return (false); }
        bool destroy_tech(const char*, bool) { return (false); }
        bool has_attached_tech(const char*, char**) { return (false); }
        bool has_local_tech(const char*, bool*) { return (false); }
        bool create_local_tech(const char*) { return (false); }
        bool save_tech() { return (false); }
        bool print_tech(FILE*, const char*, const char*, const char*)
            { return (false); }
    };

#ifdef HAVE_OA
    void setup_bang_cmds();
    void setup_vars();
    void setup_funcs();
#endif
}


cOAif *cOAif::instancePtr = 0;

cOAif::cOAif()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cOAif already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

#ifdef HAVE_OA
    char *lname = 0;
    oaPtr = find_oa(&lname);
    if (oaPtr) {
        has_oa = true;
        setup_bang_cmds();
        setup_vars();
        setup_funcs();

#ifdef HAVE_MOZY
        // Define "OpenAccess" in the help database, makes visible
        // related text and topics.
        HLP()->define("OpenAccess");
#endif

        printf("Using OpenAccess (%s).\n", lname);
        delete [] lname;
        return;
    }
#endif

    has_oa = false;
    oaPtr = new cOA_stubs;
}


// Private static error exit.
//
void
cOAif::on_null_ptr()
{
    fprintf(stderr, "Singleton class cOAif used before instantiated.\n");
    exit(1);
}


// Stubs for functions defined in graphical toolkit, include when
// not building with toolkit.
#if (!defined(WITH_QT5) && !defined(WITH_QT6) && !defined(WITH_GTK2) &&\
    !defined(GTK3))
void cOAif::PopUpOAdefs(        GRobject, ShowMode, int, int) { }
void cOAif::PopUpOAlibraries(   GRobject, ShowMode) { }
void cOAif::GetSelection(const char**, const char**) { }
void cOAif::PopUpOAtech(        GRobject, ShowMode, int, int) { }
#endif


//
// ! (Bang) Commands
//

#ifdef HAVE_OA
namespace {
    namespace oa_bangcmds {
        void oaversion(const char*);
        void oadebug(const char*);
        void oanewlib(const char*);
        void oabrand(const char*);
        void oatech(const char*);
        void oasave(const char*);
        void oaload(const char*);
        void oareset(const char*);
        void oadelete(const char*);
    }
}


void
oa_bangcmds::oaversion(const char*)
{
    PL()->ShowPromptV("Using OpenAccess %s.", OAif()->version());
}


void
oa_bangcmds::oanewlib(const char *s)
{
    const char *usage = "Usage:  oanewlib libname [oldlibname]";

    char *libname = lstring::gettok(&s);
    if (!libname) {
        PL()->ShowPrompt(usage);
        return;
    }
    GCarray<char*> gc_libname(libname);
    char *oldlibname = lstring::gettok(&s);
    GCarray<char*> gc_oldlibname(oldlibname);

    if (!OAif()->create_lib(libname, oldlibname)) {
        Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
        PL()->ErasePrompt();
        return;
    }
    PL()->ShowPromptV(
        "OpenAccess library %s has been created if it didn't exist.",
        libname);
}


void
oa_bangcmds::oabrand(const char *s)
{
    const char *usage = "Usage:  oabrand [libname] [y|n]";

    char *libname = lstring::gettok(&s);
    if (!libname) {
        s = CDvdb()->getVariable(VA_OaDefLibrary);
        libname = lstring::gettok(&s);
    }
    if (!libname) {
        PL()->ShowPromptV("No library given and no default. %s", usage);
        return;
    }
    GCarray<char*> gc_libname(libname);

    char *yn = lstring::gettok(&s);
    if (!yn) {
        bool islib;
        if (!OAif()->is_library(libname, &islib)) {
            Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
            PL()->ErasePrompt();
            return;
        }
        if (!islib) {
            PL()->ShowPromptV("Unknown OpenAccess library %s.", libname);
            return;
        }
        bool isb;
        if (!OAif()->is_lib_branded(libname, &isb)) {
            Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
            PL()->ErasePrompt();
            return;
        }
        PL()->ShowPromptV(
            "OpenAccess library %s %s branded (can save only to "
            "branded libraries).", libname, isb ? "is" : "is NOT");
        return;
    }

    lstring::strtolower(yn);
    bool yes = (*yn == 'y' || *yn == '1' || *yn == 't');
    if (!yes) {
        bool no = (*yn == 'n' || *yn == '1' || *yn == 'f'); 
        if (!no) {
            PL()->ShowPrompt(usage);
            return;
        }
    }
    bool islib;
    if (!OAif()->is_library(libname, &islib)) {
        Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
        PL()->ErasePrompt();
        return;
    }
    if (!islib) {
        if (!yes) {
            PL()->ShowPromptV("Unknown OpenAccess library %s.", libname);
            return;
        }
        const char *in = PL()->EditPrompt(
            "Library does not exist, create it? ", "n");
        in = lstring::strip_space(in);
        if (in == 0) {
            PL()->ErasePrompt();
            return;
        }   
        if (*in == 'y' || *in == '1' || *in == 't') {
            if (!OAif()->create_lib(libname, 0)) {
                Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
                PL()->ErasePrompt();
                return;
            }
            // The create_lib call brands the new library, so the
            // brand_lib call below is redundant but harmless.
        }
        else {
            PL()->ErasePrompt();
            return;
        }
    }
    if (!OAif()->brand_lib(libname, yes)) {
        Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
        return;
    }
    if (yes)
        PL()->ShowPromptV(
            "OpenAccess library %s is branded, can save to this library.",
            libname);
    else
        PL()->ShowPromptV(
            "OpenAccess library %s is NOT branded, can NOT save to this "
            "library.", libname);
}


void
oa_bangcmds::oatech(const char *s)
{
    char *tok = lstring::gettok(&s);
    if (!tok)
        return;
    char sel[4];
    strncpy(sel, tok, 4);
    sel[3] = 0;
    delete [] tok;

    char *libname = lstring::gettok(&s);
    if (!libname) {
        PL()->ShowPrompt("No library name given.");
        return;
    }
    GCarray<char*> gc_libname(libname);

    if (lstring::ciprefix("a", sel) || lstring::ciprefix("-a", sel)) {
        // attach: libname fromlib
        char *fromlib = lstring::gettok(&s);
        if (!fromlib) {
            PL()->ShowPrompt("No source library name given.");
            return;
        }
        if (!OAif()->attach_tech(libname, fromlib)) {
            Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
            PL()->ShowPrompt("Attachment failed.");
        }
        else
            PL()->ShowPrompt("Attachment succeeded.");
        delete [] fromlib;
    }
    else if (lstring::ciprefix("d", sel) || lstring::ciprefix("-d", sel)) {
        // destroy: libname
        bool has_attch = false;
        char *alib;
        if (OAif()->has_attached_tech(libname, &alib)) {
            has_attch = (alib != 0);
            delete [] alib;
        }
        if (!OAif()->destroy_tech(libname, false)) {
            Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
            PL()->ShowPromptV("Tech %s failed.",
                has_attch ? "unattach" : "delete");
        }
        else
            PL()->ShowPromptV("Tech %s succeeded.",
                has_attch ? "unattach" : "delete");
    }
    else if (lstring::ciprefix("h", sel) || lstring::ciprefix("-h", sel)) {
        // has attached: libname
        char *alib;
        if (OAif()->has_attached_tech(libname, &alib)) {
            PL()->ShowPromptV(
                "Technology from library %s is attached to library %s.",
                alib, libname);
            delete [] alib;
        }
        else
            PL()->ShowPromptV(
                "Library %s has no attached technology database.", libname);
    }
    else if (lstring::ciprefix("p", sel) || lstring::ciprefix("-p", sel)) {
        // print: libname [-o filename] [which [prname]]
        char *fname = 0;
        char *which = 0;
        char *prname = 0;

        while ((tok = lstring::gettok(&s)) != 0) {
            if (!strcmp(tok, "-o")) {
                delete [] tok;
                delete [] fname;
                fname = lstring::gettok(&s);
                continue;
            }
            if (!which) {
                which = tok;
                continue;
            }
            if (!prname) {
                prname = tok;
                continue;
            }
            delete [] tok;
        }
        GCarray<char*> gc_fname(fname);
        GCarray<char*> gc_which(which);
        GCarray<char*> gc_prname(prname);

        FILE *fp;
        if (fname) {
            fp = filestat::open_file(fname, "w");
            if (!fp) {
                Log()->ErrorLog(mh::Initialization, filestat::error_msg());
                PL()->ShowPrompt("Can't open file.");
                return;
            }
        }
        else
            fp = stdout;
        bool ok = OAif()->print_tech(fp, libname, which, prname);
        if (!ok)
            Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
        if (fp != stdout) {
            fclose(fp);
            if (ok) {
                char tbuf[256];
                snprintf(tbuf, 256,
                    "OA tech listing saved in file %s, view file? ", fname);
                char *in = PL()->EditPrompt(tbuf, "n");
                in = lstring::strip_space(in);
                if (in && (*in == 'y' || *in == 'Y'))
                    DSPmainWbag(PopUpFileBrowser(fname))
            }
        }
        PL()->ErasePrompt();
    }
    else if (lstring::ciprefix("s", sel) || lstring::ciprefix("-s", sel)) {
        // save: libname
    }
    else if (lstring::ciprefix("u", sel) || lstring::ciprefix("-u", sel)) {
        // unattach: libname
        bool has_attch = false;
        char *alib;
        if (OAif()->has_attached_tech(libname, &alib)) {
            has_attch = (alib != 0);
            delete [] alib;
        }
        if (!has_attch) {
            PL()->ShowPromptV("No attached technology database in %s.",
                libname);
            return;
        }
        if (!OAif()->destroy_tech(libname, true)) {
            Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
            PL()->ShowPromptV("Tech unattach failed.");
        }
        else
            PL()->ShowPromptV("Tech unattach succeeded.");
    }
    else {
        PL()->ShowPromptV("Unrecognized command sub-type.");
    }
}


void
oa_bangcmds::oasave(const char *s)
{
    bool all = false;
    char *libname = 0;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (*tok == '-') {
            if (tok[1] == 'a' || tok[1] == 'A')
                all = true;
            delete [] tok;
            continue;
        }
        if (!libname) {
            libname = tok;
            continue;
        }
        delete tok;
    }
    if (!libname) {
        s = CDvdb()->getVariable(VA_OaDefLibrary);
        libname = lstring::gettok(&s);
    }
    if (!libname) {
        PL()->ShowPrompt("No library given and no default.");
        return;
    }
    GCarray<char*> gc_libname(libname);

    CDcbin cbin(CurCell(Physical));
    if (OAif()->save(&cbin, libname, all)) {
        if (all) {
            PL()->ShowPromptV(
                "Hierarchy under %s saved to library %s.",
                Tstring(DSP()->CurCellName()), libname);
        }
        else {
            PL()->ShowPromptV(
                "Cell %s saved to library %s.",
                Tstring(DSP()->CurCellName()), libname);
        }
        return;
    }
    PL()->ShowPrompt("Operation FAILED.");
    if (Errs()->has_error())
        Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
}


void
oa_bangcmds::oaload(const char *s)
{
    char *libname = lstring::getqtok(&s);
    char *cellname = lstring::getqtok(&s);
    if (!libname) {
        s = CDvdb()->getVariable(VA_OaDefLibrary);
        libname = lstring::gettok(&s);
    }
    if (!libname) {
        PL()->ShowPrompt("No library given and no default.");
        return;
    }
    GCarray<char*> gc_libname(libname);

    if (cellname) {
        if (OAif()->load_cell(libname, cellname, 0, CDMAXCALLDEPTH, true))
            PL()->ShowPromptV("Cell %s from library %s loaded.", cellname,
                libname);
        else {
            PL()->ShowPrompt("Operation FAILED.");
            if (Errs()->has_error())
                Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
        }
    }
    else {
        if (OAif()->load_library(libname))
            PL()->ShowPromptV("All cells from library %s loaded.", libname);
        else {
            PL()->ShowPrompt("Operation FAILED.");
            if (Errs()->has_error())
                Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
        }
    }
    delete [] cellname;
    delete [] libname;
}


void
oa_bangcmds::oareset(const char*)
{
    OAif()->clear_name_table();
    PL()->ShowPrompt("OpenAccess cells-loaded table has been cleared.");
}


void
oa_bangcmds::oadelete(const char *s)
{
    char *libname = lstring::gettok(&s);
    char *cellname = lstring::gettok(&s);
    char *tok3 = lstring::gettok(&s);
    // tok3 can be a view name, or "electrical" or "physical"

    PCellDesc::LCVcleanup lcv(libname, cellname, tok3);

    bool ok = OAif()->destroy(libname, cellname, tok3);
    if (!ok)
        Log()->ErrorLog(mh::OpenAccess, Errs()->get_error());
}


void
oa_bangcmds::oadebug(const char *s)
{
    const char *usage = "usage: !oadebug [+|-] [l[oad]] [p[cell]] [n[et]]";

    bool plus = true;
    char onchars[64], offchars[64];
    int oncnt = 0, offcnt = 0;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        char *t = tok;
        if (*t == '+') {
            plus = true;
            t++;
        }
        else if (*t == '-') {
            plus = false;
            t++;
        }
        if (*t == 'l' || *t == 'L') {
            if (plus)
                onchars[oncnt++] = 'l';
            else
                offchars[offcnt++] = 'l';
        }
        else if (*t == 'p' || *t == 'P') {
            if (plus)
                onchars[oncnt++] = 'p';
            else
                offchars[offcnt++] = 'p';
        }
        else if (*t == 'n' || *t == 'N') {
            if (plus)
                onchars[oncnt++] = 'n';
            else
                offchars[offcnt++] = 'n';
        }
        else {
            PL()->ShowPrompt(usage);
            delete [] tok;
            return;
        }
        delete [] tok;
    }
    onchars[oncnt] = 0;
    offchars[offcnt] = 0;
    s = OAif()->set_debug_flags(onchars, offchars);
    PL()->ShowPromptV("OA debug flags %s.", s);
}


namespace {
    void
    setup_bang_cmds()
    {
        XM()->RegisterBangCmd("oaversion",  &oa_bangcmds::oaversion);
        XM()->RegisterBangCmd("oadebug",    &oa_bangcmds::oadebug);
        XM()->RegisterBangCmd("oanewlib",   &oa_bangcmds::oanewlib);
        XM()->RegisterBangCmd("oabrand",    &oa_bangcmds::oabrand);
        XM()->RegisterBangCmd("oatech",     &oa_bangcmds::oatech);
        XM()->RegisterBangCmd("oasave",     &oa_bangcmds::oasave);
        XM()->RegisterBangCmd("oaload",     &oa_bangcmds::oaload);
        XM()->RegisterBangCmd("oareset",    &oa_bangcmds::oareset);
        XM()->RegisterBangCmd("oadelete",   &oa_bangcmds::oadelete);
    }
}
#endif
// End of bang commands


#ifdef HAVE_OA

//
// Variables
//

namespace {
    void
    postOaLibs(const char*)
    {
        OAif()->PopUpOAlibraries(0, MODE_UPD);
    }

    void
    postOaDefs(const char*)
    {
        OAif()->PopUpOAdefs(0, MODE_UPD, 0, 0);
    }

    bool
    evOaDefs(const char*, bool)
    {
        CDvdb()->registerPostFunc(postOaDefs);
        return (true);
    }

    bool
    evOaLibs(const char*, bool)
    {
        CDvdb()->registerPostFunc(postOaLibs);
        return (true);
    }

#define B 'b'
#define S 's'
    void vsetup(const char *vname, char c, bool(*fn)(const char*, bool))
    {
        CDvdb()->registerInternal(vname,  fn);
        if (c == B)
            Tech()->RegisterBooleanAttribute(vname);
        else if (c == S)
            Tech()->RegisterStringAttribute(vname);
    }


    void setup_vars()
    {
        vsetup(VA_OaLibraryPath,        S,  evOaDefs);
        vsetup(VA_OaDefLibrary,         S,  evOaDefs);
        vsetup(VA_OaDefTechLibrary,     S,  evOaDefs);
        vsetup(VA_OaDefLayoutView,      S,  evOaDefs);
        vsetup(VA_OaDefSchematicView,   S,  evOaDefs);
        vsetup(VA_OaDefSymbolView,      S,  evOaDefs);
        vsetup(VA_OaDefDevPropView,     S,  evOaDefs);
        vsetup(VA_OaDmSystem,           S,  evOaDefs);
        vsetup(VA_OaDumpCdfFiles,       B,  evOaDefs);
        vsetup(VA_OaUseOnly,            S,  evOaLibs);
    }
}
// End of variables


//
// Script interface functions
//

namespace {
    namespace oa_funcs {
        // OpenAccess
        bool IFoaVersion(Variable*, Variable*, void*);
        bool IFoaIsLibrary(Variable*, Variable*, void*);
        bool IFoaListLibraries(Variable*, Variable*, void*);
        bool IFoaListLibCells(Variable*, Variable*, void*);
        bool IFoaListCellViews(Variable*, Variable*, void*);
        bool IFoaIsLibOpen(Variable*, Variable*, void*);
        bool IFoaOpenLibrary(Variable*, Variable*, void*);
        bool IFoaCloseLibrary(Variable*, Variable*, void*);
        bool IFoaIsOaCell(Variable*, Variable*, void*);
        bool IFoaIsCellInLib(Variable*, Variable*, void*);
        bool IFoaIsCellView(Variable*, Variable*, void*);
        bool IFoaIsCellViewInLib(Variable*, Variable*, void*);
        bool IFoaCreateLibrary(Variable*, Variable*, void*);
        bool IFoaBrandLibrary(Variable*, Variable*, void*);
        bool IFoaIsLibBranded(Variable*, Variable*, void*);
        bool IFoaDestroy(Variable*, Variable*, void*);
        bool IFoaLoad(Variable*, Variable*, void*);
        bool IFoaReset(Variable*, Variable*, void*);
        bool IFoaSave(Variable*, Variable*, void*);
        bool IFoaAttachTech(Variable*, Variable*, void*);
        bool IFoaGetAttachedTech(Variable*, Variable*, void*);
        bool IFoaHasLocalTech(Variable*, Variable*, void*);
        bool IFoaCreateLocalTech(Variable*, Variable*, void*);
        bool IFoaDestroyTech(Variable*, Variable*, void*);
    }
    using namespace oa_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.
    PY_FUNC(OaVersion,                              0,  IFoaVersion);
    PY_FUNC(OaIsLibrary,                            1,  IFoaIsLibrary);
    PY_FUNC(OaListLibraries,                        0,  IFoaListLibraries);
    PY_FUNC(OaListLibCells,                         1,  IFoaListLibCells);
    PY_FUNC(OaListCellViews,                        2,  IFoaListCellViews);
    PY_FUNC(OaIsLibOpen,                            1,  IFoaIsLibOpen);
    PY_FUNC(OaOpenLibrary,                          1,  IFoaOpenLibrary);
    PY_FUNC(OaCloseLibrary,                         1,  IFoaCloseLibrary);
    PY_FUNC(OaIsOaCell,                             2,  IFoaIsOaCell);
    PY_FUNC(OaIsCellInLib,                          2,  IFoaIsCellInLib);
    PY_FUNC(OaIsCellView,                           3,  IFoaIsCellView);
    PY_FUNC(OaIsCellViewInLib,                      3,  IFoaIsCellViewInLib);
    PY_FUNC(OaCreateLibrary,                        2,  IFoaCreateLibrary);
    PY_FUNC(OaBrandLibrary,                         2,  IFoaBrandLibrary);
    PY_FUNC(OaIsLibBranded,                         1,  IFoaIsLibBranded);
    PY_FUNC(OaDestroy,                              3,  IFoaDestroy);
    PY_FUNC(OaLoad,                                 2,  IFoaLoad);
    PY_FUNC(OaReset,                                0,  IFoaReset);
    PY_FUNC(OaSave,                                 3,  IFoaSave);
    PY_FUNC(OaAttachTech,                           2,  IFoaAttachTech);
    PY_FUNC(OaGetAttachedTech,                      1,  IFoaGetAttachedTech);
    PY_FUNC(OaHasLocalTech,                         1,  IFoaHasLocalTech);
    PY_FUNC(OaCreateLocalTech,                      1,  IFoaCreateLocalTech);
    PY_FUNC(OaDestroyTech,                          2,  IFoaDestroyTech);

    void py_register_oa()
    {
      cPyIf::register_func("OaVersion",             pyOaVersion);
      cPyIf::register_func("OaIsLibrary",           pyOaIsLibrary);
      cPyIf::register_func("OaListLibraries",       pyOaListLibraries);
      cPyIf::register_func("OaListLibCells",        pyOaListLibCells);
      cPyIf::register_func("OaListCellViews",       pyOaListCellViews);
      cPyIf::register_func("OaIsLibOpen",           pyOaIsLibOpen);
      cPyIf::register_func("OaOpenLibrary",         pyOaOpenLibrary);
      cPyIf::register_func("OaCloseLibrary",        pyOaCloseLibrary);
      cPyIf::register_func("OaIsOaCell",            pyOaIsOaCell);
      cPyIf::register_func("OaIsCellInLib",         pyOaIsCellInLib);
      cPyIf::register_func("OaIsCellView",          pyOaIsCellView);
      cPyIf::register_func("OaIsCellViewInLib",     pyOaIsCellViewInLib);
      cPyIf::register_func("OaCreateLibrary",       pyOaCreateLibrary);
      cPyIf::register_func("OaBrandLibrary",        pyOaBrandLibrary);
      cPyIf::register_func("OaIsLibBranded",        pyOaIsLibBranded);
      cPyIf::register_func("OaDestroy",             pyOaDestroy);
      cPyIf::register_func("OaLoad",                pyOaLoad);
      cPyIf::register_func("OaReset",               pyOaReset);
      cPyIf::register_func("OaSave",                pyOaSave);
      cPyIf::register_func("OaAttachTech",          pyOaAttachTech);
      cPyIf::register_func("OaGetAttachedTech",     pyOaGetAttachedTech);
      cPyIf::register_func("OaHasLocalTech",        pyOaHasLocalTech);
      cPyIf::register_func("OaCreateLocalTech",     pyOaCreateLocalTech);
      cPyIf::register_func("OaDestroyTech",         pyOaDestroyTech);
    }
#endif  // HAVE_PYTHON

#ifdef HAVE_TCL
    // TclTk wrappers.
    TCL_FUNC(OaVersion,                             0,  IFoaVersion);
    TCL_FUNC(OaIsLibrary,                           1,  IFoaIsLibrary);
    TCL_FUNC(OaListLibraries,                       0,  IFoaListLibraries);
    TCL_FUNC(OaListLibCells,                        1,  IFoaListLibCells);
    TCL_FUNC(OaListCellViews,                       2,  IFoaListCellViews);
    TCL_FUNC(OaIsLibOpen,                           1,  IFoaIsLibOpen);
    TCL_FUNC(OaOpenLibrary,                         1,  IFoaOpenLibrary);
    TCL_FUNC(OaCloseLibrary,                        1,  IFoaCloseLibrary);
    TCL_FUNC(OaIsOaCell,                            2,  IFoaIsOaCell);
    TCL_FUNC(OaIsCellInLib,                         2,  IFoaIsCellInLib);
    TCL_FUNC(OaIsCellView,                          3,  IFoaIsCellView);
    TCL_FUNC(OaIsCellViewInLib,                     3,  IFoaIsCellViewInLib);
    TCL_FUNC(OaCreateLibrary,                       2,  IFoaCreateLibrary);
    TCL_FUNC(OaBrandLibrary,                        2,  IFoaBrandLibrary);
    TCL_FUNC(OaIsLibBranded,                        1,  IFoaIsLibBranded);
    TCL_FUNC(OaDestroy,                             3,  IFoaDestroy);
    TCL_FUNC(OaLoad,                                2,  IFoaLoad);
    TCL_FUNC(OaReset,                               0,  IFoaReset);
    TCL_FUNC(OaSave,                                3,  IFoaSave);
    TCL_FUNC(OaAttachTech,                          2,  IFoaAttachTech);
    TCL_FUNC(OaGetAttachedTech,                     1,  IFoaGetAttachedTech);
    TCL_FUNC(OaHasLocalTech,                        1,  IFoaHasLocalTech);
    TCL_FUNC(OaCreateLocalTech,                     1,  IFoaCreateLocalTech);
    TCL_FUNC(OaDestroyTech,                         2,  IFoaDestroyTech);

    void tcl_register_oa()
    {
      cTclIf::register_func("OaVersion",            tclOaVersion);
      cTclIf::register_func("OaIsLibrary",          tclOaIsLibrary);
      cTclIf::register_func("OaListLibraries",      tclOaListLibraries);
      cTclIf::register_func("OaListLibCells",       tclOaListLibCells);
      cTclIf::register_func("OaListCellViews",      tclOaListCellViews);
      cTclIf::register_func("OaIsLibOpen",          tclOaIsLibOpen);
      cTclIf::register_func("OaOpenLibrary",        tclOaOpenLibrary);
      cTclIf::register_func("OaCloseLibrary",       tclOaCloseLibrary);
      cTclIf::register_func("OaIsOaCell",           tclOaIsOaCell);
      cTclIf::register_func("OaIsCellInLib",        tclOaIsCellInLib);
      cTclIf::register_func("OaIsCellView",         tclOaIsCellView);
      cTclIf::register_func("OaIsCellViewInLib",    tclOaIsCellViewInLib);
      cTclIf::register_func("OaCreateLibrary",      tclOaCreateLibrary);
      cTclIf::register_func("OaBrandLibrary",       tclOaBrandLibrary);
      cTclIf::register_func("OaIsLibBranded",       tclOaIsLibBranded);
      cTclIf::register_func("OaDestroy",            tclOaDestroy);
      cTclIf::register_func("OaLoad",               tclOaLoad);
      cTclIf::register_func("OaReset",              tclOaReset);
      cTclIf::register_func("OaSave",               tclOaSave);
      cTclIf::register_func("OaAttachTech",         tclOaAttachTech);
      cTclIf::register_func("OaGetAttachedTech",    tclOaGetAttachedTech);
      cTclIf::register_func("OaHasLocalTech",       tclOaHasLocalTech);
      cTclIf::register_func("OaCreateLocalTech",    tclOaCreateLocalTech);
      cTclIf::register_func("OaDestroyTech",        tclOaDestroyTech);
    }
#endif  // HAVE_TCL

    void
    setup_funcs()
    {
      using namespace oa_funcs;

      // OpenAccess
      SIparse()->registerFunc("OaVersion",          0,  IFoaVersion);
      SIparse()->registerFunc("OaIsLibrary",        1,  IFoaIsLibrary);
      SIparse()->registerFunc("OaListLibraries",    0,  IFoaListLibraries);
      SIparse()->registerFunc("OaListLibCells",     1,  IFoaListLibCells);
      SIparse()->registerFunc("OaListCellViews",    2,  IFoaListCellViews);
      SIparse()->registerFunc("OaIsLibOpen",        1,  IFoaIsLibOpen);
      SIparse()->registerFunc("OaOpenLibrary",      1,  IFoaOpenLibrary);
      SIparse()->registerFunc("OaCloseLibrary",     1,  IFoaCloseLibrary);
      SIparse()->registerFunc("OaIsOaCell",         2,  IFoaIsOaCell);
      SIparse()->registerFunc("OaIsCellInLib",      2,  IFoaIsCellInLib);
      SIparse()->registerFunc("OaIsCellView",       3,  IFoaIsCellView);
      SIparse()->registerFunc("OaIsCellViewInLib",  3,  IFoaIsCellViewInLib);
      SIparse()->registerFunc("OaCreateLibrary",    2,  IFoaCreateLibrary);
      SIparse()->registerFunc("OaBrandLibrary",     2,  IFoaBrandLibrary);
      SIparse()->registerFunc("OaIsLibBranded",     1,  IFoaIsLibBranded);
      SIparse()->registerFunc("OaDestroy",          3,  IFoaDestroy);
      SIparse()->registerFunc("OaLoad",             2,  IFoaLoad);
      SIparse()->registerFunc("OaReset",            0,  IFoaReset);
      SIparse()->registerFunc("OaSave",             3,  IFoaSave);
      SIparse()->registerFunc("OaAttachTech",       2,  IFoaAttachTech);
      SIparse()->registerFunc("OaGetAttachedTech",  1,  IFoaGetAttachedTech);
      SIparse()->registerFunc("OaHasLocalTech",     1,  IFoaHasLocalTech);
      SIparse()->registerFunc("OaCreateLocalTech",  1,  IFoaCreateLocalTech);
      SIparse()->registerFunc("OaCDestroyTech",     2,  IFoaDestroyTech);

#ifdef HAVE_PYTHON
      py_register_oa();
#endif
#ifdef HAVE_TCL
      tcl_register_oa();
#endif
    }
}


//
// OpenAccess script functions.
// An OpenAccess exception will trigger a fatal error.
//

// (string) OaVersion()
//
// Return the version string of the connected OpenAccess database.  In
// none, a null string is returned.
//
bool
oa_funcs::IFoaVersion(Variable *res, Variable*, void*)
{
    const char *vrs = OAif()->version();
    res->type = TYP_STRING;
    res->content.string = lstring::copy(vrs);
    res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) OaIsLibrary(libname)
//
// Return 1 if the library named in the string argument is known to
// OpenAccess, 0 if not.
//
bool
oa_funcs::IFoaIsLibrary(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))

    bool retval;
    if (!OAif()->is_library(libname, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (stringlist_handle) OaListLibraries()
//
// Return a handle to a list of library names known to OpenAccess.
//
bool
oa_funcs::IFoaListLibraries(Variable *res, Variable*, void*)
{
    stringlist *s0;
    if (!OAif()->list_libraries(&s0))
        return (BAD);
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (stringlist.handle) OaListLibCells(libname)
//
// Return a list of the names of cells contained in the OpenAccess
// library named in the argument.
//
bool
oa_funcs::IFoaListLibCells(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))

    stringlist *s0;
    if (!OAif()->list_lib_cells(libname, &s0))
        return (BAD);
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (stringlist_handle) OaListCellViews(libname, cellname)
//
// Return a handle to a list of view names found for the given cell in
// the given OpenAccess library.
//
bool
oa_funcs::IFoaListCellViews(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))

    stringlist *s0;
    if (!OAif()->list_cell_views(libname, cellname, &s0))
        return (BAD);
    sHdl *hdl = new sHdlString(s0);
    res->type = TYP_HANDLE;
    res->content.value = hdl->id;
    return (OK);
}


// (int) IsLibOpen(libname)
//
// Return 1 if the OpenAccess library named in the argument is open, 0
// otherwise.
//
bool
oa_funcs::IFoaIsLibOpen(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))

    bool retval;
    if (!OAif()->is_lib_open(libname, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (int) OaOpenLibrary(libname)
//
// Open the OpenAccess library of the given name, where the name
// should match a library defined in the lib.defs or cds.lib file.  A
// library being open means that it is available for resolving
// undefined references when reading cell data in Xic.  The return is
// 1 on success, 0 if error.
//
bool
oa_funcs::IFoaOpenLibrary(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    res->type = TYP_SCALAR;
    res->content.value = OAif()->set_lib_open(name, true);

    return (OK);
}


// (int) OaCloseLibrary(libname)
//
// Close the OpenAccess library of the given name, where the name
// should match a library defined in the lib.defs or cds.lib file.  A
// library being open means that it is available for resolving
// undefined references when reading cell data in Xic.  The return is
// 1 on success, 0 if error.
//
bool
oa_funcs::IFoaCloseLibrary(Variable *res, Variable *args, void*)
{
    const char *name;
    ARG_CHK(arg_string(args, 0, &name))

    res->type = TYP_SCALAR;
    res->content.value = OAif()->set_lib_open(name, false);

    return (OK);
}


// (int) OaIsOaCell(cellname, open_only)
//
// Return 1 if a cell with the given name can be resolved in an
// OpenAccess library, 0 otherwise.  If the boolean value open_only is
// true, only open libraries are considered, otherwise all libraries
// are considered.
//
bool
oa_funcs::IFoaIsOaCell(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))
    bool open_only;
    ARG_CHK(arg_boolean(args, 1, &open_only))

    bool retval;
    if (!OAif()->is_oa_cell(cellname, open_only, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (int) OaIsCellInLib(libname, cellname)
//
// Return 1 if the given cell can be found in the OpenAccess library
// given as the first argument, 0 otherwise.
//
bool
oa_funcs::IFoaIsCellInLib(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))

    bool retval;
    if (!OAif()->is_cell_in_lib(libname, cellname, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (int) OaIsCellView(cellname, viewname, open_only)
//
// Return 1 if the cellname and viewname resolve as a cellview in an
// OpenAccess library, 0 otherwise.  If the boolean open_only is true,
// only open libraries are considered, otherwise all libraries are
// considered.
//
bool
oa_funcs::IFoaIsCellView(Variable *res, Variable *args, void*)
{
    const char *cellname;
    ARG_CHK(arg_string(args, 0, &cellname))
    const char *viewname;
    ARG_CHK(arg_string(args, 1, &viewname))
    bool open_only;
    ARG_CHK(arg_boolean(args, 2, &open_only))

    bool retval;
    if (!OAif()->is_oa_cellview(cellname, viewname, open_only, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (int) OaIsCellViewInLib(libname, cellname, viewname)
//
// Return 1 is the cellname and viewname resolve as a cellview in the
// given OpenAccess library, 0 otherwise.
//
bool
oa_funcs::IFoaIsCellViewInLib(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))
    const char *viewname;
    ARG_CHK(arg_string(args, 2, &viewname))

    bool retval;
    if (!OAif()->is_cellview_in_lib(libname, cellname, viewname, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (int) OaCreateLibrary(libname, techlibname)
//
// This will create the library in the OpenAccess database if libname
// currently does not exist.  This will also set up the technology for
// the new library if techlibname is given (not null or empty).  The
// new library will attach to the same library as techlibname, or will
// attach to techlibname if it has a local tech database.  If
// techlibname is given then it must exist.
//
bool
oa_funcs::IFoaCreateLibrary(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *techlibname;
    ARG_CHK(arg_string(args, 1, &techlibname))

    if (!OAif()->create_lib(libname, techlibname))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) OaBrandLibrary(libname, branded)
//
// Set or remove the Xic "brand" of the given library.  Xic can only
// write to a branded library.  If the boolean branded is true, the
// library will have its flag set, otherwise the branded status is
// unset.
//
bool
oa_funcs::IFoaBrandLibrary(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    bool branded;
    ARG_CHK(arg_boolean(args, 1, &branded))

    if (!OAif()->brand_lib(libname, branded))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) OaIsLibBranded(libname)
//
// Return 1 if the named library is "branded" (writable by Xic), 0
// otherwise.
//
bool
oa_funcs::IFoaIsLibBranded(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))

    bool retval;
    if (!OAif()->is_lib_branded(libname, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (int) OaDestroy(libname, cellname, viewname)
//
// Destroy the named view from the given cell in the given OpenAccess
// library.  If the viewname is null or empty, destroy all views from
// the named cell, i.e., the cell itself.  If the cellname is null or
// empty, undefine the library in the library definition (lib.defs or
// cds.lib) file, and change the directory name to have a ".defunct"
// extension.  We don't blow away the data, the user can revert by
// hand, or delete the directory.
//
bool
oa_funcs::IFoaDestroy(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))
    const char *viewname;
    ARG_CHK(arg_string(args, 2, &viewname))

    if (!OAif()->destroy(libname, cellname, viewname))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) OaLoad(libname, cellname)
//
// If cellname is null or empty, load all cells in the OpenAccess
// library named in libname into Xic.  The current cell is not
// changed.  Otherwise, load the cell and its hierarchy and make it
// the current cell.  The function always returns 1, a fatal error is
// thrown if the load fails.
//
bool
oa_funcs::IFoaLoad(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *cellname;
    ARG_CHK(arg_string(args, 1, &cellname))

    if (!libname || !*libname) {
        Errs()->add_error("OaLoad: null or empty library name.");
        return (BAD);
    }

    if (cellname && *cellname) {
        if (!OAif()->load_cell(libname, cellname, 0, CDMAXCALLDEPTH, true))
            return (BAD);
    }
    else {
        if (!OAif()->load_library(libname))
            return (BAD);
    }
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) OaReset()
//
// There is a table in Xic that records the cells that have been
// loaded from OpenAccess.  This avoids the "merge control" pop-up
// which appears if a common subcell was previously read and is
// already in memory, the in-memory cell will not be overwritten. 
// This function clears the table, and should be called if this
// protection should be ended, for example if the Xic database has
// been cleared.
//
bool
oa_funcs::IFoaReset(Variable *res, Variable*, void*)
{
    OAif()->clear_name_table();
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) OaSave(libname, allhier)
//
// Write the current cell to the OpenAccess library whose name is
// given in the first argument.  This must exist, and be writable from
// Xic.  Whether the physical or electrical views are written, or
// both, is determined by the value of the OaUseOnly variable.  If the
// value is "1" or starts with 'p' or 'P', only the physical (layout)
// views are written.  If the value is "2" or starts with 'e' or 'E',
// only the electrical (schematic and symbol) views are written.  If
// anything else or not set, both physical and electrical views are
// written.  The second argument is a boolean that if true (nonzero)
// indicates that the entire cell hierarchy under the current cell
// should be saved.  Otherwise, only the current cell is saved.
//
// The actual view names used are given in the OaDefLayoutView,
// OaDefSchematicView, and OaDefSymbolView variables, or default to
// "layout", "schematic", and "symbol".
//
bool
oa_funcs::IFoaSave(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    bool all;
    ARG_CHK(arg_boolean(args, 2, &all))

    if (!libname || !*libname) {
        Errs()->add_error("OaSave: null or empty library name.");
        return (BAD);
    }

    CDcbin cbin(CurCell(Physical));
    if (!OAif()->save(&cbin, libname, all))
        return (BAD);

    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) OaAttachTech(libname, techlibname)
//
// If techlibname has an attached tech library, then that library will
// be attached to libname.  If techlibname has a local tech database,
// then techlibname itself will be attached to libname.  This will
// fail if libname has a local tech database.  The local database
// should be destroyed first.
//
bool
oa_funcs::IFoaAttachTech(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    const char *techlibname;
    ARG_CHK(arg_string(args, 1, &techlibname))

    if (!OAif()->attach_tech(libname, techlibname))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (string) OaGetAttachedTech(libname)
//
// Return the name of the OpenAccess library providing the attached
// technology, or a null string if no attachment.
//
bool
oa_funcs::IFoaGetAttachedTech(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))

    char *atchname;
    if (!OAif()->has_attached_tech(libname, &atchname))
        return (BAD);
    res->type = TYP_STRING;
    res->content.string = atchname;
    if (atchname)
        res->flags |= VF_ORIGINAL;
    return (OK);
}


// (int) OaHasLocalTech(libname)
//
// Return 1 if the OpenAccess library has a local technology database,
// 0 if not.
//
bool
oa_funcs::IFoaHasLocalTech(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))

    bool retval;
    if (!OAif()->has_local_tech(libname, &retval))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = retval;
    return (OK);
}


// (int) OaCreateLocalTech(libname)
//
// If the library does not have an attached or local technology
// database, create a new local database.
//
bool
oa_funcs::IFoaCreateLocalTech(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))

    if (!OAif()->create_local_tech(libname))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}


// (int) OaDestroyTech(libname, unattach_only)
//
// If libname has an attached technology library, unattach it.  If the
// boolean second argument is false, and the library has a local
// database, destroy the database.
//
bool
oa_funcs::IFoaDestroyTech(Variable *res, Variable *args, void*)
{
    const char *libname;
    ARG_CHK(arg_string(args, 0, &libname))
    bool unattach_only;
    ARG_CHK(arg_boolean(args, 1, &unattach_only))

    if (!OAif()->destroy_tech(libname, unattach_only))
        return (BAD);
    res->type = TYP_SCALAR;
    res->content.value = 1;
    return (OK);
}

#endif

