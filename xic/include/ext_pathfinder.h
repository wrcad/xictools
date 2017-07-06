
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
 $Id: ext_pathfinder.h,v 5.20 2014/11/05 05:43:35 stevew Exp $
 *========================================================================*/

#ifndef PATHFINDER_H
#define PATHFINDER_H

#include "cd_lgen.h"


// This is a generator for iterating through the layers, used in Paths
// and Quick Paths.  Only electable layers are returned, invisible
// layers are returned unless false is passed to the constructor. 
// Iterate up or down, the last descriptor returned is the dark field
// ground plane.
//
struct pf_lgen
{
    pf_lgen(bool);
    CDl *next();

private:
    CDlgen gen;
    CDl *gpld;          // Store ground plane, return last.
    bool skip_inv;      // Skip invisible layers.
    bool nogo;          // Don't return anything.
};


// Stack for pathfinder::extract_subpath(), to avoid an actual
// recursive function call.
//
struct pf_stack_elt
{
    pf_stack_elt(SymTab *t, CDo *od, pf_stack_elt *n) : gen(t)
        {
            odesc = od;
            oprev = 0;
            onext = 0;
            ent = 0;
            next = n;
        }

    static void destroy(pf_stack_elt *p)
        {
            while (p) {
                pf_stack_elt *x = p;
                p = p->next;
                delete x;
            }
        }

    SymTabGen gen;          // iterator for symbol table
    CDo *odesc;             // reference object (not in table)
    CDo *oprev;             // prev element in table CDo list
    CDo *onext;             // next element in table CDo list
    SymTabEnt *ent;         // table container
    pf_stack_elt *next;
};

struct pathfinder;

// Struct returned by pathfinder::extract_subpath(), provides a list of
// elements ordered along the subpath.  The destructor does not free the
// objects, which are assumed to also exist in the pf_tab table.
//
struct pf_ordered_path
{
    pf_ordered_path(pf_stack_elt*);
    pf_ordered_path(CDo*);
    ~pf_ordered_path() { delete [] elements; }

    void simplify(pathfinder*);

    int num_elements;
    CDo **elements;
};

// Base class for finding physical nets.
//
struct
pathfinder
{
    enum PFtype { PFdup, PFinserted, PFerror };

    pathfinder()
        {
            pf_topcell = 0;
            pf_pathname = 0;
            pf_tab = 0;
            pf_depth = 0;
            pf_visible = false;
        }

    virtual ~pathfinder()
        {
            clear();
        }

    void set_depth(int d) { pf_depth = d; }

    SymTab *path_table() { return (pf_tab); }

    const char *pathname() { return (pf_pathname); }

    virtual bool find_path(BBox*);
    virtual bool find_path(const CDo*);
    virtual void clear();
    virtual bool using_groups() { return (false); }

    pf_ordered_path *extract_subpath(const BBox*, const BBox*);
    bool load(pf_ordered_path*);
    void show_path(WindowDesc*, bool);
    PFtype insert(CDo*);
    bool remove(CDo*);
    CDo *find_and_remove(const BBox*);
    bool atomize_path();
    bool is_contacting(CDo*, CDo*);
    bool is_empty();
    CDo *get_object_list();
    CDo *get_via_list(const CDo*, XIrt*, bool);

protected:
    CDol *neighbors(CDs*, CDo*);

    CDcellName pf_topcell;  // the name of the top-level cell
    char *pf_pathname;      // a name for the path
    SymTab *pf_tab;         // tag is "real" odesc, datum is list of copies
    int pf_depth;           // search depth
    bool pf_visible;        // the path is being displayed
};


// Stack element for grp_pathfinder.
struct csave
{
    csave() { cdesc = 0; x = y = 0; }

    CDc *cdesc;
    unsigned short x, y;
};

struct grp_pathfinder : public pathfinder, public cTfmStack
{
    grp_pathfinder()
        {
            pf_found_ground = false;
        }

    bool find_path(BBox*);
    bool find_path(const CDo*);

    void clear()
        {
            pf_found_ground = false;
            pathfinder::clear();
        }

    bool using_groups() { return (true); }

private:
    bool grp_find_path(CDs*, BBox*, int);
    void recurse_up(int, int);
    void recurse_path(cGroupDesc*, int, int);

    bool pf_found_ground;
    csave pf_stack[CDMAXCALLDEPTH];
};

#endif

