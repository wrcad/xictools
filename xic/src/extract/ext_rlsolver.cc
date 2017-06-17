
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
 $Id: ext_rlsolver.cc,v 5.53 2017/03/14 01:26:38 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_rlsolver.h"
#include "ext_pathfinder.h"
#include "ext_errlog.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_inlines.h"
#include "geo_zgroup.h"
#include "geo_ylist.h"
#include "si_spt.h"
#include "tech.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "timer.h"

#define XIC
#include "spmatrix.h"

#include <math.h>

#ifndef M_PI
#define M_PI 3.1415926
#endif
#ifndef HAVE_ACOSH
#define atanh(x) log(((x)+1.0)/(1.0-(x)))/2.0
#endif

#define RL_PIVREL 1e-2
#define RL_PIVABS 1e-9

// If this is set, the delta will always use this value.
int RLsolver::rl_given_delta = 0;

// When not tiling, the algorithm uses a grid spacing such that the
// resistor area equals rl_numgrid grid elements.
int RLsolver::rl_numgrid = RLS_DEF_NUM_GRID;

// When tiling, keep the number of grids below this value.
int RLsolver::rl_maxgrid = RLS_DEF_MAX_GRID;

// If set, attempt to set the grid by tiling (which gives the most
// acurate results, but can be slow).
bool RLsolver::rl_try_tile = false;


namespace {
    unsigned long check_time;

    // Export to sparse package.
    //
    int check_for_interrupt()
    {
        if (Timer()->check_interval(check_time)) {
            if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wdraw())
                dspPkgIf()->CheckForInterrupt();
            return (XM()->ConfirmAbort());
        }
        return (false);
    }
}


//-------------------------------------------------------------------------
// RLsolver: solve for resistance/inductance on a single layer

namespace {
    // Return an edge list from the poly list Manhattan edges, vertical or
    // horizontal only.
    //
    RLedge *
    to_edges(PolyList *pl, bool vert)
    {
        RLedge *e0 = 0;
        while (pl) {
            Point *pts = pl->po.points;
            for (int i = 0; i < pl->po.numpts - 1; i++) {
                if (vert) {
                    if (pts[i].x == pts[i+1].x && pts[i].y != pts[i+1].y)
                        e0 = new RLedge(pts[i].x, pts[i].y, pts[i+1].y, e0);
                }
                else {
                    if (pts[i].y == pts[i+1].y && pts[i].x != pts[i+1].x)
                        e0 = new RLedge(pts[i].y, pts[i].x, pts[i+1].x, e0);
                }
            }
            pl = pl->next;
        }
        return (e0);
    }
}


RLsolver::~RLsolver()
{
    delete rl_matrix;
    delete [] rl_contacts;
    rl_zlist->free();
    rl_h_edges->free();
    rl_v_edges->free();
}


// Set up the matrix and geometrical parameters.
//
bool
RLsolver::setup(const Zlist *zb, CDl *ld, Zgroup *zg)
{
    rl_zlist = zb->copy();
    if (rl_zlist)
        rl_zlist->BB(rl_BB);
    rl_ld = ld;

    rl_num_contacts = zg->num;
    rl_contacts = new RLcontact[rl_num_contacts];
    rl_offset = rl_num_contacts > 2 ? 1 : 0;

    for (int i = 0; i < rl_num_contacts; i++) {
        rl_contacts[i].czl = zg->list[i]->copy();
        XIrt ret = Zlist::zl_and(&rl_contacts[i].czl, rl_zlist->copy());
        if (ret == XIintr) {
            Errs()->add_error("user interrupt");
            return (false);
        }
        if (ret == XIbad) {
            Errs()->add_error("RLsolver::setup: clipping error.");
            return (false);
        }
        if (!rl_contacts[i].czl) {
            Errs()->add_error(
                "RLsolver::setup: contact %d does not intersect body.", i);
            return (false);
        }
        rl_contacts[i].czl->BB(rl_contacts[i].cBB);
    }

    set_delta();
    rl_nx = (rl_BB.width() + rl_delta/2)/rl_delta;
    rl_ny = (rl_BB.height() + rl_delta/2)/rl_delta;

    rl_matrix = new spMatrixFrame(0, 0);
    int error = rl_matrix->spError();
    if (error) {
        Errs()->add_error(rl_matrix->spErrorMessage(error));
        return (false);
    }
    rl_matrix->spSetInterruptCheckFunc(&check_for_interrupt);
    return (true);
}


// Build the matrix, for resistance computation.
//
bool
RLsolver::setupR()
{
    if (!rl_zlist) {
        Errs()->add_error("setupR: body trapezoid list is empty.");
        return (false);
    }
    if (rl_num_contacts < 2) {
        Errs()->add_error("setupR: fewer than two contacts specified.");
        return (false);
    }
    char buf[64];
    sprintf(buf, "%s-thickness", rl_ld->name());
    spt_t *thick_mods = spt_t::findSpatialParameterTable(buf);

    FILE *fp = 0;
    if (ExtErrLog.log_rlsolver()) {
        rl_zlist->show();
        fp = ExtErrLog.rlsolver_log_fp();
        fprintf(fp, "nx=%d ny=%d del=%d\n", rl_nx, rl_ny, rl_delta);
        fprintf(fp, "BB=(%d,%d  %d,%d)\n", rl_BB.left, rl_BB.bottom,
            rl_BB.right, rl_BB.top);
    }

    int ncnt = rl_num_contacts + rl_offset;
    int *rr = new int[rl_nx];
    int *rn = new int[rl_nx];
    for (int i = rl_ny - 1; i >= 0; i--) {
        if (i == rl_ny - 1) {
            for (int j = 0; j < rl_nx; j++) {
                int n = nodeof(j, i);
                if (n < rl_num_contacts + rl_offset)
                    rr[j] = n;
                else
                    rr[j] = ncnt++;
            }
        }
        else {
            int *t = rr;
            rr = rn;
            rn = t;
        }
        if (i > 0) {
            for (int j = 0; j < rl_nx; j++) {
                int n = nodeof(j, i-1);
                if (n < rl_num_contacts + rl_offset)
                    rn[j] = n;
                else
                    rn[j] = ncnt++;
            }
        }
        for (int j = 0; j < rl_nx; j++) {
            int n1 = rr[j];
            if (n1 >= 0) {
                if (fp) {
                    if (n1 < rl_num_contacts + rl_offset)
                        fprintf(fp, "%d", n1);
                    else
                        fprintf(fp, ".");
                }
                if (j+1 < rl_nx) {
                    int n2 = rr[j+1];
                    if (n2 >= 0 && n1 != n2) {

                        int x1 = rl_BB.left + j*rl_delta + rl_delta/2;
                        int x2 = x1 + rl_delta;
                        int y = rl_BB.bottom + i*rl_delta + rl_delta/2;
                        int xc = (x1 + x2)/2;
                        int t = y + rl_delta/2;
                        int b = y - rl_delta/2;

                        double val = 1.0;
                        if (thick_mods) {
                            double tval = thick_mods->retrieve_item(xc, y);
                            if (tval != 0.0) {
                                tval /= dsp_prm(rl_ld)->thickness();
                                val = tval;
                            }
                        }

                        for (RLedge *e = rl_h_edges; e; e = e->next) {
                            if (e->p < y + rl_delta && e->p >= y) {
                                if (e->e2 > x1 && e->e1 < x2) {
                                    int mx = mmMin(x2, e->e2);
                                    int mn = mmMax(x1, e->e1);
                                    double a = (mx - mn)/(double)rl_delta;
                                    t = (int)(a*e->p + (1.0-a)*t);
                                }
//                                if (e->e1 <= xc && xc <= e->e2)
//                                    t = e->p;
                            }
                            else if (e->p > y - rl_delta && e->p <= y) {
                                if (e->e2 > x1 && e->e1 < x2) {
                                    int mx = mmMin(x2, e->e2);
                                    int mn = mmMax(x1, e->e1);
                                    double a = (mx - mn)/(double)rl_delta;
                                    b = (int)(a*e->p + (1.0-a)*b);
                                }
//                                if (e->e1 <= xc && xc <= e->e2)
//                                    b = e->p;
                            }
                        }
                        val *= (t - b)/(double)rl_delta;

                        if (n1 < rl_num_contacts + rl_offset) {
                            // n1 is in contact
                            int l = xc;
                            for (int k = 0; k < rl_num_contacts; k++) {
                                for (RLedge *e = rl_contacts[k].v_edges; e;
                                        e = e->next) {
                                    if (e->p >= x1 && e->p < x2) {
                                        if (e->e2 >= y && e->e1 <= y)
                                            l = e->p;
                                    }
                                }
                            }
                            val *= rl_delta/(double)(x2 - l);
                        }
                        else if (n2 < rl_num_contacts + rl_offset) {
                            // n2 is in contact
                            int r = xc;
                            for (int k = 0; k < rl_num_contacts; k++) {
                                for (RLedge *e = rl_contacts[k].v_edges; e;
                                        e = e->next) {
                                    if (e->p > x1 && e->p <= x2) {
                                        if (e->e2 >= y && e->e1 <= y)
                                            r = e->p;
                                    }
                                }
                            }
                            val *= rl_delta/(double)(r - x1);
                        }
                        add_element(n1, n2, val);
                    }
                }
                if (i > 0) {
                    int n2 = rn[j];
                    if (n2 >= 0 && n1 != n2) {
                        int y2 = rl_BB.bottom + i*rl_delta + rl_delta/2;
                        int y1 = y2 - rl_delta;
                        int x = rl_BB.left + j*rl_delta + rl_delta/2;
                        int yc = (y1 + y2)/2;
                        int r = x + rl_delta/2;
                        int l = x - rl_delta/2;

                        double val = 1.0;
                        if (thick_mods) {
                            double tval = thick_mods->retrieve_item(x, yc);
                            if (tval != 0.0) {
                                tval /= dsp_prm(rl_ld)->thickness();
                                val = tval;
                            }
                        }

                        for (RLedge *e = rl_v_edges; e; e = e->next) {
                            if (e->p < x + rl_delta && e->p >= x) {
                                if (e->e2 > y1 && e->e1 < y2) {
                                    int mx = mmMin(y2, e->e2);
                                    int mn = mmMax(y1, e->e1);
                                    double a = (mx - mn)/(double)rl_delta;
                                    r = (int)(a*e->p + (1.0-a)*r);
                                }
//                                if (e->e1 <= yc && yc <= e->e2)
//                                    r = e->p;
                            }
                            else if (e->p > x - rl_delta && e->p <= x) {
                                if (e->e2 > y1 && e->e1 < y2) {
                                    int mx = mmMin(y2, e->e2);
                                    int mn = mmMax(y1, e->e1);
                                    double a = (mx - mn)/(double)rl_delta;
                                    l = (int)(a*e->p + (1.0-a)*l);
                                }
//                                if (e->e1 <= yc && yc <= e->e2)
//                                    l = e->p;
                            }
                        }
                        val *= (r - l)/(double)rl_delta;

                        if (n1 < rl_num_contacts + rl_offset) {
                            // n1 is in contact
                            int b = yc;
                            for (int k = 0; k < rl_num_contacts; k++) {
                                for (RLedge *e = rl_contacts[k].h_edges; e;
                                        e = e->next) {
                                    if (e->p >= y1 && e->p < y2) {
                                        if (e->e2 >= x && e->e1 <= x)
                                            b = e->p;
                                    }
                                }
                            }
                            val *= rl_delta/(double)(y2 - b);
                        }
                        else if (n2 < rl_num_contacts + rl_offset) {
                            // n2 is in contact
                            int t = yc;
                            for (int k = 0; k < rl_num_contacts; k++) {
                                for (RLedge *e = rl_contacts[k].h_edges; e;
                                        e = e->next) {
                                    if (e->p > y1 && e->p <= y2) {
                                        if (e->e2 >= x && e->e1 <= x)
                                            t = e->p;
                                    }
                                }
                            }
                            val *= rl_delta/(double)(t - y1);
                        }
                        add_element(n1, n2, val);
                    }
                }
            }
            else if (fp)
                fprintf(fp, " ");
        }
        if (fp)
            fprintf(fp, "\n");
    }
    delete [] rn;
    delete [] rr;

    if (fp)
        fflush(fp);

    int error = rl_matrix->spError();
    if (error) {
        Errs()->add_error(rl_matrix->spErrorMessage(error));
        return (false);
    }
    rl_state = RLresist;
    return (true);
}


// Build the matrix, for inductance computation.
//
bool
RLsolver::setupL()
{
    if (!rl_zlist) {
        Errs()->add_error("setupL: body trapezoid list is empty.");
        return (false);
    }
    if (rl_num_contacts < 2) {
        Errs()->add_error("setupL: fewer than two contacts specified.");
        return (false);
    }

    FILE *fp = 0;
    if (ExtErrLog.log_rlsolver()) {
        rl_zlist->show();
        fp = ExtErrLog.rlsolver_log_fp();
        fprintf(fp, "nx=%d ny=%d del=%d\n", rl_nx, rl_ny, rl_delta);
        fprintf(fp, "BB=(%d,%d  %d,%d)\n", rl_BB.left, rl_BB.bottom,
            rl_BB.right, rl_BB.top);
    }

    int lastd = 0;
    double val = 0.0;
    for (int i = rl_ny - 1; i >= 0; i--) {
        int tnx;
        for (int j = 0; j < rl_nx; j = tnx) {
            tnx = fdx(j, i);
            if (tnx < 0) {
                if (fp)
                    fprintf(fp, " ");
                tnx = j+1;
                continue;
            }

            int l = rl_BB.left + j*rl_delta;
            int r = rl_BB.left + tnx*rl_delta;
            int d2 = rl_delta/2;
            int y = rl_BB.bottom + i*rl_delta + d2;

            for (RLedge *e = rl_v_edges; e; e = e->next) {
                if (e->p < r && e->p >= r - d2) {
                    if (e->e1 <= y && y <= e->e2)
                        r = e->p;
                }
                else if (e->p > l && e->p <= l + d2) {
                    if (e->e1 <= y && y <= e->e2)
                        l = e->p;
                }
            }

            int dx = r - l;
            if (dx != lastd) {
                val = 0.5*l_per_sq(dx);
                lastd = dx;
            }
            if (val <= 0.0) {
                if (fp) {
                    fprintf(fp, "%s\n*** Aborting.\n",
                        "setupL: inductance parameters incorrect for layer.");
                    fflush(fp);
                }
                Errs()->add_error(
                    "setupL: inductance parameters incorrect for layer.");
                return (false);
            }
            for ( ; j < tnx; j++) {
                int n1 = nodeof(j, i);
                if (n1 >= 0) {
                    if (fp) {
                        if (n1 < rl_num_contacts + rl_offset)
                            fprintf(fp, "%d", n1);
                        else
                            fprintf(fp, ".");
                    }
                    if (i+1 < rl_ny) {
                        int n2 = nodeof(j, i+1);
                        if (n2 >= 0 && n1 != n2)
                            add_element(n1, n2, val);
                    }
                    if (i-1 >= 0) {
                        int n2 = nodeof(j, i-1);
                        if (n2 >= 0 && n1 != n2)
                            add_element(n1, n2, val);
                    }
                }
                else if (fp)
                    fprintf(fp, " ");
            }
        }
        if (fp)
            fprintf(fp, "\n");
    }

    if (fp)
        fflush(fp);

    lastd = 0;
    for (int j = 0; j < rl_nx; j++) {
        int tny;
        for (int i = 0; i < rl_ny; i = tny) {
            tny = fdy(j, i);
            if (tny < 0) {
                tny = i+1;
                continue;
            }

            int b = rl_BB.bottom + i*rl_delta;
            int t = rl_BB.bottom + tny*rl_delta;
            int d2 = rl_delta/2;
            int x = rl_BB.left + j*rl_delta + d2;

            for (RLedge *e = rl_h_edges; e; e = e->next) {
                if (e->p < t && e->p >= t - d2) {
                    if (e->e1 <= x && x <= e->e2)
                        t = e->p;
                }
                else if (e->p > b && e->p <= b + d2) {
                    if (e->e1 <= x && x <= e->e2)
                        b = e->p;
                }
            }
            int dy = t - b;

            if (dy != lastd) {
                val = 0.5*l_per_sq(dy);
                lastd = dy;
            }
            if (val <= 0.0) {
                Errs()->add_error(
                    "setupL: inductance parameters incorrect for layer.");
                return (false);
            }
            for ( ; i < tny; i++) {
                int n1 = nodeof(j, i);
                if (n1 >= 0) {
                    if (j+1 < rl_nx) {
                        int n2 = nodeof(j+1, i);
                        if (n2 >= 0 && n1 != n2)
                            add_element(n1, n2, val);
                    }
                    if (j-1 >= 0) {
                        int n2 = nodeof(j-1, i);
                        if (n2 >= 0 && n1 != n2)
                            add_element(n1, n2, val);
                    }
                }
            }
        }
    }

    int error = rl_matrix->spError();
    if (error) {
        Errs()->add_error(rl_matrix->spErrorMessage(error));
        return (false);
    }
    rl_state = RLinduct;
    return (true);
}


// Solve the two-contact problem.
//
bool
RLsolver::solve_two(double *squares)
{
    *squares = 0.0;
    if (!(void*)this)
        return (false);
    if (rl_state == RLuninit) {
        Errs()->add_error("solve_two: solver not initialized.");
        return (false);
    }
    int size = rl_matrix->spGetSize(1);
    if (size <= 0) {
        Errs()->add_error("solve_two: matrix has zero size.");
        return (false);
    }

    if (ExtErrLog.rlsolver_msgs())
        fprintf(DBG_FP, "solve_two: size = %d delta = %d\n", size+1, rl_delta);
    if (ExtErrLog.rlsolver_log_fp()) {
        fprintf(ExtErrLog.rlsolver_log_fp(),
            "solve_two: size = %d delta = %d\n", size+1, rl_delta);
    }

    double *rhs = new double[size+1];
    for (int i = 0; i <= size; i++)
        rhs[i] = 0.0;
    rhs[1] = 1.0;  // current source
    int error = rl_matrix->spOrderAndFactor(rhs, RL_PIVREL, RL_PIVABS, 1);
    if (error) {
        Errs()->add_error(rl_matrix->spErrorMessage(error));
        return (false);
    }
    rl_matrix->spSolve(rhs, rhs);

    double ohm_per_sq = 1.0;
    if (rl_state == RLresist && rl_ld) {
        double rsh = cTech::GetLayerRsh(rl_ld);
        if (rsh > 0.0)
            ohm_per_sq = rsh;
    }

    *squares = rhs[1]*ohm_per_sq;
    delete [] rhs;
    return (true);
}


// Solve the multi-contact problem, returning the matrix of currents
// for each contact, when each contact but one is tied to ground.  The
// effective resistance i,j is Rij = -1/Gij, i != j.
//
bool
RLsolver::solve_multi(int *gmat_size, float **gmat)
{
    *gmat_size = 0;
    *gmat = 0;
    if (!(void*)this)
        return (false);
    if (rl_state == RLuninit) {
        Errs()->add_error("solve_multi: solver not initialized.");
        return (false);
    }
    int size = rl_matrix->spGetSize(1);
    if (size <= 0) {
        Errs()->add_error("solve_multi: matrix has zero size.");
        return (false);
    }

    if (ExtErrLog.rlsolver_msgs()) {
        fprintf(DBG_FP, "solve_multi: size = %d delta = %d  ", size+1,
            rl_delta);
        fflush(stdout);
    }
    if (ExtErrLog.rlsolver_log_fp()) {
        fprintf(ExtErrLog.rlsolver_log_fp(),
            "solve_multi: size = %d delta = %d  ", size+1, rl_delta);
    }

    int topindx = size + 1;
    for (int i = 0; i < rl_num_contacts; i++) {
        double *d = rl_matrix->spGetElement(topindx + i, i+1);
        *d = -1;
        d = rl_matrix->spGetElement(i+1, topindx + i);
        *d = 1;
    }
    size = rl_matrix->spGetSize(1);

    int error = rl_matrix->spOrderAndFactor(0, RL_PIVREL, RL_PIVABS, 1);
    if (error) {
        Errs()->add_error(rl_matrix->spErrorMessage(error));
        return (false);
    }

    double ohm_per_sq = 1.0;
    if (rl_state == RLresist && rl_ld) {
        double rsh = cTech::GetLayerRsh(rl_ld);
        if (rsh > 0.0)
            ohm_per_sq = rsh;
    }

    float *g = new float[rl_num_contacts*rl_num_contacts];
    float *gp = g;
    double *rhs = new double[size+1];
    for (int j = 0; j < rl_num_contacts; j++) {
        for (int i = 0; i <= size; i++)
            rhs[i] = 0.0;
        rhs[topindx+j] = 1.0;  // voltage source

        if (ExtErrLog.rlsolver_msgs()) {
            fprintf(DBG_FP, ".");
            fflush(stdout);
        }
        if (ExtErrLog.rlsolver_log_fp())
            fprintf(ExtErrLog.rlsolver_log_fp(), ".");

        rl_matrix->spSolve(rhs, rhs);
        for (int k = 0; k < rl_num_contacts; k++)
            *gp++ = rhs[topindx + k]/ohm_per_sq;
    }

    if (ExtErrLog.rlsolver_msgs())
        fprintf(DBG_FP, "\n");
    if (ExtErrLog.rlsolver_log_fp())
        fprintf(ExtErrLog.rlsolver_log_fp(), "\n");
        
    *gmat = g;
    *gmat_size = rl_num_contacts;
    delete [] rhs;
    return (true);
}


// Set up the edge lists.  The edge lists are used to adjust the
// entries along Manhattan boundaries when not tiled.
//
void
RLsolver::setup_edges()
{
    PolyList *pl = rl_zlist->copy()->to_poly_list();
    rl_h_edges = to_edges(pl, false);
    rl_v_edges = to_edges(pl, true);
    pl->free();

    for (int i = 0; i < rl_num_contacts; i++) {
        pl = rl_contacts[i].czl->copy()->to_poly_list();
        rl_contacts[i].h_edges = to_edges(pl, false);
        rl_contacts[i].v_edges = to_edges(pl, true);
        pl->free();
    }
}


// Return the side dimension of the maximum square that will tile the
// structure.  Tiling is only possible if all edges are Manhattan, in
// which case the returned value will be 1.
//
int
RLsolver::find_tile()
{
    // Unless the body and contacts are Manhattan, we can't tile.
    for (Zlist *z = rl_zlist; z; z = z->next) {
        if (!z->Z.is_rect())
            return (1);
    }
    for (int i = 0; i < rl_num_contacts; i++) {
        for (Zlist *z = rl_contacts[i].czl; z; z = z->next) {
            if (!z->Z.is_rect())
                return (1);
        }
    }

    // Clip the contact areas out of the body.  For resistor ends
    // that cover the entire resistor strip, this will remove the
    // resistor endpoints from the gcd, potentially allowing fewer
    // tile squares.

    Zlist *zl = rl_zlist->copy();
    Ylist *yl = new Ylist(zl);

    for (int i = 0; i < rl_num_contacts; i++) {
        for (Zlist *z = rl_contacts[i].czl; z; z = z->next)
            yl = yl->clip_out(&z->Z);
    }
    zl = yl->to_zlist();

    // Compute new BB.
    BBox BB;
    zl->BB(BB);

    int h = BB.height();
    int w = BB.width();
    int t = mmMin(w, h);

    for (Zlist *z = zl; z; z = z->next) {
        t = mmGCD(t, z->Z.yu - BB.bottom);
        t = mmGCD(t, z->Z.yl - BB.bottom);
        t = mmGCD(t, z->Z.xll - BB.left);
        t = mmGCD(t, z->Z.xlr - BB.left);
    }
    zl->free();

    return (t);
}


#define MIN_GRID_POINTS 1e2

//  Find the grid spacing.  This varies depending on how the static
// variables are set up.
//
void
RLsolver::set_delta()
{
    if (rl_given_delta * 100 >= CDphysResolution) {
        // If this is given and in range, the grid spacing will take this
        // value.
        rl_delta = rl_given_delta;
        int t = find_tile();
        if (t % rl_delta)
            setup_edges();
    }
    else if (rl_try_tile) {
        // This is the "old" algorithm, try to tile but adjust size if
        // too many or too few grids.  If we can't tile, use the edge
        // lists.

        // Note: the most accurate results appear to come when the
        // structure can be tiled.

        int t = find_tile();
        double a = rl_zlist->area() * CDphysResolution * CDphysResolution;
        double d = a/(t*t);
        bool needs_edges = false;

        // make sure that there aren't too many...
        while (d > rl_maxgrid) {
            t *= 2;
            d /= 4.0;
            needs_edges = true;
        }
        // or too few
        while (d < MIN_GRID_POINTS) {
            t /= 2;
            d *= 4.0;
        }
        rl_delta = t;
        if (needs_edges)
            setup_edges();
    }
    else {
        // Don't try to tile, just set a delta according to the
        // rl_numgrid value and use the edge lists.

        double a = rl_zlist->area() * CDphysResolution * CDphysResolution;
        double t = sqrt(a/rl_numgrid);
        rl_delta = (int)t;
        setup_edges();
    }
}


// Add an element to the matrix.
//
void
RLsolver::add_element(int n1, int n2, double val)
{
    double *d;
    if (n1 > 0 && n2 > 0) {
        d = rl_matrix->spGetElement(n1, n2);
        *d -= val;
        d = rl_matrix->spGetElement(n2, n1);
        *d -= val;
    }
    if (n1 > 0) {
        d = rl_matrix->spGetElement(n1, n1);
        *d += val;
    }
    if (n2 > 0) {
        d = rl_matrix->spGetElement(n2, n2);
        *d += val;
    }
}


// Compute inductance per square for width d.
//
double
RLsolver::l_per_sq(int d)
{
    double w = MICRONS(d);
    double params[8];
    if (rl_ld && cTech::GetLayerTline(rl_ld, params)) {
        // filled in:
        //   params[0] = linethick();
        //   params[1] = linepenet();
        //   params[2] = gndthick();
        //   params[3] = gndpenet();
        //   params[4] = dielthick();
        //   params[5] = dielconst();
        params[6] = w;
        params[7] = w;
        double o[4];
        sline(params, o);
        return (1.0/o[0]);
    }
    return (-1.0);
}
// End of RLsolver functions


//-------------------------------------------------------------------------
// MRsolver: solve for resistance on a wire net (multi-layer)

MRsolver::~MRsolver()
{
    SymTabGen gen(mr_ltab, true);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        cnd_t *c = (cnd_t*)h->stData;
        while (c) {
            cnd_t *cn = c->next;
            delete c;
            c = cn;
        }
        delete h;
    }
    delete mr_ltab;

    while (mr_vias) {
        via_t *vn = mr_vias->next;
        delete mr_vias;
        mr_vias = vn;
    }
}


// Find the path under the point.  The path contains copies of metal
// objects to depth.
//
bool
MRsolver::find_path(int x, int y, int depth, bool use_ex)
{
    pathfinder *pf = use_ex ? new grp_pathfinder() : new pathfinder();
    pf->set_depth(depth);
    BBox BB(x, y, x, y);
    BB.bloat(1);
    if (!pf->find_path(&BB)) {
        Errs()->add_error("find_path: no path found.");
        delete pf;
        return (false);
    }
    SymTab *tab = pf->path_table();

    // Put the objects into a new symbol table keyed by the layer.

    mr_ltab = new SymTab(false, false);
    SymTabGen gen(tab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {

        CDo *od = (CDo*)h->stData;
        SymTabEnt *hl = mr_ltab->get_ent((unsigned long)od->ldesc());
        if (!hl)
            mr_ltab->add((unsigned long)od->ldesc(), od, false);
        else {
            CDo *ox = od;
            while (ox->next_odesc())
                ox = ox->next_odesc();
            ox->set_next_odesc((CDo*)hl->stData);
            hl->stData = od;
        }
        h->stData = 0;
    }
    delete tab;

    // Convert the object list for each layer into a zoid list.

    gen = SymTabGen(mr_ltab);
    while ((h = gen.next()) != 0) {
        Zlist *z0 = 0, *ze = 0;
        CDo *on;
        for (CDo *od = (CDo*)h->stData; od; od = on) {
            on = od->next_odesc();
            Zlist *zx = od->toZlist();
            if (z0) {
                while (ze->next)
                    ze = ze->next;
                ze->next = zx;
            }
            else
                z0 = ze = zx;
            delete od;
        }
        h->stData = z0;
    }
    delete pf;
    return (true);
}


// Load the net path into the solver.  THE LIST, AND THE OBJECTS, ARE
// FREED!  Objects MUST be copies!
//
bool
MRsolver::load_path(CDol *ol)
{
    // Put the objects into a new symbol table keyed by the layer.
    //
    mr_ltab = new SymTab(false, false);
    while (ol) {
        CDo *od = ol->odesc;
        od->set_next_odesc(0);
        SymTabEnt *hl = mr_ltab->get_ent((unsigned long)od->ldesc());
        if (!hl)
            mr_ltab->add((unsigned long)od->ldesc(), od, false);
        else {
            od->set_next_odesc((CDo*)hl->stData);
            hl->stData = od;
        }
        CDol *otmp = ol;
        ol = ol->next;
        delete otmp;
    }

    // Convert the object list for each layer into a zoid list.
    //
    SymTabGen gen(mr_ltab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        Zlist *z0 = 0, *ze = 0;
        CDo *on;
        for (CDo *od = (CDo*)h->stData; od; od = on) {
            on = od->next_odesc();
            Zlist *zx = od->toZlist();
            if (z0) {
                while (ze->next)
                    ze = ze->next;
                ze->next = zx;
            }
            else
                z0 = ze = zx;
            delete od;
        }
        h->stData = z0;
    }
    return (true);
}


// Add a terminal.
//
bool
MRsolver::add_terminal(Zlist *zt)
{
    PolyList *p0 = zt->copy()->to_poly_list();
    if (!p0) {
        Errs()->add_error("add_terminal: no poly for terminal.");
        return (false);
    }
    mr_vias = new via_t(p0, 0, mr_term_count, mr_vias);
    mr_term_count++;
    return (true);
}


// Find the vias, each is assigned a node number.
//
bool
MRsolver::find_vias()
{
    int ncnt = mr_term_count;  // start numbering after terminals
    if (mr_term_count > 2) {
        // Increment terminal nodes, so that node 0 is not a terminal.
        // In the multi case, we effectively connect voltage sources
        // between terminals and ground.
        ncnt++;
        for (via_t *v = mr_vias; v; v = v->next)
            v->node++;
    }
    CDs *cursdp = CurCell(Physical);
    if (!cursdp)
        return (false);
    CDl *ld;
    CDextLgen lg(CDL_VIA);
    while ((ld = lg.next()) != 0) {
        if (!ld->isVia())
            continue;
        for (sVia *via = tech_prm(ld)->via_list(); via; via = via->next()) {

            if (!via->layer1() || !via->layer2())
                continue;
            CDl *ld1 = via->layer1();
            if (!ld1)
                continue;
            Zlist *z1 = (Zlist*)mr_ltab->get((unsigned long)ld1);
            if (z1 == (Zlist*)ST_NIL)
                continue;
            CDl *ld2 = via->layer2();
            if (!ld2)
                continue;
            Zlist *z2 = (Zlist*)mr_ltab->get((unsigned long)ld2);
            if (z2 == (Zlist*)ST_NIL)
                continue;

            XIrt ret;
            z1 = z1->copy();
            z2 = z2->copy();
            ret = Zlist::zl_and(&z1, z2);
            if (ret != XIok) {
                Errs()->add_error("find_vias: zl_and failed or user abort.");
                return (false);
            }

            z2 = cursdp->getZlist(CDMAXCALLDEPTH, ld, z1, &ret);
            if (ret != XIok) {
                Errs()->add_error(
                    "find_vias: getZlist failed or user abort.");
                return (false);
            }
            z1->free();

            PolyList *p0 = z2->to_poly_list();

            if (via->tree()) {
                PolyList *pn, *pp = 0;
                for (PolyList *p = p0; p; p = pn) {
                    pn = p->next;

                    bool istrue = !via->tree();
                    if (!istrue) {
                        sLspec lsp;
                        lsp.set_tree(via->tree());
                        Zlist *zx = p->po.toZlist();
                        ret = lsp.testContact(cursdp, CDMAXCALLDEPTH, zx,
                            &istrue);
                        zx->free();
                        lsp.set_tree(0);
                        if (ret == XIintr) {
                            Errs()->add_error("find_vias: interrupted");
                            p0->free();
                            return (false);
                        }
                        if (ret == XIbad) {
                            Errs()->add_error("find_vias: testContact failed.");
                            p0->free();
                            return (false);
                        }
                    }
                    if (!istrue) {
                        if (pp)
                            pp->next = pn;
                        else
                            p0 = pn;
                        delete p;
                        continue;
                    }
                    pp = p;
                }
            }

            while (p0) {
                PolyList *pn = p0->next;
                p0->next = 0;
                mr_vias = new via_t(p0, ld, ncnt, mr_vias);
                ncnt++;
                p0 = pn;
            }
        }
    }

    // Convert each conductor zoidlist into disjoint conductors.
    {
        SymTabGen gen(mr_ltab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            Zlist *z0 = (Zlist*)h->stData;

            int tg = Zlist::JoinMaxGroup;
            int tv = Zlist::JoinMaxVerts;
            Zlist::JoinMaxGroup = 0;
            Zlist::JoinMaxVerts = 0;
            PolyList *p0 = z0->to_poly_list();
            Zlist::JoinMaxGroup = tg;
            Zlist::JoinMaxVerts = tv;

            cnd_t *c0 = 0;
            while (p0) {
                PolyList *pn = p0->next;
                p0->next = 0;
                c0 = new cnd_t(p0, (CDl*)h->stTag, c0);
                p0 = pn;
            }
            h->stData = c0;
        }
    }

    // For each via, find the two connected conductors.  Delete vias
    // that don't have two connections.

    via_t *vp = 0, *vn;
    for (via_t *v = mr_vias; v; v = vn) {
        vn = v->next;
        if (!v->ld) {
            // a terminal
            SymTabGen gen(mr_ltab);
            SymTabEnt *h;
            while ((h = gen.next()) != 0) {
                for (cnd_t *c = (cnd_t*)h->stData; c; c = c->next) {
                    if (v->po->po.intersect(&c->po->po, false)) {
                        v->c1 = c;
                        c->vias = new vls_t(v, c->vias);
                        break;
                    }
                }
                if (v->c1)
                    break;
            }
            if (!v->c1) {
                // bad terminal
                Errs()->add_error(
                    "find_vias: terminal does not intersect net.");
                return (false);
            }
            vp = v;
            continue;
        }
        CDl *ld1 = tech_prm(v->ld)->via_list()->layer1();
        CDl *ld2 = tech_prm(v->ld)->via_list()->layer2();

        cnd_t *c1 = (cnd_t*)mr_ltab->get((unsigned long)ld1);
        if (c1 != (cnd_t*)ST_NIL) {
            for (cnd_t *c = c1; c; c = c->next) {
                if (v->po->po.intersect(&c->po->po, false)) {
                    v->c1 = c;
                    c->vias = new vls_t(v, c->vias);
                    break;
                }
            }
        }
        if (!v->c1) {
            if (vp)
                vp->next = vn;
            else
                mr_vias = vn;
            delete v;
            continue;
        }
        cnd_t *c2 = (cnd_t*)mr_ltab->get((unsigned long)ld2);
        if (c2 != (cnd_t*)ST_NIL) {
            for (cnd_t *c = c2; c; c = c->next) {
                if (v->po->po.intersect(&c->po->po, false)) {
                    v->c2 = c;
                    c->vias = new vls_t(v, c->vias);
                    break;
                }
            }
        }
        if (!v->c2) {
            if (vp)
                vp->next = vn;
            else
                mr_vias = vn;
            delete v;
            continue;
        }
        vp = v;
    }

    // Throw out conductors with less than two connections.
    {
        SymTabGen gen(mr_ltab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            cnd_t *cp = 0, *cn;
            for (cnd_t *c = (cnd_t*)h->stData; c; c = cn) {
                cn = c->next;
                if (!c->vias || !c->vias->next) {
                    if (cp)
                        cp->next = cn;
                    else
                        h->stData = cn;
                    delete c;
                    continue;
                }
                cp = c;
            }
        }
    }

    if (ExtErrLog.rlsolver_msgs()) {
        SymTabGen gen(mr_ltab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            int cnt = 0;
            for (cnd_t *c = (cnd_t*)h->stData; c; c = c->next)
                cnt++;
            fprintf(DBG_FP, "found %d elements on %s\n", cnt,
                ((CDl*)h->stTag)->name());
        }
        int cnt = 0;
        for (via_t *v = mr_vias; v; v = v->next)
            cnt++;
        fprintf(DBG_FP, "found %d vias and terminals\n", cnt);
    }
    if (ExtErrLog.rlsolver_log_fp()) {
        SymTabGen gen(mr_ltab);
        SymTabEnt *h;
        while ((h = gen.next()) != 0) {
            int cnt = 0;
            for (cnd_t *c = (cnd_t*)h->stData; c; c = c->next)
                cnt++;
            fprintf(ExtErrLog.rlsolver_log_fp(), "found %d elements on %s\n",
                cnt, ((CDl*)h->stTag)->name());
        }
        int cnt = 0;
        for (via_t *v = mr_vias; v; v = v->next)
            cnt++;
        fprintf(ExtErrLog.rlsolver_log_fp(), "found %d vias and terminals\n",
            cnt);
    }

    return (true);
}


// Material    Resistivity (ohm-m) at 20 C     Temperature coefficient* [1/K]
// Silver      1.59e-8                         0.0038
// Copper      1.72e-8                         0.0039
// Gold        2.44e-8                         0.0034
// Aluminium   2.82e-8                         0.0039


// For each object, compute resistance at nodes.
//
bool
MRsolver::solve_elements()
{
    SymTabGen gen(mr_ltab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {

        if (ExtErrLog.rlsolver_msgs())
            fprintf(DBG_FP, "solving on %s\n", ((CDl*)h->stTag)->name());
        if (ExtErrLog.rlsolver_log_fp()) {
            fprintf(ExtErrLog.rlsolver_log_fp(), "solving on %s\n",
                ((CDl*)h->stTag)->name());
        }

        for (cnd_t *c = (cnd_t*)h->stData; c; c = c->next) {
            if (c->gmat)
                continue;

            Zlist *zl = c->po->po.toZlist();

            int nc = 0;
            for (vls_t *v = c->vias; v; v = v->next)
                nc++;
            Zgroup zg;
            zg.list = new Zlist*[nc];
            zg.num = nc;
            nc = 0;
            for (vls_t *v = c->vias; v; v = v->next) {
                zg.list[nc] = v->via->po->po.toZlist();
                nc++;
            }

            RLsolver *r = new RLsolver;
            bool ret = r->setup(zl, c->ld, &zg);
            zl->free();
            if (!ret) {
                Errs()->add_error("solve_elements: setup failed.");
                return (false);
            }
            ret = r->setupR();
            if (!ret) {
                Errs()->add_error("solve_elements: setupR failed.");
                return (false);
            }
            if (nc == 2) {
                c->gmat = new float[1];
                c->gmat_size = 1;
                double result;
                ret = r->solve_two(&result);
                if (!ret) {
                    Errs()->add_error("solve_elements: solve_two failed.");
                    return (false);
                }
                c->gmat[0] = result;
            }
            else {
                ret = r->solve_multi(&c->gmat_size, &c->gmat);
                if (!ret) {
                    Errs()->add_error("solve_elements: solve_multi failed.");
                    return (false);
                }
            }
            delete r;
        }
    }
    return (true);
}


bool
MRsolver::write_spice(FILE *fp)
{
    if (!fp) {
        Errs()->add_error("write_spice: null file pointer.");
        return (false);
    }
    fprintf(fp, "* Generated by %s\n", XM()->IdString());
    fprintf(fp,
        "* Wire net resistance analysis, nodes %d-%d are terminals.\n\n",
        mr_term_count > 2 ? 1 : 0, mr_term_count > 2 ? mr_term_count : 1);
    int rnum = 1;
    SymTabGen gen(mr_ltab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        for (cnd_t *c = (cnd_t*)h->stData; c; c = c->next) {
            if (c->gmat_size > 2) {
                int *map = new int[c->gmat_size];
                vls_t *v = c->vias;
                for (int i = 0; i < c->gmat_size; i++) {
                    if (!v) {
                        Errs()->add_error(
            "write_spice: internal error, via count less than matrix size.");
                        return (false);
                    }
                    map[i] = v->via->node;
                    v = v->next;
                }
                if (v) {
                        Errs()->add_error(
            "write_spice: internal error, via count greater than matrix size.");
                    return (false);
                }

                for (int i = 0; i < c->gmat_size; i++) {
                    for (int j = i+1; j < c->gmat_size; j++) {
                        fprintf(fp, "R%d %d %d %g\n", rnum, map[i], map[j],
                            -1.0/c->gmat[i*c->gmat_size + j]);
                        rnum++;
                    }
                }
                delete [] map;
            }
            else {
                fprintf(fp, "R%d %d %d %g\n", rnum, c->vias->via->node,
                    c->vias->next->via->node, c->gmat[0]);
                rnum++;
            }
        }
    }
    return (true);
}


namespace {
    void
    add_elt(spMatrixFrame *mat, int n1, int n2, double val)
    {
        double *d;
        if (n1 > 0 && n2 > 0) {
            d = mat->spGetElement(n1, n2);
            *d -= val;
            d = mat->spGetElement(n2, n1);
            *d -= val;
        }
        if (n1 > 0) {
            d = mat->spGetElement(n1, n1);
            *d += val;
        }
        if (n2 > 0) {
            d = mat->spGetElement(n2, n2);
            *d += val;
        }
    }
}


bool
MRsolver::solve(int *gsize, float **gmat)
{
    *gsize = 0;
    *gmat = 0;
    spMatrixFrame *matrix = new spMatrixFrame(0, 0);
    int error = matrix->spError();
    if (error) {
        Errs()->add_error(matrix->spErrorMessage(error));
        delete matrix;
        return (false);
    }
    matrix->spSetInterruptCheckFunc(&check_for_interrupt);

    SymTabGen gen(mr_ltab);
    SymTabEnt *h;
    while ((h = gen.next()) != 0) {
        for (cnd_t *c = (cnd_t*)h->stData; c; c = c->next) {
            if (c->gmat_size > 2) {
                int *map = new int[c->gmat_size];
                vls_t *v = c->vias;
                for (int i = 0; i < c->gmat_size; i++) {
                    if (!v) {
                        Errs()->add_error(
                "solve: internal error, via count less than matrix size.");
                        return (false);
                    }
                    map[i] = v->via->node;
                    v = v->next;
                }
                if (v) {
                        Errs()->add_error(
                "solve: internal error, via count greater than matrix size.");
                    return (false);
                }

                for (int i = 0; i < c->gmat_size; i++) {
                    for (int j = i+1; j < c->gmat_size; j++) {
                        add_elt(matrix, map[i], map[j],
                            -c->gmat[i*c->gmat_size + j]);
                    }
                }
                delete [] map;
            }
            else
                add_elt(matrix, c->vias->via->node,
                    c->vias->next->via->node, 1.0/c->gmat[0]);
        }
    }

    int size = matrix->spGetSize(1);
    if (size <= 0) {
        Errs()->add_error("solve: matrix has zero size.");
        delete matrix;
        return (false);
    }

    int topindx = size + 1;
    if (mr_term_count > 2) {
        // Add the voltage sources.
        for (int i = 0; i < mr_term_count; i++) {
            double *d = matrix->spGetElement(topindx + i, i+1);
            *d = -1;
            d = matrix->spGetElement(i+1, topindx + i);
            *d = 1;
        }
        size = matrix->spGetSize(1);
    }

    error = matrix->spError();
    if (error) {
        Errs()->add_error(matrix->spErrorMessage(error));
        delete matrix;
        return (false);
    }

    error = matrix->spOrderAndFactor(0, RL_PIVREL, RL_PIVABS, 1);
    if (error) {
        Errs()->add_error(matrix->spErrorMessage(error));
        delete matrix;
        return (false);
    }

    if (mr_term_count > 2) {
        float *g = new float[mr_term_count*mr_term_count];
        float *gp = g;
        double *rhs = new double[size+1];
        for (int j = 0; j < mr_term_count; j++) {
            for (int i = 0; i <= size; i++)
                rhs[i] = 0.0;
            rhs[topindx+j] = 1.0;  // voltage source
            matrix->spSolve(rhs, rhs);
            for (int k = 0; k < mr_term_count; k++)
                *gp++ = rhs[topindx + k];
        }
        delete [] rhs;
        *gsize = mr_term_count;
        *gmat = g;
    }
    else {
        double *rhs = new double[size+1];
        for (int i = 0; i <= size; i++)
            rhs[i] = 0.0;
        rhs[1] = 1.0;  // current source
        matrix->spSolve(rhs, rhs);
        float *g = new float[1];
        g[0] = rhs[1];
        delete [] rhs;
        *gsize = 1;
        *gmat = g;
    }
    delete matrix;
    return (true);
}
// End of MRsolver functions


//-------------------------------------------------------------------------
// Superconducting Microstripline Model

namespace {
    struct params
    {
        double lthick;
        double ldepth;
        double gpthick;
        double gpdepth;
        double dthick;
        double dielcon;
        double lwidth;
        double llength;
    };

    struct output
    {
        double L;
        double C;
        double Z;
        double T;
    };
}


#define EP 8.85416e-6
#define MU 1.256637
#define MAX(x,y) (((x)>(y)) ? (x) : (y))

void
sline(double *tlp, double *outp)
{
    params *tl = (params*)tlp;
    double p = 1.0 + tl->lthick/tl->dthick;
    p    = 2.0*p*p - 1.0;
    p   += sqrt(p*p - 1.0);
    double pp = sqrt(p);
    double aa = (p+1.0)/(2.0*pp);

    double ff = .5*M_PI*tl->lwidth/tl->dthick;
    double eta = pp*(ff + aa*(1.0 + log(4.0/(p-1.0))) - 2.0*atanh(1.0/pp));

    double rb0 = eta + .5*(p+1.0)*log(MAX(p, eta));
    double rb = rb0;
    if (tl->lwidth/tl->dthick < 5.0) {
        double tmp = (rb0-p)/(rb0-1.0);
        rb += pp*ff - sqrt((rb0-1.0)*(rb0-p)) + (p+1.0)*atanh(sqrt(tmp)) -
            2.0*pp*atanh(sqrt(tmp/p));
    }

    double lnra = -2.0*aa*atanh(1.0/pp) - (1.0 + ff + log(.25*(p-1.0)/p));
    double kappa = (log(2.0*rb) - lnra)/ff;
    double cap = kappa*EP*tl->dielcon*tl->lwidth/tl->dthick;

    double gind = tl->dthick;
    if (tl->gpdepth > 0.0)
        gind += tl->gpdepth/tanh(tl->gpthick/tl->gpdepth);
    if (tl->ldepth > 0.0)
        gind += tl->ldepth/tanh(tl->lthick/tl->ldepth) +
            2.0*tl->ldepth*pp/(rb*sinh(tl->lthick/tl->ldepth));
    gind *= MU/(kappa*tl->lwidth);

    output *out = (output*)outp;
    out->L = gind*tl->llength;
    out->C = cap*tl->llength;
    out->Z = sqrt(gind/cap);
    out->T = sqrt(gind*cap)*tl->llength;
}

