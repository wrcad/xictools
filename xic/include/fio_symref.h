
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

#ifndef FIO_SYMREF_H
#define FIO_SYMREF_H


// symref_t flag bits, the field width is 16 bits.
#define CVskip      0x1
#define CVcif       0x2
#define CVx_srf     0x4
#define CVx_bb      0x8
#define CVbbok      0x10
#define CVusr       0x20
#define CVemty      0x40
#define CVcmpr      0x80
#define CVdef       0x100
#define CVelec      0x200
#define CVrefd      0x400
#define CVx_emp     0x800

// Flags included in files.  The CVx_srf/bb flags are always set in
// files written, but checked in input for backwards compatibility.
//
#define CVio_mask (CVcif | CVx_srf | CVx_bb | CVbbok | CVcmpr | CVdef | \
    CVelec | CVrefd)

// CVskip       Ignore cell when reading.
// CVcif        Extra cell number field for CIF.
// CVx_srf      Not used, back compat.
// CVx_bb       Not used, back compat.
// CVbbok       Indicates BBox field set.
// CVusr        Misc. flag for local use, must be reset after use.
// CVemty       Cell identified as empty, will skip.
// CVcmpr       Compressed format for cref_t list used.
// CVdef        The cell definition was read (used by readers).
// CVelec       The cell contains electrical data.
// CVrefd       Symref is referenced as a subcell
// CVx_emp      Not used, back compat.


// Instance list for symref_t.  This is locally allocated.
//
// Optionally, these are replaced with a compressed stream to save
// core.  Thus, BE CAREFUL when setting the values.  There is a
// set_flag method in crgen_t which does the right thing.
//
struct cref_t
{
    cref_t() { cr_refptr = 0; cr_tx = cr_ty = 0; cr_xfdata = 0; }

    // We need to save a couple of flags somewhere.  Since the
    // attributes ticket is almost assuredly not huge, we will steal a
    // couple of bits for the flags.
    // 0x1   misc flag, user set
    // 0x2   internal, indicates last cref in list
    // remaining bits: the attributes ticket

    ticket_t get_tkt()      const { return (cr_xfdata >> 2); }
    void set_tkt(ticket_t t)      { cr_xfdata = (t << 2) | (cr_xfdata & 0x3); }
    bool get_flg()          const { return (cr_xfdata & 0x1); }
    void set_flg(bool f)          { if (f) cr_xfdata |= 0x1;
                                    else cr_xfdata &= ~0x1; }
    bool last_cref()        const { return (cr_xfdata & 0x2); }
    void set_last_cref(bool f)    { if (f) cr_xfdata |= 0x2;
                                    else cr_xfdata &= ~0x2; }

    unsigned int data()     const { return (cr_xfdata); }
    void set_data(unsigned int d) { cr_xfdata = d; }

    ticket_t refptr()       const { return (cr_refptr); }
    void set_refptr(ticket_t t)   { cr_refptr = t; }
    int pos_x()             const { return (cr_tx); }
    void set_pos_x(int x)         { cr_tx = x; }
    int pos_y()             const { return (cr_ty); }
    void set_pos_y(int y)         { cr_ty = y; }

private:
    ticket_t cr_refptr;     // back pointer to symref
    int cr_tx, cr_ty;       // instance origin
    ticket_t cr_xfdata;     // instance attributes ticket and flag
};

// This is the instance reference element as returned from the generator.
//
struct cref_o_t
{
    cref_o_t()
        {
            srfptr = 0;
            tx = 0;
            ty = 0;
            attr = 0;
            flag = false;
        }

    void set(cref_t *c)
        {
            srfptr = c->refptr();
            tx = c->pos_x();
            ty = c->pos_y();
            attr = c->get_tkt();
            flag = c->get_flg();
        }

    ticket_t srfptr;        // back pointer to symref
    int tx, ty;             // instance origin
    ticket_t attr;          // attrigutes ticket
    bool flag;              // flag
};


// Symbol reference data item.  This is a variable-length struct, that
// includes, depending on flags, a symbol number for CIF, bounding box,
// and list heads for instance references.  This is locally allocated.
//
struct symref_t
{
    CDcellName get_name()       const { return (name); }
    void set_name(CDcellName n)       { name = n; }
    // must be unlinked from name table before name change!

    void set_offset(int64_t os)
        { offs_flags = (os << 16) | (offs_flags & 0xffff); }
    int64_t get_offset() const { return (offs_flags >> 16); }
    unsigned short get_flags() const { return ((unsigned short)offs_flags); }
    void set_flags(unsigned short f)
        {
            offs_flags &= ~(int64_t)0xffff;
            offs_flags |= (int64_t)f;
        }

    bool should_skip()  const { return (get_flags() & (CVskip | CVemty)); }

    void set_skip(bool b)     { b ? set_flag(CVskip) : unset_flag(CVskip); }

    bool get_cif()      const { return (get_flags() & CVcif); }
    void set_cif(bool b)      { b ? set_flag(CVcif) : unset_flag(CVcif); }

    bool get_bbok()     const { return (get_flags() & CVbbok); }
    void set_bbok(bool b)     { b ? set_flag(CVbbok) : unset_flag(CVbbok); }

    bool get_usr()      const { return (get_flags() & CVusr); }
    void set_usr(bool b)      { b ? set_flag(CVusr) : unset_flag(CVusr); }

    bool get_emty()     const { return (get_flags() & CVemty); }
    void set_emty(bool b)     { b ? set_flag(CVemty) : unset_flag(CVemty); }

    bool get_cmpr()     const { return (get_flags() & CVcmpr); }
    void set_cmpr(bool b)     { b ? set_flag(CVcmpr) : unset_flag(CVcmpr); }

    bool get_defseen()  const { return (get_flags() & CVdef); }
    void set_defseen(bool b)  { b ? set_flag(CVdef) : unset_flag(CVdef); }

    bool get_elec()     const { return (get_flags() & CVelec); }
    void set_elec(bool b)     { b ? set_flag(CVelec) : unset_flag(CVelec); }

    bool get_refd()     const { return (get_flags() & CVrefd); }
    void set_refd(bool b)     { b ? set_flag(CVrefd) : unset_flag(CVrefd); }

    DisplayMode mode()  const { return (get_elec() ? Electrical : Physical); }

    inline int get_num() const;
    inline void set_num(int);

    const BBox *get_bb()      const { return (&BB); }
    void set_bb(const BBox *bb)     { BB = *bb; }
    void add_to_bb(const BBox *bb)  { BB.add(bb); }

    ticket_t get_crefs()      const { return (crefs); }
    void set_crefs(ticket_t s)      { crefs = s; }

    uintptr_t tab_key()             { return ((uintptr_t)name); }
    symref_t *tab_next()            { return (next); }
    void set_tab_next(symref_t *s)  { next = s; }
    symref_t *tgen_next(bool)       { return (next); }
    ticket_t get_ticket()     const { return (indx); }
    void set_ticket(ticket_t t)     { indx = t; }

private:
    void set_flag(unsigned short f) { offs_flags |= (int64_t)f; }
    void unset_flag(unsigned short f) { offs_flags &= ~(int64_t)f; }

    int64_t offs_flags; // flags in 2 lsb bytes, file offset in rest
    CDcellName name;    // pointer to string table entry (never free!)

    // The symref_t can be obtained from the factory with a ticket_t.
    // In 64-bit mode, the cref_t can then use a 32-bit ticket_t
    // instead of a 64-bit pointer to reference the symref_t.  For
    // this to work, we need to be able to get the ticket_t given a
    // symref_t pointer, which pretty much requires a field in the
    // symref_t.  We could also use a ticket_t instead of the next
    // pointer, however this is probably not worthwhile:
    // 1) slightly less efficient hash table access.
    // 2) more complex hash table code, can't just use an
    //    itable_t<symref_t>.
    //
    symref_t *next;     // hash table link
    ticket_t indx;      // index for this
    ticket_t crefs;     // subcell list
    BBox BB;            // cell bounding box
};


// Variation for CIF, saves space when not using CIF.  This will have the
// CVcif flag set.
//
struct symref_cif_t : public symref_t
{
    int get_cif_num()       const { return (cifnum); }
    void set_cif_num(int n)       { cifnum = n; }

private:
    int cifnum;         // cell number used for CIF only
};


inline int
symref_t::get_num() const
{
    if (get_flags() & CVcif)
        return (((symref_cif_t*)this)->get_cif_num());
    return (0);
}

inline void
symref_t::set_num(int n)
{
    if (get_flags() & CVcif)
        ((symref_cif_t*)this)->set_cif_num(n);
}


// A linked list element for symref_t.
//
struct syrlist_t
{
    syrlist_t(symref_t *s, syrlist_t *n) { symref = s; next = n; }

    static void destroy(syrlist_t *syl)
        {
            while (syl) {
                syrlist_t *syx = syl;
                syl = syl->next;
                delete syx;
            }
        }

    static void sort(syrlist_t*, bool);

    symref_t *symref;
    syrlist_t *next;
};

#endif

