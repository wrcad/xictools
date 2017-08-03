
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef EXT_ANTENNA_H
#define EXT_ANTENNA_H

#include "ext_pathfinder.h"
#include "cd_netname.h"

// Deprecated in C++11
// #define THROW_int throw(int)
#define THROW_int

inline class cAntParams *AP();

class cAntParams
{
    static cAntParams *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cAntParams *AP() { return (cAntParams::ptr()); }

    cAntParams();

    // Return true if ci corresponds to a MOS gate.
    //
    bool is_mos_gate(const sDevContactInst *ci)
        {
            if (ci && ci->cont_name() == ap_gate_name) {
                sDevInst *dev = ci->dev();
                if (!dev || !dev->desc())
                    return (false);
                sDevDesc *dd = dev->desc();
                if (!dd->name())
                    return (false);
                for (itemlist<CDcellName> *s = ap_mos_names; s;
                        s = s->next) {
                    if (dd->name() == s->item)
                        return (true);
                }
            }
            return (false);
        }


    void set_gate_name(const char *gn)
        {
            if (gn)
                ap_gate_name = CDnetex::name_tab_add(gn);
        }

    CDnetName gate_name()       const { return (ap_gate_name); }

    void set_mos_name(const char *mname, bool del = false)
        {
            if (!mname)
                return;
            CDcellName mn = CD()->CellNameTableAdd(mname);
            itemlist<CDcellName> *sp = 0, *sn;
            for (itemlist<CDcellName> *s = ap_mos_names; s; s = sn) {
                sn = s->next;
                if (mn == s->item) {
                    if (del) {
                        if (sp)
                            sp->next = sn;
                        else
                            ap_mos_names = sn;
                        delete s;
                    }
                    return;
                }
                sp = s;
            }
            if (!del)
                ap_mos_names = new itemlist<CDcellName>(mn, ap_mos_names);
        }

    const itemlist<CDcellName> *mos_names() const { return (ap_mos_names); }

private:
    CDnetName               ap_gate_name;
    itemlist<CDcellName>    *ap_mos_names;

    static cAntParams *instancePtr;
};


// Per-layer limit list element.
//
struct alimit_t
{
    alimit_t(const char *l, double r, alimit_t *n)
        {
            al_lname = lstring::copy(l);
            al_max_ratio = r;
            next = n;
        }

    ~alimit_t() { delete [] al_lname; }

    const char *al_lname;
    double al_max_ratio;
    alimit_t *next;
};

// Box list element with filling factor.
//
struct abl_t
{
    abl_t(const BBox *bb, float f, abl_t *n)
        {
            BB = *bb;
            factor = f;
            next = n;
        }

    static void destroy(abl_t *bl)
        {
            while (bl) {
                abl_t *bx = bl;
                bl = bl->next;
                delete bx;
            }
        }

    static int length(const abl_t *bl)
        {
            int cnt = 0;
            while (bl) {
                cnt++;
                bl = bl->next;
            }
            return (cnt);
        }


    static double farea(const abl_t *bl)
        {
            double a = 0.0;
            while (bl) {
                a += bl->factor * bl->BB.area();
                bl = bl->next;
            }
            return (a);
        }

    float factor;
    BBox BB;
    abl_t *next;
};

// MOS gate representation list element.
//
struct gate_t
{
    gate_t(const char *p, abl_t *bl, gate_t *n)
        {
            pathname = p;
            bbs = bl;
            next = n;
        }

    ~gate_t() { abl_t::destroy(bbs); }

    gate_t *next;
    const char *pathname;   // unique gate name path, pointer to
                            // symtab data, don't free!
    abl_t *bbs;             // gate area(s), multi components supported.
};

struct ant_pathfinder : public pathfinder, public cTfmStack
{
    ant_pathfinder()
        {
            pf_topcell = 0;
            pf_gates = 0;
            pf_gate_tab = 0;
            pf_layer_specs = 0;
            pf_outfp = 0;
            pf_max_ratio = 0.0;
            pf_netcnt = 0;
            pf_found_ground = false;
        }

    ~ant_pathfinder()
        {
            clear();
            delete pf_gate_tab;
            while (pf_layer_specs) {
                alimit_t *ax = pf_layer_specs;
                pf_layer_specs = pf_layer_specs->next;
                delete ax;
            }
        }

    void clear()
        {
            pf_found_ground = false;
            pathfinder::clear();
            while (pf_gates) {
                gate_t *gx = pf_gates;
                pf_gates = pf_gates->next;
                delete gx;
            }
        }

    bool using_groups() { return (true); }

    // Set up or delete (lim <= 0) a per-layer limit.
    //
    void set_layer_lim(const char *lname, double lim)
        {
            if (!lname)
                return;
            alimit_t *ap = 0, *an;
            for (alimit_t *a = pf_layer_specs; a; a = an) {
                an = a->next;
                if (!strcmp(lname, a->al_lname)) {
                    if (lim <= 0.0) {
                        if (ap)
                            ap->next = an;
                        else
                            pf_layer_specs = an;
                        delete a;
                        return;
                    }
                    a->al_max_ratio = lim;
                    return;
                }
                ap = a;
            }
            if (lim > 0.0)
                pf_layer_specs = new alimit_t(lname, lim, pf_layer_specs);
        }

    void set_lim(double lim) { pf_max_ratio = lim; }
    const alimit_t *specs() { return (pf_layer_specs); }
    double limit() { return (pf_max_ratio); }

    void set_output(FILE *fp) { pf_outfp = fp; }

    bool find_antennae(CDs*);
    bool find_antennae_rc(CDs*, int) THROW_int;
    void process(const sDevContactInst*);

    static bool read_file(int*, BBox*);

private:
    char *dev_name(const sDevContactInst*, int);
    abl_t *dev_contacts(const sDevContactInst*);
    void recurse_up(int, int);
    void recurse_path(cGroupDesc*, int, int);
    void recurse_gnd_path(cGroupDesc*, int);

    CDs *pf_topcell;
    gate_t *pf_gates;
    SymTab *pf_gate_tab;
    alimit_t *pf_layer_specs;
    FILE *pf_outfp;
    double pf_max_ratio;
    sSubcInst *pf_subc_stack[CDMAXCALLDEPTH];
    int pf_netcnt;
    bool pf_found_ground;
};

#endif

