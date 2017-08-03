
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

#ifndef FIO_CHD_H
#define FIO_CHD_H

#include "fio_nametab.h"
#include "fio_crgen.h"


struct cv_alias_info;
struct cv_info;
struct cv_header_info;
struct cv_backend;
struct sChdPrp;
struct Sdiff;
struct zio_stream;
struct zio_index;

// Argument to cCHD::writeFlatRegions.
//
struct named_box_list
{
    named_box_list(const char *na, BBox &b, named_box_list *n) : BB(b)
        {
            name = lstring::copy(na);
            next = n;
        }

    ~named_box_list() { delete [] name; }

    static void destroy(named_box_list *t)
        {
            while (t) {
                named_box_list *tx = t;
                t = t->next;
                delete tx;
            }
        }

    BBox BB;
    char *name;
    named_box_list *next;
};

namespace fio_chd {
    struct ib_t;
}

// Cell Hierarchy Digest, contains a very compact description of the
// cells in an archive file.
//
class cCHD
{
public:
    cCHD(char *fn, FileType ft)
        {
            c_ptab = 0;
            c_etab = 0;
            c_phys_info = 0;
            c_elec_info = 0;
            c_filename = lstring::copy(fn);
            c_header_info = 0;
            c_alias_info = 0;
            c_cgd = 0;
            c_top_symref = 0;
            c_crc = 0;
            c_filetype = ft;
            c_stored = false;
            c_namecfg = false;
        }
    ~cCHD();

    // fio_chd.cc
    bool setDefaultCellname(const char*, const char** = 0);
    void clearSkipFlags();
    bool setCgd(cCGD*, const char* = 0);
    symref_t *defaultSymref(DisplayMode);
    const char *defaultCell(DisplayMode);
    stringlist *listCellnames(int, bool);
    stringlist *listCellnames(const char*, DisplayMode);
    syrlist_t *listing(DisplayMode, bool, tristate_t = ts_ambiguous);
    syrlist_t *listing(DisplayMode, const char*, bool, const BBox*);
    syrlist_t *topCells(DisplayMode, bool = false);
    stringlist *layers(DisplayMode);
    bool depthCounts(symref_t*, unsigned int*);
    SymTab *instanceCounts(symref_t*);
    syrlist_t *listUnresolved(DisplayMode);
    symref_t *findSymref(const char*, DisplayMode, bool = false);
    symref_t *findSymref(CDcellName, DisplayMode);
    bool setBoundaries(symref_t*);
    bool instanceBoundaries(symref_t*,  CDcellName, Blist**);
    void setHeaderInfo(cv_header_info*);
    bool match(const sChdPrp*);
    bool createReferenceCell(const char*);
    OItype loadCell(const char*);
    OItype open(CDcbin*, const char*, const FIOreadPrms*, bool);
    OItype open(symref_t*, cv_in*, const FIOreadPrms*, bool);
    OItype write(const char*, const FIOcvtPrms*, bool);
    OItype write(symref_t*, cv_in*, const FIOcvtPrms*, bool, SymTab* = 0);
    OItype translate_write(const FIOcvtPrms*, const char*);
    cv_in *newInput(bool);

    // fio_chd_flat.cc
    OItype writeFlatRegions(const char*, const FIOcvtPrms*, named_box_list*);
    OItype readFlat(const char*, const FIOcvtPrms*,
        cv_backend* = 0, int = -1, int = 0, bool = false);
    OItype readFlat_odb(SymTab**, const char*, const FIOcvtPrms*);
    OItype readFlat_zdb(SymTab**, const char*, const FIOcvtPrms*);
    OItype readFlat_zl(SymTab**, const char*, const FIOcvtPrms*);
    OItype readFlat_zbdb(SymTab**, const char*, const FIOcvtPrms*,
        unsigned int, unsigned int, unsigned int, unsigned int);
    OItype flatten(symref_t*, cv_in*, int, const FIOcvtPrms*);
    bool createLayers();

    // fio_chd_info.cc
    size_t memuse();
    cvINFO infoMode(DisplayMode);
    char *prInfo(FILE*, DisplayMode, int);
    char *prCells(FILE*, DisplayMode, int, const stringlist*);
    char *prCell(FILE*, DisplayMode, int, const char*);
    char *prCell(FILE*, symref_t*, int);
    char *prDepthCounts(FILE*, DisplayMode, bool);
    char *prDepthCounts(FILE*, symref_t*);
    char *prInstanceCounts(FILE*, DisplayMode, bool);
    char *prFilename(FILE*);
    char *prFiletype(FILE*);
    char *prAlias(FILE*);
    char *prUnit(FILE*, DisplayMode);
    char *prLayers(FILE*, DisplayMode);
    char *prCmpStats(FILE*, DisplayMode);
    char *prEstCxSize(FILE*);
    char *prEstSize(FILE*);
    static int infoFlags(const char*);

    // fio_chd_iter.cc
    bool iterateOverRegion(const char*, const char*, const BBox*, int, int,
        int);
    bool createDensityMaps(const char*, const BBox*, int, int, int,
        stringlist** = 0);
    static XIrt compareCHDs_fp(cCHD*, const char*, cCHD*, const char*,
        const BBox*, const char*, bool, FILE*, unsigned int, unsigned int*,
        int, int);
    static XIrt compareCHDs_sd(cCHD*, const char*, cCHD*, const char*,
        const BBox*, const char*, bool, Sdiff**, unsigned int, unsigned int*,
        int, int);

    // fio_chd_split.cc
    OItype writeMulti(const char*, const FIOcvtPrms*, const Blist*,
        unsigned int, int, unsigned int, bool, bool);

    // fio_zio.cc
    bool registerRandomMap();
    bool unregisterRandomMap();
    static zio_index *getRandomMap(unsigned int);

    nametab_t *nameTab(DisplayMode m) const
        { return (m == Physical ? c_ptab : c_etab); }
    void setNameTab(nametab_t *t, DisplayMode m)
        {
            if (m == Physical)
                c_ptab = t;
            else
                c_etab = t;
        }

    cv_info *pcInfo(DisplayMode m)  const
        { return (m == Physical ? c_phys_info : c_elec_info); }
    void setPcInfo(cv_info *i, DisplayMode m)
        {
            if (m == Physical)
                c_phys_info = i;
            else
                c_elec_info = i;
        }

    const char *filename()        const { return (c_filename); }
    FileType filetype()           const { return (c_filetype); }
    cv_header_info *headerInfo()  const { return (c_header_info); }

    const cv_alias_info *aliasInfo() const { return (c_alias_info); }
    void setAliasInfo(cv_alias_info *a) { c_alias_info = a; }

    bool isStored()               const { return (c_stored); }
    void setStored(bool b)              { c_stored = b; }

    bool hasCgd()                 const { return (c_cgd != 0); }
    cCGD *getCgd()                      { return (c_cgd); }
    const char *getCgdName()      const;

    symref_t *getConfigSymref()   const
        { return (c_namecfg ? c_top_symref : 0); }

    unsigned int crc()            const { return (c_crc); }

private:
    // Inline the test for speed.
    bool setBoundaries_rc(symref_t *p, unsigned int depth = 0)
        {
            if (p->get_bbok())
                return (true);
            return (setBoundaries_rcprv(p, depth));
        }

    // fio_chd.cc
    bool listCellnames_rc(symref_t*, SymTab*, int);
    bool depthCounts_rc(symref_t*, unsigned int, unsigned long, unsigned int*);
    bool instanceCounts_rc(symref_t*, unsigned int, unsigned long, SymTab*);
    bool setBoundaries_rcprv(symref_t*, unsigned int);
    bool instanceBoundaries_rc(symref_t*, fio_chd::ib_t*, unsigned int = 0);

    // fio_chd_split.cc
    OItype write_multi_hier(symref_t*, const FIOcvtPrms*, const Blist*,
        unsigned int, int);
    OItype write_multi_flat(symref_t*, const FIOcvtPrms*, const Blist*,
        unsigned int, int, unsigned int, bool);

    nametab_t *c_ptab;          // phyical table
    nametab_t *c_etab;          // electrical table
    cv_info *c_phys_info;       // phys info obtained during read
    cv_info *c_elec_info;       // elec info obtained during read
    char *c_filename;           // archive file name
    cv_header_info *c_header_info;  // saved file header info
    cv_alias_info *c_alias_info;    // aliasing active during creation
    cCGD *c_cgd;                // geometry database pointer
    symref_t *c_top_symref;     // cached top symref
    unsigned int c_crc;         // crc from gzipped  file, if mapping
    FileType c_filetype;        // Fcgx, Fcif, Fgds, etc.
    bool c_stored;              // saved, don't free
    bool c_namecfg;             // top_symref was configured

    static SymTab *c_index_tab; // for zio_index
};

// A linked-list element for CHDs.
//
struct sCHDlist
{
    sCHDlist(cCHD *c, sCHDlist *n) { chd = c; next = n; }

    static void destroy(sCHDlist *cl)
        {
            while (cl) {
                sCHDlist *cx = cl;
                cl = cl->next;
                delete cx;
            }
        }

    cCHD *chd;
    sCHDlist *next;
};


//
// The following two structures allow a cCHD to be saved as a disk file
// or recreated from a previously saved disk file.
//

// File type identification.  This is followed by the magic number
// providing the format level.
#define MAGIC_STRING "Xic Context 1."

// The initial byte of the magic 3 attr data has the 4 MSBs encoding
// the transform, the LSBs flag following data present.
#define MAGIC3_NX   0x1
#define MAGIC3_NY   0x2
#define MAGIC3_MG   0x4

// Flags for sCHDout::write  
#define CHD_WITH_GEOM   0x1
#define CHD_NO_GZIP     0x2
//
// CHD_WITH_GEOM
//   Include compressed geometry records in file, after the context
//   part.  Layer filtering can be set up before the write.
//
// CHD_NO_GZIP
//   For debugging, don't gzip the file contents.  By default, the
//   physical part of the context file is compressed.

struct cgd_cn_t;
struct cgd_lyr_t;

struct tkt_map_t
{
    virtual ~tkt_map_t() { }

    virtual bool map(ticket_t*) = 0;
};

// Write a cCHD to a file.
//
struct sCHDout : public tkt_map_t
{
    sCHDout(cCHD*);
    ~sCHDout();

    bool write(const char*, unsigned int);
    bool write_cell_record(cgd_cn_t*);
    bool write_layer_record(cgd_lyr_t*);

private:
    char *magic_string(unsigned int);
    bool init_tables();
    ticket_t name_index(CDcellName);
    bool map(ticket_t*);
    bool save_ticket(ticket_t);
    bool write_info();
    bool write_alias();
    bool write_tables();
    bool write_symref(symref_t*);
    bool write_cref(cref_t*);
    bool write_attr(const CDattr*);

    inline bool write_char(int);
    bool write_unsigned(unsigned);
    bool write_unsigned64(int64_t);
    bool write_signed(int);
    bool write_real(double);
    bool write_n_string(const char*, unsigned int = 0);

    cCHD *co_chd;           // context being written
    FILE *co_fp;            // output file pointer
    zio_stream *co_zio;     // zlib compressor
    SymTab *co_nmtab;       // name to number mapping
    SymTab *co_attab;       // ticket to attributes mapping
    unsigned int co_nmidx;  // symbol numbering index
    unsigned int co_magic;  // output format magic number
    DisplayMode co_mode;    // present display mode
};

// How to deal with geometry records, argument to sCGDin::read.
enum ChdCgdType { CHD_CGDmemory, CHD_CGDfile, CHD_CGDnone };

// Read a cCHD from a file.
//
struct sCHDin : public tkt_map_t
{
    sCHDin();
    ~sCHDin();

    int is_chd(FILE*);
    cCHD *read(const char *fname, ChdCgdType);
    bool check(const char *fname);

    static void set_default_cgd_type(ChdCgdType t) { ci_def_cgd_type = t; }
    static ChdCgdType get_default_cgd_type() { return (ci_def_cgd_type); }

private:
    inline int read_byte();

    int read_magic();
    bool read_chd();
    bool map(ticket_t*);
    bool read_nametab();
    cv_info *read_info();
    bool read_alias();
    bool read_tables();
    unsigned int read_unsigned();
    int64_t read_unsigned64();
    int read_signed();
    double read_real();
    char *read_n_string(unsigned int* = 0);

    cCHD *ci_chd;           // new context
    FILE *ci_fp;            // input file pointer
    zio_stream *ci_zio;     // gzip uncompressor
    SymTab *ci_nmtab;       // number to name mapping, the name is in
                            //  the cd string table
    SymTab *ci_attab;       // old ticket to new ticket mapping
    nametab_t *ci_nametab;  // name tab being filled
    unsigned int ci_flags;  // symref flags
    int ci_magic;           // file format indication
    DisplayMode ci_mode;    // data for mode
    bool ci_nogo;           // error flag

    static ChdCgdType ci_def_cgd_type;
};

#endif

