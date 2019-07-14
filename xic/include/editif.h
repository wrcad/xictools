
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

#ifndef EDITIF_H
#define EDITIF_H

// This class satisfies all references to the physical editing system
// in general application code.  It is subclassed by the physical
// editing system, but can be instantiated stand-alone to satisfy the
// physical editing references if the application does not provide
// physical editing.

// Return from PopUpModified
enum PMretType { PMok, PMerr, PMabort };

struct hyList;
struct pp_state_t;
struct ParseNode;
struct MenuEnt;

// From GDSII spec.
#define IAP_MAX_ARRAY 32767

// Instance array parameters.  Unlike CDap, the spacing values are
// between adjacent edges, i.e., if zero the elements will abut.
//
struct iap_t
{
    iap_t()
        {
            ia_nx = ia_ny = 1;
            ia_spx = ia_spy = 0;
        }

    iap_t(unsigned int x, unsigned int y, int sx, int sy)
        {
            ia_nx = x;
            ia_ny = y;
            ia_spx = sx;
            ia_spy = sy;
        }

    unsigned int nx()       const { return (ia_nx); }
    unsigned int ny()       const { return (ia_ny); }
    int spx()               const { return (ia_spx); }
    int spy()               const { return (ia_spy); }

    void set_nx(unsigned int u)
        {
            if (u == 0)
                u = 1;
            else if (u > IAP_MAX_ARRAY)
                u = IAP_MAX_ARRAY;
            ia_nx = u;
        }

    void set_ny(unsigned int u)
        {
            if (u == 0)
                u = 1;
            else if (u > IAP_MAX_ARRAY)
                u = IAP_MAX_ARRAY;
            ia_ny = u;
        }

    void set_spx(int i)           { ia_spx = i; }
    void set_spy(int i)           { ia_spy = i; }

private:
    unsigned int ia_nx, ia_ny;
    int ia_spx, ia_spy;
};

// Generator functions use this to pass operation change data.
//
struct op_change_t
{
    op_change_t(CDo *od, CDo *oa)
        {
            oc_delobj = od;
            oc_addobj = oa;
        }

    void swap()
        {
            CDo *tmp = oc_delobj;
            oc_delobj = oc_addobj;
            oc_addobj = tmp;
        }

    CDo *odel()             const { return (oc_delobj); }
    CDo *oadd()             const { return (oc_addobj); }

    void set_odel(CDo *o)   { oc_delobj = o; }
    void set_oadd(CDo *o)   { oc_addobj = o; }

protected:
    CDo *oc_delobj;         // deleted object
    CDo *oc_addobj;         // added object
};

// Orientation for break command.
enum BrkType {BrkVert, BrkHoriz};

// Opaque object for PCell setut state.
typedef void *ulPCstate;

inline class cEditIf *EditIf();

class cEditIf
{
    static cEditIf *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cEditIf *EditIf() { return (cEditIf::ptr()); }

    cEditIf();
    virtual ~cEditIf() { }

    // capability flag
    virtual bool hasEdit()
        { return (false); }

    // undo list interface
    virtual void ulUndoOperation() = 0;
    virtual void ulRedoOperation() = 0;
    virtual bool ulHasRedo() = 0;
    virtual bool ulHasChanged() = 0;
    virtual bool ulCommitChanges(bool = false, bool = false) = 0;
    virtual void ulListFinalize(bool) = 0;
    virtual void ulListBegin(bool, bool) = 0;
    virtual void ulListCheck(const char*, CDs*, bool) = 0;
    virtual op_change_t *ulFindNext(const op_change_t*) = 0;
    virtual void ulPCevalSet(CDs*, ulPCstate*) = 0;
    virtual void ulPCevalReset(ulPCstate*) = 0;
    virtual void ulPCreparamSet(ulPCstate*) = 0;
    virtual void ulPCreparamReset(ulPCstate*) = 0;
    virtual void *saveState() = 0;
    virtual void revertState(void*) = 0;

    // cell.cc
    virtual bool copySymbol(const char*, const char*) = 0;
    virtual bool renameSymbol(const char*, const char*) = 0;

    // context interface
    virtual pp_state_t *newPPstate() = 0;

    // edit.cc
    virtual void setEditingMode(bool) = 0;
    virtual void invalidateLayer(CDs*, CDl*) = 0;
    virtual void invalidateObject(CDs*, CDo*, bool) = 0;
    virtual void clearSaveState() = 0;
    virtual void popState(DisplayMode) = 0;
    virtual void pushState(DisplayMode) = 0;

    // grip.cc
    virtual bool registerGrips(CDc*) = 0;
    virtual void unregisterGrips(const CDc*) = 0;

    // instance.cc
    virtual bool replaceInstance(CDc*, CDcbin*, bool, bool) = 0;
    virtual void addMaster(const char*, const char*, cCHD* = 0) = 0;
    virtual CDc *makeInstance(CDs*, const char*, int, int, int = 0,
        int = 0) = 0;

    // polygns.cc
    virtual void sidesExec(CmdDesc*) = 0;

    // pcplace.cc
    virtual bool openPlacement(const CDcbin*, const char*) = 0;

    // prpedit.cc
    virtual bool prptyCallback(CDo*) = 0;
    virtual void prptyRelist() = 0;

    // prpmisc.cc
    virtual void assignGlobalProperties(CDcbin*) = 0;
    virtual void assertGlobalProperties(CDcbin*) = 0;

    // psdmenu.cc
    virtual const char *const *styleList() = 0;

    // transfrm.cc
    virtual void setCurTransform(int, bool, bool, double) = 0;
    virtual void incrementRotation(bool) = 0;
    virtual void flipY() = 0;
    virtual void flipX() = 0;
    virtual void saveCurTransform(int) = 0;
    virtual void recallCurTransform(int) = 0;
    virtual void clearCurTransform() = 0;

    // wires.cc
    virtual void widthCallback() = 0;
    virtual void setWireAttribute(WsType) = 0;

    // Graphics System
    virtual PMretType PopUpModified(stringlist*, bool(*)(const char*)) = 0;
    virtual PrptyText *PropertyResolve(int, int, CDo**) = 0;
    virtual void PopUpTransform(GRobject, ShowMode,
            bool(*)(const char*, bool, const char*, void*), void*) = 0;

    // edit.h inlines
    virtual void setArrayParams(const iap_t&) = 0;
    virtual WireStyle getWireStyle() = 0;

private:
    static cEditIf *instancePtr;
};

class cEditIfStubs : public cEditIf
{
    void ulUndoOperation() { }
    void ulRedoOperation() { }
    bool ulHasRedo() { return (false); }
    bool ulHasChanged() { return (false); }
    bool ulCommitChanges(bool = false, bool = false) { return (true); }
    void ulListFinalize(bool) { }
    void ulListBegin(bool, bool) { }
    void ulListCheck(const char*, CDs*, bool) { }
    op_change_t *ulFindNext(const op_change_t*) { return (0); }
    void ulPCevalSet(CDs*, ulPCstate*) { }
    void ulPCevalReset(ulPCstate*) { }
    void ulPCreparamSet(ulPCstate*) { }
    void ulPCreparamReset(ulPCstate*) { }
    void *saveState() { return (0); }
    void revertState(void*) { }

    bool copySymbol(const char*, const char*) { return (false); }
    bool renameSymbol(const char*, const char*) { return (false); }

    pp_state_t *newPPstate() { return (0); }

    void setEditingMode(bool) { }
    void invalidateLayer(CDs*, CDl*) { }
    void invalidateObject(CDs*, CDo*, bool) { }
    void clearSaveState() { }
    void popState(DisplayMode) { }
    void pushState(DisplayMode) { }

    bool registerGrips(CDc*) { return (true); }
    void unregisterGrips(const CDc*) { }

    bool replaceInstance(CDc*, CDcbin*, bool, bool) { return (false); }
    void addMaster(const char*, const char*, cCHD* = 0) { }
    CDc *makeInstance(CDs*, const char*, int, int, int = 0, int = 0)
        { return (0); }

    void sidesExec(CmdDesc*) { }

    bool openPlacement(const CDcbin*, const char*) { return (false); }

    bool prptyCallback(CDo*) { return (false); }
    void prptyRelist() { }

    void assignGlobalProperties(CDcbin*) { }
    void assertGlobalProperties(CDcbin*) { }
    bool prptyCheck(CDs*, FILE*, bool) { return (false); }
    bool prptyRegenCell() { return (false); }

    const char *const *styleList() { return (0); }

    void setCurTransform(int, bool, bool, double) { }
    void incrementRotation(bool) { }
    void flipY() { }
    void flipX() { }
    void saveCurTransform(int) { }
    void recallCurTransform(int) { }
    void clearCurTransform() { }

    void widthCallback() { }
    void setWireAttribute(WsType) { }

    PMretType PopUpModified(stringlist*, bool(*)(const char*))
        { return (PMok); }
    PrptyText *PropertyResolve(int, int, CDo**) { return (0); }
    void PopUpTransform(GRobject, ShowMode,
            bool(*)(const char*, bool, const char*, void*), void*) { }

    void setArrayParams(const iap_t&) { }
    WireStyle getWireStyle() { return (CDWIRE_EXTEND); }
};

#endif

