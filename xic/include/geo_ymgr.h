
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
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
 $Id: geo_ymgr.h,v 5.1 2015/10/09 02:07:05 stevew Exp $
 *========================================================================*/

#ifndef GEO_YMGR_H
#define GEO_YMGR_H

#include <algorithm>


// This is a "manager" for a Ylist, which temporarily removes unused rows
// when scanning top to bottom.  When applicable, this can speed up
// computations.
// 
// The passed Ylist should not be used directly while the Ymgr is
// active.  After the Ymgr is destroyed, the passed Ylist reverts
// to its original form (assuming that it was correctly ordered).
//
struct Ymgr
{
    Ymgr(const Ylist *yl)
        {
            org_list = (Ylist*)yl;
            tmp_list = 0;
            cur = org_list;
            nxt = cur ? cur->next : 0;
            prv = 0;
        }

    // When done, this puts the rows back into the Ylist in the proper
    // order.  It is assumed that ordering was correct initially.
    //
    ~Ymgr()
        {
            int cnt = 0;
            for (Ylist *y = org_list; y; y = y->next)
                cnt++;
            for (Ylist *y = tmp_list; y; y = y->next)
                cnt++;
            if (cnt > 1) {
                Ylist **ary = new Ylist*[cnt];
                cnt = 0;
                for (Ylist *y = org_list; y; y = y->next)
                    ary[cnt++] = y;
                for (Ylist *y = tmp_list; y; y = y->next)
                    ary[cnt++] = y;

                std::sort(ary, ary + cnt, yl_cmp);

                cnt--;
                for (int i = 0; i < cnt; i++)
                    ary[i]->next = ary[i+1];
                ary[cnt]->next = 0;
                delete [] ary;
            }
        }

    // Call this to restart the generator.
    //
    void reset()
        {
            cur = org_list;
            nxt = cur ? cur->next : 0;
            prv = 0;
        }

    // Return successive rows, top down.  Rows that are above yu are
    // removed from the generator and never returned.
    //
    Ylist *next(int yu)
        {
            while (cur) {
                if (cur->y_yl >= yu) {
                    if (prv)
                        prv->next = nxt;
                    else
                        org_list = nxt;
                    cur->next = tmp_list;
                    tmp_list = cur;
                    cur = nxt;
                    nxt = cur ? cur->next : 0;
                    continue;
                }
                prv = cur;
                cur = nxt;
                nxt = cur ? cur->next : 0;
                return (prv);
            }
            return (0);
        }

private:
    static bool yl_cmp(const Ylist *y1, const Ylist *y2)
        {
            return (y1->y_yu > y2->y_yu);
        }

    Ylist *org_list;        // Initial and present Ylist.
    Ylist *tmp_list;        // Unsorted container for unused rows.
    Ylist *cur;             // Curent row.
    Ylist *nxt;             // Next row to possibly return.
    Ylist *prv;             // Previous row in org_list.
};

#endif

