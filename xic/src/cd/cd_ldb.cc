
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

#include "cd.h"
#include "cd_types.h"
#include "cd_ldb.h"
#include "cd_strmdata.h"
#include <algorithm>
#include <ctype.h>


//
// Functions for the cCDldb layer database, a unified database for
// electrical and physical layers.  This keeps track of ordering and
// prevents name clashes.  Layers can be efficiently found by name,
// LPP, LPPname, or index.  All layer names are case-insensitive.
//
// This integrates OpenAcces layer and purpose numbers.  Separate
// tables hash layer and purpose names/numbers.  The Xic layers
// correspond to a layer/purpose pair (LPP), and are hashed by the
// pair, and optionally by an LPPname.  Access by name utilizes a
// (non-LPPname) name string in the form "layername[:purposename]"
// where a missing purposename is understood as "drawing".
//

// Base for assigned layer and purpose numbers.
#define LAYER_NUM_BASE      300
#define PURPOSE_NUM_BASE    300

// Base for private internal layers.
#define PRV_LAYER_NUM_BASE  1024

cCDldb *cCDldb::instancePtr = 0;

cCDldb::cCDldb() : ldb_phys(Physical), ldb_elec(Electrical)
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cCDldb already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    ldb_lpp_tab = 0;
    ldb_lppname_tab = 0;
    ldb_lppalias_tab = 0;
    ldb_oalname_tab = 0;
    ldb_oalnum_tab = 0;
    ldb_oapname_tab = 0;
    ldb_oapnum_tab = 0;
    ldb_unused_phys = 0;
    ldb_unused_elec = 0;
    ldb_invalid_list = 0;
    ldb_state = 0;
    ldb_reuse_list = 0;
    ldb_blocks = 0;
    ldb_elts_used = 0;
    ldb_num_private_lnames = 0;
    ldb_drvlyr_tab = 0;
    ldb_drvlyr_rmtab = 0;
    ldb_drv_index = 0;

    init();
}


cCDldb::~cCDldb()
{
    clear();
}


// Private static error exit.
//
void
cCDldb::on_null_ptr()
{
    fprintf(stderr, "Singleton class cCDldb used before instantiated.\n");
    exit(1);
}


// Perform application-specific initialization.
//
void
cCDldb::init()
{
    // Do a hard-coded saveOAlayer for the electrical layer names, so
    // the layer numbers will be fixed and predictable.  All have
    // purpose drawing, so no fancy name strings. 

    unsigned int lnum = PRV_LAYER_NUM_BASE;
    saveOAlayer("SCED", lnum++);
    saveOAlayer("ETC1", lnum++);
    saveOAlayer("ETC2", lnum++);
    saveOAlayer("NAME", lnum++);
    saveOAlayer("MODL", lnum++);
    saveOAlayer("VALU", lnum++);
    saveOAlayer("PARM", lnum++);
    saveOAlayer("NODE", lnum++);
    saveOAlayer("SPTX", lnum++);

    // Same for the cell layer.
    saveOAlayer(CELL_LAYER_NAME, lnum++);

    ldb_num_private_lnames = lnum - PRV_LAYER_NUM_BASE;
    // These layers won't be listed by listOAlayerTab.

    // Allocate and assign cell layer.
    CDl *ld = addNewLayer(CELL_LAYER_NAME, Physical, CDLcellInstance, 0);
    insertLayer(ld, Electrical, 0);
}


// Clear and re-init.
//
void
cCDldb::reset()
{
    clear();
    init();
}


// Throw away everything.
//
void
cCDldb::clear()
{
    // Delete the layers.
    ldb_phys.clear();
    ldb_elec.clear();

    // Delete the table shell, doesn't touch contents.
    delete ldb_lpp_tab;
    ldb_lpp_tab = 0;
    delete ldb_lppname_tab;
    ldb_lppname_tab = 0;

    if (ldb_lppalias_tab) {
        tgen_t<lppalias_t> lgen(ldb_lppalias_tab);
        lppalias_t *t;
        while ((t = lgen.next()) != 0) {
            delete [] t->tab_name();
            delete [] t->layer_name();
        }
        delete ldb_lppalias_tab;
        ldb_lppalias_tab = 0;
    }

    while (ldb_unused_phys) {
        CDll *ll = ldb_unused_phys;
        ldb_unused_phys = ldb_unused_phys->next;
        delete ll->ldesc;
        delete ll;
    }
    while (ldb_unused_elec) {
        CDll *ll = ldb_unused_elec;
        ldb_unused_elec = ldb_unused_elec->next;
        delete ll->ldesc;
        delete ll;
    }
    while (ldb_invalid_list) {
        CDll *ll = ldb_invalid_list;
        ldb_invalid_list = ldb_invalid_list->next;
        delete ll->ldesc;
        delete ll;
    }

    clearOAlayerTab();
    clearOApurposeTab();

    // Delete all elements.
    while (ldb_blocks) {
        ldb_blk *b = ldb_blocks;
        ldb_blocks = ldb_blocks->next;
        delete b;
    }
    ldb_reuse_list = 0;
    ldb_elts_used = 0;

    delete ldb_state;
    ldb_state = 0;

    tgen_t<drvlyr_t> gen(ldb_drvlyr_tab);
    drvlyr_t *d;
    while ((d = gen.next()) != 0)
        delete d;
    delete ldb_drvlyr_tab;
    ldb_drvlyr_tab = 0;

    tgen_t<drvlyr_t> rgen(ldb_drvlyr_rmtab);
    while ((d = gen.next()) != 0)
        delete d;
    delete ldb_drvlyr_rmtab;
    ldb_drvlyr_rmtab = 0;
}


// Static function.  Get a viable layer or purpose name from the
// passed string.  Return is null if the name is null or space-only,
// otherwise it is a copy.
//
char *
cCDldb::getValidName(const char *name)
{
    char *lname = strmdata::dec_to_hex(name);  // handle "layer,datatype"
    if (!lname) {
        // This strips any leading or trailing white space.
        const char *nm = name;
        lname = lstring::gettok(&nm);
        if (!lname)
            return (0);
    }
    // We allow alphanumeric chars plus '$', '_'.  Here, "bad" chars
    // are replaced by 'X'.
    //
    for (char *t = lname; *t; t++) {
        if (!isalnum(*t) && *t != '$' && *t != '_')
            *t = 'X';
    }
    return (lname);
}


// Return a name that is unique in the layer namespace, and also an
// unused layer number.  The name is constructed using the number.
//
char *
cCDldb::newLayerName(unsigned int *lnum)
{
    unsigned int n = newLayerNum();
    for (;;) {
        if (ldb_oalnum_tab && ldb_oalnum_tab->find(n)) {
            n++;
            continue;
        }
        char buf[16];
        snprintf(buf, sizeof(buf), "%c%03X", 'L', n);
        if (!ldb_oalname_tab || !ldb_oalname_tab->find(buf)) {
            if (lnum)
                *lnum = n;
            return (lstring::copy(buf));
        }
        n++;
    }
    // not reached
    // return (0);
}


// Return an available layer number.
//
unsigned int
cCDldb::newLayerNum()
{
    for (unsigned int i = LAYER_NUM_BASE; ; i++) {
        if (!ldb_oalnum_tab || !ldb_oalnum_tab->find(i))
            return (i);
    }
    // not reached
}


// Return an available purpose number.
//
unsigned int
cCDldb::newPurposeNum()
{
    for (unsigned int i = PURPOSE_NUM_BASE; ; i++) {
        if (!ldb_oapnum_tab || !ldb_oapnum_tab->find(i))
            return (i);
    }
    // not reached
}


// Save an alias name for lname.  The allias name will also resolve a
// layer, if lname can be resolved.
//
void
cCDldb::saveAlias(const char *alias, const char *lname)
{
    if (!alias || !*alias)
        return;
    if (!lname || !*lname)
        return;
    if (!ldb_lppalias_tab)
        ldb_lppalias_tab = new ctable_t<lppalias_t>;
    else {
        lppalias_t *t = ldb_lppalias_tab->find(alias);
        if (t) {
            t->set_layer_name(lname);
            return;
        }
    }
    lppalias_t *t = (lppalias_t*)new_elt();
    t->init(alias, lname);
    ldb_lppalias_tab->link(t, false);
    ldb_lppalias_tab = ldb_lppalias_tab->check_rehash();
}


// Remove the alias from the table.
//
void
cCDldb::removeAlias(const char *alias)
{
    if (!ldb_lppalias_tab)
        return;
    if (!alias || !*alias)
        return;
    lpp_t *lp = (lpp_t*)ldb_lppalias_tab->remove(alias);
    if (lp) {
        lp->set_tab_next(ldb_reuse_list);
        ldb_reuse_list = lp;
        lppalias_t *t = (lppalias_t*)lp;
        delete [] t->tab_name();
        delete [] t->layer_name();
    }
}


namespace {
    bool alicomp(const stringnumlist *s1, const stringnumlist *s2)
    {
        if (s1->num == s2->num)
            return (strcmp(s1->string, s2->string) < 0);
        return (s1->num < s2->num);
    }
}


// Return a list of aliases.  The list is filtered to contain only
// entries with a valid physical layer name, and is sorted in order
// of the layer table index.  The return list should be freed by the
// caller.
//
string2list *
cCDldb::listAliases()
{
    if (!ldb_lppalias_tab)
        return (0);
    stringnumlist *s0 = 0;
    tgen_t<lppalias_t> lgen(ldb_lppalias_tab);
    lppalias_t *t;
    int cnt = 0;
    while ((t = lgen.next()) != 0) {
        CDl *ld = CDldb()->findLayer(t->layer_name(), Physical);
        if (!ld)
            continue;
        s0 = new stringnumlist(lstring::copy(t->tab_name()), ld->physIndex(),
            s0);
        cnt++;
    }
    if (cnt > 1) {
        stringnumlist **ary = new stringnumlist*[cnt];
        cnt = 0;
        for (stringnumlist *s = s0; s; s = s->next)
            ary[cnt++] = s;
        std::sort(ary, ary+cnt, alicomp);
        for (int i = 1; i < cnt; i++)
            ary[i-1]->next = ary[i];
        ary[cnt-1]->next = 0;
        s0 = ary[0];
        delete [] ary;
    }
    string2list *s20 = 0, *s2e = 0;       
    stringnumlist *sn;
    for (stringnumlist *s = s0; s; s = sn) {
        sn = s->next;
        CDl *ld = CDldb()->layer(s->num, Physical);
        if (!ld)
            // "can't happen"
            continue;
        string2list *s2 = new string2list(s->string, lstring::copy(ld->name()),
            0);
        if (!s20)
            s20 = s2e = s2;
        else {
            s2e->next = s2;
            s2e = s2;
        }
        s->string = 0;
        delete s;
    }
    return (s20);
}


namespace {
    // Split a layername:purposename token.
    //
    void splitlp(const char *name, char **lname, char **pname)
    {
        const char *t = strchr(name, ':');
        if (t) {
            char *n = new char[t - name + 1];
            strncpy(n, name, t - name);
            n[t - name] = 0;
            *lname = n;
            *pname = lstring::copy(t+1);
        }
        else {
            *lname = lstring::copy(name);
            *pname = 0;
        }
    }
}


// Resolve the layer/purpose numbers from the name.  The format for
// the name is "layername[:purposename]", where if purposename is
// omitted, "drawing" is understood.
//
bool
cCDldb::resolveName(const char *name, unsigned int *lnp, unsigned int *pnp)
{
    if (!name)
        return (false);
    char *lname, *pname;
    splitlp(name, &lname, &pname);

    // Make sure that the names are valid.
    char *new_lname = getValidName(lname);
    delete [] lname;
    lname = new_lname;
    char *new_pname = getValidName(pname);
    delete [] pname;
    pname = new_pname;

    unsigned int lnum = getOAlayerNum(lname);
    delete [] lname;
    if (lnum == CDL_NO_LAYER) {
        delete [] pname;
        return (false);
    }
    bool unknown;
    unsigned int pnum = getOApurposeNum(pname, &unknown);
    delete [] pname;
    if (pnum == CDL_PRP_DRAWING_NUM && unknown)
        return (false);
    *lnp = lnum;
    *pnp = pnum;
    return (true);
}


// This function creates a new layer, and adds it to the end of the
// list.  It is called when reading in a new file, if an undefined
// layer is encountered.
//
// If the name starts with '$', create the layer as CDLinternal.
// These layers are not written out to files and are intended to
// be temporary.
//
// The format for the name is "layername[:purposename]", where if
// purposename is omitted, "drawing" is understood.
//
CDl *
cCDldb::newLayer(const char *name, DisplayMode mode)
{
    if (!name)
        return (0);
    CDl *ld = findLayer(name);
    if (ld) {
        // If layer already exists for this mode, return it.
        if (ld->index(mode) >= 0)
            return (ld);

        // If the layer is invalid, return it.
        if (ld->isInvalid())
            return (ld);
    }
    else
        ld = getUnusedLayer(name, mode);
    if (ld) {
        if (!insertLayer(ld, mode, layersUsed(mode))) {
            // Hmmm, something went wrong, silently ignore this
            // and try to create a new layer.
            Errs()->get_error();
            if (ld->index(Physical) < 0 && ld->index(Electrical) < 0)
                delete ld;
            ld = 0;
        }
    }
    if (!ld)
        ld = addNewLayer(name, mode,
            *name == '$' ? CDLinternal : CDLnormal, -1);
    return (ld);
}


// As above, lnum must already be in the OA layer table.  Likewise,
// pnum must be in the purpose table, or be the drawing purpose.
//
CDl *
cCDldb::newLayer(unsigned int lnum, unsigned int pnum, DisplayMode mode)
{
    if (!getOAlayerName(lnum))
        return (0);
    CDl *ld = findLayer(lnum, pnum);
    if (ld) {
        // If layer already exists, just return it.
        if (ld->index(mode) >= 0)
            return (ld);

        // If the layer is invalid, return it.
        if (ld->isInvalid())
            return (ld);
    }
    else
        ld = getUnusedLayer(lnum, pnum, mode);
    if (ld) {
        if (!insertLayer(ld, mode, layersUsed(mode))) {
            // Hmmm, something went wrong, silently ignore this
            // and try to create a new layer.
            Errs()->get_error();
            if (ld->index(Physical) < 0 && ld->index(Electrical) < 0)
                delete ld;
            ld = 0;
        }
    }
    if (!ld) {
        if (!getOApurposeName(pnum) && pnum != CDL_PRP_DRAWING_NUM) {
            Errs()->add_error("Purpose number %d not in table.", pnum);
            return (0);
        }
        ld = new CDl(CDLnormal);
        ld->setOaLpp(lnum, pnum);

        // This should never fail.
        if (!insertLayer(ld, mode, layersUsed(mode))) {
            if (ld->index(Physical) < 0 && ld->index(Electrical) < 0)
                delete ld;
            return (0);
        }
    }
    return (ld);
}


// Create a new layer for mode and add it at indx, or to the end if
// indx is negative or too large.  This takes care of ensuring that
// the name is valid and unique, a new name will be assigned if
// needed.
//
// The format for the name is "layername[:purposename]", where if
// purposename is omitted, "drawing" is understood.
//
// If indx is negative and noinsert is true, the new layer will not be
// added to the layer table, but will go to the removed list.
//
CDl *
cCDldb::addNewLayer(const char *name, DisplayMode mode, CDLtype ltype,
    int indx, bool noinsert)
{
    char *lname, *pname;
    splitlp(name, &lname, &pname);

    // Make sure that the names are valid.
    char *new_lname = getValidName(lname);
    delete [] lname;
    lname = new_lname;
    char *new_pname = getValidName(pname);
    delete [] pname;
    pname = new_pname;

    bool unknown;
    unsigned int pnum = getOApurposeNum(pname, &unknown);
    if (pnum == CDL_PRP_DRAWING_NUM && unknown) {
        // The purpose name is new, valid, and doesn't exist.
        // Create a new purpose entry.

        pnum = newPurposeNum();
        if (!saveOApurpose(pname, pnum)) {
            // "can't fail"
            delete [] lname;
            delete [] pname;
            return (0);
        }
    }

    unsigned int lnum = getOAlayerNum(lname);
    if (lnum == CDL_NO_LAYER) {
        // The layer entry doesn't exist, create it.

        lnum = newLayerNum();
        if (!saveOAlayer(lname, lnum)) {
            // "can't fail"
            delete [] lname;
            delete [] pname;
            return (0);
        }
    }
    else if (ldb_lpp_tab && ldb_lpp_tab->find(lnum, pnum)) {
        // The Xic layer already exists.  We'll add the new layer
        // under a new name.

        delete [] lname;
        lname = newLayerName(&lnum);
        if (!saveOAlayer(lname, lnum)) {
            // "can't fail"
            delete [] lname;
            delete [] pname;
            return (0);
        }
    }
    delete [] lname;
    delete [] pname;

    CDl *ld = new CDl(ltype);
    ld->setOaLpp(lnum, pnum);
    if (ltype != CDLnormal) {
        ld->setInvisible(true);
        ld->setNoSelect(true);
    }
    if (noinsert && indx < 0) {
        // Add to the LPP table.
        //
        if (!ldb_lpp_tab)
            ldb_lpp_tab = new xytable_t<lpp_t>;
        if (!ldb_lpp_tab->find(ld->oaLayerNum(), ld->oaPurposeNum())) {
            lpp_t *e = (lpp_t*)new_elt();
            e->init(ld, ld->oaLayerNum(), ld->oaPurposeNum());
            ldb_lpp_tab->link(e, false);
            ldb_lpp_tab = ldb_lpp_tab->check_rehash();
        }
        pushUnusedLayer(ld, mode);
    }
    else {
        int lused = layersUsed(mode);
        if (indx < 0 || indx > lused)
            indx = lused;

        // This should never fail.
        if (!insertLayer(ld, mode, indx)) {
            delete ld;
            return (0);
        }
    }

    return (ld);
}


// Assign an "LPPname" to the Xic layer.  The layer will be found by
// this name with the findLayer function.
//
// The LPPname is checked first in the findLayer function.  An LPPname
// that clashes (case-insensitive!) with an OA layer name will "hide"
// the layer with the matching OA name and "drawing" purpose.  This
// can be used for layer remapping, but the user must *be careful*
// with this.
//
// Unlike the normal layer names, the lppname can have arbitrary
// punctuation, embedded white space, etc.  However, leading and
// trailing white space is removed, and if the resulting string is
// empty or null, the existing LPPname (if any) will be removed.
//
bool
cCDldb::setLPPname(CDl *ld, const char *lppname)
{
    if (!ld)
        return (true);
    if (!lppname)
        lppname = "";
    while (isspace(*lppname))
        lppname++;
    if (!*lppname) {
        // Null or empty name, remove existing name if any.
        lppname = ld->lppName();
        if (lppname) {
            if (ldb_lppname_tab &&
                    (ld->physIndex() >= 0 || ld->elecIndex() >= 0))
                keep(ldb_lppname_tab->remove(lppname));
            ld->setLPPname(0);
        }
        return (true);
    }

    // Remove trailing white space, we know lppname has non-space
    // characters.
    char *tmpstr = lstring::copy(lppname);
    char *t = tmpstr + strlen(tmpstr) - 1;
    while (isspace(*t))
        *t-- = 0;
    lppname = tmpstr;
    
    if (ld->physIndex() < 0 && ld->elecIndex() < 0) {
        // Not in table.
        ld->setLPPname(lppname);
        delete [] tmpstr;
        return (true);
    }

    if (!ldb_lppname_tab)
        ldb_lppname_tab = new ctable_t<lppname_t>;
    lppname_t *e = ldb_lppname_tab->find(lppname);
    if (e) {
        delete [] tmpstr;
        return (ld == e->ldesc());
    }

    e = 0;
    if (ld->lppName())
        e = ldb_lppname_tab->remove(ld->lppName());
    if (!e)
        e = (lppname_t*)new_elt();
    ld->setLPPname(lppname);
    e->init(ld);
    ldb_lppname_tab->link(e);
    ldb_lppname_tab = ldb_lppname_tab->check_rehash();
    delete [] tmpstr;
    return (true);
}


// Destroy all layers, except don't delete any layer structs listed in
// the arguments, if these are not null.  The passed arrays are
// terminated with a zero entry.
//
void
cCDldb::clearLayers(CDl **keepPhys, CDl **keepElec)
{
    CDl *ld;
    while ((ld = removeLayer(1, Physical)) != 0) {
        bool keepme = false;
        if (keepPhys) {
            for (int i = 0; keepPhys[i]; i++) {
                if (ld == keepPhys[i]) {
                    keepme = true;
                    break;
                }
            }
        }
        if (!keepme)
            delete ld;
    }
    while ((ld = removeLayer(1, Electrical)) != 0) {
        bool keepme = false;
        if (keepElec) {
            for (int i = 0; keepElec[i]; i++) {
                if (ld == keepElec[i]) {
                    keepme = true;
                    break;
                }
            }
        }
        if (!keepme)
            delete ld;
    }
    if (ldb_unused_phys) {
        while (ldb_unused_phys) {
            CDll *ll = ldb_unused_phys;
            ldb_unused_phys = ll->next;
            ld = ll->ldesc;
            delete ll;
            bool keepme = false;
            if (keepPhys) {
                for (int i = 0; keepPhys[i]; i++) {
                    if (ld == keepPhys[i]) {
                        keepme = true;
                        break;
                    }
                }
            }
            if (!keepme)
                delete ld;
        }
        CD()->ifUnusedLayerListChange(0, Physical);
    }
    if (ldb_unused_elec) {
        while (ldb_unused_elec) {
            CDll *ll = ldb_unused_elec;
            ldb_unused_elec = ll->next;
            ld = ll->ldesc;
            delete ll;
            bool keepme = false;
            if (keepElec) {
                for (int i = 0; keepElec[i]; i++) {
                    if (ld == keepElec[i]) {
                        keepme = true;
                        break;
                    }
                }
            }
            if (!keepme)
                delete ld;
        }
        CD()->ifUnusedLayerListChange(0, Electrical);
    }

    // The LPP table has been reduced properly, however the OA tables
    // may have entries that are no longer referenced.

    SymTab *tab = new SymTab(false, false);
    tab->add(ldb_phys.layer(0)->oaLayerNum(), 0, false);
    tab->add(ldb_elec.layer(0)->oaLayerNum(), 0, false);
    if (keepPhys) {
        for (int i = 0; keepPhys[i]; i++)
            tab->add(keepPhys[i]->oaLayerNum(), 0, true);
    }
    if (keepElec) {
        for (int i = 0; keepElec[i]; i++)
            tab->add(keepElec[i]->oaLayerNum(), 0, true);
    }
    tgen_t<loa_t> lgen(ldb_oalnum_tab);
    loa_t *e;
    while ((e = lgen.next()) != 0) {
        if (SymTab::get(tab, e->num()) == ST_NIL) {
            loa_t *ne = ldb_oalname_tab->remove(e->name());
            if (ne) {
                // always true
                delete [] ne->name();
                keep(ne);
            }
            ldb_oalnum_tab->unlink(e);
            keep(e);
        }
    }
    delete tab;
    tab = new SymTab(false, false);
    tab->add(ldb_phys.layer(0)->oaPurposeNum(), 0, false);
    tab->add(ldb_elec.layer(0)->oaPurposeNum(), 0, true);
    if (keepPhys) {
        for (int i = 0; keepPhys[i]; i++)
            tab->add(keepPhys[i]->oaPurposeNum(), 0, true);
    }
    if (keepElec) {
        for (int i = 0; keepElec[i]; i++)
            tab->add(keepElec[i]->oaPurposeNum(), 0, true);
    }
    tgen_t<loa_t> pgen(ldb_oapnum_tab);
    while ((e = pgen.next()) != 0) {
        if (SymTab::get(tab, e->num()) == ST_NIL) {
            loa_t *ne = ldb_oapname_tab->remove(e->name());
            if (ne) {
                // always true
                delete [] ne->name();
                keep(ne);
            }
            ldb_oapnum_tab->unlink(e);
            keep(e);
        }
    }
    delete tab;
}


// The next two functions save and restore the state of the database. 
// This can be used, for example, to revert the layer database to the
// state just after the technology file was read, getting rid of
// layers that might have been created subsequently.
//
// Layer presentation attribute changes are not reverted, nor are
// layer name changes.  This only guarantees to have the layer descs
// in the right order and places, and will create new layer descs for
// deleted layers.

namespace {
    // List element for an Xic layer.  We don't save pointers, since
    // it is possible to delete the layers.
    struct ldb_lpp
    {
        ldb_lpp(const char *n, unsigned int ln, unsigned int pn,
                unsigned int flg)
            {
                next = 0;
                l_lpp_name = lstring::copy(n);
                l_lnum = ln;
                l_pnum = pn;
                l_flags = flg;
            }

        ~ldb_lpp()
            {
                delete [] l_lpp_name;
            }

        static void destroy(const ldb_lpp *l)
            {
                while (l) {
                    const ldb_lpp *lx = l;
                    l = l->next;
                    delete lx;
                }
            }

        ldb_lpp *next;
        char *l_lpp_name;
        unsigned int l_lnum;
        unsigned int l_pnum;
        unsigned int l_flags;
    };

    // This saves all the layers, which provides enough info to
    // rebuild the tables.
    struct LDBstate : public CDldbState
    {
        LDBstate()
            {
                s_phys_ary = 0;
                s_elec_ary = 0;
                s_unused_phys = 0;
                s_unused_elec = 0;
                s_invalid_list = 0;
            }

        ~LDBstate()
            {
                ldb_lpp::destroy(s_phys_ary);
                ldb_lpp::destroy(s_elec_ary);
                ldb_lpp::destroy(s_unused_phys);
                ldb_lpp::destroy(s_unused_elec);
                ldb_lpp::destroy(s_invalid_list);
            }

        ldb_lpp *s_phys_ary;
        ldb_lpp *s_elec_ary;
        ldb_lpp *s_unused_phys;
        ldb_lpp *s_unused_elec;
        ldb_lpp *s_invalid_list;
    };
}


// Return an opaque object describing the current state of the database.
// This has no use except for passing to the setState method.
//
CDldbState *
cCDldb::getState()
{
    LDBstate *s = new LDBstate;
    ldb_lpp *lpe = 0;
    for (int i = 0; ; i++) {
        CDl *ld = ldb_phys.layer(i);
        if (!ld)
            break;
        ldb_lpp *lpp = new ldb_lpp(ld->lppName(), ld->oaLayerNum(),
            ld->oaPurposeNum(), ld->flags());
        if (!s->s_phys_ary)
            s->s_phys_ary = lpe = lpp;
        else {
            lpe->next = lpp;
            lpe = lpp;
        }
    }
    lpe = 0;
    for (int i = 0; ; i++) {
        CDl *ld = ldb_elec.layer(i);
        if (!ld)
            break;
        ldb_lpp *lpp = new ldb_lpp(ld->lppName(), ld->oaLayerNum(),
            ld->oaPurposeNum(), ld->flags());
        if (!s->s_elec_ary)
            s->s_elec_ary = lpe = lpp;
        else {
            lpe->next = lpp;
            lpe = lpp;
        }
    }
    lpe = 0;
    for (CDll *ll = ldb_unused_phys; ll; ll = ll->next) {
        CDl *ld = ll->ldesc;
        ldb_lpp *lpp = new ldb_lpp(ld->lppName(), ld->oaLayerNum(),
            ld->oaPurposeNum(), ld->flags());
        if (!s->s_unused_phys)
            s->s_unused_phys = lpe = lpp;
        else {
            lpe->next = lpp;
            lpe = lpp;
        }
    }
    lpe = 0;
    for (CDll *ll = ldb_unused_elec; ll; ll = ll->next) {
        CDl *ld = ll->ldesc;
        ldb_lpp *lpp = new ldb_lpp(ld->lppName(), ld->oaLayerNum(),
            ld->oaPurposeNum(), ld->flags());
        if (!s->s_unused_elec)
            s->s_unused_elec = lpe = lpp;
        else {
            lpe->next = lpp;
            lpe = lpp;
        }
    }
    lpe = 0;
    for (CDll *ll = ldb_invalid_list; ll; ll = ll->next) {
        CDl *ld = ll->ldesc;
        ldb_lpp *lpp = new ldb_lpp(ld->lppName(), ld->oaLayerNum(),
            ld->oaPurposeNum(), ld->flags());
        if (!s->s_invalid_list)
            s->s_invalid_list = lpe = lpp;
        else {
            lpe->next = lpp;
            lpe = lpp;
        }
    }
    return (s);
}


// Revert the state of the database to the saved state.  Note that
// layer attributes aren't saved, only the presence and ordering of
// the arrays, and the contents of the lists.  The hash tables are
// rebuilt.  If a layer is no longer around, it will be recreated.
//
// A list of layers that are no longer in use is returned in trash, if
// it is not null.  If null, the layers are freed.  The user must
// ensure that the cell database contains no references to freed
// layers.
//
bool
cCDldb::setState(const CDldbState *sptr, CDll **trash)
{
    if (trash)
        *trash = 0;
    CDll *layers = 0;
    LDBstate *s = (LDBstate*)sptr;

    // We never shrink the arrays, so we know that they're big enough.
    ldb_lpp *lpp = s->s_phys_ary;
    ldb_phys.set_numused(0);
    for (int i = 0; i < ldb_phys.allocated(); i++) {
        CDl *ld = ldb_phys.layer(i);
        if (ld) {
            layers = new CDll(ld, layers);
            ldb_phys.set_layer(i, 0);
        }
        if (lpp) {
            ld = findAnyLayer(lpp->l_lnum, lpp->l_pnum);
            if (!ld) {
                ld = new CDl(CDLnormal);
                ld->setOaLpp(lpp->l_lnum, lpp->l_pnum);
            }
            ld->setLPPname(lpp->l_lpp_name);
            ld->setFlags(lpp->l_flags);
            ld->setPhysIndex(i);
            ldb_phys.set_layer(i, ld);
            lpp = lpp->next;
            ldb_phys.inc_numused();
        }
    }

    lpp = s->s_elec_ary;
    ldb_elec.set_numused(0);
    for (int i = 0; i < ldb_elec.allocated(); i++) {
        CDl *ld = ldb_elec.layer(i);
        if (ld) {
            layers = new CDll(ld, layers);
            ldb_elec.set_layer(i, 0);
        }
        if (lpp) {
            ld = findAnyLayer(lpp->l_lnum, lpp->l_pnum);
            if (!ld) {
                ld = new CDl(CDLnormal);
                ld->setOaLpp(lpp->l_lnum, lpp->l_pnum);
            }
            ld->setLPPname(lpp->l_lpp_name);
            ld->setFlags(lpp->l_flags);
            ld->setElecIndex(i);
            ldb_elec.set_layer(i, ld);
            lpp = lpp->next;
            ldb_elec.inc_numused();
        }
    }

    CDll *tmp_unused_phys = 0;
    CDll *end = 0;
    for (lpp = s->s_unused_phys; lpp; lpp = lpp->next) {
        CDl *ld = findAnyLayer(lpp->l_lnum, lpp->l_pnum);
        if (!ld) {
            ld = new CDl(CDLnormal);
            ld->setOaLpp(lpp->l_lnum, lpp->l_pnum);
        }
        ld->setLPPname(lpp->l_lpp_name);
        ld->setFlags(lpp->l_flags);
        ld->setPhysIndex(-1);
        if (!tmp_unused_phys)
            tmp_unused_phys = end = new CDll(ld, 0);
        else {
            end->next = new CDll(ld, 0);
            end = end->next;
        }
    }

    CDll *tmp_unused_elec = 0;
    end = 0;
    for (lpp = s->s_unused_elec; lpp; lpp = lpp->next) {
        CDl *ld = findAnyLayer(lpp->l_lnum, lpp->l_pnum);
        if (!ld) {
            ld = new CDl(CDLnormal);
            ld->setOaLpp(lpp->l_lnum, lpp->l_pnum);
        }
        ld->setLPPname(lpp->l_lpp_name);
        ld->setFlags(lpp->l_flags);
        ld->setElecIndex(-1);
        if (!tmp_unused_elec)
            tmp_unused_elec = end = new CDll(ld, 0);
        else {
            end->next = new CDll(ld, 0);
            end = end->next;
        }
    }

    CDll *tmp_invalid_list = 0;
    end = 0;
    for (lpp = s->s_invalid_list; lpp; lpp = lpp->next) {
        CDl *ld = findAnyLayer(lpp->l_lnum, lpp->l_pnum);
        if (!ld) {
            ld = new CDl(CDLnormal);
            ld->setOaLpp(lpp->l_lnum, lpp->l_pnum);
        }
        ld->setLPPname(lpp->l_lpp_name);
        ld->setFlags(lpp->l_flags);
        ld->setPhysIndex(-1);
        ld->setElecIndex(-1);
        if (!tmp_invalid_list)
            tmp_invalid_list = end = new CDll(ld, 0);
        else {
            end->next = new CDll(ld, 0);
            end = end->next;
        }
    }

    // We've got all the layers, replace the lists and fix up the
    // tables.

    if (ldb_unused_phys) {
        CDll *tmp = ldb_unused_phys;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = layers;
        layers = ldb_unused_phys;
    }
    ldb_unused_phys = tmp_unused_phys;

    if (ldb_unused_elec) {
        CDll *tmp = ldb_unused_elec;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = layers;
        layers = ldb_unused_elec;
    }
    ldb_unused_elec = tmp_unused_elec;

    if (ldb_invalid_list) {
        CDll *tmp = ldb_invalid_list;
        while (tmp->next)
            tmp = tmp->next;
        tmp->next = layers;
        layers = ldb_invalid_list;
    }
    ldb_invalid_list = tmp_invalid_list;

    // Clear and rebuild the LPP tables.
    tgen_t<lpp_t> gen(ldb_lpp_tab);
    lpp_t *e;
    while ((e = gen.next()) != 0) {
        ldb_lpp_tab->unlink(e);
        keep(e);
    }
    tgen_t<lppname_t> ngen(ldb_lppname_tab);
    lppname_t *en;
    while ((en = ngen.next()) != 0) {
        ldb_lppname_tab->unlink(en);
        keep(en);
    }
    for (int i = 0; ; i++) {
        CDl *ld = ldb_phys.layer(i);
        if (!ld)
            break;
        e = (lpp_t*)new_elt();
        e->init(ld, ld->oaLayerNum(), ld->oaPurposeNum());
        ldb_lpp_tab->link(e);
        if (ld->lppName()) {
            en = (lppname_t*)new_elt();
            en->init(ld);
            ldb_lppname_tab->link(en);
        }
    }
    for (int i = 0; ; i++) {
        CDl *ld = ldb_elec.layer(i);
        if (!ld)
            break;
        e = (lpp_t*)new_elt();
        e->init(ld, ld->oaLayerNum(), ld->oaPurposeNum());
        ldb_lpp_tab->link(e);
        if (ld->lppName()) {
            en = (lppname_t*)new_elt();
            en->init(ld);
            ldb_lppname_tab->link(en);
        }
    }
    for (CDll *ll = ldb_invalid_list; ll; ll = ll->next) {
        CDl *ld = ll->ldesc;
        e = (lpp_t*)new_elt();
        e->init(ld, ld->oaLayerNum(), ld->oaPurposeNum());
        ldb_lpp_tab->link(e);
        if (ld->lppName()) {
            en = (lppname_t*)new_elt();
            en->init(ld);
            ldb_lppname_tab->link(en);
        }
    }
    ldb_lpp_tab = ldb_lpp_tab->check_rehash();
    ldb_lppname_tab = ldb_lppname_tab->check_rehash();

    // The LPP tables have been recreated properly, however the OA
    // tables may have entries that are no longer referenced.

    SymTab *tab = new SymTab(false, false);
    for (int i = 0; ; i++) {
        CDl *ld = ldb_phys.layer(i);
        if (!ld)
            break;
        tab->add(ld->oaLayerNum(), 0, true);
    }
    for (int i = 0; ; i++) {
        CDl *ld = ldb_elec.layer(i);
        if (!ld)
            break;
        tab->add(ld->oaLayerNum(), 0, true);
    }
    for (CDll *ll = ldb_unused_phys; ll; ll = ll->next)
        tab->add(ll->ldesc->oaLayerNum(), 0, true);
    for (CDll *ll = ldb_unused_elec; ll; ll = ll->next)
        tab->add(ll->ldesc->oaLayerNum(), 0, true);
    for (CDll *ll = ldb_invalid_list; ll; ll = ll->next)
        tab->add(ll->ldesc->oaLayerNum(), 0, true);
    tgen_t<loa_t> lgen(ldb_oalnum_tab);
    loa_t *eoa;
    while ((eoa = lgen.next()) != 0) {
        if (SymTab::get(tab, eoa->num()) == ST_NIL) {
            loa_t *ne = ldb_oalname_tab->remove(eoa->name());
            if (ne) {
                // always true
                delete [] ne->name();
                keep(ne);
            }
            ldb_oalnum_tab->unlink(eoa);
            keep(eoa);
        }
    }
    delete tab;

    tab = new SymTab(false, false);
    for (int i = 0; ; i++) {
        CDl *ld = ldb_phys.layer(i);
        if (!ld)
            break;
        tab->add(ld->oaPurposeNum(), 0, true);
    }
    for (int i = 0; ; i++) {
        CDl *ld = ldb_elec.layer(i);
        if (!ld)
            break;
        tab->add(ld->oaPurposeNum(), 0, true);
    }
    for (CDll *ll = ldb_unused_phys; ll; ll = ll->next)
        tab->add(ll->ldesc->oaPurposeNum(), 0, true);
    for (CDll *ll = ldb_unused_elec; ll; ll = ll->next)
        tab->add(ll->ldesc->oaPurposeNum(), 0, true);
    for (CDll *ll = ldb_invalid_list; ll; ll = ll->next)
        tab->add(ll->ldesc->oaPurposeNum(), 0, true);
    tgen_t<loa_t> pgen(ldb_oapnum_tab);
    while ((eoa = pgen.next()) != 0) {
        if (SymTab::get(tab, eoa->num()) == ST_NIL) {
            loa_t *ne = ldb_oapname_tab->remove(eoa->name());
            if (ne) {
                // always true
                delete [] ne->name();
                keep(ne);
            }
            ldb_oapnum_tab->unlink(eoa);
            keep(eoa);
        }
    }
    delete tab;

    // Finally, check the list of old layers, keep only the ones
    // still around.  The layers are returned if the user passed
    // an address, or are deleted otherwise.

    CDll *lp = 0, *ln;
    for (CDll *ll = layers; ll; ll = ln) {
        ln = ll->next;
        if (findAnyLayer(ll->ldesc->oaLayerNum(), ll->ldesc->oaPurposeNum())) {
            if (!lp)
                layers = ln;
            else 
                lp->next = ln;
            delete ll;
            continue;
        }
        lp = ll;
    }
    if (trash)
        *trash = layers;
    else {
        while (layers) {
            CDll *ll = layers;
            layers = layers->next;
            delete ll->ldesc;
            delete ll;
        }
    }
    return (true);
}


// Save the present state locally.
//
void
cCDldb::saveState()
{
    delete ldb_state;
    ldb_state = getState();
}


// Revert to the saved state.  The cell database and anything else
// that uses CDl pointers MUST be clear, since layers may be freed.
//
void
cCDldb::revertState()
{
    if (ldb_state)
        setState(ldb_state, 0);
}


// Deal with the Invalid flag.  Invalid layers are removed from the
// array, but kept in the hash tables.  Thus, they won't apear in the
// layer table, but will be found if referenced in a layout.  If the
// Invalid flag is unset, the layer is put back at the end of the
// *Physical* array.
//
bool
cCDldb::setInvalid(CDl *ld, bool invalid)
{
    if (!ld)
        return (true);
    if (invalid && !ld->isInvalid()) {
        if (ld->physIndex() >= 0) {
            ldb_phys.remove(ld->physIndex());
            ld->setPhysIndex(-1);
        }
        if (ld->elecIndex() >= 0) {
            ldb_elec.remove(ld->elecIndex());
            ld->setElecIndex(-1);
        }
        ld->setInvalid(true);
        ldb_invalid_list = new CDll(ld, ldb_invalid_list);
    }
    if (!invalid && ld->isInvalid()) {
        ld->setInvalid(false);
        if (!insertLayer(ld, Physical, ldb_phys.numused()))
            return (false);
        CDll *lp = 0;
        for (CDll *l = ldb_invalid_list; l; l = l->next) {
            if (l->ldesc == ld) {
                if (!lp)
                    ldb_invalid_list = l->next;
                else
                    lp->next = l->next;
                delete l;
                break;
            }
            lp = l;
        }
    }
    return (true);
}


// Remove and return the layer with index ix.  Layers above ix are
// pushed down.
//
CDl *
cCDldb::removeLayer(int ix, DisplayMode mode)
{
    CDlary *ary = (mode == Physical ? &ldb_phys : &ldb_elec);

    CDl *ld = ary->layer(ix);
    if (ld) {
        ary->remove(ix);

        // Remove from LPP table.
        if (ldb_lpp_tab) {
            lpp_t *e = ldb_lpp_tab->remove(ld->oaLayerNum(),
                ld->oaPurposeNum());
            ldb_lpp_tab = ldb_lpp_tab->check_rehash();
            keep(e);
        }
        // Remove from the LPPname table.
        if (ldb_lppname_tab && ld->lppName()) {
            lppname_t *e = ldb_lppname_tab->remove(ld->lppName());
            ldb_lppname_tab = ldb_lppname_tab->check_rehash();
            keep(e);
        }
        ld->setIndex(mode, -1);

        CD()->ifLayerTableChange(mode);
    }
    return (ld);
}


// Rename the layer keyed by oldname to newname.  oldname and newname
// can be bare names, or have the form "layername:purposename".  The
// oldname can be anything that resolves a layer.  The newname, if no
// purpose field, is taken as an OA layer name (and drawing purpose). 
// The layer and purpose of newname will be created if necessary.  The
// renamed layer will have any LPP name removed.
//
bool
cCDldb::renameLayer(const char *oldname, const char *newname)
{
    if (!oldname || !newname) {
        Errs()->add_error("renameLayer: null layer name encountered.");
        return (false);
    }
    if (findLayer(newname)) {
        Errs()->add_error("The name %s is already in use.", newname);
        return (false);
    }
    CDl *ld = findLayer(oldname);
    if (!ld) {
        Errs()->add_error("The name %s is not resolved.", oldname);
        return (false);
    }
    if (ld == CellLayer()) {
        Errs()->add_error("Can't rename the cell container layer.");
        return (false);
    }

    char *lname, *pname;
    splitlp(newname, &lname, &pname);
    unsigned int lnum = getOAlayerNum(lname);
    if (lnum == CDL_NO_LAYER) {
        lnum = newLayerNum();
        saveOAlayer(lname, lnum);
    }

    bool unknown;
    unsigned int pnum = getOApurposeNum(pname, &unknown);
    if (pnum == CDL_PRP_DRAWING_NUM && unknown) {
        pnum = newPurposeNum();
        saveOApurpose(pname, pnum);
    }

    // The ld will be converted to the new LPP lnum/pnum, and any LPP
    // name is removed.

    lpp_t *ep = ldb_lpp_tab->remove(ld->oaLayerNum(), ld->oaPurposeNum());
    if (!ep) {
        // "can't happen"
        delete [] lname;
        delete [] pname;
        Errs()->add_error("Internal error: LPP not in table.");
        return (false);
    }
    if (ld->lppName()) {
        keep(ldb_lppname_tab->remove(ld->lppName()));
        ld->setLPPname(0);
    }
    ld->setOaLpp(lnum, pnum);
    ld->setIdName();
    ep->init(ld, ld->oaLayerNum(), ld->oaPurposeNum());
    ldb_lpp_tab->link(ep);
    delete [] lname;
    delete [] pname;
    return (true);
}


// Insert lnew into the layer table at position ix.  Layers at ix and
// above are pushed up.  If ix is negative, append to existing layer
// list.  Fail if there is a name clash.
//
bool
cCDldb::insertLayer(CDl *lnew, DisplayMode mode, int ix)
{
    if (!lnew)
        return (true);

    if (lnew->index(mode) >= 0) {
        Errs()->add_error("Layer has non-negative index, already in table?");
        return (false);
    }
    if (!getOAlayerName(lnew->oaLayerNum())) {
        Errs()->add_error("Layer number not resolved to name.");
        return (false);
    }
    CDl *ldtmp = findLayer(lnew->oaLayerNum(), lnew->oaPurposeNum());
    if (ldtmp && ldtmp != lnew) {
        Errs()->add_error("Layer and purpose numbers not unique.");
        return (false);
    }

    CDlary *ary = mode == Physical ? &ldb_phys : &ldb_elec;

    // Insert the layer into its array in the proper location.  Expand
    // the array if necessary.  Set the index number of the layer.
    //
    ary->insert(lnew, ix);

    // The layer may already be in the hash tables if it was previously
    // invalid.

    // Add to the LPP table.
    //
    if (!ldb_lpp_tab)
        ldb_lpp_tab = new xytable_t<lpp_t>;
    if (!ldb_lpp_tab->find(lnew->oaLayerNum(), lnew->oaPurposeNum())) {
        lpp_t *e = (lpp_t*)new_elt();
        e->init(lnew, lnew->oaLayerNum(), lnew->oaPurposeNum());
        ldb_lpp_tab->link(e, false);
        ldb_lpp_tab = ldb_lpp_tab->check_rehash();
    }

    // Add to LPPname table if the layer has an LPPname.
    //
    if (lnew->lppName()) {
        if (!ldb_lppname_tab || !ldb_lppname_tab->find(lnew->lppName())) {
            if (!setLPPname(lnew, lnew->lppName())) {
                // The LPPname must clash with another layer, null it out.
                lnew->setLPPname(0);
            }
        }
    }

    CD()->ifLayerTableChange(mode);
    return (true);
}


// Save a layer under the given lnum/pnum.  This provides access to an
// existing layer from an alternate LPP, and is used in the OpenAccess
// plug-in to map reserved Virtuoso layers for schematics into the
// similar Xic layers.
//
bool
cCDldb::saveMapping(CDl *ld, unsigned int lnum, unsigned int pnum)
{
    if (!ld)
        return (true);
    CDl *ltmp = findLayer(lnum, pnum);
    if (ltmp)
        return (ltmp == ld);
    if (!ldb_lpp_tab)
        return (false);

    lpp_t *e = (lpp_t*)new_elt();
    e->init(ld, lnum, pnum);
    ldb_lpp_tab->link(e, false);
    ldb_lpp_tab = ldb_lpp_tab->check_rehash();
    return (true);
}


// Return a layer with matching name.
//
// The format for the name is "layername[:purposename]", where if
// purposename is omitted, "drawing" is understood.
//
CDl *
cCDldb::findLayer(const char *name)
{
    // First check the LPPname table, note that this can override normal
    // names.
    if (ldb_lppname_tab) {
        lppname_t *e = ldb_lppname_tab->find(name);
        if (e)
            return (e->ldesc());
    }
    if (ldb_lpp_tab) {
        unsigned int lnum, pnum;
        if (resolveName(name, &lnum, &pnum)) {
            lpp_t *e = ldb_lpp_tab->find(lnum, pnum);
            if (e)
                return (e->ldesc());
        }
    }
    if (ldb_lppalias_tab) {
        lppalias_t *t = ldb_lppalias_tab->find(name);
        if (t)
            return (findLayer(t->layer_name()));
    }
    return (0);
}


CDl *
cCDldb::findLayer(const char *name, DisplayMode mode)
{
    CDl *ld = findLayer(name);
    if (ld) {
        // If layer already exists for this mode, return it.
        if (ld->index(mode) >= 0)
            return (ld);

        // If the layer is invalid, return it.
        if (ld->isInvalid())
            return (ld);

        // Otherwise add it to the end of the layer table and return
        // it.
        if (insertLayer(ld, mode, layersUsed(mode)))
            return (ld);
    }
    return (0);
}


// Find a layer by its Layer/Purpose pair.
//
CDl *
cCDldb::findLayer(unsigned int lnum, unsigned int pnum)
{
    if (ldb_lpp_tab) {
        lpp_t *e = ldb_lpp_tab->find(lnum, pnum);
        if (e)
            return (e->ldesc());
    }
    return (0);
}


CDl *
cCDldb::findLayer(unsigned int lnum, unsigned int pnum, DisplayMode mode)
{
    CDl *ld = findLayer(lnum, pnum);
    if (ld) {
        // If layer already exists for this mode, return it.
        if (ld->index(mode) >= 0)
            return (ld);

        // If the layer is invalid, return it.
        if (ld->isInvalid())
            return (ld);

        // Otherwise add it to the end of the layer table and return
        // it.
        if (insertLayer(ld, mode, layersUsed(mode)))
            return (ld);
    }
    return (0);
}


// Return a matching layer, even if it is in an unused or invalid
// list.
//
CDl *
cCDldb::findAnyLayer(unsigned int lnum, unsigned int pnum)
{
    CDl *ld = findLayer(lnum, pnum);
    if (ld)
        return (ld);
    if (ldb_lpp_tab) {
        // This will return lnvalid layers too.
        lpp_t *e = ldb_lpp_tab->find(lnum, pnum);
        if (e)
            return (e->ldesc());
    }
    for (CDll *ll = ldb_unused_phys; ll; ll = ll->next) {
        if (ll->ldesc->oaLayerNum() == lnum &&
                ll->ldesc->oaPurposeNum() == pnum)
            return (ll->ldesc);
    }
    for (CDll *ll = ldb_unused_elec; ll; ll = ll->next) {
        if (ll->ldesc->oaLayerNum() == lnum &&
                ll->ldesc->oaPurposeNum() == pnum)
            return (ll->ldesc);
    }
    return (0);
}


// Extract a layer by name from the unused list for the given mode, and
// return it.
//
// The format for the name is "layername[:purposename]", where if
// purposename is omitted, "drawing" is understood.
//
CDl *
cCDldb::getUnusedLayer(const char *name, DisplayMode mode)
{
    unsigned int lnum, pnum;
    if (!resolveName(name, &lnum, &pnum))
        return (0);
    return (getUnusedLayer(lnum, pnum, mode));
}


// Extract by layer number/purpose number.
//
CDl *
cCDldb::getUnusedLayer(unsigned int lnum, unsigned int pnum, DisplayMode mode)
{
    CDll *l0 = (mode == Physical ? ldb_unused_phys : ldb_unused_elec);
    CDll *lp = 0;
    for (CDll *ll = l0; ll; ll = ll->next) {
        if (ll->ldesc->oaLayerNum() == lnum &&
                ll->ldesc->oaPurposeNum() == pnum) {
            if (!lp) {
                if (mode == Physical)
                    ldb_unused_phys = ll->next;
                else
                    ldb_unused_elec = ll->next;
            }
            else
                lp->next = ll->next;
            CDl *ld = ll->ldesc;
            CD()->ifUnusedLayerListChange(
                (mode == Physical ? ldb_unused_phys : ldb_unused_elec), mode);
            delete ll;
            return (ld);
        }
        lp = ll;
    }
    return (0);
}


// Add the layer to the front of the unused list for mode.
//
void
cCDldb::pushUnusedLayer(CDl *ldesc, DisplayMode mode)
{
    if (!ldesc)
        return;
    if (mode == Physical) {
        ldb_unused_phys = new CDll(ldesc, ldb_unused_phys);
        CD()->ifUnusedLayerListChange(ldb_unused_phys, Physical);
    }
    else {
        ldb_unused_elec = new CDll(ldesc, ldb_unused_elec);
        CD()->ifUnusedLayerListChange(ldb_unused_elec, Electrical);
    }
}


// Return the layer at the front of the unused list for mode, removing
// it from the list.
//
CDl *
cCDldb::popUnusedLayer(DisplayMode mode)
{
    CDl *ldesc;
    if (mode == Physical) {
        CDll *ll = ldb_unused_phys;
        if (!ll)
            return (0);
        ldb_unused_phys = ll->next;
        ldesc = ll->ldesc;
        delete ll;
        CD()->ifUnusedLayerListChange(ldb_unused_phys, Physical);
    }
    else {
        CDll *ll = ldb_unused_elec;
        if (!ll)
            return (0);
        ldb_unused_elec = ll->next;
        ldesc = ll->ldesc;
        delete ll;
        CD()->ifUnusedLayerListChange(ldb_unused_elec, Electrical);
    }
    return (ldesc);
}


namespace {
    inline bool
    ltcmp(const CDl *l1, const CDl *l2)
    {
        return (strcmp(l1->name(), l2->name()) < 0);
    }
}


// Alphabetize the order, not sure why this would be needed.
//
void
cCDldb::sort(DisplayMode mode)
{
    CDlary *ary = (mode == Physical ? &ldb_phys : &ldb_elec);
    if (ary->numused() > 2) {
        CDl *lx = ary->layer(0);
        std::sort(ary->layers() + 1, ary->layers() + ary->numused(), ltcmp);
        for (int i = 1; i < ary->numused(); i++)
            ary->layer(i)->setIndex(mode, i);

        if (lx)
            CD()->ifLayerTableChange(mode);
    }
}


namespace {
    // Return true if lname/pname are prefixes of the layer
    // name/purpose.
    //
    bool match(const CDl *ld, const char *lname, const char *pname)
    {
        char *tname = lstring::copy(ld->name());
        char *prp = strchr(tname, ':');
        if (prp)
            *prp++ = 0;
        if (!lname || !*lname) {
            if (!pname || !*pname) {
                delete [] tname;
                return (false);
            }
            if (prp && lstring::ciprefix(pname, prp)) {
                delete [] tname;
                return (true);
            }
            delete [] tname;
            return (false);
        }
        if (lstring::ciprefix(lname, tname)) {
            if (!pname || (pname && prp && lstring::ciprefix(pname, prp))) {
                delete [] tname;
                return (true);
            }
        }
        delete [] tname;
        return (false);
    }


    // Return true if name is a prefix of the LPP name.
    //
    bool match_lpp(const CDl *ld, const char *name)
    {
        if (ld->lppName() && lstring::ciprefix(name, ld->lppName()))
            return (true);
        return (false);
    }
}


CDl *
cCDldb::search(DisplayMode mode, int start, const char *str)
{
    if (!str)
        return (0);
    char *lname, *pname;
    splitlp(str, &lname, &pname);

    // First, try to match layer names.
    CDlary *ary = (mode == Physical ? &ldb_phys : &ldb_elec);
    for (int i = start; ; i++) {
        CDl *ld = ary->layer(i);
        if (!ld)
            break;
        if (match(ld, lname, pname)) {
            delete [] lname;
            delete [] pname;
            return (ld);
        }
    }
    if (!pname) {
        // If no matches and no purpose given, try to match the LPP
        // names.

        for (int i = start; ; i++) {
            CDl *ld = ary->layer(i);
            if (!ld)
                break;
            if (match_lpp(ld, lname)) {
                delete [] lname;
                return (ld);
            }
        }
    }
    delete [] lname;
    delete [] pname;
    return (0);
}


//
// These functions hash layer and purpose names and integers, for
// OpenAccess and Cadence compatibility.  These names are
// case-insensitive.
//

unsigned int
cCDldb::getOAlayerNum(const char *lname)
{
    if (!ldb_oalname_tab || !lname || !*lname)
        return (CDL_NO_LAYER);
    loa_t *e = ldb_oalname_tab->find(lname);
    if (!e)
        return (CDL_NO_LAYER);
    return (e->num());
}


const char*
cCDldb::getOAlayerName(unsigned int lnum)
{
    if (!ldb_oalnum_tab)
        return (0);
    if (lnum == CDL_NO_LAYER)
        return (0);
    loa_t *e = ldb_oalnum_tab->find(lnum);
    if (!e)
        return (0);
    return (e->name());
}


bool
cCDldb::saveOAlayer(const char *lname, unsigned int lnum, const char *abbrev)
{
    if (!lname || !*lname)
        return (false);
    if (lnum == CDL_NO_LAYER)
        return (false);
    if (!ldb_oalname_tab)
        ldb_oalname_tab = new ctable_t<loa_t>;
    if (!ldb_oalnum_tab)
        ldb_oalnum_tab = new itable_t<loa_t>;
    if (ldb_oalname_tab->find(lname))
        return (false);

    // OK if the number is already used, the present name will be an
    // alias.
    bool have_num = ldb_oalnum_tab->find(lnum);

    char *name = lstring::copy(lname);
    loa_t *e = (loa_t*)new_elt();
    e->init(name, lnum);
    ldb_oalname_tab->link(e);
    if (abbrev && *abbrev && strcasecmp(abbrev, name) &&
            !ldb_oalname_tab->find(abbrev)) {
        // The layer number will be resolved under this name, too.
        e = (loa_t*)new_elt();
        e->init(lstring::copy(abbrev), lnum);
        ldb_oalname_tab->link(e);
    }
    ldb_oalname_tab = ldb_oalname_tab->check_rehash();
    if (!have_num) {
        e = (loa_t*)new_elt();
        e->init(name, lnum);
        ldb_oalnum_tab->link(e);
        ldb_oalnum_tab = ldb_oalnum_tab->check_rehash();
    }
    return (true);
}


stringnumlist *
cCDldb::listOAlayerTab()
{
    stringnumlist *s0 = 0;
    tgen_t<loa_t> gen(ldb_oalname_tab);
    loa_t *e;
    while ((e = gen.next()) != 0) {
        if (e->num() >= PRV_LAYER_NUM_BASE &&
                e->num() < PRV_LAYER_NUM_BASE + ldb_num_private_lnames) {
            // Layer is private, assigned in init().
            continue;
        }
        if (!e->name()) {
            // Something bogus, can't happen.
            continue;
        }
        if (*e->name() == '?') {
            // Internal or temporary layer, skip.
            continue;
        }
        s0 = new stringnumlist(lstring::copy(e->name()), e->num(), s0);
    }
    stringnumlist::sort_by_num(s0);

    // When there are aliases, make sure that the "real" name is
    // listed first.
    stringnumlist *sp = 0;
    for (stringnumlist *s = s0; s; s = s->next) {
        if ((!sp || sp->num != s->num) && s->next && s->next->num == s->num) {
            e = ldb_oalnum_tab->find(s->num);
            stringnumlist *t = s->next;
            for ( ; t && t->num == s->num; t = t->next) {
                if (!strcmp(e->tab_name(), t->string)) {
                    char *c = s->string;
                    s->string = t->string;
                    t->string = c;
                    break;
                }
            }
        }
        sp = s;
    }
    return (s0);
}


void
cCDldb::clearOAlayerTab()
{
    tgen_t<loa_t> gen(ldb_oalname_tab);
    loa_t *e;
    while ((e = gen.next()) != 0) {
        ldb_oalname_tab->unlink(e);
        delete [] e->name();
        keep(e);
    }
    delete ldb_oalname_tab;
    ldb_oalname_tab = 0;

    gen = tgen_t<loa_t>(ldb_oalnum_tab);
    while ((e = gen.next()) != 0) {
        ldb_oalnum_tab->unlink(e);
        keep(e);
    }
    delete ldb_oalnum_tab;
    ldb_oalnum_tab = 0;
}


// Find the purpose number.  If the purpose name is not found, return
// the default purpose number, but also set the unknown flag if the
// caller passes an address.
//
unsigned int
cCDldb::getOApurposeNum(const char *pname, bool *unknown)
{
    if (unknown)
        *unknown = false;
    if (!pname || !*pname || !strcasecmp(pname, CDL_PRP_DRAWING))
        return (CDL_PRP_DRAWING_NUM);
    if (!ldb_oapname_tab) {
        if (unknown)
            *unknown = true;
        return (CDL_PRP_DRAWING_NUM);
    }
    loa_t *e = ldb_oapname_tab->find(pname);
    if (!e) {
        if (unknown)
            *unknown = true;
        return (CDL_PRP_DRAWING_NUM);
    }
    return (e->num());
}


const char*
cCDldb::getOApurposeName(unsigned int pnum)
{
    if (!ldb_oapnum_tab)
        return (0);
    if (pnum == CDL_PRP_DRAWING_NUM)
        return (0);
    loa_t *e = ldb_oapnum_tab->find(pnum);
    if (!e)
        return (0);
    return (e->name());
}


bool
cCDldb::saveOApurpose(const char *pname, unsigned int pnum, const char *abbrev)
{
    if (pnum == CDL_PRP_DRAWING_NUM)
        return (true);
    if (!pname || !*pname)
        return (false);
    if (!strcasecmp(pname, CDL_PRP_DRAWING))
        return (true);
    if (!ldb_oapname_tab)
        ldb_oapname_tab = new ctable_t<loa_t>;
    if (!ldb_oapnum_tab)
        ldb_oapnum_tab = new itable_t<loa_t>;
    if (ldb_oapname_tab->find(pname))
        return (false);

    // OK if the number is already used, the present name will be an
    // alias.
    bool have_num = ldb_oapnum_tab->find(pnum);

    char *name = lstring::copy(pname);
    loa_t *e = (loa_t*)new_elt();
    e->init(name, pnum);
    ldb_oapname_tab->link(e);
    if (abbrev && *abbrev && strcasecmp(abbrev, name) &&
            !ldb_oapname_tab->find(abbrev)) {
        // The purpose number will be resolved under this name, too.
        e = (loa_t*)new_elt();
        e->init(lstring::copy(abbrev), pnum);
        ldb_oapname_tab->link(e);
    }
    ldb_oapname_tab = ldb_oapname_tab->check_rehash();
    if (!have_num) {
        e = (loa_t*)new_elt();
        e->init(name, pnum);
        ldb_oapnum_tab->link(e);
        ldb_oapnum_tab = ldb_oapnum_tab->check_rehash();
    }
    return (true);
}


stringnumlist *
cCDldb::listOApurposeTab()
{
    stringnumlist *s0 = 0;
    tgen_t<loa_t> gen(ldb_oapname_tab);
    loa_t *e;
    while ((e = gen.next()) != 0)
        s0 = new stringnumlist(lstring::copy(e->name()), e->num(), s0);
    stringnumlist::sort_by_num(s0);

    // When there are aliases, make sure that the "real" name is
    // listed first.
    stringnumlist *sp = 0;
    for (stringnumlist *s = s0; s; s = s->next) {
        if ((!sp || sp->num != s->num) && s->next && s->next->num == s->num) {
            e = ldb_oapnum_tab->find(s->num);
            stringnumlist *t = s->next;
            for ( ; t && t->num == s->num; t = t->next) {
                if (!strcmp(e->tab_name(), t->string)) {
                    char *c = s->string;
                    s->string = t->string;
                    t->string = c;
                    break;
                }
            }
        }
        sp = s;
    }
    return (s0);
}


void
cCDldb::clearOApurposeTab()
{
    tgen_t<loa_t> gen(ldb_oapname_tab);
    loa_t *e;
    while ((e = gen.next()) != 0) {
        ldb_oapname_tab->unlink(e);
        delete [] e->name();
        keep(e);
    }
    delete ldb_oapname_tab;
    ldb_oapname_tab = 0;

    gen = tgen_t<loa_t>(ldb_oapnum_tab);
    while ((e = gen.next()) != 0) {
        ldb_oapnum_tab->unlink(e);
        keep(e);
    }
    delete ldb_oapnum_tab;
    ldb_oapnum_tab = 0;
}


//
// The following functions implement a table for derived layers.
//

// Add a derived layer specification.  The lname is the layer name,
// which can be in LPP form.  The lnum is an ordering number, if 0 or
// negative the number will be assigned in sequence.  The spec is a
// layer expression which sets the layer contents.  This can be null
// here, but must be set before use of the layer.
//
// Duplicate names have entries updated here.  The layer name should
// not duplicate a normal layer name, this is not checked here.  Such
// a derived layer would be inaccessible.
//
// The mode specifies how the layer geometry is formatted, using
// the CLmode flags for createLsyer.
//
// The return is a layer descriptor for the layer, with layer type
// CDLderived.
//
CDl *
cCDldb::addDerivedLayer(const char *lname, int lnum, int mode,
    const char *expr)
{
    if (!lname || !*lname)
        return (0);

    if (!ldb_drvlyr_tab)
        ldb_drvlyr_tab = new ctable_t<drvlyr_t>;

    drvlyr_t *d = ldb_drvlyr_tab->find(lname);
    if (d) {
        d->ldesc()->setDrvExpr(expr);
        if (lnum > 0)
            d->ldesc()->setDrvIndex(lnum);
        if (mode >= 0 && mode <= 3)
            d->ldesc()->setDrvMode(mode);
        return (d->ldesc());
    }
    if (ldb_drvlyr_rmtab && (d = ldb_drvlyr_rmtab->remove(lname)) != 0) {
        d->ldesc()->setDrvExpr(expr);
        if (lnum > 0)
            d->ldesc()->setDrvIndex(lnum);
        if (mode >= 0 && mode <= 3)
            d->ldesc()->setDrvMode(mode);
        ldb_drvlyr_tab->link(d);
        ldb_drvlyr_tab = ldb_drvlyr_tab->check_rehash();
        return (d->ldesc());
    }

    CDl *ld = new CDl(CDLderived);
    ld->initName(lname);
    ld->setDrvExpr(expr);
    if (lnum <= 0)
        lnum = ++ldb_drv_index;
    ld->setDrvIndex(lnum);
    if (mode < 0 || mode > 3)
        mode = 0;
    ld->setDrvMode(mode);
    d = new drvlyr_t(ld);
    ldb_drvlyr_tab->link(d);
    ldb_drvlyr_tab = ldb_drvlyr_tab->check_rehash();
    return (ld);
}


// Remove a derived layer from the main table.  The object is placed
// in a separate table, and can be restored with addDerivedLayer.
//
CDl *
cCDldb::remDerivedLayer(const char *lname)
{
    if (!lname || !*lname)
        return (0);
    if (!ldb_drvlyr_tab)
        return (0);
    drvlyr_t *d = ldb_drvlyr_tab->remove(lname);
    if (d) {
        if (!ldb_drvlyr_rmtab)
            ldb_drvlyr_rmtab = new ctable_t<drvlyr_t>;
        ldb_drvlyr_rmtab->link(d);
        ldb_drvlyr_rmtab = ldb_drvlyr_rmtab->check_rehash();
        return (d->ldesc());
    }
    return (0);
}


// Find the descriptor of a derived layer by name.
//
CDl *
cCDldb::findDerivedLayer(const char *lname)
{
    if (!ldb_drvlyr_tab || !lname)
        return (0);
    drvlyr_t *d = ldb_drvlyr_tab->find(lname);
    return (d ? d->ldesc() : 0);
}


namespace {
    // Sort ascending in layer number, alpha for equal numbers.
    bool dl_cmp(const CDl *l1, const CDl *l2)
    {
        if (l1->drvIndex() == l2->drvIndex())
            return (strcmp(l1->name(), l2->name()) < 0);
        return (l1->drvIndex() < l2->drvIndex());
    }
}


// Return a sorted zero-terminated array of all derived layer
// description pointers, in order of the index numbers.  The array
// (but NOT the pointers!) should be freed when done.
//
CDl **
cCDldb::listDerivedLayers()
{
    if (!ldb_drvlyr_tab)
        return (0);
    CDl **ary = new CDl*[ldb_drvlyr_tab->allocated() + 1];
    tgen_t<drvlyr_t> gen(ldb_drvlyr_tab);
    int cnt = 0;
    drvlyr_t *d;
    while ((d = gen.next()) != 0)
        ary[cnt++] = d->ldesc();
    ary[cnt] = 0;

    std::sort(ary, ary+cnt, dl_cmp);
    return (ary);
}
// End of cCDldb functions.

