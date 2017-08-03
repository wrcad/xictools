
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

#ifndef CD_OBJECTS_H
#define CD_OBJECTS_H

#include "geo_rtree.h"
#include "geo_poly.h"
#include "geo_wire.h"
#include "cd_label.h"
#include "cd_property.h"
#include "texttf.h"


//-------------------------------------------------------------------------
// Database objects
//-------------------------------------------------------------------------

// State of object in database (return from state() method).
//
// CDVanilla    Normal object
// CDSelected   Shown highlihted in display, should be listed in
//               select queue.
// CDDeleted    Object being deleted, not shown in display.
// CDIncomplete Object being created, shown as outline in display.
// CDInternal   Internal object, shown normally but not converted
//               in output, can't be selected.
//
enum ObjState { CDVanilla, CDSelected, CDDeleted, CDIncomplete, CDInternal };

// Flags for CDo::oFlags
//
#define CDmergeDeleted 0x1
#define CDmergeCreated 0x2
#define CDnoDRC        0x4
#define CDexpand       0x8
// expand flags use rest of byte
#define CDoMark1       0x100
#define CDoMark2       0x200
#define CDoMarkExtG    0x400
#define CDoMarkExtE    0x800
#define CDinqueue      0x1000
#define CDnoMerge      0x4000
#define CDisCopy       0x8000
//
// CDmergeDeleted   Object has been deleted due to merge.
// CDmergeCreated   Object has been created due to merge.
// CDnoDRC          Skip DRC tests on this object.
// CDexpand         Five flags are used to keep track of cell expansion
//                  in main plus four sub-windows, in symbol calls only.
// CDoMark1,2       General purpose application flags.
// CDoMarkExtG      Extraction system, in grouping phonycell.
// CDoMarkExtE      Extraction system, in extraction phonycell.
// CDinqueue        Object is in selection queue.
// CDnoMerge        Object will not be merged.
// CDisCopy         Object is a copy, not in database.

// CDmergeInhibit will set/unset the CDnoMerge flags in a list of
// objects.  It keeps its own list of flagged objects, to be sure
// all objects are reset.
//
struct CDmergeInhibit
{
    CDmergeInhibit(CDol*);
    ~CDmergeInhibit() { revert(); }

    void revert();

private:
    CDo **mi_objects;
    int mi_count;
};


// CDo "virtual" function support.
// In order to minimize in-core memory usage, facilitate application
// interfacing, and possibly improve performance, C++ virtual
// functions are not used in CDo.  Instead, we use the computed goto
// extension for dispatching from the type() field.  This extension is
// supported by gcc and many other compilers (but not Sun Forte C++ at
// this time, but is supported by Forte C).

// Use when compiler supports computed goto.
//
#define COMPUTED_JUMP(odsc) \
    static void *array[] = { &&box, &&poly, &&wire, &&label, &&inst }; \
    goto *array[odsc->dispatch_code()];

// Used otherwise.
//
#define CONDITIONAL_JUMP(odsc) \
    switch (odsc->dispatch_code()) { \
    case 0: goto box; \
    case 1: goto poly; \
    case 2: goto wire; \
    case 3: goto label; \
    case 4: goto inst; \
    }

// The global static CDo_helper simply contains an offset translation
// method for dispatching.  The CDo::dispatch method uses this to
// return the dispatch offset.
//
struct CDo_helper
{
    CDo_helper()
        {
            codebf[(unsigned int)CDBOX] = 0;
            codebf[(unsigned int)CDPOLYGON] = 1;
            codebf[(unsigned int)CDWIRE] = 2;
            codebf[(unsigned int)CDLABEL] = 3;
            codebf[(unsigned int)CDINSTANCE] = 4;
        }
    unsigned int f(unsigned int c) { return (codebf[c & 0x7f]); }
protected:
    unsigned char codebf[128];
};

// A skeletal "virtual" function takes this form:
//
//      xxx(CDo *odesc)
//      {
//          COMPUTED_JUMP(odesc)
//      box:
//          code for box ;
//          return;
//      poly:
//          code for poly ;
//          return;
//      wire:
//          code for wire ;
//          return;
//      label:
//          code for label ;
//          return;
//      inst:
//          code for instance ;
//          return;
//      }


// Object desc
struct CDo : public RTelem
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
#endif

    CDo(CDl *ld, const BBox *BB = 0)
        {
            // The RTelem fields that aren't used in leaf nodes are
            // used here, mapped to things that apply in object-land.

            if (BB) {
                e_BB = *BB;
                e_BB.fix();
            }
            e_type = CDBOX;
            e_children = (RTelem*)ld;
            e_nchildren = CDVanilla;
            e_flags = 0;
#ifndef CD_GROUP_TAB
            oGroup = 0;                     
#endif
#ifndef CD_PRPTY_TAB
            oPrptyList = 0;
#endif
        }

    // This and derived classes have no destructors.  The RTelem
    // destructor handles all cleanup.

    // For "virtual function" dispatching.
    unsigned int dispatch_code() const { return (o_hlpr.f(type())); }

    int type()                  const { return (e_type); }
    CDl *ldesc()                const { return (CDl*)e_children; }
    int state()                 const { return (e_nchildren); }
    void set_state(int s)             { e_nchildren = (char)s; }
    unsigned int flags()        const { return (e_flags); }
    void set_flags(unsigned int f)    { e_flags = (unsigned short)f; }
    bool has_flag(int f)        const { return (e_flags & f); }
    void set_flag(int f)              { e_flags |= f; }
    void unset_flag(int f)            { e_flags &= ~f; }

    bool is_normal() const
        {
            return (state() == CDVanilla || state() == CDSelected);
        }

    bool is_copy()              const { return (has_flag(CDisCopy)); }
    void set_copy(bool b)
        {
            if (b)
                set_flag(CDisCopy);
            else
                unset_flag(CDisCopy);
        }

    // In copies, the 'e_right' pointer can be used for various things. 
    // In one case, this is used to save a pointer to the "real" odesc
    // from which the possibly transformed copy was made.  Below are
    // access functions, const and non-const versions.
    // See CDol::object_list.

    void set_next_odesc(const CDo *ptr)
        {
            if (is_copy())
                e_right = (RTelem*)ptr;
        }

    CDo *next_odesc() const
        {
            return (is_copy() ? (CDo*)e_right : 0);
        }

    const CDo *const_next_odesc() const
        {
            return (is_copy() ? (const CDo*)e_right : 0);
        }

    // Next/prev object in database (sorted order)
    CDo *db_next()                    { return ((CDo*)next()); }
    CDo *db_prev()                    { return ((CDo*)prev()); }

    void set_prpty_list(CDp *p)
        {
#ifdef CD_PRPTY_TAB
            CD()->SetPrptyList(this, p);
#else
            oPrptyList = p;
#endif
        }

    void link_prpty_list(CDp *p)
        {
#ifdef CD_PRPTY_TAB
            CD()->LinkPrptyList(this, p);
#else
            if (p) {
                p->set_next_prp(oPrptyList);
                oPrptyList = p;
            }
#endif
        }

    CDp *prpty_list() const
        {
#ifdef CD_PRPTY_TAB
            return (CD()->PrptyList(this));
#else
            return (oPrptyList);
#endif
        }

    CDp *prpty(int pnum) const
        {
            for (CDp *pd = prpty_list(); pd; pd = pd->next_prp()) {
                if (pd->value() == pnum)
                    return (pd);
            }
            return (0);
        }

    void set_group(int g)
        {
#ifdef CD_GROUP_TAB
            CD()->SetGroup(this, g);
#else
            oGroup = g;
#endif
        }

    int group() const
        {
#ifdef CD_GROUP_TAB
            return (CD()->Group(this));
#else
            return (oGroup);
#endif
        }

    bool operator!=(const CDo &od) const { return !(od == *this); }

    bool intersect(int x, int y, bool t_ok) const
        {
            Point_c p(x, y);
            return (intersect(&p, t_ok));
        }

    // cd_objhash.cc
    bool operator==(const CDo&) const;
    unsigned int hash() const;

    // cd_objmisc.cc
    void set_oBB(const BBox&);
    void set_ldesc(CDl*);
    CDp *prptyAddCopy(CDp*);
    void prptyAddCopyList(CDp*);
    CDp *prptyUnlink(CDp*);
    void prptyRemove(int);
    void prptyFreeList();
    void prptyClearInternal();
    static CDo *fromCifString(CDl*, const char*);

    // cd_objvirt.cc
    void destroy();
    bool intersect(const CDo*, bool) const;
    bool intersect(const Point*, bool) const;
    bool intersect(const BBox*, bool) const;
    bool intersect(const Poly*, bool) const;
    bool intersect(const Wire*, bool) const;
    void computeBB();
    double area() const;
    double perim() const;
    void centroid(double*, double*) const;
    Zlist *toZlist() const;
    Zlist *toZlistR() const;
    CDo *dup() const;
    CDo *dup(CDs*) const;
    CDo *copyObject(bool = false) const;
    CDo *copyObjectWithXform(const cTfmStack*, bool = false) const;
    void boundary(BBox*, Point**) const;
    char *cif_string(int, int, bool = false) const;
    bool prptyAdd(int, const char*, DisplayMode);

protected:
#ifndef CD_GROUP_TAB
    int oGroup;             // conductor group for extraction
#endif
#ifndef CD_PRPTY_TAB
    CDp *oPrptyList;        // object properties
#endif
    static CDo_helper o_hlpr;
};

// Polygon desc
struct CDpo : public CDo
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
#endif

    CDpo(CDl* ld, Poly* poly = 0) : CDo(ld)
        {
            e_type = CDPOLYGON;
            if (poly) {
                poNumpts = poly->numpts;
                poPoints = poly->points;
                computeBB();
            }
            else {
                poNumpts = 0;
                poPoints = 0;
            }
        }

    CDpo(CDo *o, int num, Point *pts) : CDo(o->ldesc(), &o->oBB())
        {
            // Create a poly from a box struct (used in DRC)
            e_type = CDPOLYGON;
            e_nchildren = o->state();
            e_flags = o->flags();
#ifndef CD_GROUP_TAB
            set_group(o->group());
#endif
            poNumpts = num;
            poPoints = pts;
        }

    // cd_hash.cc
    unsigned int add_hash(unsigned int);

    // It would be most convenient to simply have a Poly struct as a
    // member, however this wastes space in 64-bit binaries.  Thus, we
    // keep distinct members for points, numpts.  We use inline
    // wrappers to export the Poly methods needed, which facilitates
    // keeping the members correctly updated after Poly operations
    // that alter the vertex list.

    void reverse_list()
        {
            for (int i = 0, j = poNumpts-1; i < j; i++, j--) {
                Point p(poPoints[i]);
                poPoints[i] = poPoints[j];
                poPoints[j] = p;
            }
        }

    void offset_list(int dx, int dy)
        {
            for (int i = 0; i < poNumpts; i++) {
                poPoints[i].x += dx;
                poPoints[i].y += dy;
            }
        }

    const Point *points()       const { return (poPoints); }
    void set_points(Point *p)   { poPoints = p; }
    int numpts()                const { return (poNumpts); }
    void set_numpts(int n)      { poNumpts = n; }

    // Use this when we need pointer to the points.  This allows changing
    // the points list values, be careful!
    const Poly po_poly()        const { return (Poly(poNumpts, poPoints)); }

    bool po_v_compare(const CDpo *po) const
        {
            const Poly p1(poNumpts, poPoints);
            const Poly p2(po->poNumpts, po->poPoints);
            return (p1.v_compare(p2));
        }

    bool po_intersect(const Point *px, bool touchok) const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.intersect(px, touchok));
        }

    bool po_intersect(const BBox *BB, bool touchok) const
        {
            if (oBB() <= *BB)
                return (true);
            const Poly p1(poNumpts, poPoints);
            return (p1.intersect(BB, touchok));
        }

    bool po_intersect(const Poly *po, bool touchok) const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.intersect(po, touchok));
        }

    bool po_intersect(const Wire *w, bool touchok) const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.intersect(w, touchok));
        }

    void po_computeBB(BBox *BB) const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.computeBB(BB));
        }

    double po_area() const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.area());
        }

    int po_perim() const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.perim());
        }

    void po_centroid(double *pcx, double *pcy) const
        {
            const Poly p1(poNumpts, poPoints);
            p1.centroid(pcx, pcy);
        }

    Zlist *po_toZlist() const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.toZlist());
        }

    Zlist *po_toZlistR() const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.toZlistR());
        }

    PolyList *po_clip(const BBox *BB, bool *bp = 0) const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.clip(BB, bp));
        }

    Poly *po_clip_acute(int m) const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.clip_acute(m));
        }

    bool po_is_manhattan() const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.is_manhattan());
        }

    bool po_has_non45() const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.has_non45());
        }

    Otype po_winding() const
        {
            const Poly p1(poNumpts, poPoints);
            return (p1.winding());
        }

    int po_check_poly(int f, bool b)
        {
            Poly p1(poNumpts, poPoints);
            int ret = p1.check_poly(f, b);
            poNumpts = p1.numpts;
            poPoints = p1.points;
            return (ret);
        }

private:
    int poNumpts;
    Point *poPoints;
};

// Wire desc
struct CDw : public CDo
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
#endif

    CDw(CDl* ld, Wire* wire = 0) : CDo(ld)
        {
            e_type = CDWIRE;
            if (wire) {
                wNumpts = wire->numpts;
                wAttributes = wire->attributes;
                wPoints = wire->points;
                computeBB();
            }
            else {
                wNumpts = 0;
                wAttributes = 0;
                wPoints = 0;
            }
        }

    // cd_hash.cc
    unsigned int add_hash(unsigned int);

    // cd_objmisc.cc
    bool set_node_label(CDs*, CDla*, bool = false);

    // It would be most convenient to simply have a Wire struct as a
    // member, however this may waste space in 64-bit binaries.  Thus,
    // we keep distinct members for points, numpts.  We use inline
    // wrappers to export the Wire methods needed, which facilitates
    // keeping the members correctly updated after Wire operations
    // that alter the vertex list or attributes.

    void reverse_list()
        {
            for (int i = 0, j = wNumpts-1; i < j; i++, j--) {
                Point p(wPoints[i]);
                wPoints[i] = wPoints[j];
                wPoints[j] = p;
            }
        }

    void offset_list(int dx, int dy)
        {
            for (int i = 0; i < wNumpts; i++) {
                wPoints[i].x += dx;
                wPoints[i].y += dy;
            }
        }

    void delete_vertex(int i)
        {
            if (i > 0 && i < wNumpts) {
                int n = wNumpts - 1;
                for ( ; i < n; i++)
                    wPoints[i] = wPoints[i+1];
                wNumpts--;
            }
        }

    bool has_vertex_at(const Point &p) const
        {
            for (int i = 0; i < wNumpts; i++) {
                if (wPoints[i] == p)
                    return (true);
            }
            return (false);
        }

    // Return true if the wire has a bound node name label, Electrical
    // mode only.
    //
    bool has_label()
        {
            CDp_node *pn = (CDp_node*)prpty(P_NODE);
            if (pn && pn->bound())
                return (true);
            CDp_bnode *pb = (CDp_bnode*)prpty(P_BNODE);
            if (pb && pb->bound())
                return (true);
            return (false);
        }

    const Point *points()       const { return (wPoints); }
    void set_points(Point *p)   { wPoints = p; }
    int numpts()                const { return (wNumpts); }
    void set_numpts(int n)      { wNumpts = n; }
    unsigned int wire_width()   const { return ((wAttributes & ~0x3) >> 1); }
    WireStyle wire_style()  const { return ((WireStyle)(wAttributes & 0x3)); }
    void set_wire_width(unsigned int w)
        { wAttributes = ((w >> 1) << 2) | (wAttributes & 0x3); }
    void set_wire_style(WireStyle s)
        { wAttributes = (wAttributes & ~0x3) | (s & 0x3); }
    unsigned int attributes()           const { return (wAttributes); }
    void set_attributes(unsigned int a) { wAttributes = a; }

    // Use this when we need pointer to the points.  This allows changing
    // the points list values, be careful!
    const Wire w_wire()         const { return (Wire(wNumpts, wPoints,
        wAttributes)); }

    bool w_v_compare(const CDw *w) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            const Wire w2(w->wNumpts, w->wPoints, w->wAttributes);
            return (w1.v_compare(w2));
        }

    bool w_intersect(const Point *px, bool touchok) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.intersect(px, touchok));
        }

    bool w_intersect(const BBox *BB, bool touchok) const
        {
            if (oBB() <= *BB)
                return (true);
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.intersect(BB, touchok));
        }

    bool w_intersect(const Poly *po, bool touchok) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.intersect(po, touchok));
        }

    bool w_intersect(const Wire *w, bool touchok) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.intersect(w, touchok));
        }

    void w_computeBB(BBox *BB) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.computeBB(BB));
        }

    double w_area() const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.area());
        }

    int w_perim() const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.perim());
        }

    void w_centroid(double *pcx, double *pcy) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            w1.centroid(pcx, pcy);
        }

    bool w_toPoly(Point **p, int *n) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.toPoly(p, n));
        }

    Zlist *w_toZlist() const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.toZlist());
        }

    Zlist *w_toZlistR() const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.toZlistR());
        }

    PolyList *w_clip(const BBox *BB, bool b) const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.clip(BB, b));
        }

    bool w_is_manhattan() const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.is_manhattan());
        }

    bool w_has_non45() const
        {
            const Wire w1(wNumpts, wPoints, wAttributes);
            return (w1.has_non45());
        }

private:
    int wNumpts;
    unsigned int wAttributes;
    Point *wPoints;
};

// Label desc
struct CDla : public CDo
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
#endif

    CDla(CDl* ld , Label* ladesc = 0) : CDo(ld)
        {
            e_type = CDLABEL;
            if (ladesc) {
                laXform = ladesc->xform;
                laLabel = ladesc->label;
                laX = ladesc->x;
                laY = ladesc->y;
                laWidth = ladesc->width;
                laHeight = ladesc->height;
                computeBB();
            }
            else {
                laXform = 0;
                laLabel = 0;
                laX = 0;
                laY = 0;
                laWidth = 0; 
                laHeight = 0;
            }
        }

    // cd_hash.h
    unsigned int add_hash(unsigned int);

    // cd_label.cc
    bool format_string(char**, int, hyList*, bool, FileType);
    bool link(CDs*, CDo*, CDp*);

    // It would be most convenient to simply have a Label struct as a
    // member, however this may waste space in 64-bit binaries.  Thus,
    // we keep distinct fields.  We use inline wrappers to export the
    // Label methods needed, which facilitates keeping the members
    // correctly updated after Label operations that alter the
    // attributes.

    int xform()                 const { return (laXform); }
    void set_xform(int x)       { laXform = x; }
    hyList *label()             const { return (laLabel); }
    void set_label(hyList *h)   { laLabel = h; }
    int xpos()                  const { return (laX); }
    void set_xpos(int x)        { laX = x; }
    int ypos()                  const { return (laY); }
    void set_ypos(int y)        { laY = y; }
    int width()                 const { return (laWidth); }
    void set_width(int w)       { laWidth = w; }
    int height()                const { return (laHeight); }
    void set_height(int h)      { laHeight = h; }

    // When set, the label is shown only when the containing cell is
    // top level, and not in instances of the cell.
    //
    bool no_inst_view()         const { return (laXform & TXTF_TLEV); }
    void set_no_inst_view(bool b)
        {
            if (b)
                laXform |= TXTF_TLEV;
            else
                laXform &= ~TXTF_TLEV;
        }

    // When set, the label subscribes to an application-set limit on
    // the number of lines shown.
    //
    bool use_line_limit()       const { return (laXform & TXTF_LIML); }
    void set_use_line_limit(bool b)
        {
            if (b)
                laXform |= TXTF_LIML;
            else
                laXform &= ~TXTF_LIML;
        }

    const Label la_label()      const { return (Label(laLabel, laX, laY,
        laWidth, laHeight, laXform)); }

private:
    unsigned int laXform;
    hyList *laLabel;
    int laX, laY;
    int laWidth, laHeight;
};


// List element for CDo
struct CDol
{
#ifdef CD_USE_MANAGER
    void *operator new(size_t);
    void operator delete(void*, size_t);
#endif
    CDol()
        {
            next = 0;
            odesc = 0;
        }

    CDol(CDo *o, CDol *n)
        {
            next = n;
            odesc = o;
        }

    // Return a list of objects obtained from the list of copies
    // passed.  The passed list will have its pointer linkage cleared,
    // so that the returned list owns the copies.
    //
    static CDol *object_list(CDo *od)
        {
            if (!od)
                return (0);
            CDol *o0 = 0, *oe = 0;
            while (od) {
                CDo *on = od->next_odesc();
                od->set_next_odesc(0);
                if (!o0)
                    o0 = oe = new CDol(od, 0);
                else {
                    oe->next = new CDol(od, 0);
                    oe = oe->next;
                }
                od = on;
            }
            return (o0);
        }

    static void destroy(CDol *ol)
        {
            while (ol) {
                CDol *ox = ol;
                ol = ol->next;
                delete ox;
            }
        }

    // Copy the list.
    //
    static CDol *dup(const CDol *thisol)
        {
            CDol *ol0 = 0, *ole = 0;
            for (const CDol *ol = thisol; ol; ol = ol->next) {
                if (!ol0)
                    ol0 = ole = new CDol(ol->odesc, 0);
                else {
                    ole->next = new CDol(ol->odesc, 0);
                    ole = ole->next;
                }
            }
            return (ol0);
        }

    // Compute the BB of the listed objects.
    //
    static void computeBB(const CDol *thisol, BBox *nBB)
        {
            *nBB = CDnullBB;
            for (const CDol *ol = thisol; ol; ol = ol->next) {
                if (ol->odesc)
                    nBB->add(&ol->odesc->oBB());
            }
        }

    CDol *next;
    CDo *odesc;         // object desc of referencing instance
};

#endif

