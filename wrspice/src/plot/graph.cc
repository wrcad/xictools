
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
Authors: 1988 Jeffrey M. Hsu
         1992 Stephen R. Whiteley
****************************************************************************/

#include "config.h"
#include "graph.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "simulator.h"
#include "toolbar.h"
#include "spnumber/spnumber.h"
#include "miscutil/texttf.h"
#include "ginterf/grlinedb.h"


//
//  "plot" command graphics.
//

namespace {
    inline int num_colors()
    {
        return (SPMIN(GRpkg::self()->CurDev()->numcolors, NUMPLOTCOLORS));
    }
}

// Initial size of bounding box for zoomin.
#define BOXSIZE 30

// Dimension map icon position: top of window, to the right of the
// file cabinet icon.
#define DIM_ICON_X (GRpkg::self()->CurDev()->xoff + 4*gr_fontwid)
#define DIM_ICON_Y (gr_vport.top() + 2*gr_fonthei + 9)

// The left-side text filed width in y-separated plots.
#define FIELD_MIN   9
#define FIELD_MAX   20
#define FIELD_INIT  12

// Number of digits to use when printing numbers.
#define NUMDGT 5

// Maximum character width of a number.
#define VALBOX_WIDTH 14


bool
sGraph::gr_setup_dev(int type, const char *name)
{
    gr_apptype = type;
    if (GRpkg::self()->CurDev() == GRpkg::self()->MainDev()) {
        // This is a hack:  the devdep for screen graphics inherits
        // both GRdraw and GRwbag interfaces.

        gr_dev = 0;
        if (GRpkg::self()->CurDev() &&
                GRpkg::self()->CurDev()->ident != _devNULL_) {
            gr_dev = dynamic_cast<GRdraw*>(GRpkg::self()->NewWbag(name,
                gr_new_gx(type)));
        }
    }
    else {
        // For hardcopy, we only need the GRdraw interface.
        gr_dev = GRpkg::self()->NewDraw();
    }
    if (!gr_dev)
        return (false);
    gr_dev->SetUserData(this);
    return (true);
}


int
sGraph::gr_dev_init()
{
    if (GRpkg::self()->MainDev() &&
            GRpkg::self()->MainDev() == GRpkg::self()->CurDev() &&
            GRpkg::self()->MainDev()->name)
        return (gr_pkg_init());
    for (int i = 0; i < NUMPLOTCOLORS; i++)
        gr_colors[i] = SpGrPkg::DefColors[i];
    gr_area.set_width(GRpkg::self()->CurDev()->width);
    gr_area.set_height(GRpkg::self()->CurDev()->height);
    gr_area.set_left(GRpkg::self()->CurDev()->xoff);
    gr_area.set_bottom(GRpkg::self()->CurDev()->yoff);
    gr_dev->TextExtent(0, &gr_fontwid, &gr_fonthei);
    return (false);
}


int
sGraph::gr_win_ht(int minht)
{
    // Return a height for a normal plot window based on the legend
    // layout.  The argument is the minimum/default.

    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID)
        return (minht);

    // Size as if ysep was given.
    gr_dev->TextExtent(0, &gr_fontwid, &gr_fonthei);
    int nsy = gr_fonthei/4;
    if (nsy < 2)
        nsy = 2;
    int dely = 3*gr_fonthei + gr_fonthei/2 + nsy + 1;
    int h = gr_numtraces*dely + 7*gr_fonthei;
    return (h > minht ? h : minht);
}


// Clear the graph for reuse (pass to SPgraphics::Init()).
//
void
sGraph::gr_reset()
{
    const sGraph *thisgr = this;
    if (!thisgr)
        return;

    while (gr_keyed) {
        sKeyed *nextk = gr_keyed->next;
        delete gr_keyed;
        gr_keyed = nextk;
    }
    gr_destroy_data();  
    gr_plotdata = 0;
    wordlist::destroy(gr_command);  
    gr_command = 0;  
    delete [] gr_plotname;  
    gr_plotname = 0;
    delete [] gr_title;  
    gr_title = 0;
    delete [] gr_date;  
    gr_date = 0;
    delete [] gr_xlabel;  
    gr_xlabel = 0;
    delete [] gr_ylabel;  
    gr_ylabel = 0;
    clear_selections();
}


sGraph *
sGraph::gr_copy()
{
    sGraph *ret = GP.NewGraph();
    memcpy(ret, this, sizeof(sGraph));
    ret->gr_id = GP.RunningId() - 1;   // restore id

    // copy gr_keyed
    sKeyed *k;
    for (ret->gr_keyed = 0, k = gr_keyed; k; k = k->next) {
        sKeyed *nk = new sKeyed(*k);
        nk->text = lstring::copy(nk->text);
        nk->next = ret->gr_keyed;
        ret->gr_keyed = nk;
    }

    // copy selections
    if (gr_selections) {
        if (gr_selsize > 0) {
            ret->gr_selections = new char[gr_selsize];
            memcpy(ret->gr_selections, gr_selections, gr_selsize);
        }
        else
            ret->gr_selections = 0;
    }

    ret->gr_plotdata = gr_copy_data();

    if (gr_plotname)
        ret->gr_plotname = lstring::copy(gr_plotname);
    if (gr_title)
        ret->gr_title = lstring::copy(gr_title);
    if (gr_date)
        ret->gr_date = lstring::copy(gr_date);
    if (gr_xlabel)
        ret->gr_xlabel = lstring::copy(gr_xlabel);
    if (gr_ylabel)
        ret->gr_ylabel = lstring::copy(gr_ylabel);
    if (gr_command)
        ret->gr_command = wordlist::copy(gr_command);
    return (ret);
}


// Update the keyed list from fromgraph.
//
void
sGraph::gr_update_keyed(sGraph *fromgraph, bool cpy_user)
{
    if (!fromgraph)
        return;
    for (int i = 1; i < LAyval + numtraces(); i++) {
        if (i == LAxunits || i == LAyunits)
            continue;
        sKeyed *k = get_keyed(i);
        sKeyed *kf = fromgraph->get_keyed(i);
        if (k)
            k->update(kf);
        else if (kf) {
            k = new sKeyed(*kf);
            k->text = lstring::copy(k->text);
            k->next = gr_keyed;
            gr_keyed = k;
        }
    }
    if (cpy_user) {
        for (sKeyed *kf = fromgraph->gr_keyed; kf; kf = kf->next) {
            if (kf->type != LAuser)
                continue;
            sKeyed *k = new sKeyed(*kf);
            k->text = lstring::copy(k->text);
            k->next = gr_keyed;
            gr_keyed = k;
        }
    }

    // Assume thet the "fromgraph" is the source for zoomin.  This forces
    // the plot data to fill the entire scale, otherwise data outside the
    // actual clicked-on limits won't be shown, which looks funny.
    //
    gr_scale_flags |= 0x10000000;
}


// If we get a resize/redraw before the plot is complete, quit drawing
// before starting over.
//
void
sGraph::gr_abort()
{
    gr_cpage = 0;
#if defined (HAVE_SETJMP_H) && defined (HAVE_SIGNAL)
    if (gr_in_redraw)
        longjmp(jmpbuf, 1);
#endif
}


// Draw the grid and traces.  Return true if drawing ended
// prematurely.  Floating point exception longjmps are inhibited when
// any drawing is active, as this would skip the PopGraphContext call
// setting the stage for trouble.
//
bool
sGraph::gr_redraw_direct()
{
    // Push graph for possible use by err/interrupt handlers.
    GP.PushGraphContext(this);
    Sp.PushFPEinhibit();

    if (!gr_in_redraw) {
        gr_stop = false;
#if defined (HAVE_SETJMP_H) && defined (HAVE_SIGNAL)
        if (setjmp(jmpbuf) == 1) {
            if (gr_in_redraw)
                gr_in_redraw--;
            Sp.PopFPEinhibit();
            GP.PopGraphContext();
            return (true);
        }
#endif
    }
    bool ret = false;
    gr_in_redraw++;
    switch (gr_apptype) {
    case GR_PLOT:
        ret = dv_redraw();
        break;
    case GR_MPLT:
        ret = mp_redraw();
        break;
    }

    if (gr_in_redraw)
        gr_in_redraw--;
    Sp.PopFPEinhibit();
    GP.PopGraphContext();
    return (ret);
}


// Draw the relocatable text and transient marks.
//
void
sGraph::gr_redraw_keyed()
{
    if (gr_apptype == GR_MPLT) {
        for (sKeyed *k = gr_keyed; k; k = k->next) {
            int x, y;
            gr_dev->SetColor(gr_colors[k->colorindex].pixel);
            gr_get_keyed_posn(k, &x, &y);
            y = yinv(y);
            gr_dev->Text(k->text, x, y, k->xform);
        }
        return;
    }

    sKeyed *ktitle = 0;
    sKeyed *kplotname = 0;
    sKeyed *kdate = 0;

    if (!gr_present) {
        if (gr_grtype != GRID_POLAR && gr_grtype != GRID_SMITH &&
                gr_grtype != GRID_SMITHGRID) {
            for (sKeyed *k = gr_keyed; k; k = k->next) {
                if (k->type == LAname && k->text)
                    kplotname = k;
                else if (k->type == LAtitle && k->text)
                    ktitle = k;
                else if (k->type == LAdate && k->text)
                    kdate = k;
            }
        }
    }

    // Check and see if there is room for the date, if not, it won't
    // be shown.
    bool nodate = false;
    if (kdate) {
        int xd, yd;
        gr_get_keyed_posn(kdate, &xd, &yd);
        if (ktitle) {
            int xt, yt;
            gr_get_keyed_posn(ktitle, &xt, &yt);
            if (abs(yt - yd) <= 2 &&
                    (unsigned)gr_vport.width() < (1 + strlen(kdate->text) +
                    strlen(ktitle->text))*gr_fontwid)
                nodate = true;
        }
        if (!nodate && kplotname) {
            int xp, yp;
            gr_get_keyed_posn(kplotname, &xp, &yp);
            if (abs(yp - yd) <= 2 &&
                    (unsigned)gr_vport.width() < (1 + strlen(kdate->text) +
                    strlen(kplotname->text))*gr_fontwid)
                nodate = true;
        }
    }

    // restore text
    for (sKeyed *k = gr_keyed; k; k = k->next) {
        if (k == kdate && nodate)
            continue;
        int x, y;
        gr_dev->SetColor(gr_colors[k->colorindex].pixel);
        gr_get_keyed_posn(k, &x, &y);
        y = yinv(y);
        gr_dev->Text(k->text, x, y, k->xform);
    }

    // Important for Windows, text update delayed otherwise, screws
    // things up in QT.
#ifdef WIN32
#if defined(WITH_GTK2) || defined(WITH_GTK3)
    gr_dev->Update();
#endif
#endif

    if (gr_reference.mark)
        gr_mark();
}


void
sGraph::gr_init_data()
{
    switch (gr_apptype) {
    case GR_PLOT:
        dv_initdata();
        break;
    case GR_MPLT:
        mp_initdata();
        break;
    }
}


void *
sGraph::gr_copy_data()
{
    switch (gr_apptype) {
    case GR_PLOT:
        return (dv_copy_data());    // sDvList *
    case GR_MPLT:
        return (mp_copy_data());    // sChkPts *
    }
    return (0);
}


void
sGraph::gr_destroy_data()
{
    switch (gr_apptype) {
    case GR_PLOT:
        dv_destroy_data();
        break;
    case GR_MPLT:
        mp_destroy_data();
        break;
    }
    if (GP.SourceGraph() == this)
        GP.SetSourceGraph(0);
}


bool
sGraph::gr_add_trace(sDvList *ndvl, int n)
{
    if (gr_apptype != GR_PLOT)
        return (false);
    if (n < 0)
        return (false);

    sDvList *dl = static_cast<sDvList*>(gr_plotdata);
    n--;
    if (n < 0) {
        ndvl->dl_next = dl;
        gr_plotdata = ndvl;
    }
    else {
        sDvList *dx = 0;
        while (n-- && dl) {
            dx = dl;
            dl = dl->dl_next;
        }
        if (dl) {
            ndvl->dl_next = dl->dl_next;
            dl->dl_next = ndvl;
        }
        else {
            ndvl->dl_next = 0;
            dx->dl_next = ndvl;
        }
    }

    gr_numtraces++;

    // Recompute the new index.
    n = 0;
    sDvList *dv0 = static_cast<sDvList*>(gr_plotdata);
    for (sDvList *d = dv0; d; d = d->dl_next) {
        if (d == ndvl)
            break;
        n++;
    }

    // Fix up the saved trace labels.  The new label will be added later.
    for (sKeyed *k = gr_keyed; k; k = k->next) {
        if (k->type >= LAyval + n)
            k->type = (LAtype)(k->type + 1);
    }

    // Recompute the data limits.
    dv0 = static_cast<sDvList*>(gr_plotdata);
    if (!gr_xlimfixed && gr_grtype != GRID_POLAR &&
            gr_grtype != GRID_SMITH && gr_grtype != GRID_SMITHGRID) {

        sDataVec *v = ndvl->dl_dvec;
        double dd[2];
        v->minmax(dd, true);

        if ((v->flags() & VF_MINGIVEN) && dd[0] < v->minsignal())
            dd[0] = v->minsignal();
        if ((v->flags() & VF_MAXGIVEN) && dd[1] > v->maxsignal())
            dd[1] = v->maxsignal();

        if (dd[0] < gr_rawdata.ymin)
            gr_rawdata.ymin = dd[0];
        if (dd[1] > gr_rawdata.ymax)
            gr_rawdata.ymax = dd[1];
    }

    gr_init_btns();
    return (true);
}


bool
sGraph::gr_delete_trace(int n)
{
    if (gr_apptype != GR_PLOT)
        return (false);
    if (gr_numtraces <= 1)
        return (false);
    if (n < 0 || n >= gr_numtraces)
        return (false);
    sDvList *dv0 = static_cast<sDvList*>(gr_plotdata);
    if (dv0 == 0)
        return (false);

    int nn = n;
    bool hidden = false;
    sDvList *dp = 0, *dn;
    for (sDvList *d = dv0; d; d = dn) {
        dn = d->dl_next;
        if (nn == 0) {
            if (dp)
                dp->dl_next = dn;
            else
                gr_plotdata = dn;
            d->dl_next = gr_hidden_data;
            gr_hidden_data = d;
            hidden = true;
            break;
        }
        dp = d;
        nn--;
    }
    if (!hidden)
        return (false);

    gr_numtraces--;

    // Fix up the saved trace labels.
    sKeyed *kp = 0, *kn;
    for (sKeyed *k = gr_keyed; k; k = kn) {
        kn = k->next;
        if (k->type == LAyval + n) {
            if (kp)
                kp->next = kn;
            else
                gr_keyed = kn;
            delete k;
            continue;

        }
        if (k->type > LAyval + n)
            k->type = (LAtype)(k->type - 1);
        kp = k;
    }

    // Recompute the data limits.
    dv0 = static_cast<sDvList*>(gr_plotdata);
    if (!gr_xlimfixed && gr_grtype != GRID_POLAR &&
            gr_grtype != GRID_SMITH && gr_grtype != GRID_SMITHGRID) {
        for (sDvList *dl = dv0; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            double dd[2];
            v->minmax(dd, true);

            if ((v->flags() & VF_MINGIVEN) && dd[0] < v->minsignal())
                dd[0] = v->minsignal();
            if ((v->flags() & VF_MAXGIVEN) && dd[1] > v->maxsignal())
                dd[1] = v->maxsignal();

            if (dl == dv0) {
                gr_rawdata.ymin = dd[0];
                gr_rawdata.ymax = dd[1];
            }
            else {
                if (dd[0] < gr_rawdata.ymin)
                    gr_rawdata.ymin = dd[0];
                if (dd[1] > gr_rawdata.ymax)
                    gr_rawdata.ymax = dd[1];
            }
        }
    }

    // Reconfigure buttons and plotting format.
    bool grp = false;
    for (sDvList *dl = dv0; dl; dl = dl->dl_next) {
        if (!(*dv0->dl_dvec->units() == *dl->dl_dvec->units())) {
            grp = true;
            break;
        }
    }
    if (!grp && gr_format == FT_GROUP)
        gr_format = FT_MULTI;
    if (gr_numtraces == 1) {
        gr_ysep = false;
        gr_format = FT_SINGLE;
    }
    gr_init_btns();

    return (true);
}


void
sGraph::gr_key_hdlr(const char *text, int code, int tx, int ty)
{
    sKeyed *k = gr_keyed;
#if (defined (WITH_GTK2) || defined (WITH_GTK3))
    if (gr_cmdmode & grMoving) {
        if (code == UP_KEY || code == RIGHT_KEY) {
            gr_show_ghost(false);
            if (k->xform & TXTF_HJR)
                k->xform &= ~(TXTF_HJR | TXTF_HJC);
            else if (k->xform & TXTF_HJC) {
                k->xform &= ~TXTF_HJC;
                k->xform |= TXTF_HJR;
            }
            else
                k->xform |= TXTF_HJC;
            gr_show_ghost(true);
        }
        else if (code == DOWN_KEY || code == LEFT_KEY) {
            gr_show_ghost(false);
            if (k->xform & TXTF_HJR) {
                k->xform &= ~TXTF_HJR;
                k->xform |= TXTF_HJC;
            }
            else if (k->xform & TXTF_HJC)
                k->xform &= ~TXTF_HJC;
            else
                k->xform |= TXTF_HJR;
            gr_show_ghost(true);
        }
        return;
    }
#endif
    if (code == UP_KEY) {
        // Advance color index of current string
        if (!k || !gr_seltext)
            return;
        k->colorindex++;
        if (k->colorindex == NUMPLOTCOLORS)
            k->colorindex = 1;
        gr_dev->SetColor(gr_colors[0].pixel);
        int x, y;
        gr_get_keyed_posn(k, &x, &y);
        y = yinv(y);
        gr_dev->Text(k->text, x, y, k->xform);
        gr_dev->SetColor(gr_colors[k->colorindex].pixel);
        gr_dev->Text(k->text, x, y, k->xform);
    }
    else if (code == DOWN_KEY) {
        // Decrement color index
        if (!k || !gr_seltext)
            return;
        k->colorindex--;
        if (k->colorindex == 0)
            k->colorindex = NUMPLOTCOLORS-1;
        gr_dev->SetColor(gr_colors[0].pixel);
        int x, y;
        gr_get_keyed_posn(k, &x, &y);
        y = yinv(y);
        gr_dev->Text(k->text, x, y, k->xform);
        gr_dev->SetColor(gr_colors[k->colorindex].pixel);
        gr_dev->Text(k->text, x, y, k->xform);
    }
    else if (code == LEFT_KEY) {
        if (k && gr_seltext && !k->terminated) {
            if (k->inspos > 0) {
                gr_show_sel_text(false);
                k->inspos--;
#if defined (WITH_QT5) || defined (WITH_QT6)
                int x, y;
                gr_get_keyed_posn(k, &x, &y);
                y = yinv(y);
                gr_dev->SetColor(gr_colors[k->colorindex].pixel);
                gr_dev->Text(k->text, x, y, k->xform);
#else
                int xl, yb, xr, yt;
                if (gr_get_keyed_bb(k, &xl, &yb, &xr, &yt)) {
                    gr_refresh(xl, yinv(yb)+1, xr, yinv(yt));
                }
#endif
                gr_show_sel_text(true);
            }
#if defined (WITH_QT5) || defined (WITH_QT6)
            else {
                gr_show_sel_text(false);
                if (gr_xform & TXTF_HJR) {
                    gr_xform &= ~TXTF_HJR;
                    gr_xform |= TXTF_HJC;
                }
                else if (gr_xform & TXTF_HJC)
                    gr_xform &= ~TXTF_HJC;
                else
                    gr_xform |= TXTF_HJR;
                gr_show_sel_text(true);
            }
#endif
        }
        else if (k && k->next && gr_seltext) {
            // Rotate to previous string
            gr_show_sel_text(false);
            for (;;) {
                gr_keyed = k->next;
                k->next = 0;
                sKeyed *kk;
                for (kk = gr_keyed; kk->next; kk = kk->next) ;
                kk->next = k;
                if (!gr_keyed->ignore)
                    break;
                k = gr_keyed;
            }
            gr_show_sel_text(true);
        }
    }
    else if (code == RIGHT_KEY) {
        if (k && gr_seltext && !k->terminated) {
            if (k->inspos < (int)strlen(k->text)) {
                gr_show_sel_text(false);
                k->inspos++;
#if defined (WITH_QT5) || defined (WITH_QT6)
                int x, y;
                gr_get_keyed_posn(k, &x, &y);
                y = yinv(y);
                gr_dev->SetColor(gr_colors[k->colorindex].pixel);
                gr_dev->Text(k->text, x, y, k->xform);
#else
                int xl, yb, xr, yt;
                if (gr_get_keyed_bb(k, &xl, &yb, &xr, &yt)) {
                    gr_refresh(xl, yinv(yb)+1, xr, yinv(yt));
                }
#endif
                gr_show_sel_text(true);
            }
#if defined (WITH_QT5) || defined (WITH_QT6)
            else {
                gr_show_sel_text(false);
                if (gr_xform & TXTF_HJR)
                    gr_xform &= ~(TXTF_HJR | TXTF_HJC);
                else if (gr_xform & TXTF_HJC) {
                    gr_xform &= ~TXTF_HJC;
                    gr_xform |= TXTF_HJR;
                }
                else
                    gr_xform |= TXTF_HJC;
                gr_show_sel_text(true);
            }
#endif
        }
        else if (k && k->next && gr_seltext) {
            // Rotate to next string
            gr_show_sel_text(false);
            for (;;) {
                sKeyed *kk;
                for (kk = gr_keyed; kk->next->next; kk = kk->next) ;
                k = kk->next;
                kk->next = 0;
                k->next = gr_keyed;
                gr_keyed = k;
                if (!gr_keyed->ignore)
                    break;
            }
            gr_show_sel_text(true);
        }
    }
    else if (code == ENTER_KEY) {
        // Done with string, set terminated flag
        if (k && gr_seltext && !k->terminated) {
            gr_show_sel_text(false);
            k->terminated = true;
            gr_show_sel_text(true);
        }
    }
    else if (code == BSP_KEY) {
        // Erase last character.  If terminated, undo terminated status.
        // If no more characters, delete string entry.
        if (!k || !gr_seltext)
            return;
        if (k->terminated) {
            gr_show_sel_text(false);
            k->terminated = false;
            gr_show_sel_text(true);
            return;
        }
        if (k->inspos > 0) {
            int len = strlen(k->text);
            if (k->inspos > len)
                k->inspos = len;

            gr_show_sel_text(false);
            char *cptx = lstring::copy(k->text);
            char *s = cptx + k->inspos - 1;
            while (*s) {
                *s = *(s+1);
                s++;
            }
            k->inspos--;

            int xl, yb, xr, yt;
            if (gr_get_keyed_bb(k, &xl, &yb, &xr, &yt)) {
                delete [] k->text;
                k->text = cptx;
                gr_refresh(xl, yinv(yb)+1, xr, yinv(yt));
            }
            if (!cptx[0]) {
                k->ignore = true;
                gr_seltext = false;
            }
            else {
                // write it
                gr_dev->SetColor(gr_colors[k->colorindex].pixel);
                int x, y;
                gr_get_keyed_posn(k, &x, &y);
                y = yinv(y);
                gr_dev->Text(k->text, x, y, k->xform);
                gr_seltext = true;
                gr_show_sel_text(true);
            }
        }
    }
    else if (code == DELETE_KEY) {
        if (!k || !gr_seltext)
            return;
        int len = strlen(k->text);
        if (k->inspos > len)
            k->inspos = len;
        if (k->terminated) {
            gr_show_sel_text(false);
            gr_seltext = false;
            int xl, yb, xr, yt;
            if (gr_get_keyed_bb(k, &xl, &yb, &xr, &yt)) {
                k->text[0] = 0;
                gr_refresh(xl, yinv(yb)+1, xr, yinv(yt));
            }
            k->inspos = 0;
            k->ignore = true;
        }
        else if (k->inspos != len) {
            gr_show_sel_text(false);
            char *cptx = lstring::copy(k->text);
            char *s = cptx + k->inspos;
            while (*s) {
                *s = *(s+1);
                s++;
            }
            int xl, yb, xr, yt;
            if (gr_get_keyed_bb(k, &xl, &yb, &xr, &yt)) {
                delete [] k->text;
                k->text = cptx;
                gr_refresh(xl, yinv(yb)+1, xr, yinv(yt));
            }
            if (!cptx[0]) {
                k->ignore = true;
                gr_seltext = false;
            }
            else {
                // write it
                gr_dev->SetColor(gr_colors[k->colorindex].pixel);
                int x, y;
                gr_get_keyed_posn(k, &x, &y);
                y = yinv(y);
                gr_dev->Text(k->text, x, y, k->xform);
                gr_seltext = true;
                gr_show_sel_text(true);
            }
        }
    }
    else {
        // Add character to current string.  If terminated, start a new
        // string.
        //
        if (text && *text >= ' ' && *text <= '~') {
            gr_show_sel_text(false);
            if (!k || k->terminated || !gr_seltext) {
                k = new sKeyed;
                k->next = gr_keyed;
                if (gr_keyed && gr_keyed->type == LAuser)
                    k->colorindex = gr_keyed->colorindex;
                else
                    k->colorindex = 1;
                gr_keyed = k;
                k->text = lstring::copy(text);
                k->inspos = strlen(k->text);
                gr_set_keyed_posn(k, tx, ty);
            }
            else {
                int xl, yb, xr, yt;
                if (gr_get_keyed_bb(k, &xl, &yb, &xr, &yt)) {
                    int txl = strlen(text);
                    char *tmp = new char[strlen(k->text) + txl + 1];
                    int i;
                    for (i = 0; i < k->inspos; i++) {
                        tmp[i] = k->text[i];
                        if (!tmp[i]) {
                            k->inspos = i;
                            break;
                        }
                    }
                    for (i = k->inspos; i < k->inspos + txl; i++) {
                        tmp[i] = text[i - k->inspos];
                    }
                    strcpy(tmp + i, k->text + k->inspos);
                    k->inspos += strlen(text);

                    delete [] k->text;
                    k->text = tmp;
                    gr_refresh(xl, yinv(yb)+1, xr, yinv(yt));
                }
            }
            // write it
            gr_dev->SetColor(gr_colors[k->colorindex].pixel);
            int x, y;
            gr_get_keyed_posn(k, &x, &y);
            y = yinv(y);
            gr_dev->Text(k->text, x, y, k->xform);
            gr_seltext = true;
            gr_show_sel_text(true);
        }
    }
}


// Called for button press handling.
//
void
sGraph::gr_bdown_hdlr(int button, int x, int y)
{
    if (button == 1) {
        if (gr_cmdmode & grShiftMode)
            button = 2;
        else if (gr_cmdmode & grControlMode)
            button = 3;
    }
    if (!gr_present) {
        if (gr_format != FT_SINGLE || gr_ysep) {
            // Check the text field width icons.
            int xl = gr_vport.left() - 3*gr_fontwid - gr_fontwid/2;
            int xr = xl + gr_fontwid + gr_fontwid/4;
            int yb = gr_vport.top();
            int yt = yb + gr_fonthei;
            if (x >= xl && x <= xr && y >= yb && y <= yt) {
                if (gr_field > FIELD_MIN) {
                    gr_field--;
                    gr_redraw();
                }
                return;
            }
            xl = xr;
            xr = xl + gr_fontwid + gr_fontwid/4;
            if (x > xl && x <= xr && y >= yb && y <= yt) {
                if (gr_field < FIELD_MAX) {
                    gr_field++;
                    gr_redraw();
                }
                return;
            }
        }
    }

    bool was_seltext = gr_seltext;
    if (button == 1) {
        gr_seltext = false;
        gr_show_sel_text(false);
    }

    if (!gr_present) {
        if (gr_apptype == GR_PLOT && !(gr_grtype == GRID_POLAR ||
                gr_grtype == GRID_SMITH || gr_grtype == GRID_SMITHGRID)) {
            if (dv_scale_icon_hdlr(button, x, y))
                return;
        }
        if (x > DIM_ICON_X - gr_fontwid/2 &&
                x < DIM_ICON_X + gr_fontwid + gr_fontwid/2 &&
                y > DIM_ICON_Y && y < DIM_ICON_Y + gr_fonthei) {
            // Clicked on the dimension map icon.
            if (gr_selections) {
                gr_sel_show = !gr_sel_show;
                gr_dev->Clear();
                gr_redraw();
                return;
            }
        }
    }

    if (button == 1) {
        gr_cmd_data = 0;
        GP.SetSourceGraph(0);
        if (gr_reference.mark)
            gr_setref(x, y);
        else {
            sKeyed *k;
            for (k = gr_keyed; k; k = k->next) {
                if (k->ignore)
                    continue;
                int xl, yb, xr, yt;
                if (!gr_get_keyed_bb(k, &xl, &yb, &xr, &yt))
                    continue;
                if (x > xl && x < xr && y > yb && y < yt) {
                    // button press on keyed text

                    // select for editing
                    if (gr_keyed != k) {
                        sKeyed *kk;
                        for (kk = gr_keyed; kk->next != k; kk = kk->next) ;
                        kk->next = 0;
                        kk = gr_keyed;
                        gr_keyed = k;
                        while (k->next)
                            k = k->next;
                        k->next = kk;
                        k = gr_keyed;

                        gr_seltext = true;
                        gr_show_sel_text(true);
                        gr_xform = k->xform;
                        return;
                    }
                    if (!was_seltext) {
                        gr_seltext = true;
                        gr_show_sel_text(true);
                        gr_xform = k->xform;
                        return;
                    }

                    gr_pressy = yb;
                    if (k->xform & TXTF_HJR)
                        gr_pressx = xr;
                    else if (k->xform & TXTF_HJC)
                        gr_pressx = (xl + xr)/2;
                    else
                        gr_pressx = xl;
                    GP.SetSourceGraph(this);

                    gr_timer_id = GRpkg::self()->AddTimer(200, start_drag_text,
                        this);
                    return;
                }
            }
            if (!(gr_cmdmode & grMoving)) {
                if (gr_apptype == GR_MPLT) {
                    mp_bdown_hdlr(button, x, y);
                    return;
                }
                if (!gr_present) {
                    if (dv_dims_map_hdlr(button, x, y, false))
                        return;

                    // no text selected, select a trace ?
                    int i = gr_select_trace(x, y);
                    if (i >= 0) {
                        sDvList *dl = static_cast<sDvList*>(gr_plotdata);
                        while (i-- && dl)
                            dl = dl->dl_next;
                        gr_cmd_data = dl;
                        gr_timer_id = GRpkg::self()->AddTimer(200,
                            start_drag_trace, this);
                        return;
                    }
                    if (x < 50 && yinv(y) < 20 && gr_hidden_data) {
                        // Select the top of the hidden queue.
                        gr_cmd_data = gr_hidden_data;
                        gr_timer_id = GRpkg::self()->AddTimer(200,
                            start_drag_trace, this);
                        return;
                    }
                }
            }
        }
    }
    else if (button == 2) {
        if (gr_apptype == GR_MPLT) {
            mp_bdown_hdlr(button, x, y);
            return;
        }
        if (gr_present)
            return;

        if (dv_dims_map_hdlr(button, x, y, false))
            return;
        if (gr_reference.mark && gr_reference.set) {
            // remove the reference
            if (gr_reference.mark)
                gr_show_ghost(false);
            gr_refmark(true);
            if (gr_reference.mark)
                gr_show_ghost(true);
            gr_reference.set = false;
        }
    }
    else if (button == 3) {
        if (gr_apptype == GR_MPLT) {
            mp_bdown_hdlr(button, x, y);
            return;
        }
        if (gr_present)
            return;

        if (dv_dims_map_hdlr(button, x, y, false))
            return;
        if (!(gr_cmdmode & grMoving)) {
            gr_pressx = x;
            gr_pressy = y;
            gr_cmdmode |= grZoomIn;
            // Use a timeout to avoid zooming into a click, which is likely
            // inadvertant.
            gr_timer_id = GRpkg::self()->AddTimer(300, start_drag_zoom, this);
        }
    }
}


// Called for button release handling.  The new_keyed argument is used
// to push in a new keyed text element from dropping text/plain in the
// window from anywhere.  The QT code handles this, GTK does not
// presently.
//
void
sGraph::gr_bup_hdlr(int button, int x, int y, const char *new_keyed)
{
    if (button == 1) {
        sGraph *graph = GP.SourceGraph();
        if (!graph)
            graph = this;
        if (graph->gr_timer_id)
            GRpkg::self()->RemoveTimer(graph->gr_timer_id);
        graph->gr_timer_id = 0;
        if (graph->gr_cmdmode & grMoving) {
            bool doit = false;
            gr_set_ghost(0, 0, 0);
            if (graph == this) {
                int d = abs(x - gr_pressx);
                if (abs(y - gr_pressy) > d)
                    d = abs(y - gr_pressy);
                if (d)
                    doit = true;
            }
            if (graph != this || doit) {
                int dx = gr_fontwid/2;
                int dy = gr_fonthei/2;
                bool ok = true;
                if (x < effleft() + dx || x > gr_area.right() - dx ||
                        y < gr_area.bottom() + 2 || y > gr_area.top() - dy)
                    ok = false;
                if (graph != this)
                    graph->gr_set_ghost(0, 0, 0);
                sKeyed *k = graph->gr_keyed;
                graph->gr_cmdmode &= ~grMoving;
                if (ok) {
                    // should be in window
                    gr_show_sel_text(false);
                    if (graph != this || new_keyed ||
                            (gr_cmdmode & (grShiftMode | grControlMode))) {
                        // Copy the text, note that drag between
                        // windows is always a copy.
                        sKeyed *kk;
                        if (new_keyed) {
                            // Not from a plot window so create a new
                            // sKeyed struct.
                            kk = new sKeyed();
                            kk->text = lstring::copy(new_keyed);
                            kk->colorindex = 1;
                        }
                        else {
                            // Copy existing sKeyed struct.
                            kk = new sKeyed(*k);
                            kk->text = lstring::copy(kk->text);
                        }
#if (defined (WITH_QT5) || defined (WITH_QT6))
                        kk->xform &= ~(TXTF_HJC | TXTF_HJR);
                        kk->xform |= (graph->gr_xform & TXTF_HJC);
                        kk->xform |= (graph->gr_xform & TXTF_HJR);
#endif
                        kk->type = LAuser;
                        kk->fixed = false;
                        gr_set_keyed_posn(kk, x, y);
                        kk->fixed = true;
                        kk->next = gr_keyed;
                        gr_keyed = kk;
                        k = gr_keyed;
                        gr_dev->SetColor(gr_colors[k->colorindex].pixel);
                        y = yinv(y);
                        gr_dev->Text(k->text, x, y, k->xform);
                    }
                    else {
                        // Move the text, graph == this.
                        k->fixed = false;
                        int xx, yy;
                        gr_get_keyed_posn(k, &xx, &yy);
                        gr_set_keyed_posn(k, x, y);
                        k->fixed = true;
                        int w, h;
                        gr_dev->TextExtent(k->text, &w, &h);
                        yy = yinv(yy);

#if (defined (WITH_QT5) || defined (WITH_QT6))
                        k->xform &= ~(TXTF_HJC | TXTF_HJR);
                        k->xform |= (gr_xform & TXTF_HJC);
                        k->xform |= (gr_xform & TXTF_HJR);
#endif
                        // The justification may have changed!
#if defined (WITH_QT5) || defined (WITH_QT6)
                        gr_dev->SetOverlayMode(true);
#endif
                        gr_refresh(xx-w, yy+1, xx+2*w, yy-h, true);
#if defined (WITH_QT5) || defined (WITH_QT6)
                        gr_dev->SetOverlayMode(false);
#endif

                        y = yinv(y);
                        if (k->xform & TXTF_HJR)
                            x -= w;
                        else if (k->xform & TXTF_HJC)
                            x -= w/2;
                        gr_refresh(x, y+1, x+w, y-h);
                    }
                    gr_seltext = true;
                    gr_show_sel_text(true);
                }
            }
            else
                gr_cmdmode &= ~grMoving;
            return;
        }

        if (gr_present)
            return;

        int btn = button;
        if (gr_cmdmode & grShiftMode)
            btn = 2;
        else if (gr_cmdmode & grControlMode)
            btn = 3;
        if (dv_dims_map_hdlr(btn, x, y, true))
            return;

        if (graph->gr_cmd_data) {
            gr_set_ghost(0, 0, 0);
            if (this == graph) {
                if (x < 50 && yinv(y) < 20) {
                    // Dropped on the file cabinet.
                    int n = 0;
                    sDvList *dv0 = static_cast<sDvList*>(gr_plotdata);
                    for (sDvList *d = dv0; d; d = d->dl_next) {
                        if (d == gr_cmd_data) {
                            if (gr_delete_trace(n)) {
                                gr_cmd_data = 0;
                                GP.SetSourceGraph(0);
                                gr_set_ghost(0, 0, 0);
                                dev()->Clear();
                                gr_redraw();
                            }
                            return;
                        }
                        n++;
                    }
                    // cabinet to cabinet?
                    gr_cmd_data = 0;
                    GP.SetSourceGraph(0);
                    return;
                }
                if (gr_numtraces > 1) {
                    int i = gr_select_trace(x, y);
                    if (i >= 0) {
                        if (gr_cmd_data == gr_hidden_data) {
                            // Recall hidden trace from this plot.
                            graph->gr_set_ghost(0, 0, 0);

                            // Clear the Y-Units text string, if set.
                            sKeyed *kp = 0;
                            for (sKeyed *k = gr_keyed; k; k = k->next) {
                                if (k->type == LAyunits) {
                                    if (kp)
                                        kp->next = k->next;
                                    else
                                        gr_keyed = k->next;
                                    delete k;
                                    break;
                                }
                                kp = k;
                            }

                            sDvList *nv = gr_hidden_data;
                            gr_hidden_data = nv->dl_next;
                            if (gr_add_trace(nv, i)) {
                                gr_dev->Clear();
                                gr_redraw();
                            }
                            return;
                        }
                        sDvList *dl = static_cast<sDvList*>(gr_plotdata);
                        while (i-- && dl)
                            dl = dl->dl_next;
                        if (gr_cmd_data != dl) {
                            struct zz { sDataVec *v; char *t; };
                            zz *n = new zz[gr_numtraces];
                            sDvList *dx = static_cast<sDvList*>(gr_plotdata);
                            i = 0;
                            while (dx) {
                                n[i].t = get_txt(i);
                                n[i++].v = dx->dl_dvec;
                                dx = dx->dl_next;
                            }
                            int j;
                            sDataVec *vf = gr_cmd_data->dl_dvec;
                            char *t = 0;
                            for (i = 0, j = 0; i < gr_numtraces; i++) {
                                if (n[i].v != vf) {
                                    if (i != j)
                                        n[j] = n[i];
                                    j++;
                                }
                                else
                                    t = get_txt(i);
                            }
                            if (!dl) {
                                n[gr_numtraces - 1].v = vf;
                                n[gr_numtraces - 1].t = t;
                            }
                            else {
                                n[gr_numtraces - 1].v = 0;
                                n[gr_numtraces - 1].t = 0;
                                sDataVec *vt = dl->dl_dvec;
                                for (i = 0; ; i++)
                                    if (n[i].v == vt)
                                        break;
                                for (j = gr_numtraces - 1; j > i; j--)
                                    n[j] = n[j-1];
                                n[i].v = vf;
                                n[i].t = t;
                            }
                                
                            dx = static_cast<sDvList*>(gr_plotdata);
                            i = 0;
                            while (dx) {
                                set_txt(i, n[i].t);
                                dx->dl_dvec = n[i++].v;
                                dx = dx->dl_next;
                            }
                            delete [] n;

                            gr_dev->Clear();
                            gr_redraw();
                        }
                    }
                }
            }
            else {
                // moving between graphs
                graph->gr_set_ghost(0, 0, 0);
                if (x < 50 && yinv(y) < 20) {
                    // Dropped on the file cabinet.
                    sDvList *dl = static_cast<sDvList*>(gr_plotdata);
                    sDataVec *nv = graph->gr_cmd_data->dl_dvec->v_interpolate(
                        dl->dl_dvec->scale());
                    if (nv) {
                        const char *t = graph->gr_cmd_data->dl_dvec->name();
                        int sid = graph->gr_id;

                        char buf[BSIZE_SP];
                        snprintf(buf, sizeof(buf), "%d:%s", sid, t);
                        nv->set_name(buf);

                        sDvList *ndvl = new sDvList;
                        ndvl->dl_dvec = nv;
                        ndvl->dl_next = gr_hidden_data;
                        gr_hidden_data = ndvl;
                        gr_dev->Clear();
                        gr_redraw();

                    }
                    gr_cmd_data = 0;
                    GP.SetSourceGraph(0);
                    return;
                }

                int i = gr_select_trace(x, y);
                if (i >= 0) {

                    // Clear the Y-Units text string, if set.
                    sKeyed *kp = 0;
                    for (sKeyed *k = gr_keyed; k; k = k->next) {
                        if (k->type == LAyunits) {
                            if (kp)
                                kp->next = k->next;
                            else
                                gr_keyed = k->next;
                            delete k;
                            break;
                        }
                        kp = k;
                    }

                    int si = 0;  // source trace index
                    sDvList *dl = static_cast<sDvList*>(graph->gr_plotdata);
                    for ( ; dl; si++, dl = dl->dl_next) {
                        if (dl == graph->gr_cmd_data)
                            break;
                    }
                    if (!dl)
                        si = -1;  // Source is hidden.

                    dl = static_cast<sDvList*>(gr_plotdata);
                    sDataVec *nv = graph->gr_cmd_data->dl_dvec->v_interpolate(
                        dl->dl_dvec->scale());
                    if (nv) {
                        const char *t = si >= 0 ? graph->get_txt(si) :
                            graph->gr_cmd_data->dl_dvec->name();
                        // the v_plot field is 0 for these dvec's
                        int sid = graph->gr_id;

                        char buf[BSIZE_SP];
                        snprintf(buf, sizeof(buf), "%d:%s", sid, t);
                        nv->set_name(buf);

                        sDvList *ndvl = new sDvList;
                        ndvl->dl_dvec = nv;
                        if (gr_add_trace(ndvl, i)) {
                            gr_dev->Clear();
                            gr_redraw();
                        }
                        else
                            delete ndvl;
                    }
                }
            }
            gr_cmd_data = 0;
            GP.SetSourceGraph(0);
        }
    }
    if (gr_cmdmode & grZoomIn) {
        if (gr_timer_id) {
            // Don't zoomin if the user clicks, user has to hold the
            // button down for a moment.
            GRpkg::self()->RemoveTimer(gr_timer_id);
            gr_timer_id = 0;
        }
        else {
            gr_set_ghost(0, 0, 0);
            int xlbnd = gr_vport.left() - 2;
            int xubnd = gr_vport.right() + 2;
            int ylbnd = gr_vport.bottom() - 2;
            int yubnd = gr_vport.top() + 2;
            if (x >= xlbnd && x <= xubnd && y >= ylbnd && y <= yubnd) {
                int lowerx, lowery, upperx, uppery;
                if (gr_format != FT_SINGLE || gr_ysep) {
                    uppery = yubnd;
                    lowery = ylbnd;
                }
                else {
                    uppery = gr_pressy > y ? gr_pressy : y;
                    lowery = gr_pressy < y ? gr_pressy : y;
                }
                upperx = gr_pressx > x ? gr_pressx : x;
                lowerx = gr_pressx < x ? gr_pressx : x;
                double fx0, fx1, fy0, fy1;
                gr_screen_to_data(lowerx, lowery, &fx0, &fy0);
                gr_screen_to_data(upperx, uppery, &fx1, &fy1);
                if (gr_format != FT_SINGLE || gr_ysep) {
                    fy0 = gr_rawdata.ymin;
                    fy1 = gr_rawdata.ymax;
                }
                gr_zoom(fx0, fy0, fx1, fy1);
            }
        }
        gr_cmdmode &= ~grZoomIn;
    }
    dv_dims_map_hdlr(button, x, y, true);
}


// Return the trace index (starting at 0) of the trace whose legend is
// under x,y or -1 if none.  Returns numtraces if pointing below all
// traces.
//
int
sGraph::gr_select_trace(int x, int y)
{
    if (x > gr_vport.left())
        return (-1);
    if (gr_format != FT_SINGLE || gr_ysep) {
        int scry = gr_vport.top() - 3*gr_fonthei;
        int dely = 3*gr_fonthei + gr_fonthei/2;
        int spa;
        if (gr_ysep) {
            spa = gr_vport.height()/gr_numtraces;
            if (spa < dely)
                spa = dely;
        }
        else
            spa = dely;
        dely -= gr_fonthei/4;
        for (int i = 0; i < gr_numtraces; i++) {
            if (y > scry && y < scry + 3*gr_fonthei)
                return (i);
            scry -= spa;
        }
        if (y < scry + spa)
            return (gr_numtraces);
    }
    else {
        int spa = 2*gr_fonthei + gr_fonthei/4;
        int scry = gr_vport.top() - 2*gr_fonthei;
        if (gr_pltype == PLOT_POINT) {
            for (int i = 0; i < gr_numtraces; i++) {
                if (y > scry && y < scry + gr_fonthei)
                    return (i);
                scry -= spa;
            }
        }
        else {
            scry += gr_fonthei + gr_fonthei/4;
            for (int i = 0; i < gr_numtraces; i++) {
                if (y > scry - gr_fonthei && y < scry + gr_fonthei/2)
                    return (i);
                scry -= spa;
            }
        }
        if (y < scry + spa)
            return (gr_numtraces);
    }
    return (-1);
}


void
sGraph::gr_replot()
{
    GP.Plot(0, this, 0, 0, GR_PLOT);
}

namespace {
    void ghost_zoom(int x, int y, int ref_x, int ref_y, bool)
    {
        GP.Cur()->gr_ghost_zoom(x, y, ref_x, ref_y);
    }
}


// Initiate a zoomin at x0, y0.  Called from event handler.
//
void
sGraph::gr_zoomin(int x0, int y0)
{
    // open box and get area to zoom in on
    int xlbnd = gr_vport.left() - 2;
    int xubnd = gr_vport.right() + 2;
    if (x0 < xlbnd || x0 > xubnd)
        return;
    int ylbnd = gr_vport.bottom() - 2;
    int yubnd = gr_vport.top() + 2;
    int x1 = x0 + BOXSIZE;
    if (x1 > xubnd)
        x1 = x0 - BOXSIZE;
    int y1;
    if (gr_format != FT_SINGLE || !gr_ysep)
        y1 = y0;
    else {
        if (y0 < ylbnd || y0 > yubnd)
            return;
        y1 = y0 - BOXSIZE;
        if (y1 < ylbnd)
            y1 = y0 + BOXSIZE;
    }

    // redundant
    gr_cmdmode |= grZoomIn;
    gr_pressx = x0;
    gr_pressy = y0;

    gr_set_ghost(ghost_zoom, x0, y0);
    GP.PushGraphContext(this);
    gr_dev->DrawGhost();
    GP.PopGraphContext();
}


// Bring up a new plot of the region specified.
//
void
sGraph::gr_zoom(double fx0, double fy0, double fx1, double fy1)
{
    if (fx0 < gr_rawdata.xmin)
        fx0 = gr_rawdata.xmin;
    if (fx0 > gr_rawdata.xmax)
        fx0 = gr_rawdata.xmax;
    if (fx1 < gr_rawdata.xmin)
        fx1 = gr_rawdata.xmin;
    if (fx1 > gr_rawdata.xmax)
        fx1 = gr_rawdata.xmax;

    if (fy0 < gr_rawdata.ymin)
        fy0 = gr_rawdata.ymin;
    if (fy0 > gr_rawdata.ymax)
        fy0 = gr_rawdata.ymax;
    if (fy1 < gr_rawdata.ymin)
        fy1 = gr_rawdata.ymin;
    if (fy1 > gr_rawdata.ymax)
        fy1 = gr_rawdata.ymax;

    // Expand slightly to ensure that we get endpoints.
    fx0 *= 1.000000001;
    fx1 *= 1.000000001;
    fy0 *= 1.000000001;
    fy1 *= 1.000000001;

    ToolBar()->SuppressUpdate(true);
    GP.SetTmpGraph(this);
    dv_pl_environ(fx0, fx1, fy0, fy1, 0);
    gr_replot();
    dv_pl_environ(0.0, 0.0, 0.0, 0.0, 1);
    GP.SetTmpGraph(0);
    ToolBar()->SuppressUpdate(false);
}


// End a plot.
//
void
sGraph::gr_end()
{
    const char *text = "Enter p for hardcopy, return to continue";
    const char *text1 = "Hit p for hardcopy, any other key to continue";
    gr_dev->Update();
    if (GRpkg::self()->CurDev()->devtype == GRfullScreen) {
        char c;
        VTvalue vv;
        if (Sp.GetVar(kw_device, VTYP_STRING, &vv) &&
                lstring::prefix("/dev/tty", vv.get_string())) {
            TTY.printf("%s", text);
            TTY.flush();
            c = getchar();
        }
        else {
            int x = (gr_area.right() - strlen(text1)*gr_fontwid)/2;
            if (x < 0)
                x = 0;
            int y = gr_area.top() - gr_fonthei - 1;
            gr_dev->SetColor(gr_colors[8].pixel);
            gr_dev->Text(text1, x, yinv(y), 0);
            c = GP.Getchar(TTY.infileno(), -1, 0);
            gr_dev->SetColor(gr_colors[0].pixel);
            gr_dev->Box(x, yinv(y), x + strlen(text1)*gr_fontwid, 
                yinv(y + gr_fonthei));
            gr_dev->Update();
        }
        if (c == 'p')
            gr_hardcopy();
    }
}


namespace {
    void ghost_mark(int x, int y, int, int, bool erase)
    {
        GP.Cur()->gr_ghost_mark(x, y, erase);
    }
}


// Handle entry/exit of the marker.  On exit, call with gr_reference.set
// true, gr_reference.mark false.
//
void
sGraph::gr_mark()
{
    if (!gr_reference.mark) {
        gr_set_ghost(0, 0, 0);
        gr_refmark(true);
        // redraw scale factors
        dv_erase_factors();
        dv_trace(true);
    }
    else {
        gr_refmark(false);
        if (GRpkg::self()->CurDev()->devtype == GRhardcopy)
            return;
        dv_erase_factors();

        gr_set_ghost(ghost_mark, 0, 0);
        if (GRpkg::self()->CurDev()->devtype == GRfullScreen) {
            GP.ReturnEvent(0, 0, 0, 0);
            gr_set_ghost(0, 0, 0);
            gr_reference.mark = false;
            gr_dev->Clear();
            gr_redraw();
        }
    }
}


// Draw/undraw the reference marker.
//
void
sGraph::gr_refmark(bool erase)
{
    if (gr_reference.set) {
#if defined (WITH_QT5) || defined (WITH_QT6)
        if (erase) {
            gr_dev->SetOverlayMode(true);
            if (gr_xmono) {
                gr_dev->Update(gr_reference.x, yinv(gr_vport.top()), 
                    1, gr_vport.top() - gr_vport.bottom() + 1);
            }
            else {
                gr_dev->Update(gr_reference.x-8, yinv(gr_reference.y), 17, 1); 
                gr_dev->Update(gr_reference.x, yinv(gr_reference.y+8), 1, 17); 
            }
            gr_dev->SetOverlayMode(false);
        }
        else {
            dv_erase_factors();
            gr_dev->SetOverlayMode(true);
            gr_dev->SetGhostColor(gr_colors[1].pixel);
            if (gr_xmono) {
                gr_dev->Line(gr_reference.x, yinv(gr_vport.bottom()), 
                    gr_reference.x, yinv(gr_vport.top()));
            }
            else {
                gr_dev->Line(gr_reference.x-8, yinv(gr_reference.y), 
                    gr_reference.x+8, yinv(gr_reference.y));
                gr_dev->Line(gr_reference.x, yinv(gr_reference.y-8), 
                    gr_reference.x, yinv(gr_reference.y+8));
            }
            gr_dev->SetOverlayMode(false);
        }
#else
        (void)erase;
        dv_erase_factors();
        gr_dev->SetXOR(GRxXor);
        gr_dev->SetGhostColor(gr_colors[1].pixel);
        if (gr_xmono) {
            gr_dev->Line(gr_reference.x, yinv(gr_vport.bottom()), 
                gr_reference.x, yinv(gr_vport.top()));
        }
        else {
            gr_dev->Line(gr_reference.x-8, yinv(gr_reference.y), 
                gr_reference.x+8, yinv(gr_reference.y));
            gr_dev->Line(gr_reference.x, yinv(gr_reference.y-8), 
                gr_reference.x, yinv(gr_reference.y+8));
        }
        gr_dev->SetXOR(GRxNone);
#endif
    }
}


// Set a vertical reference line, and print out coordinates
// relative to this reference (marker must be active).
//
void
sGraph::gr_setref(int x0, int y0)
{
    if (!gr_reference.mark)
        return;
    if (x0 < gr_vport.left() || x0 > gr_vport.right())
        return;
    if (y0 < gr_vport.bottom() || y0 > gr_vport.top())
        return;
    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID) {
        int dx = x0 - gr_xaxis.circular.center;
        int dy = yinv(y0) - gr_yaxis.circular.center;
        if (dx*dx + dy*dy > gr_xaxis.circular.radius*gr_xaxis.circular.radius)
            return;
    }
    if (gr_xmono) {
        sDataVec *scale = ((sDvList*)gr_plotdata)->dl_dvec->scale();
        double dd[2];
        scale->minmax(dd, true);
        int nmin, nmax, dummy;
        gr_data_to_screen(dd[0], 1.0, &nmin, &dummy);
        gr_data_to_screen(dd[1], 1.0, &nmax, &dummy);
        if (x0 < nmin || x0 > nmax)
            return;
    }

    gr_set_ghost(0, 0, 0);

    if (gr_reference.set)
        gr_refmark(true);
    dv_erase_factors();
    gr_reference.set = true;
    gr_reference.x = x0;
    gr_reference.y = y0;
    gr_refmark(false);

    gr_set_ghost(ghost_mark, 0, 0);
}


void
sGraph::gr_data_to_screen(double x, double y, int *screenx, int *screeny)
{
    if (gr_grtype == GRID_XLOG || gr_grtype == GRID_LOGLOG) {
        double low = mylog10(gr_datawin.xmin);
        double high = mylog10(gr_datawin.xmax);
        double xx = ((mylog10(x) - low)/(high - low))*gr_vport.width() +
            gr_vport.left();
        *screenx = rnd(xx);
    }
    else {
        double xx = (x - gr_datawin.xmin)/gr_aspect_x + gr_vport.left();
        // look out for integer overflow
        if (xx > 1e6)
            xx = 1e6;
        else if (xx < -1e6)
            xx = -1e6;
        *screenx = rnd(xx);
    }
    if (gr_grtype == GRID_YLOG || gr_grtype == GRID_LOGLOG) {
        double low = mylog10(gr_datawin.ymin);
        double high = mylog10(gr_datawin.ymax);
        double yy = ((mylog10(y) - low)/(high - low))*gr_vport.height() +
            gr_vport.bottom();
        *screeny = rnd(yy);
    }
    else {
        double yy = (y - gr_datawin.ymin)/gr_aspect_y + gr_vport.bottom();
        // look out for integer overflow
        if (yy > 1e6)
            yy = 1e6;
        else if (yy < -1e6)
            yy = -1e6;
        *screeny = rnd(yy);
    }
}


void
sGraph::gr_screen_to_data(int screenx, int screeny, double *x, double *y)
{
    if (gr_grtype == GRID_XLOG || gr_grtype == GRID_LOGLOG) {
        double high = mylog10(gr_datawin.xmax);
        double low = mylog10(gr_datawin.xmin);
        *x = (screenx - gr_vport.left())*(high - low)/gr_vport.width() +
            low;
        *x = pow(10.0, *x);
    }
    else
        *x = (screenx - gr_vport.left())*gr_aspect_x + gr_datawin.xmin;
    if (gr_grtype == GRID_YLOG || gr_grtype == GRID_LOGLOG) {
        double high = mylog10(gr_datawin.ymax);
        double low = mylog10(gr_datawin.ymin);
        *y = (screeny - gr_vport.bottom())*(high - low)/gr_vport.height() +
            low;
        *y = pow(10.0, *y);
    }
    else
        *y = (screeny - gr_vport.bottom())*gr_aspect_y + gr_datawin.ymin;
}   


// Function to save a text string in the plot.  Used to save
// internally generated titles, etc., so that the user can edit them
// later.
//
void
sGraph::gr_save_text(const char *text, int x, int y, int type, int colorindex,
    int rcode)
{
    sKeyed *k = 0;
    if (type != LAuser) {
        // internally generated
        for (k = gr_keyed; k; k = k->next) {
            if (k->type == type)
                break;
        }
    }
    if (k) {
        // already there, just update position
        gr_set_keyed_posn(k, x, y);
        return;
    }
    k = new sKeyed;
    k->next = gr_keyed;
    gr_keyed = k;
    k->text = lstring::copy(text);
    k->terminated = true;
    gr_set_keyed_posn(k, x, y);
    k->type = (LAtype)type;
    k->colorindex = colorindex;
    k->xform = rcode;
}


void
sGraph::gr_set_keyed_posn(sKeyed *k, int x, int y)
{
    if (!k || k->fixed)
        return;
    if (x > gr_vport.left() && x < gr_vport.right()) {
        if (y > gr_vport.bottom() && y < gr_vport.top())
            k->region = KeyedPlot;
        else if (y <= gr_vport.bottom())
            k->region = KeyedDown;
        else
            k->region = KeyedUp;
    }
    else if (x <= gr_vport.left())
        k->region = KeyedLeft;
    else
        k->region = KeyedRight;

    switch (k->region) {
    case KeyedPlot:
        k->x = (int)(0.5 + ((x - gr_vport.left())*1e3)/gr_vport.width());
        k->y = (int)(0.5 + ((y - gr_vport.bottom())*1e3)/gr_vport.height());
        break;
    case KeyedLeft:
        k->x = (int)(0.5 + ((x - effleft())*1e3)/
            (gr_vport.left() - effleft()));
        k->y = (int)(0.5 + ((y - gr_area.bottom())*1e3)/gr_area.height());
        break;
    case KeyedDown:
        k->x = (int)(0.5 + ((x - gr_vport.left())*1e3)/gr_vport.width());
        k->y = (int)(0.5 + ((y - gr_area.bottom())*1e3)/
            (gr_vport.bottom() - gr_area.bottom()));
        break;
    case KeyedRight:
        k->x = (int)(0.5 + ((x - gr_vport.right())*1e3)/
            (gr_area.right() - gr_vport.right()));
        k->y = (int)(0.5 + ((y - gr_area.bottom())*1e3)/gr_area.height());
        break;
    case KeyedUp:
        k->x = (int)(0.5 + ((x - gr_vport.left())*1e3)/gr_vport.width());
        k->y = (int)(0.5 + ((y - gr_vport.top())*1e3)/
            (gr_area.top() - gr_vport.top()));
        break;
    }
}


void
sGraph::gr_get_keyed_posn(sKeyed *k, int *x, int *y)
{
    if (!k) {
        *x = 0;
        *y = 0;
        return;
    }
    switch (k->region) {
    case KeyedPlot:
        *x = (int)(0.5 + (k->x*gr_vport.width())*1e-3 + gr_vport.left());
        *y = (int)(0.5 + (k->y*gr_vport.height())*1e-3 + gr_vport.bottom());
        break;
    case KeyedLeft:
        *x = (int)(0.5 + (k->x*(gr_vport.left() - effleft()))*1e-3) +
            effleft();
        *y = (int)(0.5 + (k->y*gr_area.height())*1e-3) + gr_area.bottom();
        break;
    case KeyedDown:
        *x = (int)(0.5 + (k->x*gr_vport.width())*1e-3 + gr_vport.left());
        *y = (int)(0.5 + (k->y*(gr_vport.bottom() - gr_area.bottom()))*1e-3) +
            gr_area.bottom();;
        break;
    case KeyedRight:
        *x = (int)(0.5 + (k->x*(gr_area.right() - gr_vport.right()))*1e-3 +
            gr_vport.right());
        *y = (int)(0.5 + (k->y*gr_area.height())*1e-3) + gr_area.bottom();
        break;
    case KeyedUp:
        *x = (int)(0.5 + (k->x*gr_vport.width())*1e-3 + gr_vport.left());
        *y = (int)(0.5 + (k->y*(gr_area.top() - gr_vport.top()))*1e-3 +
            gr_vport.top());
        break;
    }
}


bool
sGraph::gr_get_keyed_bb(sKeyed *k, int *xl, int *yb, int *xr, int *yt)
{
    if (!k)
        return (false);
    int fw, fh;
    gr_dev->TextExtent(k->text, &fw, &fh);
    int x, y;
    gr_get_keyed_posn(k, &x, &y);
    if (k->xform & TXTF_HJR)
        x -= fw;
    else if (k->xform & TXTF_HJC)
        x -= fw/2;
    if (xl)
        *xl = x;
    if (yb)
        *yb = y;
    if (xr)
        *xr = x + fw;
    if (yt)
        *yt = y + fh;
    return (true);
}


void
sGraph::gr_writef(double d, const sUnits *units, int x, int y, bool limit)
{
    const char *tmp = SPnum.printnum(d, units, true, NUMDGT);
    if (limit) {
        if (strlen(tmp) > VALBOX_WIDTH)
            tmp = SPnum.printnum(d, (const char*)0, true, NUMDGT);
        // Will always fit now with NUMDGT=5 and VALBOX_WIDTH=14.
    }
#if (defined (WITH_QT5) || defined (WITH_QT6))
    gr_dev->Text(tmp, x, yinv(y) - 2, 0);
#else
    gr_dev->Text(tmp, x, yinv(y), 0);
#endif
}


// Connect the last two points (for iplot).  This won't be done with
// curve interpolation, so it might look funny.
//
void
sGraph::gr_draw_last(int len)
{
    sDvList *dl0 = (sDvList*)gr_plotdata;
    sDataVec *xs = dl0->dl_dvec->scale();

    if (gr_format != FT_SINGLE || gr_ysep) {
        double tmpd[2];
        tmpd[0] = gr_datawin.ymin;
        tmpd[1] = gr_datawin.ymax;
        double tmpary = gr_aspect_y;
        int tmposy=0, tmpht=0;
        int nsy = 0;
        if (gr_ysep) {
            nsy = gr_fonthei/4;
            if (nsy < 2)
                nsy = 2;
            tmposy = gr_vport.bottom();
            tmpht = gr_vport.height();
            gr_vport.set_height(tmpht/gr_numtraces - nsy);
        }
        int j;
        sDvList *dl;
        for (j = 0,dl = dl0; dl; j++,dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            if (gr_ysep)
                gr_vport.set_bottom(tmposy +
                    (gr_numtraces - j - 1)*(gr_vport.height()+nsy) + nsy);
            if (gr_format == FT_GROUP) {
                if (*v->units() == UU_VOLTAGE) {
                    gr_datawin.ymin = gr_grpmin[0];
                    gr_datawin.ymax = gr_grpmax[0];
                }
                else if (*v->units() == UU_CURRENT) {
                    gr_datawin.ymin = gr_grpmin[1];
                    gr_datawin.ymax = gr_grpmax[1];
                }
                else {
                    gr_datawin.ymin = gr_grpmin[2];
                    gr_datawin.ymax = gr_grpmax[2];
                }
            }
            else if (gr_format == FT_MULTI) {
                gr_datawin.ymin = v->minsignal();
                gr_datawin.ymax = v->maxsignal();
            }
            gr_aspect_y =
                (gr_datawin.ymax - gr_datawin.ymin)/gr_vport.height();

            gr_nolinecache = true;
            dv_point(v, xs->realval(len - 1), v->realval(len - 1),
                xs->realval(len - 2), v->realval(len - 2), len - 1);
            gr_nolinecache = false;
        }
        if (gr_ysep) {  
            gr_vport.set_bottom(tmposy);
            gr_vport.set_height(tmpht);
        }
        gr_datawin.ymin = tmpd[0];
        gr_datawin.ymax = tmpd[1];
        gr_aspect_y = tmpary;
    }
    else {
        if (gr_numtraces == 1 && num_colors() > 2) {
            // deal with colored multi-d plot
            sDataVec *v = dl0->dl_dvec;
            if (v->numdims() > 1) {
                int npt = v->dims(v->numdims() - 1);
                int c = v->length()/npt;
                c += 2;
                while (c >= num_colors())
                    c -= num_colors() - 2;
                int ctmp = v->color();
                v->set_color(c);
                gr_nolinecache = true;
                dv_point(v, xs->realval(len - 1), v->realval(len - 1),
                    xs->realval(len - 2), v->realval(len - 2), len - 1);
                gr_nolinecache = false;
                v->set_color(ctmp);
                return;
            }
        }
        gr_nolinecache = true;
        for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            dv_point(v, xs->realval(len - 1), v->realval(len - 1),
                xs->realval(len - 2), v->realval(len - 2), len - 1);
        }
        gr_nolinecache = false;
    }
}


// The next two functions hide the push/pop necessary for the ghost
// drawing functions.
//
void
sGraph::gr_set_ghost(GhostDrawFunc cb, int x, int y)
{
    GP.PushGraphContext(this);
    gr_dev->SetGhost(cb, x, y);
    GP.PopGraphContext();
}


void
sGraph::gr_show_ghost(bool on)
{
    GP.PushGraphContext(this);
    gr_dev->ShowGhost(on);
    GP.PopGraphContext();
}


// Ghost drawing functions, used in wrappers.

void
sGraph::gr_ghost_mark(int x, int y, bool erase)
{
    if (erase) {
#if defined (WITH_QT5) || defined (WITH_QT6)
        dv_erase_factors();
#else
        gr_dev->SetXOR(GRxNone);
        dv_erase_factors();
        gr_dev->SetXOR(GRxXor);
#endif
    }
    gr_dev->SetGhostColor(gr_colors[1].pixel);
    y = yinv(y);
    if (x < gr_vport.left() || x > gr_vport.right())
        return;
    if (y < gr_vport.bottom() || y > gr_vport.top())
        return;
    int xu = gr_vport.right();
    int yu = gr_vport.top();

    sDvList *link = (sDvList*)gr_plotdata;
    sDataVec *scale = link->dl_dvec->scale();

    int indx, rindx;
    double fx, frefx;
    if (gr_xmono) {
        if (!dv_find_where(scale, x, &fx, &indx))
            return;
        if (gr_reference.set) {
            if (!dv_find_where(scale, gr_reference.x, &frefx, &rindx))
                return;
        }
        gr_dev->Line(x, yinv(gr_vport.bottom()), x, yinv(yu));
    }
    else {
        int x1 = x;
        int y1 = gr_vport.bottom();
        int x2 = x;
        int y2 = yu;
        if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                gr_grtype == GRID_SMITHGRID) {
            if (!clip_to_circle(&x1, &y1, &x2, &y2, 
                    gr_xaxis.circular.center, gr_yaxis.circular.center, 
                    gr_xaxis.circular.radius))
                gr_dev->Line(x1, yinv(y1), x2, yinv(y2));
        }
        else
            gr_dev->Line(x1, yinv(y1), x2, yinv(y2));

        x1 = gr_vport.left();
        y1 = y;
        x2 = xu;
        y2 = y;
        if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                gr_grtype == GRID_SMITHGRID) {
            if (!clip_to_circle(&x1, &y1, &x2, &y2, 
                    gr_xaxis.circular.center, gr_yaxis.circular.center, 
                    gr_xaxis.circular.radius))
                gr_dev->Line(x1, yinv(y1), x2, yinv(y2));
        }
        else
            gr_dev->Line(x1, yinv(y1), x2, yinv(y2));
    }
    if (erase)
        return;

    double fy, frefy;
    if (gr_format != FT_SINGLE || gr_ysep) {
        int scrx = gr_vport.left() - (gr_field + 1)*gr_fontwid - gr_fontwid/2;
        int dely = 3*gr_fonthei + gr_fonthei/2;
        int spa;
        if (gr_ysep) {
            spa = gr_vport.height()/gr_numtraces;
            if (spa < dely)
                spa = dely;
        }
        else
            spa = dely;

        if (gr_xmono) {
            for (int i = 0; link; i++, link = link->dl_next) {
                dv_find_y(scale, link->dl_dvec, indx, fx, &fy);
                if (gr_reference.set) {
                    dv_find_y(scale, link->dl_dvec, rindx, frefx, &frefy);
                    fy -= frefy;
                }
                gr_writef(fy, link->dl_dvec->units(), scrx, 
                    yu - i*spa - 2*gr_fonthei - 2, true);
            }
            if (gr_reference.set)
                fx -= frefx;
        }
        else {
            if (gr_ysep)
                // shouldn't be in this mode
                return;
            for (int i = 0; link; i++, link = link->dl_next) {
                double dtmp[2];
                dtmp[0] = gr_datawin.ymin;
                dtmp[1] = gr_datawin.ymax;
                if (gr_format == FT_GROUP) {
                    if (*link->dl_dvec->units() == UU_VOLTAGE) {
                        gr_datawin.ymin = gr_grpmin[0];
                        gr_datawin.ymax = gr_grpmax[0];
                    }
                    else if (*link->dl_dvec->units() == UU_CURRENT) {
                        gr_datawin.ymin = gr_grpmin[1];
                        gr_datawin.ymax = gr_grpmax[1];
                    }
                    else {
                        gr_datawin.ymin = gr_grpmin[2];
                        gr_datawin.ymax = gr_grpmax[2];
                    }
                }
                else {
                    gr_datawin.ymin = link->dl_dvec->minsignal();
                    gr_datawin.ymax = link->dl_dvec->maxsignal();
                }
                double ty = gr_aspect_y;
                gr_aspect_y =
                    (gr_datawin.ymax - gr_datawin.ymin)/gr_vport.height();
                gr_screen_to_data(x, y, &fx, &fy);
                if (gr_reference.set) {
                    gr_screen_to_data(gr_reference.x, gr_reference.y,
                        &frefx, &frefy);
                    fx -= frefx;
                    fy -= frefy;
                }
                gr_aspect_y = ty;
                gr_datawin.ymin = dtmp[0];
                gr_datawin.ymax = dtmp[1];
                gr_writef(fy, link->dl_dvec->units(), scrx, 
                    yu - i*spa - 2*gr_fonthei - 2, true);
            }
        }
    }
    else {
        int scrx = effleft() + 4*gr_fontwid;
        if (gr_xmono) {
            yu += 2*gr_fonthei + gr_fonthei/2;;
            int spa = VALBOX_WIDTH*gr_fontwid;
            int nshow = (gr_vport.width() - gr_fontwid)/spa;
            for (int i = 0; link && i < nshow; i++, link = link->dl_next) {
                dv_find_y(scale, link->dl_dvec, indx, fx, &fy);
                if (gr_reference.set) {
                    dv_find_y(scale, link->dl_dvec, rindx, frefx, &frefy);
                    fy -= frefy;
                }
                gr_writef(fy, link->dl_dvec->units(), scrx + i*spa, yu, true);
            }
            if (gr_reference.set)
                fx -= frefx;
        }
        else {
            gr_screen_to_data(x, y, &fx, &fy);
            if (gr_reference.set) {
                gr_screen_to_data(gr_reference.x, gr_reference.y,
                    &frefx, &frefy);
                fx -= frefx;
                fy -= frefy;
            }
            if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                    gr_grtype == GRID_SMITHGRID) {
                yu -= gr_fonthei/2;
                int dx = x - gr_xaxis.circular.center;
                int dy = y - gr_yaxis.circular.center;
                if (dx*dx + dy*dy > gr_xaxis.circular.radius*
                        gr_xaxis.circular.radius)
                    return;
                double rad = sqrt(fx*fx + fy*fy);
                double ang = (180/M_PI)*atan2(fy, fx);
                if (ang < 0)
                    ang += 360;
                gr_writef(rad, 0, gr_fontwid, gr_vport.bottom(), true);
                gr_writef(ang, 0, gr_fontwid, gr_vport.bottom() - gr_fonthei,
                    true);
                if (gr_grtype == GRID_POLAR)
                    gr_writef(fy, 0, scrx, yu, true);
                else {
                    double dnom = (1.0 - fx)*(1.0 - fx) + fy*fy;
                    if (dnom != 0.0) {
                        double re = (1.0 - fx*fx - fy*fy)/dnom;
                        double im = 2*fy/dnom;
                        fx = re;
                        gr_writef(im, 0, scrx, yu, true);
                    }
                }
            }
            else {
                yu += 2*gr_fonthei + gr_fonthei/2;;
                gr_writef(fy, link->dl_dvec->units(), scrx, yu, true);
            }
        }
    }

    // x factor
    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID)
        gr_writef(fx, 0, gr_vport.right() - gr_field*gr_fontwid,
            gr_vport.bottom() - 3*gr_fonthei, true);
    else
        gr_writef(fx, scale->units(), gr_vport.left(),
            gr_vport.bottom() - 3*gr_fonthei, true);
}


void
sGraph::gr_ghost_zoom(int x, int y, int ref_x, int ref_y)
{
    y = yinv(y);
    int xlbnd = gr_vport.left() - 2;
    int xubnd = gr_vport.right() + 2;
    if (x < xlbnd || x > xubnd)
        return;
    int ylbnd = gr_vport.bottom() - 2;
    int yubnd = gr_vport.top() + 2;
    if (y < ylbnd || y > yubnd)
        return;
    int lowerx, lowery, upperx, uppery;
    if (gr_format != FT_SINGLE || gr_ysep) {
        uppery = yubnd;
        lowery = ylbnd;
    }
    else {
        uppery = ref_y > y ? ref_y : y;
        lowery = ref_y < y ? ref_y : y;
    }
    upperx = ref_x > x ? ref_x : x;
    lowerx = ref_x < x ? ref_x : x;
    gr_dev->SetGhostColor(gr_colors[1].pixel);
    gr_linebox(lowerx, lowery, upperx, uppery);
}


// Show an outline box of the selected text.
//
void
sGraph::gr_ghost_tbox(int x, int y)
{
    if (!GP.SourceGraph())
        return;
    int dx = gr_fontwid/2;
    int dy = gr_fonthei/2;
    if (x < effleft() + dx || x > gr_area.right() - dx ||
            y < gr_area.bottom() + dy || y > gr_area.top() - 2)
        return;
    int fw, fh;
    sKeyed *k = GP.SourceGraph()->gr_keyed;
    if (k) {
        gr_dev->TextExtent(k->text, &fw, &fh);
        if (k->xform & TXTF_HJR)
            x -= fw;
        else if (k->xform & TXTF_HJC)
            x -= fw/2;
        gr_dev->SetGhostColor(gr_colors[1].pixel);
        gr_linebox(x, yinv(y), x + fw, yinv(y - fh));
    }
}


void
sGraph::gr_ghost_trace(int x, int y)
{
    gr_dev->SetGhostColor(gr_colors[1].pixel);
    gr_dev->Line(x+2, y-2, x+8, y-2);
    gr_dev->Line(x+8, y-2, x+8, y-8);
    gr_dev->Line(x+8, y-8, x+16, y-8);
    gr_dev->Line(x+16, y-8, x+16, y-2);
    gr_dev->Line(x+16, y-2, x+24, y-2);
    gr_dev->Line(x+24, y-2, x+24, y-8);
    gr_dev->Line(x+24, y-8, x+32, y-8);
}


void
sGraph::gr_show_sel_text(bool show)
{
    sKeyed *k = gr_keyed;
    if (!k)
        return;

    gr_show_ghost(false);
    int xl, yb, xr, yt;
    if (gr_get_keyed_bb(k, &xl, &yb, &xr, &yt)) {
        gr_dev->SetOverlayMode(true);
        if (show) {
            gr_dev->SetColor(gr_colors[1].pixel);
            gr_dev->Box(xl-4, yinv(yb), xl-3, yinv(yt));
            if (!k->terminated)
                gr_dev->SetColor(gr_colors[2].pixel);
            gr_dev->Line(xr+2, yinv(yb), xr+2, yinv(yt));
            if (k->text && !k->terminated &&
                    k->inspos < (int)strlen(k->text)) {
                gr_dev->SetColor(gr_colors[3].pixel);
                int xx = xl-1 + k->inspos*gr_fontwid;
                gr_dev->Line(xx, yinv(yb), xx, yinv(yt));
            }
#if (defined (WITH_QT5) || defined (WITH_QT6))
            // Update the justification indicator, QT only.
            gr_dev->SetColor(gr_colors[4].pixel);
            if (gr_xform & TXTF_HJC) {
                int xc = (xl + xr)/2;
                gr_dev->Line(xc-1, yinv(yb)+1, xc+1, yinv(yb)+1);
                gr_dev->Line(xc-1, yinv(yb)+2, xc+1, yinv(yb)+2);
            }
            else if (gr_xform & TXTF_HJR) {
                gr_dev->Line(xr-2, yinv(yb)+1, xr, yinv(yb)+1);
                gr_dev->Line(xr-2, yinv(yb)+2, xr, yinv(yb)+2);
            }
            else {
                gr_dev->Line(xl, yinv(yb)+1, xl+2, yinv(yb)+1);
                gr_dev->Line(xl, yinv(yb)+2, xl+2, yinv(yb)+2);
            }
#endif
        }
        else {
            gr_refresh(xl-5, yinv(yb)+1, xl-2, yinv(yt));
            gr_refresh(xr+2, yinv(yb)+3, xr+3, yinv(yt));
            int xx = xl-1 + k->inspos*gr_fontwid;
            gr_refresh(xx, yinv(yb)+1, xx+1, yinv(yt));
            gr_refresh(xl-1, yinv(yb)+2, xr+1, yinv(yb)+1);
        }
        gr_dev->SetOverlayMode(false);
    }
    gr_show_ghost(true);
}


namespace {
    // Whiteley Research Logo bitmap.
    //    
    // XPM
    const char *tm24[] = {
        // width height ncolors chars_per_pixel
        "48 16 2 1",
        // colors
        "   c none",
        ".  c blue",
        // pixels
        "                                                ",
        "                                                ",
        " ...     ..     .....                           ",
        " ...     ..    .......                          ",
        " ....   ....   ...  ...                         ",
        "  ....  ....  ....  ...  ...  ...  .  ...  .... ",
        "  ....  ..... ......... .   . .  . . .   . .    ",
        "   .......... .......   .     .  . . .     .    ",
        "   .................    .     .  . . .     .    ",
        "    ...... ..........    ...  ...  . .     ...  ",
        "    .....  .....  ....      . .    . .     .    ",
        "     ....   ....   ....     . .    . .     .    ",
        "     ....   ....   .... .   . .    . .   . .    ",
        "      ..     ..     ..   ...  .    .  ...  .... ",
        "                                                ",
        "                                                "};

    // Polygon representation of above, on a 1000-unit grid.  This is
    // basically CIF, but semicolons must be separate tokens.
    //
    const char *logo_polys = 
    "P 0 10000 0 12000 2000 12000 6000 6000 9000 12000 13000 6500 15000"
    "  12000 17000 12000 20000 11000 22000 10000 22000 9000 20000 9000"
    "  18000 10000 17000 10000 16000 8000 18000 7000 19000 7000 20000"
    "  9000 22000 9000 22000 7000 20000 5000 22000 2000 22000 0 20000 0"
    "  16000 5000 13000 0 9000 6000 6000 0 0 10000 ; "
    "P 24000 3000 24000 2000 25000 1000 28000 1000 29000 2000 29000"
    "  5000 28000 6000 25000 6000 25000 9000 28000 9000 28000 8000"
    "  29000 8000 29000 9000 28000 10000 25000 10000 24000 9000 24000"
    "  6000 25000 5000 28000 5000 28000 2000 25000 2000 25000 3000"
    "  24000 3000 ; "
    "P 30000 1000 30000 10000 33000 10000 34000 9000 34000 6000 33000"
    "  5000 31000 5000 31000 6000 33000 6000 33000 9000 31000 9000"
    "  31000 1000 30000 1000 ; "
    "P 35000 1000 35000 10000 36000 10000 36000 1000 35000 1000 ; "
    "P 41000 8000 41000 9000 38000 9000 38000 2000 41000 2000 41000"
    "  3000 42000 3000 42000 2000 41000 1000 38000 1000 37000 2000"
    "  37000 9000 38000 10000 41000 10000 42000 9000 42000 8000 41000"
    "  8000 ; "
    "P 47000 10000 43000 10000 43000 1000 47000 1000 47000 2000 44000"
    "  2000 44000 5000 46000 5000 46000 6000 44000 6000 44000 9000"
    "  47000 9000 47000 10000 ; ";
}


// Show a "WRspice" logo in the plot.
//
void
sGraph::gr_show_logo()
{
    if (gr_noplotlogo || gr_present)
        return;
    int ident = GRpkg::self()->CurDev()->ident;
    if (ident == _devHP_)
        // HPGL plotter.  A pen plotter may have trouble with the logo,
        // so skip it.
        return;

    gr_dev->SetColor(gr_colors[4].pixel);
    int x = gr_area.right() - 48 - 4;

    int y;
    if (gr_apptype == GR_MPLT)
        y = gr_area.top() - 2*gr_fonthei;
    else
        y = gr_vport.top() + 3*gr_fonthei;

    if (ident == _devPS_ || ident == _devPSBC_ || ident == _devPSBM_ ||
            gr_fonthei > 28) {
        // Using PostScript back end, or the character height supports
        // at least 2X the single-pixel resolution.  In this case, use
        // the polygon representation.

        int h = gr_fonthei;
        if (h < 14)
            h = 14;
        double scale = h/14.0;;

        int itmp[128];  // must be big enough!
        int icnt = 0;
        char type = 0;
        const char *s = logo_polys;
        char *tok;
        while ((tok = lstring::gettok(&s)) != 0) {
            if (*tok == ';') {
                if (type == 'B') {
                    int dx = itmp[0]/2;
                    int dy = itmp[1]/2;
                    gr_dev->Box(
                        x + itmp[2] - dx, yinv(y + itmp[3] - dy + 1),
                        x + itmp[2] + dx - (dx > 0 ? 1 : 0),
                        yinv(y + itmp[3] + dy));
                }
                else if (type == 'P') {
                    int npts = icnt/2;
                    GRmultiPt p(npts);
                    for (int i = 0; i < npts; i++)
                        p.assign(i, x + itmp[2*i], yinv(y + itmp[2*i + 1]));
                    gr_dev->Polygon(&p, npts);
                }
                delete [] tok;
                icnt = 0;
                continue;
            }
            if (isalpha(*tok)) {
                type = *tok;
                delete [] tok;
                icnt = 0;
                continue;
            }
            itmp[icnt] = (int)(scale*atoi(tok)/1000);
            icnt++;
            delete [] tok;
        }
    }
    else {
        // Single pixel resolution, blast out the bits.

        y += gr_fonthei;
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 48; j++) {
                if (tm24[3+i][j] == '.')
                    gr_dev->Pixel(x+j, yinv(y-i));

            }
        }
    }
}
// End of public functions.


// Handle clicks on a scale translation icon, return true if handled.
//
bool
sGraph::dv_scale_icon_hdlr(int button, int x, int y)
{
    if (y > gr_area.bottom() + gr_fonthei/2 - gr_fontwid/2 &&
            y < gr_area.bottom() + gr_fontwid + gr_fonthei/2 +
            gr_fontwid/2) {
        if (x > gr_vport.left() - gr_fontwid/2 &&
                x < gr_vport.left() + gr_fontwid + gr_fontwid/2) {
            // left arrow
            double dx = gr_datawin.xmax - gr_datawin.xmin;
            if (gr_grtype == GRID_XLOG || gr_grtype == GRID_LOGLOG) {
                if (button == 1) {
                    gr_rawdata.xmin = gr_datawin.xmin/10;
                    gr_rawdata.xmax = gr_datawin.xmax/10;
                }
                else if (button == 2)
                    gr_rawdata.xmin = gr_datawin.xmin/10;
                else if (gr_datawin.xmin*10 < gr_datawin.xmax/2)
                    gr_rawdata.xmin = gr_datawin.xmin*10;
            }
            else {
                int nsp = gr_xaxis.lin.numspace;
                double del = dx/nsp;
                if (button == 1) {
                    gr_rawdata.xmin = gr_datawin.xmin - del;
                    gr_rawdata.xmax = gr_datawin.xmax - del;
                }
                else if (button == 2)
                    gr_rawdata.xmin = gr_datawin.xmin - del;
                else if (gr_datawin.xmin + del < gr_datawin.xmax - del/2)
                    gr_rawdata.xmin = gr_datawin.xmin + del;
            }
            gr_dev->Clear();
            gr_scale_flags |= 1;
            gr_redraw();
            return (true);
        }
        if (x > gr_vport.left() + 3*gr_fontwid - gr_fontwid/2 &&
                x < gr_vport.left() + 4*gr_fontwid + gr_fontwid/2) {
            // right arrow
            double dx = gr_datawin.xmax - gr_datawin.xmin;
            if (gr_grtype == GRID_XLOG || gr_grtype == GRID_LOGLOG) {
                if (button == 1) {
                    gr_rawdata.xmin = gr_datawin.xmin*10;
                    gr_rawdata.xmax = gr_datawin.xmax*10;
                }
                else if (button == 2)
                    gr_rawdata.xmax = gr_datawin.xmax*10;
                else if (gr_datawin.xmax/10 > gr_datawin.xmin*2)
                    gr_rawdata.xmax = gr_datawin.xmax/10;
            }
            else {
                int nsp = gr_xaxis.lin.numspace;
                double del = dx/nsp;
                if (button == 1) {
                    gr_rawdata.xmin = gr_datawin.xmin + del;
                    gr_rawdata.xmax = gr_datawin.xmax + del;
                }
                else if (button == 2)
                    gr_rawdata.xmax = gr_datawin.xmax + del;
                else if (gr_datawin.xmax - del > gr_datawin.xmin + del/2)
                    gr_rawdata.xmax = gr_datawin.xmax - del;
            }
            gr_dev->Clear();
            gr_scale_flags |= 1;
            gr_redraw();
            return (true);
        }
    }
    if (x > gr_area.right() - 2*gr_fontwid - gr_fontwid/2 &&
            x < gr_area.right() - gr_fontwid/2) {
        int y0 = gr_vport.top();
        int d = gr_fontwid;
        sDvList *dl = (sDvList*)gr_plotdata;
        unsigned mask = 16;
        for (int i = 0; i < gr_numtraces; i++, dl = dl->dl_next) {
            if (!dl)
                break;
            if (gr_format == FT_SINGLE && i > 0)
                break;
            sDataVec *v = dl->dl_dvec;
            mask <<= 1;

            int dely = 3*gr_fonthei + gr_fonthei/2;
            int spa;
            if (gr_ysep)
                spa = gr_vport.height()/gr_numtraces;
            else
                spa = dely;

            if (y > y0 - d - d/2 && y < y0 + d/2) {
                // up arrow for trace
                if (gr_format == FT_SINGLE) {
                    double dy = gr_datawin.ymax - gr_datawin.ymin;
                    if (gr_grtype == GRID_YLOG ||
                            gr_grtype == GRID_LOGLOG) {
                        if (button == 1) {
                            gr_rawdata.ymin = gr_datawin.ymin*10;
                            gr_rawdata.ymax = gr_datawin.ymax*10;
                        }
                        else if (button == 2)
                            gr_rawdata.ymax = gr_datawin.ymax*10;
                        else if (gr_datawin.ymax/10 > gr_datawin.ymin*2)
                            gr_rawdata.ymax = gr_datawin.ymax/10;
                    }
                    else {
                        int nsp = gr_yaxis.lin.numspace;
                        double del = dy/nsp;
                        if (button == 1) {
                            gr_rawdata.ymin = gr_datawin.ymin + del;
                            gr_rawdata.ymax = gr_datawin.ymax + del;
                        }
                        else if (button == 2)
                            gr_rawdata.ymax = gr_datawin.ymax + del;
                        else
                            gr_rawdata.ymax = gr_datawin.ymax - 2*del;
                    }
                    gr_datawin.ymin = gr_rawdata.ymin;
                    gr_datawin.ymax = gr_rawdata.ymax;
                    gr_scale_flags |= 16;
                }
                else if (gr_format == FT_GROUP) {
                    int m;
                    if (*v->units() == UU_VOLTAGE) {
                        m = 0;
                        gr_scale_flags |= 2;
                    }
                    else if (*v->units() == UU_CURRENT) {
                        m = 1;
                        gr_scale_flags |= 4;
                    }
                    else {
                        m = 2;
                        gr_scale_flags |= 8;
                    }
                    if (gr_grtype == GRID_YLOG ||
                            gr_grtype == GRID_LOGLOG) {
                        if (button == 1) {
                            gr_grpmin[m] *= 10;
                            gr_grpmax[m] *= 10;
                        }
                        else if (gr_ysep) {
                            if (button == 2)
                                gr_grpmax[m] *= 10;
                            else if (gr_grpmax[m]/10 > gr_grpmin[m]*2)
                                gr_grpmax[m] /= 10;
                        }
                    }
                    else {
                        int nsp = 4;
                        double l = gr_grpmin[m];
                        double u = gr_grpmax[m];
                        if (gr_ysep)
                            set_scale(l, u, &l, &u, &nsp, 0.0);
                        else
                            set_scale_4(l, u, &l, &u, 0.0);
                        double del = (u - l)/nsp;
                        if (button == 1) {
                            gr_grpmin[m] += del;
                            gr_grpmax[m] += del;
                        }
                        else if (button == 2)
                            gr_grpmax[m] += del;
                        else 
                            gr_grpmax[m] -= 2*del;
                    }
                }
                else if (gr_format == FT_MULTI) {
                    if (gr_grtype == GRID_YLOG ||
                            gr_grtype == GRID_LOGLOG) {
                        if (button == 1) {
                            v->set_minsignal(v->minsignal() * 10);
                            v->set_maxsignal(v->maxsignal() * 10);
                            gr_scale_flags |= mask;
                        }
                        else if (gr_ysep) {
                            if (button == 2)
                                v->set_maxsignal(v->maxsignal() * 10);
                            else if (v->maxsignal()/10 > v->minsignal()*2)
                                v->set_maxsignal(v->maxsignal() / 10);
                            gr_scale_flags |= mask;
                        }
                    }
                    else {
                        int nsp = 4;
                        double l = v->minsignal();
                        double u = v->maxsignal();
                        if (gr_ysep)
                            set_scale(l, u, &l, &u, &nsp, 0.0);
                        else
                            set_scale_4(l, u, &l, &u, 0.0);
                        double del = (u - l)/nsp;
                        if (button == 1) {
                            v->set_minsignal(v->minsignal() + del);
                            v->set_maxsignal(v->maxsignal() + del);
                        }
                        else if (button == 2)
                            v->set_maxsignal(v->maxsignal() + del);
                        else 
                            v->set_maxsignal(v->maxsignal() - 2*del);
                        gr_scale_flags |= mask;
                    }
                }
                gr_dev->Clear();
                gr_redraw();
                return (true);
            }
            y0 -= 2*d;
            if (y > y0 - d - d/2 && y < y0 + d/2) {
                // down arrow for trace
                if (gr_format == FT_SINGLE) {
                    double dy = gr_datawin.ymax - gr_datawin.ymin;
                    if (gr_grtype == GRID_YLOG ||
                            gr_grtype == GRID_LOGLOG) {
                        if (button == 1) {
                            gr_rawdata.ymin = gr_datawin.ymin/10;
                            gr_rawdata.ymax = gr_datawin.ymax/10;
                        }
                        else if (button == 2)
                            gr_rawdata.ymax = gr_datawin.ymax*10;
                        else if (gr_datawin.ymax/10 > gr_datawin.ymin*2)
                            gr_rawdata.ymax = gr_datawin.ymax/10;
                    }
                    else {
                        int nsp = gr_yaxis.lin.numspace;
                        double del = dy/nsp;
                        if (button == 1) {
                            gr_rawdata.ymin = gr_datawin.ymin - del;
                            gr_rawdata.ymax = gr_datawin.ymax - del;
                        }
                        else if (button == 2)
                            gr_rawdata.ymin = gr_datawin.ymin - del;
                        else
                            gr_rawdata.ymin = gr_datawin.ymin + 2*del;
                    }
                    gr_datawin.ymin = gr_rawdata.ymin;
                    gr_datawin.ymax = gr_rawdata.ymax;
                    gr_scale_flags |= 16;
                }
                if (gr_format == FT_GROUP) {
                    int m;
                    if (*v->units() == UU_VOLTAGE) {
                        m = 0;
                        gr_scale_flags |= 2;
                    }
                    else if (*v->units() == UU_CURRENT) {
                        m = 1;
                        gr_scale_flags |= 4;
                    }
                    else {
                        m = 2;
                        gr_scale_flags |= 8;
                    }
                    if (gr_grtype == GRID_YLOG ||
                            gr_grtype == GRID_LOGLOG) {
                        if (button == 1) {
                            gr_grpmin[m] /= 10;
                            gr_grpmax[m] /= 10;
                        }
                        else if (gr_ysep) {
                            if (button == 2)
                                gr_grpmax[m] /= 10;
                            else if (gr_grpmin[m]*10 < gr_grpmax[m]/2)
                                gr_grpmin[m] *= 10;
                        }
                    }
                    else {
                        int nsp = 4;
                        double l = gr_grpmin[m];
                        double u = gr_grpmax[m];
                        if (gr_ysep)
                            set_scale(l, u, &l, &u, &nsp, 0.0);
                        else
                            set_scale_4(l, u, &l, &u, 0.0);
                        double del = (u - l)/nsp;
                        if (button == 1) {
                            gr_grpmin[m] -= del;
                            gr_grpmax[m] -= del;
                        }
                        else if (button == 2)
                            gr_grpmin[m] -= del;
                        else 
                            gr_grpmin[m] += 2*del;
                    }
                }
                else if (gr_format == FT_MULTI) {
                    if (gr_grtype == GRID_YLOG ||
                            gr_grtype == GRID_LOGLOG) {
                        if (button == 1) {
                            v->set_minsignal(v->minsignal() / 10);
                            v->set_maxsignal(v->maxsignal() / 10);
                            gr_scale_flags |= mask;
                        }
                        else if (gr_ysep) {
                            if (button == 2)
                                v->set_minsignal(v->minsignal() / 10);
                            else if (v->minsignal()*10 < v->maxsignal()/2)
                                v->set_minsignal(v->minsignal() * 10);
                            gr_scale_flags |= mask;
                        }
                    }
                    else {
                        int nsp = 4;
                        double l = v->minsignal();
                        double u = v->maxsignal();
                        if (gr_ysep)
                            set_scale(l, u, &l, &u, &nsp, 0.0);
                        else
                            set_scale_4(l, u, &l, &u, 0.0);
                        double del = (u - l)/nsp;
                        if (button == 1) {
                            v->set_minsignal(v->minsignal() - del);
                            v->set_maxsignal(v->maxsignal() - del);
                        }
                        else if (button == 2)
                            v->set_minsignal(v->minsignal() - del);
                        else 
                            v->set_minsignal(v->minsignal() + 2*del);
                        gr_scale_flags |= mask;
                    }
                }
                gr_dev->Clear();
                gr_redraw();
                return (true);
            }
            y0 += 2*d;
            y0 -= spa;
        }
    }
    return (false);
}


namespace {
    bool col_dec(sDataVec *xs, int x, int x0, int delta, int *nx)
    {
        if (x < x0)
            return (false);
        int cst = 0;
        for (int i = 0; i < xs->numdims() - 1; i++) {
            int cnd = cst + 1;
            int z = xs->dims(i);
            while (z) {
                z /= 10;
                cnd++;
            }
            if (x >= x0 + cst*delta && x < x0 + cnd*delta) {
                *nx = i;
                return (true);
            }
            cst = cnd;
        }
        return (false);
    }
}


namespace {
    int dimcols(sDataVec *xs, bool flat)
    {
        int cst = 0;
        int cnd = 0;
        if (flat)
            return (4);
        else {
            for (int i = 0; i < xs->numdims() - 1; i++) {
                cnd = cst + 1;
                int z = xs->dims(i);
                while (z) {
                    z /= 10;
                    cnd++;
                }
                cst = cnd;
            }
        }
        return (cnd);
    }
}


// Handle clicks on the dimension map, return true if handled.
//
bool
sGraph::dv_dims_map_hdlr(int button, int xin, int yin, bool up)
{
    if (!gr_sel_show)
        return (false);

    // Dragging setup, user can drag over multiple numbers to toggle
    // their state.
    int x, ymin, ymax;
    if (!up) {
        x = xin;
        ymin = yin;
        ymax = yin;
        gr_sel_x = xin;
        gr_sel_y = yin;
        gr_sel_drag = false;
    }
    else {
        if (!gr_sel_drag)
            return (false);
        gr_sel_drag = false;
        x = gr_sel_x;
        if (yin > gr_sel_y) {
            ymin = gr_sel_y;
            ymax = yin;
        }
        else {
            ymin = yin;
            ymax = gr_sel_y;
        }
    }

    sDataVec *xs = (gr_oneval ? 0 : ((sDvList*)gr_plotdata)->dl_dvec->scale());
    if (!xs)
        return (false);
    int x0;
    if (gr_numtraces == 1)
        x0 = gr_fontwid + effleft();
    else
        x0 = effleft() - dimcols(xs, gr_sel_flat)*gr_fontwid;

    bool need_redraw = false;
    for (int y = ymin; y/gr_fonthei <= ymax/gr_fonthei; y += gr_fonthei) {
        int y0;
        if (gr_numtraces == 1)
            y0 = gr_vport.top() - 4*gr_fonthei;
        else
            y0 = gr_vport.top();
        int numdtab = y0/gr_fonthei - 2;
        y0 += gr_fonthei;
        if (gr_sel_flat) {
            if (x >= x0 && x <= x0 + 4*gr_fontwid) {
                if (y <= y0) {
                    int nm = (y0 - y)/gr_fonthei;
                    if (nm == numdtab) {
                        if (gr_npage > 1) {
                            gr_cpage++;
                            if (gr_cpage >= gr_npage)
                                gr_cpage = 0;
                            gr_dev->Clear();
                            gr_redraw();
                            return (true);
                        }
                    }
                    else if (nm < numdtab) {
                        if (!up) {
                            gr_sel_drag = true;
                            return (true);
                        }
                        nm += gr_cpage*numdtab;
                        if (nm < gr_selsize) {
                            gr_selections[nm] ^= 1;
                            need_redraw = true;
                            continue;
                        }
                    }
                }
            }
        }
        else {
            int ncol = xs->numdims() - 1;
            int nx;
            if (col_dec(xs, x, x0, gr_fontwid, &nx)) {
                if (y <= y0) {
                    int ny = (y0 - y)/gr_fonthei;
                    int mtot = 1;
                    for (int i = 0; i < ncol; i++)
                        mtot *= xs->dims(i);

                    if (ny == numdtab && numdtab > 0) {
                        if (gr_npage > 1) {
                            gr_cpage++;
                            if (gr_cpage >= gr_npage)
                                gr_cpage = 0;
                            gr_dev->Clear();
                            gr_redraw();
                            return (true);
                        }
                    }
                    else if (ny < numdtab) {
                        if (!up) {
                            gr_sel_drag = true;
                            return (true);
                        }
                        if (gr_npage > 1 && xs->dims(nx) > numdtab)
                            ny += gr_cpage*numdtab;
                        if (ny < xs->dims(nx)) {
                            int mask = 1 << (ncol - nx - 1);
                            if (nx == ncol - 1) {
                                int nn = xs->dims(nx);
                                for (int i = ny; i < mtot; i += nn) {
                                    if (i < gr_selsize) {
                                        gr_selections[i] ^= mask;
                                        need_redraw = true;
                                    }
                                }
                            }
                            else if (nx == 0) {
                                int nn = xs->dims(ncol-1);
                                for (int i = ncol-2; i > nx; i--)
                                    nn *= xs->dims(i);
                                nn *= ny;
                                if (nn < gr_selsize) {
                                    gr_selections[nn] ^= mask;
                                    need_redraw = true;
                                }
                            }
                            else {
                                int nn = xs->dims(ncol-1);
                                for (int i = ncol-2; i > nx; i--)
                                    nn *= xs->dims(i);
                                int st = nn*ny;
                                nn *= xs->dims(nx);
                                for (int i = st; i < mtot; i += nn) {
                                    if (i < gr_selsize) {
                                        gr_selections[i] ^= mask;
                                        need_redraw = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (need_redraw) {
        if (button == 2) {
            for (int i = 0; i < gr_selsize; i++)
                gr_selections[i] = 0;
        }
        else if (button == 3) {
            for (int i = 0; i < gr_selsize; i++)
                gr_selections[i] = -1;
        }
        gr_dev->Clear();
        gr_redraw();
        return (true);
    }
    return (false);
}


// Copy dvecs.
//
sDvList *
sGraph::dv_copy_data()
{
    sDvList *dl = (sDvList*)gr_plotdata;
    if (dl == 0)
        return (0);

    sDvList *dn = new sDvList;
    sDvList *linkp = dn;

    sDataVec *scale = dl->dl_dvec->scale();
    if (scale) {
        scale = scale->copy();
        scale->set_plot(0);
    }

    for ( ; dl; dl = dl->dl_next) {
        // the copy is not linked into any plot
        sDataVec *newv = dl->dl_dvec->copy();
        newv->set_plot(0);
        // vec->copy doesn't set color or linestyle
        newv->set_color(dl->dl_dvec->color());
        newv->set_linestyle(dl->dl_dvec->linestyle());
        newv->set_scale(scale);
        dn->dl_dvec = newv;
        if (dl->dl_next) {
            dn->dl_next = new sDvList;
            dn = dn->dl_next;
        }
    }
    return (linkp);
}


// De-allocate dveclist.
//
void
sGraph::dv_destroy_data()
{
    sDvList *dl = static_cast<sDvList*>(gr_plotdata);
    if (dl) {
        delete dl->dl_dvec->scale();

        sDvList *dn;
        for ( ; dl; dl = dn) {
            dn = dl->dl_next;
            delete dl->dl_dvec;
            delete dl;
        }
    }
    gr_plotdata = 0;

    dl = static_cast<sDvList*>(gr_hidden_data);
    if (dl) {
        sDvList *dn;
        for ( ; dl; dl = dn) {
            dn = dl->dl_next;
            delete dl->dl_dvec;
            delete dl;
        }
    }
    gr_hidden_data = 0;
}


namespace {
    // Icon for hidden trces.
    // XPM
    const char *cabinet_xpm[] = {
    "16 20 3 1",
    " 	c none",
    ".	c blue",
    "X	c black",
    "                ",
    "   ...........  ",
    "  .         ..  ",
    " ........... .  ",
    " .         . .  ",
    " .         . .  ",
    " .   XXX   . .  ",
    " .   XXX   . .  ",
    " .         . .  ",
    " .         . .  ",
    " ........... .  ",
    " .         . .  ",
    " .         . .  ",
    " .   XXX   . .  ",
    " .   XXX   . .  ",
    " .         . .  ",
    " .         ..   ",
    " ...........    ",
    "                ",
    "                "};
}


// Redraw everything in struct graph.
//
bool
sGraph::dv_redraw()
{
    bool tstop = gr_stop;
    gr_stop = false;
    gr_init_data();
    dv_resize();

    // redraw grid
    gr_redrawgrid();

    // Most of the time, the xlimit is observed if set, meaning that
    // only the plot data between the limits is shown, no matter the
    // scale.  We show all data available for the scale, however, if
    // the condition below holds.  If this is a "zoomin" plot,
    // gr_scale_flags has a bit set.  If the '1' flag is set, the
    // horizontal scrolling has been used, and the gr_rawdata have
    // already been fixed.  I can't think of why we need to do this if
    // a vertical scale icon has been used, I suppose that there is
    // some subtle reason?
    //
    if (gr_scale_flags && !(gr_scale_flags & 1)) {
        // do this only after a scale change icon was selected
        if (gr_rawdata.xmin > gr_datawin.xmin)
            gr_rawdata.xmin = gr_datawin.xmin;
        if (gr_rawdata.xmax < gr_datawin.xmax)
            gr_rawdata.xmax = gr_datawin.xmax;
    }

    if (!gr_reference.mark)
        gr_set_ghost(0, 0, 0);

    // Do the plotting.
    dv_trace(false);

    if (!gr_present) {
        // Show the dimensions map icon, but not in hard-copies.
        if (gr_selections && gr_selsize &&
                GRpkg::self()->CurDev()->devtype != GRhardcopy) {
            int x = DIM_ICON_X;
            int y = DIM_ICON_Y;
            int w = gr_fontwid+2;
            int d = gr_fonthei/2;
            y += d;
            GRmultiPt p(6);
            if (gr_sel_show) {
                p.assign(0, x, yinv(y-d));
                p.assign(1, x+w, yinv(y-d));
                p.assign(2, x+w+d, yinv(y));
                p.assign(3, x+w, yinv(y+d));
                p.assign(4, x, yinv(y+d));
                p.assign(5, x, yinv(y-d));
#ifdef WIN32
                y -= 2;
#endif
                gr_dev->SetColor(gr_colors[5].pixel);
                gr_dev->PolyLine(&p, 6);
                gr_dev->SetColor(gr_colors[4].pixel);
                gr_dev->Text("d", x+2, yinv(y-d), 0);
            }
            else {
                p.assign(0, x, yinv(y-d));
                p.assign(1, x+w, yinv(y-d));
                p.assign(2, x+w, yinv(y+d));
                p.assign(3, x, yinv(y+d));
                p.assign(4, x-d, yinv(y));
                p.assign(5, x, yinv(y-d));
#ifdef WIN32
                y -= 2;
#endif
                gr_dev->SetColor(gr_colors[5].pixel);
                gr_dev->PolyLine(&p, 6);
                gr_dev->SetColor(gr_colors[4].pixel);
                gr_dev->Text("d", x+1, yinv(y-d), 0);
            }
        }

        int yu = gr_vport.top() + gr_fonthei + gr_fonthei/2;
        if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                gr_grtype == GRID_SMITHGRID) {
            gr_save_text(gr_title, effleft() + gr_fontwid, yu, LAtitle, 1, 0);
            gr_save_text(gr_plotname, effleft() + gr_fontwid, yu - gr_fonthei,
                LAname, 8, 0);
            gr_save_text(gr_date, effleft() + gr_fontwid,
                gr_area.bottom() + gr_fonthei, LAdate, 8, 0);
        }
        else {
            gr_save_text(gr_title, gr_vport.left(), yu, LAtitle, 1,0);
            gr_save_text(gr_plotname, gr_vport.left(), yu - gr_fonthei, LAname,
                8, 0);
            int nt = gr_title ? strlen(gr_title) : 0;
            int np = gr_plotname ? strlen(gr_plotname) : 0;
            if (nt < np) {
                gr_save_text(gr_date, gr_vport.right(), yu, LAdate, 8,
                    TXTF_HJR);
            }
            else {
                gr_save_text(gr_date, gr_vport.right(), yu - gr_fonthei, LAdate,
                    8, TXTF_HJR);
            }

            if (GRpkg::self()->CurDev()->devtype != GRhardcopy) {
                if (gr_format != FT_SINGLE || gr_ysep) {
                    // Icons to shift the left side field width.
                    int x = gr_vport.left() - 3*gr_fontwid - gr_fontwid/2;
                    int y = gr_fonthei/2 + gr_vport.top();
                    int d = gr_fontwid;
                    gr_dev->SetColor(gr_colors[1].pixel);
                    GRmultiPt p(4);
                    if (gr_field > FIELD_MIN) {
                        p.assign(0, x + d, yinv(y));
                        p.assign(1, x, yinv(y + d/2));
                        p.assign(2, x + d, yinv(y + d));
                        p.assign(3, x + d, yinv(y));
                        gr_dev->Polygon(&p, 4);
                    }
                    if (gr_field < FIELD_MAX) {
                        x += gr_fontwid + gr_fontwid/2;
                        p.assign(0, x, yinv(y));
                        p.assign(1, x + d, yinv(y + d/2));
                        p.assign(2, x, yinv(y + d));
                        p.assign(3, x, yinv(y));
                        gr_dev->Polygon(&p, 4);
                    }
                }

                // scale translation icons
                int x = gr_vport.left();
                int y = gr_fonthei/2 + gr_area.bottom();
                int d = gr_fontwid;
                gr_dev->SetColor(gr_colors[1].pixel);
                GRmultiPt p(4);
                p.assign(0, x + d, yinv(y));
                p.assign(1, x, yinv(y + d/2));
                p.assign(2, x + d, yinv(y + d));
                p.assign(3, x + d, yinv(y));
                gr_dev->Polygon(&p, 4);
                x += 3*gr_fontwid;
                p.assign(0, x, yinv(y));
                p.assign(1, x + d, yinv(y + d/2));
                p.assign(2, x, yinv(y + d));
                p.assign(3, x, yinv(y));
                gr_dev->Polygon(&p, 4);

                // The vertical scale icons are rendered with the trace
                // legends.
            }
        }
    }
    bool ret = gr_stop;
    gr_stop = tstop;

    // Show a logo on the plot.
    gr_show_logo();

    if (GRpkg::self()->CurDev()->devtype != GRhardcopy) {
        // "file cabinet" drop target
        int x = 2;
        int y = yinv(2);
        if (gr_hidden_data)
            gr_dev->SetColor(gr_colors[4].pixel);
        else
            gr_dev->SetColor(0xaaaaaa);
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 16; j++) {
                if (cabinet_xpm[4+i][j] == '.')
                    gr_dev->Pixel(x+j, yinv(y-i));

            }
        }
        gr_dev->SetColor(gr_colors[1].pixel);
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 16; j++) {
                if (cabinet_xpm[4+i][j] == 'X')
                    gr_dev->Pixel(x+j, yinv(y-i));

            }
        }
    }

    return (ret);
}


namespace { const char *PointChars; }

void
sGraph::dv_initdata()
{
    gr_dev->TextExtent(0, &gr_fontwid, &gr_fonthei);

    // Note:  GRpkg::self()->Cur->numlinestyles == 0 implies that the
    // actual number is not bounded in the application code.
    //
    // Set up colors and line styles.
    int curlst;
    if (GRpkg::self()->CurDev()->numlinestyles == 1 || num_colors() > 2 ||
            gr_ysep)
        curlst = -1; // Use the same one all the time.
    else
        curlst = 0;
    int curcolor;
    if (num_colors() > 4 && (gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID))
        curcolor = 3;
    else
        curcolor = 1;
    if (gr_pltype == PLOT_POINT) {
        // If the pointchars variable is set, or if doing a hardcopy,
        // use a text glyph for point plots.  Otherwise (under X) use
        // the internal glyphs (positioning is more accurate).
        variable *va = Sp.GetRawVar(kw_pointchars);
        if (va && va->type() == VTYP_STRING)
            PointChars = va->string();
        else if (va)
            // a bool
            PointChars = SpGrPkg::DefPointchars;
        else {
            if (GRpkg::self()->CurDev()->devtype == GRhardcopy)
                PointChars = SpGrPkg::DefPointchars;
            else
                PointChars = 0;
        }
    }
    int curptchar = 0;

    sDvList *dl0 = static_cast<sDvList*>(gr_plotdata);
    sDvList *dl;
    for (dl = dl0; dl; dl = dl->dl_next) {
        sDataVec *v = dl->dl_dvec;

        // Find a (hopefully) new line style and color
        if (++curcolor == num_colors()) {
            if ((gr_grtype == GRID_SMITH || gr_grtype == GRID_SMITHGRID) &&
                    num_colors() > 4)
                curcolor = 4;
            else if (num_colors() > 2)
                curcolor = 2;
            else
                curcolor = 1;
        }
        v->set_color(curcolor);
        // change the color to that of the trace, if given
        if (v->defcolor()) {
            gr_colors[curcolor].pixel = GRpkg::self()->NameColor(v->defcolor());
            int r, g, b;
            GRpkg::self()->RGBofPixel(gr_colors[curcolor].pixel, &r, &g, &b);
            gr_colors[curcolor].red = r;
            gr_colors[curcolor].green = g;
            gr_colors[curcolor].blue = b;
        }

        // Do something special with poles and zeros.  Poles are 'x's, and
        // zeros are 'o's.
        //
        if (v->flags() & VF_POLE) {
            PointChars = SpGrPkg::DefPointchars;
            v->set_linestyle(1);
            continue;
        }
        else if (v->flags() & VF_ZERO) {
            PointChars = SpGrPkg::DefPointchars;
            v->set_linestyle(0);
            continue;
        }
        if (gr_pltype == PLOT_POINT) {
            if (PointChars && !PointChars[curptchar])
                curptchar = 0;
            v->set_linestyle(curptchar);
            curptchar++;
        }
        else  {
            v->set_linestyle((curlst >= 0 ? curlst : 0));
            if ((curlst >= 0) &&
                    (++curlst == GRpkg::self()->CurDev()->numlinestyles))
                curlst = 0;
            if (curlst == 1 && GRpkg::self()->CurDev()->numlinestyles != 2)
                // reserve linestyle 1 for grid
                curlst++;
        }
    }
}


// Call this function after viewport size changes.
//
void
sGraph::dv_resize()
{
    gr_datawin.xmin = gr_rawdata.xmin;
    gr_datawin.xmax = gr_rawdata.xmax;
    gr_datawin.ymin = gr_rawdata.ymin;
    gr_datawin.ymax = gr_rawdata.ymax;
    if (gr_field == 0)
        gr_field = FIELD_INIT;

    // shrink gr_datawin slightly to avoid numerical strangeness
    if (gr_grtype != GRID_POLAR && gr_grtype != GRID_SMITH &&
            gr_grtype != GRID_SMITHGRID) {
        if (gr_grtype != GRID_LOGLOG && gr_grtype != GRID_XLOG) {
            double scalex = 1e-3*(gr_datawin.xmax - gr_datawin.xmin);
            gr_datawin.xmin += scalex;
            gr_datawin.xmax -= scalex;
        }
        if (gr_grtype != GRID_LOGLOG && gr_grtype != GRID_YLOG) {
            double scaley = 1e-3*(gr_datawin.ymax - gr_datawin.ymin);
            gr_datawin.ymin += scaley;
            gr_datawin.ymax -= scaley;
        }
    }

    // When showing the dimension map in a multi-trace plot, change the
    // effective viewport so as to leave space on the left.  Use the
    // effleft and effwid methods instead of area.left and area.width.

    gr_dimwidth = 0;
    if (gr_sel_show && gr_selections && gr_numtraces > 1) {
        sDataVec *xs = (gr_oneval ? 0 :
            ((sDvList*)gr_plotdata)->dl_dvec->scale());
        if (xs && xs->numdims() > 1) {
            if (gr_sel_flat)
                gr_dimwidth = 5*gr_fontwid;
            else {
                int n = dimcols(xs, gr_sel_flat) + 1;
                gr_dimwidth = n*gr_fontwid;
            }
        }
    }

    gr_fixgrid();
    gr_vport.set_bottom(4*gr_fonthei + gr_area.bottom());
    if (gr_format != FT_SINGLE || gr_ysep)
        gr_vport.set_left(gr_fontwid*(gr_field+3) + effleft());
    else {
        int mlen = 0;
        sDvList *dl0 = (sDvList*)gr_plotdata;
        for (sDvList *dl = dl0; dl; dl = dl->dl_next) {
            sDataVec *v = dl->dl_dvec;
            int len = v->name() ? strlen(v->name()) : 0;
            if (len > mlen)
                mlen = len;
        }
        if (gr_pltype == PLOT_POINT)
            mlen += 2;
        if (mlen > FIELD_MAX)
            mlen = FIELD_MAX;
        else if (mlen < 4)
            mlen = 4;
        mlen += gr_scalewidth + 2;
        gr_vport.set_left(gr_fontwid*mlen + effleft());
    }
    gr_vport.set_width(gr_area.right() - 4*gr_fontwid - gr_vport.left());
    gr_vport.set_height(gr_area.top() - 4*gr_fonthei - gr_vport.bottom());
    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID) {
        // Center the plot horizontally in the viewport.
        if (gr_vport.width() > gr_vport.height())
            gr_vport.set_left(
                gr_vport.left() + (gr_vport.width() - gr_vport.height())/2);
    }
}


// If not leg_only, plot the traces and draw the legend.  If leg_only, 
// draw the legend.
//
void
sGraph::dv_trace(bool leg_only)
{
    // replot data
    // if gr_oneval, pass it a 0 scale
    // otherwise, if vec has its own scale, pass that
    //     else pass vec's plot's scale
    //
    sDataVec *scale =
        (gr_oneval ? 0 : ((sDvList*)gr_plotdata)->dl_dvec->scale());
    if (gr_format != FT_SINGLE || gr_ysep) {
        double tmpary, tmpd[2];
        tmpd[0] = gr_datawin.ymin;
        tmpd[1] = gr_datawin.ymax;
        tmpary = gr_aspect_y;
        int tmposy = 0;
        int tmpht = 0;
        int nsy = 0;
        if (gr_ysep) {
            nsy = gr_fonthei/4;
            if (nsy < 2)
                nsy = 2;
            tmposy = gr_vport.bottom();
            tmpht = gr_vport.height();
            gr_vport.set_height(tmpht/gr_numtraces - nsy);
        }
        sDvList *link;
        int tracenum;
        for (link = (sDvList*)gr_plotdata, tracenum = 0; link;
                link = link->dl_next, tracenum++) {
            if (gr_ysep) {
                gr_vport.set_bottom(tmposy +
                    (gr_numtraces - tracenum - 1)*
                    (gr_vport.height()+nsy) + nsy);
            }
            if (gr_format == FT_GROUP) {
                if (*link->dl_dvec->units() == UU_VOLTAGE) {
                    gr_datawin.ymin = gr_grpmin[0];
                    gr_datawin.ymax = gr_grpmax[0];
                }
                else if (*link->dl_dvec->units() == UU_CURRENT) {
                    gr_datawin.ymin = gr_grpmin[1];
                    gr_datawin.ymax = gr_grpmax[1];
                }
                else {
                    gr_datawin.ymin = gr_grpmin[2];
                    gr_datawin.ymax = gr_grpmax[2];
                }
            }
            else if (gr_format == FT_MULTI) {
                gr_datawin.ymin = link->dl_dvec->minsignal();
                gr_datawin.ymax = link->dl_dvec->maxsignal();
            }
            gr_aspect_y =
                (gr_datawin.ymax - gr_datawin.ymin)/gr_vport.height();
            if (leg_only)
                dv_legend(tracenum, link->dl_dvec);
            else {
                if (gr_stop)
                    break;
                dv_set_trace(link->dl_dvec, scale, tracenum);
            }
        }
        if (gr_ysep) {
            gr_vport.set_bottom(tmposy);
            gr_vport.set_height(tmpht);
        }
        gr_datawin.ymin = tmpd[0];
        gr_datawin.ymax = tmpd[1];
        gr_aspect_y = tmpary;
    }
    else {
        sDvList *link;
        int tracenum;
        for (link = (sDvList*)gr_plotdata, tracenum = 0; link;
                link = link->dl_next, tracenum++) {
            if (leg_only)
                dv_legend(tracenum, link->dl_dvec);
            else {
                if (gr_stop)
                    break;
                dv_set_trace(link->dl_dvec, scale, tracenum);
            }
        }
    }
}


void
sGraph::dv_legend(int tracenum, sDataVec *dv)
{
    if (gr_present)
        return;

    gr_dev->SetLinestyle(0);
    int yu = gr_vport.top();
    int spa;
    if (gr_format != FT_SINGLE || gr_ysep) {
        int scrx = gr_vport.left() - (gr_field+1)*gr_fontwid - gr_fontwid/2;
        int dely = 3*gr_fonthei + gr_fonthei/2;
        if (gr_ysep) {
            int nsy = gr_fonthei/4;
            if (nsy < 2)
                nsy = 2;
            if ((gr_vport.height() + nsy) < dely)
                spa = dely - (gr_vport.height() + nsy);
            else
                spa = 0;
        }
        else
            spa = dely;
        dely -= gr_fonthei/4;
        int scry = yu - tracenum*spa - gr_fonthei;
    
        gr_dev->SetColor(gr_colors[1].pixel);
        gr_save_text(dv->name(), scrx, scry, LAyval+tracenum, 1, 0);
        if (!(gr_reference.mark)) {
            double uu = gr_datawin.ymax;
            double ll = gr_datawin.ymin;
            gr_writef(uu, dv->units(), scrx, scry - gr_fonthei);
            gr_writef(ll, dv->units(), scrx, scry - 2*gr_fonthei);
        }
        if (dv->numdims() > 1 && gr_ysep)
            // dimensions use multi-colors
            gr_dev->SetColor(gr_colors[2].pixel);
        else
            gr_dev->SetColor(gr_colors[dv->color()].pixel);
        if (num_colors() == 2 && gr_pltype != PLOT_POINT && !gr_ysep)
            gr_dev->setDefaultLinestyle(dv->linestyle());
        gr_linebox(scrx - gr_fontwid/2, yu - tracenum*spa - dely + 1,
            gr_vport.left() - gr_fontwid, yu - tracenum*spa + 1);
    }
    else {
        int scrx = gr_fontwid + effleft();
        spa = 2*gr_fonthei + gr_fonthei/4;
        int scry = gr_vport.top() - 2*gr_fonthei - tracenum*spa;
        if (gr_pltype == PLOT_POINT) {
            gr_dev->SetColor(gr_colors[1].pixel);
            gr_save_text(dv->name(), scrx + 2*gr_fontwid, scry, LAyval+tracenum,
                1, 0);
            gr_dev->SetColor(gr_colors[dv->color()].pixel);
            if (PointChars) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%c ", PointChars[dv->linestyle()]);
                gr_dev->Text(buf, scrx, yinv(scry), 0);
            }
            else
                gr_dev->ShowGlyph(dv->linestyle(), scrx,
                    yinv(scry + gr_fonthei/2));
        }
        else {
            gr_dev->SetColor(gr_colors[1].pixel);
            gr_save_text(dv->name(), scrx, scry, LAyval+tracenum, 1, 0);
            gr_dev->SetColor(gr_colors[dv->color()].pixel);
            gr_dev->setDefaultLinestyle(dv->linestyle());
            int y = yinv(scry + gr_fonthei + gr_fonthei/4);
            gr_dev->Line(scrx, y, scrx + 4*gr_fontwid, y);
        }
    }
    if (GRpkg::self()->CurDev()->devtype != GRhardcopy) {
        // Draw the vertical scale translation icons.

        if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                gr_grtype == GRID_SMITHGRID)
            return;
        if (gr_format == FT_SINGLE && tracenum > 0)
            return;

        GRmultiPt p(4);
        int d = gr_fontwid;
        int x = gr_area.right() - gr_fontwid;
        int y = yu;
        if (!gr_ysep)
            y -= tracenum*spa;

        // probably an X oddity, but the 1's below are needed to
        // keep the figures symmetrical
        p.assign(0, x - d - 1, yinv(y - d - 1));
        p.assign(1, x - d/2, yinv(y));
        p.assign(2, x, yinv(y - d - 1));
        p.assign(3, x - d - 1, yinv(y - d - 1));
        gr_dev->Polygon(&p, 4);
        y -= 2*d;
        p.assign(0, x - d, yinv(y));
        p.assign(1, x - d/2, yinv(y - d));
        p.assign(2, x, yinv(y));
        p.assign(3, x - d, yinv(y));
        gr_dev->Polygon(&p, 4);
    }
}


namespace {
    void rowcol(int nblk, int *dims, int ncols)
    {
        for (int i = ncols-1; i >= 0; i--) {
            int d = dims[i];
            dims[i] = nblk % d;
            nblk /= d;
        }
    }
}


// Take care of multi-dimensional vectors here.  We know that the
// vectors have been coerced to the same data length.  The scale
// sets the block length, and we plot according to the dimensionality
// of the vector, block by block.
//
void
sGraph::dv_set_trace(sDataVec *v, sDataVec *xs, int tracenum)
{
    if (!xs || xs->numdims() <= 1) {
        // If the scale has unit dimension, ignore dimensionality
        // of vector.
        //
        dv_legend(tracenum, v);
        dv_plot_trace(v, xs, tracenum);
        return;
    }

    // Suppress interrupts until finished with trace, since we muck
    // with the length and offset.
    bool tmp_evc = gr_noevents;
    gr_noevents = true;

    int blsize = xs->dims(xs->numdims()-1);
    int nblks = 1;
    int maxd = -1;
    for (int i = 0; i < xs->numdims() - 1; i++) {
        nblks *= v->dims(i);
        if (maxd < xs->dims(i))
            maxd = xs->dims(i);
    }

    int vlen = v->length();
    int slen = xs->length();
    int len = SPMIN(vlen, slen);
    void *sdata = xs->isreal() ? (void*)xs->realvec() : (void*)xs->compvec();
    void *vdata = v->isreal() ? (void*)v->realvec() : (void*)v->compvec();;
    xs->set_length(blsize);
    v->set_length(blsize);

    // trace display map
    if (!gr_selections) {
        dv_find_selections();
        if (!gr_selections) {
            gr_selections = new char[nblks];
            gr_selsize = nblks;
            memset(gr_selections, 0xff, nblks);
        }
    }

    int x = 0;
    int y = 0;
    int numdtab = 0;
    int numcolors = num_colors() - 2;
    if (v == ((sDvList*)gr_plotdata)->dl_dvec) {
        if (gr_numtraces == 1)
            x = gr_fontwid + effleft();
        else
            x = effleft() - dimcols(xs, gr_sel_flat)*gr_fontwid;
        if (gr_numtraces == 1)
            y = gr_vport.top() - 4*gr_fonthei;
        else
            y = gr_vport.top();
        numdtab = y/gr_fonthei - 2;

        if (numdtab > 0) {
            if (gr_sel_flat)
                gr_npage = nblks/numdtab + (nblks%numdtab ? 1 : 0);
            else
                gr_npage = maxd/numdtab + (maxd%numdtab ? 1 : 0);
            if (gr_npage > 1 && gr_sel_show &&
                    v == ((sDvList*)gr_plotdata)->dl_dvec) {
                gr_dev->SetColor(gr_colors[2].pixel);
                gr_dev->Text("more", x, yinv(y - numdtab*gr_fonthei), 0);
            }
        }
        if (!gr_sel_flat && xs->numdims() > 2 && gr_numtraces == 1 &&
                numcolors > xs->dims(xs->numdims() - 2))
            numcolors = xs->dims(xs->numdims() - 2);
    }

    int size = 0;
    int lastcolor = 0;
    dv_legend(tracenum, v);

    // Prevent writing dimension map entries more than once, looks bad
    // with anti-aliased fonts.
    int rows_done[MAXDIMS];
    memset(rows_done, 0, MAXDIMS*sizeof(int));

    for (int i = 0; i < nblks; i++) {
        if (gr_stop)
            break;
        size += blsize;
        bool stop = false;
        if (size > len) {
            // unfinished block
            blsize = len - (size - blsize);
            stop = true;
            if (blsize > 0) {
                xs->set_length(blsize);
                v->set_length(blsize);
            }
            else
                break;
        }

        bool enabled[MAXDIMS];
        memset(enabled, true, MAXDIMS);
        bool show_trace = true;
        if (gr_selections && i < gr_selsize) {
            if (gr_sel_flat) {
                if (!(gr_selections[i] & 0x1))
                    show_trace = false;
            }
            else {
                if (!(gr_selections[i] & 0x1)) {
                    enabled[xs->numdims() - 2] = false;
                    show_trace = false;
                }
                int d = xs->dims(xs->numdims() - 2);
                int mask = 0x2;
                for (int k = xs->numdims() - 3; k >= 0; k--) {
                    int f = (i/d)*d;
                    if (!(gr_selections[f] & mask)) {
                        show_trace = false;
                        enabled[k] = false;
                    }
                    mask <<= 1;
                    d *= xs->dims(k);
                }
            }
        }

        // Use separate colors for the dimensions, if one trace or
        // traces separated.
        int clr = v->color();
        if (gr_numtraces == 1 || gr_ysep) {
            clr = 2;
            if (lastcolor == 0)
                lastcolor = clr;
            else
                lastcolor++;
            if (lastcolor >= numcolors + 2)
                lastcolor = 2;
            v->set_color(lastcolor);
        }
        if (show_trace)
            dv_plot_trace(v, xs, tracenum);
        if (gr_sel_show && numdtab > 0) {
            // Show dimension indices.
            char buf[32];
            if (gr_sel_flat) {
                if (i >= gr_cpage*numdtab && i < (gr_cpage+1)*numdtab) {
                    int j = i - gr_cpage*numdtab;
                    snprintf(buf, sizeof(buf), "%d", i);
                    if (!show_trace)
                        gr_dev->SetColor(gr_colors[1].pixel);
                    gr_dev->Text(buf, x, yinv(y - j*gr_fonthei), 0);
                }
            }
            else {
                int dims[MAXDIMS];
                for (int k = 0; k < xs->numdims(); k++)
                    dims[k] = xs->dims(k);
                rowcol(i, dims, xs->numdims()-1);
                int cp = 0;
                for (int k = 0; k < xs->numdims()-1; k++) {
                    int ypos = dims[k];
                    int skip = false;
                    if (gr_npage > 1 && xs->dims(k) > numdtab) {
                        if (dims[k] < gr_cpage*numdtab ||
                                dims[k] >= (gr_cpage+1)*numdtab)
                            skip = true;
                        else
                            ypos -= gr_cpage*numdtab;
                    }
                    if (!skip) {
                        snprintf(buf, sizeof(buf), "%d", dims[k]);
                        if (k == xs->numdims() - 2 &&
                                (gr_numtraces == 1 || gr_ysep)) {
                            if (enabled[k])
                                gr_dev->SetColor(gr_colors[lastcolor].pixel);
                            else
                                gr_dev->SetColor(gr_colors[1].pixel);
                        }
                        else {
                            if (enabled[k])
                                gr_dev->SetColor(gr_colors[2].pixel);
                            else
                                gr_dev->SetColor(gr_colors[1].pixel);
                        }
                        if (ypos == rows_done[k]) {
                            // Write each entry once only.

                            gr_dev->Text(buf, x + cp*gr_fontwid,
                                yinv(y - ypos*gr_fonthei), 0);
                            rows_done[k]++;
                        }
                    }
                    cp++;
                    int z = xs->dims(k);
                    while (z) {
                        z /= 10;
                        cp++;
                    }
                }
            }
            v->set_color(clr);
        }
        if (stop)
            break;
        
        if (xs->isreal())
            xs->set_realvec(xs->realvec() + blsize);
        else
            xs->set_compvec(xs->compvec() + blsize);
        if (v->isreal())
            v->set_realvec(v->realvec() + blsize);
        else
            v->set_compvec(v->compvec() + blsize);
    }
    xs->set_length(slen);
    if (xs->isreal())
        xs->set_realvec((double*)sdata);
    else
        xs->set_compvec((complex*)sdata);
    v->set_length(vlen);
    if (v->isreal())
        v->set_realvec((double*)vdata);
    else
        v->set_compvec((complex*)vdata);

    // Now safe to handle redraws, check for interupts
    gr_noevents = tmp_evc;
    if (!gr_noevents) {
        int i1 = gr_vport.bottom();
        int i2 = gr_vport.height();
        double d1 = gr_aspect_y;
        double d2 = gr_datawin.ymin;
        double d3 = gr_datawin.ymax;
        if (gr_check_plot_events()) {
            gr_stop = true;
            return;
        }
        gr_vport.set_bottom(i1);
        gr_vport.set_height(i2);
        gr_aspect_y = d1;
        gr_datawin.ymin = d2;
        gr_datawin.ymax = d3;
    }
}


namespace {
    // Here's a cache for lines, which is used to reduce the data size
    // of the plot.  When there are a lot of points in the plot,
    // rendering can be very slow.  This was a real problem in
    // Windows, at least with my hardware, until this was added.

    struct LineCache {

#define LC_SIZE 4096
        LineCache()
            {
                lc_lines = new GRmultiPt(2*LC_SIZE);
                lc_draw = 0;
                lc_size = LC_SIZE;
                lc_indx = 0;
            }

        ~LineCache()
            {
                delete lc_lines;
            }

        void init(GRdraw *d)
            {
                lc_draw = d;
                lc_indx = 0;
            }

        void add(int x1, int y1, int x2, int y2)
            {
                int ix = lc_indx + lc_indx;
                lc_lines->assign(ix, x1, y1);
                lc_lines->assign(ix + 1, x2, y2);
                lc_indx++;
                if (lc_indx == LC_SIZE)
                    flush();
            }

        void flush()
            {
                if (lc_indx) {
                    // Push the lines through the line clipper, so
                    // that overlapping parts are clipped out.  This
                    // can really cut down on the data to write when
                    // the plot is very dense.

                    GRlineDb ldb;
                    for (int i = 0; i < lc_indx; i++) {
                        int ix = i + i;
                        int x1 = lc_lines->at(ix).x;
                        int y1 = lc_lines->at(ix).y;
                        int x2 = lc_lines->at(ix+1).x;
                        int y2 = lc_lines->at(ix+1).y;
                        if (x1 == x2) {
                            const llist_t *ll = ldb.add_vert(x1, y1, y2);
                            while (ll) {
                                lc_draw->Line(x1, ll->vmin(), x1, ll->vmax());
                                ll = ll->next();
                            }
                        }
                        else if (y1 == y2) {
                            const llist_t *ll = ldb.add_horz(y1, x1, x2);
                            while (ll) {
                                lc_draw->Line(ll->vmin(), y1, ll->vmax(), y1);
                                ll = ll->next();
                            }
                        }
                        else {
                            const nmllist_t *ll = ldb.add_nm(x1, y1, x2, y2);
                            while (ll) {
                                lc_draw->Line(ll->x1(), ll->y1(), ll->x2(),
                                    ll->y2());
                                ll = ll->next();
                            }
                        }
                    }
                    // Note:  In Windows, use of PolyLine and Lines didn't
                    // really improve much over multiple calls to Line.

                    lc_indx = 0;
                }
            }

    private:
        GRmultiPt *lc_lines;
        GRdraw *lc_draw;
        int lc_size;
        int lc_indx;
    };

    LineCache LC;
}


// Plot the vector v, with scale xs.  If we are doing curve-fitting,
// then do some tricky stuff.
//
void
sGraph::dv_plot_trace(sDataVec *v, sDataVec *xs, int tracenum)
{
    if (v->length() < 1) {
        GRpkg::self()->ErrPrintf(ET_WARN,
            "trace %d contains no data, can't plot.\n", tracenum);
        return;
    }
    int deg = gr_degree;
    if (deg > v->length())
        deg = v->length();
    if (deg < 1) {
        if (tracenum == 0) {
            GRpkg::self()->ErrPrintf(ET_ERROR, "polydegree is %d, set to 1.\n",
                deg);
        }
        deg = 1;
    }

    VTvalue vv;
    int gridsize = 0;
    if (Sp.GetVar(kw_gridsize, VTYP_NUM, &vv))
        gridsize = vv.get_int();
    if ((gridsize < 0) || (gridsize > 10000)) {
        if (tracenum == 0) {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "grid size %d, out of range 0-10000, set to 0.\n", gridsize);
        }
        gridsize = 0;
    }
    if (gridsize) {
        if (gr_grtype != GRID_LIN) {
            if (tracenum == 0) {
                GRpkg::self()->ErrPrintf(ET_WARN,
                    "scale not linear, gridsize ignored.\n");
            }
            gridsize = 0;
        }
        if (xs && !gr_xmono) {
            if (tracenum == 0) {
                GRpkg::self()->ErrPrintf(ET_WARN,
                    "scale not monotonic, gridsize ignored.\n");
            }
            gridsize = 0;
        }
    }

    // Do the one value case
    if (!xs) {
        for (int i = 0; i < v->length(); i++) {

            if (gr_stop)
                break;

            // We should do the one - point case too!
            //    Important for polar plots of pole-zero for example
            //
            int j;
            if (v->length() == 1)
                j = 0;
            else {
                j = i-1;
                if (i == 0)
                    continue;
            }
            if ((v->flags() & (VF_POLE | VF_ZERO)) &&
                    (gr_grtype ==  GRID_LIN ||
                    gr_grtype == GRID_LOGLOG || gr_grtype == GRID_XLOG ||
                    gr_grtype == GRID_YLOG))
                dv_point(v, v->imagval(i), v->realval(i), v->imagval(j),
                    v->realval(j), (j==i ? 1 : i));
            else
                dv_point(v, v->realval(i), v->imagval(i), v->realval(j),
                    v->imagval(j), (j==i ? 1 : i));
        }
        gr_dev->Update();
        return;
    }

    // Init the line cache, this is used for linear scale plots only.
    LC.init(gr_dev);

    // First check the simple case, where we don't have to do any 
    // interpolation.
    //
    if (deg == 1 && gridsize == 0) {
        if (v->length() == 1 && gr_pltype == PLOT_LIN) {
            // Use a glyph.  A single pixel would be shown otherwise,
            // which can't be seen.

            double lx = xs->realval(0);
            double ly = v->realval(0);
            int tox, toy;
            gr_data_to_screen(lx, ly, &tox, &toy);
            gr_dev->SetColor(gr_colors[v->color()].pixel);
            if (PointChars)
                gr_dev->Text("x", tox - gr_fontwid/2, yinv(toy - gr_fonthei/2),
                    0);
            else
                gr_dev->ShowGlyph(1, tox, yinv(toy));
        }
        else {
            double lx = xs->realval(0);
            double ly = v->realval(0);
            double dx = lx;
            double dy = ly;
            for (int i = 0, j = v->length(); i < j; i++) {
                if (gr_stop)
                    return;
                dx = xs->realval(i);
                dy = v->realval(i);
                dv_point(v, dx, dy, lx, ly, i);
                lx = dx;
                ly = dy;
            }
            if (v->length() == 1)
                dv_point(v, dx, dy, lx, ly, 1);
        }
        LC.flush();
        gr_dev->Update();
        return;
    }

    if (gridsize < deg + 1)
        gridsize = 0;

    if (gridsize) {
        // This is done quite differently from what we do below...
        double *gridbuf = new double[gridsize];
        double *result = new double[gridsize];
        double *ydata;
        if (v->isreal())
            ydata = v->realvec();
        else {
            ydata = new double[v->length()];
            for (int i = 0; i < v->length(); i++)
                ydata[i] = v->realval(i);
        }
        double *xdata;
        if (xs->isreal())
            xdata = xs->realvec();
        else {
            xdata = new double[xs->length()];
            for (int i = 0; i < xs->length(); i++)
                xdata[i] = xs->realval(i);
        }
        
        double mm[2];
        xs->minmax(mm, true);
        double dx = (mm[1] - mm[0])/gridsize;
        double dy;
        int i;
        if (xdata[1] > xdata[0])
            for (i = 0, dy = mm[0]; i < gridsize; i++, dy += dx)
                gridbuf[i] = dy;
        else
            for (i = 0, dy = mm[1]; i < gridsize; i++, dy -= dx)
                gridbuf[i] = dy;

        sPoly po(deg);
        if (!po.interp(ydata, result, xdata, v->length(), gridbuf, gridsize)) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "can't put %s on gridsize %d.\n", v->name(), gridsize);
            delete [] gridbuf;
            delete [] result;
            if (!v->isreal())
                delete [] ydata;
            if (!xs->isreal())
                delete [] xdata;
            return;
        }

        // Now this is a problem.  There's no way that we can
        // figure out where to put the tic marks to correspond with
        // the actual data...
        //
        for (i = 0; i < gridsize; i++) {
            if (gr_stop)
                break;
            dv_point(v, gridbuf[i], result[i], gridbuf[i ? (i - 1) : i],
                result[i ? (i - 1) : i], -1);
        }
        delete [] gridbuf;
        delete [] result;
        if (!v->isreal())
            delete [] ydata;
        if (!xs->isreal())
            delete [] xdata;
        LC.flush();
        gr_dev->Update();
        return;
    }

    // We need to do curve fitting now. First get some scratch 
    // space.
    //
    sPoly po(deg);
    double *result = new double[deg + 1];
    double *xdata = new double[deg + 1];
    double *ydata = new double[deg + 1];

    // Plot the first degree segments...
    int i;
    for (i = 0; i <= deg; i++)
        ydata[i] = v->realval(i);
    for (i = 0; i <= deg; i++)
        xdata[i] = xs->realval(i);

    bool rot = false;
    while (!po.polyfit(xdata, ydata, result)) {
        // Rotate the coordinate system 90 degrees and try again.
        // If it doesn't work this time, bump the interpolation
        // degree down by one...
        //
        if (po.polyfit(ydata, xdata, result)) {
            rot = true;
            break;
        }
        if (po.dec_degree() == 0) {
            GRpkg::self()->ErrPrintf(ET_INTERR, "plotcurve: #1.\n");
            delete [] xdata;
            delete [] ydata;
            delete [] result;
            return;
        }
    }

    // Plot this part of the curve...
    for (i = 0; i < po.degree(); i++) {
        if (gr_stop)
            break;
        if (rot)
            dv_plot_interval(v, &ydata[i], result, &po, true);
        else
            dv_plot_interval(v, &xdata[i], result, &po, false);
    }

    // Now plot the rest, piece by piece... l is the 
    // last element under consideration.
    //
    int length = v->length();
    for (int l = po.degree() + 1; l < length; l++) {

        if (gr_stop)
            break;

        // Shift the old stuff by one and get another value
        for (i = 0; i < po.degree(); i++) {
            xdata[i] = xdata[i + 1];
            ydata[i] = ydata[i + 1];
        }
        ydata[i] = v->realval(l);
        xdata[i] = xs->realval(l);

        rot = false;
        while (!po.polyfit(xdata, ydata, result)) {
            if (po.polyfit(ydata, xdata, result)) {
                rot = true;
                break;
            }
            if (po.dec_degree() == 0) {
                GRpkg::self()->ErrPrintf(ET_INTERR, "plotcurve: #2.\n");
                delete [] xdata;
                delete [] ydata;
                delete [] result;
                return;
            }
        }
        if (rot)
            dv_plot_interval(v, &ydata[po.degree()-1], result, &po, true);
        else
            dv_plot_interval(v, &xdata[po.degree()-1], result, &po, false);
    }
    delete [] xdata;
    delete [] ydata;
    delete [] result;
    LC.flush();
    gr_dev->Update();
}


void
sGraph::dv_plot_interval(sDataVec *v, double *lohi, double *coeffs,
    sPoly *po, bool rotated)
{
    int steps = DEF_polysteps;
    VTvalue vv;
    if (Sp.GetVar(kw_polysteps, VTYP_NUM, &vv))
        steps = vv.get_int();

    double lo = *lohi;
    double hi = *(lohi + 1);
    double incr = (hi - lo) / (double) (steps + 1);
    double dx = lo + incr;
    double lx = lo;
    double ly = po->peval(lo, coeffs);
    for (int i = 0; i <= steps; i++, dx += incr) {
        double dy = po->peval(dx, coeffs);
        if (rotated)
            dv_point(v, dy, dx, ly, lx, -1);
        else
            dv_point(v, dx, dy, lx, ly, -1);
        lx = dx;
        ly = dy;
        if (gr_stop)
            return;
    }
}


// Add a point to the curve we're currently drawing.  Arg np is the
// point index, really only used for the ticmarks, pass -1 to suppress
// ticmarks.  The first call should have old = new when two points are
// needed.
//
void
sGraph::dv_point(sDataVec *dv, double newx, double newy,
    double oldx, double oldy, int np)
{
    if (!gr_dev)
        return;

    // Look for button presses.
    static unsigned int check_count;
    if (!gr_noevents && check_count && !(check_count & 0x3f)) {
        check_count++;
        int i1 = gr_vport.bottom();
        int i2 = gr_vport.height();
        double d1 = gr_aspect_y;
        double d2 = gr_datawin.ymin;
        double d3 = gr_datawin.ymax;
        if (gr_check_plot_events()) {
            gr_stop = true;
            return;
        }
        gr_vport.set_bottom(i1);
        gr_vport.set_height(i2);
        gr_aspect_y = d1;
        gr_datawin.ymin = d2;
        gr_datawin.ymax = d3;
    }

    double ff = (gr_rawdata.xmax - gr_rawdata.xmin)*1e-9;
    if (newx < gr_rawdata.xmin - ff || newx > gr_rawdata.xmax + ff)
        return;
    if (gr_pltype == PLOT_POINT || gr_pltype == PLOT_COMB) {
        // Only care about the current point.
        oldx = newx;
        oldy = newy;
    }
    else {
        if (oldx == newx && oldy == newy)
            return;
        if (oldx < gr_rawdata.xmin - ff || oldx > gr_rawdata.xmax + ff)
            // Need two good points.
            return;
    }

    int fromx, fromy;
    gr_data_to_screen(oldx, oldy, &fromx, &fromy);
    int tox, toy;
    gr_data_to_screen(newx, newy, &tox, &toy);

    // note: we do not particularly want to clip here
    int oldtox = tox;
    int oldtoy = toy;
    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID) {
        if (clip_to_circle(&fromx, &fromy, &tox, &toy, 
            gr_xaxis.circular.center, 
            gr_yaxis.circular.center, 
            gr_xaxis.circular.radius))
            return;
    }
    else {
        if (clip_line(&fromx, &fromy, &tox, &toy, 
                gr_vport.left(), gr_vport.bottom(), 
                gr_vport.right(), gr_vport.top()))
            return;
    }

    if (gr_pltype != PLOT_POINT)
        gr_dev->setDefaultLinestyle(dv->linestyle());
    else {
        // if PLOT_POINT, 
        // don't want to plot an endpoint which have been clipped
        if (tox != oldtox || toy != oldtoy)
            return;
    }
    gr_dev->SetColor(gr_colors[dv->color()].pixel);

    if (gr_pltype == PLOT_LIN) {
        if (gr_nolinecache)
            gr_dev->Line(fromx, yinv(fromy), tox, yinv(toy));
        else
            LC.add(fromx, yinv(fromy), tox, yinv(toy));
        if (gr_ticmarks > 0 && np > -1 && (np % gr_ticmarks == 0)) {
            // Draw an 'x'
            if (PointChars)
                gr_dev->Text("x", tox - gr_fontwid/2, yinv(toy - gr_fonthei/2),
                    0);
            else
                gr_dev->ShowGlyph(1, tox, yinv(toy));
        }
    }
    else if (gr_pltype == PLOT_COMB) {
        if (gr_grtype == GRID_POLAR ||
                gr_grtype == GRID_SMITH || gr_grtype == GRID_SMITHGRID) {
            int xm, ym;
            gr_data_to_screen(0.0, 0.0, &xm, &ym);
            if (!clip_to_circle(&xm, &ym, &tox, &toy, 
                    gr_xaxis.circular.center, gr_yaxis.circular.center, 
                    gr_xaxis.circular.radius))
                gr_dev->Line(xm, yinv(ym), tox, yinv(toy));
        }
        else {
            int ymin, dummy;
            gr_data_to_screen(0.0, gr_datawin.ymin, &dummy, &ymin);
            if (gr_nolinecache)
                gr_dev->Line(tox, yinv(ymin), tox, yinv(toy));
            else
                LC.add(tox, yinv(ymin), tox, yinv(toy));
        }
    }
    else if (gr_pltype == PLOT_POINT) {
        // Here, linestyle() is the character index used for the point.
        if (PointChars) {
            char pointc[2];
            pointc[0] = PointChars[dv->linestyle()];
            pointc[1] = '\0';
            gr_dev->Text(pointc, tox - gr_fontwid/2, yinv(toy - gr_fonthei/2),
                0);
        }
        else
            gr_dev->ShowGlyph(dv->linestyle(), tox, yinv(toy));
    }
}


void
sGraph::dv_erase_factors()
{
    gr_dev->SetColor(gr_colors[0].pixel);
    int yu = gr_vport.top();

    if (gr_format != FT_SINGLE || gr_ysep) {
        int scrx = gr_vport.left() - (gr_field+1)*gr_fontwid - gr_fontwid/2;
        int dely = 3*gr_fonthei + gr_fonthei/2;
        int spa;
        if (gr_ysep) {
            spa = gr_vport.height()/gr_numtraces;
            if (spa < dely)
                spa = dely;
        }
        else
            spa = dely;
        int fw = gr_field*gr_fontwid;
        for (int i = 0; i < gr_numtraces; i++) {
            gr_dev->Box(scrx, yinv(yu - i*spa - 3*gr_fonthei), 
                scrx + fw, yinv(yu - i*spa - gr_fonthei));
        }
    }
    else {
        int fw = VALBOX_WIDTH*gr_fontwid;
        int scrx = 4*gr_fontwid + effleft();
        if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
                gr_grtype == GRID_SMITHGRID) {
            yu -= gr_fonthei/2;
            gr_dev->Box(scrx, yinv(yu), scrx + fw, yinv(yu + gr_fonthei));
            yu = gr_vport.bottom();
            gr_dev->Box(scrx, yinv(yu), scrx + fw, yinv(yu + gr_fonthei));
            yu -= gr_fonthei;
            gr_dev->Box(scrx, yinv(yu), scrx + fw, yinv(yu + gr_fonthei));
        }
        else {
            yu += 2*gr_fonthei + gr_fonthei/2;;
            int spa = fw;
            int nshow = (gr_vport.width() - gr_fontwid)/spa;
            for (int i = 0; i < gr_numtraces && i < nshow; i++) {
                gr_dev->Box(scrx, yinv(yu), scrx + fw, yinv(yu + gr_fonthei));
                scrx += spa;
            }
        }
    }
    int fw = VALBOX_WIDTH*gr_fontwid;
    if (gr_grtype == GRID_POLAR || gr_grtype == GRID_SMITH ||
            gr_grtype == GRID_SMITHGRID)
        gr_dev->Box(gr_vport.right() - fw, 
            yinv(gr_vport.bottom() - 3*gr_fonthei), 
            gr_vport.right(), yinv(gr_vport.bottom() - 2*gr_fonthei));
    else
        gr_dev->Box(gr_vport.left(), 
            yinv(gr_vport.bottom() - 3*gr_fonthei), 
            gr_vport.left() + fw, yinv(gr_vport.bottom() - 2*gr_fonthei));
}


bool
sGraph::dv_find_where(sDataVec *scale, int x, double *fx, int *indx)
{
    double tx;
    if (gr_grtype == GRID_LOGLOG || gr_grtype == GRID_XLOG) {
        tx =  (x - gr_vport.left())*(log10(gr_datawin.xmax) -
            log10(gr_datawin.xmin))/gr_vport.width() + log10(gr_datawin.xmin);
        tx = pow(10.0, tx);
    }
    else
        tx = (x - gr_vport.left())*gr_aspect_x + gr_datawin.xmin;
    int i;
    for (i = 1; i < scale->length(); i++) {
        if ((scale->realval(i-1) <= tx && scale->realval(i) >= tx) ||
                (scale->realval(i-1) >= tx && scale->realval(i) <= tx))
            break;
    }
    if (i == scale->length())
        return (false);
    *indx = i;
    *fx = tx;
    return (true);
}


void
sGraph::dv_find_y(sDataVec *scale, sDataVec *dv, int indx, double fx,
    double *fy)
{
    double x0 = scale->realval(indx-1);
    double x1 = scale->realval(indx);
    double y0 = dv->realval(indx-1);
    double y1 = dv->realval(indx);
    if (x0 == x1)
        *fy = y0;
    else {
        if (gr_grtype == GRID_LOGLOG || gr_grtype == GRID_XLOG) {
            fx = log10(fx);
            x0 = log10(x0);
            x1 = log10(x1);
        }
        *fy = y0 + (fx - x0)*(y1 - y0)/(x1 - x0);
    }
}


// Switch the current plotting environment by setting temporary shell
// variables.  This is used during zoomin.
//
void
sGraph::dv_pl_environ(double x0, double x1, double y0, double y1, bool unset)
{
    char buf[BSIZE_SP];
    const char *s = "_temp_xlimit";
    if (unset)
        Sp.RemVar(s);
    else {
        snprintf(buf, sizeof(buf), "%.12e, %.12e", x0, x1);
        Sp.SetVar(s, buf);
    }

    s = "_temp_ylimit";
    if (unset)
        Sp.RemVar(s);
    else {
        snprintf(buf, sizeof(buf), "%.12e, %.12e", y0, y1);
        Sp.SetVar(s, buf);
    }

    s = 0;
    switch (gr_format) {
    case FT_SINGLE:
        s = "_temp_single";
        break;
    case FT_GROUP:
        s = "_temp_group";
        break;
    case FT_MULTI:
        s = "_temp_multi";
        break;
    }
    if (s) {
        if (unset)
            Sp.RemVar(s);
        else
            Sp.SetVar(s);
    }

    s = 0;
    switch (gr_pltype) {
    case PLOT_LIN:
        s = "_temp_linplot";
        break;
    case PLOT_COMB:
        s = "_temp_combplot";
        break;
    case PLOT_POINT:
        s = "_temp_pointplot";
        break;
    }
    if (s) {
        if (unset)
            Sp.RemVar(s);
        else
            Sp.SetVar(s);
    }

    s = 0;
    switch (gr_grtype) {
    case GRID_LIN:
        s = "_temp_lingrid";
        break;
    case GRID_LOGLOG:
        s = "_temp_loglog";
        break;
    case GRID_XLOG:
        s = "_temp_xlog";
        break;
    case GRID_YLOG:
        s = "_temp_ylog";
        break;
    case GRID_POLAR:
        s = "_temp_polar";
        break;
    case GRID_SMITH:
        s = "_temp_smith";
        break;
    case GRID_SMITHGRID:
        s = "_temp_smithgrid";
        break;
    }
    if (s) {
        if (unset)
            Sp.RemVar(s);
        else
            Sp.SetVar(s);
    }

    if (gr_ysep) {
        s = "_temp_ysep";
        if (unset)
            Sp.RemVar(s);
        else
            Sp.SetVar(s);
    }

    if (gr_noplotlogo) {
        s = "_temp_noplotlogo";
        if (unset)
            Sp.RemVar(s);
        else
            Sp.SetVar(s);
    }

    if (gr_nogrid) {
        s = "_temp_nogrid";
        if (unset)
            Sp.RemVar(s);
        else
            Sp.SetVar(s);
    }

    if (gr_present) {
        s = "_temp_present";
        if (unset)
            Sp.RemVar(s);
        else
            Sp.SetVar(s);
    }

}


// Look for an mplot window with selections, if a matching one is found
// create a new selections list.
//
void
sGraph::dv_find_selections()
{
    clear_selections();
    char *tpname = lstring::copy(gr_plotname);
    if (!tpname)
        return;
    char *t = strchr(tpname, ':');
    if (t)
        *t = 0;
    sGgen *g = GP.InitGgen();
    sGraph *graph;
    while ((graph = g->next()) != 0) {
        if (graph->gr_apptype != GR_MPLT)
            continue;
        sChkPts *p = static_cast<sChkPts*>(graph->gr_plotdata);
        if (p->plotname && tpname &&
                lstring::cieq(p->plotname, tpname) && p->sel) {
            int i;
            for (i = 0; i < p->size; i++) {
                if (p->sel[i])
                    break;
            }
            if (i < p->size) {
                gr_selections = new char[p->size];
                memcpy(gr_selections, p->sel, p->size);
                gr_selsize = p->size;
                gr_sel_flat = true;
            }
            delete g;
            break;
        }
    }
    delete [] tpname;
}


#if (defined (WITH_GTK2) || defined (WITH_GTK3))
namespace {
    // Show an outline box of the selected text
    //
    void ghost_tbox(int x, int y, int, int, bool)
    {
        GP.Cur()->gr_ghost_tbox(x, y);
    }


    void ghost_trace(int x, int y, int, int, bool)
    {
        GP.Cur()->gr_ghost_trace(x, y);
    }
}
#endif


int
sGraph::start_drag_trace(void *arg)
{
#if (defined (WITH_QT5) || defined (WITH_QT6))
    // In QT, start in QT's drag mode.
    sGraph *graph = (sGraph*)arg;
    graph->gr_timer_id = 0;
    sGraph::drag_trace(graph);
#else
#if (defined (WITH_GTK2) || defined (WITH_GTK3))
    // In GTK, use the local drag mode, which works between windows,
    // unlike this mode in QT.  For QT6, it is also possible to start
    // in local mode and switch to QT mode in the leave event handler,
    // but this doesn't work in QT5 under Linux.
    sGraph *graph = (sGraph*)arg;
    GP.SetSourceGraph(graph);
    if (graph->gr_cmd_data) {
        graph->gr_set_ghost(ghost_trace, 0, 0);
        graph->gr_show_ghost(true);
    }
#else
    (void)arg;
#endif
#endif
    return (false);
}


int
sGraph::start_drag_text(void *arg)
{
    sGraph *graph = (sGraph*)arg;
#if (defined (WITH_QT5) || defined (WITH_QT6))
    // In QT, start in QT's drag mode.
    graph->gr_timer_id = 0;
    sGraph::drag_text(graph);
    graph->gr_cmdmode &= ~grMoving;
#else
#if (defined (WITH_GTK2) || defined (WITH_GTK3))
    // In GTK, use the local drag mode, which works between windows,
    // unlike this mode in QT.  For QT6, it is also possible to start
    // in local mode and switch to QT mode in the leave event handler,
    // but this doesn't work in QT5 under Linux.
    graph->gr_set_ghost(ghost_tbox, 0, 0);
    GP.PushGraphContext(graph);
    graph->gr_dev->DrawGhost();
    GP.PopGraphContext();
    graph->gr_cmdmode |= grMoving;
#endif
#endif
    graph->gr_timer_id = 0;
    return (false);
}


// Static function.
int
sGraph::start_drag_zoom(void *arg)
{
    sGraph *graph = (sGraph*)arg;
    if (graph->gr_reference.mark)
        graph->gr_show_ghost(false);
    graph->gr_zoomin(graph->gr_pressx, graph->gr_pressy);
    if (graph->gr_reference.mark)
        graph->gr_show_ghost(true);
    graph->gr_timer_id = 0;
    return (false);
}
// End of sGraph private functions.


//
// The graphics-package dependent part of the sGraph class.
//

// Below are stubs needed to link without a graphics package.  The
// functions below are gemerally implemented in graphics code.

#if (!defined(WITH_QT5) && !defined(WITH_QT6) && \
    !defined(WITH_GTK2) && !defined(WITH_GTK3))


// Return a new graphics context struct.
//
GRwbag *
sGraph::gr_new_gx(int type)
{
    return (0);
}


// Initialization of graphics, return false on success.
//
int
sGraph::gr_pkg_init()
{
    return (true);
}


// Fill in the current colors in the graph from the DefColors array,
// which is updated.
//
void
sGraph::gr_pkg_init_colors()
{
}


// This rebuilds the button array after adding/removing a trace.  The
// return is true if the button count changes
//
bool
sGraph::gr_init_btns()
{
    return (false);
}


// This is called periodically while a plot is being drawn.  If a button
// press or exposure event is detected for the plot window, return true.
//
bool
sGraph::gr_check_plot_events()
{
    return (false);
}


void
sGraph::gr_redraw()
{
    if (GRpkg::self()->CurDev()->devtype == GRhardcopy) {
        gr_redraw_direct();
        gr_redraw_keyed();
    }
}


void
sGraph::gr_refresh(int left, int bottom, int right, int top, bool notxt)
{
}


// Pop down and destroy.
//
void
sGraph::gr_popdown()
{
}


// Static function
// Timeout function, start dragging a trace.
//
int
sGraph::drag_trace(void *arg)
{
    return (0);
}


// Static function
// Timeout function, start dragging text.
//
int
sGraph::drag_text(void *arg)
{
    return (0);
}

#endif

