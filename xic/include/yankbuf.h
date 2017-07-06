
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
 $Id: edit.h,v 5.139 2016/09/01 16:42:27 stevew Exp $
 *========================================================================*/

#ifndef YANKBUF_H
#define YANKBUF_H


//
// The yank buffer, user in yank/put commands.
//

// List interface element to hold yanked object.
struct yb
{
    yb()
        {
            next = 0;
            type = 0;
            ldesc = 0;
        }

    virtual ~yb() { }

    virtual void add_bbox(BBox*) const = 0;

    static void destroy(yb *y)
        {
            while (y) {
                yb *yx = y;
                y = y->next;
                delete yx;
            }
        }

    static void computeBB(const yb* thisyb, BBox *BB)
        {
            *BB = CDnullBB;
            for (const yb *yx = thisyb; yx; yx = yx->next)
                yx->add_bbox(BB);
        }

    yb *next;
    CDl *ldesc;
    int type;
};

struct yb_b : public yb
{
    yb_b(CDl *ld, BBox *iBB)
        {
            type = CDBOX;
            ldesc = ld;
            BB = *iBB;
        }

    void add_bbox(BBox *tBB) const
        {
            tBB->add(&BB);
        }

    BBox BB;
};

struct yb_p : public yb
{
    yb_p(CDl *ld, Poly *po)
        {
            type = CDPOLYGON;
            ldesc = ld;
            poly = *po;
        }

    ~yb_p()
        {
            delete [] poly.points;
        }

    void add_bbox(BBox *tBB) const
        {
            BBox BB; poly.computeBB(&BB);
            tBB->add(&BB);
        }

    Poly poly;
};

struct yb_w : public yb
{
    yb_w(CDl *ld, Wire *w)
        {
            type = CDWIRE;
            ldesc = ld;
            wire = *w;
        }

    ~yb_w()
        {
            delete [] wire.points;
        }

    void add_bbox(BBox *tBB) const
        {
            BBox BB; wire.computeBB(&BB);
            tBB->add(&BB);
        }

    Wire wire;
};

#endif

