
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
 $Id: fio_oas_reps.h,v 5.13 2013/05/02 19:26:35 stevew Exp $
 *========================================================================*/

#ifndef FIO_OAS_REPS_H
#define FIO_OAS_REPS_H


struct oas_out;

// Minimum run length (the default) when extracting for replication.
// Algorithm requires value 4 or larger.
#define REP_RUN_MIN 4
#define REP_RUN_MAX 65535

// Minimum array width (the default) when extracting for replication.
// Value must be 2 or larger.
#define REP_ARRAY_MIN 2
#define REP_ARRAY_MAX 65535

// Maximum number of unique objects of each type saved in repetition
// cache, will flush when exceeded.  Note that the per-object
// repetition count is unbounded.
#define REP_MAX_ITEMS  5000
#define REP_MAX_ITEMS_MIN  10
#define REP_MAX_ITEMS_MAX  50000

// Maximum total number of repetitions (the default) to save.
#define REP_MAX_REPS 1000000
#define REP_MAX_REPS_MIN 100
#define REP_MAX_REPS_MAX 1000000000


// Repetition cache definitions

// A point list element.
//
struct coor_t
{
    coor_t *next;
    int x;
    int y;
};

// List element for a 1-dimensional placement sequence.
//
struct oned_t
{
    oned_t *next;
    int eltx;           // origin x
    int elty;           // origin y
    int dx;             // period x
    int dy;             // period y
    int nelts;          // run length
};

// List element for a 2-dimensional placement sequence.
//
struct twod_t
{
    twod_t *next;
    int eltx;
    int elty;
    int dx1;
    int dy1;
    int nelts1;
    int dx2;
    int dy2;
    int nelts2;
};

// Placement representatons.
//
struct reps_t
{
    reps_t(int r, int a, coor_t *p)
        {
            x_arrays = 0;
            y_arrays = 0;
            x_lists = 0;
            y_lists = 0;
            residual = p;
            unused = 0;
            runmin = r;
            arraymin = a;
        }

    void build();
    const twod_t *x_array_list()    const { return (x_arrays); }
    const twod_t *y_array_list()    const { return (y_arrays); }
    const oned_t *x_run_list()      const { return (x_lists); }
    const oned_t *y_run_list()      const { return (y_lists); }
    const coor_t *point_list()      const { return (residual); }

private:
    oned_t *new_oned(int x, int y, int dx, int dy, int n)
        {
            oned_t *o = oned_factory.new_element();
            o->next = 0;
            o->eltx = x;
            o->elty = y;
            o->dx = dx;
            o->dy = dy;
            o->nelts = n;
            return (o);
        }

    twod_t *new_twod(oned_t *od)
        {
            twod_t *t = twod_factory.new_element();
            t->next = 0;
            if (od) {
                t->eltx = od->eltx;
                t->elty = od->elty;
                t->dx1 = od->dx;
                t->dy1 = od->dy;
                t->nelts1 = od->nelts;
                if (od->next) {
                    t->dx2 = od->next->eltx - t->eltx;
                    t->dy2 = od->next->elty - t->elty;
                    int n = 0;
                    for ( ; od; od = od->next, n++) ;
                    t->nelts2 = n;
                }
                else {
                    t->dx2 = 0;
                    t->dy2 = 0;
                    t->nelts2 = 0;
                }
            }
            else {
                t->eltx = 0;
                t->elty = 0;
                t->dx1 = 0;
                t->dy1 = 0;
                t->nelts1 = 0;
                t->dx2 = 0;
                t->dy2 = 0;
                t->nelts2 = 0;
            }
            return (t);
        }

    oned_t *get_row_runs();
    oned_t *get_col_runs();
    void sort_residual_by_row();
    void sort_residual_by_col();
    oned_t *row_compat_list(oned_t**);
    oned_t *col_compat_list(oned_t**);
    void get_row_arrays();
    void get_col_arrays();
    inline oned_t *next_row_poss(int, int, int*, oned_t**, int);
    inline bool row_upd(int&, int&, int, int, int);
    oned_t *find_row_array(oned_t**, int);
    inline oned_t *next_col_poss(int, int, int*, oned_t**, int);
    inline bool col_upd(int&, int&, int, int, int);
    oned_t *find_col_array(oned_t**, int);

    twod_t *x_arrays;   // 2-dimension periodic row arrays
    twod_t *y_arrays;   // 2-dimension periodic col arrays
    oned_t *x_lists;    // 1-dimension periodic rows
    oned_t *y_lists;    // 1-dimension periodic cols
    coor_t *residual;   // random placement
    coor_t *unused;     // recycle unused coor_t's
    int runmin;         // min run length
    int arraymin;       // min array width

    eltab_t<oned_t> oned_factory;
    eltab_t<twod_t> twod_factory;
};


//-----------------------------------------------------------------------------
// Element representation structs.  These are locally allocated from a
// pool, so there are no constructors or destructors.  The set and
// cleanup methods perform initialization and heap deallocation.

struct rf_box
{
    void set(const BBox *BB, unsigned int l, unsigned int d, CDp *p)
        {
            layer = l;
            datatype = d;
            width = BB->right - BB->left;
            height = BB->top - BB->bottom;
            x = BB->left;
            y = BB->bottom;
            others = 0;
            properties = p;
        }

    void set(unsigned int l, unsigned int d, CDp *p)
        {
            layer = l;
            datatype = d;
            width = 0;
            height = 0;
            x = 0;
            y = 0;
            others = 0;
            properties = p;
        }

    void set()
        {
            layer = 0;
            datatype = 0;
            width = 0;
            height = 0;
            x = 0;
            y = 0;
            others = 0;
            properties = 0;
        }

    void cleanup()
        {
            properties->free_list();
        }

    static unsigned int diff(const rf_box*, const rf_box*);

    bool operator==(const rf_box&) const;
    bool operator<(const rf_box&) const;
    unsigned int hash();

    unsigned short layer;
    unsigned short datatype;
    unsigned int width;
    unsigned int height;
    int x;
    int y;
    coor_t *others;
    CDp *properties;
};

struct rf_poly
{
    void set(const Poly *po, unsigned int l, unsigned int d, CDp *p)
        {
            boxpart.set(l, d, p);
            BBox BB;
            po->computeBB(&BB);
            boxpart.width = BB.right - BB.left;
            boxpart.height = BB.top - BB.bottom;
            boxpart.x = po->points[0].x;
            boxpart.y = po->points[0].y;
            numpts = po->numpts - 2;
            points = new Point[numpts];
            for (int i = 0; i < numpts; i++) {
                points[i].x = po->points[i+1].x - po->points[0].x;
                points[i].y = po->points[i+1].y - po->points[0].y;
            }
        }

    void set()
        {
            boxpart.set();
            numpts = 0;
            points = 0;
        }

    void cleanup()
        {
            boxpart.cleanup();
            delete [] points;
        }

    static unsigned int diff(const rf_poly*, const rf_poly*);

    bool operator==(const rf_poly&) const;
    bool operator<(const rf_poly&) const;
    unsigned int hash();

    rf_box boxpart;
    int numpts;
    Point *points;
};

struct rf_wire
{
    void set(const Wire *w, unsigned int l, unsigned int d, CDp *p)
        {
            boxpart.set(l, d, p);
            BBox BB;
            w->computeBB(&BB);
            boxpart.width = BB.right - BB.left;
            boxpart.height = BB.top - BB.bottom;
            boxpart.x = w->points[0].x;
            boxpart.y = w->points[0].y;
            numpts = w->numpts - 1;
            points = new Point[numpts];
            if (numpts) {
                for (int i = 0; i < numpts; i++) {
                    points[i].x = w->points[i+1].x - w->points[0].x;
                    points[i].y = w->points[i+1].y - w->points[0].y;
                }
            }
            pw = w->wire_width();
            style = w->wire_style();
        }

    void set()
        {
            boxpart.set();
            numpts = 0;
            points = 0;
            pw = 0;
            style = CDWIRE_FLUSH;
        }

     void cleanup()
        {
            boxpart.cleanup();
            delete [] points;
        }

    static unsigned int diff(const rf_wire*, const rf_wire*);

    bool operator==(const rf_wire&) const;
    bool operator<(const rf_wire&) const;
    unsigned int hash();

    rf_box boxpart;
    int numpts;
    Point *points;
    unsigned int pw;
    WireStyle style;
};

struct rf_label
{
    void set(const Text *text, unsigned int l, unsigned int d, CDp *p)
        {
            boxpart.set(l, d, p);
            boxpart.width = text->width;
            boxpart.height = text->height;
            boxpart.x = text->x;
            boxpart.y = text->y;
            label = lstring::copy(text->text);
            xform = text->xform;
        }

    void set()
        {
            boxpart.set();
            label = 0;
            xform = 0;
        }

    void cleanup()
        {
            boxpart.cleanup();
            delete [] label;
        }

    static unsigned int diff(const rf_label*, const rf_label*);

    bool operator==(const rf_label&) const;
    bool operator<(const rf_label&) const;
    unsigned int hash();

    rf_box boxpart;
    char *label;
    int xform;
};

struct rf_sref
{
    void set(const Instance *inst, const char *name, CDp *p)
        {
            cname = name;  // assert: name is known to be a stable copy
            properties = p;
            others = 0;
            tx.magn = inst->magn;
            tx.tx = inst->origin.x;
            tx.ty = inst->origin.y;
            tx.ax = inst->ax;
            tx.ay = inst->ay;
            tx.refly = inst->reflection;
        }

    void set()
        {
            cname = 0;
            properties = 0;
            others = 0;
        }

    void cleanup()
        {
            // cname is not deleted here
            properties->free_list();
        }

    static unsigned int diff(const rf_sref*, const rf_sref*);

    bool operator==(const rf_sref&) const;
    bool operator<(const rf_sref&) const;
    unsigned int hash();

    const char *cname;
    CDp *properties;
    coor_t *others;
    CDtx tx;
};


//-----------------------------------------------------------------------------
// The hash table for rf_box, etc.  This ensures uniqueness.

template<class T> struct rpitem_t
{
    T item;
    rpitem_t<T> *next;
};


template<class T>
struct rpdb_t
{
    rpdb_t()
        {
            array = new rpitem_t<T>*[1];
            array[0] = 0;
            count = 0;
            hashmask = 0;
        }

    ~rpdb_t()
        {
            for (unsigned int i = 0;  i <= this->hashmask; i++) {
                for (rpitem_t<T> *ee = this->array[i]; ee; ee = ee->next)
                    ee->item.cleanup();
            }
            delete [] array;
        }

    T *record(T*);
    T **extract_array();

    unsigned int allocated() { return (count); }

private:
    rpitem_t<T> **array;
    unsigned int count;
    unsigned int hashmask;
    eltab_t< rpitem_t<T> > eltab;
};

template<class T>
T *
rpdb_t<T>::record(T *t)
{
    unsigned int j = (t->hash() & this->hashmask);
    for (rpitem_t<T> *e = this->array[j]; e; e = e->next) {
        if (*t == e->item)
            return (&e->item);
    }
    rpitem_t<T> *e = this->eltab.new_element();
    e->item = *t;
    e->next = this->array[j];
    this->array[j] = e;
    this->count++;

    if (this->count/(this->hashmask+1) > ST_MAX_DENS) {

        unsigned int newmask = (this->hashmask << 1) | 1;
        rpitem_t<T> **tmp = new rpitem_t<T>*[newmask+1];
        for (unsigned int i = 0; i <= newmask; i++)
            tmp[i] = 0;
        for (unsigned int i = 0;  i <= this->hashmask; i++) {
            rpitem_t<T> *en;
            for (rpitem_t<T> *ee = this->array[i]; ee; ee = en) {
                en = ee->next;
                j = (ee->item.hash() & newmask);
                ee->next = tmp[j];
                tmp[j] = ee;
            }
            this->array[i] = 0;
        }
        this->hashmask = newmask;
        delete [] this->array;
        this->array = tmp;
    }
    return (&e->item);
}

template<class T>
T **
rpdb_t<T>::extract_array()
{
    if (!count)
        return (0);
    T **ary = new T*[count];

    unsigned int blsize = eltab.block_size();
    unsigned int nblks = (count-1)/blsize;

    for (typename eltab_t< rpitem_t<T> >::elbf_t *ebf = eltab.block_list();
            ebf; ebf = ebf->next) {
        unsigned int base = nblks * blsize;
        for (unsigned int i = 0; i < blsize; i++) {
            if (base + i < count)
                ary[i+base] = &ebf->elts[i].item;
            else
                break;
        }
        nblks--;
    }
    return (ary);
}


//-----------------------------------------------------------------------------
// The cache, instantiated in the OASIS writer.

struct oas_cache
{
    oas_cache(oas_out *out)
        {
            oc_out = out;
            oc_box_tab = 0;
            oc_poly_tab = 0;
            oc_wire_tab = 0;
            oc_label_tab = 0;
            oc_sref_tab = 0;
            oc_cname_tab = 0;

            oc_cache_max = REP_MAX_ITEMS;
            oc_rep_max = REP_MAX_REPS;
            oc_box_rep_cnt = 0;
            oc_poly_rep_cnt = 0;
            oc_wire_rep_cnt = 0;
            oc_label_rep_cnt = 0;
            oc_sref_rep_cnt = 0;

            oc_minrun = REP_RUN_MIN;
            oc_minarr = REP_ARRAY_MIN;
            oc_random_placement = false;
            oc_rep_debug = false;
            oc_gcd_check = false;
            oc_repetitions = 0;
        }

    ~oas_cache()
        {
            delete oc_box_tab;
            delete oc_poly_tab;
            delete oc_wire_tab;
            delete oc_label_tab;
            delete oc_sref_tab;
            delete oc_cname_tab;
        }

    void setup_repetition(const char*, bool);

    bool cache_box(const BBox*, unsigned int, unsigned int, CDp*);
    bool cache_poly(const Poly*, unsigned int, unsigned int, CDp*);
    bool cache_wire(const Wire*, unsigned int, unsigned int, CDp*);
    bool cache_label(const Text*, unsigned int, unsigned int, CDp*);
    bool cache_sref(const Instance*, CDp*);

    bool flush();
    bool flush_box();
    bool flush_poly();
    bool flush_wire();
    bool flush_label();
    bool flush_sref();

    bool check_flush_box()
        { return (oc_box_tab->allocated() >= oc_cache_max ||
            (oc_box_rep_cnt > oc_rep_max && oc_rep_max) ?
            flush_box() : true); }

    bool check_flush_poly()
        { return (oc_poly_tab->allocated() >= oc_cache_max ||
            (oc_poly_rep_cnt > oc_rep_max && oc_rep_max) ?
            flush_poly() : true); }

    bool check_flush_wire()
        { return (oc_wire_tab->allocated() >= oc_cache_max ||
            (oc_wire_rep_cnt > oc_rep_max && oc_rep_max) ?
            flush_wire() : true); }

    bool check_flush_label()
        { return (oc_label_tab->allocated() >= oc_cache_max ||
            (oc_label_rep_cnt > oc_rep_max && oc_rep_max) ?
            flush_label() : true); }

    bool check_flush_sref()
        { return (oc_sref_tab->allocated() >= oc_cache_max ||
            (oc_sref_rep_cnt > oc_rep_max && oc_rep_max) ?
            flush_sref() : true); }

    bool caching_boxes()    { return (oc_repetitions & OAS_CR_BOX); }
    bool caching_polys()    { return (oc_repetitions & OAS_CR_POLY); }
    bool caching_wires()    { return (oc_repetitions & OAS_CR_WIRE); }
    bool caching_labels()   { return (oc_repetitions & OAS_CR_LAB); }
    bool caching_srefs()    { return (oc_repetitions & OAS_CR_CELL); }

private:

    coor_t *new_pt(int x, int y, coor_t *n)
    {
        coor_t *p = oc_pt_factory.new_element();
        p->next = n;
        p->x = x;
        p->y = y;
        return (p);
    }

    bool write_box_cache(const rf_box*);
    bool write_poly_cache(const rf_poly*);
    bool write_wire_cache(const rf_wire*);
    bool write_label_cache(const rf_label*);
    bool write_sref_cache(const rf_sref*);
    bool write_cached_box(const rf_box*, int, int);
    bool write_cached_poly(const rf_poly*, int, int);
    bool write_cached_wire(const rf_wire*, int, int);
    bool write_cached_label(const rf_label*, int, int);
    bool write_cached_sref(const rf_sref*, int, int);

    void set_repetition(const coor_t*);
    void set_repetition(const oned_t*);
    void set_repetition(const twod_t*);
    void unset_repetition();

    oas_out             *oc_out;        // Pointer to output processor
    rpdb_t<rf_box>      *oc_box_tab;    // Boc table
    rpdb_t<rf_poly>     *oc_poly_tab;   // Polygon table
    rpdb_t<rf_wire>     *oc_wire_tab;   // Path table
    rpdb_t<rf_label>    *oc_label_tab;  // Text element table
    rpdb_t<rf_sref>     *oc_sref_tab;   // Placement table
    strtab_t            *oc_cname_tab;  // String table for placement names

    unsigned int        oc_cache_max;   // Flush threshold for objects
    unsigned int        oc_rep_max;     // Flush threshold for reps
    unsigned int        oc_box_rep_cnt;
    unsigned int        oc_poly_rep_cnt;
    unsigned int        oc_wire_rep_cnt;
    unsigned int        oc_label_rep_cnt;
    unsigned int        oc_sref_rep_cnt;

    unsigned int        oc_minrun;      // Minimum repetition run length
    unsigned int        oc_minarr;      // Minimum repetition array width
    bool                oc_random_placement; // No run/array extraction
    bool                oc_rep_debug;   // Debug repetition extraction
    bool                oc_gcd_check;   // Try to compact using gcd
    unsigned char       oc_repetitions; // Enabling flags
    eltab_t<coor_t>     oc_pt_factory;  // Local allocator
};

#endif

