
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
 $Id: ext_rlsolver.h,v 5.8 2014/04/14 05:22:32 stevew Exp $
 *========================================================================*/

#ifndef RLSOLVER_H
#define RLSOLVER_H

#include "geo_zlist.h"


//
// Interconnect resistance extractor
//

class spMatrixFrame;

//-------------------------------------------------------------------------
// RLsolver: solve for resistance/inductance on a single layer

#define RLS_DEF_NUM_GRID 1000
#define RLS_DEF_MAX_GRID 50000

// Struct to save an edge, horizontal or vertical.  The e2 is always
// larger than e1.
//
struct RLedge
{
    RLedge(int pt, int end1, int end2, RLedge *n)
        {
            next = n;
            if (end2 > end1) {
                e1 = end1;
                e2 = end2;
            }
            else {
                e1 = end2;
                e2 = end1;
            }
            p = pt;
        }

    void free()
        {
            RLedge *e = this;
            while (e) {
                RLedge *en = e->next;
                delete e;
                e = en;
            }
        }

    RLedge *next;
    int e1;
    int e2;
    int p;
};

// Per-contact info.
//
struct RLcontact
{
    RLcontact()
        {
            czl = 0;
            h_edges = 0;
            v_edges = 0;
        }

    ~RLcontact()
        {
            Zlist::free(czl);
            h_edges->free();
            v_edges->free();
        }

    BBox cBB;             // contact bounding box
    Zlist *czl;           // contact zoid list
    RLedge *h_edges;      // Manhattan horizontal edge list;
    RLedge *v_edges;      // Manhattan vertical edge list;
};

// Initialization state
enum RLstate { RLuninit, RLresist, RLinduct };

// Struct to support per-square extraction (resistance and inductance).
//
struct RLsolver
{
    RLsolver()
        {
            rl_matrix = 0;
            rl_zlist = 0;
            rl_h_edges = 0;
            rl_v_edges = 0;
            rl_contacts = 0;
            rl_ld = 0;
            rl_logfp = 0;
            rl_nx = rl_ny = 0;
            rl_delta = 0;
            rl_num_contacts = 0;
            rl_offset = 0;
            rl_state = RLuninit;
        }

    ~RLsolver();

    bool setup(const Zlist*, CDl*, Zgroup*);
    bool setupR();
    bool setupL();
    bool solve_two(double*);
    bool solve_multi(int*, float**);

private:

    // Return the node value at x, y
    //
    int nodeof(int x, int y)
        {
            Point_c px(rl_BB.left + rl_delta*x + rl_delta/2,
                rl_BB.bottom + rl_delta*y + rl_delta/2);
            for (int i = 0; i < rl_num_contacts; i++) {
                if (rl_contacts[i].cBB.intersect(&px, true) &&
                        Zlist::intersect(rl_contacts[i].czl, &px, true))
                    return (i + rl_offset);
            }
            if (Zlist::intersect(rl_zlist, &px, true))
                return (y*rl_nx + x + rl_num_contacts + rl_offset);
            return (-1);
        }

    // Return true if x,y is over object
    //
    bool inside(int x, int y)
        {
            Point_c px(rl_BB.left + rl_delta*x + rl_delta/2,
                rl_BB.bottom + rl_delta*y + rl_delta/2);
            if (Zlist::intersect(rl_zlist, &px, true))
                return (true);
            return (false);
        }

    // Return the first x to right not on object
    //
    int fdx(int x, int y)
        {
            if (!inside(x, y))
                return (-1);
            int xh = x + 1;
            while (xh < rl_nx) {
                if (!inside(xh, y))
                    break;
                xh++;
            }
            return (xh);
        }

    // Return the first y above not on object
    //
    int fdy(int x, int y)
    {
        if (!inside(x, y))
            return (-1);
        int yh = y;
        while (yh < rl_ny) {
            if (!inside(x, yh))
                break;
            yh++;
        }
        return (yh);
    }

    void setup_edges();
    int find_tile();
    void set_delta();
    void add_element(int, int, double = 1.0);
    double l_per_sq(int);

    spMatrixFrame *rl_matrix;   // the matrix
    Zlist *rl_zlist;            // body area
    RLedge *rl_h_edges;         // horizontal edges
    RLedge *rl_v_edges;         // vertical edges
    RLcontact *rl_contacts;     // contact list
    CDl *rl_ld;                 // body layer desc
    BBox rl_BB;                 // device bounding box
    FILE *rl_logfp;             // log file pointer
    int rl_nx, rl_ny;           // size of matrix
    int rl_delta;               // spatial increment
    int rl_num_contacts;        // number of contacts
    int rl_offset;              // set to 1 if num_contacts > 2
    RLstate rl_state;           // initialization state

public:
    static int rl_given_delta;  // overriding grid spacing
    static int rl_numgrid;      // number of internal grid points, used
                                // when not tiling and no delta given
    static int rl_maxgrid;      // maximum number of grid cells, used
                                // when tiling
    static bool rl_try_tile;    // attempt to tile
};


//-------------------------------------------------------------------------
// MRsolver: solve for resistance on a wire net (multi-layer)

struct via_t;
struct cnd_t;

// Struct describing a via.
//
struct via_t
{
    via_t(PolyList *p, CDl *l, int n, via_t *v)
        { po = p; ld = l; node = n; next = v; c1 = 0; c2 = 0; }
    ~via_t() { delete po; }

    via_t *next;    // link
    PolyList *po;   // via shape (list length = 1)
    CDl *ld;        // via layer
    cnd_t *c1;      // first conductor
    cnd_t *c2;      // second conductor
    int node;       // internal resistor node
};

// Via list element for conductor.
//
struct vls_t
{
    vls_t(via_t *v, vls_t *n) { via = v; next = n; }
    void free()
        {
            vls_t *v = this;
            while (v) {
                vls_t *vn = v->next;
                delete v;
                v = vn;
            }
        }

    via_t *via;     // pointer to via
    vls_t *next;    // link
};

// Struct describing a conducting object.
//
struct cnd_t
{
    cnd_t(PolyList *p, CDl *l, cnd_t *n)
        { po = p; ld = l; vias = 0; next = n; gmat = 0; gmat_size = 0; }
    ~cnd_t()
        {
            delete po;
            vias->free();
            delete [] gmat;
        }

    cnd_t *next;    // link
    PolyList *po;   // object shape (list length = 1)
    CDl *ld;        // conducting layer
    vls_t *vias;    // list of associated vias (includes terminals)
    float *gmat;    // result conductance matrix
    int gmat_size;  // matrix size
};

// Main solver struct.
//
struct MRsolver
{
    MRsolver()
        { mr_ltab = 0; mr_vias = 0; mr_term_count = 0; }
    ~MRsolver();

    bool find_path(int, int, int, bool);
    bool load_path(CDol*);
    bool add_terminal(Zlist*);
    bool find_vias();
    bool solve_elements();
    bool write_spice(FILE*);
    bool solve(int*, float**);

    SymTab *mr_ltab;    // table of conductor lists keyed by layer
    via_t *mr_vias;     // list of vias
    int mr_term_count;  // terminals added
};

#endif

