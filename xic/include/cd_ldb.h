
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

#ifndef CD_LDB_H
#define CD_LDB_H


// Names used for the physical and electrical cell layers, which are
// assigned index 0.
//
#define CELL_LAYER_NAME "$$"

// Return if layer name not found, this is an error.
//
#define CDL_NO_LAYER (unsigned int)-1

// Default purpose.
//
#define CDL_PRP_DRAWING "drawing"

// Return if purpose not found, this is oavPurposeNumberDrawing, the
// "drawing" purpose is never in the table.
//
#define CDL_PRP_DRAWING_NUM (unsigned int)-1

// Dynamic layer table initial and incremental sizes.
#define LDB_BLOCK_SIZE 64

// Base class for opaque object used to store database state.
//
struct CDldbState
{
    virtual ~CDldbState() { }
};

inline class cCDldb *CDldb();

// A unified database for electrical and physical layers.  This keeps
// track of ordering and prevents name clashes.  Layers can be
// efficiently found by name, LPP, LPPname, or index.  All layer names
// are case insensitive.
//
class cCDldb
{
    // Below, lpp_t, lppname_t, lppalias_t, and loa_t are allocated
    // as a fixed-sized object (sizeof(lpp_t)) by a common allocator.

    // Table element for LPP pairs.  These are the layernum,purposenum
    // from the OA tables, though the purposenum for "drawing" is not
    // in the table - it defaults.  This table allows rapid access to
    // Xic layers by the LPP.
    //
    struct lpp_t
    {
        int tab_x()                 { return (pp_lyrnum); }
        int tab_y()                 { return (pp_prpnum); }

        lpp_t *tab_next()           { return (pp_next); }
        void set_tab_next(lpp_t *n) { pp_next = n; }
        lpp_t *tgen_next(bool)      { return (pp_next); }

        // Note:  the pp_lyrnum/pp_prpnum generally track the values
        // in pp_ldesc, but not necessarily.  It is possible to alias
        // a CDl to multiple lpp's.  Also, changing the values in the
        // ldesc will not change the access values in the table.

        void init(CDl *ld, unsigned int l, unsigned int p)
            {
                pp_next = 0;
                pp_ldesc = ld;
                pp_lyrnum = l;
                pp_prpnum = p;
            }

        CDl *ldesc()                { return (pp_ldesc); }

    private:
        lpp_t *pp_next;
        CDl *pp_ldesc;
        unsigned int pp_lyrnum;
        unsigned int pp_prpnum;
    };


    // A separate table is available for named LPP pairs (Xic layers).
    //
    struct lppname_t
    {
        const char *tab_name()      { return (pn_ldesc->lppName()); }

        lppname_t *tab_next()       { return (pn_next); }
        void set_tab_next(lppname_t *n) { pn_next = n; }
        lppname_t *tgen_next(bool)  { return (pn_next); }

        void init(CDl *ld)          { pn_next = 0; pn_ldesc = ld; }
        CDl *ldesc()                { return (pn_ldesc); }

    private:
        lppname_t *pn_next;
        CDl *pn_ldesc;
    };


    // A table of alternative layer names, or descriptive names,
    // which can also be used to resolve layers by name.  This will
    // support the techParams in a Virtuoso technology file that
    // specify layers by descriptive names, e.g. "active_layer",
    // "pplus_layer", etc.  One can use these in macros or device
    // blocks to gain some portability among a group of processes,
    // assuming we set up the aliases as appropriate for each
    // process.  We alias to a layer name, not a layer desc, and
    // there is no enforcement that the given name exists.

    struct lppalias_t
    {
        const char *tab_name()      { return (pa_alias); }

        lppalias_t *tab_next()      { return (pa_next); }
        void set_tab_next(lppalias_t *n) { pa_next = n; }
        lppalias_t *tgen_next(bool) { return (pa_next); }

        void init(const char *al, const char *nm)
            {
                pa_next = 0;
                pa_alias = lstring::copy(al);
                pa_lname = lstring::copy(nm);
            }

        const char *layer_name() const  { return (pa_lname); }

        void set_layer_name(const char *nm)
            {
                char *n = lstring::copy(nm);
                delete [] pa_lname;
                pa_lname = n;
            }

    private:
        lppalias_t *pa_next;
        char *pa_alias;
        char *pa_lname;
    };


    // Table element for OA layer and purpose tables.  We can access
    // name by number and vice-versa.
    //
    struct loa_t
    {
        const char *tab_name()      { return (oa_name); }
        uintptr_t tab_key()         { return (oa_num); }

        loa_t *tab_next()           { return (oa_next); }
        void set_tab_next(loa_t *n) { oa_next = n; }
        loa_t *tgen_next(bool)      { return (oa_next); }

        void init(char *na, unsigned int nm)
            {
                oa_next = 0;
                oa_name = na;
                oa_num = nm;
            }

        const char *name()          { return (oa_name); }
        unsigned int num()          { return (oa_num); }

    private:
        loa_t *oa_next;
        char *oa_name;
        unsigned int oa_num;
    };


    // Element block, for allocator.
    struct ldb_blk
    {
        ldb_blk(ldb_blk *n) { next = n; }

        lpp_t elts[LDB_BLOCK_SIZE];
        ldb_blk *next;
    };


    // Derived layer hash table element.
    struct drvlyr_t
    {
        drvlyr_t(CDl *ld)
            {
                dl_next = 0;
                dl_ldesc = ld;
            }

        ~drvlyr_t()
            {
                delete dl_ldesc;
            }

        const char *tab_name()      const { return (dl_ldesc->name()); }
        drvlyr_t *tab_next()              { return (dl_next); }
        void set_tab_next(drvlyr_t *n)    { dl_next = n; }
        drvlyr_t *tgen_next(bool)         { return (dl_next); }

        CDl *ldesc()                const { return (dl_ldesc); }

    private:
        drvlyr_t *dl_next;
        CDl *dl_ldesc;
    };


    static cCDldb *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cCDldb *CDldb() { return (cCDldb::ptr()); }

    cCDldb();
    ~cCDldb();

    void init();
    void reset();
    void clear();

    static char *getValidName(const char*);

    char *newLayerName(unsigned int*);
    unsigned int newLayerNum();
    unsigned int newPurposeNum();
    void saveAlias(const char*, const char*);
    void removeAlias(const char*);
    string2list *listAliases();
    bool resolveName(const char*, unsigned int*, unsigned int*);
    CDl *newLayer(const char*, DisplayMode);
    CDl *newLayer(unsigned int, unsigned int, DisplayMode);
    CDl *addNewLayer(const char*, DisplayMode, CDLtype, int, bool = false);
    bool setLPPname(CDl*, const char*);
    void clearLayers(CDl**, CDl**);
    CDldbState *getState();
    bool setState(const CDldbState*, CDll**);
    void saveState();
    void revertState();
    bool setInvalid(CDl*, bool);
    CDl *removeLayer(int, DisplayMode);
    bool renameLayer(const char*, const char*);
    bool insertLayer(CDl*, DisplayMode, int);
    bool saveMapping(CDl*, unsigned int, unsigned int);
    CDl *findLayer(const char*);
    CDl *findLayer(const char*, DisplayMode);
    CDl *findLayer(unsigned int, unsigned int);
    CDl *findLayer(unsigned int, unsigned int, DisplayMode);
    CDl *findAnyLayer(unsigned int, unsigned int);
    CDl *getUnusedLayer(const char*, DisplayMode);
    CDl *getUnusedLayer(unsigned int, unsigned int, DisplayMode);
    void pushUnusedLayer(CDl*, DisplayMode);
    CDl *popUnusedLayer(DisplayMode);
    void sort(DisplayMode);
    CDl *search(DisplayMode, int, const char*);

    unsigned int getOAlayerNum(const char*);
    const char *getOAlayerName(unsigned int); 
    bool saveOAlayer(const char*, unsigned int, const char* = 0);
    stringnumlist *listOAlayerTab();
    void clearOAlayerTab();
    unsigned int getOApurposeNum(const char*, bool*);
    const char *getOApurposeName(unsigned int); 
    bool saveOApurpose(const char*, unsigned int, const char* = 0);
    stringnumlist *listOApurposeTab();
    void clearOApurposeTab();
    CDl *addDerivedLayer(const char*, int, int, const char*);
    CDl *remDerivedLayer(const char*);
    CDl *findDerivedLayer(const char*);
    CDl **listDerivedLayers();

    CDl *layer(int ix, DisplayMode mode)
        {
            return (mode == Physical ?
                ldb_phys.layer(ix) : ldb_elec.layer(ix));
        }

    int layersUsed(DisplayMode mode)
        {
            return (mode == Physical ?
                ldb_phys.numused() : ldb_elec.numused());
        }

    CDll *unusedLayers(DisplayMode mode)
        {
            return (mode == Physical ? ldb_unused_phys : ldb_unused_elec);
        }

    CDll *invalidLayers()
        {
            return (ldb_invalid_list);
        }

private:
    // "Destructor" for elements.
    void keep(void *item)
        {
            if (item) {
                lpp_t *elt = (lpp_t*)item;
                elt->set_tab_next(ldb_reuse_list);
                ldb_reuse_list = elt;
            }
        }

    // Element allocator, cast return to type.  Items not initialized.
    void *new_elt()
        {
            if (ldb_reuse_list) {
                void *rptr = ldb_reuse_list;
                ldb_reuse_list = ldb_reuse_list->tab_next();
                return (rptr);
            }
            if (!ldb_blocks || ldb_elts_used >= LDB_BLOCK_SIZE) {
                ldb_blocks = new ldb_blk(ldb_blocks);
                ldb_elts_used = 0;
            }
            return (ldb_blocks->elts + ldb_elts_used++);
        }

    // Xic Layer database.
    CDlary ldb_phys;
    CDlary ldb_elec;
    xytable_t<lpp_t> *ldb_lpp_tab;
    ctable_t<lppname_t> *ldb_lppname_tab;
    ctable_t<lppalias_t> *ldb_lppalias_tab;

    // OA Layer hash tables.
    ctable_t<loa_t> *ldb_oalname_tab;
    itable_t<loa_t> *ldb_oalnum_tab;

    // OA Purpose hash tables.
    ctable_t<loa_t> *ldb_oapname_tab;
    itable_t<loa_t> *ldb_oapnum_tab;

    // Removed/unused layers.
    CDll *ldb_unused_phys;
    CDll *ldb_unused_elec;
    CDll *ldb_invalid_list;

    // Saved state.
    CDldbState *ldb_state;

    // Allocator for layer table elements.
    lpp_t *ldb_reuse_list;
    ldb_blk *ldb_blocks;
    int ldb_elts_used;

    // Number of private layer names.
    unsigned int ldb_num_private_lnames;

    // The derived layer table.
    ctable_t<drvlyr_t> *ldb_drvlyr_tab;
    ctable_t<drvlyr_t> *ldb_drvlyr_rmtab;
    int ldb_drv_index;

    static cCDldb *instancePtr;
};


// Some useful inlines...

// The 0'th layer is the cell layer, which is not in the layer table
// but is used to store the spatial location of cell instances.  The
// same layer, named "$$", is used for both electrical and physical. 
// All other indices contain layer table layers, with no assumptions
// about position.

inline CDl *CellLayer()     { return (CDldb()->layer(0, Physical)); }

#endif

