
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

#include "cd.h"
#include "cd_types.h"
#include "cd_strmdata.h"
#include "cd_lgen.h"
#include <algorithm>
#include <ctype.h>


#ifndef M_PI
#define	M_PI  3.14159265358979323846  // pi
#endif

// Statics, from the extraction layer generator.
CDextLtab *CDextLgen::eg_ltab;
bool CDextLgen::eg_dirty;


CDl::CDl(CDLtype t)
{
    ld_flags            = 0;
    ld_flags2           = t;
    ld_drv_mode         = CLdefault;
    ld_drv_index        = 0;
    ld_datatypes[0]     = 0;
    ld_datatypes[1]     = 0;
    ld_datatypes[2]     = 0;
    ld_datatypes[3]     = 0;
    ld_oa_layernum      = 0;
    ld_oa_purpose       = 0;
    ld_strm_in          = 0;
    ld_strm_out         = 0;
    ld_idname           = 0;
    ld_lpp_name         = 0;
    ld_description      = 0;
    ld_drv_expr         = 0;
    ld_dsp_data         = 0;
    ld_app_data         = 0;
    ld_phys_index       = -1;
    ld_elec_index       = -1;

    CD()->ifNewLayer(this);
}


CDl::~CDl()
{
    delete ld_strm_in;
    strm_odata::destroy(ld_strm_out);
    delete [] ld_idname;
    delete [] ld_description;
    delete [] ld_drv_expr;
    CD()->ifInvalidateLayer(this);
}


// Return a pointer to the OA layer name.
//
const char *
CDl::oaLayerName() const
{
    return (CDldb()->getOAlayerName(ld_oa_layernum));
}


// Return a pointer to the OA purpose name.
//
const char *
CDl::oaPurposeName() const
{
    return (CDldb()->getOApurposeName(ld_oa_purpose));
}


namespace {
    bool in_order(const strm_odata *s1, const strm_odata *s2)
    {
        if (s1->layer() < s2->layer())
            return (true);
        if (s1->layer() > s2->layer())
            return (false);
        return (s1->dtype() < s2->dtype());
    }
}


// Add an output mapping record.
//
void
CDl::addStrmOut(unsigned int lnum, unsigned int dtype)
{
    if (lnum >= GDS_MAX_LAYERS || dtype >= GDS_MAX_DTYPES)
        return;
    for (strm_odata *so = ld_strm_out; so; so = so->next()) {
        if (so->layer() == lnum && so->dtype() == dtype)
            return;
        if (so->layer() > lnum)
            break;
    }
    strm_odata *so = new strm_odata(lnum, dtype);
    if (!ld_strm_out)
        ld_strm_out = so;
    else if (in_order(so, ld_strm_out)) {
        so->set_next(ld_strm_out);
        ld_strm_out = so;
    }
    else {
        strm_odata *sn;
        for (strm_odata *sd = ld_strm_out; sd; sd = sn) {
            sn = sd->next();
            if (in_order(sd, so) && (!sn || in_order(so, sn))) {
                so->set_next(sn);
                sd->set_next(so);
                break;
            }
        }
    }
}


// Return the first layer/datatype record found.  These will be the
// parameters used in layers exported to OpenAccess.  Return false if
// no record.
//
bool
CDl::getStrmOut(unsigned int *lnum, unsigned int *dtype, const CDo *odesc)
{
    if (!ld_strm_out) {
        *lnum = -1;
        *dtype = -1;
        return (false);
    }
    *lnum = ld_strm_out->layer();
    if (odesc && odesc->has_flag(CDnoDRC) && (ld_flags & CDL_NODRC))
        *dtype = ld_datatypes[CDNODRC_DT];
    else
        *dtype = ld_strm_out->dtype();
    return (true);
}


// Delete matching output mapping records.  Negative argument
// values represent "all".  Return the number of records deleted.
//
int
CDl::clearStrmOut(unsigned int lnum, int unsigned dtype)
{
    int cnt = 0;
    strm_odata *sop = 0, *son;
    for (strm_odata *so = ld_strm_out; so; so = son) {
        son = so->next();
        if ((so->layer() == lnum || (int)lnum < 0) &&
                (so->dtype() == dtype || (int)dtype < 0)) {
            if (!sop)
                ld_strm_out = son;
            else
                sop->set_next(son);
            delete so;
            cnt++;
            continue;
        }
        sop = so;
    }
    return (cnt);
}


// Return the datatype to use for output of the DRC suppression
// datatype is set up.
//
unsigned int
CDl::getStrmNoDrcDatatype(const CDo *odesc)
{
    if (odesc->has_flag(CDnoDRC) && (ld_flags & CDL_NODRC))
        return (ld_datatypes[CDNODRC_DT]);
    return (-1);
}


// Parse string and generate an input mapping record.  Format is
// xx yy-zz, aa bb-cc   (layers, datatypes).
// Each field can have any number of numbers or number ranges.  The
// datatypes apply to all layers given.
//
bool
CDl::setStrmIn(const char *string)
{
    const char *msg = "Parse error in layer specification.";
    bool newhere = false;
    if (!ld_strm_in) {
        ld_strm_in = new strm_idata;
        newhere = true;
    }
    if (!ld_strm_in->parse_lspec(&string)) {
        Errs()->add_error(msg);
        if (newhere) {
            delete ld_strm_in;
            ld_strm_in = 0;
        }
        return (false);
    }
    return (true);
}


bool
CDl::setStrmIn(unsigned int lnum, unsigned int dtype)
{
    if (lnum >= GDS_MAX_LAYERS)
        return (false);
    bool all = false;
    if ((int)dtype < 0)
        all = true;
    else if (dtype >= GDS_MAX_DTYPES)
        return (false);
    strm_idata *si = strmIn();
    if (!si) {
        si = new strm_idata;
        setStrmIn(si);
    }
    si->set_lspec(lnum, lnum, false);
    if (all)
        si->enable_all_dtypes();
    else
        si->set_lspec(dtype, dtype, true);
    return (true);
}


// Return true if the layer has an input mapping to the layer/datatype
// given.
//
bool
CDl::isStrmIn(unsigned int lnum, unsigned int dtype)
{
    if (ld_strm_in)
        return (ld_strm_in->check(lnum, dtype));
    return (false);
}


int
CDl::clearStrmIn()
{
    int cnt = (ld_strm_in != 0);
    delete ld_strm_in;
    ld_strm_in = 0;
    return (cnt);
}


int
CDl::getStrmDatatypeFlags(int dtype)
{
    if (dtype >= 0 && (ld_flags & CDL_NODRC) &&
            dtype == ld_datatypes[CDNODRC_DT])
        return (CDnoDRC);
    return (0);
}


// Static function.
// Return a token, delimited by white space and '=', advance s.
//
char *
CDl::get_layer_tok(const char **s)
{
    if (s == 0 || *s == 0)
        return (0);
    while (isspace(**s) || **s == '=')
        (*s)++;
    if (!**s)
        return (0);
    const char *st = *s;
    while (**s && !isspace(**s) && **s != '=')
        (*s)++;
    char *cbuf = new char[*s - st + 1];
    char *c = cbuf;
    while (st < *s)
        *c++ = *st++;
    *c = 0;
    while (isspace(**s) || **s == '=')
        (*s)++;
    return (cbuf);
}


// Set the id name, which has the format "layername[:purposename]". 
// The purposename is omitted if the purpose is "drawing".  This
// private function is called by friend cCDldb when the layer is
// placed in the layer table or the OA layer name is changed.
//
void
CDl::setIdName()
{
    delete [] ld_idname;
    const char *lname = CDldb()->getOAlayerName(ld_oa_layernum);
    if (!lname) {
        ld_idname = lstring::copy("");
        return;
    }
    const char *pname = CDldb()->getOApurposeName(ld_oa_purpose);
    if (!pname)
        ld_idname = lstring::copy(lname);
    else {
        ld_idname = new char[strlen(lname) + strlen(pname) + 2];
        char *p = lstring::stpcpy(ld_idname, lname);
        *p++ = ':';
        strcpy(p, pname);
    }
}
// End of CDl functions.


namespace {
    DisplayMode tmp_mode;

    inline bool db_lcmp(const CDl *l1, const CDl *l2)
    {
        return (l1->index(tmp_mode) < l2->index(tmp_mode));
    }
}


// Create a sorted layer list for the generator.
//
void
CDsLgen::sort()
{
    if (g_size <= 1)
        return;

    int sz = 0;
    g_descs = new CDl*[g_size];
    const CDtree *rt = g_tree;
    for (int i = 0; i < g_size; i++) {
        if (rt->num_elements()) {
            CDl *ld = rt->ldesc();
            if (ld && ((ld->index(g_mode) > 0) ||
                    (g_with_cell && ld->index(g_mode) == 0)))
                g_descs[sz++] = ld;
        }
        rt++;
    }
    g_size = sz;
    tmp_mode = g_mode;
    std::sort(g_descs, g_descs + g_size, db_lcmp);
}
// End of CDsLgen functions

