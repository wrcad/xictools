
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

#ifndef CD_H
#define CD_H

#if defined(__GNUC__) && (__GNUC__ == 2)
// Hack for gcc-2.x (Linux 7.2)
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "miscutil/lstring.h"
#include "miscutil/errorrec.h"
#include "miscutil/symtab.h"

#include "geo.h"
#include "geo_box.h"

#include "cd_if.h"
#include "cd_const.h"
#include "cd_transform.h"
#include "cd_symtab.h"
#include "cd_layer.h"
#include "cd_ldb.h"
#include "cd_variable.h"


//-------------------------------------------------------------------------
// Misc. structures used by cell and object functions
//-------------------------------------------------------------------------

// If defined, include test for use of main class before initialized.
//#define CD_TEST_NULL

class cCGD;
class cCHD;
struct CDattr;
struct CDla;
struct CDcbin;
struct CDnameCache;
struct CDlabelCache;
struct hyList;
struct cv_alias_info;
namespace cd_attrdb {
    struct adb_t;
}

// Structure to store flags in.
//
struct FlagDef
{
    const char *name;       // name of flag
    unsigned int value;     // value of flag
    bool user_settable;     // settable by user?
    const char *desc;       // description of flag
};
extern FlagDef SdescFlags[];
extern FlagDef OdescFlags[];
extern FlagDef TermFlags[];
extern FlagDef TermTypes[];


// Base class for properties.
//
struct CDp
{
    CDp(int n)
        {
            p_next = 0;
            p_string = 0;
            p_value = n;
        }

    CDp(const char *str, int num)
        {
            p_next = 0;
            p_string = lstring::copy(str);
            p_value = num;
        }

    CDp(const CDp &p)
        {
            p_next = 0;
            p_string = lstring::copy(p.p_string);
            p_value = p.p_value;
        }

    CDp &operator=(const CDp &p)
        {
            p_next = 0;
            p_string = lstring::copy(p.p_string);
            p_value = p.p_value;
            return (*this);
        }

    virtual ~CDp() { delete [] p_string; }

    virtual CDp *dup()            const { return (new CDp(*this)); }
    virtual bool print(sLstr *lstr, int, int) const
        {
            lstr->add(p_string);
            return (true);
        }

    virtual hyList *hpstring(CDs*) const;
    virtual void transform(const cTfmStack*) { }
    virtual bool cond_bind(CDla*)       { return(true); }
    virtual void bind(CDla*)            { }
    virtual CDla *bound()         const { return (0); }
    virtual void purge(CDo*)            { }

    // Virtual stub for properties that have associated labels.  This
    // returns the text that should appear in the label.
    //
    virtual hyList *label_text(bool *copied, CDc* = 0) const
        {
            *copied = false;
            return (0);
        }

    virtual bool is_elec()        const { return (false); }

    static void destroy(CDp *p)
        {
            while (p) {
                CDp *px = p;
                p = p->p_next;
                delete px;
            }
        }

    bool string(char**) const;
    bool is_longtext() const;
    void scale(double, double, DisplayMode);
    void scale_grid(double, double);
    void scale_node(double, double);
    void scale_name(double, double);
    void scale_mut(double, double);
    void scale_branch(double, double);
    void scale_symblc(double, double);
    void scale_nodmp(double, double);
    void hy_scale(double);

    const char *string()      const { return (p_string); }
    void set_string(const char *s)
        {
            char *t = lstring::copy(s);
            delete [] p_string;
            p_string = t;
        }
    int value()               const { return (p_value); }
    void set_value(int v)           { p_value = v; }
    CDp *next_prp()           const { return (p_next); }
    void set_next_prp(CDp* p)       { p_next = p; }

    CDp *next_n()
        {
            for (CDp *pd = p_next; pd; pd = pd->next_prp()) {
                if (pd->value() == p_value)
                    return (pd);
            }
            return (0);
        }

    static const char *elec_prp_name(int);
    static bool elec_prp_num(const char*, int*);

protected:
    CDp *p_next;
    char *p_string;
    int p_value;
};

// Unset this to add a field to CDo for the property list head. 
// Otherwise, property lists are maintained in a separate hash table.
//
#define CD_PRPTY_TAB

#ifdef CD_PRPTY_TAB
struct prpelt_t
{
    unsigned long tab_key()     { return (p_key); }
    prpelt_t *tab_next()        { return (p_next); }
    prpelt_t *tgen_next(bool)   { return (tab_next()); }

    void set_tab_next(prpelt_t *n) { p_next = n; }
    void set_key(const void *k) { p_key = (unsigned long)k; }
    CDp *get_list()             { return (p_list); }
    void set_list(CDp *p)       { p_list = p; }

private:
    CDp *p_list;
    unsigned long p_key;
    prpelt_t *p_next;
};

#endif

// The default (ground) group, export to extraction system.
#define DEFAULT_GROUP 0

// This wraps the string table pointers, which we keep as a separate
// type, and provides conversion functions to const char*.
//
struct CDtptr
{
    static const char *string(const void *v)
        {
            return ((const char*)v);
        }

    static const char *stringNN(const void *v)
        {
            return (v ? (const char*)v : "");
        }
};

struct CDcellNameStr : public CDtptr { };
typedef CDcellNameStr* CDcellName;      // cCD::CellNameTable string pointer.

struct CDpfxNameStr : public CDtptr { };
typedef CDpfxNameStr* CDpfxName;        // cCD::PfxTable string pointer.

struct CDinstNameStr : public CDtptr { };
typedef CDinstNameStr* CDinstName;      // cCD::InstNameTable string pointer.

struct CDarchiveNameStr : public CDtptr { };
typedef CDarchiveNameStr* CDarchiveName; // cCD::ArchiveTable string pointer.

struct CDnetNameStr : public CDtptr { };
typedef CDnetNameStr* CDnetName;        // cTnameTab string pointer.

inline const char *Tstring(const CDtptr *p)   { return (CDtptr::string(p)); }
inline const char *TstringNN(const CDtptr *p) { return (CDtptr::stringNN(p)); }


//-------------------------------------------------------------------------
// The main CD class.
// This is just a container for flags and utilities, and the constructor
// creates the exported classes that do most of the work.
//-------------------------------------------------------------------------

// Default subcircuit name concatenation character (sync with WRspice).
#define DEF_SUBC_CATCHAR '.'

inline class cCD *CD();

class cCD : public CDif
{
#ifdef CD_TEST_NULL
    static cCD *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();
#endif

public:
#ifdef CD_TEST_NULL
    friend inline cCD *CD() { return (cCD::ptr()); }
#else
    friend inline cCD *CD() { return (cCD::instancePtr); }
#endif

    // How to handle coincident identical items when reading:  skip
    // testing altogether, or test and warn user, or test, warm, and
    // remove duplicates.
    //
    enum DupCheck { DupNoTest, DupWarn, DupRemove };

    // Algorithm used for subcircuit node name mapping for hypertext
    // references.
    //
    enum { SUBC_CATMODE_WR, SUBC_CATMODE_SPICE3 };

    // cd.cc
    cCD();

    void RegisterCreate(const char*);
    void RegisterDestroy(const char*);
    void CheckAlloc();
    void ClearAll(bool);
    void ClearStringTables();
    void Error(int, const char*);
    void DbgError(const char*, const char*);
    static char *AlterName(const char*, const char*, const char*);

    // cd_cell.cc
    bool EnableLabelPatchCache(bool);
    CDlabelCache *GetLabelCache(const CDs*);
    CDc *LabelCacheFind(const CDlabelCache*, CDpfxName, int);

    // cd_database.cc
    bool FindAttr(ticket_t, CDattr*);
    ticket_t RecordAttr(CDattr*);
    void PrintAttrStats();
    void ClearAttrDB();

    // cd_grid.cc
    bool CheckGrid(CDs*, int, const BBox*, const char*, bool, const char*, int,
        FILE*);

    // cd_instance.cc
    bool EnableNameCache(bool);
    CDnameCache *GetNameCache(const CDs*);

    // cd_label.cc
    void GetLabel(const char**, sLstr*);
    int DefaultLabelSize(const char*, DisplayMode, double*, double*);

    // cd_merge.cc
    bool ClipMerge(CDo*, CDo*, CDs*, bool*, bool);

    // cd_open.cc
    CDs *ReopenCell(const char*, DisplayMode);
    OItype OpenExisting(const char*, CDcbin*);
    void Close(CDcellName);
    void Clear(CDcellName);

    // Return the precision to use when printing coordinates
    // accounting for the current resolution.
    //
    int numDigits()
        {
            if (CDphysResolution <= 100)
                return (2);
            if (CDphysResolution <= 1000)
                return (3);
            if (CDphysResolution <= 10000)
                return (4);
            return (5);
        }

    //
    // String table for cell names.  All cell names in database objects
    // are in this table.
    //

    CDcellName CellNameTableAdd(const char *n)
        {
            if (!cdCellNameTable)
                cdCellNameTable = new strtab_t;
            return ((CDcellName)cdCellNameTable->add(n));
        }

    CDcellName CellNameTableFind(const char *n)
        {
            if (!cdCellNameTable)
                return (0);
            return ((CDcellName)cdCellNameTable->find(n));
        }

    //
    // String table for device name prefixes.
    //

    CDpfxName PfxTableAdd(const char *n)
        {
            if (!cdPfxTable)
                cdPfxTable = new cstrtab_t(cstrtab_t::SaveUpper);
            return ((CDpfxName)cdPfxTable->add(n));
        }

    CDpfxName PfxTableFind(const char *n)
        {
            if (!cdPfxTable)
                return (0);
            return ((CDpfxName)cdPfxTable->find(n));
        }

    //
    // String table for internal instance names.
    //

    CDinstName InstNameTableAdd(const char *n)
        {
            if (!cdInstNameTable)
                cdInstNameTable = new cstrtab_t(cstrtab_t::SaveUpper);
            return ((CDinstName)cdInstNameTable->add(n));
        }

    CDinstName InstNameTableFind(const char *n)
        {
            if (!cdInstNameTable)
                return (0);
            return ((CDinstName)cdInstNameTable->find(n));
        }

    //
    // String table for source archive file paths, native cell paths,
    // OpenAccess library names, saved with cells.
    //

    CDarchiveName ArchiveTableAdd(const char *n)
        {
            if (!cdArchiveTable)
                cdArchiveTable = new strtab_t;
            return ((CDarchiveName)cdArchiveTable->add(n));
        }

    CDarchiveName ArchiveTableFind(const char *n)
        {
            if (!cdArchiveTable)
                return (0);
            return ((CDarchiveName)cdArchiveTable->find(n));
        }

#ifdef CD_PRPTY_TAB
    void SetPrptyList(const CDo *od, CDp *p)
        {
            if (!od)
                return;
            if (!cdPrptyTab)
                cdPrptyTab = new itable_t<prpelt_t>;
            prpelt_t *elt = cdPrptyTab->find(od);
            if (!elt) {
                if (!p)
                    return;
                elt = cdPrptyFct.new_element();
                elt->set_key(od);
                cdPrptyTab->link(elt);
                cdPrptyTab = cdPrptyTab->check_rehash();
            }
            elt->set_list(p);
        }

    // Set property list to p, p->next to original list.
    void LinkPrptyList(const CDo *od, CDp *p)
        {
            if (!od || !p)
                return;
            if (!cdPrptyTab)
                cdPrptyTab = new itable_t<prpelt_t>;
            prpelt_t *elt = cdPrptyTab->find(od);
            if (!elt) {
                elt = cdPrptyFct.new_element();
                elt->set_key(od);
                cdPrptyTab->link(elt);
                cdPrptyTab = cdPrptyTab->check_rehash();
                p->set_next_prp(0);
            }
            else
                p->set_next_prp(elt->get_list());
            elt->set_list(p);
        }

    CDp *PrptyList(const CDo *od)
        {
            if (!od || !cdPrptyTab)
                return (0);
            prpelt_t *elt = cdPrptyTab->find(od);
            if (!elt)
                return (0);
            return (elt->get_list());
        }

    void DestroyPrptyList(const CDo *od)
        {
            // Called from CDo destructor.
            if (!od || !cdPrptyTab)
                return;
            prpelt_t *elt = cdPrptyTab->remove(od);
            if (elt) {
                CDp::destroy(elt->get_list());
                elt->set_list(0);
            }
        }

    void CompactPrptyTab()
        {
            itable_t<prpelt_t> *tbk = cdPrptyTab;
            cdPrptyTab = new itable_t<prpelt_t>;

            eltab_t<prpelt_t> fct(cdPrptyFct);
            cdPrptyFct.zero();

            tgen_t<prpelt_t> gen(tbk);
            prpelt_t *el;
            while ((el = gen.next()) != 0) {
                prpelt_t *nel = cdPrptyFct.new_element();
                nel->set_key((const void*)el->tab_key());
                nel->set_list(el->get_list());
                cdPrptyTab->link(nel);
                cdPrptyTab = cdPrptyTab->check_rehash();
            }
            delete tbk;
            fct.clear();
        }

    void ClearPrptyTab()
        {
            // Does not free lists!
            delete cdPrptyTab;
            cdPrptyTab = 0;
            cdPrptyFct.clear();
        }
#else
    void CompactPrptyTab() { }
    void ClearPrptyTab() { }
#endif

    bool IsReading()                { return (cdReading != 0); }
    void SetReading(bool b)
        {
            if (b)
                cdReading++;
            else if (cdReading > 0)
                cdReading--;
        }

    bool IsDeferInst()              { return (cdDeferInst != 0); }
    void SetDeferInst(bool b)
        {
            if (b)
                cdDeferInst++;
            else if (cdDeferInst > 0)
                cdDeferInst--;
        }

    void SetIgnoreIntr(bool b)      { cdIgnoreIntr = b; }
    bool CheckInterrupt(const char *msg = 0)
        {
            return (!cdIgnoreIntr && ifCheckInterrupt(msg));
        }

    bool IsNotStrict()              { return (cdNotStrict); }
    void SetNotStrict(bool b)       { cdNotStrict = b; }

    bool IsNoPolyCheck()            { return (cdNoPolyCheck); }
    void SetNoPolyCheck(bool b)     { cdNoPolyCheck = b; }

    void InitCoincCheck()           { cdCoincErrCnt = 0; }
    int CheckCoincErrs()            { return (cdCoincErrCnt++ - 100); }

    bool IsNoElectrical()           { return (cdNoElec); }
    void SetNoElectrical(bool b)    { cdNoElec= b; }

    bool IsNoMergeObjects()         { return (cdNoMergeObjects); }
    void SetNoMergeObjects(bool b)  { cdNoMergeObjects = b; }

    bool IsNoMergePolys()           { return (cdNoMergePolys); }
    void SetNoMergePolys(bool b)    { cdNoMergePolys = b; }

    char GetSubcCatchar()           { return (cdSubcCatchar); }
    void SetSubcCatchar(char c)     { cdSubcCatchar = c; }

    int GetSubcCatmode()            { return (cdSubcCatmode); }
    void SetSubcCatmode(int m)      { cdSubcCatmode = m; }

    DupCheck DupCheckMode()         { return (cdDupCheckMode); }
    void SetDupCheckMode(DupCheck c) { cdDupCheckMode = c; }

    bool Out32nodes()               { return (cdOut32nodes); }
    void SetOut32nodes(bool b)      { cdOut32nodes = b; }

private:
    strtab_t *cdCellNameTable;      // Cell name table
    cstrtab_t *cdPfxTable;          // Device prefix table
    cstrtab_t *cdInstNameTable;     // Instance name table
    strtab_t *cdArchiveTable;       // Archive/library name table
    cd_attrdb::adb_t *cdAttrDB;     // Instance attribute database
#ifdef CD_PRPTY_TAB
    itable_t<prpelt_t> *cdPrptyTab; // Object-to-property list hash table
#endif
    SymTab *cdAllocTab;             // Struct allocation, for debugging
    CDnameCache *cdNameCache;       // Transient cache for CDc::nameOK
    CDlabelCache *cdLabelCache;     // Transient cache for CDs::prptyLabelPatch
    int cdCoincErrCnt;              // Count coincident objects in cell

    int cdReading;
        // nonzero when building database from input file.

    int cdDeferInst;
        // When nonzero don't put instances with invalid BBs in the
        // database while reading, these will be inserted later after
        // the BB is known.  The insert/reinsert is expensive for
        // large/deep hierarchies.

    bool cdUseNameCache;            // Use cdNameCache
    bool cdUseLabelCache;           // Use cdLabelCache

    bool cdIgnoreIntr;              // Ignore interrupts.

    bool cdNotStrict;
        // True if not strict testing for bad objects.

    bool cdNoPolyCheck;
        // True to skip testing for bad polygons on read.

    bool cdNoElec;
        // Skip the electrical part of cells, and generally keep
        // electrical data from appearing in the database.

    bool cdNoMergeObjects;
        // Suppress merging of new objects.

    bool cdNoMergePolys;
        // If merging objects, clip/merge boxes only.

    char cdSubcCatchar;
        // Field separation character used when constructing node and
        // device names in hypertext processing.

    char cdSubcCatmode;
        // The name-mapping mode used to "flatten" a hierarchy of
        // subcircuits for hypertext references.  Set to one of the
        // SUBC_CATCHAR values.

    DupCheck cdDupCheckMode;
        // Duplicate item testing while reading.

    bool cdOut32nodes;
        // Output old 3.2 node string format.

#ifdef CD_PRPTY_TAB
    eltab_t<prpelt_t> cdPrptyFct;   // Element factory for property list table.
#endif
    static cCD *instancePtr;
};

#endif

