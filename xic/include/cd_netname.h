
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2013 Whiteley Research Inc, all rights reserved.        *
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
 $Id: cd_netname.h,v 5.16 2016/10/04 01:48:43 stevew Exp $
 *========================================================================*/

#ifndef CD_NETNAME_H
#define CD_NETNAME_H

#include "symtab.h"


//
// Net expression parsing and terminal name string table.
//

// Default leading character in terminal names.
#define DEF_TERM_CHAR '_'

// Default subscripting chars.
#define DEF_SUBSCR_OPEN '<'
#define DEF_SUBSCR_CLOSE '>'

//-----------------------------------------------------------------------------
// Vector expression list element.
//
// The expression is represented as a list of terms, each term
// represented by one of these structs.  The possible types are:
//
// 1.  a single integer N[*p]
// 2.  a range N:M:I[*p]
// 3.  a list terms  (T, ...)[*p]
// where p is the post multiplier,
// 
struct CDvecex
{
    CDvecex(CDvecex *vx, unsigned int b, unsigned int e, unsigned int i,
            unsigned int m)
        {
            vx_next = 0;
            vx_group = vx;
            vx_beg_range = b;
            vx_end_range = e;
            vx_incr = i;
            vx_postmult = m;
        }

    CDvecex(const CDvecex&);
    CDvecex &operator=(const CDvecex&);

    ~CDvecex()
        {
            destroy(vx_group);
        }

    static void destroy(const CDvecex *v)
        {
            while (v) {
                const CDvecex *vx = v;
                v = v->next();
                delete vx;
            }
        }

    static CDvecex *dup(const CDvecex *v)
        {
            return (v ? new CDvecex(*v) : 0);
        }

    CDvecex *next()                 const { return (vx_next); }
    void set_next(CDvecex *v)             { vx_next = v; }
    CDvecex *group()                const { return (vx_group); }
    unsigned int beg_range()        const { return (vx_beg_range); }
    unsigned int end_range()        const { return (vx_end_range); }
    unsigned int increment()        const { return (vx_incr); }
    unsigned int postmult()         const { return (vx_postmult); }
    void set_postmult(unsigned int n)
        {
            vx_postmult = (n >= 1 ? n : 1);
        }

    bool is_simple(int *b, int *e)
        {
            if (!vx_next && vx_incr == 1 && vx_postmult == 1) {
                if (vx_group)
                    return (vx_group->is_simple(b, e));
                if (b)
                    *b = vx_beg_range;
                if (e)
                    *e = vx_end_range;
                return (true);
            }
            return (false);
        }

    unsigned int width()
        {
            unsigned int wid = 0;
            if (vx_group)
                wid = vx_group->width() * vx_postmult;
            else {
                unsigned int n = abs(vx_beg_range - vx_end_range);
                unsigned int w = 1 + n/vx_incr;
                wid = w * vx_postmult;
            }
            if (vx_next)
                wid += vx_next->width();
            return (wid);
        }

    bool operator==(const CDvecex&) const;
    bool operator!=(const CDvecex&) const;

    void print_this(sLstr*) const;

    static void print_all(const CDvecex *thisv, sLstr *lstr)
        {
            for (const CDvecex *v = thisv; v; v = v->next()) {
                v->print_this(lstr);
                if (v->next())
                    lstr->add_c(',');
            }
        }

    static bool parse(const char*, CDvecex**, const char** = 0);

private:
    CDvecex *vx_next;
    CDvecex *vx_group;
    unsigned short vx_beg_range;
    unsigned short vx_end_range;
    unsigned short vx_incr;
    unsigned short vx_postmult;
};


//-----------------------------------------------------------------------------
// Vector expression iterator.
//
struct CDvecexGen
{
    CDvecexGen(const CDvecex*);
    ~CDvecexGen();

    bool next(int*);

private:
    const CDvecex *vg_next;
    CDvecexGen *vg_sub;
    CDvecex *vg_group;
    int vg_beg;
    int vg_end;
    unsigned int vg_incr;
    unsigned int vg_mult;
    unsigned int vg_mcnt;
    bool vg_ascend;
    bool vg_done;
};


//-----------------------------------------------------------------------------
// An encapsulation of the cTnameTab string pointer.
//
struct CDnetNameStr
{
//XXX fixme, can't call thru void pointer.
    const char *string()    const { return ((const char*)this); }
    const char *stringNN()  const
        {
            const CDnetNameStr *cn = this;
            return (cn ? (const char*)cn : "");
        }
};
typedef CDnetNameStr* CDnetName;


//-----------------------------------------------------------------------------
// A class for maintaining node/terminal names in a string table.
//
class cTnameTab
{
public:
    cTnameTab()
        {
            tnt_u.tab = 0;
            tnt_defnames = 0;
            tnt_unname = 0;
            tnt_dnsize = 0;
            tnt_ci = tnt_case_insens;
        }

    ~cTnameTab()
        {
            if (tnt_ci)
                delete tnt_u.citab;
            else
                delete tnt_u.tab;
            delete [] tnt_defnames;
        }

    void clear()
        {
            if (tnt_ci)
                delete tnt_u.citab;
            else
                delete tnt_u.tab;
            tnt_u.tab = 0;
            delete [] tnt_defnames;
            tnt_defnames = 0;
            tnt_unname = 0;
            tnt_dnsize = 0;
        }

    // Add a name, or return matching existing name.
    //
    CDnetName add(const char *n)
        {
            if (!n || !*n)
                return (0);
            if (tnt_ci) {
                if (!tnt_u.citab)
                    tnt_u.citab = new cstrtab_t(cstrtab_t::SaveUpper);
                return ((CDnetName)tnt_u.citab->add(subscr_fix(n)));
            }
            if (!tnt_u.tab)
                tnt_u.tab = new strtab_t;
            return ((CDnetName)tnt_u.tab->add(subscr_fix(n)));
        }

    // Return matching name if it exists, 0 otherwise.
    //
    CDnetName find(const char *n)
        {
            if (!n || !*n)
                return (0);
            if (!tnt_u.tab)
                return (0);

            if (tnt_ci)
                return ((CDnetName)tnt_u.citab->find(subscr_fix(n)));
            return ((CDnetName)tnt_u.tab->find(subscr_fix(n)));
        }

    // Return the "default" name.  the ix is a node index value. 
    // These are cached in an array.  These are terminal/node names
    // if no other name is assigned.
    //
    CDnetName default_name(unsigned int ix)
        {
            if (ix >= tnt_dnsize) {
                unsigned int ns = tnt_dnsize ? tnt_dnsize : 32;
                while (ix >= ns)
                    ns *= 2;
                CDnetName *nn = new CDnetName[ns];
                if (tnt_dnsize)
                    memcpy(nn, tnt_defnames, tnt_dnsize*sizeof(CDnetName));
                memset(nn + tnt_dnsize, 0,
                    (ns - tnt_dnsize)*sizeof(CDnetName));
                delete [] tnt_defnames;
                tnt_defnames = nn;
                tnt_dnsize = ns;
            }
            if (!tnt_defnames[ix]) {
                char buf[64];
                snprintf(buf, 64, "%c%u", DEF_TERM_CHAR, ix);
                tnt_defnames[ix] = add(buf);
            }
            return (tnt_defnames[ix]);
        }

    // Return the "unnamed" name, used for CDterm instances with a
    // null node property pointer.
    //
    CDnetName unnamed_name()
        {
            if (!tnt_unname)
                tnt_unname = add("unnamed");
            return (tnt_unname);
        }

    // Global flag to set case-sensitivity in terminal name matching. 
    // This is for use by the application.  The status will be used in
    // the constructor to set up the table accordingly.

    static bool case_insensitive_mode()         { return (tnt_case_insens); }
    static void set_case_insensitive_mode(bool b)  { tnt_case_insens = b; }

    // Case sensitivity of this instance, as was set in the
    // constructor.  This can't be changed.

    bool case_insens()                  const { return (tnt_ci); }

    // There is a preferred subscripting character pair, which is used
    // for subscripted names in the string table.  It is also used
    // when explicitly creating names.  By converting to the preferred
    // char pair, we avoid duplications and the need for string
    // comparisons.

    static char subscr_open()           { return (tnt_subopen); }
    static char subscr_close()          { return (tnt_subclose); }
    static void subscr_set(char o, char c)
        {
            tnt_subopen = o;
            tnt_subclose = c;
        }

    // Translate subscript chars.
    static const char *subscr_fix(const char *str)
        {
            if (!str)
                return (0);
            for (const char *t = str; *t; t++) {
                // I found some labels in a Cadence database with a
                // trailing colon.  This must have some significance,
                // but for now strip it.

                if ((*t == '<' || *t == '[' || *t == '{') &&
                        *t != tnt_subopen) {
                    delete [] tnt_buf;
                    tnt_buf = new char[t - str + strlen(t) + 1];
                    char *b = tnt_buf;
                    t = str;
                    while (*t) {
                        if (*t == '<' || *t == '[' || *t == '{') {
                            t++;
                            *b++ = tnt_subopen;
                        }
                        else if (*t == '>' || *t == ']' || *t == '}') {
                            t++;
                            *b++ = tnt_subclose;
                        }
                        else
                            *b++ = *t++;
                    }
                    *b = 0;

                    if (*(b-1) == ':')
                        *(b-1) = 0;

                    return (tnt_buf);
                }
                if (*t == ':' && *(t+1) == 0) {
                    delete [] tnt_buf;
                    int len = t - str;
                    tnt_buf = new char[len + 1];
                    strncpy(tnt_buf, str, len);
                    tnt_buf[len] = 0;
                    return (tnt_buf);
                }
            }
            return (str);
        }

private:
    union {
        strtab_t *tab;
        cstrtab_t *citab;
    } tnt_u;
    CDnetName *tnt_defnames;
    CDnetName tnt_unname;
    unsigned int tnt_dnsize;
    bool tnt_ci;                    // Case sensitivity of this instance.

    static char *tnt_buf;           // Temp buffer for subscr_fix return.
    static bool tnt_case_insens;    // Flag indicates case-insensitive mode.
    static char tnt_subopen;        // Default open subscript character.
    static char tnt_subclose;       // Default close subscript character.
};


//-----------------------------------------------------------------------------
// Net expression list element.
//
// The expression is represented as a list of terms, each term
// represented by one of these structs.  The possible types are:
//
// 1.  <*p>[name][vecex]
// 2.  <*p>(term, ...)
// where p is the pre-multiplier.
//
struct CDnetex
{
    CDnetex(CDnetName nm, unsigned int m, CDvecex *vx, CDnetex *gp)
        {
            nx_next = 0;
            nx_group = gp;
            nx_name = nm;
            nx_vecex = vx;
            nx_multip = m;
        }

    CDnetex(const CDnetex&);
    CDnetex &operator=(const CDnetex&);

    ~CDnetex()
        {
            CDnetex::destroy(nx_group);
            CDvecex::destroy(nx_vecex);
        }

    static void destroy(const CDnetex *n)
        {
            while (n) {
                const CDnetex *nx = n;
                n = n->next();
                delete nx;
            }
        }

    static CDnetex *dup(const CDnetex *n)
        {
            return (n ? new CDnetex(*n) : 0);
        }

    CDnetex *next()                 const { return (nx_next); }
    void set_next(CDnetex *n)             { nx_next = n; }
    CDnetex *group()                const { return (nx_group); }
    CDnetName name()                const { return (nx_name); }
    CDvecex *vecexp()               const { return (nx_vecex); }
    unsigned int multip()           const { return (nx_multip); }

    // Expression is a single non-repeated name with no index.
    //
    bool is_scalar(CDnetName *pn) const
        {
            if (!nx_next && nx_multip == 1) {
                if (nx_group)
                    return (nx_group->is_scalar(pn));
                if (!nx_vecex) {
                    if (pn)
                        *pn = nx_name;
                    return (true);
                }
            }
            return (false);
        }

    // Expression is a single non-repeated unit increment range, with
    // or without a name.
    //
    bool is_simple(CDnetName *n, int *b, int *e) const
        {
            if (!nx_next && nx_multip == 1) {
                if (nx_group)
                    return (is_simple(n, b, e));
                if (nx_vecex) {
                    if (nx_vecex->is_simple(b, e)) {
                        if (n)
                            *n = nx_name;
                        return (true);
                    }
                }
            }
            return (false);
        }

    // True if named and width == 1.
    //
    bool is_unit() const
        {
            if (is_scalar(0))
                return (true);
            CDnetName nm;
            int b, e;
            return (is_simple(&nm, &b, &e) && nm && b == e);
        }

    CDnetName first_name()
        {
            if (nx_group)
                return (nx_group->first_name());
            return (nx_name);
        }

    void set_first_name(CDnetName nm)
        {
            if (nx_group) {
                nx_group->set_first_name(nm);
                return;
            }
            nx_name = nm;
        }

    CDnetName last_name() const
        {
            const CDnetex *nx = this;
            while (nx->nx_next)
                nx = nx->nx_next;
            if (nx->nx_group)
                return (nx->nx_group->last_name());
            return (nx_name);
        }

    unsigned int width() const
        {
            unsigned int wid = 0;
            if (nx_group)
                wid = nx_group->width() * nx_multip;
            else if (nx_vecex)
                wid = nx_vecex->width() * nx_multip;
            else
                wid = nx_multip;
            if (nx_next)
                wid += nx_next->width();
            return (wid);
        }

    bool operator==(const CDnetex&) const;
    bool operator!=(const CDnetex&) const;

    void print_this(sLstr*) const;

    static void print_all(const CDnetex *thisn, sLstr *lstr)
        {
            for (const CDnetex *n = thisn; n; n = n->next()) {
                n->print_this(lstr);
                if (n->next())
                    lstr->add_c(',');
            }
        }

    static char *id_text(const CDnetex *n)
        {
            sLstr lstr;
            print_all(n, &lstr);
            return (lstr.string_trim());
        }

    static bool check_compatible(const CDnetex*, const CDnetex*);
    static bool check_set_compatible(CDnetex *thisn, const CDnetex *nm)
        {
            if (!thisn)
                return (true);
            return (thisn->check_set_compatible_prv(nm));
        }

private:
    bool check_set_compatible_prv(const CDnetex*);
    bool resolve(CDnetName, int) const;

public:
    // Return true if the expressions are logically identical.
    static bool cmp(const CDnetex *n1, const CDnetex *n2)
        {
            if (!n1 && !n2)
                return (true);
            if (!n1 || !n2)
                return (false);
            return (*n1 == *n2);
        }

    // Return true if the expressions are logically identical.
    static bool cmp(const char *s1, const char *s2)
        {
            CDnetex *n1, *n2;
            if (!parse(s1, &n1))
                return (false);
            if (!parse(s2, &n2)) {
                destroy(n1);
                return (false);
            }
            if (!n1 && !n2)
                return (true);
            if (!n1) {
                destroy(n2);
                return (false);
            }
            if (!n2) {
                destroy(n1);
                return (false);
            }
            bool res = (*n1 == *n2);
            destroy(n1);
            destroy(n2);
            return (res);
        }

    // Return true if the expressions are logically identical.
    static bool cmp(const char *s1, const CDnetex *n2)
        {
            CDnetex *n1;
            if (!parse(s1, &n1))
                return (false);
            if (!n1 && !n2)
                return (true);
            if (!n1)
                return (false);
            if (!n2) {
                destroy(n1);
                return (false);
            }
            bool res = (*n1 == *n2);
            destroy(n1);
            return (res);
        }

    // Create a 1-bit name, add it to the string table.
    //
    static CDnetName mk_name(const char *bsn, int ix)
        {
            if (ix < 0)
                return (name_tab_add(bsn));
            sLstr lstr;
            lstr.add(bsn);
            lstr.add_c(cTnameTab::subscr_open());
            lstr.add_i(ix);
            lstr.add_c(cTnameTab::subscr_close());
            return (name_tab_add(lstr.string()));
        }

    static bool parse(const char*, CDnetex**);
    static bool parse_bit(const char*, CDnetName*, int*, bool = false);
    static bool is_default_name(const char*);

    //
    // Static string table interface.
    //

    static CDnetName name_tab_add(const char *nm)
        {
            if (!nm)
                return (0);
            if (!nx_nametab)
                nx_nametab = new cTnameTab;
            return (nx_nametab->add(nm));
        }

    static CDnetName name_tab_find(const char *nm)
        {
            if (!nx_nametab || !nm)
                return (0);
            return (nx_nametab->find(nm));
        }

    static CDnetName default_name(unsigned int ix)
        {
            if (!nx_nametab)
                nx_nametab = new cTnameTab;
            return (nx_nametab->default_name(ix));
        }

    static CDnetName undef_name()
        {
            if (!nx_nametab)
                nx_nametab = new cTnameTab;
            return (nx_nametab->unnamed_name());
        }

    static void name_tab_clear()
        {
            delete nx_nametab;
            nx_nametab = 0;
        }

    static bool name_tab_case_insens()
        {
            if (nx_nametab)
                return (nx_nametab->case_insens());
            return (cTnameTab::case_insensitive_mode());
        }

private:
    CDnetex *nx_next;
    CDnetex *nx_group;
    CDnetName nx_name;
    CDvecex *nx_vecex;
    unsigned int nx_multip;

private:
    static cTnameTab *nx_nametab;   // Global node/terminal name table.
};


//-----------------------------------------------------------------------------
// Net expression iterator.
//
struct CDnetexGen
{
    CDnetexGen(const CDnetex*);
    CDnetexGen(const CDp_bnode*);
    ~CDnetexGen();

    bool next(CDnetName*, int*);

private:
    CDnetexGen *ng_sub;
    CDvecexGen *ng_vecgen;
    const CDnetex *ng_next;
    CDnetex *ng_group;
    const CDvecex *ng_vecex;
    CDnetName ng_name;
    CDnetex *ng_netex;
    unsigned int ng_mult;
    unsigned int ng_mcnt;
};

#endif

