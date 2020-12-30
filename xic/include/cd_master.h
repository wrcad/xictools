
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

#ifndef CD_MASTER_H
#define CD_MASTER_H

#include "cd_memmgr_cfg.h"


// Master list desc.
//
struct CDm
{
    // itable_t (CDs masters list) and ptable_t (CDs master refs list)
    // requirements
    uintptr_t tab_key()         const { return ((uintptr_t)mName); }
    CDm *tab_next()             const { return (mTabNext); }
    void set_tab_next(CDm *m)         { mTabNext = m; }
    CDm *ptab_next()            const { return (mPtabNext); }
    void set_ptab_next(CDm *m)        { mPtabNext = m; }
    CDm *tgen_next(bool b)      const { return (b ? mPtabNext : mTabNext); }

#ifdef CD_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif

    CDm(CDcellName n)
        {
            // The name must be in the name string table if it is
            // not null.
            mName = Tstring(n);
            mTabNext = 0;
            mPtabNext = 0;
            mParent = 0;
            mSdesc = 0;
            mObjRefs = 0;
            mUnlinked = 0;
        }

    ~CDm();

    inline void linkRef(CDs*);
    inline void unlinkRef();
    inline bool unlink();

    bool clear();           // delete this, and all instances
    bool reflect();         // update instance boundaries
    void linkCdesc(CDc*, bool);
    void unlinkCdesc(CDc*, bool);
    int listInstance(stringnumlist**, bool);
    void setSdesc();
    void setParent(CDs*);
    void setCelldesc(CDs*);

    CDcellName cellname()           const { return ((CDcellName)mName); }
    void setCellname(CDcellName n)        { mName = Tstring(n); }

    CDs *parent()                   const { return (mParent); }

    CDs *celldesc()
        {
            if (!mSdesc)
                setSdesc();
            return (mSdesc);
        }

    bool isNullCelldesc()           const { return (!mSdesc); }

    uintptr_t instances()           const { return (mObjRefs); }
    uintptr_t unlinked()            const { return (mUnlinked); }

    // Each CDm is normally contained in two symbol tables:  a
    // name-keyed table in the cell containing instances, and a
    // self-keyed table on the cell the instances resolve to.

    static CDm *findInList(CDm *thism, const char *n)
        {
            for (CDm *m = thism; m; m = m->mTabNext) {
                if (str_compare(n, m->mName))
                    return (m);
            }
            return (0);
        }

    bool hasInstances() const
        {
            if (mObjRefs & 1) {
                ptable_t<CDc> *tab = (ptable_t<CDc>*)(mObjRefs & ~1);
                return (tab && tab->allocated() > 0);
            }
            return (mObjRefs);
        }

    bool hasUnlinked() const
        {
            if (mUnlinked & 1) {
                ptable_t<CDc> *tab = (ptable_t<CDc>*)(mUnlinked & ~1);
                return (tab && tab->allocated() > 0);
            }
            return (mUnlinked);
        }

private:
    const char *mName;          // The cell name, in string table.
    CDm *mTabNext;              // Link for use by itable_t.
    CDm *mPtabNext;             // Link for use by ptable_t.

    CDs *mParent;               // Back pointer to parent.
    CDs *mSdesc;                // Structure referenced.
    uintptr_t mObjRefs;         // Linked instance pointers (ptable_t).
    uintptr_t mUnlinked;        // Unlinked instance pointers (ptable_t).
};

// Generators defined in cd_cell.h.

#endif

