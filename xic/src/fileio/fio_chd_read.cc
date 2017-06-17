
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
 $Id: fio_chd_read.cc,v 1.37 2013/09/10 06:53:02 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_chd.h"
#include "fio_cvt_base.h"
#include "fio_cgd.h"
#include "fio_zio.h"
#include "cd_digest.h"


//-----------------------------------------------------------------------------
// sCHDin functions
// This is a class to retrieve a cCHD from a file.
//-----------------------------------------------------------------------------

ChdCgdType sCHDin::ci_def_cgd_type = CHD_CGDmemory;

sCHDin::sCHDin()
{
    ci_chd = 0;
    ci_fp = 0;
    ci_zio = 0;
    ci_nmtab = 0;
    ci_attab = 0;
    ci_nametab = 0;
    ci_flags = 0;
    ci_magic = 0;
    ci_mode = Physical;
    ci_nogo = false;
}


sCHDin::~sCHDin()
{
    delete ci_zio;
    delete ci_nmtab;
    delete ci_attab;
    if (ci_fp)
        fclose(ci_fp);
}


inline int
sCHDin::read_byte()
{
    int c;
    if (ci_zio)
        c = ci_zio->zio_getc();
    else
        c = getc(ci_fp);
    if (c == EOF) {
        ci_nogo = true;
        Errs()->add_error("read_byte: premature EOF");
        return (0);
    }
    return (c);
}


// The cgdtype can be CGDmemory or CGDfile, this determines the CGD
// type created if geometry records are found.
//
cCHD *
sCHDin::read(const char *fname, ChdCgdType cgdtype)
{
    if (!fname || !*fname) {
        Errs()->add_error("read: null or empty file name.");
        return (0);
    }
    ci_fp = fopen(fname, "rb");
    if (!ci_fp) {
        Errs()->add_error("read: failed to open %s for input.", fname);
        return (0);
    }

    int geo_rev = read_magic();
    if (geo_rev < 0) {
        Errs()->add_error("read: incorrect file type.");
        return (0);
    }

    if (ci_magic >= 6) {
        // Magic 6 and larger compress the physical records.
        ci_zio = zio_stream::zio_open(ci_fp, "r");
        if (!ci_zio)
            return (0);
    }

    ci_mode = Physical;
    bool ret = read_chd();

    if (ci_zio) {
        int v = ci_zio->stream().avail_in;  // give back buffered chars
        delete ci_zio;
        ci_zio = 0;
        large_fseek(ci_fp, -v, SEEK_CUR);
    }

    if (ret) {
        if (geo_rev >= 1) {
            // File contains physical geometry records.
            if (cgdtype == CHD_CGDmemory || cgdtype == CHD_CGDfile) {
                CgdType tp = (cgdtype == CHD_CGDfile ? CGDfile : CGDmemory);
                cCGD *cgd = new cCGD(0);
                if (!cgd->read(ci_fp, tp, false)) {
                    delete cgd;
                    cgd = 0;
                }
                if (cgd) {
                    // Save in the CD list.
                    char *dbname = CDcgd()->newCgdName();
                    CDcgd()->cgdStore(dbname, cgd);
                    delete [] dbname;

                    // Link to the CHD, and set the flag to free the CGD
                    // when unlinked.
                    cgd->set_free_on_unlink(true);
                    ci_chd->setCgd(cgd);
                }
                else {
                    Errs()->add_error("read: geometry read failed.");
                    delete ci_chd;
                    ci_chd = 0;
                }
            }
        }
        else {
            ci_mode = Electrical;
            ret = read_chd();
        }
    }

    if (ci_fp) {
        fclose(ci_fp);
        ci_fp = 0;
    }
    delete ci_nmtab;
    ci_nmtab = 0;
    delete ci_attab;
    ci_attab = 0;
    if (ci_nogo) {
        ci_nogo = false;
        delete ci_chd;
        ci_chd = 0;
    }
    cCHD *chd = ci_chd;
    ci_chd = 0;
    return (chd);
}


bool
sCHDin::check(const char *fname)
{
    if (!fname || !*fname)
        return (false);
    ci_fp = fopen(fname, "rb");
    if (!ci_fp)
        return (false);
    int ok = read_magic();
    fclose(ci_fp);
    ci_chd = 0;
    ci_fp = 0;
    ci_nmtab = 0;
    ci_attab = 0;
    ci_magic = 0;
    ci_nogo = false;
    return (ok >= 0);
}


// Check the file header and set co_magic to the file format number. 
// Return the geometry format number (currently 1) if geometry is
// included, 0 if no geometry, -1 if unrecognized header.
//
int
sCHDin::read_magic()
{
    char *string = read_n_string();
    if (!string)
        return (false);
    GCarray<char*> gc_string(string);
    // If '+N' is concatenated to magic string, N is a decimal integer
    // giving geometry format version..

    int geo_rev = 0;
    char *t = strchr(string, '+');
    if (t) {
        *t++ = 0;
        geo_rev = atoi(t);
    }

    unsigned int n = strlen(MAGIC_STRING);
    if (strncmp(string, MAGIC_STRING, n))
        return (-1);
    ci_magic = atoi(string + n);

    if (ci_magic > 6)
        return (-1);
    if (geo_rev && geo_rev != 1)
        return (-1);

    return (geo_rev);
}


bool
sCHDin::read_chd()
{
    ci_nametab = 0;
    ci_flags = 0;
    if (ci_mode == Physical) {
        char *fn = read_n_string();
        if (ci_nogo)
            return (false);
        GCarray<char*> gc_fn(fn);
        FileType ft = (FileType)read_unsigned();
        if (ci_nogo)
            return (false);
        double sc = read_real();
        if (ci_nogo)
            return (false);
        // NOTUSED
        // Non-unit scaling is no longer supported.
        if (fabs(sc - 1.0) > 1e-12) {
            Errs()->add_error("read_chd: non-unit scaling, not supported.");
            return (false);
        }
        ci_chd = new cCHD(fn, ft);
        if (ci_magic >= 1) {
            ci_chd->setPcInfo(read_info(), Physical);
            if (ci_nogo)
                return (false);
        }
        if (ci_magic >= 4) {
            if (!read_alias())
                return (false);
        }
    }
    else {
        if (!ci_chd)
            return (false);
        char *str = read_n_string();
        if (!str) {
            ci_nogo = false;  // EOF sets this
            return (true);
        }
        GCarray<char*> gc_str(str);
        if (strcmp(str, "ELECTRICAL")) {
            Errs()->add_error("read_chd: electrical data corrupt.");
            return (false);
        }
        ci_chd->setPcInfo(read_info(), Electrical);
        if (ci_nogo)
            return (false);
    }

    if (!read_tables())
        return (false);

    if (read_nametab())
        ci_chd->setNameTab(ci_nametab, ci_mode);
    if (ci_nogo)
        return (false);
    return (true);
}


bool
sCHDin::map(ticket_t *ptkt)
{
    if (!ptkt)
        return (false);
    CDcellName n = (CDcellName)ci_nmtab->get(*ptkt);
    if (n == (CDcellName)ST_NIL) {
        Errs()->add_error(
            "read_nametab: name index from instance not in table.");
        return (false);
    }
    symref_t *p = ci_nametab->get(n);
    if (!p) {
        ci_nametab->new_symref(n->string(), ci_mode, &p);
        ci_nametab->add(p);
        p->set_flags(ci_flags);
    }
    *ptkt = p->get_ticket();
    return (true);
}


bool
sCHDin::read_nametab()
{
    ci_nametab = new nametab_t(ci_mode);
    for (;;) {
        unsigned int ni = read_unsigned();
        if (ci_nogo)
            break;
        if (ni == 0)
            break;
        int64_t off = read_unsigned64();
        if (ci_nogo)
            break;
        CDcellName s = (CDcellName)ci_nmtab->get(ni);
        if (s == (CDcellName)ST_NIL) {
            Errs()->add_error(
                "read_nametab: name index not in table (%d).", ni);
            ci_nogo = true;
            break;
        }
        ci_flags = read_unsigned() & CVio_mask;
        if (ci_nogo)
            break;

        symref_t *p = ci_nametab->get(s);
        if (!p) {
            ci_nametab->new_symref(s->string(), ci_mode, &p,
                (ci_flags & CVcif));
            ci_nametab->add(p);
        }
        p->set_flags(ci_flags);
        p->set_offset(off);

        if (p->get_flags() & CVcif) {
            p->set_num(read_signed());
            if (ci_nogo)
                break;
        }
        // The CVx_xxx flags are currently always set.
        if (p->get_flags() & CVx_bb) {
            BBox BB;
            BB.left = read_signed();
            if (ci_nogo)
                break;
            BB.bottom = read_signed();
            if (ci_nogo)
                break;
            BB.right = read_signed();
            if (ci_nogo)
                break;
            BB.top = read_signed();
            if (ci_nogo)
                break;
            p->set_bb(&BB);
        }
        if (p->get_flags() & CVx_srf) {
            if (ci_magic >= 3) {
                // Using compressed instance lists.  This reads
                // segmented (magic 5) and non-segmented lists.

                // Note: if p has no instances, the CVcmpr flag might
                // not be set.

                ticket_t last_tkt = 0;
                unsigned int offs = 0;
                bool start = true;
                crgen_t gen(ci_nametab, 0);
                for (;;) {
                    unsigned int cnt;
                    unsigned char *str = (unsigned char*)read_n_string(&cnt);

                    gen.set(str);
                    bool chained;
                    unsigned char *nstr = gen.next_remap(0, this, ci_attab,
                        &cnt, &chained);
                    if (!nstr) {
                        ci_nogo = true;
                        break;
                    }

                    ticket_t t = ci_nametab->get_space(cnt);
                    if (!t) {
                        if (ci_nametab->is_full()) {
                            Errs()->add_error(
                            "Ticket allocation failed, internal block "
                            "limit %d reached in\nname table allocation.",
                            BDB_MAX);
                        }
                        else {
                            Errs()->add_error(
                                "Ticket allocation failure, request for 0 "
                                "bytes in name\ntable allocation.");
                        }
                        ci_nogo = true;
                        break;
                    }

                    if (last_tkt) {
                        unsigned char *ss =
                            ci_nametab->find_space(last_tkt);
                        ss += offs;
                        ss[0] = t & 0xff;
                        ss[1] = (t >> 8) & 0xff;
                        ss[2] = (t >> 16) & 0xff;
                        ss[3] = (t >> 24) & 0xff;
                        // Compress the bytes of long streams.
                        ci_nametab->save_space(last_tkt);
                    }
                    last_tkt = t;
                    offs = cnt - 4;
                    unsigned char *ss = ci_nametab->find_space(t);
                    memcpy(ss, nstr, cnt);
                    if (start) {
                        p->set_crefs(t);
                        start = false;
                    }
                    delete [] nstr;
                    delete [] str;

                    if (!chained) {
                        // Compress the bytes of long streams.
                        ci_nametab->save_space(t);
                        break;
                    }
                }
            }
            else {
                ticket_t cref_end = 0;
                for (;;) {
                    unsigned int ci = read_unsigned();
                    if (ci_nogo)
                        break;
                    if (!ci)
                        break;
                    cref_t *c;
                    ticket_t ctk = ci_nametab->new_cref(&c);

                    CDcellName n = (CDcellName)ci_nmtab->get(ci);
                    if (n == (CDcellName)ST_NIL) {
                        Errs()->add_error(
                        "read_nametab: name index from instance not in table.");
                        ci_nogo = true;
                        delete ci_nametab;
                        ci_nametab = 0;
                        return (false);
                    }
                    symref_t *pt = ci_nametab->get(n);
                    if (!pt) {
                        ci_nametab->new_symref(n->string(), ci_mode, &pt);
                        ci_nametab->add(pt);
                        pt->set_flags(ci_flags);
                    }
                    c->set_refptr(pt->get_ticket());

                    c->set_data(read_unsigned());
                    if (ci_nogo)
                        break;
                    ticket_t otkt = c->get_tkt();
                    void *xx = ci_attab->get(otkt);
                    if (xx == ST_NIL) {
                        Errs()->add_error(
                            "read_nametab: ticket not in table.");
                        ci_nogo = true;
                        break;
                    }

                    c->set_tkt((ticket_t)((unsigned int)(long)xx - 1));
                    if (ci_nogo)
                        break;
                    c->set_pos_x(read_signed());
                    if (ci_nogo)
                        break;
                    c->set_pos_y(read_signed());
                    if (ci_nogo)
                        break;
                    if (!cref_end)
                        p->set_crefs(ctk);
                    cref_end = ctk;
                }
                if (cref_end) {
                    cref_t *c = ci_nametab->find_cref(cref_end);
                    c->set_last_cref(true);
                }
            }
        }
    }
    if (ci_nogo) {
        delete ci_nametab;
        ci_nametab = 0;
        return (false);
    }
    return (true);
}


cv_info *
sCHDin::read_info()
{
    cv_info *cv = new cv_info(false, false);
    cv->set_records(read_unsigned());
    if (ci_nogo)
        return (cv);
    cv->set_cells(read_unsigned());
    if (ci_nogo)
        return (cv);
    cv->set_boxes(read_unsigned());
    if (ci_nogo)
        return (cv);
    cv->set_polys(read_unsigned());
    if (ci_nogo)
        return (cv);
    cv->set_wires(read_unsigned());
    if (ci_nogo)
        return (cv);
    if (ci_magic >= 2) {
        cv->set_vertices(read_unsigned());
        if (ci_nogo)
            return (cv);
    }
    cv->set_labels(read_unsigned());
    if (ci_nogo)
        return (cv);
    cv->set_srefs(read_unsigned());
    if (ci_nogo)
        return (cv);
    cv->set_arefs(read_unsigned());
    if (ci_nogo)
        return (cv);

    for (;;) {
        char *s = read_n_string();
        if (ci_nogo) {
            delete [] s;
            return (cv);
        }
        if (!s)
            break;
        cv->add_layer(s);
        delete [] s;
    }
    return (cv);
}


bool
sCHDin::read_alias()
{
    // First unsigned is the symref flags.
    // NOTUSED
    unsigned int flags = read_unsigned();

    // Second unsigned is the alias flags.
    flags = read_unsigned();
    if (ci_nogo)
        return (false);
    cv_alias_info *ai = new cv_alias_info();
    ai->set_flags(flags);
    if (flags & CVAL_PREFIX) {
        ai->set_prefix(read_n_string());
        if (ci_nogo) {
            delete ai;
            return (false);
        }
    }
    if (flags & CVAL_SUFFIX) {
        ai->set_suffix(read_n_string());
        if (ci_nogo) {
            delete ai;
            return (false);
        }
    }
    ci_chd->setAliasInfo(ai);
    return (true);
}


bool
sCHDin::read_tables()
{
    if (!ci_nmtab)
        ci_nmtab = new SymTab(false, false);
    ci_nmtab->clear();
    for (;;) {
        char *s = read_n_string();
        if (ci_nogo)
            return (false);
        if (!s)
            break;
        unsigned int n = read_unsigned();
        if (ci_nogo)
            return (false);
        if (ci_nmtab->get(n) == ST_NIL) {
            // The names are inserted into the CD string table here.
            const char *nm = CD()->CellNameTableAdd(s)->string();
            ci_nmtab->add(n, nm, false);
        }
        delete [] s;
    }

    if (!ci_attab)
        ci_attab = new SymTab(false, false);
    ci_attab->clear();
    for (;;) {
        unsigned int otkt = read_unsigned();
        if (ci_nogo)
            return (false);

        unsigned char fl = read_byte();
        if (ci_nogo)
            return (false);
        if (fl == 0xff)
            break;
        CDattr at;
        if (ci_magic >= 3) {
            at.decode_transform(fl >> 4);
            if (fl & MAGIC3_NX) {
                at.nx = read_unsigned();
                if (ci_nogo)
                    return (false);
                at.dx = read_signed();
                if (ci_nogo)
                    return (false);
            }
            if (fl & MAGIC3_NY) {
                at.ny = read_unsigned();
                if (ci_nogo)
                    return (false);
                at.dy = read_signed();
                if (ci_nogo)
                    return (false);
            }
            if (fl & MAGIC3_MG) {
                at.magn = read_real();
                if (ci_nogo)
                    return (false);
            }
        }
        else {
            at.ax = read_byte();
            if (ci_nogo)
                return (false);
            at.ay = read_byte();
            if (ci_nogo)
                return (false);
            at.refly = read_byte();
            if (ci_nogo)
                return (false);
            if (fl & 1) {
                at.magn = read_real();
                if (ci_nogo)
                    return (false);
            }
            if (fl & 2) {
                at.nx = read_unsigned();
                if (ci_nogo)
                    return (false);
                at.ny = read_unsigned();
                if (ci_nogo)
                    return (false);
                at.dx = read_signed();
                if (ci_nogo)
                    return (false);
                at.dy = read_signed();
                if (ci_nogo)
                    return (false);
            }
        }
        ticket_t ntkt = CD()->RecordAttr(&at);
        if (ci_attab->get(otkt) == ST_NIL)
            // save as ntkt+1 to avoid 0
            ci_attab->add((unsigned int)otkt, (void*)(long)(ntkt+1), false);
    }
    return (true);
}


unsigned int
sCHDin::read_unsigned()
{
    unsigned int b = read_byte();
    if (ci_nogo)
        return (0);
    unsigned int i = (b & 0x7f);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 7);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 14);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 21);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0xf) << 28); // 4 bits left

    // Only 4 LSBs can be set.
    if (b & 0xf0) {
        Errs()->add_error("read_unsigned: int32 overflow.");
        ci_nogo = true;
        return (0);
    }
    return (i);
}


int64_t
sCHDin::read_unsigned64()
{
    int64_t b = read_byte();
    if (ci_nogo)
        return (0);
    int64_t i = (b & 0x7f);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 7);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 14);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 21);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 28);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 35);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 42);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 49);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x7f) << 56);
    if (!(b & 0x80))
        return (i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= ((b & 0x1) << 63); // 1 bit left

    // Only 1 LSB can be set.
    if (b & 0xfe) {
        Errs()->add_error("read_unsigned64: int64 overflow.");
        ci_nogo = true;
        return (0);
    }
    return (i);
}


int
sCHDin::read_signed()
{
    unsigned int b = read_byte();
    if (ci_nogo)
        return (0);
    bool neg = b & 1;
    unsigned int i = (b & 0x7f) >> 1;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= (b & 0x7f) << 6;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= (b & 0x7f) << 13;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= (b & 0x7f) << 20;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_byte();
    if (ci_nogo)
        return (0);
    i |= (b & 0xf) << 27; // 4 bits left (sign bit excluded)

    // Only 4 LSBs can be set (sign bit excluded).
    if (b & 0xf0) {
        Errs()->add_error("read_signed: overflow.");
        ci_nogo = true;
        return (0);
    }
    return (neg ? -i : i);
}


double
sCHDin::read_real()
{
    // ieee double
    // first byte is lsb of mantissa
    union { double n; unsigned char b[sizeof(double)]; } u;
    u.n = 1.0;
    if (u.b[0] == 0) {
        // machine is little-endian, read bytes in order
        for (int i = 0; i < (int)sizeof(double); i++) {
            u.b[i] = read_byte();
            if (ci_nogo)
                return (0.0);
        }
    }
    else {
        // machine is big-endian, read bytes in reverse order
        for (int i = sizeof(double) - 1; i >= 0; i--) {
            u.b[i] = read_byte();
            if (ci_nogo)
                return (0.0);
        }
    }
#ifndef WIN32
    if (isnan(u.n)) {
        Errs()->add_error("read_real: bad value (nan).");
        ci_nogo = true;
        return (0.0);
    }
#endif
    return (u.n);
}


char *
sCHDin::read_n_string(unsigned int *n)
{
    int len = read_unsigned();
    if (n)
        *n = len;
    if (len == 0)
        return (0);
    char *str = new char[len+1];
    char *s = str;
    while (len--) {
        int c = read_byte();
        if (ci_nogo) {
            delete [] str;
            return (0);
        }
        *s++ = c;
    }
    *s = 0;
    return (str);
}

