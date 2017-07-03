
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
 $Id: fio_chd_write.cc,v 1.35 2014/11/05 05:43:34 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_chd.h"
#include "fio_cvt_base.h"
#include "fio_oasis.h"
#include "fio_cgd.h"
#include "timedbg.h"
#include "coresize.h"
#include "filestat.h"


//-----------------------------------------------------------------------------
// sCHDout functions
// This is a class to save a cCHD to a file.
//-----------------------------------------------------------------------------

// Parse and run the command given.
// Args: -i archive_file -o saved_chd_file [-g] [-c]
//  -g  Include geometry data in file.
//  -c  Don't gzip compress.
//
bool
cFIO::WriteCHDfile(const char *string)
{
    if (!string) {
        Errs()->add_error("WriteCGDfile: null command string.");
        return (false);
    }
    while (isspace(*string))
        string++;
    if (!*string) {
        Errs()->add_error("WriteCGDfile: empty command string.");
        return (false);
    }

    char *ifname = 0;
    char *ofname = 0;
    bool incl_geom = false;
    bool no_compr = false;

#define matching(s) lstring::cieq(tok, s)

    bool ret = true;
    char *tok;
    while ((tok = lstring::gettok(&string)) != 0) {
        const char *msg = "WriteCHDfile: missing %s argument.";

        if (matching("-i")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-i");
                ret = false;
                break;
            }
            ifname = tok;
            tok = 0;
        }
        else if (matching("-o")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-o");
                ret = false;
                break;
            }
            ofname = tok;
            tok = 0;
        }
        else if (matching("-c"))
            no_compr = true;
        else if (matching("-g"))
            incl_geom = true;
        else {
            Errs()->add_error("WriteCGDfile: unknown token %s.", tok);
            delete [] tok;
            ret = false;
            break;
        }
        delete [] tok;
    }

    if (ret && !ifname) {
        Errs()->add_error("WriteCHDfile: no source file name (-i) given.");
        ret = false;
    }
    if (ret && !ofname) {
        Errs()->add_error("WriteCHDfile: no output file name (-o) given.");
        ret = false;
    }

    Tdbg()->start_timing("CHD create");
    double m0 = have_coresize_metric() ? coresize() : 0.0;

    cCHD *chd = 0;
    if (ret) {
        char *realname;
        FILE *fp = POpen(ifname, "r", &realname);
        if (fp) {
            FileType ft = FIO()->GetFileType(fp);
            fclose(fp);
            if (IsSupportedArchiveFormat(ft))
                chd = NewCHD(realname, ft, Physical, 0);
            else {
                Errs()->add_error(
                    "WriteCHDfile: input file has unknown format.");
                ret = false;
            }
        }
        delete [] realname;

        if (!chd) {
            Errs()->add_error("WriteCHDfile: could not obtain CHD for input.");
            ret = false;
        }
    }

    Tdbg()->stop_timing("CHD create");

    if (have_coresize_metric() && Tdbg()->is_active())
        printf("mem use %gMb\n", (coresize() - m0)/1000.0);

    Tdbg()->start_timing("Write CHD file");

    if (ret) {
        unsigned int f = 0;
        if (incl_geom)
            f |= CHD_WITH_GEOM;
        if (no_compr)
            f |= CHD_NO_GZIP;
        sCHDout out(chd);
        ret = out.write(ofname, f);
    }

    Tdbg()->stop_timing("Write CHD file");

    delete [] ifname;

#ifdef READ_TEST
    delete chd;
    if (have_coresize_metric() && Tdbg()->is_active())
        printf("mem use after deleting CHD %gMb\n", (coresize() - m0)/1000.0);
    Tdbg()->start_timing("Read CHD file");
    {
        sCHDin in;
        chd = in.read(ofname);
        if (!chd)
            printf("%s\n", CD()->Errs.get_error());
    }
    Tdbg()->stop_timing("Read CHD file");
    if (have_coresize_metric() && Tdbg()->is_active())
        printf("mem use %gMb\n", (coresize() - m0)/1000.0);
#endif

    delete chd;
    delete [] ofname;
    if (have_coresize_metric() && Tdbg()->is_active())
        printf("mem use after deleting CHD %gMb\n", (coresize() - m0)/1000.0);
    return (true);
}


sCHDout::sCHDout(cCHD *chd)
{
    co_chd = chd;
    co_fp = 0;
    co_zio = 0;
    co_nmtab = 0;
    co_attab = 0;
    co_nmidx = 1;
    co_magic = 0;
    co_mode = Physical;
}


sCHDout::~sCHDout()
{
    delete co_zio;
    delete co_nmtab;
    delete co_attab;
    if (co_fp)
        fclose(co_fp);
}


inline bool
sCHDout::write_char(int c)
{
    if (co_zio) {
        if (co_zio->zio_putc(c) < 0) {
            Errs()->add_error("write_char: write failed!");
            return (false);
        }
    }
    else if (putc(c, co_fp) == EOF) {
        Errs()->add_error("write_char: write failed!");
        return (false);
    }
    return (true);
}


// Write the CHD into fname.
//
bool
sCHDout::write(const char *fname, unsigned int flags)
{
    if (!co_chd) {
        Errs()->add_error("write: null CHD pointer.");
        return (false);
    }
    if (!fname || !*fname) {
        Errs()->add_error("write: null or empty file name.");
        return (false);
    }
    if (!filestat::create_bak(fname)) {
        Errs()->add_error(filestat::error_msg());
        Errs()->add_error("write: error backing up file.");
        return (false);
    }
    co_fp = fopen(fname, "wb");
    if (!co_fp) {
        Errs()->add_error("write: failed to open %s for output.", fname);
        return (false);
    }
    char *header = magic_string(flags);

    co_mode = Physical;

    if (!init_tables())
        return (false);

    if (!write_n_string(header))
        return (false);

    if (co_magic >= 6) {
        co_zio = zio_stream::zio_open(co_fp, "w");
        if (!co_zio)
            return (false);
    }

    if (!write_n_string(co_chd->filename()))
        return (false);
    if (!write_unsigned(co_chd->filetype()))
        return (false);
    // NOTUSED
    if (!write_real(1.0))
        return (false);
    if (!write_info())  // new for magic 1
        return (false);
    if (co_magic >= 4) {
        if (!write_alias())
            return (false);
    }
    if (!write_tables())
        return (false);

    namegen_t gen(co_chd->nameTab(Physical));
    symref_t *p;
    while ((p = gen.next()) != 0) {
        if (!write_symref(p))
            return (false);
    }

    // List termination, the reader must look for a symbol number 0.
    if (!write_unsigned(0))
        return (false);

    // Stop compressing.
    if (co_zio) {
        delete co_zio;
        co_zio = 0;
    }
    fflush(co_fp);

    if (flags & CHD_WITH_GEOM) {
        // We are concatenating cCGD data.
        bool ret = false;
        if (co_chd->hasCgd()) {
            // Has a CGD linked, write it out.
            cCGD *cgd = co_chd->getCgd();
            ret = cgd->write(co_fp, false);
            // write closes co_fp
        }
        else {
#ifdef MEM_DEBUG
printf("before geom write %g\n", coresize());
#endif
            cCGD::set_chd_out(this);
            // This will write data to the file as it is being
            // read from the source.
            cCGD *cgd = FIO()->NewCGD(0, 0, CGDmemory, co_chd);
            cCGD::set_chd_out(0);
            // The CGD produced in this manner is bogus, data have
            // been freed, but tables remain.
#ifdef MEM_DEBUG
printf("after geom write %g\n", coresize());
#endif
            ret = write_unsigned(CGD_END_REC);
            fclose(co_fp);
            if (!cgd)
                ret = false;
            delete cgd;
#ifdef MEM_DEBUG
printf("after table free %g\n", coresize());
#endif
        }
        co_fp = 0;
        return (ret);
    }

    if (co_chd->nameTab(Electrical)) {
        co_mode = Electrical;

        if (!init_tables())
            return (false);
        if (!write_n_string("ELECTRICAL"))
            return (false);
        if (!write_info())
            return (false);
        if (!write_tables())
            return (false);

        namegen_t egen(co_chd->nameTab(Electrical));
        while ((p = egen.next()) != 0) {
            if (!write_symref(p))
                return (false);
        }

        // List termination, the reader must look for a symbol number 0.
        if (!write_unsigned(0))
            return (false);
    }

    fclose(co_fp);
    co_fp = 0;
    return (true);
}


// Callback, when creating CGD data file.
//
bool
sCHDout::write_cell_record(cgd_cn_t *cn)
{
    if (!write_unsigned(CGD_CNAME_REC))
        return (false);
    if (!write_n_string(cn->tab_name()))
        return (false);
    return (true);
}


// Callback, when creating CGD data file.
//
bool
sCHDout::write_layer_record(cgd_lyr_t *lyr)
{
    if (!write_unsigned(CGD_LNAME_REC))
        return (false);
    if (!write_n_string(lyr->tab_name()))
        return (false);

    if (!write_unsigned64(lyr->get_csize()))
        return (false);
    if (!write_unsigned64(lyr->get_usize()))
        return (false);
    size_t size = lyr->get_csize();
    if (!size)
        size = lyr->get_usize();

    uint64_t posn = large_ftell(co_fp);
    if (fwrite(lyr->get_data(), 1, size, co_fp) != size)
        return (false);

    // Save the file offset, the data should be freed.
    delete [] lyr->get_data();
    lyr->set_offset(posn);
    return (true);
}


// Compose the file header string.
// Magic 2 is the ancient format containing cref lists.
// Magic 3 is the same as Magic 2, but instance lists are compressed.
// Magic 4 is the same as Magic 3, but includes the alias record.
// Magic 5 allows chained instance lists.
// Magic 6 allows gzip compression and geometry (CGD) records.
//
char *
sCHDout::magic_string(unsigned int flags)
{
    static char buf[24];

    flags &= (CHD_WITH_GEOM | CHD_NO_GZIP);

    if (flags == CHD_NO_GZIP) {
        // No compression, no geometry: MAGIC 5.
        co_magic = 5;
        sprintf(buf, "%s%d", MAGIC_STRING, 5);
    }
    else {
        // MAGIC 6
        co_magic = 6;
        if (flags & CHD_WITH_GEOM)
            // Add a suffix "+1", the integer may be used
            // as a geometry revision code in future releases.
            sprintf(buf, "%s%d+1", MAGIC_STRING, 6);
        else
            sprintf(buf, "%s%d", MAGIC_STRING, 6);
    }
    return (buf);
}


// Initialize the tables used during the write:  the symbol name table
// that has a symbol number as a data element which is used in the
// structures that follow, and a numeric-key table for the instance
// attributes.
//
bool
sCHDout::init_tables()
{
    if (!co_chd) {
        Errs()->add_error("init_tables: null CHD pointer.");
        return (false);
    }

    if (co_nmtab)
        co_nmtab->clear();
    if (co_attab)
        co_attab->clear();
    co_nmidx = 1;

    nametab_t *tab = co_chd->nameTab(co_mode);
    namegen_t gen(tab);
    symref_t *p;
    while ((p = gen.next()) != 0) {
        name_index(p->get_name());
        crgen_t cgen(tab, p);
        const cref_o_t *c;
        while ((c = cgen.next()) != 0) {
            if (!save_ticket(c->attr)) {
                symref_t *cp = tab->find_symref(c->srfptr);
                Errs()->add_error(
                    "init_tables: instance of %s in %s, bad ticket.",
                    cp ? cp->get_name()->string() : "UNRESOLVED",
                    p->get_name()->string());
                return (false);
            }
        }
    }
    return (true);
}


// Return (assigned if necessary) the symbol number associated with
// the passed symbol name.  The symbol number is 1 or larger, 0 is
// used as a special case to terminate cref_t lists.
//
// The name is in cCD::CellNameTable, or otherwise must not be volatile!
//
unsigned int
sCHDout::name_index(CDcellName name)
{
    if (!co_nmtab)
        co_nmtab = new SymTab(false, false);
    void *ret = co_nmtab->get(name->string());
    if (ret == ST_NIL) {
        co_nmtab->add(name->string(), (void*)(long)co_nmidx, false);
        co_nmidx++;
        return (co_nmidx - 1);
    }
    return ((unsigned int)(long)ret);
}


// Map symref ticket to new value.
//
bool
sCHDout::map(ticket_t *ptkt)
{
    if (!ptkt)
        return (false);
    nametab_t *tab = co_chd->nameTab(co_mode);
    symref_t *p = tab->find_symref(*ptkt);
    if (!p) {
        Errs()->add_error(
            "write_symref: symref for ticket %d not in table.", *ptkt);
        return (false);
    }
    *ptkt = name_index(p->get_name());
    return (true);
}


// Record the attribute tickets seen in the table.
//
bool
sCHDout::save_ticket(ticket_t t)
{
    if (co_magic >= 3 && t < 16)
        // no need to record these
        return (true);

    if (!co_attab)
        co_attab = new SymTab(false, false);
    void *ret = co_attab->get(t);
    if (ret == ST_NIL)
        co_attab->add(t, 0, false);
    return (true);
}


// Write the info struct to the file.
//
bool
sCHDout::write_info()
{
    cv_info *cv = co_chd->pcInfo(co_mode);
    if (!write_unsigned(cv ? cv->total_records() : 0))
        return (false);
    if (!write_unsigned(cv ? cv->total_cells() : 0))
        return (false);
    if (!write_unsigned(cv ? cv->total_boxes() : 0))
        return (false);
    if (!write_unsigned(cv ? cv->total_polys() : 0))
        return (false);
    if (!write_unsigned(cv ? cv->total_wires() : 0))
        return (false);
    if (!write_unsigned(cv ? cv->total_vertices() : 0))  // new for magic 2
        return (false);
    if (!write_unsigned(cv ? cv->total_labels() : 0))
        return (false);
    if (!write_unsigned(cv ? cv->total_srefs() : 0))
        return (false);
    if (!write_unsigned(cv ? cv->total_arefs() : 0))
        return (false);
    if (cv) {
        stringlist *s0 = cv->layers();
        for (stringlist *s = s0; s; s = s->next) {
            if (!write_n_string(s->string)) {
                stringlist::destroy(s0);
                return (false);
            }
        }
        stringlist::destroy(s0);
    }
    // Write end indicator for the layer name entries.  Reader must
    // look for null byte leading n-string.
    if (!write_char(0))
        return (false);
    return (true);
}


bool
sCHDout::write_alias()
{
    // First, the symref flags.
    // NOTUSED
    if (!write_unsigned(0xff))
        return (false);

    // Followed by alias flags and data.
    const cv_alias_info *ai = co_chd->aliasInfo();
    if (ai) {
        if (!write_unsigned(ai->flags()))
            return (false);
        if (ai->flags() & CVAL_PREFIX) {
            if (!write_n_string(ai->prefix()))
                return (false);
        }
        if (ai->flags() & CVAL_SUFFIX) {
            if (!write_n_string(ai->suffix()))
                return (false);
        }
    }
    else {
        if (!write_unsigned(0))
            return (false);
    }
    return (true);
}


// Write the tables to the file.
//
bool
sCHDout::write_tables()
{
    SymTabGen nmgen(co_nmtab);
    SymTabEnt *h;
    while ((h = nmgen.next()) != 0) {
        if (!write_n_string(h->stTag))
            return (false);
        if (!write_unsigned((unsigned int)(long)h->stData))
            return (false);
    }

    // Write end indicator for the name table entries.  Reader must
    // look for null byte leading n-string.
    if (!write_char(0))
        return (false);

    SymTabGen atgen(co_attab);
    while ((h = atgen.next()) != 0) {
        ticket_t t = (ticket_t)(long)h->stTag;
        if (!write_unsigned(t))
            return (false);
        CDattr at;
        if (!CD()->FindAttr(t, &at)) {
            Errs()->add_error(
                "write_tables: no attributes for ticket! (%d).", t);
            return (false);
        }
        if (!write_attr(&at))
            return (false);
    }

    // Write end indicator for the attribute table entries.  Reader
    // must look for a ticket = 0 with flags = 0xff;
    if (!write_char(0))
        return (false);
    if (!write_char(0xff))
        return (false);

    return (true);
}


// Write a record for the symref_t.  This will be variable length
// depending on the flags.  The cref_t list is written as well.
//
bool
sCHDout::write_symref(symref_t *p)
{
    if (!write_unsigned(name_index(p->get_name())))
        return (false);
    if (!write_unsigned64(p->get_offset()))
        return (false);

    // OR-in the compatibility flags.
    if (!write_unsigned((p->get_flags() & CVio_mask) | (CVx_srf | CVx_bb)))
        return (false);

    if (p->get_flags() & CVcif) {
        if (!write_signed(p->get_num()))
            return (false);
    }

    // has CVx_bb
    const BBox *BB = p->get_bb();
    if (!write_signed(BB->left))
        return (false);
    if (!write_signed(BB->bottom))
        return (false);
    if (!write_signed(BB->right))
        return (false);
    if (!write_signed(BB->top))
        return (false);

    // has CVx_sref
    // New for MAGIC 3, write out the compresed instance stream.
    nametab_t *tab = co_chd->nameTab(co_mode);
    if (p->get_cmpr()) {
        // We have to map the srfptr to the local value.

        crgen_t gen(tab, p);
        ticket_t ntkt = 0;
        for (;;) {
            unsigned int cnt;
            bool chained;
            unsigned char *str = gen.next_remap(ntkt, this, 0, &cnt, &chained);
            if (!str)
                return (false);
            ticket_t ttkt = 0;
            if (chained) {
                unsigned char *s = str + cnt - 4;
                ttkt = s[0] | (s[1] << 8) | (s[2] << 16) | (s[3] << 24);
                s[0] = 0;
                s[1] = 0;
                s[2] = 0;
                s[3] = 0;
            }
            if (!write_n_string((const char*)str, cnt))
                return (false);
            delete [] str;
            if (!ttkt)
                break;
            ntkt = ttkt;
        }
    }
    else {
        // Note: this is still the default if no instances.

        for (ticket_t t = p->get_crefs(); ; t++) {
            cref_t *c = tab->find_cref(t);
            if (!c)
                break;
            if (!write_cref(c))
                return (false);
            if (c->last_cref())
                break;
        }
        // Write cref_t list termination.  The reader must look for a
        // symbol number 0.
        if (!write_unsigned(0))
            return (false);
    }
    return (true);
}


// Write an instance record.
//
bool
sCHDout::write_cref(cref_t *c)
{
    symref_t *cp = co_chd->nameTab(co_mode)->find_symref(c->refptr());
    if (!cp) {
        Errs()->add_error(
            "write_cref: symref for ticket %d not in table.",
            c->refptr());
        return (false);
    }
    if (!write_unsigned(name_index(cp->get_name())))
        return (false);
    if (!write_unsigned(c->data()))
        return (false);
    if (!write_signed(c->pos_x()))
        return (false);
    if (!write_signed(c->pos_y()))
        return (false);
    return (true);
}


// Write an attributes record, which is variable length set by the
// first byte, which contains flags.
//
bool
sCHDout::write_attr(const CDattr *at)
{
    if (co_magic >= 3) {
        unsigned char c = 0;
        if (at->nx > 1)
            c |= MAGIC3_NX;
        if (at->ny > 1)
            c |= MAGIC3_NY;
        if (at->magn != 1.0)
            c |= MAGIC3_MG;
        c |= at->encode_transform() << 4;
        if (!write_char(c))
            return (false);

        if (at->nx > 1) {
            if (!write_unsigned(at->nx))
                return (false);
            if (!write_signed(at->dx))
                return (false);
        }
        if (at->ny > 1) {
            if (!write_unsigned(at->ny))
                return (false);
            if (!write_signed(at->dy))
                return (false);
        }
        if (c & MAGIC3_MG)
            if (!write_real(at->magn))
                return (false);
    }
    else {
        char fl = 0;
        if (at->magn != 1.0)
            fl |= 1;
        if (at->nx > 1 || at->ny > 1)
            fl |= 2;

        if (!write_char(fl))
            return (false);
        if (!write_char(at->ax))
            return (false);
        if (!write_char(at->ay))
            return (false);
        if (!write_char(at->refly))
            return (false);
        if (fl & 1) {
            if (!write_real(at->magn))
                return (false);
        }
        if (fl & 2) {
            if (!write_unsigned(at->nx))
                return (false);
            if (!write_unsigned(at->ny))
                return (false);
            if (!write_signed(at->dx))
                return (false);
            if (!write_signed(at->dy))
                return (false);
        }
    }
    return (true);
}


// Write an unsigned integer.
//
bool
sCHDout::write_unsigned(unsigned i)
{
    unsigned char b = i & 0x7f;
    i >>= 7;        // >> 7
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 14
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 21
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 28
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0xf;    // 4 bits left
    return (write_char(b));
}


// Write an unsigned 64-bit integer.
//
bool
sCHDout::write_unsigned64(int64_t i)
{
    unsigned char b = i & 0x7f;
    i >>= 7;        // >> 7
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 14
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 21
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 28
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 35
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 42
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 49
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 56
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 63
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x1;    // 1 bit left
    return (write_char(b));
}


// Write a signed integer.
//
bool
sCHDout::write_signed(int i)
{
    bool neg = false;
    if (i < 0) {
        neg = true;
        i = -i;
    }
    unsigned char b = i & 0x3f;
    b <<= 1;
    if (neg)
        b |= 1;
    i >>= 6;        // >> 6
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 13
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 20
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0x7f;
    i >>= 7;        // >> 27
    if (!i)
        return (write_char(b));
    b |= 0x80;
    if (!write_char(b))
        return (false);
    b = i & 0xf;    // 4 bits left (sign bit excluded)
    return (write_char(b));
}


// Write a double-precision real.
//
bool
sCHDout::write_real(double val)
{
    // ieee double
    // first byte is lsb of mantissa
    union { double n; unsigned char b[sizeof(double)]; } u;
    u.n = 1.0;
    bool ret;
    if (u.b[0] == 0) {
        // machine is little-endian, write bytes in order
        u.n = val;
        write_char(u.b[0]);
        write_char(u.b[1]);
        write_char(u.b[2]);
        write_char(u.b[3]);
        write_char(u.b[4]);
        write_char(u.b[5]);
        write_char(u.b[6]);
        ret = write_char(u.b[7]);   
    }
    else {
        // machine is big-endian, write bytes in reverse order
        u.n = val;
        write_char(u.b[7]);
        write_char(u.b[6]);
        write_char(u.b[5]);
        write_char(u.b[4]);
        write_char(u.b[3]);
        write_char(u.b[2]);
        write_char(u.b[1]);
        ret = write_char(u.b[0]);
    }
    return (ret);
}


// Write an ascii text string.
//
bool
sCHDout::write_n_string(const char *string, unsigned int n)
{
    if (!string)
        return (false);
    unsigned int len = n ? n : strlen(string);
    if (!write_unsigned(len))
        return (false);
    const char *s = string;
    while (len--) {
        if (!write_char(*s))
            return (false);
        s++;
    }
    return (true);
}

