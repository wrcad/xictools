
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
 $Id: cfilter.cc,v 5.3 2015/03/21 19:50:24 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_propnum.h"
#include "cd_celldb.h"
#include "cfilter.h"


//
// cfilter_t:  A struct to filter cell listings.
//

cfilter_t::cfilter_t(DisplayMode m)
{
    cf_p_list = 0;
    cf_np_list = 0;
    cf_s_list = 0;
    cf_ns_list = 0;
    cf_l_list = 0;
    cf_nl_list = 0;
    cf_flags = 0;
    cf_f_bits = 0;
    cf_nf_bits = 0;
    cf_ft_mask = 0;
    cf_nft_mask = 0;
    cf_mode = m;
}


cfilter_t::~cfilter_t()
{
    cf_p_list->free();
    cf_np_list->free();
    cf_s_list->free();
    cf_ns_list->free();
    CDll::destroy(cf_l_list);
    CDll::destroy(cf_nl_list);
}


// Setup mode-specific default filtering.
//
void
cfilter_t::set_default()
{
    cf_p_list->free();
    cf_np_list->free();
    cf_s_list->free();
    cf_ns_list->free();
    CDll::destroy(cf_l_list);
    CDll::destroy(cf_nl_list);
    cf_f_bits = 0;
    cf_nf_bits = 0;
    cf_ft_mask = 0;
    cf_nft_mask = 0;

    if (cf_mode == Physical)
        cf_flags = CF_NOTVIASUBM | CF_NOTPCELLSUBM;
    else
        cf_flags = CF_NOTDEVICE;
}


// Return true if the mode part of cbin should be listed.  This will be
// the logical AND of the various clauses.
//
bool
cfilter_t::inlist(const CDcbin *cbin) const
{
    const CDs *sdesc = cbin->celldesc(cf_mode);
    if (!sdesc)
        return (false);

    if (cf_flags & CF_VIASUBM) {
        if (!sdesc->isViaSubMaster())
            return (false);
    }
    if (cf_flags & CF_NOTVIASUBM) {
        if (sdesc->isViaSubMaster())
            return (false);
    }
    if (cf_flags & CF_PCELLSUBM) {
        if (!sdesc->isPCellSubMaster())
            return (false);
    }
    if (cf_flags & CF_NOTPCELLSUBM) {
        if (sdesc->isPCellSubMaster())
            return (false);
    }
    if (cf_flags & CF_PCELLSUPR) {
        if (!sdesc->isPCellSuperMaster())
            return (false);
    }
    if (cf_flags & CF_NOTPCELLSUPR) {
        if (sdesc->isPCellSuperMaster())
            return (false);
    }
    if (cf_flags & CF_IMMUTABLE) {
        if (!sdesc->isImmutable())
            return (false);
    }
    if (cf_flags & CF_NOTIMMUTABLE) {
        if (sdesc->isImmutable())
            return (false);
    }
    if (cf_flags & CF_DEVICE) {
        if (!cbin->isDevice())
            return (false);
    }
    if (cf_flags & CF_NOTDEVICE) {
        if (cbin->isDevice())
            return (false);
    }
    if (cf_flags & CF_LIBRARY) {
        if (!sdesc->isLibrary())
            return (false);
    }
    if (cf_flags & CF_NOTLIBRARY) {
        if (sdesc->isLibrary())
            return (false);
    }
    if (cf_flags & CF_MODIFIED) {
        if (!sdesc->isModified())
            return (false);
    }
    if (cf_flags & CF_NOTMODIFIED) {
        if (sdesc->isModified())
            return (false);
    }
    if (cf_flags & CF_REFERENCE) {
        if (!sdesc->prpty(XICP_CHD_REF))
            return (false);
    }
    if (cf_flags & CF_NOTREFERENCE) {
        if (!sdesc->prpty(XICP_CHD_REF))
            return (false);
    }
    if (cf_flags & CF_TOPLEV) {
        if (sdesc->isSubcell())
            return (false);
    }
    if (cf_flags & CF_NOTTOPLEV) {
        if (!sdesc->isSubcell())
            return (false);
    }
    if (cf_flags & CF_WITHALT) {
        if (!cbin->alt_celldesc(cf_mode))
            return (false);
    }
    if (cf_flags & CF_NOTWITHALT) {
        if (cbin->alt_celldesc(cf_mode))
            return (false);
    }
    if (cf_flags & CF_PARENT) {
        if (cf_p_list) {
            // Return false if cell does not contain any of listed subcells.
            bool found = false;
            for (cnlist_t *s = cf_p_list; s; s = s->next) {
                if (sdesc->findMaster(s->cname)) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return (false);
        }
        else {
            // Return false if cell does not contain any subcells.
            bool found = false;
            CDm_gen gen(sdesc, GEN_MASTERS);
            for (CDm *md = gen.m_first(); md; md = gen.m_next()) {
                if (md->hasInstances()) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return (false);
        }
    }
    if (cf_flags & CF_NOTPARENT) {
        if (cf_np_list) {
            // Return false if cell contains any of listed subcells.
            for (cnlist_t *s = cf_np_list; s; s = s->next) {
                if (sdesc->findMaster(s->cname))
                    return (false);
            }
        }
        else {
            // Return false if cell contains any subcells.
            CDm_gen gen(sdesc, GEN_MASTERS);
            for (CDm *md = gen.m_first(); md; md = gen.m_next()) {
                if (md->hasInstances())
                    return (false);
            }
        }
    }
    if (cf_flags & CF_SUBCELL) {
        if (cf_s_list) {
            // Return false if cell is not used as a subcell in listed parent.
            bool found = false;
            for (cnlist_t *s = cf_s_list; s; s = s->next) {
                CDs *prnt = CDcdb()->findCell(s->cname, cf_mode);
                if (!prnt)
                    continue;
                if (prnt->findMaster(sdesc->cellname())) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return (false);
        }
        else {
            // Return false if cell in not used as a subcell.
            if (!sdesc->isSubcell())
                return (false);
        }
    }
    if (cf_flags & CF_NOTSUBCELL) {
        if (cf_np_list) {
            // Return false if cell is used as a subcell in listed parent.
            for (cnlist_t *s = cf_np_list; s; s = s->next) {
                CDs *prnt = CDcdb()->findCell(s->cname, cf_mode);
                if (!prnt)
                    continue;
                if (prnt->findMaster(sdesc->cellname()))
                    return (false);
            }
        }
        else {
            // Return false if cell is used as a subcell.
            if (sdesc->isSubcell())
                return (false);
        }
    }
    if (cf_flags & CF_LAYER) {
        if (cf_l_list) {
            // Return false if cell does not contain any of listed layers.
            bool found = false;
            for (CDll *ll = cf_l_list; ll; ll = ll->next) {
                RTree *rt = sdesc->db_find_layer_head(ll->ldesc);
                if (rt && rt->first_element()) {
                    found = true;
                    break;
                }
            }
            if (!found)
                return (false);
        }
        else {
            // Return false if cell does not contain any geometry.
            if (sdesc->db_is_empty(0))
                return (false);
        }
    }
    if (cf_flags & CF_NOTLAYER) {
        if (cf_l_list) {
            // Return false if cell contains any of listed layers.
            for (CDll *ll = cf_nl_list; ll; ll = ll->next) {
                RTree *rt = sdesc->db_find_layer_head(ll->ldesc);
                if (rt && rt->first_element())
                    return (false);
            }
        }
        else {
            // Return false if cell contains geometry.
            if (!sdesc->db_is_empty(0))
                return (false);
        }
    }
    if (cf_flags & CF_FLAG) {
        if (cf_f_bits) {
            // Return false if cell does not have any masked flags set.
            if (!(sdesc->getFlags() & cf_f_bits))
                return (false);
        }
    }
    if (cf_flags & CF_NOTFLAG) {
        if (cf_nf_bits) {
            // Return false if cell does has any masked flags set.
            if (sdesc->getFlags() & cf_nf_bits)
                return (false);
        }
    }
    if (cf_flags & CF_FTYPE) {
        if (cf_ft_mask) {
            // Return false if mask bit is not set for file type.
            if (!(cf_ft_mask & (1 <<sdesc->fileType())))
                return (false);
        }
    }
    if (cf_flags & CF_NOTFTYPE) {
        if (cf_nft_mask) {
            // Return false if mask bit is set for file type.
            if (cf_ft_mask & (1 << sdesc->fileType()))
                return (false);
        }
    }
    return (true);
}


char *
cfilter_t::prnt_list() const
{
    sLstr lstr;
    for (cnlist_t *s = cf_p_list; s; s = s->next) {
        lstr.add(s->cname->string());
        if (s->next)
            lstr.add_c(' ');
    }
    return (lstr.string_trim());
}


char *
cfilter_t::not_prnt_list() const
{
    sLstr lstr;
    for (cnlist_t *s = cf_np_list; s; s = s->next) {
        lstr.add(s->cname->string());
        if (s->next)
            lstr.add_c(' ');
    }
    return (lstr.string_trim());
}


char *
cfilter_t::subc_list() const
{
    sLstr lstr;
    for (cnlist_t *s = cf_s_list; s; s = s->next) {
        lstr.add(s->cname->string());
        if (s->next)
            lstr.add_c(' ');
    }
    return (lstr.string_trim());
}


char *
cfilter_t::not_subc_list() const
{
    sLstr lstr;
    for (cnlist_t *s = cf_ns_list; s; s = s->next) {
        lstr.add(s->cname->string());
        if (s->next)
            lstr.add_c(' ');
    }
    return (lstr.string_trim());
}


char *
cfilter_t::layer_list() const
{
    sLstr lstr;
    for (CDll *l = cf_l_list; l; l = l->next) {
        lstr.add(l->ldesc->name());
        if (l->next)
            lstr.add_c(' ');
    }
    return (lstr.string_trim());
}


char *
cfilter_t::not_layer_list() const
{
    sLstr lstr;
    for (CDll *l = cf_nl_list; l; l = l->next) {
        lstr.add(l->ldesc->name());
        if (l->next)
            lstr.add_c(' ');
    }
    return (lstr.string_trim());
}


char *
cfilter_t::flag_list() const
{
    sLstr lstr;
    bool first = true;
    for (FlagDef *f = SdescFlags; f->name; f++) {
        if (f->value & cf_f_bits) {
            if (first)
                first = false;
            else
                lstr.add_c(' ');
            lstr.add(f->name);
        }
    }
    return (lstr.string_trim());
}


char *
cfilter_t::not_flag_list() const
{
    sLstr lstr;
    bool first = true;
    for (FlagDef *f = SdescFlags; f->name; f++) {
        if (f->value & cf_nf_bits) {
            if (first)
                first = false;
            else
                lstr.add_c(' ');
            lstr.add(f->name);
        }
    }
    return (lstr.string_trim());
}


char *
cfilter_t::ftype_list() const
{
    sLstr lstr;
    int cnt = 0;
    if (cf_ft_mask & (1 << Fnone)) {
        lstr.add("none");
        cnt++;
    }
    if (cf_ft_mask & (1 << Fnative)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("native");
        cnt++;
    }
    if (cf_ft_mask & (1 << Fgds)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("gds");
        cnt++;
    }
    if (cf_ft_mask & (1 << Fcgx)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("cgx");
        cnt++;
    }
    if (cf_ft_mask & (1 << Foas)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("oas");
        cnt++;
    }
    if (cf_ft_mask & (1 << Fcif)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("cif");
        cnt++;
    }
    if (cf_ft_mask & (1 << Foa)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("opa");
        cnt++;
    }
    return (lstr.string_trim());
}


char *
cfilter_t::not_ftype_list() const
{
    sLstr lstr;
    int cnt = 0;
    if (cf_nft_mask & (1 << Fnone)) {
        lstr.add("none");
        cnt++;
    }
    if (cf_nft_mask & (1 << Fnative)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("native");
        cnt++;
    }
    if (cf_nft_mask & (1 << Fgds)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("gds");
        cnt++;
    }
    if (cf_nft_mask & (1 << Fcgx)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("cgx");
        cnt++;
    }
    if (cf_nft_mask & (1 << Foas)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("oas");
        cnt++;
    }
    if (cf_nft_mask & (1 << Fcif)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("cif");
        cnt++;
    }
    if (cf_nft_mask & (1 << Foa)) {
        if (cnt)
            lstr.add_c(' ');
        lstr.add("opa");
        cnt++;
    }
    return (lstr.string_trim());
}


// Return a reconsitiuted options string.
//
char *
cfilter_t::string() const
{
    sLstr lstr;
    if (cf_flags & CF_VIASUBM)
        lstr.add(" viasubm");
    if (cf_flags & CF_NOTVIASUBM)
        lstr.add(" notviasubm");
    if (cf_flags & CF_PCELLSUBM)
        lstr.add(" pcellsubm");
    if (cf_flags & CF_NOTPCELLSUBM)
        lstr.add(" notpcellsubm");
    if (cf_flags & CF_PCELLSUPR)
        lstr.add(" pcellsupr");
    if (cf_flags & CF_NOTPCELLSUPR)
        lstr.add(" notpcellsupr");
    if (cf_flags & CF_IMMUTABLE)
        lstr.add(" immutable");
    if (cf_flags & CF_NOTIMMUTABLE)
        lstr.add(" notimmutable");
    if (cf_flags & CF_DEVICE)
        lstr.add(" device");
    if (cf_flags & CF_NOTDEVICE)
        lstr.add(" notdevice");
    if (cf_flags & CF_LIBRARY)
        lstr.add(" library");
    if (cf_flags & CF_NOTLIBRARY)
        lstr.add(" notlibrary");
    if (cf_flags & CF_MODIFIED)
        lstr.add(" modified");
    if (cf_flags & CF_NOTMODIFIED)
        lstr.add(" notmodified");
    if (cf_flags & CF_REFERENCE)
        lstr.add(" reference");
    if (cf_flags & CF_NOTREFERENCE)
        lstr.add(" notreference");
    if (cf_flags & CF_TOPLEV)
        lstr.add(" toplev");
    if (cf_flags & CF_NOTTOPLEV)
        lstr.add(" nottoplev");
    if (cf_flags & CF_WITHALT)
        lstr.add(" withalt");
    if (cf_flags & CF_NOTWITHALT)
        lstr.add(" notwithalt");
    if (cf_flags & CF_PARENT) {
        lstr.add(" parent ");
        lstr.add_c('"');
        for (cnlist_t *s = cf_p_list; s; s = s->next) {
            lstr.add(s->cname->string());
            if (s->next)
                lstr.add_c(' ');
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_NOTPARENT) {
        lstr.add(" notparent ");
        lstr.add_c('"');
        for (cnlist_t *s = cf_np_list; s; s = s->next) {
            lstr.add(s->cname->string());
            if (s->next)
                lstr.add_c(' ');
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_SUBCELL) {
        lstr.add(" subcell ");
        lstr.add_c('"');
        for (cnlist_t *s = cf_s_list; s; s = s->next) {
            lstr.add(s->cname->string());
            if (s->next)
                lstr.add_c(' ');
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_NOTSUBCELL) {
        lstr.add(" notsubcell ");
        lstr.add_c('"');
        for (cnlist_t *s = cf_ns_list; s; s = s->next) {
            lstr.add(s->cname->string());
            if (s->next)
                lstr.add_c(' ');
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_LAYER) {
        lstr.add(" layer ");
        lstr.add_c('"');
        for (CDll *l = cf_l_list; l; l = l->next) {
            lstr.add(l->ldesc->name());
            if (l->next)
                lstr.add_c(' ');
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_NOTLAYER) {
        lstr.add(" notlayer ");
        lstr.add_c('"');
        for (CDll *l = cf_nl_list; l; l = l->next) {
            lstr.add(l->ldesc->name());
            if (l->next)
                lstr.add_c(' ');
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_FLAG) {
        lstr.add(" flag ");
        lstr.add_c('"');
        bool first = true;
        for (FlagDef *f = SdescFlags; f->name; f++) {
            if (f->value & cf_f_bits) {
                if (first)
                    first = false;
                else
                    lstr.add_c(' ');
                lstr.add(f->name);
            }
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_NOTFLAG) {
        lstr.add(" notflag ");
        lstr.add_c('"');
        bool first = true;
        for (FlagDef *f = SdescFlags; f->name; f++) {
            if (f->value & cf_nf_bits) {
                if (first)
                    first = false;
                else
                    lstr.add_c(' ');
                lstr.add(f->name);
            }
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_FTYPE) {
        lstr.add(" ftype ");
        lstr.add_c('"');
        int cnt = 0;
        if (cf_ft_mask & (1 << Fnone)) {
            lstr.add("none");
            cnt++;
        }
        if (cf_ft_mask & (1 << Fnative)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("native");
            cnt++;
        }
        if (cf_ft_mask & (1 << Fgds)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("gds");
            cnt++;
        }
        if (cf_ft_mask & (1 << Fcgx)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("cgx");
            cnt++;
        }
        if (cf_ft_mask & (1 << Foas)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("oas");
            cnt++;
        }
        if (cf_ft_mask & (1 << Fcif)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("cif");
            cnt++;
        }
        if (cf_ft_mask & (1 << Foa)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("opa");
            cnt++;
        }
        lstr.add_c('"');
    }
    if (cf_flags & CF_NOTFTYPE) {
        lstr.add(" notftype ");
        lstr.add_c('"');
        int cnt = 0;
        if (cf_nft_mask & (1 << Fnone)) {
            lstr.add("none");
            cnt++;
        }
        if (cf_nft_mask & (1 << Fnative)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("native");
            cnt++;
        }
        if (cf_nft_mask & (1 << Fgds)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("gds");
            cnt++;
        }
        if (cf_nft_mask & (1 << Fcgx)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("cgx");
            cnt++;
        }
        if (cf_nft_mask & (1 << Foas)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("oas");
            cnt++;
        }
        if (cf_nft_mask & (1 << Fcif)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("cif");
            cnt++;
        }
        if (cf_nft_mask & (1 << Foa)) {
            if (cnt)
                lstr.add_c(' ');
            lstr.add("opa");
            cnt++;
        }
        lstr.add_c('"');
    }
    if (!lstr.length())
        return (0);
    return (lstring::copy(1 + lstr.string()));
}


// Parse the list of cells used in the parent test.
//
void
cfilter_t::parse_parent(bool non, const char *str, stringlist **perr)
{
    cnlist_t *list = 0;
    char *tok;
    while ((tok = lstring::getqtok(&str)) != 0) {
        CDcellName name = CD()->CellNameTableFind(tok);
        if (name)
            list = new cnlist_t(name, list);
        else if (perr) {
            sLstr lstr;
            lstr.add("Unknown cell ");
            lstr.add(tok);
            lstr.add(".\n");
            *perr = new stringlist(lstr.string_trim(), *perr);
        }
        delete [] tok;
    }
    if (non)
        cf_np_list = list;
    else
        cf_p_list = list;
}


// Parse the list of cells used in the subcell test.
//
void
cfilter_t::parse_subcell(bool non, const char *str, stringlist **perr)
{
    cnlist_t *list = 0;
    char *tok;
    while ((tok = lstring::getqtok(&str)) != 0) {
        CDcellName name = CD()->CellNameTableFind(tok);
        if (name)
            list = new cnlist_t(name, list);
        else if (perr) {
            sLstr lstr;
            lstr.add("Unknown cell ");
            lstr.add(tok);
            lstr.add(".\n");
            *perr = new stringlist(lstr.string_trim(), *perr);
        }
        delete [] tok;
    }
    if (non)
        cf_ns_list = list;
    else
        cf_s_list = list;
}


// Parse the list of layers used in the layer test.
//
void
cfilter_t::parse_layers(bool non, const char *str, stringlist **perr)
{
    CDll *list = 0;
    char *tok;
    while ((tok = lstring::getqtok(&str)) != 0) {
        CDl *ld = CDldb()->findLayer(tok, cf_mode);
        if (ld)
            list = new CDll(ld, list);
        else if (perr) {
            sLstr lstr;
            lstr.add("Unknown layer ");
            lstr.add(tok);
            lstr.add(".\n");
            *perr = new stringlist(lstr.string_trim(), *perr);
        }
        delete [] tok;
    }
    if (non)
        cf_nl_list = list;
    else
        cf_l_list = list;
}


// Parse the flags list and set up a bit field.
//
void
cfilter_t::parse_flags(bool non, const char *str, stringlist **perr)
{
    unsigned int flg = 0;
    char *tok;
    while ((tok = lstring::gettok(&str)) != 0) {
        bool found = false;
        for (FlagDef *f = SdescFlags; f->name; f++) {
            if (lstring::cieq(tok, f->name)) {
                flg |= f->value;
                found = true;
                break;
            }
        }
        if (!found && perr) {
            sLstr lstr;
            lstr.add("Unknown flag ");
            lstr.add(tok);
            lstr.add(".\n");
            *perr = new stringlist(lstr.string_trim(), *perr);
        }
        delete [] tok;
    }
    if (non)
        cf_nf_bits = flg;
    else
        cf_f_bits = flg;
}


// Parse the file types list and set up a bit field.
//
void
cfilter_t::parse_ftypes(bool non, const char *str, stringlist **perr)
{
    unsigned int flg = 0;
    char *tok;
    while ((tok = lstring::gettok(&str)) != 0) {
        if (lstring::ciprefix("no", tok))
            flg |= (1 << Fnone);
        else if (lstring::ciprefix("na", tok))
            flg |= (1 << Fnative);
        else if (lstring::ciprefix("xi", tok))
            flg |= (1 << Fnative);
        else if (lstring::ciprefix("gd", tok))
            flg |= (1 << Fgds);
        else if (lstring::ciprefix("st", tok))
            flg |= (1 << Fgds);
        else if (lstring::ciprefix("cg", tok))
            flg |= (1 << Fcgx);
        else if (lstring::ciprefix("oa", tok))
            flg |= (1 << Foas);
        else if (lstring::ciprefix("ci", tok))
            flg |= (1 << Fcif);
        else if (lstring::ciprefix("op", tok))
            flg |= (1 << Foa);
        else if (perr) {
            sLstr lstr;
            lstr.add("Unknown file type ");
            lstr.add(tok);
            lstr.add(".\n");
            *perr = new stringlist(lstr.string_trim(), *perr);
        }
        delete [] tok;
    }
    if (non)
        cf_nft_mask = flg;
    else
        cf_ft_mask = flg;
}


// Static function.
// Create a new cfilter_t based on the list of tokens passed.  If the
// string is null, per-mode defaults are set.  Errors are ignored, but
// messages will be pased back if the errs address is not null.  This
// will always return a valid struct pointer.
//
cfilter_t *
cfilter_t::parse(const char *str, DisplayMode mode, stringlist **perr)
{
    cfilter_t *cf = new cfilter_t(mode);

    if (perr)
        *perr = 0;
    unsigned int f = 0;
    if (str) {
        char *tok;
        while ((tok = lstring::getqtok(&str)) != 0) {
            if (lstring::cieq(tok, "VIASUBM"))
                f |= CF_VIASUBM;
            else if (lstring::cieq(tok, "NOTVIASUBM"))
                f |= CF_NOTVIASUBM;
            else if (lstring::cieq(tok, "PCELLSUBM"))
                f |= CF_PCELLSUBM;
            else if (lstring::cieq(tok, "NOTPCELLSUBM"))
                f |= CF_NOTPCELLSUBM;
            else if (lstring::cieq(tok, "PCELLSUPR"))
                f |= CF_PCELLSUPR;
            else if (lstring::cieq(tok, "NOTPCELLSUPR"))
                f |= CF_NOTPCELLSUPR;
            else if (lstring::cieq(tok, "IMMUTABLE"))
                f |= CF_IMMUTABLE;
            else if (lstring::cieq(tok, "NOTIMMUTABLE"))
                f |= CF_NOTIMMUTABLE;
            else if (lstring::cieq(tok, "DEVICE") ||
                    lstring::cieq(tok, "LIBDEV"))
                f |= CF_DEVICE;
            else if (lstring::cieq(tok, "NOTDEVICE") ||
                    lstring::cieq(tok, "NOTLIBDEV"))
                f |= CF_NOTDEVICE;
            else if (lstring::cieq(tok, "LIBRARY"))
                f |= CF_LIBRARY;
            else if (lstring::cieq(tok, "NOTLIBRARY"))
                f |= CF_NOTLIBRARY;
            else if (lstring::cieq(tok, "MODIFIED"))
                f |= CF_MODIFIED;
            else if (lstring::cieq(tok, "NOTMODIFIED"))
                f |= CF_NOTMODIFIED;
            else if (lstring::cieq(tok, "REFERENCE"))
                f |= CF_REFERENCE;
            else if (lstring::cieq(tok, "NOTREFERENCE"))
                f |= CF_NOTREFERENCE;
            else if (lstring::cieq(tok, "TOPLEV"))
                f |= CF_TOPLEV;
            else if (lstring::cieq(tok, "NOTTOPLEV"))
                f |= CF_NOTTOPLEV;
            else if (lstring::cieq(tok, "WITHALT"))
                f |= CF_WITHALT;
            else if (lstring::cieq(tok, "NOTWITHALT"))
                f |= CF_NOTWITHALT;
            else if (lstring::cieq(tok, "PARENT")) {
                f |= CF_PARENT;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_parent(false, tok, perr);
            }
            else if (lstring::cieq(tok, "NOTPARENT")) {
                f |= CF_NOTPARENT;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_parent(true, tok, perr);
            }
            else if (lstring::cieq(tok, "SUBCELL")) {
                f |= CF_SUBCELL;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_subcell(false, tok, perr);
            }
            else if (lstring::cieq(tok, "NOTSUBCELL")) {
                f |= CF_NOTSUBCELL;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_subcell(true, tok, perr);
            }
            else if (lstring::cieq(tok, "LAYER")) {
                f |= CF_LAYER;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_layers(false, tok, perr);
            }
            else if (lstring::cieq(tok, "NOTLAYER")) {
                f |= CF_NOTLAYER;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_layers(true, tok, perr);
            }
            else if (lstring::cieq(tok, "FLAG")) {
                f |= CF_FLAG;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_flags(false, tok, perr);
            }
            else if (lstring::cieq(tok, "NOTFLAG")) {
                f |= CF_NOTFLAG;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_flags(true, tok, perr);
            }
            else if (lstring::cieq(tok, "FTYPE")) {
                f |= CF_FTYPE;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_ftypes(false, tok, perr);
            }
            else if (lstring::cieq(tok, "NOTFTYPE")) {
                f |= CF_NOTFTYPE;
                delete [] tok;
                tok = lstring::getqtok(&str);
                cf->parse_ftypes(true, tok, perr);
            }
            else if (perr) {
                sLstr lstr;
                lstr.add("Unknown keyword ");
                lstr.add(tok);
                lstr.add(".\n");
                *perr = new stringlist(lstr.string_trim(), *perr);
            }
            delete [] tok;
        }
    }
    if (!f) {
        if (!str)
            cf->set_default();
    }
    else
        cf->set_flags(f);
    return (cf);
}


// Look at the booleans, if both the X and NOTX bits are set, unset
// both as they cancel out.  Note that the forms with lists can have
// both flags set, presumably the lists are different.
//
void
cfilter_t::check_flags()
{
    unsigned int f = cf_flags;
    if ((f & CF_NOTVIASUBM) && (f & CF_VIASUBM))
        f &= ~(CF_NOTVIASUBM | CF_VIASUBM);
    if ((f & CF_NOTPCELLSUBM) && (f & CF_PCELLSUBM))
        f &= ~(CF_NOTPCELLSUBM | CF_PCELLSUBM);
    if ((f & CF_NOTPCELLSUPR) && (f & CF_PCELLSUPR))
        f &= ~(CF_NOTPCELLSUPR | CF_PCELLSUPR);
    if ((f & CF_NOTIMMUTABLE) && (f & CF_IMMUTABLE))
        f &= ~(CF_NOTIMMUTABLE | CF_IMMUTABLE);
    if ((f & CF_NOTDEVICE) && (f & CF_DEVICE))
        f &= ~(CF_NOTDEVICE | CF_DEVICE);
    if ((f & CF_NOTLIBRARY) && (f & CF_LIBRARY))
        f &= ~(CF_NOTLIBRARY | CF_LIBRARY);
    if ((f & CF_NOTMODIFIED) && (f & CF_MODIFIED))
        f &= ~(CF_NOTMODIFIED | CF_MODIFIED);
    if ((f & CF_NOTREFERENCE) && (f & CF_REFERENCE))
        f &= ~(CF_NOTREFERENCE | CF_REFERENCE);
    if ((f & CF_NOTTOPLEV) && (f & CF_TOPLEV))
        f &= ~(CF_NOTTOPLEV | CF_TOPLEV);
    if ((f & CF_NOTWITHALT) && (f & CF_WITHALT))
        f &= ~(CF_NOTWITHALT | CF_WITHALT);
    cf_flags = f;
}

