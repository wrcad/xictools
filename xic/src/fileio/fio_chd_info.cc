
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
 $Id: fio_chd_info.cc,v 1.20 2015/08/29 15:39:51 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_chd.h"
#include "fio_cvt_base.h"
#include <algorithm>


cvINFO
cCHD::infoMode(DisplayMode mode)
{
    if (mode == Physical)
        return (cv_info::savemode(c_phys_info));
    return (cv_info::savemode(c_elec_info));
}


// Return the total memory bytes used by this CHD.
//
size_t
cCHD::memuse()
{
    size_t base = sizeof(cCHD);
    if (c_ptab) {
        base += c_ptab->memuse();
        namegen_t gen(c_ptab);
        symref_t *p;
        while ((p = gen.next()) != 0)
            base += strlen(p->get_name()->string()) + 1;
    }
    if (c_etab) {
        // Assume here that cell names are already included from c_ptab.
        base += c_etab->memuse();
    }
    if (c_filename)
        base += strlen(c_filename) + 1;

    if (c_phys_info)
        base += sizeof(cv_info) + c_phys_info->memuse();
    if (c_elec_info)
        base += sizeof(cv_info) + c_elec_info->memuse();
    return (base);
}


namespace {
    // String tokens for prInfo flags.
    //
    struct infotok
    {
        infotok(const char *t, int f) { token = t; flag = f; }
        const char *token;
        int flag;
    };
}


char *
cCHD::prInfo(FILE *fp, DisplayMode mode, int dflags)
{
    char buf[256];
    cv_info *info = pcInfo(mode);
    sLstr lstr;
    if (!dflags)
        dflags = ~(FIO_INFO_ALLCELLS | FIO_INFO_INSTANCES | FIO_INFO_FLAGS |
            FIO_INFO_INSTCNTS | FIO_INFO_INSTCNTSP);
    bool state = false;

    {
        const char *s =
            mode == Physical ? "Mode: Physical\n" : "Mode: Electrical\n";
        if (fp)
            fputs(s, fp);
        else
            lstr.add(s);
    }

    if (dflags & FIO_INFO_FILENAME) {
        char *s = prFilename(0);
        if (s) {
            sprintf(buf, "%-16s %s\n", "FileName", s);
            delete [] s;
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
            state = true;
        }
    }
    if (dflags & FIO_INFO_FILETYPE) {
        char *s = prFiletype(0);
        if (s) {
            sprintf(buf, "%-16s %s\n", "FileType", s);
            delete [] s;
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
            state = true;
        }
    }
    if (dflags & FIO_INFO_UNIT) {
        char *s = prUnit(0, mode);
        if (s) {
            sprintf(buf, "%-16s %s\n", "Unit", s);
            delete [] s;
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
            state = true;
        }
    }
    if (dflags & FIO_INFO_ALIAS) {
        char *s = prAlias(0);
        if (s) {
            sprintf(buf, "%-16s %s\n", "Aliasing", s);
            delete [] s;
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
            state = true;
        }
    }
    if (dflags & FIO_INFO_RECCOUNTS) {
        if (info) {
            char *t = info->pr_records(0);
            if (t) {
                if (fp) {
                    if (state)
                        fprintf(fp, "\n");
                    fprintf(fp, "Records\n-------\n");
                    fputs(t, fp);
                }
                else {
                    if (state)
                        lstr.add_c('\n');
                    lstr.add(   "Records\n-------\n");
                    lstr.add(t);
                }
                delete [] t;
                state = true;
            }
        }
    }
    if (dflags & FIO_INFO_OBJCOUNTS) {
        if (info) {
            char *t = info->format_totals();
            if (t) {
                if (fp) {
                    if (state)
                        fprintf(fp, "\n");
                    fprintf(fp, "Contents\n--------\n");
                    fputs(t, fp);
                    fprintf(fp, "\n");
                }
                else {
                    if (state)
                        lstr.add_c('\n');
                    lstr.add(   "Contents\n--------\n");
                    lstr.add(t);
                    lstr.add_c('\n');
                }
                delete [] t;
                state = true;
            }
        }
    }
    if (dflags & FIO_INFO_DEPTHCNTS) {
        char *t = prDepthCounts(fp, mode, dflags & FIO_INFO_OFFSORT);
        if (t) {
            if (fp) {
                if (state)
                    fprintf(fp, "\n");
                fputs(t, fp);
            }
            else {
                if (state)
                    lstr.add_c('\n');
                lstr.add(t);
            }
            delete [] t;
            state = false;  // already has trailing newline
        }
    }
    if (dflags & FIO_INFO_ESTSIZE) {
        char *s = prEstSize(0);
        if (s) {
            sprintf(buf, "%-16s %s Mb\n", "EstSize", s);
            delete [] s;
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
            state = true;
        }
    }
    if (dflags & FIO_INFO_ESTCXSIZE) {
        char *s = prEstCxSize(0);
        if (s) {
            sprintf(buf, "%-16s %s Kb\n", "EstCxSize", s);
            delete [] s;
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
            nametab_t *ntab = nameTab(mode);
            if (ntab->has_compression()) {
                s = prCmpStats(0, mode);
                sprintf(buf, "%-16s (%s)\n", "", s);
                delete [] s;
                if (fp)
                    fputs(buf, fp);
                else
                    lstr.add(buf);
            }
            state = true;
        }
    }
    if (dflags & FIO_INFO_LAYERS) {
        char *s = prLayers(0, mode);
        if (s) {
            if (state) {
                if (fp)
                    fprintf(fp, "\n");
                else
                    lstr.add_c('\n');
            }

            bool needs_dump = false;
            sprintf(buf, "%-16s", "Layers");
            char *ls = s;
            char *tok;
            while ((tok = lstring::gettok(&ls)) != 0) {
                char *e = buf + strlen(buf);
                if (e - buf + strlen(tok) < 78) {
                    *e++ = ' ';
                    strcpy(e, tok);
                }
                else {
                    if (fp) {
                        fputs(buf, fp);
                        fputs("\n", fp);
                    }
                    else {
                        lstr.add(buf);
                        lstr.add_c('\n');
                    }
                    sprintf(buf, "%-16s", "");
                    e = buf + strlen(buf);
                    *e++ = ' ';
                    strcpy(e, tok);
                }
                needs_dump = true;
                delete [] tok;
            }
            if (needs_dump) {
                if (fp) {
                    fputs(buf, fp);
                    fputs("\n", fp);
                }
                else {
                    lstr.add(buf);
                    lstr.add_c('\n');
                }
            }
            delete [] s;
            state = true;
        }
    }
    if (dflags & FIO_INFO_UNRESOLVED) {
        syrlist_t *sy0 = listUnresolved(mode);
        if (sy0) {
            if (state) {
                if (fp)
                    fprintf(fp, "\n");
                else
                    lstr.add_c('\n');
            }
            if (fp)
                fputs("Unresolved Cells:\n", fp);
            else
                lstr.add("Unresolved Cells:\n");
            syrlist_t::sort(sy0, false);
            for (syrlist_t *sy = sy0; sy; sy = sy->next) {
                sprintf(buf, "%-16s\n", sy->symref->get_name()->string());
                if (fp)
                    fputs(buf, fp);
                else
                    lstr.add(buf);
            }
            syrlist_t::destroy(sy0);
            state = true;
        }
    }
    if (dflags & FIO_INFO_TOPCELLS) {
        if (state) {
            if (fp)
                fprintf(fp, "\n");
            else
                lstr.add_c('\n');
        }
        syrlist_t *sy0 = topCells(mode, dflags & FIO_INFO_OFFSORT);
        for (syrlist_t *sy = sy0; sy; sy = sy->next) {
            if (sy == sy0) {
                if (fp)
                    fputs("Top Level Cells:\n", fp);
                else
                    lstr.add("Top Level Cells:\n");
            }
            char *str = prCell(fp, sy->symref, dflags);
            if (str) {
                if (fp)
                    fputs(str, fp);
                else
                    lstr.add(str);
                delete [] str;
            }
        }
        syrlist_t::destroy(sy0);
        state = true;
    }
    if (dflags & FIO_INFO_ALLCELLS) {
        if (state) {
            if (fp)
                fprintf(fp, "\n");
            else
                lstr.add_c('\n');
        }
        syrlist_t *sy0 = listing(mode, dflags & FIO_INFO_OFFSORT);
        for (syrlist_t *sy = sy0; sy; sy = sy->next) {
            if (sy == sy0) {
                if (fp)
                    fputs("All Cells:\n", fp);
                else
                    lstr.add("All Cells:\n");
            }
            if (!sy->symref->get_defseen()) {
                sprintf(buf, "%-16s   (UNRESOLVED)\n",
                    sy->symref->get_name()->string());
                if (fp)
                    fputs(buf, fp);
                else
                    lstr.add(buf);
                continue;
            }
            char *str = prCell(fp, sy->symref, dflags);
            if (str) {
                if (fp)
                    fputs(str, fp);
                else
                    lstr.add(str);
                delete [] str;
            }
        }
        syrlist_t::destroy(sy0);
        state = true;
    }

    if (dflags & FIO_INFO_INSTCNTSP) {
        char *t = prInstanceCounts(fp, Physical, true);
        if (t) {
            if (fp) {
                if (state)
                    fprintf(fp, "\n");
                fputs(t, fp);
            }
            else {
                if (state)
                    lstr.add_c('\n');
                lstr.add(t);
            }
            delete [] t;
            state = true;
        }
    }
    else if (dflags & FIO_INFO_INSTCNTS) {
        char *t = prInstanceCounts(fp, Physical, false);
        if (t) {
            if (fp) {
                if (state)
                    fprintf(fp, "\n");
                fputs(t, fp);
            }
            else {
                if (state)
                    lstr.add_c('\n');
                lstr.add(t);
            }
            delete [] t;
            state = true;
        }
    }
    return (lstr.string_trim());
}


// Print some into about the listed cells.
//
char *
cCHD::prCells(FILE *fp, DisplayMode mode, int dflags, const stringlist *cnames)
{
    sLstr lstr;
    for (const stringlist *sl = cnames; sl; sl = sl->next) {
        char *s = prCell(fp, mode, dflags, sl->string);
        if (s) {
            if (fp)
                fputs(s, fp);
            else
                lstr.add(s);
        }
        delete [] s;
    }
    return (lstr.string_trim());
}


char *
cCHD::prCell(FILE *fp, DisplayMode mode, int dflags, const char *cname)
{
    sLstr lstr;
    char buf[256];
    symref_t *p = findSymref(CD()->CellNameTableFind(cname), mode);
    if (!p) {
        sprintf(buf, "%-16s  ERROR, cell not found.\n", cname);
        if (fp)
            fputs(buf, fp);
        else
            lstr.add(buf);
        return (lstr.string_trim());
    }
    if (!p->get_defseen()) {
        sprintf(buf,
            "%-16s  WARNING, cell referenced but not defined in file.\n",
            cname);
        if (fp)
            fputs(buf, fp);
        else
            lstr.add(buf);
        return (lstr.string_trim());
    }
    char *s = prCell(fp, p, dflags);
    if (s) {
        if (fp)
            fputs(s, fp);
        else
            lstr.add(s);
        delete [] s;
    }
    return (lstr.string_trim());
}


char *
cCHD::prCell(FILE *fp, symref_t *p, int dflags)
{
    if (!p)
        return (0);

    DisplayMode mode = p->mode();
    bool show_instances = dflags & FIO_INFO_INSTANCES;
    bool show_bbs = dflags & FIO_INFO_BBS;
    bool show_offs = dflags & FIO_INFO_OFFSET;

    sLstr lstr;
    char buf[256];
    sprintf(buf, "Name: %s\n", p->get_name()->string());
    if (fp)
        fputs(buf, fp);
    else
        lstr.add(buf);

    if (show_offs) {
#ifdef WIN32
        sprintf(buf, "Offset: %I64d\n", (long long)p->get_offset());
#else
        sprintf(buf, "Offset: %lld\n", (long long)p->get_offset());
#endif
        if (fp)
            fputs(buf, fp);
        else
            lstr.add(buf);
    }

    if (show_bbs && setBoundaries(p)) {
        const BBox *bbp = p->get_bb();
        if (bbp) {
            int ndgt = CD()->numDigits();
            sprintf(buf, "Bounding Box: %.*f,%.*f %.*f,%.*f\n",
                ndgt, MICRONS(bbp->left), ndgt, MICRONS(bbp->bottom),
                ndgt, MICRONS(bbp->right), ndgt, MICRONS(bbp->top));
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);
        }
    }

    cv_info *info = pcInfo(mode);
    if (info) {
        char *s = info->format_counts(p);
        if (s) {
            if (fp)
                fputs(s, fp);
            else
                lstr.add(s);
            delete [] s;
        }
    }

    if (show_instances) {
        nametab_t *ntab = nameTab(mode);
        crgen_t gen(ntab, p);
        const cref_o_t *c;
        while ((c = gen.next()) != 0) {
            symref_t *cp = ntab->find_symref(c->srfptr);
            sprintf(buf, "Subcell:  %-14s\n",
                cp ? cp->get_name()->string() : "UNRESOLVED");
            if (fp)
                fputs(buf, fp);
            else
                lstr.add(buf);

            if (show_bbs) {
                int ndgt = CD()->numDigits();
                sprintf(buf, "  Origin: %.*f,%.*f\n",
                    ndgt, MICRONS(c->tx), ndgt, MICRONS(c->ty));
                if (fp)
                    fputs(buf, fp);
                else
                    lstr.add(buf);

                char *e = lstring::stpcpy(buf, "  Placement Code:");
                strcpy(e, " none");

                CDattr at;
                if (!CD()->FindAttr(c->attr, &at))
                    e = lstring::stpcpy(e,
                        " ERROR: UNRESOLVED TRANSFORM TICKET!");
                else {
                    if (at.refly)
                        e = lstring::stpcpy(e, " MY");
                    else if (at.ax != 1 || at.ay != 0 || at.magn != 1.0 ||
                            at.nx > 1 || at.ny > 1)
                        e = lstring::stpcpy(e, " ");

                    if (at.ax != 1 || at.ay != 0) {
                        sprintf(e, "R%d,%d", at.ax, at.ay);
                        while (*e)
                            e++;
                    }
                    if (at.magn != 1.0) {
                        sprintf(e, "M%.6e", at.magn);
                        while (*e)
                            e++;
                    }
                    if (at.nx > 1 || at.ny > 1) {
                        sprintf(e, "A%d,%d,%.*f,%.*f", at.nx, at.ny,
                            ndgt, MICRONS(at.dx), ndgt, MICRONS(at.dy));
                        while (*e)
                            e++;
                    }
                }
                while (*e)
                    e++;
                *e++ = '\n';
                *e = 0;
                if (fp)
                    fputs(buf, fp);
                else
                    lstr.add(buf);
            }
        }
    }
    return (lstr.string_trim());
}


char *
cCHD::prDepthCounts(FILE *fp, DisplayMode mode, bool sort_by_offset)
{
    sLstr lstr;
    unsigned int array[CDMAXCALLDEPTH];
    const char *ifmt = "%4d   %u\n";
    const char *tfmt = "Cell Instance Depth Counts, under %s:\n";
    syrlist_t *s0 = topCells(mode, sort_by_offset);
    for (syrlist_t *s = s0; s; s = s->next) {
        if (!depthCounts(s->symref, array)) {
            const char *errstr = Errs()->get_error();
            char *msg = new char[strlen(errstr) + 20];
            sprintf(msg, "ERROR: %s\n\n", errstr);
            if (fp)
                fputs(msg, fp);
            else
                lstr.add(msg);
            delete [] msg;
            continue;
        }
        if (fp) {
            fprintf(fp, tfmt, s->symref->get_name());
            unsigned long tcnt = 0;
            for (int i = 0; array[i]; i++) {
                fprintf(fp, ifmt, i, array[i]);
                tcnt += array[i];
            }
            fprintf(fp, "Total Instance Count: %ld\n\n", tcnt);
        }
        else {
            char buf[256];
            sprintf(buf, tfmt, s->symref->get_name());
            lstr.add(buf);
            unsigned long tcnt = 0;
            for (int i = 0; array[i]; i++) {
                sprintf(buf, ifmt, i, array[i]);
                lstr.add(buf);
                tcnt += array[i];
            }
            sprintf(buf, "Total Instance Count: %ld\n\n", tcnt);
            lstr.add(buf);
        }
    }
    syrlist_t::destroy(s0);
    if (fp)
        return (0);
    return (lstr.string_trim());
}


char *
cCHD::prDepthCounts(FILE *fp, symref_t *p)
{
    if (!p)
        return (0);
    sLstr lstr;
    unsigned int array[CDMAXCALLDEPTH];
    const char *ifmt = "%4d   %u\n";
    const char *tfmt = "Cell Instance Depth Counts, under %s:\n";
    if (!depthCounts(p, array)) {
        const char *errstr = Errs()->get_error();
        char *msg = new char[strlen(errstr) + 20];
        sprintf(msg, "ERROR: %s\n", errstr);
        if (fp) {
            fputs(msg, fp);
            delete [] msg;
            return (0);
        }
        return (msg);
    }
    if (fp) {
        fprintf(fp, tfmt, p->get_name());
        unsigned long tcnt = 0;
        for (int i = 0; array[i]; i++) {
            fprintf(fp, ifmt, i, array[i]);
            tcnt += array[i];
        }
        fprintf(fp, "Total Instance Count: %ld\n", tcnt);
    }
    else {
        char buf[256];
        sprintf(buf, tfmt, p->get_name());
        lstr.add(buf);
        unsigned long tcnt = 0;
        for (int i = 0; array[i]; i++) {
            sprintf(buf, ifmt, i, array[i]);
            lstr.add(buf);
            tcnt += array[i];
        }
        sprintf(buf, "Total Instance Count: %ld\n", tcnt);
        lstr.add(buf);
        return (lstr.string_trim());
    }
    return (0);
}


namespace {
    struct icnt_t { symref_t *symref; unsigned long cnt; };

    inline bool instcmp(const icnt_t &i1, const icnt_t &i2)
    {
        return (strcmp(i1.symref->get_name()->string(),
            i2.symref->get_name()->string()) < 0);
    }
}


char *
cCHD::prInstanceCounts(FILE *fp, DisplayMode mode, bool print)
{
    syrlist_t *s0 = topCells(mode, true);

    char buf[256];
    SymTab *tab = instanceCounts(s0->symref);
    if (!tab) {
        syrlist_t::destroy(s0);
        const char *errstr = Errs()->get_error();
        char *msg = new char[strlen(errstr) + 20];
        sprintf(msg, "ERROR: %s\n", errstr);
        if (fp) {
            fputs(msg, fp);
            delete [] msg;
            return (0);
        }
        return (msg);
    }
    syrlist_t::destroy(s0);
    {
        unsigned long totcnt = 0;
        SymTabGen stgen(tab);
        SymTabEnt *ent;
        while ((ent = stgen.next()) != 0)
            totcnt += (unsigned long)ent->stData;
        if (fp)
            fprintf(fp, "Total Instances: %ld\n", totcnt);
        else
            sprintf(buf, "Total Instances: %ld\n", totcnt);
    }
    if (!print) {
        delete tab;
        if (fp)
            return (0);
        return (lstring::copy(buf));
    }

    icnt_t *ary = new icnt_t[tab->allocated() + 1];
    SymTabGen stgen(tab);
    SymTabEnt *ent;
    int n = 0;
    while ((ent = stgen.next()) != 0) {
        ary[n].symref = (symref_t*)ent->stTag;
        ary[n].cnt = (unsigned long)ent->stData;
        n++;
    }
    delete tab;
    std::sort(ary, ary + n, instcmp);

    sLstr lstr;
    if (fp)
        fputs("Instantiation Counts:\n", fp);
    else
        lstr.add("Instantiation Counts:\n");
    for (int i = 0; i < n; i++) {
        sprintf(buf, "%-26s %ld\n", ary[i].symref->get_name()->string(),
            ary[i].cnt);
        if (fp)
            fputs(buf, fp);
        else
            lstr.add(buf);
    }
    delete [] ary;
    if (fp)
        return (0);
    return (lstr.string_trim());
}


char *
cCHD::prFilename(FILE *fp)
{
    if (fp) {
        fputs(c_filename, fp);
        return (0);
    }
    return (lstring::copy(c_filename));
}


char *
cCHD::prFiletype(FILE *fp)
{
    const char *string = cFIO::TypeName(c_filetype);
    if (fp) {
        fputs(string, fp);
        return (0);
    }
    return (lstring::copy(string));
}


char *
cCHD::prAlias(FILE *fp)
{
    sLstr lstr;
    if (c_alias_info) {
        if (c_alias_info->prefix()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("prefix=");
            lstr.add(c_alias_info->prefix());
        }
        if (c_alias_info->suffix()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("suffix=");
            lstr.add(c_alias_info->suffix());
        }
        if (c_alias_info->auto_rename()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("auto_rename");
        }
        if (c_alias_info->to_lower()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("to_lower");
        }
        if (c_alias_info->to_upper()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("to_upper");
        }
        if (c_alias_info->rd_file()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("read_alias_file");
        }
        if (c_alias_info->gds_check()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("gds_chars");
        }
        if (c_alias_info->limit32()) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add("limit32");
        }
    }
    if (!lstr.string())
        lstr.add("none");
    if (fp) {
        fprintf(fp, "%s", lstr.string());
        return (0);
    }
    return (lstr.string_trim());
}


char *
cCHD::prUnit(FILE *fp, DisplayMode)
{
    double u = 0.0;
    if (c_header_info)
        u = c_header_info->unit();
    if (fp) {
        fprintf(fp, "%.5f", u);
        return (0);
    }
    char buf[256];
    sprintf(buf, "%.5e", u);
    return (lstring::copy(buf));
}


char *
cCHD::prLayers(FILE *fp, DisplayMode mode)
{
    cv_info *info = pcInfo(mode);
    if (!info)
        return (0);
    stringlist *s0 = info->layers();
    sLstr lstr;
    for (stringlist *s = s0; s; s = s->next) {
        if (lstr.string())
            lstr.add_c(' ');
        lstr.add(s->string);
    }
    stringlist::destroy(s0);
    if (fp) {
        fputs(lstr.string(), fp);
        return (0);
    }
    return (lstr.string_trim());
}


char *
cCHD::prCmpStats(FILE *fp, DisplayMode mode)
{
    unsigned int ncrefs;
    unsigned long br;
    nametab_t *ntab = nameTab(mode);
    ntab->cref_count(&ncrefs, &br);
    char buf[80];
    sprintf(buf, "Compression: refs=%d  bytes=%ld  ratio=%.3f",
        ncrefs, br, br/(double)(16*ncrefs));
    if (fp) {
        fputs(buf, fp);
        return (0);
    }
    return (lstring::copy(buf));
}


char *
cCHD::prEstCxSize(FILE *fp)
{
    size_t sz = memuse();
    char buf[64];
    sprintf(buf, "%.2f", sz/1000.0);
    if (fp) {
        fputs(buf, fp);
        return (0);
    }
    return (lstring::copy(buf));
}


char *
cCHD::prEstSize(FILE *fp)
{
    double sz = memuse();  // symbol names
    if (c_phys_info) {
        sz += c_phys_info->total_cells() * sizeof(CDs);
        sz += c_phys_info->total_boxes() * sizeof(CDo);
        sz += c_phys_info->total_polys() * sizeof(CDpo);
        sz += c_phys_info->total_wires() * sizeof(CDw);
        sz += c_phys_info->total_vertices() * sizeof(Point);
        // est. avg. label length is 16
        sz += c_phys_info->total_labels() * (sizeof(CDla) + 16);
        sz += c_phys_info->total_srefs() * sizeof(CDc);
        sz += c_phys_info->total_arefs() * sizeof(CDc);

        double nobjs = c_phys_info->total_boxes() +
            c_phys_info->total_polys() +
            c_phys_info->total_wires() +
            c_phys_info->total_labels() +
            c_phys_info->total_srefs() +
            c_phys_info->total_arefs();
        sz += nobjs * .3 * sizeof(RTelem);
    }
    if (c_elec_info) {
        sz += c_elec_info->total_cells() * sizeof(CDs);
        sz += c_elec_info->total_boxes() * sizeof(CDo);
        sz += c_elec_info->total_polys() * sizeof(CDpo);
        sz += c_elec_info->total_wires() * sizeof(CDw);
        sz += c_elec_info->total_vertices() * sizeof(Point);
        // est. avg. label length is 16
        sz += c_elec_info->total_labels() * (sizeof(CDla) + 16);
        sz += c_elec_info->total_srefs() * sizeof(CDc);
        sz += c_elec_info->total_arefs() * sizeof(CDc);

        double nobjs = c_elec_info->total_boxes() +
            c_elec_info->total_polys() +
            c_elec_info->total_wires() +
            c_elec_info->total_labels() +
            c_elec_info->total_srefs() +
            c_elec_info->total_arefs();
        sz += nobjs * .3 * sizeof(RTelem);
    }
    char buf[64];
    sprintf(buf, "%.2f", sz*1e-6);
    if (fp) {
        fputs(buf, fp);
        return (0);
    }
    return (lstring::copy(buf));
}


namespace {
    infotok info_tokens[] = {
        infotok("filename",     FIO_INFO_FILENAME),
        infotok("filetype",     FIO_INFO_FILETYPE),
        infotok("unit",         FIO_INFO_UNIT),
        infotok("alias",        FIO_INFO_ALIAS),
        infotok("reccounts",    FIO_INFO_RECCOUNTS),
        infotok("objcounts",    FIO_INFO_OBJCOUNTS),
        infotok("depthcnts",    FIO_INFO_DEPTHCNTS),
        infotok("estsize",      FIO_INFO_ESTSIZE),
        infotok("estcxsize",    FIO_INFO_ESTCXSIZE),
        infotok("layers",       FIO_INFO_LAYERS),
        infotok("unresolved",   FIO_INFO_UNRESOLVED),
        infotok("topcells",     FIO_INFO_TOPCELLS),
        infotok("allcells",     FIO_INFO_ALLCELLS),
        infotok("offsort",      FIO_INFO_OFFSORT),
        infotok("offset",       FIO_INFO_OFFSET),
        infotok("instances",    FIO_INFO_INSTANCES),
        infotok("bbs",          FIO_INFO_BBS),
        infotok("flags",        FIO_INFO_FLAGS),
        infotok("instcnts",     FIO_INFO_INSTCNTS),
        infotok("instcntsp",    FIO_INFO_INSTCNTSP),
        infotok("all",          -1),
        infotok(0,              0)
    };

    const char *comma = ",";
}


// Static function.
// Parse a flag string, return a flags integer that can be passed to
// prInfo.  The string can be a space or comma separated list of
// token names or hex integers.  If hex integers, they are all or'ed
// together.
//
int
cCHD::infoFlags(const char *s)
{
    int fl = 0;
    char *tok;
    while ((tok = lstring::gettok(&s, comma)) != 0) {
        if (isdigit(*tok)) {
            char *ss = tok;
            if (ss[0] == '0' && (ss[1] == 'x' || ss[1] == 'X'))
                ss += 2;
            int f = 0;
            if (sscanf(ss, "%x", &f) == 1)
                fl |= f;
        }
        else {
            for (int i = 0; ; i++) {
                if (!info_tokens[i].token)
                    break;
                if (lstring::cieq(info_tokens[i].token, tok)) {
                    fl |= info_tokens[i].flag;
                    break;
                }
            }
        }
        delete [] tok;
    }
    return (fl);
}
// End of cCHD functions.


//-----------------------------------------------------------------------------
// syrlist_t functions
// This is a linked list element for symrefs.

namespace {
    // Sort comparison function, alpha sort.
    //
    inline bool
    cmp_alp(const symref_t *s1, const symref_t *s2)
    {
        const char *n1 = s1->get_name()->string();
        const char *n2 = s2->get_name()->string();
        return (strcmp(n1, n2) < 0);
    }


    // Sort comparison function, increasing with offset.
    //
    inline bool
    cmp_off(const symref_t *s1, const symref_t *s2)
    {
        int64_t n1 = s1->get_offset();
        int64_t n2 = s2->get_offset();
        return (n1 < n2);
    }
}


// Static function.
// Sort the list, alpha by name, or by offset.
//
void
syrlist_t::sort(syrlist_t *thissy, bool by_offset)
{
    int cnt = 0;
    for (syrlist_t *s = thissy; s; s = s->next)
        cnt++;
    if (cnt <= 1)
        return;
    symref_t **ary = new symref_t*[cnt];
    cnt = 0;
    for (syrlist_t *s = thissy; s; s = s->next)
        ary[cnt++] = s->symref;
    if (by_offset)
        std::sort(ary, ary + cnt, cmp_off);
    else
        std::sort(ary, ary + cnt, cmp_alp);
    cnt = 0;
    for (syrlist_t *s = thissy; s; s = s->next)
        s->symref = ary[cnt++];
    delete [] ary;
}

