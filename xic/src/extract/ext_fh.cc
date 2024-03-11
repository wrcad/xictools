
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
#include "ext.h"
#include "ext_fh.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "geo_zgroup.h"
#include "promptline.h"
#include "errorlog.h"
#include "tech.h"
#include "tech_layer.h"
#include "tech_ldb3d.h"
#include "miscutil/filestat.h"
#include "miscutil/hashfunc.h"

#include <algorithm>

//========================================================================
// Interface to FastHenry program
//========================================================================


// Instantiate FastHenry interface.
namespace {
    cFH _fh_;
}


//------------------------------------------------------------------------
// Exported Interface Functions
//------------------------------------------------------------------------

cFH *cFH::instancePtr = 0;

cFH::cFH()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cFH already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    fh_popup_visible = false;
}


// Private static error exit.
//
void
cFH::on_null_ptr()
{
    fprintf(stderr, "Singleton class cFH used before instantiated.\n");
    exit(1);
}


// Stubs for functions defined in graphical toolkit, include when
// not building with toolkit.
#if (!defined(WITH_QT5) && !defined(WITH_QT6) && !defined(WITH_GTK2) &&\
    !defined(GTK3))
void cFH::PopUpExtIf(GRobject, ShowMode) { }
void cFH::updateString() { }
#endif


// Perform an operation, according to the keyword given.
//
void
cFH::doCmd(const char *op, const char *str)
{
    if (!op) {
        PL()->ShowPromptV("Error: no keyword given.");
        return;
    }
    else if (lstring::cieq(op, "dump") || lstring::cieq(op, "dumph")) {
        char *fn = lstring::getqtok(&str);
        fhDump(fn);
        delete [] fn;
        PL()->ErasePrompt();
    }
    else if (lstring::cieq(op, "run") || lstring::cieq(op, "runh")) {
        char *tok = lstring::gettok(&str);
        char *infile = 0;
        char *outfile = 0;
        char *resfile = 0;
        while (tok) {
            if (!strcmp(tok, "-i")) {
                delete [] tok;
                infile = lstring::getqtok(&str);
                tok = lstring::gettok(&str);
            }
            else if (!strcmp(tok, "-o")) {
                delete [] tok;
                outfile = lstring::getqtok(&str);
                tok = lstring::gettok(&str);
            }
            else if (!strcmp(tok, "-r")) {
                delete [] tok;
                resfile = lstring::getqtok(&str);
                tok = lstring::gettok(&str);
            }
            else {
                PL()->ShowPromptV("Unrecognized token %s.", tok);
                delete [] tok;
                delete [] infile;
                delete [] outfile;
                delete [] resfile;
                return;
            }
        }
        fhRun(infile, outfile, resfile, (infile != 0));
        delete [] infile;
        delete [] outfile;
        delete [] resfile;
        PL()->ErasePrompt();
    }
    else
        PL()->ShowPromptV("Error: unknown keyword %s given.", op);
}


namespace {
    char *getword(const char *kw, const char *str)
    {
        const char *s = strstr(str, kw);
        if (!s)
            return (lstring::copy(""));
        s += strlen(kw);
        const char *t = s;
        while (*t && !isspace(*t))
            t++;
        char *r = new char[t-s + 1];
        strncpy(r, s, t-s);
        r[t-s] = 0;
        return (r);
    }


    bool get_freq_spec(char **sminp, char **smaxp, char **sdecp)
    {
        const char *str = CDvdb()->getVariable(VA_FhFreq);
        char *smin = str ? getword("fmin=", str) : lstring::copy("1e3");
        char *smax = str ? getword("fmax=", str) : lstring::copy("1e3");
        char *sdec = str ? getword("ndec=", str) : lstring::copy("");

        bool ok = true;
        double fmin;
        if (sscanf(smin, "%lf", &fmin) != 1 || fmin < 0.0) {
            fmin = 0.0;
            ok = false;
        }
        double fmax;
        if (sscanf(smax, "%lf", &fmax) != 1 || fmax < fmin)
            ok = false;
        double ndec;
        if (*sdec && (sscanf(sdec, "%lf", &ndec) != 1 || ndec < 0.1))
            ok = false;

        if (sminp)
            *sminp = smin;
        else
            delete [] smin;
        if (smaxp)
            *smaxp = smax;
        else
            delete [] smax;
        if (sdecp)
            *sdecp = sdec;
        else
            delete [] sdec;
        return (ok);
    }
}


// Dump a FastHenry input file.
//
bool
cFH::fhDump(const char *fname)
{
    if (!CurCell(Physical)) {
        Log()->PopUpErr("No current cell!");
        return (false);
    }

    fhLayout fhl;
    if (Errs()->has_error())
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());

    int ccnt = 0;
    for (Layer3d *l = fhl.layers(); l; l = l->next()) {
        if (l->is_conductor())
            ccnt++;
    }
    if (!ccnt) {
        Log()->ErrorLog(mh::Initialization,
            "No conductor layers sequenced.\n");
        return (false);
    }

    if (!get_freq_spec(0, 0, 0)) {
        Log()->ErrorLogV(mh::Initialization,
            "Error: bad frequency specification.\n  .Freq %s\n",
            CDvdb()->getVariable(VA_FhFreq));
        return (false);
    }
    if (!fhl.setup()) {
        Log()->ErrorLog(mh::Initialization, Errs()->get_error());
        return (false);
    }

    if (!fname || !*fname)
        fname = getFileName(FH_INP_SFX);
    else
        fname = lstring::copy(fname);
    GCarray<const char*> gc_fname(fname);

    if (!filestat::create_bak(fname)) {
        Log()->ErrorLog(mh::Initialization, filestat::error_msg());
        return (false);
    }

    FILE *fp = filestat::open_file(fname, "w");
    if (!fp) {
        Log()->ErrorLog(mh::Initialization, filestat::error_msg());
        return (false);
    }

    bool ret = fhl.fh_dump(fp);
    fclose(fp);
    updateString();
    return (ret);
}


// Dump an input file and run FastHenry.  Collect the output and do
// some processing, present the results in a file browser window.
//
void
cFH::fhRun(const char *infile, const char *outfile, const char *resfile,
    bool nodump)
{
    if (!CurCell(Physical)) {
        Log()->PopUpErr("No current cell!");
        return;
    }
    bool run_foreg = CDvdb()->getVariable(VA_FhForeg);
    bool monitor = CDvdb()->getVariable(VA_FhMonitor);

    fxJob *newjob = new fxJob(Tstring(CurCell(Physical)->cellname()),
        fxIndMode, this, fxJob::jobs());
    fxJob::set_jobs(newjob);
    char *in_f = 0, *ot_f = 0, *lg_f = 0;
    if (run_foreg) {
        if (!infile || !*infile) {
            in_f = FHif()->getFileName(FH_INP_SFX);
            infile = in_f;
        }
        if (!outfile || !*outfile) {
            ot_f = FHif()->getFileName("fh_out");
            outfile = ot_f;
        }
        if (!resfile || !*resfile) {
            lg_f = FHif()->getFileName("fh_log");
            resfile = lg_f;
        }
    }
    if (!infile || !*infile)
        newjob->set_flag(FX_UNLINK_IN);
    if (!outfile || !*outfile)
        newjob->set_flag(FX_UNLINK_OUT);

    newjob->set_infiles(new stringlist(infile && *infile   ?
        lstring::copy(infile)  : filestat::make_temp("fhi"), 0));
    newjob->set_outfile(outfile && *outfile ?
        lstring::copy(outfile) : filestat::make_temp("fho"));
    newjob->set_resfile(resfile && *resfile ? lstring::copy(resfile) : 0);
    delete [] in_f;
    delete [] ot_f;
    delete [] lg_f;

    if (!newjob->setup_fh_run(run_foreg, monitor)) {
        delete newjob;
        return;
    }
    if (!nodump && !FHif()->fhDump(newjob->infile())) {
        delete newjob;
        return;
    }
    if (!filestat::create_bak(newjob->outfile())) {
        DSPpkg::self()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        delete newjob;
        return;
    }

    if (!newjob->run(run_foreg, monitor)) {
        delete newjob;
        return;
    }
    if (run_foreg) {
        newjob->fh_post_process();
        delete newjob;
    }

    updateString();
}


// Used by the control panel to record visibility.
//
void
cFH::setPopUpVisible(bool vis)
{
    fh_popup_visible = vis;
}


// Return a file name for use with output.
//
char *
cFH::getFileName(const char *ext, int pid)
{
    if (!CurCell(Physical))
        return (0);
    char buf[128];
    const char *s = Tstring(CurCell(Physical)->cellname());
    if (pid > 0)
        snprintf(buf, sizeof(buf), "%s-%d.%s", s, pid, ext);
    else
        snprintf(buf, sizeof(buf), "%s.%s", s, ext);
    return (lstring::copy(buf));
}


// Return the "real" units string for the argument, which can be a
// long form.  If the string is not recognized or the default unit, 0
// is returned.
//
const char *
cFH::getUnitsString(const char *str)
{
    int u = unit_t::find_unit(str);
    if (u < 0 || u == FX_DEF_UNITS)
        return (0);
    return (unit_t::units((e_unit)u)->name());
}


// Return the unit index corresponding to the passed string, the
// default is returned if not recognized.
//
int
cFH::getUnitsIndex(const char *str)
{
    int u = unit_t::find_unit(str);
    if (u < 0)
        u = FX_DEF_UNITS;
    return (u);
}


// Return status string for the panel label.
//
char *
cFH::statusString()
{
    int njobs = fxJob::num_jobs();
    char buf[128];
    snprintf(buf, sizeof(buf), "Running Jobs: %d", njobs);
    return (lstring::copy(buf));
}


// Return the command string for the pid of a running process.
//
const char *
cFH::command(int pid)
{
    fxJob *j = fxJob::find(pid);
    return (j ? j->command() : 0);
}


// Return a list of active background jobs.
//
char *
cFH::jobList()
{
    sLstr lstr;
    for (fxJob *j = fxJob::jobs(); j; j = j->next_job())
        j->pid_string(lstr);
    return (lstr.string_trim());
}


// Kill the running process.
//
void
cFH::killProcess(int pid)
{
    fxJob *j = fxJob::find(pid);
    if (j)
        j->kill_process();
}
// End of cFH functions.


namespace {
    inline double get_sigma(CDl *ld)
    {
        if (!ld)
            return (0.0);
        TechLayerParams *lp = tech_prm(ld);
        DspLayerParams *dp = dsp_prm(ld);
        double rho = lp->rho();
        if (rho <= 0.0 && lp->ohms_per_sq() > 0.0 && dp->thickness() > 0.0)
            rho = 1e-6*lp->ohms_per_sq()*dp->thickness();
        if (rho <= 0.0)
            return (0.0);
        return (1.0/rho);
    }
}


#define TPRINT(...) if (db3_logfp) fprintf(db3_logfp, __VA_ARGS__)

// Constructor.  Create ordered lists of the conducting and dielectric
// layers found in the layer table.  The layers of each type must be
// in order, but not necessarily with respect to layers of the other
// type.  Conducting layers must have the CONDUCTOR attribute, and
// have nonzero thickness.  Insulating layers must have the VIA
// attribute and nonzero thickness.
//
fhLayout::fhLayout()
{
    fhl_layers = 0;
    fhl_terms = 0;
    fhl_zoids = false;

    if (logging())
        set_logfp(DBG_FP);

    // Specify a masking layer.  If it exists in the cell, then it
    // clips the extracted geometry.
    const char *lname = CDvdb()->getVariable(VA_FhLayerName);
    if (!lname)
        lname = FH_LAYER_NAME;

    // This is used only when approximating non-Manhattan edges.
    int manh_gcnt = FH_DEF_MANH_GRID_CNT;
    const char *var = CDvdb()->getVariable(VA_FhManhGridCnt);
    if (var) {
        int val = atoi(var);
        if (val >= FH_MIN_MANH_GRID_CNT && val <= FH_MAX_MANH_GRID_CNT)
            manh_gcnt = val;
    }
    if (!init_for_extraction(CurCell(Physical), 0, lname,
            Tech()->SubstrateEps(), Tech()->SubstrateThickness(),
            manh_gcnt, 0)) {
        Errs()->add_error("Layer setup failed.");
    }
}


namespace {
#ifdef XXX_NOTUSED
    // If line p1,p2 is horizontal and intersects z->Z, split the
    // (Manhattan) zoid and link in the new piece after z.
    //
    void cut_zoid_h(glZlist3d *z, Point &p1, Point &p2)
    {
        if (p1.y == p2.y) {
            int y = p1.y;
            if (y >= z->Z.yu || y <= z->Z.yl)
                return;
            int xmin = mmMin(p1.x, p2.x);
            int xmax = mmMax(p1.x, p2.x);
            if (xmax <= z->Z.xll || xmin >= z->Z.xlr)
                return;

            glZlist3d *zx = new glZlist3d(&z->Z);
            zx->next = z->next;
            z->next = zx;
            z->Z.yu = y;
            zx->Z.yl = y;
        }
    }


    // If line p1,p2 is vertical and intersects z->Z, split the
    // (Manhattan) zoid and link in the new piece after z.
    //
    void cut_zoid_v(glZlist3d *z, Point &p1, Point &p2)
    {
        if (p1.x == p2.x) {
            int x = p1.x;
            if (x <= z->Z.xll || x >= z->Z.xur)
                return;
            int yu = mmMax(p1.y, p2.y);
            int yl = mmMin(p1.y, p2.y);
            if (yl >= z->Z.yu || yu <= z->Z.yl)
                return;

            glZlist3d *zx = new glZlist3d(&z->Z);
            zx->next = z->next;
            z->next = zx;
            z->Z.xlr = z->Z.xur = x;
            zx->Z.xll = zx->Z.xul = x;
        }
    }
#endif


    // Split a terminal name into port name and suffix.  Both returns
    // are copies, or null.
    //
    void splitname(const char *name, char **pname, char **suffix)
    {
        if (name) {
            while (isspace(*name))
                name++;
        }
        if (!name || !*name) {
            *pname = 0;
            *suffix = 0;
            return;
        }

        char *nm = lstring::copy(name);
        for (char *s = nm+1; *s; s++) {
            if (isspace(*s) || ispunct(*s)) {
                while (isspace(*s) || ispunct(*s))
                    *s++ = 0;
                *pname = nm;
                if (*s)
                    *suffix = lstring::copy(s);
                else
                    *suffix = 0;
                return;
            }
        }
        *pname = nm;
        int len = strlen(nm);
        if (len >= 2) {
            char *s = nm + len - 1;
            *suffix = lstring::copy(s);
            *s = 0;
        }
        else
            *suffix = 0;
    }
}


namespace {
    // Return the NHINC and RH for the layer.
    //
    void get_nhinc(Layer3d *l, int *pnh, double *prh)
    {
        TechLayerParams *lp = tech_prm(l->layer_desc());
        if (pnh) {
            int nh = lp->fh_nhinc();
            if (nh >= FH_MIN_DEF_NHINC && nh <= FH_MAX_DEF_NHINC) {
                if (CDvdb()->getVariable(VA_FhOverride)) {
                    const char *str = CDvdb()->getVariable(VA_FhDefNhinc);
                    if (str) {
                        int nht = atoi(str);
                        if (nht >= FH_MIN_DEF_NHINC && nht <= FH_MAX_DEF_NHINC)
                            nh = nht;
                    }
                }
            }
            else {
                nh = DEF_FH_NHINC;
                const char *str = CDvdb()->getVariable(VA_FhDefNhinc);
                if (str) {
                    int nht = atoi(str);
                    if (nht >= FH_MIN_DEF_NHINC && nht <= FH_MAX_DEF_NHINC)
                        nh = nht;
                }
            }
            *pnh = nh;
        }

        if (prh) {
            double rh = lp->fh_rh();
            if (rh >= FH_MIN_DEF_RH && rh <= FH_MAX_DEF_RH) {
                if (CDvdb()->getVariable(VA_FhOverride)) {
                    const char *str = CDvdb()->getVariable(VA_FhDefRh);
                    if (str) {
                        double rht = atof(str);
                        if (rht >= FH_MIN_DEF_RH && rht <= FH_MAX_DEF_RH)
                            rh = rht;
                    }
                }
            }
            else {
                rh = DEF_FH_RH;
                const char *str = CDvdb()->getVariable(VA_FhDefRh);
                if (str) {
                    double rht = atof(str);
                    if (rht >= FH_MIN_DEF_RH && rht <= FH_MAX_DEF_RH)
                        rh = rht;
                }
            }
            *prh = rh;
        }
    }
}


// Create the segments, which will also create the nodes.  The nodes
// are associated with the terminals.
//
bool
fhLayout::setup()
{
    if (!num_layers())
        return (false);

//XXX document this!
    // Create debugging layers/zoids if set.
    fhl_zoids = CDvdb()->getVariable(VA_FhZoids);

    // We keep all edges smaller than this length, for crude segment
    // refinement.  If 0, no refinement is done beyond the initial
    // splitting for tiling.
    int max_rect_size = 0;
    int min_rect_size = 0;

    const char *var = CDvdb()->getVariable(VA_FhVolElEnable);
    if (var) {

        var = CDvdb()->getVariable(VA_FhVolElTarget);
        double val = var ? atof(var) : FH_DEF_VOLEL_TARG;
        if (val < FH_MIN_VOLEL_TARG)
            val = FH_MIN_VOLEL_TARG;
        else if (val > FH_MAX_VOLEL_TARG)
            val = FH_MAX_VOLEL_TARG;
        double a = 0.0;
        for (Layer3d *l = layers(); l; l = l->next()) {
            if (l->is_conductor()) {
                int nhinc;
                get_nhinc(l, &nhinc, 0);
                DspLayerParams *dp = dsp_prm(l->layer_desc());
                a += nhinc*glYlist3d::volume(l->yl3d())/dp->thickness();
            }
        }
        max_rect_size = INTERNAL_UNITS(sqrt(a/val));

        var = CDvdb()->getVariable(VA_FhVolElMin);
        val = var ? atof(var) : FH_DEF_VOLEL_MIN;
        if (val < FH_MIN_VOLEL_MIN)
            val = FH_MIN_VOLEL_MIN;
        if (val > FH_MAX_VOLEL_MIN)
            val = FH_MAX_VOLEL_MIN;
        min_rect_size = val*max_rect_size;

        TPRINT("Area %g, FhVolElTarget %s, max_rect_size %d min_rect_size %d\n",
            a, var, max_rect_size, min_rect_size);
    }

    // All geometry has been Manhattanized.

    // Create the arrays.
    fhl_layers = new fhLayer[num_layers()];

    // Create lists of conducting objects.
    for (Layer3d *l = layers(); l; l = l->next()) {
        if (l->is_conductor()) {
            fhLayer *cl = &fhl_layers[l->index()];
            cl->set_layer(l);
            cl->clear_list();
            cl->addz3d(l->yl3d());

            // Fill in the cached material properties.
            double sigma = get_sigma(l->layer_desc());
            double tau = tech_prm(l->layer_desc())->tau();
            double lambda = tech_prm(l->layer_desc())->lambda();
            if (lambda < 0.0)
                lambda = 0.0;
            for (fhConductor *c = cl->cndlist(); c; c = c->next()) {
                c->set_sigma(sigma);
                c->set_tau(tau);
                c->set_lambda(lambda);
            }
        }
    }

    slice_groups(max_rect_size, min_rect_size);

#ifdef XXX_NOTUSED
    // Cut at outside edges of other objects, along the long
    // dimension first, then the short dimension.
    //
    for (Layer3d *l1 = layers(); l1; l1 = l1->next()) {
        if (!l1->is_conductor())
            continue;
        fhLayer *cl1 = &fhl_layers[l1->index()];
        for (fhConductor *c1 = cl1->cndlist(); c1; c1 = c1->next()) {
            glZlist3d *z0 = c1->zlist3d();
            for (Layer3d *l2 = layers(); l2; l2 = l2->next()) {
                if (!l2->is_conductor())
                    continue;
                fhLayer *cl2 = &fhl_layers[l2->index()];
                if (cl2 == cl1)
                    continue;
                for (fhConductor *c2 = cl2->cndlist(); c2; c2 = c2->next()) {
                    PolyList *pl = c2->polylist();
                    for (glZlist3d *z = z0; z; z = z->next) {
                        for (PolyList *p = pl; p; p = p->next) {
                            for (int i = 1; i < p->po.numpts; i++) {
                                if (z->Z.xlr - z->Z.xll > z->Z.yu - z->Z.yl) {
                                    cut_zoid_h(z, p->po.points[i-1],
                                        p->po.points[i]);
                                }
                                else {
                                    cut_zoid_v(z, p->po.points[i-1],
                                        p->po.points[i]);
                                }
                            }
                        }
                    }
                    PolyList::destroy(pl);
                }
            }
            for (Layer3d *l2 = layers(); l2; l2 = l2->next()) {
                if (!l2->is_conductor())
                    continue;
                fhLayer *cl2 = &fhl_layers[l2->index()];
                if (cl2 == cl1)
                    continue;
                for (fhConductor *c2 = cl2->cndlist(); c2; c2 = c2->next()) {
                    PolyList *pl = c2->polylist();
                    for (glZlist3d *z = z0; z; z = z->next) {
                        for (PolyList *p = pl; p; p = p->next) {
                            for (int i = 1; i < p->po.numpts; i++) {
                                if (z->Z.xlr - z->Z.xll > z->Z.yu - z->Z.yl) {
                                    cut_zoid_v(z, p->po.points[i-1],
                                        p->po.points[i]);
                                }
                                else {
                                    cut_zoid_h(z, p->po.points[i-1],
                                        p->po.points[i]);
                                }
                            }
                        }
                    }
                    PolyList::destroy(pl);
                }
            }
        }
    }

    // Do the self-cutting again, to propagate new boundaries.
    //
    slice_groups(0, 0);
#endif
    slice_groups_z(max_rect_size);

    if (db3_logfp) {
        int ztot = 0;
        for (Layer3d *l = layers(); l; l = l->next()) {
            if (l->is_conductor()) {
                fhLayer *cl = &fhl_layers[l->index()];
                int cnum = 0;
                for (fhConductor *cd = cl->cndlist(); cd; cd = cd->next()) {
                    int nz = 0;
                    for (glZlist3d *z = cd->zlist3d(); z; z = z->next)
                        nz++;
                    TPRINT("%-8s g=%-4d c=%-2d zcnt=%d\n",
                        l->layer_desc()->name(), cd->group(), cnum, nz);
                    cnum++;
                    ztot += nz;
                }
            }
        }
        TPRINT("Total zcnt=%d\n", ztot);
    }

    // Create the segments, this also creates the nodes.
    for (Layer3d *l = layers(); l; l = l->next()) {
        if (l->is_conductor()) {
            fhLayer *cl = &fhl_layers[l->index()];
            for (fhConductor *cd = cl->cndlist(); cd; cd = cd->next())
                cd->segmentize(fhl_ngen);
        }
    }
    if (db3_logfp) {
        TPRINT("Nodes allocated=%d\n", fhl_ngen.allocated());
    }

    // Create terminals array.
    fhl_terms = new fhTermList*[num_groups()];
    for (int i = 0; i < num_groups(); i++)
        fhl_terms[i] = 0;

    // Hunt for terminal instances.  A terminal instance is a box or
    // polygon on a layer with layer name of a conductor and purpose
    // "fhterm".  The figure must intersect a label on the same LPP
    // that specifies the terminal name.  Shapes are clipped by the
    // masking layer.
    //
    // Nodes on the conductor/layer that touch the figure are all
    // equivalenced together.  The same collection can be used multiple
    // times if there are multiple name labels.
    //
    // Each FastHenry conductor can have an arbitrary number of terminal
    // pairs.  Pairs are identified by name: the last char of the name
    // is used for ordering, other chars must match in the pair but be
    // unique in the conductor.

    char buf[256];
    bool err = false;
    for (Layer3d *l = layers(); l; l = l->next()) {
        if (l->is_conductor()) {
            snprintf(buf, sizeof(buf), "%s:%s",
                l->layer_desc()->oaLayerName(), "fhterm");
            CDl *fld = CDldb()->findLayer(buf, Physical);
            if (!fld)
                continue;
            CDs *sd = CurCell(Physical);
            XIrt xret;
            Zlist *zl = sd->getZlist(CDMAXCALLDEPTH, fld, ref_zlist(), &xret);
            if (!zl)
                continue;
            Zgroup *zg = Zlist::group(zl, 0);

            for (int i = 0; i < zg->num; i++) {
                Zlist *z = zg->list[i];
                zg->list[i] = 0;
                PolyList *p0 = Zlist::to_poly_list(z);
                if (!p0)
                    continue;

                BBox BB;
                p0->po.computeBB(&BB);
                sPF pfg(sd, &BB, fld, CDMAXCALLDEPTH);
                pfg.set_returned("l");
                CDo *od;
                int ccnt = 0;
                while ((od = pfg.next(false, false)) != 0) {
                    if (!p0->po.intersect(&od->oBB(), false)) {
                        delete od;
                        continue;
                    }
                    CDo *odla = od->next_odesc();
                    if (!odla || odla->type() != CDLABEL) {
                        delete od;
                        continue;
                    }
                    char *text = hyList::string(((CDla*)odla)->label(),
                        HYcvPlain, false);
                    char *portname, *sfx;
                    splitname(text, &portname, &sfx);
                    delete [] text;
                    if (portname && sfx) {
                        add_terminal(&p0->po, portname, sfx, l->index(),
                            ccnt++);
                    }
                    else {
                        Errs()->add_error(
                            "Error: bad terminal label string %s, "
                            "layer %s.\n",
                            text && *text ? text : "empty",
                            l->layer_desc()->name());
                        err = true;
                    }
                    delete od;
                }
                PolyList::destroy(p0);
            }
            delete zg;
        }
    }
    if (err)
        return (false);

    // Set the terminal nodes.
    //
    for (int i = 0; i < num_groups(); i++) {
        fhTermList *tp = 0, *tn;
        for (fhTermList *t = fhl_terms[i]; t; t = tn) {
            tn = t->next();
            fhConductor *c = find_conductor(t->lnum(), t->cnum());
            if (!c) {
                // should never happen
                if (tp)
                    tp->set_next(tn);
                else
                    fhl_terms[i] = tn;
                delete t;
                continue;
            }
            t->set_nodes(c->get_nodes(fhl_ngen, t->num_points(), t->points()));
            tp = t;
        }
    }

    char *s = check_sort_terms();
    if (s) {
        Errs()->add_error(s);
        delete [] s;
        return (false);
    }

    if (fhl_zoids) {
        for (Layer3d *l = layers(); l; l = l->next()) {
            if (l->is_conductor()) {
                fhLayer *cl = &fhl_layers[l->index()];
                for (fhConductor *c = cl->cndlist(); c; c = c->next())
                    c->save_dbg_zlist();
            }
        }
    }
    return (true);
}


// Dump a FastHenry input file.
//
bool
fhLayout::fh_dump(FILE *fp)
{
    if (!fp)
        return (false);
    fprintf(fp, "** FastHenry input\n");
    fprintf(fp, "** Generated by %s\n", XM()->IdString());

    const char *ustring = CDvdb()->getVariable(VA_FhUnits);
    int u = ustring ? unit_t::find_unit(ustring) : FX_DEF_UNITS;
    if (u < 0) {
        Log()->WarningLogV(mh::Initialization,
            "Unknown units \"%s\", set to default.", ustring);
        u = FX_DEF_UNITS;
    }
    e_unit unit = (e_unit)u;

    fprintf(fp, ".Units %s\n", unit_t::units(unit)->name());
    const char *defs = CDvdb()->getVariable(VA_FhDefaults);
    if (defs && *defs)
        fprintf(fp, ".Default %s\n", defs);

    fprintf(fp, "\n");
    layer_dump(fp);
    fprintf(fp, "\n");

    int nnodes = fhl_ngen.fh_nodes_print(fp, unit);
    int nsegs = 0;
    for (Layer3d *l = layers(); l; l = l->next()) {
        if (l->is_conductor()) {
            fhLayer *cl = &fhl_layers[l->index()];
            int cnum = 0;
            for (fhConductor *cd = cl->cndlist(); cd; cd = cd->next()) {
                fprintf(fp, "\n");
                fprintf(fp,
                    "*Segments of conductor element %d, group %d, layer %s.\n",
                    cnum, cd->group(), l->layer_desc()->name());
                nsegs += cd->segments_print(fp, unit, this);
            }
        }
    }
    fprintf(fp, "\n");
    TPRINT("Wrote %d nodes, %d segments.\n", nnodes, nsegs);

    // Tie together nodes in each terminal
    for (int i = 0; i < num_groups(); i++) {
        for (fhTermList *t = fhl_terms[i]; t; t = t->next()) {
            if (!t->nodes())
                continue;
            if (t->ccnt() > 0)
                continue;
            for (fhNodeList *nl = t->nodes()->next; nl; nl = nl->next) {
                fprintf(fp, ".Equiv N%d N%d\n", t->nodes()->nd->number(),
                    nl->nd->number());
            }
        }
    }

    // Print externals
    dump_ports(fp);

    char *smin, *smax, *sdec;
    if (get_freq_spec(&smin, &smax, &sdec)) {
        if (*sdec)
            fprintf(fp, ".Freq fmin=%s fmax=%s ndec=%s\n", smin, smax, sdec);
        else
            fprintf(fp, ".Freq fmin=%s fmax=%s\n", smin, smax);
    }
    else {
        if (*sdec)
            fprintf(fp, "* .Freq fmin=%s fmax=%s ndec=%s\n", smin, smax, sdec);
        else
            fprintf(fp, "* .Freq fmin=%s fmax=%s\n", smin, smax);
    }
    delete [] smin;
    delete [] smax;
    delete [] sdec;

    fprintf(fp, ".End\n");
    return (true);
}


namespace {
    // Create a sorted, monotonically increasing array of values from
    // the hash table.
    //
    void sort_points(SymTab *tab, int **ppts, int *pnp)
    {
        int *pts = new int[tab->allocated() + 1];
        SymTabEnt *ent;
        SymTabGen gen(tab);
        int cnt = 0;
        while ((ent = gen.next()) != 0)
            pts[cnt++] = (int)(uintptr_t)ent->stTag;
        std::sort(pts, pts + cnt);
        *ppts = pts;
        *pnp = cnt;
    }


    // Throw out values that are spaced less thjan mindim.
    //
    void filter(int *ppts, int *pnp, int mindim)
    {
        if (mindim < 10)
            return;

        int np = *pnp;
        int npn = np;
        int *pp = ppts;
        for (int i = 1; i < np; i++) {
            int d = ppts[i] - *pp;
            if (d < mindim) {
                npn--;
                continue;
            }
            pp++;
            if (npn != np)
                *pp = ppts[i];
        }
        *pnp = npn;
    }


    // For positive maxdim, resize the array inserting intermediate
    // points so that no delta is larger than maxdim.
    //
    void expand(int **ppts, int *pnp, int maxdim)
    {
        if (maxdim <= 0)
            return;

        int sz = 0;
        int npm1 = *pnp - 1;
        int *oldpts = *ppts;
        for (int i = 0; i < npm1; i++) {
            int d = oldpts[i+1] - oldpts[i];
            sz += d/maxdim + 1;
        }
        sz++;
        int *newpts = new int[sz];
        int npcnt = 0;
        for (int i = 0; i < npm1; i++) {
            int k1 = oldpts[i];
            int k2 = oldpts[i+1];
            int d = k2 - k1;
            int dk = d / maxdim;
            if (d % maxdim)
                dk++;

            newpts[npcnt++] = k1;
            if (dk > 1) {
                d /= dk;
                dk--;
                for (int j = 0; j < dk; j++) {
                    k1 += d;
                    newpts[npcnt++] = k1;
                }
            }
        }
        newpts[npcnt++] = oldpts[npm1];
        delete [] *ppts;
        *ppts = newpts;
        *pnp = npcnt;
    }
}


void
fhLayout::slice_groups(int maxdim, int mindim)
{
    SymTab xtab(false, false), ytab(false, false);
    for (int n = 0; n < num_layers(); n++) {
        Layer3d *l = fhl_layers[n].layer();
        if (!l || !l->is_conductor())
            continue;

        for (fhConductor *c = fhl_layers[n].cndlist(); c; c = c->next())
            c->accum_points(&xtab, &ytab);
    }

    int *xpts, nx;
    int *ypts, ny;
    sort_points(&xtab, &xpts, &nx);
    sort_points(&ytab, &ypts, &ny);
    filter(xpts, &nx, mindim);
    filter(ypts, &ny, mindim);
    expand(&xpts, &nx, maxdim);
    expand(&ypts, &ny, maxdim);

    for (int n = 0; n < num_layers(); n++) {
        Layer3d *l = fhl_layers[n].layer();
        if (!l || !l->is_conductor())
            continue;

        for (fhConductor *c = fhl_layers[n].cndlist(); c; c = c->next())
            c->split(xpts, nx, ypts, ny);
    }

    delete [] xpts;
    delete [] ypts;
}


namespace {
    // Return an array of nh thicknesses, with adjacent thickness
    // ratio r, normalized such that the total thickness = 1.0.  This
    // is used to set up the "nhinc" slicing.
    //
    double *get_ztab(int nh, double r)
    {
        if (nh < 1)
            return (0);
        if (nh == 1) {
            double *a = new double[1];
            a[0] = 1.0;
            return (a);
        }

        if (r <= 0.0)
            r = 2.0;

        // Compute t = total thickness in units of the surface filament.
        double t = 0.0;
        int n2 = nh/2;
        double nn = 1.0;
        for (int i = 0; i < n2; i++) {
            t += nn;
            nn *= r;
        }
        t += t;
        if (nh & 1)
            t += nn;

        // Load and return the thickness values.
        double *a = new double[nh];
        nn = 1.0;
        for (int i = 0; i < n2; i++) {
            a[i] = nn/t;
            a[nh-i-1] = a[i];
            nn *= r;
        }
        if (nh & 1)
            a[n2] = nn/t;
        return (a);
    }
}


void
fhLayout::slice_groups_z(int maxdim)
{
    bool fhfils = CDvdb()->getVariable(VA_FhUseFilament);
    for (int n = 0; n < num_layers(); n++) {
        Layer3d *l = fhl_layers[n].layer();
        if (!l || !l->is_conductor())
            continue;

        SymTab ztab(false, false);

        if (!fhfils && l->plane() > 0) {
            int nhinc;
            double rh;
            get_nhinc(l, &nhinc, &rh);
            if (nhinc > 1) {
                double th = dsp_prm(l->layer_desc())->thickness();
                double *a = get_ztab(nhinc, rh);
                ztab.add((uintptr_t)l->plane(), 0, true);
                double h = MICRONS(l->plane());
                for (int i = 0; i < nhinc; i++) {
                    h += th*a[i];
                    ztab.add((uintptr_t)INTERNAL_UNITS(h), 0, true);
                }
                delete [] a;
            }
        }

        for (fhConductor *c = fhl_layers[n].cndlist(); c; c = c->next())
            c->accum_points_z(&ztab);

        int *zpts, nz;
        sort_points(&ztab, &zpts, &nz);
        expand(&zpts, &nz, maxdim);

        for (fhConductor *c = fhl_layers[n].cndlist(); c; c = c->next())
            c->split_z(zpts, nz);

        delete [] zpts;
    }
}


// Return the conductor indicated by the two indices.
//
fhConductor *
fhLayout::find_conductor(int lnum, int cnum)
{
    for (Layer3d *l = layers(); l; l = l->next()) {
        if (l->is_conductor() && l->index() == lnum) {
            fhLayer *cl = &fhl_layers[l->index()];
            int cc = 0;
            for (fhConductor *c = cl->cndlist(); c; c = c->next(), cc++) {
                if (cc == cnum)
                    return (c);
            }
            break;
        }
    }
    return (0);
}


// Add a new terminal to a conductor with layer index lnum.
//
void
fhLayout::add_terminal(const Poly *po, const char *portname, const char *sfx,
    int lnum, int ccnt)
{
    BBox tBB;
    po->computeBB(&tBB);
    bool isrc = po->is_rect();
    int cnum = 0;
    fhLayer *cl = &fhl_layers[lnum];
    for (fhConductor *c = cl->cndlist(); c; c = c->next(), cnum++) {
        for (glZlist3d *z = c->zlist3d(); z; z = z->next) {
            if (z->Z.Zoid::intersect(&tBB, false)) {
                if (isrc) {
                    int grp = c->group();
                    fhl_terms[grp] = new fhTermList(&tBB, portname, sfx, lnum,
                        cnum, ccnt, fhl_terms[grp]);
                    return;
                }
                else {
                    Zlist *z0 = po->toZlist();
                    for (Zlist *zl = z0; zl; zl = zl->next) {
                        if (z->Z.Zoid::intersect(&zl->Z, false)) {
                            Zlist::destroy(z0);
                            int grp = c->group();
                            fhl_terms[grp] = new fhTermList(po, portname, sfx,
                                lnum, cnum, ccnt, fhl_terms[grp]);
                            return;
                        }
                    }
                    Zlist::destroy(z0);
                }
            }
        }
    }
}


namespace {
    inline bool tcomp(const fhTermList *t1, const fhTermList *t2)
    {
        const char *p1 = t1->portname();
        const char *p2 = t2->portname();
        int n = strcmp(p1, p2);
        if (n < 0)
            return (true);
        if (n > 0)
            return (false);
        return (strcmp(t1->suffix(), t2->suffix()) < 0);
    }


    // Sort the terminal list alphabetically, case sensitive.  There
    // should be an even number of terminals, and pairs of terminals
    // should share a uniqe base name, and the last character should
    // set ordering.  Thus pairs should be grouped after sorting.
    //
    void termsort(fhTermList **listp)
    {
        int cnt = 0;
        for (fhTermList *t = *listp; t; t = t->next())
            cnt++;
        if (cnt < 2)
            return;
        fhTermList **ary = new fhTermList*[cnt];
        cnt = 0;
        for (fhTermList *t = *listp; t; t = t->next())
            ary[cnt++] = t;
        std::sort(ary, ary + cnt, tcomp);
        for  (int i = 1; i < cnt; i++)
            ary[i-1]->set_next(ary[i]);
        ary[cnt-1]->set_next(0);
        *listp = ary[0];
        delete [] ary;
    }
}


// Sort the terminals, then check that they have nodes and have names
// that allow association into ports properly.  Null is returned if no
// errors, otherwise an error message is returned, caller should free.
//
char *
fhLayout::check_sort_terms()
{
    // Sort the terminal lists.  Badly-formed names have already been
    // purged.
    for (int i = 0; i < num_groups(); i++)
        termsort(&fhl_terms[i]);

    char buf[256];
    sLstr lstr;
    int nports = 0;
    bool err = false;
    for (int i = 0; i < num_groups(); i++) {
        if (!fhl_terms[i])
            continue;

        // Make sure that each terminal has a node.
        for (fhTermList *t = fhl_terms[i]; t; t = t->next()) {
            if (!t->nodes()) {
                snprintf(buf, sizeof(buf),
                    "Error: terminal without node, name %s %s layer %s "
                    "group %d.\n", t->portname(), t->suffix(),
                    layer(t->lnum())->layer_desc()->name(), i);
                lstr.add(buf);
                err = true;
            }
        }

        // Check that the names are correct.
        fhTermList *tn, *tnn;
        for (fhTermList *t = fhl_terms[i]; t; t = tnn) {
            tn = t->next();
            if (!tn) {
                snprintf(buf, sizeof(buf),
                    "Error: singleton terminal, name %s %s layer %s "
                    "group %d.\n", t->portname(), t->suffix(),
                    layer(t->lnum())->layer_desc()->name(), i);
                lstr.add(buf);
                err = true;
                break;
            }
            tnn = tn->next();
            if (strcmp(t->portname(), tn->portname())) {
                snprintf(buf, sizeof(buf),
                    "Error: singleton terminal, name %s %s layer %s "
                    "group %d.\n", t->portname(), t->suffix(),
                    layer(t->lnum())->layer_desc()->name(), i);
                lstr.add(buf);
                err = true;
                tnn = tn;
                continue;
            }
            if (tnn && !strcmp(t->portname(), tnn->portname())) {
                snprintf(buf, sizeof(buf),
                    "Error: redundant port name %s.\n", t->portname());
                lstr.add(buf);
                err = true;
                continue;
            }
            if (!strcmp(t->suffix(), tn->suffix())) {
                snprintf(buf, sizeof(buf),
                    "Error: redundant suffix, name %s %s layer %s "
                    "group %d.\n", t->portname(), t->suffix(),
                    layer(t->lnum())->layer_desc()->name(), i);
                lstr.add(buf);
                err = true;
                continue;
            }
            nports++;
        }
    }

    if (!nports && !err) {
        strcpy(buf, "Error: port count is 0.\n");
        lstr.add(buf);
        err = true;
    }
    return (lstr.string_trim());
}


// Dump the port specifications.  We've already checked that all is
// well with the terminals.
//
void
fhLayout::dump_ports(FILE *fp)
{
    if (!fp)
        return;
    for (int i = 0; i < num_groups(); i++) {
        if (!fhl_terms[i])
            continue;

        fhTermList *tn, *tnn;
        for (fhTermList *t = fhl_terms[i]; t; t = tnn) {
            tn = t->next();
            tnn = tn->next();
            fprintf(fp, ".external N%d N%d %s\n",
                t->nodes()->nd->number(), tn->nodes()->nd->number(),
                t->portname());
        }
    }
}
// End of fhLayout functions.


// Static function.
fhConductor *
fhConductor::addz3d(fhConductor *thisc, const glZoid3d *Z)
{
    if (!Z)
        return (thisc);
    fhConductor *cl0 = thisc;
    for (fhConductor *cl = cl0; cl; cl = cl->next()) {
        if (cl->layer_index() == Z->layer_index && cl->group() == Z->group) {
            cl->hc_zlist3d_ref = new glZlistRef3d(Z, cl->hc_zlist3d_ref);
            cl->hc_zlist3d = new glZlist3d(Z, cl->hc_zlist3d);
            return (cl0);
        }
    }
    fhConductor *c = new fhConductor(Z->layer_index, Z->group);
    c->set_next(cl0);
    cl0 = c;
    c->set_zlist3d_ref(new glZlistRef3d(Z, 0));
    c->set_zlist3d(new glZlist3d(Z, 0));
    return (cl0);
}


// Return the nodes in BB.
//
fhNodeList *
fhConductor::get_nodes(const fhNodeGen &ngen, int numpts, const Point *pts)
{
    // Make sure all returned nodes are actually referenced by a
    // segment.
    SymTab *st = new SymTab(false, false);
    for (fhSegment *s = hc_segments; s; s = s->next()) {
        st->add(s->node1(), s, true);
        st->add(s->node2(), s, true);
    }

    fhNodeList *nodes = ngen.find_nodes(numpts, pts);
    fhNodeList *np = 0, *nn;
    for (fhNodeList *n = nodes; n; n = nn) {
        nn = n->next;
        if (SymTab::get(st, n->nd->number()) == ST_NIL) {
            if (np)
                np->next = nn;
            else
                nodes = nn;
            delete n;
            continue;
        }
        np = n;
    }
    delete st;
    return (nodes);
}


void
fhConductor::segmentize(fhNodeGen &ngen)
{
    // Each rectangle is partitioned into six segments, each ending at
    // the midpoint.  There are seven nodes, one at the midpoint of
    // each edge and one in the center.  Given the way that the figure
    // has been partitioned, the nodes of touching pieces will
    // coincide.
    //
    fhSegment *s0 = 0;
    for (glZlist3d *zx = hc_zlist3d; zx; zx = zx->next) {
        int x = (zx->Z.xll + zx->Z.xlr)/2;
        int y = (zx->Z.yl + zx->Z.yu)/2;
        int z = (zx->Z.zbot + zx->Z.ztop)/2;

        int dx = zx->Z.xlr - zx->Z.xll;
        int dy = zx->Z.yu - zx->Z.yl;
        int dz = zx->Z.ztop - zx->Z.zbot;

        xyz3d p1 = xyz3d(zx->Z.xll, y, z);
        xyz3d p2 = xyz3d(x, y, z);
        s0 = new fhSegment(ngen.newnode(&p1), ngen.newnode(&p2), &p1, &p2,
            dy, dz, s0);
        p1 = p2;
        p2 = xyz3d(zx->Z.xlr, y, z);
        s0 = new fhSegment(ngen.newnode(&p1), ngen.newnode(&p2), &p1, &p2,
            dy, dz, s0);

        p1 = xyz3d(x, zx->Z.yl, z);
        p2 = xyz3d(x, y, z);
        s0 = new fhSegment(ngen.newnode(&p1), ngen.newnode(&p2), &p1, &p2,
            dx, dz, s0);
        p1 = p2;
        p2 = xyz3d(x, zx->Z.yu, z);
        s0 = new fhSegment(ngen.newnode(&p1), ngen.newnode(&p2), &p1, &p2,
            dx, dz, s0);

        p1 = xyz3d(x, y, zx->Z.zbot);
        p2 = xyz3d(x, y, z);
        s0 = new fhSegment(ngen.newnode(&p1), ngen.newnode(&p2), &p1, &p2,
            dx, dy, s0);
        p1 = p2;
        p2 = xyz3d(x, y, zx->Z.ztop);
        s0 = new fhSegment(ngen.newnode(&p1), ngen.newnode(&p2), &p1, &p2,
            dx, dy, s0);
    }
    hc_segments = s0;
}


// Print the FastHenry segment definition.
//
int
fhConductor::segments_print(FILE *fp, e_unit unit, const fhLayout *fhl)
{
    char buf[128];
    buf[0] = 0;
    const unit_t *u = unit_t::units(unit);
    if (hc_sigma > 0.0) {
        snprintf(buf, sizeof(buf), " sigma= %g", hc_sigma*u->sigma_factor());
        if (hc_tau > 0.0 && hc_lambda <= 0.0) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, " tau= %g", hc_tau);
        }
    }
    if (hc_lambda > 0) {
        int len = strlen(buf);
        snprintf(buf + len, sizeof(buf) - len, " lambda= %g",
            hc_lambda*u->lambda_factor());
    }
    double sc = u->coord_factor();
    const char *ff = u->float_format();
    char tbuf[64];
    int cnt = 0;

    Layer3d *l = (fhl->fhlayers() + hc_layer_ix)->layer();
    DspLayerParams *dp = dsp_prm(l->layer_desc());
    int thickness = INTERNAL_UNITS(dp->thickness());
    int nhinc = 0;
    double rh = 0.0;
    if (CDvdb()->getVariable(VA_FhUseFilament))
        get_nhinc(l, &nhinc, &rh);

    for (fhSegment *s = hc_segments; s; s = s->next()) {
        fhNode *n1 = fhl->nodes()->find_node(s->node1(), s->pn1());
        if (!n1 || n1->refcnt() < 2)
            continue;
        fhNode *n2 = fhl->nodes()->find_node(s->node2(), s->pn2());
        if (!n2 || n2->refcnt() < 2)
            continue;
        snprintf(tbuf, sizeof(tbuf), "En%dn%d", s->node1(), s->node2());
        fprintf(fp, "%-12s ", tbuf);
        fprintf(fp, "N%-4d N%-4d", s->node1(), s->node2());
        fprintf(fp, " w=");
        fprintf(fp, ff, sc*s->wid());
        fprintf(fp, " h=");
        fprintf(fp, ff, sc*s->hei());
        if (nhinc > 1 && s->pn1()->z == s->pn2()->z &&
                INTERNAL_UNITS(sc*s->hei()) == thickness) {
            fprintf(fp, " nhinc=%d", nhinc);
            if (rh != DEF_FH_RH && rh > 0.0)
                fprintf(fp, " rh=%g", rh);
        }

        fprintf(fp, "%s\n", buf);
        cnt++;
    }
    return (cnt);
}


// Split each zoid that overlaps BB into a 3X3 array of zoids (each zoid
// is Manhattan).  Have to split by an odd number to keep the midpoint
// constant.
//
bool
fhConductor::refine(const BBox *BB)
{
    glZlist3d *z0 = 0;
    glZlist3d *zn, *zp = 0;
    for (glZlist3d *z = hc_zlist3d; z; z = zn) {
        zn = z->next;
        if (z->Z.Zoid::intersect(BB, false)) {
            if (!zp)
                hc_zlist3d = zn;
            else
                zp->next = zn;
            z->next = z0;
            z0 = z;
            continue;
        }
        zp = z;
    }
    if (!z0)
        return (false);
    for (glZlist3d *z = z0; z; z = z->next) {

        int dx = (z->Z.xlr - z->Z.xll)/3;
        int dy = (z->Z.yu - z->Z.yl)/3;

        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xlr = hc_zlist3d->Z.xur = z->Z.xll + dx;
        hc_zlist3d->Z.yu = z->Z.yl + dy;
        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xll = hc_zlist3d->Z.xul = z->Z.xll + dx;
        hc_zlist3d->Z.xlr = hc_zlist3d->Z.xur = z->Z.xlr - dx;
        hc_zlist3d->Z.yu = z->Z.yl + dy;
        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xll = hc_zlist3d->Z.xul = z->Z.xlr - dx;
        hc_zlist3d->Z.yu = z->Z.yl + dy;

        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xlr = hc_zlist3d->Z.xur = z->Z.xll + dx;
        hc_zlist3d->Z.yl = z->Z.yl + dy;
        hc_zlist3d->Z.yu = z->Z.yu - dy;
        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xll = hc_zlist3d->Z.xul = z->Z.xll + dx;
        hc_zlist3d->Z.xlr = hc_zlist3d->Z.xur = z->Z.xlr - dx;
        hc_zlist3d->Z.yl = z->Z.yl + dy;
        hc_zlist3d->Z.yu = z->Z.yu - dy;
        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xll = hc_zlist3d->Z.xul = z->Z.xlr - dx;
        hc_zlist3d->Z.yl = z->Z.yl + dy;
        hc_zlist3d->Z.yu = z->Z.yu - dy;

        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xlr = hc_zlist3d->Z.xur = z->Z.xll + dx;
        hc_zlist3d->Z.yl = z->Z.yu - dy;
        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xll = hc_zlist3d->Z.xul = z->Z.xll + dx;
        hc_zlist3d->Z.xlr = hc_zlist3d->Z.xur = z->Z.xlr - dx;
        hc_zlist3d->Z.yl = z->Z.yu - dy;
        hc_zlist3d = new glZlist3d(&z->Z, hc_zlist3d);
        hc_zlist3d->Z.xll = hc_zlist3d->Z.xul = z->Z.xlr - dx;
        hc_zlist3d->Z.yl = z->Z.yu - dy;
    }
    glZlist3d::destroy(z0);
    return (true);
}


PolyList *
fhConductor::polylist()
{
    Zlist *z0 = hc_zlist3d_ref->to_zlist();
    return (Zlist::to_poly_list(z0));
}


void
fhConductor::accum_points(SymTab *xtab, SymTab *ytab)
{
    for (const glZlist3d *z = hc_zlist3d; z; z = z->next) {
        xtab->add((uintptr_t)z->Z.xll, 0, true);
        xtab->add((uintptr_t)z->Z.xlr, 0, true);
        ytab->add((uintptr_t)z->Z.yl, 0, true);
        ytab->add((uintptr_t)z->Z.yu, 0, true);
    }
}


void
fhConductor::accum_points_z(SymTab *ztab)
{
    for (const glZlist3d *z = hc_zlist3d; z; z = z->next) {
        ztab->add((uintptr_t)z->Z.zbot, 0, true);
        ztab->add((uintptr_t)z->Z.ztop, 0, true);
    }
}


bool
fhConductor::split(int *xpts, int nx, int *ypts, int ny)
{
    bool sliced = false;

    // Divide each zoid along y for each x in xpts.
    for (glZlist3d *z = hc_zlist3d; z; z = z->next) {
        for (int i = 0; i < nx; i++) {
            if (xpts[i] <= z->Z.xll)
                continue;
            if (xpts[i] >= z->Z.xlr)
                break;
            glZlist3d *zx = new glZlist3d(&z->Z);
            zx->next = z->next;
            z->next = zx;
            z->Z.xlr = z->Z.xur = xpts[i];
            zx->Z.xll = zx->Z.xul = xpts[i];
            sliced = true;
        }
    }

    // Divide each zoid along x for each y in ypts.
    for (glZlist3d *z = hc_zlist3d; z; z = z->next) {
        for (int i = 0; i < ny; i++) {
            if (ypts[i] <= z->Z.yl)
                continue;
            if (ypts[i] >= z->Z.yu)
                break;
            glZlist3d *zx = new glZlist3d(&z->Z);
            zx->next = z->next;
            z->next = zx;
            z->Z.yu = ypts[i];
            zx->Z.yl = ypts[i];
            sliced = true;
        }
    }
    return (sliced);
}


bool
fhConductor::split_z(int *zpts, int nz)
{
    bool sliced = false;

    // Divide each zoid along xy for each z in zpts.
    for (glZlist3d *z = hc_zlist3d; z; z = z->next) {
        for (int i = 0; i < nz; i++) {
            if (zpts[i] <= z->Z.zbot)
                continue;
            if (zpts[i] >= z->Z.ztop)
                break;
            glZlist3d *zx = new glZlist3d(&z->Z);
            zx->next = z->next;
            z->next = zx;
            z->Z.ztop = zpts[i];
            zx->Z.zbot = zpts[i];
            sliced = true;
        }
    }
    return (sliced);
}


namespace {
    stringlist *dbg_layers = 0;
}


// Add the Zlist as objects in the current cell, for debugging.
//
void
fhConductor::save_dbg_zlist()
{
    char buf[64];
    snprintf(buf, sizeof(buf), "G%dL%d:FH", hc_group, hc_layer_ix);
    bool there = false;
    for (stringlist *sl = dbg_layers; sl; sl = sl->next) {
        if (!strcmp(sl->string, buf)) {
            there = true;
            break;
        }
    }
    if (!there)
        dbg_layers = new stringlist(lstring::copy(buf), dbg_layers);
    
    CDl *ld = CDldb()->newLayer(buf, Physical);
    CurCell(Physical)->db_clear_layer(ld);
    Zlist *zl = glZlist3d::to_zlist(hc_zlist3d);
    Zlist::add(zl, CurCell(Physical), ld, false, false);
    Zlist::destroy(zl);
}
// End of fhConductor functions.


// Clear the added objects.
// Static function.
//
void
fhLayout::clear_dbg_zlist()
{
    while (dbg_layers) {
        stringlist *sl = dbg_layers;
        dbg_layers = dbg_layers->next;
        CDl *ld = CDldb()->findLayer(sl->string, Physical);
        if (ld)
            CurCell(Physical)->db_clear_layer(ld);
        delete [] sl->string;
        delete sl;
    }
}


namespace {
    unsigned int nhash(int x, int y, int z, unsigned int mask)
    {
        unsigned int k = INCR_HASH_INIT;
        k = incr_hash(k, &x);
        k = incr_hash(k, &y);
        k = incr_hash(k, &z);
        return (k & mask);
    }
}


// Table max density.
#define FH_NGDENS 5

// Return a node number, check if this node has already been assigned.
//
int
fhNodeGen::newnode(xyz3d *p)
{
    unsigned int h = nhash(p->x, p->y, p->z, ng_mask);
    for (fhNode *n = ng_tab[h]; n; n = n->next()) {
        if (*p == *n->loc()) {
            n->inc_ref();
            return (n->number());
        }
    }
    ng_tab[h] = new fhNode(ng_allocated++, p, ng_tab[h]);
    ng_tab[h]->inc_ref();
    int n = ng_tab[h]->number();
    if (ng_allocated/(ng_mask+1) > FH_NGDENS)
        rehash();
    return (n);
}


// Set BB to enclose all nodes in the x-y plane.
//
void
fhNodeGen::nodeBB(BBox *BB) const
{
    bool first = true;
    for (unsigned int i = 0; i <= ng_mask; i++) {
        for (fhNode *n = ng_tab[i]; n; n = n->next()) {
            if (first) {
                first = false;
                BB->left = BB->right = n->loc()->x;
                BB->bottom = BB->top = n->loc()->y;
                continue;
            }
            if (n->loc()->x < BB->left)
                BB->left = n->loc()->x;
            else if (n->loc()->x > BB->right)
                BB->right = n->loc()->x;
            if (n->loc()->y < BB->bottom)
                BB->bottom = n->loc()->y;
            else if (n->loc()->y > BB->top)
                BB->top = n->loc()->y;
        }
    }
}


// Return a list of the nodes enclosed in or touching the figure
// described by the pts list.
//
fhNodeList *
fhNodeGen::find_nodes(int npts, const Point *pts) const
{
    fhNodeList *n0 = 0;
    if (npts == 2) {
        // This is a rect (L,B), (R,T).
        for (unsigned int i = 0; i <= ng_mask; i++) {
            for (fhNode *n = ng_tab[i]; n; n = n->next()) {
                if (pts[0].x <= n->loc()->x && n->loc()->x <= pts[1].x &&
                        pts[0].y <= n->loc()->y && n->loc()->y <= pts[1].y)
                    n0 = new fhNodeList(n, n0);
            }
        }
    }
    else if (npts >= 4) {
        Poly po(npts, (Point*)pts);  // ugly
        for (unsigned int i = 0; i <= ng_mask; i++) {
            for (fhNode *n = ng_tab[i]; n; n = n->next()) {
                Point_c p(n->loc()->x, n->loc()->y);
                if (po.intersect(&p, true))
                    n0 = new fhNodeList(n, n0);
            }
        }
    }
    return (n0);
}


fhNode *
fhNodeGen::find_node(int num, const xyz3d *p) const
{
    unsigned int h = nhash(p->x, p->y, p->z, ng_mask);
    for (fhNode *n = ng_tab[h]; n; n = n->next()) {
        if (num == (int)n->number() && *p == *n->loc())
            return (n);
    }
    return (0);
}


// Print all assigned nodes in FastHenry format.
//
int
fhNodeGen::fh_nodes_print(FILE *fp, e_unit unit) const
{
    double sc = unit_t::units(unit)->coord_factor();
    const char *ff = unit_t::units(unit)->float_format();
    int cnt = 0;
    for (unsigned int i = 0; i <= ng_mask; i++) {
        for (fhNode *n = ng_tab[i]; n; n = n->next()) {
            if (n->refcnt() > 1) {
                fprintf(fp, "N%-4d", n->number());
                fprintf(fp, " x=");
                fprintf(fp, ff, sc*n->loc()->x);
                fprintf(fp, " y=");
                fprintf(fp, ff, sc*n->loc()->y);
                fprintf(fp, " z=");
                fprintf(fp, ff, sc*n->loc()->z);
                fprintf(fp, "\n");
                cnt++;
            }
        }
    }
    return (cnt);
}


// Clear the node generator.
//
void
fhNodeGen::clear()
{
    for (unsigned int i = 0; i <= ng_mask; i++)
        fhNode::destroy(ng_tab[i]);
    delete [] ng_tab;
    ng_allocated = 0;
    ng_mask = ~((unsigned int)-1 << FH_NDHASHW);
    ng_tab = new fhNode*[ng_mask+1];
    memset(ng_tab, 0, (ng_mask+1)*sizeof(fhNode*));
}


// Grow the hash table width.  This is called after adding elements if
// the element count to width ratio exceeds FH_NGDENS.
//
void
fhNodeGen::rehash()
{
    unsigned int oldmask = ng_mask;
    ng_mask = (oldmask << 1) | 1;
    fhNode **oldent = ng_tab;
    ng_tab = new fhNode*[ng_mask + 1];
    for (unsigned int i = 0; i <= ng_mask; i++)
        ng_tab[i] = 0;
    for (unsigned int i = 0; i <= oldmask; i++) {
        fhNode *hn;
        for (fhNode *h = oldent[i]; h; h = hn) {
            hn = h->next();
            unsigned int j = nhash(h->loc()->x, h->loc()->y, h->loc()->z,
                ng_mask);
            h->set_next(ng_tab[j]);
            ng_tab[j] = h;
        }
    }
    delete [] oldent;
}
// End of fhNodeGen functions.

