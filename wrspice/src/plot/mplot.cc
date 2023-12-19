
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
Authors: 1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "miscutil/texttf.h"


/*********************************************************************
 *
 * Operating range analysis plotting.
 *
 *********************************************************************/

namespace {
    sChkPts *get_fromvec(sDataVec*);
    void extpts(FILE*, sChkPts**, const char*);
    void combin(sChkPts*);
    void addit(sChkPts*, int, int, char);

    // common error messages
    const char *errmsg_gralloc = "can't allocate new graph.\n";
    const char *errmsg_newvp = "can't init viewport for graphics.\n";
}


// Operating range analysis plotting command.
//
void
CommandTab::com_mplot(wordlist *wl)
{
    FILE *fp = 0;
    sChkPts *p0 = 0;

    if (!wl) {
        VTvalue vv;
        if (Sp.GetVar(kw_mplot_cur, VTYP_STRING, &vv)) {
            fp = fopen(vv.get_string(), "r");
            if (fp) {
                extpts(fp, &p0, vv.get_string());
                fclose(fp);
            }
        }
        if (!fp) {
            GRpkg::self()->ErrPrintf(ET_ERROR, "no current data file.\n");
            return;
        }
    }
    if (wl && !wl->wl_next) {
        // If one arg, it can be the name of a multi-dimensional vector
        // for use in selecting the components to plot
        sDataVec *d = OP.vecGet(wl->wl_word, 0);
        if (d) {
            p0 = get_fromvec(d);
            if (!p0) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "not a multi-dimensional vector.\n");
                return;
            }
            wl = 0;
        }
    }
    char c = 0, d = 0;
    for (wordlist *ww = wl; ww; ww = ww->wl_next) {
        if (*ww->wl_word == '-' && *(ww->wl_word+1) == 'c') {
            c = 1;
            continue;
        }    
        if (*ww->wl_word == '-' && *(ww->wl_word+1) == 'o' &&
            *(ww->wl_word+2) == 'n') {
            GP.SetMplotOn(true);
            TTY.printf_force("Will display plot during analysis.\n");
            continue;
        }    
        if (*ww->wl_word == '-' && *(ww->wl_word+1) == 'o' &&
            *(ww->wl_word+2) == 'f') {
            GP.SetMplotOn(false);
            TTY.printf_force("Will not display plot during analysis.\n");
            continue;
        }    
        fp = fopen(ww->wl_word, "r");
        if (!fp) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "data file \"%s\" not found.\n", ww->wl_word);
            continue;
        }
        d = 1;
        extpts(fp, &p0, ww->wl_word);
        fclose(fp);
    }
    if (!p0) {
        if (d)
            GRpkg::self()->ErrPrintf(ET_ERROR, "nothing in file to plot.\n");
        return;
    }
    if (c) combin(p0);

    sChkPts *p;
    for (p = p0; p; p = p0) {
        p0 = p->next;

        sGraph *graph = GP.NewGraph(GR_MPLT, GR_MPLTstr);
        if (!graph) {
            GRpkg::self()->ErrPrintf(ET_ERROR, errmsg_gralloc);
            for (; p; p = p0) {
                p0 = p->next;
                delete p;
            }
            return;
        }
        graph->set_plotdata(p);

        if (graph->gr_dev_init()) {
            GRpkg::self()->ErrPrintf(ET_ERROR, errmsg_newvp);
            graph->halt();
            GP.DestroyGraph(graph->id());
            continue;
        }

        if (GRpkg::self()->CurDev()->devtype != GRmultiWindow) {
            graph->gr_redraw();
            if (GRpkg::self()->CurDev()->devtype == GRfullScreen)
                graph->mp_prompt(p);
            graph->halt();
            GP.DestroyGraph(graph->id());
        }
    }
}
// End of CommandTab functions.


// Full-screen mode prompt function.
//
void
sGraph::mp_prompt(sChkPts *p)
{
    gr_dev->SetColor(gr_colors[0].pixel);
    gr_dev->Box(gr_area.left(), yinv(gr_area.bottom()),
        gr_area.right(), yinv(gr_area.bottom() + gr_fonthei));
    gr_dev->SetColor(gr_colors[8].pixel);
    gr_dev->Text( "Hit any key to continue, 'p' for hardcopy", 
        p->xc + p->d/2 - 20*gr_fontwid - gr_fontwid/2,
        yinv(gr_area.bottom()), 0);
    gr_dev->Update();
    if (GP.Getchar(0, -1, 0) == 'p') {
        gr_dev->SetColor(gr_colors[0].pixel);
        gr_dev->Box(gr_area.left(), yinv(gr_area.bottom()),
            gr_area.right(), yinv(gr_area.bottom() + gr_fonthei));
        gr_hardcopy();
    }
}


void
sGraph::mp_mark(char pf)
{
    sChkPts *p = static_cast<sChkPts*>(gr_plotdata);
    addit(p, p->d1, p->d2, pf);
    int x = p->xc + p->d1*p->d;
    int y = p->yc + p->d2*p->d;
    if (pf) {
        gr_dev->SetColor(gr_colors[3].pixel);
        mp_pbox(x+1, y+1, x+p->d-1, y+p->d-1);
    }
    else {
        gr_dev->SetColor(gr_colors[2].pixel);
        mp_xbox(x+1, y+1, x+p->d-1, y+p->d-1);
    }
    gr_dirty = true;
    gr_dev->Update();
}


// Handle button presses in the viewport.  This allows cells to be
// "selected".
//
void
sGraph::mp_bdown_hdlr(int button, int x, int y)
{
    sChkPts *p = static_cast<sChkPts*>(gr_plotdata);
    int j;
    int x1 = 0, y1 = 0;
    for (j = 0; j < p->size; j++) {
        x1 = p->xc + p->v1[j]*p->d;
        y1 = p->yc + p->v2[j]*p->d;
        if (x1 < x && x < x1 + p->d && y1 < y && y < y1 + p->d)
            break;
    }
    if (j == p->size)
        return;
    mp_free_sel();

    if (!p->sel) {
        p->sel = new char[p->size];
        memset(p->sel, 0, p->size);
    }
    bool ison = p->sel[j];

    for (j = 0; j < p->size; j++) {
        int x2 = p->xc + p->v1[j]*p->d;
        int y2 = p->yc + p->v2[j]*p->d;
        if ((button == 1 && x1 == x2 && y1 == y2) ||
                (button == 2 && y1 == y2) ||
                (button == 3 && x1 == x2)) {
            if (ison) {
                p->sel[j] = false;
                gr_dev->SetColor(gr_colors[0].pixel);
                gr_dev->Box(x2, yinv(y2), x2+p->d, yinv(y2+p->d));
            }
            else {
                p->sel[j] = true;
                gr_dev->SetColor(gr_colors[17].pixel);
                gr_dev->Box(x2, yinv(y2), x2+p->d, yinv(y2+p->d));
                gr_dev->SetColor(gr_colors[1].pixel);
                char buf[64];
                if (p->type == MDdimens && p->delta2 > 1 && !p->flat) {
                    int d1 = (p->delta1 - 1)/2;
                    int d2 = (p->delta2 - 1)/2;
                    snprintf(buf, sizeof(buf), "%d", p->v2[j]+d2);
                    int w, h;
                    gr_dev->TextExtent(buf, &w, &h);
                    gr_dev->Text(buf, x2 + 2, yinv(y2 + (p->d - h)/2), 0);
                    snprintf(buf, sizeof(buf), "%d", p->v1[j]+d1);
                    gr_dev->TextExtent(buf, &w, &h);
                    gr_dev->Text(buf, x2 + (p->d - w)/2, yinv(y2 + 2), 0);
                }
                else {
                    snprintf(buf, sizeof(buf), "%d", j);
                    int w, h;
                    gr_dev->TextExtent(buf, &w, &h);
                    gr_dev->Text(buf, x2 + (p->d - w)/2, yinv(y2 + 2), 0);
                }
            }
            if (p->pf[j]) {
                gr_dev->SetColor(gr_colors[3].pixel);
                mp_pbox(x2+1, y2+1, x2+p->d-1, y2+p->d-1);
            }
            else {
                gr_dev->SetColor(gr_colors[2].pixel);
                mp_xbox(x2+1, y2+1, x2+p->d-1, y2+p->d-1);
            }
        }
    }
    gr_dirty = true;
}


// Clear the selections on *other* windows.
//
void
sGraph::mp_free_sel()
{
    sGgen *g = GP.InitGgen();
    sGraph *graph;
    while ((graph = g->next()) != 0) {
        if (graph->gr_apptype != GR_MPLT)
            continue;
        if (graph == this)
            continue;
        sChkPts *p = static_cast<sChkPts*>(graph->gr_plotdata);
        if (p->sel) {
            delete [] p->sel;
            p->sel = 0;
            graph->gr_redraw();
        }
    }
}


// Redraw procedure.
//
bool
sGraph::mp_redraw()
{
    sChkPts *p = static_cast<sChkPts*>(gr_plotdata);
    gr_init_data();

    const char *param1 = 0;
    if (p->param1)
        param1 = p->param1;
    else if (p->type == MDnormal)
        param1 = "Param 1";
    const char *param2 = 0;
    if (p->param2)
        param2 = p->param2;
    else if (p->type == MDnormal)
        param2 = "Param 2";

    bool nopf = false;
    if (p->type == MDdimens && !p->pfset)
        nopf = true;

    int iy = 1;
    if (param1)
        iy += 2;
    if (param2)
        iy += 3;
    if (p->date)
        iy++;
    if (!nopf)
        iy += 3;

    int xx = gr_area.width() - 4*gr_fontwid;
    int yy = gr_area.height() - (iy+2)*gr_fonthei;
    double dx = (1.0 + (double)p->delta1)/xx;
    double dy = (1.0 + (double)p->delta2)/yy;

    int d;
    if (dx > dy)
        d = (int)(1.0/dx);
    else
        d = (int)(1.0/dy);

    int dd1 = ((p->delta1-1)*d) >> 1;
    int dd2 = ((p->delta2-1)*d) >> 1;
    p->d = d;

    int spa = gr_fonthei/4;
    int w = d + 2*(dd1 + spa);
    // Account (sort-of) for the text width.
    int tw = 26*gr_fontwid;
    if (w < tw)
        w = tw;
    int marg = (gr_area.width() - w)/2;
    p->xc = gr_area.left() + dd1 + spa + marg;
    
    int h = d + 2*(dd2 + spa);
    h += iy*gr_fonthei;
    marg = (gr_area.height() - h)/2;
    if (param1)
        iy -= 2;
    if (param2)
        iy--;
    p->yc = gr_area.bottom() + dd2 + spa + iy*gr_fonthei + marg;

    int xc = p->xc;
    int yc = p->yc;
    if (!(p->delta1 & 1)) p->xc += d/2;
    if (!(p->delta2 & 1)) p->yc += d/2;
    int xl = xc-dd1-spa;
    int yb = yc-dd2-spa;
    int xr = xc+dd1+d+spa;
    int yt = yc+dd2+d+spa;
    // used for keyed text
    gr_vport.set_left(xl);
    gr_vport.set_bottom(yb);
    gr_vport.set_width(xr - xl);
    gr_vport.set_height(yt - yb);

    // draw outline around viewport
    gr_dev->SetLinestyle(0);
    gr_dev->SetColor(gr_colors[4].pixel);
    gr_linebox(xl, yb, xr, yt);

    char buf[BSIZE_SP];
    // scale factors and parameter names
    if (p->type != MDmonte) {
        if (param1) {
            if (dd1)
                snprintf(buf, sizeof(buf), "%s   delta: %g", param1,
                    (p->maxv1-p->minv1)/(p->delta1-1));
            else
                strcpy(buf, param1);

            int y = yt + gr_fonthei/2;
            if (param2 && dd2)
                y += gr_fonthei;
            gr_dev->SetColor(gr_colors[1].pixel);
            gr_save_text(buf, xc + d/2, y + gr_fonthei, LAxval, 1, TXTF_HJC);
            gr_dev->SetColor(gr_colors[8].pixel);
            mp_writeg(p->minv1, xl, y, 'l');
            if (dd1) {
                if (dd1 < 2)
                    mp_writeg(p->maxv1, xc+dd1+d, y, 'l');
                else
                    mp_writeg(p->maxv1, xc+dd1+d, y, 'r');
            }
        }
        if (param2) {
            if (dd2) {
                snprintf(buf, sizeof(buf), "%s   delta: %g", param2,
                    (p->maxv2-p->minv2)/(p->delta2-1));
            }
            else
                strcpy(buf, param2);
            int y = yb - gr_fonthei - gr_fonthei/2;
            gr_dev->SetColor(gr_colors[1].pixel);
            gr_save_text(buf, xl, y - gr_fonthei, LAyval, 1, 0);
            gr_dev->SetColor(gr_colors[8].pixel);
            mp_writeg(p->minv2, xl, y, 'l');
            if (dd2)
                mp_writeg(p->maxv2, xl, yt + gr_fonthei/2, 'l');
        }
    }

    yy = yb - gr_fonthei - gr_fonthei/2;
    if (param2)
        yy -= 2*gr_fonthei + gr_fonthei/2;
    xx = xl;
    switch (p->type) {
    case MDnormal:
        gr_save_text("Operating Range Analysis", xx, yy, LAtitle, 1, 0);
        yy -= gr_fonthei;
        break;
    case MDmonte:
        gr_save_text("Monte Carlo Analysis", xx, yy, LAtitle, 1, 0);
        yy -= gr_fonthei;
        break;
    default:
        break;
    }
    if (p->filename) {
        if (p->type == MDdimens)
            snprintf(buf, sizeof(buf), "Vector: %s", p->filename);
        else
            snprintf(buf, sizeof(buf), "Circuit file: %s", p->filename);
        gr_save_text(buf, xx, yy, LAname, 1, 0);
        yy -= gr_fonthei;
    }
    else {
        VTvalue vv;
        if (Sp.GetVar(kw_mplot_cur, VTYP_STRING, &vv)) {
            snprintf(buf, sizeof(buf), "Output file: %s", vv.get_string());
            gr_save_text(buf, xx, yy, LAname, 1, 0);
            yy -= gr_fonthei;
        }
    }
    if (p->date) {
        gr_save_text(p->date, xx, yy, LAdate, 1, 0);
        yy -= gr_fonthei;
    }

    yy -= gr_fonthei;

    if (!nopf) {
        gr_dev->SetColor(gr_colors[3].pixel);
        mp_pbox(xx, yy, xx + gr_fonthei, yy + gr_fonthei);
        gr_dev->SetColor(gr_colors[8].pixel);
        gr_dev->Text("pass", xx + gr_fonthei + gr_fonthei/2, yinv(yy), 0);

        xx += gr_fonthei + gr_fonthei/2 + 7*gr_fontwid;

        gr_dev->SetColor(gr_colors[2].pixel);
        mp_xbox(xx, yy, xx + gr_fonthei, yy + gr_fonthei);
        gr_dev->SetColor(gr_colors[8].pixel);
        gr_dev->Text("fail", xx + gr_fonthei + gr_fonthei/2, yinv(yy), 0);
    }

    for (int j = 0; j < p->size; j++) {
        int x = p->xc + p->v1[j]*p->d;
        int y = p->yc + p->v2[j]*p->d;
        bool invclr = false;
        if (p->sel) {
            if (!p->sel[j]) {
                gr_dev->SetColor(gr_colors[0].pixel);
                gr_dev->Box(x, yinv(y), x+p->d, yinv(y+p->d));
            }
            else {
                if (GRpkg::self()->CurDev()->numcolors == 2)
                    invclr = true;
                    // all colors except 0 are black, use reverse video
                    // in this cell
                gr_dev->SetColor(gr_colors[17].pixel);
                gr_dev->Box(x, yinv(y), x+p->d, yinv(y+p->d));
                gr_dev->SetColor(
                    invclr ? gr_colors[0].pixel : gr_colors[1].pixel);
                if (p->type == MDdimens && p->delta2 > 1 && !p->flat) {
                    int d1 = (p->delta1 - 1)/2;
                    int d2 = (p->delta2 - 1)/2;
                    snprintf(buf, sizeof(buf), "%d", p->v2[j]+d2);
                    int ww, hh;
                    gr_dev->TextExtent(buf, &ww, &hh);
                    gr_dev->Text(buf, x + 2, yinv(y + (p->d - hh)/2), 0);
                    snprintf(buf, sizeof(buf), "%d", p->v1[j]+d1);
                    gr_dev->TextExtent(buf, &ww, &hh);
                    gr_dev->Text(buf, x + (p->d - ww)/2, yinv(y + hh/4), 0);
                }
                else {
                    snprintf(buf, sizeof(buf), "%d", j);
                    int ww, hh;
                    gr_dev->TextExtent(buf, &ww, &hh);
                    gr_dev->Text(buf, x + (p->d - ww)/2, yinv(y + hh/4), 0);
                }
            }
        }
        if (p->pf[j]) {
            gr_dev->SetColor(invclr ? gr_colors[0].pixel : gr_colors[3].pixel);
            mp_pbox(x+1, y+1, x+p->d-1, y+p->d-1);
        }
        else {
            gr_dev->SetColor(invclr ? gr_colors[0].pixel : gr_colors[2].pixel);
            mp_xbox(x+1, y+1, x+p->d-1, y+p->d-1);
        }
    }

    // Show a logo on the plot.
    gr_show_logo();

    return (false);
}


void
sGraph::mp_initdata()
{
    gr_dev->TextExtent(0, &gr_fontwid, &gr_fonthei);
}


sChkPts *
sGraph::mp_copy_data()
{
    sChkPts *p = static_cast<sChkPts*>(gr_plotdata);
    sChkPts *newc = new sChkPts;
    *newc = *p;
    newc->param1 = lstring::copy(newc->param1);
    newc->param2 = lstring::copy(newc->param2);
    newc->date = lstring::copy(newc->date);
    newc->filename = lstring::copy(newc->filename);
    newc->plotname = lstring::copy(newc->plotname);
    newc->v1 = new char[newc->size];
    newc->v2 = new char[newc->size];
    newc->pf = new char[newc->size];
    memcpy(newc->v1, p->v1, newc->size);
    memcpy(newc->v2, p->v2, newc->size);
    memcpy(newc->pf, p->pf, newc->size);
    if (newc->sel) {
        newc->sel = new char[newc->size];
        memcpy(newc->sel, p->sel, newc->size);
    }
    newc->next = 0;
    return (newc);
}


// Free data procedure.
//
void
sGraph::mp_destroy_data()
{
    sChkPts *p = static_cast<sChkPts*>(gr_plotdata);
    delete p;
    gr_plotdata = 0;
}


void
sGraph::mp_xbox(int xl, int yl, int xu, int yu)
{
    gr_linebox(xl, yl, xu, yu);
    yl = yinv(yl);
    yu = yinv(yu);
    gr_dev->Line(xl, yl, xu, yu);
    gr_dev->Line(xl, yu, xu, yl);
}


void
sGraph::mp_pbox(int xl, int yl, int xu, int yu)
{
    gr_linebox(xl, yl, xu, yu);
    gr_dev->Line(
        xl+(xu-xl)/4, yinv(yl+(yu-yl)/2),
        xu-(xu-xl)/4, yinv(yl+(yu-yl)/2));
    gr_dev->Line(
        xl+(xu-xl)/2, yinv(yl+(yu-yl)/4),
        xl+(xu-xl)/2, yinv(yu-(yu-yl)/4));
}


// Write floating point number on screen.
// int j;  'r' right justify, else left justify
//
void
sGraph::mp_writeg(double d, int x, int y, int j)
{
    char t[20];
    snprintf(t, sizeof(t), "%g", d);
    char *tt;
    if ((tt = strchr(t, 'e')) != 0) {
        tt += 1;
        int sn;
        if (*tt == '-')
            sn = -1;
        else
            sn = 1;
        tt++;
        int xp = atoi(tt);
        if (xp > 99) {
            if (sn > 0)
                xp = 99;
            else {  
                if (j == 'r')
                    gr_dev->Text("0", x - 2*gr_fontwid, yinv(y), 0);
                else
                    gr_dev->Text("0", x, yinv(y), 0);
                return;
            }       
        }
        snprintf(tt, 14, "%02d", xp);
    }           
    if (j == 'r')
        gr_dev->Text(t, x - (int)strlen(t)*gr_fontwid, yinv(y), 0);
    else
        gr_dev->Text(t, x, yinv(y), 0);
}
// End of sGraph functions.


// Obtain the parsed tokens after "[DATA]".  Return false if not a
// data line, or bad data.
//
bool
SPgraphics::MpParse(const char *str, int *d1, int *d2, char *s1, char *s2,
    char *s3) const
{
    const char *end = str + strlen(str) - 6;
    while (str < end) {
        if (*str++ != '[')
            continue;
        if (*str++ != 'D')
            continue;
        if (*str++ != 'A')
            continue;
        if (*str++ != 'T')
            continue;
        if (*str++ != 'A')
            continue;
        if (*str++ != ']')
            continue;
        int j = sscanf(str, "%d %d %s %s %s", d1, d2, s1, s2, s3);
        if (j != 5 || (strcmp(s3, "PASS") && strcmp(s3, "FAIL")))
            return (false);
        return (true);
    }
    return (false);
}


// The next four functions are called from the margin analysis driver.


// Initialize the operating range analysis plotting functions.
// Return the id.
//
int
SPgraphics::MpInit(int delta1, int delta2, double v1min, double v1max,
    double v2min, double v2max, bool monte, const sPlot *plot)
{
    if (!spg_mplotOn)
        return (0);
    sGraph *graph = NewGraph(GR_MPLT, GR_MPLTstr);
    if (!graph) {
        GRpkg::self()->ErrPrintf(ET_ERROR, errmsg_gralloc);
        return (0);
    }
    sChkPts *p0 = new sChkPts;
    if (plot)
        p0->plotname = lstring::copy(plot->type_name());
    p0->date = lstring::copy(datestring());
    p0->delta1 = delta1;
    p0->delta2 = delta2;
    p0->minv1 = v1min;
    p0->maxv1 = v1max;
    p0->minv2 = v2min;
    p0->maxv2 = v2max;
    p0->type = (monte ? MDmonte : MDnormal);
    graph->set_plotdata(p0);
    graph->set_apptype(GR_MPLT);
 
    if (graph->gr_dev_init()) {
        GRpkg::self()->ErrPrintf(ET_ERROR, errmsg_newvp);
        graph->halt();
        DestroyGraph(graph->id());
        return (0);
    }

    // Show a logo on the plot.
    graph->gr_show_logo();
    
    if (GRpkg::self()->CurDev()->devtype != GRmultiWindow)
        graph->gr_redraw_direct();
    spg_echogr = graph;

    return (graph->id());
}


// Define the lower left corner of the marker array.
//
int
SPgraphics::MpWhere(int id, int d1, int d2)
{
    if (!spg_mplotOn || !id)
        return (1);
    sGraph *graph = FindGraph(id);
    if (!graph)
        return (1);
    sChkPts *p = static_cast<sChkPts*>(graph->plotdata());
    p->d1 = d1;
    p->d2 = d2;
    return (0);
}


// Plot a data point.
//
int
SPgraphics::MpMark(int id, char pf)
{
    if (!spg_mplotOn || !id)
        return (1);
    sGraph *graph = FindGraph(id);
    if (!graph)
        return (1);
    graph->mp_mark(pf);
    return (0);
}


// Called after all data points have been output.
//
int
SPgraphics::MpDone(int id)
{
    spg_echogr = 0;
    if (!spg_mplotOn || !id)
        return (1);
    sGraph *graph = FindGraph(id);
    if (GRpkg::self()->CurDev()->devtype == GRmultiWindow) {
        if (graph)
            graph->gr_redraw();  // for backing store
        return (0);
    }
    if (!graph)
        return (1);
    sChkPts *p = static_cast<sChkPts*>(graph->plotdata());
    if (GRpkg::self()->CurDev()->devtype == GRfullScreen)
        graph->mp_prompt(p);
    graph->halt();
    DestroyGraph(id);
    return (0);
}
// End of SPgraphics functions


namespace {
  // Fill in checkpoints based on the dimensions of the multi-dimensional
  // vector, for use in setting up a mask for plotting the vector.
  //
  sChkPts *get_fromvec(sDataVec *v)
  {
#define MAXINROW 15
    sPlot *pl = v->plot();
    if (v->numdims() <= 1 &&
            !(pl && pl->dimensions() && pl->dimensions()->v1()))
        return (0);
    sChkPts *p = new sChkPts;
    p->type = MDdimens;
    p->filename = lstring::copy(v->name());
    p->plotname = lstring::copy(pl ? pl->type_name() : 0);
    if (pl && pl->date())
        p->date = lstring::copy(pl->date());
    else
        p->date = lstring::copy("");
    if (pl && pl->dimensions() && pl->dimensions()->v1()) {
        // data came from range/monte analysis - it has dimension 2, and is
        // in the search order for range analysis
        p->flat = true;
        p->pfset = true;
        sDimen *dm = pl->dimensions();
        p->delta1 = dm->delta1();
        if (p->delta1 < 1)
            p->delta1 = 1;
        p->delta2 = dm->delta2();
        if (p->delta2 < 1)
            p->delta2 = 1;
        int sz = dm->size();
        p->v1 = new char[sz];
        memcpy(p->v1, dm->v1(), sz);
        p->v2 = new char[sz];
        memcpy(p->v2, dm->v2(), sz);
        p->pf = new char[sz];
        memcpy(p->pf, dm->pf(), sz);
        p->size = sz;
    }
    else {
        // data came from loop or nested dc analysis, or some other source
        int x1 = 1;
        int y1 = 1;
        int sz = 0;
        if (v->numdims() == 2) {
            x1 = v->dims(0);
            sz = x1;
            if (x1 > MAXINROW) {
                y1 = x1/MAXINROW;
                x1 = MAXINROW;
                p->flat = true;
            }
        }
        else if (v->numdims() >= 3) {
            y1 = v->dims(0);
            x1 = v->dims(1);
            sz = x1*y1;
        }
        int d1 = x1/2;
        int d2 = y1/2;

        p->next = 0;
        p->delta1 = 2*d1 + 1;
        p->delta2 = 2*d2 + 1;
        p->v1 = new char[sz];
        p->v2 = new char[sz];
        p->pf = new char[sz];
        p->size = sz;

        int k = 0;
        for (int i = 0; i < y1; i++) {
            for (int j = 0; j < x1; j++) {
                if (k < sz) {
                    p->v1[k] = j - d1;
                    p->v2[k] = i - d2;
                    p->pf[k] = 0;
                }
                k++;
            }
        }
    }
    if (pl && pl->dimensions()) {
        sDimen *dm = pl->dimensions();
        p->param1 = lstring::copy(dm->varname1());
        p->minv1 = dm->start1();
        p->maxv1 = p->minv1 + (p->delta1 - 1)*dm->step1();
        p->param2 = lstring::copy(dm->varname2());
        p->minv2 = dm->start2();
        p->maxv2 = p->minv2 + (p->delta2 - 1)*dm->step2();
    }
    return (p);
  }


  // Extract points from file.
  //
  void extpts(FILE *fp, sChkPts **p0, const char *fname)
  {
    int i = 0, d1, d2;
    char buf[BSIZE_SP];
    char string1[80], string2[80], string3[80];
    while (fgets(buf, 80, fp))
        if (GP.MpParse(buf, &d1, &d2, string1, string2, string3))
            i++;
    rewind(fp);
    if (!i) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "no data points in file %s.\n", fname);
        return;
    }

    sChkPts *p = new sChkPts;
    p->v1 = new char[i];
    p->v2 = new char[i];
    p->pf = new char[i];
    p->size = i;
    p->next = 0;
    p->plotname = sCHECKprms::mfilePlotname(lstring::strip_path(fname));

    double maxv1 = 0, minv1 = 0, maxv2 = 0, minv2 = 0;
    bool firstpt = true;
    while (fgets(buf, 80, fp)) {
        char *s, *t;
        if (!GP.MpParse(buf, &d1, &d2, string1, string2, string3)) {
            if (lstring::ciprefix("Date:", buf)) {
                s = buf + 5;
                while (isspace(*s)) s++;
                if (*s) {
                    t = s + strlen(s) - 1;
                    while (t > s && isspace(*t)) *t-- = '\0';
                    p->date = lstring::copy(s);
                }
            }
            else if (lstring::ciprefix("File:", buf)) {
                s = buf + 5;
                while (isspace(*s)) s++;
                if (*s) {
                    t = s + strlen(s) - 1;
                    while (t > s && isspace(*t)) *t-- = '\0';
                    // name of input file, not data file
                    p->filename = lstring::copy(s);
                }
            }
            else if (lstring::ciprefix("Parameter 1:", buf)) {
                s = buf + 12;
                while (isspace(*s)) s++;
                if (*s) {
                    t = s + strlen(s) - 1;
                    while (t > s && isspace(*t)) *t-- = '\0';
                    p->param1 = lstring::copy(s);
                }
            }
            else if (lstring::ciprefix("Parameter 2:", buf)) {
                s = buf + 12;
                while (isspace(*s)) s++;
                if (*s) {
                    t = s + strlen(s) - 1;
                    while (t > s && isspace(*t)) *t-- = '\0';
                    p->param2 = lstring::copy(s);
                }
            }
            continue;
        }
        if (firstpt) {
            double v1 = 0, v2 = 0;
            if (string1[0] == 'r' && string1[1] == 'u' && string1[2] == 'n')
                p->type = MDmonte;
            else {
                v1 = atof(string1);
                v2 = atof(string2);
            }
            i = 1;
            maxv1 = minv1 = v1;
            maxv2 = minv2 = v2;
            p->delta1 = -2*d1 + 1;
            p->delta2 = -2*d2 + 1;
            (p->v1)[0] = d1;
            (p->v2)[0] = d2;
            if (!strcmp(string3, "PASS"))
                (p->pf)[0] = 1;
            else
                (p->pf)[0] = 0;
            firstpt = false;
            continue;
        }
        if (p->type == MDnormal) {
            double v1 = atof(string1);
            double v2 = atof(string2);
            if (v1 > maxv1) maxv1 = v1;
            if (v1 < minv1) minv1 = v1;
            if (v2 > maxv2) maxv2 = v2;
            if (v2 < minv2) minv2 = v2;
        }
        (p->v1)[i] = d1;
        (p->v2)[i] = d2;
        if (!strcmp(string3, "PASS"))
            (p->pf)[i] = 1;
        else
            (p->pf)[i] = 0;
        i++;
    }
    p->minv1 = minv1;
    p->maxv1 = maxv1;
    p->minv2 = minv2;
    p->maxv2 = maxv2;
    if (p0 && !*p0)
         *p0 = p;
    else {
        sChkPts *q;
        for (q = *p0; q->next; q = q->next) ;
        q->next = p;
    }
  }


  // Combine sets of data points into a single plot.
  //
  void combin(sChkPts *p0)
  {
    sChkPts *p;
    for (p = p0; p; p = p->next) {
        if (p->type == MDmonte) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "can't combine Monte Carlo data.\n");
            return;
        }
    }
    
    double d1 = (p0->maxv1 - p0->minv1)/(p0->delta1 - 1);
    double d2 = (p0->maxv2 - p0->minv2)/(p0->delta2 - 1);
    sChkPts *q;
    for (p = p0->next; p; p = q) {
        double maxv1 = (p->maxv1 - p->minv1)/(p->delta1 - 1);
        double maxv2 = (p->maxv2 - p->minv2)/(p->delta2 - 1);
        if (fabs((d1-maxv1)/(d1+maxv1)) + fabs((d2-maxv2)/(d2+maxv2)) > 1e-6) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "can't combine, incompatible data.\n");
            getchar();
            q = p->next;
            delete p;
            continue;
        }
        maxv1 = (p0->maxv1 > p->maxv1 ? p0->maxv1 : p->maxv1);
        double minv1 = (p0->minv1 < p->minv1 ? p0->minv1 : p->minv1);
        maxv2 = (p0->maxv2 > p->maxv2 ? p0->maxv2 : p->maxv2);
        double minv2 = (p0->minv2 < p->minv2 ? p0->minv2 : p->minv2);
        double temp = -(maxv1 + minv1)/d1 + 2*p->minv1/d1 + p->delta1 - 1;
        if (temp > 0) temp += .5;
        else temp -= .5;
        int o1 = (int)temp;
        if (o1 & 1) o1--;
        o1 >>= 1;
        temp =  -(maxv2 + minv2)/d2 + 2*p->minv2/d2 + p->delta2 - 1;
        if (temp > 0) temp += .5;
        else temp -= .5;
        int o2 = (int)temp;
        if (o2 & 1) o2--;
        o2 >>= 1;
        temp =  -(maxv1 + minv1)/d1 + 2*p0->minv1/d1 + p0->delta1 - 1;
        if (temp > 0) temp += .5;
        else temp -= .5;
        int o3 = (int)temp;
        if (o3 & 1) o3--;
        o3 >>= 1;
        temp =  -(maxv2 + minv2)/d2 + 2*p0->minv2/d2 + p0->delta2 - 1;
        if (temp > 0) temp += .5;
        else temp -= .5;
        int o4 = (int)temp;
        if (o4 & 1) o4--;
        o4 >>= 1;
        p0->delta1 = (int)((maxv1 - minv1)/d1 + 1.5);
        p0->delta2 = (int)((maxv2 - minv2)/d2 + 1.5);
        for (int i = 0; i < p0->size; i++) {
            p0->v1[i] += o3;
            p0->v2[i] += o4;
        }
        Realloc(&p0->v1, p0->size + p->size, p0->size);
        Realloc(&p0->v2, p0->size + p->size, p0->size);
        Realloc(&p0->pf, p0->size + p->size, p0->size);
        char *p1 = p0->v1 + p0->size;
        char *p2 = p0->v2 + p0->size;
        char *p3 = p0->pf + p0->size;
        p0->size += p->size;
        for (int i = 0; i < p->size; i++) {
            *p1++ = p->v1[i] + o1;
            *p2++ = p->v2[i] + o2;
            *p3++ = p->pf[i];
        }
        p0->minv1 = minv1;
        p0->maxv1 = maxv1;
        p0->minv2 = minv2;
        p0->maxv2 = maxv2;
        q = p->next;
        delete p;
    }
    p0->next = 0;
  }


  // Save points for redraw during interactive plot.
  //
  void addit(sChkPts *p, int v1, int v2, char pf)
  {
    if (p->size == 0) {
        p->rsize = 100;
        p->v1 = new char[p->rsize];
        p->v2 = new char[p->rsize];
        p->pf = new char[p->rsize];
    }
    if (p->size >= p->rsize) {
        Realloc(&p->v1, p->rsize + 100, p->rsize);
        Realloc(&p->v2, p->rsize + 100, p->rsize);
        Realloc(&p->pf, p->rsize + 100, p->rsize);
        p->rsize += 100;
    }
    p->v1[p->size] = (char) v1;
    p->v2[p->size] = (char) v2;
    p->pf[p->size] = pf;
    p->size++;
  }
}

