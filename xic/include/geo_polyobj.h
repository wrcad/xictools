
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
 $Id: geo_polyobj.h,v 5.7 2016/04/03 21:30:15 stevew Exp $
 *========================================================================*/

#ifndef GEO_POLYOBJ_H
#define GEO_POLYOBJ_H

#include "geo_poly.h"


struct Zlist;
struct BBox;
struct CDo;

// Polygon used to represent any figure or object, mostly for DRC
//
struct PolyObj
{
    PolyObj()
        {
            po_zlist = 0;
            po_odesc = 0;
            po_cw = false;
            po_cwset = false;
            po_isrect = false;
            po_freepts = false;
            po_width = 0;
        }

    PolyObj(const BBox&);
    PolyObj(Poly&, bool);
    PolyObj(const CDo*, bool);
    ~PolyObj();

    const Zlist *zlist()
        {
            if (!po_zlist)
                po_zlist = po_po.toZlist();
            return (po_zlist);
        }

    const CDo *odesc()      const { return (po_odesc); }

    const Point *points()   const { return (po_po.points); }
    int numpts()            const { return (po_po.numpts); }

    bool iscw()             const { return (po_cw); }
    bool setcw()
        {
            if (!po_cwset) {
                Otype o = po_po.winding();
                if (o == Ocw) {
                    po_cw = true;
                    po_cwset = true;
                }
                else if (o == Occw) {
                    po_cw = false;
                    po_cwset = true;
                }
                else {
                    // poly too weird, should abort
                    return (false);
                }
            }
            return (true);
        }

    bool isrect()           const { return (po_isrect); }

    int eff_width()
        {
            if (!po_width)
                set_eff_width();
            return (po_width);
        }

private:
    void set_eff_width();

    Zlist *po_zlist;     // list of zoids representing poly
    const CDo *po_odesc; // original object, if any
    Poly po_po;          // our shape
    bool po_cw;          // true if winding clockwise
    bool po_cwset;       // true is winding value (cw) is valid
    bool po_isrect;      // true if rectangle
    bool po_freepts;     // true to free points in destructor
    int po_width;        // effective object width;
};

#endif

