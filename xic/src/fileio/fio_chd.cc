
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
 $Id: fio_chd.cc,v 1.126 2015/10/11 19:35:45 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_chd.h"
#include "fio_cvt_base.h"
#include "fio_chd_cvtab.h"
#include "fio_chd_ecf.h"
#include "fio_chd_flat.h"
#include "fio_chd_flat_prv.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "fio_library.h"
#include "cd_propnum.h"
#include "cd_digest.h"
#include "cd_celldb.h"
#include "cd_chkintr.h"
#include "filestat.h"
#include "timedbg.h"


// Return a cCHD (cell hierarchy digest) for the archive file.
// If mode == Physical, save physical data only, otherwise save physical
// and electrical data.
//  atab:
//      Use/reuse the alias table passed.
//  info_mode:
//      Specify what info to keep.
//
// The names stored in the name tables are the *aliased* names (if any
// aliasing is applied during the read).  When accessing the file
// through the digest, aliasing should *always* be disabled.
//
cCHD *
cFIO::NewCHD(const char *fname, FileType ft, DisplayMode mode,
    FIOaliasTab *atab, cvINFO info_mode)
{
    TimeDbg tdbg("chd_create");

    bool tflg = CD()->IsNoElectrical();
    if (mode == Physical)
        CD()->SetNoElectrical(true);
    cv_header_info *hinfo = 0;
    cCHD *chd = 0;
    cv_in *in = NewInput(ft, false);
    if (in) {
        ifSetWorking(true);
        in->set_show_progress(true);
        in->assign_alias(atab);
        bool ret = in->setup_source(fname);
        if (ret) {
            in->set_to_database();
            ret = in->parse(Physical, true, 1.0, true, info_mode);
        }
        if (ret)
            hinfo = in->chd_get_header_info();
        if (ret && !CD()->IsNoElectrical() && in->has_electrical())
            ret = in->parse(Electrical, true, 1.0, true, info_mode);
        if (ret)
            chd = in->new_chd();
        if (chd)
            chd->setHeaderInfo(hinfo);
        if (atab)
            // don't free!
            in->extract_alias();
        delete in;
        ifSetWorking(false);
    }
    CD()->SetNoElectrical(tflg);

    // If set, build a random access map if the file is gzipped, for
    // fast access to arbitrary locations.
    //
    if (chd && fioChdRandomGzip)
        chd->registerRandomMap();

    return (chd);
}


// Print information on each record in infilename in outfilename.  If
// endstr_term is true, exit after the first end-structure record.
// Exit false if an error occurs.
//
bool
cFIO::GetCHDinfo(const char *infilename, FileType ft,
    const char *outfilename, int64_t start_ofs, int64_t end_ofs,
    int numrecs, int numcells)
{
    if (!outfilename)
        outfilename = "";

    cv_in *in = NewInput(ft, false);
    if (!in)
        return (false);
    in->set_show_progress(true);
    bool ret = in->setup_source(infilename);
    if (ret)
        ret = in->setup_ascii_out(outfilename, start_ofs, end_ofs, numrecs,
            numcells);
    if (ret)
        ret = in->parse(Physical, false, 1.0);
    if (ret && in->has_electrical())
        ret = in->parse(Electrical, false, 1.0);
    delete in;
    return (ret);
}


//-----------------------------------------------------------------------------
// cCHD functions
// The cCHD is a compact archive file context representation.
//-----------------------------------------------------------------------------

cCHD::~cCHD()
{
    delete c_ptab;
    delete c_etab;
    delete c_phys_info;
    delete c_elec_info;
    delete [] c_filename;
    delete c_header_info;
    delete c_alias_info;
    if (c_cgd) {
        c_cgd->dec_refcnt();
        if (!c_cgd->refcnt() && c_cgd->free_on_unlink())
            delete c_cgd;
    }
    unregisterRandomMap();
}


// Specify the default top cell.
//
bool
cCHD::setDefaultCellname(const char *topname, const char **prvname)
{
    if (!c_ptab)
        return (false);

    if (prvname) {
        if (c_top_symref)
            *prvname = c_top_symref->get_name()->string();
        else
            *prvname = 0;
    }

    symref_t *p = 0;
    if (topname && *topname) {
        CDcellName cn = CD()->CellNameTableFind(topname);
        if (!cn)
            return (false);
        p = c_ptab->get(cn);
        if (!p)
            return (false);
        if (!setBoundaries(p))
            return (false);
        c_top_symref = p;
        c_namecfg = true;
    }
    else {
        c_top_symref = 0;
        c_namecfg = false;
    }
    CD()->ifChdDbChange();
    return (true);
}


void
cCHD::clearSkipFlags()
{
    namegen_t gen(c_ptab);
    symref_t *p;
    while ((p = gen.next()) != 0)
        p->set_skip(false);
}


// Associate a CGD database of physical data with the CHD.  The CHD
// will use the database, rather than the original file, to obtain
// physical data.  At most one of the arguments should be given.  If
// the CGD is given, link it in and increment its reference count
// (idname is ignored).  If idname is given (null cgd), obtain and
// link the named CGD from the CD table.  If both args are null, just
// unlink any existing CGD link.
//
bool
cCHD::setCgd(cCGD *cgd, const char *idname)
{
    if (!cgd && idname && *idname) {
        cgd = CDcgd()->cgdRecall(idname, false);
        if (!cgd) {
            Errs()->add_error(
                "cCHD::setCgd: unresolved CGD access name %s.", idname);
            return (false);
        }
    }
    if (cgd == c_cgd)
        return (true);
    if (c_cgd) {
        c_cgd->dec_refcnt();
        if (!c_cgd->refcnt()) {
            if (c_cgd->free_on_unlink())
                delete c_cgd;
            CD()->ifCgdDbChange();
        }
        c_cgd = 0;
    }
    if (cgd) {
        c_cgd = cgd;
        c_cgd->inc_refcnt();
        if (c_cgd->refcnt() == 1)
            CD()->ifCgdDbChange();
    }
    CD()->ifChdDbChange();
    return (true);
}


// Return the symref of the first top-level cell found in the file, for
// the given mode.
//
symref_t *
cCHD::defaultSymref(DisplayMode mode)
{
    if (c_top_symref) {
        if (mode == Physical)
            return (c_top_symref);
        else
            // Let the phys top cell define the default for
            // electrical, too.
            return (c_etab->get(c_top_symref->get_name()));
    }

    syrlist_t *sl = topCells(mode, true);
    if (!sl) {
        Errs()->add_error(
            "cCHD::defaultSymref: no top-level cell to process.");
        return (0);
    }
    symref_t *s = sl->symref;
    if (mode == Physical)
        // Cache the default cell.  This is important since calling
        // topCells() is slow.
        c_top_symref = sl->symref;

    sl->free();
    return (s);
}


// Return the name of the first top-level cell found in the file, for
// the given mode.
//
const char *
cCHD::defaultCell(DisplayMode mode)
{
    symref_t *s = defaultSymref(mode);
    if (s)
        return (s->get_name()->string());
    return (0);
}


namespace {
    // Sort comparison, strip junk prepended to cell names.
    inline bool
    ls_comp(const char *s1, const char *s2)
    {
        while (isspace(*s1) || *s1 == '*' || *s1 == '+')
            s1++;
        while (isspace(*s2) || *s2 == '*' || *s2 == '+')
            s2++;
        return ((strcmp(s1, s2) < 0));
    }
}


// Return a list of the cell names found in the archive.  The mode value
// is Physical or Electrical, or negative for both.
//
stringlist *
cCHD::listCellnames(int mode, bool mark)
{
    char buf[128];
    stringlist *s0 = 0;
    symref_t *p;
    namegen_t gen(c_ptab);
    if (mode < 0 || mode == Physical) {
        symref_t *pref = defaultSymref(Physical);
        while ((p = gen.next()) != 0) {
            if (!p->get_defseen() && FIO()->LookupLibCell(0,
                    p->get_name()->string(), LIBdevice, 0))
                continue;
            if (mark) {
                buf[0] = p == pref ? '*' : ' ';
                strcpy(buf+1, p->get_name()->string());
                s0 = new stringlist(lstring::copy(buf), s0);
            }
            else
                s0 = new stringlist(
                    lstring::copy(p->get_name()->string()), s0);
        }
    }
    if (mode < 0 || mode == Electrical) {
        symref_t *pref = defaultSymref(Electrical);
        gen = namegen_t(c_etab);
        while ((p = gen.next()) != 0) {
            if (!p->get_defseen() && FIO()->LookupLibCell(0,
                    p->get_name()->string(), LIBdevice, 0))
                continue;
            if (!c_ptab || !c_ptab->get(p->get_name())) {
                if (mark) {
                    buf[0] = p == pref ? '*' : ' ';
                    strcpy(buf+1, p->get_name()->string());
                    s0 = new stringlist(lstring::copy(buf), s0);
                }
                else
                    s0 = new stringlist(
                        lstring::copy(p->get_name()->string()), s0);
            }
        }
    }
    s0->sort(mark ? ls_comp : 0);
    return (s0);
}


// Return a list of the cells found in the archive, under the given
// cell for the given mode.  If the cname is 0, the default cell is
// used.
//
stringlist *
cCHD::listCellnames(const char *cname, DisplayMode mode)
{
    symref_t *p = findSymref(cname, mode, true);
    if (!p || !p->get_defseen())
        return (0);
    SymTab *tab = new SymTab(false, false);
    stringlist *sl = 0;
    if (listCellnames_rc(p, tab, 0)) {
        sl = tab->names();
        sl->sort();
    }
    delete tab;
    return (sl);
}


// Return a list of symref_t's.  If sort_by_offset, the list is sorted
// by offset, otherwise use alpha sort of name.  If ts_set, only the
// symrefs with the flag set are returned, and if is ts_unset, symrefs
// with the flag not set are returned.
//
syrlist_t *
cCHD::listing(DisplayMode mode, bool sort_by_offset, tristate_t skip)
{
    nametab_t *ntab = nameTab(mode);
    syrlist_t *s0 = 0;
    namegen_t gen(ntab);
    symref_t *p;
    while ((p = gen.next()) != 0) {
        if (!p->get_defseen() && FIO()->LookupLibCell(0,
                p->get_name()->string(), LIBdevice, 0))
            continue;

        bool b2;
        if (skip == ts_set)
            b2 = p->should_skip();
        else if (skip == ts_unset)
            b2 = !p->should_skip();
        else
            b2 = true;

        if (b2)
            s0 = new syrlist_t(p, s0);
    }
    s0->sort(sort_by_offset);
    return (s0);
}


// Return a list of symrefs of cells found in the hierarchy of cname,
// framed in AOI, if nonzero.
//
syrlist_t *
cCHD::listing(DisplayMode mode, const char *cname, bool sort_by_offset,
    const BBox *AOI)
{
    symref_t *p = findSymref(cname, mode, true);
    if (!p)
        return (0);

    cCVtab *ctab = new cCVtab((mode == Electrical), 1);
    if (!ctab->build_BB_table(this, p, 0, AOI)) {
        delete ctab;
        return (0);
    }
    syrlist_t *list = ctab->listing(0);
    delete ctab;
    list->sort(sort_by_offset);
    return (list);
}


// Return a list of the top cell names found in the archive.  This will
// set the referenced flags.
//
syrlist_t *
cCHD::topCells(DisplayMode mode, bool sort_by_offset)
{
    nametab_t *ntab = nameTab(mode);
    namegen_t gen(ntab);
    symref_t *p;
    while ((p = gen.next()) != 0) {
        crgen_t cgen(ntab, p);
        const cref_o_t *c;
        while ((c = cgen.next()) != 0) {
            symref_t *cp = ntab->find_symref(c->srfptr);
            if (cp)
                cp->set_refd(true);
        }
    }
    syrlist_t *s0 = 0;
    gen = namegen_t(ntab);
    while ((p = gen.next()) != 0) {
        if (!p->get_refd())
            s0 = new syrlist_t(p, s0);
    }
    s0->sort(sort_by_offset);
    return (s0);
}


stringlist *
cCHD::layers(DisplayMode mode)
{
    cv_info *info = pcInfo(mode);
    if (info)
        return (info->layers());
    return (0);
}


// Return in array a set of depth-indexed population counts.
//   array[0]     1, the cell itself
//   array[1]     number of subcells in top cell
//   array[2]     number of subcells in subcells
//   ...
// The array must have size CDMAXCALLDEPTH or larger.  The unused
// elements are zeroed.   Instances are counted by expanding arrays,
// i.e., each arrayed element is counted as an "instance".
//
bool
cCHD::depthCounts(symref_t *ptop, unsigned int *array)
{
    memset(array, 0, CDMAXCALLDEPTH*sizeof(unsigned int));
    if (!ptop)
        return (true);
    array[0] = 1;
    return (depthCounts_rc(ptop, 1, 1, array));
}


// Create and return a hash table containing instantiation counts for
// all masters found in the hierarchy, including the top level.
// Instances are counted by expanding arrays, i.e., each arrayed
// element is counted as an "instance".
//
SymTab *
cCHD::instanceCounts(symref_t *ptop)
{
    if (!ptop)
        return (0);
    SymTab *tab = new SymTab(false, false);
    if (!instanceCounts_rc(ptop, 0, 1, tab)) {
        delete tab;
        return (0);
    }
    return (tab);
}


// Return a list of the names of unresolved cells, which have zero
// offsets.
//
// The unresolved symrefs should not do any harm, as chd_read_cell
// ignores them.
//
syrlist_t *
cCHD::listUnresolved(DisplayMode mode)
{
    syrlist_t *s0 = 0, *se = 0;
    nametab_t *ntab = nameTab(mode);
    namegen_t gen(ntab);
    symref_t *p;
    while ((p = gen.next()) != 0) {
        if (!p->get_defseen()) {
            if (s0) {
                se->next = new syrlist_t(p, 0);
                se = se->next;
            }
            else
                s0 = se = new syrlist_t(p, 0);
        }
    }
    return (s0);
}


symref_t *
cCHD::findSymref(const char *cname, DisplayMode mode, bool allow_default)
{
    if (cname && *cname) {
        CDcellName cn = CD()->CellNameTableFind(cname);
        return (findSymref(cn, mode));
    }
    if (c_namecfg && c_top_symref && mode == Physical)
        return (c_top_symref);
    else if (allow_default)
        return (defaultSymref(mode));
    return (0);
}


symref_t *
cCHD::findSymref(CDcellName name, DisplayMode mode)
{
    if (name) {
        nametab_t *ntab = nameTab(mode);
        if (ntab)
            return (ntab->get(name));
    }
    return (0);
}


namespace {
    // Class to handle interrupts and user feedback when methods are
    // running.
    //
    struct fb_t
    {
        fb_t(int d, const char *m)
            {
                msg = m;
                count = 0;
                if (d < 31 && d > 0) {
                    del = 1 << d;
                    del--;
                }
                else
                    del = 1;
            }

        bool record()
            {
                fb_t *fbt = this;
                if (fbt) {
                    count++;
                    if (!(count & del)) {
                        FIO()->ifInfoMessage(IFMSG_INFO, "%s:  %u",
                            msg ? msg : "processed", count);
                        if (checkInterrupt())
                            return (true);
                    }
                }
                return (false);
            }

        unsigned int count;
        unsigned int del;
        const char *msg;
    };

    fb_t *user_fb;
}


// Set the bounding boxes of the physical cells p and under.  This is
// fairly compute-intensive so there is user feedback provided.  The
// initial bounding boxes contain the physical objects, set during
// read.  This expands them to include the subcells.
//
bool
cCHD::setBoundaries(symref_t *p)
{
    TimeDbg tdbg("set_boundaries");

    if (!p)
        return (false);

    nametab_t *ntab = nameTab(p->mode());
    if (!ntab)
        return (false);

    fb_t *tbak = user_fb;
    user_fb = new fb_t(10, "Cell boundaries processed");
    bool ret = setBoundaries_rc(p);
    delete user_fb;
    user_fb = tbak;
    return (ret);
}


namespace fio_chd {
    // Struct used to pass state in instanceBoundaries.
    //
    struct ib_t : public cTfmStack
    {
        ib_t(CDcellName n)
            {
                refname = n;
                blist = 0;
            }

        ~ib_t()
            {
                Blist::destroy(blist);
            }

        CDcellName refname;
        Blist *blist;
        ptrtab_t check_tab;
    };
}


// Return a list of the bounding boxes of instances of refname under p.
// This will recurse into libraries.
//
bool
cCHD::instanceBoundaries(symref_t *p, CDcellName refname, Blist **bl)
{
    *bl = 0;
    if (!p || !refname) {
        Errs()->add_error(
            "cCHD::instanceBoundaries: unexpected null argument.");
        return (false);
    }
    if (!setBoundaries(p))
        return (false);
    if (refname == p->get_name()) {
        *bl = new Blist(p->get_bb(), *bl);
        return (true);
    }
    fio_chd::ib_t ib(refname);
    if (!instanceBoundaries_rc(p, &ib))
        return (false);
    *bl = ib.blist;
    ib.blist = 0;
    return (true);
}


void
cCHD::setHeaderInfo(cv_header_info *info)
{
    delete c_header_info;
    c_header_info = info;
}


namespace {
    inline int
    nstrcmp(const char *s1, const char *s2)
    {
        if (!s1)
            return (s2 != 0);
        if (!s2)
            return (1);
        return (strcmp(s1, s2));
    }
}


// Return true if the CHD parameters match those in prp.
//
bool
cCHD::match(const sChdPrp *prp)
{
    if (!prp->get_path() || strcmp(prp->get_path(), c_filename))
        return (false);
    if (!c_alias_info) {
        if (prp->get_alias_flags() || prp->get_alias_prefix() ||
                prp->get_alias_suffix())
            return (false);
    }
    else if (prp->get_alias_flags() != (int)c_alias_info->flags() ||
            nstrcmp(prp->get_alias_prefix(), c_alias_info->prefix()) ||
            nstrcmp(prp->get_alias_suffix(), c_alias_info->suffix()))
        return (false);
    return (true);
}


// Physical only.
// Create a reference cell for cellname.
//
bool
cCHD::createReferenceCell(const char *cellname)
{
    if (!cellname || !*cellname) {
        cellname = defaultCell(Physical);
        if (!cellname) {
            Errs()->add_error(
            "cCHD::createReferenceCell: no default for null/empty cell name.");
            return (false);
        }
    }

    symref_t *p = findSymref(cellname, Physical);
    if (!p) {
        Errs()->add_error(
            "cCHD::createReferenceCell: symref for %s not found in CHD.",
            cellname);
        return (false);
    }
    if (!setBoundaries(p)) {
        Errs()->add_error(
            "cCHD::createReferenceCell: CHD cell boundary setup failed.");
        return (false);
    }

    CDcbin cbin;
    if (CDcdb()->findSymbol(cellname, &cbin)) {
        // Already a cell existing, fail unless it matches what we would
        // create anyway.
        bool ok = false;
        if (!cbin.elec() && cbin.phys()->isChdRef()) {
            CDp *pd = cbin.phys()->prpty(XICP_CHD_REF);
            if (pd) {
                sChdPrp prp(pd->string());
                ok = match(&prp);
            }
        }
        if (ok)
            return (true);

        Errs()->add_error("cCHD::createReferenceCell: cell name %s in use.",
            cellname);
        return (false);
    }

    CDs *sd = CDcdb()->insertCell(cellname, Physical);
    if (!sd) {
        Errs()->add_error(
            "cCHD::createReferenceCell: new cell creattion failed.");
        return (false);
    }

    sChdPrp prp(this, c_filename, cellname, p->get_bb());
    char *pstr = prp.compose();
    if (!pstr) {
        Errs()->add_error(
            "cCHD::createReferenceCell: unable to compose property string.");
        return (false);
    }
    sd->incModified();  // must do this before setting property
    sd->prptyAdd(XICP_CHD_REF, pstr);
    delete [] pstr;

    return (true);
}


// Physical only.
// Read one cell from the CHD into memory.  The subcells become
// reference cells in memory, i.e., only the named cell is read
// through the CHD.  The cell name is added to the Cell Tab.
//
OItype
cCHD::loadCell(const char *cellname)
{
    if (!cellname || !*cellname) {
        cellname = defaultCell(Physical);
        if (!cellname) {
            Errs()->add_error(
                "cCHD::loadCell: no default for null/empty cell name.");
            return (OIerror);
        }
    }

    symref_t *p = findSymref(cellname, Physical);
    if (!p) {
        Errs()->add_error("cCHD::loadCell: symref for %s not found in CHD.",
            cellname);
        return (OIerror);
    }

    crgen_t cgen(c_ptab, p);
    const cref_o_t *c;
    while ((c = cgen.next()) != 0) {
        symref_t *cp = c_ptab->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "cCHD::loadCell: instance back pointer unresolved.");
            return (OIerror);
        }
        if (!createReferenceCell(cp->get_name()->string())) {
            Errs()->add_error(
                "cCHD::loadCell: createReferenceCell failed for %s.",
                cp->get_name()->string());
            return (OIerror);
        }
    }
    FIOreadPrms prms;
    OItype oiret = open(0, cellname, &prms, false);
    if (oiret != OIok)
        return (oiret);
    if (CDcdb()->auxCellTab())
        CDcdb()->auxCellTab()->add(cellname, false);
    return (OIok);
}


// Read the named cell into memory, recursively if allcells is set.
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//    OInew         cellname not resolved in CHD
//
// The cellname is a name found in the name tables, i.e., *after*
// aliasing.
//
OItype
cCHD::open(CDcbin *cbret, const char *cellname, const FIOreadPrms *prms,
    bool allcells)
{
    TimeDbg tdbg("chd_open");

    if (cbret)
        cbret->reset();

    if (!prms) {
        Errs()->add_error("cCHD::open: null parameters pointer.");
        return (OIerror);
    }
    if (prms->scale() < .001 || prms->scale() > 1000.0) {
        Errs()->add_error("cCHD::open: bad scale.");
        return (OIerror);
    }
    if (!cellname || !*cellname) {
        cellname = defaultCell(Physical);
        if (!cellname) {
            Errs()->add_error(
                "cCHD::open: no default for null/empty cell name.");
            return (OIerror);
        }
    }

    symref_t *srf_phys = findSymref(cellname, Physical, false);
    symref_t *srf_elec = 0;
    if (!CD()->IsNoElectrical() && !c_cgd)
        srf_elec = findSymref(cellname, Electrical, false);
    if (!srf_phys && !srf_elec) {
        if (cbret) {
            // We may not want this to be a fatal error, so return
            // OInew to identify this condition.
            return (OInew);
        }
        Errs()->add_error("cCHD::open: cell %s not found in CHD.", cellname);
        return (OIerror);
    }

    // Create input for this CHD.
    //
    cv_in *in = newInput(prms->allow_layer_mapping());
    if (!in) {
        Errs()->add_error("cCHD::open: main input channel creation failed.");
        return (OIerror);
    }

    // Set input aliasing to CHD aliasing.
    in->assign_alias(new FIOaliasTab(true, false, c_alias_info));

    in->set_to_database();

    // This takes care of enabling and cleaning up after the
    // MergeControl2 pop-up.
    sMCenable mc2_enable;

    if (srf_phys) {
        OItype oiret = open(srf_phys, in, prms, allcells);
        if (oiret != OIok) {
            delete in;
            return (oiret);
        }
    }
    if (srf_elec) {
        OItype oiret = open(srf_elec, in, prms, allcells);
        if (oiret != OIok) {
            delete in;
            return (oiret);
        }
    }
    delete in;

    CDcbin cbin;
    if (!CDcdb()->findSymbol(cellname, &cbin))
        return (OIerror);
    if (!cbin.fixBBs())
        return (OIerror);
    if (cbret)
        *cbret = cbin;

    return (OIok);
}


// Read the cell(s) represented by symref into memory, recursively if
// allcells is set.
//
//  Return values:
//    OIerror       unspecified error
//    OIok          file was read and new cells opened ok
//    OIaborted     user aborted
//
OItype
cCHD::open(symref_t *p, cv_in *in, const FIOreadPrms *prms, bool allcells)
{
    // Local struct for cleanup and timing info.
    struct open_cleanup : public TimeDbg
    {
        open_cleanup() : TimeDbg("chd_open_prv")
            {
                ctab = 0;
                itab = 0;
            }

        ~open_cleanup()
            {
                delete ctab;
                delete itab;
            }
            
        cCVtab      *ctab;
        chd_intab   *itab;
    };
    open_cleanup oc;

    if (!p || !in || !prms) {
        Errs()->add_error("cCHD::open: unexpected null argument.");
        return (OIerror);
    }
    DisplayMode mode = p->mode();

    // Build table of cells to read.
    //
    oc.ctab = new cCVtab((mode == Electrical), 1);
    if (!oc.ctab->build_BB_table(this, p, 0, 0, !allcells)) {
        Errs()->add_error("cCHD::open: cell table build failed.");
        return (OIerror);
    }

    // Set up additional CHD input channels, for library references.
    //
    oc.itab = new chd_intab;
    for (int i = 0; ; i++) {
        cCHD *tchd = oc.ctab->get_chd(i);
        if (!tchd)
            break;
        if (tchd == this) {
            oc.itab->insert(tchd, in);
            oc.itab->set_no_free(in);
            continue;
        }
        cv_in *new_in = tchd->newInput(prms->allow_layer_mapping());
        if (!new_in) {
            Errs()->add_error(
                "cCHD::open: reference channel setup failed.");
            return (OIerror);
        }
        oc.itab->insert(tchd, new_in);
    }

    // Read in the table contents.
    //
    bool ok = true;
    CVtabGen ctgen(oc.ctab, 0, p);
    cCHD *tchd;
    symref_t *tp;
    cv_in *cin = in;
    cCHD *cchd = this;
    if (!cin->chd_setup(cchd, oc.ctab, 0, mode, prms->scale())) {
        Errs()->add_error("cCHD::open: main channel setup failed.");
        ok = false;
    }
    if (ok) {
        cvtab_item_t *item;
        while ((item = ctgen.next()) != 0) {
            tchd = oc.ctab->get_chd(item->get_chd_tkt());
            tp = item->symref();
            if (tchd != cchd) {
                cin->chd_finalize();
                FIOaliasTab *at = cin->extract_alias();
                cin = oc.itab->find(tchd);
                cin->assign_alias(at);
                cchd = tchd;

                if (!cin->chd_setup(cchd, oc.ctab, 0, mode, prms->scale())) {
                    Errs()->add_error(
                        "cCHD::open: reference channel setup failed.");
                    ok = false;
                    break;
                }
            }
            bool reading = CD()->IsReading();
            CD()->SetReading(true);
            ok = cin->chd_read_cell(tp, false);
            CD()->SetReading(reading);
            if (!ok) {
                Errs()->add_error("cCHD::open: cell read failed.");
                break;
            }
        }
    }
    cin->chd_finalize();
    in->assign_alias(cin->extract_alias());

    OItype oiret =
        ok ? OIok : cin->was_interrupted() ? OIaborted : OIerror;

    return (oiret);
}


// Write cell data to disk, or to the main database, recursively if
// allcells is set.  This is primarily intended for writing files to
// disk, however if no destination or file type is set in prms, the
// output will be "written" to the main database.  This is similar to
// open/readFlat but allows windowing while not flattening.
//
//  Return values:
//    OIerror       unspecified error
//    OIok          ok
//    OIaborted     user aborted
//
//  Arguments
//    cellname      Name of top-level cell
//    prms          Mode flags and parameters
//    allcells      Include geometry from subcells (ignored when flattening)
//
OItype
cCHD::write(const char *cellname, const FIOcvtPrms *prms, bool allcells)
{
    // Local struct for cleanup and timing info.
    struct write_cleanup : public TimeDbg
    {
        write_cleanup() : TimeDbg("chd_write")
            {
                in = 0;
                out = 0;
            }

        ~write_cleanup()
            {
                delete in;
                delete out;
            }
            
        cv_in           *in;
        cv_out          *out;
    };
    write_cleanup wc;

    if (!prms) {
        Errs()->add_error("cCHD::write: null parameters pointer.");
        return (OIerror);
    }
    if (prms->scale() < .001 || prms->scale() > 1000.0) {
        Errs()->add_error("cCHD::write: bad scale.");
        return (OIerror);
    }

    if (!cellname || !*cellname) {
        cellname = defaultCell(Physical);
        if (!cellname) {
            Errs()->add_error(
                "cCHD::write: no default for null/empty cell name.");
            return (OIerror);
        }
    }

    symref_t *srf_phys = findSymref(cellname, Physical, false);
    symref_t *srf_elec = 0;
    if (!CD()->IsNoElectrical() && !prms->use_window() &&
            !prms->flatten() && !prms->to_cgd())
        srf_elec = findSymref(cellname, Electrical, false);
    if (!srf_phys && !srf_elec) {
        Errs()->add_error("cCHD::write: cell %s not found in CHD.", cellname);
        return (OIerror);
    }
    if ((prms->use_window() || prms->flatten()) && !srf_phys) {
        Errs()->add_error("cCHD::write: physical cell %s not found in CHD.",
            cellname);
        return (OIerror);
    }

    if (prms->use_window()) {
        // Call this here to catch error early.  This is also called in
        // flatten and write.
        if (!setBoundaries(srf_phys)) {
            Errs()->add_error("cCHD::write: CHD cell boundary setup failed.");
            return (OIerror);
        }
    }

    // Create input for this CHD.
    //
    wc.in = newInput(prms->allow_layer_mapping());
    if (!wc.in) {
        Errs()->add_error("cCHD::write: main input channel creation failed.");
        return (OIerror);
    }

    // Set input aliasing to CHD aliasing.
    //
    wc.in->assign_alias(new FIOaliasTab(true, false, c_alias_info));

    // Create output channel.
    //
    bool todb = false;
    FileType ftype = prms->filetype();
    const char *fname = prms->destination();
    if (ftype == Fnone && !fname) {
        // Output goes to cells in main database.
        wc.in->set_to_database();
        todb = true;
    }
    else {
        if (ftype == Fnone)
            ftype = cFIO::TypeExt(fname);
        if (ftype == Fnone &&
                (!fname || !*fname || filestat::is_directory(fname)))
            ftype = Fnative;
        if (!wc.in->setup_destination(fname, ftype, prms->to_cgd())) {
            Errs()->add_error("cCHD::write: destination setup failed.");
            return (OIerror);
        }
    }

    // Apply back-end aliasing.
    //
    wc.out = wc.in->backend();
    if (wc.out) {
        unsigned int mask = prms->alias_mask();
        if (ftype == Fgds)
            mask |= CVAL_GDS;
        wc.out->assign_alias(FIO()->NewWritingAlias(mask, false));
        wc.out->read_alias(fname);
    }

    if (prms->flatten() && todb) {
        // some stuff from readFlat.

        // Make sure all layers exist.
        if (!createLayers())
            return (OIerror);
        CDs *sd = CD()->ReopenCell(cellname, Physical);

        if (!sd) {
            Errs()->add_error("cCHD::write: top cell reopen failed.");
            return (OIerror);
        }
        // Since windowing/clipping is done in the cv_in, we can skip
        // this in the back end.
        // out = new rf_out(sd, prms->use_window() ? prms->regionBB() : 0,
        //     in, prms->clip());
        wc.out = new rf_out(sd, 0, wc.in, false);
        if (!wc.in->setup_backend(wc.out)) {
            Errs()->add_error("cCHD::write: back-end setup failed.");
            return (OIerror);
        }
    }

    // This takes care of enabling and cleaning up after the
    // MergeControl2 pop-up, normally done by instantiating sMCenable.
    if (todb)
        FIO()->IncMergeControl();

    OItype oiret = OIok;
    if (prms->flatten())
        oiret = flatten(srf_phys, wc.in, CDMAXCALLDEPTH, prms);
    else {
        if (srf_phys)
            oiret = write(srf_phys, wc.in, prms, allcells);
        if (oiret == OIok && srf_elec)
            oiret = write(srf_elec, wc.in, prms, allcells);
    }
    if (todb) {
        FIO()->DecMergeControl();
        CDcbin cbin(CDcdb()->findCell(cellname, Physical));
        if (!cbin.fixBBs())
            return (OIerror);
    }
    return (oiret);
}


// Write the cell represented by the symref to disk, recursively if
// allcells is set.
//
//  Return values:
//    OIerror       unspecified error
//    OIok          ok
//    OIaborted     user aborted
//
//  Arguments
//    prms          Mode flags and parameters
//    allcells      Include geometry from subcells
//    vtab          Visited symbol table, if used
//
OItype
cCHD::write(symref_t *p, cv_in *in, const FIOcvtPrms *prms, bool allcells,
    SymTab *vtab)
{
    // Local struct for cleanup and timing info.
    struct write_prv_cleanup : public TimeDbg
    {
        write_prv_cleanup() : TimeDbg("chd_write_prv")
            {
                ctab = 0;
                itab = 0;
            }

        ~write_prv_cleanup()
            {
                delete ctab;
                delete itab;
            }
            
        cCVtab      *ctab;
        chd_intab   *itab;
    };
    write_prv_cleanup wc;

    if (!p || !in || !prms) {
        Errs()->add_error("cCHD::write: unexpected null argument.");
        return (OIerror);
    }

    DisplayMode mode = p->mode();
    if (mode != Physical && prms->to_cgd()) {
        Errs()->add_error(
        "cCHD::write: attempt to add electrical data to geometry database.");
        return (OIerror);
    }

    // Set up windowing/clipping.
    //
    bool usewin = mode == Physical && prms->use_window();
    if (usewin) {
        if (!setBoundaries(p)) {
            Errs()->add_error("cCHD::write: CHD cell boundary setup failed.");
            return (OIerror);
        }
        in->set_clip(prms->clip());
    }
    else
        in->set_clip(false);

    in->set_flatten(0, cvNoFlatten);

    // If layer filtering, do the empty-cell pre-filtering, which sets
    // the CVemty flags in the symrefs in the hierarchy.  These are
    // cleared from the ecf destructor.
    //
    CVecFilt ecf;
    if (prms->ecf_level() == ECFall || prms->ecf_level() == ECFpre)
        ecf.setup(this, p);

    // Build table of cells to write.
    //
    wc.ctab = new cCVtab(false, 1);
    bool ok = true;
    if (usewin) {
        BBox tBB(*prms->window());
        tBB.scale(1.0/prms->scale());
        ok = wc.ctab->build_BB_table(this, p, 0, &tBB, !allcells);
    }
    else
        ok = wc.ctab->build_BB_table(this, p, 0, 0, !allcells);
    if (!ok) {
        Errs()->add_error("cCHD::write: cell table build failed.");
        return (OIerror);
    }

    // Set up additional CHD input channels, for library references.
    //
    wc.itab = new chd_intab;
    for (int i = 0; ; i++) {
        cCHD *tchd = wc.ctab->get_chd(i);
        if (!tchd)
            break;
        if (tchd == this) {
            wc.itab->insert(tchd, in);
            wc.itab->set_no_free(in);
            continue;
        }
        cv_in *new_in = tchd->newInput(prms->allow_layer_mapping());
        if (!new_in) {
            Errs()->add_error(
                "cCHD::write: reference channel setup failed.");
            return (OIerror);
        }
        if (usewin)
            new_in->set_clip(prms->clip());
        new_in->setup_backend(in->peek_backend());
        wc.itab->insert(tchd, new_in);
    }

    if ((prms->ecf_level() == ECFall || prms->ecf_level() == ECFpost) &&
            wc.ctab->prune_empties(p, wc.itab, 0) != OIok) {
        Errs()->add_error("cCHD::write: empty cell check failed.");
        return (OIerror);
    }

    // Write out the table contents.
    //
    cv_out *out = in->peek_backend();  // don't free!
    CVtabGen ctgen(wc.ctab, 0, p);
    cCHD *tchd;
    symref_t *tp;
    const BBox *tBB;
    cv_in *cin = in;
    cCHD *cchd = this;
    if (!cin->chd_setup(cchd, wc.ctab, 0, mode, prms->scale())) {
        Errs()->add_error("cCHD::write: main channel setup failed.");
        ok = false;
    }
    if (ok && out) {
        if (!in->no_open_lib() && !out->open_library(mode, 1.0)) {
            Errs()->add_error(
                "cCHD::write: main channel header write failed.");
            ok = false;
        }
    }
    if (ok) {
        cvtab_item_t *item;
        while ((item = ctgen.next()) != 0) {
            tchd = wc.ctab->get_chd(item->get_chd_tkt());
            tp = item->symref();
            tBB = item->get_bb();
            if (tchd != cchd) {
                cin->chd_finalize();
                FIOaliasTab *at = cin->extract_alias();
                cin = wc.itab->find(tchd);
                cin->assign_alias(at);
                cchd = tchd;

                if (!cin->chd_setup(cchd, wc.ctab, 0, mode, prms->scale())) {
                    Errs()->add_error(
                        "cCHD::write: reference channel setup failed.");
                    ok = false;
                    break;
                }
            }

            // These filter out cells that have been written
            // previously.
            //
            if (vtab) {
                if (vtab == CHD_USE_VISITED && out) {
                    if (tp != p &&
                            out->visited(tp->get_name()->string()) >= 0)
                        continue;
                    out->add_visited(tp->get_name()->string());
                }
                else {
                    if (vtab->get((unsigned long)tp->get_name()) != ST_NIL)
                        continue;
                    vtab->add((unsigned long)tp->get_name(), 0, false);
                }
            }
            if (tp->should_skip())
                continue;

            if (usewin) {
                BBox xBB(*tBB);
                xBB.scale(prms->scale());
                cin->set_area_filt(true, &xBB);
            }
            ok = cin->chd_read_cell(tp, true);
            if (!ok) {
                Errs()->add_error("cCHD::write: cell read failed.");
                break;
            }
        }
    }
    cin->chd_finalize();
    in->assign_alias(cin->extract_alias());

    if (ok && out) {
        if (!in->no_end_lib() &&
                !out->write_endlib(p->get_name()->string())) {
            Errs()->add_error("cCHD::write: write end lib failed.");
            ok = false;
        }
    }
    OItype oiret =
        ok ? OIok : cin->was_interrupted() ? OIaborted : OIerror;

    return (oiret);
}


// This is used in the translators to handle CHD writes.
//
OItype
cCHD::translate_write(const FIOcvtPrms *prms, const char *chdcell)
{
    OItype oiret = OIerror;
    if (chdcell && *chdcell)
        oiret = write(chdcell, prms, true);
    else {
        symref_t *p = getConfigSymref();
        if (p)
            // CHD has been configured, use as-is.
            oiret = write(p->get_name()->string(), prms, true);
        else {
            // Write all top-level symrefs, using area if given.
            syrlist_t *sl0 = topCells(Physical, true);
            oiret = OIok;
            for (syrlist_t *sl = sl0; sl; sl = sl->next) {
                const char *cn = sl->symref->get_name()->string();
                oiret = write(cn, prms, true);
                if (oiret != OIok)
                    break;
            }
            sl0->free();
        }
    }
    return (oiret);
}


// Create a new input channel.  This supports the OASIS database stream
// feature.
//
cv_in *
cCHD::newInput(bool lmap_ok)
{
    if (c_cgd) {
        // The oas_in is connected to the OASIS byte stream, and
        // handles decompression and parsing.
        oas_in *oas = new oas_in(lmap_ok);
        if (!oas->setup_cgd_if(c_cgd, lmap_ok, c_filename,
                c_filetype, this)) {
            delete oas;
            return (0);
        }
        return (oas);
    }
    cv_in *in = FIO()->NewInput(c_filetype, lmap_ok);
    if (in) {
        // This will set the crc in zio_stream for gzipped files,
        // enabling random access.
        if (!in->setup_source(c_filename, this)) {
            delete in;
            return (0);
        }
    }
    return (in);
}


// Return the database name of a linked geometry database, if any.
//
const char *
cCHD::getCgdName() const
{
    return (c_cgd ? c_cgd->id_name() : 0);
}


// Private recursive core of the listCellnames function.
//
bool
cCHD::listCellnames_rc(symref_t *p, SymTab *tab, int depth)
{
    if (depth >= CDMAXCALLDEPTH)
        return (false);
    tab->add(p->get_name()->string(), 0, false);
    DisplayMode mode = p->mode();
    nametab_t *ntab = nameTab(mode);
    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        symref_t *cp = ntab->find_symref(c->srfptr);
        if (!cp || !cp->get_defseen())
            continue;
        if (tab->get(cp->get_name()->string()) == ST_NIL)
            listCellnames_rc(cp, tab, depth+1);
    }
    return (true);
}


// Private recursive core of the depthCounts function.
//
bool
cCHD::depthCounts_rc(symref_t *p, unsigned int depth, unsigned long pcnt,
    unsigned int *array)
{
    if (depth >= CDMAXCALLDEPTH) {
        Errs()->add_error(
            "depthCounts: call stack depth exceeded, "
            "recursive hierarchy?");
        return (false);
    }

    SymTab ctab(false, false);

    unsigned long lcnt = 0;
    crgen_t gen(nameTab(p->mode()), p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        symref_t *cp = nameTab(p->mode())->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "depthCounts: unresolved master back-pointer");
            return (false);
        }
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
                "depthCounts: unresolved transform ticket %d", c->attr);
            return (false);
        }
        unsigned long cnt = at.nx*at.ny;
        SymTabEnt *ent = ctab.get_ent((unsigned long)cp);
        if (!ent)
            ctab.add((unsigned long)cp, (void*)cnt, false);
        else
            ent->stData = (void*)((unsigned long)ent->stData + cnt);
        lcnt += cnt;
    }
    array[depth] += pcnt*lcnt;
    SymTabGen stgen(&ctab);
    SymTabEnt *ent;
    while ((ent = stgen.next()) != 0) {
        if (!depthCounts_rc((symref_t*)ent->stTag, depth+1,
                pcnt*(unsigned long)ent->stData, array))
            return (false);
    }
    return (true);
}


// Private recursive core of the instanceCounts function.
//
bool
cCHD::instanceCounts_rc(symref_t *p, unsigned int depth,
    unsigned long pcnt, SymTab *tab)
{
    if (depth >= CDMAXCALLDEPTH) {
        Errs()->add_error(
            "instanceCounts: call stack depth exceeded, "
            "recursive hierarchy?");
        return (false);
    }

    SymTabEnt *ent = tab->get_ent((unsigned long)p);
    if (!ent)
        tab->add((unsigned long)p, (void*)pcnt, false);
    else
        ent->stData = (void*)((unsigned long)ent->stData + pcnt);

    SymTab ctab(false, false);

    crgen_t gen(nameTab(p->mode()), p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        symref_t *cp = nameTab(p->mode())->find_symref(c->srfptr);
        if (!cp) {
            Errs()->add_error(
                "instanceCounts: unresolved master back-pointer");
            return (false);
        }
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
                "instanceCounts: unresolved transform ticket %d",
                c->attr);
            return (false);
        }
        unsigned long cnt = at.nx*at.ny;
        ent = ctab.get_ent((unsigned long)cp);
        if (!ent)
            ctab.add((unsigned long)cp, (void*)cnt, false);
        else
            ent->stData = (void*)((unsigned long)ent->stData + cnt);
    }
    SymTabGen stgen(&ctab);
    while ((ent = stgen.next()) != 0) {
        if (!instanceCounts_rc((symref_t*)ent->stTag, depth+1,
                pcnt*(unsigned long)ent->stData, tab))
            return (false);
    }
    return (true);
}


// Private recursive core of setup_bounds.
//
bool
cCHD::setBoundaries_rcprv(symref_t *p, unsigned int depth)
{
    if (depth >= CDMAXCALLDEPTH) {
        Errs()->add_error(
            "cCHD::setBoundaries_rc: hierarchy too deep, recursive?");
        return (false);
    }
    DisplayMode mode = p->mode();

    // Handle symrefs that were not defined in the source, through the
    // library mechanism.  If the referenced cell is accessed through
    // a CHD, switch the context to that CHD.  The symref bounding box
    // and flag will be set correctly if all goes well.
    //
    // NOTE: the library reference must be contained in an archive, not
    // a native cell.

    if (!p->get_defseen()) {
        cCHD *tchd;
        int rval = FIO()->ResolveLibSymref(p, &tchd, 0);
        if (rval < 0)
            // error
            return (false);
        if (rval == 0) {
            // unresolved
            if (FIO()->IsChdFailOnUnresolved()) {
                Errs()->add_error(
                    "cCHD::setBoundaries_rc: unresolved symref %s.",
                    p->get_name()->string());
                return (false);
            }
            p->set_bbok(true);
            return (true);
        }
        if (tchd) {
            symref_t *px = tchd->findSymref(p->get_name()->string(), mode);
            if (!px) {
                Errs()->add_error(
                    "cCHD::setBoundaries_rc: symref %s not found in CHD.",
                    p->get_name()->string());
                return (false);
            }
            if (!tchd->setBoundaries_rc(px, depth))
                return (false);
            BBox BB = *px->get_bb();
            if (BB == CDnullBB) {
                // Empty cell, ignored in BB calculations.
                // We know that p is also empty.
                p->set_bbok(true);
                return (true);
            }
            p->add_to_bb(&BB);
            p->set_bbok(true);

            if (user_fb->record())
                return (false);
            return (true);
        }
        Errs()->add_error(
            "cCHD::setBoundaries_rc: unresolvable symref %s.",
            p->get_name()->string());
        return (false);
    }

    cTfmStack stk;

    nametab_t *ntab = nameTab(mode);
    crgen_t gen(ntab, p);
    ticket_t last_srfptr = 0;
    symref_t *px = 0;
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr != last_srfptr) {
            last_srfptr = c->srfptr;
            px = ntab->find_symref(c->srfptr);
            if (!px) {
                Errs()->add_error(
                "cCHD::setBoundaries_rc: instance back pointer unresolved.");
                return (false);
            }
            if (!setBoundaries_rc(px, depth+1))
                return (false);
            if (*px->get_bb() == CDnullBB) {
                // Empty cell, ignored in BB calculations.
                px = 0;
                continue;
            }
        }
        if (!px)
            continue;
        CDattr at;
        if (!CD()->FindAttr(c->attr, &at)) {
            Errs()->add_error(
            "cCHD::setBoundaries_rc: unresolved transform ticket %d.",
                c->attr);
            return (false);
        }
        BBox BB(*px->get_bb());
        if (at.nx > 1) {
            if (at.dx > 0)
                BB.right += (at.nx - 1)*at.dx;
            else
                BB.left += (at.nx - 1)*at.dx;
        }
        if (at.ny > 1) {
            if (at.dy > 0)
                BB.top += (at.ny - 1)*at.dy;
            else
                BB.bottom += (at.ny - 1)*at.dy;
        }
        stk.TPush();
        stk.TApply(c->tx, c->ty, at.ax, at.ay, at.magn, at.refly);
        stk.TBB(&BB, 0);
        stk.TPop();
        p->add_to_bb(&BB);
    }
    p->set_bbok(true);
    if (user_fb->record())
        return (false);
    return (true);
}


// Private recursive core for instanceBoundaries.
//
bool
cCHD::instanceBoundaries_rc(symref_t *p, fio_chd::ib_t *ib, unsigned int depth)
{
    if (depth >= CDMAXCALLDEPTH) {
        Errs()->add_error(
            "cCHD::instanceWindows_rc: hierarchy too deep, recursive?");
        return (false);
    }
    DisplayMode mode = p->mode();

    if (!p->get_defseen()) {
        cCHD *tchd;
        bool isdev;
        int rval = FIO()->ResolveLibSymref(p, &tchd, &isdev);
        if (rval < 0)
            // error
            return (false);
        if (rval == 0) {
            // unresolved
            if (FIO()->IsChdFailOnUnresolved()) {
                Errs()->add_error(
                    "cCHD::instanceBoundaries_rc: unresolved symref %s.",
                    p->get_name()->string());
                return (false);
            }
            return (true);
        }
        if (!tchd) {
            // can't handle native reference
            if (isdev)
                // ignore inlines (device library)
                return (true);
            Errs()->add_error(
                "cCHD::instanceBoundaries_rc: unresolvable symref %s.",
                p->get_name()->string());
            return (false);
        }

        symref_t *px = tchd->findSymref(p->get_name()->string(), mode);
        if (!px) {
            Errs()->add_error(
                "cCHD::instanceWindows_rc: cell %s not found in CHD.",
                p->get_name()->string());
        }
        if (tchd->instanceBoundaries_rc(px, ib, depth)) {
            if (ib->check_tab.find(px))
                ib->check_tab.add(p);
            return (true);
        }
        return (false);
    }

    Blist *btmp = ib->blist;
    ticket_t last_srfptr = 0;
    symref_t *px = 0;
    nametab_t *ntab = nameTab(mode);
    crgen_t gen(ntab, p);
    const cref_o_t *c;
    while ((c = gen.next()) != 0) {
        if (c->srfptr != last_srfptr) {
            last_srfptr = c->srfptr;
            px = ntab->find_symref(c->srfptr);
            if (!px) {
                Errs()->add_error(
                    "cCHD::instanceBoundaries_rc: instance back pointer "
                    "unresolved.");
                return (false);
            }
            if (ib->check_tab.find(px)) {
                px = 0;
                continue;
            }
        }
        if (!px)
            continue;

        if (px->get_crefs() || px->get_name() == ib->refname ||
                !px->get_defseen()) {
            CDattr at;
            if (!CD()->FindAttr(c->attr, &at)) {
                Errs()->add_error(
                "cCHD::instanceBoundaries_rc: unresolved transform ticket %d.",
                    c->attr);
                return (false);
            }
            int x = c->tx;
            int y = c->ty;
            int dx = at.dx;
            int dy = at.dy;

            ib->TPush();
            ib->TApply(x, y, at.ax, at.ay, at.magn, at.refly);
            ib->TPremultiply();

            if (px->get_name() == ib->refname) {
                BBox BB = *px->get_bb();
                if (at.nx > 1) {
                    if (dx > 0)
                        BB.right += (at.nx - 1)*dx;
                    else
                        BB.left += (at.nx - 1)*dx;
                }
                if (at.ny > 1) {
                    if (dy > 0)
                        BB.top += (at.ny - 1)*dy;
                    else
                        BB.bottom += (at.ny - 1)*dy;
                }

                ib->TBB(&BB, 0);
                ib->blist = new Blist(&BB, ib->blist);
                ib->TPop();
                continue;
            }

            int tx, ty;
            ib->TGetTrans(&tx, &ty);
            xyg_t xyg(0, at.nx - 1, 0, at.ny - 1);

            do {
                ib->TTransMult(xyg.x*dx, xyg.y*dy);
                if (!instanceBoundaries_rc(px, ib, depth + 1)) {
                    ib->TPop();
                    return (false);
                }
                ib->TSetTrans(tx, ty);
            } while (xyg.advance());

            ib->TPop();
        }
    }
    if (ib->blist == btmp) {
        // No ref instance was found under the present symref, mark it
        // so we will skip future testing.
        ib->check_tab.add(p);
    }

    return (true);
}

