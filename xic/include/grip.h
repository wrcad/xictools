
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

#ifndef GRIP_H
#define GRIP_H

 // Ciranova Location enum.
enum cnLocation {
    CN_LL,  // lower_left
    CN_CL,  // center left
    CN_UL,  // upper_left
    CN_LC,  // lower_center
    CN_CC,  // center_center
    CN_UC,  // upper_center
    CN_LR,  // lower_right
    CN_CR,  // center_right
    CN_UR   // upper_right
};

// This struct holds parameters obtained from parsing a Ciranova
// stretch handle property.  We will follow this methodology in Xic. 
// The properties are applied to objects in PCells.  Each property
// will have a corresponding grip.
//
struct sCniGripDesc
{
    sCniGripDesc();
    ~sCniGripDesc();

    bool parse(const char**);

    const char *name()          const { return (gd_name); }
    void set_name(const char *n)
        {
            char *t = lstring::copy(n);
            delete [] gd_name;
            gd_name = t;
        }

    const char *param_name()    const { return (gd_param); }

    void setgd(const sCniGripDesc &gd)
        {
            gd_name = lstring::copy(gd.gd_name);
            gd_param = lstring::copy(gd.gd_param);
            gd_key = lstring::copy(gd.gd_key);
            gd_minval = gd.gd_minval;
            gd_maxval = gd.gd_maxval;
            gd_scale = gd.gd_scale;
            gd_snap = gd.gd_snap;
            gd_loc = gd.gd_loc;
            gd_absolute = gd.gd_absolute;
            gd_vert = gd.gd_vert;
        }

protected:
    char *gd_name;          // Name of stretch handle.
    char *gd_param;         // Name of parameter being adjusted.
    char *gd_key;           // Key name for multi-valued parameters.
    double gd_minval;       // Parameter minimum value.
    double gd_maxval;       // Parameter maximum value.
    double gd_scale;        // Scale factgor for parameter value.
    double gd_snap;         // Snap grid for parameter value.
    cnLocation gd_loc;      // Object active edge/ grip location.
    bool gd_absolute;       // True if increment measured with absolute
                            // coordinates, otherwise increment is
                            // meaasured relative to object center.
    bool gd_vert;           // True measurement direction vertical.
};


class cGripDb;
struct PConstraint;

// Xic grip descriptor.
//
struct sGrip : public sCniGripDesc
{
    sGrip(CDc*);
    ~sGrip();

    bool setup(const sCniGripDesc&, const BBox&);
    void set_active(bool);
    bool param_value(int, int, int, int, double*) const;
    void show_ghost(int, int, bool);

    void set(int x1, int y1, int x2,int y2)
        {
            g_x1 = x1;
            g_y1 = y1;
            g_x2 = x2;
            g_y2 = y2;
        }

    const char *param_name()    const { return (gd_param); }
    CDc *cdesc()                const { return (g_cdesc); }
    void set_id(int i)                { g_id = i; }
    int id()                    const { return ( g_id); }
    int end1x()                 const { return (g_x1); }
    int end1y()                 const { return (g_y1); }
    int end2x()                 const { return (g_x2); }
    int end2y()                 const { return (g_y2); }
    int ux()                    const { return (g_ux); }
    int uy()                    const { return (g_uy); }
    bool active()               const { return (g_active); }

private:
    double g_value;             // Parameter value.
    CDc *g_cdesc;               // PCell instance pointer.
    const PConstraint *g_constr;// Pointer to parameter constraint.
    int g_id;                   // Unique id for this grip.
    int g_x1;                   // These define the grip: either a
    int g_y1;                   // line segment or point.
    int g_x2;
    int g_y2;
    signed char g_ux;           // Unit vector of allowed motion.
    signed char g_uy;
    bool g_active;              // Inactive when instance not expanded.
};


// Note that the element structs are the same size, so we will use a
// common allocator.

struct idelt_t
{
    unsigned long tab_key()  const { return (id_key); }
    idelt_t *tab_next()            { return (id_next); }
    idelt_t *tgen_next(bool)       { return (id_next); }
    void set_tab_next(idelt_t *n)  { id_next = n; }

    void set_key(int k)            { id_key = k; }
    sGrip *grip()                  { return (id_grip); }
    void set_grip(sGrip *g)        { id_grip = g; }

private:
    unsigned long id_key;
    idelt_t *id_next;
    sGrip *id_grip;
};

struct cdelt_t
{
    unsigned long tab_key()  const { return (cd_key); }
    cdelt_t *tab_next()            { return (cd_next); }
    cdelt_t *tgen_next(bool)       { return (cd_next); }
    void set_tab_next(cdelt_t *n)  { cd_next = n; }

    void set_key(const void *p)    { cd_key = (unsigned long)p; }
    itable_t<idelt_t> *table()     { return (cd_tab); }
    void set_table(itable_t<idelt_t> *t) { cd_tab = t; }

private:
    unsigned long cd_key;
    cdelt_t *cd_next;
    itable_t<idelt_t> *cd_tab;
};


// Grip database for a cell.
//
class cGripDb
{
public:
    cGripDb();
    ~cGripDb();

    int saveGrip(sGrip*);
    bool deleteGrip(const CDc*, int);
    bool activateGrip(const CDc*, int, bool);
    sGrip *findGrip(const CDc*, int);
    bool hasCdesc(const CDc*);

    itable_t<cdelt_t> *table()      const { return (gdb_grip_tab); }

private:
    void recycle(void*);

    itable_t<cdelt_t> *gdb_grip_tab;
        // Hash table keyed by instance address which has payload of
        // hash table keyed by id pointing to sGrips.
    cdelt_t *gdb_unused;                // recycled elements
    int gdb_idcnt;                      // for ID generation
    eltab_t<cdelt_t> gdb_elt_allocator; // element factory
};

#define GG_ALL_GRIPS (const CDc*)1L

// Generator.  If the CDc argument is GG_ALL_GRIPS to the constructor,
// all grips for will be returned.  If this argument is null, grips of
// the current cell are returned.
//
struct sGripGen
{
    sGripGen(const cGripDb*, const CDc*);

    sGrip *next();

private:
    cdelt_t *gg_celt;
    tgen_t<cdelt_t> gg_cdgen;
    tgen_t<idelt_t> gg_idgen;
};

#endif

