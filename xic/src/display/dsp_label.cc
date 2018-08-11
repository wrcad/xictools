
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

#include "dsp.h"
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "cd_hypertext.h"
#include "cd_lgen.h"
#include "miscutil/texttf.h"
#include "ginterf/grfont.h"


//
// Functions to render labels and to establish label size.
//

int
cDisplay::DefaultLabelSize(const char *text, DisplayMode mode, int *width,
    int *height)
{
    int lwid, lhei;
    int nlines = LabelExtent(text, &lwid, &lhei);
    if (mode == Physical) {
        *width = INTERNAL_UNITS(
            (CDphysDefTextWidth * lwid)/FT.cellWidth());
        *height = INTERNAL_UNITS(
            (CDphysDefTextHeight * lhei)/FT.cellHeight());
    }
    else {
        *width = ELEC_INTERNAL_UNITS(
            (CDelecDefTextWidth * lwid)/FT.cellWidth());
        *height = ELEC_INTERNAL_UNITS(
            (CDelecDefTextHeight * lhei)/FT.cellHeight());
    }
    return (nlines);
}


int
cDisplay::DefaultLabelSize(hyList *htext, DisplayMode mode, int *width,
    int *height)
{
    int lwid, lhei;
    int nlines = LabelExtent(htext, &lwid, &lhei);
    if (mode == Physical) {
        *width = INTERNAL_UNITS(
            (CDphysDefTextWidth * lwid)/FT.cellWidth());
        *height = INTERNAL_UNITS(
            (CDphysDefTextHeight * lhei)/FT.cellHeight());
    }
    else {
        *width = ELEC_INTERNAL_UNITS(
            (CDelecDefTextWidth * lwid)/FT.cellWidth());
        *height = ELEC_INTERNAL_UNITS(
            (CDelecDefTextHeight * lhei)/FT.cellHeight());
    }
    return (nlines);
}


int
cDisplay::LabelExtent(const char *text, int *width, int *height)
{
    int numlines;
    FT.textExtent(text, width, height, &numlines);
    return (numlines);
}


int
cDisplay::LabelExtent(hyList *text, int *width, int *height)
{
    int numlines;
    if (!text) {
        FT.textExtent(0, width, height, &numlines);
        return (numlines);
    }
    char *str = hyList::string(text, HYcvPlain, false);
    FT.textExtent(str, width, height, &numlines);
    delete [] str;
    return (numlines);
}


int
cDisplay::LabelSize(const char *text, DisplayMode mode, int *width,
    int *height)
{
    int lwid, lhei;
    int nlines = LabelExtent(text, &lwid, &lhei);
    if (mode == Physical) {
        *width = INTERNAL_UNITS(
            (d_phys_char_width * lwid)/FT.cellWidth());
        *height = INTERNAL_UNITS(
            (d_phys_char_height * lhei)/FT.cellHeight());
    }
    else {
        *width = ELEC_INTERNAL_UNITS(
            (d_elec_char_width * lwid)/FT.cellWidth());
        *height = ELEC_INTERNAL_UNITS(
            (d_elec_char_height * lhei)/FT.cellHeight());
    }
    return (nlines);
}


int
cDisplay::LabelSize(hyList *htext, DisplayMode mode, int *width, int *height)
{
    int lwid, lhei;
    int nlines = LabelExtent(htext, &lwid, &lhei);
    if (mode == Physical) {
        *width = INTERNAL_UNITS(
            (d_phys_char_width * lwid)/FT.cellWidth());
        *height = INTERNAL_UNITS(
            (d_phys_char_height * lhei)/FT.cellHeight());
    }
    else {
        *width = ELEC_INTERNAL_UNITS(
            (d_elec_char_width * lwid)/FT.cellWidth());
        *height = ELEC_INTERNAL_UNITS(
            (d_elec_char_height * lhei)/FT.cellHeight());
    }
    return (nlines);
}


void
cDisplay::LabelResize(const char *newlab, const char *oldlab, int *width,
    int *height)
{
    int wo, ho, wn, hn;
    LabelExtent(oldlab, &wo, &ho);
    LabelExtent(newlab, &wn, &hn);
    if (!oldlab)
        *width = (*height * wn)/ho;
    else {
        *height = (*height * hn)/ho;
        *width = (*width * wn)/wo;
    }
}


void
cDisplay::LabelResize(hyList *newlab, hyList *oldlab, int *width,
    int *height)
{
    int wo, ho, wn, hn;
    LabelExtent(oldlab, &wo, &ho);
    LabelExtent(newlab, &wn, &hn);
    if (!oldlab)
        *width = (*height * wn)/ho;
    else {
        *height = (*height * hn)/ho;
        *width = (*width * wn)/wo;
    }
}


// Set up the transform for the label.
//
void
cDisplay::LabelSetTransform(int xform, SLtype slmode, int *x, int *y,
    int *width, int *height)
{
    TSetTransformFromXform(xform, *width, *height);
    TTranslate(*x, *y);
    TPremultiply();

    if (slmode == SLupright) {
        // create a transformation so that the text is in "legible"
        // orientation, yet resides in the given bounding box
        int w = *width;
        int h = *height;
        Point px[4];
        px[0].set(0, 0);
        px[1].set(0, h);
        px[2].set(w, h);
        px[3].set(w, 0);
        TPath(4, px);

        double magn = TGetMagn();
        if (magn != 1.0) {
            w = mmRnd(w*magn);
            h = mmRnd(h*magn);
        }

        // "x" vector
        int vx = px[3].x - px[0].x;
        int vy = px[3].y - px[0].y;
        TPush();
        if (vy && !vx) {
            // 90 or 270 rotation
            TRotate(0, 1);
            *x = mmMax(px[0].x, px[2].x);
            *y = mmMin(px[0].y, px[2].y);
        }
        else if (vy && vx) {
            // one of the 45's
            int i = 0;
            if ((vx > 0 && vy > 0) || (vx < 0 && vy < 0)) {
                TRotate(1, 1);
                if (px[1].y < px[i].y)
                    i = 1;
                if (px[2].y < px[i].y)
                    i = 2;
                if (px[3].y < px[i].y)
                    i = 3;
            }
            else {
                TRotate(1, -1);
                if (px[1].x < px[i].x)
                    i = 1;
                if (px[2].x < px[i].x)
                    i = 2;
                if (px[3].x < px[i].x)
                    i = 3;
            }
            *x = px[i].x;
            *y = px[i].y;
        }
        else {
            // no rotation
            *x = mmMin(px[0].x, px[2].x);
            *y = mmMin(px[0].y, px[2].y);
        }
        if (w < 0)
            w = -w;
        if (h < 0)
            h = -h;
        *width = w;
        *height = h;
        TTranslate(*x, *y);
    }
}


// Resize and redraw all unbound labels that contain a hypertext link. 
// This is called after a modification which could change the label
// text.
//
void
cDisplay::LabelHyUpdate()
{
    if (DSP()->CurMode() != Electrical)
        return;
    CDlgen lgen(Electrical);
    CDl *ld;
    while ((ld =  lgen.next()) != 0) {
        if (ld->isInvisible())
            continue;

        CDg gdesc;
        gdesc.init_gen(CurCell(Electrical), ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (odesc->type() != CDLABEL)
                continue;
            CDla *ladesc = (CDla*)odesc;
            if (ladesc->prpty(P_LABRF))
                continue;

            hyList *hl = ladesc->label();
            if (!hl || hl->ref_type() == HLrefLongText || (!hl->next() &&
                    hl->ref_type() == HLrefText))
                continue;

            bool do_it = false;
            for (hyList *h = hl; h; h = h->next()) {
                if (h->ref_type() & (HLrefNode | HLrefBranch | HLrefDevice)) {
                    do_it = true;
                    break;
                }
            }
            if (!do_it)
                continue;
            int wid, hei;
            DSP()->DefaultLabelSize(hl, Electrical, &wid, &hei);
            double r = (double)ladesc->height()/hei;
            ladesc->set_width(mmRnd(r*wid));
            BBox BB(ladesc->oBB());
            ladesc->computeBB();
            BB.add(&ladesc->oBB());
            DSP()->RedisplayArea(&BB);
        }
    }
}
// End of cDisplay functions.


void
WindowDesc::ShowLabel(const Label *label)
{
    if (!label || !label->label)
        return;
    show_label(label->label, label->x, label->y, label->width, label->height,
        label->xform, true, &FT);
    if (label->label->is_label_script())
        ShowLabelOutline(label->x, label->y, label->width, label->height,
            label->xform);
}


void
WindowDesc::ShowLabel(const char *label, int x, int y, int width,
    int height, int xform, GRvecFont *ft)
{
    if (!label)
        return;
    show_label(label, x, y, width, height, xform, false, ft);
}


void
WindowDesc::ShowLabel(const hyList *label, int x, int y,
    int width, int height, int xform, GRvecFont *ft)
{
    if (!label)
        return;
    show_label(label, x, y, width, height, xform, true, ft);
    if (label->is_label_script())
        ShowLabelOutline(x, y, width, height, xform);
}


// Utility to write some text directly on the viewport, x and y are
// viewport coords.  Used in the terminal and plot markers.
//
void
WindowDesc::ViewportText(const char *label, int x, int y, double scale,
    bool up)
{
    if (!label || !*label)
        return;
    if (!w_draw)
        return;
    int xos = (int)scale;
    int yos = 0;
    if (!up) {
        y -= (int)(scale*FT.cellHeight());
        for (const char *s = label; *s; s++) {
            GRvecFont::Character *cp = FT.entry(*s);
            if (cp) {
                for (int i = 0; i < cp->numstroke; i++) {
                    GRvecFont::Cstroke *stroke = &cp->stroke[i];
                    int x0 = (int)(scale*(stroke->cp[0].x - cp->ofst)) + xos;
                    int y0 = (int)(scale*stroke->cp[0].y) + yos;
                    for (int j = 1; j < stroke->numpts; j++) {
                        int x1 = (int)(scale*(stroke->cp[j].x - cp->ofst)) +
                            xos;
                        int y1 = (int)(scale*stroke->cp[j].y) + yos;
                        int x0p = x0 + x;
                        int y0p = y0 + y;
                        int x1p = x1 + x;
                        int y1p = y1 + y;
                        if (!cGEO::line_clip(&x0p, &y0p, &x1p, &y1p,
                                &w_clip_rect))
                            w_draw->Line(x0p, y0p, x1p, y1p);
                        x0 = x1;
                        y0 = y1;
                    }
                }
                xos += (int)(scale*cp->width);
            }
            else if (*s == '\n') {
                xos = (int)scale;
                yos += (int)(scale*(FT.cellHeight()+2));
            }
            else
                xos += (int)(scale*FT.cellWidth());
        }
    }
    else {
        for (const char *s = label; *s; s++) {
            GRvecFont::Character *cp = FT.entry(*s);
            if (cp) {
                for (int i = 0; i < cp->numstroke; i++) {
                    GRvecFont::Cstroke *stroke = &cp->stroke[i];
                    int y0 = -(int)(scale*(stroke->cp[0].x - cp->ofst)) + yos;
                    int x0 = (int)(scale*stroke->cp[0].y) + xos;
                    for (int j = 1; j < stroke->numpts; j++) {
                        int y1 = -(int)(scale*(stroke->cp[j].x - cp->ofst)) +
                            yos;
                        int x1 = (int)(scale*stroke->cp[j].y) + xos;
                        int x0p = x0 + x;
                        int y0p = y0 + y;
                        int x1p = x1 + x;
                        int y1p = y1 + y;
                        if (!cGEO::line_clip(&x0p, &y0p, &x1p, &y1p,
                                &w_clip_rect))
                            w_draw->Line(x0p, y0p, x1p, y1p);
                        x0 = x1;
                        y0 = y1;
                    }
                }
                yos -= (int)(scale*cp->width);
            }
            else if (*s == '\n') {
                yos = 0;
                xos += (int)(scale*(FT.cellHeight()+2));
            }
            else
                yos -= (int)(scale*FT.cellWidth());
        }
    }
}


// Draw an outline around a label.
//
void
WindowDesc::ShowLabelOutline(int x, int y, int width, int height, int xform)
{
    Point *pts = 0;
    BBox tBB(x, y, x + width, y + height);
    Label::TransformLabelBB(xform, &tBB, &pts);
    if (pts) {
        ShowPath(pts, 5, true);
        delete [] pts;
    }
    else
        ShowBox(&tBB, CDL_OUTLINED, 0);
}


namespace {
    const char *rotns[] =
    {
        "90",
        "180",
        "270",
        "45",
        "135",
        "225",
        "315"
    };

    // Return a code describing the current rotations/reflections.
    // If buf, fill in the string with the transform info.
    //
    int
    get_codestr(char *buf)
    {
        CDtf tf;
        DSP()->TCurrent(&tf);
        int a, b, c, d;
        tf.abcd(&a, &b, &c, &d);

        // Set code according to current rotation/reflection.
        int code = 0;
        if ((a && (a == -d)) || (b && (b == c))) {
            // tm is reflected, un-reflect and get T
            code = 8;
            c = -c;
        }
        if (!a && c) {
            if (c > 0)
                code += 1;       // 90
            else
                code += 3;       // 270
        }
        else if (a && !c) {
            if (a > 0)
                code += 0;       // 0
            else
                code += 2;       // 180
        }
        else {
            if (a > 0) {
                if (c > 0)
                    code += 4;   // 45
                else
                    code += 7;   // 315
            }
            else {
                if (c > 0)
                    code += 5;   // 135
                else
                    code += 6;   // 225
            }
        }
        if (buf) {
            char *s = buf;
            if (tf.mag() > 0 && tf.mag() != 1.0) {
                buf[0] = 'X';
                s = mmDtoA(buf+1, tf.mag(), 3);
            }
            if (code & 7) {
                *s++ = 'R';
                const char *t = rotns[(code & 7)-1];
                while ((*s = *t++) != 0)
                    s++;
            }
            if (code & 8)
                *s++ = 'M';
            *s = 0;
        }
        return (code);
    }
}


// Write str1/str2 into the instance bounding box area.
//
void
WindowDesc::ShowUnexpInstanceLabel(double *magn, const BBox *BB,
    const char *str1, const char *str2, int sltype)
{
    if (sltype == SLnone)
        return;
    if (BB->left == CDinfinity)
        return;

    int bw = BB->width();
    int bh = BB->height();
    if (magn) {
        bw = (int)(bw* *magn);
        bh = (int)(bh* *magn);
    }
    bool nofix = (sltype == SLtrueOrient);

    char buf[64];
    int code = get_codestr(nofix ? 0 : buf);
    if (!nofix && *buf)
        str2 = buf;
    if (str2 && !*str2)
        str2 = 0;

again:
    // find the best orientation for text
    int w1, h1;
    DSP()->DefaultLabelSize(str1, w_mode, &w1, &h1);
    int w2, h2;
    if (str2)
        DSP()->DefaultLabelSize(str2, w_mode, &w2, &h2);
    else
        h2 = w2 = 0;
    int h = h1 + h2;
    int w = mmMax(w1, w2);

    // scale to readable size
    double hp = 4*FT.cellHeight()/w_ratio;
    if (h < hp) {
        w1 = (int)(w1*hp/h);
        if (h2) {
            w2 = (int)(w2*hp/h);
            w = mmMax(w1, w2);
            h = (int)hp;
            h1 = h2 = h/2;
        }
        else {
            h = h1 = (int)hp;
            w = w1;
        }
    }

    if (w < bw && h < bh) {
        // fits
        show_inst_text(code, str1, str2, BB, w1, w2, h1, false, nofix);
        return;
    }
    if (w < bh && h < bw && !nofix) {
        // fits if rotated
        show_inst_text(code, str1, str2, BB, w1, w2, h1, true, nofix);
        return;
    }

    // scale to fit
    double r;
    int mx, mn;
    if (nofix) {
        if (w > bw)
            r = (double)bw/w;
        else
            r = 1.0;
        if (h > bh) {
            if (r > (double)bh/h)
                r = (double)bh/h;
        }
        w1 = (int)(w1 * r);
        w2 = (int)(w2 * r);
        h1 = (int)(h1 * r);
        mx = 0;
    }
    else {
        mx = mmMax(bw, bh);
        mn = mmMin(bw, bh);
        if (w > h) {
            if (mx < w) {
                r = (double)mx/w;
                h = (int)(h * r);
                h1 = (int)(h1 * r);
                w1 = (int)(w1 * r);
                w2 = (int)(w2 * r);
                w = mx;
            }
            if (mn < h) {
                r = (double)mn/h;
                w = (int)(w * r);
                h1 = (int)(h1 * r);
                w1 = (int)(w1 * r);
                w2 = (int)(w2 * r);
                h = mn;
            }
        }
        else {
            if (mx < h) {
                r = (double)mx/h;
                w = (int)(w * r);
                h1 = (int)(h1 * r);
                w1 = (int)(w1 * r);
                w2 = (int)(w2 * r);
                h = mx;
            }
            if (mn < w) {
                r = (double)mn/w;
                h = (int)(h * r);
                h1 = (int)(h1 * r);
                w1 = (int)(w1 * r);
                w2 = (int)(w2 * r);
                w = mn;
            }
        }
    }
    if (h1*w_ratio >= 8) {
        if ((mx == bw && w >= h) || (mx == bh && h >= w) || nofix)
            show_inst_text(code, str1, str2, BB, w1, w2, h1, false, nofix);
        else
            show_inst_text(code, str1, str2, BB, w1, w2, h1, true, nofix);
    }
    else if (str2) {
        str2 = 0;
        goto again;
    }
}


// Static function.
// Return true if the label is "too long" and should be shown as a
// marker instead.
//
bool
WindowDesc::LabelHideTest(CDla *la)
{
    HLmode hlm = DSP()->HiddenLabelMode();
    if (hlm  == HLnone)
        return (false);
    if (hlm != HLall && DSP()->CurMode() != Electrical)
        return (false);
    if (hlm == HLelprp && DSP()->CurMode() == Electrical) {
        CDp_lref *prf = (CDp_lref*)la->prpty(P_LABRF);
        if (!prf || !prf->devref())
            return (false);
    }

    int w, h;
    int nl = DSP()->LabelExtent(la->label(), &w, &h);
    h = la->height()/nl;  // Height of one text line.
    int cw = (h*FT.cellWidth())/FT.cellHeight();
    return (la->width() > DSP()->MaxLabelLen()*cw);
}


// Static function.
// Fill in the bounding box of the mark used for hidden labels.
//
void
WindowDesc::LabelHideBB(CDla *la, BBox *BB)
{
    int w, h;
    int nl = DSP()->LabelExtent(la->label(), &w, &h);
    h = la->height()/nl;  // Height of one text line.
    int cw = (h*FT.cellWidth())/FT.cellHeight();
    BB->left = la->xpos();
    BB->bottom = la->ypos();
    BB->right = la->xpos() + cw;
    BB->top = la->ypos() + cw;
    Label::TransformLabelBB(la->xform(), BB, 0);
}


// Static function.
// Handle redisplay of a hide/show flag status change.  Called from
// the main button press handler.  Return true if a label was found
// and processed, only one will be considered.
//
bool
WindowDesc::LabelHideHandler(CDol *slist)
{
    HLmode hlm = DSP()->HiddenLabelMode();
    if (hlm  == HLnone)
        return (false);
    if (hlm != HLall && DSP()->CurMode() != Electrical)
        return (false);

    for (CDol *sl = slist; sl; sl = sl->next) {
        if (sl->odesc->type() != CDLABEL)
            continue;
        CDla *la = (CDla*)sl->odesc;
        if (la->state() != CDobjVanilla)
            continue;

        int xform = la->xform();
        if (xform & TXTF_SHOW) {
            xform &= ~TXTF_SHOW;
            la->set_xform(xform);
            DSP()->RedisplayArea(&la->oBB(), DSP()->CurMode());
            return (true);
        }
        if (xform & TXTF_HIDE) {
            xform &= ~TXTF_HIDE;
            la->set_xform(xform);
            DSP()->RedisplayArea(&la->oBB(), DSP()->CurMode());
            return (true);
        }

        if (hlm == HLelprp && DSP()->CurMode() == Electrical) {
            CDp_lref *prf = (CDp_lref*)la->prpty(P_LABRF);
            if (!prf || !prf->devref())
                continue;
        }

        int w, h;
        int nl = DSP()->LabelExtent(la->label(), &w, &h);
        h = la->height()/nl;  // Height of one text line.
        int cw = (h*FT.cellWidth())/FT.cellHeight();
        if (la->width() > DSP()->MaxLabelLen()*cw)
            xform |= TXTF_SHOW;
        else
            xform |= TXTF_HIDE;
        la->set_xform(xform);
        DSP()->RedisplayArea(&la->oBB(), DSP()->CurMode());
            return (true);
    }
    return (false);
}


// Private function to render label text.
//
void
WindowDesc::show_label(const void *lptr, int x, int y, int width, int height,
    int xform, bool ishytext, const GRvecFont *ft)
{
    if (!w_draw)
        return;
    if (!ft)
        ft = &FT;
    SLtype slmode = w_attributes.display_labels(w_mode);
    if (!slmode)
        return;
    bool freelabel = false;
    const char *label;
    if (ishytext) {
        hyList *hlabel = (hyList*)lptr;
        if (!hlabel || (hlabel->ref_type() == HLrefText &&
                (!hlabel->text() || !hlabel->text()[0])))
            return;
        label = hyList::string(hlabel, HYcvPlain, false);
        freelabel = true;
    }
    else
        label = (const char*)lptr;
    if (!label)
        return;

    DSP()->LabelSetTransform(xform, slmode, &x, &y, &width, &height);

    if (DSP()->UseDriverLabels()) {
        // In hard copy rendering, use the hard copy driver's text routine
        // instead of the vector font.  This usually looks better in plots.
        const char *ss;
        if (!(ss = strchr(label, '\n')) || !*(ss+1)) {
            // Can only do this for single-line text
            CDtf tf;
            DSP()->TCurrent(&tf);
            x = 0;
            y = 0;
            DSP()->TPoint(&x, &y);
            xform = tf.get_xform();
            if (slmode == SLupright)
                DSP()->TPop();
            DSP()->TPop();
            LToP(x, y, x, y);
            width = (int)(width*w_ratio);
            height = (int)(height*w_ratio);
            w_draw->Text(label, x, y, xform, width, height);
            if (freelabel)
                delete [] label;
            return;
        }
    }

    w_draw->SetFillpattern(0);
    w_draw->SetLinestyle(0);
    int lwid, lhei, numlines;
    ft->textExtent(label, &lwid, &lhei, &numlines);
    width /= lwid;
    height /= lhei;
    int xos = ft->xoffset(label, xform, width, lwid);
    int yos = 0;

    if (numlines > 1) {
        if ((xform & TXTF_LIML) && DSP()->MaxLabelLines() > 0 &&
                numlines > DSP()->MaxLabelLines()) {
            int maxl = DSP()->MaxLabelLines();
            if (xform & TXTF_VJT)
                yos = (numlines-1)*height*ft->cellHeight();
            else if (xform & TXTF_VJC)
                yos = ((numlines + maxl - 2)*height*ft->cellHeight())/2;
            else
                yos = (maxl-1)*height*ft->cellHeight();
        }
        else
            yos = (numlines-1)*height*ft->cellHeight();
    }

    int lcnt = 0;
    for (const char *s = label; *s; s++) {
        GRvecFont::Character *cp = ft->entry(*s);
        if (cp) {
            for (int i = 0; i < cp->numstroke; i++) {
                GRvecFont::Cstroke *stroke = &cp->stroke[i];
                int x0 = width*(stroke->cp[0].x - cp->ofst) + xos;
                int y0 = height*(ft->cellHeight() - 1 -
                    stroke->cp[0].y) + yos;
                DSP()->TPoint(&x0, &y0);
                for (int j = 1; j < stroke->numpts; j++) {
                    int x1 = width*(stroke->cp[j].x - cp->ofst) + xos;
                    int y1 = height*(ft->cellHeight() - 1 -
                        stroke->cp[j].y) + yos;
                    DSP()->TPoint(&x1, &y1);
                    int x0p, y0p, x1p, y1p;
                    LToP(x0, y0, x0p, y0p);
                    LToP(x1, y1, x1p, y1p);
                    if (!cGEO::line_clip(&x0p, &y0p, &x1p, &y1p, &w_clip_rect))
                        w_draw->Line(x0p, y0p, x1p, y1p);
                    x0 = x1;
                    y0 = y1;
                }
            }
            xos += width*cp->width;
        }
        else if (*s == '\n') {
            if ((xform & TXTF_LIML) && DSP()->MaxLabelLines() > 0) {
                if (++lcnt == DSP()->MaxLabelLines())
                    break;
            }
            xos = ft->xoffset(s+1, xform, width, lwid);
            yos -= height*ft->cellHeight();
        }
        else
            xos += width*ft->cellWidth();
    }
    if (slmode == SLupright)
        DSP()->TPop();
    DSP()->TPop();
    if (freelabel)
        delete [] label;
}


// Private function to transform text so as to be in readable
// orientation.  Actually draws the instance labels.
// int code;        code describing current transform
// BBox *BB;        untransformed enclosing BB
// int w1, w2, h1;  label size params
// bool rot;        true when text is to be rotated
// bool nofix;      don't change transform
//
void
WindowDesc::show_inst_text(int code, const char *str1, const char *str2,
    const BBox *BB, int w1, int w2, int h1, bool rot, bool nofix)
{
    if (!w_draw)
        return;
    SLtype sltmp = w_attributes.display_labels(w_mode);
    w_attributes.set_display_labels(w_mode, SLtrueOrient);
    if (nofix) {
        w_draw->SetColor(DSP()->Color(InstanceNameColor, w_mode));
        ShowLabel(str1, BB->left, (BB->bottom + BB->top)/2, w1, h1, 0);
        if (str2 && *str2) {
            w_draw->SetColor(DSP()->Color(InstanceSizeColor, w_mode));
            ShowLabel(str2, BB->left, (BB->bottom + BB->top)/2 - h1,
                w2, h1, 0);
        }
    }
    else {
        Point p[4];
        p[0].set(BB->left, BB->bottom);
        p[1].set(BB->left, BB->top);
        p[2].set(BB->right, BB->top);
        p[3].set(BB->right, BB->bottom);
        DSP()->TPath(4, p);

        // set up a new transform for the labels
        DSP()->TPop();
        DSP()->TPush();

        int tx = 0, ty = 0;
        switch (code) {
        case 0:
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 1:
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
                DSP()->TRotate(0, 1);
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
            }
            break;
        case 2:
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 3:
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
                DSP()->TRotate(0, 1);
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
            }
            break;
        case 4:
            DSP()->TRotate(1, 1);
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
                DSP()->TRotate(0, -1);
            }
            break;
        case 5:
            DSP()->TRotate(1, -1);
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 6:
            DSP()->TRotate(1, 1);
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
                DSP()->TRotate(0, -1);
            }
            break;
        case 7:
            DSP()->TRotate(1, -1);
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 8:
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 9:
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
                DSP()->TRotate(0, 1);
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
            }
            break;
        case 10:
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 11:
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
                DSP()->TRotate(0, 1);
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
            }
            break;
        case 12:
            DSP()->TRotate(1, -1);
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 13:
            DSP()->TRotate(1, 1);
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
            }
            else {
                tx = (p[1].x + p[2].x)/2;
                ty = (p[1].y + p[2].y)/2;
                DSP()->TRotate(0, -1);
            }
            break;
        case 14:
            DSP()->TRotate(1, -1);
            if (!rot) {
                tx = (p[2].x + p[3].x)/2;
                ty = (p[2].y + p[3].y)/2;
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
                DSP()->TRotate(0, 1);
            }
            break;
        case 15:
            DSP()->TRotate(1, 1);
            if (!rot) {
                tx = (p[0].x + p[1].x)/2;
                ty = (p[0].y + p[1].y)/2;
            }
            else {
                tx = (p[0].x + p[3].x)/2;
                ty = (p[0].y + p[3].y)/2;
                DSP()->TRotate(0, -1);
            }
            break;
        }
        DSP()->TTranslate(tx, ty);
        w_draw->SetColor(DSP()->Color(InstanceNameColor, w_mode));
        ShowLabel(str1, 0, 0, w1, h1, (str2 && *str2) ? 0 : TXTF_VJC);
        if (str2 && *str2) {
            w_draw->SetColor(DSP()->Color(InstanceSizeColor, w_mode));
            ShowLabel(str2, 0, -h1, w2, h1, 0);
        }
    }
    w_attributes.set_display_labels(w_mode, sltmp);
}

