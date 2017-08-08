
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
#include "edit.h"
#include "drc.h"
#include "editif.h"
#include "cd_celldb.h"
#include "cd_lgen.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_ylist.h"
#include "geo_zgroup.h"
#include "geo_polyobj.h"
#include "geo_grid.h"
#include "fio.h"
#include "fio_chd.h"
#include "si_lexpr.h"
#include "tech.h"
#include "tech_layer.h"
#include "promptline.h"
#include "errorlog.h"
#include "miscutil/filestat.h"
#include "miscutil/timer.h"
#include "miscutil/timedbg.h"
#include "miscutil/childproc.h"

#ifdef WIN32
#include "miscutil/msw.h"
#else
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#endif


//-----------------------------------------------------------------------------
// Evaluation functions


// Abort cDRCLLrunDRCregion after this many errors.
#define DRC_PTMAX 15

char *
cDRC::jobs()
{
    sLstr lstr;
    char buf[256];
    for (DRCjob *j = drc_job_list; j; j = j->next()) {
        sprintf(buf, "%-6u %s\n", j->pid(), j->cellname());
        lstr.add(buf);
    }
    return (lstr.string_trim());
}


// Perform DRC in AOI.
//
void
cDRC::runDRCregion(const BBox *AOI)
{
    if (!DSP()->CurCellName())
        return;
    dspPkgIf()->SetWorking(true);
    PL()->ShowPrompt("Working...");

    // clear existing errors
    WindowDesc *wd;
    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
    while ((wd = wgen.next()) != 0)
        showCurError(wd, ERASE);
    clearCurError();

    int tmpmax = maxErrors();
    setMaxErrors(DRC_PTMAX);
    sLstr lstr;
    batchTest(AOI, 0, &lstr, 0);
    setMaxErrors(tmpmax);
    if (getErrCount() >= DRC_PTMAX)
        PL()->ShowPromptV(
            "%d objects checked, hit maximum error count %d.",
            getObjCount(), DRC_PTMAX);
    else
        PL()->ShowPromptV("%d objects checked, %d errors found.",
            getObjCount(), getErrCount());
    if (getErrCount()) {
        wgen = WDgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0)
            showCurError(wd, DISPLAY);
    }
    Log()->PopUpErrEx(lstr.string() ? lstr.string() : "No errors", false,
        LW_LR);
    dspPkgIf()->SetWorking(false);
}


#ifdef WIN32

namespace {
    void thread_proc(void *arg)
    {
        PROCESS_INFORMATION *info = (PROCESS_INFORMATION*)arg;
        if (WaitForSingleObject(info->hProcess, INFINITE) == WAIT_OBJECT_0) {
            DWORD status;
            GetExitCodeProcess(info->hProcess, &status);
            char buf[256];
            if (status)
                sprintf(buf,
                    "Background DRC job %ld exited with error status %ld.",
                    info->dwProcessId, status);
            else
                sprintf(buf, "Background DRC job %ld done.",
                    info->dwProcessId);
            DRC()->removeJob(info->dwProcessId);

            // Can't use PopUpMessage() here, needs to be modal.
            MessageBox(0, buf, "Message", MB_ICONINFORMATION);
            CloseHandle(info->hProcess);
            delete info;
        }
    }
}

#endif


void
cDRC::runDRC(const BBox *AOI, bool backg, cCHD *chd, const char *cellname,
    bool flatten)
{
    const char *msg1 = "DRC done: %d errors recorded in file \"%s\".";
    char *errfile = 0;
    const char *topcname;
    if (chd) {
        if (cellname && *cellname)
            topcname = cellname;
        else
            topcname = chd->defaultCell(Physical);
    }
    else
        topcname = Tstring(DSP()->CurCellName());
    if (!topcname) {
        PL()->ShowPrompt("No applicable cell name!");
        return;
    }
           
    if (backg) {

#ifdef WIN32
        // Spin off a batch process to do the DRC.  This has much less
        // flexibility that fork/exec.

        if (chd) {
            Log()->ErrorLog(mh::DRC,
                "Sorry, Windows does not support CHD background jobs.\n");
            return;
        }
        if (drc_doing_grid) {
            Log()->ErrorLog(mh::DRC,
            "Sorry, Windows does not support partitioning grid in background "
            "jobs,\nrunning without grid.\n");
        }

        char *tf = filestat::make_temp("drc");
        stringlist *namelist = new stringlist(lstring::copy(topcname), 0);
        FIOcvtPrms prms;
        prms.set_destination(tf, Fgds);
        FIO()->ConvertToGds(namelist, &prms);

        filestat::queue_deletion(tf);
        stringlist::destroy(namelist);

        char cmdline[512];
        GetModuleFileName(0, cmdline, 512);
        if (Tech()->TechExtension() && *Tech()->TechExtension())
            sprintf(cmdline + strlen(cmdline), " -T%s",
                Tech()->TechExtension());
        // The new process will delete the temp file when done (@d directive).
        if (AOI) {
            sprintf(cmdline + strlen(cmdline),
                " -B-drc@w=%.3f,%.3f,%.3f,%.3f@m=%d@r=%d@d %s",
                MICRONS(AOI->left), MICRONS(AOI->bottom),
                MICRONS(AOI->right), MICRONS(AOI->top),
                maxErrors(), errorLevel(), tf);
        }
        else {
            sprintf(cmdline + strlen(cmdline), " -B-drc@m=%d@r=%d@d %s",
                maxErrors(), errorLevel(), tf);
        }
        delete [] tf;

        PROCESS_INFORMATION *info = msw::NewProcess(cmdline,
            DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, false);
        if (!info) {
            PL()->ShowPrompt("Couldn't execute background process.");
            return;
        }
        PL()->ShowPromptV("Starting DRC background process, id=%d.",
            info->dwProcessId);
        // Record this job.
        drc_job_list = new DRCjob(topcname, info->dwProcessId, drc_job_list);
        PopUpDrcRun(0, MODE_UPD);
        _beginthread(thread_proc, 0, info);
        return;

#else
        int pid;
        if ((pid = forkToBackground()) != 0) {
            // we're in parent
            PL()->ShowPromptV("Starting DRC background process, id=%d.", pid);

            // Record this job.
            drc_job_list = new DRCjob(topcname, pid, drc_job_list);
            PopUpDrcRun(0, MODE_UPD);
            return;
        }
        cancelNext();
        // open errors file
        errfile = cDRC::errFilename(topcname, (int)getpid());
        setErrFilePtr(0);
        if (filestat::create_bak(errfile))
            setErrFilePtr(fopen(errfile, "w"));
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        if (!errFilePtr()) {
            cTimer::milli_sleep(250);
            // xlib connection screws up without this
            exit(1);
        }
#endif
    }
    else {
        setErrFilePtr(0);
        errfile = cDRC::errFilename(topcname, 0);
        if (filestat::create_bak(errfile))
            setErrFilePtr(fopen(errfile, "w"));
        else
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        if (!errFilePtr()) {
            PL()->ShowPrompt("DRC aborted.");
            return;
        }
    }

    Tdbg()->start_timing("DRCcheck");

    if (XM()->RunMode() == ModeNormal) {
        dspPkgIf()->SetWorking(true);
        PL()->ShowPrompt("Working...");
    }

    XIrt xrt;
    if (chd)
        xrt = chdGridBatchTest(chd, topcname, AOI, errFilePtr(), flatten);
    else
        xrt = gridBatchTest(AOI, errFilePtr());
    if (xrt == XIbad)
        fprintf(errFilePtr(), "Warning: Run terminated with error.\n");
    else if (xrt == XIintr)
        fprintf(errFilePtr(), "Warning: Run terminated with user abort.\n");

    Tdbg()->stop_timing("DRCcheck", getObjCount());

    if (XM()->RunMode() == ModeNormal)
        PL()->ShowPromptV(msg1, getErrCount(), errfile);

    fclose (errFilePtr());
    setErrFilePtr(0);

    if (XM()->RunMode() == ModeNormal) {
        errReset(errfile, 0);
        if (getErrCount()) {
            WindowDesc *wd;
            WDgen wgen(WDgen::MAIN, WDgen::CDDB);
            while ((wd = wgen.next()) != 0)
                showCurError(wd, DISPLAY);
        }
        dspPkgIf()->SetWorking(false);
        delete [] errfile;
    }
    else {
#ifndef WIN32
        cTimer::milli_sleep(250);
#endif
        _exit(0);  // gtk error if exit() called due to atexit()
                   // and closed X file desc
    }
}


// This is specific to the interactive DRC function, check the added
// objects.  If errors are found, errBB is set to enclose all 'bad'
// objects, if it is not 0.
//
// True is returned if errors were found.
//
bool
cDRC::intrListTest(const op_change_t *list, BBox *errBB)
{
    const CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    if (!isInteractive())
        return (false);
    BBox AOI(CDnullBB);
    const op_change_t *oc = list;
    while (oc) {
        if (oc->oadd())
            AOI.add(&oc->oadd()->oBB());
        oc = EditIf()->ulFindNext(oc);
    }
    if (AOI.right <= AOI.left) {
        drc_err_count = 0;
        return (false);
    }
    if (!update_rule_disable())
        return (false);
    PL()->SavePrompt();
    PL()->ShowPrompt("Working (DRC) ...");
    DRClevelType errtmp = errorLevel();  // just report one error per object
    setErrorLevel(DRCone_err);;
    Blist *blist;
    if (init_drc(&AOI, &blist, true) != XIok)
        return (false);
    drc_doing_inter = true;  // force abort on interrupt, no confirmation
    bool aborting = false;

    for (oc = list; oc; oc = EditIf()->ulFindNext(oc)) {
        CDo *pointer = oc->oadd();
        if (!pointer)
            continue;
        if (pointer->has_flag(CDnoDRC))
            continue;
        if (pointer->type() == CDLABEL)
            continue;
        if (pointer->type() == CDINSTANCE) {
            if (!isIntrSkipInst() &&
                    !eval_instance((CDc*)pointer, errBB, blist)) {
                aborting = true;
                break;
            }
            if (check_interrupt(false)) {
                aborting = true;
                break;
            }
            continue;
        }
        CDl *ldesc = pointer->ldesc();
        if (skip_layer(ldesc))
            continue;

        DRCerrRet *er = 0;
        if (!Blist::intersect(blist, &pointer->oBB(), false)) {
            if (objectRules(pointer, 0, &er) != XIok) {
                aborting = true;
                break;
            }
            drc_num_checked++;
        }

        // If there are no errors thus far, check constraints implied
        // from other layers.  If the new object causes a violation, flag
        // it with the rule violated.
        //
        CDl *ld;
        CDlgenDrv lgen;
        while ((ld = lgen.next()) != 0) {
            if (er)
                break;
            if (ld == ldesc || !*tech_prm(ld)->rules_addr())
                continue;
            if (skip_layer(ld))
                continue;
            int i;
            const BBox *BBp;
            if ((i = bloatmst(ld, ldesc)) != 0) {
                // bloat search area to catch MinSpaceTo errors
                AOI = pointer->oBB();
                AOI.bloat(i);
                BBp = &AOI;
            }
            else
                BBp = &pointer->oBB();
            sPF gen(cursdp, BBp, ld, CDMAXCALLDEPTH);
            CDo *odesc;
            while ((odesc = gen.next(false, false)) != 0) {

                if (!Blist::intersect(blist, &odesc->oBB(), false)) {
                    if (objectRules(odesc, 0, &er, ldesc) != XIok) {
                        delete odesc;
                        aborting = true;
                        goto done;
                    }
                    drc_num_checked++;
                    if (er) {
                        cursdp = gen.cur_sdesc();
                        delete odesc;
                        break;
                    }
                }
                delete odesc;

                if (check_interrupt(false)) {
                    aborting = true;
                    goto done;
                }
            }
        }

        bool waserr = (er != 0);
        handle_errors(er, 0, pointer, errBB, &drc_err_list, 0, 0);
        if (check_interrupt(waserr)) {
            aborting = true;
            break;
        }
    }

done:
    close_drc(blist);
    if (!aborting)
        PL()->RestorePrompt();
    setErrorLevel(errtmp);
    drc_doing_inter = false;
    drc_stop_time = Timer()->elapsed_msec();
    return (drc_err_count > 0);
}


// Main function to check a hierarchy in batch mode.  The strategy is
// to use the pseudo-flat generator to obtain a transformed odesc for
// every object in the database, and perform drc on the copied object. 
// Upon return, the statistics are available in the DRCtestDesc struct
// globals.  If given, errBB will enclose all violating objects. 
// Output goes to fp, or if fp == 0 to lstr, or if lstr == 0 to the
// error log.
//
XIrt
cDRC::batchTest(const BBox *AOI, FILE *fp, sLstr *lstr, BBox *errBB,
    const char *fbmsg)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (XIbad);

    if (!drc_doing_grid) {
        if (!update_rule_disable())
            return (XIbad);
    }
    if (!AOI)
        AOI = cursdp->BB();

    if (XM()->RunMode() == ModeNormal && !drc_doing_grid) {
        PL()->ShowPrompt("Working ... ");
        eraseErrors(AOI);
    }
    Blist *blist;
    XIrt ret = init_drc(AOI, &blist, XM()->RunMode() != ModeNormal);
    if (ret != XIok)
        return (ret);

    DRCerrList *el0 = 0;
    {
        CDl *ld;
        CDlgenDrv lgen;
        while ((ld = lgen.next()) != 0) {
            if (!*tech_prm(ld)->rules_addr())
                continue;
            if (skip_layer(ld))
                continue;
            DRCerrRet *er = 0;
            ret = layerRules(ld, AOI, &er);
            handle_errors(er, 0, 0, errBB, &el0, fp, lstr);
            if (ret != XIok)
                break;
        }
    }

    if (ret == XIok) {

        bool pass_halo = false;
        BBox bltAOI;
        if (drc_doing_grid) {
            // Expand the region, if given, to include test areas.  This
            // is used to clip objects that extend outside of the AOI.
            if (AOI && *AOI != *cursdp->BB()) {
                int bloat_val = haloWidth();
                bltAOI = *AOI;
                bltAOI.bloat(bloat_val);
                pass_halo = true;
            }
        }

        CDl *ld;
        bool done = false;
        CDlgenDrv lgen;
        while ((ld = lgen.next()) != 0) {
            if (done)
                break;
            if (!*tech_prm(ld)->rules_addr())
                continue;
            if (skip_layer(ld))
                continue;
            sPF gen(cursdp, AOI, ld, CDMAXCALLDEPTH);
            CDo *odesc;
            while ((odesc = gen.next(false, false)) != 0) {
                if (XM()->RunMode() == ModeNormal &&
                        !(drc_num_checked % 500)) {
                    if (fbmsg) {
                        PL()->ShowPromptV("%s %d/%d", fbmsg,
                            drc_num_checked, drc_obj_count);
                    }
                    else {
                        PL()->ShowPromptV("Working ... %d/%d",
                            drc_num_checked, drc_obj_count);
                    }
                }
                drc_num_checked++;

                // throw out any that overlap NDRC layer
                if (!Blist::intersect(blist, &odesc->oBB(), false)) {

                    DRCerrRet *er;
                    ret = objectRules(odesc, pass_halo ? &bltAOI : 0, &er);
                    if (ret != XIok) {
                        delete odesc;
                        done = true;
                        break;
                    }

                    done = handle_errors(er, &gen, odesc, errBB, &el0,
                        fp, lstr);
                }
                delete odesc;

                if (done)
                    break;

                if (check_interrupt(false)) {
                    done = true;
                    ret = XIintr;
                    break;
                }
            }
        }
    }
    if (el0) {
        if (!drc_err_list)
            drc_err_list = el0;
        else {
            DRCerrList *el;
            for (el = el0; el->next(); el = el->next()) ;
            el->set_next(drc_err_list);
            drc_err_list = el0;
        }
    }
    close_drc(blist);
    drc_stop_time = Timer()->elapsed_msec();
    return (ret);
}


XIrt
cDRC::gridBatchTest(const BBox *AOI, FILE *outfp)
{
    const int gridsize = drc_grid_size;
    if (gridsize < 0) {
        Errs()->add_error("gridBatchTest: nagative coarse grid spacing.");
        return (XIbad);
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp) {
        Errs()->add_error("gridBatchTest: no current cell.");
        return (XIbad);
    }
    if (!update_rule_disable())
        return (XIbad);

    BBox coarseBB = AOI ? *AOI : *cursdp->BB();

    int nxc = 1, nyc = 1;
    int gsx = coarseBB.width();
    int gsy = coarseBB.height();
    if (gridsize > 0) {
        nxc = coarseBB.width()/gridsize + (coarseBB.width()%gridsize != 0);
        nyc = coarseBB.height()/gridsize + (coarseBB.height()%gridsize != 0);
        gsx = gridsize;
        gsy = gridsize;
    }
    int nvals = nxc*nyc;

    int bloat_val = haloWidth();

    int regcnt = 0;
    char *outfile = 0;
    FILE *fp = 0;
    if (!outfp) {
        outfile = errFilename(Tstring(cursdp->cellname()), 0);
        if (!filestat::create_bak(outfile)) {
            Errs()->add_error(
                "gridBatchTest: can't open backup output file %s.", outfile);
            delete [] outfile;
            return (XIbad);
        }
        fp = fopen(outfile, "w");
        if (!fp) {
            Errs()->add_error("gridBatchTest: can't open output file %s.",
                outfile);
            delete [] outfile;
            return (XIbad);
        }
    }
    else
        fp = outfp;

    if (XM()->RunMode() == ModeNormal) {
        PL()->ShowPrompt("Working ... ");
        eraseErrors(AOI);
    }
    if (!printFileHeader(fp, Tstring(cursdp->cellname()), AOI, 0)) {
        if (fp && fp != outfp)
            fclose(fp);
        return (XIbad);
    }

    long start_time = Timer()->elapsed_msec();
    unsigned int tot_errs = 0;
    unsigned int tot_checked = 0;
    drc_doing_grid = (nvals > 1);

    // If doing the full cell in more than one grid, do the strange
    // per-layer rules that are done only when checking the full cell.

    if (nvals > 1 && (!AOI || (*AOI >= *cursdp->BB()))) {
        DRCerrList *el0 = 0;
        CDl *ld;
        CDlgenDrv lgen;
        while ((ld = lgen.next()) != 0) {
            if (!*tech_prm(ld)->rules_addr())
                continue;
            if (skip_layer(ld))
                continue;
            DRCerrRet *er = 0;
            XIrt ret = layerRules(ld, AOI, &er);
            handle_errors(er, 0, 0, 0, &el0, fp, 0);
            if (ret != XIok)
                break;
        }
        if (el0) {
            if (!drc_err_list)
                drc_err_list = el0;
            else {
                DRCerrList *el;
                for (el = el0; el->next(); el = el->next()) ;
                el->set_next(drc_err_list);
                drc_err_list = el0;
            }
        }
        tot_errs += drc_err_count;
    }

    XIrt ret = XIok;
    for (int ic = 0; ic < nyc; ic++) {
        for (int jc = 0; jc < nxc; jc++) {
            int cur_cx = coarseBB.left + jc*gsx;
            int cur_cy = coarseBB.bottom + ic*gsy;
            BBox coarse_cBB(cur_cx, cur_cy,
                cur_cx + gsx, cur_cy + gsy);
            if (coarse_cBB.right > coarseBB.right)
                coarse_cBB.right = coarseBB.right;
            if (coarse_cBB.top > coarseBB.top)
                coarse_cBB.top = coarseBB.top;

            char fbbuf[64];
            sprintf(fbbuf, "Checking region %d of %d  ", ic*nxc + jc + 1,
                nvals);
            PL()->ShowPromptV("Starting %d of %d.", ic*nxc + jc + 1, nvals);

            BBox tcBB(coarse_cBB);
            tcBB.bloat(bloat_val);

            fprintf(fp, "# REGION %d (%g,%g %g,%g)\n", regcnt,
                MICRONS(coarse_cBB.left), MICRONS(coarse_cBB.bottom),
                MICRONS(coarse_cBB.right), MICRONS(coarse_cBB.top));
            ret = batchTest(&coarse_cBB, fp, 0, 0, nvals == 1 ? 0 : fbbuf);
            regcnt++;

            tot_errs += drc_err_count;
            tot_checked += drc_num_checked;

            if (ret != XIok)
                goto done;
        }
    }
done:
    drc_start_time = start_time;
    drc_err_count = tot_errs;
    drc_num_checked = tot_checked;
    drc_doing_grid = false;
    printFileEnd(fp, getErrCount(), true);
    if (fp && fp != outfp)
        fclose(fp);
    if (outfile) {
        if (ret == XIok)
            errReset(outfile, 0);
        delete [] outfile;
    }
    return (ret);
}


// This variation works from a CHD, so does not require the entire
// design to be in memory.  Rather, is uses a grid, and reads only the
// cells needed to describe the grid area.  If gridsize == 0, the whole
// design is done in one shot (same as regular batch).  If outfp is given,
// output goes to that file.  Otherwise, a file is created in the CWD as
// "drcerr.log.cellname", which is used to update the Next command when
// finished.
//
XIrt
cDRC::chdGridBatchTest(cCHD *chd, const char *cellname, const BBox *AOI,
    FILE *outfp, bool flatten)
{
    const int gridsize = drc_grid_size;
    if (!chd) {
        Errs()->add_error("chdGridBatchTest: null CHD pointer given.");
        return (XIbad);
    }
    if (gridsize < 0) {
        Errs()->add_error("chdGridBatchTest: nagative coarse grid spacing.");
        return (XIbad);
    }
    if (!update_rule_disable())
        return (XIbad);

    symref_t *top = chd->findSymref(cellname, Physical, true);
    if (!top) {
        Errs()->add_error("chdGridBatchTest: unresolved top cell.");
        return (XIbad);
    }
    chd->setBoundaries(top);

    BBox coarseBB = AOI ? *AOI : *top->get_bb();

    int nxc = 1, nyc = 1;
    int gsx = coarseBB.width();
    int gsy = coarseBB.height();
    if (gridsize > 0) {
        nxc = coarseBB.width()/gridsize + (coarseBB.width()%gridsize != 0);
        nyc = coarseBB.height()/gridsize + (coarseBB.height()%gridsize != 0);
        gsx = gridsize;
        gsy = gridsize;
    }
    int nvals = nxc*nyc;

    int bloat_val = haloWidth();

    int regcnt = 0;
    char *outfile = 0;
    FILE *fp = 0;
    if (!outfp) {
        outfile = errFilename(Tstring(top->get_name()), 0);
        if (!filestat::create_bak(outfile)) {
            Errs()->add_error(
                "chdGridBatchTest: can't open backup output file %s.",
                outfile);
            delete [] outfile;
            return (XIbad);
        }
        fp = fopen(outfile, "w");
        if (!fp) {
            Errs()->add_error("chdGridBatchTest: can't open output file %s.",
                outfile);
            delete [] outfile;
            return (XIbad);
        }
    }
    else
        fp = outfp;

    if (!printFileHeader(fp, Tstring(top->get_name()), AOI, chd)) {
        if (fp && fp != outfp)
            fclose(fp);
        return (XIbad);
    }

    char *st_old = lstring::copy(CDcdb()->tableName());

#define STNAME "drc_tmp_table"
    if (CDcdb()->findTable(STNAME)) {
        CDcdb()->switchTable(STNAME);
        CDcdb()->destroyTable(false);
    }

    long start_time = Timer()->elapsed_msec();
    unsigned int tot_errs = 0;
    unsigned int tot_checked = 0;
    drc_doing_grid = (nvals > 1);
    drc_with_chd = true;

    XIrt ret = XIok;
    for (int ic = 0; ic < nyc; ic++) {
        for (int jc = 0; jc < nxc; jc++) {
            int cur_cx = coarseBB.left + jc*gsx;
            int cur_cy = coarseBB.bottom + ic*gsy;
            BBox coarse_cBB(cur_cx, cur_cy,
                cur_cx + gsx, cur_cy + gsy);
            if (coarse_cBB.right > coarseBB.right)
                coarse_cBB.right = coarseBB.right;
            if (coarse_cBB.top > coarseBB.top)
                coarse_cBB.top = coarseBB.top;

            char fbbuf[64];
            sprintf(fbbuf, "Checking region %d of %d  ", ic*nxc + jc + 1,
                nvals);
            PL()->ShowPromptV("Starting %d of %d.", ic*nxc + jc + 1, nvals);

            BBox tcBB(coarse_cBB);
            tcBB.bloat(bloat_val);

            CDcdb()->switchTable(STNAME);
            bool ne = CD()->IsNoElectrical();
            CD()->SetNoElectrical(true);
            FIOcvtPrms prms;
            prms.set_allow_layer_mapping(true);
            prms.set_use_window(true);
            prms.set_window(&tcBB);
            prms.set_flatten(flatten);
            prms.set_clip(true);
            OItype oiret = chd->write(cellname, &prms, true);
            CD()->SetNoElectrical(ne);
            if (oiret != OIok) {
                Errs()->add_error("chdGridBatchTest: read failed at %d,%d.",
                    cur_cx, cur_cy);
                if (oiret == OIaborted)
                    ret = XIintr;
                else
                    ret = XIbad;
                goto done;
            }
            CDcellName tname = DSP()->MainWdesc()->CurCellName();
            DSP()->MainWdesc()->SetCurCellName(top->get_name());

            fprintf(fp, "# REGION %d (%g,%g %g,%g)\n", regcnt,
                MICRONS(coarse_cBB.left), MICRONS(coarse_cBB.bottom),
                MICRONS(coarse_cBB.right), MICRONS(coarse_cBB.top));

            ret = batchTest(&coarse_cBB, fp, 0, 0, nvals == 1 ? 0 : fbbuf);
            CDcdb()->destroyTable(false);
            DSP()->SetCurCellName(tname);
            regcnt++;

            tot_errs += drc_err_count;
            tot_checked += drc_num_checked;

            if (ret != XIok)
                goto done;
        }
    }
done:
    drc_start_time = start_time;
    drc_err_count = tot_errs;
    drc_num_checked = tot_checked;
    drc_doing_grid = false;
    drc_with_chd = false;
    printFileEnd(fp, getErrCount(), true);
    if (fp && fp != outfp)
        fclose(fp);
    CDcdb()->switchTable(st_old);
    delete [] st_old;
    if (outfile) {
        if (ret == XIok)
            errReset(outfile, 0);
        delete [] outfile;
    }
    return (ret);
}


// As for batchTest, but act on a list (slist) of objects (no subcells
// allowed).
//
void
cDRC::batchListTest(const CDol *slist, FILE *fp, sLstr *lstr, BBox *errBB)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return;
    if (!update_rule_disable())
        return;
    BBox AOI;
    CDol::computeBB(slist, &AOI);
    if (XM()->RunMode() == ModeNormal) {
        PL()->ShowPrompt("Working ... ");
        eraseErrors(&AOI);
    }
    Blist *blist;
    if (init_drc(&AOI, &blist, XM()->RunMode() != ModeNormal) != XIok) {
        if (XM()->RunMode() == ModeNormal)
            PL()->ShowPrompt("DRC aborted");
        return;
    }
    bool done = false;
    DRCerrList *el0 = 0;

    for (const CDol *sl = slist; sl; sl = sl->next) {
        CDo *odesc = sl->odesc;
        if (odesc->type() == CDLABEL || odesc->type() == CDINSTANCE)
            continue;
        if (skip_layer(odesc->ldesc()))
            continue;

        if (!(drc_num_checked % 50)) {
            if (XM()->RunMode() == ModeNormal)
                PL()->ShowPromptV("Working ... %d/%d", drc_num_checked,
                    drc_obj_count);
        }
        drc_num_checked++;

        // throw out any that overlap NDRC layer
        if (!Blist::intersect(blist, &odesc->oBB(), false)) {

            DRCerrRet *er;
            if (objectRules(odesc, 0, &er) != XIok) {
                done = true;
                break;
            }
            done = handle_errors(er, 0, odesc, errBB, &el0, fp, lstr);
        }

        if (done)
            break;

        if (check_interrupt(false)) {
            done = true;
            break;
        }
    }
    if (el0) {
        if (!drc_err_list)
            drc_err_list = el0;
        else {
            DRCerrList *el;
            for (el = el0; el->next(); el = el->next()) ;
            el->set_next(drc_err_list);
            drc_err_list = el0;
        }
    }
    close_drc(blist);
    drc_stop_time = Timer()->elapsed_msec();
}


// Do the Connected and NoHoles tests, which require processing
// of the entire layer, not on a per-object basis.  The Connected and
// NoHoles tests will be skipped unless the full chip is being tested.
//
XIrt
cDRC::layerRules(const CDl *ld, const BBox *AOI, DRCerrRet **eret)
{
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (XIbad);
    if (AOI != cursdp->BB() && !(*AOI >= *cursdp->BB()))
        return (XIok);
    if (AOI->left == AOI->right || AOI->bottom == AOI->top)
        return (XIok);
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next()) {
        if (td->inhibited() || drc_rule_disable[td->type()])
            continue;
        PolarityType polarity;
        if (td->type() == drConnected) {
            polarity = PolarityDark;
            if (XM()->RunMode() == ModeNormal)
                PL()->ShowPromptV(
                    "Checking \"Connected\" on %s...", ld->name());
        }
        else if (td->type() == drNoHoles) {
            polarity = PolarityClear;
            if (XM()->RunMode() == ModeNormal)
                PL()->ShowPromptV(
                    "Checking \"NoHoles\" on %s...", ld->name());
        }
        else
            continue;

        grd_t grd(AOI, grd_t::def_gridsize());
        const BBox *gBB;
        Zlist *zret = 0;
        while ((gBB = grd.advance()) != 0) {
            SIlexprCx cx(cursdp, CDMAXCALLDEPTH, gBB);
            Zlist *zr = 0;
            XIrt ret = td->srcZlist(&cx, &zr, (polarity == PolarityClear));

            if (ret == XIbad) {
                PL()->ShowPrompt("Evaluation failed!");
                return (XIbad);
            }
            if (ret == XIintr)
                return (XIintr);
            if (zr) {
                Zlist *zn = zr;
                while (zn->next)
                    zn = zn->next;
                zn->next = zret;
                zret = zr;
            }
        }
        if (!zret)
            continue;

        if (td->type() == drConnected) {
            Ylist *y0 = new Ylist(zret);
            if (!y0)
                continue;
            Zgroup *g = Ylist::group(y0);

            // There should be exactly one group.  If multiple groups,
            // the one with the largest area is the good one.

            if (g->num > 1) {
                double amax = 0;
                int skipme = -1;
                for (int i = 0; i < g->num; i++) {
                    double a = Zlist::area(g->list[i]);
                    if (a > amax) {
                        amax = a;
                        skipme = i;
                    }
                }
                for (int i = 0; i < g->num; i++) {
                    if (i == skipme)
                        continue;
                    DRCerrRet *er = new DRCerrRet(td, 0);
                    er->set_errtype(TT_CON);
                    BBox BB;
                    Zlist::BB(g->list[i], BB);
                    er->set_pbad(BB);
                    er->set_next(*eret);
                    *eret = er;
                }
            }
            delete g;
        }
        else if (td->type() == drNoHoles) {
            Ylist *y0 = new Ylist(zret);
            y0 = Ylist::remove_backg(y0, AOI);
            if (!y0)
                continue;
            Zgroup *g = Ylist::group(y0);

            // We've inverted the layer and thrown out any zoids in
            // the group that touch the edges, Any groups that
            // remain are holes.

            double minarea = td->area();
            int minwidth = td->value(0);
            for (int i = 0; i < g->num; i++) {
                BBox BB;
                if (minarea > 0.0) {
                    // MinHoleArea test
                    double area = Zlist::area(g->list[i]);
                    if (area >= minarea)
                        continue;
                }
                Zlist::BB(g->list[i], BB);
                if (minwidth > 0) {
                    // MinHoleWidth test
                    if (BB.width() >= minwidth && BB.height() >= minwidth)
                        continue;
                }
                DRCerrRet *er = new DRCerrRet(td, 0);
                er->set_errtype(TT_NOH);
                er->set_pbad(BB);
                er->set_next(*eret);
                *eret = er;
            }
            delete g;
        }
    }
    return (XIok);
}


// Check object.  Layer filtering is not done here.
//
XIrt
cDRC::objectRules(const CDo *odesc, const BBox *AOI, DRCerrRet **erptr,
    CDl *ltarget)
{
    *erptr = 0;
    DRCerrCx errs;
    XIrt ret = XIbad;
    if (odesc->type() == CDBOX || odesc->type() == CDPOLYGON ||
            odesc->type() == CDWIRE) {
        drc_obj_type = odesc->type();
        if (AOI && !(odesc->oBB() <= *AOI)) {
            // The object extends outside of the AOI.  Clip the object to
            // AOI, and run DRC on the pieces.  The "internal" edges
            // will be ignored, since the original object overlays them.

            ret = XIok;
            DRCerrRet *e0 = 0, *ee = 0;
            Zlist *zl = odesc->toZlist();
            Zoid Zc(AOI);
            Zlist::zl_and(&zl, &Zc);
            PolyList *p0 = Zlist::to_poly_list(zl);
            drc_obj_type = odesc->type();
            for (PolyList *p = p0; p; p = p->next) {

                PolyObj dpo(p->po, true);
                p->po.points = 0;
                // The dpo owns the points.

                if (dpo.points()) {
                    DRCtestDesc *td = *tech_prm(odesc->ldesc())->rules_addr();
                    while (td) {
                        td = td->testPoly(&dpo, ltarget, errs, &ret);
                        if (errorLevel() == DRCone_err && errs.has_errs())
                            break;
                    }
                    DRCerrRet *er = DRCerrRet::filter(errs.get_errs());
                    if (er) {
                        if (!e0)
                            e0 = ee = er;
                        else {
                            while (ee->next())
                                ee = ee->next();
                            ee->set_next(er);
                        }
                        if (errorLevel() == DRCone_err)
                            break;
                    }
                    if (ret != XIok)
                        break;
                }
            }
            PolyList::destroy(p0);
            *erptr = e0;
        }
        else {
            PolyObj dpo(odesc, true);
            if (dpo.points()) {
                ret = XIok;
                DRCtestDesc *td = *tech_prm(odesc->ldesc())->rules_addr();
                while (td) {
                    td = td->testPoly(&dpo, ltarget, errs, &ret);
                    if (errorLevel() == DRCone_err && errs.has_errs())
                        break;
                }
                *erptr = DRCerrRet::filter(errs.get_errs());
            }
        }
    }
    return (ret);
}


// Return the largest rule test dimension plus some extra.
//
int
cDRC::haloWidth()
{
    int dim = 0;
    CDl *ld;
    CDlgenDrv lgen;
    while ((ld = lgen.next()) != 0) {
        // Check all rules, ignore layer and rule filtering, so that
        // the halo width is constant run to run.

        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td;
                td = td->next()) {
            switch (td->type()) {
            case drNoRule:
            case drConnected:
            case drNoHoles:
                continue;

            case drExist:
            case drOverlap:
            case drIfOverlap:
            case drNoOverlap:
            case drAnyOverlap:
            case drPartOverlap:
            case drAnyNoOverlap:
                continue;

            case drMinArea:
            case drMaxArea:
                continue;

            case drMinEdgeLength:
            case drMaxWidth:
                if (td->dimen() > dim)
                    dim = td->dimen();
                continue;

            case drMinWidth:
                if (td->dimen() > dim)
                    dim = td->dimen();
                if (td->value(0) > dim)
                    dim = td->value(0);
                continue;

            case drMinSpace:
                {
                    const sTspaceTable *st = td->spaceTab();
                    if (st) {
                        st++;
                        int num = st->entries;
                        for (int i = 0; i < num; i++) {
                            if (st->dimen > dim)
                                dim = st->dimen;
                            st++;
                        }
                    }
                    if (td->value(0) > dim)
                        dim = td->value(0);
                    if (td->value(1) > dim)
                        dim = td->value(1);
                }
                continue;

            case drMinSpaceTo:
                if (td->dimen() > dim)
                    dim = td->dimen();
                if (td->value(0) > dim)
                    dim = td->value(0);
                if (td->value(1) > dim)
                    dim = td->value(1);
                continue;

            case drMinSpaceFrom:
                if (td->dimen() > dim)
                    dim = td->dimen();
                if (td->value(0) > dim)
                    dim = td->value(0);
                if (td->value(1) > dim)
                    dim = td->value(1);
                if (td->value(2) > dim)
                    dim = td->value(2);
                continue;

            case drMinOverlap:
            case drMinNoOverlap:
                if (td->dimen() > dim)
                    dim = td->dimen();
                continue;

            case drUserDefinedRule:
                const DRCtest *ur = td->userRule();
                if (ur) {
                    for (DRCtestCnd *tc = ur->tests(); tc; tc = tc->next()) {
                        if (tc->dimension() > dim)
                            dim = tc->dimension();
                    }
                }
                break;
            }
        }
    }
    dim += dim/4;  // Add some slop.
    if (dim < 50)
        dim = 50;
    return (dim);
}


#ifndef WIN32

namespace {
    // Inform when child process finishes.
    //
    void
    child_hdlr(int pid, int status, void*)
    {
        char buf[128];
        *buf = '\0';
        if (WIFEXITED(status)) {
            DRC()->removeJob(pid);
            sprintf(buf, "Process %d exited ", pid);
            if (WEXITSTATUS(status))
                sprintf(buf + strlen(buf), "with error status %d.",
                    WEXITSTATUS(status));
            else {
                strcat(buf, "normally.");
                DRC()->errReset(0, pid);
            }
            DRC()->PopUpDrcRun(0, MODE_UPD);
        }
        else if (WIFSIGNALED(status)) {
            DRC()->removeJob(pid);
            sprintf(buf, "Process %d exited on signal %d.", pid,
                WIFSIGNALED(status));
            DRC()->PopUpDrcRun(0, MODE_UPD);
        }
        if (*buf)
            DSPmainWbag(PopUpMessage(buf, false))
    }
}


// Fork, and in the child process halt graphics and detach from the
// parent's descriptors.  Set up signal handling in the parent, and
// set the child to ignore SIGHUP.  Used to fork off a process to do
// DRC in the background.
//
int
cDRC::forkToBackground()
{
    int pid;
    if ((pid = fork()) != 0) {
        Proc()->RegisterChildHandler(pid, child_hdlr, 0);
        return (pid);
    }

    // halt graphics
    XM()->SetRunMode(ModeBackground);
    dspPkgIf()->ReinitNoGraphics();

    // go to bg
    for (int i = 0; i < 10; i++)
        close(i);
    open("/", O_RDONLY);
    dup2(0, 1);
    dup2(0, 2);
    // Disconnect from the controlling terminal
    setsid();
    return (0);
}

#endif


// This opens a DRC evaluation.  Count the number of objects to be
// considered.  If blist is not 0, create a list of boxes from the
// layer named "NDRC", we don't perform drc on any object touching or
// overlapping boxes on this layer.
//
XIrt
cDRC::init_drc(const BBox *AOI, Blist **blist, bool skip_cnt)
{
    DSP()->SetInterrupt(DSPinterNone);
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (XIbad);

    // Evaluate the derived layers.  First find the derived layers used.

    delete drc_drv_tab;
    drc_drv_tab = new SymTab(false, false);
    CDlgenDrv lgen;
    CDl *ld;
    while ((ld = lgen.next()) != 0) {
        if (skip_layer(ld))
            continue;
        bool added_me = false;
        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td;
                td = td->next()) {
            if (td->inhibited() || drc_rule_disable[td->type()])
                continue;
            if (!added_me && ld->layerType() == CDLderived) {
                drc_drv_tab->add((unsigned long)ld, 0, true);
                added_me = true;
            }

            CDll *l0 = td->sourceLayers();
            for (CDll *l = l0; l; l = l->next) {
                if (l->ldesc->layerType() == CDLderived)
                    drc_drv_tab->add((unsigned long)l->ldesc, 0, true);
            }
            CDll::destroy(l0);
            l0 = td->targetLayers();
            for (CDll *l = l0; l; l = l->next) {
                if (l->ldesc->layerType() == CDLderived)
                    drc_drv_tab->add((unsigned long)l->ldesc, 0, true);
            }
            CDll::destroy(l0);
        }
    }

    // The drc_drv_tab contains all of the derived layer descs used
    // in the rules.  We now create the layer content, but this has
    // to be done in order so that referenced layers will be filled
    // before used.

    int objcnt = 0;
    if (drc_drv_tab->allocated() > 0) {
        BBox BB(AOI ? *AOI : *cursdp->BB());
        if (BB != *cursdp->BB()) {

            // If we're not doing the whole cell, find the BB of the
            // objects being tested, and bloat by the halo.  This will
            // be used when evaluating derived layers.  Save the
            // object count as we can avoid counting below.

            BBox xBB(BB);
            CDl *tld;
            CDlgenDrv tlgen;
            while ((tld = tlgen.next()) != 0) {
                if (!*tech_prm(tld)->rules_addr())
                    continue;
                sPF gen(cursdp, &BB, tld, CDMAXCALLDEPTH);
                CDo *odesc;
                while ((odesc = gen.next(false, false)) != 0) {
                    xBB.add(&odesc->oBB());
                    delete odesc;
                    objcnt++;
                }
            }
            BB = xBB;
            BB.bloat(haloWidth());
            if (BB.left < cursdp->BB()->left)
                BB.left = cursdp->BB()->left;
            if (BB.bottom < cursdp->BB()->bottom)
                BB.bottom = cursdp->BB()->bottom;
            if (BB.right > cursdp->BB()->right)
                BB.right = cursdp->BB()->right;
            if (BB.top > cursdp->BB()->top)
                BB.top = cursdp->BB()->top;
        }

        SymTabGen stgen(drc_drv_tab);
        SymTabEnt *ent;
        CDll *l0 = 0;
        while ((ent = stgen.next()) != 0)
            l0 = new CDll((CDl*)ent->stTag, l0);
        XIrt ret = ED()->evalDerivedLayers(&l0, cursdp, AOI ? &BB : 0);

        // Update the table, for derived expression clearing in
        // close_drc.  The list contains actual layers evaluated.

        delete drc_drv_tab;
        drc_drv_tab = 0;
        if (l0) {
            drc_drv_tab = new SymTab(false, false);
            for (CDll *l = l0; l; l = l->next)
                drc_drv_tab->add((unsigned long)l->ldesc, 0, false);
            CDll::destroy(l0);
        }

        if (ret != XIok) {
            close_drc(0);
            return (ret);
        }
    }
    else {
        delete drc_drv_tab;
        drc_drv_tab = 0;
    }

    sPF::set_skip_drc(true);  // ignore objects with CDNoDRC set
    drc_start_time = Timer()->elapsed_msec();

    drc_num_checked = 0;
    drc_err_count = 0;

    if (!skip_cnt) {
        // Count the objects to be considered.
        if (!objcnt) {
            CDl *tld;
            CDlgenDrv tlgen;
            while ((tld = tlgen.next()) != 0) {
                if (skip_layer(tld))
                    continue;
                if (!*tech_prm(tld)->rules_addr())
                    continue;
                sPF gen(cursdp, AOI, tld, CDMAXCALLDEPTH);
                CDo *odesc;
                while ((odesc = gen.next(true, false)) != 0)
                    objcnt++;
            }
        }
        drc_obj_count = objcnt;
    }
    else
        drc_obj_count = 0;

    // Generate a list of boxes from the ndrc layer.
    if (blist) {
        *blist = 0;
        CDl *nodrcld = CDldb()->findLayer("NDRC", Physical);
        if (nodrcld) {
            Blist *bl0 = 0;
            sPF gen(cursdp, AOI, nodrcld, CDMAXCALLDEPTH);
            CDo *odesc;
            while ((odesc = gen.next(false, false)) != 0) {
                if (odesc->type() != CDBOX) {
                    delete odesc;
                    continue;
                }
                Blist *bl = new Blist;
                bl->BB = odesc->oBB();
                delete odesc;
                bl->next = bl0;
                bl0 = bl;
            }
            bl0 = Blist::merge(bl0);
            *blist = bl0;
        }
    }
    return (XIok);
}


// Compose and save the error reports.  Return true if the error limit
// has been reached.
//
bool
cDRC::handle_errors(DRCerrRet *er, sPF *gen, const CDo *odesc, BBox *errBB,
    DRCerrList **pel0, FILE *fp, sLstr *lstr)
{
    if (!er || !pel0)
        return (false);
    if (odesc && errBB) {
        if (!drc_err_count)
            *errBB = odesc->oBB();
        else
            errBB->add(&odesc->oBB());
    }

    bool done = false;
    cTfmStack stk;
    while (er) {
        drc_err_count++;

        if (!odesc && errBB) {
            BBox xBB(er->pbad(0).x, er->pbad(0).y, er->pbad(2).x,
                er->pbad(2).y);
            xBB.fix();
            if (!drc_err_count)
                *errBB = xBB;
            else
                errBB->add(&xBB);
        }

        const CDs *sdesc = odesc && gen ? gen->cur_sdesc() : CurCell(Physical);

        char buf[256];
        char *s = er->errmsg(odesc);
        if (drc_doing_inter && !isIntrNoErrMsg()) {
            if (gen) {
                sprintf(buf, "In instance of %s:\n",
                    Tstring(sdesc->cellname()));
                char *str = lstring::copy(buf);
                str = lstring::build_str(str, s);
                Log()->WarningLog(mh::DRCViolation, str);
                delete [] str;
            }
            else
                Log()->WarningLog(mh::DRCViolation, s);
        }
        else if (fp || lstr ||
                (XM()->RunMode() == ModeNormal && !isIntrNoErrMsg())) {
            sprintf(buf, "DRC error %d in cell %s:\n", drc_err_count,
                Tstring(sdesc->cellname()));
            char *str = lstring::copy(buf);
            str = lstring::build_str(str, s);
            if (fp)
                fputs(str,fp);
            else if (lstr) {
                lstr->add(str);
                lstr->add("\n");
            }
            else
                Log()->WarningLog(mh::DRCViolation, str);
            delete [] str;
        }

        if (XM()->RunMode() == ModeNormal && !drc_with_chd) {
            sprintf(buf, "In %s: ", Tstring(sdesc->cellname()));
            char *str = lstring::copy(buf);
            str = lstring::build_str(str, s);
            delete [] s;
            s = strchr(str, '\n');
            if (s)
                *s++ = 0;
            DRCerrList *el = new DRCerrList;
            el->set_message(str);
            if (s)
                el->set_descr(s);
            delete [] str;
            // Transform to top level coords
            stk.TPush();
            stk.TLoad(CDtfRegI0);
            stk.TInverse();
            stk.TLoadInverse();
            el->transf(er, &stk);
            el->set_pointer(odesc ? odesc->copyObjectWithXform(&stk) : 0);
            stk.TPop();
            el->newstack(gen);
            el->set_next(*pel0);
            *pel0 = el;
        }
        else
            delete [] s;

        if (maxErrors() > 0 && drc_err_count >= maxErrors()) {
            done = true;
            DRCerrRet::destroy(er);
            break;
        }
        DRCerrRet *en = er->next();
        delete er;
        er = en;
    }
    return (done);
}


// Private function to find the MinSpaceTo dimension.
//
int
cDRC::bloatmst(const CDl *ld, const CDl *ldtarget)
{
    int dim = 0;
    for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = td->next()) {
        if (td->type() == drMinSpaceTo &&
                (!ldtarget || ldtarget == td->targetLayer())) {
            if (td->dimen() > dim)
                dim = td->dimen();
        }
    }
    return (dim);
}


// Private function to check for interrupts during testing.
//
bool
cDRC::check_interrupt(bool force)
{
    if (drc_abort) {
        drc_abort = false;
        PL()->ShowPrompt("Interrupted!");
        return (true);
    }
    if (force)
        drc_check_time = 0;
    if (Timer()->check_interval(drc_check_time)) {
        if (drc_num_checked) {
            dspPkgIf()->CheckForInterrupt();
            if (DSP()->Interrupt() && drc_doing_inter) {
                DSP()->SetInterrupt(DSPinterNone);
                PL()->ShowPrompt("Interrupted!");
                return (true);
            }
            if (XM()->ConfirmAbort())
                return (true);
        }
        if (drc_doing_inter) {
            if (intrMaxObjs() && drc_num_checked >= intrMaxObjs()) {
                PL()->ShowPrompt("DRC number limit reached.");
                return (true);
            }
            if (intrMaxTime() > 0 &&
                    Timer()->elapsed_msec() - drc_start_time > intrMaxTime()) {
                PL()->ShowPrompt("DRC time limit reached.");
                return (true);
            }
            if (intrMaxErrors() && drc_err_count >= intrMaxErrors()) {
                PL()->ShowPrompt("Reached max errors, DRC terminated.");
                return (true);
            }
        }
    }
    return (false);
}


// Private function to evaluate a subcell, used in listTest().  Each
// object in the subcell's hierarchy is checked.  If errBB is given,
// it will enclose all errors upon return.  Objects that overlap a box
// in blist are not checked.  Returns false if aborted by interrupt or
// error, true otherwise.
//
bool
cDRC::eval_instance(const CDc *cdesc, BBox *errBB, const Blist *blist)
{
    bool aborted = false;
    DRCerrList *el0 = 0;
    bool done = false;
    CDl *ld;
    CDlgenDrv lgen;
    while ((ld = lgen.next()) != 0) {
        if (done)
            break;
        if (!*tech_prm(ld)->rules_addr())
            continue;
        if (skip_layer(ld))
            continue;
        if (check_interrupt(true)) {
            done = true;
            aborted = true;
            break;
        }
        sPF gen(cdesc, &cdesc->oBB(), ld, CDMAXCALLDEPTH);
        CDo *odesc;
        while ((odesc = gen.next(false, false)) != 0) {

            // throw out any that overlap NDRC layer
            if (!Blist::intersect(blist, &odesc->oBB(), false)) {

                DRCerrRet *er;
                if (objectRules(odesc, 0, &er) != XIok) {
                    delete odesc;
                    done = true;
                    aborted = true;
                    break;
                }
                drc_num_checked++;
                handle_errors(er, &gen, odesc, errBB, &el0, 0, 0);
            }
            delete odesc;

            if (check_interrupt(false)) {
                done = true;
                aborted = true;
                break;
            }
        }
    }
    if (el0) {
        if (!drc_err_list)
            drc_err_list = el0;
        else {
            DRCerrList *el;
            for (el = el0; el->next(); el = el->next()) ;
            el->set_next(drc_err_list);
            drc_err_list = el0;
        }
    }
    return (!aborted);
}


// Close a DRC evaluation.
//
void
cDRC::close_drc(Blist *blist)
{
    Blist::destroy(blist);
    sPF::set_skip_drc(false);

    // Clear the derived layers that we created for DRC.
    if (drc_drv_tab) {
        SymTabGen stgen(drc_drv_tab);
        SymTabEnt *ent;
        while ((ent = stgen.next()) != 0) {
            CDl *ld = (CDl*)ent->stTag;
            CurCell(Physical)->clearLayer(ld);
        }
        delete drc_drv_tab;
        drc_drv_tab = 0;
    }
}


// Return true if we should skip evaluating the rules on this layer,
// due to the user's layer filtering.
//
bool
cDRC::skip_layer(const CDl *ld)
{
    if (!ld)
        return (false);
    if (!drc_layer_list)
        return (false);
    if (drc_use_layer_list == DrcListNone)
        return (false);

    // The layer list must be actually set to something non-space or
    // we ignore it.
    const char *s = drc_layer_list;
    while (isspace(*s))
        s++;
    if (!*s)
        return (false);

    bool found = false;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        CDl *tld = CDldb()->findLayer(tok, Physical);
        if (!tld)
            tld = CDldb()->findDerivedLayer(tok);
        delete [] tok;
        if (tld == ld) {
            found = true;
            break;
        }
    }
    if (drc_use_layer_list == DrcListOnly)
        return (!found);
    return (found);  // DrcListSkip
}


// Set the skip rule flags from the string.  This is called before
// starting a DRC run.
//
bool
cDRC::update_rule_disable()
{
    memset(drc_rule_disable, 0, sizeof(drc_rule_disable));
    if (drc_use_rule_list == DrcListNone)
        return (true);
    const char *s = drc_rule_list;
    if (!s)
        return (true);
    while (isspace(*s))
        s++;
    if (!*s)
        return (true);

    char *tok;
    bool found = false;
    while ((tok = lstring::gettok(&s)) != 0) {
        DRCtype t = DRCtestDesc::ruleType(tok);
        if (t == drNoRule) {
            Log()->ErrorLogV(mh::Variables,
            "Incorrect DrcRuleList: unknown rule %s", tok);
            delete [] tok;
            return (false);
        }
        delete [] tok;
        found = true;
    }
    if (!found) {
        Log()->ErrorLogV(mh::Variables,
            "Incorrect DrcRuleList: no rule names given.");
        return (false);
    }

    if (drc_use_rule_list == DrcListOnly)
        memset(drc_rule_disable, true, sizeof(drc_rule_disable));

    s = drc_rule_list;
    while ((tok = lstring::gettok(&s)) != 0) {
        DRCtype t = DRCtestDesc::ruleType(tok);
        delete [] tok;
        drc_rule_disable[t] = (drc_use_rule_list == DrcListSkip);
    }
    return (true);
}

