
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
 $Id: ext_tech.cc,v 5.57 2017/05/01 17:12:04 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "ext.h"
#include "ext_extract.h"
#include "ext_antenna.h"
#include "dsp_layer.h"
#include "cd_lgen.h"
#include "tech.h"
#include "tech_extract.h"
#include "errorlog.h"


// Parse all of the device template blocks found in the file, which
// is the "device_templates" file read on startup.  Presently, this
// can contain only device templates.
//
void
cExt::parseDeviceTemplates(FILE *techfp)
{
    if (!techfp)
        return;
    Tech()->BeginParse();
    while (Tech()->GetKeywordLine(techfp) != 0) {
        if (Tech()->Matching(Ekw.DeviceTemplate())) {
            const char *t = Tech()->InBuf();
            char *tname = lstring::gettok(&t);
            parseDeviceTemplate(techfp, tname);
            delete [] tname;
            continue;
        }
        // For now, just ignore anything else in the file.
    }
    Tech()->EndParse();
}


// Read the DeviceTemplate lines into a stringlist, which is saved in
// a hash table.
//
bool
cExt::parseDeviceTemplate(FILE *fp, const char *tname)
{
    if (!ext_devtmpl_tab)
        ext_devtmpl_tab = new SymTab(false, false);

    Tech()->BeginParse();
    stringlist *s0 = 0, *se = 0;
    const char *inbuf = Tech()->InBuf();
    while (Tech()->GetKeywordLine(fp, tVerbatim) != 0) {
        const char *t = inbuf;
        while (isspace(*t))
            t++;
        if (!strncasecmp(t, "end", 3)) {
            t += 3;
            while (isspace(*t))
                t++;
            if (!*t)
                break;
        }
        stringlist *sl = new stringlist(lstring::copy(inbuf), 0);
        if (!s0)
            s0 = se = sl;
        else {
            se->next = sl;
            se = sl;
        }
    }
    Tech()->EndParse();

    if (!tname || !*tname) {
        char *e = Tech()->SaveError("%s: missing name, block ignored.",
            Ekw.DeviceTemplate());
        Log()->WarningLogV(mh::Techfile, "%s\n", e);
        delete [] e;
        stringlist::destroy(s0);
        return (false);
    }

    if (!s0) {
        char *e = Tech()->SaveError("%s %s: block is empty, ignored.",
            Ekw.DeviceTemplate(), tname);
        Log()->WarningLogV(mh::Techfile, "%s\n", e);
        delete [] e;
        return (false);
    }

    SymTabEnt *ent = SymTab::get_ent(ext_devtmpl_tab, tname);
    if (ent) {
        // Name is already in use, replace text.
        stringlist::destroy((stringlist*)ent->stData);
        ent->stData = s0;
    }
    else
        ext_devtmpl_tab->add(lstring::copy(tname), s0, false);
    return (true);
}


// Parse a Device block.  Return an error message if error.
//
bool
cExt::parseDevice(FILE *fp, bool initialize)
{
    sDevDesc *d = new sDevDesc;
    if (!d->parse_device(fp, initialize)) {
        delete d;
        return (false);
    }

    sDevDesc *dold;
    char *errm = addDevice(d, &dold);
    if (errm) {
        delete d;
        Log()->WarningLogV(mh::Techfile, "%s %s: failed to add device.\n%s",
            Ekw.Device(), d->name()->stringNN(), errm);
        delete [] errm;
        return (false);
    }
    if (dold) {
        dold->set_next(ext_dead_devices);
        ext_dead_devices = dold;
    }
    invalidateGroups();
    return (true);
}


void
cExt::addMOS(const sMOSdev *m)
{
    stringlist *s0 = 0;
    if (m && m->name) {
        if (*m->name == 'p' || *m->name == 'P')
            s0 = findDeviceTemplate(EXT_PMOS_TEMPLATE);
        else if (*m->name == 'n' || *m->name == 'N')
            s0 = findDeviceTemplate(EXT_NMOS_TEMPLATE);
    }
    if (!s0)
        return;

    // Set up the arguments, by adding them to the tech variables
    // temporarily.  They will be applied during set expansion.
    Tech()->SetTechVariable("_1", m->name);
    Tech()->SetTechVariable("_2", m->base);
    Tech()->SetTechVariable("_3", m->poly);
    Tech()->SetTechVariable("_4", m->actv);
    Tech()->SetTechVariable("_5", m->well);

    sDevDesc *d = new sDevDesc;
    bool nogo = false;
    Tech()->BeginParse();

    // Recursively process the template lines, by expanding the
    // text which performs argument substitution, pushing the text
    // into the tech buffers, and calling this function again.

    for (stringlist *s = s0; s; s = s->next) {
        const char *str = s->string;
        char *kw = lstring::gettok(&str);
        if (!kw)
            continue;
        if (*kw == '#' || (kw[0] == '/' && kw[1] == '/')) {
            delete [] kw;
            continue;
        }
        Tech()->WriteBuf(kw, str, 0);
        Tech()->ExpandLine(0, tReadNormal);

        TCret tcret = d->parse_device_line();
        if (tcret != TCmatch) {
            if (tcret == TCnone) {
                char *e = Tech()->SaveError("Unrecognized keyword %s.", kw);
                Log()->WarningLogV(mh::Techfile,
                    "(In Template Expansion) %s\n", e);
                delete [] e;
            }
            else {
                Log()->WarningLogV(mh::Techfile,
                    "(In Template Expansion) %s\n", tcret);
                delete [] tcret;
                nogo = true;
            }
        }
        delete [] kw;
    }
    Tech()->EndParse();

    // Unset temporary variables.
    Tech()->ClearTechVariable("_1");
    Tech()->ClearTechVariable("_2");
    Tech()->ClearTechVariable("_3");
    Tech()->ClearTechVariable("_4");
    Tech()->ClearTechVariable("_5");

    if (!nogo) {
        if (!d->finalize_device(true))
            nogo = true;
    }
    if (nogo) {
        Log()->WarningLogV(mh::Techfile,
            "Device %s block ignored due to errors.\n",
            d->name () ? d->name()->stringNN() : "");
        delete d;
        return;
    }

    sDevDesc *dold;
    char *errm = addDevice(d, &dold);
    if (errm) {
        delete d;
        Log()->WarningLogV(mh::Techfile, "%s %s: failed to add device.\n%s",
            Ekw.Device(), d->name()->stringNN(), errm);
        delete [] errm;
        return;
    }
    if (dold) {
        dold->set_next(ext_dead_devices);
        ext_dead_devices = dold;
    }
    invalidateGroups();
}


void
cExt::addRES(const sRESdev*)
{
}


namespace {
    const char *sep =
        "#--------------------------------------------------------------"
        "----------------\n";
}


// Dump the device templates.
//
void
cExt::techPrintDeviceTemplates(FILE *techfp, bool tech_only)
{
    if (!ext_devtmpl_tab || ext_devtmpl_tab->allocated() == 0)
        return;

    const char *top = "# Device Block Templates\n";

    bool didhead = false;
    if (tech_only) {
        // Print only the templates read from the tech file.

        for (stringlist *sl = ext_tech_devtmpls; sl; sl = sl->next) {
            stringlist *lines = (stringlist*)SymTab::get(
                ext_devtmpl_tab, sl->string);
            if (lines) {
                if (!didhead) {
                    fputs(sep, techfp);
                    fputs(top, techfp);
                    didhead = true;
                }
                fprintf(techfp, "\n%s %s\n", Ekw.DeviceTemplate(), sl->string);
                for (stringlist *s = lines; s; s = s->next)
                    fprintf(techfp, "%s\n", s->string);
                fputs("End\n", techfp);
            }
        }
    }
    else {
        // Print all templates.

        stringlist *tnames = SymTab::names(ext_devtmpl_tab);
        stringlist::sort(tnames);
        for (stringlist *sl = tnames; sl; sl = sl->next) {
            if (!didhead) {
                fputs(sep, techfp);
                fputs(top, techfp);
                didhead = true;
            }
            stringlist *lines = (stringlist*)SymTab::get(
                ext_devtmpl_tab, sl->string);
            fprintf(techfp, "\n%s %s\n", Ekw.DeviceTemplate(), sl->string);
            for (stringlist *s = lines; s; s = s->next)
                fprintf(techfp, "%s\n", s->string);
            fputs("End\n", techfp);
        }
        stringlist::destroy(tnames);
    }
    if (didhead) {
        fputs(sep, techfp);
        fputc('\n', techfp);
    }
}


// Dump the device blocks.
//
void
cExt::techPrintDevices(FILE *techfp)
{
    const char *top = "# Device Blocks\n";

    if (ext_device_tab && ext_device_tab->allocated()) {
        fputs(sep, techfp);
        fputs(top, techfp);
        fputs("\n", techfp);

        stringlist *dnames = SymTab::names(ext_device_tab);
        stringlist::sort(dnames);
        for (stringlist *sl = dnames; sl; sl = sl->next) {
            sDevDesc *descs = (sDevDesc*)SymTab::get(
                ext_device_tab, sl->string);
            // The descs are already sorted.
            for (sDevDesc *d = descs; d; d = d->next())
                d->print(techfp);
        }
        stringlist::destroy(dnames);

        fputs(sep, techfp);
        fputc('\n', techfp);
    }
}
// End of cExt functions.


// Parse a device block, used when reading the tech file, and
// stand-alone when reading device block files.  Erros go to the
// logging system.
//
bool
sDevDesc::parse_device(FILE *fp, bool initialize)
{
    bool nogo = false;

    Tech()->BeginParse();
    Tech()->SetCmtType(tBlkDev, 0);
    while (Tech()->GetKeywordLine(fp) != 0) {

        TCret tcret = parse_device_line();
        if (tcret == TCmatch)
            continue;
        if (tcret == TCnone) {
            if (Tech()->Matching(Ekw.End())) {
                // Done
                break;
            }
            char *e = Tech()->SaveError(
                "Unknown keyword %s in Device %s block.\n",
                Tech()->KwBuf(), name()->stringNN());
            Log()->WarningLogV(mh::Techfile, "%s\n", e);
            delete [] e;
            continue;
        }
        Log()->WarningLogV(mh::Techfile, "%s\n", tcret);
        delete [] tcret;
        nogo = true;
    }
    Tech()->EndParse();

    if (!nogo) {
        if (!finalize_device(initialize))
            nogo = true;
    }
    if (nogo) {
        Log()->WarningLogV(mh::Techfile,
            "Device %s block ignored due to errors.\n",
            name () ? name()->stringNN() : "");
    }
    return (!nogo);
}


// Parse a device block line, returning a TCret (as defined for the
// Tech system).  The return is char* to avoid needing TCret defined
// in the extraction header file(s).  Returns are
// TCnone  : (constant)  No matching keyword.
// TCmatch : (constant)  Match was found and processed without error.
// char*   : (user must free) Error message.
//
char *
sDevDesc::parse_device_line()
{
    const char *line = Tech()->InBuf();
    if (Tech()->Matching(Ekw.Template())) {
        char *tname = lstring::gettok(&line);
        if (!tname) {
            return (Tech()->SaveError("Device %s: missing name.",
                Ekw.Template()));
        }
        stringlist *s0 = EX()->findDeviceTemplate(tname);
        if (!s0) {
            char *e = Tech()->SaveError("Device %s %s: not found or empty.",
                Ekw.Template(), tname);
            delete [] tname;
            return (e);
        }

        // Set up the arguments, by adding them to the tech variables
        // temporarily.  They will be applied during set expansion.
#define VFMT "_%d"
        char buf[16];
        int ac = 0;
        char *tok;
        while ((tok = lstring::getqtok(&line)) != 0) {
            ac++;
            sprintf(buf, VFMT, ac);
            Tech()->SetTechVariable(buf, tok);
            delete [] tok;
        }

        // Back up the tech buffers.
        char *kw_bak = lstring::copy(Tech()->KwBuf());
        char *in_bak = lstring::copy(Tech()->InBuf());

        // Recursively process the template lines, by expanding the
        // text which performs argument substitution, pushing the text
        // into the tech buffers, and calling this function again.

        for (stringlist *s = s0; s; s = s->next) {
            const char *str = s->string;
            char *kw = lstring::gettok(&str);
            if (!kw || *kw == '#')
                continue;
            Tech()->WriteBuf(kw, str, 0);
            Tech()->ExpandLine(0, tReadNormal);

            TCret tcret = parse_device_line();
            if (tcret != TCmatch) {
                if (tcret == TCnone) {
                    char *e = Tech()->SaveError("Unrecognized keyword %s.",
                        kw);
                    Log()->WarningLogV(mh::Techfile,
                        "(In Template Expansion) %s\n", e);
                    delete [] e;
                }
                else {
                    Log()->WarningLogV(mh::Techfile,
                        "(In Template Expansion) %s\n", tcret);
                    delete [] tcret;
                }
            }
            delete [] kw;
        }

        // Put back original text.
        Tech()->WriteBuf(kw_bak, in_bak, 0);
        delete [] kw_bak;
        delete [] in_bak;

        // Unset temporary variables.
        for (int i = 1; i <= ac; i++) {
            sprintf(buf, VFMT, i);
            Tech()->ClearTechVariable(buf);
        }

        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Name())) {
        // Name <name>
        char *nm = lstring::gettok(&line);
        if (!nm) {
            return (Tech()->SaveError("Device %s: missing name.",
                Ekw.Name()));
        }
        else {
            // The device name will match the name of a cell if it
            // is used, so use the cell name string table.

            d_name = CD()->CellNameTableAdd(nm);
            Tech()->SetCmtType(tBlkDev, nm);
            delete [] nm;
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Prefix())) {
        // Prefix <prefix>
        d_prefix = lstring::gettok(&line);
        if (!d_prefix) {
            return (Tech()->SaveError("Device %s %s: missing prefix.",
                name()->stringNN(), Ekw.Prefix()));
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Body())) {
        // Body <expression>
        if (!d_body.parseExpr(&line)) {
            return (Tech()->SaveError("Device %s %s: parse error.\n%s",
                name()->stringNN(), Ekw.Body(), Errs()->get_error()));
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Find())) {
        // Find <device_name[.prefix]>
        char *tok = lstring::gettok(&line);
        if (tok) {
            if (!d_finds)
                d_finds = new stringlist(tok, 0);
            else {
                stringlist *fend = d_finds;
                while (fend->next)
                    fend = fend->next;
                fend->next = new stringlist(tok, 0);
            }
        }
        else {
            return (Tech()->SaveError("Device %s %s: missing value.",
                name()->stringNN(), Ekw.Find()));
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.DevContact())) {
        // Contact <tname> <layername> <expression> <...>
        char *errm = sDevContactDesc::parse_contact(&line, this);
        if (errm) {
            char *e = Tech()->SaveError("Device %s %s: parse error.\n%s",
                name()->stringNN(), Ekw.Contact(), errm);
            delete [] errm;
            return (e);
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.BulkContact())) {
        // BulkContact <tname> <level> [<name> | <bloat> <layername>
        //   <expression>]
        char *errm = sDevContactDesc::parse_bulk_contact(&line, this);
        if (errm) {
            char *e = Tech()->SaveError("Device %s %s: parse error.\n%s",
                name()->stringNN(), Ekw.BulkContact(), errm);
            delete [] errm;
            return (e);
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Permute())) {
        // Permute name1 name2
        if (d_prmconts) {
            return (Tech()->SaveError("Device %s %s: permutes already given.",
                name()->stringNN(), Ekw.Permute()));
        }
        else {
            stringlist *pend = 0;
            char *tok;
            while ((tok = lstring::gettok(&line)) != 0) {
                if (!pend)
                    pend = d_prmconts = new stringlist(tok, 0);
                else {
                    pend->next = new stringlist(tok, 0);
                    pend = pend->next;
                }
            }
            if (!d_prmconts || !d_prmconts->next ||
                    d_prmconts->next->next) {
                stringlist::destroy(d_prmconts);
                d_prmconts = 0;
                return (Tech()->SaveError("Device %s %s: syntax error, "
                    "exactly two args required.",
                    name()->stringNN(), Ekw.Permute()));
            }
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Depth())) {
        int dep;
        if (*line == 'a' || *line == 'A')
            d_depth = CDMAXCALLDEPTH;
        else if (sscanf(line, "%d", &dep) == 1 && dep >= 0) {
            if (dep > CDMAXCALLDEPTH)
                dep = CDMAXCALLDEPTH;
            d_depth = dep;
        }
        else {
            return (Tech()->SaveError("Device %s %s: parse error, "
                "expecting integer or 'a'.",
                name()->stringNN(), Ekw.Depth()));
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Bloat())) {
        double blt;
        if (sscanf(line, "%lf", &blt) == 1)
            d_bloat = blt;
        else {
            return (Tech()->SaveError("Device %s %s: parse error, "
                "expecting real value.",
                name()->stringNN(), Ekw.Bloat()));
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Merge())) {
        char *tok = lstring::gettok(&line);
        if (!tok)
            d_flags |= EXT_DEV_MERGE_PARALLEL;
        else if (lstring::ciprefix("ns", tok) ||
                lstring::ciprefix("sn", tok))
            d_flags |= EXT_DEV_MERGE_SERIES;
        else if (*tok == 's' || *tok == 'S')
            d_flags |= (EXT_DEV_MERGE_PARALLEL | EXT_DEV_MERGE_SERIES);
        else {
            return (Tech()->SaveError("Device %s %s: parse error, "
                "unknown token.",
                name()->stringNN(), Ekw.Merge()));
        }
        delete [] tok;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Measure())) {
        if (!parse_measure(line)) {
            return (Tech()->SaveError("Device %s %s: parse error.",
                name()->stringNN(), Ekw.Measure()));
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.LVS())) {
        char *tok = lstring::gettok(&line);
        if (tok) {
            char *word = lstring::gettok(&line);
            if (!word)
                word = lstring::copy("");
            if (!set_lvs_word(tok, word)) {
                char *e = Tech()->SaveError("Device %s %s: measure %s "
                    "not found.",
                    name()->stringNN(), Ekw.LVS(), tok);
                delete [] tok;
                return (e);
            }
            delete [] tok;
        }
        else {
            return (Tech()->SaveError("Device %s %s: missing token.",
                name()->stringNN(), Ekw.LVS()));
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Spice())) {
        d_netline = lstring::copy(line);
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Cmput())) {
        d_netline1 = lstring::copy(line);
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Model())) {
        d_model = lstring::copy(line);
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Value())) {
        d_value = lstring::copy(line);
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Param())) {
        d_param = lstring::copy(line);
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Initc())) {
        // back compatability
        d_param = lstring::copy(line);
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.ContactsOverlap())) {
        d_flags |= EXT_DEV_CONTS_OVERLAP;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.SimpleMinDimen())) {
        d_flags |= EXT_DEV_SIMPLE_MINDIM;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.ContactMinDimen())) {
        d_flags |= EXT_DEV_CONTS_MINDGVN;
        char *tok = lstring::gettok(&line);
        if (!tok)
            d_flags |= EXT_DEV_CONTS_MINDIM;
        else {
            if (*tok == 'n' || *tok == 'N' || *tok == '0' || *tok == 'f' ||
                    *tok == 'F' || lstring::ciprefix("of", tok))
                ;
            else
                d_flags |= EXT_DEV_CONTS_MINDIM;
            delete [] tok;
        }
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.MOS())) {
        d_flags |= EXT_DEV_MOS;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.NotMOS())) {
        d_flags |= EXT_DEV_NOTMOS_GIVEN;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.NMOS())) {
        d_flags |= EXT_DEV_MOS;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.PMOS())) {
        d_flags |= EXT_DEV_MOS;
        d_flags |= EXT_DEV_PTYPE;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Ntype())) {
        d_flags |= EXT_DEV_NTYPE_GIVEN;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Ptype())) {
        d_flags |= EXT_DEV_PTYPE;
        return (TCmatch);
    }
    if (Tech()->Matching(Ekw.Device())) {
        // ignore this
        return (TCmatch);
    }
    return (TCnone);
}


bool
sDevDesc::finalize_device(bool initialize)
{
    bool nogo = false;
    if (initialize && !nogo) {
        if (!init()) {
            Log()->WarningLogV(mh::Techfile,
                "Initialization failed for Device %s block.\n%s\n",
                name()->stringNN(), Errs()->get_error());
            nogo = true;
        }
    }
    
    // If the keywords weren't explicitly given, set the MOS flag for
    // an 'm' prefix, and guess p/n from the first character of the
    // name.
    //
    if (!(d_flags & EXT_DEV_MOS)) {
        if (!(d_flags & EXT_DEV_NOTMOS_GIVEN) && d_prefix &&
                (*d_prefix == 'm' || *d_prefix == 'M'))
            d_flags |= EXT_DEV_MOS;
    }
    if (d_flags & EXT_DEV_MOS) {
        if (!(d_flags & EXT_DEV_PTYPE) && !(d_flags & EXT_DEV_NTYPE_GIVEN) &&
                d_name) {
            if (*d_name->string() == 'p' || *d_name->string() == 'P')
                d_flags |= EXT_DEV_PTYPE;
        }

        // Register the MOS device in the Antenna Params database.
        AP()->set_mos_name(d_name->string());
    }

    if (!nogo && merge_series()) {
        // Series merging of two-terminal permutable devices only.
        int cnt = 0;
        for (sDevContactDesc *c = d_contacts; c; c = c->next())
            cnt++;
        if (cnt != 2 || !d_prmconts)
            d_flags &= ~EXT_DEV_MERGE_SERIES;
    }
    return (!nogo);
}


// Print a device description, for generating the tech file.
//
void
sDevDesc::print(FILE *techfp)
{
    fprintf(techfp, "%s\n", Ekw.Device());
    Tech()->CommentDump(techfp, 0, tBlkDev, 0, Ekw.Device());
    fprintf(techfp, "%s %s\n", Ekw.Name(), name()->stringNN());
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Name());
    if (d_prefix)
        fprintf(techfp, "%s %s\n", Ekw.Prefix(), d_prefix);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Prefix());
    fprintf(techfp, "%s ", Ekw.Body());
    d_body.print(techfp);
    fprintf(techfp, "\n");
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Body());
    for (stringlist *f = d_finds; f; f = f->next) {
        fprintf(techfp, "%s %s\n", Ekw.Find(), f->string);
        Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Find());
    }
    if (overlap_conts())
        fprintf(techfp, "%s\n", Ekw.ContactsOverlap());
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(),
        Ekw.ContactsOverlap());
    for (sDevContactDesc *c = d_contacts; c; c = c->next()) {
        if (c->is_bulk()) {
            fprintf(techfp, "%s %s", Ekw.BulkContact(), c->name()->stringNN());
            if (c->level() == BC_defer) {
                char *expr = c->lspec()->string();
                fprintf(techfp, " %s %s %g %s %s\n", Ekw.defer(),
                    c->netname()->stringNN(), c->bulk_bloat(), c->lname(),
                    expr);
                delete [] expr;
            }
            else if (c->level() == BC_skip) {
                fprintf(techfp, " %s %s\n", Ekw.skip(),
                    c->netname()->stringNN());
            }
            else {
                char *expr = c->lspec()->string();
                fprintf(techfp, " %g %s %s\n", c->bulk_bloat(), c->lname(),
                    expr);
                delete [] expr;
            }
        }
        else {
            fprintf(techfp, "%s %s %s ", Ekw.Contact(), c->name()->stringNN(),
                c->lname());
            c->lspec()->print(techfp);
            if (c->multiple())
                fprintf(techfp, " ...\n");
            else
                fprintf(techfp, "\n");
            Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(),
                Ekw.Contact());
        }
    }
    if (d_prmconts) {
        fprintf(techfp, "%s", Ekw.Permute());
        for (stringlist *l = d_prmconts; l; l = l->next)
            fprintf(techfp, " %s", l->string);
        fprintf(techfp, "\n");
    }
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Permute());
    if (d_depth != 0)
        fprintf(techfp, "%s %d\n", Ekw.Depth(), d_depth);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Depth());
    if (d_bloat != 0.0)
        fprintf(techfp, "%s %g\n", Ekw.Bloat(), d_bloat);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Bloat());
    if (merge_parallel() || merge_series()) {
        if (merge_parallel() && merge_series())
            fprintf(techfp, "%s S\n", Ekw.Merge());
        else if (!merge_parallel() && merge_series())
            fprintf(techfp, "%s NS\n", Ekw.Merge());
        else
            fprintf(techfp, "%s\n", Ekw.Merge());
    }
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Merge());
    for (sMeasure *m = d_measures; m; m = m->next()) {
        m->print(techfp);
        Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(),
            Ekw.Measure());
    }
    for (sMeasure *m = d_measures; m; m = m->next()) {
        if (m->lvsword())
            fprintf(techfp, "%s %s %s\n", Ekw.LVS(), m->name(), m->lvsword());
        Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.LVS());
    }
    if (d_netline)
        fprintf(techfp, "%s %s\n", Ekw.Spice(), d_netline);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Spice());
    if (d_netline1)
        fprintf(techfp, "%s %s\n", Ekw.Cmput(), d_netline1);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Cmput());
    if (d_model)
        fprintf(techfp, "%s %s\n", Ekw.Model(), d_model);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Model());
    if (d_value)
        fprintf(techfp, "%s %s\n", Ekw.Value(), d_value);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Value());
    if (d_param)
        fprintf(techfp, "%s %s\n", Ekw.Param(), d_param);
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Param());
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.Initc());
    fprintf(techfp, "%s\n", Ekw.End());
    Tech()->CommentDump(techfp, 0, tBlkDev, name()->stringNN(), Ekw.End());
    fprintf(techfp, "\n");
}

