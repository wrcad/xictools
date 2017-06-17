
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: cd_memmgr.h,v 5.4 2015/07/17 19:44:02 stevew Exp $
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

