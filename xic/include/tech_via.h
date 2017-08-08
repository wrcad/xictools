
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

#ifndef TECH_VIA_H
#define TECH_VIA_H

#include "miscutil/hashfunc.h"


// The cTech::StdViaTab provides the standard vias, as defined for the
// technology, by name.  Each standard via has a variations list of
// instantiated variations different from the standard via itself. 
// The variations list may become a hash table at some point.

// Description of a standard via.  This follows OpenAccess/Virtuoso.
//
struct sStdVia
{
    friend bool operator==(const sStdVia&, const sStdVia&);

    sStdVia(const char *nm, CDl *ldvia, CDl *ldbot, CDl *ldtop)
        {
            sv_next = 0;
            sv_variations = 0;
            sv_reference = 0;
            sv_name = lstring::copy(nm);
            sv_sdesc = 0;
            sv_res_per_cut = 0.0;

            sv_via = ldvia;
            sv_bot = ldbot;
            sv_top = ldtop;

            sv_via_wid = 0;
            sv_via_hei = 0;
            sv_via_rows = 1;
            sv_via_cols = 1;
            sv_via_spa_x = 0;
            sv_via_spa_y = 0;

            sv_bot_enc_x = 0;
            sv_bot_enc_y = 0;
            sv_bot_off_x = 0;
            sv_bot_off_y = 0;

            sv_top_enc_x = 0;
            sv_top_enc_y = 0;
            sv_top_off_x = 0;
            sv_top_off_y = 0;

            sv_org_off_x = 0;
            sv_org_off_y = 0;

            sv_implant1 = 0;
            sv_imp1_enc_x = 0;
            sv_imp1_enc_y = 0;
            sv_implant2 = 0;
            sv_imp2_enc_x = 0;
            sv_imp2_enc_y = 0;
        }

    sStdVia(const sStdVia &v)
        {
            sv_next = 0;
            sv_variations = 0;
            sv_reference = 0;
            sv_name = lstring::copy(v.sv_name);
            sv_sdesc = 0;
            sv_res_per_cut = v.sv_res_per_cut;

            sv_via = v.sv_via;
            sv_bot = v.sv_bot;
            sv_top = v.sv_top;

            sv_via_wid = v.sv_via_wid;
            sv_via_hei = v.sv_via_hei;
            sv_via_rows = v.sv_via_rows;
            sv_via_cols = v.sv_via_cols;
            sv_via_spa_x = v.sv_via_spa_x;
            sv_via_spa_y = v.sv_via_spa_y;

            sv_bot_enc_x = v.sv_bot_enc_x;
            sv_bot_enc_y = v.sv_bot_enc_y;
            sv_bot_off_x = v.sv_bot_off_x;
            sv_bot_off_y = v.sv_bot_off_y;

            sv_top_enc_x = v.sv_top_enc_x;
            sv_top_enc_y = v.sv_top_enc_y;
            sv_top_off_x = v.sv_top_off_x;
            sv_top_off_y = v.sv_top_off_y;

            sv_org_off_x = v.sv_org_off_x;
            sv_org_off_y = v.sv_org_off_y;

            sv_implant1 = v.sv_implant1;
            sv_imp1_enc_x = v.sv_imp1_enc_x;
            sv_imp1_enc_y = v.sv_imp1_enc_y;
            sv_implant2 = v.sv_implant2;
            sv_imp2_enc_x = v.sv_imp2_enc_x;
            sv_imp2_enc_y = v.sv_imp2_enc_y;
        }

    ~sStdVia()
        {
            delete [] sv_name;
        }

    void clear_variations()
        {
            while (sv_variations) {
                sStdVia *vx = sv_variations;
                sv_variations = sv_variations->sv_variations;
                delete vx;
            }
        }

    unsigned int hash(unsigned int mask)
        {
            unsigned int k = INCR_HASH_INIT;
            unsigned int *p = (unsigned int*)sv_via;
            while (p <= (unsigned int*)&sv_org_off_y) {
                k = incr_hash(k, p);
                p++;
            }
            if (sv_implant1) {
                while (p <= (unsigned int*)&sv_imp1_enc_y) {
                    k = incr_hash(k, p);
                    p++;
                }
                if (sv_implant2) {
                    while (p <= (unsigned int*)&sv_imp2_enc_y) {
                        k = incr_hash(k, p);
                        p++;
                    }
                }
            }

            return (k & mask);
        }

    bool parse(const char*);
    char *string() const;
    CDs *open();
    void reset();
    void tech_print(FILE*) const;

    static TCret tech_parse(const char*);

    void clean_pre_insert()
        {
            sv_next = 0;
            sv_variations = 0;
            sv_reference = 0;
            sv_sdesc = 0;
        }

    sStdVia *tab_next()             const { return (sv_next); }
    sStdVia *tgen_next(bool)        const { return (sv_next); }
    void set_tab_next(sStdVia *n)   { sv_next = n; }

    sStdVia *variations()           const { return (sv_variations); }
    void add_variation(sStdVia *v)
        {
            if (!v)
                return;
            v->sv_variations = sv_variations;
            sv_variations = v;
            v->sv_reference = this;
            delete [] v->sv_name;
            v->sv_name = 0;
            v->sv_sdesc = 0;
        }

    const char *tab_name()          const { return (sv_name); }
    CDs *sdesc()                    const { return (sv_sdesc); }

    double res_per_cut()            const { return (sv_res_per_cut); }
    void set_res_per_cut(double r)  { sv_res_per_cut = r; }

    CDl *via()                      const { return (sv_via); }
    void set_via(CDl *ld)           { sv_via = ld; }
    CDl *bottom()                   const { return (sv_bot); }
    void set_bottom(CDl *ld)        { sv_bot = ld; }
    CDl *top()                      const { return (sv_top); }
    void set_top(CDl *ld)           { sv_top = ld; }

    int via_wid()                   const { return (sv_via_wid); }
    void set_via_wid(int w)         { sv_via_wid = w; }
    int via_hei()                   const { return (sv_via_hei); }
    void set_via_hei(int h)         { sv_via_hei = h; }
    int via_rows()                  const { return (sv_via_rows); }
    void set_via_rows(int i)        { sv_via_rows = i; }
    int via_cols()                  const { return (sv_via_cols); }
    void set_via_cols(int i)        { sv_via_cols = i; }
    int via_spa_x()                 const { return (sv_via_spa_x); }
    void set_via_spa_x(int i)       { sv_via_spa_x = i; }
    int via_spa_y()                 const { return (sv_via_spa_y); }
    void set_via_spa_y(int i)       { sv_via_spa_y = i; }

    int bot_enc_x()                 const { return (sv_bot_enc_x); }
    void set_bot_enc_x(int i)       { sv_bot_enc_x = i; }
    int bot_enc_y()                 const { return (sv_bot_enc_y); }
    void set_bot_enc_y(int i)       { sv_bot_enc_y = i; }
    int bot_off_x()                 const { return (sv_bot_off_x); }
    void set_bot_off_x(int i)       { sv_bot_off_x = i; }
    int bot_off_y()                 const { return (sv_bot_off_y); }
    void set_bot_off_y(int i)       { sv_bot_off_y = i; }

    int top_enc_x()                 const { return (sv_top_enc_x); }
    void set_top_enc_x(int i)       { sv_top_enc_x = i; }
    int top_enc_y()                 const { return (sv_top_enc_y); }
    void set_top_enc_y(int i)       { sv_top_enc_y = i; }
    int top_off_x()                 const { return (sv_top_off_x); }
    void set_top_off_x(int i)       { sv_top_off_x = i; }
    int top_off_y()                 const { return (sv_top_off_y); }
    void set_top_off_y(int i)       { sv_top_off_y = i; }

    int org_off_x()                 const { return (sv_org_off_x); }
    void set_org_off_x(int i)       { sv_org_off_x = i; }
    int org_off_y()                 const { return (sv_org_off_y); }
    void set_org_off_y(int i)       { sv_org_off_y = i; }

    CDl *implant1()                 const { return (sv_implant1); }
    void set_implant1(CDl *ld)      { sv_implant1 = ld; }
    int imp1_enc_x()                const { return (sv_imp1_enc_x); }
    void set_imp1_enc_x(int i)      { sv_imp1_enc_x = i; }
    int imp1_enc_y()                const { return (sv_imp1_enc_y); }
    void set_imp1_enc_y(int i)      { sv_imp1_enc_y = i; }
    CDl *implant2()                 const { return (sv_implant2); }
    void set_implant2(CDl *ld)      { sv_implant2 = ld; }
    int imp2_enc_x()                const { return (sv_imp2_enc_x); }
    void set_imp2_enc_x(int i)      { sv_imp2_enc_x = i; }
    int imp2_enc_y()                const { return (sv_imp2_enc_y); }
    void set_imp2_enc_y(int i)      { sv_imp2_enc_y = i; }

private:
    sStdVia *sv_next;
    sStdVia *sv_variations;
    sStdVia *sv_reference;
    char *sv_name;
    CDs *sv_sdesc;
    double sv_res_per_cut;

    // WARNING
    // Don't change this, the offsets are holy, and used in the
    // XICP_STDVIA property.

    CDl *sv_via;
    CDl *sv_bot;
    CDl *sv_top;

    int sv_via_wid;
    int sv_via_hei;
    int sv_via_rows;
    int sv_via_cols;
    int sv_via_spa_x;
    int sv_via_spa_y;

    int sv_bot_enc_x;
    int sv_bot_enc_y;
    int sv_bot_off_x;
    int sv_bot_off_y;

    int sv_top_enc_x;
    int sv_top_enc_y;
    int sv_top_off_x;
    int sv_top_off_y;

    int sv_org_off_x;
    int sv_org_off_y;

    CDl *sv_implant1;
    int sv_imp1_enc_x;
    int sv_imp1_enc_y;
    CDl *sv_implant2;
    int sv_imp2_enc_x;
    int sv_imp2_enc_y;
};


inline bool operator==(const sStdVia &v1, const sStdVia &v2)
{
    unsigned int *i1 = (unsigned int*)&v1.sv_via;
    unsigned int *i2 = (unsigned int*)&v2.sv_via;
    while (i1 <= (unsigned int*)&v1.sv_org_off_y) {
        if (*i1 != *i2)
            return (false);
        i1++;
        i2++;
    }
    if (v1.sv_implant1 || v2.sv_implant1) {
        while (i1 <= (unsigned int*)&v1.sv_imp1_enc_y) {
            if (*i1 != *i2)
                return (false);
            i1++;
            i2++;
        }
        if (v1.sv_implant2 || v2.sv_implant2) {
            while (i1 <= (unsigned int*)&v1.sv_imp2_enc_y) {
                if (*i1 != *i2)
                    return (false);
                i1++;
                i2++;
            }
        }
    }
    return (true);
}


struct ParseNode;

// Structure describing a via object.
//
struct sVia
{
    // layers.cc
    sVia(char* = 0, char* = 0, ParseNode* = 0);
    ~sVia();

    static void destroy(sVia *v)
        {
            while (v) {
                sVia *vx = v;
                v = v->v_next;
                delete vx;
            }
        }

    sVia *next()                    { return (v_next); }
    void setNext(sVia *n)           { v_next = n; }

    const char *layername1()    const { return (v_lname1); }
    const char *layername2()    const { return (v_lname2); }
    CDl *layer1()                   { return (v_ld1 ? v_ld1 : getld1()); }
    CDl *layer2()                   { return (v_ld2 ? v_ld2 : getld2()); }

    ParseNode *tree()           const { return (v_tree); }

    CDl *top_layer();
    CDl *bottom_layer();

private:
    CDl *getld1();
    CDl *getld2();

    sVia *v_next;
    char *v_lname1;                 // conductor 1 layer name
    char *v_lname2;                 // conductor 2 layer name
    CDl *v_ld1;                     // cached conductor 1 layer desc
    CDl *v_ld2;                     // cached conductor 2 layer desc
    ParseNode *v_tree;              // parse tree for conjunction
};

#endif

