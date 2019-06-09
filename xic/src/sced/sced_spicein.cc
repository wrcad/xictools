
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

#include "main.h"
#include "sced.h"
#include "sced_spicein.h"
#include "edit.h"
#include "undolist.h"
#include "ext.h"
#include "ext_extract.h"
#include "dsp_inlines.h"
#include "cd_terminal.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "fio.h"
#include "fio_library.h"
#include "promptline.h"
#include "errorlog.h"
#include "spnumber/spnumber.h"
#include "miscutil/filestat.h"


// Default names for terminals devices in library.
#define SCD_GND_NAME    "gnd"
#define SCD_TRM_NAME    "tbar"

// Parameter strings with more chars than this will be in "long text"
// form.
#define LT_THRESHOLD 22

//-----------------------------------------------------------------------------
// Functions for parsing a Spice file and creating a hierarchy.
//

// positioning offsets for device
#define spMARG (4*CDelecResolution)
#define spFWID (300*CDelecResolution)
#define spTX1  (7*CDelecResolution)
#define spTX2  (5*CDelecResolution)
#define spTY   (5*CDelecResolution)

namespace {
    const char *process_spice = "process SPICE";
    bool add_spicetext(CDs*, stringlist*);

    // Return a copy with leading and trailing junk stripped.
    //
    char *parmlist(const char *text)
    {
        if (!text)
            return (0);
        while (isspace(*text) || *text == ',' || *text == '(')
            text++;
        char *ctxt = lstring::copy(text);
        char *e = ctxt + strlen(ctxt) - 1;
        while (e >= ctxt) {
            if (isspace(*e) || *e == ')' || *e == ',' || *e == '=') {
                *e-- = 0;
                continue;
            }
            break;
        }
        return (ctxt);
    }
}

using namespace sced_spicein;


void
cSced::dumpDevKeys()
{
    sKeyDb db;
    db.dump(stdout);
}


#define MAX_NEST 20

// Read in the contents of the spice file.  New devices and
// subcircuits are placed in a rectangular array, with named terminals
// added to the device contact points to provide connectivity. 
// Existing devices have properties updated.
//
// The modeflag is a combination of the following:
//   EFS_ALLDEVS     Update all existing devices, otherwise
//                    update only those with user-supplied names.
//   EFS_CREATE      Create missing devices and subcircuits.
//   EFS_CLEAR       Clear each cell first, then create objects,
//                    implies EFS_CREATE.
//
void
cSced::extractFromSpice(CDs *sdesc, FILE *fp, int modeflag)
{
    if (!sdesc) {
        Log()->ErrorLog(process_spice,
            "extractFromSpice: null cell pointer (internal error).");
        return;
    }
    if (!sdesc->isElectrical()) {
        Log()->ErrorLog(process_spice,
            "extractFromSpice: cell not electrical (internal error).");
        return;
    }
    CDs *tsd = sdesc->owner();
    if (tsd)
        sdesc = tsd;

    if ((modeflag & EFS_CLEAR) || sdesc->isEmpty())
        modeflag |= EFS_CREATE;

    // Check if text file.
    {
        long pos = ftell(fp);
        char buf[64];
        int nrd = fread(buf, 1, 64, fp);
        for (int i = 0; i < nrd; i++) {
            if (!isascii(buf[i])) {
                Log()->ErrorLog(process_spice,
                    "Non-ascii characters in SPICE file, aborting read.");
                return;
            }
        }
        fseek(fp, pos, SEEK_SET);
    }

    // skip title
    int lastc = 0;
    char *line = cSpiceBuilder::readline(fp, &lastc);
    if (!line && lastc == EOF) {
        Log()->ErrorLog(process_spice, "No data in SPICE file, aborting.");
        return;
    }
    delete [] line;

    cSpiceBuilder pr;
    if ((modeflag & EFS_CREATE) && !pr.init_term_names())
        return;

    if (modeflag & EFS_CLEAR) {
        Ulist()->ListCheck("clear", sdesc, false);
        cSpiceBuilder::clear_cell(sdesc);
        while ((line = cSpiceBuilder::readline(fp, &lastc)) != 0) {
            if (*line == '.') {
                const char *tline = line + 1;
                char *tok = sp_gettok(&tline);
                for (char *s = tok; *s; s++) {
                    if (isupper(*s))
                        *s = tolower(*s);
                }
                if (!strcmp(tok, "subckt")) {
                    char *scname = sp_gettok(&tline);
                    CDs *sd = CDcdb()->findCell(scname, Electrical);
                    delete [] scname;
                    if (sd && sd != sdesc) {
                        Ulist()->ListChangeCell(sd);
                        cSpiceBuilder::clear_cell(sd);
                    }
                }
                delete [] tok;
            }
            delete [] line;
        }
        rewind(fp);
        lastc = 0;
        cSpiceBuilder::readline(fp, &lastc);
        Ulist()->CommitChanges();
        if (sdesc == CurCell(Electrical))
            assertSymbolic(false);
    }

    Ulist()->ListCheck("source", sdesc, false);
    int level = 0;
    sSubcLink *subc = 0;
    sNodeLink *lptr = 0;
    cSpiceBuilder::mksymtab(sdesc, &pr.sb_stab, (modeflag & EFS_ALLDEVS));

    // We try to deal with subcircuit nesting.
    SymTab *ary[MAX_NEST];
    memset(ary, 0, MAX_NEST*sizeof(SymTab*));

    stringlist *dotcards = 0;
    bool inblock = false;
    while ((line = cSpiceBuilder::readline(fp, &lastc)) != 0) {
        if (!*line) {
            delete [] line;
            continue;
        }
        if (*line == '.') {
            const char *tline = line + 1;
            char *tok = sp_gettok(&tline);
            for (char *s = tok; *s; s++) {
                if (isupper(*s))
                    *s = tolower(*s);
            }
            if (!strcmp(tok, "exec") || !strcmp(tok, "control") ||
                    !strcmp(tok, "verilog"))
                inblock = true;
            else if (inblock &&
                    (!strcmp(tok, "endc") || !strcmp(tok, "endv"))) {
                inblock = false;
                delete [] tok;
                delete [] line;
                continue;
            }
            if (!inblock) {
                if (!strcmp(tok, "subckt")) {
                    delete [] tok;
                    char *scname = sp_gettok(&tline);
                    if (level > 0) {
                        // Nested subcircuit, these aren't guaranteed
                        // to not have name collisions in a flat
                        // namespace (like cells) so change the name.

                        char *t = new char[strlen(scname) +
                            strlen(subc->name + 1)];
                        char *e = lstring::stpcpy(t, scname);
                        *e++ = '_';
                        strcpy(e, subc->name);
                        if (!ary[level])
                            ary[level] = new SymTab(true, false);
                        ary[level]->add(scname, t, false);
                        scname = t;
                    }

                    // Parameters are added to the nodes list, deal with
                    // this later.
                    while ((tok = sp_gettok(&tline, true)) != 0)
                        pr.add_node(tok);

                    subc = new sSubcLink(scname, pr.sb_nodes, level, subc);
                    pr.sb_nodes = 0;
                    delete [] line;
                    level++;
                    if (level > MAX_NEST) {
                        Log()->ErrorLog(process_spice,
                            "Nesting too deep, aborting.");
                        for (int i = MAX_NEST-1; i > 0; i--)
                            delete ary[i];
                        delete subc;
                        return;
                    }
                    continue;
                }
                if (!strcmp(tok, "model")) {
                    char *mname = sp_gettok(&tline);
                    char *mod = sp_gettok(&tline);
                    char *text = parmlist(tline);
                    if (level > 0)
                        subc->models = new sModLink(mname, mod, text,
                            subc->models);
                    else
                        pr.sb_models = new sModLink(mname, mod, text,
                            pr.sb_models);
                }
                else if (!strcmp(tok, "param") && level > 0) {
                    subc->dotparams = new stringlist(lstring::copy(line),
                        subc->dotparams);
                }
                else if (!strncmp(tok, "end", 3)) {
                    if (level) {
                        bool check = false;
                        for (int i = level; i > 0; i--) {
                            if (ary[i]) {
                                check = true;
                                break;
                            }
                        }
                        if (check) {
                            // Have to fix subcircuit calls.
                            for (sNodeLink *n = subc->lines; n; n = n->next) {
                                char *t = n->node;
                                while (isspace(*t))
                                    t++;
                                if (*t != 'x' && *t != 'X')
                                    continue;

                                // Have to change the subcircuit call
                                // to the alias, much bother since we
                                // don't know which token is the
                                // subcircuit name.

                                sLstr lstr;
                                char *tk = lstring::gettok(&t);
                                lstr.add(tk);
                                delete [] tk;
                                tk = lstring::gettok(&t);
                                lstr.add_c(' ');
                                lstr.add(tk);
                                delete [] tk;
                                char *r = 0;
                                while ((tk = lstring::gettok(&t)) != 0) {
                                    if (r) {
                                        lstr.add_c(' ');
                                        lstr.add(tk);
                                        delete [] tk;
                                        continue;
                                    }
                                    for (int i = level; i > 0; i--) {
                                        if (!ary[i])
                                            continue;
                                        r = (char*)SymTab::get(ary[i], tk);
                                        if (r == (char*)ST_NIL) {
                                            r = 0;
                                            continue;
                                        }
                                        break;
                                    }
                                    lstr.add_c(' ');
                                    lstr.add(r ? r : tk);
                                    delete [] tk;
                                }
                                delete [] n->node;
                                n->node = lstr.string_trim();
                            }
                        }

                        sSubcLink *sc = subc;
                        subc = subc->next;
                        sc->next = pr.sb_subckts;
                        pr.sb_subckts = sc;
                        delete ary[level];
                        ary[level] = 0;
                        level--;
                    }
                }
                else 
                    dotcards = new stringlist(lstring::copy(line), dotcards);
            }
            delete [] tok;
            continue;
        }
        if (!inblock) {
            if (subc) {
                if (subc->lines == 0)
                    subc->lines = subc->lptr =
                        new sNodeLink(lstring::copy(line), 0);
                else {
                    subc->lptr->next = new sNodeLink(lstring::copy(line), 0);
                    subc->lptr = subc->lptr->next;
                }
            }
            else {
                if (pr.sb_lines == 0)
                    pr.sb_lines = lptr = new sNodeLink(lstring::copy(line), 0);
                else {
                    lptr->next = new sNodeLink(lstring::copy(line), 0);
                    lptr = lptr->next;
                }
            }
        }
        delete [] line;
    }
    // done with fp

    if (level) {
        Log()->WarningLog(process_spice, ".ENDS card missing.");
        while (level >= 0) {
            delete ary[level];
            level--;
        }
    }

    sSubcLink *s;
    for (s = pr.sb_subckts; s; s = s->next)
        s->addcalls();

    if (pr.sb_subckts && pr.sb_subckts->next) {
        sSortSubc sort(pr.sb_subckts);
        pr.sb_subckts = sort.sorted();
    }
    // process subcircuits
    for (s = pr.sb_subckts; s; s = s->next)
        pr.make_subc(s->name, modeflag);

    // process main circuit
    Ulist()->ListChangeCell(sdesc);

    // dump the models found to a file
    pr.dump_models(Tstring(sdesc->cellname()));
    if (pr.modfile_name()) {
        // The file was created successfully (there were models).  Add
        // an include label for the file.

        char *tmp = new char[strlen(pr.modfile_name()) + 12];
        sprintf(tmp, ".include %s", pr.modfile_name());
        if (!dotcards)
            dotcards = new stringlist(tmp, 0);
        else {
            stringlist *sl = dotcards;
            while (sl->next)
                sl = sl->next;
            sl->next = new stringlist(tmp, 0);
        }
    }

    // add the dotcards as spicetext labels at top level
    if (dotcards) {
        add_spicetext(sdesc, dotcards);
        stringlist::destroy(dotcards);
    }

    const BBox *sBB = sdesc->BB();
    if (sBB->left == CDinfinity) {
        // empty or contains only empty subcells
        pr.sb_xpos = spMARG;
        pr.sb_ypos = -spMARG;
    }
    else {
        pr.sb_xpos = sBB->left + spMARG;
        pr.sb_ypos = sBB->bottom - spMARG;
    }

    bool trd = DSP()->NoRedisplay();
    DSP()->SetNoRedisplay(true);
    pr.sb_lineht = 0;
    sGlobNode::destroy(pr.sb_globs);
    pr.sb_globs = 0;
    for (sNodeLink *n = pr.sb_lines; n; n = n->next)
        pr.parseline(sdesc, n->node, (modeflag & EFS_CREATE));
    pr.process_muts(sdesc);
    pr.sub_glob(sdesc);
    DSP()->SetNoRedisplay(trd);

    // Make sure that the top-level has a name property.
    if (!sdesc->prpty(P_NAME))
        sdesc->prptyAdd(P_NAME, P_NAME_SUBC_STR);

    CDs *psdesc = CDcdb()->findCell(sdesc->cellname(), Physical);
    if (psdesc)
        psdesc->setAssociated(false);

    Ulist()->CommitChanges();
    if (sdesc->cellname() == DSP()->CurCellName()) {
        WindowDesc *wdesc;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wdesc = wgen.next()) != 0) {
            if (wdesc->IsSimilar(Electrical, DSP()->MainWdesc())) {
                wdesc->CenterFullView();
                wdesc->Redisplay(0);
            }
        }
    }
}


namespace {
    inline bool is_n(const char *name)
    {
        if (*name == 'n' || *name == 'N')
            return (true);
        if ((strchr(name, 'n') || strchr(name, 'N')) &&
                !strchr(name, 'p') && !strchr(name, 'P'))
            return (true);
        return (false);
    }


    inline bool is_p(const char *name)
    {
        if (*name == 'p' || *name == 'P')
            return (true);
        if ((strchr(name, 'p') || strchr(name, 'P')) &&
                !strchr(name, 'n') && !strchr(name, 'N'))
            return (true);
        return (false);
    }


    inline CDs *open_device(const char *name)
    {
        CDs *sd = CDcdb()->findCell(name, Electrical);
        if (!sd) {
            CDcbin cbin;
            if (FIO()->OpenNative(name, &cbin, 1.0, 0) == OIok)
                sd = cbin.elec();
        }
        return (sd);
    }


    // Add each string as a label on the SPTX layer, if not already
    // there.
    //
    bool add_spicetext(CDs *sdesc, stringlist *list)
    {
        if (!sdesc || !sdesc->isElectrical())
            return (false);
        if (!list)
            return (true);
        CDl *ld = CDldb()->findLayer("SPTX", Electrical);
        if (!ld)
            return (false);

        int cnt = 0;
        for (stringlist *sl = list; sl; sl = sl->next)
            cnt++;
        char **ary = new char*[cnt];
        cnt = 0;
        for (stringlist *sl = list; sl; sl = sl->next)
            ary[cnt++] = sl->string;

        CDg gdesc;
        gdesc.init_gen(sdesc, ld);
        CDo *od;
        while ((od = gdesc.next()) != 0) {
            if (od->type() != CDLABEL)
                continue;
            hyList *hp = ((CDla*)od)->label();
            if (!hp)
                continue;
            char *str = hyList::string(hp, HYcvPlain, false);
            if (!str)
                continue;
            for (int i = 0; i < cnt; i++) {
                if (!ary[i])
                    continue;
                if (!strcmp(str, ary[i])) {
                    ary[i] = 0;
                    break;
                }
            }
            delete [] str;
        }

        bool ret = true;
        bool first = true;
        int x = 0, y = 0;
        for (int i = 0; i < cnt; i++) {
            if (!ary[i])
                continue;
            if (first) {
                sdesc->computeBB();
                const BBox *sBB = sdesc->BB();
                if (sBB->left != CDinfinity) {
                    x = sBB->left;
                    y = sBB->top;
                }
                first = false;
            }
            Label label;
            label.label = new hyList(0, ary[i], HYcvPlain);
            label.x = x;
            label.y = y;
            DSP()->DefaultLabelSize(label.label, Electrical, &label.width,
                &label.height);
            if (!sdesc->newLabel(0, &label, ld, 0, false))
                ret = false;
            ary[i] = 0;

            y += label.height;
        }
        delete [] ary;
        return (ret);
    }
}
// End of cSced functions


cSpiceBuilder::cSpiceBuilder()
{
    sb_name = sb_model = 0;
    sb_value = sb_param = 0;
    sb_nodes = 0;
    sb_devs = 0;
    sb_models = 0;
    sb_subckts = 0;
    sb_globs = 0;
    sb_cdesc = 0;
    sb_lines = 0;
    sb_stab = 0;
    sb_mutlist = 0;
    sb_keydb = new sKeyDb;
    sb_term_name = 0;
    sb_gnd_name = 0;
    sb_modfile = 0;

    sb_xpos = 0;
    sb_ypos = 0;
    sb_lineht = 0;
}


cSpiceBuilder::~cSpiceBuilder()
{
    delete [] sb_name;
    delete [] sb_model;
    delete [] sb_value;
    delete [] sb_param;
    sNodeLink::destroy(sb_nodes);
    sNodeLink::destroy(sb_devs);
    sModLink::destroy(sb_models);
    sSubcLink::destroy(sb_subckts);
    sGlobNode::destroy(sb_globs);
    sNodeLink::destroy(sb_lines);
    delete sb_stab;
    sMut::destroy(sb_mutlist);
    delete sb_keydb;
    delete [] sb_term_name;
    delete [] sb_gnd_name;
    delete [] sb_modfile;
}


// Set up the terminal and ground device names.  These should exist in
// the device library, if not return false.
//
bool
cSpiceBuilder::init_term_names()
{
    const char *msgfmt =
        "No %s device named \"%s\" found in device library, aborting.";

    const char *tname = CDvdb()->getVariable(VA_SourceTermDevName);
    if (!tname) {
        tname = SCD_TRM_NAME;

        // Backwards compatibility.
        if (!FIO()->LookupLibCell(0, tname, LIBdevice, 0))
            tname = "vcc";
    }
    if (!FIO()->LookupLibCell(0, tname, LIBdevice, 0)) {
        Log()->ErrorLogV(process_spice, msgfmt, "terminal", tname);
        return (false);
    }

    CDs *sd = open_device(tname);
    if (!sd) {
        Log()->ErrorLogV(process_spice, msgfmt, "terminal", tname);
        return (false);
    }
    if (sd->elecCellType() != CDelecTerm) {
        Log()->ErrorLogV(process_spice, msgfmt, "terminal", tname);
        return (false);
    }
    sb_term_name = lstring::copy(tname);

    tname = CDvdb()->getVariable(VA_SourceGndDevName);
    if (!tname)
        tname = SCD_GND_NAME;
    if (!FIO()->LookupLibCell(0, tname, LIBdevice, 0)) {
        Log()->ErrorLogV(process_spice, msgfmt, "ground", tname);
        return (false);
    }

    sd = open_device(tname);
    if (!sd) {
        Log()->ErrorLogV(process_spice, msgfmt, "ground", tname);
        return (false);
    }
    if (sd->elecCellType() != CDelecGnd) {
        Log()->ErrorLogV(process_spice, msgfmt, "ground", tname);
        return (false);
    }
    sb_gnd_name = lstring::copy(tname);
    return (true);
}


// Link a new node into the list.
//
void
cSpiceBuilder::add_node(char *nm)
{
    sNodeLink *n = new sNodeLink(nm, 0);
    if (!sb_nodes)
        sb_nodes = n;
    else {
        sNodeLink *nn = sb_nodes;
        while (nn->next)
            nn = nn->next;
        nn->next = n;
    }
}


// Link a new device reference into the list.
//
void
cSpiceBuilder::add_dev(char *nm)
{
    sNodeLink *n = new sNodeLink(nm, 0);
    if (!sb_devs)
        sb_devs = n;
    else {
        sNodeLink *nn = sb_devs;
        while (nn->next)
            nn = nn->next;
        nn->next = n;
    }
}


// Return the symbol name for the key, resolve n/p.
//
const char *
cSpiceBuilder::devname(const sKey *dev)
{
    if (!dev || !dev->k_ndev)
        return (0);
    if (!dev->k_pdev)
        return (dev->k_ndev);
    if (!sb_model)
        // format error
        return (0);

    // Note that model resolution is case insensitive.

    for (sModLink *m = sb_models; m; m = m->next) {
        if (!strcasecmp(sb_model, m->modname))
            return (setnp(dev, m->modtype));
    }

    // The model wasn't found in the text, first look in the model.lib
    // database.
    sp_line_t *l = SCD()->modelText(sb_model);
    if (l) {
        const char *s = l->li_line;
        char *tok = cSced::sp_gettok(&s);
        delete [] tok;
        tok = cSced::sp_gettok(&s);
        delete [] tok;
        tok = cSced::sp_gettok(&s);
        sp_line_t::destroy(l);
        s = setnp(dev, tok);
        delete [] tok;
        return (s);
    }

    // Still not found, look at the name itself.
    return (setnp(dev, sb_model, true));
}


// Return the i'th node name in the list.
//
char *
cSpiceBuilder::nodename(int i)
{
    sNodeLink *n = sb_nodes;
    while (i-- && n)
        n = n->next;
    if (!n)
        return (0);
    return (n->node);
}


// Return the named model, searching the hierarchy.  If the model was
// defined in a subcircuit, a new model name is returned.
//
sModLink *
cSpiceBuilder::findmod(const CDs *sdesc, const char *modname, char **newname)
{
    if (newname)
        *newname = 0;
    if (!sdesc || !modname)
        return (0);
    sSubcLink *sc = findsc(Tstring(sdesc->cellname()));
    if (sc) {
        do {
            for (sModLink *m = sc->models; m; m = m->next) {
                if (lstring::cieq(modname, m->modname)) {
                    if (newname) {
                        sLstr lstr;
                        lstr.add(modname);
                        lstr.add_c('_');
                        lstr.add(sc->name);
                        *newname = lstr.string_trim();
                    }
                    return (m);
                }
            }
            sc = sc->owner;
        } while (sc);
    }

    for (sModLink *m = sb_models; m; m = m->next) {
        if (lstring::cieq(modname, m->modname))
            return (m);
    }
    return (0);
}


// Return the named subckt from the list.
//
sSubcLink *
cSpiceBuilder::findsc(const char *nm)
{
    for (sSubcLink *s = sb_subckts; s; s = s->next)
        if (!strcmp(nm, s->name))
            return (s);
    return (0);
}


// Process the subcircuit, and any referenced subcircuits.
//
void
cSpiceBuilder::make_subc(const char *cname, int modeflag)
{
    char tbuf[256];
    sSubcLink *s = findsc(cname);
    if (!s) {
        /*
        sprintf(tbuf, "can't find subcircuit %s.", cname);
        record_error(tbuf);
        */
        return;
    }
    if (s->done)
        return;

    CDs *sdesc = CDcdb()->findCell(cname, Electrical);
    bool exists = (sdesc != 0);

    sNodeLink *n;
    for (n = s->lines; n; n = n->next) {
        // look for subckt references to process first
        const char *line = n->node;
        while (isspace(*line))
            line++;

        if (*line == 'x' || *line == 'X') {
            char *tok = cSced::sp_gettok(&line);  // skip xnnnn
            delete [] tok;
            char *ltok = 0;
            // nodes... subcname [params]
            while ((tok = cSced::sp_gettok(&line, true)) != 0) {

                // Assume here that node and subcircuit names never
                // contain '='.  Parameters are always name=value
                // pairs so these tokens always contain '='.

                if (strchr(tok, '=')) {
                    delete tok;
                    break;
                }

                delete [] ltok;
                ltok = tok;
            }
            // ltok is subcircuit name
            make_subc(ltok, modeflag);
            delete [] ltok;
        }
    }

    if (!exists && !(modeflag & EFS_CREATE))
        return;

    if (!sdesc)
        sdesc = CDcdb()->insertCell(cname, Electrical);
    if (!sdesc)
        return;

    Ulist()->ListChangeCell(sdesc);

    // add name property if needed
    if (!sdesc->prpty(P_NAME))
        sdesc->prptyAdd(P_NAME, P_NAME_SUBC_STR);

    mksymtab(sdesc, &s->stab, (modeflag & EFS_ALLDEVS));

    if (s->dotparams)
        add_spicetext(sdesc, s->dotparams);

    const BBox *sBB = sdesc->BB();
    if (sBB->left == CDinfinity) {
        // empty or contains only empty subcells
        sb_xpos = spMARG;
        sb_ypos = -spMARG;
    }
    else {
        sb_xpos = sBB->left + spMARG;
        sb_ypos = sBB->bottom - spMARG;
    }
    sb_lineht = 0;

    sGlobNode::destroy(sb_globs);
    sb_globs = 0;
    SymTab *st = sb_stab;
    sb_stab = s->stab;
    for (n = s->lines; n; n = n->next)
        parseline(sdesc, n->node, (modeflag & EFS_CREATE));
    process_muts(sdesc);
    sb_stab = st;
    sub_glob(sdesc);

    if (!exists || !sdesc->prpty(P_NODE)) {
        sCurTx curtf = *GEO()->curTx();
        sCurTx ct;
        ct.set_angle(180);
        GEO()->setCurTx(ct);

        int cnt = 0;
        int x = spTX1;
        int y = spTY;
        if (sdesc->BB()->left != CDinfinity) {
            // Array along cell top ON GRID!
            int ty = sdesc->BB()->top;
            y += ((ty + CDelecResolution/2)/CDelecResolution)*CDelecResolution;
        }

        // The list contains nodes and parameters.  Assume that node
        // names never contain '='.  Parameters are name=value pairs. 
        // Nodes are listed first.

        for (n = s->nodes; n; n = n->next) {
            if (strchr(n->node, '='))
                break;

            // A node, create terminals and properties.
            CDc *tcd = ED()->makeInstance(sdesc, sb_term_name, x, y);
            if (tcd) {

                // alter node name if numeric
                bool numeric = true;
                for (char *t = n->node; *t; t++) {
                    if (!isdigit(*t)) {
                        numeric = false;
                        break;
                    }
                }
                if (numeric)
                    sprintf(tbuf, "n%s", n->node);
                else
                    strcpy(tbuf, n->node);
                SCD()->prptyModify(tcd, 0, P_NAME, tbuf, 0);
            }
            else {
                sprintf(tbuf, "instance creation failed for %s.",
                    sb_term_name);
                record_error(tbuf);
            }
            sprintf(tbuf, "0 %d %d %d _%d 0 0 %d", cnt, x, y, cnt, TE_UNINIT);
            sdesc->prptyAdd(P_NODE, tbuf);
            x += spTX2;
            cnt++;
        }
        GEO()->setCurTx(curtf);
    }

    if (!exists || !sdesc->prpty(P_PARAM)) {
        sLstr plstr;
        for (n = s->nodes; n; n = n->next) {
            if (!strchr(n->node, '='))
                continue;

            if (plstr.length())
                plstr.add_c(' ');
            plstr.add(n->node);
        }
        if (plstr.length())
            sdesc->prptyAdd(P_PARAM, plstr.string());
    }

    CDs *psdesc = CDcdb()->findCell(cname, Physical);
    if (psdesc)
        psdesc->setAssociated(false);

    s->done = true;
}


// Place the device or subcircuit just parsed.
//
void
cSpiceBuilder::place(CDs *sdesc, const char *key, bool create)
{
    if (!key || !isalpha(*key))
        return;
    if (!sdesc) {
        record_error("place: internal error, null parent.");
        return;
    }
    char tbuf[256];
    const char *cname = 0;
    if (*key == 'x' || *key == 'X') {

        // x??? node1 ... nodeN scname param=xxx ....
        // We have to hunt down the subcircuit name.  At this point,
        // the nodes list contains all tokens.

        // First, reverse the list, nl0 is the new head.
        //
        sNodeLink *nl0 = 0, *nn;
        for (sNodeLink *n = sb_nodes; n; n = nn) {
            nn = n->next;
            n->next = nl0;
            nl0 = n;
        }

        // Search for a token that matches a name in the subckts list.
        // If there are no params, the first token should match.  The
        // matching link will be in nx.
        //
        sNodeLink *nx = 0;
        for (sNodeLink *n = nl0; n; n = n->next) {
            for (sSubcLink *s = sb_subckts; s; s = s->next) {
                if (!strcmp(s->name, n->node)) {
                    nx = n;
                    break;
                }
            }
            if (nx)
                break;
        }

        if (!nx) {
            // Error, the subckt wasn't found.
            sprintf(tbuf,
                "no subckt found for %s in %s, saved as spicetext label.",
                sb_name, Tstring(sdesc->cellname()));
            record_error(tbuf);

            sb_nodes = 0;
            for (sNodeLink *n = nl0; n; n = nn) {
                nn = n->next;
                n->next = sb_nodes;
                sb_nodes = n;
            }

            // Add the call as a spicetext label.  This will hopefully
            // be resolved by an include.
            sLstr lstr;
            lstr.add(key);
            for (sNodeLink *n = sb_nodes; n; n = n->next) {
                lstr.add_c(' ');
                lstr.add(n->node);
            }
            stringlist *sl = new stringlist(lstr.string_trim(), 0);
            add_spicetext(sdesc, sl);
            stringlist::destroy(sl);
        }
        else {
            // The links following nx are the actual nodes, in reverse
            // order.  Reverse these and put them back in the nodes
            // field.
            //
            sb_nodes = 0;
            for (sNodeLink *n = nx->next; n; n = nn) {
                nn = n->next;
                n->next = sb_nodes;
                sb_nodes = n;
            }
            nx->next = 0;

            // Reverse the remaining links into the params.  The first
            // element will be nx, which is popped off.
            //
            sNodeLink *params = 0;
            for (sNodeLink *n = nl0; n; n = nn) {
                nn = n->next;
                n->next = params;
                params = n;
            }
            params = params->next;

            // Write a param string from params.
            sLstr lstr;
            for (sNodeLink *n = params; n; n = n->next) {
                if (n != params)
                    lstr.add_c(' ');
                lstr.add(n->node);
            }
            sb_param = lstr.string_trim();
            sNodeLink::destroy(params);

            // Grab the name out of nx, then destroy nx.
            cname = nx->node;
            nx->node = 0;
            delete nx;
        }
    }
    else if (*key == 'k' || *key == 'K') {
        // mutual inductor
        if (!sb_devs || !sb_devs->next)
            return;
        char *l1 = sb_devs->node;
        char *l2 = sb_devs->next->node;
        if (!l1 || !l2)
            return;
        char *val = sb_value;
        if (!val)
            val = sb_model;
        if (!val)
            return;
        sb_mutlist = new sMut(sb_name, l1, l2, val, sb_mutlist);
        sb_name = 0;
        sb_devs->node = 0;
        sb_devs->next->node = 0;
        sb_value = 0;
        sb_model = 0;
    }
    else {
        const sKey *which = sb_keydb->find_key(key);
        cname = devname(which);
        if (!cname) {
            sprintf(tbuf, "can't resolve device %s in %s.  No model?",
                sb_name, Tstring(sdesc->cellname()));
            record_error(tbuf);
        }
        cname = lstring::copy(cname);
    }
    if (cname) {
        GCarray<const char*> gc_cname(cname);
        sb_cdesc = (CDc*)SymTab::get(sb_stab, sb_name);
        if (sb_cdesc == (CDc*)ST_NIL) {
            sb_cdesc = 0;
            if (!create)
                return;
            Errs()->init_error();
            CDcbin cbin;
            if (OIfailed(CD()->OpenExisting(cname, &cbin))) {
                // shouldn't happen
                Errs()->add_error("Open failed");
                record_error(Errs()->get_error());
                return;
            }
            CDs *esdesc = cbin.elec();
            if (!esdesc)
                esdesc = CDcdb()->insertCell(cname, Electrical);
            if (esdesc->isEmpty()) {
                if (!fix_empty_cell(esdesc)) {
                    sprintf(tbuf,
                        "Attempt to fix empty instance of %s in %s failed.",
                        cname, Tstring(sdesc->cellname()));
                    record_error(tbuf);
                    return;
                }
            }
            // add name property if needed
            if (!esdesc->isDevice() && !esdesc->prpty(P_NAME))
                esdesc->prptyAdd(P_NAME, P_NAME_SUBC_STR);

            int x, y;
            cur_posn(esdesc, &x, &y);
            sCurTx curtf = *GEO()->curTx();
            sCurTx ct;
            GEO()->setCurTx(ct);
            sb_cdesc = ED()->makeInstance(sdesc, cname, x, y);
            GEO()->setCurTx(curtf);
            if (sb_cdesc) {
                apply_properties();
                apply_terms(sdesc);
                cur_rpos(sb_cdesc);
                sb_cdesc = 0;
            }
            else {
                sprintf(tbuf, "instance creation failed for %s in %s.",
                    cname, Tstring(sdesc->cellname()));
                record_error(tbuf);
            }
        }
        else {
            apply_properties();
            sb_cdesc = 0;
        }
    }
}


// Add the extracted properties.
//
bool
cSpiceBuilder::apply_properties()
{
    // If the cdesc field in not 0, a matching device/subckt was
    // found.  In this case, update the properties.
    //
    if (sb_cdesc) {
        int len = sb_param ? strlen(sb_param) : 0;
        if (len > LT_THRESHOLD) {
            const char *lttok = HYtokPre HYtokLT HYtokSuf;
            char *tp = new char[len + strlen(lttok) + 1];
            strcpy(tp, lttok);
            strcat(tp, sb_param);
            delete [] sb_param;
            sb_param = tp;
        }

        if (!sb_cdesc->isDevice()) {
            CDp_cname *pn = (CDp_cname*)sb_cdesc->prpty(P_NAME);
            if (pn && (!pn->assigned_name() ||
                    strcmp(sb_name, pn->assigned_name()))) {
                pn->set_assigned_name(sb_name);
            }
            CDp *p = sb_cdesc->prpty(P_PARAM);
            SCD()->prptyModify(sb_cdesc, p, P_PARAM, sb_param, 0);
        }
        else {
            if (sb_name) {
                CDp_cname *pn = (CDp_cname*)sb_cdesc->prpty(P_NAME);
                if (pn && (!pn->assigned_name() ||
                        strcmp(sb_name, pn->assigned_name()))) {
                    pn->set_assigned_name(sb_name);
                }
            }
            CDp *p = sb_cdesc->prpty(P_MODEL);
            SCD()->prptyModify(sb_cdesc, p, P_MODEL, sb_model, 0);
            p = sb_cdesc->prpty(P_VALUE);
            SCD()->prptyModify(sb_cdesc, p, P_VALUE, sb_value, 0);
            p = sb_cdesc->prpty(P_PARAM);
            SCD()->prptyModify(sb_cdesc, p, P_PARAM, sb_param, 0);
            if (sb_devs) {
                sLstr lstr;
                for (sNodeLink *n = sb_devs; n; n = n->next) {
                    if (lstr.string())
                        lstr.add_c(' ');
                    lstr.add(n->node);
                }
                p = sb_cdesc->prpty(P_DEVREF);
                SCD()->prptyModify(sb_cdesc, p, P_DEVREF, lstr.string(), 0);
            }
        }
    }
    return (true);
}


// Add the node to the global list.
//
void
cSpiceBuilder::add_glob(const char *nname, const char *cname)
{
    if (!nname)
        return;
    bool numeric = true;
    for (const char *s = nname; *s; s++) {
        if (!isdigit(*s)) {
            numeric = false;
            break;
        }
    }
    char buf[64];
    if (numeric) {
        sprintf(buf, "n%s", nname);
        nname = buf;
    }
    for (sGlobNode *n = sb_globs; n; n = n->next) {
        if (!strcmp(nname, n->nname))
            // already there
            return;
    }

    const stringlist *sl = FIO()->GetLibraryProperties(XM()->DeviceLibName());
    for ( ; sl; sl = sl->next) {
        const char *s = sl->string;
        char *ntok = lstring::gettok(&s);
        if (!ntok)
            continue;
        if (!strcasecmp(ntok, LpDefaultNodeStr) ||
                LpDefaultNodeVal == atoi(ntok)) {
            char *name = lstring::gettok(&s);
            lstring::advtok(&s);
            char *val = lstring::gettok(&s);
            if (val) {
                if (!strcmp(cname, name)) {
                    sb_globs = new sGlobNode(lstring::copy(nname), val,
                        sb_globs);
                    delete [] name;
                    delete [] ntok;
                    return;
                }
            }
            delete [] name;
            delete [] val;
        }
        delete [] ntok;
    }
}


// Hunt through the named terminals, and reassign the names of substrate
// connections.
//
void
cSpiceBuilder::sub_glob(CDs *sdesc)
{
    if (!sb_globs)
        return;
    if (!sdesc) {
        record_error("glob: internal error, null parent.");
        return;
    }
    // all substrate nodes for device should be the same
    for (sGlobNode *g = sb_globs; g; g = g->next) {
        for (sGlobNode *gg = g->next; gg; gg = gg->next) {
            if (!strcmp(g->subst, gg->subst)) {
                char buf[128];
                sprintf(buf, "ambiguous mapping for %s (%s and %s) in %s.",
                    g->subst, g->nname, gg->nname, Tstring(sdesc->cellname()));
                record_error(buf);
            }
        }
    }
    CDm_gen mgen(sdesc, GEN_MASTERS);
    for (CDm *m = mgen.m_first(); m; m = mgen.m_next()) {
        CDs *msdesc = m->celldesc();
        if (!msdesc)
            continue;
        CDelecCellType tp = msdesc->elecCellType();
        if (tp == CDelecTerm) {
            // terminal
            CDc_gen cgen(m);
            for (CDc *c = cgen.c_first(); c; c = cgen.c_next()) {
                CDp_cname *pna = (CDp_cname*)c->prpty(P_NAME);
                if (!pna)
                    continue;
                CDla *olabel = pna->bound();
                if (!olabel)
                    continue;
                char *label = hyList::string(olabel->label(), HYcvPlain,
                    false);
                if (label) {
                    for (sGlobNode *g = sb_globs; g; g = g->next) {
                        if (!strcmp(label, g->nname)) {
                            SCD()->prptyModify(c, pna, P_NAME, g->subst, 0);
                            break;
                        }
                    }
                }
                delete [] label;
            }
            break;
        }
    }
}


enum ptType { ptUp, ptLeft, ptDown, ptRight};

// Add a terminal for each node.
//
bool
cSpiceBuilder::apply_terms(CDs *sdesc)
{
    if (!sdesc) {
        record_error("apply_terms: internal error, null parent.");
        return (false);
    }
    if (sb_cdesc) {
        sCurTx curtf = *GEO()->curTx();
        sCurTx ct = *GEO()->curTx();
        ct.set_reflectX(false);
        ct.set_reflectY(false);
        ct.set_magn(1.0);
        GEO()->setCurTx(ct);
        CDp_cnode *pn = (CDp_cnode*)sb_cdesc->prpty(P_NODE);

        // store substrate node for defaulting
        int ncnt = 0;
        for ( ; pn; pn = pn->next())
            ncnt++;
        int acnt = 0;
        for (sNodeLink *nk = sb_nodes; nk; nk = nk->next)
            acnt++;
        if (ncnt < acnt)
            add_glob(nodename(acnt-1), Tstring(sb_cdesc->cellname()));

        pn = (CDp_cnode*)sb_cdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            char *nname = nodename(pn->index());
            if (nname) {
                ct.set_angle(0);
                int x, y;
                if (!pn->get_pos(0, &x, &y))
                    continue;
                int code = orient(x, y, &sb_cdesc->oBB());
                if (strcmp(nname, "0")) {
                    // not a ground node
                    switch (code) {
                    case ptUp:
                        break;
                    case ptLeft:
                        ct.set_angle(90);
                        break;
                    case ptDown:
                        ct.set_angle(180);
                        break;
                    case ptRight:
                        ct.set_angle(270);
                        break;
                    }
                    GEO()->setCurTx(ct);
                    char buf[128];
                    CDc *tcd = ED()->makeInstance(sdesc, sb_term_name, x, y);
                    if (tcd) {
                        bool numeric = true;
                        for (char *s = nname; *s; s++) {
                            if (!isdigit(*s)) {
                                numeric = false;
                                break;
                            }
                        }
                        if (numeric)
                            sprintf(buf, "n%s", nname);
                        else
                            strcpy(buf, nname);
                        SCD()->prptyModify(tcd, 0, P_NAME, buf, 0);
                    }
                    else {
                        sprintf(buf, "instance creation failed for %s in %s.",
                            sb_term_name, Tstring(sdesc->cellname()));
                        record_error(buf);
                    }
                }
                else {
                    // ground node
                    switch (code) {
                    case ptUp:
                        ct.set_angle(180);
                        break;
                    case ptLeft:
                        ct.set_angle(270);
                        break;
                    case ptDown:
                        break;
                    case ptRight:
                        ct.set_angle(90);
                        break;
                    }
                    GEO()->setCurTx(ct);
                    if (!ED()->makeInstance(sdesc, sb_gnd_name, x, y)) {
                        char buf[128];
                        sprintf(buf, "instance creation failed for %s in %s.",
                            sb_gnd_name, Tstring(sdesc->cellname()));
                        record_error(buf);
                    }
                }
            }
        }
        GEO()->setCurTx(curtf);
    }
    return (true);
}


// Update the current placement position.
//
void
cSpiceBuilder::cur_rpos(CDc *cd)
{
    cd->updateBB();
    int lh = 2*spMARG + cd->oBB().height();
    if (lh > sb_lineht)
        sb_lineht = lh;
    sb_xpos += 2*spMARG + cd->oBB().width();
    sb_xpos =
        ((sb_xpos + CDelecResolution/2)/CDelecResolution)*CDelecResolution;
    if (sb_xpos > spFWID) {
        sb_xpos = spMARG;
        sb_ypos -= sb_lineht;
        sb_ypos =
            ((sb_ypos - CDelecResolution/2)/CDelecResolution)*CDelecResolution;
        sb_lineht = 0;
    }
}


// Return the placement position for an instance of sdesc.  This is
// the position of the reference terminal.  Use the closest grid point
// to keep the upper-left of the bounding box near xpos,ypos.
//
void
cSpiceBuilder::cur_posn(CDs *sdesc, int *x, int *y)
{
    // The sdesc is always a schematic cell, but instances will be
    // symbolic if there is an active symbolic rep.

    sdesc->computeBB();
    const BBox *bbp = sdesc->BB();
    int rx = bbp->left;
    int ry = bbp->top;

    // Index 0 is the reference point.
    CDp_snode *pn = (CDp_snode*)sdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        if (pn->index() == 0) {
            if (sdesc->symbolicRep(0))
                pn->get_pos(0, &rx, &ry);
            else
                pn->get_schem_pos(&rx, &ry);
            break;
        }
    }
    int tx = sb_xpos + rx - bbp->left;
    *x = (tx/CDelecResolution) * CDelecResolution;
    int ty = sb_ypos + ry - bbp->top;
    *y = (ty/CDelecResolution) * CDelecResolution;
}


namespace {
    bool is_valid_modname(const char *name)
    {
        if (!isalpha(*name))
            return (false);
        for (const char *s = name+1; *s; s++) {
            if (isalnum(*s))
                continue;
            if (*s == '.')
                continue;
            if (*s == '_')
                continue;
            return (false);
        }
        return (true);
    }
}


// Parse the line and process data.
//
void
cSpiceBuilder::parseline(CDs *sdesc, const char *line, bool create)
{
    while (isspace(*line))
        line++;
    char *key = cSced::sp_gettok(&line);
    if (!key)
        return;
    GCarray<char*> gc_key(key);
    if (*key == 'x' || *key == 'X') {
        // subcircuit reference: X? N1 ... N? subc [PARAMS]
        sb_name = lstring::copy(key);
        char *tok;
        while ((tok = cSced::sp_gettok(&line, true)) != 0)
            add_node(tok);
        place(sdesc, key, create);
    }
    else if (isalpha(*key)) {
        const sKey *which = sb_keydb->find_key(key);
        if (!which || (!which->k_min_nodes && !which->k_num_devs))
            return;
        sb_name = lstring::copy(key);
        int ic = which->k_min_nodes;
        while (ic--)
            add_node(cSced::sp_gettok(&line));
        ic = which->k_num_devs;
        while (ic--)
            add_dev(cSced::sp_gettok(&line));
        if (which->k_val_only) {
            // These devices don't have a model, put the rest of the
            // line in a value.

            if (*line) {
                sb_value = lstring::copy(line);
                char *s = sb_value + strlen(sb_value) - 1;
                while (isspace(*s) && s >= sb_value)
                    *s-- = 0;
            }
        }
        else {
            // nodes... [model or value] [params]
            // From here on, don't use sp_gettok, since we may need to
            // preserve a leading '(' in a value expression.

            char *tok = lstring::gettok(&line, ",");
            if (tok) {
                bool foundmod = false;
                for (int i = which->k_min_nodes; i < which->k_max_nodes; i++) {
                    char *newname;
                    if (findmod(sdesc, tok, &newname) && newname) {
                        foundmod = true;
                        sb_model = newname;
                        delete [] tok;
                        break;
                    }
                    if (SCD()->isModel(tok)) {
                        foundmod = true;
                        sb_model = tok;
                        break;
                    }
                    add_node(tok);
                    tok = lstring::gettok(&line, ",");
                }
                if (!foundmod && tok) {
                    char *newname;
                    if (findmod(sdesc, tok, &newname) && newname) {
                        foundmod = true;
                        sb_model = newname;
                        delete [] tok;
                    }
                    else if (SCD()->isModel(tok)) {
                        foundmod = true;
                        sb_model = tok;
                    }
                }
                if (!foundmod && tok) {
                    if (is_valid_modname(tok))
                        sb_model = tok;
                    else
                        sb_value = tok;
                }
                if (*line) {
                    char *t = lstring::copy(line);
                    char *e = t + strlen(t) - 1;
                    while (e >= t && isspace(*e))
                        e--;
                    sb_param = t;
                }
            }
        }
        place(sdesc, key, create);
    }
    delete [] sb_name;
    delete [] sb_model;
    delete [] sb_value;
    delete [] sb_param;
    sb_name = 0;
    sb_model = 0;
    sb_value = 0;
    sb_param = 0;
    sNodeLink::destroy(sb_nodes);
    sb_nodes = 0;
    sNodeLink::destroy(sb_devs);
    sb_devs = 0;
}


// Add to or modify the mutual inductor properties according to
// input.
//
void
cSpiceBuilder::process_muts(CDs *sdesc)
{
    if (!sdesc) {
        record_error("process_muts: internal error, null parent.");
        return;
    }
    for (sMut *m = sb_mutlist; m; m = m->next) {
        CDp_nmut *pm = (CDp_nmut*)sdesc->prpty(P_NEWMUT);
        bool found = false;
        const char *name1 = 0;
        const char *name2 = 0;
        for ( ; pm; pm = pm->next()) {
            CDc *cdesc1, *cdesc2;
            if (!pm->get_descs(&cdesc1, &cdesc2))
                continue;
            name1 = cdesc1->getElecInstBaseName();
            name2 = cdesc2->getElecInstBaseName();
            if ((!strcmp(name1, m->ind1) && !strcmp(name2, m->ind2)) ||
                    (!strcmp(name1, m->ind2) && !strcmp(name2, m->ind2))) {
                found = true;
                pm->set_coeff_str(m->val);
                Errs()->init_error();
                if (!pm->rename(sdesc, m->name))
                    Log()->ErrorLog(process_spice, Errs()->get_error());
                if (!SCD()->setMutLabel(sdesc, pm, pm)) {
                    Errs()->add_error("setMutLabel failed");
                    Log()->ErrorLog(process_spice, Errs()->get_error());
                }
                break;
            }
        }
        if (!found) {
            CDc *cdesc1 = 0, *cdesc2 = 0;
            CDg gdesc;
            gdesc.init_gen(sdesc, CellLayer());
            CDc *cd;
            while ((cd = (CDc*)gdesc.next()) != 0) {
                if (!cd->is_normal())
                    continue;
                CDp_cname *pna = (CDp_cname*)cd->prpty(P_NAME);
                if (!pna)
                    continue;
                name1 = cd->getElecInstBaseName(pna);
                if (!strcmp(name1, m->ind1))
                    cdesc1 = cd;
                else if (!strcmp(name1, m->ind2))
                    cdesc2 = cd;
                if (cdesc1 && cdesc2)
                    break;
            }
            char buf[256];
            if (!cdesc1) {
                sprintf(buf, "can't find %s for mutual pair in %s.",
                    m->ind1, Tstring(sdesc->cellname()));
                record_error(buf);
            }
            else if (!cdesc2) {
                sprintf(buf, "can't find %s for mutual pair in %s.",
                    m->ind2, Tstring(sdesc->cellname()));
                record_error(buf);
            }
            else if (!sdesc->prptyMutualAdd(cdesc1, cdesc2, m->val,
                    m->name))
                record_error(Errs()->get_error());
        }
    }
    sMut::destroy(sb_mutlist);
    sb_mutlist = 0;
}


namespace {
    struct sMparm
    {
        sMparm(char *n)
            {
                next = 0;
                pname = n;
                pval = 0;
            }

        ~sMparm()
            {
                delete [] pname;
                delete [] pval;
            }

        static void destroy(const sMparm *m)
            {
                while (m) {
                    const sMparm *mx = m;
                    m = m->next;
                    delete mx;
                }
            }

        sMparm *next;
        char *pname;
        char *pval;
    };


    // Attempt to get a "name=value" token from the string.
    //
    sMparm *get_mtok(const char **p)
    {
        const char *s = *p;
        while (isspace(*s) || *s == ',' || *s == '(' || *s == ')')
            s++;
        bool was_eq = (*s == '=');
        if (was_eq) {
            while (isspace(*s) || *s == ',' || *s == '=')
                s++;
        }
        const char *t1 = s;
        while (*s && !isspace(*s) && *s != '=' && *s != ',' && *s != ')')
            s++;
        if (s == t1) {
            *p = s;
            return (0);
        }
        sMparm *m = new sMparm(new char[s - t1 + 1]);
        strncpy(m->pname, t1, s-t1);
        m->pname[s-t1] = 0;

        if (was_eq || SPnum.parse(&t1, false)) {
            // Parameter name looks numeric, just return it.
            m->pval = m->pname;
            m->pname = 0;
            *p = s;
            return (m);
        }

        while (isspace(*s))
            s++;
        was_eq = (*s == '=');
        while (isspace(*s) || *s == ',' || *s == '=')
            s++;
        const char *t2 = s;
        while (*s && !isspace(*s) && *s != '=' && *s != ',' && *s != ')')
            s++;
        if (s == t2) {
            *p = s;
            return (m);
        }
        char *tok = new char[s - t2 + 1];
        strncpy(tok, t2, s-t2);
        tok[s-t2] = 0;
        if (was_eq || SPnum.parse(&t2, false)) {
            m->pval = tok;
            *p = s;
            return (m);
        }
        *p = s;
        m->next = new sMparm(tok);
        return (m);
    }
}


// Dump all models found in input to a file.
//
bool
cSpiceBuilder::dump_models(const char *cname)
{
    bool hasmod = (sb_models != 0);
    if (!hasmod) {
        for (sSubcLink *sc = sb_subckts; sc; sc = sc->next) {
            if (sc->models) {
                hasmod = true;
                break;
            }
        }
    }
    if (!hasmod)
        return (true);

    char *fname;
    if (!cname || !*cname)
        fname = lstring::copy("models.inc");
    else {
        fname = new char[strlen(cname) + 12];
        char *e = lstring::stpcpy(fname, cname);
        *e++ = '_';
        strcpy(e, "models.inc");
    }
    if (!filestat::create_bak(fname)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        delete [] fname;
        return (false);
    }
    FILE *fp = fopen(fname, "w");
    if (!fp) {
        GRpkgIf()->Perror(fname);
        delete [] fname;
        return (false);
    }
    delete [] sb_modfile;
    sb_modfile = fname;

    fprintf(fp, "*****  %s\n", XM()->IdString());

    dump_models(fp, sb_models, 0);
    for (sSubcLink *sc = sb_subckts; sc; sc = sc->next) {
        if (!sc->models)
            continue;
        sLstr lstr;
        lstr.add_c('_');
        lstr.add(sc->name);
        dump_models(fp, sc->models, lstr.string());
    }

    fclose(fp);
    PL()->ShowPromptV("Wrote models into %s file.", fname);
    return (true);
}


// Format and dump to fp each model in the list.  Models defined in
// subcircuits will have a suffix added to the name, for uniqueness.
//
void
cSpiceBuilder::dump_models(FILE *fp, sModLink *mods, const char *namesfx)
{
    for (sModLink *m = mods; m; m = m->next) {
        sMparm *p0 = 0, *pe = 0;
        const char *t = m->text;
        for (;;) {
            sMparm *p = get_mtok(&t);
            if (!p)
                break;
            if (!p0)
                p0 = pe = p;
            else {
                while (pe->next)
                    pe = pe->next;
                pe->next = p;
            }
        }
        if (namesfx)
            fprintf(fp, "\n.model %s%s %s\n", m->modname, namesfx, m->modtype);
        else
            fprintf(fp, "\n.model %s %s\n", m->modname, m->modtype);
        for (sMparm *p = p0; p; ) {
            fprintf(fp, "+ ");
            for (int col = 0; p && col < 3; col++) {
                fprintf(fp, "%-10s", p->pname ? p->pname : "");
                if (p->pval)
                    fprintf(fp, "= %-14s", p->pval);
                else if (p->next && !p->next->pname) {
                    p = p->next;
                    fprintf(fp, "= %-14s", p->pval);
                }
                else
                    fprintf(fp, "  %-14s", "");
                p = p->next;
            }
            fprintf(fp, "\n");
        }
        sMparm::destroy(p0);
    }
}


// Static function.
// Read a logical line from fp.
//
char *
cSpiceBuilder::readline(FILE *fp, int *lastc)
{
    if (*lastc == EOF)
        return (0);
    sLstr lstr;
    if (*lastc)
        lstr.add_c(*lastc);
    int c;
    do {
        while ((c = getc(fp)) != EOF) {
            if (c == '\r')
                continue;
            if (c == '\n')
                break;
            lstr.add_c(c);
        }
        if (c == EOF) {
            if (lstr.length() == 0)
                return (0);
            *lastc = EOF;
        }
        else {
            while ((c = getc(fp)) != EOF) {
                if (isspace(c))
                    continue;
                break;
            }
            *lastc = c;
            if (c == '+')
                lstr.add_c(' ');
        }
    } while (c == '+');
    return (lstr.string_trim());
}


// Resolve n/p from mname.
//
const char *
cSpiceBuilder::setnp(const sKey *dev, char *mname, bool noerr)
{
    if (is_n(mname))
        return (dev->k_ndev);
    if (is_p(mname))
        return (dev->k_pdev);
    if (!noerr) {
        char buf[128];
        sprintf(buf, "strange model %s.", mname);
        record_error(buf);
    }
    return (0);
}


// Return a code indicating orientation for terminal to be placed.
//
int
cSpiceBuilder::orient(int x, int y, const BBox *BB)
{
    if (y > (BB->bottom + 3*BB->top)/4)
        return (ptUp);
    if (y < (3*BB->bottom + BB->top)/4)
        return (ptDown);
    if (x > (BB->left + 3*BB->right)/4)
        return (ptRight);
    if (x < (3*BB->left + BB->right)/4)
        return (ptLeft);
    return (ptUp);
}


void
cSpiceBuilder::record_error(const char *line)
{
    char *t = lstring::copy(line);
    char *s = t + strlen(t) - 1;
    while (s >= t && isspace(*s))
        *s-- = 0;
    Log()->WarningLogV(process_spice, "%s", t);
    delete [] t;
}


// Add some geometry to an empty cell.
//
bool
cSpiceBuilder::fix_empty_cell(CDs *esdesc)
{
    // find the BB of the terminals
    BBox BB(CDnullBB);
    CDp_snode *ps = (CDp_snode*)esdesc->prpty(P_NODE);
    for ( ; ps; ps = ps->next()) {
        int x, y;
        ps->get_schem_pos(&x, &y);
        BB.add(x, y);
    }
    int wmin = 10*CDelecResolution;
    if (BB.left == CDinfinity)
        BB = BBox(0, 0, wmin, wmin);
    if (BB.right < BB.left + wmin)
        BB.right = BB.left + wmin;
    if (BB.top < BB.bottom + wmin)
        BB.bottom = BB.top - wmin;

    CDl *ldesc = CDldb()->findLayer("ETC1", Electrical);
    if (!ldesc)
        ldesc = CDldb()->findLayer("SCED", Electrical);
    if (!ldesc)
        return (false);

    Wire wire;
    Point p[2];
    wire.numpts = 2;
    wire.set_wire_width(0);
    p[0].set(BB.left, BB.bottom);
    p[1].set(BB.left, BB.top);
    CDw *wptr;
    wire.points = new Point[2];
    wire.points[0] = p[0];
    wire.points[1] = p[1];
    if (esdesc->makeWire(ldesc, &wire, &wptr) != CDok)
        return (false);
    p[0].y = BB.top;
    p[1].x = BB.right;
    wire.points = new Point[2];
    wire.points[0] = p[0];
    wire.points[1] = p[1];
    if (esdesc->makeWire(ldesc, &wire, &wptr) != CDok)
        return (false);
    p[0].x = BB.right;
    p[1].y = BB.bottom;
    wire.points = new Point[2];
    wire.points[0] = p[0];
    wire.points[1] = p[1];
    if (esdesc->makeWire(ldesc, &wire, &wptr) != CDok)
        return (false);
    p[0].y = BB.bottom;
    p[1].x = BB.left;
    wire.points = new Point[2];
    wire.points[0] = p[0];
    wire.points[1] = p[1];
    if (esdesc->makeWire(ldesc, &wire, &wptr) != CDok)
        return (false);
    Label la;
    la.x = BB.left + CDelecResolution;
    la.y = BB.bottom + CDelecResolution;
    la.width = 8*CDelecResolution;
    la.height = 2*CDelecResolution;
    la.label = new hyList(esdesc, "empty", HYcvAscii);
    CDla *lptr;
    if (esdesc->makeLabel(ldesc, &la, &lptr) != CDok)
        return (false);
    return (true);
}


// Static function.
// Clear the contents of sdesc, allows undo.
//
void
cSpiceBuilder::clear_cell(CDs *sdesc)
{
    if (sdesc->owner())
        sdesc = sdesc->owner();
    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0)
        Ulist()->RecordObjectChange(sdesc, cdesc, 0);
    CDl *ld;
    CDsLgen lgen(sdesc);
    while ((ld = lgen.next()) != 0) {
        gdesc.init_gen(sdesc, ld);
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0)
            Ulist()->RecordObjectChange(sdesc, odesc, 0);
    }
    sdesc->setFlags(sdesc->getFlags() & ~CDs_BBVALID);
    sdesc->computeBB();

    // Free and clear all properties except the symbolic property,
    // which will be retained though set inactive if found.
    CDp_sym *ps = (CDp_sym*)sdesc->prpty(P_SYMBLC);
    if (ps)
        sdesc->prptyUnlink(ps);
    sdesc->prptyFreeList();
    if (ps) {
        ps->set_active(false);
        ps->set_next_prp(0);
        sdesc->setPrptyList(ps);
    }

    // The groups contain formal terminals that have been freed,
    // so have to purge them.
    ExtIf()->clearFormalTerms(sdesc);

    CDs *tsd = CDcdb()->findCell(sdesc->cellname(), Physical);
    if (tsd)
        tsd->setAssociated(false);
}


// Static function.
// Greate a symbol table containing all of the devices or subcircuits
// in sdesc with user set names.
//
void
cSpiceBuilder::mksymtab(CDs *sdesc, SymTab **symtab, bool alldevs)
{
    *symtab = new SymTab(true, false);
    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer());
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (!cdesc->is_normal())
            continue;
        CDs *msdesc = cdesc->masterCell();
        if (!msdesc)
            continue;
        CDp_cname *pn = (CDp_cname*)cdesc->prpty(P_NAME);
        if (alldevs) {
            // add all devices
            if (pn) {
                const char *instname = cdesc->getElecInstBaseName(pn);
                (*symtab)->add(lstring::copy(instname), cdesc, false);
            }
        }
        else {
            // add only devices with user-set names
            if (pn && pn->assigned_name())
                (*symtab)->add(lstring::copy(pn->assigned_name()), cdesc,
                    false);
        }
    }
}
// End of cSpiceBuilder functions.


namespace {
    // Return true/false depending on string content, defaults to false
    // if unrecognized.
    //
    bool boolval(char *str)
    {
        if (!str)
            return (false);
        char c = isupper(*str) ? tolower(*str) : *str;
        if (c == '0' || c == 'f' || c == 'n')
            return (false);
        if (isdigit(c) || c == 't' || c == 'y')
            return (true);
        if (c == 'o') {
            if (str[1] == 'n' || str[1] == 'N')
                return (true);
        }
        return (false);
    }


    void log_mosfoo(int nn, const char *mn, int nx)
    {
        Log()->ErrorLogV(process_spice,
    "Warning: The schematic will use %d-node MOS devices.  The technology\n"
    "file defines %s with %d nodes.  The node count must be consistent\n"
    "for LVS.",
            nn, mn, nx);
    }

    sKeyDb::sKeyDb()
    {
        for (int i = 0; i < 26; i++)
            db_keys[i] = 0;

        // First add the defaults.
        //                 min max dev val
        db_constr = true;
        add_key(new_key("a", 2, 2, 0, true,   "vsrc", 0));
        add_key(new_key("b", 2, 3, 0, false,  "jj",   0));
        add_key(new_key("c", 2, 2, 0, false,  "cap",  0));
        add_key(new_key("d", 2, 2, 0, false,  "dio",  0));
        add_key(new_key("e", 4, 4, 0, true,   "vcvs", 0));
        add_key(new_key("f", 2, 2, 1, true,   "cccs", 0));
        add_key(new_key("g", 4, 4, 0, true,   "vccs", 0));
        add_key(new_key("h", 2, 2, 1, true,   "ccvs", 0));
        add_key(new_key("i", 2, 2, 0, true,   "isrc", 0));
        add_key(new_key("j", 3, 3, 0, false,  "njf",  "pjf"));
        add_key(new_key("k", 0, 0, 2, true,   0,      0));
        add_key(new_key("l", 2, 2, 0, true,   "ind",  0));
        add_key(new_key("m", 4, 4, 0, false,  "nmos", "pmos"));
        add_key(new_key("n", 0, 0, 0, false,  0,      0));
        add_key(new_key("o", 4, 4, 0, false,  "ltra", 0));
        add_key(new_key("p", 0, 0, 0, false,  0,      0));
        add_key(new_key("q", 3, 4, 0, false,  "npn",  "pnp"));
        add_key(new_key("r", 2, 2, 0, false,  "res",  0));
        add_key(new_key("s", 4, 4, 0, false,  "sw",   0));
        add_key(new_key("t", 4, 4, 0, false,  "tra",  0));
        add_key(new_key("u", 4, 4, 0, false,  "urc",  0));
        add_key(new_key("v", 2, 2, 0, true,   "vsrc", 0));
        add_key(new_key("w", 2, 2, 1, false,  "csw",  0));
        add_key(new_key("x", 0, 0, 0, false,  0,      0));
        add_key(new_key("y", 0, 0, 0, false,  0,      0));
        add_key(new_key("z", 3, 3, 0, false,  "nmes", "pmes"));
        db_constr = false;

        // Add the DeviceKey properties.
        const stringlist *sl =
            FIO()->GetLibraryProperties(XM()->DeviceLibName());
        for ( ; sl; sl = sl->next) {
            const char *s = sl->string;
            char *ntok = lstring::gettok(&s);
            if (!ntok)
                continue;
            if (!strcasecmp(ntok, LpDeviceKeyStr) ||
                    LpDeviceKeyVal == atoi(ntok)) {
                // Old style.

                char *toks[8];
                memset(toks, 0, 8*sizeof(char*));
                char *tok;
                int cnt = 0;
                while ((tok = lstring::gettok(&s)) != 0) {
                    toks[cnt++] = tok;
                    if (cnt == 8)
                        break;
                }
                const char *prf = toks[0];
                bool val_only = boolval(toks[2]);
                int min_nodes = atoi(toks[3]);
                int max_nodes = boolval(toks[1]) ? min_nodes+1 : min_nodes;
                int num_devs = 0;
                const char *nname = toks[4];
                const char *pname = toks[5];
                if (nname) {
                    sKey *key = new_key(prf, min_nodes, max_nodes,
                        num_devs, val_only, nname, pname);
                    if (key)
                        add_key(key);
                }
                for (int i = 0; i < 8; i++)
                    delete [] toks[i];
            }
            if (!strcasecmp(ntok, LpDeviceKeyV2Str) ||
                    LpDeviceKeyV2Val == atoi(ntok)) {
                // New (current) style property.

                char *toks[8];
                memset(toks, 0, 8*sizeof(char*));
                char *tok;
                int cnt = 0;
                while ((tok = lstring::gettok(&s)) != 0) {
                    toks[cnt++] = tok;
                    if (cnt == 8)
                        break;
                }
                const char *prf = toks[0];
                int min_nodes = atoi(toks[1]);
                int max_nodes = atoi(toks[2]);
                int num_devs = atoi(toks[3]);
                bool val_only = boolval(toks[4]);
                const char *nname = toks[5];
                const char *pname = toks[6];
                if (nname) {
                    sKey *key = new_key(prf, min_nodes, max_nodes,
                        num_devs, val_only, nname, pname);
                    if (key)
                        add_key(key);
                }
                for (int i = 0; i < 8; i++)
                    delete [] toks[i];
            }
            delete [] ntok;
        }

        // Check the MOS devices against the techfile.  The "real"
        // node count must match between schematic and physical or
        // LVS can't work.  It should be possible for n and p devices
        // to have different node counts, for example if only the p
        // devices are in a tub.  In this case, one may want to use
        // 3-t nmos devices, and 4-t pmos devices to validate the tub
        // connection.  The property would be
        //
        //   Property DeviceKey m false false 4 nmos pmos1
        //
        // Devices nmos and pmos1 are defined in the tech file with
        // three and four nodes, respectively, and the user must
        // remember to use nmos/pmos1 in the schematic.  A schematic
        // imported from SPICE should use the correct devices.

        const sKey *k = find_key("m");
        if (k) {
            CDs *sd = open_device(k->k_ndev);
            if (sd) {
                int nn = 0;
                CDp_node *pn = (CDp_node*)sd->prpty(P_NODE);
                for ( ; pn; pn = pn->next())
                    nn++;

                sDevDesc *d = EX()->findDevices(Tstring(sd->cellname()));
                if (d) {
                    int nx = 0;
                    for (sDevContactDesc *cx = d->contacts(); cx;
                            cx = cx->next())
                        nx++;
                    if (nn != nx)
                        log_mosfoo(nn, TstringNN(d->name()), nx);
                }
            }
            sd = open_device(k->k_pdev);
            if (sd) {
                int nn = 0;
                CDp_node *pn = (CDp_node*)sd->prpty(P_NODE);
                for ( ; pn; pn = pn->next())
                    nn++;

                sDevDesc *d = EX()->findDevices(Tstring(sd->cellname()));
                if (d) {
                    int nx = 0;
                    for (sDevContactDesc *cx = d->contacts(); cx;
                            cx = cx->next())
                        nx++;
                    if (nn != nx)
                        log_mosfoo(nn, TstringNN(d->name()), nx);
                }
            }
        }
    }


    // Return a new key struct allocated from local store.  All names are
    // converted to lower case.  If there is a sanity error from the args,
    // 0 is returned.
    //
    sKey *
    sKeyDb::new_key(const char *pfx, int min_nodes, int max_nodes,
        int num_devs, bool val_only, const char *nname, const char *pname)
    {
        if (!pfx || !isalpha(*pfx))
            return (0);
        if (!db_constr) {
            if (*pfx == 'x' || *pfx == 'X' || *pfx == 'k' || *pfx == 'K')
                // These are reserved: subckts and mutual inductors.
                return (0);
            if (!nname || !*nname)
                return (0);
        }
        if (min_nodes < 0 || min_nodes > max_nodes)
            return (0);

        char buf[256];
        sKey *key = db_eltab.new_element();
        char *s = buf;
        while (*pfx) {
            *s++ = isupper(*pfx) ? tolower(*pfx) : *pfx;
            pfx++;
        }
        *s = 0;
        key->k_key = db_strtab.add(buf);
        key->k_min_nodes = min_nodes;
        key->k_max_nodes = max_nodes;
        key->k_num_devs = num_devs;
        key->k_val_only = val_only;
        if (nname) {
            s = buf;
            while (*nname) {
                *s++ = isupper(*nname) ? tolower(*nname) : *nname;
                nname++;
            }
            *s = 0;
            key->k_ndev = db_strtab.add(buf);
        }
        else
            key->k_ndev = 0;
        if (pname) {
            s = buf;
            while (*pname) {
                *s++ = isupper(*pname) ? tolower(*pname) : *pname;
                pname++;
            }
            *s = 0;
            key->k_pdev = db_strtab.add(buf);
        }
        else
            key->k_pdev = 0;
        key->k_next = 0;
        return (key);
    };


    // There are 26 list heads - one for each letter.  The list heads are
    // ordered lexicographically reversed, e.g., abd, abc, ab, a.  A
    // duplicate prefix overwrites existing data.
    //
    bool
    sKeyDb::add_key(sKey *key)
    {
        if (!key || !key->k_key)
            return (false);
        int i = *key->k_key - 'a';
        if (i < 0 || i >= 26)
            return (false);
        if (!db_keys[i]) {
            db_keys[i] = key;
            return (true);
        }

        sKey *kp = 0, *kn;
        for (sKey *kx = db_keys[i]; kx; kx = kn) {
            kn = kx->k_next;
            int c = strcmp(key->k_key, kx->k_key);
            if (c == 0) {
                key->k_next = kn;
                if (kp)
                    kp->k_next = key;
                else
                    db_keys[i] = key;
            }
            else if (c > 0) {
                if (kp) {
                    key->k_next = kp->k_next;
                    kp->k_next = key;
                }
                else {
                    key->k_next = db_keys[i];
                    db_keys[i] = key;
                }
                break;
            }
            else if (kn == 0) {
                kx->k_next = key;
                break;
            }
        }
        return (true);
    }


    // Return the first key found that matches the prefix, case insensitive.
    //
    const sKey *
    sKeyDb::find_key(const char *pfx)
    {
        if (!pfx || !isalpha(*pfx))
            return (0);
        char buf[256];
        while (*pfx) {
            char *s = buf;
            while (*pfx) {
                *s++ = isupper(*pfx) ? tolower(*pfx) : *pfx;
                pfx++;
            }
            *s = 0;
        }

        int i = buf[0] - 'a';
        if (!db_keys[i])
            return (0);
        for (sKey *kx = db_keys[i]; kx; kx = kx->k_next) {
            if (lstring::prefix(kx->k_key, buf))
                return (kx);
        }
        return (0);
    }


    void
    sKeyDb::dump(FILE *fp)
    {
        for (int i = 0; i < 26; i++) {
            for (sKey *kx = db_keys[i]; kx; kx = kx->k_next) {
                fprintf(fp, "%-6s %d %d %d %d %-8s %-8s\n", kx->k_key,
                    kx->k_min_nodes, kx->k_max_nodes,
                    kx->k_num_devs, kx->k_val_only,
                    kx->k_ndev ? kx->k_ndev : "",
                    kx->k_pdev ? kx->k_pdev : "");
            }
        }
    }
    // End of sKeyDb functions.
}

