
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
 $Id: cd_transform.cc,v 5.47 2016/05/26 05:18:01 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "fio_gencif.h"
#include "texttf.h"


// Set the transformation matrix from the angle vector and reflection.
//
void
CDtf::set_tfm(int ax, int ay, bool refly)
{
    int r = refly ? -1 : 1;

    if (!ay) {
        if (ax > 0) {
            tfm[0] = 1;
            tfm[1] = 0;
            tfm[2] = 0;
            tfm[3] = r;
        }
        else {
            tfm[0] = -1;
            tfm[1] = 0;
            tfm[2] = 0;
            tfm[3] = -r;
        }
        return;
    }
    if (!ax) {
        if (ay > 0) {
            tfm[0] = 0;
            tfm[1] = 1;
            tfm[2] = -r;
            tfm[3] = 0;
        }
        else {
            tfm[0] = 0;
            tfm[1] = -1;
            tfm[2] = r;
            tfm[3] = 0;
        }
        return;
    }
    if (ax > 0) {
        if (ay > 0) {
            tfm[0] = 1;
            tfm[1] = 1;
            tfm[2] = -r;
            tfm[3] = r;
        }
        else {
            tfm[0] = 1;
            tfm[1] = -1;
            tfm[2] = r;
            tfm[3] = r;
        }
    }
    else {
        if (ay > 0) {
            tfm[0] = -1;
            tfm[1] = 1;
            tfm[2] = -r;
            tfm[3] = -r;
        }
        else {
            tfm[0] = -1;
            tfm[1] = -1;
            tfm[2] = r;
            tfm[3] = -r;
        }
    }
}


// Functions to deal with label transformations.  The xform parameter
// associated with each label sets the orientation and presentation
// of the label.
//
// xform bits:
// 0-1, 0-no rotation, 1-90, 2-180, 3-270.
// 2, mirror y after rotation
// 3, mirror x after rotation and mirror y
// ---- above are legacy ----
// 4, shift rotation to 45, 135, 225, 315
// 5-6 horiz justification 00,11 left, 01 center, 10 right
// 7-8 vert justification 00,11 bottom, 01 center, 10 top
// 9-10 font (gds)


// Return an xform given the transformation vector.
//
int
CDtf::get_xform()
{
    // Take the transformation defined in this and set
    // the returned bit field accordingly.
    //
    //                  | a    c    0  |
    // Transform = tm = | b    d    0  |
    //                  | tx   ty   1  |
    //
    int a, b, c, d;
    abcd(&a, &b, &c, &d);

    int mirror = false;
    if ((a && (a == -d)) || (b && (b == c))) {
        // tm is reflected, un-reflect and get T
        c = -c;
        // d = -d;
        mirror = true;
    }
    int val = 0;
    if (!a && c) {
        if (c > 0)
            val = 1;        // 90
        else
            val = 3;        // 270
    }
    else if (a && !c) {
        if (a < 0)
            val = 2;        // 180
    }
    else {
        if (a > 0) {
            if (c < 0)
                val = 3;    // 315
        }
        else {
            if (c > 0)
                val = 1;    // 135
            else
                val = 2;    // 225
        }
        val |= TXTF_45;     // shift
    }
    if (mirror)
        val |= TXTF_MY;     // mirror Y
    return (val);
}
// End of CDtf functions.


// Add a transformation primitive.
//
void
CDtx::add_transform(int type, int x, int y)
{
    sTT tt;
    tt.set(this);
    if (type == CDROTATE)
        tt.rotate(x, y);
    else if (type == CDMIRRORX)
        tt.mx();
    else if (type == CDMIRRORY)
        tt.my();
    else if (type == CDTRANSLATE)
        tt.translate(x, y);
    tt.get(this);
}


// Set the transformation from the passed struct, including the
// magnification.
//
void
CDtx::set_tf(CDtf *tf)
{
    magn = tf->mag();
    tf->txty(&tx, &ty);
    int a, b, c, d;
    tf->abcd(&a, &b, &c, &d);
    refly = ((a && (a == -d)) || (b && (b == c)));
    ax = (a > 0 ? 1 : (a < 0 ? -1 : 0));
    ay = (c > 0 ? 1 : (c < 0 ? -1 : 0));
}


// Print a CIF-style transformation string into lstr.
//
void
CDtx::print_string(sLstr &lstr)
{
    Gen.MirrorY(lstr, refly);
    Gen.Rotation(lstr, ax, ay);
    Gen.Translation(lstr, tx, ty);
}


// Parse the CIF transform string and set up transformation.
//
bool
CDtx::parse(const char *str)
{
    clear();
    const char *s = str;
    for (;;) {
        while (isspace(*s))
            s++;
        int c = *s++;
        if (!c || c == ';')
            break;
        int x = 0, y = 0;
        if (c == 'T') {
            char *tok1 = lstring::gettok(&s, ";");
            char *tok2 = lstring::gettok(&s, ";");
            if (!tok2) {
                delete [] tok1;
                return (false);
            }
            bool ok =
                (sscanf(tok1, "%d", &x) == 1 && sscanf(tok2, "%d", &y));
            delete [] tok1;
            delete [] tok2;
            if (!ok)
                return (false);
            add_transform(CDTRANSLATE, x, y);
        }
        else if (c == 'M') {
            c = *s++;
            if (c == 'X')
                add_transform(CDMIRRORX, x, y);
            else if (c == 'Y')
                add_transform(CDMIRRORY, x, y);
            else
                return (false);
        }
        else if (c == 'R') {
            char *tok1 = lstring::gettok(&s, ";");
            char *tok2 = lstring::gettok(&s, ";");
            if (!tok2) {
                delete [] tok1;
                return (false);
            }
            bool ok =
                (sscanf(tok1, "%d", &x) == 1 && sscanf(tok2, "%d", &y));
            delete [] tok1;
            delete [] tok2;
            if (!ok)
                return (false);
            add_transform(CDROTATE, x, y);
        }
    }
    return (true);
}


// Print a string using our current transform code.
//
char *
CDtx::tfstring()
{
    // This compensates for the current transform assuming
    // mirroring is after rotation, but CDtx assumes
    // mirroring comes first.
    //
    int tay = ay;
    if (refly)
        tay = -tay;

    const char *ang = 0;
    if (ax < 0) {
        if (tay < 0)
            ang = "225";
        else if (tay == 0)
            ang = "180";
        else
            ang = "135";
    }
    else if (ax == 0) {
        if (tay < 0)
            ang = "270";
        else if (tay == 0)
            ;
        else
            ang = "90";
    }
    else {
        if (tay < 0)
            ang = "315";
        else if (tay == 0)
            ;
        else
            ang = "45";
    }
    char tfbuf[64];
    tfbuf[0] = 0;
    if (ang != 0)
        sprintf(tfbuf, "R%s", ang);
    if (refly)
        strcat(tfbuf, "MY");
    if (magn != 1.0)
        sprintf(tfbuf + strlen(tfbuf), "M%.8f", magn);
    return (lstring::copy(tfbuf));
}


// Print a DEF or OA orientation string, and compute a "fix" for the
// placement location.  Note that DEF and OA don't support 45s, we
// return null in such cases.  Magnification is ignored.
//
//  LEF/DEF   OpenAccess  Xic      Origin
//  N         R0          R0       LL
//  W         R90         R90      LR
//  S         R180        R180     UR
//  E         R270        R270     UL
//  FN        MY          MX       LR
//  FW        MX90        R90MX    LL
//  FS        MX          MY       UL
//  FE        MY90        R90MY    UR
//
// In DEF, the placement location is the lower-left corner of the
// instance bounding box, for any orientation.  The arguments are the
// instance width and height, and the master lower-left corner.
//
const char *
CDtx::defstring(bool oa, int w, int h, int ox, int oy, int *pcx, int *pcy)
{
    // This compensates for the current transform assuming
    // mirroring is after rotation, but CDtx assumes
    // mirroring comes first.
    //
    int tay = ay;
    if (refly)
        tay = -tay;

    int ang = 0;
    if (ax < 0) {
        if (tay == 0)
            ang = 180;
        else
            return (0);
    }
    else if (ax == 0) {
        if (tay < 0)
            ang = 270;
        else if (tay == 0)
            ;
        else
            ang = 90;
    }
    else if (tay != 0)
        return (0);

    switch (ang) {
    case 0:
        if (refly) {
            *pcx = -ox;
            *pcy = h + oy;
            return (oa ? "MX" : "FS");
        }
        else {
            *pcx = -ox;
            *pcy = -oy;
            return (oa ? "R0" : "N");
        }
    case 90:
        if (refly) {
            *pcx = h + oy;
            *pcy = w + ox;
            return (oa ? "MY90" : "FE");
        }
        else {
            *pcx = h + oy;
            *pcy = -ox;
            return (oa ? "R90" : "W");
        }
    case 180:
        if (refly) {
            *pcx = w + ox;
            *pcy = -oy;
            return (oa ? "MY" : "FN");
        }
        else {
            *pcx = w + ox;
            *pcy = h + oy;
            return (oa ? "R180" : "S");
        }
    case 270:
        if (refly) {
            *pcx = -oy;
            *pcy = -ox;
            return (oa ? "MX90" : "FW");
        }
        else {
            *pcx = -oy;
            *pcy = w + ox;
            return (oa ? "R270" : "E");
        }
    }
    return (0);
}
// End of CDtx functions.


//
// Transformation stack element methods.
//

// We have two modes when rotating boxes/polys non-Manhattan.  The
// default is to use an offset technique referenced to the lower-left
// box coordinate, or the first vertex of polygons.  This ensures that
// the same figure is generated for any location, and seems to ensure
// that all angles are exactly multiples of 45 or 90, after rotation. 
// However, this has the problem that two figures that abut before
// rotation might no longer abut after rotation.  For example, use the
// split function to split a disk, then rotate the collection by 45
// degrees.  It is likely that some of the figures no longer touch.
//
// If ttNoFix45 is set, the offset fix is not done.  This solves the
// problem of gaps appearing between rotated objects, but has its own
// problems.  Namely, rectangles aren't preserved, angles can differ
// from 45's.  Try rotating a small rectangle, say 3x5 internal units,
// by 45s in this mode, and one can see it is a mess.  Larger
// rectangles are not visually distorted, but there are 1-count errors
// in the vertex placements relative to preservation of 45s or 90s. 
// This is probably not acceptable for normal work.
//
// Really, rotating by 45 is something best avoided.
//
bool sTT::ttNoFix45;

// Rotate arctan(y/x), increments of 45 degrees only (x, y taken
// as 0, 1, or -1 only).
//
void
sTT::rotate(int x, int y)
{
    // x' = x*cos() - y*sin();
    // y' = y*cos() + x*sin();
    //
    if (!y) {
        if (x < 0) {
            // Rotate ccw by 180 degrees.
            ttMatrix[0] = -ttMatrix[0];
            ttMatrix[1] = -ttMatrix[1];
            ttMatrix[2] = -ttMatrix[2];
            ttMatrix[3] = -ttMatrix[3];
            ttTx = -ttTx;
            ttTy = -ttTy;
        }
        // Else don't rotate at all.
        return;
    }
    if (!x) {
        if (y > 0) {
            // Rotate ccw by 90 degrees
            int i = ttMatrix[0];
            ttMatrix[0] = -ttMatrix[1];
            ttMatrix[1] = i;
            i = ttMatrix[2];
            ttMatrix[2] = -ttMatrix[3];
            ttMatrix[3] = i;
            i = ttTx;
            ttTx = -ttTy;
            ttTy = i;
        }
        else {
            // Rotate ccw by 270 degrees
            int i = ttMatrix[0];
            ttMatrix[0] = ttMatrix[1];
            ttMatrix[1] = -i;
            i = ttMatrix[2];
            ttMatrix[2] = ttMatrix[3];
            ttMatrix[3] = -i;
            i = ttTx;
            ttTx = ttTy;
            ttTy = -i;
        }
        return;
    }
    // non-orthogonal rotations, normalization required
    int ortho = is_orthogonal();
    if (x > 0) {
        if (y > 0) {
            // Rotate ccw by 45 degrees
            int i = ttMatrix[0] - ttMatrix[1];
            int j = ttMatrix[1] + ttMatrix[0];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[0] = i;
            ttMatrix[1] = j;
            i = ttMatrix[2] - ttMatrix[3];
            j = ttMatrix[3] + ttMatrix[2];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[2] = i;
            ttMatrix[3] = j;

            i = ttTx;
            j = ttTy;
            ttTx = mmRnd(M_SQRT1_2*(i-j));
            ttTy = mmRnd(M_SQRT1_2*(i+j));
        }
        else {
            // Rotate ccw by 315 degrees
            int i = ttMatrix[0] + ttMatrix[1];
            int j = ttMatrix[1] - ttMatrix[0];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[0] = i;
            ttMatrix[1] = j;
            i = ttMatrix[2] + ttMatrix[3];
            j = ttMatrix[3] - ttMatrix[2];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[2] = i;
            ttMatrix[3] = j;

            i = ttTx;
            j = ttTy;
            ttTx = mmRnd(M_SQRT1_2*(i+j));
            ttTy = mmRnd(M_SQRT1_2*(-i+j));
        }
        return;
    }
    else {
        if (y > 0) {
            // Rotate ccw by 135 degrees
            int i = -ttMatrix[0] - ttMatrix[1];
            int j = -ttMatrix[1] + ttMatrix[0];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[0] = i;
            ttMatrix[1] = j;
            i = -ttMatrix[2] - ttMatrix[3];
            j = -ttMatrix[3] + ttMatrix[2];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[2] = i;
            ttMatrix[3] = j;

            i = ttTx;
            j = ttTy;
            ttTx = mmRnd(M_SQRT1_2*(-i-j));
            ttTy = mmRnd(M_SQRT1_2*(i-j));
        }
        else {
            // Rotate ccw by 225 degrees
            int i = -ttMatrix[0] + ttMatrix[1];
            int j = -ttMatrix[1] - ttMatrix[0];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[0] = i;
            ttMatrix[1] = j;
            i = -ttMatrix[2] + ttMatrix[3];
            j = -ttMatrix[3] - ttMatrix[2];
            if (!ortho) { i >>= 1; j >>= 1; }
            ttMatrix[2] = i;
            ttMatrix[3] = j;

            i = ttTx;
            j = ttTy;
            ttTx = mmRnd(M_SQRT1_2*(-i+j));
            ttTy = mmRnd(M_SQRT1_2*(-i-j));
        }
    }
}


//=============================================================================
// Rotating 45

// There are multiple issues here.  If we use an "accurate" transformation:
//   rx = mmRnd(a*(mul(x1, 0)) + mul(y1, 2))) + ttTx;
//   ry = mmRnd(a*(mul(x1, 1)) + mul(y1, 3))) + ttTy;
//
// there are two problems after rotation:
// 1.  The shape of small figures is position dependent.
// 2.  Lines that start out vertical or horizontal may end up not
//     exactly 45 degrees, and 45s may end up non-Manhattan.
//
// The first problem is inherent in the way rotation is done, we
// effectively rotate about the origin, then translate.  The
// quantizing makes this position dependent.  To fix this, to rotate a
// box or polygon, we will first translate to a reference point, which
// is the LL box corner or the first poly vertex, rotate about that
// point, and translate to the final location.  This eliminates the
// position dependence.
//
// The second problem has the following fix.  Suppose we have a line
// x1,y1 -- x2,y2.  We rotate 45 using
//
// rx1 = mmRnd(a*mul(x1, 0)) + mmRnd(a*mul(y1, 2)) + ttTx;
// ry1 = mmRnd(a*mul(x1, 1)) + mmRnd(a*mul(y1, 3)) + ttTy;
// rx2 = mmRnd(a*mul(x2, 0)) + mmRnd(a*mul(y2, 2)) + ttTx;
// ry2 = mmRnd(a*mul(x2, 1)) + mmRnd(a*mul(y2, 3)) + ttTy;
//
// i.e., round each term individually.
//
// This transformation has the property that manhattan -> 45, and 45 ->
// Manhattan.  One can see this by working through some examples, assuming
// x1 == x2, y1 == y2, etc., and looking at rx1-rx2 and ry1-ry2.
//
// We use this rotation transform only when rotating boxes or polys,
// and NOT for general points, and not for the reference point for
// boxes and polys.  BEWARE:  (unless NoFix45 is set) rotating a box,
// for example, by calling point() on each vertex will give different
// results that calling the bb() function.  The user needs to keep
// this in mind.
//
//=============================================================================


// Transform the box, the returned BB contains the smallest
// containing box, and ppnts[5] represents the transformation if
// non-Manhattan.
//
void
sTT::bb(BBox *BB, Point **ppts) const
{
    if (is_orthogonal()) {
        // It suffices to transform the two corners.
        BBox tBB(*BB);
        if (ttMagset) {
            BB->left =
                mmRnd(ttMagn*(mul(tBB.left, 0) + mul(tBB.bottom, 2))) + ttTx;
            BB->bottom =
                mmRnd(ttMagn*(mul(tBB.left, 1) + mul(tBB.bottom, 3))) + ttTy;
            BB->right =
                mmRnd(ttMagn*(mul(tBB.right, 0) + mul(tBB.top, 2))) + ttTx;
            BB->top =
                mmRnd(ttMagn*(mul(tBB.right, 1) + mul(tBB.top, 3))) + ttTy;
        }
        else {
            BB->left = mul(tBB.left, 0) + mul(tBB.bottom, 2) + ttTx;
            BB->bottom = mul(tBB.left, 1) + mul(tBB.bottom, 3) + ttTy;
            BB->right = mul(tBB.right, 0) + mul(tBB.top, 2) + ttTx;
            BB->top = mul(tBB.right, 1) + mul(tBB.top, 3) + ttTy;
        }
        if (BB->right < BB->left)
            mmSwapInts(BB->left, BB->right);
        if (BB->top < BB->bottom)
            mmSwapInts(BB->bottom, BB->top);
        if (ppts)
            *ppts = 0;
        return;
    }

    // The BB returned is the orthogonal BB which contains the rotated
    // BB.
    //
    Point *pts = new Point[5];
    double a = M_SQRT1_2;
    if (ttMagset)
        a *= ttMagn;

    if (ttNoFix45) {
        int x = BB->left;
        int y = BB->bottom;
        pts[0].x = mmRnd(a*(mul(x, 0) + mul(y, 2))) + ttTx;
        pts[0].y = mmRnd(a*(mul(x, 1) + mul(y, 3))) + ttTy;
        y = BB->top;
        pts[1].x = mmRnd(a*(mul(x, 0) + mul(y, 2))) + ttTx;
        pts[1].y = mmRnd(a*(mul(x, 1) + mul(y, 3))) + ttTy;
        x = BB->right;
        pts[2].x = mmRnd(a*(mul(x, 0) + mul(y, 2))) + ttTx;
        pts[2].y = mmRnd(a*(mul(x, 1) + mul(y, 3))) + ttTy;
        y = BB->bottom;
        pts[3].x = mmRnd(a*(mul(x, 0) + mul(y, 2))) + ttTx;
        pts[3].y = mmRnd(a*(mul(x, 1) + mul(y, 3))) + ttTy;
    }
    else {
        int x = BB->left;
        int y = BB->bottom;
        pts[0].x = mmRnd(a*(mul(x, 0) + mul(y, 2))) + ttTx;
        pts[0].y = mmRnd(a*(mul(x, 1) + mul(y, 3))) + ttTy;
        y = BB->top - BB->bottom;
        pts[1].x = mmRnd(a*mul(y, 2)) + pts[0].x;
        pts[1].y = mmRnd(a*mul(y, 3)) + pts[0].y;
        x = BB->right - BB->left;
        y = BB->top - BB->bottom;

        // This guarantees that 45 <--> Manhattan.
        pts[2].x = mmRnd(a*mul(x, 0)) + mmRnd(a*mul(y, 2)) + pts[0].x;
        pts[2].y = mmRnd(a*mul(x, 1)) + mmRnd(a*mul(y, 3)) + pts[0].y;
        /*
        pts[2].x = mmRnd(a*(mul(x, 0) + mul(y, 2))) + pts[0].x;
        pts[2].y = mmRnd(a*(mul(x, 1) + mul(y, 3))) + pts[0].y;
        */
        x = BB->right - BB->left;
        pts[3].x = mmRnd(a*mul(x, 0)) + pts[0].x;
        pts[3].y = mmRnd(a*mul(x, 1)) + pts[0].y;
    }

    pts[4] = pts[0];
    BB->right = BB->left = pts[0].x;
    BB->top = BB->bottom = pts[0].y;
    BB->add(pts[1].x, pts[1].y);
    BB->add(pts[2].x, pts[2].y);
    BB->add(pts[3].x, pts[3].y);
    if (ppts)
        *ppts = pts;
    else
        delete [] pts;
}


// Transform the path points.  For non-orthogonal, the first vertex is
// used as an anchor, so that the transformed figure is not dependent
// on the initial position.
//
void
sTT::path(int numpts, Point *p, const Point *psrc) const
{
    if (!p || (numpts <= 0))
        return;
    if (!psrc)
        psrc = p;
    bool closed = (numpts > 1 && psrc[numpts-1] == psrc[0]);
    if (closed)
        numpts--;
    if (is_orthogonal()) {
        if (ttMagset) {
            for (int i = 0; i < numpts; i++) {
                int tx = psrc[i].x;
                int ty = psrc[i].y;
                p[i].x = mmRnd(ttMagn*(mul(tx, 0) + mul(ty, 2))) + ttTx;
                p[i].y = mmRnd(ttMagn*(mul(tx, 1) + mul(ty, 3))) + ttTy;
            }
        }
        else {
            for (int i = 0; i < numpts; i++) {
                int tx = psrc[i].x;
                int ty = psrc[i].y;
                p[i].x = mul(tx, 0) + mul(ty, 2) + ttTx;
                p[i].y = mul(tx, 1) + mul(ty, 3) + ttTy;
            }
        }
    }
    else {
        double a = M_SQRT1_2;
        if (ttMagset)
            a *= ttMagn;
        if (ttNoFix45) {
            for (int i = 0; i < numpts; i++) {
                int x = psrc[i].x;
                int y = psrc[i].y;
                p[i].x = mmRnd(a*(mul(x, 0) + mul(y, 2))) + ttTx;
                p[i].y = mmRnd(a*(mul(x, 1) + mul(y, 3))) + ttTy;
            }
        }
        else {
            const int rx = psrc[0].x;
            const int ry = psrc[0].y;
            p[0].x = mmRnd(a*(mul(rx, 0) + mul(ry, 2))) + ttTx;
            p[0].y = mmRnd(a*(mul(rx, 1) + mul(ry, 3))) + ttTy;
            for (int i = 1; i < numpts; i++) {
                int tx = psrc[i].x - rx;
                int ty = psrc[i].y - ry;
                // This guarantees that 45 <--> Manhattan.
                p[i].x = mmRnd(a*mul(tx, 0)) + mmRnd(a*mul(ty, 2)) + p[0].x;
                p[i].y = mmRnd(a*mul(tx, 1)) + mmRnd(a*mul(ty, 3)) + p[0].y;
                /*
                p[i].x = mmRnd(a*(mul(tx, 0) + mul(ty, 2))) + p[0].x;
                p[i].y = mmRnd(a*(mul(tx, 1) + mul(ty, 3))) + p[0].y;
                */
            }
        }
    }
    if (closed)
        p[numpts] = p[0];
}


// Compute the inverse transform.
//
void
sTT::inverse(sTT *inv)
{
    int det = ttMatrix[0]*ttMatrix[3] - ttMatrix[2]*ttMatrix[1];
    if (det > 0) {
        inv->ttMatrix[0] = ttMatrix[3];
        inv->ttMatrix[1] = -ttMatrix[1];
        inv->ttMatrix[2] = -ttMatrix[2];
        inv->ttMatrix[3] = ttMatrix[0];
        inv->ttTx = ttMatrix[2]*ttTy - ttMatrix[3]*ttTx;
        inv->ttTy = -ttMatrix[0]*ttTy + ttMatrix[1]*ttTx;
    }
    else {
        inv->ttMatrix[0] = -ttMatrix[3];
        inv->ttMatrix[1] = ttMatrix[1];
        inv->ttMatrix[2] = ttMatrix[2];
        inv->ttMatrix[3] = -ttMatrix[0];
        inv->ttTx = -ttMatrix[2]*ttTy + ttMatrix[3]*ttTx;
        inv->ttTy = ttMatrix[0]*ttTy - ttMatrix[1]*ttTx;
    }
    if (ttMagset) {
        inv->ttMagn = 1.0/ttMagn;
        inv->ttMagset = true;
        double a = inv->ttMagn;
        if (!is_orthogonal())
            a *= M_SQRT1_2;
        inv->ttTx = mmRnd(a*inv->ttTx);
        inv->ttTy = mmRnd(a*inv->ttTy);
    }
    else {
        inv->ttMagset = false;
        if (!is_orthogonal()) {
            inv->ttTx = mmRnd(M_SQRT1_2*inv->ttTx);
            inv->ttTy = mmRnd(M_SQRT1_2*inv->ttTy);
        }
    }
}


// Right-multiply in place by the next element.
//
void
sTT::multiply(sTT *next)
{
    if (!next) {
        fprintf(stderr,
            "Internal error: attempt to multiply empty transform.\n");
        return;
    }
    int nonortho = !is_orthogonal() && !next->is_orthogonal();

    int i1 = ttMatrix[0];
    int i2 = ttMatrix[1];
    ttMatrix[0] = next->mul(i1, 0) + next->mul(i2, 2);
    ttMatrix[1] = next->mul(i1, 1) + next->mul(i2, 3);
    if (nonortho) {
        ttMatrix[0] >>= 1;
        ttMatrix[1] >>= 1;
    }

    int i3 = ttMatrix[2];
    int i4 = ttMatrix[3];
    ttMatrix[2] = next->mul(i3, 0) + next->mul(i4, 2);
    ttMatrix[3] = next->mul(i3, 1) + next->mul(i4, 3);
    if (nonortho) {
        ttMatrix[2] >>= 1;
        ttMatrix[3] >>= 1;
    }

    next->point(&ttTx, &ttTy);

    if (next->ttMagset) {
        if (ttMagset) {
            ttMagn *= next->ttMagn;
            if (ttMagn == 1.0)
                ttMagset = false;
        }
        else {
            ttMagn = next->ttMagn;
            ttMagset = true;
        }
    }
}
// End of sTT functions.

sTT cTfmStack::tTStorage[CDTFREGISTERS];


//--- Transformed Box Overlap Tests

// Transform BB1 and return true if the transformed shape overlaps
// BB2.  This accounts reasonably for 45's.  In this case, if true is
// returned, BB1 encloses the actual polygon.
//
bool
cTfmStack::TBBcheck(BBox *BB1, const BBox *BB2, bool touchok)
{
    Point *pts;
    TBB(BB1, &pts);
    if (pts) {
        Poly po(5, pts);
        bool ok = po.intersect(BB2, touchok);
        delete [] pts;
        return (ok);
    }
    return (touchok ? BB1->isect_i(BB2) : BB1->isect_x(BB2));
}


// As above, but an inverse transform is applied.  This optionally allows
// touching, for use in generator functions.
//
bool
cTfmStack::TInvBBcheck(BBox *BB1, const BBox *BB2, bool touchok)
{
    Point *pts;
    TInverseBB(BB1, &pts);
    if (pts) {
        Poly po(5, pts);
        bool ok = po.intersect(BB2, touchok);
        delete [] pts;
        return (ok);
    }
    return (touchok ? BB1->isect_i(BB2) : BB1->isect_x(BB2));
}


//--- Labels

// Establish a new transformation stack element, obtained from the
// given xform.  NOTE that this calls TPush().
//
void
cTfmStack::TSetTransformFromXform(int xform, int wid, int hei)
{
    TPush();
    if (xform & TXTF_HJC)
        TTranslate(-wid/2, 0);
    else if (xform & TXTF_HJR)
        TTranslate(-wid, 0);
    if (xform & TXTF_VJC)
        TTranslate(0, -hei/2);
    else if (xform & TXTF_VJT)
        TTranslate(0, -hei);
    int shift = xform & TXTF_45;
    int mx = xform & TXTF_MX;
    int my = xform & TXTF_MY;
    xform &= TXTF_ROT;
    if (!shift) {
        if (xform == 1) TRotate(0, 1);
        else if (xform == 2) TRotate(-1, 0);
        else if (xform == 3) TRotate(0, -1);
    }
    else {
        if (xform == 0) TRotate(1, 1);
        else if (xform == 1) TRotate(-1, 1);
        else if (xform == 2) TRotate(-1, -1);
        else TRotate(1, -1);
    }
    // reflections go last
    if (my) TMY();
    if (mx) TMX();
}


//--- Cell Instances

// If px,py is in the instance (or on the boundary if touchok is true)
// return true, with ix,iy giving an array component containing px,py.
// This includes space between array instances counting as part of the
// instance.
//
bool
cTfmStack::TIndex(const BBox *BB, int px, int py,
    unsigned int nx, int dx, unsigned int ny, int dy,
    unsigned int *ix, unsigned int *iy, bool touchok)
{
    if (!BB)
        return (false);
    TInverse();
    BBox xBB(px, py, px, py);
    if (nx <= 1 && ny <= 1) {
        if (!TInvBBcheck(&xBB, BB, true))
            return (false);
        *ix = 0;
        *iy = 0;
        return (true);
    }
    else {
        BBox tBB(*BB);
        if (dx > 0)
            tBB.right += (nx - 1)*dx;
        else if (dx < 0)
            tBB.left += (nx - 1)*dx;
        else
            nx = 1;
        if (dy > 0)
            tBB.top += (ny - 1)*dy;
        else if (dy < 0)
            tBB.bottom += (ny - 1)*dy;
        else
            ny = 1;
        if (!TInvBBcheck(&xBB, &tBB, true))
            return (false);
    }

    if (!dx)
        dx = BB->width();
    if (!dx)
        return (false);
    bool xneg = (dx < 0);
    if (xneg)
        dx = -dx;
    int x = (xneg ? (BB->left - xBB.right) : (xBB.left - BB->right));
    if (x < 0)
        *ix = 0;
    else if (x == 0)
        *ix = touchok ? 0 : 1;
    else {
        // x1 >(=) (Lx-Ls-W)/Dx
        unsigned int xt = x/dx;
        if (!touchok || (x % dx))
            xt++;
        if (xt >= nx)
            return (false);
        *ix = xt;
    }

    if (!dy)
        dy = BB->height();
    if (!dy)
        return (false);
    bool yneg = (dy < 0);
    if (yneg)
        dy = -dy;
    int y = (yneg ? (BB->bottom - xBB.top) : (xBB.bottom - BB->top));
    if (y < 0)
        *iy = 0;
    else if (y == 0)
        *iy = touchok ? 0 : 1;
    else {
        // y1 >(=) (Bx-Bs-H)/Dy
        unsigned int yt = y/dy;
        if (!touchok || (y % dy))
            yt++;
        if (yt >= ny)
            return (false);
        *iy = yt;
    }

    return (true);
}


// This is a special-purpose export, used when iterating over an arrayed
// instance.
//
//  BB              The master sdesc BB.
//  AOI             The region of interest at the top of the transform
//                  stack, i.e., in the top-level cell.
//  nx,dx,ny,dy     Array params.
//  x1,x2,y1,y2     Return, index ranges of overlap.
//
// If there is no overlap, false is returned.
//
bool
cTfmStack::TOverlap(const BBox *BB, const BBox *AOI, unsigned int nx, int dx,
    unsigned int ny, int dy,
    unsigned int *x1, unsigned int *x2, unsigned int *y1, unsigned int *y2,
    bool touchok)
{
    if (!BB)
        return (false);
    BBox xBB = AOI ? *AOI : CDinfiniteBB;
    TInverse();
    if (nx <= 1 && ny <= 1) {
        if (!TInvBBcheck(&xBB, BB, true))
            return (false);
    }
    else {
        BBox tBB(*BB);
        if (dx > 0)
            tBB.right += (nx - 1)*dx;
        else if (dx < 0)
            tBB.left += (nx - 1)*dx;
        else
            nx = 1;
        if (dy > 0)
            tBB.top += (ny - 1)*dy;
        else if (dy < 0)
            tBB.bottom += (ny - 1)*dy;
        else
            ny = 1;
        if (!TInvBBcheck(&xBB, &tBB, true))
            return (false);
    }

    if (!dx)
        dx = BB->width();
    if (!dx)
        return (false);
    bool xneg = (dx < 0);
    if (xneg)
        dx = -dx;
    int x = (xneg ? (BB->left - xBB.right) : (xBB.left - BB->right));
    if (x < 0)
        *x1 = 0;
    else if (x == 0)
        *x1 = touchok ? 0 : 1;
    else {
        // x1 >(=) (Lx-Ls-W)/Dx
        unsigned int xt = x/dx;
        if (!touchok || (x % dx))
            xt++;
        if (xt >= nx)
            return (false);
        *x1 = xt;
    }

    x = (xneg ? (BB->right - xBB.left) : (xBB.right - BB->left));
    if (x < 0)
        return (false);
    // x2 <(=) (Rx-Ls)/Dx
    unsigned int xt = x/dx;
    if (!touchok && !(x % dx))
        xt--;
    if (xt >= nx)
        xt = nx - 1;
    if (xt < *x1)
        return (false);
    *x2 = xt;

    if (!dy)
        dy = BB->height();
    if (!dy)
        return (false);
    bool yneg = (dy < 0);
    if (yneg)
        dy = -dy;
    int y = (yneg ? (BB->bottom - xBB.top) : (xBB.bottom - BB->top));
    if (y < 0)
        *y1 = 0;
    else if (y == 0)
        *y1 = touchok ? 0 : 1;
    else {
        // y1 >(=) (Bx-Bs-H)/Dy
        unsigned int yt = y/dy;
        if (!touchok || (y % dy))
            yt++;
        if (yt >= ny)
            return (false);
        *y1 = yt;
    }

    y = (yneg ? (BB->top - xBB.bottom) : (xBB.top - BB->bottom));
    if (y < 0)
        return (false);
    // y2 <(=) (Tx-Bs)/Dy
    unsigned int yt = y/dy;
    if (!touchok && !(y % dy))
        yt--;
    if (yt >= ny)
        yt = ny - 1;
    if (yt < *y1)
        return (false);
    *y2 = yt;

    return (true);
}


// Return true if AOI overlaps or touches the array elements of cdesc.  The
// x1,...,y2 return the erray elements touched or overlapped (all zero if
// not an array).
//
// This messes with the transform stack; should be used between TPush()
// and TPop().
//
bool
cTfmStack::TOverlapInst(const CDc *cdesc, const BBox *AOI, unsigned int *x1,
    unsigned int *x2, unsigned int *y1, unsigned int *y2, bool touchok)
{
    if (!cdesc)
        return (false);
    CDs *sd = cdesc->masterCell();
    if (!sd)
        return (false);
    const BBox *sBB = sd->BB();
    CDtx tx(cdesc);
    TApply(tx.tx, tx.ty, tx.ax, tx.ay, tx.magn, tx.refly);
    TPremultiply();
    CDap ap(cdesc);
    return (TOverlap(sBB, AOI, ap.nx, ap.dx, ap.ny, ap.dy,
        x1, x2, y1, y2, touchok));
}


// As above, but restrict search area to objects on ldesc (if
// non-null) and subcells.
//
bool
cTfmStack::TOverlapInstForLayer(const CDc *cdesc, const CDl *ldesc,
    const BBox *AOI, unsigned int *x1,
    unsigned int *x2, unsigned int *y1, unsigned int *y2, bool touchok)
{
    if (!cdesc)
        return (false);
    CDs *sd = cdesc->masterCell();
    if (!sd)
        return (false); 
    CDl *cell_layer = CellLayer();

    BBox xBB;
    const BBox *pBB = ldesc ? sd->db_layer_bb(ldesc) : 0;
    if (!pBB) {
        if (ldesc != cell_layer) {
            pBB = sd->db_layer_bb(cell_layer);
            if (!pBB)
                return (false);
            xBB.add((BBox*)pBB);
        }
        else
            return (false);
    }
    else {
        xBB.add((BBox*)pBB);
        if (ldesc != cell_layer) {
            pBB = sd->db_layer_bb(cell_layer);
            if (pBB)
                xBB.add((BBox*)pBB);
        }
    }

    CDtx tx(cdesc);
    TApply(tx.tx, tx.ty, tx.ax, tx.ay, tx.magn, tx.refly);
    TPremultiply();
    CDap ap(cdesc);
    return (TOverlap(&xBB, AOI, ap.nx, ap.dx, ap.ny, ap.dy,
        x1, x2, y1, y2, touchok));
}


// Establish the transform contained in the instance.
//
void
cTfmStack::TApplyTransform(const CDc *cdesc)
{
    CDtx tx(cdesc);
    TApply(tx.tx, tx.ty, tx.ax, tx.ay, tx.magn, tx.refly);
}


//--- Object Retrieval

// Initialize the generator descriptor.
//
bool
cTfmStack::TInitGen(const CDs *sdesc, const CDl *ldesc, const BBox *pBB,
    CDg *gdesc)
{
    if (!ldesc)
        return (false);
    if (!pBB || !gdesc)
        return (false);
    if (sdesc->isChdRef()) {
        Errs()->add_warning(
"A CHD reference cell was passed to the hierarchy traversal\n"
"generator.  This will be treated as an empty cell.  The present operation\n"
"may not yield expected results.  (cellname %s)",
            sdesc->cellname()->string());
    }

    // Apply inverse of current transformation to AOI.
    BBox AOI(*pBB);
    TInverse();

    int flags = 0;
    // If the passed BB covers the infinite BB, assume that the user
    // wants to cycle through all objects.  Setting this flag makes
    // this traversal more efficient.
    if (*pBB >= CDinfiniteBB)
        flags |= GEN_RET_ALL;

    if (TInvBBcheck(&AOI, sdesc->BB(), true))
        gdesc->setup(sdesc, ldesc, &AOI, flags);
    else
        gdesc->clear();

    return (true);
}

