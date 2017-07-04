
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
 $Id: cd_compare.h,v 5.25 2015/06/11 05:54:06 stevew Exp $
 *========================================================================*/

#ifndef CD_COMPARE_H
#define CD_COMPARE_H


//
// Definitions for cell/hierarchy comparison function.
//

// Tokens for log file.
#define DIFF_LTOK "<<<"
#define DIFF_RTOK ">>>"
#define DIFF_LAYER "Layer"
#define DIFF_INSTANCES "Instances"

// Struct to hold property comparison results for an object.
//
struct Pdiff
{
    Pdiff(char *n)
        {
            pd_name =  n;
            pd_list12 = 0;
            pd_list21 = 0;
            pd_next = 0;
        }
    ~Pdiff();

    static void destroy(Pdiff *p)
        {
            while (p) {
                Pdiff *px = p;
                p = p->pd_next;
                delete px;
            }
        }

    void add(const char*, const char*);
    void reset(const char*);
    void print(FILE*);

    static Pdiff *save(Pdiff*, const char*, const char*, const char*);

    Pdiff *next_pdiff()             { return (pd_next); }
    void set_next_pdiff(Pdiff *p)   { pd_next = p; }

private:
    char *pd_name;              // string describing object
    stringlist *pd_list12;      // properties that differ, left
    stringlist *pd_list21;      // properties that differ, right
    Pdiff *pd_next;
};

// Struct to hold comparison results for a layer.
//
struct Ldiff
{
    Ldiff(CDl *ld)
        {
            ld_ldesc = ld;
            ld_list12 = 0;
            ld_list21 = 0;
            ld_pdiffs = 0;
            ld_next = 0;
        }
    ~Ldiff();


    static void destroy(Ldiff *l)
        {
            while (l) {
                Ldiff *lx = l;
                l = l->ld_next;
                delete lx;
            }
        }

    void add(const char*, const char*);
    void print(FILE*, bool = false);

    static Ldiff * save(Ldiff*, CDl*, const char*, const char*);

    void set_pdiffs(Pdiff *p)       { ld_pdiffs = p; }
    Ldiff *next_ldiff()             { return (ld_next); }
    void set_next_ldiff(Ldiff *l)   { ld_next = l; }
    stringlist *list12()            { return (ld_list12); }
    stringlist *list21()            { return (ld_list21); }
    CDl *ldesc()                    { return (ld_ldesc); }

private:
    CDl *ld_ldesc;              // layer compared
    stringlist *ld_list12;      // objects in 1 not 2
    stringlist *ld_list21;      // objects in 2 not 1
    Pdiff *ld_pdiffs;           // property diffs
    Ldiff *ld_next;
};

// Struct to hold comparison results for a cell.
//
struct Sdiff
{
    Sdiff(Ldiff *l, Pdiff *p)
        {
            sd_ldiffs = l;
            sd_pdiffs = p;
        }

    ~Sdiff()
        {
            Ldiff::destroy(sd_ldiffs);
            Pdiff::destroy(sd_pdiffs);
        }

    void print(FILE*);

    Ldiff *ldiffs()             { return (sd_ldiffs); }
    void set_ldiffs(Ldiff *l)   { sd_ldiffs = l; }

    static void ufb_setup(void(*cb)())  { ufb_callback = cb; }
    static void ufb_check() { if (ufb_callback) (*ufb_callback)(); }

private:
    Ldiff *sd_ldiffs;           // layer diffs
    Pdiff *sd_pdiffs;           // cell property diffs

    static void(*ufb_callback)();  // Hook for user progress feedback.
};


// Property filter.
//
struct prpfilt_t
{
    prpfilt_t()
        {
            pf_tab = 0;
            pf_skip = false;
        }

    ~prpfilt_t()
        {
            delete pf_tab;
        }

    void set_skip(bool b)   { pf_skip = b; }

    void add(int n)
        {
            if (!pf_tab)
                pf_tab = new SymTab(false, false);
            pf_tab->add(n, 0, true);
        }

    // Return true if property number n should be considered.
    bool check(int n)
        {
            if (!pf_tab || pf_tab->get(n) == ST_NIL)
                return (pf_skip);
            return (!pf_skip);
        }

    // Return true if filtering everything.
    static bool filter_all(prpfilt_t *pt)
        {
            return (pt && !pt->pf_skip && !pt->pf_tab);
        }

    void parse(const char*);

private:
    SymTab *pf_tab;
    bool pf_skip;
};


// Return from CDdiff content comparison functions.
enum DFtype
{
    DFabort     = -2,       // user aborted comparison
    DFerror     = -1,       // unspecified error
    DFsame      = 0,        // no differences
    DFdiffer    = 1,        // differences found

    // Not used in CDdiff functions.
    DFnoL       = 2,        // null left cell/obj pointer
    DFnoR       = 3,        // null right cell.obj pointer
    DFnoLR      = 4         // both dell/obj pointers null
};

// Property filtering mode passed to CDdiff:setup_filtering.
enum PrpFltMode { PrpFltDflt, PrpFltNone, PrpFltCstm };

// Flags for CDdiff functions.
#define DiffSkipLayers  0x1         // Listed layers will be skipped.
#define DiffSloppyBoxes 0x2         // Compare rect wire/poly as box
#define DiffIgnoreDups  0x4         // Don't report duplicates
#define DiffGeometric   0x8         // Use geometric (trapezoid) comparison.
#define DiffExpArrays   0x10        // Expand instance arrays before comp.
#define DiffPrpCell     0x20        // Compare cell properties.
#define DiffPrpBox      0x40        // Compare box properties.
#define DiffPrpPoly     0x80        // Compare poly properties.
#define DiffPrpWire     0x100       // Compare wire properties.
#define DiffPrpLabl     0x200       // Compare label properties.
#define DiffPrpInst     0x400       // Compare instance properties.
#define DiffPrpCstm     0x800       // Use custom Filter (overrides Dflt).
#define DiffPrpDflt     0x1000      // Use default filter.  It neither of
                                    // Cstm or Dflt set, no filtering.

// Variable names for filtering specifications, used for custom
// filtering mode.
#define VA_PhysPrpFltCell   "PhysPrpFltCell"
#define VA_PhysPrpFltInst   "PhysPrpFltInst"
#define VA_PhysPrpFltObj    "PhysPrpFltObj"
#define VA_ElecPrpFltCell   "ElecPrpFltCell"
#define VA_ElecPrpFltInst   "ElecPrpFltInst"
#define VA_ElecPrpFltObj    "ElecPrpFltObj"

// Main class for comparison operation.
//
class CDdiff
{
public:
    CDdiff()
        {
            cdf_filt_cell = 0;
            cdf_filt_inst = 0;
            cdf_filt_obj = 0;
            cdf_obj_types = 0;
            cdf_layer_list = 0;
            cdf_diffmax = 0;
            cdf_diffcnt = 0;
            cdf_flags = 0;
            cdf_electrical = false;
        }

    ~CDdiff()
        {
            delete cdf_filt_cell;
            delete cdf_filt_inst;
            delete cdf_filt_obj;
            delete [] cdf_obj_types;
            delete [] cdf_layer_list;
        }

    void set_cell_prp_filter(const char *numlist)
        {
            delete cdf_filt_cell;
            cdf_filt_cell = 0;
            if (numlist) {
                cdf_filt_cell = new prpfilt_t;
                cdf_filt_cell->parse(numlist);
            }
        }

    void set_inst_prp_filter(const char *numlist)
        {
            delete cdf_filt_inst;
            cdf_filt_inst = 0;
            if (numlist) {
                cdf_filt_inst = new prpfilt_t;
                cdf_filt_inst->parse(numlist);
            }
        }

    void set_obj_prp_filter(const char *numlist)
        {
            delete cdf_filt_obj;
            cdf_filt_obj = 0;
            if (numlist) {
                cdf_filt_obj = new prpfilt_t;
                cdf_filt_obj->parse(numlist);
            }
        }

    void set_obj_types(const char *ot)
        {
            delete [] cdf_obj_types;
            cdf_obj_types = lstring::copy(ot);
        }

    void set_layer_list(const char *ll)
        {
            delete [] cdf_layer_list;
            cdf_layer_list = lstring::copy(ll);
        }

    void set_max_diffs(unsigned int m)          { cdf_diffmax = m; }
    void set_diff_count(unsigned int c)         { cdf_diffcnt = c; }
    unsigned int diff_count()                   { return (cdf_diffcnt); }
    void set_flags(unsigned int f)              { cdf_flags = f; }

    void setup_filtering(DisplayMode, PrpFltMode);
    DFtype diff(const CDs*, const CDs*, Sdiff**);
    DFtype diff(const CDs*, const CDs*);

private:
    DFtype diff_layer(CDl*, const CDs*, const CDs*, Ldiff**);
    DFtype diff_layer(CDl*, const CDs*, const CDs*);
    DFtype diff_instances(const CDs*, const CDs*, Ldiff**);
    DFtype diff_instances(const CDs*, const CDs*);
    Pdiff *diff_obj_props(const CDo*, const CDo*);
    Pdiff *diff_cell_props(const CDs*, const CDs*);
    Pdiff *diff_plist(const char*, CDp*, CDp*, prpfilt_t*);

    bool max_diffs()
        {
            return (cdf_diffmax > 0 && cdf_diffcnt >= cdf_diffmax);
        }

    prpfilt_t *cdf_filt_cell;
    prpfilt_t *cdf_filt_inst;
    prpfilt_t *cdf_filt_obj;
    const char *cdf_obj_types;
    const char *cdf_layer_list;
    unsigned int cdf_diffmax;
    unsigned int cdf_diffcnt;
    unsigned int cdf_flags;
    bool cdf_electrical;
};

#endif

