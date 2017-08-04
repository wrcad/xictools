
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

#ifndef EXT_GRPGEN_H
#define EXT_GRPGEN_H


namespace ext_group {

    // Special generator function, will return objects from the
    // phonycells as well as the main cell.  Only grouped
    // (non-negative group number) objects are returned.

    struct sGrpGen
    {
        sGrpGen()
            {
                gg_g_sdesc = 0;
                gg_e_sdesc = 0;
                gg_did_g = false;
                gg_did_e = false;
            }

        void setflags(int f)
            {
                gg_gdesc.setflags(f);
                gg_g_gdesc.setflags(f);
                gg_e_gdesc.setflags(f);
            }

        void init_gen(const cGroupDesc*, const CDl*, const BBox*,
            cTfmStack* = 0);
        CDo *next();

    private:
        CDg gg_gdesc;
        CDg gg_g_gdesc;
        CDg gg_e_gdesc;
        const CDs *gg_g_sdesc;
        const CDs *gg_e_sdesc;
        bool gg_did_g;
        bool gg_did_e;
    };


    // The remaining definitions apply to a hierarchy traversal
    // generator for the group descritions.  This is used when
    // determining connectivity between cells.

    // Link element to hold part of a tree of subcircuits.
    //
    struct sSubcLink
    {
        sSubcLink(CDc *c, unsigned int x, unsigned int y)
            {
                cdesc = c;
                ix = x;
                iy = y;
                next = 0;
                subs = 0;
            }

        ~sSubcLink()    { destroy(subs); }

        static void destroy(sSubcLink *l)
            {
                while (l) {
                    sSubcLink *x = l;
                    l = l->next;
                    delete x;
                }
            }

        CDc *cdesc;             // the subcircuit
        unsigned int ix, iy;    // element if an array
        sSubcLink *next;        // next subcircuit at this level
        sSubcLink *subs;        // list of sub-subcircuits
    };


    // The generator, allows traversal of the hierarchy of subcircuits.
    //
    struct sSubcGen
    {
        // List element used in the generator for the "next" process.
        //
        struct sLL
        {
            sLL(sSubcLink *c, int d, sLL *n)
                {
                    cl = c;
                    depth = d;
                    next = n;
                }

            static void destroy(sLL *l)
                {
                    while (l) {
                        sLL *x = l;
                        l = l->next;
                        delete x;
                    }
                }

            sSubcLink   *cl;
            int         depth;
            sLL         *next;
        };

        // List element used in the generator for the "prev" process (the
        // stack).
        //
        struct sTF
        {
            sTF(sSubcLink *c, sTF *n)
                {
                    cl = c;
                    next = n;
                }

            static void destroy(sTF *s)
                {
                    while (s) {
                        sTF *x = s;
                        s = s->next;
                        delete x;
                    }
                }

            sSubcLink   *cl;
            CDtf        tf;
            sTF         *next;
        };

        sSubcGen()
            {
                cg_cur = 0;
                cg_stack = 0;
                cg_list = 0;
                cg_depth = 0;
            }

        sSubcGen(const sSubcGen&);

        ~sSubcGen()
            {
                sTF::destroy(cg_stack);
                sLL::destroy(cg_list);
            }

        void clear()
            {
                sTF::destroy(cg_stack);
                sLL::destroy(cg_list);
                cg_cur = 0;
                cg_stack = 0;
                cg_list = 0;
                cg_depth = 0;
            }

        void set(sSubcLink *c)
            {
                sTF::destroy(cg_stack);
                cg_stack = new sTF(0, 0);
                sLL::destroy(cg_list);
                cg_list = 0;
                cg_depth = 0;
                cg_cur = c;
                pushxf();
            }

        CDc *current()
            {
                return (cg_stack->cl ? cg_stack->cl->cdesc : 0);
            }

        CDtf *transform()   { return (&cg_stack->tf); }

        void advance(bool);
        void advance_top();
        const sSubcLink *getlink(int) const;
        const sSubcLink *toplink() const;
        void print() const;

    private:
        void pushxf();
        void popxf();

        sSubcLink   *cg_cur;
        sTF         *cg_stack;
        sLL         *cg_list;
        int         cg_depth;
    };
}

using namespace ext_group;

#endif

