
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

#ifndef TECH_DRF_IN_H
#define TECH_DRF_IN_H


//-----------------------------------------------------------------------------
// cTechDrfIn  Cadence DRF reader

// Color element.
//
struct sDrfColor
{
    sDrfColor(const char *nm, int r, int g, int b, bool blnk)
        {
            cl_name = lstring::copy(nm);
            cl_r = r;
            cl_g = g;
            cl_b = b;
            cl_blink = blnk;
        }

    ~sDrfColor()
        {
            delete [] cl_name;
        }

    const char *name()              const { return (cl_name); }
    int red()                       const { return (cl_r); }
    int green()                     const { return (cl_g); }
    int blue()                      const { return (cl_b); }
    bool blink()                    const { return (cl_blink); }

private:
    char *cl_name;
    unsigned char cl_r, cl_g, cl_b;
    bool cl_blink;
};

// Stipple mask element.
//
struct sDrfStipple
{
    sDrfStipple(const char *nm, unsigned int x, unsigned int y,
            unsigned char *m)
        {
            st_name = lstring::copy(nm);
            st_nx = x;
            st_ny = y;
            st_map = m;
        }

    ~sDrfStipple()
        {
            delete [] st_name;
            delete [] st_map;
        }

    const char *name()              const { return (st_name); }
    unsigned int nx()               const { return (st_nx); }
    unsigned int ny()               const { return (st_ny); }
    const unsigned char *map()      const { return (st_map); }

private:
    char *st_name;
    unsigned short st_nx, st_ny;
    unsigned char *st_map;
};

// Line style element.
//
struct sDrfLine
{
    sDrfLine(const char *nm, int sz, int len, unsigned int msk)
        {
            l_name = lstring::copy(nm);
            l_size = sz;
            l_len = len;
            l_mask = msk;
        }

    ~sDrfLine()
        {
            delete [] l_name;
        }

    const char *name()      const { return (l_name); }
    int size()              const { return (l_size); }
    int length()            const { return (l_len); }
    unsigned int mask()     const { return (l_mask); }

private:
    char *l_name;
    int l_size;
    int l_len;
    unsigned int l_mask;
};

// Packet element.
//
struct sDrfPacket
{
    sDrfPacket(const char *nm, const char *sn, const char *ln,
            const char *fc, const char *oc)
        {
            pk_name = lstring::copy(nm);
            pk_stipple_name = lstring::copy(sn);
            pk_linestyle_name = lstring::copy(ln);
            pk_fill_color = lstring::copy(fc);
            pk_outline_color = lstring::copy(oc);
        }

    ~sDrfPacket()
        {
            delete [] pk_name;
            delete [] pk_stipple_name;
            delete [] pk_linestyle_name;
            delete [] pk_fill_color;
            delete [] pk_outline_color;
        }

    const char *name()              const { return (pk_name); }
    const char *stipple_name()      const { return (pk_stipple_name); }
    const char *linestyle_name()    const { return (pk_linestyle_name); }
    const char *fill_color()        const { return (pk_fill_color); }
    const char *outline_color()     const { return (pk_outline_color); }

private:
    char *pk_name;
    char *pk_stipple_name;
    char *pk_linestyle_name;
    char *pk_fill_color;
    char *pk_outline_color;
};

// List of layers with packet names that need to be updated when the
// packet is defined.
//
struct sDrfDeferred
{
    sDrfDeferred(CDl *ld, const char *pkt, sDrfDeferred *n)
        {
            ldesc = ld;
            packet = lstring::copy(pkt);
            next = n;
        }

    ~sDrfDeferred()
        {
            delete [] packet;
        }

    sDrfDeferred *next;
    CDl *ldesc;
    char *packet;
};

inline class cTechDrfIn *DrfIn();

class cTechDrfIn : public cLispEnv
{
public:
    friend inline cTechDrfIn *DrfIn()
        {
            if (!cTechDrfIn::instancePtr)
                new cTechDrfIn;
            return (cTechDrfIn::instancePtr);
        }

    cTechDrfIn();
    ~cTechDrfIn();

    bool read(const char*, char**);
    void apply_packet(CDl*, const char*);
    void report_unresolved(sLstr&);

    const sDrfColor *find_color(const char*);
    const sDrfColor *find_color(int, int, int);
    const sDrfStipple *find_stipple(const char*);
    const sDrfStipple *find_stipple(const GRfillType*);
    const sDrfLine *find_line(const char*);
    const sDrfPacket *find_packet(const char*, const char*, const char*,
        const char*);
    const sDrfPacket *find_packet(const char*);

    void add_color(sDrfColor*);
    void add_stipple(sDrfStipple*);
    void add_line(sDrfLine*);
    void add_packet(sDrfPacket*);

    const char *display() const     { return (c_display); }
    void set_display(const char *d) { c_display = lstring::copy(d); }

    static char *mk_color_name(int, int, int);
    static char *mk_stp_str(unsigned int, unsigned int, const unsigned char*);
    static char *mk_packet_name(const char*, const char*, const char*,
        const char*);

    static void save(CDl *ld, const char *pktname)
        {
            deferred = new sDrfDeferred(ld, pktname, deferred);
        }

private:
    static bool drDefineDisplay(lispnode*, lispnode*, char**);
    static bool drDefineColor(lispnode*, lispnode*, char**);
    static bool drDefineStipple(lispnode*, lispnode*, char**);
    static bool drDefineLineStyle(lispnode*, lispnode*, char**);
    static bool drDefinePacket(lispnode*, lispnode*, char**);

    char *c_display;            // Display name (first in drDefineDisplay list).
    SymTab *c_color_tab;
    SymTab *c_color_rtab;
    SymTab *c_stipple_tab;
    SymTab *c_stipple_rtab;
    SymTab *c_line_tab;
    SymTab *c_packet_tab;
    SymTab *c_packet_rtab;
    stringlist *c_badpkts;      // List of unresolved packet reference names.
    stringlist *c_badstips;     // List of unresolved stipple reference names.
    stringlist *c_badclrs;      // List of unresolved color reference names.

    static cTechDrfIn *instancePtr;
    static sDrfDeferred *deferred;
};

#endif

