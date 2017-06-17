
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
 $Id: cd_cbin.h,v 5.15 2015/07/04 22:19:50 stevew Exp $
 *========================================================================*/

#ifndef CD_CBIN_H
#define CD_CBIN_H

#include "cd_master.h"
#include "cd_cell.h"

class cGroupDesc;
class cNodeMap;

// The CDcbin "cell bin" struct is used by the file readers, etc. 
// This provides a local container for physical and electrical cells,
// and methods that generally treat the two cells in a unified manner.
//
struct CDcbin
{
    CDcbin(CDs* = 0);
    CDcbin(CDcellName);

    void reset()
        {
            cbPhysDesc = 0;
            cbElecDesc = 0;
        }

    CDs *phys() const { return (cbPhysDesc); }
    CDs *elec() const { return (cbElecDesc); }

    CDs *celldesc(DisplayMode m) const
        { return (m == Physical ? cbPhysDesc : cbElecDesc); }

    CDs *alt_celldesc(DisplayMode m) const
        { return (m == Physical ? cbElecDesc : cbPhysDesc); }

    bool operator==(const CDcbin &s) const
        {
            return (cbPhysDesc == s.cbPhysDesc && cbElecDesc == s.cbElecDesc);
        }

    bool operator==(const CDcellName nm) const
        {
            return (nm == cellname());
        }

    bool operator!=(const CDcbin &s) const
        {
            return (cbPhysDesc != s.cbPhysDesc || cbElecDesc != s.cbElecDesc);
        }

    void setPhys(CDs*);
    void setElec(CDs*);

    CDcellName cellname() const;

    FileType fileType() const;
    void setFileType(FileType);
    const char *fileName() const;
    void setFileName(const char*);
    void updateFileName(const char*);

    bool isUnread() const;
    bool isEmpty() const;
    bool isSubcell() const;
    bool isModified() const;
    bool isCompressed() const;
    bool isSaventv() const;
    bool isAltered() const;
    bool isDevice() const;
    bool isLibrary() const;
    bool isImmutable() const;
    bool isOpaque() const;
    bool isConnector() const;
    bool isUflag(int) const;
    bool isArchiveTopLevel() const;

    void setCompressed(bool);
    void setSaventv(bool);
    void setAltered(bool);
    void setDevice(bool);
    void setLibrary(bool);
    void setImmutable(bool);
    void setOpaque(bool);
    void setConnector(bool);
    void setUflag(int, bool);

    cGroupDesc *groups();
    cNodeMap *nodes();

    bool fixBBs();
    bool rename(const char*);
    bool deleteCells();   // old "clear()"
    stringlist *listEmpties() const;
    bool hasElectrical() const;
    void setSaveNative();

private:
    CDs *cbPhysDesc;        // Physical structure
    CDs *cbElecDesc;        // Electrical structure
};


// Generator to traverse a cbin hierarchy, fills in the passed cbin
// with cells for each unique cell name for each call of next().
//
struct CDgenHierDn_cbin
{
    CDgenHierDn_cbin(const CDcbin*);
    ~CDgenHierDn_cbin() { delete tab; }
    bool next(CDcbin*, bool*);

    CDs *sdescs[CDMAXCALLDEPTH];
    tgen_t<CDm> generators[CDMAXCALLDEPTH];
    int dp;
    ptrtab_t *tab;
};

#endif

