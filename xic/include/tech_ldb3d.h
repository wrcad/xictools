
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

#ifndef TECH_LDB3D_H
#define TECH_LDB3D_H

#include "geo_3d.h"

// Extraction Tech Variables
#define VA_NoPlanarize          "NoPlanarize"
#define VA_LayerReorderMode     "LayerReorderMode"
#define VA_Db3ZoidLimit         "Db3ZoidLimit"

#define DB3_DEF_ZLIMIT  10000

struct Ldb3d;
struct Ylist;
struct CDs;
struct CDo;

// Description of a layer in the structure.
//
struct Layer3d
{
    Layer3d(CDl *ld, Layer3d *n)
        {
            l3_next = n;
            l3_prev = 0;
            l3_ldesc = ld;
            l3_uncut = 0;
            l3_cut = 0;
            l3_yl3d = 0;
            l3_index = -1;
            l3_plane = 0;
        }

    ~Layer3d();

    static void destroy(Layer3d *l)
        {
            while (l) {
                Layer3d *x = l;
                l = l->l3_next;
                delete x;
            }
        }

    Layer3d *next()             const { return (l3_next); }
    void set_next(Layer3d *n)   { l3_next = n; }
    Layer3d *prev()             const { return (l3_prev); }
    void set_prev(Layer3d *p)   { l3_prev = p; }

    CDl *layer_desc()           const { return (l3_ldesc); }
    Ylist *uncut()              const { return (l3_uncut); }
    void set_uncut(Ylist *y)    { l3_uncut = y; }

    int index()                 const { return (l3_index); }
    void set_index(int i)       { l3_index = i; }

    int plane()                 const { return (l3_plane); }

    glYlist3d *yl3d()           const { return (l3_yl3d); }
    void set_yl3d(glYlist3d *l) { l3_yl3d = l; }

    bool is_conductor()     const { return (is_conductor(l3_ldesc)); }
    bool is_insulator()     const { return (is_insulator(l3_ldesc)); }

    double epsrel() const;
    bool extract_geom(const CDs*, const Zlist*, int, int);
    bool intersect(int, int) const;
    bool cut(const Layer3d*);
    void mk3d(bool);
    glZlist3d *find_max_ztop_at(int, int) const;
    void planarize();
    CDl *bottom_layer();
    CDl *top_layer();
    bool add_db();

    static bool is_conductor(const CDl*);
    static bool is_insulator(const CDl*);

private:
    Layer3d *l3_next;           // Next in linked list.
    Layer3d *l3_prev;           // Previous in linked list.
    CDl *l3_ldesc;              // Associated CD layer.
    Ylist *l3_uncut;            // Initial patterning.
    Ylist *l3_cut;              // After cutting at edges on lower layers.
    glYlist3d *l3_yl3d;         // 
    int l3_index;               // Local layer table index.
    int l3_plane;               // Nonzero if planarized, elevation.
};

// Database
//
struct Ldb3d
{
    Ldb3d();
    ~Ldb3d();

    Layer3d *layer(int ix) const
        {
            Layer3d *l = db3_stack;
            for ( ; l; l = l->next()) {
                if (!ix--)
                    break;
            }
            return (l);
        }

    CDs *celldesc()             const { return (db3_sdesc); }
    const BBox *aoi()           const { return (&db3_aoi); }
    Zlist *ref_zlist()          const { return (db3_zlref); }
    Layer3d *layers()           const { return (db3_stack); }
    int num_layers()            const { return (db3_nlayers); }
    int num_groups()            const { return (db3_ngroups); }

    void set_logfp(FILE *fp)    { db3_logfp = fp; }

    bool init_cross_sect(CDs *sd, const BBox *BB)
        {
            return (init_stack(sd, BB, true, 0, 0.0, 0.0));
        }

    bool init_for_extraction(CDs *sd, const BBox *BB, const char *fn,
            double d, double t, int manh_gcnt=0, int manh_mode=0)
        {
            return (init_stack(sd, BB, false, fn, d, t, manh_gcnt, manh_mode));
        }

    bool order_layers();
    bool check_dielectrics();
    void layer_dump(FILE*) const;
    Blist **line_scan(const Point*, const Point*);

    static unsigned int zoid_limit()            { return (db3_zlimit); }
    static void set_zoid_limit(unsigned int i)  { db3_zlimit = i; }
    static bool logging()                       { return (db3_logging); }
    static void set_logging(bool b)             { db3_logging = b; }

protected:
    bool init_stack(CDs*, const BBox*, bool, const char*, double, double,
        int=0, int=0);
    bool insert_layer(Layer3d*);
    glZlist3d *find_object_under(const glZoid3d*) const;

    CDs *db3_sdesc;             // Source of geometry.
    BBox db3_aoi;               // Area of geometry extraction.
    double db3_subseps;         // Substrate relative diel. const.
    double db3_substhick;       // Substrate thickness, microns.
    Zlist *db3_zlref;           // AOI mask.
    Layer3d *db3_stack;         // Root of layer sequence.
    glZgroupRef3d *db3_groups;  // Conductor groups.
    int db3_nlayers;            // Number of layers in table.
    int db3_ngroups;            // Number of conductor groups.
    bool db3_is_cross_sect;     // True for cross section display.
    FILE *db3_logfp;            // Output some debug info if set.

    static unsigned int db3_zlimit;  // Abort if more than this many zoids.
    static bool db3_logging;         // Enable logging messages flag.  User
                                     //  must set_logfp after creating this.
};

#endif

