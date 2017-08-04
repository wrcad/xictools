
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

#ifndef CD_TRANSFORM_H
#define CD_TRANSFORM_H

#ifndef M_SQRT1_2
#define M_SQRT1_2   0.70710678118654752440  // 1/sqrt(2)
#endif

// Maximum allowed call hierarchy depth.
#define CDMAXCALLDEPTH      40

// Number of transform scratch registers.
#define CDTFREGISTERS       8
//
// Register indices (args to cTfmStack::Store/Load).
enum { CDtfRegI0, CDtfRegI1, CDtfRegI2,
    CDtfRegU0, CDtfRegU1, CDtfRegU2, CDtfRegU3, CDtfRegU4 };

// Xic usage:
// CDtfRegI0:  Internal for push/pop context transform.
// CDtfRegI1:  Internal, unused.
// CDtfRegI2:  Internal for flat read initial transform.
// CDtfRegU0/4:  Unassigned, free for use by user.

// This handles the coordinate transformations of objects and cells.
// The class supports translations, reflections, and rotations in
// increments of 45 degrees.  The transforms are managed logically in
// a stack of matrices, where each matrix has the form
//    |  cos  -sin   0  |
//    |  sin   cos   0  |
//    |  tx    ty    1  |
// The tx, ty represent the translations, and the cos, sin terms
// represent rotations.  Reflections are represented by changing signs
// of various terms.  For orthogonal rotations, one of the cos/sin is
// unity, the other zero, so integer artihmetic is applied.  For
// non-orthogonal rotations, abs(cos) == abs(sin) which is set to 1,
// and a renormalization is applied for computations.


struct CDc;
struct CDg;
struct BBox;
struct Point;

// Compact transformation representation.
//
struct CDtf
{
    CDtf() { magn = 1.0; tfm[0] = tfm[3] = 1; tfm[1] = tfm[2] = 0;
        tx = ty = 0; }

    void abcd(int *a, int *b, int *c, int *d) const
        { *a = tfm[0]; *b = tfm[2]; *c = tfm[1]; *d = tfm[3]; }
    void set_abcd(int a, int b, int c, int d)
        { tfm[0] = a; tfm[2] = b; tfm[1] = c; tfm[3] = d; }

    void txty(int *x, int *y) const { *x = tx; *y = ty; }
    void set_txty(int x, int y) { tx = x; ty = y; }

    double mag() const { return (magn); }
    void set_magn(double m) { magn = m; }

    void set_tfm(int, int, bool);
    int get_xform();

private:
    double magn;
    int tx;
    int ty;
    signed char tfm[4];
};


// Rotation/mirror transformation element, use in CDtx and CDattr.
//
struct CDxf
{
    // no constructor

    void decode_transform(unsigned int c)
        {
            // Set up the rotation/reflecton according to this code:
            // x  y
            //  1  0   E        000
            //  0 -1   S        001
            // -1  0   W        010
            //  0  1   N        011
            //  1  1   NE       100
            //  1 -1   SE       101
            // -1 -1   SW       110
            // -1  1   NW       111

            static signed char vals[] =
                { 1, 0, -1, 0, 1, 1, -1, -1, 0, -1, 0, 1, 1, -1, -1, 1 };

            refly = (c & 0x8);
            c &= 0x7;
            ax = vals[c];
            ay = vals[c + 8];
        }

    unsigned int encode_transform() const
        {
            // Encode the rotation/reflection as above.
            unsigned int c = 0;
            if (ax > 0) {
                if (ay > 0)
                    c = 0x4;        //  1  1
                else if (ay == 0)
                    c = 0x0;        //  1  0
                else
                    c = 0x5;        //  1 -1
            }
            else if (ax == 0) {
                if (ay > 0)
                    c = 0x3;        //  0  1
                else if (ay == 0)
                    c = 0x0;        // --
                else
                    c = 0x1;        //  0 -1
            }
            else {
                if (ay > 0)
                    c = 0x7;        // -1  1
                else if (ay == 0)
                    c = 0x2;        // -1  0
                else
                    c = 0x6;        // -1 -1
            }
            if (refly)
                c |= 0x8;
            return (c);
        }

    signed char ax, ay; // rotation vector
    bool refly;         // y -> -y *before* rotation (same as GDSII)
};


// The instance transform.
//
struct CDtx : public CDxf
{
    inline CDtx(const CDc *cd = 0);

    CDtx(bool rf, int i, int j, int x, int y, double m)
        {
            refly = rf;
            ax = i;
            ay = j;
            tx = x;
            ty = y;
            magn = m;
        }

    CDtx(CDtf *tf)
        {
            if (tf)
                set_tf(tf);
            else
                clear();
        }

    void clear()
        {
            tx = ty = 0;
            ax = 1;
            ay = 0;
            refly = false;
            magn = 1.0;
        }

    void scale(double s)
        {
            if (s != 1.0) {
                tx = mmRnd(tx*s);
                ty = mmRnd(ty*s);
            }
        }

    void add_mag(double m) { if (m > 0.0 && m != 1.0) magn *= m; }
    void add_transform(int, int, int);
    void set_tf(CDtf*);
    void print_string(sLstr&);
    bool parse(const char*);
    char *tfstring();
    const char *defstring(bool, int, int, int, int, int*, int*);

    double magn;        // magnification
    int tx, ty;         // translation
};


// Transformation stack element.
//
struct sTT
{
    friend class cTfmStack;

    // No constructor/destructor.

    // Initialize from another sTT.
    //
    void set(const sTT *t)
        {
            ttMagn = t->ttMagn;
            ttMatrix[0] = t->ttMatrix[0];
            ttMatrix[1] = t->ttMatrix[1];
            ttMatrix[2] = t->ttMatrix[2];
            ttMatrix[3] = t->ttMatrix[3];
            ttTx = t->ttTx;
            ttTy = t->ttTy;
            ttMagset = t->ttMagset;
        }

    // Initialize from a compact representation.
    //
    void set(const CDtf *tf)
        {
            tf->abcd(ttMatrix, ttMatrix+2, ttMatrix+1, ttMatrix+3);
            tf->txty(&ttTx, &ttTy);
            if (tf->mag() > 0 && tf->mag() != 1.0) {
                ttMagn = tf->mag();
                ttMagset = true;
            }
            else
                ttMagset = false;
        }

    // Initialize from instance attribute transform struct.
    //
    void set(const CDtx *tx)
        {
            ttMatrix[0] = 1;
            ttMatrix[1] = 0;
            ttMatrix[2] = 0;
            ttMatrix[3] = tx->refly ? -1 : 1;
            ttTx = 0;
            ttTy = 0;
            ttMagset = false;
            rotate(tx->ax, tx->ay);
            translate(tx->tx, tx->ty);
        }

    // Return the values in a compact representation.
    //
    void get(CDtf *tf) const
        {
            tf->set_abcd(ttMatrix[0], ttMatrix[2], ttMatrix[1], ttMatrix[3]);
            tf->set_txty(ttTx, ttTy);
            if (ttMagset)
                tf->set_magn(ttMagn);
            else
                tf->set_magn(1.0);
        }

    // Return the values in an instance attribute struct.
    //
    void get(CDtx *tx) const
        {
            tx->magn = ttMagset ? ttMagn : 1.0;
            tx->tx = ttTx;
            tx->ty = ttTy;
            tx->refly = ((ttMatrix[0] && (ttMatrix[0] == -ttMatrix[3])) ||
                (ttMatrix[2] && (ttMatrix[2] == ttMatrix[1])));
            tx->ax = (ttMatrix[0] > 0 ? 1 : (ttMatrix[0] < 0 ? -1 : 0));
            tx->ay = (ttMatrix[1] > 0 ? 1 : (ttMatrix[1] < 0 ? -1 : 0));
        }

    // Reset to identity transformation.
    //
    void clear()
        {
            ttMagn = 1.0;
            ttMatrix[0] = 1;
            ttMatrix[1] = 0;
            ttMatrix[2] = 0;
            ttMatrix[3] = 1;
            ttTx = ttTy = 0;
            ttMagset = false;
        }

    // Return true if transformation is Manhattan.
    //
    bool is_orthogonal() const
        {
            return (!ttMatrix[0] || !ttMatrix[1]);
        }

    // Add a translation.
    //
    void translate(int x, int y)
        {
            ttTx += x;
            ttTy += y;
        }

    // Add mirror about Y axis.
    //
    void my()
        {
            ttMatrix[1] = -ttMatrix[1];
            ttMatrix[3] = -ttMatrix[3];
            ttTy = -ttTy;
        }

    // Add mirror about X axis.
    //
    void mx()
        {
            ttMatrix[0] = -ttMatrix[0];
            ttMatrix[2] = -ttMatrix[2];
            ttTx = -ttTx;
        }

    // Compute the point transformation.
    //
    void point(int *x, int *y) const
        {
            int tx = *x;
            int ty = *y;
            if (is_orthogonal()) {
                if (ttMagset) {
                    *x = mmRnd(ttMagn*(mul(tx, 0) + mul(ty, 2))) + ttTx;
                    *y = mmRnd(ttMagn*(mul(tx, 1) + mul(ty, 3))) + ttTy;
                }
                else {
                    *x = mul(tx, 0) + mul(ty, 2) + ttTx;
                    *y = mul(tx, 1) + mul(ty, 3) + ttTy;
                }
            }
            else {
                double a = M_SQRT1_2;
                if (ttMagset)
                    a *= ttMagn;
                *x = mmRnd(a*(mul(tx, 0) + mul(ty, 2))) + ttTx;
                *y = mmRnd(a*(mul(tx, 1) + mul(ty, 3))) + ttTy;
            }
        }

    // Left-multiply in place by a translation x,y.
    //
    void transmult(int x, int y)
        {
            point(&x, &y);
            ttTx = x;
            ttTy = y;
        }

    static void set_nofix45(bool b)     { ttNoFix45 = b; }

    // Debugging aid.
    //
    void print() const
        {
            printf("{%d, %d, %d, %d}, tx=%d ty=%d m=%g\n",
                ttMatrix[0], ttMatrix[1], ttMatrix[2], ttMatrix[3],
                ttTx, ttTy, ttMagn);
        }

    // cd_transform.cc
    void rotate(int, int);
    void bb(BBox*, Point**) const;
    void path(int, Point*, const Point*) const;
    void inverse(sTT*);
    void multiply(sTT*);

private:
    int mul(int x, int e) const
        {
            return (x*ttMatrix[e]);
        }

    double ttMagn;       // magnification
    int ttMatrix[4];     // rotation matrix
    int ttTx;            // translate x
    int ttTy;            // translate y
    bool ttMagset;       // flag, set if magnification not unity

    static bool ttNoFix45;
};

class cTfmStack
{
public:
    cTfmStack()
        {
            tTStackDepth = 0;
            tTStackHighWater = 0;
            tTransforms = tTStack;
            tTransforms->clear();
            tTInverse.clear();
        }

    cTfmStack(const cTfmStack &stk)
        {
            *this = stk;
            tTransforms = tTStack + tTStackDepth;
        }

    cTfmStack &operator=(const cTfmStack &stk)
        {
            tTStackDepth = stk.tTStackDepth;
            if (tTStackDepth > CDMAXCALLDEPTH)
                tTStackDepth = CDMAXCALLDEPTH;
            tTStackHighWater = stk.tTStackHighWater;
            tTransforms = tTStack + tTStackDepth;
            tTInverse = stk.tTInverse;
            for (unsigned int i = 0; i <= tTStackDepth; i++)
                tTStack[i] = stk.tTStack[i];
            return (*this);
        }

    unsigned int TDepth() const { return (tTStackDepth); }

    unsigned int THighWater()
        {
            unsigned int r = tTStackHighWater;
            tTStackHighWater = 0;
            return (r);
        }

    // Initialize the transform stack.  Must be called first!
    //
    bool TInit()
        {
            tTransforms = tTStack;
            tTransforms->clear();
            tTStackDepth = 0;
            return (true);
        }

    // Return true if the stack depth exceeds a preset quota.  The
    // application may treat this as an error.
    //
    bool TFull() const
        {
            return (tTStackDepth >= CDMAXCALLDEPTH);
        }

    // Create a new transformation and push it in the stack.
    //
    void TPush()
        {
            if (tTStackDepth < CDMAXCALLDEPTH) {
                tTransforms++;
                tTransforms->clear();
                tTStackDepth++;
                if (tTStackDepth > tTStackHighWater)
                    tTStackHighWater = tTStackDepth;
            }
            else
                fprintf(stderr,
                    "Internal error: attempt to push full transform stack.\n");

        }

    // Pop the previous transformation.
    //
    void TPop()
        {
            if (tTransforms > tTStack) {
                tTransforms--;
                tTStackDepth--;
            }
            else
                fprintf(stderr,
                    "Internal error: attempt to pop empty transform stack.\n");
        }

    // Store the current transformation in the arg.
    //
    void TCurrent(CDtf *tf) const
        {
            tTransforms->get(tf);
        }

    // Store the current transformation in the arg.
    //
    void TCurrent(CDtx *tx) const
        {
            tTransforms->get(tx);
        }

    // Load the transformation from the arg into the current
    // stack element.
    //
    void TLoadCurrent(const CDtf *tf)
        {
            tTransforms->set(tf);
        }

    // Load the transformation from the arg into the current
    // stack element.
    //
    void TLoadCurrent(const CDtx *tx)
        {
            tTransforms->set(tx);
        }

    // Apply a complete transform set.
    //
    void TApply(int tx, int ty, int ax, int ay, double m, bool rf)
        {
            if (rf)
                tTransforms->my();
            tTransforms->rotate(ax, ay);
            tTransforms->translate(tx, ty);
            if (m > 0 && m != 1.0) {
                tTransforms->ttMagset = true;
                tTransforms->ttMagn = m;
            }
            else {
                tTransforms->ttMagset = false;
                tTransforms->ttMagn = 0.0;
            }
        }

    // Add a translation to the current transformation.
    //
    void TTranslate(int x, int y)
        {
            tTransforms->translate(x, y);
        }

    // Add a mirror about the x-axis to the current transformation.
    //
    void TMY()
        {
            tTransforms->my();
        }

    // Add a mirror about the y-axis to the current transformation.
    //
    void TMX()
        {
            tTransforms->mx();
        }

    // Add a rotation to the current transformation.  The rotation
    // angle is expressed as a CIF-style direction vector.  If x or y
    // is zero, the transformation is orthogonal.  Otherwise, it is
    // off orthogonal by 45 degrees.  The relative signs provide the
    // angle.
    //
    void TRotate(int x, int y)
        {
            tTransforms->rotate(x, y);
        }

    // Load the identity transformation into the current stack element.
    //
    void TIdentity()
        {
            tTransforms->clear();
        }

    // Add a floating point scale factor to the transform.
    //
    void TSetMagn(double magn)
        {
            if (magn > 0 && magn != 1.0) {
                tTransforms->ttMagset = true;
                tTransforms->ttMagn = magn;
            }
            else {
                tTransforms->ttMagset = false;
                tTransforms->ttMagn = 0.0;
            }
        }

    double TGetMagn() const
        {
            if (tTransforms->ttMagset)
                return (tTransforms->ttMagn);
            return (1.0);
        }

    // Apply the current transformation to the coordinate pair given.
    //
    void TPoint(int *x, int *y) const
        {
            tTransforms->point(x, y);
        }

    // Apply the current transformation to the corner points of the
    // box.  If the current rotation is non-orthogonal, the BB is the
    // smallest orthogonal box enclosing the transformed original BB,
    // and pts is a point list of the actual transformed points.
    //
    void TBB(BBox *BB, Point **ppts) const
        {
            tTransforms->bb(BB, ppts);
        }

    void TPath(int numpts, Point *pts, const Point *psrc = 0) const
        {
            tTransforms->path(numpts, pts, psrc);
        }

    // Form the instance transform.
    // This is done by computing
    //   Transforms->ttMatrix * Transforms->ttNext->ttMatrix and
    // placing the product in Transforms.ttMatrix.
    // So, the scenario for transforming the coordinates of a master
    // follows:
    //   TPush();
    //   (Invoke TMX, Translate, etc. to build instance transform)
    //   TPremultiply();
    //   (Invoke TPoint to transform master points to instance points)
    //   TPop();
    //
    void TPremultiply()
        {
            sTT *next = tTransforms > tTStack ? tTransforms-1 : 0;
            tTransforms->multiply(next);
        }

    // This gives a result equivalent to
    //    TPush()
    //    TTranslate(x, y)
    //    TPremultiply()
    //
    void TTransMult(int x, int y)
        {
            tTransforms->transmult(x, y);
        }

    // Get current translation.
    //
    void TGetTrans(int *x, int *y) const
        {
            *x = tTransforms->ttTx;
            *y = tTransforms->ttTy;
        }

    // Set Current translation.
    //
    void TSetTrans(int x, int y)
        {
            tTransforms->ttTx = x;
            tTransforms->ttTy = y;
        }

    // Compute the inverse transform of the current transform.
    //
    void TInverse()
        {
            tTransforms->inverse(&tTInverse);
        }

    void TCurrentInverse(CDtf *tf) const
        {
            tTInverse.get(tf);
        }

    // Apply the inverse transformation to the coordinate pair given.
    //
    void TInversePoint(int *x, int *y) const
        {
            tTInverse.point(x, y);
        }

    // Apply the inverse transformation to the corner points of the
    // box.  If the current rotation is non-orthogonal, the BB is the
    // smallest orthogonal box enclosing the transformed original BB,
    // and ppts is a point list of the actual transformed points.
    //
    void TInverseBB(BBox *BB, Point **ppts) const
        {
            tTInverse.bb(BB, ppts);
        }

    // Copy the current transformation to static storage.
    //
    bool TStore(int n)
        {
            if (n < 0 || n >= CDTFREGISTERS)
                return (false);
            tTStorage[n].set(tTransforms);
            return (true);
        }

    // Load the current transform matrix from static storage.
    //
    bool TLoad(int n)
        {
            if (n < 0 || n >= CDTFREGISTERS)
                return (false);
            tTransforms->set(&tTStorage[n]);
            return (true);
        }

    // Clear/initialize the storage registers.
    //
    static void TClearStorage()
        {
            for (int i = 0; i < CDTFREGISTERS; i++)
                tTStorage[i].clear();
        }

    // Load the current transform matrix from the inverse matrix.
    //
    void TLoadInverse()
        {
            tTransforms->set(&tTInverse);
        }

    void TPrint() const
        {
            tTransforms->print();
        }

    void TPrintInverse() const
        {
            tTInverse.print();
        }

    void TPrintStorage(int n) const
        {
            tTStorage[n].print();
        }

    //
    // The remaining functions are more specialized.
    //

    //--- Transformed Box Overlap Tests

    bool TBBcheck(BBox*, const BBox*, bool = false);
    bool TInvBBcheck(BBox*, const BBox*, bool = false);

    //--- Labels

    // Establish a new transformation stack element, obtained from the
    // given xform.  NOTE that this calls TPush().
    //
    void TSetTransformFromXform(int, int, int);

    //--- Cell Instances

    // Find the instance array component containing a given point.
    //
    bool TIndex(const BBox*, int, int, unsigned int, int, unsigned int, int,
        unsigned int*, unsigned int*, bool);

    // This is a special-purpose export, used (for example) when
    // iterating over an arrayed instance.
    //
    bool TOverlap(const BBox*, const BBox*, unsigned int, int, unsigned int,
        int, unsigned int*, unsigned int*, unsigned int*, unsigned int*,
        bool = false);

    // Return true if AOI overlaps or touches the array elements of
    // cdesc.  The x1,...,y2 return the erray elements touched or
    // overlapped (all zero if not an array).
    //
    bool TOverlapInst(const CDc*, const BBox*, unsigned int*, unsigned int*,
        unsigned int*, unsigned int*, bool = false);

    // As above, but restrict search to objects on ld and subcells.
    //
    bool TOverlapInstForLayer(const CDc*, const CDl*, const BBox*,
        unsigned int*, unsigned int*, unsigned int*, unsigned int*,
        bool = false);

    // Establish the transform contained in the instance.
    //
    void TApplyTransform(const CDc*);

    //--- Object Retrieval

    // Initialize the generator descriptor.
    //
    bool TInitGen(const CDs*, const CDl*, const BBox*, CDg*);

private:

    unsigned int tTStackDepth;      // Transform stack depth
    unsigned int tTStackHighWater;  // Max transform depth so far
    sTT *tTransforms;               // Transformation stack
    sTT tTInverse;                  // Inverse transform register
    sTT tTStack[CDMAXCALLDEPTH+1];  // Transform stack

    static sTT tTStorage[];         // Scratch transform registers
};

#endif

