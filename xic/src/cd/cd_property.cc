
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
 $Id: cd_property.cc,v 5.155 2017/04/16 20:27:57 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "cd_types.h"
#include "cd_propnum.h"
#include "cd_terminal.h"
#include "cd_netname.h"
#include "cd_hypertext.h"
#include "cd_lgen.h"
#include "cd_celldb.h"
#include "fio.h"
#include "fio_cif.h"
#include "fio_chd.h"
#include <ctype.h>
#include <algorithm>


namespace {
    // Electrical property names, as in  cd_propnum.h.
    const char *prpty_name[] = {
        "0",        // 0    unused
        "model",    // 1    P_MODEL
        "value",    // 2    P_VALUE
        "param",    // 3    P_PARAM
        "other",    // 4    P_OTHER
        "nophys",   // 5    P_NOPHYS
        "virtual",  // 6    P_VIRTUAL
        "flatten",  // 7    P_FLATTEN
        "range",    // 8    P_RANGE
        "bnode",    // 9    P_BNODE
        "node",     // 10   P_NODE
        "name",     // 11   P_NAME
        "labloc",   // 12   P_LABLOC
        "mut",      // 13   P_MUT
        "newmut",   // 14   P_NEWMUT
        "branch",   // 15   P_BRANCH
        "labrf",    // 16   P_LABRF
        "mutlrf",   // 17   P_MUTLRF
        "symblc",   // 18   P_SYMBLC
        "nodmap",   // 19   P_NODMAP
        "macro",    // 20   P_MACRO
        "devref"    // 21   P_DEVREF
    };


    // Fill in the pts array from the string.
    //
    void getpts(const char *str, Point **ppts, int *pnum)
    {
        Plist *p0 = 0, *pe = 0;
        int i = 0;
        const char *t = str;
        while (*t) {
            while (isspace(*t))
                t++;
            char tbuf[64];
            char *s = tbuf;
            while (*t && !isspace(*t))
                *s++ = *t++;
            *s = '\0';
            int x = atoi(tbuf);
            while (isspace(*t))
                t++;
            s = tbuf;
            while (*t && !isspace(*t))
                *s++ = *t++;
            *s = '\0';
            int y = atoi(tbuf);
            if (!p0)
                p0 = pe = new Plist(x, y, 0);
            else {
                pe->next = new Plist(x, y, 0);
                pe = pe->next;
            }
            i++;
        }
        Point *po = new Point[i];
        i = 0;
        for (Plist *p = p0; p; p = p->next) {
            po[i].set(p->x, p->y);
            i++;
        }
        *ppts = po;
        *pnum = i;
        Plist::destroy(p0);
    }


    // The str is a list of ints separated by commas or colons.  Parse
    // and return the list as an array and size.  False is returned on
    // error.  At least one integer must be found.
    //
    bool slurp_ints(const char *str, int *pnum, int **pvals)
    {
        if (!str)
            return (false);
        int cnt = 1;
        for (const char *s = str; *s; s++) {
            if (*s == ':' || *s == ',')
                cnt++;
        }
        int *vals = new int[cnt];

        cnt = 0;
        const char *s = str;
        while (*s) {
            const char *t = s;
            while (*t && *t != ':' && *t != ',')
                t++;
            if (sscanf(s, "%d", vals + cnt) != 1) {
                delete [] vals;
                return (false);
            }
            cnt++;
            if (!*t)
                break;
            s = t+1;
        }
        if (!cnt) {
            delete [] vals;
            return (false);
        }
        *pnum = cnt;
        *pvals = vals;
        return (true);
    }
}


FlagDef TermFlags[] =
{
    { "BYNAME", TE_BYNAME, true, "Terminal connects by name in schematic." },
    { "FIXED", TE_FIXED, true, "User has set physical location." },
    { "SCINVIS", TE_SCINVIS, true, "Terminal not shown in schematic." },
    { "SYINVIS", TE_SYINVIS, true, "Terminal not shown in symbol." },
    { "UNINIT", TE_UNINIT, false, "Not placed/connected in layout." },
    { "LOCSET", TE_LOCSET, false, "Physical location has been set." },
    { "PTS", TE_PTS, false, "Terminal has multi-point allocation." },
    { 0, 0, false, 0 }
};


FlagDef TermTypes[] =
{
    { "INPUT", TE_tINPUT, false, "Input signal (default)" },
    { "OUTPUT", TE_tOUTPUT, false, "Output signal" },
    { "INOUT", TE_tINOUT, false, "Input/output signal" },
    { "TRISTATE", TE_tTRISTATE, false, "Tristate" },
    { "CLOCK", TE_tCLOCK, false, "Clock signal" },
    { "OUTCLOCK", TE_tOUTCLOCK, false, "Clock output" },
    { "SUPPLY", TE_tSUPPLY, false, "Power connection" },
    { "OUTSUPPLY", TE_tOUTSUPPLY, false, "Power output connection" },
    { "GROUND", TE_tGROUND, false, "Ground connection" },
    { 0, 0, false, 0 }
};


// Return a hypertext list representing the string.
//
hyList *
CDp::hpstring(CDs *sdesc) const
{
    if (p_string && *p_string) {
        const char *lttok = HYtokPre HYtokLT HYtokSuf;
        if (lstring::prefix(lttok, p_string))
            return (new hyList(sdesc, p_string, HYcvAscii));
        return (new hyList(sdesc, p_string, HYcvPlain));
    }
    return (0);
}


// Free the entire CDp struct list headed by pdesc.
//
void
CDp::free_list()
{
    CDp *pd, *pnxt;
    for (pd = this; pd; pd = pnxt) {
        pnxt = pd->p_next;
        delete pd;
    }
}


// Return a copy of the text for property.
//
bool
CDp::string(char **pstr) const
{
    sLstr lstr;
    if (!print(&lstr, 0, 0)) {
        *pstr = 0;
        return (false);
    }
    *pstr = lstr.string_trim();
    return (true);
}


// Return true is the string is "long text".
//
bool
CDp::is_longtext() const
{
    char *str;
    string(&str);
    bool lt = str && lstring::prefix(HYtokPre HYtokLT HYtokSuf, str);
    delete [] str;
    return (lt != 0);
}


// If the cell scaling is being changed as the cell is being read,
// we have to change the scale of coordinates in the properties, too.
// The following functions accomplish this.
// - Only CDs and CDc properties may require scaling.
// - The GDSII_PROPERTY_BASE+xxx properties do not require scaling.
//
// This is called by the converters.  The properties are all CDp type,
// i.e., the string is significant.  This is NOT the format used in
// the objects.
//
// Replace the string with a new string containing scaled parameter
// values as necessary.  If mode is Electrical, the physical scale
// is needed to fix the coordinates in NODE and NAME properties.
//
void
CDp::scale(double elec_scale, double phys_scale, DisplayMode mode)
{
    if (mode == Physical) {
        if (p_value == XICP_GRID)
            scale_grid(elec_scale, phys_scale);
    }
    else {
        if (p_value == P_NODE)
            scale_node(elec_scale, phys_scale);
        else if (p_value == P_NAME)
            scale_name(elec_scale, phys_scale);
        else if (p_value == P_MUT)
            scale_mut(elec_scale, phys_scale);
        else if (p_value == P_BRANCH)
            scale_branch(elec_scale, phys_scale);
        else if (p_value == P_SYMBLC)
            scale_symblc(elec_scale, phys_scale);
        else if (p_value == P_NODMAP)
            scale_nodmp(elec_scale, phys_scale);
        else if (p_value == P_MODEL)
            hy_scale(elec_scale);
        else if (p_value == P_VALUE)
            hy_scale(elec_scale);
        else if (p_value == P_PARAM)
            hy_scale(elec_scale);
        else if (p_value == P_OTHER)
            hy_scale(elec_scale);
        else if (p_value == P_DEVREF)
            hy_scale(elec_scale);
        else if (p_value == XICP_PLOT)
            hy_scale(elec_scale);
        else if (p_value == XICP_IPLOT)
            hy_scale(elec_scale);
    }
}


#define ESCALE(n) mmRnd((n)*elec_scale)
#define PSCALE(n) mmRnd((n)*phys_scale)

void
CDp::scale_grid(double, double phys_scale)
{
    if (phys_scale == 1.0)
        return;
    // "grid" *resol snap

    sLstr lstr;
    const char *s = p_string;
    char *tok;
    int tcnt = 0;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (tcnt == 0)
            lstr.add(tok);
        else if (tcnt == 1) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(PSCALE(d));
        }
        else if (tcnt == 2) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(d);
        }
        delete [] tok;
        tcnt++;
    }
    if (tcnt >= 2) {
        if (tcnt == 2) {
            lstr.add_c(' ');
            lstr.add("1");
        }
        delete [] p_string;
        p_string = lstr.string_trim();
    }
}


void
CDp::scale_node(double elec_scale, double phys_scale)
{
    // enode index *x[:*syx] *y[:*syy] [name *physx *physy flags lname

    if (elec_scale == 1.0 && phys_scale == 1.0)
        return;
    if (!p_string)
        return;

    sLstr lstr;
    const char *s = p_string;
    char *tok;
    int tcnt = 0;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (tcnt == 0) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_i(d);
        }
        else if (tcnt == 1) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(d);
        }
        else if (tcnt == 2) {
            int i1, i2;
            int j = sscanf(tok, "%d:%d", &i1, &i2);
            if (j < 1)
                return;
            if (j == 1) {
                lstr.add_c(' ');
                lstr.add_i(ESCALE(i1));
            }
            else {
                lstr.add_c(' ');
                lstr.add_i(ESCALE(i1));
                lstr.add_c(':');
                lstr.add_i(ESCALE(i2));
            }
        }
        else if (tcnt == 3) {
            int i1, i2;
            int j = sscanf(tok, "%d:%d", &i1, &i2);
            if (j < 1)
                return;
            if (j == 1) {
                lstr.add_c(' ');
                lstr.add_i(ESCALE(i1));
            }
            else {
                lstr.add_c(' ');
                lstr.add_i(ESCALE(i1));
                lstr.add_c(':');
                lstr.add_i(ESCALE(i2));
            }
        }
        else if (tcnt == 4) {
            lstr.add_c(' ');
            lstr.add(tok);
        }
        else if (tcnt == 5) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            if (d != -1)
                d = PSCALE(d);
            lstr.add_c(' ');
            lstr.add_i(d);
        }
        else if (tcnt == 6) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            if (d != -1)
                d = PSCALE(d);
            lstr.add_c(' ');
            lstr.add_i(d);
        }
        else if (tcnt == 7) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(d);
            if (*s) {
                lstr.add_c(' ');
                lstr.add(s);
                tcnt++;
            }
            s = 0;
        }
        delete [] tok;
        tcnt++;
    }
    if (tcnt < 4)
        return;
    delete [] p_string;
    p_string = lstr.string_trim();
}


void
CDp::scale_name(double, double phys_scale)
{
    // name num [subame *physx *physy]

    if (phys_scale == 1.0)
        return;
    if (!p_string)
        return;

    sLstr lstr;
    const char *s = p_string;
    char *tok;
    int tcnt = 0;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (tcnt == 0)
            lstr.add(tok);
        else if (tcnt == 1) {
            lstr.add_c(' ');
            lstr.add(tok);
        }
        else if (tcnt == 2) {
            lstr.add_c(' ');
            lstr.add(tok);
        }
        else if (tcnt == 3) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(PSCALE(d));
        }
        else if (tcnt == 4) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(PSCALE(d));
            delete [] p_string;
            p_string = lstr.string_trim();
            delete [] tok;
            break;
        }
        delete [] tok;
        tcnt++;
    }
}


void
CDp::scale_mut(double elec_scale, double)
{
    // *x1 *y1 *x2 *y2 coeff

    if (elec_scale == 1.0)
        return;
    if (!p_string)
        return;

    sLstr lstr;
    const char *s = p_string;
    char *tok;
    int tcnt = 0;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (tcnt == 0) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_i(ESCALE(d));
        }
        else if (tcnt == 1) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(ESCALE(d));
        }
        else if (tcnt == 2) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(ESCALE(d));
        }
        else if (tcnt == 3) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(ESCALE(d));
            if (*s) {
                lstr.add_c(' ');
                lstr.add(s);
            }
            delete [] p_string;
            p_string = lstr.string_trim();
            break;
        }
        delete [] tok;
        tcnt++;
    }
}


void
CDp::scale_branch(double elec_scale, double)
{
    // *x *y r1 r2 [string]

    if (elec_scale == 1.0)
        return;
    if (!p_string)
        return;

    sLstr lstr;
    const char *s = p_string;
    char *tok;
    int tcnt = 0;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (tcnt == 0) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_i(ESCALE(d));
        }
        else if (tcnt == 1) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(ESCALE(d));
        }
        else if (tcnt == 2) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(d);
        }
        else if (tcnt == 3) {
            int d;
            if (sscanf(tok, "%d", &d) != 1) {
                delete [] tok;
                return;
            }
            lstr.add_c(' ');
            lstr.add_i(d);
            if (*s) {
                lstr.add_c(' ');
                lstr.add(s);
            }
            delete [] p_string;
            p_string = lstr.string_trim();
            delete [] tok;
            break;
        }
        delete [] tok;
        tcnt++;
    }
}


void
CDp::scale_symblc(double elec_scale, double)
{
    if (elec_scale == 1.0)
        return;
    if (!p_string)
        return;

    const char *termination = ":\n";
    const char *s = p_string;
    while (isspace(*s))
        s++;
    sLstr lstr;
    if (*s == '0')
        lstr.add("0\n");
    else
        lstr.add("1\n");
    if (*s)
        s++;
    if (*s)
        s++;

    while (*s) {
        const char *nxt = s;
        while (isspace(*nxt))
            nxt++;
        if (!*nxt)
            break;

        sLstr lbuf;
        while (*nxt) {
            if (*nxt == ':' && (*(nxt+1) == '\n' || *(nxt+1) == '\r'))
                break;
            lbuf.add_c(*nxt++);
        }
        const char *c = lbuf.string();
        // lbuf contains single entry

        switch (*c) {
        case 'L':  // Layer
        case 'l':
            lstr.add("  ");
            lstr.add(lbuf.string());
            lstr.add(termination);
            break;
        case 'B': // Box
        case 'b': {
            c++;
            while (isspace(*c))
                c++;
            int w, h, x, y;
            if (sscanf(c, "%d %d %d %d", &w, &h, &x, &y) == 4) {
                lstr.add("  ");
                lstr.add("B ");
                lstr.add_i(ESCALE(w));
                lstr.add_c(' ');
                lstr.add_i(ESCALE(h));
                lstr.add_c(' ');
                lstr.add_i(ESCALE(x));
                lstr.add_c(' ');
                lstr.add_i(ESCALE(y));
                lstr.add(termination);
            }
            break;
        }
        case 'P': // Polygon
        case 'p': {
            c++;
            while (isspace(*c))
                c++;
            lstr.add("  ");
            lstr.add("P");
            Poly poly;
            getpts(c, &poly.points, &poly.numpts);
            for (int i = 0; i < poly.numpts; i++) {
                lstr.add_c(' ');
                lstr.add_i(ESCALE(poly.points[i].x));
                lstr.add_c(' ');
                lstr.add_i(ESCALE(poly.points[i].y));
            }
            delete [] poly.points;
            lstr.add(termination);
            break;
        }
        case 'W': // Wire
        case 'w': {
            c++;
            while (isspace(*c))
                c++;
            Wire wire;
            wire.set_wire_width(atoi(c));
            while (*c && !isspace(*c))
                c++;
            getpts(c, &wire.points, &wire.numpts);
            lstr.add("  ");
            lstr.add("W ");
            lstr.add_i(ESCALE(wire.wire_width()));
            for (int i = 0; i < wire.numpts; i++) {
                lstr.add_c(' ');
                lstr.add_i(ESCALE(wire.points[i].x));
                lstr.add_c(' ');
                lstr.add_i(ESCALE(wire.points[i].y));
            }
            delete [] wire.points;
            lstr.add(termination);
            break;
        }
        case '9': // Label
            c++;
            if (*c != '4')
                break;
            c++;
            {
                sLstr ls;
                CD()->GetLabel(&c, &ls);
                Label label;
                int nargs = sscanf(c, "%d%d%d%d%d", &label.x, &label.y,
                    &label.xform, &label.width, &label.height);
                // xform, width, height optional
                if (nargs < 2)
                    break;

                lstr.add("  ");
                lstr.add("94 <<");
                lstr.add(ls.string());
                lstr.add(">> ");
                lstr.add_i(ESCALE(label.x));
                lstr.add_c(' ');
                lstr.add_i(ESCALE(label.y));
                if (nargs >= 3) {
                    // xform
                    lstr.add_c(' ');
                    lstr.add_i(label.xform);
                }
                if (nargs >= 4) {
                    // width
                    lstr.add_c(' ');
                    lstr.add_i(ESCALE(label.width));
                }
                if (nargs == 5) {
                    // height
                    lstr.add_c(' ');
                    lstr.add_i(ESCALE(label.height));
                }
                lstr.add(termination);
                break;
            }
        }
        lbuf.free();
        if (*nxt == ':')
            nxt++;
        while (isspace(*nxt))
            nxt++;
        s = nxt;
    }
    delete [] p_string;
    p_string = lstr.string_trim();
}


void
CDp::scale_nodmp(double elec_scale, double)
{
    if (elec_scale == 1.0)
        return;
    if (!p_string)
        return;

    sLstr lstr;
    const char *stmp = p_string;
    for (;;) {
        char *tok = lstring::gettok(&stmp);
        if (!tok)
            break;
        char *xtok = lstring::gettok(&stmp);
        if (!xtok) {
            delete [] tok;
            break;
        }
        char *ytok = lstring::gettok(&stmp);
        if (!ytok) {
            delete [] tok;
            delete [] xtok;
            break;
        }
        int x, y;
        if (sscanf(xtok, "%d", &x) == 1 && sscanf(ytok, "%d", &y) == 1) {
            if (lstr.string())
                lstr.add_c(' ');
            lstr.add(tok);
            lstr.add_c(' ');
            lstr.add_i(ESCALE(x));
            lstr.add_c(' ');
            lstr.add_i(ESCALE(y));
        }
        delete [] tok;
        delete [] xtok;
        delete [] ytok;
    }
    delete [] p_string;
    p_string = lstr.string_trim();
}


// Scale hypertext references found in the string.
//
void
CDp::hy_scale(double escale)
{
    p_string = hyList::hy_scale(p_string, escale);
}


// Static function.
// Return the electrical name of the given property index, or the text
// string of the index if invalid (static storage!).
//
const char *
CDp::elec_prp_name(int num)
{
    static char buf[32];
    if (num < 1 || num > P_MAX_PRP_NUM) {
        sprintf(buf, "%d", num);
        return (buf);
    }
    return (prpty_name[num]);
}


// Static function.
// Return true and set pnum if str is recognized as an electrical
// property name.
//
bool
CDp::elec_prp_num(const char *str, int *pnum)
{
    if (!str)
        return (false);
    if (!str[0] || !str[1])
        return (false);
    if (!pnum)
        return (false);
    else
        *pnum = 0;
    char c1 = isupper(str[0]) ? tolower(str[0]) : str[0];
    if (c1 == 'b') {
        // bnode
        // branch
        if (lstring::ciprefix(str, prpty_name[P_BNODE])) {
            *pnum = P_BNODE;
            return (true);
        }
        if (lstring::ciprefix(str, prpty_name[P_BRANCH])) {
            *pnum = P_BRANCH;
            return (true);
        }
    }
    else if (c1 == 'd') {
        // devref
        if (lstring::ciprefix(str, prpty_name[P_DEVREF])) {
            *pnum = P_DEVREF;
            return (true);
        }
    }
    else if (c1 == 'f') {
        // flatten
        if (lstring::ciprefix(str, prpty_name[P_FLATTEN])) {
            *pnum = P_FLATTEN;
            return (true);
        }
    }
    else if (c1 == 'i') {
        // initc
        // Obsolete, kept for backward compatibility.
        if (lstring::ciprefix(str, "initc")) {
            *pnum = P_PARAM;
            return (true);
        }
    }
    else if (c1 == 'l') {
        // labloc
        // labrf
        const char *s = str + 1;
        char c = isupper(*s) ? tolower(*s) : *s;
        if (c == 'a') {
            s++;
            c = isupper(*s) ? tolower(*s) : *s;
            if (c == 'b') {
                s++;
                if (!isalpha(*s))
                    return (false);
                if (lstring::ciprefix(str, prpty_name[P_LABLOC])) {
                    *pnum = P_LABLOC;
                    return (true);
                }
                if (lstring::ciprefix(str, prpty_name[P_LABRF])) {
                    *pnum = P_LABRF;
                    return (true);
                }
            }
        }
    }
    else if (c1 == 'm') {
        // model
        // mut
        // mutlrf
        // macro
        if (lstring::ciprefix(str, prpty_name[P_MODEL])) {
            *pnum = P_MODEL;
            return (true);
        }
        const char *s = str + 1;
        char c = isupper(*s) ? tolower(*s) : *s;
        if (c == 'u') {
            s++;
            c = isupper(*s) ? tolower(*s) : *s;
            if (c == 't') {
                s++;
                if (!*s) {
                    *pnum = P_MUT;
                    return (true);
                }
                if (lstring::ciprefix(str, prpty_name[P_MUTLRF])) {
                    *pnum = P_MUTLRF;
                    return (true);
                }
            }
        }
        if (lstring::ciprefix(str, prpty_name[P_MACRO])) {
            *pnum = P_MACRO;
            return (true);
        }
    }
    else if (c1 == 'n') {
        // nophys
        // node
        // name
        // newmut
        // nodmap
        if (lstring::ciprefix(str, prpty_name[P_NAME])) {
            *pnum = P_NAME;
            return (true);
        }
        if (lstring::ciprefix(str, prpty_name[P_NEWMUT])) {
            *pnum = P_NEWMUT;
            return (true);
        }
        const char *s = str + 1;
        char c = isupper(*s) ? tolower(*s) : *s;
        if (c == 'o') {
            s++;
            c = isupper(*s) ? tolower(*s) : *s;
            if (c == 'p') {
                if (lstring::ciprefix(str, prpty_name[P_NOPHYS])) {
                    *pnum = P_NOPHYS;
                    return (true);
                }
            }
            else if (c == 'd') {
                s++;
                if (isalpha(*s)) {
                    if (lstring::ciprefix(str, prpty_name[P_NODE])) {
                        *pnum = P_NODE;
                        return (true);
                    }
                    if (lstring::ciprefix(str, prpty_name[P_NODMAP])) {
                        *pnum = P_NODMAP;
                        return (true);
                    }
                }
            }
        }
    }
    else if (c1 == 'o') {
        // other
        if (lstring::ciprefix(str, prpty_name[P_OTHER])) {
            *pnum = P_OTHER;
            return (true);
        }
    }
    else if (c1 == 'p') {
        // param
        if (lstring::ciprefix(str, prpty_name[P_PARAM])) {
            *pnum = P_PARAM;
            return (true);
        }
    }
    else if (c1 == 's') {
        // symblc
        if (lstring::ciprefix(str, prpty_name[P_SYMBLC])) {
            *pnum = P_SYMBLC;
            return (true);
        }
    }
    else if (c1 == 'v') {
        // value
        // virtual
        if (lstring::ciprefix(str, prpty_name[P_VALUE])) {
            *pnum = P_VALUE;
            return (true);
        }
        if (lstring::ciprefix(str, prpty_name[P_VIRTUAL])) {
            *pnum = P_VIRTUAL;
            return (true);
        }
    }
    return (false);
}
// End CDp functions


//
//  Physical Properties
//

// These are applied to physical objects that are the targets of the
// terminal's oset field.  These are used internally only
//
CDp_oset::CDp_oset(CDterm *t) : CDp(XICP_TERM_OSET)
{
    p_string = lstring::copy("oset");
    po_term = t;
};


CDp_oset::~CDp_oset()
{
    if (po_term)
        po_term->clear_ref();
}
// End CDp_oset functions


//
// Global Properties
//

CDp_glob::CDp_glob(const CDp_glob &pd) : CDp(pd.p_string, pd.p_value)
{
    pg_data = hyList::dup(pd.pg_data);
}


CDp_glob::~CDp_glob()
{
    hyList::destroy(pg_data);
}


bool
CDp_glob::print(sLstr *lstr, int, int) const
{
    if (pg_data) {
        char *s = hyList::string(pg_data, HYcvAscii, false);
        if (s) {
            lstr->add(s);
            delete [] s;
        }
        return (true);
    }
    if (p_string) {
        lstr->add(p_string);
        return (true);
    }
    return (false);
}


hyList *
CDp_glob::hpstring(CDs*) const
{
    return (hyList::dup(pg_data));
}
// End CDp_glob functions


//
//  Electrical Properties
//

//
// User property
//
// The model, value, param, other, and devref properties which provide
// parametric or device name information.  These are applied to cells
// and instances, except devref is applied to instances only.  It applies
// to current-controlled sources and switch devices and provides the
// reference branch device name.

// Parse the string and initialize.
//
CDp_user::CDp_user(CDs *sdesc, const char *str, int val) : CDp(0, val)
{
    pu_label = 0;
    pu_data = new hyList(sdesc, str, HYcvAscii);
}


CDp_user::CDp_user(const CDp_user &pd) : CDp(0, pd.p_value)
{
    pu_label = pd.pu_label;
    pu_data = hyList::dup(pd.pu_data);
}


CDp_user &
CDp_user::operator=(const CDp_user &pd)
{
    (CDp&)*this = (const CDp&)pd;
    pu_label = pd.pu_label;
    pu_data = hyList::dup(pd.pu_data);
    return (*this);
}


CDp_user::~CDp_user()
{
    hyList::destroy(pu_data);
}


// Print the property text to lstr, offset by xo, yo.
//
bool
CDp_user::print(sLstr *lstr, int, int) const
{
    if (!pu_data)
        return (false);
    char *s = hyList::string(pu_data, HYcvAscii, true);
    if (s) {
        lstr->add(s);
        delete [] s;
        return (true);
    }
    return (false);
}


hyList *
CDp_user::hpstring(CDs*) const
{
    return (hyList::dup(pu_data));
}


// Generate the label text for the corresponding label.
//
hyList *
CDp_user::label_text(bool *copied, CDc*) const
{
    *copied = false;
    return (pu_data);
}
// End CDp_user functions


//
// Vector Instance property.
//

CDp_range::CDp_range(const CDp_range &pr) : CDp(0, pr.p_value)
{
    pr_beg_range = pr.beg_range();
    pr_end_range = pr.end_range();
    pr_nodes = 0;
    pr_names = 0;
    pr_numnodes = 0;
    pr_asize = 0;
}


CDp_range &
CDp_range::operator=(const CDp_range &pr)
{
    (CDp&)*this = (const CDp&)pr;
    pr_beg_range = pr.beg_range();
    pr_end_range = pr.end_range();
    pr_nodes = 0;
    pr_names = 0;
    pr_numnodes = 0;
    pr_asize = 0;
    return (*this);
}


CDp_range::~CDp_range()
{
    delete [] pr_nodes;
    delete [] pr_names;
}


// Print the property text in lstr, offset by xo, yo.
//
bool
CDp_range::print(sLstr *lstr, int, int) const
{
    lstr->add_i(pr_beg_range);
    lstr->add_c(' ');
    lstr->add_i(pr_end_range);
    return (true);
}


// Parse input string for bus node values, and initialize.
// syntax: one index integer
//
bool
CDp_range::parse_range(const char *str)
{
    pr_beg_range = 0;
    pr_end_range = 0;

    char buf[128];
    const char *errmsg = "Parse error in RANGE property string: %s\n  %s.";
    const char *erripf = "token %d unsigned integer parse failed";

    if (str) {
        const char *s = str;

        // beg_range, unsigned integer
        char *tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 2);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pr_beg_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // end_range, unsigned integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 3);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pr_end_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }
    }
    return (true);
}


// Set up the array of node properties that are used to establish
// connectivity of the instance bits.
//
void
CDp_range::setup(const CDc *cdesc)
{
    if (!cdesc)
        return;
    CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
    if (!pn) {
        // keep the names
        delete [] pr_nodes;
        pr_nodes = 0;
        pr_asize = 0;
        pr_numnodes = 0;
        return;
    }
    unsigned int maxix = 0;
    for ( ; pn; pn = pn->next()) {
        if (pn->index() > maxix)
            maxix = pn->index();
    }
    maxix++;
    unsigned int ow = pr_numnodes ? pr_asize/pr_numnodes : 0;
    unsigned int nsize = (width() - 1)*maxix;
    if (nsize != pr_asize) {
        delete [] pr_nodes;
        pr_nodes = new CDp_cnode[nsize];
        pr_asize = nsize;
        if (ow != width() - 1) {
            delete [] pr_names;
            pr_names = new CDp_name[width() - 1];
        }
    }
    pr_numnodes = maxix;

    // The name properties are used only for the physical location
    // coordinates.

    pn = (CDp_cnode*)cdesc->prpty(P_NODE);
    for ( ; pn; pn = pn->next()) {
        pn->set_enode(-1);
        CDgenRange rgen(this);
        unsigned int ix = 0;
        rgen.next(0);
        while (rgen.next(0)) {
            CDp_cnode &p = pr_nodes[maxix*ix++ + pn->index()];
            p = *pn;
            // Above will copy the terminal struct and set the
            // property back-pointer, but does not set the instance.

            // Note that the index should be set explicitly, as it may
            // not be set in the source terminal.

            if (p.inst_terminal() && pn->inst_terminal() &&
                    pn->inst_terminal()->instance())
                p.inst_terminal()->set_instance(
                    pn->inst_terminal()->instance(), ix);
        }
    }
}


// Return the node property.  The instix is the 0-based index of the
// instance bit, 0 corresponding to the beg range value.  The ndix is
// the index of the node to retrieve.  Zero is returned on indexing
// error or if the nodes have not been set up.
//
// The cdesc is required only for instix == 0.
//
CDp_cnode *
CDp_range::node(const CDc *cdesc, unsigned int instix, unsigned int ndix) const
{
    if (ndix >= pr_numnodes)
        return (0);
    if (instix == 0) {
        if (!cdesc)
            return (0);
        CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            if (pn->index() == ndix)
                return (pn);
        }
        return (0);
    }
    if (instix >= width())
        return (0);
    unsigned int ix = (instix - 1)*pr_numnodes + ndix;
    if (ix >= pr_asize)
        return (0);
    return (&pr_nodes[ix]);
}


void
CDp_range::print_nodes(const CDc *cdesc, FILE *fp, sLstr *lstr) const
{
    int cnt = 0;
    CDgenRange rgen(this);
    while (rgen.next(0)) {
        CDp_cnode *pn = (CDp_cnode*)cdesc->prpty(P_NODE);
        for ( ; pn; pn = pn->next()) {
            CDp_cnode *px = node(cdesc, cnt, pn->index());
            if (px) {
                if (fp)
                    fprintf(fp, " %-4d", px->enode());
                else {
                    char buf[32];
                    sprintf(buf, " %-4d", px->enode());
                    lstr->add(buf);
                }
            }
        }
        if (fp)
            fputs("\n", fp);
        else
            lstr->add_c('\n');
        cnt++;
    }
}


CDp_name *
CDp_range::name_prp(const CDc *cdesc, unsigned int instix) const
{
    if (instix == 0) {
        if (!cdesc)
            return (0);
        return ((CDp_name*)cdesc->prpty(P_NAME));
    }
    if (instix >= width())
        return (0);
    if (!pr_names)
        return (0);
    return (&pr_names[instix - 1]);
}
// End CDp_range functions


//
// Bus node property base class for wires
//

CDp_bnode::CDp_bnode(const CDp_bnode &pd) : CDp(0, pd.p_value)
{
    pbn_beg_range = pd.pbn_beg_range;
    pbn_end_range = pd.pbn_end_range;
    pbn_name = pd.pbn_name;
    pbn_bundle = CDnetex::dup(pd.bundle_spec());
    pbn_label = 0;
}


CDp_bnode &
CDp_bnode::operator=(const CDp_bnode &pd)
{
    (CDp&)*this = (const CDp&)pd;
    pbn_beg_range = pd.pbn_beg_range;
    pbn_end_range = pd.pbn_end_range;
    pbn_name = pd.pbn_name;
    pbn_bundle = CDnetex::dup(pd.bundle_spec());
    pbn_label = 0;
    return (*this);
}


CDp_bnode::~CDp_bnode()
{
    CDnetex::destroy(pbn_bundle);
}


// Print the property text in lstr, offset by xo, yo.
//
bool
CDp_bnode::print(sLstr *lstr, int, int) const
{
    lstr->add_i(pbn_beg_range);
    lstr->add_c(' ');
    lstr->add_i(pbn_end_range);
    if (pbn_bundle) {
        lstr->add_c(' ');
        add_label_text(lstr);
    }
    return (true);
}


// Parse input string for bus node values, and initialize.
// syntax: beg_range end_range
// (all integers)
//
bool
CDp_bnode::parse_bnode(const char *str)
{
    pbn_beg_range = 0;
    pbn_end_range = 0;

    const char *errmsg = "Parse error in BNODE property string: %s\n  %s.";
    const char *erripf = "token %d integer parse failed";
    char buf[128];

    if (str) {
        const char *s = str;

        // beg_range, unsigned integer
        char *tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 2);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbn_beg_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // end_range, unsigned integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 3);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbn_end_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }
    }
    return (true);
}


void
CDp_bnode::update_bundle(CDnetex *nx)
{
    CDnetex::destroy(pbn_bundle);
    pbn_bundle = 0;
    if (!nx)
        return;

    CDnetName nm;
    if (nx->is_scalar(&nm)) {
        // This is a scalar net, so we shouldn't be here.  Treat as a
        // 1-element 0:0 vector.
        pbn_beg_range = 0;
        pbn_end_range = 0;
        pbn_name = nm;
        CDnetex::destroy(nx);
        return;
    }

    int b, e;
    if (nx->is_simple(&nm, &b, &e)) {
        // Set the range to be consistent.
        pbn_beg_range = b;
        pbn_end_range = e;
        pbn_name = nm;
        CDnetex::destroy(nx);
        return;
    }

    // Set the range to cover the overall width.
    unsigned int wid = nx->width();
    if (wid > 0)
        wid--;
    pbn_beg_range = 0;
    pbn_end_range = wid;
    pbn_bundle = nx;
    pbn_name = 0;
}


// Put the net expresion into lstr.
//
void
CDp_bnode::add_label_text(sLstr *lstr) const
{
    if (!lstr)
        return;
    if (!pbn_bundle) {
        lstr->add(pbn_name->string());
        lstr->add_c('<');
        lstr->add_i(pbn_beg_range);
        lstr->add_c(':');
        lstr->add_i(pbn_end_range);
        lstr->add_c('>');
        return;
    }
    CDnetex::print_all(pbn_bundle, lstr);
}


// Return a netex for the property, including the case where the
// property is unnamed.
//
CDnetex *
CDp_bnode::get_netex() const
{
    if (pbn_bundle)
        return (CDnetex::dup(pbn_bundle));
    CDvecex *vx = new CDvecex(0, pbn_beg_range, pbn_end_range, 1, 1);
    return (new CDnetex(pbn_name, 1, vx, 0));
}


// Return true if the terminal can access its bits by name.
//
bool
CDp_bnode::has_name() const
{
    return (pbn_name || (pbn_bundle && pbn_bundle->first_name()));
}
// End CDp_bnode functions


NetexWrap::~NetexWrap()
{
    CDnetex::destroy(nxtmp);
}
// End of NetexWrap functions.


//
// Bus node property class for instances
//

CDp_bcnode::CDp_bcnode(const CDp_bcnode &pd) : CDp_bnode(pd)
{
    pbcn_index = pd.pbcn_index;
    pbcn_flags = pd.pbcn_flags;
    pbcn_map = 0;
    if (pd.pbcn_map) {
        unsigned int sz = width();
        pbcn_map = new unsigned short[sz];
        memcpy(pbcn_map, pd.pbcn_map, sz*sizeof(unsigned short));
    }

    if (pd.pts_flag()) {
        int sz = pd.pbcn_u.pts[0];
        sz += (sz + 1);
        pbcn_u.pts = new int[sz];
        memcpy(pbcn_u.pts, pd.pbcn_u.pts, sz*sizeof(int));
    }
    else {
        pbcn_u.pos[0] = pd.pbcn_u.pos[0];
        pbcn_u.pos[1] = pd.pbcn_u.pos[1];
    }
}


CDp_bcnode &
CDp_bcnode::operator=(const CDp_bcnode &pd)
{
    (CDp_bnode&)*this = (const CDp_bnode&)pd;
    pbcn_index = pd.pbcn_index;
    pbcn_flags = pd.pbcn_flags;
    pbcn_map = 0;
    if (pd.pbcn_map) {
        unsigned int sz = width();
        pbcn_map = new unsigned short[sz];
        memcpy(pbcn_map, pd.pbcn_map, sz*sizeof(unsigned short));
    }

    if (pd.pts_flag()) {
        int sz = pd.pbcn_u.pts[0];
        sz += (sz + 1);
        pbcn_u.pts = new int[sz];
        memcpy(pbcn_u.pts, pd.pbcn_u.pts, sz*sizeof(int));
    }
    else {
        pbcn_u.pos[0] = pd.pbcn_u.pos[0];
        pbcn_u.pos[1] = pd.pbcn_u.pos[1];
    }
    return (*this);
}


CDp_bcnode::~CDp_bcnode()
{
    if (pts_flag())
        delete [] pbcn_u.pts;
    delete [] pbcn_map;
}


// Print the property text in lstr, offset by xo, yo.
//
bool
CDp_bcnode::print(sLstr *lstr, int xo, int yo) const
{
    lstr->add_i(pbcn_index);
    lstr->add_c(' ');
    lstr->add_i(pbn_beg_range);
    lstr->add_c(' ');
    lstr->add_i(pbn_end_range);
    lstr->add_c(' ');

    // Multi-points are comma-separated without space.
    for (unsigned int ix = 0; ; ix++) {
        int x, y;
        if (!get_pos(ix, &x, &y))
            break;
        if (ix)
            lstr->add_c(',');
        lstr->add_i(x - xo);
    }
    lstr->add_c(' ');
    for (unsigned int ix = 0; ; ix++) {
        int x, y;
        if (!get_pos(ix, &x, &y))
            break;
        if (ix)
            lstr->add_c(',');
        lstr->add_i(y - yo);
    }

    // We write the name, but ignore this when parsing as input.
    char *name = full_name();
    if (name) {
        lstr->add_c(' ');
        lstr->add(name);
        delete [] name;
    }
    return (true);
}


// Parse input string for bus node values, and initialize.
// syntax: beg_range end_range x y
// The x and y can be commma-separated integer lists.
//
bool
CDp_bcnode::parse_bcnode(const char *str)
{
    pbn_beg_range = 0;
    pbn_end_range = 0;
    pbcn_index = 0;
    alloc_pos(0);

    const char *errmsg = "Parse error in BNODE property string: %s\n  %s.";
    const char *erripf = "token %d integer parse failed";
    char buf[128];

    if (str) {
        const char *s = str;

        // index, integer
        char *tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 1);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbcn_index = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // beg_range, integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 2);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbn_beg_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // end_range, integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 3);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbn_end_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // x and y values, these may be integers or comma-separated
        // integer lists

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }
        int numxvals;
        int *xvals;
        if (!slurp_ints(tok, &numxvals, &xvals)) {
            Errs()->add_error(errmsg, "bad x value string", str);
            delete [] tok;
            return (false);
        }
        delete [] tok;

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            delete [] xvals;
            return (false);
        }
        int numyvals;
        int *yvals;
        if (!slurp_ints(tok, &numyvals, &yvals)) {
            Errs()->add_error(errmsg, "bad y value string", str);
            delete [] tok;
            delete [] xvals;
            return (false);
        }
        delete [] tok;

        if (numxvals != numyvals) {
            Errs()->add_error(errmsg, "x/y component count mismatch", str);
            delete [] xvals;
            delete [] yvals;
            return (false);
        }
        alloc_pos(numxvals);
        for (int i = 0; i < numxvals; i++)
            set_pos(i, xvals[i], yvals[i]);
        delete [] xvals;
        delete [] yvals;
    }
    return (true);
}


// Set a name for the terminal.  If the name is in the form of a
// default name, the assigned name pointer will be zeroed.  The caller
// is responsible for uniqueness and suitable syntax of the name.
//
void
CDp_bcnode::set_term_name(const char *n)
{
    if (CDnetex::is_default_name(n))
        pbn_name = 0;
    else
        pbn_name = CDnetex::name_tab_add(n);
}


// Return the name of the terminal.  This will always return a name,
// either the assigned name if given, or a default name otherwise.
//
CDnetName
CDp_bcnode::term_name() const
{
    if (pbn_name)
        return (pbn_name);
    return (CDnetex::default_name(index()));
}


// The "full name" as used in property strings is in the form
//   [name][:netex]
// This can be null/empty.  A simple range is never included, as
// the overall range values are printed separately.
//
// For a CDp_bcnode of a normal instance:
//   - The name will be generated by Xic.
//   - There will be no netex.
// For a CDp_bcnode of a bus terminal instance:
//   - The name will be null.
//   - There the netex corresponds to the label.
// For a CDp_bsnode of a bus terminal instance:
//   - The name is optional, if given it will be used when generating
//     the corresponding name in instances.
//   - The netex may or may not be set.  This provides a
//     connect-by-name mapping within the cell.  It does NOT propagate
//     to instances.
//
char *
CDp_bcnode::full_name() const
{
    const char *nmstr = pbn_name->string();
    // The nmstr is always a simple text token, no range, index, or
    // commas.

    sLstr lstr;
    CDnetex::print_all(bundle_spec(), &lstr);
    if (!lstr.string())
        return (lstring::copy(nmstr));
    if (!nmstr)
        return (lstr.string_trim());
    sLstr nstr;
    nstr.add(nmstr);
    nstr.add_c(':');
    lstr.add(lstr.string());
    return (nstr.string_trim());
}


// Return a name string, using a default if no name was given.
//
char *
CDp_bcnode::id_text() const
{
    sLstr lstr;   
    if (!pbn_bundle) {
        lstr.add(term_name()->string());
        lstr.add_c('<');
        lstr.add_i(pbn_beg_range);
        lstr.add_c(':');
        lstr.add_i(pbn_end_range);
        lstr.add_c('>');
    }
    else
        CDnetex::print_all(pbn_bundle, &lstr);
    return (lstr.string_trim());
}


// In a normal cell instance, the bus node properties have no bundle
// list, but do have a range consistent with the corresponding
// terminal in the master.  This is established here.
//
// The exception is in terminal device instances, where the bundle
// list is obtained from the terminal net expression.
//
void
CDp_bcnode::update_range(const CDp_bsnode *ps)
{
    if (ps) {
        pbn_beg_range = ps->beg_range();
        pbn_end_range = ps->end_range();
        pbcn_index = ps->index();
        delete [] pbcn_map;
        pbcn_map = 0;
        if (ps->index_map()) {
            unsigned int sz = width();
            pbcn_map = new unsigned short[sz];
            memcpy(pbcn_map, ps->index_map(), sz*sizeof(unsigned short));
        }
    }
}
// End CDp_bcnode functions


//
// Bus node property class for cells
//

CDp_bsnode::CDp_bsnode(const CDp_bsnode &pd) : CDp_bcnode(pd)
{
    pbsn_x = pd.pbsn_x;
    pbsn_y = pd.pbsn_y;
}


CDp_bsnode &
CDp_bsnode::operator=(const CDp_bsnode &pd)
{
    (CDp_bcnode&)*this = (const CDp_bcnode&)pd;
    pbsn_x = pd.pbsn_x;
    pbsn_y = pd.pbsn_y;
    return (*this);
}


// Print the property text in lstr, offset by xo, yo.
//
bool
CDp_bsnode::print(sLstr *lstr, int xo, int yo) const
{
    lstr->add_i(pbcn_index);
    lstr->add_c(' ');
    lstr->add_i(pbn_beg_range);
    lstr->add_c(' ');
    lstr->add_i(pbn_end_range);
    lstr->add_c(' ');

    if (pts_flag()) {
        lstr->add_i(pbsn_x - xo);
        lstr->add_c(':');
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!get_pos(ix, &x, &y))
                break;
            if (ix)
                lstr->add_c(',');
            lstr->add_i(x - xo);
        }
        lstr->add_c(' ');
        lstr->add_i(pbsn_y - yo);
        lstr->add_c(':');
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!get_pos(ix, &x, &y))
                break;
            if (ix)
                lstr->add_c(',');
            lstr->add_i(y - yo);
        }
    }
    else {
        if (pbsn_x != pbcn_u.pos[0] || pbsn_y != pbcn_u.pos[1]) {
            lstr->add_i(pbsn_x - xo);
            lstr->add_c(':');
            lstr->add_i(pbcn_u.pos[0] - xo);
            lstr->add_c(' ');
            lstr->add_i(pbsn_y - yo);
            lstr->add_c(':');
            lstr->add_i(pbcn_u.pos[1] - yo);
        }
        else {
            lstr->add_i(pbsn_x - xo);
            lstr->add_c(' ');
            lstr->add_i(pbsn_y - yo);
        }
    }
    char *name = full_name();
    if (name) {
        lstr->add_c(' ');
        lstr->add(name);
        delete [] name;
    }
    return (true);
}


// Parse input string for bus node values, and initialize.
// syntax: beg_range end_range x y
// (all integers)
//
bool
CDp_bsnode::parse_bsnode(const char *str)
{
    pbn_beg_range = 0;
    pbn_end_range = 0;
    pbcn_index = 0;
    alloc_pos(0);
    pbsn_x = 0;
    pbsn_y = 0;

    const char *errmsg = "Parse error in NODE property string: %s\n  %s.";
    const char *erripf = "token %d integer parse failed";
    char buf[128];

    if (str) {
        const char *s = str;

        // index, integer
        char *tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 1);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbcn_index = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // beg_range, integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 2);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbn_beg_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // end_range, integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int u;
            int n = sscanf(tok, "%u", &u);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 3);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            pbn_end_range = u;
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // x and y values, these may be integers or comma-separated
        // integer lists

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }
        int numxvals;
        int *xvals;
        if (!slurp_ints(tok, &numxvals, &xvals)) {
            Errs()->add_error(errmsg, "bad x value string", str);
            delete [] tok;
            return (false);
        }
        delete [] tok;

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            delete [] xvals;
            return (false);
        }
        int numyvals;
        int *yvals;
        if (!slurp_ints(tok, &numyvals, &yvals)) {
            Errs()->add_error(errmsg, "bad y value string", str);
            delete [] tok;
            delete [] xvals;
            return (false);
        }
        delete [] tok;

        if (numxvals > 2 || numyvals > 2) {
            if (numxvals != numyvals) {
                Errs()->add_error(errmsg, "x/y component count mismatch", str);
                delete [] xvals;
                delete [] yvals;
                return (false);
            }
            pbsn_x = xvals[0];
            pbsn_y = yvals[0];
            numxvals--;
            alloc_pos(numxvals);
            for (int i = 0; i < numxvals; i++)
                set_pos(i, xvals[i+1], yvals[i+1]);
        }
        else {
            pbsn_x = xvals[0];
            pbsn_y = yvals[0];
            pbcn_u.pos[0] = numxvals == 1 ? xvals[0] : xvals[1];
            pbcn_u.pos[1] = numyvals == 1 ? yvals[0] : yvals[1];
        }
        delete [] xvals;
        delete [] yvals;

        // name and/or net expression:  [name][:netex]
        tok = lstring::getqtok(&s);
        if (tok) {
            const char *estart = tok;
            const char *tchrs = "<[{(,";
            for (char *t = tok; *t; t++) {
                // Found a name, this applies when a netex is given that
                // is not a simple vector.
                if (*t == ':') {
                    *t = 0;
                    set_term_name(tok);
                    estart = t+1;
                    break;
                }
                if (strchr(tchrs, *t))
                    break;
            }

            bool nxc = false;
            for (const char *t = estart; *t; t++) {
                if (strchr(tchrs, *t)) {
                    nxc = true;
                    break;
                }
            }

            if (!nxc) {
                // Not a netex, but may be a simple name.  We already
                // have the range.
                if (*estart)
                    set_term_name(estart);
                delete [] tok;
                return (true);
            }

            CDnetex *nx;
            if (!CDnetex::parse(estart, &nx)) {
                Errs()->add_error(errmsg,
                    "farse error for net expression", str);
                update_bundle(0);
                delete [] tok;
                return (false);
            }
            update_bundle(nx);
            delete [] tok;
        }
    }
    return (true);
}


// Create an instance bus node from the structure bus node, which is
// assumed linked in sdesc.  Used when instantiating, the x,y field
// must later be transformed.
//
CDp_bcnode *
CDp_bsnode::dupc(CDs *sdesc, const CDc *cdesc)
{
    CDp_bcnode *pn = new CDp_bcnode;
    pn->set_index(pbcn_index);
    pn->update_range(this);

    if (sdesc && sdesc->owner())
        sdesc = sdesc->owner();
    if (sdesc && sdesc->symbolicRep(cdesc)) {
        unsigned int sz = pts_flag() ? pbcn_u.pts[0] : 1;
        pn->alloc_pos(sz);
        for (unsigned int ix = 0; ix < sz; ix++) {
            int x, y;
            if (get_pos(ix, &x, &y))
                pn->set_pos(ix, x, y);
        }
    }
    else {
        pn->alloc_pos(0);
        pn->set_pos(0, pbsn_x, pbsn_y);
    }
    return (pn);
}
// End CDp_bsnode functions


//
// Node property base class
//
// This is the base class for nodes, used for wires.

CDp_node::CDp_node(const CDp_node &pd) : CDp(0, pd.p_value)
{
    pno_enode = pd.pno_enode;
    pno_name = pd.pno_name;
    pno_label = 0;
}


CDp_node &
CDp_node::operator=(const CDp_node &pd)
{
    (CDp&)*this = (const CDp&)pd;
    pno_enode = pd.pno_enode;
    pno_name = pd.pno_name;
    pno_label = 0;
    return (*this);
}


// Print the property text in lstr, offset by xo, yo.
//
bool
CDp_node::print(sLstr *lstr, int, int) const
{
    lstr->add_i(pno_enode);
    return (true);
}


// Parse input string for node values, and initialize.
// syntax: enode ignored x y
// (all integers)
//
bool
CDp_node::parse_node(const char *str)
{
    pno_enode = 0;

    // There are three unused fields that follow, these are ignored
    // here.
    if (str && sscanf(str, "%d", &pno_enode) != 1) {
        Errs()->add_error("Bad NODE property string: %s.", str);
        return (false);
    }
    return (true);
}
// End CDp_node functions

// Extended node property, used as base for cell and instance nodes.

CDp_nodeEx::CDp_nodeEx(const CDp_nodeEx &pd) : CDp_node(pd)
{
    if (pd.pts_flag()) {
        int sz = pd.pxno_u.pts[0];
        sz += (sz + 1);
        pxno_u.pts = new int[sz];
        memcpy(pxno_u.pts, pd.pxno_u.pts, sz*sizeof(int));
    }
    else {
        pxno_u.pos[0] = pd.pxno_u.pos[0];
        pxno_u.pos[1] = pd.pxno_u.pos[1];
    }
    pxno_flags = pd.pxno_flags;
    pxno_termtype = pd.pxno_termtype;
    pxno_index = pd.pxno_index;
}


CDp_nodeEx &
CDp_nodeEx::operator=(const CDp_nodeEx &pd)
{
    (CDp_node&)*this = (const CDp_node&)pd;
    if (pd.pts_flag()) {
        int sz = pd.pxno_u.pts[0];
        sz += (sz + 1);
        pxno_u.pts = new int[sz];
        memcpy(pxno_u.pts, pd.pxno_u.pts, sz*sizeof(int));
    }
    else {
        pxno_u.pos[0] = pd.pxno_u.pos[0];
        pxno_u.pos[1] = pd.pxno_u.pos[1];
    }
    pxno_flags = pd.pxno_flags;
    pxno_termtype = pd.pxno_termtype;
    pxno_index = pd.pxno_index;
    return (*this);
}


CDp_nodeEx::~CDp_nodeEx()
{
    if (pts_flag())
        delete [] pxno_u.pts;
}


// Set a name for the terminal.  If the name is in the form of a
// default name, the assigned name pointer will be zeroed.  The caller
// is responsible for uniqueness and suitable syntax of the name.
//
void
CDp_nodeEx::set_term_name(const char *n)
{
    if (CDnetex::is_default_name(n))
        pno_name = 0;
    else
        pno_name = CDnetex::name_tab_add(n);
}


// Return the name of the terminal.  This will always return a name,
// either the assigned name if given, or a default name otherwise.
//
CDnetName
CDp_nodeEx::term_name() const
{
    if (pno_name)
        return (pno_name);
    return (CDnetex::default_name(index()));
}


// Return a name string, using a default if no name was given.
//
char *
CDp_nodeEx::id_text() const
{
    return (lstring::copy(term_name()->string()));
}
// End of CDp_nodeEx functions.


// Node property for instances
//
// Derived node class used for instance property.  The coordinate
// values are generally derived from the structure node class (below). 
// These values depend on whether or not the cell is symbolic.  The
// instance node has fields for an "internal" node for port-ordering,
// and a Terminal struct.

CDp_cnode::CDp_cnode(const CDp_cnode &pd) : CDp_nodeEx(pd)
{
    if (pd.pcno_term) {
        pcno_term = new CDcterm(*pd.pcno_term);
        pcno_term->set_node_prpty(this);
    }
    else
        pcno_term = 0;
}


CDp_cnode &
CDp_cnode::operator=(const CDp_cnode &pd)
{
    (CDp_nodeEx&)*this = (const CDp_nodeEx&)pd;
    if (pd.pcno_term) {
        pcno_term = new CDcterm(*pd.pcno_term);
        pcno_term->set_node_prpty(this);
    }
    else
        pcno_term = 0;
    return (*this);
}


CDp_cnode::~CDp_cnode()
{
    delete pcno_term;
}


// Print the instance node text to lstr, offset by xo, yo.
//
bool
CDp_cnode::print(sLstr *lstr, int xo, int yo) const
{
    lstr->add_i(enode());
    lstr->add_c(' ');
    lstr->add_i(index());
    lstr->add_c(' ');

    // Multi-points are comma-separated without space.
    for (unsigned int ix = 0; ; ix++) {
        int x, y;
        if (!get_pos(ix, &x, &y))
            break;
        if (ix)
            lstr->add_c(',');
        lstr->add_i(x - xo);
        if (CD()->Out32nodes())
            break;
    }
    lstr->add_c(' ');
    for (unsigned int ix = 0; ; ix++) {
        int x, y;
        if (!get_pos(ix, &x, &y))
            break;
        if (ix)
            lstr->add_c(',');
        lstr->add_i(y - yo);
        if (CD()->Out32nodes())
            break;
    }

    if (CD()->Out32nodes()) {
        if (!inst_terminal())
            return (true);

        lstr->add_c(' ');
        lstr->add(term_name()->string());

        lstr->add_c(' ');
        lstr->add_i(inst_terminal()->lx());
        lstr->add_c(' ');
        lstr->add_i(inst_terminal()->ly());

        lstr->add(" 0 ");
        CDl *ld = inst_terminal()->layer();
        lstr->add(ld ? ld->name() : "?");

        if (pxno_termtype != TE_tINPUT) {
            for (FlagDef *f = TermTypes; f->name; f++) {
                if (pxno_termtype == f->value) {
                    lstr->add_c(' ');
                    lstr->add(f->name);
                    break;
                }
            }
        }
    }
    else if (pno_name) {
        // We'll print the assigned name, but the reader will ignore it.
        lstr->add_c(' ');
        lstr->add(pno_name->string());
    }

    return (true);
}


// Return a flags vector, pulling the bits from the proper place.
//
unsigned int
CDp_cnode::term_flags() const
{
    unsigned int f = pxno_flags;
    if (pcno_term && pcno_term->is_fixed())
        f |= TE_FIXED;
    else
        f &= ~TE_FIXED;
    if (pcno_term && pcno_term->is_uninit())
        f |= TE_UNINIT;
    else
        f &= ~TE_UNINIT;
    if (pcno_term && pcno_term->is_loc_set())
        f |= TE_LOCSET;
    else
        f &= ~TE_LOCSET;
    return (f);
}


// Parse input string for instance node values, and initialize. 
// syntax:  enode index x y [tname physx physy flags layer_name type]
// where enode, index, physx, physy are integers, x and y are possibly
// comma-separated integer lists, tname and layer_name are strings,
// and flags is an integer.
//
bool
CDp_cnode::parse_cnode(const char *str)
{
    pno_enode = 0;
    pno_name = 0;
    alloc_pos(0);
    pxno_flags = 0;
    pxno_termtype = TE_tINPUT;
    pxno_index = 0;

    const char *errmsg = "Parse error in NODE property string: %s\n  %s.";
    const char *erripf = "token %d integer parse failed";
    char buf[128];

    if (str) {
        const char *s = str;

        // enode, integer
        char *tok = lstring::gettok(&s);
        if (tok) {
            int n = sscanf(tok, "%d", &pno_enode);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 1);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // index, integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int ival;
            int n = sscanf(tok, "%u", &ival);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 2);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            if (ival >= 1024*1024) {
                Errs()->add_error(errmsg, "bad index value", str);
                return (false);
            }
            set_index(ival);
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // x and y values, these may be integers or comma-separated
        // integer lists

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }
        int numxvals;
        int *xvals;
        if (!slurp_ints(tok, &numxvals, &xvals)) {
            Errs()->add_error(errmsg, "bad x value string", str);
            delete [] tok;
            return (false);
        }
        delete [] tok;

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            delete [] xvals;
            return (false);
        }
        int numyvals;
        int *yvals;
        if (!slurp_ints(tok, &numyvals, &yvals)) {
            Errs()->add_error(errmsg, "bad y value string", str);
            delete [] tok;
            delete [] xvals;
            return (false);
        }
        delete [] tok;

        if (numxvals != numyvals) {
            Errs()->add_error(errmsg, "x/y component count mismatch", str);
            delete [] xvals;
            delete [] yvals;
            return (false);
        }
        alloc_pos(numxvals);
        for (int i = 0; i < numxvals; i++)
            set_pos(i, xvals[i], yvals[i]);
        delete [] xvals;
        delete [] yvals;

        // That's all of the parameters we need, anything else is
        // obtained from the corresponding CDp_snode.
    }
    return (true);
}
// End CDp_cnode functions


//
// Node property for structures
//
// Derived from the instance node class, with an additional position
// field.  The additional poszition saves the schematic hot-spot.  The
// inherited position list is used for the symbolic representation
// only.
//
// Here we will support the old 3.2 string format, as well as the
// present format.
//
// 3.2 node property:
// 5 10 node index ex ey [name px py flags lname ttype_name]
//
// 3.3 node property
// 5 10 node index ex,, ey,, [(flgs<<8)|ttype name px py lname]
// Differences:
//   The ex/ey may contain lists of points.
//   The flags.ttype is a HEX ENCODED integer that MUST begin with "0x"
//   or "0X".  (flags/24 | ttype/8).
//
// If CD()->Out32nodes() is true, the 3.2 node string will be created
// by the print method.  The reader will use the "0x" to detect
// new format.
//
// If the px argument is given in either case, a CDterm struct will be
// created.

CDp_snode::CDp_snode(const CDp_snode &pd) : CDp_nodeEx(pd)
{
    if (pd.psno_term) {
        psno_term = new CDsterm(*pd.psno_term);
        psno_term->set_node_prpty(this);
        pxno_flags |= TE_OWNTERM;
    }
    else
        psno_term = 0;
    psno_x = pd.psno_x;
    psno_y = pd.psno_y;
}


CDp_snode &
CDp_snode::operator=(const CDp_snode &pd)
{
    (CDp_nodeEx&)*this = (const CDp_nodeEx&)pd;
    if (pd.psno_term) {
        psno_term = new CDsterm(*pd.psno_term);
        psno_term->set_node_prpty(this);
        pxno_flags |= TE_OWNTERM;
    }
    else
        psno_term = 0;
    psno_x = pd.psno_x;
    psno_y = pd.psno_y;
    return (*this);
}


CDp_snode::~CDp_snode()
{
    if (psno_term) {
        if (pxno_flags & TE_OWNTERM)
            delete psno_term;
        else
            psno_term->set_node_prpty(0);
    }
}


// Print the structure node to lstr, offset by xo, yo.
//
bool
CDp_snode::print(sLstr *lstr, int xo, int yo) const
{
    lstr->add_i(enode());
    lstr->add_c(' ');
    lstr->add_i(index());
    lstr->add_c(' ');

    if (pts_flag()) {
        lstr->add_i(psno_x - xo);
        lstr->add_c(':');
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!get_pos(ix, &x, &y))
                break;
            if (ix)
                lstr->add_c(',');
            lstr->add_i(x - xo);
            if (CD()->Out32nodes())
                break;
        }
        lstr->add_c(' ');
        lstr->add_i(psno_y - yo);
        lstr->add_c(':');
        for (unsigned int ix = 0; ; ix++) {
            int x, y;
            if (!get_pos(ix, &x, &y))
                break;
            if (ix)
                lstr->add_c(',');
            lstr->add_i(y - yo);
            if (CD()->Out32nodes())
                break;
        }
    }
    else {
        if (psno_x != pxno_u.pos[0] || psno_y != pxno_u.pos[1]) {
            lstr->add_i(psno_x - xo);
            lstr->add_c(':');
            lstr->add_i(pxno_u.pos[0] - xo);
            lstr->add_c(' ');
            lstr->add_i(psno_y - yo);
            lstr->add_c(':');
            lstr->add_i(pxno_u.pos[1] - yo);
        }
        else {
            lstr->add_i(psno_x - xo);
            lstr->add_c(' ');
            lstr->add_i(psno_y - yo);
        }
    }

    if (CD()->Out32nodes()) {
        // termname posx posy flags lname type

        if (!cell_terminal())
            return (true);

        lstr->add_c(' ');
        lstr->add(term_name()->string());

        // phys position x, y
        lstr->add_c(' ');
        lstr->add_i(cell_terminal()->lx());
        lstr->add_c(' ');
        lstr->add_i(cell_terminal()->ly());

        // flags
        lstr->add_c(' ');
        lstr->add_i(term_flags() & TE_IOFLAGS);

        // terminal layer name
        CDl *ld = cell_terminal()->layer();
        if (ld) {
            lstr->add_c(' ');
            lstr->add(ld->name());
        }

        if (pxno_termtype != TE_tINPUT) {
            if (!ld)
                lstr->add(" ?");
            for (FlagDef *f = TermTypes; f->name; f++) {
                if (pxno_termtype == f->value) {
                    lstr->add_c(' ');
                    lstr->add(f->name);
                    break;
                }
            }
        }
    }
    else {
        // flags termname posx posy lname

        unsigned int flgs = term_flags() & TE_IOFLAGS;
        flgs = (flgs << 8) | pxno_termtype;
        if (!flgs && !pno_name && !cell_terminal())
            return (true);
        lstr->add_c(' ');
        lstr->add_h(flgs, true);

        if (!pno_name && !cell_terminal())
            return (true);
        lstr->add_c(' ');
        lstr->add(term_name()->string());

        if (!cell_terminal())
            return (true);

        // phys position x, y
        lstr->add_c(' ');
        lstr->add_i(cell_terminal()->lx());
        lstr->add_c(' ');
        lstr->add_i(cell_terminal()->ly());

        // terminal layer name
        CDl *ld = cell_terminal()->layer();
        if (ld) {
            lstr->add_c(' ');
            lstr->add(ld->name());
        }
    }
    return (true);
}


// Return a flags vector, pulling the bits from the proper place.
//
unsigned int
CDp_snode::term_flags() const
{
    unsigned int f = pxno_flags;
    if (psno_term && psno_term->is_fixed())
        f |= TE_FIXED;
    else
        f &= ~TE_FIXED;
    if (psno_term && psno_term->is_uninit())
        f |= TE_UNINIT;
    else
        f &= ~TE_UNINIT;
    if (psno_term && psno_term->is_loc_set())
        f |= TE_LOCSET;
    else
        f &= ~TE_LOCSET;
    return (f);
}


// Set a name for the terminal.  If the name is in the form of a
// default name, the assigned name pointer will be zeroed.  The caller
// is responsible for uniqueness and suitable syntax of the name.
//
void
CDp_snode::set_term_name(const char *n)
{
    if (CDnetex::is_default_name(n))
        pno_name = 0;
    else
        pno_name = CDnetex::name_tab_add(n);
    if (psno_term)
        psno_term->set_name(pno_name);
}


// Parse input string for structure node values, and initialize.
// syntax: enode index xloc yloc [tname physx physy flags layer_name typname]
// where enode, index, physx, physy are integers,
// xloc and yloc are of the form int:int,int... for, e.g., x:syx0,syx1..., or
// just an int for syx = x and there is only the one value.
// tname and layer_name are strings, and flags is an int.
//
bool
CDp_snode::parse_snode(CDs *esd, const char *str)
{
    pno_enode = 0;
    pno_name = 0;
    alloc_pos(0);
    pxno_flags = 0;
    pxno_termtype = TE_tINPUT;
    pxno_index = 0;
    psno_term = 0;
    psno_x = 0;
    psno_y = 0;

    const char *errmsg = "Parse error in NODE property string: %s\n  %s.";
    const char *erripf = "token %d integer parse failed";
    char buf[128];

    if (str) {
        const char *s = str;

        // enode, integer
        char *tok = lstring::gettok(&s);
        if (tok) {
            int n = sscanf(tok, "%d", &pno_enode);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 1);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // index, integer
        tok = lstring::gettok(&s);
        if (tok) {
            unsigned int ival;
            int n = sscanf(tok, "%u", &ival);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 2);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
            if (ival >= 1024*1024) {
                Errs()->add_error(errmsg, "bad index value", str);
                return (false);
            }
            set_index(ival);
        }
        else {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }

        // x and y values, these may be integers or comma-separated
        // integer lists

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            return (false);
        }
        int numxvals;
        int *xvals;
        if (!slurp_ints(tok, &numxvals, &xvals)) {
            Errs()->add_error(errmsg, "bad x value string", str);
            delete [] tok;
            return (false);
        }
        delete [] tok;

        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error(errmsg, "too few tokens", str);
            delete [] xvals;
            return (false);
        }
        int numyvals;
        int *yvals;
        if (!slurp_ints(tok, &numyvals, &yvals)) {
            Errs()->add_error(errmsg, "bad y value string", str);
            delete [] tok;
            delete [] xvals;
            return (false);
        }
        delete [] tok;

        if (numxvals > 2 || numyvals > 2) {
            if (numxvals != numyvals) {
                Errs()->add_error(errmsg, "x/y component count mismatch", str);
                delete [] xvals;
                delete [] yvals;
                return (false);
            }
            psno_x = xvals[0];
            psno_y = yvals[0];
            numxvals--;
            alloc_pos(numxvals);
            for (int i = 0; i < numxvals; i++)
                set_pos(i, xvals[i+1], yvals[i+1]);
        }
        else {
            psno_x = xvals[0];
            psno_y = yvals[0];
            pxno_u.pos[0] = numxvals == 1 ? xvals[0] : xvals[1];
            pxno_u.pos[1] = numyvals == 1 ? yvals[0] : yvals[1];
        }
        delete [] xvals;
        delete [] yvals;

        // string token, 3.2/3.3 format selection, flags or name
        tok = lstring::gettok(&s);
        if (!tok)
            return (true);
        bool old32 = true;
        if (lstring::ciprefix("0x", tok)) {
            // must be flags, 3.3 format
            old32 = false;
            unsigned int flgs;
            if (sscanf(tok+2, "%x", &flgs) != 1) {
                Errs()->add_error(errmsg, "bad flags", str);
                delete [] tok;
                return (false);
            }
            delete [] tok;
            set_flag((flgs >> 8) & TE_IOFLAGS);
            for (FlagDef *f = TermTypes; f->name; f++) {
                if (f->value == (flgs & 0xff)) {
                    set_termtype((CDtermType)f->value);
                    break;
                }
            }

            // this will be the name
            tok = lstring::gettok(&s);
        }
        if (tok) {
            set_term_name(tok);
            delete [] tok;
        }
        else
            return (true);

        // phys x, integer
        int phx = 0;
        tok = lstring::gettok(&s);
        if (tok) {
            int n = sscanf(tok, "%d", &phx);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 6);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
        }
        else
            return (true);

        // phys y, integer
        int phy = 0;
        tok = lstring::gettok(&s);
        if (tok) {
            int n = sscanf(tok, "%d", &phy);
            delete [] tok;
            if (n != 1) {
                snprintf(buf, 128, erripf, 7);
                Errs()->add_error(errmsg, buf, str);
                return (false);
            }
        }
        else
            return (true);

        // phx and phy given, indicates that we need a terminal
        CDs *psd = CDcdb()->findCell(esd->cellname(), Physical);
        if (psd)
            psno_term = psd->findPinTerm(pno_name, true);
        if (psno_term) {
            psno_term->set_node_prpty(this);
            psno_term->set_loc(phx, phy);

            if (old32) {
                // flags, integer
                unsigned int flgs = 0;
                tok = lstring::gettok(&s);
                if (tok) {
                    int n = sscanf(tok, "%u", &flgs);
                    delete [] tok;
                    if (n != 1) {
                        snprintf(buf, 128, erripf, 8);
                        Errs()->add_error(errmsg, buf, str);
                        return (false);
                    }
                    set_flag(flgs & TE_IOFLAGS);
                }
                else
                    return (true);
            }
            
            // The TE_FIXED value is actually kept in the CDterm.
            cell_terminal()->set_fixed(pxno_flags & TE_FIXED);

            // layer name, string token
            char *lname = lstring::gettok(&s);
            if (lname) {
                CDl *ld = CDldb()->findLayer(lname);
                delete [] lname;
                cell_terminal()->set_layer(ld);
            }

            if (old32) {
                // term type name, string token
                tok = lstring::gettok(&s);
                if (tok) {
                    for (FlagDef *f = TermTypes; f->name; f++) {
                        if (lstring::cieq(f->name, tok)) {
                            set_termtype((CDtermType)f->value);
                            break;
                        }
                    }
                    delete [] tok;
                }
            }
        }
    }
    return (true);
}


// Create an instance node from the structure node, which is assumed
// linked in sdesc.  Used when instantiating, the x,y field must later
// be transformed.
//
CDp_cnode *
CDp_snode::dupc(CDs *sdesc, const CDc *cdesc)
{
    CDp_cnode *pn = new CDp_cnode;
    pn->set_enode(enode());
    pn->set_index(index());
    pn->set_termtype(termtype());

    // These are the only flag passed to instances.
    if (has_flag(TE_SCINVIS))
        pn->set_flag(TE_SCINVIS);
    if (has_flag(TE_SYINVIS))
        pn->set_flag(TE_SYINVIS);

    if (sdesc && sdesc->owner())
        sdesc = sdesc->owner();
    if (sdesc && sdesc->symbolicRep(cdesc)) {
        unsigned int sz = pts_flag() ? pxno_u.pts[0] : 1;
        pn->alloc_pos(sz);
        for (unsigned int ix = 0; ix < sz; ix++) {
            int x, y;
            if (get_pos(ix, &x, &y))
                pn->set_pos(ix, x, y);
        }
    }
    else {
        pn->alloc_pos(0);
        pn->set_pos(0, psno_x, psno_y);
    }
    return (pn);
}
// End CDp_snode functions.


//
// Name property
//
// Property used by structures and instances to record name.  The name
// of an instance is by default assigned by the application, using the
// name and num fields.  If the setname field is assigned, that name will
// be used instead.  The subname being set indicates a subcircuit,
// rather than a library device.

CDp_name::CDp_name(const CDp_name &pd) : CDp(0, pd.p_value)
{
    pna_num = pd.pna_num;
    pna_label = pd.pna_label;
    pna_name = pd.pna_name;
    pna_setname = lstring::copy(pd.pna_setname);
    pna_labtext = lstring::copy(pd.pna_labtext);
    pna_scindex = pd.pna_scindex;
    pna_subckt = pd.pna_subckt;
    pna_located = pd.pna_located;
    pna_x = pd.pna_x;
    pna_y = pd.pna_y;
}


CDp_name &
CDp_name::operator=(const CDp_name &pd)
{
    (CDp&)*this = (const CDp&)pd;
    pna_num = pd.pna_num;
    pna_label = pd.pna_label;
    pna_name = pd.pna_name;
    pna_setname = lstring::copy(pd.pna_setname);
    pna_labtext = lstring::copy(pd.pna_labtext);
    pna_scindex = pd.pna_scindex;
    pna_subckt = pd.pna_subckt;
    pna_located = pd.pna_located;
    pna_x = pd.pna_x;
    pna_y = pd.pna_y;
    return (*this);
}


// Print the name property text to lstr, using offset xo, yo.
//
bool
CDp_name::print(sLstr *lstr, int, int) const
{
    if (!pna_name)
        lstr->add_c(P_NAME_NULL);
    else
        lstr->add(pna_name->string());
    if (pna_setname) {
        lstr->add_c('.');
        lstr->add(pna_setname);
    }
    lstr->add_c(' ');
    lstr->add_i(number());

    if (is_subckt()) {
        lstr->add_c(' ');
        lstr->add("subckt");
        if (pna_located) {
            lstr->add_c(' ');
            lstr->add_i(pna_x);
            lstr->add_c(' ');
            lstr->add_i(pna_y);
        }
    }
    return (true);
}


// Return the label to be associated with the name property.
//
// The cna_labtext is used to set a default label, before the actual
// label is created/attached.  It DOES NOT track subsequent label text
// changes, nor is it read/written to files.
//
hyList *
CDp_name::label_text(bool *copied, CDc *cdesc) const
{
    if (copied)
        *copied = true;
    if (!pna_name)
        return (new hyList(0, "??", HYcvPlain));
    // Check the name field.  If it is not alpha, treat it
    // specially.
    if (isalpha(*pna_name->string())) {
        // ordinary device name
        if (cdesc) {
            CDp_range *pr = (CDp_range*)cdesc->prpty(P_RANGE);
            const char *instname = cdesc->getBaseName(this);
            if (pr) {
                // This is a vector instance, tack on the range.
                sLstr lstr;
                lstr.add(instname);
                lstr.add_c(cTnameTab::subscr_open());
                lstr.add_i(pr->beg_range());
                lstr.add_c(':');
                lstr.add_i(pr->end_range());
                lstr.add_c(cTnameTab::subscr_close());
                return (new hyList(0, lstr.string(), HYcvPlain));
            }
            return (new hyList(0, instname, HYcvPlain));
        }
        return (new hyList(0, "??", HYcvPlain));
    }
    // else assume a terminal
    if (pna_label) {
        if (copied)
            *copied = false;
        return (pna_label->label());
    }
    // default terminal name
    if (pna_labtext)
        return (new hyList(0, pna_labtext, HYcvPlain));
    if (cdesc && cdesc->master())
        return (new hyList(0, cdesc->cellname()->string(), HYcvPlain));
    return (new hyList(0, "label", HYcvPlain));
}


// Parse the name from the string, and initialize.
// syntax: name[.setname] num [subname physx physy]
// name, setname, subname are strings, num, physx, physy are
// integers.  The physx, physy locate the physical name label.
//
bool
CDp_name::parse_name(const char *str)
{
    char namebf[128];
    char subnbf[128];
    pna_num = 0;
    pna_label = 0;
    pna_name = 0;
    pna_setname = 0;
    pna_labtext = 0;
    pna_scindex = 0;
        // The xcindex is an alternate id number for subcircuits, which
        // will be zero-based for each master.  The pna_num is an absolute
        // count for all subcircuits.
    pna_subckt = false;
        // Set true for subcircuits.
    pna_located = false;
        // Set true when the location is set.
    pna_x = 0;
    pna_y = 0;
        // The location in the physical layout where the label appears,
        // after extraction/association.

    if (str) {
        unsigned int num = 0;
        int nset = sscanf(str, "%s %u %s %d %d", namebf, &num,
            subnbf, &pna_x, &pna_y);
        if (nset < 1) {
            namebf[0] = P_NAME_NULL;
            namebf[1] = 0;
        }
        set_number(num);

        // The name property name string is of the form
        // default_name[.set_name]
        //
        char *cp;
        if ((cp = strchr(namebf, '.')) != 0)
            *cp++ = '\0';

        // The '#' that used to specify "bus" terminals is no longer
        // used, map it to the regular terminal prefix.
        if (*namebf == P_NAME_BTERM_DEPREC)
            *namebf = P_NAME_TERM;

        set_name_string(namebf);
        if (cp && *cp)
            pna_setname = lstring::copy(cp);
        if (nset >= 3)
            // the subname is just a flag at present
            set_subckt(true);
        // Label coords are ignored for now.
    }
    return (true);
}
// End CDp_name functions


// CDp_labloc property
// This property provides default locations for the property labels which
// appear on-screen, for the given device.  If this property does not
// appear, built-in default locations are used.  Each of the four lables
// has 16 locations each in upright and rotated orientation.  These are
// packed into a char[2], with the upright code in the lsb's.  A -1
// indicates default location.

bool
CDp_labloc::print(sLstr *lstr, int, int) const
{
    bool first = true;
    if (ploc_name != -1) {
        lstr->add(KW_NAME);
        lstr->add_c(' ');
        lstr->add_i(ploc_name);
        first = false;
    }
    if (ploc_model != -1) {
        if (!first)
            lstr->add_c(' ');
        lstr->add(KW_MODEL);
        lstr->add_c(' ');
        lstr->add_i(ploc_model);
        first = false;
    }
    if (ploc_value != -1) {
        if (!first)
            lstr->add_c(' ');
        lstr->add(KW_VALUE);
        lstr->add_c(' ');
        lstr->add_i(ploc_value);
        first = false;
    }
    if (ploc_param != -1) {
        if (!first)
            lstr->add_c(' ');
        lstr->add(KW_PARAM);
        lstr->add_c(' ');
        lstr->add_i(ploc_param);
        first = false;
    }
    if (ploc_devref != -1) {
        if (!first)
            lstr->add_c(' ');
        lstr->add(KW_DEVREF);
        lstr->add_c(' ');
        lstr->add_i(ploc_devref);
        first = false;
    }
    return (true);
}


bool
CDp_labloc::parse_labloc(const char *str)
{
    ploc_name = -1;
    ploc_model = -1;
    ploc_value = -1;
    ploc_param = -1;
    ploc_devref = -1;

    if (str) {
        for (;;) {
            char *tok = lstring::gettok(&str);
            if (!tok)
                break;
            int pnum = 0;
            if (lstring::cieq(tok, KW_NAME))
                pnum = P_NAME;
            else if (lstring::cieq(tok, KW_MODEL))
                pnum = P_MODEL;
            else if (lstring::cieq(tok, KW_VALUE))
                pnum = P_VALUE;
            else if (lstring::cieq(tok, KW_PARAM) ||
                    lstring::cieq(tok, KW_INITC))
                pnum = P_PARAM;
            else if (lstring::cieq(tok, KW_DEVREF))
                pnum = P_DEVREF;
            else {
                Errs()->add_error("Bad LABLOC keyword: %s.", tok);
                delete [] tok;
                return (false);
            }
            delete [] tok;
            tok = lstring::gettok(&str);
            if (tok) {
                int n1;
                if (sscanf(tok, "%d", &n1) != 1 || n1 < -1 || n1 > 15) {
                    Errs()->add_error("Bad LABLOC index: %s.", tok);
                    delete [] tok;
                    return (false);
                }
                delete [] tok;
                if (pnum == P_NAME)
                    ploc_name = n1;
                else if (pnum == P_MODEL)
                    ploc_model = n1;
                else if (pnum == P_VALUE)
                    ploc_value = n1;
                else if (pnum == P_PARAM)
                    ploc_param = n1;
                else if (pnum == P_DEVREF)
                    ploc_devref = n1;
            }
        }
    }
    return (true);
}
// End CDp_labloc functions


//
// Old Mutual property
//
// Compatibility property for old mutual inductor format.  Property
// appears in structures.

// Print the property string to lstr, offset by xo, yo.
//
bool
CDp_mut::print(sLstr *lstr, int xo, int yo) const
{
    lstr->add_i(pmu_x1 - xo);
    lstr->add_c(' ');
    lstr->add_i(pmu_y1 - yo);
    lstr->add_c(' ');
    lstr->add_i(pmu_x2 - xo);
    lstr->add_c(' ');
    lstr->add_i(pmu_y2 - yo);
    lstr->add_c(' ');
    lstr->add_d(pmu_coeff, 6, true);
    return (true);
}


// Parse string and initialize.
// syntax: x1 y1 x2 y2 coeff
// where ints x1,y1 and x2, y2 are the lower left corner of the referenced
// inductor instances bounding boxes, and float coeff [-1, 1] is the factor.
//
bool
CDp_mut::parse_mut(const char *str)
{
    pmu_coeff = 0.0;
    pmu_x1 = 0;
    pmu_y1 = 0;
    pmu_x2 = 0;
    pmu_y2 = 0;
    if (str && sscanf(str, "%d %d %d %d %lf", &pmu_x1, &pmu_y1,
            &pmu_x2, &pmu_y2, &pmu_coeff) != 5) {
        Errs()->add_error("Bad MUTUAL property string: %s.", str);
        return (false);
    }
    return (true);
}


// Return the coordinates.
//
void
CDp_mut::get_coords(int *px1, int *py1, int *px2, int *py2) const
{
    *px1 = pmu_x1;
    *py1 = pmu_y1;
    *px2 = pmu_x2;
    *py2 = pmu_y2;
}


// Size of Jspice3 inductor.
#define OLDWID 360
#define OLDHEI 1000

// Static function.
// Return the inductor indicated in the Jspice3 property spec.  We
// are given the lower left corner of the bounding box.
//
CDc *
CDp_mut::find(int x, int y, CDs *sdesc)
{
    CDol *s0 = 0;
    BBox BB;
    BB.left = x;
    BB.bottom = y;
    BB.right = x + OLDHEI;
    BB.top = y + OLDHEI;
    CDg gdesc;
    gdesc.init_gen(sdesc, CellLayer(), &BB);
    CDc *cdesc;
    while ((cdesc = (CDc*)gdesc.next()) != 0) {
        if (!cdesc->is_normal())
            continue;
        const char *name = cdesc->cellname()->string();
        if (!lstring::cieq(name, "ind"))
            continue;
        if (cdesc->oBB().left - x > OLDWID &&
                cdesc->oBB().bottom - y > OLDWID)
            continue;
        s0 = new CDol(cdesc, s0);
    }
    if (!s0)
        return (0);
    if (!s0->next) {
        cdesc = (CDc*)s0->odesc;
        delete s0;
        return (cdesc);
    }
    // Assume that the one closest to the LL corner is the correct
    // one.

    int d = CDinfinity;
    cdesc = 0;
    for (CDol *sl = s0; sl; sl = sl->next) {
        int dd = (sl->odesc->oBB().left - x)*(sl->odesc->oBB().left - x) +
            (sl->odesc->oBB().bottom - y)*(sl->odesc->oBB().bottom - y);
        if (dd < d) {
            d = dd;
            cdesc = (CDc*)sl->odesc;
        }
    }
    CDol::destroy(s0);
    return (cdesc);
}
// End CDp_mut functions


//
// New Mutual property
//
// Property for mutual inductor, appears in structure list.

CDp_nmut::CDp_nmut(int ix, char *nm1, int n1, char *nm2, int n2,
    char *cstr, char *nm) : CDp(P_NEWMUT)
{
    pnmu_indx = ix;
    pnmu_label = 0;
    pnmu_coefstr = cstr;
    pnmu_num1 = n1;
    pnmu_num2 = n2;
    pnmu_odesc1 = 0;
    pnmu_odesc2 = 0;
    pnmu_name1 = nm1;
    pnmu_name2 = nm2;
    pnmu_setname = nm;
}


CDp_nmut::CDp_nmut(const CDp_nmut &pd) : CDp(0, pd.p_value)
{
    pnmu_indx = pd.pnmu_indx;
    pnmu_label = pd.pnmu_label;
    pnmu_coefstr = lstring::copy(pd.pnmu_coefstr);
    pnmu_num1 = pd.pnmu_num1;
    pnmu_num2 = pd.pnmu_num2;
    pnmu_odesc1 = pd.pnmu_odesc1;
    pnmu_odesc2 = pd.pnmu_odesc2;
    pnmu_name1 = lstring::copy(pd.pnmu_name1);
    pnmu_name2 = lstring::copy(pd.pnmu_name2);
    pnmu_setname = lstring::copy(pd.pnmu_setname);
}


CDp_nmut &
CDp_nmut::operator=(const CDp_nmut &pd)
{
    (CDp&)*this = (const CDp&)pd;
    pnmu_indx = pd.pnmu_indx;
    pnmu_label = pd.pnmu_label;
    pnmu_coefstr = lstring::copy(pd.pnmu_coefstr);
    pnmu_num1 = pd.pnmu_num1;
    pnmu_num2 = pd.pnmu_num2;
    pnmu_odesc1 = pd.pnmu_odesc1;
    pnmu_odesc2 = pd.pnmu_odesc2;
    pnmu_name1 = lstring::copy(pd.pnmu_name1);
    pnmu_name2 = lstring::copy(pd.pnmu_name2);
    pnmu_setname = lstring::copy(pd.pnmu_setname);
    return (*this);
}


// Print the property string to lstr, offset by xo, yo.
//
bool
CDp_nmut::print(sLstr *lstr, int, int) const
{
    if (!pnmu_odesc1 || !pnmu_odesc2)
        return (false);
    CDp_name *pn1 = (CDp_name*)pnmu_odesc1->prpty(P_NAME);
    if (!pn1 || !pn1->name_string())
        return (false);
    CDp_name *pn2 = (CDp_name*)pnmu_odesc2->prpty(P_NAME);
    if (!pn2 || !pn2->name_string())
        return (false);

    lstr->add_i(pnmu_indx);
    lstr->add_c(' ');
    lstr->add(pn1->name_string()->string());
    lstr->add_c(' ');
    lstr->add_i(pnmu_num1);
    lstr->add_c(' ');
    lstr->add(pn2->name_string()->string());
    lstr->add_c(' ');
    lstr->add_i(pnmu_num2);
    lstr->add_c(' ');
    lstr->add(pnmu_coefstr);

    if (pnmu_setname) {
        lstr->add_c(' ');
        lstr->add(pnmu_setname);
    }
    return (true);
}


// Create the label text to be used for the associated label.
//
hyList *
CDp_nmut::label_text(bool *copied, CDc*) const
{
    char buf[128];
    if (pnmu_setname) {
        char *s = lstring::stpcpy(buf, pnmu_setname);
        *s++ = '=';
        strcpy(s, pnmu_coefstr);
    }
    else {
        char *s = lstring::stpcpy(buf, MUT_CODE);
        s = mmItoA(s, pnmu_indx);
        *s++ = '=';
        strcpy(s, pnmu_coefstr);
    }
    *copied = true;
    return (new hyList(0, buf, HYcvPlain));
}


// Parse the string and initialize.
// syntax: indx name1 num1 name2 num2 coeff [name]
// where int indx is the reference number, name1, name2 are the default
// inductor names (strings), num1 and num2 (ints) are the corresponding
// indices, from the name properties of the referenced devices.  Coeff is
// a string for the coefficient, and name is an overriding name for the
// mutual inductor.  If name is not given, it will be generated internally.
//
bool
CDp_nmut::parse_nmut(const char *str)
{
    char name1bf[128];
    char name2bf[128];
    char cbuf[128];
    char namebf[128];
    pnmu_indx = 0;
    pnmu_label = 0;
    pnmu_coefstr = 0;
    pnmu_num1 = 0;
    pnmu_num2 = 0;
    pnmu_odesc1 = 0;
    pnmu_odesc2 = 0;
    pnmu_name1 = 0;
    pnmu_name2 = 0;
    pnmu_setname = 0;

    if (str) {
        int nset;
        if ((nset = sscanf(str, "%d %s %d %s %d %s %s", &pnmu_indx, name1bf,
                &pnmu_num1, name2bf, &pnmu_num2, cbuf, namebf)) < 6) {
            Errs()->add_error("Bad NEWMUT property string: %s.", str);
            return (false);
        }
        if (nset >= 2)
            pnmu_name1 = lstring::copy(name1bf);
        if (nset >= 4)
            pnmu_name2 = lstring::copy(name2bf);
        if (nset >= 6)
            pnmu_coefstr = lstring::copy(cbuf);
        if (nset == 7)
            pnmu_setname = lstring::copy(namebf);
    }
    return (true);
}


// Rename the mutual inductor.
//
bool
CDp_nmut::rename(CDs *sdesc, const char *name)
{
    if (!name || !*name) {
        delete [] pnmu_setname;
        pnmu_setname = 0;
        return (true);
    }
    char buf[128];
    if (sdesc) {
        // should always be true
        for (CDp_nmut *pm = (CDp_nmut*)sdesc->prpty(P_NEWMUT); pm;
                pm = pm->next()) {
            if (pm != this) {
                if (pm->pnmu_setname)
                    strcpy(buf, pm->pnmu_setname);
                else {
                    buf[0] = 'K';
                    mmItoA(buf + 1, pm->pnmu_indx);
                }
                if (!strcmp(buf, name)) {
                    delete [] pnmu_setname;
                    pnmu_setname = 0;
                    Errs()->add_error(
                        "Name %s conflict, default used.", name);
                    return (false);
                }
            }
        }
    }
    if (pnmu_setname) {
        if (strcmp(pnmu_setname, name)) {
            delete [] pnmu_setname;
            pnmu_setname = lstring::copy(name);
        }
    }
    else {
        char *s = lstring::stpcpy(buf, MUT_CODE);
        mmItoA(s, pnmu_indx);
        if (strcmp(buf, name))
            pnmu_setname = lstring::copy(name);
    }
    return (true);
}


// Update the pointer reference.
//
void
CDp_nmut::updat_ref(const CDc *old, CDc *repl)
{
    if (pnmu_odesc1 == old)
        pnmu_odesc1 = repl;
    else if (pnmu_odesc2 == old)
        pnmu_odesc2 = repl;
}


// Return the pointer references.
//
bool
CDp_nmut::get_descs(CDc **od1, CDc **od2) const
{
    if (pnmu_odesc1 && pnmu_odesc2 &&
            pnmu_odesc1->prpty_list() && pnmu_odesc2->prpty_list()) {
        *od1 = pnmu_odesc1;
        *od2 = pnmu_odesc2;
        return (true);
    }
    return (false);
}


// Return true and set other of oref is used as a pointer reference.
//
bool
CDp_nmut::match(CDc *oref, CDc **other) const
{
    if (pnmu_odesc1 == oref) {
        if (other)
            *other = pnmu_odesc2;
        return (true);
    }
    if (pnmu_odesc2 == oref) {
        if (other)
            *other = pnmu_odesc1;
        return (true);
    }
    return (false);
}
// End CDp_nmut functions


//
// Branch property
//
// Property to maintain location where info can be obtained by clicking.
// A reference direction is also maintained for currents.  This property
// appears in instances.

CDp_branch::CDp_branch(const CDp_branch &pd) : CDp(0, pd.p_value)
{
    pb_x = pd.pb_x;
    pb_y = pd.pb_y;
    pb_r1 = pd.pb_r1;
    pb_r2 = pd.pb_r2;
    pb_bstring = lstring::copy(pd.pb_bstring);
}


CDp_branch &
CDp_branch::operator=(const CDp_branch &pd)
{
    (CDp&)*this = (const CDp&)pd;
    pb_x = pd.pb_x;
    pb_y = pd.pb_y;
    pb_r1 = pd.pb_r1;
    pb_r2 = pd.pb_r2;
    pb_bstring = lstring::copy(pd.pb_bstring);
    return (*this);
}


// Print the property text to lstr, using offset xo, yo.
//
bool
CDp_branch::print(sLstr *lstr, int xo, int yo) const
{
    lstr->add_i(pb_x - xo);
    lstr->add_c(' ');
    lstr->add_i(pb_y - yo);
    lstr->add_c(' ');
    lstr->add_i(pb_r1);
    lstr->add_c(' ');
    lstr->add_i(pb_r2);
    if (pb_bstring) {
        lstr->add_c(' ');
        lstr->add(pb_bstring);
    }
    return (true);
}


// Parse the string and initialize.
// syntax: x y r1 r2 [string]
// where x,y are (int) coordinates, r1, r2 are (int) direction vectors,
// and string is a directive.
//
bool
CDp_branch::parse_branch(const char *str)
{
    char buf[128];
    pb_r1 = 0;
    pb_r2 = 0;
    pb_x = 0;
    pb_y = 0;
    pb_bstring = 0;

    if (str) {
        int nset, rx, ry;
        if ((nset = sscanf(str, "%d %d %d %d %s", &pb_x, &pb_y, &rx, &ry,
                buf)) < 4) {
            Errs()->add_error("Bad BRANCH property string: %s.", str);
            return (false);
        }
        pb_r1 = rx;
        pb_r2 = ry;
        if (nset == 5)
            pb_bstring = lstring::copy(buf);
    }
    return (true);
}
// End CDp_branch functions


//
// Label Reference property
//
// Property applied to labels to indicate that the label is associated with
// some other instance or structure property.

CDp_lref::CDp_lref(const CDp_lref &pd) : CDp(0, pd.p_value)
{
    plr_num = pd.plr_num;
    plr_name = pd.plr_name;
    plr_prpnum = pd.plr_prpnum;
    plr_prpref = 0;
    plr.devref = 0;
}


CDp_lref &
CDp_lref::operator=(const CDp_lref &pd)
{
    (CDp&)*this = (const CDp&)pd;
    plr_num = pd.plr_num;
    plr_name = pd.plr_name;
    plr_prpnum = pd.plr_prpnum;
    plr_prpref = 0;
    plr.devref = 0;
    return (*this);
}


// Print the property text to lstr, offset by xo, yo.
//
bool
CDp_lref::print(sLstr *lstr, int, int) const
{
    if (!plr_name) {
        // Wire property.
        if (plr_prpref || (plr.wireref && plr.wireref->type() != CDWIRE))
            return (false);
        lstr->add_i(pos_x());
        lstr->add_c(' ');
        lstr->add_i(pos_y());
        lstr->add_c(' ');
        lstr->add_i(propnum());
    }
    else {
        // Instance property.
        if (plr.devref && plr.devref->type() == CDWIRE)
            return (false);
        lstr->add(name()->string());
        lstr->add_c(' ');
        lstr->add_i(number());
        lstr->add_c(' ');
        lstr->add_i(propnum());
    }
    return (true);
}


// Parse the string and initialize.
// syntax for instance property labels: name num property
// where name (string), num (int) come from the instance name property of
// the instance of the target property, property (int) is the pValue of
// the target property.
//
// syntax for wire node labels: x y P_NODE
// The x,y represent a vertex of the indicated wire.
//
bool
CDp_lref::parse_lref(const char *str)
{
    char namebf[128];
    plr_num = 0;
    plr_num2 = 0;
    plr_prpnum = 0;
    plr_name = 0;
    plr_prpref = 0;
    plr.devref = 0;

    if (str) {
        int nset;
        if ((nset = sscanf(str, "%s %d %d", namebf, &plr_num,
                &plr_prpnum)) != 3) {
            Errs()->add_error("Bad LABREF property string: %s.", str);
            return (false);
        }
        int x;
        if (sscanf(namebf, "%d", &x) == 1 &&
                (plr_prpnum == P_NODE || plr_prpnum == P_BNODE)) {
            // Wire node/bnode label.
            plr_num2 = plr_num;
            plr_num = x;
        }
        else
            set_name(namebf);
    }
    return (true);
}


// Return the object (cell instance or wire) which is referenced.
//
CDo *
CDp_lref::find_my_object(CDs *sd, CDla *la)
{
    if (!sd)
        return (0);

    if (!name()) {
        // Wire node label.

        int x = pos_x();
        int y = pos_y();
        BBox aBB(x, y, x, y);
        CDsLgen gen(sd);
        CDl *ld;
        while ((ld = gen.next()) != 0) {
            if (!ld->isWireActive())
                continue;
            CDg gdesc;
            gdesc.init_gen(sd, ld, &aBB);
            CDo *od;
            while ((od = gdesc.next()) != 0) {
                if (!od->is_normal())
                    continue;
                if (od->type() != CDWIRE)
                    continue;
                CDw *wd = (CDw*)od;

#ifdef OLDBIND
                if (x == wd->points()[0].x && y == wd->points()[0].y) {
#else
                // The location is not on the vertex, which could be
                // ambiguous.  Instead, it is placed 100 units along
                // the wire from the endpoint.  The location is off
                // grid so unlikely to be occupied by another wire
                // (but not impossible).  We support non-Manhattan,
                // but it is not a good idea to use these.

                int x1 = wd->points()[0].x; 
                int y1 = wd->points()[0].y; 
                int x2 = wd->points()[1].x; 
                int y2 = wd->points()[1].y; 
                bool match;
                if (x1 == x2) {
                    if (y1 == y2)
                        continue;
                    if (y1 < y2)
                        y1 += 100;
                    else
                        y1 -= 100;
                    match = (x == x1 && y == y1);
                }
                else if (y1 == y2) {
                    if (x1 < x2)
                        x1 += 100;
                    else
                        x1 -= 100;
                    match = (x == x1 && y == y1);
                }
                else {
                    double dx = x2 - x1;
                    double dy = y2 - y1; 
                    double d = sqrt(dx*dx + dy*dy);
                    x1 = mmRnd(dx*100/d);
                    y1 = mmRnd(dy*100/d);
                    match = (abs(x - x1) <= 1 && abs(y - y1) <= 1);
                }
                if (match) {
#endif
                    CDp_node *pn = (CDp_node*)wd->prpty(propnum());
                    if (pn) {
                        if (la) {
                            pn->bind(la);
                            la->link(sd, wd, 0);
                        }
                        return (od);
                    }
                }
            }
        }
        return (0);
    }

    unsigned int num = number();
    int prp_num = propnum();

    if (prp_num == P_NEWMUT)
        return (0);  // Mutual inductor reference, no object.

    // This is set for terminals created to tie split nets when reading
    // OpenAccess.
    CDc *cdesc = devref();
    CDlabelCache *cache = CD()->GetLabelCache(sd);
    if (!cdesc && cache)
        cdesc = CD()->LabelCacheFind(cache, name(), num);
    if (cdesc) {
        CDp *pd = cdesc->prpty(prp_num);
        if (!pd)
            return (0);
        if (la) {
            if (!pd->cond_bind(la))
                return (0);  // Something different already bound!
            la->link(sd, cdesc, pd);
        }
        return (cdesc);
    }

    if (!cache) {
        // Look for device with name property that of reference,
        // comparison is to the "real" name, not the set name.

        CDpfxName lname = name();
        CDm_gen mgen(sd, GEN_MASTERS);
        for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
            CDc_gen cgen(mdesc);
            for (cdesc = cgen.c_first(); cdesc; cdesc = cgen.c_next()) {
                CDp_name *pn = (CDp_name*)cdesc->prpty(P_NAME);
                if (!pn || !pn->name_string())
                    continue;
                if (lname != pn->name_string())
                    break;
                if (num == pn->number()) {
                    CDp *pd = cdesc->prpty(prp_num);
                    if (!pd)
                        return (0);
                    if (la) {
                        if (!pd->cond_bind(la))
                            return (0);  // Something already bound!
                        la->link(sd, cdesc, pd);
                    }
                    return (cdesc);
                }
            }
        }
    }
    return (0);
}

// End CDp_lref functions


//
// Mutual Reference property
//
// Property added to inductor instances to indicate being referenced in
// a (new style) mutual inductor.

bool
CDp_mutlrf::print(sLstr *lstr, int, int) const
{
    lstr->add("mutual");
    return (true);
}
// End CDp_mutlrf functions


//
// The Symbolic property.
//
// Property applied to a structure which provides info for a symbolic
// representation.  The representation is kept in the (CDs*) rep
// pointer.  If the active flag is false, the symbolic info is not used.
//
// As of 3.2.23, Symbolic properties can appear in subcircuit
// instances as well.  In this case, the rep field is null, the name
// is ignored.  If active is false, the instance will be treated
// non-symbolically.  Otherwise, it will be trated as per the master.

CDp_sym::CDp_sym(const CDp_sym &ps) : CDp(ps.p_string, ps.p_value)
{
    ps_active = ps.ps_active;
    ps_rep = (ps.ps_rep ? ps.ps_rep->cloneCell() : 0);
}


CDp_sym &
CDp_sym::operator=(const CDp_sym &pd)
{
    (CDp&)*this = (const CDp&)pd;
    ps_active = pd.ps_active;
    ps_rep = (pd.ps_rep ? pd.ps_rep->cloneCell() : 0);
    return (*this);
}


// Print the fields of the 'rep' entry in CIF format, but with ":\n"
// as terminator.  Here, the colon character MUST be followed by a
// newline character to be taken as a terminator.
//
bool
CDp_sym::print(sLstr *lstr, int xo, int yo) const
{
    const char *termination = ":\n";
    lstr->add_c(ps_active ? '1' : '0');
    if (!ps_rep)
        return (true);
    lstr->add_c('\n');
    CDsLgen gen(ps_rep);
    CDl *ldesc;
    while ((ldesc = gen.next()) != 0) {
        CDg gdesc;
        gdesc.init_gen(ps_rep, ldesc);
        bool ldone = false;
        CDo *odesc;
        while ((odesc = gdesc.next()) != 0) {
            if (!ldone) {
                lstr->add("  L ");
                lstr->add(ldesc->name());
                lstr->add(termination);
                ldone = true;
            }
            char *s = odesc->cif_string(xo, yo);
            lstr->add("  ");

            // Fix any spurious line termination by adding a space
            // char after the colon.

            for (char *t = s; *t; t++) {
                lstr->add_c(*t);
                if (*t == ':' && (*(t+1) == '\n' || *(t+1) == '\r'))
                    lstr->add_c(' ');
            }
            lstr->add(termination);
            delete [] s;
        }
    }
    return (true);
}


// Parse the string, build the 'rep' structure.
//
bool
CDp_sym::parse_sym(CDs *sdesc, const char* str)
{
    if (!sdesc)
        return (true);
    char tbuf[64];
    ps_rep = new CDs(0, Electrical);
    ps_rep->setSymbolic(true);
    ps_rep->setOwner(sdesc);

    if (!str || !*str)
        return (true);
    const char *s = str;
    while (isspace(*s))
        s++;
    if (*s == '0')
        ps_active = false;
    else
        ps_active = true;
    if (*s)
        s++;
    if (*s)
        s++;
    CDl *ldesc = 0;
    while (*s) {
        const char *t = s;
        while (*t) {
            if (*t == ':' && (*(t+1) == '\n' || *(t+1) == '\r'))
                break;
            t++;
        }

        int lsz = t - s;
        char *lbuf = new char[lsz +  1];
        memcpy(lbuf, s, lsz);
        lbuf[lsz] = 0;

        const char *c = lbuf;
        while (isspace(*c))
            c++;
        switch (*c) {
        case 'L':  // Layer
        case 'l':
            c++;
            while (isspace(*c))
                c++;
            ldesc = CDldb()->findLayer(c, Electrical);
            break;
        case 'B': // Box
        case 'b':
            if (ldesc) {
                c++;
                while (isspace(*c))
                    c++;
                int w, h, x, y;
                if (sscanf(c, "%d %d %d %d", &w, &h, &x, &y) == 4) {
                    BBox BB;
                    BB.set_cif(x, y, w, h);
                    ps_rep->makeBox(ldesc, &BB, 0);
                }
            }
            break;
        case 'P': // Polygon
        case 'p':
            if (ldesc) {
                c++;
                while (isspace(*c))
                    c++;
                Poly poly;
                getpts(c, &poly.points, &poly.numpts);
                ps_rep->makePolygon(ldesc, &poly, 0);
            }
            break;
        case 'W': // Wire
        case 'w':
            if (ldesc) {
                c++;
                while (isspace(*c))
                    c++;
                char *ss = tbuf;
                while (*c && !isspace(*c))
                    *ss++ = *c++;
                *ss = '\0';
                Wire wire;
                wire.set_wire_width(atoi(tbuf));
                getpts(c, &wire.points, &wire.numpts);
                ps_rep->makeWire(ldesc, &wire, 0);
            }
            break;
        case '9': // Label
            if (ldesc) {
                c++;
                if (*c != '4')
                    break;
                c++;
                sLstr lstr;
                CD()->GetLabel(&c, &lstr);
                Label label;
                int nargs = sscanf(c, "%d%d%d%d%d", &label.x, &label.y,
                    &label.xform, &label.width, &label.height);
                // xform, width, height optional
                if (nargs < 2)
                    break;
                if (nargs <= 3) {
                    // default width, height
                    double tw, th;
                    CD()->DefaultLabelSize(lstr.string(), Electrical,
                        &tw, &th);
                    label.width = ELEC_INTERNAL_UNITS(tw);
                    label.height = ELEC_INTERNAL_UNITS(th);
                }
                else if (nargs == 4) {
                    // given width but not height
                    double tw, th;
                    CD()->DefaultLabelSize(lstr.string(), Electrical,
                        &tw, &th);
                    label.height = mmRnd(label.width*th/tw);
                }
                label.label = new hyList(ps_rep, lstr.string(), HYcvAscii);
                ps_rep->makeLabel(ldesc, &label, 0);
            }
        }
        delete [] lbuf;
        if (*t == ':')
            t++;
        while (isspace(*t))
            t++;
        s = t;
    }
    return (true);
}
// End CDp_sym functions

//
// The Nodemap property.
//
// Property applied to a structure to store a list of mapped node names
// along with hypertext coordinates.

CDp_nodmp::CDp_nodmp(const CDp_nodmp &pn) : CDp(pn.p_string, pn.p_value)
{
    pnm_size = pn.pnm_size;
    if (pnm_size && pn.pnm_list) {
        pnm_list = new CDnmapRef[pnm_size];
        for (int i = 0; i < pnm_size; i++)
            pnm_list[i] = pn.pnm_list[i];
    }
    else
        pnm_list = 0;
}


CDp_nodmp &
CDp_nodmp::operator=(const CDp_nodmp &pd)
{
    (CDp&)*this = (const CDp&)pd;
    pnm_size = pd.pnm_size;
    if (pnm_size && pd.pnm_list) {
        pnm_list = new CDnmapRef[pnm_size];
        for (int i = 0; i < pnm_size; i++)
            pnm_list[i] = pd.pnm_list[i];
    }
    else
        pnm_list = 0;
    return (*this);
}


// Print the fields of the 'list' entry in CIF format.
//
bool
CDp_nodmp::print(sLstr *lstr, int xo, int yo) const
{
    // The "active" flag is no longer used, mapping is always active.
    lstr->add("1");

    int len = 1;
    char buf[256];
    for (int i = 0; i < pnm_size; i++) {
        char *s = lstring::stpcpy(buf, pnm_list[i].name->string());
        *s++ = ' ';
        s = mmItoA(s, xo + pnm_list[i].x);
        *s++ = ' ';
        s = mmItoA(s, yo + pnm_list[i].y);

        int slen = strlen(buf) + 1;
        if (len + slen < 80) {
            lstr->add_c(' ');
            len += slen;
        }
        else {
            lstr->add_c('\n');
            len = slen;
        }
        lstr->add(buf);
    }
    return (true);
}


// Parse the string, build the 'list' array.  The string is in the form
// "1/0 name x y name x y ..."
//
bool
CDp_nodmp::parse_nodmp(const char* str)
{
    pnm_size = 0;
    pnm_list = 0;
    if (!str)
        return (true);
    while (isspace(*str))
        str++;
    // Advance past unused "active" flag.
    lstring::advtok(&str);

    // the rest of the string are name/coordinate fields
    int cnt = 0;
    const char *s = str;
    while (*s) {
        lstring::advtok(&s);
        cnt++;
    }
    cnt /= 3;
    if (!cnt)
        return (true);
    pnm_list = new CDnmapRef[cnt];

    cnt = 0;
    s = str;
    while (*s) {
        char *tok = lstring::gettok(&s);
        if (!tok)
            break;
        char *xtok = lstring::gettok(&s);
        if (!xtok) {
            delete [] tok;
            break;
        }
        char *ytok = lstring::gettok(&s);
        if (!ytok) {
            delete [] tok;
            delete [] xtok;
            break;
        }
        int x, y;
        if (sscanf(xtok, "%d", &x) == 1 && sscanf(ytok, "%d", &y) == 1) {
            pnm_list[cnt].name = CDnetex::name_tab_add(tok);
            pnm_list[cnt].x = x;
            pnm_list[cnt].y = y;
            cnt++;
        }
        delete [] tok;
        delete [] xtok;
        delete [] ytok;
    }
    if (!cnt) {
        delete pnm_list;
        pnm_list = 0;
    }
    pnm_size = cnt;
    return (true);
}
// End CDp_nodmp functions


namespace {
    // Comparison function for properties.
    //
    inline bool
    p_comp(const CDp *p1, const CDp *p2)
    {
        return (p1->value() < p2->value());
    }
}


// Static function.
// Sort a list of properties by increasing value.
//
void
CDpl::sort(CDpl *thisp)
{
    int cnt = 0;
    for (CDpl *p = thisp; p; p = p->next, cnt++) ;
    if (cnt < 2)
        return;
    CDp **aa = new CDp*[cnt];
    cnt = 0;
    for (CDpl *p = thisp; p; p = p->next, cnt++)
        aa[cnt] = p->pdesc;
    std::sort(aa, aa + cnt, p_comp);
    cnt = 0;
    for (CDpl *p = thisp; p; p = p->next, cnt++)
        p->pdesc = aa[cnt];
    delete [] aa;
}
// End CDpl functions

//
// Parser/composer for the XICP_CHD_REF property string.
// Applies for physical mode only.
//

// Construct an sChdPrp from the arguments,
//
sChdPrp::sChdPrp(const cCHD *chd, const char *path, const char *cname,
    const BBox *BB)
{
    cp_path = lstring::copy(path);
    cp_cellname = lstring::copy(cname);
    if (BB)
        cp_BB = *BB;
    else
        cp_BB = CDnullBB;
    cp_scale = 1.0;
    cp_alias_prefix = 0;
    cp_alias_suffix = 0;
    cp_alias_flags = 0;
    if (chd) {
        const cv_alias_info *info = chd->aliasInfo();
        if (info) {
            cp_alias_flags = info->flags();
            cp_alias_prefix = lstring::copy(info->prefix());
            cp_alias_suffix = lstring::copy(info->suffix());
        }
    }
}


// Construct an sChdPrp by parsing the property string.
//
sChdPrp::sChdPrp(const char *str)
{
    cp_path = 0;
    cp_cellname = 0;
    cp_alias_prefix = 0;
    cp_alias_suffix = 0;
    cp_BB = CDnullBB;
    cp_scale = 1.0;
    cp_alias_flags = 0;

    const char *sepchars = "=";
    char *tok;
    while ((tok = lstring::gettok(&str, sepchars)) != 0) {
        char *tok1 = lstring::getqtok(&str);
        if (!strcmp(tok, CHDKW_FILENAME)) {
            if (!cp_path)
                cp_path = tok1;
            else
                delete [] tok1;
        }
        else if (!strcmp(tok, CHDKW_CELLNAME)) {
            if (!cp_cellname)
                cp_cellname = tok1;
            else
                delete [] tok1;
        }
        else if (!strcmp(tok, CHDKW_BOUND)) {
            double l, b, r, t;
            if (sscanf(tok1, "%lf,%lf,%lf,%lf", &l, &b, &r, &t) == 4) {
                cp_BB.left = INTERNAL_UNITS(l);
                cp_BB.bottom = INTERNAL_UNITS(b);
                cp_BB.right = INTERNAL_UNITS(r);
                cp_BB.top = INTERNAL_UNITS(t);
            }
            delete [] tok1;
        }
        else if (!strcmp(tok, CHDKW_SCALE)) {
            double sc;
            if (sscanf(tok1, "%lf", &sc) == 1)
                cp_scale = sc;
            delete [] tok1;
        }
        else if (!strcmp(tok, CHDKW_AFLAGS)) {
            cp_alias_flags = atoi(tok1);
            delete [] tok1;
        }
        else if (!strcmp(tok, CHDKW_APREFIX)) {
            if (!cp_alias_prefix)
                cp_alias_prefix = tok1;
            else
                delete [] tok1;
        }
        else if (!strcmp(tok, CHDKW_ASUFFIX)) {
            if (!cp_alias_suffix)
                cp_alias_suffix = tok1;
            else
                delete [] tok1;
        }
        else
            delete [] tok1;
        delete tok;
    }
}


unsigned int
sChdPrp::hash(unsigned int mask)
{
    unsigned int h = string_hash(cp_path, mask);
    h ^= string_hash(cp_alias_prefix, mask);
    h ^= string_hash(cp_alias_suffix, mask);
    h ^= number_hash((int)(cp_scale*CDphysResolution), mask);
    h ^= number_hash(cp_alias_flags, mask);
    return (h);
}


namespace {
    bool s_eq(const char *s1, const char *s2)
    {
        if (!s1)
            return (s2 == 0);
        if (!s2)
            return (false);
        return (!strcmp(s1, s2));
    }
}


bool
sChdPrp::operator==(const sChdPrp &prp) const
{
    if (!s_eq(cp_path, prp.cp_path))
        return (false);
    if (!s_eq(cp_alias_prefix, prp.cp_alias_prefix))
        return (false);
    if (!s_eq(cp_alias_suffix, prp.cp_alias_suffix))
        return (false);
    if (fabs(cp_scale - prp.cp_scale) > 1e-12*cp_scale)
        return (false);
    if (cp_alias_flags != prp.cp_alias_flags)
        return (false);
    return (true);
}


sChdPrp::~sChdPrp()
{
    delete [] cp_path;
    delete [] cp_cellname;
    delete [] cp_alias_prefix;
    delete [] cp_alias_suffix;
}


// Return a string suitable as a XICP_CHD_REF property.
//
char *
sChdPrp::compose()
{
    if (!cp_path || !cp_cellname || cp_BB == CDnullBB)
        return (0);

    sLstr lstr;
    lstr.add(CHDKW_FILENAME);
    lstr.add_c('=');
    lstr.add_c('"');
    lstr.add(cp_path);
    lstr.add_c('"');
    lstr.add_c(' ');
    lstr.add(CHDKW_CELLNAME);
    lstr.add_c('=');
    lstr.add(cp_cellname);
    lstr.add_c(' ');

    lstr.add(CHDKW_BOUND);
    lstr.add_c('=');
    lstr.add_d(MICRONS(cp_BB.left), 3);
    lstr.add_c(',');
    lstr.add_d(MICRONS(cp_BB.bottom), 3);
    lstr.add_c(',');
    lstr.add_d(MICRONS(cp_BB.right), 3);
    lstr.add_c(',');
    lstr.add_d(MICRONS(cp_BB.top), 3);

    if (cp_scale != 1.0) {
        lstr.add_c(' ');
        lstr.add(CHDKW_SCALE);
        lstr.add_c('=');
        lstr.add_d(cp_scale, 9, true);
    }
    if (cp_alias_flags) {
        lstr.add_c(' ');
        lstr.add(CHDKW_AFLAGS);
        lstr.add_c('=');
        lstr.add_u(cp_alias_flags);
    }
    if (cp_alias_prefix) {
        lstr.add_c(' ');
        lstr.add(CHDKW_APREFIX);
        lstr.add_c('=');
        lstr.add(cp_alias_prefix);
    }
    if (cp_alias_suffix) {
        lstr.add_c(' ');
        lstr.add(CHDKW_ASUFFIX);
        lstr.add_c('=');
        lstr.add(cp_alias_suffix);
    }
    return (lstr.string_trim());
}


void
sChdPrp::scale_bb(double scale)
{
    if (scale != 1.0) {
        cp_scale *= scale;
        if (cp_BB != CDnullBB) {
            cp_BB.left = mmRnd(cp_BB.left * scale);
            cp_BB.bottom = mmRnd(cp_BB.bottom * scale);
            cp_BB.right = mmRnd(cp_BB.right * scale);
            cp_BB.top = mmRnd(cp_BB.top * scale);
        }
    }
}

