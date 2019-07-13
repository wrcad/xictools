
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

#ifndef YANKBUF_H
#define YANKBUF_H


//
// The yank buffer, user in yank/put commands.
//

// List interface element to hold yanked object.
struct YankBuf
{
    YankBuf(int t, CDl *ld) : yb_next(0), yb_ldesc(ld), yb_type(t) { }
    virtual ~YankBuf() { }

    virtual void add_bbox(BBox*) const = 0;

    static void destroy(YankBuf *y)
        {
            while (y) {
                YankBuf *yx = y;
                y = y->yb_next;
                delete yx;
            }
        }

    static void computeBB(const YankBuf *thisyb, BBox *BB)
        {
            *BB = CDnullBB;
            for (const YankBuf *yx = thisyb; yx; yx = yx->yb_next)
                yx->add_bbox(BB);
        }

    YankBuf *next()             { return (yb_next); }
    void set_next(YankBuf *n)   { yb_next = n; }
    CDl *ldesc()                { return (yb_ldesc); }
    int type()                  { return (yb_type); }

    virtual void display_ghost_put(CDtf*, CDtf*) = 0;
    virtual double area() = 0;

protected:
    YankBuf *yb_next;
    CDl *yb_ldesc;
    int yb_type;
};

struct YankBufB : public YankBuf
{
    YankBufB(CDl *ld, BBox *BB) : YankBuf(CDBOX, ld),  yb_BB(*BB) { }

    void add_bbox(BBox *tBB)    const { tBB->add(&yb_BB); }
    BBox *box()                 { return (&yb_BB); }

    // erase.cc
    void display_ghost_put(CDtf*, CDtf*);
    double area();

private:
    BBox yb_BB;
};

struct YankBufP : public YankBuf
{
    YankBufP(CDl *ld, Poly *po) : YankBuf(CDPOLYGON, ld), yb_poly(*po) { }
    ~YankBufP()     { delete [] yb_poly.points; }

    void add_bbox(BBox *tBB) const
        {
            BBox BB;
            yb_poly.computeBB(&BB);
            tBB->add(&BB);
        }

    Poly *poly()    { return (&yb_poly); }

    // erase.cc
    void display_ghost_put(CDtf*, CDtf*);
    double area();

private:
    Poly yb_poly;
};

struct YankBufW : public YankBuf
{
    YankBufW(CDl *ld, Wire *w) : YankBuf(CDWIRE, ld), yb_wire(*w) { }
    ~YankBufW()     { delete [] yb_wire.points; }

    void add_bbox(BBox *tBB) const
        {
            BBox BB;
            yb_wire.computeBB(&BB);
            tBB->add(&BB);
        }

    Wire *wire()    { return (&yb_wire); }

    // erase.cc
    void display_ghost_put(CDtf*, CDtf*);
    double area();

private:
    Wire yb_wire;
};

#endif

