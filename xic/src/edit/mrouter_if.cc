
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
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
 $Id: mrouter_if.cc,v 1.26 2017/03/14 01:26:34 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "edit.h"
#include "ext.h"
#include "fio.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "cd_lgen.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_property.h"
#include "tech.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "promptline.h"
#include "undolist.h"
#include "errorlog.h"
#include "si_handle.h"
#include "si_args.h"
#include "si_parser.h"
#include "../../mrouter/include/mrouter.h"
#include "mrouter_if.h"
#include "help/help_defs.h"
#ifdef WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif


//
// Implement an I/O object for the router, redirect output to log
// files.
//

class cMRio : public cLDio
{
public:
    cMRio();
    virtual ~cMRio();

    void emitErrMesg(const char*);
    void flushErrMesg();
    void emitMesg(const char*);
    void flushMesg();
    void destroy();

private:
    FILE *log_fp;
    FILE *err_fp;
    int  io_num;
    static int io_cnt;
};

int cMRio::io_cnt = 0;

cMRio::cMRio()
{
    log_fp = 0;
    err_fp = 0;
    io_num = io_cnt++;
}


cMRio::~cMRio()
{
    if (log_fp && log_fp != stdout)
        fclose(log_fp);
    if (err_fp && err_fp != stderr)
        fclose(err_fp);
}

#define LOGBASE     "mrouter"

void
cMRio::emitErrMesg(const char *msg)
{
    if (!msg || !*msg)
        return;
    if (!err_fp) {
        char buf[32];
        char *e = lstring::stpcpy(buf, LOGBASE);
        if (io_num > 0)
            sprintf(e, "-%d", io_num);
        strcat(e, ".errs");
        err_fp = Log()->OpenLog(buf, "w");
        if (!err_fp)
            err_fp = stderr;
    }
    fputs(msg, err_fp);
}

void
cMRio::flushErrMesg()
{
    if (err_fp)
        fflush(err_fp);
}

void
cMRio::emitMesg(const char *msg)
{
    if (!msg || !*msg)
        return;
    if (!log_fp) {
        char buf[32];
        char *e = lstring::stpcpy(buf, LOGBASE);
        if (io_num > 0)
            sprintf(e, "-%d", io_num);
        strcat(e, ".log");
        log_fp = Log()->OpenLog(buf, "w");
        if (!log_fp)
            log_fp = stdout;
    }
    fputs(msg, log_fp);
}

void
cMRio::flushMesg()
{
    if (log_fp)
        fflush(log_fp);
}

void
cMRio::destroy()
{
    delete this;
}
// End of cMRio functions.


// The text command interface, currently consists of only one command.
//
namespace {
    namespace mr_bangcmds {
        void mr(const char*);
    }

    const char *RouterHdr = "MRouter";
    const char *LefDefHdr = "LEF/DEF";
}

#define MROUTER_HOME    "/usr/local/mrouter"

cMRcmdIf *cMRcmdIf::instancePtr;

cMRcmdIf::cMRcmdIf()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cMRcmdIf already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    if_l = 0;
    if_r = 0;
    if_new_db = 0;
    if_new_router = 0;
    if_error = false;
    if_stepnet = -1;
    if_donemsg = 0;
    if_version = MR_VERSION;

    if_layers = 0;
    if_numlayers = 0;

    if (openRouter()) {
        if (createRouter()) {
            // Register the "!mr" command.
            XM()->RegisterBangCmd("mr", &mr_bangcmds::mr);

            // Register the script functions.
            loadMRouterFuncs();

            // Define "MRouter" in the help database.  This enables some
            // help text otherwise invisible.
            HLP()->define("MRouter");

            // Finally, add the MRouter help directory to the HelpPath,
            // making the help topics available to the user.
            const char *mrhome = getenv("MROUTER_HOME");
            if (!mrhome)
                mrhome = MROUTER_HOME;
            sLstr lstr;
            lstr.add(mrhome);
            lstr.add("/doc/xic");
            char *np = FIO()->PAppendPath(
                lstring::copy(CDvdb()->getVariable(VA_HelpPath)),
                lstr.string(), false);
            CDvdb()->setVariable(VA_HelpPath, np);
            delete [] np;
        }
        else
            fprintf(stderr, "%s\n", Errs()->get_error());
    }
}


// Private static error exit.
//
void
cMRcmdIf::on_null_ptr()
{
    fprintf(stderr, "Singleton class cMRcmdIf used before instantiated.\n");
    exit(1);
}


namespace {
    const char *so_sfx()
    {
#ifdef __APPLE__
        return (".dylib");
#else
#ifdef WIN32
        return (".dll");
#else
        return (".so");
#endif
#endif
    }

    // We have two version strings each consisting of three integers
    // separated by periods, as <major>.<minor>.<release>.  The major
    // and minor fields must match (this represents the interface
    // level).  We allow any release number, but may want to warn if
    // there is a mismatch.  In this case there may be feature
    // differences, but nothing should crash and burn.
    //
    // Return value:
    // 0    interface mismatch or syntax error
    // 1    all components match
    // 2    release values differ, others match
    //
    int check_version(const char *v1, const char *v2)
    {
        if (!v1 || !v2)
            return (0);

        unsigned int major1, minor1, release1;
        if (sscanf(v1, "%u.%u.%u", &major1, &minor1, &release1) != 3)
            return (0);

        unsigned int major2, minor2, release2;
        if (sscanf(v2, "%u.%u.%u", &major2, &minor2, &release2) != 3)
            return (0);

        if (major1 != major2 || minor1 != minor2)
            return (0);
        return (release1 == release2 ? 1 : 2);
    }
}


#define LIBMROUTER      "libmrouter"

// Open the router plug-in and connect, if possible.  On success, the
// constructor functions are set non-null and this function returns
// true.  On failure, the error flag is set and false is returned.
//
bool
cMRcmdIf::openRouter()
{
    if_r = 0;
    if_l = 0;
    if_new_db = 0;
    if_new_router = 0;
    if_error = false;

    // We are silent unless a debugging flag is set in the environment.
    bool verbose = (getenv("XIC_PLUGIN_DBG") != 0);
#ifdef WIN32
    HMODULE handle = 0;
#else
    void *handle = 0;
#endif

    sLstr lstr;
    if (access("./WRdevelop", F_OK) == 0) {
        // Hack for development.  If this file exists, look for
        // development router code.

#ifdef __APPLE__
        lstr.add("/Users/stevew/src/xictools/src/mrouter/mrouter/");
#else
#ifdef WIN32
        lstr.add(
            "c:\\cygwin\\home\\stevew\\src\\xictools\\src\\mrouter\\mrouter\\");
#else
        lstr.add("/home/stevew/src/xictools/src/mrouter/mrouter/");
#endif
#endif
        lstr.add(LIBMROUTER);
        lstr.add(so_sfx());
        verbose = true;
    }
    else
        lstr.add(getenv("MROUTER_PATH"));

    if (!lstr.string() || !*lstr.string()) {
        const char *mrhome = getenv("MROUTER_HOME");
        if (!mrhome)
            mrhome = MROUTER_HOME;

        lstr.add(mrhome);
        lstr.add("/lib/");
        lstr.add(LIBMROUTER);
        lstr.add(so_sfx());
    }

#ifdef WIN32
    lstr.to_dosdirsep();
    handle = LoadLibrary(lstr.string());
    if (!handle) {
        if (verbose) {
            int code = GetLastError();
            printf("Failed to load %s,\ncode %d.\n", lstr.string(), code);
        }
        if_error = true;
        return (false);
    }
#else
    handle = dlopen(lstr.string(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        if (verbose)
            printf("Failed to load %s,\n%s.\n", lstr.string(), dlerror());
        if_error = true;
        return (false);;
    }
#endif

#ifdef WIN32
    const char *(*vrs)() = (const char*(*)())GetProcAddress(handle,
        "ld_version_string");
    if (!vrs) {
        if (verbose) {
            int code = GetLastError();
            printf("GetProcAddress failed, error code %d.\n", code);
        }
        if_error = true;
        return (false);
    }
    int vc = check_version((*vrs)(), if_version);
    if (vc == 0) {
        if (verbose) {
            printf("fatal version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
        if_error = true;
        return (false);
    }
    if (vc == 2) {
        if (verbose) {
            printf("acceptable version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
    }

    cLDDBif*(*dptr)() = (cLDDBif*(*)())GetProcAddress(handle, "new_lddb");
    if (!dptr) {
        if (verbose) {
            int code = GetLastError();
            printf("GetProcAddress failed, error code %d.\n", code);
        }
        if_error = true;
        return (false);
    }
    cMRif*(*rptr)(cLDDBif*) = (cMRif*(*)(cLDDBif*))GetProcAddress(handle,
        "new_router");
    if (!rptr) {
        if (verbose) {
            int code = GetLastError();
            printf("GetProcAddress failed, error code %d.\n", code);
        }
        if_error = true;
        return (false);
    }
#else
    const char *(*vrs)() = (const char*(*)())dlsym(handle,
        "ld_version_string");
    if (!vrs) {
        if (verbose)
            printf("dlsym failed: %s\n", dlerror());
        if_error = true;
        return (false);
    }
    int vc = check_version((*vrs)(), if_version);
    if (vc == 0) {
        if (verbose) {
            printf("fatal version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
        if_error = true;
        return (false);
    }
    if (vc == 2) {
        if (verbose) {
            printf("acceptable version mismatch: Xic=%s mrouter=%s\n",
                if_version, (*vrs)());
        }
    }

    cLDDBif*(*dptr)() = (cLDDBif*(*)())dlsym(handle, "new_lddb");
    if (!dptr) {
        if (verbose)
            printf("dlsym failed: %s\n", dlerror());
        if_error = true;
        return (false);
    }
    cMRif*(*rptr)(cLDDBif*) = (cMRif*(*)(cLDDBif*))dlsym(handle, "new_router");
    if (!rptr) {
        if (verbose)
            printf("dlsym failed: %s\n", dlerror());
        if_error = true;
        return (false);
    }
#endif
    if_new_db = dptr;
    if_new_router = rptr;
    return (true);
}


// Create a new router object, destroys any existing.
//
bool
cMRcmdIf::createRouter()
{
    if (if_error) {
        Errs()->add_error(
            "createRouter:  error flag set, can't create new router.");
        return (false);
    }
    if (!if_new_db) {
        Errs()->add_error(
            "createRouter:  null access function, can't create new LDDB.");
        return (false);
    }
    if (!if_new_router) {
        Errs()->add_error(
            "createRouter:  null access function, can't create new router.");
        return (false);
    }
    delete if_l;
    if_l = (*if_new_db)();
    if_l->setIOhandler(new cMRio);
    delete if_r;
    if_r = (*if_new_router)(if_l);
    printf("Loading MRouter\n");
    return (true);
}


// Create a new database object, to be managed by the caller.
//
cLDDBif *
cMRcmdIf::newLDDB()
{
    if (if_error) {
        Errs()->add_error(
            "newLDDB:  error flag set, can't create new database.");
        return (0);
    }
    if (!if_new_db) {
        Errs()->add_error(
            "newLDDB:  null access function, can't create new database.");
        return (0);
    }
    cLDDBif *db = (*if_new_db)();  // This never fails.
    db->setIOhandler(new cMRio);
    return (db);
}


// Create a new router object, to be managed by the caller.
//
cMRif *
cMRcmdIf::newRouter(cLDDBif *db)
{
    if (if_error) {
        Errs()->add_error(
            "newRouter:  error flag set, can't create new router.");
        return (0);
    }
    if (!db) {
        Errs()->add_error(
            "newRouter:  null database pointer, can't create new router.");
        return (0);
    }
    if (!if_new_router) {
        Errs()->add_error(
            "newRouter:  null access function, can't create new router.");
        return (0);
    }
    cMRif *mr = (*if_new_router)(db);  // This never fails.
    return (mr);
}


// Destroy the current router object.  Currently, the application can
// have only one router object, but there is no real reason for this
// limitation.
//
void
cMRcmdIf::destroyRouter()
{
    delete if_r;
    delete if_l;
    if_r = 0;
    if_l = 0;
}


// Parse and execute the command.
//
bool
cMRcmdIf::doCmd(const char *cmd)
{
    if (!if_r) {
        Errs()->add_error("Router not available.");
        return (false);
    }

    if_l->clearMsgs();
    const char *s = cmd;
    char *tok = lstring::gettok(&s);
    if (!tok)
        return (true);

    bool ret = LD_BAD;
    bool unhandled = false;
    if (!strcmp(tok,        "source"))
        ret = cmdSource(s);
    else if (!strcmp(tok,   "place"))
        ret = cmdPlace(s);
    else if (!strcmp(tok,   "draw"))
        ret = cmdImplementRoutes(s);
    else if (!strcmp(tok,   "read")) {
        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error("Missing directive to read operation.");
            return (false);
        }
        if (!strcmp(tok,   "tech"))
            ret = cmdReadTech(s);
        else
            unhandled = true;
    }
    else if (!strcmp(tok,   "write")) {
        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error("Missing directive to write operation.");
            return (false);
        }
        if (!strcmp(tok,   "tech"))
            ret = cmdWriteTech(s);
        else
            unhandled = true;
    }
    else
        unhandled = true;
    if (unhandled)
        ret = if_r->doCmd(cmd);
    delete [] tok;

    if (ret == LD_BAD) {
        if (if_l->errMsg())
            Log()->ErrorLog(RouterHdr, if_l->errMsg());
    }
    else {
        if (if_l->warnMsg())
            Log()->WarningLog(RouterHdr, if_l->warnMsg());
        if_donemsg = lstring::copy(if_l->doneMsg());
    }

    return (ret == LD_OK);
}


//
// Application commands.  These have the MRouter template, i.e., they
// return LD_OK/LD_BAD.  Warning:  this is numerically opposite of the
// Xic convention.
//

namespace {
    // Return true for the layers we care about, namely vias and
    // routing layers.
    //
    bool layer_filter(const CDl *ld)
    {
        // We'll take only visible and non-symbolic layers.
        if (ld->isInvisible() || ld->isSymbolic())
            return (false);

        // Accept all routing layers.
        if (ld->isRouting())
            return (true);

        // We want only vias between routing layers.
        if (ld->isVia()) {
            bool ok = true;
            for (sVia *vl = tech_prm(ld)->via_list(); vl; vl = vl->next()) {
                if (!vl || !vl->layer1() || !vl->layer2()) {
                    ok = false;
                    break;
                }
                if (!vl->layer1()->isRouting() || !vl->layer2()->isRouting()) {
                    ok = false;
                    break;
                }
            }
            return (ok);
        }
        return (false);
    }
}


// Read tech information from the Xic technology database into the
// LDDB.  This must be done before LEF/DEF files are read, or after a
// reset.
//
bool
cMRcmdIf::cmdReadTech(const char*)
{
    if (!if_l) {
        Errs()->add_error("Database not available.");
        return (LD_BAD);
    }

    // Database resolution.
    if (if_l->setLefResol(CDphysResolution) != LD_OK)
        return (LD_BAD);

    // Manufacturing grid.
    if (if_l->setManufacturingGrid(if_l->micToLef(Tech()->MfgGrid())) != LD_OK)
        return (LD_BAD);

    // Identify and sequence the layers in the tech database.  We
    // can then add them to the LEF database in order.

    CDll *llist = Tech()->sequenceLayers(&layer_filter);
    if (!llist)
        return (LD_BAD);
    for (CDll *l = llist; l; l = l->next) {
        CDl *ld = l->ldesc;
        if (ld->isRouting()) {
            lefRouteLayer *lefl =
                new lefRouteLayer(lstring::copy(ld->name()));
            lefl->route.width =
                if_l->micToLefGrid(MICRONS(tech_prm(ld)->route_width()));

            // I'm not 100% sure that Cadence V/H interpretation is
            // the same as X/Y in Qrouter/MRouter.  I'll assume that
            // "verticalPitch" is the width of horizontal channels along
            // the Y axis.

            lefl->route.pitchX =
                if_l->micToLefGrid(MICRONS(tech_prm(ld)->route_h_pitch()));
            lefl->route.pitchY =
                if_l->micToLefGrid(MICRONS(tech_prm(ld)->route_v_pitch()));
            lefl->route.offsetX =
                if_l->micToLefGrid(MICRONS(tech_prm(ld)->route_h_offset()));
            lefl->route.offsetY =
                if_l->micToLefGrid(MICRONS(tech_prm(ld)->route_v_offset()));
            lefl->route.direction =
                tech_prm(ld)->route_dir() == tDirHoriz ? DIR_HORIZ : DIR_VERT;
            if (tech_prm(ld)->spacing() > 0) {
                lefl->route.spacing = new lefSpacingRule(0,
                    if_l->micToLefGrid(MICRONS(tech_prm(ld)->spacing())), 0);
            }
            if_l->lefAddObject(lefl);
        }
        else {
            // A via.
            lefCutLayer *lefc = new lefCutLayer(lstring::copy(ld->name()));
            if (tech_prm(ld)->spacing() > 0) {
                lefc->spacing =
                    if_l->micToLefGrid(MICRONS(tech_prm(ld)->spacing()));
            }
            if_l->lefAddObject(lefc);
        }
    }
    llist->free();

    // Now add the 1x1 standard vias.
    tgen_t<sStdVia> gen(Tech()->StdViaTab());
    const sStdVia *sv;
    while ((sv = gen.next()) != 0) {
        if (!sv->via() || !sv->bottom() || !sv->top())
            continue;
        if (sv->via_rows() > 1 || sv->via_cols() > 1)
            continue;
        lefViaObject *lefv =
            new lefViaObject(lstring::copy(sv->tab_name()));

        lefObject *lo = if_l->getLefObject(sv->bottom()->name());
        if (!lo || lo->lefClass != CLASS_ROUTE) {
            // Error
            delete lefv;
            continue;
        }
        int w2 = sv->via_wid()/2;
        int h2 = sv->via_hei()/2;

        lefv->via.bot.x1 = if_l->micToLefGrid(-MICRONS(w2 + sv->bot_enc_x()));
        lefv->via.bot.y1 = if_l->micToLefGrid(-MICRONS(h2 + sv->bot_enc_y()));
        lefv->via.bot.x2 = if_l->micToLefGrid(MICRONS(w2 + sv->bot_enc_x()));
        lefv->via.bot.y2 = if_l->micToLefGrid(MICRONS(h2 + sv->bot_enc_y()));
        lefv->via.bot.layer = lo->layer;
        lefv->via.bot.lefId = lo->lefId;

        lo = if_l->getLefObject(sv->top()->name());
        if (!lo || lo->lefClass != CLASS_ROUTE) {
            // Error
            delete lefv;
            continue;
        }

        lefv->via.top.x1 = if_l->micToLefGrid(-MICRONS(w2 + sv->top_enc_x()));
        lefv->via.top.y1 = if_l->micToLefGrid(-MICRONS(h2 + sv->top_enc_y()));
        lefv->via.top.x2 = if_l->micToLefGrid(MICRONS(w2 + sv->top_enc_x()));
        lefv->via.top.y2 = if_l->micToLefGrid(MICRONS(h2 + sv->top_enc_y()));
        lefv->via.top.layer = lo->layer;
        lefv->via.top.lefId = lo->lefId;

        lo = if_l->getLefObject(sv->via()->name());
        if (!lo || lo->lefClass != CLASS_CUT) {
            // Error
            delete lefv;
            continue;
        }

        lefv->via.cut.x1 = if_l->micToLefGrid(-MICRONS(w2));
        lefv->via.cut.y1 = if_l->micToLefGrid(-MICRONS(h2));
        lefv->via.cut.x2 = if_l->micToLefGrid(MICRONS(w2));
        lefv->via.cut.y2 = if_l->micToLefGrid(MICRONS(h2));
        lefv->via.cut.layer = lo->layer;
        lefv->via.cut.lefId = lo->lefId;

        if_l->lefAddObject(lefv);
    }
    return (LD_OK);
}


// Write tech information from the LDDB into the Xic technology database.
//
bool
cMRcmdIf::cmdWriteTech(const char*)
{
    if (!if_l) {
        Errs()->add_error("Database not available.");
        return (LD_BAD);
    }
    if (!setLayers()) {
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());
        return (LD_BAD);
    }
    if (!createVias()) {
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());
        return (LD_BAD);
    }

    // Manufacturing grid.
    Tech()->SetMfgGrid(if_l->lefToMic(if_l->manufacturingGrid()));

    for (u_int i = 0; ; i++) {
        lefObject *lo = if_l->getLefObject(i);
        if (!lo)
            break;
        if (lo->lefClass == CLASS_ROUTE) {
            lefRouteLayer *ll = (lefRouteLayer*)lo;
            CDl *ld;
            if (!findLayer(ll->lefName, 0, &ld)) {
                Log()->WarningLogV(LefDefHdr, "Failed to map layer %s.\n%s",
                    ll->lefName, Errs()->get_error());
                continue;
            }
            tech_prm(ld)->set_route_width(
                INTERNAL_UNITS(if_l->lefToMic(ll->route.width)));
            tech_prm(ld)->set_route_v_pitch(
                INTERNAL_UNITS(if_l->lefToMic(ll->route.pitchY)));
            tech_prm(ld)->set_route_h_pitch(
                INTERNAL_UNITS(if_l->lefToMic(ll->route.pitchX)));
            tech_prm(ld)->set_route_v_offset(
                INTERNAL_UNITS(if_l->lefToMic(ll->route.offsetY)));
            tech_prm(ld)->set_route_h_offset(
                INTERNAL_UNITS(if_l->lefToMic(ll->route.offsetX)));

            if (ll->route.direction == DIR_HORIZ)
                tech_prm(ld)->set_route_dir(tDirHoriz);
            else
                tech_prm(ld)->set_route_dir(tDirVert);
            if (ll->route.spacing) {
                tech_prm(ld)->set_spacing(
                    INTERNAL_UNITS(if_l->lefToMic(ll->route.spacing->spacing)));
            }
        }
        else if (lo->lefClass == CLASS_CUT) {
            lefCutLayer *lc = (lefCutLayer*)lo;
            CDl *ld = CDldb()->findLayer(lc->lefName, Physical);
            if (!ld || !ld->isVia()) {
                Log()->WarningLogV(LefDefHdr, "Failed to map layer %s.",
                    lc->lefName);
                continue;
            }
            tech_prm(ld)->set_spacing(
                INTERNAL_UNITS(if_l->lefToMic(lc->spacing)));
        }
    }
    return (LD_OK);
}


// The argument is a list of cell names, that should be resolvable in
// Xic.  Each is loaded into Xic if not already present (using the
// library resolution method), and loaded into the LDDB as if given in
// a LEF file.
//
bool
cMRcmdIf::cmdSource(const char *cmd)
{
    if (!if_l) {
        Errs()->add_error("Database not available.");
        return (LD_BAD);
    }

    Errs()->init_error();

    char *tok;
    while ((tok = lstring::gettok(&cmd)) != 0) {
        if (if_l->getLefGate(tok)) {
            // Gate is already in the LDDB so skip source.
            delete [] tok;
            continue;
        }
        
        CDcbin cbin;
        if (!OIfailed(CD()->OpenExisting(tok, &cbin)) && cbin.phys()) {
            CDs *sd = cbin.phys();
            lefMacro *gate = new lefMacro(lstring::copy(tok), 0.0, 0.0);
            gate->width = if_l->micToLefGrid(MICRONS(sd->BB()->width()));
            gate->height = if_l->micToLefGrid(MICRONS(sd->BB()->height()));
            gate->placedX = if_l->micToLefGrid(MICRONS(sd->BB()->left));
            gate->placedY = if_l->micToLefGrid(MICRONS(sd->BB()->bottom));

            CDs *esd = cbin.elec();
            if (esd) {
                lefPin *pe = 0;
                for (CDp_snode *p = (CDp_snode*)esd->prpty(P_NODE); p;
                        p = p->next()) {
                    CDsterm *t = p->cell_terminal();
                    if (t && t->layer()) {
                        CDl *ld = t->layer();
                        lefRouteLayer *ll;
                        if (findLefLayer(ld, &ll)) {
                            double lx = MICRONS(t->lx());
                            double ly = MICRONS(t->ly());
                            double rw = 0.5*if_l->lefToMic(ll->route.width);
                            lefu_t x1 = if_l->micToLefGrid(lx - rw);
                            lefu_t y1 = if_l->micToLefGrid(ly - rw);
                            lefu_t x2 = if_l->micToLefGrid(lx + rw);
                            lefu_t y2 = if_l->micToLefGrid(ly + rw);
                            dbDseg *ds = new dbDseg(x1, y1, x2, y2,
                                ll->layer, ll->lefId, 0);

                            lefPin *pin = new lefPin(
                                lstring::copy(p->term_name()->string()),
                                ds,     // geom
                                0,      // direction
                                0,      // use
                                0,      // shape
                                0);     // next
                            if (!pe)
                                pe = gate->pins = pin;
                            else {
                                pe->next = pin;
                                pe = pe->next;
                            }
                            gate->nodes++;
                        }
                    }
                }
            }
            if_l->lefAddGate(gate);
        }
        else {
            Log()->WarningLogV(LefDefHdr, "Failed to open cell %s.", tok);
        }
        delete [] tok;
    }
    return (LD_OK);
}


// All cells that have a placement record in the LDDB are instantiated
// in Xic at the proper location and orientation.
//
// TODO: handle cell and instance names.
//       -u to un-place.
//
bool
cMRcmdIf::cmdPlace(const char*)
{
    if (!if_l) {
        Errs()->add_error("Database not available.");
        return (LD_BAD);
    }
    if (DSP()->CurMode() != Physical) {
        PL()->ShowPrompt("Switch to physical mode for cell placement.");
        return (LD_BAD);
    }
    bool ret = LD_OK;
    Ulist()->ListCheck("mrPLACE", CurCell(Physical), false);
    for (u_int i = 0; i < if_l->numGates(); i++) {
        dbGate *gate = if_l->nlGate(i);
        const char *mname = gate->gatetype->gatename;
        int px = INTERNAL_UNITS(if_l->lefToMic(gate->placedX));
        int py = INTERNAL_UNITS(if_l->lefToMic(gate->placedY));
        int w = INTERNAL_UNITS(if_l->lefToMic(gate->width));
        int h = INTERNAL_UNITS(if_l->lefToMic(gate->height));
        int ox = INTERNAL_UNITS(if_l->lefToMic(gate->gatetype->placedX));
        int oy = INTERNAL_UNITS(if_l->lefToMic(gate->gatetype->placedY));

        const char *tfstring = "";
        switch (gate->orient) {
        case ORIENT_NORTH:
            px -= ox;
            py -= oy;
            break;
        case ORIENT_WEST:
            tfstring = "R90";
            px += h + oy;
            py -= ox;
            break;
        case ORIENT_SOUTH:
            tfstring = "R180";
            px += w + ox;
            py += h + oy;
            break;
        case ORIENT_EAST:
            tfstring = "R270";
            px -= oy;
            py += w + ox;
            break;
        case ORIENT_FLIPPED_NORTH:
            tfstring = "MX";
            px += w + ox;
            py -= oy;
            break;
        case ORIENT_FLIPPED_WEST:
            tfstring = "R90MX";
            px -= oy;
            py -= ox;
            break;
        case ORIENT_FLIPPED_SOUTH:
            tfstring = "MY";
            px -= ox;
            py += h + oy;
            break;
        case ORIENT_FLIPPED_EAST:
            tfstring = "R90MY";
            px += h + oy;
            py += w + ox;
            break;
        }
        sCurTx cx;
        if (!cx.parse_tform_string(tfstring, false)) {
            Log()->ErrorLogV(mh::CellPlacement,
                "error parsing transform string %s.", tfstring);
            ret = LD_BAD;
            break;
        }
        sCurTx txbak = *GEO()->curTx();
        GEO()->setCurTx(cx);
        CDc *cd = ED()->placeInstance(mname, px, py, 1, 1, 0, 0,
            PL_ORIGIN, false, pcpNone);
        GEO()->setCurTx(txbak);
        if (!cd) {
            Log()->ErrorLog(mh::CellPlacement, Errs()->get_error());
            ret = LD_BAD;
            break;
        }
    }
    Ulist()->CommitChanges();
    return (ret);
}


// Implememt the routes using wires and standard vias, in the Xic
// current cell.
//
// TODO: implement routes by name (arg is list of routes),
//       -u to destroy route objs
//
bool
cMRcmdIf::cmdImplementRoutes(const char*)
{
    if (!if_l) {
        Errs()->add_error("Database not available.");
        return (LD_BAD);
    }
    if (!setLayers()) {
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (LD_BAD);
    }
    if (!createVias()) {
        Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
        return (LD_BAD);
    }

    if_r->setupRoutePaths();
    for (u_int i = 0; i < if_l->numNets(); i++) {
        dbNet *net = if_l->nlNet(i);
        if (!implementRoute(net)) {
            Log()->ErrorLog(mh::ObjectCreation, Errs()->get_error());
            if_r->clearPhysRouteGen();
            return (LD_BAD);
        }
    }

    // Place the pins.
    CDs *sd = CurCell(Physical);
    for (u_int i = 0; i < if_l->numPins(); i++) {
        dbGate *gate = if_l->nlPin(i);
        dbDseg *ds = gate->taps[0];
        if (!ds) {
            Log()->WarningLogV(mh::ObjectCreation,
                "Pin %s, no physical object, not placed.", gate->gatename);
            continue;
        }
        if (ds->layer < 0) {
            Log()->WarningLogV(mh::ObjectCreation,
                "Pin %s, unknown or bad layer, not placed.", gate->gatename);
            continue;
        }

        CDl *ld;
        if (!findLayer(ds->layer, 0, &ld)) {
            Log()->WarningLogV(mh::ObjectCreation,
                "Pin %s, can't map layer %d, not placed.", gate->gatename,
                ds->layer);
            continue;
        }

        CDl *pld = EX()->getPinLayer(ld, false);
        if (!pld) {
            Log()->WarningLogV(mh::ObjectCreation,
                "Pin %s, no PIN layer for %s, not placed.", gate->gatename,
                ld->name());
            continue;
        }

        int x = INTERNAL_UNITS(if_l->lefToMic(gate->placedX));
        int y = INTERNAL_UNITS(if_l->lefToMic(gate->placedY));
        Label label;   
        label.label = new hyList(0, gate->gatename, HYcvPlain);
        label.x = x;
        label.y = y;
        label.xform = TXTF_HJC | TXTF_VJC;
        DSP()->DefaultLabelSize(gate->gatename, Physical, &label.width,
            &label.height);
        CDo *od = sd->newLabel(0, &label, pld, 0, false);
        if (!od) {
            Log()->WarningLogV(mh::ObjectCreation,
                "Pin %s, label creation failed, not placed.", gate->gatename);
            if (Errs()->has_error()) {
                Log()->WarningLogV(mh::ObjectCreation, "%s",
                    Errs()->get_error());
            }
            continue;
        }
        BBox BB(
            INTERNAL_UNITS(if_l->lefToMic(ds->x1)),
            INTERNAL_UNITS(if_l->lefToMic(ds->y1)),
            INTERNAL_UNITS(if_l->lefToMic(ds->x2)),
            INTERNAL_UNITS(if_l->lefToMic(ds->y2)));

        od = sd->newBox(0, &BB, ld, 0);
        if (!od) {
            Log()->WarningLogV(mh::ObjectCreation,
                "Pin %s, box creation on %s failed, not placed.",
                gate->gatename, ld->name());
            if (Errs()->has_error()) {
                Log()->WarningLogV(mh::ObjectCreation, "%s",
                    Errs()->get_error());
            }
            continue;
        }

        // Add pin data to the physical cell.
        CDsterm *term = sd->findPinTerm(CDnetex::name_tab_add(gate->gatename),
            true);
        term->set_loc(x, y);
        term->set_fixed(true);
        term->set_layer(ld);
    }
    return (LD_OK);
}


//
// Remaining functions are support utilities.
//

// Return a layer corresponding to LEF layer lname.  If we can't match
// by name, match by routing layer index.
//
bool
cMRcmdIf::findLayer(const char *lname, lefRouteLayer **pll, CDl **pld)
{
    lefRouteLayer *ll = if_l->getLefRouteLayer(lname);
    if (!ll) {
        Errs()->add_error("findLayer: layer %s is not in LEF data.", lname);
        return (false);
    }
    if (pll)
        *pll = ll;

    CDl *ld = CDldb()->findLayer(lname);
    if (ld && !ld->isRouting()) {
        Errs()->add_error("findLayer: layer %s is not a ROUTING layer.", lname);
        return (false);
    }
    if (!ld) {
        // Can't identify layer by name, use the sequence number.
        if (ll->layer < if_numlayers)
            ld = if_layers[ll->layer];
    }
    if (!ld)
        Errs()->add_error("findLayer: layer %s can't be mapped.", lname);
    if (pld)
        *pld = ld;
    return (ld != 0);
}


// Given a LEF routing layer number, return the lefRouteLayer and CDl.
//
bool
cMRcmdIf::findLayer(int layer, lefRouteLayer **pll, CDl **pld)
{
    if (layer < 0)
        return (false);
    lefRouteLayer *ll = if_l->getLefRouteLayer(layer);
    if (!ll) {
        Errs()->add_error("findLayer: layer %d is not in LEF data.", layer);
        return (false);
    }
    if (pll)
        *pll = ll;

    CDl *ld = CDldb()->findLayer(ll->lefName);
    if (ld && !ld->isRouting()) {
        Errs()->add_error("findLayer: layer %s is not a ROUTING layer.",
            ll->lefName);
        return (false);
    }
    if (!ld) {
        // Can't identify layer by name, use the sequence number.
        if (layer < if_numlayers)
            ld = if_layers[layer];
    }
    if (!ld)
        Errs()->add_error("findLayer: layer %d can't be mapped.", layer);
    if (pld)
        *pld = ld;
    return (ld != 0);
}


// Given a layer desc, return the corresponding LEF layer.  If not
// matching name, match using offsets into routing layer lists.  This
// applies to routing layers only.
//
bool
cMRcmdIf::findLefLayer(const CDl *ld, lefRouteLayer **pll)
{
    if (!ld)
        return (false);
    if (!ld->isRouting())
        return (false);
    lefRouteLayer *ll = if_l->getLefRouteLayer(ld->name());
    if (ll) {
        if (pll)
            *pll = ll;
        return (true);
    }
    for (int i = 0; i < if_numlayers; i++) {
        if (if_layers[i] == ld) {
            ll = if_l->getLefRouteLayer(i);
            if (ll) {
                if (pll)
                    *pll = ll;
                return (true);
            }
            break;
        }
    }
    return (false);
}


// Create the standard vias in the Xic technology database.
//
bool
cMRcmdIf::createVias()
{
    // Failure to create is not an error, will be flagged later if an
    // instance is needed.

    for (u_int i = 0; ; i++) {
        lefObject *lo = if_l->getLefObject(i);
        if (!lo)
            break;
        if (lo->lefClass != CLASS_VIA)
            continue;

        lefViaObject *lv = (lefViaObject*)lo;
        if (lv->via.bot.layer < 0 || lv->via.top.layer < 0)
            continue;

        CDl *ldb = 0;
        if (lv->via.bot.layer >= 0 && lv->via.bot.layer < if_numlayers)
            ldb = if_layers[lv->via.bot.layer];
        CDl *ldt = 0;
        if (lv->via.top.layer >= 0 && lv->via.top.layer < if_numlayers)
            ldt = if_layers[lv->via.top.layer];

        if (!ldb || !ldt) {
            Log()->WarningLogV(LefDefHdr,
                "Standard via %s creation failed, metal not mapped.",
                lv->lefName);
            continue;
        }

        // Identify the via layer.  First look between the top and
        // bottom indices, where it belongs physically.

        CDl *ldv = 0;
        bool found = false;
        for (int j = ldb->physIndex() + 1; j < ldt->physIndex(); j++) {
            ldv = CDldb()->layer(j, Physical);
            if (ldv->isVia()) {
                for (sVia *v = tech_prm(ldv)->via_list(); v; v = v->next()) {
                    if ((v->layer1() == ldb && v->layer2() == ldt) ||
                            (v->layer2() == ldb && v->layer1() == ldt)) {
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }
        if (!found) {
            // Try the index just above the top layer.
            ldv = CDldb()->layer(ldt->physIndex() + 1, Physical);
            if (ldv && ldv->isVia()) {
                for (sVia *v = tech_prm(ldv)->via_list(); v; v = v->next()) {
                    if ((v->layer1() == ldb && v->layer2() == ldt) ||
                            (v->layer2() == ldb && v->layer1() == ldt)) {
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }
        if (!found) {
            // Look through the whole layer table.
            CDextLgen gen(CDL_VIA);
            while ((ldv = gen.next()) != 0) {
                if (ldv->physIndex() >= ldb->physIndex() &&
                        ldv->physIndex() <= ldt->physIndex() + 1)
                    continue;
                for (sVia *v = tech_prm(ldv)->via_list(); v; v = v->next()) {
                    if ((v->layer1() == ldb && v->layer2() == ldt) ||
                            (v->layer2() == ldb && v->layer1() == ldt)) {
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }
        if (!found) {
            Log()->WarningLogV(LefDefHdr,
                "Standard via %s creation failed, cut not mapped.",
                lv->lefName);
            continue;
        }

        // These are units if 1/2 lambda in the router, don't know
        // why.

        sStdVia svia(lv->lefName, ldv, ldb, ldt);
        double bw = if_l->lefToMic(lv->via.bot.x2 - lv->via.bot.x1);
        double bh = if_l->lefToMic(lv->via.bot.y2 - lv->via.bot.y1);
        double bx = if_l->lefToMic(lv->via.bot.x2 + lv->via.bot.x1);
        double by = if_l->lefToMic(lv->via.bot.y2 + lv->via.bot.y1);

        double vw = if_l->lefToMic(lv->via.cut.x2 - lv->via.cut.x1);
        double vh = if_l->lefToMic(lv->via.cut.y2 - lv->via.cut.y1);
        double vx = if_l->lefToMic(lv->via.cut.x2 + lv->via.cut.x1);
        double vy = if_l->lefToMic(lv->via.cut.y2 + lv->via.cut.y1);

        double tw = if_l->lefToMic(lv->via.top.x2 - lv->via.top.x1);
        double th = if_l->lefToMic(lv->via.top.y2 - lv->via.top.y1);
        double tx = if_l->lefToMic(lv->via.top.x2 + lv->via.top.x1);
        double ty = if_l->lefToMic(lv->via.top.y2 + lv->via.top.y1);

        svia.set_via_wid(INTERNAL_UNITS(vw));
        svia.set_via_hei(INTERNAL_UNITS(vh));
        svia.set_bot_enc_x(INTERNAL_UNITS(0.25*(bw - vw)));
        svia.set_bot_enc_y(INTERNAL_UNITS(0.25*(bh - vh)));
        svia.set_bot_off_x(INTERNAL_UNITS(0.25*(bx - vx)));
        svia.set_bot_off_y(INTERNAL_UNITS(0.25*(by - vy)));
        svia.set_top_enc_x(INTERNAL_UNITS(0.25*(tw - vw)));
        svia.set_top_enc_y(INTERNAL_UNITS(0.25*(th - vh)));
        svia.set_top_off_x(INTERNAL_UNITS(0.25*(tx - vx)));
        svia.set_top_off_y(INTERNAL_UNITS(0.25*(ty - vy)));

        sStdVia *ov = Tech()->StdViaTab()->find(lv->lefName);
        if (ov) {
            if (!(*ov == svia)) {
                Log()->WarningLogV(LefDefHdr,
                    "Standard via %s exists and is different from LEF/DEF,\n"
                    "using existing.",
                    lv->lefName);
            }
        }
        else
            Tech()->AddStdVia(svia);
    }
    return (true);
}
        

// Instantiate a copy of the named standard via at x,y.
//
bool
cMRcmdIf::placeVia(const char *vname, int x, int y)
{
    if (!vname || !*vname) {
        Errs()->add_error("placeVia: null or empty via name.");
        return (false);
    }
    CDs *vsd = Tech()->OpenViaSubMaster(vname);
    if (vsd) {
        CallDesc call(vsd->cellname(), vsd);
        CDtx tx(false, 1, 0, x, y, 1.0);
        CDap ap;
        CDc *newc;
        CDs *sd = CurCell(Physical);
        if (OIfailed(sd->makeCall(&call, &tx, &ap, CDcallNone, &newc)))
            return (false);
        Ulist()->RecordObjectChange(sd, 0, newc);
        return (true);
    }
    Errs()->add_error(
        "placeVia: failed to open standard via sub-master for %s.", vname);
    return (false);
}


// If an instance of the named standard via exists as placed at x,y
// remove it.
//
bool
cMRcmdIf::removeVia(const char *vname, int x, int y)
{
    if (!vname || !*vname) {
        Errs()->add_error("removeVia: null or empty via name.");
        return (false);
    }
    BBox BB(x, y, x, y);
    BB.bloat(1);
    CDs *sd = CurCell(Physical);
    CDg gdesc;
    gdesc.init_gen(sd, CellLayer(), &BB);
    CDc *cd;
    while ((cd = (CDc*)gdesc.next()) != 0) {
        CDs *ms = cd->masterCell();
        if (!strcmp(vname, ms->cellname()->string()) &&
                cd->posX() == x && cd->posY() == y) {
            Ulist()->RecordObjectChange(sd, cd, 0);
            break;
        }
    }
    return (true);
}


// Create a wire object on the named layer, along the path given in
// the points list.
//
bool
cMRcmdIf::newWire(const char *lname, Plist *p0, int width)
{

    if (!p0) {
        Errs()->add_error("newWire: null points list.");
        return (false);
    }

    lefRouteLayer *ll;
    CDl *ld;
    if (!findLayer(lname, &ll, &ld)) {
        Errs()->add_error("newWire: layer mapping failed.");
        return (false);
    }

    int npts = 0;
    for (Plist *p = p0; p; p = p->next)
        npts++;
    Point *points = new Point[npts];
    npts = 0;
    for (Plist *p = p0; p; p = p->next) {
        points[npts].x = p->x;
        points[npts].y = p->y;
        npts++;
    }

    Wire wire;
    wire.points = points;
    wire.numpts = npts;
    if (width > 0)
        wire.set_wire_width(width);
    else
        wire.set_wire_width(INTERNAL_UNITS(if_l->lefToMic(ll->route.width)));
    wire.set_wire_style(CDWIRE_EXTEND);

    CDs *sd = CurCell(Physical);
    // New wire will take ownership of points array.
    CDw *wd = sd->newWire(0, &wire, ld, 0, false);
    // Prevent merging these wires so removeWire will always work.
    if (wd)
        wd->set_flag(CDnoMerge);
    return (wd != 0);
}


// If there is a wire in the database that exactly matches, remove it.
//
bool
cMRcmdIf::removeWire(const char *lname, Plist *p0, int width)
{
    if (!p0) {
        Errs()->add_error("removeWire: null points list.");
        return (false);
    }

    lefRouteLayer *ll;
    CDl *ld;
    if (!findLayer(lname, &ll, &ld)) {
        Errs()->add_error("removeWire: layer mapping failed.");
        return (false);
    }

    int npts = 0;
    for (Plist *p = p0; p; p = p->next)
        npts++;
    BBox BB(p0->x, p0->y, p0->x, p0->y);
    Point *points = new Point[npts];
    npts = 0;
    for (Plist *p = p0; p; p = p->next) {
        points[npts].x = p->x;
        points[npts].y = p->y;
        npts++;
        BB.add(p->x, p->y);
    }

    Wire wire;
    wire.points = points;
    wire.numpts = npts;
    if (width > 0)
        wire.set_wire_width(width);
    else
        wire.set_wire_width(INTERNAL_UNITS(if_l->lefToMic(ll->route.width)));
    wire.set_wire_style(CDWIRE_EXTEND);

    CDs *sd = CurCell(Physical);
    CDg gdesc;
    gdesc.init_gen(sd, ld, &BB);
    CDo *od;
    while ((od = gdesc.next()) != 0) {
        if (od->type() != CDWIRE)
            continue;
        CDw *wd = (CDw*)od;
        if (wd->wire_width() != wire.wire_width())
            continue;
        if (wd->wire_style() != wire.wire_style())
            continue;
        if (wd->numpts() != wire.numpts)
            continue;
        bool found = true;
        for (int i = 0; i < wire.numpts; i++) {
            if (wd->points()[i] != wire.points[i]) {
                found = false;
                break;
            }
        }
        if (!found) {
            found = true;
            for (int i = 0; i < wire.numpts; i++) {
                if (wd->points()[i] != wire.points[wire.numpts - i - 1]) {
                    found = false;
                    break;
                }
            }
        }
        if (!found)
            continue;

        Ulist()->RecordObjectChange(sd, od, 0);
        break;
    }
    // Not an error if not found, but may want to warn.
    return (true);
}


// Create a physical implementation of the routes in the net,
// consisting of wire objects and standard vias.
//
bool
cMRcmdIf::implementRoute(dbNet *net)
{
    for (int special = 0; special <= 1; special++) {
        // First pass is normal routes, second pass is for stubs.

        dbPath *p = special ? net->spath : net->path;
        if (!p)
            continue;

        Plist *p0 = 0, *pe = 0;
        const char *lname = 0;
        int width = 0;
        int lx = 0, ly = 0;
        for ( ; p; p = p->next) {
            if (p->layer >= 0) {
                if (p0) {
                    if (!newWire(lname, p0, width)) {
                        Errs()->add_error(
                            "implementRoute: net %s, wire creation failed.",
                            net->netname);
                        Plist::destroy(p0);
                        return (false);
                    }
                    Plist::destroy(p0);
                    p0 = pe = 0;
                }
                lname = 0;
                width = 0;
                if (p->vid >= 0) {
                    lefObject *lefo = if_l->getLefObject(p->vid);
                    if (!lefo) {
                        Errs()->add_error(
                            "implementRoute: net %s, unresolved via id %d.",
                            net->netname, p->vid);
                        return (false);
                    }
                    if (!placeVia(lefo->lefName,
                            INTERNAL_UNITS(if_l->lefToMic(p->x)),
                            INTERNAL_UNITS(if_l->lefToMic(p->y)))) {
                        Errs()->add_error(
                            "implementRoute: net %s, via %s creation failed.",
                            net->netname, lefo->lefName);
                        return (false);
                    }
                }
                else {
                    lname = if_l->layerName(p->layer);;
                    width = INTERNAL_UNITS(if_l->lefToMic(p->width));
                    p0 = pe = new Plist(INTERNAL_UNITS(if_l->lefToMic(p->x)),
                        INTERNAL_UNITS(if_l->lefToMic(p->y)), 0);
                    lx = p0->x;
                    ly = p0->y;
                }
                continue;
            }
            if (p0) {
                int nx = INTERNAL_UNITS(if_l->lefToMic(p->x));
                int ny = INTERNAL_UNITS(if_l->lefToMic(p->y));
                if (nx != lx || ny != ly) {
                    pe->next = new Plist(nx, ny, 0);
                    pe = pe->next;
                    lx = nx;
                    ly = ny;
                }

                if (p->vid >= 0) {
                    if (!newWire(lname, p0, width)) {
                        Errs()->add_error(
                            "implementRoute: net %s, wire creation failed.",
                            net->netname);
                        Plist::destroy(p0);
                        return (false);
                    }
                    Plist::destroy(p0);
                    p0 = pe = 0;

                    lefObject *lefo = if_l->getLefObject(p->vid);
                    if (!lefo) {
                        Errs()->add_error(
                            "implementRoute: net %s, unresolved via id %d.",
                            net->netname, p->vid);
                        return (false);
                    }
                    if (!placeVia(lefo->lefName,
                            INTERNAL_UNITS(if_l->lefToMic(p->x)),
                            INTERNAL_UNITS(if_l->lefToMic(p->y)))) {
                        Errs()->add_error(
                            "implementRoute: net %s, via %s creation failed.",
                            net->netname, lefo->lefName);
                        return (false);
                    }
                }
            }
        }
        if (p0) {
            if (!newWire(lname, p0, width)) {
                Errs()->add_error(
                    "implementRoute: net %s, wire creation failed.",
                    net->netname);
                Plist::destroy(p0);
                return (false);
            }
            Plist::destroy(p0);
            p0 = pe = 0;
        }
    }

    return (true);
}


// Generate an ordered list of routing layers from the technology
// database.  Unless we can match these layers by name, they will be
// matched by index, i.e., the layer field of a lefRoutingLayer will
// index the array created here.
//
bool
cMRcmdIf::setLayers()
{
    delete [] if_layers;
    if_layers = 0;
    if_numlayers = 0;

    CDll *llist = Tech()->sequenceLayers(&layer_filter);
    if (!llist)
        return (false);
    int cnt = 0;
    for (CDll *l = llist; l; l = l->next) {
        if (l->ldesc->isRouting())
            cnt++;
    }
    if (!cnt) {
        Errs()->add_error(
            "No sequencable routing layers found in layer table.");
        llist->free();
        return (false);
    }
    if_layers = new CDl*[cnt];
    if_numlayers = cnt;
    cnt = 0;
    for (CDll *l = llist; l; l = l->next) {
        if (l->ldesc->isRouting())
            if_layers[cnt++] = l->ldesc;
    }
    llist->free();
    return (true);
}


/*XXX
// Open the cell whose name is given, which is most likely a library
// cell.  Extract, and add a description of the cell, its pins and
// obstructions, to the LEF database.  This is equivalent to reading a
// MACRO in a LEF file
//
bool
cMRcmdIf::readMacro(const char *cname)
{
    CDcbin cbin;
    if (FIO()->OpenImport(cname, FIO()->DefReadPrms(), 0, 0, &cbin) != OIok) {
        Errs()->add_error("Open of cell %s failed.", cname);
        return (false);
    }
    COs *sdesc = cbin.phys();

    // Extract.
    if (!EX()->extract(sdesc)) {
        Errs()->add_error("Extraction of cell %s failed.", cname);
        return (false);
    }
    // The pin nets are marked by labels.  output these nets.

    // Look through the remaining nets, add the objects as
    // obstructions.

    return (true);
}
*/
// End of cMRcmdIf functions.


// The command implementation.
//
void
mr_bangcmds::mr(const char *s)
{
    if (!s)
        return;

    Errs()->init_error();
    bool ret = MRcmd()->doCmd(s);
    if (Errs()->has_error())
        Log()->PopUpErr(Errs()->get_error());

    if (!ret)
        PL()->ShowPrompt("Operation FAILED");
    else {
        char *donemsg = MRcmd()->doneMsg();
        if (donemsg) {
            // If the donemsg has more than one line, show it in a
            // window, otherwise use the prompt line.

            int ncnt = 0;
            for (const char *t = donemsg; *t; t++) {
                if (*t == '\n') {
                    ncnt++;
                    if (ncnt == 2)
                        break;
                }
            }
            if (ncnt > 1) {
                DSPmainWbag(PopUpInfo(MODE_ON, donemsg, STY_FIXED))
                PL()->ShowPrompt("OK");
            }
            else
                PL()->ShowPrompt(donemsg);
            delete [] donemsg;
        }
        else
            PL()->ShowPrompt("OK");
    }
}


//
// Script command interface.
//

namespace {
    namespace mrouter_funcs {
        bool IFmRouter(Variable*, Variable*, void*);
    }
    using namespace mrouter_funcs;

#ifdef HAVE_PYTHON
    // Python wrappers.

    PY_FUNC(MRouter,            1,  IFmRouter);

    void py_register_mrouter()
    {
        cPyIf::register_func("MRouter",     pyMRouter);
    }
#endif

#ifdef HAVE_TCL
    // Tcl/Tk wrappers.

    TCL_FUNC(MRouter,           1,  IFmRouter);

    void tcl_register_mrouter()
    {
        cTclIf::register_func("MRouter",    tclMRouter);
    }
#endif
}


void
cMRcmdIf::loadMRouterFuncs()
{
    SIparse()->registerFunc("MRouter",          1, IFmRouter);
#ifdef HAVE_PYTHON
    py_register_mrouter();
#endif
#ifdef HAVE_TCL
    tcl_register_mrouter();
#endif
}


// (string) MRouter(command)
//
// The cmd is a router command line as would be given to the !mr bang
// command.  The return value is a string containing any
// non-error/warning text generated by the operation.  On error,
// script execution will halt, with a message available from GetError. 
// Non-fatal errors and warnings may pop-up a message window.
//
bool
mrouter_funcs::IFmRouter(Variable *res, Variable *args, void*)
{
    const char *cmd;
    ARG_CHK(arg_string(args, 0, &cmd));
    if (!cmd)
        return (OK);

    Errs()->init_error();
    bool ret = MRcmd()->doCmd(cmd);
    if (!ret) {
        Errs()->add_error("MRouter:  operation \"%s\" failed.", cmd);
        return (BAD);
    }
    if (Errs()->has_error())
        Log()->PopUpWarn(Errs()->get_error());

    res->type = TYP_STRING;
    res->content.string = MRcmd()->doneMsg();
    if (res->content.string)
        res->flags |= VF_ORIGINAL;

    return (OK);
}

