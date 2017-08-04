
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

#ifndef CD_MEMMGR_H
#define CD_MEMMGR_H

#include "cd_types.h"
#include "memmgr_tmpl.h"


inline class cCDmmgr *CDmmgr();

class cCDmmgr
{
public:
    friend inline cCDmmgr *CDmmgr() { return (cCDmmgr::instancePtr); }

    cCDmmgr();

    void stats(int*, int*);

    void collectTrash()
        {
            CDo_db.collectTrash();
            CDpo_db.collectTrash();
            CDw_db.collectTrash();
            CDla_db.collectTrash();
            CDc_db.collectTrash();
            CDs_db.collectTrash();
            CDm_db.collectTrash();
            CDol_db.collectTrash();
            CDcl_db.collectTrash();
            SymTabEnt_db.collectTrash();
        }

    void *new_CDol()            { return (CDol_db.newElem()); }
    void del_CDol(CDol *e)      { if (e) CDol_db.delElem(e); }

    void *new_CDcl()            { return (CDcl_db.newElem()); }
    void del_CDcl(CDcl *e)      { if (e) CDcl_db.delElem(e); }

    MemMgr<CDo> CDo_db;
    MemMgr<CDpo> CDpo_db;
    MemMgr<CDw> CDw_db;
    MemMgr<CDla> CDla_db;
    MemMgr<CDc> CDc_db;

    MemMgr<CDs> CDs_db;
    MemMgr<CDm> CDm_db;

    MemMgr<CDol> CDol_db;
    MemMgr<CDcl> CDcl_db;
    MemMgr<SymTabEnt> SymTabEnt_db;

private:
    static cCDmmgr *instancePtr;
};

#endif

