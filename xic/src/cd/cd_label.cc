
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

#include "cd.h"
#include "cd_types.h"
#include "cd_hypertext.h"
#include "cd_propnum.h"
#include "miscutil/texttf.h"
#include <ctype.h>


void
Label::computeBB(BBox *BB)
{
    BB->left = 0;
    BB->bottom = 0;
    BB->right = width;
    BB->top = height;
    cTfmStack stk;
    stk.TSetTransformFromXform(xform, width, height);
    stk.TTranslate(x, y);
    stk.TBB(BB, 0);
    stk.TPop();
}


bool
Label::intersect(const BBox *BB, bool tok)
{
    BBox tBB;
    computeBB(&tBB);
    return (tBB.intersect(BB, tok));
}


// Static function.
// Transform BB according to xform.  For non-manhattan, points is returned
// with the bounding shape enclosed by BB.
//
void
Label::TransformLabelBB(int xform, BBox *BB, Point **pts)
{
    int wid = BB->width();
    int hei = BB->height();
    int x = BB->left;
    int y = BB->bottom;
    BB->left = 0;
    BB->bottom = 0;
    BB->right = wid;
    BB->top = hei;
    cTfmStack stk;
    stk.TSetTransformFromXform(xform, wid, hei);
    stk.TTranslate(x, y);
    stk.TBB(BB, pts);
    stk.TPop();
}


// Static function.
// Get back the untransformed BB.  If xform is non-orthogonal, will
// take input from pts, otherwise from BB.  Box is returned in BB.
//
void
Label::InvTransformLabelBB(int xform, BBox *BB, Point *pts)
{
    // This is a bit subtle.  When translating back to the origin,
    // we have to subtract off the reference point coordinates.  For
    // the poly, this is always point 0.  For a box, it depends on the
    // transform.  We transform a test box to obtain the transformed
    // reference.
    //
    int x, y;
    if (pts) {
        x = pts[0].x;
        y = pts[0].y;
    }
    else {
        BBox tBB;
        tBB.left = tBB.bottom = 0;
        tBB.right = tBB.top = 1;
        TransformLabelBB(xform, &tBB, 0);
        if (tBB.right > 0) {
            if (tBB.top > 0) {
                // LL
                x = BB->left;
                y = BB->bottom;
            }
            else {
                // UL
                x = BB->left;
                y = BB->top;
            }
        }
        else {
            if (tBB.top > 0) {
                // LR
                x = BB->right;
                y = BB->bottom;
            }
            else {
                // UR
                x = BB->right;
                y = BB->top;
            }
        }
    }
    cTfmStack stk;
    stk.TSetTransformFromXform(xform, 0, 0);
    stk.TTranslate(x, y);
    stk.TInverse();
    stk.TLoadInverse();
    stk.TTranslate(x, y);

    if (pts) {
        *BB = CDnullBB;
        for (int i = 0; i < 5; i++) {
            stk.TPoint(&pts[i].x, &pts[i].y);
            if (BB->left > pts[i].x)
                BB->left = pts[i].x;
            if (BB->bottom > pts[i].y)
                BB->bottom = pts[i].y;
            if (BB->right < pts[i].x)
                BB->right = pts[i].x;
            if (BB->top < pts[i].y)
                BB->top = pts[i].y;
        }
    }
    else
        stk.TBB(BB, 0);
    stk.TPop();
}
// End of Label functions


// Copy the appropriately formatted label into buf.  If the label is too
// long, truncate and return false.  If size == 0, return a malloc'ed
// copy.
//
bool
CDla::format_string(char **buf, int size, hyList *ltext, bool longtxt,
    FileType ftype)
{
    bool toolong = false;
    char *str = hyList::string(ltext, HYcvAscii, longtxt);
    if (str) {
        if (size == 0) {
            sLstr lstr;
            if (ftype == Fnative)
                lstr.add("<<");
            lstr.add(str);
            if (ftype == Fnative)
                lstr.add(">>");
            *buf = lstr.string_trim();
        }
        else {
            int rsize;
            if (ftype == Fnative)
                rsize = size - 5;
            else
                rsize = size - 1;
            char *t = *buf;
            char *s = str;
            if (ftype == Fnative) {
                *t++ = '<';
                *t++ = '<';
            }
            for (int i = 0; *s; i++) {
                if (i + 1 >= rsize) {
                    toolong = true;
                    break;
                }
                *t++ = *s++;
            }
            if (ftype == Fnative) {
                *t++ = '>';
                *t++ = '>';
            }
            *t = 0;
        }
        for (char *s = *buf; *s; s++) {
            if (*s == ';')
                // should already have been encoded
                *s = (ftype == Fcif ? '_' : ' ');  // ; breaks CIF
            else if (ftype == Fcif && isspace(*s))
                // "real" cif has no spaces
                *s = '_';
        }
        delete [] str;
    }
    else {
        if (size) {
            if (ftype == Fnative)
                strcpy(*buf, "<<>>");
            else
                **buf = '\0';
        }
        else {
            if (ftype == Fnative)
                *buf = lstring::copy("<<>>");
            else
                *buf = lstring::copy("");
        }
    }
    if (toolong)
        return (false);
    return (true);
}


// Link the label to the given object/property.  Linked properties will
// be displayed on-screen, and can be used to modify values.
//
bool
CDla::link(CDs *sdesc, CDo *odesc, CDp *pdesc)
{
    char buf[128];
    if (pdesc && odesc && odesc->type() == CDINSTANCE) {
        // Device property label.

        CDp_cname *pn = (CDp_cname*)odesc->prpty(P_NAME);
        if (!pn) {
            if (pdesc->value() == P_NAME)
                pn = (CDp_cname*)pdesc;
            else {
                Errs()->add_error("CDla::link: no name property");
                return (false);
            }
        }
        if (!pn->name_string())
            return (true);

        CDp_lref *prf = (CDp_lref*)prpty(P_LABRF);
        if (!prf) {
            char *s = lstring::stpcpy(buf,  TstringNN(pn->name_string()));
            *s++ = ' ';
            s = mmItoA(s, pn->number());
            *s++ = ' ';
            mmItoA(s, pdesc->value());

            if (!prptyAdd(P_LABRF, buf, Electrical))
                return (false);
            prf = (CDp_lref*)prpty(P_LABRF);
        }
        prf->set_propref(pdesc);
        prf->set_devref((CDc*)odesc);
        return (true);
    }

    if (odesc && odesc->type() == CDWIRE) {
        // Wire node/bnode name label.

        CDp_lref *prf = (CDp_lref*)prpty(P_LABRF);
        CDw *wd = (CDw*)odesc;
        int x = wd->points()[0].x;
        int y = wd->points()[0].y;

#ifdef OLDBIND
#else
        // We can't just use the vertex as the reference point as this
        // may be ambiguous.  Instead, we offset by 100 units, which
        // if off-grid so unlikely to be occupied by another wire (but
        // not impossible).  We probably shouldn't try to support
        // non-Manhattan wire segments, but will give it a shot
        // anyway.

        int x1 = wd->points()[1].x;
        int y1 = wd->points()[1].y;
        if (x == x1) {
            if (y == y1) {
                // WTF
            }
            if (y1 > y)
                y += 100;
            else
                y -= 100;
        }
        else if (y == y1) {
            if (x1 > x)
                x += 100;
            else
                x -= 100;
        }
        else {
            double dx = x1 - x;
            double dy = y1 - y; 
            double d = sqrt(dx*dx + dy*dy);
            x = mmRnd(dx*100/d);
            y = mmRnd(dy*100/d);
        }
#endif

        int pval = -1;
        if (pdesc) {
            pval = pdesc->value();
            if (pval != P_NODE && pval != P_BNODE)
                pval = -1;
        }
        if (pval < 0) {
            if (odesc->prpty(P_BNODE))
                pval = P_BNODE;
            else
                pval = P_NODE;
        }

        if (!prf) {
            snprintf(buf, sizeof(buf), "%d %d %d", x, y, pval);
            if (!prptyAdd(P_LABRF, buf, Electrical))
                return (false);
            prf = (CDp_lref*)prpty(P_LABRF);
        }
        else {
            prf->set_xy(x, y);
            prf->set_propnum(pval);
        }
        prf->set_wireref(wd);
        return (true);
    }

    if (sdesc) {
        // Mutual inductor label.

        CDp_lref *prf = (CDp_lref*)prpty(P_LABRF);
        if (!prf) {
            char *s = lstring::stpcpy(buf,  MUT_CODE);
            *s++ = ' ';
            *s++ = '0';
            *s++ = ' ';
            mmItoA(s, P_NEWMUT);

            if (!prptyAdd(P_LABRF, buf, Electrical))
                return (false);
            prf = (CDp_lref*)prpty(P_LABRF);
        }
        prf->set_propref(0);
        prf->set_cellref(sdesc);
        return (true);
    }
    Errs()->add_error("CDla::link: incorrect arguments");
    return (false);
}
// End CDla functions


// Copy the raw text from an input file into the actual label text,
// advancing the text pointer.
//
void
cCD::GetLabel(const char **ptext, sLstr *lstr)
{
    if (!ptext || !lstr)
        return;
    const char *text = *ptext;
    while (isspace(*text))
        text++;
    if (*text == '<' && *(text+1) == '<') {
        const char *t = strrchr(text+2, '>');
        if (t && *(t-1) == '>') {
            // Native format, the label string is surrounded by angle
            // brackets.

            t--;
            text += 2;
            //  Make sure "<<>>" returns empty string rather than null.
            lstr->add("");
            while (text < t)
                lstr->add_c(*text++);
            text += 2;
            *ptext = text;
            return;
        }
    }
    // cif format
    while (*text && !isspace(*text)) {
        if (*text == '_') {
            lstr->add_c(' ');
            text++;
        }
        else
            lstr->add_c(*text++);
    }
    *ptext = text;
}


int
cCD::DefaultLabelSize(const char *text, DisplayMode mode, double *width,
    double *height)
{
    int nl = ifDefaultLabelSize(text, mode, width, height);
    if (nl)
        return (nl);

    int nlines = 1;
    int maxl = 0;
    int cnt = 0;
    for (const char *s = text; *s; s++) {
        if (*s == '\n') {
            if (cnt > maxl)
                maxl = cnt;
            cnt = 0;
            if (s[1])
                nlines++;
        }
        else
            cnt++;
    }
    if (cnt) {
        if (cnt > maxl)
            maxl = cnt;
    }
    if (mode == Physical) {
        *width = maxl * CDphysDefTextWidth;
        *height = nlines * CDphysDefTextHeight;
    }
    else {
        *width = maxl * CDelecDefTextWidth;
        *height = nlines * CDelecDefTextHeight;
    }
    return (nlines);
}

