
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

#include "fio.h"
#include "fio_gds_layer.h"
#include "fio_layermap.h"
#include "cd_lgen.h"
#include <ctype.h>


// Return the database layers which map to the layer/datatype given,
// or the first match found if multi-mapping is disabled.
//
CDll *
cFIO::GetGdsInputLayers(unsigned int lnum, unsigned int dtype,
    DisplayMode mode)
{
    if (lnum >= GDS_MAX_LAYERS || dtype >= GDS_MAX_DTYPES)
        return (0);
    CDll *l0 = 0;
    CDll *le = 0;
    CDl *ld;
    CDlgen gen(mode);
    while ((ld = gen.next()) != 0) {
        if (ld->isStrmIn(lnum, dtype)) {
            if (!l0) {
                l0 = new CDll(ld, 0);
                if (!IsMultiLayerMapOk())
                    return (l0);
                le = l0;
            }
            else {
                le->next = new CDll(ld, 0);
                le = le->next;
            }
        }
    }
    return (l0);
}


// Return the name of the first input layer found which matches the
// given layer/datatype (no wildcarding allowed).
//
const char *
cFIO::GetGdsInputLayer(unsigned int lnum, unsigned int dtype, DisplayMode mode)
{
    if (lnum >= GDS_MAX_LAYERS || dtype >= GDS_MAX_DTYPES)
        return (0);
    CDlgen gen(mode);
    CDl *ld;
    while ((ld = gen.next()) != 0) {
        if (ld->isStrmIn(lnum, dtype))
            return (ld->name());
    }
    return (0);
}


// Return a layer that will be mapped to when reading lnum/dtype in
// GDSII/OASIS input.  It is assumed that there is no input mapping
// specified (GetGDSinLayer returned null for this layer/dtype).  If
// create is true, a new layer will be created if no existing mapping
// exists.  The hex code is returned in hexstr, if not nil.  This will
// be LLDD for layer/dtype < 256, or LLLLDDDD otherwise.  The new
// layer will use this name.  The return value will be 0 and the error
// return will be nonzero if an error occurs.
//
CDl *
cFIO::MapGdsLayer(unsigned int lnum, unsigned int dtype, DisplayMode mode,
    char *hexstr, bool create, bool *error)
{
    *error = false;

    if (lnum >= GDS_MAX_LAYERS || dtype >= GDS_MAX_DTYPES)
        return (0);

    char hexname[32];
    strmdata::hexpr(hexname, lnum, IsNoMapDatatypes() ? -1 : dtype);
    if (hexstr)
        strcpy(hexstr, hexname);

    // First look for a layer with matching stream output with no
    // input specified, and map to it if found.
    //
    CDl *ld;
    CDlgen gen(mode);
    while ((ld = gen.next()) != 0) {
        if (ld->strmIn())
            continue;
        strm_odata *so = ld->strmOut();
        while (so) {
            if (so->layer() == lnum &&
                    (IsNoMapDatatypes() || so->dtype() == dtype))
                break;
            so = so->next();
        }
        if (so) {
            // Found one, map to it.
            if (!ld->setStrmIn(lnum, IsNoMapDatatypes() ? -1 : dtype)) {
                *error = true;
                return (0);
            }
            return (ld);
        }
    }

    // Next, look for a layer with a matching numerical name or
    // LPPname, and map to that layer if unmapped.
    //
    gen = CDlgen(mode);
    while ((ld = gen.next()) != 0) {
        if (ld->strmIn())
            continue;
        if (!strcasecmp(ld->name(), hexname)) {
            // Found name, map to it.
            if (!ld->setStrmIn(lnum, IsNoMapDatatypes() ? -1 : dtype)) {
                *error = true;
                return (0);
            }
            return (ld);
        }
        if (ld->lppName() && !strcasecmp(ld->lppName(), hexname)) {
            // Found LPPname, map to it.
            if (!ld->setStrmIn(lnum, IsNoMapDatatypes() ? -1 : dtype)) {
                *error = true;
                return (0);
            }
            return (ld);
        }
    }

    // No mapping candidate layer was found, create a new layer if
    // the flag is set.
    //
    if (create) {
        if (IsNoCreateLayer()) {
            Errs()->add_error(
                "Unresolved layer, not allowed to create new layer %s.",
                hexname);
            *error = true;
            return (0);
        }

        if (mode == Electrical && lnum >= 200 && lnum <= 255 &&
                dtype <= 255) {
            // In electrical mode, if the layer is a Virtuoso reserved
            // layer and the datatype is <= 255, use the lnum/dtype as
            // the OA layer and purpose numbers.  We use this encodong
            // when writing electrical data.

            // The Virtuoso reserved "drawing" purpose is 252.
            unsigned int dt = dtype;
            if (dt == 252)
                dt = -1;

            CDl *lnew = CDldb()->newLayer(lnum, dt, mode);
            if (lnew) {
                if (!lnew->setStrmIn(lnum, dt)) {
                    *error = true;
                    return (0);
                }
                lnew->addStrmOut(lnum, dt);
                return (lnew);
            }
            // Above fails if lnum/pnum do not exist in the OA layer
            // and purpose tables.  Not a fatal error.
        }

        char buf[256];
        if (!CDldb()->findLayer(hexname, mode)) {
            CDl *lnew = CDldb()->newLayer(hexname, mode);
            if (!lnew) {
                Errs()->add_error("Could not create new layer %s.", hexname);
                *error = true;
                return (0);
            }
            if (!lnew->setStrmIn(lnum, IsNoMapDatatypes() ? -1 : dtype)) {
                *error = true;
                return (0);
            }
            lnew->addStrmOut(lnum, dtype);
            snprintf(buf, 256, "created for layer=%u, datatype=%u",
                lnum, dtype);
            lnew->setDescription(buf);
            return (lnew);
        }
        // The hexname already names a layer, which must have the wrong
        // GDSII mapping so we can't use it.  Create a new layer with an
        // internal name.

        unsigned int oalnum;
        char *lname = CDldb()->newLayerName(&oalnum);
        CDldb()->saveOAlayer(lname, oalnum);
        CDl *lnew = CDldb()->newLayer(oalnum, CDL_PRP_DRAWING_NUM, mode);
        if (!lnew) {
            Errs()->add_error("Could not create new layer %s (for %s).",
                lname, hexname);
            delete [] lname;
            *error = true;
            return (0);
        }
        delete [] lname;
        if (!lnew->setStrmIn(lnum, IsNoMapDatatypes() ? -1 : dtype)) {
            *error = true;
            return (0);
        }
        lnew->addStrmOut(lnum, dtype);
        snprintf(buf, 256, "created for layer=%u, datatype=%u", lnum, dtype);
        lnew->setDescription(buf);
        return (lnew);
    }
    return (0);
}
// End of cFIO functions.


// Set the variables according to the state of 'this'.
//
void
FIOlayerFilt::push()
{
    if ((lf_use_only || lf_skip) && lf_layer_list && *lf_layer_list) {
        FIO()->SetLayerList(lf_layer_list);
        FIO()->SetUseLayerList(lf_skip ? ULLskipList : ULLonlyList);
    }
    else {
        FIO()->SetLayerList(0);
        FIO()->SetUseLayerList(ULLnoList);
    }
    if (lf_aliases && *lf_aliases) {
        FIO()->SetUseLayerAlias(true);
        FIO()->SetLayerAlias(lf_aliases);
    }
    else {
        FIO()->SetUseLayerAlias(false);
        FIO()->SetLayerAlias(0);
    }
}


// Set 'this' from the current state of the variables.
//
void
FIOlayerFilt::set()
{
    delete [] lf_layer_list;
    delete [] lf_aliases;
    lf_layer_list = 0;
    lf_aliases = 0;
    lf_use_only = false;
    lf_skip = false;

    ULLtype ull = FIO()->UseLayerList();
    if (ull != ULLnoList) {
        if (ull == ULLskipList)
            lf_skip = true;
        else
            lf_use_only = true;
        lf_layer_list = lstring::copy(FIO()->LayerList());
    }
    if (FIO()->IsUseLayerAlias()) {
        const char *la = FIO()->LayerAlias();
        if (la && *la)
            lf_aliases = lstring::copy(la);
    }
}


//-----------------------------------------------------------------------------
// The layer alias table.
//
// When reading a data file, layers encountered are aliased according
// to entries in this table.  This occurs after the LayerList is
// processed.

namespace {
    // Copy name, hex encodong "layer,datatype" tokens.
    //
    inline char *
    todb(const char *name)
    {
        char *s = strmdata::dec_to_hex(name);
        if (s)
            return (s);
        return (lstring::copy(name));
    }


    // Copy name, decoding hex tokens to "layer,datatype" form.
    //
    inline char *
    frdb(const char *dbname)
    {
        char *s = strmdata::hex_to_dec(dbname);
        if (s)
            return (s);
        return (lstring::copy(dbname));
    }


    // Return true if name is valid.  The is 1-4 chars alphanumeric
    // (CIF name) or 8 char hex for GDSII layer mapping.
    //
    bool
    is_name_ok(const char *lname)
    {
        int n = strlen(lname);
        if (n < 1)
            return (false);
        for (int i = 0; i < n; i++) {
            if (!isalnum(lname[i]))
                return (false);
        }
        if (n > 4) {
            if (n != 8)
                return (false);
            for (int i = 0; i < n; i++) {
                if (!isxdigit(lname[i]))
                    return (false);
            }
        }
        return (true);
    }
}


// Add an alias to the table.
//
bool
FIOlayerAliasTab::addAlias(const char *name, const char *new_name)
{
    if (!name || !new_name ||
            !is_name_ok(name) || !is_name_ok(new_name))
        return (false);
    char *o = todb(name);
    char *n = todb(new_name);
    add(o, n);
    delete [] o;
    delete [] n;
    return (true);
}


// Remove any alias defined for name.
//
void
FIOlayerAliasTab::remove(const char *name)
{
    if (!name || !*name || !at_table)
        return;
    al_t *e = at_table->find(name);
    if (e)
        at_table->unlink(e);
}


// Clear the layer alias table.
//
void
FIOlayerAliasTab::clear()
{
    delete at_table;
    at_table = 0;
    at_eltab.clear();
    at_stringtab.clear();
}


// Parse a string containing oldlayer=newlayer pairs, and add each
// alias pair to the table.  The layers are simply extracted in pairs,
// with '=' taken as a delimiter.  Bad names and singletons are simply
// ignored.
//
void
FIOlayerAliasTab::parse(const char *string)
{
    while (string && *string) {
        char *oldl = CDl::get_layer_tok(&string);
        char *newl = CDl::get_layer_tok(&string);
        if (oldl && newl) {
            char *o = todb(oldl);
            char *n = todb(newl);
            if (is_name_ok(o) && is_name_ok(n))
                add(o, n);
            delete [] o;
            delete [] n;
        }
        delete [] oldl;
        delete [] newl;
    }
}


// Read a list of aliases from the file.
//
void
FIOlayerAliasTab::readFile(FILE *fp)
{
    char buf[256], *s;
    while ((s = fgets(buf, 256, fp)) != 0) {
        while (isspace(*s))
            s++;
        if (*s == '#' || !*s)
            continue;
        parse(s);
    }
}


// Return a string listing all name=alias pairs.
//
char *
FIOlayerAliasTab::toString(bool todec)
{
    if (!at_table)
        return (0);

    stringlist *s0 = 0;
    char buf[128];

    tgen_t<al_t> gen(at_table);
    al_t *e;
    while ((e = gen.next()) != 0) {
        if (todec) {
            char *t = frdb(e->tab_name());
            strcpy(buf, t);
            delete [] t;
        }
        else
            strcpy(buf, e->tab_name());
        strcat(buf, "=");
        if (todec) {
            char *t = frdb(e->tab_alias());
            strcat(buf, t);
            delete [] t;
        }
        else
            strcat(buf, e->tab_alias());
        s0 = new stringlist(lstring::copy(buf), s0);
    }
    stringlist::sort(s0);

    sLstr lstr;
    for (stringlist *s = s0; s; s = s->next) {
        if (lstr.string())
            lstr.add_c(' ');
        lstr.add(s->string);
    }
    stringlist::destroy(s0);

    return (lstr.string_trim());
}


// Dump a file containing the current alias list.
//
void
FIOlayerAliasTab::dumpFile(FILE *fp)
{
    if (!at_table)
        return;
    stringlist *s0 = 0;
    char buf[128];

    tgen_t<al_t> gen(at_table);
    al_t *e;
    while ((e = gen.next()) != 0) {
        sprintf(buf, "%s=%s", e->tab_name(), e->tab_alias());
        s0 = new stringlist(lstring::copy(buf), s0);
    }
    stringlist::sort(s0);

    for (stringlist *s = s0; s; s = s->next)
        fprintf(fp, "%s\n", s->string);
    stringlist::destroy(s0);
}

