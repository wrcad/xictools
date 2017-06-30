
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
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
 $Id: ext_fc.cc,v 1.9 2017/03/14 01:26:38 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_fc.h"
#include "tech.h"
#include "promptline.h"
#include "errorlog.h"
#include "geo_ylist.h"
#include "cd_layer.h"
#include "cd_lgen.h"
#include "cd_propnum.h"
#include "dsp_inlines.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "filestat.h"

//
// A new interface to FasterCap and FastCap-WR.
//


#define DEBUG

namespace { cFC _fc_; }

cFC *cFC::instancePtr = 0;

cFC::cFC()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cFC already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    fc_groups = 0;
    fc_ngroups = 0;
    fc_popup_visible = false;
}


// Private static error exit.
//
void
cFC::on_null_ptr()
{
    fprintf(stderr, "Singleton class cFC used before instantiated.\n");
    exit(1);
}


// Perform an operation, according to the keyword given.
//
void
cFC::doCmd(const char *op, const char *str)
{
    if (!op) {
        PL()->ShowPrompt("Error: no keyword given.");
        return;
    }
    else if (lstring::cieq(op, "dump") || lstring::cieq(op, "dumpc")) {
        char *s1 = lstring::getqtok(&str);
        fcDump(s1);
        delete [] s1;
        PL()->ErasePrompt();
    }
    else if (lstring::cieq(op, "run") || lstring::cieq(op, "runc")) {
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
        fcRun(infile, outfile, resfile, (infile != 0));
        delete [] infile;
        delete [] outfile;
        delete [] resfile;
        PL()->ErasePrompt();
    }
    else
        PL()->ShowPromptV("Error: unknown keyword %s given.", op);
}


// Dump a unified list input file, readable by FasterCap and FastCap-WR.
//
bool
cFC::fcDump(const char *fname)
{
    if (!CurCell(Physical)) {
        Log()->PopUpErr("No current cell!");
        return (false);
    }
    if (!fname || !*fname)
        fname = getFileName(FC_LST_SFX);
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

    fcLayout fcl;

    // Specify a masking layer.  If it exists in the cell, then it
    // clips the extracted geometry.
    const char *fcap = CDvdb()->getVariable(VA_FcLayerName);
    if (!fcap)
        fcap = FC_LAYER_NAME;
    bool ret = fcl.init_for_extraction(CurCell(Physical), 0, fcap,
        Tech()->SubstrateEps(), Tech()->SubstrateThickness());
    if (ret)
        ret = fcl.check_dielectrics();
    if (!ret) {
        fclose(fp);
        if (Errs()->has_error())
            Log()->ErrorLog(mh::Initialization, Errs()->get_error());
        return (false);
    }
    if (Errs()->has_error())
        Log()->WarningLog(mh::Initialization, Errs()->get_error());

    fprintf(fp, "** Fast[er]Cap input from cell %s\n",
        CurCell()->cellname()->string());
    fprintf(fp, "** Generated by %s\n", XM()->IdString());

    const char *ustring = CDvdb()->getVariable(VA_FcUnits);
    int u = ustring ? unit_t::find_unit(ustring) : FC_DEF_UNITS;
    if (u < 0) {
        Log()->WarningLogV(mh::Initialization,
            "Unknown units \"%s\", set to default.", ustring);
        u = FC_DEF_UNITS;
    }
    e_unit unit = (e_unit)u;
    fprintf(fp, "** Units %s\n", unit_t::units(unit)->name());
    fprintf(fp, "\n");

    fcl.layer_dump(fp);

    const char *str = CDvdb()->getVariable(VA_FcPanelTarget);
    double d;
    if (str && sscanf(str, "%lf", &d) == 1 && d >= FC_MIN_TARG_PANELS &&
            d <= FC_MAX_TARG_PANELS)
        fcl.setup_refine(d);

    fcl.write_panels(fp, 0, 0, unit);
    fclose(fp);

    delete [] fc_groups;
    fc_groups = fcl.group_points();
    fc_ngroups = fcl.num_groups();

    updateString();
    updateMarks();
    return (true);
}


// Dump a unified list file and run the extraction.  Collect the
// output and do some processing, present the results in a file
// browser window.
//
void
cFC::fcRun(const char *infile, const char *outfile, const char *resfile,
    bool nodump)
{
    if (!CurCell(Physical)) {
        Log()->PopUpErr("No current cell!");
        return;
    }
    bool run_foreg = CDvdb()->getVariable(VA_FcForeg);
    bool monitor = CDvdb()->getVariable(VA_FcMonitor);

    fxJob *newjob = new fxJob(CurCell()->cellname()->string(), fxCapMode,
        this, fxJob::jobs());
    fxJob::set_jobs(newjob);

    char *in_f = 0, *ot_f = 0, *lg_f = 0;
    if (run_foreg) {
        if (!infile || !*infile) {
            in_f = getFileName(FC_LST_SFX);
            infile = in_f;
        }
        if (!outfile || !*outfile) {
            ot_f = getFileName("fc_out");
            outfile = ot_f;
        }
        if (!resfile || !*resfile) {
            lg_f = getFileName("fc_log");
            resfile = lg_f;
        }
    }
    if (!infile || !*infile)
        newjob->set_flag(FX_UNLINK_IN);
    if (!outfile || !*outfile)
        newjob->set_flag(FX_UNLINK_OUT);

    newjob->set_infiles(new stringlist(infile && *infile ?
        lstring::copy(infile) : filestat::make_temp("fci"), 0));
    newjob->set_outfile(outfile && *outfile ? lstring::copy(outfile) :
        filestat::make_temp("fco"));
    newjob->set_resfile(resfile && *resfile ? lstring::copy(resfile) : 0);
    delete [] in_f;
    delete [] ot_f;
    delete [] lg_f;

    if (!newjob->setup_fc_run(run_foreg, monitor)) {
        delete newjob;
        return;
    }
    if (!nodump && newjob->if_type() == fxJobMIT) {
        // The original FastCap from MIT is not supported currently,
        // as it does not have the unified list file support.
        GRpkgIf()->ErrPrintf(ET_ERROR,
    "\nThe FastCap program found is not supported.  This interface requires\n"
    "either the FasterCap program from FastFieldSolvers.com, or the free\n"
    "Whiteley Research FastCap program from wrcad.com.  Other FastCap\n"
    "programs can not handle the unified list file format generated here.\n");
        delete newjob;
        return;
    }
    if (!nodump && newjob->if_type() == fxJobNEW) {
        double d;
        const char *s = CDvdb()->getVariable(VA_FcPanelTarget);
        if (s && sscanf(s, "%lf", &d) == 1 && d >= FC_MIN_TARG_PANELS &&
                d <= FC_MAX_TARG_PANELS)
            GRpkgIf()->ErrPrintf(ET_WARN,
    "\nYou appear to be running FasterCap while performing panel refinement\n"
    "using the FcPanelTarget variable.  FasterCap does its own refinement\n"
    "and using refinement here is redundant at best.  I'll continue but\n"
    "you have been warned!");
    }

    if (!nodump && !fcDump(newjob->infile())) {
        delete newjob;
        return;
    }
    if (!filestat::create_bak(newjob->outfile())) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        delete newjob;
        return;
    }

    if (!newjob->run(run_foreg, monitor)) {
        delete newjob;
        return;
    }
    if (run_foreg)
        newjob->fc_post_process();

    updateString();
}


// Return a file name for use with output.
//
char *
cFC::getFileName(const char *ext, int pid)
{
    if (!CurCell(Physical))
        return (0);
    char buf[128];
    const char *s = CurCell(Physical)->cellname()->string();
    if (pid > 0)
        sprintf(buf, "%s-%d.%s", s, pid, ext);
    else
        sprintf(buf, "%s.%s", s, ext);
    return (lstring::copy(buf));
}


// Return the "real" units string for the argument, which can be a
// long form.  If the string is not recognized or the default unit, 0
// is returned.
//
const char *
cFC::getUnitsString(const char *str)
{
    int u = unit_t::find_unit(str);
    if (u < 0 || u == FC_DEF_UNITS)
        return (0);
    return (unit_t::units((e_unit)u)->name());
}


// Return the unit index corresponding to the passed string, the
// default is returned if not recognized.
//
int
cFC::getUnitsIndex(const char *str)
{
    int u = unit_t::find_unit(str);
    if (u < 0)
        u = FC_DEF_UNITS;
    return (u);
}


// Return status string for the panel label.
//
char *
cFC::statusString()
{
    int njobs = fxJob::num_jobs();
    char buf[128];
    sprintf(buf, "Running Jobs: %d", njobs);
    return (lstring::copy(buf));
}


void
cFC::showMarks(bool display)
{
    if (!fc_groups)
        return;
    if (!display) {
        DSP()->EraseCrossMarks(Physical, CROSS_MARK_FC);
        return;
    }
    for (int i = 0; i < fc_ngroups; i++) {
        char buf[16];
        sprintf(buf, "%d", i);
        DSP()->ShowCrossMark(display, fc_groups[i].x, fc_groups[i].y,
            HighlightingColor, 20, Physical, 1, buf,
            fc_groups[i].layer_desc->name());
    }
}


// Return a list of active background jobs.
//
char *
cFC::jobList()
{
    sLstr lstr;
    for (fxJob *j = fxJob::jobs(); j; j = j->next_job())
        j->pid_string(lstr);
    return (lstr.string_trim());
}


// Kill the running process.
//
void
cFC::killProcess(int pid)
{
    fxJob *j = fxJob::find(pid);
    if (j)
        j->kill_process();
}
// End of cFC functions.


#define TPRINT(...) if (db3_logfp) fprintf(db3_logfp, __VA_ARGS__)

fcLayout::fcLayout()
{
    fcl_cond_area = 0.0;
    fcl_diel_area = 0.0;
    fcl_numpanels_target = 0.0;
    fcl_maxdim = 0;
    fcl_num_c_panels_raw = 0;
    fcl_num_c_panels_written = 0;
    fcl_num_d_panels_raw = 0;
    fcl_num_d_panels_written = 0;
    fcl_zoids = false;
    fcl_verbose_out = false;
    fcl_domerge = false;
    fcl_mrgflgs = 0;

    if (logging())
        set_logfp(DBG_FP);
}


bool
fcLayout::setup_refine(double target)
{
    if (target != 0.0 && (target < 1e2 || target > 1e7))
        return (false);
    fcl_numpanels_target = target;
    fcl_cond_area = 0.0;
    fcl_diel_area = 0.0;
    fcl_maxdim = 0;

    // Compute the total panel area, in internal units.
    for (int i = 0; i < db3_ngroups; i++) {

        double a = area_group_zbot(db3_groups->list[i]);
        TPRINT("Area g%dztop %.3e\n", i, a);
        fcl_cond_area += a;
        a = area_group_ztop(db3_groups->list[i]);
        TPRINT("Area g%dztop %.3e\n", i, a);
        fcl_cond_area += a;
        a = area_group_yl(db3_groups->list[i]);
        TPRINT("Area g%dyl %.3e\n", i, a);
        fcl_cond_area += a;
        a = area_group_yu(db3_groups->list[i]);
        TPRINT("Area g%dyu %.3e\n", i, a);
        fcl_cond_area += a;
        a = area_group_left(db3_groups->list[i]);
        TPRINT("Area g%dleft %.3e\n", i, a);
        fcl_cond_area += a;
        a = area_group_right(db3_groups->list[i]);
        TPRINT("Area g%dright %.3e\n", i, a);
        fcl_cond_area += a;
    }

    if (Tech()->SubstrateEps() != 1.0) {
        int plane_bloat = INTERNAL_UNITS(FC_PLANE_BLOAT_DEF);
        const char *var = CDvdb()->getVariable(VA_FcPlaneBloat);
        if (var) {
            double val = atof(var);
            if (val >= FC_PLANE_BLOAT_MIN && val <= FC_PLANE_BLOAT_MAX)
                plane_bloat = INTERNAL_UNITS(val);
        }
        if (db3_substhick > 0) {
            int th = INTERNAL_UNITS(db3_substhick);
            BBox BB(db3_aoi);
            BB.bloat(plane_bloat);
            fcl_diel_area += BB.area();
            BBox BB2(BB.left, 0, BB.right, th);
            fcl_diel_area += 2*BB2.area();
            BB2.left = BB.bottom;
            BB2.right = BB.top;
            fcl_diel_area += 2*BB2.area();
        }
        else if (plane_bloat) {
            BBox BB(db3_aoi);
            BBox BB2(BB);
            BB.bloat(plane_bloat);
            fcl_diel_area += BB.area();
            fcl_diel_area -= BB2.area();
        }
    }

    for (Layer3d *l = db3_stack; l; l = l->next()) {
        if (!l->is_insulator())
            continue;

        double a = area_dielectric_zbot(l);
        TPRINT("Area %sxbot %.3e\n", l->layer_desc()->name(), a);
        fcl_diel_area += a;
        a = area_dielectric_ztop(l);
        TPRINT("Area %sztop %.3e\n", l->layer_desc()->name(), a);
        fcl_diel_area += a;
        a = area_dielectric_yl(l);
        TPRINT("Area %syl %.3e\n", l->layer_desc()->name(), a);
        fcl_diel_area += a;
        a = area_dielectric_yu(l);
        TPRINT("Area %syu %.3e\n", l->layer_desc()->name(), a);
        fcl_diel_area += a;
        a = area_dielectric_left(l);
        TPRINT("Area %sleft %.3e\n", l->layer_desc()->name(), a);
        fcl_diel_area += a;
        a = area_dielectric_right(l);
        TPRINT("Area %sright %.3e\n", l->layer_desc()->name(), a);
        fcl_diel_area += a;
    }
    TPRINT("Area: cndr %.3f  diel %.3f  total %.3e\n",
        fcl_cond_area, fcl_diel_area, fcl_cond_area + fcl_diel_area);

    if (fcl_numpanels_target != 0.0) {
        double a = (fcl_cond_area + fcl_diel_area)/fcl_numpanels_target;
        fcl_maxdim = INTERNAL_UNITS(sqrt(a));
    }

    return (true);
}


// Write the panel descriptions.  We use a consolidated list-file
// format.  Caller beware!  At this point, all conductors must be in
// db3_groups.
//
bool
fcLayout::write_panels(FILE *fp, int xo, int yo, e_unit unit)
{
    fcl_num_c_panels_raw = 0;
    fcl_num_c_panels_written = 0;
    fcl_num_d_panels_raw = 0;
    fcl_num_d_panels_written = 0;

    fcl_zoids = CDvdb()->getVariable(VA_FcZoids);
    fcl_verbose_out = CDvdb()->getVariable(VA_FcVerboseOut);
    fcl_domerge = !CDvdb()->getVariable(VA_FcNoMerge);
    const char *stmp = CDvdb()->getVariable(VA_FcMergeFlags);
    if (stmp)
        fcl_mrgflgs = Tech()->GetInt(stmp) & MRG_ALL;
    else
        fcl_mrgflgs = MRG_ALL;

    // Open a temp file for the "files".  This will be appended to the
    // end of the list file.

    char *tfname = filestat::make_temp("fc");
    GCarray<char*> gc_tfname(tfname);
    FILE *tfp = fopen(tfname, "w+");
    if (!tfp) {
        Errs()->add_error("Failed to create temporary file for output.");
        return (false);
    }
    const char *ff = unit_t::units(unit)->float_format();
    double sc = unit_t::units(unit)->coord_factor();

    fprintf(fp, "\n* Conductor Groups\n");

    // For each group, we need to find the surfaces that bound
    // dielectric and write them out.
    char nbuf[32];
    for (int i = 0; i < db3_ngroups; i++) {
        TPRINT("Outputting panels of group %d", i);

        // Write the group objects as a comment.
        if (fcl_verbose_out) {
            int fw = unit_t::units(unit)->field_width();
            fprintf(fp, "\n* Group %d\n", i);
            fprintf(fp, "* %-3s%-*s%-*s%-*s%-*s%-*s%-*s%-*s%s\n",
                "lr", fw, "zbot", fw, "ztop", fw, "yl", fw, "yu", fw, "xll",
                fw, "xul", fw, "xlr", "xur");
            for (glZlistRef3d *z = db3_groups->list[i]; z; z = z->next)
                z->PZ->print(fp, sc, ff, 0);

            // Now print the panels, "C" lines to fp, "Q" and "T"
            // lines to tfp.  We know that none of these sub-lists can
            // be null.

            fcCpanel *panels = panelize_group_zbot(db3_groups->list[i]);
            fprintf(fp, "* Group %d zbot\n", i);
            int pc = 0;
            for (fcCpanel *p = panels; p; p = p->next) {
                sprintf(nbuf, "g%dzbot%d", i, pc);
                p->print_c(fp, nbuf, unit);
                p->print_c_term(fp, true);
                pc++;
                p->print_panel_begin(tfp, nbuf);
                p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                    &fcl_num_c_panels_written);
                p->print_panel_end(tfp);
            }
            panels->free();
            fcl_num_c_panels_raw += pc;
            TPRINT(".");

            panels = panelize_group_ztop(db3_groups->list[i]);
            fprintf(fp, "* Group %d ztop\n", i);
            pc = 0;
            for (fcCpanel *p = panels; p; p = p->next) {
                sprintf(nbuf, "g%dztop%d", i, pc);
                p->print_c(fp, nbuf, unit);
                p->print_c_term(fp, true);
                pc++;
                p->print_panel_begin(tfp, nbuf);
                p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                    &fcl_num_c_panels_written);
                p->print_panel_end(tfp);
            }
            panels->free();
            fcl_num_c_panels_raw += pc;
            TPRINT(".");

            panels = panelize_group_yl(db3_groups->list[i]);
            fprintf(fp, "* Group %d yl\n", i);
            pc = 0;
            for (fcCpanel *p = panels; p; p = p->next) {
                sprintf(nbuf, "g%dyl%d", i, pc);
                p->print_c(fp, nbuf, unit);
                p->print_c_term(fp, true);
                pc++;
                p->print_panel_begin(tfp, nbuf);
                p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                    &fcl_num_c_panels_written);
                p->print_panel_end(tfp);
            }
            panels->free();
            fcl_num_c_panels_raw += pc;
            TPRINT(".");

            panels = panelize_group_yu(db3_groups->list[i]);
            fprintf(fp, "* Group %d yu\n", i);
            pc = 0;
            for (fcCpanel *p = panels; p; p = p->next) {
                sprintf(nbuf, "g%dyu%d", i, pc);
                p->print_c(fp, nbuf, unit);
                p->print_c_term(fp, true);
                pc++;
                p->print_panel_begin(tfp, nbuf);
                p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                    &fcl_num_c_panels_written);
                p->print_panel_end(tfp);
            }
            panels->free();
            fcl_num_c_panels_raw += pc;
            TPRINT(".");

            panels = panelize_group_left(db3_groups->list[i]);
            fprintf(fp, "* Group %d left\n", i);
            pc = 0;
            for (fcCpanel *p = panels; p; p = p->next) {
                sprintf(nbuf, "g%dleft%d", i, pc);
                p->print_c(fp, nbuf, unit);
                p->print_c_term(fp, true);
                pc++;
                p->print_panel_begin(tfp, nbuf);
                p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                    &fcl_num_c_panels_written);
                p->print_panel_end(tfp);
            }
            panels->free();
            fcl_num_c_panels_raw += pc;
            TPRINT(".");

            panels = panelize_group_right(db3_groups->list[i]);
            fprintf(fp, "* Group %d right\n", i);
            pc = 0;
            for (fcCpanel *p = panels; p; p = p->next) {
                sprintf(nbuf, "g%dright%d", i, pc);
                p->print_c(fp, nbuf, unit);
                p->print_c_term(fp, (p->next));
                pc++;
                p->print_panel_begin(tfp, nbuf);
                p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                    &fcl_num_c_panels_written);
                p->print_panel_end(tfp);
            }
            panels->free();
            fcl_num_c_panels_raw += pc;
        }
        else {
            // Create a complete list of panels for this conductor group. 
            // We know that none of these sub-groups can be null.

            fcCpanel *p0 = panelize_group_zbot(db3_groups->list[i]);
            fcCpanel *pe = p0;
            while (pe->next)
                pe = pe->next;
            pe->next = panelize_group_ztop(db3_groups->list[i]);
            while (pe->next)
                pe = pe->next;
            pe->next = panelize_group_yl(db3_groups->list[i]);
            while (pe->next)
                pe = pe->next;
            pe->next = panelize_group_yu(db3_groups->list[i]);
            while (pe->next)
                pe = pe->next;
            pe->next = panelize_group_left(db3_groups->list[i]);
            while (pe->next)
                pe = pe->next;
            pe->next = panelize_group_right(db3_groups->list[i]);

            // Now print the panels, "C" lines to fp, "Q" and "T"
            // lines to tfp.  Output the panels for each "outperm" as
            // a separate "C" line.

            int pc = 0;
            while (p0) {
                // Link to px all panels that have different outperm than p0,
                fcCpanel *px = 0, *pxe = 0, *pp = p0;
                for (fcCpanel *p = p0->next; p; p = pe) {
                    pe = p->next;
                    if (p->outperm != p0->outperm) {
                        pp->next = pe;
                        if (!px)
                            px = pxe = p;
                        else {
                            pxe->next = p;
                            pxe = p;
                        }
                        p->next = 0;
                        continue;
                    }
                    pp = p;
                }

                // Output the p0 list, free, and set it to px for the next
                // iteration.
                sprintf(nbuf, "g%dpnls%d", i, pc);
                pc++;
                p0->print_c(fp, nbuf, unit);
                p0->print_c_term(fp, (px != 0));
                p0->print_panel_begin(tfp, nbuf);
                for (fcCpanel *p = p0; p; p = p->next) {
                    p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                        &fcl_num_c_panels_written);
                    fcl_num_c_panels_raw++;
                }
                p0->print_panel_end(tfp);
                p0->free();
                p0 = px;
            }
        }
        TPRINT(". done\n");
    }

    // Now print the dielectric interfaces.  We need to find and print
    // all dielectric-dielectric interface panels where the dielectric
    // constants differ.
    fprintf(fp, "\n* Dielectric Interfaces\n");

    write_subs_panels(fp, tfp, xo, yo, unit);

    for (Layer3d *l = db3_stack; l; l = l->next()) {
        if (!l->is_insulator())
            continue;
        TPRINT("Outputting dielectric panels for layer %s",
            l->layer_desc()->name());

        if (fcl_verbose_out) {
            // Write the layer objects as a comment.
            int fw = unit_t::units(unit)->field_width();
            fprintf(fp, "\n* Layer %s\n", l->layer_desc()->name());
            fprintf(fp, "* %-3s%-*s%-*s%-*s%-*s%-*s%-*s%-*s%s\n",
                "lr", fw, "zbot", fw, "ztop", fw, "yl", fw, "yu", fw, "xll",
                fw, "xul", fw, "xlr", "xur");
            for (glYlist3d *y = l->yl3d(); y; y = y->next) {
                for (glZlist3d *z = y->y_zlist; z; z = z->next)
                    z->Z.print(fp, sc, ff, 0);
            }
        }

        fcDpanel *p0 = panelize_dielectric_zbot(l);
        sprintf(nbuf, "%szbot", l->layer_desc()->name());
        write_d_panels(fp, tfp, p0, nbuf, xo, yo, unit);
        TPRINT(".");

        p0 = panelize_dielectric_ztop(l);
        sprintf(nbuf, "%sztop", l->layer_desc()->name());
        write_d_panels(fp, tfp, p0, nbuf, xo, yo, unit);
        TPRINT(".");

        p0 = panelize_dielectric_yl(l);
        sprintf(nbuf, "%syl", l->layer_desc()->name());
        write_d_panels(fp, tfp, p0, nbuf, xo, yo, unit);
        TPRINT(".");

        p0 = panelize_dielectric_yu(l);
        sprintf(nbuf, "%syu", l->layer_desc()->name());
        write_d_panels(fp, tfp, p0, nbuf, xo, yo, unit);
        TPRINT(".");

        p0 = panelize_dielectric_left(l);
        sprintf(nbuf, "%sleft", l->layer_desc()->name());
        write_d_panels(fp, tfp, p0, nbuf, xo, yo, unit);
        TPRINT(".");

        p0 = panelize_dielectric_right(l);
        sprintf(nbuf, "%sright", l->layer_desc()->name());
        write_d_panels(fp, tfp, p0, nbuf, xo, yo, unit);
        TPRINT(". done\n");
    }
    fprintf(fp, "End\n\n");
    rewind(tfp);
    int c;
    while ((c = getc(tfp)) != EOF)
        putc(c, fp);
    fclose(tfp);
    unlink(tfname);
    TPRINT("Raw conductor panels:       %u\n", fcl_num_c_panels_raw);
    TPRINT("Raw dielectric panels:      %u\n", fcl_num_d_panels_raw);
    TPRINT("Partition grid size:        %.3f\n", MICRONS(fcl_maxdim));
    TPRINT("Conductor panels written:   %u\n", fcl_num_c_panels_written);
    TPRINT("Dielectric panels written:  %u\n", fcl_num_d_panels_written);
    return (true);
}


// Print the substrate-vacuum interface panels.
//
void
fcLayout::write_subs_panels(FILE *fp, FILE *tfp, int xo, int yo, e_unit unit)
{
    if (Tech()->SubstrateEps() == 1.0)
        return;

    int plane_bloat = INTERNAL_UNITS(FC_PLANE_BLOAT_DEF);
    const char *var = CDvdb()->getVariable(VA_FcPlaneBloat);
    if (var) {
        double val = atof(var);
        if (val >= FC_PLANE_BLOAT_MIN && val <= FC_PLANE_BLOAT_MAX)
            plane_bloat = INTERNAL_UNITS(val);
    }

    BBox BB(db3_aoi);
    xyz3d ref;
    ref.x = (BB.left + BB.right)/2;
    ref.y = (BB.bottom + BB.top)/2;
    ref.z = -1000;

    bool printed = false;
    if (plane_bloat) {
        // Write horizontal panels extending out from the AOI.
        BB.bloat(plane_bloat);
        fcDpanel pl(fcP_ZTOP,
            BB.left, BB.bottom, 0, BB.left, db3_aoi.bottom, 0,
            BB.right, db3_aoi.bottom, 0, BB.right, BB.bottom, 0,
            0, Tech()->SubstrateEps(), 1.0);
        printed = true;
        pl.print_d(fp, "substr", unit, &ref);
        pl.print_panel_begin(tfp, "substr");
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        pl = fcDpanel(fcP_ZTOP,
            BB.left, db3_aoi.bottom, 0, BB.left, db3_aoi.top, 0,
            db3_aoi.left, db3_aoi.top, 0, db3_aoi.left, db3_aoi.bottom, 0,
            0, Tech()->SubstrateEps(), 1.0);
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        pl = fcDpanel(fcP_ZTOP,
            db3_aoi.right, db3_aoi.bottom, 0, db3_aoi.right, db3_aoi.top, 0,
            BB.right, db3_aoi.top, 0, BB.right, db3_aoi.bottom, 0,
            0, Tech()->SubstrateEps(), 1.0);
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        pl = fcDpanel(fcP_ZTOP,
            BB.left, db3_aoi.top, 0, BB.left, BB.top, 0,
            BB.right, BB.top, 0, BB.right, db3_aoi.top, 0,
            0, Tech()->SubstrateEps(), 1.0);
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        fcl_num_d_panels_raw += 4;
    }
    if (db3_substhick > 0) {
        // Write the side and bottom panels.
        int th = INTERNAL_UNITS(db3_substhick);

        fcDpanel pl(fcP_ZBOT,
            BB.left, BB.bottom, -th, BB.left, BB.top, -th,
            BB.right, BB.top, -th, BB.right, BB.bottom, -th,
            0, Tech()->SubstrateEps(), 1.0);
        if (!printed) {
            printed = true;
            pl.print_d(fp, "substr", unit, &ref);
            pl.print_panel_begin(tfp, "substr");
        }
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        pl = fcDpanel(fcP_YL,
            BB.left, BB.bottom, -th, BB.left, BB.bottom, 0,
            BB.right, BB.bottom, 0, BB.right, BB.bottom, -th,
            0, Tech()->SubstrateEps(), 1.0);
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        pl = fcDpanel(fcP_YU,
            BB.left, BB.top, -th, BB.left, BB.top, 0,
            BB.right, BB.top, 0, BB.right, BB.top, -th,
            0, Tech()->SubstrateEps(), 1.0);
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        pl = fcDpanel(fcP_LEFT,
            BB.left, BB.bottom, -th, BB.left, BB.bottom, 0,
            BB.left, BB.top, 0, BB.left, BB.top, -th,
            0, Tech()->SubstrateEps(), 1.0);
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        pl = fcDpanel(fcP_RIGHT,
            BB.right, BB.bottom, -th, BB.right, BB.bottom, 0,
            BB.right, BB.top, 0, BB.right, BB.top, -th,
            0, Tech()->SubstrateEps(), 1.0);
        pl.print_panel(tfp, xo, yo, unit, fcl_maxdim,
            &fcl_num_d_panels_written);
        fcl_num_d_panels_raw += 5;
    }
    if (printed)
        fcDpanel::print_panel_end(tfp);
}

// Write out the list of planes, the list is freed.
//
void
fcLayout::write_d_panels(FILE *fp, FILE *tfp, fcDpanel *p0, char *bname,
    int xo, int yo, e_unit unit)
{
    char *e = bname + strlen(bname);
    int pc = 0;
    if (fcl_verbose_out) {
        for (fcDpanel *p = p0; p; p = p->next) {
            sprintf(e, "%d", pc);
            pc++;
            p->print_d(fp, bname, unit);
            p->print_panel_begin(tfp, bname);
            p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                &fcl_num_d_panels_written);
            p->print_panel_end(tfp);
        }
        p0->free();
        fcl_num_d_panels_raw += pc;
        return;
    }
    while (p0) {
        // Link to px all panels that have different outperm than p0,
        fcDpanel *pe, *px = 0, *pxe = 0, *pp = p0;
        for (fcDpanel *p = p0->next; p; p = pe) {
            pe = p->next;
            if (p->outperm != p0->outperm) {
                pp->next = pe;
                if (!px)
                    px = pxe = p;
                else {
                    pxe->next = p;
                    pxe = p;
                }
                p->next = 0;
                continue;
            }
            pp = p;
        }
        sprintf(e, "%d", pc);
        pc++;
        p0->print_d(fp, bname, unit);
        p0->print_panel_begin(tfp, bname);
        for (fcDpanel *p = p0; p; p = p->next) {
            p->print_panel(tfp, xo, yo, unit, fcl_maxdim,
                &fcl_num_d_panels_written);
            fcl_num_d_panels_raw++;
        }
        p0->print_panel_end(tfp);
        p0->free();
        p0 = px;
    }
}


namespace {
    void addpoint(fcGrpPtr *pts, glZoid3d &Z, CDl *ld)
    {
        int i = Z.group;
        int j = i % 4;
        if (j == 0) {
            int x = Z.xll;
            int y = Z.yl;
            if (y < pts[i].y) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
            else if (y == pts[i].y && x < pts[i].x) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
        }
        else if (j == 1) {
            int x = Z.xul;
            int y = Z.yu;
            if (y > pts[i].y) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
            else if (y == pts[i].y && x < pts[i].x) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
        }
        else if (j == 2) {
            int x = Z.xur;
            int y = Z.yu;
            if (y > pts[i].y) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
            else if (y == pts[i].y && x > pts[i].x) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
        }
        else if (j == 3) {
            int x = Z.xlr;
            int y = Z.yl;
            if (y < pts[i].y) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
            else if (y == pts[i].y && x > pts[i].x) {
                pts[i].x = x;
                pts[i].y = y;
                pts[i].layer_desc = ld;
            }
        }
    }
}


// Find a point over each group.  Return an array of the points, user
// must free.
//
fcGrpPtr *
fcLayout::group_points() const
{
    fcGrpPtr *pts = new fcGrpPtr[db3_ngroups];
    for (int i = 0; i < db3_ngroups; i++) {
        int j = i % 4;
        switch (j) {
            case 0:
                pts[i].x = db3_aoi.right;
                pts[i].y = db3_aoi.top;
                break;
            case 1:
                pts[i].x = db3_aoi.right;
                pts[i].y = db3_aoi.bottom;
                break;
            case 2:
                pts[i].x = db3_aoi.left;
                pts[i].y = db3_aoi.bottom;
                break;
            case 3:
            default:
                pts[i].x = db3_aoi.left;
                pts[i].y = db3_aoi.top;
                break;
        }
        pts[i].layer_desc = 0;
    }
    for (Layer3d *l = db3_stack; l; l = l->next()) {
        if (!l->is_conductor())
            continue;
        for (glYlist3d *y = l->yl3d(); y; y = y->next) {
            for (glZlist3d *z = y->y_zlist; z; z = z->next) {
                // Don't use planarization material.
                if (z->Z.ztop == l->plane())
                    continue;
                addpoint(pts, z->Z, l->layer_desc());
            }
        }
    }
    for (int i = 0; i < db3_ngroups; i++) {
        // This is a bit whacked.  Look for groups where no material
        // was found.  If any, they must consist of planarizing
        // material only, which is unlikely but possible.  We accept
        // the planarizing material in this case.

        if (!pts[i].layer_desc) {
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                if (!l->is_conductor())
                    continue;
                for (glYlist3d *y = l->yl3d(); y; y = y->next) {
                    for (glZlist3d *z = y->y_zlist; z; z = z->next) {
                        if (z->Z.group != i)
                            continue;
                        addpoint(pts, z->Z, l->layer_desc());
                    }
                }
            }
        }
    }
    return (pts);
}


namespace {
    // Return true if the right side of Zr and the left side of Zl
    // are colinear.  We don't really care about overlap here.
    //
    bool rl_match(const Zoid *Zr, const Zoid *Zl)
    {
        if (Zr->xlr == Zr->xur)
            return (Zl->xll == Zr->xlr && Zl->xul == Zr->xlr);
        if (Zl->xll == Zl->xul)
            return (false);
        Point_c p1_1(Zr->xlr, Zr->yl);
        Point_c p1_2(Zr->xur, Zr->yu);
        Point_c p2_1(Zl->xll, Zl->yl);
        Point_c p2_2(Zl->xul, Zl->yu);
        return (GEO()->clip_colinear(p1_1, p1_2, p2_1, p2_2));
    }


    // List element for panels on the same plane with the same abutting
    // dielectric constant.
    //
    struct sPgrp
    {
        sPgrp(double e, int z)
            {
                eps = e;
                zval = z;
                list = 0;
                next = 0;
            }

        ~sPgrp()
            {
                Zlist::destroy(list);
            }

        sPgrp *add(double e, int z, int xll, int xlr, int yl,
            int xul, int xur, int yu)
            {
                for (sPgrp *tg = this; tg; tg = tg->next) {
                    if (tg->zval == z && tg->eps == e) {
                        tg->list =
                            new Zlist(xll, xlr, yl, xul, xur, yu, tg->list);
                        return (this);
                    }
                }
                sPgrp *tg = new sPgrp(e, z);
                tg->list = new Zlist(xll, xlr, yl, xul, xur, yu, 0);
                tg->next = this;
                return (tg);
            }

        double eps;
        int zval;
        Zlist *list;
        sPgrp *next;
    };


    // Add the Zlist as objects in the current cell, for debugging.
    //
    void save_zlist(Zlist *zl, const char *s1, const char *s2, int i)
    {
        char buf[64];
        sprintf(buf, "%s%s%d", s1, s2, i);
        CDl *ld = CDldb()->newLayer(buf, Physical);
        CurCell(Physical)->db_clear_layer(ld);
        Zlist::add(zl, CurCell(Physical), ld, false, false);
    }


    void save_zlist(Zlist *zl, int g, const char *s2, int i)
    {
        char buf[64];
        sprintf(buf, "g%d%s%d", g, s2, i);
        CDl *ld = CDldb()->newLayer(buf, Physical);
        CurCell(Physical)->db_clear_layer(ld);
        Zlist::add(zl, CurCell(Physical), ld, false, false);
    }
}


fcCpanel *
fcLayout::panelize_group_zbot(const glZlistRef3d *z0) const
{
    // Due to the cutting, we know that each bottom is homogeneous. 
    // We need to output the bottoms that have dielectric below.

    sPgrp *grps = 0;
    fcCpanel *p0 = 0, *pe = 0;;
    for (const glZlistRef3d *z = z0; z; z = z->next) {
        glZlist3d *zu = find_object_under(z->PZ);
#ifdef DEBUG
        if (zu) {
            Zoid Zt = *z->PZ;
            Zoid Zx = zu->Z;
            bool novl;
            Zlist *zz = Zt.clip_out(&Zx, &novl);
            if (zz || novl) {
                printf("Internal: panelize_group, cutting error, area %g, "
                    "%s/%s.\n", Zlist::area(zz),
                    layer(z->PZ->layer_index)->layer_desc()->name(),
                    layer(zu->Z.layer_index)->layer_desc()->name());
                printf("no overlap %d\n", novl);
                printf("zoid: ");
                Zt.print();
                printf("under: ");
                Zx.print();
                printf("residual\n");
                Zlist::print(zz);
                Zlist::destroy(zz);
            }
        }
#endif

        double op = 0;
        if (!zu)
            op = Tech()->SubstrateEps();
        else if (layer(zu->Z.layer_index)->is_insulator())
            op = layer(zu->Z.layer_index)->epsrel();
        else
            continue;
        if (fcl_domerge) {
            grps = grps->add(op, z->PZ->zbot, z->PZ->xll, z->PZ->xlr,
                z->PZ->yl, z->PZ->xul, z->PZ->xur, z->PZ->yu);
        }
        else {
            fcCpanel *p = new fcCpanel(fcP_ZBOT,
                z->PZ->xll, z->PZ->yl, z->PZ->zbot,
                z->PZ->xul, z->PZ->yu, z->PZ->zbot,
                z->PZ->xur, z->PZ->yu, z->PZ->zbot,
                z->PZ->xlr, z->PZ->yl, z->PZ->zbot,
                z0->PZ->group, op);
            if (!p0)
                p0 = pe = p;
            else {
                pe->next = p;
                pe = pe->next;
            }
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        int cnt = 0;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_C_ZBOT)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            if (fcl_zoids) {
                save_zlist(zg->list, z0->PZ->group, "zbot", cnt);
                cnt++;
            }
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcCpanel *p = new fcCpanel(fcP_ZBOT,
                    zl->Z.xll, zl->Z.yl, zg->zval,
                    zl->Z.xul, zl->Z.yu, zg->zval,
                    zl->Z.xur, zl->Z.yu, zg->zval,
                    zl->Z.xlr, zl->Z.yl, zg->zval,
                    z0->PZ->group, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcCpanel *
fcLayout::panelize_group_ztop(const glZlistRef3d *z0) const
{
    // The top is not homogeneous, so we have to identify the parts
    // that are covered by dielectric, which is a bit of a chore.

    sPgrp *grps = 0;
    fcCpanel *p0 = 0, *pe = 0;;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        Layer3d *l = layer(z1->PZ->layer_index);
        Ylist *yl = new Ylist(new Zlist(z1->PZ));
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (l = l->next(); l; l = l->next()) {
            for (glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl >= z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yl)
                    break;
                for (glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (ovl.check_continue(z2->Z))
                        continue;
                    if (z2->Z.zbot != z1->PZ->ztop)
                        continue;

                    if (l->is_insulator()) {
                        Zlist *zx = yl->clip_to(&z2->Z);
                        for (Zlist *zz = zx; zz; zz = zz->next) {
                            if (fcl_domerge) {
                                grps = grps->add(l->epsrel(), z1->PZ->ztop,
                                    zz->Z.xll, zz->Z.xlr, zz->Z.yl,
                                    zz->Z.xul, zz->Z.xur, zz->Z.yu);
                            }
                            else {
                                fcCpanel *p = new fcCpanel(fcP_ZTOP,
                                    zz->Z.xll, zz->Z.yl, z1->PZ->ztop,
                                    zz->Z.xul, zz->Z.yu, z1->PZ->ztop,
                                    zz->Z.xur, zz->Z.yu, z1->PZ->ztop,
                                    zz->Z.xlr, zz->Z.yl, z1->PZ->ztop,
                                    z0->PZ->group, l->epsrel());
                                if (!p0)
                                    p0 = pe = p;
                                else {
                                    pe->next = p;
                                    pe = pe->next;
                                }
                            }
                        }
                        Zlist::destroy(zx);
                    }
                    yl = yl->clip_out(&z2->Z);
                    if (!yl)
                        goto done;
                }
            }
        }
done:   ;
        if (yl) {
            // These abut the vacuum assumed to surround
            // everything.

            Zlist *zx = yl->to_zlist();
            for (Zlist *zz = zx; zz; zz = zz->next) {
                if (fcl_domerge) {
                    grps = grps->add(1.0, z1->PZ->ztop,
                        zz->Z.xll, zz->Z.xlr, zz->Z.yl,
                        zz->Z.xul, zz->Z.xur, zz->Z.yu);
                }
                else {
                    fcCpanel *p = new fcCpanel(fcP_ZTOP,
                        zz->Z.xll, zz->Z.yl, z1->PZ->ztop,
                        zz->Z.xul, zz->Z.yu, z1->PZ->ztop,
                        zz->Z.xur, zz->Z.yu, z1->PZ->ztop,
                        zz->Z.xlr, zz->Z.yl, z1->PZ->ztop,
                        z0->PZ->group, 1.0);
                    if (!p0)
                        p0 = pe = p;
                    else {
                        pe->next = p;
                        pe = pe->next;
                    }
                }
            }
            Zlist::destroy(zx);
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        int cnt = 0;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_C_ZTOP)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            if (fcl_zoids) {
                save_zlist(zg->list, z0->PZ->group, "ztop", cnt);
                cnt++;
            }
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcCpanel *p = new fcCpanel(fcP_ZTOP,
                    zl->Z.xll, zl->Z.yl, zg->zval,
                    zl->Z.xul, zl->Z.yu, zg->zval,
                    zl->Z.xur, zl->Z.yu, zg->zval,
                    zl->Z.xlr, zl->Z.yl, zg->zval,
                    z0->PZ->group, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcCpanel *
fcLayout::panelize_group_yl(const glZlistRef3d *z0) const
{
    sPgrp *grps = 0;
    fcCpanel *p0 = 0, *pe = 0;;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        if (z1->PZ->xlr <= z1->PZ->xll)
            continue;
        Zlist *z1yl =
            new Zlist(z1->PZ->xll, z1->PZ->zbot, z1->PZ->xlr, z1->PZ->ztop);

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (z2->PZ->yu != z1->PZ->yl)
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->xur <= z1->PZ->xll || z2->PZ->xul >= z1->PZ->xlr)
                continue;
            Zoid Z2(z2->PZ->xul, z2->PZ->zbot, z2->PZ->xur, z2->PZ->ztop);
            Zlist::zl_andnot(&z1yl, &Z2);
            if (!z1yl)
                break;
        }
        if (!z1yl)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yu > z1->PZ->yl)
                    continue;
                if (y2->y_yu < z1->PZ->yl)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (z2->Z.xur <= z1->PZ->xll || z2->Z.xul >= z1->PZ->xlr)
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.xul, z2->Z.zbot, z2->Z.xur, z2->Z.ztop);
                    Zlist *zn = Zlist::copy(z1yl);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        if (fcl_domerge) {
                            grps = grps->add(l->epsrel(), z1->PZ->yl,
                                zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                zn->Z.xul, zn->Z.xur, zn->Z.yu);
                        }
                        else {
                            fcCpanel *p = new fcCpanel(fcP_YL,
                                zn->Z.xll, z1->PZ->yl, zn->Z.yl,
                                zn->Z.xul, z1->PZ->yl, zn->Z.yu,
                                zn->Z.xur, z1->PZ->yl, zn->Z.yu,
                                zn->Z.xlr, z1->PZ->yl, zn->Z.yl,
                                z0->PZ->group, l->epsrel());
                            if (!p0)
                                p0 = pe = p;
                            else {
                                pe->next = p;
                                pe = pe->next;
                            }
                        }
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1yl, &Z2);
                    if (!z1yl)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1yl) {
            if (fcl_domerge) {
                grps = grps->add(1.0, z1->PZ->yl,
                    z1yl->Z.xll, z1yl->Z.xlr, z1yl->Z.yl,
                    z1yl->Z.xul, z1yl->Z.xur, z1yl->Z.yu);
            }
            else {
                fcCpanel *p = new fcCpanel(fcP_YL,
                    z1yl->Z.xll, z1->PZ->yl, z1yl->Z.yl,
                    z1yl->Z.xul, z1->PZ->yl, z1yl->Z.yu,
                    z1yl->Z.xur, z1->PZ->yl, z1yl->Z.yu,
                    z1yl->Z.xlr, z1->PZ->yl, z1yl->Z.yl,
                    z0->PZ->group, 1.0);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            Zlist *zx = z1yl;
            z1yl = z1yl->next;
            delete zx;
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_C_YL)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcCpanel *p = new fcCpanel(fcP_YL,
                    zl->Z.xll, zg->zval, zl->Z.yl,
                    zl->Z.xul, zg->zval, zl->Z.yu,
                    zl->Z.xur, zg->zval, zl->Z.yu,
                    zl->Z.xlr, zg->zval, zl->Z.yl,
                    z0->PZ->group, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcCpanel *
fcLayout::panelize_group_yu(const glZlistRef3d *z0) const
{
    sPgrp *grps = 0;
    fcCpanel *p0 = 0, *pe = 0;;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        if (z1->PZ->xur <= z1->PZ->xul)
            continue;
        Zlist *z1yu =
            new Zlist(z1->PZ->xul, z1->PZ->zbot, z1->PZ->xur, z1->PZ->ztop);

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (z2->PZ->yl != z1->PZ->yu)
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->xlr <= z1->PZ->xul || z2->PZ->xll >= z1->PZ->xur)
                continue;
            Zoid Z2(z2->PZ->xll, z2->PZ->zbot, z2->PZ->xlr, z2->PZ->ztop);
            Zlist::zl_andnot(&z1yu, &Z2);
            if (!z1yu)
                break;
        }
        if (!z1yu)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl > z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yu)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (z2->Z.yl != z1->PZ->yu)
                        continue;
                    if (z2->Z.xlr <= z1->PZ->xul || z2->Z.xll >= z1->PZ->xur)
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.xll, z2->Z.zbot, z2->Z.xlr, z2->Z.ztop);
                    Zlist *zn = Zlist::copy(z1yu);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        if (fcl_domerge) {
                            grps = grps->add(l->epsrel(), z1->PZ->yu,
                                zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                zn->Z.xul, zn->Z.xur, zn->Z.yu);
                        }
                        else {
                            fcCpanel *p = new fcCpanel(fcP_YU,
                                zn->Z.xll, z1->PZ->yu, zn->Z.yl,
                                zn->Z.xul, z1->PZ->yu, zn->Z.yu,
                                zn->Z.xur, z1->PZ->yu, zn->Z.yu,
                                zn->Z.xlr, z1->PZ->yu, zn->Z.yl,
                                z0->PZ->group, l->epsrel());
                            if (!p0)
                                p0 = pe = p;
                            else {
                                pe->next = p;
                                pe = pe->next;
                            }
                        }
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1yu, &Z2);
                    if (!z1yu)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1yu) {
            if (fcl_domerge) {
                grps = grps->add(1.0, z1->PZ->yu,
                    z1yu->Z.xll, z1yu->Z.xlr, z1yu->Z.yl,
                    z1yu->Z.xul, z1yu->Z.xur, z1yu->Z.yu);
            }
            else {
                fcCpanel *p = new fcCpanel(fcP_YU,
                    z1yu->Z.xll, z1->PZ->yu, z1yu->Z.yl,
                    z1yu->Z.xul, z1->PZ->yu, z1yu->Z.yu,
                    z1yu->Z.xur, z1->PZ->yu, z1yu->Z.yu,
                    z1yu->Z.xlr, z1->PZ->yu, z1yu->Z.yl,
                    z0->PZ->group, 1.0);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            Zlist *zx = z1yu;
            z1yu = z1yu->next;
            delete zx;
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_C_YU)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcCpanel *p = new fcCpanel(fcP_YU,
                    zl->Z.xll, zg->zval, zl->Z.yl,
                    zl->Z.xul, zg->zval, zl->Z.yu,
                    zl->Z.xur, zg->zval, zl->Z.yu,
                    zl->Z.xlr, zg->zval, zl->Z.yl,
                    z0->PZ->group, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcCpanel *
fcLayout::panelize_group_left(const glZlistRef3d *z0) const
{
    sPgrp *grps = 0;
    fcCpanel *p0 = 0, *pe = 0;;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        Zlist *z1l =
            new Zlist(z1->PZ->yl, z1->PZ->zbot, z1->PZ->yu, z1->PZ->ztop);
        double sl = z1->PZ->slope_left();

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (!rl_match(z2->PZ, z1->PZ))
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->yl >= z1->PZ->yu || z2->PZ->yu <= z1->PZ->yl)
                continue;
            Zoid Z2(z2->PZ->yl, z2->PZ->zbot, z2->PZ->yu, z2->PZ->ztop);
            Zlist::zl_andnot(&z1l, &Z2);
            if (!z1l)
                break;
        }
        if (!z1l)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl >= z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yl)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (ovl.check_continue(z2->Z))
                        continue;
                    if (!rl_match(&z2->Z, z1->PZ))
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                    Zlist *zn = Zlist::copy(z1l);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        if (sl == 0.0 && fcl_domerge) {
                            // Can only merge Manhattan panels for now.
                            grps = grps->add(l->epsrel(), z1->PZ->xll,
                                zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                zn->Z.xul, zn->Z.xur, zn->Z.yu);
                        }
                        else {
                            int x1 = z1->PZ->xll +
                                mmRnd((zn->Z.xll - z1->PZ->yl)*sl);
                            int x2 = z1->PZ->xll +
                                mmRnd((zn->Z.xlr - z1->PZ->yl)*sl);
                            fcCpanel *p = new fcCpanel(fcP_LEFT,
                                x1, zn->Z.xll, zn->Z.yl,
                                x1, zn->Z.xul, zn->Z.yu,
                                x2, zn->Z.xur, zn->Z.yu,
                                x2, zn->Z.xlr, zn->Z.yl,
                                z0->PZ->group, l->epsrel());
                            if (!p0)
                                p0 = pe = p;
                            else {
                                pe->next = p;
                                pe = pe->next;
                            }
                        }
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1l, &Z2);
                    if (!z1l)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1l) {
            if (sl == 0.0 && fcl_domerge) {
                grps = grps->add(1.0, z1->PZ->xll,
                    z1l->Z.xll, z1l->Z.xlr, z1l->Z.yl,
                    z1l->Z.xul, z1l->Z.xur, z1l->Z.yu);
            }
            else {
                int x1 = z1->PZ->xll +
                    mmRnd((z1l->Z.xll - z1->PZ->yl)*sl);
                int x2 = z1->PZ->xll +
                    mmRnd((z1l->Z.xlr - z1->PZ->yl)*sl);
                fcCpanel *p = new fcCpanel(fcP_LEFT,
                    x1, z1l->Z.xll, z1l->Z.yl,
                    x1, z1l->Z.xul, z1l->Z.yu,
                    x2, z1l->Z.xur, z1l->Z.yu,
                    x2, z1l->Z.xlr, z1l->Z.yl,
                    z0->PZ->group, 1.0);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            Zlist *zx = z1l;
            z1l = z1l->next;
            delete zx;
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_C_LEFT)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcCpanel *p = new fcCpanel(fcP_LEFT,
                    zg->zval, zl->Z.xll, zl->Z.yl,
                    zg->zval, zl->Z.xul, zl->Z.yu,
                    zg->zval, zl->Z.xur, zl->Z.yu,
                    zg->zval, zl->Z.xlr, zl->Z.yl,
                    z0->PZ->group, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcCpanel *
fcLayout::panelize_group_right(const glZlistRef3d *z0) const
{
    sPgrp *grps = 0;
    fcCpanel *p0 = 0, *pe = 0;;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        Zlist *z1r =
            new Zlist(z1->PZ->yl, z1->PZ->zbot, z1->PZ->yu, z1->PZ->ztop);
        double sr = z1->PZ->slope_right();

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (!rl_match(z1->PZ, z2->PZ))
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->yl >= z1->PZ->yu || z2->PZ->yu <= z1->PZ->yl)
                continue;
            Zoid Z2(z2->PZ->yl, z2->PZ->zbot, z2->PZ->yu, z2->PZ->ztop);
            Zlist::zl_andnot(&z1r, &Z2);
            if (!z1r)
                break;
        }
        if (!z1r)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl >= z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yl)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (ovl.check_continue(z2->Z))
                        continue;
                    if (!rl_match(z1->PZ, &z2->Z))
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                    Zlist *zn = Zlist::copy(z1r);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        if (sr == 0.0 && fcl_domerge) {
                            // Can only merge Manhattan panels for now.
                            grps = grps->add(l->epsrel(), z1->PZ->xlr,
                                zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                zn->Z.xul, zn->Z.xur, zn->Z.yu);
                        }
                        else {
                            int x1 = z1->PZ->xlr +
                                mmRnd((zn->Z.xll - z1->PZ->yl)*sr);
                            int x2 = z1->PZ->xlr +
                                mmRnd((zn->Z.xlr - z1->PZ->yl)*sr);
                            fcCpanel *p = new fcCpanel(fcP_RIGHT,
                                x1, zn->Z.xll, zn->Z.yl,
                                x1, zn->Z.xul, zn->Z.yu,
                                x2, zn->Z.xur, zn->Z.yu,
                                x2, zn->Z.xlr, zn->Z.yl,
                                z0->PZ->group, l->epsrel());
                            if (!p0)
                                p0 = pe = p;
                            else {
                                pe->next = p;
                                pe = pe->next;
                            }
                        }
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1r, &Z2);
                    if (!z1r)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1r) {
            if (sr == 0.0 && fcl_domerge) {
                grps = grps->add(1.0, z1->PZ->xlr,
                    z1r->Z.xll, z1r->Z.xlr, z1r->Z.yl,
                    z1r->Z.xul, z1r->Z.xur, z1r->Z.yu);
            }
            else {
                int x1 = z1->PZ->xlr + mmRnd((z1r->Z.xll - z1->PZ->yl)*sr);
                int x2 = z1->PZ->xlr + mmRnd((z1r->Z.xlr - z1->PZ->yl)*sr);
                fcCpanel *p = new fcCpanel(fcP_RIGHT,
                    x1, z1r->Z.xll, z1r->Z.yl,
                    x1, z1r->Z.xul, z1r->Z.yu,
                    x2, z1r->Z.xur, z1r->Z.yu,
                    x2, z1r->Z.xlr, z1r->Z.yl,
                    z0->PZ->group, 1.0);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            Zlist *zx = z1r;
            z1r = z1r->next;
            delete zx;
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_C_RIGHT)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcCpanel *p = new fcCpanel(fcP_RIGHT,
                    zg->zval, zl->Z.xll, zl->Z.yl,
                    zg->zval, zl->Z.xul, zl->Z.yu,
                    zg->zval, zl->Z.xur, zl->Z.yu,
                    zg->zval, zl->Z.xlr, zl->Z.yl,
                    z0->PZ->group, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcDpanel *
fcLayout::panelize_dielectric_zbot(const Layer3d *l) const
{
    // Due to the cutting, we know that each bottom is homogeneous.  We
    // need to output the bottoms that have different dielectric below.

    double dc1 = l->epsrel();
    double dc2 = Tech()->SubstrateEps();
    if (dc1 == dc2)
        return (0);
    sPgrp *grps = 0;
    fcDpanel *p0 = 0, *pe = 0;
    for (const glYlist3d *y = l->yl3d(); y; y = y->next) {
        for (const glZlist3d *z = y->y_zlist; z; z = z->next) {
            // Panels with an object under are redundant with the other
            // layer's ztop, so we skip them here.
#ifdef DEBUG
            glZlist3d *zu = find_object_under(&z->Z);
            if (zu) {
                Zoid Zt = z->Z;
                Zoid Zx = zu->Z;
                bool novl;
                Zlist *zz = Zt.clip_out(&Zx, &novl);
                if (zz || novl) {
                    printf("Internal: panelize_dielectric, cutting error.\n");
                    Zlist::destroy(zz);

                    printf("Internal: panelize_electric, cutting error, "
                        "area %g, %s/%s.\n", Zlist::area(zz),
                        layer(z->Z.layer_index)->layer_desc()->name(),
                        layer(zu->Z.layer_index)->layer_desc()->name());
                    printf("no overlap %d\n", novl);
                    printf("zoid: ");
                    Zt.print();
                    printf("under: ");
                    Zx.print();
                    printf("residual\n");
                    Zlist::print(zz);
                    Zlist::destroy(zz);
                }
                continue;
            }
#else
            if (find_object_under(&z->Z))
                continue;
#endif

            if (fcl_domerge) {
                grps = grps->add(dc2, z->Z.zbot, z->Z.xll, z->Z.xlr,
                    z->Z.yl, z->Z.xul, z->Z.xur, z->Z.yu);
            }
            else {
                fcDpanel *p = new fcDpanel(fcP_ZBOT,
                    z->Z.xll, z->Z.yl, z->Z.zbot,
                    z->Z.xul, z->Z.yu, z->Z.zbot,
                    z->Z.xur, z->Z.yu, z->Z.zbot,
                    z->Z.xlr, z->Z.yl, z->Z.zbot,
                    l->index() + 1, dc1, dc2);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        int cnt = 0;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_D_ZBOT)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            if (fcl_zoids) {
                CDl *ld = l->layer_desc();
                save_zlist(zg->list, ld->name(), "zbot", cnt);
                cnt++;
            }
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcDpanel *p = new fcDpanel(fcP_ZBOT,
                    zl->Z.xll, zl->Z.yl, zg->zval,
                    zl->Z.xul, zl->Z.yu, zg->zval,
                    zl->Z.xur, zl->Z.yu, zg->zval,
                    zl->Z.xlr, zl->Z.yl, zg->zval,
                    l->index() + 1, dc1, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcDpanel *
fcLayout::panelize_dielectric_ztop(const Layer3d *l1) const
{
    // The top is not homogeneous, so we have to identify the parts
    // that are covered by different dielectric, which is a bit of a
    // chore.

    double dc1 = l1->epsrel();
    sPgrp *grps = 0;
    fcDpanel *p0 = 0, *pe = 0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            Ylist *yl = new Ylist(new Zlist(&z1->Z));
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = l1->next(); l; l = l->next()) {
                for (glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl >= z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yl)
                        break;
                    for (glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (ovl.check_continue(z2->Z))
                            continue;
                        if (z2->Z.zbot != z1->Z.ztop)
                            continue;

                        if (l->is_insulator()) {
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zx = yl->clip_to(&z2->Z);
                                for (Zlist *zz = zx; zz; zz = zz->next) {
                                    if (fcl_domerge) {
                                        grps = grps->add(dc2, z1->Z.ztop,
                                            zz->Z.xll, zz->Z.xlr, zz->Z.yl,
                                            zz->Z.xul, zz->Z.xur, zz->Z.yu);
                                    }
                                    else {
                                        fcDpanel *p = new fcDpanel(fcP_ZTOP,
                                            zz->Z.xll, zz->Z.yl, z1->Z.ztop,
                                            zz->Z.xul, zz->Z.yu, z1->Z.ztop,
                                            zz->Z.xur, zz->Z.yu, z1->Z.ztop,
                                            zz->Z.xlr, zz->Z.yl, z1->Z.ztop,
                                            l1->index() + 1, dc1, dc2);
                                        if (!p0)
                                            p0 = pe = p;
                                        else {
                                            pe->next = p;
                                            pe = pe->next;
                                        }
                                    }
                                }
                                Zlist::destroy(zx);
                            }
                        }
                        yl = yl->clip_out(&z2->Z);
                        if (!yl)
                            goto done;
                    }
                }
            }
done:       ;
            if (yl) {
                // These abut the vacuum assumed to surround
                // everything.

                if (dc1 != 1.0) {
                    Zlist *zx = yl->to_zlist();
                    for (Zlist *zz = zx; zz; zz = zz->next) {
                        if (fcl_domerge) {
                            grps = grps->add(1.0, z1->Z.ztop,
                                zz->Z.xll, zz->Z.xlr, zz->Z.yl,
                                zz->Z.xul, zz->Z.xur, zz->Z.yu);
                        }
                        else {
                            fcDpanel *p = new fcDpanel(fcP_ZTOP,
                                zz->Z.xll, zz->Z.yl, z1->Z.ztop,
                                zz->Z.xul, zz->Z.yu, z1->Z.ztop,
                                zz->Z.xur, zz->Z.yu, z1->Z.ztop,
                                zz->Z.xlr, zz->Z.yl, z1->Z.ztop,
                                l1->index() + 1, dc1, 1.0);
                            if (!p0)
                                p0 = pe = p;
                            else {
                                pe->next = p;
                                pe = pe->next;
                            }
                        }
                    }
                    Zlist::destroy(zx);
                }
            }
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        int cnt = 0;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_D_ZTOP)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            if (fcl_zoids) {
                CDl *ld = l1->layer_desc();
                save_zlist(zg->list, ld->name(), "ztop", cnt);
                cnt++;
            }
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcDpanel *p = new fcDpanel(fcP_ZTOP,
                    zl->Z.xll, zl->Z.yl, zg->zval,
                    zl->Z.xul, zl->Z.yu, zg->zval,
                    zl->Z.xur, zl->Z.yu, zg->zval,
                    zl->Z.xlr, zl->Z.yl, zg->zval,
                    l1->index() + 1, dc1, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcDpanel *
fcLayout::panelize_dielectric_yl(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    sPgrp *grps = 0;
    fcDpanel *p0 = 0, *pe = 0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            if (z1->Z.xlr <= z1->Z.xll)
                continue;
            Zlist *z1yl =
                new Zlist(z1->Z.xll, z1->Z.zbot, z1->Z.xlr, z1->Z.ztop);
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yu > z1->Z.yl)
                        continue;
                    if (y2->y_yu < z1->Z.yl)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (z2->Z.xur <= z1->Z.xll || z2->Z.xul >= z1->Z.xlr)
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.xul, z2->Z.zbot, z2->Z.xur, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1yl);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    if (fcl_domerge) {
                                        grps = grps->add(dc2, z1->Z.yl,
                                            zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                            zn->Z.xul, zn->Z.xur, zn->Z.yu);
                                    }
                                    else {
                                        fcDpanel *p = new fcDpanel(fcP_YL,
                                            zn->Z.xll, z1->Z.yl, zn->Z.yl,
                                            zn->Z.xul, z1->Z.yl, zn->Z.yu,
                                            zn->Z.xur, z1->Z.yl, zn->Z.yu,
                                            zn->Z.xlr, z1->Z.yl, zn->Z.yl,
                                            l1->index() + 1, dc1, dc2);
                                        if (!p0)
                                            p0 = pe = p;
                                        else {
                                            pe->next = p;
                                            pe = pe->next;
                                        }
                                    }
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1yl, &Z2);
                        if (!z1yl)
                            goto done;
                    }
                }
            }
done:       ;
            if (dc1 != 1.0) {
                while (z1yl) {
                    if (fcl_domerge) {
                        grps = grps->add(1.0, z1->Z.yl,
                            z1yl->Z.xll, z1yl->Z.xlr, z1yl->Z.yl,
                            z1yl->Z.xul, z1yl->Z.xur, z1yl->Z.yu);
                    }
                    else {
                        fcDpanel *p = new fcDpanel(fcP_YL,
                            z1yl->Z.xll, z1->Z.yl, z1yl->Z.yl,
                            z1yl->Z.xul, z1->Z.yl, z1yl->Z.yu,
                            z1yl->Z.xur, z1->Z.yl, z1yl->Z.yu,
                            z1yl->Z.xlr, z1->Z.yl, z1yl->Z.yl,
                            l1->index() + 1, dc1, 1.0);
                        if (!p0)
                            p0 = pe = p;
                        else {
                            pe->next = p;
                            pe = pe->next;
                        }
                    }
                    Zlist *zx = z1yl;
                    z1yl = z1yl->next;
                    delete zx;
                }
            }
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_D_YL)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcDpanel *p = new fcDpanel(fcP_YL,
                    zl->Z.xll, zg->zval, zl->Z.yl,
                    zl->Z.xul, zg->zval, zl->Z.yu,
                    zl->Z.xur, zg->zval, zl->Z.yu,
                    zl->Z.xlr, zg->zval, zl->Z.yl,
                    l1->index() + 1, dc1, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcDpanel *
fcLayout::panelize_dielectric_yu(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    sPgrp *grps = 0;
    fcDpanel *p0 = 0, *pe = 0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            if (z1->Z.xur <= z1->Z.xul)
                continue;
            Zlist *z1yu =
                new Zlist(z1->Z.xul, z1->Z.zbot, z1->Z.xur, z1->Z.ztop);
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl > z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yu)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (z2->Z.yl != z1->Z.yu)
                            continue;
                        if (z2->Z.xlr <= z1->Z.xul || z2->Z.xll >= z1->Z.xur)
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.xll, z2->Z.zbot, z2->Z.xlr, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1yu);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    if (fcl_domerge) {
                                        grps = grps->add(dc2, z1->Z.yu,
                                            zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                            zn->Z.xul, zn->Z.xur, zn->Z.yu);
                                    }
                                    else {
                                        fcDpanel *p = new fcDpanel(fcP_YU,
                                            zn->Z.xll, z1->Z.yu, zn->Z.yl,
                                            zn->Z.xul, z1->Z.yu, zn->Z.yu,
                                            zn->Z.xur, z1->Z.yu, zn->Z.yu,
                                            zn->Z.xlr, z1->Z.yu, zn->Z.yl,
                                            l1->index() + 1, dc1, dc2);
                                        if (!p0)
                                            p0 = pe = p;
                                        else {
                                            pe->next = p;
                                            pe = pe->next;
                                        }
                                    }
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1yu, &Z2);
                        if (!z1yu)
                            goto done;
                    }
                }
            }
done:       ;
            if (dc1 != 1.0) {
                while (z1yu) {
                    if (fcl_domerge) {
                        grps = grps->add(1.0, z1->Z.yu,
                            z1yu->Z.xll, z1yu->Z.xlr, z1yu->Z.yl,
                            z1yu->Z.xul, z1yu->Z.xur, z1yu->Z.yu);
                    }
                    else {
                        fcDpanel *p = new fcDpanel(fcP_YU,
                            z1yu->Z.xll, z1->Z.yu, z1yu->Z.yl,
                            z1yu->Z.xul, z1->Z.yu, z1yu->Z.yu,
                            z1yu->Z.xur, z1->Z.yu, z1yu->Z.yu,
                            z1yu->Z.xlr, z1->Z.yu, z1yu->Z.yl,
                            l1->index() + 1, dc1, 1.0);
                        if (!p0)
                            p0 = pe = p;
                        else {
                            pe->next = p;
                            pe = pe->next;
                        }
                    }
                    Zlist *zx = z1yu;
                    z1yu = z1yu->next;
                    delete zx;
                }
            }
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_D_YU)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcDpanel *p = new fcDpanel(fcP_YU,
                    zl->Z.xll, zg->zval, zl->Z.yl,
                    zl->Z.xul, zg->zval, zl->Z.yu,
                    zl->Z.xur, zg->zval, zl->Z.yu,
                    zl->Z.xlr, zg->zval, zl->Z.yl,
                    l1->index() + 1, dc1, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcDpanel *
fcLayout::panelize_dielectric_left(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    sPgrp *grps = 0;
    fcDpanel *p0 = 0, *pe = 0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            Zlist *z1l = new Zlist(z1->Z.yl, z1->Z.zbot, z1->Z.yu, z1->Z.ztop);
            double sl = z1->Z.slope_left();
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl >= z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yl)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (ovl.check_continue(z2->Z))
                            continue;
                        if (!rl_match(&z2->Z, &z1->Z))
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1l);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    if (sl == 0.0 && fcl_domerge) {
                                        // Can only merge Manhattan
                                        // panels for now.

                                        grps = grps->add(dc2, z1->Z.xll,
                                            zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                            zn->Z.xul, zn->Z.xur, zn->Z.yu);
                                    }
                                    else {
                                        int x1 = z1->Z.xll +
                                            mmRnd((zn->Z.xll - z1->Z.yl)*sl);
                                        int x2 = z1->Z.xll +
                                            mmRnd((zn->Z.xlr - z1->Z.yl)*sl);
                                        fcDpanel *p = new fcDpanel(fcP_LEFT,
                                            x1, zn->Z.xll, zn->Z.yl,
                                            x1, zn->Z.xul, zn->Z.yu,
                                            x2, zn->Z.xur, zn->Z.yu,
                                            x2, zn->Z.xlr, zn->Z.yl,
                                            l1->index() + 1, dc1, dc2);
                                        if (!p0)
                                            p0 = pe = p;
                                        else {
                                            pe->next = p;
                                            pe = pe->next;
                                        }
                                    }
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1l, &Z2);
                        if (!z1l)
                            goto done;
                    }
                }
            }
done:       ;
            if (dc1 != 1.0) {
                while (z1l) {
                    if (sl == 0.0 && fcl_domerge) {
                        grps = grps->add(1.0, z1->Z.xll,
                            z1l->Z.xll, z1l->Z.xlr, z1l->Z.yl,
                            z1l->Z.xul, z1l->Z.xur, z1l->Z.yu);
                    }
                    else {
                        int x1 = z1->Z.xll + mmRnd((z1l->Z.xll - z1->Z.yl)*sl);
                        int x2 = z1->Z.xll + mmRnd((z1l->Z.xlr - z1->Z.yl)*sl);
                        fcDpanel *p = new fcDpanel(fcP_LEFT,
                            x1, z1l->Z.xll, z1l->Z.yl,
                            x1, z1l->Z.xul, z1l->Z.yu,
                            x2, z1l->Z.xur, z1l->Z.yu,
                            x2, z1l->Z.xlr, z1l->Z.yl,
                            l1->index() + 1, dc1, 1.0);
                        if (!p0)
                            p0 = pe = p;
                        else {
                            pe->next = p;
                            pe = pe->next;
                        }
                    }
                    Zlist *zx = z1l;
                    z1l = z1l->next;
                    delete zx;
                }
            }
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_D_LEFT)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcDpanel *p = new fcDpanel(fcP_LEFT,
                    zg->zval, zl->Z.xll, zl->Z.yl,
                    zg->zval, zl->Z.xul, zl->Z.yu,
                    zg->zval, zl->Z.xur, zl->Z.yu,
                    zg->zval, zl->Z.xlr, zl->Z.yl,
                    l1->index() + 1, dc1, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


fcDpanel *
fcLayout::panelize_dielectric_right(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    sPgrp *grps = 0;
    fcDpanel *p0 = 0, *pe = 0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            Zlist *z1r = new Zlist(z1->Z.yl, z1->Z.zbot, z1->Z.yu, z1->Z.ztop);
            double sr = z1->Z.slope_right();
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl >= z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yl)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (z2 == z1)
                            continue;
                        if (ovl.check_break(z2->Z))
                            break;
                        if (ovl.check_continue(z2->Z))
                            continue;
                        if (!rl_match(&z1->Z, &z2->Z))
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1r);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    if (sr == 0.0 && fcl_domerge) {
                                        // Can only merge Manhattan
                                        // panels for now.

                                        grps = grps->add(dc2, z1->Z.xlr,
                                            zn->Z.xll, zn->Z.xlr, zn->Z.yl,
                                            zn->Z.xul, zn->Z.xur, zn->Z.yu);
                                    }
                                    else {
                                        int x1 = z1->Z.xlr +
                                            mmRnd((zn->Z.xll - z1->Z.yl)*sr);
                                        int x2 = z1->Z.xlr +
                                            mmRnd((zn->Z.xlr - z1->Z.yl)*sr);
                                        fcDpanel *p = new fcDpanel(fcP_RIGHT,
                                            x1, zn->Z.xll, zn->Z.yl,
                                            x1, zn->Z.xul, zn->Z.yu,
                                            x2, zn->Z.xur, zn->Z.yu,
                                            x2, zn->Z.xlr, zn->Z.yl,
                                            l1->index() + 1, dc1, dc2);
                                        if (!p0)
                                            p0 = pe = p;
                                        else {
                                            pe->next = p;
                                            pe = pe->next;
                                        }
                                    }
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1r, &Z2);
                        if (!z1r)
                            goto done;
                    }
                }
            }
done:       ;
            if (dc1 != 1.0) {
                while (z1r) {
                    if (sr == 0.0 && fcl_domerge) {
                        grps = grps->add(1.0, z1->Z.xlr,
                            z1r->Z.xll, z1r->Z.xlr, z1r->Z.yl,
                            z1r->Z.xul, z1r->Z.xur, z1r->Z.yu);
                    }
                    else {
                        int x1 = z1->Z.xlr + mmRnd((z1r->Z.xll - z1->Z.yl)*sr);
                        int x2 = z1->Z.xlr + mmRnd((z1r->Z.xlr - z1->Z.yl)*sr);
                        fcDpanel *p = new fcDpanel(fcP_RIGHT,
                            x1, z1r->Z.xll, z1r->Z.yl,
                            x1, z1r->Z.xul, z1r->Z.yu,
                            x2, z1r->Z.xur, z1r->Z.yu,
                            x2, z1r->Z.xlr, z1r->Z.yl,
                            l1->index() + 1, dc1, 1.0);
                        if (!p0)
                            p0 = pe = p;
                        else {
                            pe->next = p;
                            pe = pe->next;
                        }
                    }
                    Zlist *zx = z1r;
                    z1r = z1r->next;
                    delete zx;
                }
            }
        }
    }
    if (fcl_domerge) {
        sPgrp *zn;
        for (sPgrp *zg = grps; zg; zg = zn) {
            zn = zg->next;
            if (fcl_mrgflgs & MRG_D_RIGHT)
                zg->list = Zlist::repartition_ni(zg->list);
            zg->list = Zlist::filter_slivers(zg->list, 1);
            for (Zlist *zl = zg->list; zl; zl = zl->next) {
                fcDpanel *p = new fcDpanel(fcP_RIGHT,
                    zg->zval, zl->Z.xll, zl->Z.yl,
                    zg->zval, zl->Z.xul, zl->Z.yu,
                    zg->zval, zl->Z.xur, zl->Z.yu,
                    zg->zval, zl->Z.xlr, zl->Z.yl,
                    l1->index() + 1, dc1, zg->eps);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->next = p;
                    pe = pe->next;
                }
            }
            delete zg;
        }
    }
    return (p0);
}


double
fcLayout::area_group_zbot(const glZlistRef3d *z0) const
{
    // Due to the cutting, we know that each bottom is homogeneous. 
    // We need to output the bottoms that have dielectric below.

    double area = 0.0;
    for (const glZlistRef3d *z = z0; z; z = z->next) {
        glZlist3d *zu = find_object_under(z->PZ);
        if (zu && !layer(zu->Z.layer_index)->is_insulator())
            continue;
        area += z->PZ->area();
    }
    return (area);
}


double
fcLayout::area_group_ztop(const glZlistRef3d *z0) const
{
    // The top is not homogeneous, so we have to identify the parts
    // that are covered by dielectric, which is a bit of a chore.

    double area = 0.0;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        Layer3d *l = layer(z1->PZ->layer_index);
        Ylist *yl = new Ylist(new Zlist(z1->PZ));
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (l = l->next(); l; l = l->next()) {
            for (glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl >= z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yl)
                    break;
                for (glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (ovl.check_continue(z2->Z))
                        continue;
                    if (z2->Z.zbot != z1->PZ->ztop)
                        continue;

                    if (l->is_insulator()) {
                        Zlist *zx = yl->clip_to(&z2->Z);
                        area += Zlist::area(zx);
                        Zlist::destroy(zx);
                    }
                    yl = yl->clip_out(&z2->Z);
                    if (!yl)
                        goto done;
                }
            }
        }
done:   ;
        if (yl) {
            // These abut the vacuum assumed to surround
            // everything.

            Zlist *zx = yl->to_zlist();
            area += Zlist::area(zx);
            Zlist::destroy(zx);
        }
    }
    return (area);
}


double
fcLayout::area_group_yl(const glZlistRef3d *z0) const
{
    double area = 0.0;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        if (z1->PZ->xlr <= z1->PZ->xll)
            continue;
        Zlist *z1yl =
            new Zlist(z1->PZ->xll, z1->PZ->zbot, z1->PZ->xlr, z1->PZ->ztop);

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (z2->PZ->yu != z1->PZ->yl)
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->xur <= z1->PZ->xll || z2->PZ->xul >= z1->PZ->xlr)
                continue;
            Zoid Z2(z2->PZ->xul, z2->PZ->zbot, z2->PZ->xur, z2->PZ->ztop);
            Zlist::zl_andnot(&z1yl, &Z2);
            if (!z1yl)
                break;
        }
        if (!z1yl)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yu > z1->PZ->yl)
                    continue;
                if (y2->y_yu < z1->PZ->yl)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (z2->Z.xur <= z1->PZ->xll || z2->Z.xul >= z1->PZ->xlr)
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.xul, z2->Z.zbot, z2->Z.xur, z2->Z.ztop);
                    Zlist *zn = Zlist::copy(z1yl);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        area += zn->Z.area();
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1yl, &Z2);
                    if (!z1yl)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1yl) {
            area += z1yl->Z.area();
            Zlist *zx = z1yl;
            z1yl = z1yl->next;
            delete zx;
        }
    }
    return (area);
}


double
fcLayout::area_group_yu(const glZlistRef3d *z0) const
{
    double area = 0.0;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        if (z1->PZ->xur <= z1->PZ->xul)
            continue;
        Zlist *z1yu =
            new Zlist(z1->PZ->xul, z1->PZ->zbot, z1->PZ->xur, z1->PZ->ztop);

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (z2->PZ->yl != z1->PZ->yu)
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->xlr <= z1->PZ->xul || z2->PZ->xll >= z1->PZ->xur)
                continue;
            Zoid Z2(z2->PZ->xll, z2->PZ->zbot, z2->PZ->xlr, z2->PZ->ztop);
            Zlist::zl_andnot(&z1yu, &Z2);
            if (!z1yu)
                break;
        }
        if (!z1yu)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl > z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yu)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (z2->Z.yl != z1->PZ->yu)
                        continue;
                    if (z2->Z.xlr <= z1->PZ->xul || z2->Z.xll >= z1->PZ->xur)
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.xll, z2->Z.zbot, z2->Z.xlr, z2->Z.ztop);
                    Zlist *zn = Zlist::copy(z1yu);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        area += zn->Z.area();
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1yu, &Z2);
                    if (!z1yu)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1yu) {
            area += z1yu->Z.area();
            Zlist *zx = z1yu;
            z1yu = z1yu->next;
            delete zx;
        }
    }
    return (area);
}


double
fcLayout::area_group_left(const glZlistRef3d *z0) const
{
    double area = 0.0;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        Zlist *z1l =
            new Zlist(z1->PZ->yl, z1->PZ->zbot, z1->PZ->yu, z1->PZ->ztop);
        double sl = z1->PZ->slope_left();
        double atmp = 0.0;

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (!rl_match(z2->PZ, z1->PZ))
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->yl >= z1->PZ->yu || z2->PZ->yu <= z1->PZ->yl)
                continue;
            Zoid Z2(z2->PZ->yl, z2->PZ->zbot, z2->PZ->yu, z2->PZ->ztop);
            Zlist::zl_andnot(&z1l, &Z2);
            if (!z1l)
                break;
        }
        if (!z1l)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl >= z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yl)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (ovl.check_continue(z2->Z))
                        continue;
                    if (!rl_match(&z2->Z, z1->PZ))
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                    Zlist *zn = Zlist::copy(z1l);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        atmp += zn->Z.area();
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1l, &Z2);
                    if (!z1l)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1l) {
            atmp += z1l->Z.area();
            Zlist *zx = z1l;
            z1l = z1l->next;
            delete zx;
        }
        if (sl != 0.0)
            atmp *= sqrt(1.0 + sl*sl);
        area += atmp;
    }
    return (area);
}


double
fcLayout::area_group_right(const glZlistRef3d *z0) const
{
    double area = 0.0;
    for (const glZlistRef3d *z1 = z0; z1; z1 = z1->next) {
        Zlist *z1r =
            new Zlist(z1->PZ->yl, z1->PZ->zbot, z1->PZ->yu, z1->PZ->ztop);
        double sr = z1->PZ->slope_right();
        double atmp = 0.0;

        // First clip out the areas that abut a conducting element,
        // which must be in the same group.
        for (const glZlistRef3d *z2 = z0; z2; z2 = z2->next) {
            if (!rl_match(z1->PZ, z2->PZ))
                continue;
            if (z2->PZ->zbot >= z1->PZ->ztop || z2->PZ->ztop <= z1->PZ->zbot)
                continue;
            if (z2->PZ->yl >= z1->PZ->yu || z2->PZ->yu <= z1->PZ->yl)
                continue;
            Zoid Z2(z2->PZ->yl, z2->PZ->zbot, z2->PZ->yu, z2->PZ->ztop);
            Zlist::zl_andnot(&z1r, &Z2);
            if (!z1r)
                break;
        }
        if (!z1r)
            continue;

        // Next find adjacent dielectric.
        fc_ovlchk_t ovl(z1->PZ->minleft(), z1->PZ->maxright(), z1->PZ->yu);
        for (Layer3d *l = db3_stack; l; l = l->next()) {
            if (!l->is_insulator())
                continue;
            for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                if (y2->y_yl >= z1->PZ->yu)
                    continue;
                if (y2->y_yu <= z1->PZ->yl)
                    break;
                for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                    if (ovl.check_break(z2->Z))
                        break;
                    if (ovl.check_continue(z2->Z))
                        continue;
                    if (!rl_match(z1->PZ, &z2->Z))
                        continue;
                    if (z2->Z.zbot >= z1->PZ->ztop ||
                            z2->Z.ztop <= z1->PZ->zbot)
                        continue;

                    Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                    Zlist *zn = z1r->Zlist::copy(z1r);
                    Zlist::zl_and(&zn, &Z2);
                    while (zn) {
                        atmp += zn->Z.area();
                        Zlist *zx = zn;
                        zn = zn->next;
                        delete zx;
                    }
                    Zlist::zl_andnot(&z1r, &Z2);
                    if (!z1r)
                        goto done;
                }
            }
        }
done:   ;

        // Anything left must abut vacuum.
        while (z1r) {
            atmp += z1r->Z.area();
            Zlist *zx = z1r;
            z1r = z1r->next;
            delete zx;
        }
        if (sr != 0.0)
            atmp *= sqrt(1.0 * sr*sr);
        area += atmp;
    }
    return (area);
}


double
fcLayout::area_dielectric_zbot(const Layer3d *l) const
{
    // Due to the cutting, we know that each bottom is homogeneous.  We
    // need to output the bottoms that have different dielectric below.

    double dc1 = l->epsrel();
    double dc2 = Tech()->SubstrateEps();
    if (dc1 == dc2)
        return (0);
    double area = 0.0;
    for (const glYlist3d *y = l->yl3d(); y; y = y->next) {
        for (const glZlist3d *z = y->y_zlist; z; z = z->next) {
            if (find_object_under(&z->Z))
                continue;
            // Panels with an object under are redundant with the other
            // layer's ztop, so we skip them here.

            area += z->Z.area();
        }
    }
    return (area);
}


double
fcLayout::area_dielectric_ztop(const Layer3d *l1) const
{
    // The top is not homogeneous, so we have to identify the parts
    // that are covered by different dielectric, which is a bit of a
    // chore.

    double dc1 = l1->epsrel();
    double area = 0.0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            Ylist *yl = new Ylist(new Zlist(&z1->Z));
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = l1->next(); l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl >= z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yl)
                        break;
                    for (glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (ovl.check_continue(z2->Z))
                            continue;
                        if (z2->Z.zbot != z1->Z.ztop)
                            continue;

                        if (l->is_insulator()) {
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zx = yl->clip_to(&z2->Z);
                                for (Zlist *zz = zx; zz; zz = zz->next)
                                    area += zz->Z.area();
                                Zlist::destroy(zx);
                            }
                        }
                        yl = yl->clip_out(&z2->Z);
                        if (!yl)
                            goto done;
                    }
                }
            }
done:       ;
            if (yl) {
                // These abut the vacuum assumed to surround
                // everything.

                if (dc1 != 1.0) {
                    Zlist *zx = yl->to_zlist();
                    for (Zlist *zz = zx; zz; zz = zz->next)
                        area += zz->Z.area();
                    Zlist::destroy(zx);
                }
            }
        }
    }
    return (area);
}


double
fcLayout::area_dielectric_yl(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    double area = 0.0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            if (z1->Z.xlr <= z1->Z.xll)
                continue;
            Zlist *z1yl =
                new Zlist(z1->Z.xll, z1->Z.zbot, z1->Z.xlr, z1->Z.ztop);
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yu > z1->Z.yl)
                        continue;
                    if (y2->y_yu < z1->Z.yl)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (z2->Z.xur <= z1->Z.xll || z2->Z.xul >= z1->Z.xlr)
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.xul, z2->Z.zbot, z2->Z.xur, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1yl);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    area += zn->Z.area();
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1yl, &Z2);
                        if (!z1yl)
                            goto done;
                    }
                }
            }
done:       ;
            while (z1yl) {
                area += z1yl->Z.area();
                Zlist *zx = z1yl;
                z1yl = z1yl->next;
                delete zx;
            }
        }
    }
    return (area);
}


double
fcLayout::area_dielectric_yu(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    double area = 0.0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            if (z1->Z.xur <= z1->Z.xul)
                continue;
            Zlist *z1yu =
                new Zlist(z1->Z.xul, z1->Z.zbot, z1->Z.xur, z1->Z.ztop);
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl > z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yu)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (z2->Z.yl != z1->Z.yu)
                            continue;
                        if (z2->Z.xlr <= z1->Z.xul || z2->Z.xll >= z1->Z.xur)
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.xll, z2->Z.zbot, z2->Z.xlr, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1yu);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    area += zn->Z.area();
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1yu, &Z2);
                        if (!z1yu)
                            goto done;
                    }
                }
            }
done:       ;
            while (z1yu) {
                while (z1yu) {
                    area += z1yu->Z.area();
                    Zlist *zx = z1yu;
                    z1yu = z1yu->next;
                    delete zx;
                }
            }
        }
    }
    return (area);
}


double
fcLayout::area_dielectric_left(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    double area = 0.0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            Zlist *z1l = new Zlist(z1->Z.yl, z1->Z.zbot, z1->Z.yu, z1->Z.ztop);
            double sl = z1->Z.slope_left();
            double atmp = 0.0;
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl >= z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yl)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (ovl.check_continue(z2->Z))
                            continue;
                        if (!rl_match(&z2->Z, &z1->Z))
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1l);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    atmp += zn->Z.area();
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1l, &Z2);
                        if (!z1l)
                            goto done;
                    }
                }
            }
done:       ;
            while (z1l) {
                atmp += z1l->Z.area();
                Zlist *zx = z1l;
                z1l = z1l->next;
                delete zx;
            }
            if (sl != 0.0)
                atmp += sqrt(1.0 + sl*sl);
            area += atmp;
        }
    }
    return (area);
}


double
fcLayout::area_dielectric_right(const Layer3d *l1) const
{
    double dc1 = l1->epsrel();
    double area = 0.0;
    for (const glYlist3d *y1 = l1->yl3d(); y1; y1 = y1->next) {
        for (const glZlist3d *z1 = y1->y_zlist; z1; z1 = z1->next) {
            Zlist *z1r = new Zlist(z1->Z.yl, z1->Z.zbot, z1->Z.yu, z1->Z.ztop);
            double sr = z1->Z.slope_right();
            double atmp = 0.0;
            fc_ovlchk_t ovl(z1->Z.minleft(), z1->Z.maxright(), z1->Z.yu);
            for (Layer3d *l = db3_stack; l; l = l->next()) {
                for (const glYlist3d *y2 = l->yl3d(); y2; y2 = y2->next) {
                    if (y2->y_yl >= z1->Z.yu)
                        continue;
                    if (y2->y_yu <= z1->Z.yl)
                        break;
                    for (const glZlist3d *z2 = y2->y_zlist; z2; z2 = z2->next) {
                        if (ovl.check_break(z2->Z))
                            break;
                        if (ovl.check_continue(z2->Z))
                            continue;
                        if (!rl_match(&z1->Z, &z2->Z))
                            continue;
                        if (z2->Z.zbot >= z1->Z.ztop ||
                                z2->Z.ztop <= z1->Z.zbot)
                            continue;

                        Zoid Z2(z2->Z.yl, z2->Z.zbot, z2->Z.yu, z2->Z.ztop);
                        if (l->index() > l1->index() && l->is_insulator()) {
                            // Avoid generating redundant panels.
                            double dc2 = l->epsrel();
                            if (dc1 != dc2) {
                                Zlist *zn = Zlist::copy(z1r);
                                Zlist::zl_and(&zn, &Z2);
                                while (zn) {
                                    atmp += zn->Z.area();
                                    Zlist *zx = zn;
                                    zn = zn->next;
                                    delete zx;
                                }
                            }
                        }
                        Zlist::zl_andnot(&z1r, &Z2);
                        if (!z1r)
                            goto done;
                    }
                }
            }
done:       ;
            while (z1r) {
                atmp += z1r->Z.area();
                Zlist *zx = z1r;
                z1r = z1r->next;
                delete zx;
            }
            if (sr != 0.0)
                atmp *= sqrt(1.0 + sr*sr);
            area += atmp;
        }
    }
    return (area);
}
// End of fcLayout functions.


void
fcCpanel::print_c(FILE *fp, const char *name, e_unit unit) const
{
    // dielectric constants must be scaled, since internal FastCap default
    // assumes meters.
    double esc = unit_t::units(unit)->sigma_factor();
    fprintf(fp, "C %-12s %.3e 0.0 0.0 0.0", name, outperm*esc);
}


// Static function.
void
fcCpanel::print_c_term(FILE *fp, bool addplus)
{
    fprintf(fp, "%s\n", addplus ? " +" : "");
}


// Static function.
void
fcCpanel::print_panel_begin(FILE *fp, const char *name)
{
    fprintf(fp, "File %s\n", name);
    fprintf(fp, "0 %s\n", name);
}


void
fcCpanel::print_panel(FILE *fp, int xo, int yo, e_unit unit,
    unsigned int maxdim, unsigned int *pcnt) const
{
    const double sc = unit_t::units(unit)->coord_factor();
    const char *ffmt = unit_t::units(unit)->float_format();
    if (maxdim > 0) {
        qflist3d *list = Q.split(maxdim);
        for (qflist3d *q = list; q; q = q->next) {
            q->Q.print(fp, group, xo, yo, sc, ffmt);
            if (pcnt)
                (*pcnt)++;
        }
        qflist3d::destroy(list);
    }
    else {
        Q.print(fp, group, xo, yo, sc, ffmt);
        if (pcnt)
            (*pcnt)++;
    }
}


// Static function.
void
fcCpanel::print_panel_end(FILE *fp)
{
    fprintf(fp, "End\n");
}
// End of fcCpanel functions.


void
fcDpanel::print_d(FILE *fp, const char *name, e_unit unit,
    const xyz3d *ipref) const
{
    // D name oprm iprm xt yt zt xr yr zr [-]

    if (ipref) {
        // The reference point given is in the "inperm" side.

        // Dielectric constants must be scaled, since internal FastCap
        // default assumes meters.
        const char *ffmt = unit_t::units(unit)->float_format();
        const double sc = unit_t::units(unit)->coord_factor();
        double esc = unit_t::units(unit)->sigma_factor();
        fprintf(fp, "D %s %.3e %.3e 0.0 0.0 0.0 ", name,
            outperm*esc, inperm*esc);
        char buf[64];
        sprintf(buf, ffmt, sc*ipref->x);
        char *s = buf + strlen(buf) - 1;
        while (isspace(*s))
            *s-- = 0;
        fprintf(fp, "%s ", buf);
        sprintf(buf, ffmt, sc*ipref->y);
        s = buf + strlen(buf) - 1;
        while (isspace(*s))
            *s-- = 0;
        fprintf(fp, "%s ", buf);
        sprintf(buf, ffmt, sc*ipref->z);
        s = buf + strlen(buf) - 1;
        while (isspace(*s))
            *s-- = 0;
        fprintf(fp, "%s -\n", buf);
        return;
    }

    // Create a reference point at "infinity" along the appropriate
    // axis.  The reference point is in the "outperm" region.
    const char *ref = 0;
    switch (type) {
    case fcP_NONE:
        break;
    case fcP_ZBOT:
        ref = "0.0 0.0 -1e9";
        break;
    case fcP_ZTOP:
        ref = "0.0 0.0 1e9";
        break;
    case fcP_YL:
        ref = "0.0 -1e9 0.0";
        break;
    case fcP_YU:
        ref = "0.0 1e9 0.0";
        break;
    case fcP_LEFT:
        ref = "-1e9 0.0 0.0";
        break;
    case fcP_RIGHT:
        ref = "1e9 0.0 0.0";
        break;
    }

    // dielectric constants must be scaled, since internal FastCap default
    // assumes meters.
    double esc = unit_t::units(unit)->sigma_factor();
    fprintf(fp, "D %s %.3e %.3e 0.0 0.0 0.0 %s\n", name,
        outperm*esc, inperm*esc, ref);
}


// Static function.
void
fcDpanel::print_panel_begin(FILE *fp, const char *name)
{
    fprintf(fp, "File %s\n", name);
    fprintf(fp, "0 %s\n", name);
}


void
fcDpanel::print_panel(FILE *fp, int xo, int yo, e_unit unit,
    unsigned int maxdim, unsigned int *pcnt) const
{
    const double sc = unit_t::units(unit)->coord_factor();
    const char *ffmt = unit_t::units(unit)->float_format();
    if (maxdim > 0) {
        qflist3d *list = Q.split(maxdim);
        for (qflist3d *q = list; q; q = q->next) {
            q->Q.print(fp, index, xo, yo, sc, ffmt);
            if (pcnt)
                (*pcnt)++;
        }
        qflist3d::destroy(list);
    }
    else {
        Q.print(fp, index, xo, yo, sc, ffmt);
        if (pcnt)
            (*pcnt)++;
    }
}


// Static function.
void
fcDpanel::print_panel_end(FILE *fp)
{
    fprintf(fp, "End\n");
}
// End of fcDpanel functions.

