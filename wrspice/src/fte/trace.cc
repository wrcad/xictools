
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "frontend.h"
#include "runop.h"
#include "cshell.h"
#include "commands.h"
#include "toolbar.h"
#include "outplot.h"
#include "outdata.h"
#include "rundesc.h"

//
// The trace and iplot commands.
//

// keywords
const char *kw_trace  = "trace";
const char *kw_iplot  = "iplot";


// Trace a node.
//
void
CommandTab::com_trace(wordlist *wl)
{
    OP.TraceCmd(wl);
}


// Incrementally plot a value, similar to trace.
//
void
CommandTab::com_iplot(wordlist *wl)
{
    OP.iplotCmd(wl);
}
// End of CommandTab functions.


// Trace a node. Usage is "trace expr ..."
//
void
IFoutput::TraceCmd(wordlist *wl)
{
    const char *msg = "already tracing %s, ignored.\n";

    sDbComm *d = new sDbComm;
    d->set_type(DB_TRACE);
    d->set_active(true);
    d->set_string(wordlist::flatten(wl));
    d->set_number(o_debugs->new_count());

    if (CP.GetFlag(CP_INTERACTIVE) || !Sp.CurCircuit()) {
        if (o_debugs->traces()) {
            sDbComm *ld = 0;
            for (sDbComm *td = o_debugs->traces(); td; ld = td, td = td->next()) {
                if (lstring::eq(td->string(), d->string())) {
                    GRpkgIf()->ErrPrintf(ET_WARN, msg, td->string());
                    sDbComm::destroy(d);
                    o_debugs->decrement_count();
                    return;
                }
            }
            ld->set_next(d);
        }
        else
            o_debugs->set_traces(d);
    }
    else {
        sDebug *db = &Sp.CurCircuit()->debugs();
        if (db->traces()) {
            sDbComm *ld = 0;
            for (sDbComm *td = db->traces(); td; ld = td, td = td->next()) {
                if (lstring::eq(td->string(), d->string())) {
                    GRpkgIf()->ErrPrintf(ET_WARN, msg, td->string());
                    sDbComm::destroy(d);
                    o_debugs->decrement_count();
                    return;
                }
            }
            ld->set_next(d);
        }
        else
            db->set_traces(d);
    }
    ToolBar()->UpdateTrace();
}


// Incrementally plot a value, similar to trace.
//
void
IFoutput::iplotCmd(wordlist *wl)
{
    if (!wl) {
        GRpkgIf()->ErrPrintf(ET_ERRORS, "no vectors given.\n");
        return;
    }
    wl = wordlist::copy(wl);
    for (wordlist *ww = wl; ww; ww = ww->wl_next) {
        if (lstring::eq(ww->wl_word, ".")) {
            wordlist *wx = Sp.ExtractPlotCmd(0, "tran");
            if (!wx) {
                GRpkgIf()->ErrPrintf(ET_ERRORS,
                    "no vectors found for '.'.\n");
                wordlist::destroy(wl);
                return;
            }
            if (ww == wl)
                wl = wx;
            ww = ww->splice(wx);
            continue;
        }
        if (ww->wl_word[0] == '.' && ww->wl_word[1] == '@' &&
                isdigit(ww->wl_word[2])) {
            int n = atoi(ww->wl_word + 2);
            wordlist *wx = Sp.ExtractPlotCmd(n ? n-1 : n, "tran");
            if (!wx) {
                GRpkgIf()->ErrPrintf(ET_ERRORS,
                    "no vectors found for '.@%d'.\n", n);
                wordlist::destroy(wl);
                return;
            }
            if (ww == wl)
                wl = wx;
            ww = ww->splice(wx);
            continue;
        }
    }

    sDbComm *d = new sDbComm;
    d->set_type(DB_IPLOT);
    d->set_active(true);
    d->set_string(wordlist::flatten(wl));
    wordlist::destroy(wl);
    d->set_number(o_debugs->new_count());

    if (CP.GetFlag(CP_INTERACTIVE) || !Sp.CurCircuit()) {
        if (o_debugs->iplots()) {
            sDbComm *td;
            for (td = o_debugs->iplots(); td->next(); td = td->next()) ;
            td->set_next(d);
        }
        else
            o_debugs->set_iplots(d);
    }
    else {
        sDebug *db = &Sp.CurCircuit()->debugs();
        if (db->iplots()) {
            sDbComm *td;
            for (td = db->iplots(); td->next(); td = td->next()) ;
            td->set_next(d);
        }
        else
            db->set_iplots(d);
    }
    ToolBar()->UpdateTrace();
}

namespace {
    // Compute the last entry, and grow the iplot's dvecs.
    //
    void update_dvecs(sGraph *graph, sDbComm *db, sDataVec *xs, bool *vs_flag,
        sRunDesc *run)
    {
        const char *msg = "iplot #%d.\n";
        *vs_flag = false;
        sDvList *dvl = (sDvList*)graph->plotdata();
        wordlist *wl = CP.LexStringSub(db->string());
        if (!wl) {
            db->set_bad(true);
            GRpkgIf()->ErrPrintf(ET_INTERR, msg, 1);
            return;
        }
        run->scalarizeVecs();
        sPlot *plot = OP.curPlot();
        sFtCirc *circ = Sp.CurCircuit();
        OP.setCurPlot(run->runPlot());
        Sp.SetCurCircuit(run->circuit());
        pnlist *pl = Sp.GetPtree(wl, false);
        wordlist::destroy(wl);
        sDvList *dl0 = 0;
        if (pl) {
            Sp.SetFlag(FT_SILENT, true);  // silence "vec not found" msgs
            dl0 = Sp.DvList(pl);
            Sp.SetFlag(FT_SILENT, false);
        }
        OP.setCurPlot(plot);
        Sp.SetCurCircuit(circ);
        if (!dl0) {
            db->set_bad(true);
            GRpkgIf()->ErrPrintf(ET_INTERR, msg, 2);
            run->unscalarizeVecs();
            return;
        }
        sDvList *dl, *dd, *dp, *dn;
        for (dp = 0, dl = dl0; dl; dl = dn) {
            dn = dl->dl_next;
            if (!dl->dl_dvec) {
                // only from "vs"
                *vs_flag = true;
                if (!dp || !dn) {
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 3);
                    sDvList::destroy(dl0);
                    db->set_bad(true);
                    return;
                }
                xs = dn->dl_dvec;
                dp->dl_next = dn->dl_next;
                delete dl;
                delete dn;
                break;
            }
            dp = dl;
        }
        for (dl = dvl, dd = dl0; dl && dd;
                dl = dl->dl_next, dd = dd->dl_next) {
            sDataVec *vt = dl->dl_dvec;
            sDataVec *vf = dd->dl_dvec;

            if (vt->isreal()) {
                if (!vf->isreal()) {
                    db->set_bad(true);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 3);
                    break;
                }
                if (vt->length() >= vt->allocated()) {
                    double *tv = new double[vt->length() + 1];
                    for (int i = 0; i < vt->length(); i++)
                        tv[i] = vt->realval(i);
                    vt->set_realvec(tv, true);
                    vt->set_allocated(vt->length() + 1);
                }
                if (graph->gridtype() == GRID_SMITH) {
                    double re = vf->realval(0);
                    vt->set_realval(vt->length(), (re - 1) / (re + 1));
                }
                else
                    vt->set_realval(vt->length(), vf->realval(0));
            }
            else {
                if (vf->isreal()) {
                    db->set_bad(true);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 4);
                    break;
                }
                if (vt->length() >= vt->allocated()) {
                    complex *cv = new complex[vt->length() + 1];
                    for (int i = 0; i < vt->length(); i++)
                        cv[i] = vt->compval(i);
                    vt->set_compvec(cv, true);
                    vt->set_allocated(vt->length() + 1);
                }
                if (graph->gridtype() == GRID_SMITH) {
                    double re = vf->realval(0);
                    double im = vf->imagval(0);
                    double dnom = (re+1.0)*(re+1.0) + im*im;
                    vt->set_realval(vt->length(), (re*re + im*im + 1.0)/dnom);
                    vt->set_imagval(vt->length(), 2.0*im/dnom);
                }
                else
                    vt->set_compval(vt->length(), vf->compval(0));
            }
            vt->set_length(vt->length() + 1);;
        }
        if (dl || dd) {
            db->set_bad(true);
            GRpkgIf()->ErrPrintf(ET_INTERR, msg, 5);
        }
        sDataVec *scale = dvl->dl_dvec->scale();
        // is it already updated?
        for (dl = dvl; dl; dl = dl->dl_next)
            if (scale == dl->dl_dvec)
                break;
        if (!dl) {
            // no need to Smith transform the scale
            if (xs->isreal()) {
                if (!scale->isreal()) {
                    db->set_bad(true);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 6);
                }
                if (scale->length() >= scale->allocated()) {
                    double *tv = new double[scale->length() + 1];
                    for (int i = 0; i < scale->length(); i++)
                        tv[i] = scale->realval(i);
                    scale->set_realvec(tv, true);
                    scale->set_allocated(scale->length() + 1);
                }
                scale->set_realval(scale->length(), xs->realval(0));
            }
            else {
                if (scale->isreal()) {
                    db->set_bad(true);
                    GRpkgIf()->ErrPrintf(ET_INTERR, msg, 7);
                }
                if (scale->length() >= scale->allocated()) {
                    complex *cv = new complex[scale->length() + 1];
                    for (int i = 0; i < scale->length(); i++)
                        cv[i] = scale->compval(i);
                    scale->set_compvec(cv, true);
                    scale->set_allocated(scale->length() + 1);
                }
                scale->set_compval(scale->length(), xs->compval(0));
            }
            scale->set_length(scale->length() + 1);
        }
        sDvList::destroy(dl0);

        // The following resizes the storage in scroll mode.  The scale of
        // the analysis must be monotonically increasing.
        //
        if (xs->flags() & VF_ROLLOVER) {
            int i;
            for (i = 0; i < scale->length(); i++) {
                // This is true when we bump up the lower limit
                if (scale->realval(i) > graph->datawin().xmin)
                    break;
            }
            if (i) {
                for (dl = dvl; dl; dl = dl->dl_next) {
                    sDataVec *v = dl->dl_dvec;
                    v->set_length(v->length() - i);
                    v->set_allocated(v->allocated() - i);
                    if (v->isreal()) {
                        double *d = new double[v->allocated()];
                        for (int j = 0; j < v->allocated(); j++)
                            d[j] = v->realval(j+i);
                        v->set_realvec(d, true);
                    }
                    else {
                        complex *c = new complex[v->allocated()];
                        for (int j = 0; j < v->allocated(); j++)
                            c[j] = v->compval(j+i);
                        v->set_compvec(c, true);
                    }
                }
                for (dl = dvl; dl; dl = dl->dl_next) {
                    if (scale == dl->dl_dvec)
                        break;
                }
                if (!dl) {
                    scale->set_length(scale->length() - i);
                    scale->set_allocated(scale->allocated() - i);
                    if (scale->isreal()) {
                        double *d = new double[scale->allocated()];
                        for (int j = 0; j < scale->allocated(); j++)
                            d[j] = scale->realval(j+i);
                        scale->set_realvec(d, true);
                    }
                    else {
                        complex *c = new complex[scale->allocated()];
                        for (int j = 0; j < scale->allocated(); j++)
                            c[j] = scale->compval(j+i);
                        scale->set_compvec(c, true);
                    }
                }
            }
        }
        run->unscalarizeVecs();
    }


    void update_dims(sPlot *pl, sGraph *graph)
    {
        if (!pl || !graph)
            return;
        sDataVec *xs = pl->scale();
        if (xs && xs->numdims() > 1) {
            sDvList *dl0 = (sDvList*)graph->plotdata();
            sDataVec *ys = dl0->dl_dvec->scale();
            ys->set_numdims(xs->numdims());
            if (ys) {
                for (int i = 0; i < xs->numdims(); i++)
                    ys->set_dims(i, xs->dims(i));
                for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
                    sDataVec *v = dl->dl_dvec;
                    v->set_numdims(xs->numdims());
                    for (int i = 0; i < xs->numdims(); i++)
                        v->set_dims(i, xs->dims(i));
                }
            }
        }
    }
}


// Do some incremental plotting. 3 cases -- first, if length < IPOINTMIN, 
// don't do anything. Second, if length = IPOINTMIN, plot what we have
// so far. Third, if length > IPOINTMIN, plot the last points and resize
// if needed.
// Note we don't check for pole / zero because they are of length 1.
//
#define FACTOR .25     // How much to expand the scale during iplot.
#define IPLTOL 2e-3    // Allow this fraction out of range before redraw.

void
IFoutput::iplot(sDbComm *db, sRunDesc *run)
{
    if (!run || !db || db->bad())
        return;
    if (Sp.GetFlag(FT_GRDB)) {
        GRpkgIf()->ErrPrintf(ET_MSGS, "Entering iplot, len = %d\n\r",
            run->pointCount());
    }

    sGraph *graph;
    sDvList *dl0, *dl;
    if (!db->graphid()) {
        // Draw the grid for the first time, and plot everything

        // Error handling:  If an error occurs during plot initialization, 
        // set_bad() is set true (graphic() remains 0).
        // If an error occurs later, set_bad() is set true, and the plot
        // is "frozen".

        wordlist *wl = CP.LexStringSub(db->string());
        if (!wl) {
            db->set_bad(true);
            return;
        }
        dl0 = 0;

        sDataVec *xs = run->runPlot()->scale();
        if (!xs) {
            db->set_bad(true);
            return;
        }
        if (xs->length() < IPOINTMIN)
            return;
        int rlen = xs->allocated();

        sPlot *plot = OP.curPlot();
        sFtCirc *circ = Sp.CurCircuit();
        OP.setCurPlot(run->runPlot());
        Sp.SetCurCircuit(run->circuit());
        pnlist *pl = Sp.GetPtree(wl, false);
        if (pl)
            dl0 = Sp.DvList(pl);
        OP.setCurPlot(plot);
        Sp.SetCurCircuit(circ);
        if (!dl0) {
            db->set_bad(true);
            return;
        }
        // check for "vs"
        bool foundvs = false;
        sDvList *dp, *dn;
        for (dp = 0, dl = dl0; dl; dl = dn) {
            dn = dl->dl_next;
            if (!dl->dl_dvec) {
                // only from "vs"
                if (!dp || !dn) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "misplaced vs arg.\n");
                    sDvList::destroy(dl0);
                    dl0 = 0;
                    db->set_bad(true);
                    return;
                }
                foundvs = true;
                xs = dn->dl_dvec;
                dp->dl_next = dn->dl_next;
                delete dl;
                delete dn;
                break;
            }
            dp = dl;
        }
        if (!foundvs) {
            xs = xs->copy();
            xs->newtemp();
        }

        dl = dl0;
        sGrInit gr;
        if (!GP.Setup(&gr, &dl0, 0, xs, 0)) {
            db->set_bad(true);
            return;
        }
        if (dl != dl0)
            // copied
            sDvList::destroy(dl);
        gr.command = wl;

        // the dvec list is copied to graph->plotdata

        bool reuse = false;
        graph = 0;
        if (db->reuseid() > 0) {
            // if this is set, reuse the iplot window
            graph = GP.FindGraph(db->reuseid());
            if (graph) {
                graph->dev()->Clear();
                reuse = true;
                graph->gr_reset();
            }
        }
        graph = GP.Init(dl0, &gr, graph);
        sDvList::destroy(dl0);
        if (graph)
            db->set_graphid(graph->id());
        if (!db->graphid()) {
            db->set_bad(true);
            return;
        }

        // push graph for possible use by err/interrupt handlers
        if (GRpkgIf()->CurDev() &&
                GRpkgIf()->CurDev()->devtype == GRfullScreen)
            GP.PushGraphContext(graph);

        // extend the vectors to the length of the scale
        dl0 = (sDvList*)graph->plotdata();
        if (dl0->dl_dvec->scale()->allocated() != rlen) {
            xs = dl0->dl_dvec->scale();
            if (xs->isreal()) {
                double *d = new double[rlen];
                if (!d)
                    return;
                for (int i = 0; i < xs->length(); i++)
                    d[i] = xs->realval(i);
                xs->set_realvec(d, true);
            }
            else {
                complex *c = new complex[rlen];
                if (!c)
                    return;
                for (int i = 0; i < xs->length(); i++)
                    c[i] = xs->compval(i);
                xs->set_compvec(c, true);
            }
            xs->set_allocated(rlen);
        }
        for (dl = dl0; dl; dl = dl->dl_next) {
            if (dl->dl_dvec->allocated() != rlen) {
                if (dl->dl_dvec->isreal()) {
                    double *d = new double[rlen];
                    if (!d)
                        return;
                    for (int i = 0; i < dl->dl_dvec->length(); i++)
                        d[i] = dl->dl_dvec->realval(i);
                    dl->dl_dvec->set_realvec(d, true);
                }
                else {
                    complex *c = new complex[rlen];
                    if (!c)
                        return;
                    for (int i = 0; i < dl->dl_dvec->length(); i++)
                        c[i] = dl->dl_dvec->compval(i);
                    dl->dl_dvec->set_compvec(c, true);
                }
                dl->dl_dvec->set_allocated(rlen);
            }
        }
        if (reuse)
            graph->gr_redraw();
        return;
    }

    graph = GP.FindGraph(db->graphid());
    if (!graph) {
        // shouldn't happen
        db->set_bad(true);
        return;
    }
    graph->set_noevents(true);  // suppress event checking

    dl0 = (sDvList*)graph->plotdata();
    bool vs_flag;
    update_dvecs(graph, db, run->runPlot()->scale(), &vs_flag, run);
    sDataVec *xs = dl0->dl_dvec->scale();
    int len = xs->length() - 1;

    // First see if we have to make the screen bigger
    double val = xs->realval(len - 1);
    if (Sp.GetFlag(FT_GRDB))
        GRpkgIf()->ErrPrintf(ET_MSGS, "x = %G\n\r", val);

    // hack to keep from overshooting range
    double start, stop;
    run->runPlot()->range(&start, &stop, 0);
    if (stop < start) {
        double tmp = start;
        start = stop;
        stop = tmp;
    }

    bool changed = false;
    if (graph->rawdata().xmax < graph->rawdata().xmin)
        graph->rawdata().xmax = graph->rawdata().xmin;

    if (graph->datawin().xmin == graph->datawin().xmax)
        changed = true;
    else if (val < graph->datawin().xmin) {
        changed = true;
        if (run->runPlot()->scale()->flags() & VF_ROLLOVER) {
            double dxs = stop - start;
            if (dxs > 0 && graph->datawin().xmax - graph->datawin().xmin <
                    .99*dxs) {
                graph->rawdata().xmin = graph->datawin().xmin;
                do {
                    graph->rawdata().xmin -=
                        (graph->datawin().xmax - graph->rawdata().xmin)*FACTOR;

                    if (!vs_flag && graph->rawdata().xmin <
                            graph->rawdata().xmax - dxs) {
                        graph->rawdata().xmin = graph->rawdata().xmax - dxs;
                        break;
                    }
                } while (val < graph->rawdata().xmin);
            }
            else {
                int nsp = graph->xaxis().lin.numspace;
                double del =
                    (graph->datawin().xmax - graph->datawin().xmin)/nsp;
                graph->datawin().xmax -= del;
                graph->datawin().xmin -= del;
                graph->rawdata().xmin -= del;
                graph->rawdata().xmax -= del;
            }
        }
        else {
            graph->rawdata().xmin = graph->datawin().xmin;
            do {
                if (Sp.GetFlag(FT_GRDB)) {
                    GRpkgIf()->ErrPrintf(ET_MSGS, "resize: xlo %G -> %G\n\r", 
                        graph->datawin().xmin, graph->datawin().xmin -
                        (graph->datawin().xmax -
                        graph->datawin().xmin)*FACTOR);
                }

                graph->rawdata().xmin -=
                    (graph->datawin().xmax - graph->rawdata().xmin)*FACTOR;

                if (!vs_flag && start != stop &&
                        graph->rawdata().xmin < start) {
                    graph->rawdata().xmin = start;
                    break;
                }
            } while (val < graph->rawdata().xmin);
        }
    }
    else if (val > graph->datawin().xmax) {
        changed = true;
        if (run->runPlot()->scale()->flags() & VF_ROLLOVER) {
            double dxs = stop - start;
            if (dxs > 0 && graph->datawin().xmax - graph->datawin().xmin <
                    .99*dxs) {
                graph->rawdata().xmax = graph->datawin().xmax;
                do {
                    graph->rawdata().xmax +=
                        (graph->rawdata().xmax - graph->datawin().xmin)*FACTOR;

                    if (!vs_flag && graph->rawdata().xmax >
                            graph->rawdata().xmin + dxs) {
                        graph->rawdata().xmax = graph->rawdata().xmin + dxs;
                        break;
                    }
                } while (val > graph->rawdata().xmax);
            }
            else {
                int nsp = graph->xaxis().lin.numspace;
                double del =
                    (graph->datawin().xmax - graph->datawin().xmin)/nsp;
                graph->datawin().xmax += del;
                graph->datawin().xmin += del;
                graph->rawdata().xmin += del;
                graph->rawdata().xmax += del;
            }
        }
        else {
            graph->rawdata().xmax = graph->datawin().xmax;
            do {
                if (Sp.GetFlag(FT_GRDB)) {
                    GRpkgIf()->ErrPrintf(ET_MSGS, "resize: xhi %G -> %G\n\r", 
                        graph->datawin().xmax, graph->datawin().xmax +
                        (graph->datawin().xmax -
                        graph->datawin().xmin)*FACTOR);
                }

                graph->rawdata().xmax +=
                    (graph->rawdata().xmax - graph->datawin().xmin)*FACTOR;

                if (!vs_flag && start != stop &&
                        graph->rawdata().xmax > stop) {
                    graph->rawdata().xmax = stop;
                    break;
                }
            } while (val > graph->rawdata().xmax);
        }
    }
    else if (val > graph->rawdata().xmax)
        graph->rawdata().xmax = val;
    else if (val < graph->rawdata().xmin)
        graph->rawdata().xmin = val;

    sDataVec *v;
    for (dl = dl0; dl; dl = dl->dl_next) {
        v = dl->dl_dvec;
        val = v->realval(len - 1);
        if (Sp.GetFlag(FT_GRDB))
            GRpkgIf()->ErrPrintf(ET_MSGS, "y = %G\n\r", val);
        if (val < graph->rawdata().ymin)
            graph->rawdata().ymin = val;
        else if (val > graph->rawdata().ymax)
            graph->rawdata().ymax = val;
        double delta;
        if (graph->format() == FT_GROUP) {
            if (*v->units() == UU_VOLTAGE) {
                delta = IPLTOL*(graph->grpmax(0) - graph->grpmin(0));
                if (val < graph->grpmin(0) - delta ||
                        val > graph->grpmax(0) + delta) {
                    changed = true;
                    break;
                }
            }
            else if (*v->units() == UU_CURRENT) {
                delta = IPLTOL*(graph->grpmax(1) - graph->grpmin(1));
                if (val < graph->grpmin(1) - delta ||
                        val > graph->grpmax(1) + delta) {
                    changed = true;
                    break;
                }
            }
            else {
                delta = IPLTOL*(graph->grpmax(2) - graph->grpmin(2));
                if (val < graph->grpmin(2) - delta ||
                        val > graph->grpmax(2) + delta) {
                    changed = true;
                    break;
                }
            }
        }
        else if (graph->format() == FT_MULTI) {
            delta = IPLTOL*(v->maxsignal() - v->minsignal());
            if (val < v->minsignal() - delta || val > v->maxsignal() + delta) {
                changed = true;
                break;
            }
        }
        else {
            delta = IPLTOL*(graph->datawin().ymax - graph->datawin().ymin);
            if (val < graph->datawin().ymin - delta ||
                    val > graph->datawin().ymax + delta) {
                changed = true;
                break;
            }
        }
    }

    update_dims(run->runPlot(), graph);

    if (changed) {
        // Redraw everything
        graph->clear_selections();
        graph->dev()->Clear();
        graph->clear_saved_text();
        graph->gr_redraw();
    }
    else {
        // blank the "backtrace" in multi-dimensional plot
        bool blank = false;
        if (xs && xs->numdims() > 1 &&
                (len - 1) % xs->dims(xs->numdims() - 1) == 0)
            blank = true;
        if (!blank) {
            graph->gr_draw_last(len);
            graph->dev()->Update();
        }
    }
    graph->set_noevents(false);
}


void
IFoutput::endIplot(sRunDesc *run)
{
    if (!run)
        return;
    sDebug *db = run->circuit() ? &run->circuit()->debugs() : 0;
    if (o_debugs->iplots() || (db && db->iplots())) {
        if (GRpkgIf()->CurDev() &&
                GRpkgIf()->CurDev()->devtype == GRfullScreen)
            // redraw
            GP.PopGraphContext();
        for (sDbComm *d = o_debugs->iplots(); d; d = d->next()) {
            d->set_reuseid(0);
            if (d->type() == DB_IPLOT && !d->bad()) {
                if (d->graphid()) {
                    sGraph *graph = GP.FindGraph(d->graphid());
                    graph->dev()->Clear();
                    graph->clear_selections();
                    update_dims(run->runPlot(), graph);
                    graph->gr_redraw();
                    graph->gr_end();
                }
            }
            if (d->type() == DB_DEADIPLOT) {
                // user killed the window while it was running
                if (d->graphid())
                    GP.DestroyGraph(d->graphid());
                d->set_type(DB_IPLOT);
                d->set_graphid(0);
            }
        }
        if (db) {
            for (sDbComm *d = db->iplots(); d; d = d->next()) {
                d->set_reuseid(0);
                if (d->type() == DB_IPLOT && !d->bad()) {
                    if (d->graphid()) {
                        sGraph *graph = GP.FindGraph(d->graphid());
                        graph->dev()->Clear();
                        graph->clear_selections();
                        update_dims(run->runPlot(), graph);
                        graph->gr_redraw();
                        graph->gr_end();
                    }
                }
                if (d->type() == DB_DEADIPLOT) {
                    // user killed the window while it was running
                    if (d->graphid())
                        GP.DestroyGraph(d->graphid());
                    d->set_type(DB_IPLOT);
                    d->set_graphid(0);
                }
            }
        }
    }
}


// Return true if an iplot is active.
//
bool
IFoutput::isIplot(bool resurrect)
{
    if (resurrect) {
        // Turn dead iplots back on.
        for (sDbComm *d = o_debugs->iplots(); d; d = d->next()) {
            if (d->type() == DB_DEADIPLOT)
                d->set_type(DB_IPLOT);
        }
        if (Sp.CurCircuit()) {
            sDebug *db = &Sp.CurCircuit()->debugs();
            for (sDbComm *d = db->iplots(); d; d = d->next()) {
                if (d->type() == DB_DEADIPLOT)
                    d->set_type(DB_IPLOT);
            }
        }
    }

    for (sDbComm *d = o_debugs->iplots(); d; d = d->next()) {
        if ((d->type() == DB_IPLOT || d->type() == DB_IPLOTALL) && d->active())
            return (true);
    }
    if (Sp.CurCircuit()) {
        sDebug *db = &Sp.CurCircuit()->debugs();
        for (sDbComm *d = db->iplots(); d; d = d->next())
            if ((d->type() == DB_IPLOT || d->type() == DB_IPLOTALL) &&
                    d->active())
                return (true);
    }
    return (false);
}
// End of IFsimulator functions.

