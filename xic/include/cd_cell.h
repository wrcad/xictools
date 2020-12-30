
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

#ifndef CD_CELL_H
#define CD_CELL_H

// Values for sStatus flags field in CDs (below).
#define CDs_FILETYPE_MASK   0x7
#define CDs_BBVALID         0x8
#define CDs_BBSUBNG         0x10
#define CDs_ELECTR          0x20
#define CDs_SYMBOLIC        0x40
#define CDs_CONNECT         0x80
#define CDs_GPINV           0x100
#define CDs_DSEXT           0x200
#define CDs_DUALS           0x400
#define CDs_UNREAD          0x800
#define CDs_COMPRESSED      0x1000
#define CDs_SAVNTV          0x2000
#define CDs_ALTERED         0x4000
#define CDs_CHDREF          0x8000
#define CDs_DEVICE          0x10000
#define CDs_LIBRARY         0x20000
#define CDs_IMMUTABLE       0x40000
#define CDs_OPAQUE          0x80000
#define CDs_CONNECTOR       0x100000
#define CDs_SPCONNECT       0x200000
#define CDs_USER            0x400000
#define CDs_N_USER          2
// user1  0x400000
// user2  0x800000
#define CDs_PCELL           0x1000000
#define CDs_PCSUPR          0x2000000
#define CDs_PCOA            0x4000000
#define CDs_PCKEEP          0x8000000
#define CDs_STDVIA          0x10000000
#define CDs_ARCTOP          0x20000000
#define CDs_INSTNUM         0x40000000


// flags:
// CDs_BBVALID
//   Physican and Electrical
//     Set when BB is computed, and all substructure BB's are known. 
//
// CDs_BBSUBNG
//   Physican and Electrical
//     Set in parent if subcell in hierarchy has unknown BB.  Used during
//     cell read in.
//
// CDs_ELECTR
//   Electrical
//     Set if structure defined from Electrical field of file.  This
//     should always be true in CDcbin::cbElecDesc, and never true in
//     CDcbin::cbPhysDesc.
//
// CDs_SYMBOLIC
//   Electrical
//     The cell is a symbolic representation.  Such cells are never
//     placed directly in the database, rather they are kept in the
//     symbolic property of an owning electrical cell.
//
// CDs_CONNECT
//   Physical
//     Set when grouping is complete, in extraction system.
//   Electrical
//     Set if the connectivity structure is valid, NOPHYS devices ignored
//     or shorted.  Tracks CDs_SPCONNECT if no shorted NOPHYS devs.
//
// CDs_GPINV
//   Physical
//     Ground plane layer inverted and current for grouping.
//
// CDs_DSEXT
//   Physical
//     Devices/subcircuits have been extracted.
//
// CDs_DUALS
//   Physical
//     Physical/electrical duality has been established.
//
// CDs_UNREAD
//   Physical and Electrical
//     Created to satisfy reference unread thus far in input stream.
//
// CDs_COMPRESSED
//   Physical and Electrical
//     Set if originating file was gzip compressed.
//
// CDs_SAVNTV
//   Physical and Electrical
//     Cell should be saved as native file before exit.
//
// CDs_ALTERED
//   Physical and Electrical
//     Set if the data read from the file is filtered, altered, or
//     incomplete, so should not be written to the originating file.
//
// CDs_CHDREF
//   Physical
//     Set if cell is a reference to a CHD hierarchy.
//
// CDs_DEVICE
//   Electrical
//     The cell represents a device or macro symbol.
//
// CDs_LIBRARY
//   Physical and Electrical
//     Set if cell is from a library.
//
// CDs_IMMUTABLE
//   Physical and Electrical
//     Set if cell is read-only.
//
// CDs_OPAQUE
//   Physical
//     Set if the physical contents of the cell should be ignored in
//     extraction.
//
// CDs_CONNECTOR
//   Physical
//     Set if the cell is a via or other connector, that contains no
//     devices or subcircuits.
//
// CDs_SPCONNECT
//   Electrical
//     Set if connectivity is valid for SPICE simulation, i.e., NOPHYS
//     devices included.  Tracks CDs_CONNECT if no shorted NOPHYS devices.
//
// CDs_USER
//   Physical and Electrical
//     Base for user flags.  CDs_N_USER is the number of available user
//     flags.
//
// CDs_PCELL
//   Physical
//     Set if the cell is a pcell super- or sub-master.
//
// CDs_PCSUPR
//   Physical
//     Set if CDs_PCELL and cell is a super-master.
//
// CDs_PCOA
//   Physical
//     Set if CDs_PCELL and cell is imported from OA (sub-masters only).
//
// CDs_PCKEEP
//   Physical
//      Set if CDs_PCELL, not CDs_PCSUPR, and cell was read not created.
//     
// CDs_STDVIA
//   Physical
//     Set if cell is a standard via sub-master.
//
// CDs_ARCTOP
//   Physical or Electrical
//     Set if the cell is top-level and was read from an archive
//     file and it was the only top-level cell in the file.
//
// CDs_INSTNUM
//   Physical
//     Set if instance numbering is valid (numberInstances called),
//     invalidated by change to instance database.

// The name property identifies the type of cell.  The type is
// returned by CDs::elecCellType().  These are:
// 
// CDelecBad
//    Error return, something is wrong.
// CDelecNull
//    Cell is decorative, not electrically active, but may have a special
//    function such as the "mut" cell for mutual inductors.
// CDelecGnd
//    A grounding device, applies a ground contact where placed.
// CDelecTerm
//    A terminal device, for connecting/naming nets, scalar or not.
// CDelecDev
//    A device known to SPICE.
// CDelecMacro
//    A subcircuit macro from the device library.
// CDelecSubc
//    A subcircuit.

enum CDelecCellType
{
    CDelecBad,
    CDelecNull,
    CDelecGnd,
    CDelecTerm,
    CDelecDev,
    CDelecMacro,
    CDelecSubc
};

// Encapsulate common test.
inline bool isDevOrSubc(CDelecCellType tp)
{
    return (tp == CDelecDev || tp == CDelecMacro || tp == CDelecSubc);
}


// Struct to encapsulate a text token for cell types.
struct CDelecCellTypeName
{
    CDelecCellTypeName(CDelecCellType);

    const char *name();

private:
    CDelecCellType ectn_tp;
};


// Pcell script type.
enum CDpcType { CDpcXic, CDpcOA };
// CDpcXic      Xic handles evaluation.
// CDpcOA       OpenAccess handles evaluation.

class cCHD;
class cGroupDesc;
class cNodeMap;
struct CallDesc;
struct hyEnt;
struct CDo;
struct CDpo;
struct CDw;
struct CDp_sym;
struct CDp_snode;
struct CDap;
template <class T> struct CDtlist;
struct CDsterm;
struct CDcterm;
typedef struct CDtlist<CDsterm> CDpin;
typedef struct CDtlist<CDcterm> CDcont;
struct strm_idata;
struct strm_odata;
struct PolyList;
struct CDnetNameStr;
typedef CDnetNameStr* CDnetName;



// Cell descriptor base
//
struct CDs : public CDdb
{
    // table necessities
    uintptr_t tab_key() const       { return ((uintptr_t)sName); }
    CDs *tab_next() const           { return (sTabNext); }
    void set_tab_next(CDs *t)       { sTabNext = t; }
    CDs *tgen_next(bool) const      { return (sTabNext); }

    // arg to updateTermNames
    enum UTNtype { UTNinstances, UTNterms };

    CDs(CDcellName n, DisplayMode m)
        {
            sStatus = CDs_BBVALID;
            if (m == Electrical)
                sStatus |= CDs_ELECTR;
            // The name must be in the name string table if it is
            // not null.
            sName = n;
            sLibname = 0;
            sTabNext = 0;
            sPrptyList = 0;
            sMasters = 0;
            sMasterRefs = 0;
            sBB = CDnullBB;
            Ugrp.sGroups = 0;
            Uhy.sHY = 0;
        }

    // No destructor! The CDdb destructor handles all cleanup.  This
    // is a requirement of the memory manager.

    // If this is a symbolic representation, find the "real" cell.
    CDs *owner() const          { return (isSymbolic() ? Ugrp.sOwner : 0); }
    void setOwner(CDs *sd)      { if (isSymbolic()) Ugrp.sOwner = sd; }

    CDcellName cellname() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sName);
            return (sName);
        }

    void setName(CDcellName n)
        {
            CDs *sd = owner();
            if (sd)
                sd->sName = n;
            else
                sName = n;
        }

    uintptr_t masterRefs() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sMasterRefs);
            return (sMasterRefs);
        }

    void setMasterRefs(uintptr_t mr)
        {
            CDs *sd = owner();
            if (sd)
                sd->sMasterRefs = mr;
            else
                sMasterRefs = mr;
        }

    cNodeMap *nodes() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->Ugrp.sNodes);
            return (Ugrp.sNodes);
        }

    void setNodes(cNodeMap *n)
        {
            CDs *sd = owner();
            if (sd)
                sd->Ugrp.sNodes = n;
            else
                Ugrp.sNodes = n;
        }

    FileType fileType() const
        {
            CDs *sd = owner();
            if (sd)
               return (sd->sStatus & CDs_FILETYPE_MASK);
            return (sStatus & CDs_FILETYPE_MASK);
        }

    void setFileType(FileType t)
        {
            CDs *sd = owner();
            if (sd)
                sd->sStatus = (sd->sStatus & ~CDs_FILETYPE_MASK) |
                    (t & CDs_FILETYPE_MASK);
            else
                sStatus = (sStatus & ~CDs_FILETYPE_MASK) |
                    (t & CDs_FILETYPE_MASK);
        }

    // Note:  the file path is saved in cells from archive files, and
    // in cells from native cell files.  The OpenAccess library name
    // is saved in cells from OA.
    //
    const char *fileName() const
        {
            CDs *sd = owner();
            if (sd)
                return (Tstring(sd->sLibname));
            return (Tstring(sLibname));
        }

    void setFileName(CDarchiveName fn)
        {
            CDs *sd = owner();
            if (sd)
                sd->sLibname = fn;
            else
                sLibname = fn;
        }

    void setFileName(const char *fn)
        {
            CDs *sd = owner();
            if (sd)
                sd->sLibname = CD()->ArchiveTableAdd(fn);
            else
                sLibname = CD()->ArchiveTableAdd(fn);
        }

    uintptr_t masters() const           { return (sMasters); }
    void setMasters(uintptr_t sm)       { sMasters = sm; }

    CDm *findMaster(CDcellName mname) const
        {
            if (sMasters & 1) {
                itable_t<CDm> *tab = (itable_t<CDm>*)(sMasters & ~1);
                return (tab ? tab->find(Tstring(mname)) : 0);
            }
            return (CDm::findInList((CDm*)sMasters, Tstring(mname)));
        }

    // Use this to invalidate all connectivity of electrical cell,
    // or grouping/extraction/association of physical cell.
    //
    void unsetConnected()
        {
            setConnected(false);
            setSPconnected(false);
        }

    cGroupDesc *groups() const          { return (Ugrp.sGroups); }
    void setGroups(cGroupDesc *g)       { Ugrp.sGroups = g; }

    CDpin *pins()                       { return (Uhy.sPins); }
    void setPins(CDpin *p)              { Uhy.sPins = p; }

    hyEnt **getHY() const
        { return ((sStatus & CDs_ELECTR) ? Uhy.sHY : 0); }
    void setHY(hyEnt **h)
        { if (sStatus & CDs_ELECTR) Uhy.sHY = h; }

    bool isPCell() const                { return (sStatus & CDs_PCELL); }
    bool isPCellSuperMaster() const
        { return ((sStatus & CDs_PCELL) && (sStatus & CDs_PCSUPR)); }
    bool isPCellSubMaster() const
        { return ((sStatus & CDs_PCELL) && !(sStatus & CDs_PCSUPR)); }
    CDpcType pcType() const
        { return ((sStatus & CDs_PCOA) ? CDpcOA : CDpcXic); }
    void setPCell(bool b, bool super, bool from_oa)
        {
            if (b) {
                sStatus |= CDs_PCELL;
                if (super)
                    sStatus |= CDs_PCSUPR;
                else
                    sStatus &= ~CDs_PCSUPR;
                if (from_oa)
                    sStatus |= CDs_PCOA;
                else
                    sStatus &= ~CDs_PCOA;
            }
            else
                sStatus &= ~(CDs_PCELL | CDs_PCSUPR | CDs_PCOA);
        }
    void setPCellReadFromFile(bool b)
        { if (b) sStatus |= CDs_PCKEEP; else sStatus &= ~CDs_PCKEEP; }
    bool isPCellReadFromFile() const    { return (sStatus & CDs_PCKEEP); }

    bool isViaSubMaster() const         { return (sStatus & CDs_STDVIA); }
    void setViaSubMaster(bool b)
        { if (b) sStatus |= CDs_STDVIA; else sStatus &= ~CDs_STDVIA; }

    bool isArchiveTopLevel() const      { return (sStatus & CDs_ARCTOP); }
    void setArchiveTopLevel(bool b)
        { if (b) sStatus |= CDs_ARCTOP; else sStatus &= ~CDs_ARCTOP; }

    bool isInstNumValid() const         { return (sStatus & CDs_INSTNUM); }
    void setInstNumValid(bool b)
        { if (b) sStatus |= CDs_INSTNUM; else sStatus &= ~CDs_INSTNUM; }

    const BBox *BB() const              { return (&sBB); }
    void setBB(const BBox *tBB)         { sBB = *tBB; }

    // This returns the BB appropriate for displaying cd.
    const BBox *BBforInst(const CDc *cd)
        {
            if (isElectrical()) {
                CDs *tsd = symbolicRep(cd);
                if (tsd)
                    return tsd->BB();
            }
            return (BB());
        }

    // Recursively set the CDs_BBSUBNG flag in 'this' and in parent
    // cell descs, i.e.  those cells which contain an instance of
    // 'this'.
    //
    void reflectBadBB()
        { if (!isBBsubng()) reflect_bad_BB(); }

    // If a physical subcell is modified, the extraction of all
    // ancestors has to be redone.  This propagates the bad news
    // upward
    //
    void reflectBadExtract()
        { if (isExtracted()) reflect_bad_extract(); }

    // If an electrical subcell is modified, the parent needs to be
    // reassociated, since the subcircuit's terminals may have changed
    //
    void reflectBadAssoc()
        { if (isAssociated()) reflect_bad_assoc(); }

    // If a physical subcell bounding box changes, the inverted ground
    // plane of all ancestors has to be redone.  This propagates the
    // bad news upward
    //
    void reflectBadGroundPlane()
        { if (isGPinv()) reflect_bad_gplane(); }

    bool isBBvalid() const
        {
            return (sStatus & CDs_BBVALID);
        }

    void setBBvalid(bool b)
        {
            if (b)
                sStatus |= CDs_BBVALID;
            else
                sStatus &= ~CDs_BBVALID;
        }

    bool isBBsubng() const
        {
            CDs *sd = owner();
            if (sd)
               return (sd->sStatus & CDs_BBSUBNG);
            return (sStatus & CDs_BBSUBNG);
        }

    void setBBsubng(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_BBSUBNG;
            else
                sd->sStatus &= ~CDs_BBSUBNG;
        }

    bool isElectrical() const
        {
            return (sStatus & CDs_ELECTR);
        }

    DisplayMode displayMode() const
        {
            return ((sStatus & CDs_ELECTR) ? Electrical : Physical);
        }

    bool isSymbolic() const
        {
            return (sStatus & CDs_SYMBOLIC);
        }

    void setSymbolic(bool b)
        {
            if (b)
                sStatus |= CDs_SYMBOLIC;
            else
                sStatus &= ~CDs_SYMBOLIC;
        }

    bool isConnected() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_CONNECT);
            return (sStatus & CDs_CONNECT);
        }

    void setConnected(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_CONNECT;
            else
                sd->sStatus &= ~(CDs_CONNECT | CDs_DSEXT | CDs_DUALS);
        }

    bool isGPinv() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_GPINV);
            return (sStatus & CDs_GPINV);
        }

    void setGPinv(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_GPINV;
            else
                sd->sStatus &= ~CDs_GPINV;
        }

    bool isExtracted() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_DSEXT);
            return (sStatus & CDs_DSEXT);
        }

    void setExtracted(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_DSEXT;
            else
                sd->sStatus &= ~(CDs_DSEXT | CDs_DUALS);
        }

    bool isAssociated() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_DUALS);
            return (sStatus & CDs_DUALS);
        }

    void setAssociated(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_DUALS;
            else
                sd->sStatus &= ~CDs_DUALS;
        }

    bool isUnread() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_UNREAD);
            return (sStatus & CDs_UNREAD);
        }

    void setUnread(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_UNREAD;
            else
                sd->sStatus &= ~CDs_UNREAD;
        }

    bool isCompressed() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_COMPRESSED);
            return (sStatus & CDs_COMPRESSED);
        }

    void setCompressed(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_COMPRESSED;
            else
                sd->sStatus &= ~CDs_COMPRESSED;
        }

    bool isSaventv() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_SAVNTV);
            return (sStatus & CDs_SAVNTV);
        }

    void setSaventv(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_SAVNTV;
            else
                sd->sStatus &= ~CDs_SAVNTV;
        }

    bool isAltered() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_ALTERED);
            return (sStatus & CDs_ALTERED);
        }

    void setAltered(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_ALTERED;
            else
                sd->sStatus &= ~CDs_ALTERED;
        }

    bool isChdRef() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_CHDREF);
            return (sStatus & CDs_CHDREF);
        }

    void setChdRef(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_CHDREF;
            else
                sd->sStatus &= ~CDs_CHDREF;
        }

    bool isDevice() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_DEVICE);
            return (sStatus & CDs_DEVICE);
        }

    void setDevice(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_DEVICE;
            else
                sd->sStatus &= ~CDs_DEVICE;
        }

    bool isLibrary() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_LIBRARY);
            return (sStatus & CDs_LIBRARY);
        }

    void setLibrary(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_LIBRARY;
            else
                sd->sStatus &= ~CDs_LIBRARY;
        }

    bool isImmutable() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_IMMUTABLE);
            return (sStatus & CDs_IMMUTABLE);
        }

    void setImmutable(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_IMMUTABLE;
            else
                sd->sStatus &= ~CDs_IMMUTABLE;
        }

    bool isOpaque() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_OPAQUE);
            return (sStatus & CDs_OPAQUE);
        }

    void setOpaque(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_OPAQUE;
            else
                sd->sStatus &= ~CDs_OPAQUE;
        }

    bool isConnector() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_CONNECTOR);
            return (sStatus & CDs_CONNECTOR);
        }

    void setConnector(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_CONNECTOR;
            else
                sd->sStatus &= ~CDs_CONNECTOR;
        }

    bool isSPconnected() const
        {
            CDs *sd = owner();
            if (sd)
                return (sd->sStatus & CDs_SPCONNECT);
            return (sStatus & CDs_SPCONNECT);
        }

    void setSPconnected(bool b)
        {
            CDs *sd = owner();
            if (!sd)
                sd = this;
            if (b)
                sd->sStatus |= CDs_SPCONNECT;
            else
                sd->sStatus &= ~CDs_SPCONNECT;
        }

    bool isUflag(int i) const
        {
            if (i >= 0 && i < CDs_N_USER) {
                CDs *sd = owner();
                if (sd)
                    return (sd->sStatus & (CDs_USER << i));
                return (sStatus & (CDs_USER << i));
            }
            return (false);
        }

    void setUflag(int i, bool b)
        {
            if (i >= 0 && i < CDs_N_USER) {
                CDs *sd = owner();
                if (!sd)
                    sd = this;
                int j = (CDs_USER << i);
                if (b)
                    sd->sStatus |= j;
                else
                    sd->sStatus &= ~j;
            }
        }

    // cd_cell.cc
    void setPCellFlags();
    unsigned int getFlags() const;
    void setFlags(unsigned int);
    void clear(bool);
    void incModified();
    void decModified();
    bool isModified() const;
    void clearModified();
    bool isEmpty(const CDl* = 0) const;
    bool isSubcell() const;
    bool isHierModified() const;
    bool isRecursive(CDs*);
    CDp_sym *symbolicPrpty(const CDc*) const;
    CDs *symbolicRep(const CDc*) const;
    double area(const CDl*, bool) const;
    void clearLayer(CDl*, bool = false);
    bool computeBB();
    bool fixBBs(ptrtab_t* = 0);
    bool checkInstances();
    bool findNode(int, int, int* = 0);
    bool checkVertex(int, int, CDw*);
    bool checkVertex(int*, int*, int, bool);
    bool reflect();
    CDs *cloneCell(CDs* = 0) const;
    CDsterm *findPinTerm(CDnetName, bool = false);
    bool removePinTerm(CDsterm*);
    CDerrType makeBox(CDl*, const BBox*, CDo**, bool = false);
    CDo *newBox(CDo*, const BBox*, CDl*, CDp*);
    CDo *newBox(CDo*, int, int, int, int, CDl*, CDp*);
    CDerrType makePolygon(CDl*, Poly*, CDpo**, int* = 0, bool = false);
    CDpo *newPoly(CDo*, Poly*, CDl*, CDp*, bool);
    CDerrType makeWire(CDl*, Wire*, CDw**, int* = 0, bool = false);
    CDw *newWire(CDo*, Wire*, CDl*, CDp*, bool);
    CDerrType makeLabel(CDl*, Label*, CDla**, bool = false);
    CDla *newLabel(CDo*, Label*, CDl*, CDp*, bool);
    CDerrType addToDb(Poly&, CDl*, bool=false, CDo** = 0,
        const cTfmStack* = 0, bool = false);
    CDerrType addToDb(PolyList*, CDl*, bool=false, CDol** = 0,
        const cTfmStack* = 0, bool = false);
    OItype makeCall(CallDesc*, const CDtx*, const CDap*, CDcallType, CDc**);
    bool insert(CDo*);
    bool reinsert(CDo*, const BBox*);
    CDm *insertMaster(CDcellName);
    CDm *removeMaster(CDcellName);
    CDm *linkMaster(CDm*);
    CDm *unlinkMaster(CDm*);
    void linkMasterRef(CDm*);
    void unlinkMasterRef(CDm*);
    void updateTermNames(UTNtype);
    void reflectTermNames();
    void reflectTerminals();
    unsigned int checkTerminals(CDp_snode*** = 0);
    void checkBterms();
    bool unlink(CDo*, int);
    void numberInstances();
    CDc *findInstance(const char*);
    void addMissingInstances();
    bool hasSubcells() const;
    int listSubcells(stringnumlist**, bool) const;
    stringlist *listSubcells(int, bool, bool, const BBox*);
    int listParents(stringnumlist**, bool) const;
    bool hasLayer(const CDl*);
    void bincnt(const CDl*, int) const;

    // Property functions.
    CDelecCellType elecCellType(CDpfxName* = 0);
    CDp *prptyList() const;
    void setPrptyList(CDp*);
    CDp *prptyCopy(const CDp*);
    CDp *prpty(int) const;
    stringlist *prptyApplyList(CDo*, CDp**);
    void prptyUpdateFlags(const char*);
    bool prptyAdd(int, const char*);
    CDp *prptyAddCopy(CDp*);
    void prptyAddCopyList(CDp*);
    void prptyPatchAll();
    CDp *prptyUnlink(CDp*);
    void prptyRemove(int);
    void prptyPurge(CDo*);
    void prptyFreeList();
    bool prptyTransformRefs(const cTfmStack*, CDo*);
    void prptyWireLink(const cTfmStack*, CDw*, CDw*, CDmcType);
    void prptyInstLink(const cTfmStack*, CDc*, CDc*, CDmcType);
    void prptyInstPatch(CDc*);
    void prptyUnref(CDo*);
    void prptyLabelLink(const cTfmStack*, CDo*, CDp*, CDmcType);
    void prptyLabelPatch(CDla*);
    bool prptyLabelUpdate(CDla*, CDla*);
    bool prptyMutualAdd(CDc*, CDc*, const char*, const char*);
    void prptyMutualLink(const cTfmStack*, CDc*, CDc*, CDp_nmut*);
    void prptyMutualUpdate();
    bool prptyMutualFind(CDp*, CDc**, CDc**);

    // cd_hypertext.cc
    void hyInit() const;
    char *hyString(cTfmStack*, const BBox*, int, hyEnt**);
    hyEnt *hyPoint(cTfmStack*, const BBox*, int);
    hyEnt *hyPoint(cTfmStack*, CDc*, const BBox*, int);
    hyEnt *hyNode(cTfmStack*, int, int);
    hyList *hyPrpList(CDo*, const CDp*);
    void hyTransformMove(cTfmStack*, const CDo*, CDo*, bool);
    void hyTransformStretch(const CDo*, CDo*, int, int, int, int, bool);
    void hyMergeReference(const CDo*, CDo*, bool);
    void hyDeleteReference(const CDo*, bool);

    // cd_merge.cc
    bool mergeBoxOrPoly(CDo*, bool);
    bool mergeBox(CDo*, bool);
    bool mergeWire(CDw*, bool, CDw** = 0);

    // cd_scriptout.cc
    bool writeScript(FILE*, const char*) const;

    // cd_terminal.cc
    bool checkPhysTerminals(bool);

    // cd_zlist.cc
    Zlist *getRawZlist(int, const CDl*, const BBox*, XIrt*) const;
    Zlist *getZlist(int, const CDl*, const Zlist*, XIrt*) const;

private:
    unsigned int getMcnt() const        { return (db_mod_count); }
    void setMcnt(unsigned int m)        { db_mod_count = m; }

    // cd_cell.cc
    bool fix_bb(ptrtab_t*, int);
    void reflect_bad_BB();
    void reflect_bad_extract();
    void reflect_bad_assoc();
    void reflect_bad_gplane();
    bool is_active(const CDo*, int, int) const;

    // Note: start with int object for 64-bit alignment, since CDdb
    // ends with an int.

    unsigned int sStatus;       // Status flags.

    CDcellName sName;           // Cell name (in string table).
    CDarchiveName sLibname;     // Source file or library (in string tab).
    CDs *sTabNext;              // Link for use by symbol table.

    CDp *sPrptyList;            // List of properties.
    intptr_t sMasters;          // Subcell master descs table (itable_t).
    intptr_t sMasterRefs;       // Referencing master descs table (ptable_t).
    BBox sBB;                   // Structure's bounding box.
    union {
        cGroupDesc *sGroups;    // Connectivity group info for physical.
        cNodeMap *sNodes;       // Node name map for electrical.
        CDs *sOwner;            // Pointer to owner, symbolic only.
    } Ugrp;
    union {
        CDpin *sPins;           // Terminal list in physical mode.
        hyEnt **sHY;            // Hypertext database for electrical.
    } Uhy;
};


// Data block passed to CDs::makeCall.
//
struct CallDesc
{
    CallDesc()
        {
            c_name = 0;
            c_sdesc = 0;
        }

    CallDesc(CDcellName n, CDs *sd)
        {
            c_name = n;
            if (sd && sd->owner()) {
                sd = sd->owner();
                CD()->DbgError("symbolic", "CallDesc constructor");
            }
            c_sdesc = sd;
        }

    CDcellName name()      const { return (c_name); }
    void setName(CDcellName n)   { c_name = n; }

    CDs *celldesc()         const { return (c_sdesc); }
    void setCelldesc(CDs *sd)
        {
            if (sd && sd->owner()) {
                sd = sd->owner();
                CD()->DbgError("symbolic", "setCallDesc");
            }
            c_sdesc = sd;
        }

private:
    CDcellName c_name;
    CDs *c_sdesc;
};


// Generator to descend into a hierarchy of CDs structs, returns each
// struct once only for each call of next().
//
struct CDgenHierDn_s
{
    CDgenHierDn_s(CDs *sd, int md = -1)     { init(sd, md); }
    ~CDgenHierDn_s()                        { delete hd_tab; }

    void init(CDs*, int = -1);
    CDs *next(bool* = 0);

private:
    CDs *hd_sdescs[CDMAXCALLDEPTH];
    tgen_t<CDm> hd_generators[CDMAXCALLDEPTH];
    int hd_dp;
    int hd_maxdp;
    ptrtab_t *hd_tab;
};


// Generator to ascend into the ancestors of a hierarchy of CDs
// structs, returns each struct once only for each call of next().
//
struct CDgenHierUp_s
{
    CDgenHierUp_s(CDs *sd, int md = -1)     { init(sd, md); }
    ~CDgenHierUp_s()                        { delete hu_tab; }

    void init(CDs*, int = -1);
    CDs *next(bool* = 0);

private:
    CDs *hu_sdescs[CDMAXCALLDEPTH];
    tgen_t<CDm> hu_generators[CDMAXCALLDEPTH];
    int hu_dp;
    int hu_maxdp;
    ptrtab_t *hu_tab;
};


//------------------------------
// Master desc deferred inlines.

enum GENtype { GEN_MASTERS, GEN_MASTER_REFS };

// Iterator class for CDm contained in CDs
//
struct CDm_gen : public tgen_t<CDm>
{
    CDm_gen(const CDs *s, GENtype type) : tgen_t<CDm>(type == GEN_MASTERS ?
        (s ? s->masters() : 0) : (s ? s->masterRefs() : 0),
        (type != GEN_MASTERS)) { }

    CDm *m_first() { return (next()); }
    CDm *m_next() { return (next()); }
};


inline void
CDm::linkRef(CDs *sdesc)
{
    if (mSdesc != sdesc) {
        if (mSdesc)
            mSdesc->unlinkMasterRef(this);
        mSdesc = sdesc;
        if (mSdesc)
            mSdesc->linkMasterRef(this);
    }
}


inline void
CDm::unlinkRef()
{
    if (mSdesc) {
        mSdesc->unlinkMasterRef(this);
        mSdesc = 0;
    }
}


inline bool
CDm::unlink()
{
    return (mParent->unlinkMaster(this));
}

#endif

